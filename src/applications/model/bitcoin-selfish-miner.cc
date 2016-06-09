#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/bitcoin-selfish-miner.h"
#include "../../rapidjson/document.h"
#include "../../rapidjson/writer.h"
#include "../../rapidjson/stringbuffer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BitcoinSelfishMiner");

NS_OBJECT_ENSURE_REGISTERED (BitcoinSelfishMiner);

TypeId 
BitcoinSelfishMiner::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BitcoinSelfishMiner")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<BitcoinSelfishMiner> ()
    .AddAttribute ("Local",
                   "The Address on which to Bind the rx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&BitcoinSelfishMiner::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("Protocol",
                   "The type id of the protocol to use for the rx socket.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&BitcoinSelfishMiner::m_tid),
                   MakeTypeIdChecker ())
    .AddAttribute ("NumberOfMiners", 
				   "The number of miners",
                   UintegerValue (16),
                   MakeUintegerAccessor (&BitcoinSelfishMiner::m_noMiners),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("FixedBlockSize", 
				   "The fixed size of the block",
                   UintegerValue (0),
                   MakeUintegerAccessor (&BitcoinSelfishMiner::m_fixedBlockSize),
                   MakeUintegerChecker<uint32_t> ())				   
    .AddAttribute ("FixedBlockIntervalGeneration", 
                   "The fixed time to wait between two consecutive block generations",
                   DoubleValue (0),
                   MakeDoubleAccessor (&BitcoinSelfishMiner::m_fixedBlockTimeGeneration),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("InvTimeoutMinutes", 
				   "The timeout of inv messages in minutes",
                   TimeValue (Minutes (20)),
                   MakeTimeAccessor (&BitcoinSelfishMiner::m_invTimeoutMinutes),
                   MakeTimeChecker())
    .AddAttribute ("HashRate", 
				   "The hash rate of the selfish miner",
                   DoubleValue (0.2),
                   MakeDoubleAccessor (&BitcoinSelfishMiner::m_hashRate),
                   MakeDoubleChecker<double> ())	
    .AddAttribute ("BlockGenBinSize", 
				   "The block generation bin size",
                   DoubleValue (-1),
                   MakeDoubleAccessor (&BitcoinSelfishMiner::m_blockGenBinSize),
                   MakeDoubleChecker<double> ())	
    .AddAttribute ("BlockGenParameter", 
				   "The block generation distribution parameter",
                   DoubleValue (-1),
                   MakeDoubleAccessor (&BitcoinSelfishMiner::m_blockGenParameter),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("AverageBlockGenIntervalSeconds", 
				   "The average block generation interval we aim at (in seconds)",
                   DoubleValue (10*60),
                   MakeDoubleAccessor (&BitcoinSelfishMiner::m_averageBlockGenIntervalSeconds),
                   MakeDoubleChecker<double> ())
    .AddTraceSource ("Rx",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&BitcoinSelfishMiner::m_rxTrace),
                     "ns3::Packet::AddressTracedCallback")
  ;
  return tid;
}


BitcoinSelfishMiner::BitcoinSelfishMiner () : BitcoinMiner(), m_attackFinished(false), m_la(0), m_lh(0), m_forkType(IRRELEVANT)
{
  NS_LOG_FUNCTION (this);
  m_attackerTopBlock = *(m_blockchain.GetCurrentTopBlock());
  m_honestNetworkTopBlock = *(m_blockchain.GetCurrentTopBlock());
  m_maxAttackBlocks = sqrt(sizeof(m_decisionMatrix)/sizeof(char)/3);
}


BitcoinSelfishMiner::~BitcoinSelfishMiner(void)
{
  NS_LOG_FUNCTION (this);
}


void 
BitcoinSelfishMiner::StartApplication ()    // Called at time specified by Start
{
  BitcoinNode::StartApplication ();
  NS_LOG_WARN ("Selfish Miner " << GetNode()->GetId() << " m_realAverageBlockGenIntervalSeconds = " << m_realAverageBlockGenIntervalSeconds << "s");
  NS_LOG_WARN ("Selfish Miner " << GetNode()->GetId() << " m_averageBlockGenIntervalSeconds = " << m_averageBlockGenIntervalSeconds << "s");
  NS_LOG_WARN ("Selfish Miner " << GetNode()->GetId() << " m_fixedBlockTimeGeneration = " << m_fixedBlockTimeGeneration << "s");
  NS_LOG_WARN ("Selfish Miner " << GetNode()->GetId() << " m_hashRate = " << m_hashRate);
  NS_LOG_WARN ("Selfish Miner " << GetNode()->GetId() << " m_maxAttackBlocks = " << m_maxAttackBlocks);

  if (m_blockGenBinSize < 0 && m_blockGenParameter < 0)
  {
    m_blockGenBinSize = 1./m_secondsPerMin/1000;
    m_blockGenParameter = 0.19 * m_blockGenBinSize / 2;
  }
  else
    m_blockGenParameter *= m_hashRate;

  if (m_fixedBlockTimeGeneration == 0)
	m_blockGenTimeDistribution.param(std::geometric_distribution<int>::param_type(m_blockGenParameter)); 

  if (m_fixedBlockSize > 0)
    m_nextBlockSize = m_fixedBlockSize;
  else
  {
    std::array<double,201> intervals {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100, 105, 110, 115, 120, 125, 
                                     130, 135, 140, 145, 150, 155, 160, 165, 170, 175, 180, 185, 190, 195, 200, 205, 210, 215, 220, 225, 230, 235, 
                                     240, 245, 250, 255, 260, 265, 270, 275, 280, 285, 290, 295, 300, 305, 310, 315, 320, 325, 330, 335, 340, 345, 
                                     350, 355, 360, 365, 370, 375, 380, 385, 390, 395, 400, 405, 410, 415, 420, 425, 430, 435, 440, 445, 450, 455, 
                                     460, 465, 470, 475, 480, 485, 490, 495, 500, 505, 510, 515, 520, 525, 530, 535, 540, 545, 550, 555, 560, 565, 
                                     570, 575, 580, 585, 590, 595, 600, 605, 610, 615, 620, 625, 630, 635, 640, 645, 650, 655, 660, 665, 670, 675, 
                                     680, 685, 690, 695, 700, 705, 710, 715, 720, 725, 730, 735, 740, 745, 750, 755, 760, 765, 770, 775, 780, 785, 
                                     790, 795, 800, 805, 810, 815, 820, 825, 830, 835, 840, 845, 850, 855, 860, 865, 870, 875, 880, 885, 890, 895, 
                                     900, 905, 910, 915, 920, 925, 930, 935, 940, 945, 950, 955, 960, 965, 970, 975, 980, 985, 990, 995, 1000};
    std::array<double,200> weights {3.58, 0.33, 0.35, 0.4, 0.38, 0.4, 0.53, 0.46, 0.43, 0.48, 0.56, 0.69, 0.62, 0.62, 0.63, 0.62, 0.62, 0.63, 0.73, 
                                    1.96, 0.75, 0.76, 0.73, 0.64, 0.66, 0.66, 0.66, 0.7, 0.66, 0.73, 0.68, 0.66, 0.67, 0.66, 0.72, 0.68, 0.64, 0.61, 
                                    0.63, 0.58, 0.66, 0.6, 0.7, 0.62, 0.49, 0.59, 0.58, 0.59, 0.63, 1.59, 0.6, 0.58, 0.54, 0.62, 0.55, 0.54, 0.52, 
                                    0.5, 0.53, 0.55, 0.49, 0.47, 0.51, 0.49, 0.52, 0.49, 0.49, 0.49, 0.56, 0.75, 0.51, 0.42, 0.46, 0.47, 0.43, 0.38, 
                                    0.39, 0.39, 0.41, 0.43, 0.38, 0.41, 0.36, 0.41, 0.38, 0.42, 0.42, 0.37, 0.41, 0.41, 0.34, 0.32, 0.37, 0.32, 0.34, 
                                    0.34, 0.34, 0.32, 0.41, 0.62, 0.33, 0.4, 0.32, 0.32, 0.29, 0.35, 0.32, 0.32, 0.28, 0.26, 0.25, 0.29, 0.26, 0.27, 
                                    0.27, 0.24, 0.28, 0.3, 0.27, 0.23, 0.23, 0.28, 0.25, 0.29, 0.24, 0.21, 0.26, 0.29, 0.23, 0.2, 0.24, 0.25, 0.23, 
                                    0.21, 0.26, 0.38, 0.24, 0.21, 0.25, 0.23, 0.22, 0.22, 0.24, 0.23, 0.23, 0.26, 0.24, 0.28, 0.64, 9.96, 0.15, 0.11, 
                                    0.11, 0.1, 0.1, 0.1, 0.11, 0.11, 0.12, 0.13, 0.12, 0.16, 0.12, 0.13, 0.12, 0.1, 0.13, 0.13, 0.13, 0.25, 0.1, 0.14, 
                                    0.14, 0.12, 0.14, 0.14, 0.17, 0.15, 0.19, 0.38, 0.2, 0.19, 0.24, 0.26, 0.36, 1.58, 1.49, 0.1, 0.2, 1.98, 0.05, 0.08, 
                                    0.07, 0.07, 0.14, 0.08, 0.08, 0.53, 3.06, 3.31};
                                
    m_blockSizeDistribution = std::piecewise_constant_distribution<double> (intervals.begin(), intervals.end(), weights.begin());
  }
  
/*   if (GetNode()->GetId() == 0)
  {
    Block newBlock(1, 0, -1, 500000, 0, 0, Ipv4Address("0.0.0.0"));
    m_blockchain.AddBlock(newBlock); 
  } */
  
  m_nodeStats->hashRate = m_hashRate;
  m_nodeStats->miner = 1;

  ScheduleNextMiningEvent ();
}

void 
BitcoinSelfishMiner::StopApplication ()
{
  BitcoinNode::StopApplication ();  
  Simulator::Cancel (m_nextMiningEvent);
  
  NS_LOG_WARN ("The selfish miner " << GetNode ()->GetId () << " with hash rate = " << m_hashRate << " generated " << m_minerGeneratedBlocks 
                << " blocks "<< "(" << 100. * m_minerGeneratedBlocks / (m_blockchain.GetTotalBlocks() - 1) 
                << "%) with average block generation time = " << m_minerAverageBlockGenInterval
                << "s or " << static_cast<int>(m_minerAverageBlockGenInterval) / m_secondsPerMin << "min and " 
                << m_minerAverageBlockGenInterval - static_cast<int>(m_minerAverageBlockGenInterval) / m_secondsPerMin * m_secondsPerMin << "s"
                << " and average size " << m_minerAverageBlockSize << " Bytes");
				
  m_nodeStats->minerGeneratedBlocks = m_minerGeneratedBlocks;
  m_nodeStats->minerAverageBlockGenInterval = m_minerAverageBlockGenInterval;
  m_nodeStats->minerAverageBlockSize = m_minerAverageBlockSize;
  
  Block b = m_honestNetworkTopBlock;
  bool stop = false;
  
  do
  {
    if (b.GetMinerId() == GetNode()->GetId())
      m_nodeStats->minedBlocksInMainChain++;
    if (m_blockchain.GetParent(b))
      b = *(m_blockchain.GetParent(b));
    else
      stop = true;
  }while (!stop);
}

void 
BitcoinSelfishMiner::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  BitcoinMiner::DoDispose ();

}

