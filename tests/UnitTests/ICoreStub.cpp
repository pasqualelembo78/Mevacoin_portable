// Copyright (c) 2011-2016 The Mevacoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ICoreStub.h"
#include <algorithm>

#include "MevaCoinCore/MevaCoinFormatUtils.h"
#include "MevaCoinCore/MevaCoinTools.h"
#include "MevaCoinCore/IBlock.h"
#include "MevaCoinCore/VerificationContext.h"


ICoreStub::ICoreStub() :
    topHeight(0),
    globalIndicesResult(false),
    randomOutsResult(false),
    poolTxVerificationResult(true),
    poolChangesResult(true) {
}

ICoreStub::ICoreStub(const MevaCoin::Block& genesisBlock) :
    topHeight(0),
    globalIndicesResult(false),
    randomOutsResult(false),
    poolTxVerificationResult(true),
    poolChangesResult(true) {
  addBlock(genesisBlock);
}

bool ICoreStub::addObserver(MevaCoin::ICoreObserver* observer) {
  return true;
}

bool ICoreStub::removeObserver(MevaCoin::ICoreObserver* observer) {
  return true;
}

void ICoreStub::get_blockchain_top(uint32_t& height, Crypto::Hash& top_id) {
  height = topHeight;
  top_id = topId;
}

std::vector<Crypto::Hash> ICoreStub::findBlockchainSupplement(const std::vector<Crypto::Hash>& remoteBlockIds, size_t maxCount,
  uint32_t& totalBlockCount, uint32_t& startBlockIndex) {

  //Sending all blockchain
  totalBlockCount = static_cast<uint32_t>(blocks.size());
  startBlockIndex = 0;
  std::vector<Crypto::Hash> result;
  result.reserve(std::min(blocks.size(), maxCount));
  for (uint32_t height = 0; height < static_cast<uint32_t>(std::min(blocks.size(), maxCount)); ++height) {
    assert(blockHashByHeightIndex.count(height) > 0);
    result.push_back(blockHashByHeightIndex[height]);
  }
  return result;
}

bool ICoreStub::get_random_outs_for_amounts(const MevaCoin::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_request& req,
    MevaCoin::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_response& res) {
  res = randomOuts;
  return randomOutsResult;
}

bool ICoreStub::get_tx_outputs_gindexs(const Crypto::Hash& tx_id, std::vector<uint32_t>& indexs) {
  std::copy(globalIndices.begin(), globalIndices.end(), std::back_inserter(indexs));
  return globalIndicesResult;
}

MevaCoin::i_mevacoin_protocol* ICoreStub::get_protocol() {
  return nullptr;
}

bool ICoreStub::handle_incoming_tx(MevaCoin::BinaryArray const& tx_blob, MevaCoin::tx_verification_context& tvc, bool keeped_by_block) {
  return true;
}

void ICoreStub::set_blockchain_top(uint32_t height, const Crypto::Hash& top_id) {
  topHeight = height;
  topId = top_id;
}

void ICoreStub::set_outputs_gindexs(const std::vector<uint32_t>& indexs, bool result) {
  globalIndices.clear();
  std::copy(indexs.begin(), indexs.end(), std::back_inserter(globalIndices));
  globalIndicesResult = result;
}

void ICoreStub::set_random_outs(const MevaCoin::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_response& resp, bool result) {
  randomOuts = resp;
  randomOutsResult = result;
}

std::vector<MevaCoin::Transaction> ICoreStub::getPoolTransactions() {
  return std::vector<MevaCoin::Transaction>();
}

bool ICoreStub::getPoolChanges(const Crypto::Hash& tailBlockId, const std::vector<Crypto::Hash>& knownTxsIds,
                               std::vector<MevaCoin::Transaction>& addedTxs, std::vector<Crypto::Hash>& deletedTxsIds) {
  std::unordered_set<Crypto::Hash> knownSet;
  for (const Crypto::Hash& txId : knownTxsIds) {
    if (transactionPool.find(txId) == transactionPool.end()) {
      deletedTxsIds.push_back(txId);
    }

    knownSet.insert(txId);
  }

  for (const std::pair<Crypto::Hash, MevaCoin::Transaction>& poolEntry : transactionPool) {
    if (knownSet.find(poolEntry.first) == knownSet.end()) {
      addedTxs.push_back(poolEntry.second);
    }
  }

  return poolChangesResult;
}

