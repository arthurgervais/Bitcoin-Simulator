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

#include <fstream>
#include <time.h>
#include <sys/time.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/mpi-interface.h"

#ifdef NS3_MPI
#include <mpi.h>
#endif

using namespace ns3;

double get_wall_time();
int GetNodeIdByIpv4 (Ipv4InterfaceContainer container, Ipv4Address addr);
void PrintStatsForEachNode (nodeStatistics *stats, int totalNodes);
void PrintTotalStats (nodeStatistics *stats, int totalNodes, double start, double finish, double averageBlockGenIntervalMinutes);
void PrintBitcoinRegionStats (uint32_t *bitcoinNodesRegions, uint32_t totalNodes);
void PrintAttackStats (nodeStatistics *stats, int attackerId, double ud, double r);

NS_LOG_COMPONENT_DEFINE ("SelfishMinerTest");

int 
main (int argc, char *argv[])
{
#ifdef NS3_MPI
  bool nullmsg = false;
  bool unsolicited = false;
  bool relayNetwork = false;
  bool unsolicitedRelayNetwork = false;
  double ud = 0;
  double r = 0;
  enum Cryptocurrency  cryptocurrency = BITCOIN;
  double tStart = get_wall_time(), tFinish, tSimStart, tSimFinish;
  const int secsPerMin = 60;
  const uint16_t bitcoinPort = 8333;
  const double realAverageBlockGenIntervalMinutes = 10; //minutes
  int targetNumberOfBlocks = 1000;
  double averageBlockGenIntervalSeconds = 10 * secsPerMin; //seconds
  double fixedHashRate = 0.5;
  int start = 0;
  double bandwidth = 8;
  double latency = 40;
  bool test = false;
  
  
  double minersHash[] = {0.185, 0.159, 0.133, 0.066, 0.054,
                                0.029, 0.016, 0.012, 0.012, 0.012, 0.009,
                                0.005, 0.005, 0.002, 0.002, 0.3};
  enum BitcoinRegion minersRegions[] = {ASIA_PACIFIC, ASIA_PACIFIC, NORTH_AMERICA, ASIA_PACIFIC, NORTH_AMERICA,
                                               EUROPE, EUROPE, NORTH_AMERICA, NORTH_AMERICA, NORTH_AMERICA, EUROPE,
                                               NORTH_AMERICA, NORTH_AMERICA, NORTH_AMERICA, NORTH_AMERICA, ASIA_PACIFIC};
/*   double minersHash[] = {0.4, 0.4, 0.3};
  enum BitcoinRegion minersRegions[] = {ASIA_PACIFIC, ASIA_PACIFIC, ASIA_PACIFIC}; */

  int totalNoNodes = sizeof(minersHash)/sizeof(double);
  int noMiners = totalNoNodes;
  int attackerId = totalNoNodes - 1;
  int minConnectionsPerNode = 1;
  int maxConnectionsPerNode = 1;
  
  int iterations = 1;
  int successfullAttacks = 0;
  int secureBlocks = 6;
  
  double averageBlockGenIntervalMinutes = averageBlockGenIntervalSeconds/secsPerMin;
  double stop;
  long blockSize = 450000*averageBlockGenIntervalMinutes/realAverageBlockGenIntervalMinutes;
  int  nodesInSystemId0 = 0;
  
  
  nodeStatistics *stats = new nodeStatistics[totalNoNodes];

  srand (1000);
  Time::SetResolution (Time::NS);
  

  uint32_t systemId = 0;
  uint32_t systemCount = 1;
  
  //LogComponentEnable("BitcoinNode", LOG_LEVEL_INFO);
  //LogComponentEnable("BitcoinMiner", LOG_LEVEL_INFO);
  //LogComponentEnable("BitcoinSelfishMiner", LOG_LEVEL_INFO);
  
  //LogComponentEnable("ObjectFactory", LOG_LEVEL_FUNCTION);
  //LogComponentEnable("Ipv4AddressGenerator", LOG_LEVEL_INFO);
  //LogComponentEnable("OnOffApplication", LOG_LEVEL_DEBUG);
  //LogComponentEnable("OnOffApplication", LOG_LEVEL_WARN);

  if (noMiners > totalNoNodes)
  {
    std::cout << "The number of miners is larger than the total number of nodes\n";
    return 0;
  }
  
  if (noMiners < 1)
  {
    std::cout << "You need at least one miner\n";
    return 0;
  }
  
  if (sizeof(minersHash)/sizeof(double) != noMiners)
  {
    std::cout << "The minersHash entries does not match the number of miners\n";
    return 0;
  }
  
  CommandLine cmd;
  cmd.AddValue ("blockIntervalMinutes", "The average block generation interval in minutes", averageBlockGenIntervalMinutes);
  cmd.AddValue ("noBlocks", "The number of generated blocks", targetNumberOfBlocks);
  cmd.AddValue ("iterations", "The number of iterations of the attack", iterations);
  cmd.AddValue ("test", "Test the attack", test);
  cmd.AddValue ("ud", "The transaction value which is double-spent", ud);
  cmd.AddValue ("r", "The stale block rate", r);
  cmd.AddValue ("unsolicited", "Change the miners block broadcast type to UNSOLICITED", unsolicited);
  cmd.AddValue ("relayNetwork", "Change the miners block broadcast type to RELAY_NETWORK", relayNetwork);
  cmd.AddValue ("unsolicitedRelayNetwork", "Change the miners block broadcast type to UNSOLICITED_RELAY_NETWORK", unsolicitedRelayNetwork);
  
  cmd.Parse(argc, argv);
  
  averageBlockGenIntervalSeconds = averageBlockGenIntervalMinutes * secsPerMin;
  stop = targetNumberOfBlocks * averageBlockGenIntervalMinutes; //seconds
  
  for (int iter = 0; iter < iterations; iter++)
  { 
    std::cout << "Iteration : " << iter + 1 << " " << secureBlocks << " " << averageBlockGenIntervalSeconds 
	          << " " << averageBlockGenIntervalMinutes << " " << targetNumberOfBlocks << "\n";
    Ipv4InterfaceContainer                               ipv4InterfaceContainer;
    std::map<uint32_t, std::vector<Ipv4Address>>         nodesConnections;
    std::map<uint32_t, std::map<Ipv4Address, double>>    peersDownloadSpeeds;
    std::map<uint32_t, std::map<Ipv4Address, double>>    peersUploadSpeeds;
    std::map<uint32_t, nodeInternetSpeeds>               nodesInternetSpeeds;
    std::vector<uint32_t>                                miners;
  
	
    BitcoinTopologyHelper bitcoinTopologyHelper (systemCount, totalNoNodes, noMiners, minersRegions,
                                                 cryptocurrency, minConnectionsPerNode, 
                                                 maxConnectionsPerNode, 2, systemId);

    // Install stack on Grid
    InternetStackHelper stack;
    bitcoinTopologyHelper.InstallStack (stack);

    // Assign Addresses to Grid
    bitcoinTopologyHelper.AssignIpv4Addresses (Ipv4AddressHelperCustom ("1.0.0.0", "255.255.255.0", false));
    ipv4InterfaceContainer = bitcoinTopologyHelper.GetIpv4InterfaceContainer();
    nodesConnections = bitcoinTopologyHelper.GetNodesConnectionsIps();
    miners = bitcoinTopologyHelper.GetMiners();
    peersDownloadSpeeds = bitcoinTopologyHelper.GetPeersDownloadSpeeds();
    peersUploadSpeeds = bitcoinTopologyHelper.GetPeersUploadSpeeds();
    nodesInternetSpeeds = bitcoinTopologyHelper.GetNodesInternetSpeeds();
    if (systemId == 0)
      PrintBitcoinRegionStats(bitcoinTopologyHelper.GetBitcoinNodesRegions(), totalNoNodes);


    //Install miners
    BitcoinMinerHelper bitcoinMinerHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), bitcoinPort),
                                            nodesConnections[miners[0]], noMiners, peersDownloadSpeeds[0], peersUploadSpeeds[0], nodesInternetSpeeds[0], 
										    stats, minersHash[0], averageBlockGenIntervalSeconds);
    ApplicationContainer bitcoinMiners;
    int count = 0;
	
    
    for(auto &miner : miners)
    {
	  Ptr<Node> targetNode = bitcoinTopologyHelper.GetNode (miner);
	
	  if (systemId == targetNode->GetSystemId())
	  {
		  
	    if (attackerId == miner)
        {
          bitcoinMinerHelper.SetMinerType (SELFISH_MINER);
        }
		
        bitcoinMinerHelper.SetAttribute("HashRate", DoubleValue(minersHash[count]));
	    bitcoinMinerHelper.SetPeersAddresses (nodesConnections[miner]);
	    bitcoinMinerHelper.SetNodeStats (&stats[miner]);
	    bitcoinMinerHelper.SetPeersDownloadSpeeds (peersDownloadSpeeds[miner]);
	    bitcoinMinerHelper.SetPeersUploadSpeeds (peersUploadSpeeds[miner]);

        if (test == true && attackerId != miner)
          bitcoinMinerHelper.SetAttribute("FixedBlockIntervalGeneration", DoubleValue(100));
        else if (test == true && attackerId == miner)
          bitcoinMinerHelper.SetAttribute("FixedBlockIntervalGeneration", DoubleValue(500));
	  
	    if(unsolicited)
	      bitcoinMinerHelper.SetBlockBroadcastType (UNSOLICITED);
	    if(relayNetwork)
	      bitcoinMinerHelper.SetBlockBroadcastType (RELAY_NETWORK);
	    if(unsolicitedRelayNetwork)
	      bitcoinMinerHelper.SetBlockBroadcastType (UNSOLICITED_RELAY_NETWORK);
	  
	    bitcoinMiners.Add(bitcoinMinerHelper.Install (targetNode));
/*         std::cout << "SystemId " << systemId << ": Miner " << miner.first << " with hash power = " << minersHash[count] 
	              << " and systemId = " << targetNode->GetSystemId() << " was installed in node (" 
                  << miner.first / ySize << ", " << miner.first % ySize << ")" << std::endl;  */
	  
	    if (systemId == 0)
          nodesInSystemId0++;
	  }				
	  count++;
   
    }
    bitcoinMiners.Start (Seconds (start));
    bitcoinMiners.Stop (Minutes (stop));

  
    // Set up the actual simulation
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  
    Simulator::Stop (Minutes (stop + 0.1));
	
    tSimStart = get_wall_time();
    Simulator::Run ();
    Simulator::Destroy ();
    tSimFinish = get_wall_time();


    std::cout << "Iteration " << iter+1 << " lasted " << tSimFinish - tSimStart << "s\n";
    std::cout << std::endl;



  
    if (systemId == 0)
    {
      tFinish = get_wall_time();
	
      PrintAttackStats(stats, attackerId, ud, r);
      //PrintStatsForEachNode(stats, totalNoNodes);
      //PrintTotalStats(stats, totalNoNodes);
      std::cout << "\nThe simulation ran for " << tFinish - tStart << "s simulating "
                << stop << "mins.\nIt consisted of " << totalNoNodes
                << " nodes (" << noMiners << " miners) with minConnectionsPerNode = "
                << minConnectionsPerNode << " and maxConnectionsPerNode = " << maxConnectionsPerNode << ".\n"
                << "\nThe number of secure blocks required was " << secureBlocks << ".\n"
                << "The averageBlockGenIntervalMinutes was " << averageBlockGenIntervalMinutes 
                << "min and averageBlockGenIntervalSeconds was " << averageBlockGenIntervalSeconds << ".\n"
                << "Each attack had a duration of " << targetNumberOfBlocks << " generated blocks.\n"
                << "The attacker's hash rate was " << minersHash[attackerId] << ".\n"
                << "The number of iterations was " << iterations << ".\n\n";
    }
  }  
  
  delete[] stats;  

  return 0;
  