void 
BitcoinSelfishMiner::MineBlock (void)  
{
  NS_LOG_FUNCTION (this);

  int height =  m_attackerTopBlock.GetBlockHeight() + 1;
  int minerId = GetNode ()->GetId ();
  int parentBlockMinerId = m_attackerTopBlock.GetMinerId();
  double currentTime = Simulator::Now ().GetSeconds ();
  std::ostringstream stringStream;  
  std::string blockHash;

  
  stringStream << height << "/" << minerId;
  blockHash = stringStream.str();


/*   //For attacks
   if (GetNode ()->GetId () == 0)
     height = 2 - m_minerGeneratedBlocks; 
   
   if (GetNode ()->GetId () == 0)
   {
	if (height == 1)
      parentBlockMinerId = -1;
    else 
	  parentBlockMinerId = 0;
   } */
   
  
  if (m_fixedBlockSize > 0)
    m_nextBlockSize = m_fixedBlockSize;
  else
  {
    m_nextBlockSize = m_blockSizeDistribution(m_generator) * 1000;	// *1000 because the m_blockSizeDistribution returns KBytes

    if (m_cryptocurrency == BITCOIN)
    {
      // The block size is linearly dependent on the averageBlockGenIntervalSeconds
      if(m_nextBlockSize < m_maxBlockSize - m_headersSizeBytes)
        m_nextBlockSize = m_nextBlockSize*m_averageBlockGenIntervalSeconds / m_realAverageBlockGenIntervalSeconds
                        + m_headersSizeBytes;	
      else
        m_nextBlockSize = m_nextBlockSize*m_averageBlockGenIntervalSeconds / m_realAverageBlockGenIntervalSeconds;
    }
  }

  if (m_nextBlockSize < m_averageTransactionSize)
    m_nextBlockSize = m_averageTransactionSize + m_headersSizeBytes;

  Block newBlock (height, minerId, parentBlockMinerId, m_nextBlockSize,
                  currentTime, currentTime, Ipv4Address("127.0.0.1"));
  m_attackerTopBlock = newBlock;
  m_blockchain.AddBlock(newBlock);
  
  m_la++;
  
  if (m_la == m_maxAttackBlocks)
  {
    NS_LOG_INFO("m_la == m_maxAttackBlocks in MineBlock");
    std::vector<Block> blocks;

    Block b = m_blockchain.ReturnBlock(m_attackerTopBlock.GetBlockHeight(), GetNode ()->GetId ());
	  
    for (int j = 0; j < m_la; j++)
    {
      blocks.insert(blocks.begin(), b);
      if (m_blockchain.GetParent(b))
        b = *(m_blockchain.GetParent(b));
    }
	  
    ReleaseChain(blocks);
	  
    m_la = 0;
    m_lh = 0;
    m_forkType = IRRELEVANT;
    m_honestNetworkTopBlock = m_attackerTopBlock;
	m_nodeStats->attackSuccess++;
	
    NS_LOG_INFO("---New State = (" << m_la << ", " << m_lh << ", " << getForkType(m_forkType) << ")");  
	
  }
  else if (m_forkType != ACTIVE)
  {
    m_forkType = IRRELEVANT;
  
    NS_LOG_INFO("---New State = (" << m_la << ", " << m_lh << ", " << getForkType(m_forkType) << ")");


    switch(ReadActionMatrix(m_forkType, m_la, m_lh))
    {
      case ADOPT:
      {
        NS_LOG_INFO("MineBlock: ADOPT");
        m_forkType = IRRELEVANT;
        m_la = 0;
        m_lh = 0;
        m_attackerTopBlock = m_honestNetworkTopBlock;
        break;
      }
      case OVERRIDE:
      {
        NS_LOG_INFO("MineBlock: OVERRIDE");
        std::vector<Block> blocks;
  
        Block b = m_blockchain.ReturnBlock(m_honestNetworkTopBlock.GetBlockHeight() + 1, GetNode ()->GetId ());
	    
        for (int j = 0; j < m_lh + 1; j++)
        {
          blocks.insert(blocks.begin(), b);
          if (m_blockchain.GetParent(b))
           b = *(m_blockchain.GetParent(b));
        }
	 
        ReleaseChain(blocks);
	  
        m_forkType = IRRELEVANT;
        m_la = m_la - m_lh - 1;
        m_lh = 0;
        break;
      }
      case MATCH:
      {
        NS_LOG_INFO("MineBlock: MATCH");
        std::vector<Block> blocks;

        Block b = m_blockchain.ReturnBlock(m_honestNetworkTopBlock.GetBlockHeight(), GetNode ()->GetId ());
	  
        for (int j = 0; j < m_lh; j++)
        {
          blocks.insert(blocks.begin(), b);
		  if (m_blockchain.GetParent(b))
            b = *(m_blockchain.GetParent(b));
        }
	  
        ReleaseChain(blocks);

        m_forkType = ACTIVE;
        break;
      }
      case WAIT:
	  {
        NS_LOG_INFO("MineBlock: WAIT");
        break;
      }
      case EXIT:
      {
        NS_LOG_INFO("MineBlock: EXIT");
        std::vector<Block> blocks;

        Block b = m_blockchain.ReturnBlock(m_attackerTopBlock.GetBlockHeight(), GetNode ()->GetId ());
	  
        for (int j = 0; j < m_la; j++)
        {
          blocks.insert(blocks.begin(), b);
		  if (m_blockchain.GetParent(b))
            b = *(m_blockchain.GetParent(b));
        }
	  
        ReleaseChain(blocks);
	  
        m_la = 0;
        m_lh = 0;
        m_forkType = IRRELEVANT;
        m_honestNetworkTopBlock = m_attackerTopBlock;
      
	    m_nodeStats->attackSuccess++;
        break;
      }
      case ERROR:
      {
        NS_FATAL_ERROR("MineBlock: ERROR");
        break; 
      }
    }
  }
  
  NS_LOG_INFO("m_attackerTopBlock = " << m_attackerTopBlock);
  NS_LOG_INFO("m_honestNetworkTopBlock = " << m_honestNetworkTopBlock);
  
  /**
   * Update m_meanBlockReceiveTime with the timeCreated of the newly generated block
   */
  m_meanBlockReceiveTime = (m_blockchain.GetTotalBlocks() - 1)/static_cast<double>(m_blockchain.GetTotalBlocks())*m_meanBlockReceiveTime 
                         + (currentTime - m_previousBlockReceiveTime)/(m_blockchain.GetTotalBlocks());
  m_previousBlockReceiveTime = currentTime;	
  
  m_meanBlockPropagationTime = (m_blockchain.GetTotalBlocks() - 1)/static_cast<double>(m_blockchain.GetTotalBlocks())*m_meanBlockPropagationTime;
  
  m_meanBlockSize = (m_blockchain.GetTotalBlocks() - 1)/static_cast<double>(m_blockchain.GetTotalBlocks())*m_meanBlockSize  
                  + (m_nextBlockSize)/static_cast<double>(m_blockchain.GetTotalBlocks());

  m_minerAverageBlockGenInterval = m_minerGeneratedBlocks/static_cast<double>(m_minerGeneratedBlocks+1)*m_minerAverageBlockGenInterval 
                                 + (Simulator::Now ().GetSeconds () - m_previousBlockGenerationTime)/(m_minerGeneratedBlocks+1);
  m_minerAverageBlockSize = m_minerGeneratedBlocks/static_cast<double>(m_minerGeneratedBlocks+1)*m_minerAverageBlockSize 
                          + static_cast<double>(m_nextBlockSize)/(m_minerGeneratedBlocks+1);
  m_previousBlockGenerationTime = Simulator::Now ().GetSeconds ();
  m_minerGeneratedBlocks++;
			   
  ScheduleNextMiningEvent ();
}