bool ICoreStub::getPoolChangesLite(const Crypto::Hash& tailBlockId, const std::vector<Crypto::Hash>& knownTxsIds,
        std::vector<MevaCoin::TransactionPrefixInfo>& addedTxs, std::vector<Crypto::Hash>& deletedTxsIds) {
  std::vector<MevaCoin::Transaction> added;
  bool returnStatus = getPoolChanges(tailBlockId, knownTxsIds, added, deletedTxsIds);

  for (const auto& tx : added) {
    MevaCoin::TransactionPrefixInfo tpi;
    tpi.txPrefix = tx;
    tpi.txHash = getObjectHash(tx);

    addedTxs.push_back(std::move(tpi));
  }

  return returnStatus;
}

void ICoreStub::getPoolChanges(const std::vector<Crypto::Hash>& knownTxsIds, std::vector<MevaCoin::Transaction>& addedTxs,
                               std::vector<Crypto::Hash>& deletedTxsIds) {
}

bool ICoreStub::queryBlocks(const std::vector<Crypto::Hash>& block_ids, uint64_t timestamp,
  uint32_t& start_height, uint32_t& current_height, uint32_t& full_offset, std::vector<MevaCoin::BlockFullInfo>& entries) {
  //stub
  return true;
}

bool ICoreStub::queryBlocksLite(const std::vector<Crypto::Hash>& block_ids, uint64_t timestamp,
  uint32_t& start_height, uint32_t& current_height, uint32_t& full_offset, std::vector<MevaCoin::BlockShortInfo>& entries) {
  //stub
  return true;
}

std::vector<Crypto::Hash> ICoreStub::buildSparseChain() {
  std::vector<Crypto::Hash> result;
  result.reserve(blockHashByHeightIndex.size());
  for (auto kvPair : blockHashByHeightIndex) {
    result.emplace_back(kvPair.second);
  }

  std::reverse(result.begin(), result.end());
  return result;
}

std::vector<Crypto::Hash> ICoreStub::buildSparseChain(const Crypto::Hash& startBlockId) {
  // TODO implement
  assert(blocks.count(startBlockId) > 0);
  std::vector<Crypto::Hash> result;
  result.emplace_back(blockHashByHeightIndex[0]);
  return result;
}

size_t ICoreStub::addChain(const std::vector<const MevaCoin::IBlock*>& chain) {
  size_t blocksCounter = 0;
  for (const MevaCoin::IBlock* block : chain) {
    for (size_t txNumber = 0; txNumber < block->getTransactionCount(); ++txNumber) {
      const MevaCoin::Transaction& tx = block->getTransaction(txNumber);
      Crypto::Hash txHash = MevaCoin::NULL_HASH;
      size_t blobSize = 0;
      getObjectHash(tx, txHash, blobSize);
      addTransaction(tx);
    }
    addBlock(block->getBlock());
    ++blocksCounter;
  }

  return blocksCounter;
}

Crypto::Hash ICoreStub::getBlockIdByHeight(uint32_t height) {
  auto iter = blockHashByHeightIndex.find(height);
  if (iter == blockHashByHeightIndex.end()) {
    return MevaCoin::NULL_HASH;
  }
  return iter->second;
}

bool ICoreStub::getBlockByHash(const Crypto::Hash &h, MevaCoin::Block &blk) {
  auto iter = blocks.find(h);
  if (iter == blocks.end()) {
    return false;
  }
  blk = iter->second;
  return true;
}

bool ICoreStub::getBlockHeight(const Crypto::Hash& blockId, uint32_t& blockHeight) {
  auto it = blocks.find(blockId);
  if (it == blocks.end()) {
    return false;
  }
  blockHeight = get_block_height(it->second);
  return true;
}

void ICoreStub::getTransactions(const std::vector<Crypto::Hash>& txs_ids, std::list<MevaCoin::Transaction>& txs, std::list<Crypto::Hash>& missed_txs, bool checkTxPool) {
  for (const Crypto::Hash& hash : txs_ids) {
    auto iter = transactions.find(hash);
    if (iter != transactions.end()) {
      txs.push_back(iter->second);
    } else {
      missed_txs.push_back(hash);
    }
  }
  if (checkTxPool) {
    std::list<Crypto::Hash> pullTxIds(std::move(missed_txs));
    missed_txs.clear();
    for (const Crypto::Hash& hash : pullTxIds) {
      auto iter = transactionPool.find(hash);
      if (iter != transactionPool.end()) {
        txs.push_back(iter->second);
      }
      else {
        missed_txs.push_back(hash);
      }
    }
  }
}

bool ICoreStub::getBackwardBlocksSizes(uint32_t fromHeight, std::vector<size_t>& sizes, size_t count) {
  return true;
}

bool ICoreStub::getBlockSize(const Crypto::Hash& hash, size_t& size) {
  return true;
}

bool ICoreStub::getAlreadyGeneratedCoins(const Crypto::Hash& hash, uint64_t& generatedCoins) {
  return true;
}

