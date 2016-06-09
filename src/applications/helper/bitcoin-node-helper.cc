/**
 * This file contains the definitions of the functions declared in bitcoin-node-helper.h
 */

#include "bitcoin-node-helper.h"
#include "ns3/string.h"
#include "ns3/inet-socket-address.h"
#include "ns3/names.h"
#include "../model/bitcoin-node.h"

namespace ns3 {

BitcoinNodeHelper::BitcoinNodeHelper (std::string protocol, Address address, std::vector<Ipv4Address> &peers, 
                                      std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address, double> &peersUploadSpeeds,
                                      nodeInternetSpeeds &internetSpeeds, nodeStatistics *stats) 
{
  m_factory.SetTypeId ("ns3::BitcoinNode");
  commonConstructor (protocol, address, peers, peersDownloadSpeeds, peersUploadSpeeds, internetSpeeds, stats);
}

BitcoinNodeHelper::BitcoinNodeHelper (void)
{
}

void 
BitcoinNodeHelper::commonConstructor(std::string protocol, Address address, std::vector<Ipv4Address> &peers, 
                                     std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address, double> &peersUploadSpeeds,
                                     nodeInternetSpeeds &internetSpeeds, nodeStatistics *stats) 
{
  m_protocol = protocol;
  m_address = address;
  m_peersAddresses = peers;
  m_peersDownloadSpeeds = peersDownloadSpeeds;
  m_peersUploadSpeeds = peersUploadSpeeds;
  m_internetSpeeds = internetSpeeds;
  m_nodeStats = stats;
  m_protocolType = STANDARD_PROTOCOL;
  
  m_factory.Set ("Protocol", StringValue (m_protocol));
  m_factory.Set ("Local", AddressValue (m_address));

}

void 
BitcoinNodeHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
BitcoinNodeHelper::Install (Ptr<Node> node)
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
BitcoinNodeHelper::Install (std::string nodeName)
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
BitcoinNodeHelper::Install (NodeContainer c)
{ 

  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
  {
    apps.Add (InstallPriv (*i));
  }

  return apps;
}

Ptr<Application>
BitcoinNodeHelper::InstallPriv (Ptr<Node> node)
{
  Ptr<BitcoinNode> app = m_factory.Create<BitcoinNode> ();
  app->SetPeersAddresses(m_peersAddresses);
  app->SetPeersDownloadSpeeds(m_peersDownloadSpeeds);
  app->SetPeersUploadSpeeds(m_peersUploadSpeeds);
  app->SetNodeInternetSpeeds(m_internetSpeeds);
  app->SetNodeStats(m_nodeStats);
  app->SetProtocolType(m_protocolType);

  node->AddApplication (app);

  return app;
}

void 
BitcoinNodeHelper::SetPeersAddresses (std::vector<Ipv4Address> &peersAddresses)
{
  m_peersAddresses = peersAddresses;	
}

void 
BitcoinNodeHelper::SetPeersDownloadSpeeds (std::map<Ipv4Address, double> &peersDownloadSpeeds)
{
  m_peersDownloadSpeeds = peersDownloadSpeeds;
}

void 
BitcoinNodeHelper::SetPeersUploadSpeeds (std::map<Ipv4Address, double> &peersUploadSpeeds)
{
  m_peersUploadSpeeds = peersUploadSpeeds;
}


void 
BitcoinNodeHelper::SetNodeInternetSpeeds (nodeInternetSpeeds &internetSpeeds)
{
  m_internetSpeeds = internetSpeeds;	
}

void 
BitcoinNodeHelper::SetNodeStats (nodeStatistics *nodeStats)
{
  m_nodeStats = nodeStats;
}

void 
BitcoinNodeHelper::SetProtocolType (enum ProtocolType protocolType)
{
  m_protocolType = protocolType;
}
} // namespace ns3
