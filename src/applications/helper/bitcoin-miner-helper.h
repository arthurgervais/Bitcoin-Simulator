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
 */

/**
 * This file contains declares the BitcoinMinerHelper class.
 */
 
#ifndef BITCOIN_MINER_HELPER_H
#define BITCOIN_MINER_HELPER_H

#include "ns3/bitcoin-node-helper.h"
#include "ns3/bitcoin-simple-attacker.h"
#include "ns3/bitcoin-selfish-miner.h"
#include "ns3/bitcoin-selfish-miner-trials.h"


namespace ns3 {

/**
 * Based on packet-sink-helper
 */
 
class BitcoinMinerHelper : public BitcoinNodeHelper
{
  public:
  /**
   * Create a PacketSinkHelper to make it easier to work with BitcoinMiner applications
   *
   * \param protocol the name of the protocol to use to receive traffic
   *        This string identifies the socket factory type used to create
   *        sockets for the applications.  A typical value would be 
   *        ns3::TcpSocketFactory.
   * \param address the address of the bitcoin node
   * \param noMiners total number of miners in the simulation
   * \param peers a reference to a vector containing the Ipv4 addresses of peers of the bitcoin node
   * \param peersDownloadSpeeds a map containing the download speeds of the peers of the node
   * \param peersUploadSpeeds a map containing the upload speeds of the peers of the node
   * \param internetSpeeds a reference to a struct containing the internet speeds of the node
   * \param stats a pointer to struct holding the node statistics
   * \param hashRate the hash rate of the miner
   * \param averageBlockGenIntervalSeconds the average block generation interval in seconds
   */
  BitcoinMinerHelper (std::string protocol, Address address, std::vector<Ipv4Address> peers, int noMiners,
                      std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address, double> &peersUploadSpeeds,
                      nodeInternetSpeeds &internetSpeeds, nodeStatistics *stats, double hashRate, double averageBlockGenIntervalSeconds);
					  
  enum MinerType GetMinerType(void);
  void SetMinerType (enum MinerType m);
  void SetBlockBroadcastType (enum BlockBroadcastType m);

protected:
  /**
   * Install an ns3::PacketSink on the node configured with all the
   * attributes set with SetAttribute.
   *
   * \param node The node on which an PacketSink will be installed.
   * \returns Ptr to the application installed.
   */
  virtual Ptr<Application> InstallPriv (Ptr<Node> node);
  
  /**
   * Customizes the factory object according to the arguments of the constructor
   */
  void SetFactoryAttributes (void);
  
  
  enum MinerType            m_minerType;
  enum BlockBroadcastType   m_blockBroadcastType;
  int                       m_noMiners;
  double                    m_hashRate;
  double                    m_blockGenBinSize;
  double                    m_blockGenParameter;
  double                    m_averageBlockGenIntervalSeconds;
  uint32_t                  m_secureBlocks;
};

} // namespace ns3

#endif /* BITCOIN_MINER_HELPER_H */
