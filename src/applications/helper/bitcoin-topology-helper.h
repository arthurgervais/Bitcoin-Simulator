/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Josh Pelkey <jpelkey@gatech.edu>
 */

#ifndef BITCOIN_TOPOLOGY_HELPER_H
#define BITCOIN_TOPOLOGY_HELPER_H

#include <vector>

#include "internet-stack-helper.h"
#include "point-to-point-helper.h"
#include "ipv4-address-helper.h"
#include "ipv6-address-helper.h"
#include "ipv4-interface-container.h"
#include "ipv6-interface-container.h"
#include "net-device-container.h"
#include "ipv4-address-helper-custom.h"
#include "ns3/bitcoin.h"
#include <random>

namespace ns3 {

/**
 * \ingroup point-to-point-layout
 *
 * \brief A helper to make it easier to create a grid topology
 * with p2p links
 */
class BitcoinTopologyHelper 
{
public: 
  /**
   * Create a BitcoinTopologyHelper in order to easily create
   * grid topologies using p2p links
   *
   * \param nRows total number of rows in the grid
   *
   * \param nCols total number of colums in the grid
   *
   * \param pointToPoint the PointToPointHelper which is used 
   *                     to connect all of the nodes together 
   *                     in the grid
   */
  BitcoinTopologyHelper (uint32_t noCpus, uint32_t totalNoNodes, uint32_t noMiners, enum BitcoinRegion *minersRegions,
                         enum Cryptocurrency cryptocurrency, int minConnectionsPerNode, int maxConnectionsPerNode, 
                         double latencyParetoShapeDivider, uint32_t systemId);

  ~BitcoinTopologyHelper ();

  /**
   * \param row the row address of the node desired
   *
   * \param col the column address of the node desired
   *
   * \returns a pointer to the node specified by the 
   *          (row, col) address
   */
  Ptr<Node> GetNode (uint32_t id);

  /**
   * This returns an Ipv4 address at the node specified by 
   * the (row, col) address.  Technically, a node will have 
   * multiple interfaces in the grid; therefore, it also has 
   * multiple Ipv4 addresses.  This method only returns one of 
   * the addresses. If you picture the grid, the address returned 
   * is the left row device of all the nodes, except the left-most 
   * grid nodes, which returns the right row device.
   *
   * \param row the row address of the node desired
   *
   * \param col the column address of the node desired
   *
   * \returns Ipv4Address of one of the interfaces of the node 
   *          specified by the (row, col) address
   */
  Ipv4Address GetIpv4Address (uint32_t row, uint32_t col);


  /**
   * \param stack an InternetStackHelper which is used to install 
   *              on every node in the grid
   */
  void InstallStack (InternetStackHelper stack);

  /**
   * Assigns Ipv4 addresses to all the row and column interfaces
   *
   * \param ip the Ipv4AddressHelper used to assign Ipv4 addresses 
   *              to all of the row interfaces in the grid
   *
   * \param ip the Ipv4AddressHelper used to assign Ipv4 addresses 
   *              to all of the row interfaces in the grid
   */
  void AssignIpv4Addresses (Ipv4AddressHelperCustom ip);


  /**
   * Sets up the node canvas locations for every node in the grid.
   * This is needed for use with the animation interface
   *
   * \param ulx upper left x value
   * \param uly upper left y value
   * \param lrx lower right x value
   * \param lry lower right y value
   */
  void BoundingBox (double ulx, double uly, double lrx, double lry);

  /**
   * Get the interface container
   */
   Ipv4InterfaceContainer GetIpv4InterfaceContainer (void) const;
   
   std::map<uint32_t, std::vector<Ipv4Address>> GetNodesConnectionsIps (void) const;
   
   std::vector<uint32_t> GetMiners (void) const;
   
   uint32_t* GetBitcoinNodesRegions (void);
   
   std::map<uint32_t, std::map<Ipv4Address, double>> GetPeersDownloadSpeeds(void) const;
   std::map<uint32_t, std::map<Ipv4Address, double>> GetPeersUploadSpeeds(void) const;

   std::map<uint32_t, nodeInternetSpeeds> GetNodesInternetSpeeds (void) const;

private:

  void AssignRegion (uint32_t id);
  void AssignInternetSpeeds(uint32_t id);
  
  uint32_t     m_totalNoNodes;                  //!< The total number of nodes
  uint32_t     m_noMiners;                      //!< The total number of miners
  uint32_t     m_noCpus;                        //!< The number of the available cpus in the simulation
  double       m_latencyParetoShapeDivider;     //!<  The pareto shape for the latency of the point-to-point links
  int          m_minConnectionsPerNode;         //!<  The minimum connections per node
  int          m_maxConnectionsPerNode;         //!<  The maximum connections per node
  int          m_minConnectionsPerMiner;        //!<  The minimum connections per node
  int          m_maxConnectionsPerMiner;        //!<  The maximum connections per node
  double       m_minerDownloadSpeed;            //!<  The download speed of miners
  double       m_minerUploadSpeed;              //!<  The upload speed of miners
  uint32_t     m_totalNoLinks;                  //!<  Total number of links
  uint32_t     m_systemId;
  
  enum BitcoinRegion                             *m_minersRegions;
  enum Cryptocurrency                             m_cryptocurrency;
  std::vector<uint32_t>                           m_miners;                  //!< The ids of the miners
  std::map<uint32_t, std::vector<uint32_t>>       m_nodesConnections;        //!< key = nodeId
  std::map<uint32_t, std::vector<Ipv4Address>>    m_nodesConnectionsIps;     //!< key = nodeId
  std::vector<NodeContainer>                      m_nodes;                   //!< all the nodes in the network
  std::vector<NetDeviceContainer>                 m_devices;                 //!< NetDevices in the network
  std::vector<Ipv4InterfaceContainer>             m_interfaces;              //!< IPv4 interfaces in the network
  uint32_t                                       *m_bitcoinNodesRegion;      //!< The region in which the bitcoin nodes are located
  double                                          m_regionLatencies[6][6];   //!< The inter- and intra-region latencies
  double                                          m_regionDownloadSpeeds[6];     
  double                                          m_regionUploadSpeeds[6];     
  

  std::map<uint32_t, std::map<Ipv4Address, double>>    m_peersDownloadSpeeds;     //!< key1 = nodeId, key2 = Ipv4Address of peer
  std::map<uint32_t, std::map<Ipv4Address, double>>    m_peersUploadSpeeds;       //!< key1 = nodeId, key2 = Ipv4Address of peer
  std::map<uint32_t, nodeInternetSpeeds>               m_nodesInternetSpeeds;     //!< key = nodeId
  std::map<uint32_t, int>                              m_minConnections;          //!< key = nodeId
  std::map<uint32_t, int>                              m_maxConnections;          //!< key = nodeId

  std::default_random_engine                     m_generator;
  std::piecewise_constant_distribution<double>   m_nodesDistribution;
  std::piecewise_constant_distribution<double>   m_connectionsDistribution;
  std::piecewise_constant_distribution<double>   m_europeDownloadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_europeUploadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_northAmericaDownloadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_northAmericaUploadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_asiaPacificDownloadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_asiaPacificUploadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_japanDownloadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_japanUploadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_southAmericaDownloadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_southAmericaUploadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_australiaDownloadBandwidthDistribution;
  std::piecewise_constant_distribution<double>   m_australiaUploadBandwidthDistribution;
};



} // namespace ns3

#endif /* BITCOIN_TOPOLOGY_HELPER_H */
