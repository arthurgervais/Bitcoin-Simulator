/**
 * This file contains the definitions of the functions declared in bitcoin-miner-helper.h
 */

#include "ns3/bitcoin-miner-helper.h"
#include "ns3/string.h"
#include "ns3/inet-socket-address.h"
#include "ns3/names.h"
#include "ns3/uinteger.h"
#include "ns3/bitcoin-miner.h"
#include "ns3/log.h"
#include "ns3/double.h"

namespace ns3 {

BitcoinMinerHelper::BitcoinMinerHelper (std::string protocol, Address address, std::vector<Ipv4Address> peers, int noMiners,
                                        std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address, double> &peersUploadSpeeds, 
                                        nodeInternetSpeeds &internetSpeeds, nodeStatistics *stats, double hashRate, double averageBlockGenIntervalSeconds) : 
                                        BitcoinNodeHelper (),  m_minerType (NORMAL_MINER), m_blockBroadcastType (STANDARD),
                                        m_secureBlocks (6), m_blockGenBinSize (-1), m_blockGenParameter (-1)
{
  m_factory.SetTypeId ("ns3::BitcoinMiner");
  commonConstructor(protocol, address, peers, peersDownloadSpeeds, peersUploadSpeeds, internetSpeeds, stats);
  
  m_noMiners = noMiners;
  m_hashRate = hashRate;
  m_averageBlockGenIntervalSeconds = averageBlockGenIntervalSeconds;
  
  m_factory.Set ("NumberOfMiners", UintegerValue(m_noMiners));
  m_factory.Set ("HashRate", DoubleValue(m_hashRate));
  m_factory.Set ("AverageBlockGenIntervalSeconds", DoubleValue(m_averageBlockGenIntervalSeconds));

}

Ptr<Application>
BitcoinMinerHelper::InstallPriv (Ptr<Node> node) //FIX ME
{

   switch (m_minerType) 
   {
      case NORMAL_MINER: 
      {
        Ptr<BitcoinMiner> app = m_factory.Create<BitcoinMiner> ();
        app->SetPeersAddresses(m_peersAddresses);
        app->SetPeersDownloadSpeeds(m_peersDownloadSpeeds);
        app->SetPeersUploadSpeeds(m_peersUploadSpeeds);
        app->SetNodeInternetSpeeds(m_internetSpeeds);
        app->SetNodeStats(m_nodeStats);
        app->SetBlockBroadcastType(m_blockBroadcastType);
        app->SetProtocolType(m_protocolType);

        node->AddApplication (app);
        return app;
      }
      case SIMPLE_ATTACKER: 
      {
        Ptr<BitcoinSimpleAttacker> app = m_factory.Create<BitcoinSimpleAttacker> ();
        app->SetPeersAddresses(m_peersAddresses);
        app->SetPeersDownloadSpeeds(m_peersDownloadSpeeds);
        app->SetPeersUploadSpeeds(m_peersUploadSpeeds);
        app->SetNodeInternetSpeeds(m_internetSpeeds);
        app->SetNodeStats(m_nodeStats);
        app->SetBlockBroadcastType(m_blockBroadcastType);
        app->SetProtocolType(m_protocolType);

        node->AddApplication (app);
        return app;
      }
      case SELFISH_MINER: 
      {
        Ptr<BitcoinSelfishMiner> app = m_factory.Create<BitcoinSelfishMiner> ();
        app->SetPeersAddresses(m_peersAddresses);
        app->SetPeersDownloadSpeeds(m_peersDownloadSpeeds);
        app->SetPeersUploadSpeeds(m_peersUploadSpeeds);
        app->SetNodeInternetSpeeds(m_internetSpeeds);
        app->SetNodeStats(m_nodeStats);
        app->SetBlockBroadcastType(m_blockBroadcastType);
        app->SetProtocolType(m_protocolType);

        node->AddApplication (app);
        return app;
      }
      case SELFISH_MINER_TRIALS: 
      {
        Ptr<BitcoinSelfishMinerTrials> app = m_factory.Create<BitcoinSelfishMinerTrials> ();
        app->SetPeersAddresses(m_peersAddresses);
        app->SetPeersDownloadSpeeds(m_peersDownloadSpeeds);
        app->SetPeersUploadSpeeds(m_peersUploadSpeeds);
        app->SetNodeInternetSpeeds(m_internetSpeeds);
        app->SetNodeStats(m_nodeStats);
        app->SetBlockBroadcastType(m_blockBroadcastType);
        app->SetProtocolType(m_protocolType);

        node->AddApplication (app);
        return app;
      }
   }
   
}

enum MinerType 
BitcoinMinerHelper::GetMinerType(void)
{
  return m_minerType;
}

void 
BitcoinMinerHelper::SetMinerType (enum MinerType m)  //FIX ME
{
   m_minerType = m;
  
   switch (m) 
   {
      case NORMAL_MINER: 
      {
        m_factory.SetTypeId ("ns3::BitcoinMiner");
        SetFactoryAttributes();
        break;
      }
      case SIMPLE_ATTACKER:  
      {
        m_factory.SetTypeId ("ns3::BitcoinSimpleAttacker");
        SetFactoryAttributes();
        m_factory.Set ("SecureBlocks", UintegerValue(m_secureBlocks));

        break;
      }
      case SELFISH_MINER:  
      {
        m_factory.SetTypeId ("ns3::BitcoinSelfishMiner");
        SetFactoryAttributes();

        break;
      }
      case SELFISH_MINER_TRIALS:  
      {
        m_factory.SetTypeId ("ns3::BitcoinSelfishMinerTrials");
        SetFactoryAttributes();
        m_factory.Set ("SecureBlocks", UintegerValue(m_secureBlocks));

        break;
      }
   }
}


void 
BitcoinMinerHelper::SetBlockBroadcastType (enum BlockBroadcastType m)
{
  m_blockBroadcastType = m;
}


void 
BitcoinMinerHelper::SetFactoryAttributes (void)
{
  m_factory.Set ("Protocol", StringValue (m_protocol));
  m_factory.Set ("Local", AddressValue (m_address));
  m_factory.Set ("NumberOfMiners", UintegerValue(m_noMiners));
  m_factory.Set ("HashRate", DoubleValue(m_hashRate));
  m_factory.Set ("AverageBlockGenIntervalSeconds", DoubleValue(m_averageBlockGenIntervalSeconds));
  
  if (m_blockGenBinSize > 0 && m_blockGenParameter)
  {
    m_factory.Set ("BlockGenBinSize", DoubleValue(m_blockGenBinSize));
    m_factory.Set ("BlockGenParameter", DoubleValue(m_blockGenParameter));
  }
}

} // namespace ns3
