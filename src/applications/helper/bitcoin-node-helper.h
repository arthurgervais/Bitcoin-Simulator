/**
 * This file contains declares the BitcoinNodeHelper class.
 */
 
#ifndef BITCOIN_NODE_HELPER_H
#define BITCOIN_NODE_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"
#include "ns3/uinteger.h"
#include "ns3/bitcoin.h"

namespace ns3 {

/**
 * Based on packet-sink-helper
 */
 
class BitcoinNodeHelper
{
public:
  /**
   * Create a BitcoinNodeHelper to make it easier to work with BitcoinNode applications
   *
   * \param protocol the name of the protocol to use to receive traffic
   *        This string identifies the socket factory type used to create
   *        sockets for the applications.  A typical value would be 
   *        ns3::TcpSocketFactory.
   * \param address the address of the bitcoin node
   * \param peers a reference to a vector containing the Ipv4 addresses of peers of the bitcoin node
   * \param peersDownloadSpeeds a map containing the download speeds of the peers of the node
   * \param peersUploadSpeeds a map containing the upload speeds of the peers of the node
   * \param internetSpeeds a reference to a struct containing the internet speeds of the node
   * \param stats a pointer to struct holding the node statistics
   */
  BitcoinNodeHelper (std::string protocol, Address address, std::vector<Ipv4Address> &peers, 
                     std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address, double> &peersUploadSpeeds,
                     nodeInternetSpeeds &internetSpeeds, nodeStatistics *stats);
  
  /**
   * Called by subclasses to set a different factory TypeId
   */
  BitcoinNodeHelper (void);
  
  /**
   * Common Constructor called both from the base class and the subclasses
   * \param protocol the name of the protocol to use to receive traffic
   *        This string identifies the socket factory type used to create
   *        sockets for the applications.  A typical value would be 
   *        ns3::TcpSocketFactory.
   * \param address the address of the bitcoin node
   * \param peers a reference to a vector containing the Ipv4 addresses of peers of the bitcoin node
   * \param peersDownloadSpeeds a map containing the download speeds of the peers of the node
   * \param peersUploadSpeeds a map containing the upload speeds of the peers of the node
   * \param internetSpeeds a reference to a struct containing the internet speeds of the node
   * \param stats a pointer to struct holding the node statistics
   */
   void commonConstructor(std::string protocol, Address address, std::vector<Ipv4Address> &peers, 
                          std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address, double> &peersUploadSpeeds,
                          nodeInternetSpeeds &internetSpeeds, nodeStatistics *stats);
  
  /**
   * Helper function used to set the underlying application attributes.
   *
   * \param name the name of the application attribute to set
   * \param value the value of the application attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Install an ns3::PacketSinkApplication on each node of the input container
   * configured with all the attributes set with SetAttribute.
   *
   * \param c NodeContainer of the set of nodes on which a PacketSinkApplication 
   * will be installed.
   * \returns Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (NodeContainer c);

  /**
   * Install an ns3::PacketSinkApplication on each node of the input container
   * configured with all the attributes set with SetAttribute.
   *
   * \param node The node on which a PacketSinkApplication will be installed.
   * \returns Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (Ptr<Node> node);

  /**
   * Install an ns3::PacketSinkApplication on each node of the input container
   * configured with all the attributes set with SetAttribute.
   *
   * \param nodeName The name of the node on which a PacketSinkApplication will be installed.
   * \returns Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (std::string nodeName);

  void SetPeersAddresses (std::vector<Ipv4Address> &peersAddresses);
  
  void SetPeersDownloadSpeeds (std::map<Ipv4Address, double> &peersDownloadSpeeds);
  void SetPeersUploadSpeeds (std::map<Ipv4Address, double> &peersUploadSpeeds);
  
  void SetNodeInternetSpeeds (nodeInternetSpeeds &internetSpeeds);

  void SetNodeStats (nodeStatistics *nodeStats);

  void SetProtocolType (enum ProtocolType protocolType);
  
protected:
  /**
   * Install an ns3::PacketSink on the node configured with all the
   * attributes set with SetAttribute.
   *
   * \param node The node on which an PacketSink will be installed.
   * \returns Ptr to the application installed.
   */
  virtual Ptr<Application> InstallPriv (Ptr<Node> node);
  
  ObjectFactory                                       m_factory;              //!< Object factory.
  std::string                                         m_protocol;             //!< The name of the protocol to use to receive traffic
  Address                                             m_address;              //!< The address of the bitcoin node
  std::vector<Ipv4Address>		                      m_peersAddresses;       //!< The addresses of peers
  std::map<Ipv4Address, double>                       m_peersDownloadSpeeds;  //!< The download speeds of the peers
  std::map<Ipv4Address, double>                       m_peersUploadSpeeds;    //!< The upload speeds of the peers
  nodeInternetSpeeds                                  m_internetSpeeds;       //!< The internet speeds of the node
  nodeStatistics                                      *m_nodeStats;           //!< The struct holding the node statistics
  enum ProtocolType									  m_protocolType;         //!< The protocol that the nodes use to advertise new blocks (DEFAULT: STANDARD)

};

} // namespace ns3

#endif /* BITCOIN_NODE_HELPER_H */