void 
BitcoinSelfishMiner::ReceivedHigherBlock(const Block &newBlock) 
{
  NS_LOG_FUNCTION (this);
  NS_LOG_WARN("Bitcoin selfish miner "<< GetNode ()->GetId () << " added a new block in the m_blockchain with higher height: " << newBlock);
/*   NS_LOG_WARN (m_winningStreak);
  m_winningStreak = 0;
  Simulator::Cancel (m_nextMiningEvent);
  ScheduleNextMiningEvent();
 */
}

void 
BitcoinSelfishMiner::ReceiveBlock(const Block &newBlock) 
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("BitcoinSelfishMiner ReceiveBlock: At time " << Simulator::Now ().GetSeconds ()
                << "s bitcoin node " << GetNode ()->GetId () << " received " << newBlock);

  std::ostringstream   stringStream;  
  std::string          blockHash = stringStream.str();
				
  stringStream << newBlock.GetBlockHeight() << "/" << newBlock.GetMinerId();
  blockHash = stringStream.str();
  
  if (m_blockchain.HasBlock(newBlock) || m_blockchain.IsOrphan(newBlock) || ReceivedButNotValidated(blockHash))
  {
    NS_LOG_INFO ("BitcoinSelfishMiner ReceiveBlock: Bitcoin node " << GetNode ()->GetId () << " has already added this block in the m_blockchain: " << newBlock);
    
    if (m_invTimeouts.find(blockHash) != m_invTimeouts.end())
    {
      m_queueInv.erase(blockHash);
      Simulator::Cancel (m_invTimeouts[blockHash]);
      m_invTimeouts.erase(blockHash);
    }
  }
  else
  {
    NS_LOG_INFO ("BitcoinSelfishMiner ReceiveBlock: Bitcoin node " << GetNode ()->GetId () << " has NOT added this block in the m_blockchain: " << newBlock);

    m_receivedNotValidated[blockHash] = newBlock;
	//PrintQueueInv();
	//PrintInvTimeouts();
	
    m_queueInv.erase(blockHash);
    Simulator::Cancel (m_invTimeouts[blockHash]);
    m_invTimeouts.erase(blockHash);
	
    //PrintQueueInv();
	//PrintInvTimeouts();
    
    if (newBlock.GetBlockHeight() > m_honestNetworkTopBlock.GetBlockHeight())
    {
      if (m_forkType == ACTIVE && newBlock.GetParentBlockMinerId() == GetNode()->GetId())
      {
        m_la -= m_lh;
        m_lh = 1;
	  }
      else
      {
        m_lh++;
      }	
	 
      m_forkType = RELEVANT;
      m_honestNetworkTopBlock = newBlock;
	  
      if (m_lh == m_maxAttackBlocks)
      {
        m_la = 0;
        m_lh = 0;
        m_forkType = IRRELEVANT;
        m_attackerTopBlock = newBlock;
      }
    }
    else if (m_forkType != ACTIVE)
      m_forkType = IRRELEVANT;


    NS_LOG_INFO("---New State = (" << m_la << ", " << m_lh << ", " << getForkType(m_forkType) << ")");

	
    switch(ReadActionMatrix(m_forkType, m_la, m_lh))
    {
      case ADOPT:
      {
        NS_LOG_INFO("ReceiveBlock: ADOPT");
        m_forkType = IRRELEVANT;
        m_la = 0;
        m_lh = 0;
        m_attackerTopBlock = m_honestNetworkTopBlock;
        Simulator::Cancel (m_nextMiningEvent);
        ScheduleNextMiningEvent();
        break;
      }
      case OVERRIDE:
      {
        NS_LOG_INFO("ReceiveBlock: OVERRIDE");
        std::vector<Block> blocks;

        Block b = m_blockchain.ReturnBlock(m_honestNetworkTopBlock.GetBlockHeight() + 1, GetNode ()->GetId ());
	  
        for (int j = 0; j < m_lh + 1; j++)
        {
          blocks.insert(blocks.begin(), b);
		  if (m_blockchain.GetParent(b))
            b = *(m_blockchain.GetParent(b));
        }
	 
        ReleaseChain(blocks);
	  
        m_forkType = IRRELEVANT;
        m_la = m_la - m_lh - 1;
        m_lh = 0;
        break;
      }
      case MATCH:
      {
        NS_LOG_INFO("ReceiveBlock: MATCH");
        std::vector<Block> blocks;

        Block b = m_blockchain.ReturnBlock(m_honestNetworkTopBlock.GetBlockHeight(), GetNode ()->GetId ());
	  
        for (int j = 0; j < m_lh; j++)
        {
          blocks.insert(blocks.begin(), b);
		  if (m_blockchain.GetParent(b))
            b = *(m_blockchain.GetParent(b));        
        }
	  
        ReleaseChain(blocks);
		
        m_forkType = ACTIVE;
        break;
      }
      case WAIT:
      {
        NS_LOG_INFO("ReceiveBlock: WAIT");
        break;
      }
      case EXIT:
      {
        NS_FATAL_ERROR("ReceiveBlock: EXIT");
        break;
      }
      case ERROR:
      {
        NS_FATAL_ERROR("ReceiveBlock: ERROR");
        break; 
      }
    }

    NS_LOG_INFO("m_attackerTopBlock = " << m_attackerTopBlock);
    NS_LOG_INFO("m_honestNetworkTopBlock = " << m_honestNetworkTopBlock);
    ValidateBlock (newBlock);
  }
}