#else
  NS_FATAL_ERROR ("Can't use distributed simulator without MPI compiled in");
#endif
}

double get_wall_time()
{
    struct timeval time;
    if (gettimeofday(&time,NULL)){
        //  Handle error
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

int GetNodeIdByIpv4 (Ipv4InterfaceContainer container, Ipv4Address addr)
{
  for (auto it = container.Begin(); it != container.End(); it++)
  {
	int32_t interface = it->first->GetInterfaceForAddress (addr);
	if ( interface != -1)
      return it->first->GetNetDevice (interface)-> GetNode()->GetId();
  }	  
  return -1; //if not found
}

void PrintStatsForEachNode (nodeStatistics *stats, int totalNodes)
{
  int secPerMin = 60;
  
  for (int it = 0; it < totalNodes; it++ )
  {
    std::cout << "\nNode " << stats[it].nodeId << " statistics:\n";
    std::cout << "Connections = " << stats[it].connections << "\n";
    std::cout << "Mean Block Receive Time = " << stats[it].meanBlockReceiveTime << " or " 
              << static_cast<int>(stats[it].meanBlockReceiveTime) / secPerMin << "min and " 
			  << stats[it].meanBlockReceiveTime - static_cast<int>(stats[it].meanBlockReceiveTime) / secPerMin * secPerMin << "s\n";
    std::cout << "Mean Block Propagation Time = " << stats[it].meanBlockPropagationTime << "s\n";
    std::cout << "Mean Block Size = " << stats[it].meanBlockSize << " Bytes\n";
    std::cout << "Total Blocks = " << stats[it].totalBlocks << "\n";
    std::cout << "Stale Blocks = " << stats[it].staleBlocks << " (" 
              << 100. * stats[it].staleBlocks / stats[it].totalBlocks << "%)\n";
    std::cout << "The size of the longest fork was " << stats[it].longestFork << " blocks\n";
    std::cout << "There were in total " << stats[it].blocksInForks << " blocks in forks\n";
    std::cout << "The total received INV messages were " << stats[it].invReceivedBytes << " Bytes\n";
    std::cout << "The total received GET_HEADERS messages were " << stats[it].getHeadersReceivedBytes << " Bytes\n";
    std::cout << "The total received HEADERS messages were " << stats[it].headersReceivedBytes << " Bytes\n";
    std::cout << "The total received GET_DATA messages were " << stats[it].getDataReceivedBytes << " Bytes\n";
    std::cout << "The total received BLOCK messages were " << stats[it].blockReceivedBytes << " Bytes\n";
    std::cout << "The total sent INV messages were " << stats[it].invSentBytes << " Bytes\n";
    std::cout << "The total sent GET_HEADERS messages were " << stats[it].getHeadersSentBytes << " Bytes\n";
    std::cout << "The total sent HEADERS messages were " << stats[it].headersSentBytes << " Bytes\n";
    std::cout << "The total sent GET_DATA messages were " << stats[it].getDataSentBytes << " Bytes\n";
    std::cout << "The total sent BLOCK messages were " << stats[it].blockSentBytes << " Bytes\n";


    if ( stats[it].miner == 1)
    {
      std::cout << "The miner " << stats[it].nodeId << " with hash rate = " << stats[it].hashRate*100 << "% generated " << stats[it].minerGeneratedBlocks 
                << " blocks "<< "(" << 100. * stats[it].minerGeneratedBlocks / (stats[it].totalBlocks - 1)
                << "%) with average block generation time = " << stats[it].minerAverageBlockGenInterval
                << "s or " << static_cast<int>(stats[it].minerAverageBlockGenInterval) / secPerMin << "min and " 
                << stats[it].minerAverageBlockGenInterval - static_cast<int>(stats[it].minerAverageBlockGenInterval) / secPerMin * secPerMin << "s"
                << " and average size " << stats[it].minerAverageBlockSize << " Bytes\n";
    }
  }
}


void PrintAttackStats (nodeStatistics *stats, int attackerId, double ud, double r)
{
  int secPerMin = 60;

  std::cout << "\nNode " << stats[attackerId].nodeId << " statistics:\n";
  std::cout << "Connections = " << stats[attackerId].connections << "\n";
  std::cout << "Mean Block Receive Time = " << stats[attackerId].meanBlockReceiveTime << " or " 
            << static_cast<int>(stats[attackerId].meanBlockReceiveTime) / secPerMin << "min and " 
			<< stats[attackerId].meanBlockReceiveTime - static_cast<int>(stats[attackerId].meanBlockReceiveTime) / secPerMin * secPerMin << "s\n";
  std::cout << "Mean Block Propagation Time = " << stats[attackerId].meanBlockPropagationTime << "s\n";
  std::cout << "Mean Block Size = " << stats[attackerId].meanBlockSize << " Bytes\n";
  std::cout << "Total Blocks = " << stats[attackerId].totalBlocks << "\n";
  std::cout << "Stale Blocks = " << stats[attackerId].staleBlocks << " (" 
            << 100. * stats[attackerId].staleBlocks / stats[attackerId].totalBlocks << "%)\n";
  std::cout << "The size of the longest fork was " << stats[attackerId].longestFork << " blocks\n";
  std::cout << "There were in total " << stats[attackerId].blocksInForks << " blocks in forks\n";
  std::cout << "There were " << stats[attackerId].attackSuccess << " successful double-spending attacks.\n";



  if (stats[attackerId].miner == 1)
  {
    std::cout << "The miner " << stats[attackerId].nodeId << " with hash rate = " << stats[attackerId].hashRate*100 << "% generated " << stats[attackerId].minerGeneratedBlocks 
              << " blocks "<< "(" << 100. * stats[attackerId].minerGeneratedBlocks / (stats[attackerId].totalBlocks - 1)
              << "%) with average block generation time = " << stats[attackerId].minerAverageBlockGenInterval
              << "s or " << static_cast<int>(stats[attackerId].minerAverageBlockGenInterval) / secPerMin << "min and " 
              << stats[attackerId].minerAverageBlockGenInterval - static_cast<int>(stats[attackerId].minerAverageBlockGenInterval) / secPerMin * secPerMin << "s"
              << " and average size " << stats[attackerId].minerAverageBlockSize << " Bytes\n";
  }
  
  double increase = (stats[attackerId].attackSuccess * ud + stats[attackerId].minedBlocksInMainChain) /
                    (stats[attackerId].minerGeneratedBlocks * (1-r));
  std::cout << "Total Blocks = " << stats[attackerId].totalBlocks << "\n";
  std::cout << "Mined Blocks in main blockchain = " << stats[attackerId].minedBlocksInMainChain << "\n";
  std::cout << "Honest Mining Income = " << stats[attackerId].minerGeneratedBlocks * (1-r) << "\n";
  std::cout << "Attacker Income = " << stats[attackerId].attackSuccess * ud + stats[attackerId].minedBlocksInMainChain << "(";
  
  if (increase >= 1)
   std::cout << "+" << (increase - 1) * 100 << "%)\n";
  else
   std::cout << "-" << (1 - increase) * 100 << "%)\n";

  std::cout << std::endl;

}


void PrintTotalStats (nodeStatistics *stats, int totalNodes, double start, double finish, double averageBlockGenIntervalMinutes)
{
  const int  secPerMin = 60;
  double     meanBlockReceiveTime = 0;
  double     meanBlockPropagationTime = 0;
  double     meanMinersBlockPropagationTime = 0;
  double     meanBlockSize = 0;
  int        totalBlocks = 0;
  int        staleBlocks = 0;
  double     invReceivedBytes = 0;
  double     invSentBytes = 0;
  double     getHeadersReceivedBytes = 0;
  double     getHeadersSentBytes = 0;
  double     headersReceivedBytes = 0;
  double     headersSentBytes = 0;
  double     getDataReceivedBytes = 0;
  double     getDataSentBytes = 0;
  double     blockReceivedBytes = 0;
  double     blockSentBytes = 0;
  double     longestFork = 0;
  double     blocksInForks = 0;
  double     averageBandwidthPerNode = 0;
  double     connectionsPerNode = 0;
  double     connectionsPerMiner = 0;
  uint32_t   nodes = 0;
  uint32_t   miners = 0;
  std::vector<double>    propagationTimes;
  std::vector<double>    minersPropagationTimes;

  for (int it = 0; it < totalNodes; it++ )
  {
    meanBlockReceiveTime = meanBlockReceiveTime*totalBlocks/(totalBlocks + stats[it].totalBlocks)
                         + stats[it].meanBlockReceiveTime*stats[it].totalBlocks/(totalBlocks + stats[it].totalBlocks);
    meanBlockPropagationTime = meanBlockPropagationTime*totalBlocks/(totalBlocks + stats[it].totalBlocks)
                         + stats[it].meanBlockPropagationTime*stats[it].totalBlocks/(totalBlocks + stats[it].totalBlocks);
    meanBlockSize = meanBlockSize*totalBlocks/(totalBlocks + stats[it].totalBlocks)
                  + stats[it].meanBlockSize*stats[it].totalBlocks/(totalBlocks + stats[it].totalBlocks);
    totalBlocks += stats[it].totalBlocks;
    staleBlocks += stats[it].staleBlocks;
    invReceivedBytes = invReceivedBytes*it/static_cast<double>(it + 1) + stats[it].invReceivedBytes/static_cast<double>(it + 1);
    invSentBytes = invSentBytes*it/static_cast<double>(it + 1) + stats[it].invSentBytes/static_cast<double>(it + 1);
    getHeadersReceivedBytes = getHeadersReceivedBytes*it/static_cast<double>(it + 1) + stats[it].getHeadersReceivedBytes/static_cast<double>(it + 1);
    getHeadersSentBytes = getHeadersSentBytes*it/static_cast<double>(it + 1) + stats[it].getHeadersSentBytes/static_cast<double>(it + 1);
    headersReceivedBytes = headersReceivedBytes*it/static_cast<double>(it + 1) + stats[it].headersReceivedBytes/static_cast<double>(it + 1);
    headersSentBytes = headersSentBytes*it/static_cast<double>(it + 1) + stats[it].headersSentBytes/static_cast<double>(it + 1);
    getDataReceivedBytes = getDataReceivedBytes*it/static_cast<double>(it + 1) + stats[it].getDataReceivedBytes/static_cast<double>(it + 1);
    getDataSentBytes = getDataSentBytes*it/static_cast<double>(it + 1) + stats[it].getDataSentBytes/static_cast<double>(it + 1);
    blockReceivedBytes = blockReceivedBytes*it/static_cast<double>(it + 1) + stats[it].blockReceivedBytes/static_cast<double>(it + 1);
    blockSentBytes = blockSentBytes*it/static_cast<double>(it + 1) + stats[it].blockSentBytes/static_cast<double>(it + 1);
    longestFork = longestFork*it/static_cast<double>(it + 1) + stats[it].longestFork/static_cast<double>(it + 1);
    blocksInForks = blocksInForks*it/static_cast<double>(it + 1) + stats[it].blocksInForks/static_cast<double>(it + 1);
	
	propagationTimes.push_back(stats[it].meanBlockPropagationTime);
	
	if(stats[it].miner == 0)
    {
      connectionsPerNode = connectionsPerNode*nodes/static_cast<double>(nodes + 1) + stats[it].connections/static_cast<double>(nodes + 1);
      nodes++;
    }
    else
    {
      connectionsPerMiner = connectionsPerMiner*miners/static_cast<double>(miners + 1) + stats[it].connections/static_cast<double>(miners + 1);
      meanMinersBlockPropagationTime = meanMinersBlockPropagationTime*miners/static_cast<double>(miners + 1) + stats[it].meanBlockPropagationTime/static_cast<double>(miners + 1);
      minersPropagationTimes.push_back(stats[it].meanBlockPropagationTime);
      miners++;
    }
  }
  
  averageBandwidthPerNode = invReceivedBytes + invSentBytes + getHeadersReceivedBytes + getHeadersSentBytes + headersReceivedBytes
                          + headersSentBytes + getDataReceivedBytes + getDataSentBytes + blockReceivedBytes + blockSentBytes;
				   
  totalBlocks /= static_cast<double>(totalNodes);
  staleBlocks /= static_cast<double>(totalNodes);
  sort(propagationTimes.begin(), propagationTimes.end());
  double median = *(propagationTimes.begin()+propagationTimes.size()/2);
  double p_25 = *(propagationTimes.begin()+int(propagationTimes.size()*.25));
  double p_75 = *(propagationTimes.begin()+int(propagationTimes.size()*.75));
  double p_90 = *(propagationTimes.begin()+int(propagationTimes.size()*.90));
  double minersMedian = *(minersPropagationTimes.begin()+int(propagationTimes.size()/2));
  
  std::cout << "\nTotal Stats:\n";
  std::cout << "Average Connections/node = " << connectionsPerNode << "\n";
  std::cout << "Average Connections/miner = " << connectionsPerMiner << "\n";
  std::cout << "Mean Block Receive Time = " << meanBlockReceiveTime << " or " 
            << static_cast<int>(meanBlockReceiveTime) / secPerMin << "min and " 
	        << meanBlockReceiveTime - static_cast<int>(meanBlockReceiveTime) / secPerMin * secPerMin << "s\n";
  std::cout << "Mean Block Propagation Time = " << meanBlockPropagationTime << "s\n";
  std::cout << "Median Block Propagation Time = " << median << "s\n";
  std::cout << "25% percentile of Block Propagation Time = " << p_25 << "s\n";
  std::cout << "75% percentile of Block Propagation Time = " << p_75 << "s\n";
  std::cout << "90% percentile of Block Propagation Time = " << p_90 << "s\n";
  std::cout << "Miners Mean Block Propagation Time = " << meanMinersBlockPropagationTime << "s\n";
  std::cout << "Miners Median Block Propagation Time = " << minersMedian << "s\n";
  std::cout << "Mean Block Size = " << meanBlockSize << " Bytes\n";
  std::cout << "Total Blocks = " << totalBlocks << "\n";
  std::cout << "Stale Blocks = " << staleBlocks << " (" 
            << 100. * staleBlocks / totalBlocks << "%)\n";
  std::cout << "The size of the longest fork was " << longestFork << " blocks\n";
  std::cout << "There were in total " << blocksInForks << " blocks in forks\n";
  std::cout << "The average received INV messages were " << invReceivedBytes << " Bytes (" 
            << 100. * invReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received GET_HEADERS messages were " << getHeadersReceivedBytes << " Bytes (" 
            << 100. * getHeadersReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received HEADERS messages were " << headersReceivedBytes << " Bytes (" 
            << 100. * headersReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received GET_DATA messages were " << getDataReceivedBytes << " Bytes (" 
            << 100. * getDataReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received BLOCK messages were " << blockReceivedBytes << " Bytes (" 
            << 100. * blockReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent INV messages were " << invSentBytes << " Bytes (" 
            << 100. * invSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent GET_HEADERS messages were " << getHeadersSentBytes << " Bytes (" 
            << 100. * getHeadersSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent HEADERS messages were " << headersSentBytes << " Bytes (" 
            << 100. * headersSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent GET_DATA messages were " << getDataSentBytes << " Bytes (" 
            << 100. * getDataSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent BLOCK messages were " << blockSentBytes << " Bytes (" 
            << 100. * blockSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to INV messages = " << invReceivedBytes +  invSentBytes << " Bytes(" 
            << 100. * (invReceivedBytes +  invSentBytes) / averageBandwidthPerNode << "%)\n";	
  std::cout << "Total average traffic due to GET_HEADERS messages = " << getHeadersReceivedBytes +  getHeadersSentBytes << " Bytes(" 
            << 100. * (getHeadersReceivedBytes +  getHeadersSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to HEADERS messages = " << headersReceivedBytes +  headersSentBytes << " Bytes(" 
            << 100. * (headersReceivedBytes +  headersSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to GET_DATA messages = " << getDataReceivedBytes +  getDataSentBytes << " Bytes(" 
            << 100. * (getDataReceivedBytes +  getDataSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to BLOCK messages = " << blockReceivedBytes +  blockSentBytes << " Bytes(" 
            << 100. * (blockReceivedBytes +  blockSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic/node = " << averageBandwidthPerNode << " Bytes (" 
            << averageBandwidthPerNode / (1000 *(totalBlocks - 1) * averageBlockGenIntervalMinutes * secPerMin) 
            << " Kbps and " << averageBandwidthPerNode / (1000 * (totalBlocks - 1)) << " KB/block)\n";
  std::cout << (finish - start)/ (totalBlocks - 1)<< "s per generated block\n";
}

void PrintBitcoinRegionStats (uint32_t *bitcoinNodesRegions, uint32_t totalNodes)
{
  uint32_t regions[7] = {0, 0, 0, 0, 0, 0, 0};
  
  for (uint32_t i = 0; i < totalNodes; i++)
    regions[bitcoinNodesRegions[i]]++;
  
  std::cout << "Nodes distribution: \n";
  for (uint32_t i = 0; i < 7; i++)
  {
    std::cout << getBitcoinRegion(getBitcoinEnum(i)) << ": " << regions[i] * 100.0 / totalNodes << "%\n";
  }
}