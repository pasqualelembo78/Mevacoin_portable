// Copyright (c) 2011-2016 The Mevacoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <queue>

#include <System/ContextGroup.h>
#include <System/Event.h>

#include "BlockchainMonitor.h"
#include "Logging/LoggerRef.h"
#include "Miner.h"
#include "MinerEvent.h"
#include "MiningConfig.h"

namespace System {
class Dispatcher;
}

namespace Miner {

class MinerManager {
public:
  MinerManager(System::Dispatcher& dispatcher, const MevaCoin::MiningConfig& config, Logging::ILogger& logger);
  ~MinerManager();

  void start();

private:
  System::Dispatcher& m_dispatcher;
  Logging::LoggerRef m_logger;
  System::ContextGroup m_contextGroup;
  MevaCoin::MiningConfig m_config;
  MevaCoin::Miner m_miner;
  BlockchainMonitor m_blockchainMonitor;

  System::Event m_eventOccurred;
  System::Event m_httpEvent;
  std::queue<MinerEvent> m_events;

  MevaCoin::Block m_minedBlock;

  uint64_t m_lastBlockTimestamp;

  void eventLoop();
  MinerEvent waitEvent();
  void pushEvent(MinerEvent&& event);

  void startMining(const MevaCoin::BlockMiningParameters& params);
  void stopMining();

  void startBlockchainMonitoring();
  void stopBlockchainMonitoring();

  bool submitBlock(const MevaCoin::Block& minedBlock, const std::string& daemonHost, uint16_t daemonPort);
  MevaCoin::BlockMiningParameters requestMiningParameters(System::Dispatcher& dispatcher, const std::string& daemonHost, uint16_t daemonPort, const std::string& miningAddress);

  void adjustBlockTemplate(MevaCoin::Block& blockTemplate) const;
};

} //namespace Miner