void 
BitcoinSelfishMiner::ReleaseChain(std::vector<Block> blocks)
{
  NS_LOG_FUNCTION (this);
  
  rapidjson::Document inv; 
  rapidjson::Document block; 
  
  inv.SetObject();
  block.SetObject();
  
  switch(m_blockBroadcastType)				  
  {
    case STANDARD:
    {
      rapidjson::Value value;
      rapidjson::Value array(rapidjson::kArrayType);
      rapidjson::Value blockInfo(rapidjson::kObjectType);

      value.SetString("block"); //Remove
      inv.AddMember("type", value, inv.GetAllocator());
	  
      if (m_protocolType == STANDARD_PROTOCOL)
      {
        value = INV;
        inv.AddMember("message", value, inv.GetAllocator());
  		  
        for(auto it = blocks.begin(); it != blocks.end(); it++)
        {
          std::ostringstream   stringStream;  
          std::string          blockHash = stringStream.str();
				
          stringStream << it->GetBlockHeight() << "/" << it->GetMinerId();
          blockHash = stringStream.str();
          value.SetString(blockHash.c_str(), blockHash.size(), inv.GetAllocator());
          array.PushBack(value, inv.GetAllocator());
        }
		
        inv.AddMember("inv", array, inv.GetAllocator());
		
      }
      else if (m_protocolType == SENDHEADERS)
      {
        value = HEADERS;
        inv.AddMember("message", value, inv.GetAllocator());
		
        for(auto it = blocks.begin(); it != blocks.end(); it++)
        {
          value = it->GetBlockHeight ();
          blockInfo.AddMember("height", value, inv.GetAllocator ());

          value = it->GetMinerId ();
          blockInfo.AddMember("minerId", value, inv.GetAllocator ());

          value = it->GetParentBlockMinerId ();
          blockInfo.AddMember("parentBlockMinerId", value, inv.GetAllocator ());

          value = it->GetBlockSizeBytes ();
          blockInfo.AddMember("size", value, inv.GetAllocator ());

          value = it->GetTimeCreated ();
          blockInfo.AddMember("timeCreated", value, inv.GetAllocator ());

          value = it->GetTimeReceived ();							
          blockInfo.AddMember("timeReceived", value, inv.GetAllocator ());
		
          array.PushBack(blockInfo, inv.GetAllocator());
        }
		
        inv.AddMember("blocks", array, inv.GetAllocator());      
      }	
      break;
    }
    case UNSOLICITED:
    {
      rapidjson::Value value (BLOCK);
      rapidjson::Value blockInfo(rapidjson::kObjectType);
      rapidjson::Value array(rapidjson::kArrayType);
	  
      block.AddMember("message", value, block.GetAllocator());

      value.SetString("block"); //Remove
      block.AddMember("type", value, block.GetAllocator());

      for(auto it = blocks.begin(); it != blocks.end(); it++)
      {
        value = it->GetBlockHeight ();
        blockInfo.AddMember("height", value, block.GetAllocator ());

        value = it->GetMinerId ();
        blockInfo.AddMember("minerId", value, block.GetAllocator ());

        value = it->GetParentBlockMinerId ();
        blockInfo.AddMember("parentBlockMinerId", value, block.GetAllocator ());

        value = it->GetBlockSizeBytes ();
        blockInfo.AddMember("size", value, block.GetAllocator ());

        value = it->GetTimeCreated ();
        blockInfo.AddMember("timeCreated", value, block.GetAllocator ());

        value = it->GetTimeReceived ();							
        blockInfo.AddMember("timeReceived", value, block.GetAllocator ());

        array.PushBack(blockInfo, block.GetAllocator());
      }
	  
      block.AddMember("blocks", array, block.GetAllocator());
      
      break;
    }
    case RELAY_NETWORK:
    {
      rapidjson::Value value;
      rapidjson::Value headersInfo(rapidjson::kObjectType);
      rapidjson::Value invArray(rapidjson::kArrayType);
      rapidjson::Value blockArray(rapidjson::kArrayType);
	  
      value.SetString("block"); //Remove
      inv.AddMember("type", value, inv.GetAllocator());
	  
      if (m_protocolType == STANDARD_PROTOCOL)
      {

        value = INV;
        inv.AddMember("message", value, inv.GetAllocator());
		  
        for(auto it = blocks.begin(); it != blocks.end(); it++)
        {
          std::ostringstream   stringStream;  
          std::string          blockHash = stringStream.str();
				
          stringStream << it->GetBlockHeight() << "/" << it->GetMinerId();
          blockHash = stringStream.str();
          value.SetString(blockHash.c_str(), blockHash.size(), inv.GetAllocator());
          invArray.PushBack(value, inv.GetAllocator());
        }
		
        inv.AddMember("inv", invArray, inv.GetAllocator()); 
 
      }
      else if (m_protocolType == SENDHEADERS)
      {
        value = HEADERS;
        inv.AddMember("message", value, inv.GetAllocator());
		
        for(auto it = blocks.begin(); it != blocks.end(); it++)
        {
          value = it->GetBlockHeight ();
          headersInfo.AddMember("height", value, inv.GetAllocator ());

          value = it->GetMinerId ();
          headersInfo.AddMember("minerId", value, inv.GetAllocator ());

          value = it->GetParentBlockMinerId ();
          headersInfo.AddMember("parentBlockMinerId", value, inv.GetAllocator ());

          value = it->GetBlockSizeBytes ();
          headersInfo.AddMember("size", value, inv.GetAllocator ());

          value = it->GetTimeCreated ();
          headersInfo.AddMember("timeCreated", value, inv.GetAllocator ());

          value = it->GetTimeReceived ();							
          headersInfo.AddMember("timeReceived", value, inv.GetAllocator ());
		  
          invArray.PushBack(headersInfo, inv.GetAllocator());
        }
		
        inv.AddMember("blocks", invArray, inv.GetAllocator());      
      }	
	  
	  
	  
      //Unsolicited for miners
      value = BLOCK;
      block.AddMember("message", value, block.GetAllocator());

      value.SetString("compressed-block"); //Remove
      block.AddMember("type", value, block.GetAllocator());

      for(auto it = blocks.begin(); it != blocks.end(); it++)
      {
        rapidjson::Value blockInfo(rapidjson::kObjectType);
		
        value = it->GetBlockHeight ();
        blockInfo.AddMember("height", value, block.GetAllocator ());

        value = it->GetMinerId ();
        blockInfo.AddMember("minerId", value, block.GetAllocator ());

        value = it->GetParentBlockMinerId ();
        blockInfo.AddMember("parentBlockMinerId", value, block.GetAllocator ());

        value = it->GetBlockSizeBytes ();
        blockInfo.AddMember("size", value, block.GetAllocator ());

        value = it->GetTimeCreated ();
        blockInfo.AddMember("timeCreated", value, block.GetAllocator ());

        value = it->GetTimeReceived ();							
        blockInfo.AddMember("timeReceived", value, block.GetAllocator ());

        blockArray.PushBack(blockInfo, block.GetAllocator());
      }
	  
      block.AddMember("blocks", blockArray, block.GetAllocator());
      
      break;
    }
    case UNSOLICITED_RELAY_NETWORK:
    {
      rapidjson::Value value;
      rapidjson::Value blockNodesInfo(rapidjson::kObjectType);
      rapidjson::Value blockInfo(rapidjson::kObjectType);
      rapidjson::Value invArray(rapidjson::kArrayType);
      rapidjson::Value blockArray(rapidjson::kArrayType);
	  
      //Unsolicited for nodes
      value = BLOCK;
      inv.AddMember("message", value, inv.GetAllocator());

      value.SetString("block"); //Remove
      inv.AddMember("type", value, inv.GetAllocator());

      for(auto it = blocks.begin(); it != blocks.end(); it++)
      {
        value = it->GetBlockHeight ();
        blockNodesInfo.AddMember("height", value, inv.GetAllocator ());

        value = it->GetMinerId ();
        blockNodesInfo.AddMember("minerId", value, inv.GetAllocator ());

        value = it->GetParentBlockMinerId ();
        blockNodesInfo.AddMember("parentBlockMinerId", value, inv.GetAllocator ());

        value = it->GetBlockSizeBytes ();
        blockNodesInfo.AddMember("size", value, inv.GetAllocator ());

        value = it->GetTimeCreated ();
        blockNodesInfo.AddMember("timeCreated", value, inv.GetAllocator ());

        value = it->GetTimeReceived ();							
        blockNodesInfo.AddMember("timeReceived", value, inv.GetAllocator ());

        invArray.PushBack(blockNodesInfo, inv.GetAllocator());
      }
	  
      inv.AddMember("blocks", invArray, inv.GetAllocator());
	  
	  
      //Unsolicited for miners
      value = BLOCK;
      block.AddMember("message", value, block.GetAllocator());

      value.SetString("compressed-block"); //Remove
      block.AddMember("type", value, block.GetAllocator());

      for(auto it = blocks.begin(); it != blocks.end(); it++)
      {
        value = it->GetBlockHeight ();
        blockInfo.AddMember("height", value, block.GetAllocator ());

        value = it->GetMinerId ();
        blockInfo.AddMember("minerId", value, block.GetAllocator ());

        value = it->GetParentBlockMinerId ();
        blockInfo.AddMember("parentBlockMinerId", value, block.GetAllocator ());

        value = it->GetBlockSizeBytes ();
        blockInfo.AddMember("size", value, block.GetAllocator ());

        value = it->GetTimeCreated ();
        blockInfo.AddMember("timeCreated", value, block.GetAllocator ());

        value = it->GetTimeReceived ();							
        blockInfo.AddMember("timeReceived", value, block.GetAllocator ());

        blockArray.PushBack(blockInfo, block.GetAllocator());
      }
	  
      block.AddMember("blocks", blockArray, block.GetAllocator());
      
      break;
    }
  }
  

  // Stringify the DOM
  rapidjson::StringBuffer invInfo;
  rapidjson::Writer<rapidjson::StringBuffer> invWriter(invInfo);
  inv.Accept(invWriter);
  
  rapidjson::StringBuffer blockInfo;
  rapidjson::Writer<rapidjson::StringBuffer> blockWriter(blockInfo);
  block.Accept(blockWriter);
  
  int count = 0;
  
  for (std::vector<Ipv4Address>::const_iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i, ++count)
  {
    
    const uint8_t delimiter[] = "#";

    switch(m_blockBroadcastType)				  
    {
      case STANDARD:
      {
        m_peersSockets[*i]->Send (reinterpret_cast<const uint8_t*>(invInfo.GetString()), invInfo.GetSize(), 0);
        m_peersSockets[*i]->Send (delimiter, 1, 0);
		
        if (m_protocolType == STANDARD_PROTOCOL && !m_blockTorrent)
          m_nodeStats->invSentBytes += m_bitcoinMessageHeader + m_countBytes + inv["inv"].Size()*m_inventorySizeBytes;
        else if (m_protocolType == SENDHEADERS && !m_blockTorrent)
          m_nodeStats->headersSentBytes += m_bitcoinMessageHeader + m_countBytes + inv["blocks"].Size()*m_headersSizeBytes;
        else if (m_protocolType == STANDARD_PROTOCOL && m_blockTorrent)
        {
          m_nodeStats->extInvSentBytes += m_bitcoinMessageHeader + m_countBytes + inv["inv"].Size()*m_inventorySizeBytes;
          for (int j=0; j<inv["inv"].Size(); j++)
          {
            m_nodeStats->extInvSentBytes += 5; //1Byte(fullBlock) + 4Bytes(numberOfChunks)
            if (!inv["inv"][j]["fullBlock"].GetBool())
              m_nodeStats->extInvSentBytes += inv["inv"][j]["availableChunks"].Size()*1;
          }
        }
        else if (m_protocolType == SENDHEADERS && m_blockTorrent)
        {
          m_nodeStats->extHeadersSentBytes += m_bitcoinMessageHeader + m_countBytes + inv["blocks"].Size()*m_headersSizeBytes;
          for (int j=0; j<inv["blocks"].Size(); j++)
          {
            m_nodeStats->extHeadersSentBytes += 1;//fullBlock
            if (!inv["blocks"][j]["fullBlock"].GetBool())
              m_nodeStats->extHeadersSentBytes += inv["inv"][j]["availableChunks"].Size();
          }	
        }
		
        NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                     << "s bitcoin miner " << GetNode ()->GetId () 
                     << " sent a packet " << invInfo.GetString() 
			         << " to " << *i);
        break;
      }
      case UNSOLICITED:
      {
        long blockMessageSize = 0;
		
        for (int j=0; j<block["blocks"].Size(); j++)
          blockMessageSize += block["blocks"][j]["size"].GetInt();

        m_nodeStats->blockSentBytes += m_bitcoinMessageHeader + blockMessageSize;

        double sendTime = blockMessageSize / m_uploadSpeed;
        double eventTime;	
				
/*                 std::cout << "Node " << GetNode()->GetId() << "-" << InetSocketAddress::ConvertFrom(from).GetIpv4 () 
                          << " " << m_peersDownloadSpeeds[InetSocketAddress::ConvertFrom(from).GetIpv4 ()] << " Mbps , time = "
                          << Simulator::Now ().GetSeconds() << "s \n"; */
                
        if (m_sendBlockTimes.size() == 0 || Simulator::Now ().GetSeconds() >  m_sendBlockTimes.back())
        {
          eventTime = 0; 
        }
        else
        {
          //std::cout << "m_sendBlockTimes.back() = m_sendBlockTimes.back() = " << m_sendBlockTimes.back() << std::endl;
          eventTime = m_sendBlockTimes.back() - Simulator::Now ().GetSeconds(); 
        }
        m_sendBlockTimes.push_back(Simulator::Now ().GetSeconds() + eventTime + sendTime);
 
 
        /* std::cout << sendTime << " " << eventTime << " " << m_sendBlockTimes.size() << std::endl; */
        NS_LOG_INFO("Node " << GetNode()->GetId() << " will start sending the block to " << *i 
                    << " at " << Simulator::Now ().GetSeconds() + eventTime << "\n");

        std::string packet = blockInfo.GetString();
        Simulator::Schedule (Seconds(eventTime), &BitcoinSelfishMiner::SendBlock, this, packet, m_peersSockets[*i]);
        Simulator::Schedule (Seconds(eventTime + sendTime), &BitcoinSelfishMiner::RemoveSendTime, this);

        break;
      }
      case RELAY_NETWORK:
      {
        if(count < m_noMiners - 1)
        {
          
		  long blockMessageSize = 0;
		  
          for (int j=0; j<block["blocks"].Size(); j++)
          {  
            int    noTransactions = static_cast<int>((block["blocks"][j]["size"].GetInt() - m_blockHeadersSizeBytes)/m_averageTransactionSize);
            long   blockSize = m_blockHeadersSizeBytes + m_transactionIndexSize*noTransactions;
            blockMessageSize += blockSize;
          }
		  
          double sendTime = blockMessageSize / m_uploadSpeed;
          double eventTime;
		  
          m_nodeStats->blockSentBytes += m_bitcoinMessageHeader + blockMessageSize;
			  
/* 				std::cout << "Node " << GetNode()->GetId() << "-" << *i 
                            << " " << m_peersDownloadSpeeds[*i] << " Mbps , time = "
                            << Simulator::Now ().GetSeconds() << "s \n"; */
                
          if (m_sendCompressedBlockTimes.size() == 0 || Simulator::Now ().GetSeconds() >  m_sendCompressedBlockTimes.back())
          {
            eventTime = 0; 
          }
          else
          {
            //std::cout << "m_sendCompressedBlockTimes.back() = m_sendCompressedBlockTimes.back() = " << m_sendCompressedBlockTimes.back() << std::endl;
            eventTime = m_sendCompressedBlockTimes.back() - Simulator::Now ().GetSeconds(); 
          }
          m_sendCompressedBlockTimes.push_back(Simulator::Now ().GetSeconds() + eventTime + sendTime);
 
          //std::cout << sendTime << " " << eventTime << " " << m_sendCompressedBlockTimes.size() << std::endl;
          NS_LOG_INFO("Node " << GetNode()->GetId() << " will start sending the block to " << *i
                      << " at " << Simulator::Now ().GetSeconds() + eventTime << "\n");

          //sendTime = blockSize / m_uploadSpeed * count;		  
          //std::cout << sendTime << std::endl;

          std::string packet = blockInfo.GetString();
          Simulator::Schedule (Seconds(sendTime), &BitcoinSelfishMiner::SendBlock, this, packet, m_peersSockets[*i]);
          Simulator::Schedule (Seconds(eventTime + sendTime), &BitcoinSelfishMiner::RemoveCompressedBlockSendTime, this);

        }
        else
        {	    
          m_peersSockets[*i]->Send (reinterpret_cast<const uint8_t*>(invInfo.GetString()), invInfo.GetSize(), 0);
          m_peersSockets[*i]->Send (delimiter, 1, 0);
	  
          if (m_protocolType == STANDARD_PROTOCOL && !m_blockTorrent)
            m_nodeStats->invSentBytes += m_bitcoinMessageHeader + m_countBytes + inv["inv"].Size()*m_inventorySizeBytes;
          else if (m_protocolType == SENDHEADERS && !m_blockTorrent)
            m_nodeStats->headersSentBytes += m_bitcoinMessageHeader + m_countBytes + inv["blocks"].Size()*m_headersSizeBytes;
          else if (m_protocolType == STANDARD_PROTOCOL && m_blockTorrent)
          {
            m_nodeStats->extInvSentBytes += m_bitcoinMessageHeader + m_countBytes + inv["inv"].Size()*m_inventorySizeBytes;
            for (int j=0; j<inv["inv"].Size(); j++)
            {
              m_nodeStats->extInvSentBytes += 5; //1Byte(fullBlock) + 4Bytes(numberOfChunks)
              if (!inv["inv"][j]["fullBlock"].GetBool())
                m_nodeStats->extInvSentBytes += inv["inv"][j]["availableChunks"].Size()*1;
            }
          }
          else if (m_protocolType == SENDHEADERS && m_blockTorrent)
          {
            m_nodeStats->extHeadersSentBytes += m_bitcoinMessageHeader + m_countBytes + inv["blocks"].Size()*m_headersSizeBytes;
            for (int j=0; j<inv["blocks"].Size(); j++)
            {
            m_nodeStats->extHeadersSentBytes += 1;//fullBlock
            if (!inv["blocks"][j]["fullBlock"].GetBool())
                m_nodeStats->extHeadersSentBytes += inv["blocks"][j]["availableChunks"].Size()*1;
            }	
          }
	  
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s bitcoin miner " << GetNode ()->GetId () 
                       << " sent a packet " << invInfo.GetString() 
                       << " to " << *i);
        }
        break;
      }
      case UNSOLICITED_RELAY_NETWORK:
      {
        double sendTime;
        double eventTime;
        std::string packet;
			  
/* 				std::cout << "Node " << GetNode()->GetId() << "-" << *i 
                            << " " << m_peersDownloadSpeeds[*i] << " Mbps , time = "
                            << Simulator::Now ().GetSeconds() << "s \n"; */
							
        if(count < m_noMiners - 1)
        {
		  long blockMessageSize = 0;
		  
          for (int j=0; j<block["blocks"].Size(); j++)
          {  
            int    noTransactions = static_cast<int>((block["blocks"][j]["size"].GetInt() - m_blockHeadersSizeBytes)/m_averageTransactionSize);
            long   blockSize = m_blockHeadersSizeBytes + m_transactionIndexSize*noTransactions;
            blockMessageSize += blockSize;
          }
		  
          sendTime = blockMessageSize / m_uploadSpeed;

          m_nodeStats->blockSentBytes += m_bitcoinMessageHeader + blockMessageSize;
		  
          if (m_sendCompressedBlockTimes.size() == 0 || Simulator::Now ().GetSeconds() >  m_sendCompressedBlockTimes.back())
          {
            eventTime = 0; 
          }
          else
          {
            //std::cout << "m_sendCompressedBlockTimes.back() = m_sendCompressedBlockTimes.back() = " << m_sendCompressedBlockTimes.back() << std::endl;
            eventTime = m_sendCompressedBlockTimes.back() - Simulator::Now ().GetSeconds(); 
          }
          m_sendCompressedBlockTimes.push_back(Simulator::Now ().GetSeconds() + eventTime + sendTime);
 
          //std::cout << sendTime << " " << eventTime << " " << m_sendCompressedBlockTimes.size() << std::endl;
          NS_LOG_INFO("Node " << GetNode()->GetId() << " will start sending the block to " << *i
                      << " at " << Simulator::Now ().GetSeconds() + eventTime << "\n");

          //sendTime = blockMessageSize / m_uploadSpeed * count;		  
          //std::cout << sendTime << std::endl;

          std::string packet = blockInfo.GetString();
          Simulator::Schedule (Seconds(sendTime), &BitcoinSelfishMiner::SendBlock, this, packet, m_peersSockets[*i]);
          Simulator::Schedule (Seconds(eventTime + sendTime), &BitcoinSelfishMiner::RemoveCompressedBlockSendTime, this);
        }
        else
        {
          long blockMessageSize = 0;
		
          for (int j=0; j<inv["blocks"].Size(); j++)
            blockMessageSize += inv["blocks"][j]["size"].GetInt();

          sendTime = blockMessageSize / m_uploadSpeed;
          m_nodeStats->blockSentBytes += m_bitcoinMessageHeader + blockMessageSize;
		  
          if (m_sendBlockTimes.size() == 0 || Simulator::Now ().GetSeconds() >  m_sendBlockTimes.back())
          {
            eventTime = 0; 
          }
          else
          {
            //std::cout << "m_sendBlockTimes.back() = m_sendBlockTimes.back() = " << m_sendBlockTimes.back() << std::endl;
            eventTime = m_sendBlockTimes.back() - Simulator::Now ().GetSeconds(); 
          }
          m_sendBlockTimes.push_back(Simulator::Now ().GetSeconds() + eventTime + sendTime);
          packet = invInfo.GetString();
		  
          /* std::cout << sendTime << " " << eventTime << " " << m_sendBlockTimes.size() << std::endl; */
          NS_LOG_INFO("Node " << GetNode()->GetId() << " will send the block to " << *i 
                      << " at " << Simulator::Now ().GetSeconds() + eventTime << ", eventTime = " << eventTime  << "\n");

          Simulator::Schedule (Seconds(eventTime), &BitcoinSelfishMiner::SendBlock, this, packet, m_peersSockets[*i]);
          Simulator::Schedule (Seconds(eventTime + sendTime), &BitcoinSelfishMiner::RemoveSendTime, this);

        }
	   break;
      }
    }
  } 
}


enum Action 
BitcoinSelfishMiner::ReadActionMatrix(enum ForkType f, int la, int lh)
{
  NS_LOG_FUNCTION (this);
  switch (m_decisionMatrix[f][la][lh]) 
  {
    case 'a': return ADOPT;
    case 'o': return OVERRIDE;
    case 'm': return MATCH;
    case 'w': return WAIT;
    case 'e': return EXIT;
    case '*': return ERROR;

  }
}

const char* getForkType(enum ForkType m)
{
  switch (m) 
  {
    case IRRELEVANT: return "IRRELEVANT";
    case RELEVANT: return "RELEVANT";
    case ACTIVE: return "ACTIVE";
  }
}


const char* getAction(enum Action m)
{
  switch (m) 
  {
    case ADOPT: return "ADOPT";
    case OVERRIDE: return "OVERRIDE";
    case MATCH: return "MATCH";
    case WAIT: return "WAIT";
    case EXIT: return "EXIT";
    case ERROR: return "ERROR";

  }
}

} // Namespace ns3