bool ICoreStub::getBlockReward(size_t medianSize, size_t currentBlockSize, uint64_t alreadyGeneratedCoins, uint64_t fee,
    uint64_t& reward, int64_t& emissionChange) {
  return true;
}

bool ICoreStub::scanOutputkeysForIndices(const MevaCoin::KeyInput& txInToKey, std::list<std::pair<Crypto::Hash, size_t>>& outputReferences) {
  return true;
}

bool ICoreStub::getBlockDifficulty(uint32_t height, MevaCoin::difficulty_type& difficulty) {
  return true;
}

bool ICoreStub::getBlockContainingTx(const Crypto::Hash& txId, Crypto::Hash& blockId, uint32_t& blockHeight) {
  auto iter = blockHashByTxHashIndex.find(txId);
  if (iter == blockHashByTxHashIndex.end()) {
    return false;
  }
  blockId = iter->second;
  auto blockIter = blocks.find(blockId);
  if (blockIter == blocks.end()) {
    return false;
  }
  blockHeight = boost::get<MevaCoin::BaseInput>(blockIter->second.baseTransaction.inputs.front()).blockIndex;
  return true;
}

bool ICoreStub::getMultisigOutputReference(const MevaCoin::MultisignatureInput& txInMultisig, std::pair<Crypto::Hash, size_t>& outputReference) {
  return true;
}

void ICoreStub::addBlock(const MevaCoin::Block& block) {
  uint32_t height = boost::get<MevaCoin::BaseInput>(block.baseTransaction.inputs.front()).blockIndex;
  Crypto::Hash hash = MevaCoin::get_block_hash(block);
  if (height > topHeight) {
    topHeight = height;
    topId = hash;
  }
  blocks.emplace(std::make_pair(hash, block));
  blockHashByHeightIndex.emplace(std::make_pair(height, hash));

  blockHashByTxHashIndex.emplace(std::make_pair(MevaCoin::getObjectHash(block.baseTransaction), hash));
  for (auto txHash : block.transactionHashes) {
    blockHashByTxHashIndex.emplace(std::make_pair(txHash, hash));
  }
}

void ICoreStub::addTransaction(const MevaCoin::Transaction& tx) {
  Crypto::Hash hash = MevaCoin::getObjectHash(tx);
  transactions.emplace(std::make_pair(hash, tx));
}

bool ICoreStub::getGeneratedTransactionsNumber(uint32_t height, uint64_t& generatedTransactions) {
  return true;
}

bool ICoreStub::getOrphanBlocksByHeight(uint32_t height, std::vector<MevaCoin::Block>& blocks) {
  return true;
}

bool ICoreStub::getBlocksByTimestamp(uint64_t timestampBegin, uint64_t timestampEnd, uint32_t blocksNumberLimit, std::vector<MevaCoin::Block>& blocks, uint32_t& blocksNumberWithinTimestamps) {
  return true;
}

bool ICoreStub::getPoolTransactionsByTimestamp(uint64_t timestampBegin, uint64_t timestampEnd, uint32_t transactionsNumberLimit, std::vector<MevaCoin::Transaction>& transactions, uint64_t& transactionsNumberWithinTimestamps) {
  return true;
}

bool ICoreStub::getTransactionsByPaymentId(const Crypto::Hash& paymentId, std::vector<MevaCoin::Transaction>& transactions) {
  return true;
}

std::error_code ICoreStub::executeLocked(const std::function<std::error_code()>& func) {
  return func();
}

std::unique_ptr<MevaCoin::IBlock> ICoreStub::getBlock(const Crypto::Hash& blockId) {
  return std::unique_ptr<MevaCoin::IBlock>(nullptr);
}

bool ICoreStub::handleIncomingTransaction(const MevaCoin::Transaction& tx, const Crypto::Hash& txHash, size_t blobSize, MevaCoin::tx_verification_context& tvc, bool keptByBlock) {
  auto result = transactionPool.emplace(std::make_pair(txHash, tx));
  tvc.m_verifivation_failed = !poolTxVerificationResult;
  tvc.m_added_to_pool = true;
  tvc.m_should_be_relayed = result.second;
  return poolTxVerificationResult;
}

bool ICoreStub::have_block(const Crypto::Hash& id) {
  return blocks.count(id) > 0;
}

void ICoreStub::setPoolTxVerificationResult(bool result) {
  poolTxVerificationResult = result;
}

bool ICoreStub::addMessageQueue(MevaCoin::MessageQueue<MevaCoin::BlockchainMessage>& messageQueuePtr) {
  return true;
}

bool ICoreStub::removeMessageQueue(MevaCoin::MessageQueue<MevaCoin::BlockchainMessage>& messageQueuePtr) {
  return true;
}

void ICoreStub::setPoolChangesResult(bool result) {
  poolChangesResult = result;
}
