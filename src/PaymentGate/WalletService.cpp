// Copyright (c) 2011-2016 The Mevacoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "WalletService.h"


#include <future>
#include <assert.h>
#include <sstream>
#include <unordered_set>

#include <boost/filesystem/operations.hpp>

#include <System/Timer.h>
#include <System/InterruptedException.h>
#include "Common/Util.h"

#include "crypto/crypto.h"
#include "MevaCoin.h"
#include "MevaCoinCore/MevaCoinFormatUtils.h"
#include "MevaCoinCore/MevaCoinBasicImpl.h"
#include "MevaCoinCore/TransactionExtra.h"

#include <System/EventLock.h>

#include "PaymentServiceJsonRpcMessages.h"
#include "WalletFactory.h"
#include "NodeFactory.h"

#include "Wallet/LegacyKeysImporter.h"
#include "Wallet/WalletErrors.h"
#include "Wallet/WalletUtils.h"
#include "WalletServiceErrorCategory.h"

namespace PaymentService {

namespace {

bool checkPaymentId(const std::string& paymentId) {
  if (paymentId.size() != 64) {
    return false;
  }

  return std::all_of(paymentId.begin(), paymentId.end(), [] (const char c) {
    if (c >= '0' && c <= '9') {
      return true;
    }

    if (c >= 'a' && c <= 'f') {
      return true;
    }

    if (c >= 'A' && c <= 'F') {
      return true;
    }

    return false;
  });
}

Crypto::Hash parsePaymentId(const std::string& paymentIdStr) {
  if (!checkPaymentId(paymentIdStr)) {
    throw std::system_error(make_error_code(MevaCoin::error::WalletServiceErrorCode::WRONG_PAYMENT_ID_FORMAT));
  }

  Crypto::Hash paymentId;
  bool r = Common::podFromHex(paymentIdStr, paymentId);
  assert(r);

  return paymentId;
}

bool getPaymentIdFromExtra(const std::string& binaryString, Crypto::Hash& paymentId) {
  return MevaCoin::getPaymentIdFromTxExtra(Common::asBinaryArray(binaryString), paymentId);
}

std::string getPaymentIdStringFromExtra(const std::string& binaryString) {
  Crypto::Hash paymentId;

  if (!getPaymentIdFromExtra(binaryString, paymentId)) {
    return std::string();
  }

  return Common::podToHex(paymentId);
}

}

struct TransactionsInBlockInfoFilter {
  TransactionsInBlockInfoFilter(const std::vector<std::string>& addressesVec, const std::string& paymentIdStr) {
    addresses.insert(addressesVec.begin(), addressesVec.end());

    if (!paymentIdStr.empty()) {
      paymentId = parsePaymentId(paymentIdStr);
      havePaymentId = true;
    } else {
      havePaymentId = false;
    }
  }

  bool checkTransaction(const MevaCoin::WalletTransactionWithTransfers& transaction) const {
    if (havePaymentId) {
      Crypto::Hash transactionPaymentId;
      if (!getPaymentIdFromExtra(transaction.transaction.extra, transactionPaymentId)) {
        return false;
      }

      if (paymentId != transactionPaymentId) {
        return false;
      }
    }

    if (addresses.empty()) {
      return true;
    }

    bool haveAddress = false;
    for (const MevaCoin::WalletTransfer& transfer: transaction.transfers) {
      if (addresses.find(transfer.address) != addresses.end()) {
        haveAddress = true;
        break;
      }
    }

    return haveAddress;
  }

  std::unordered_set<std::string> addresses;
  bool havePaymentId = false;
  Crypto::Hash paymentId;
};

namespace {

void addPaymentIdToExtra(const std::string& paymentId, std::string& extra) {
  std::vector<uint8_t> extraVector;
  if (!MevaCoin::createTxExtraWithPaymentId(paymentId, extraVector)) {
    throw std::runtime_error("Couldn't add payment id to extra");
  }

  std::copy(extraVector.begin(), extraVector.end(), std::back_inserter(extra));
}

void validatePaymentId(const std::string& paymentId, Logging::LoggerRef logger) {
  if (!checkPaymentId(paymentId)) {
    logger(Logging::WARNING) << "Can't validate payment id: " << paymentId;
    throw std::system_error(make_error_code(MevaCoin::error::WalletServiceErrorCode::WRONG_PAYMENT_ID_FORMAT));
  }
}

bool createOutputBinaryFile(const std::string& filename, std::fstream& file) {
  file.open(filename.c_str(), std::fstream::in | std::fstream::out | std::ofstream::binary);
  if (file) {
    file.close();
    return false;
  }

  file.open(filename.c_str(), std::fstream::out | std::fstream::binary);
  return true;
}

std::string createTemporaryFile(const std::string& path, std::fstream& tempFile) {
  bool created = false;
  std::string temporaryName;

  for (size_t i = 1; i < 100; i++) {
    temporaryName = path + "." + std::to_string(i++);

    if (createOutputBinaryFile(temporaryName, tempFile)) {
      created = true;
      break;
    }
  }

  if (!created) {
    throw std::runtime_error("Couldn't create temporary file: " + temporaryName);
  }

  return temporaryName;
}

//returns true on success
bool deleteFile(const std::string& filename) {
  boost::system::error_code err;
  return boost::filesystem::remove(filename, err) && !err;
}

void replaceWalletFiles(const std::string &path, const std::string &tempFilePath) {
  Tools::replace_file(tempFilePath, path);
}

Crypto::Hash parseHash(const std::string& hashString, Logging::LoggerRef logger) {
  Crypto::Hash hash;

  if (!Common::podFromHex(hashString, hash)) {
    logger(Logging::WARNING) << "Can't parse hash string " << hashString;
    throw std::system_error(make_error_code(MevaCoin::error::WalletServiceErrorCode::WRONG_HASH_FORMAT));
  }

  return hash;
}

std::vector<MevaCoin::TransactionsInBlockInfo> filterTransactions(
  const std::vector<MevaCoin::TransactionsInBlockInfo>& blocks,
  const TransactionsInBlockInfoFilter& filter) {

  std::vector<MevaCoin::TransactionsInBlockInfo> result;

  for (const auto& block: blocks) {
    MevaCoin::TransactionsInBlockInfo item;
    item.blockHash = block.blockHash;

    for (const auto& transaction: block.transactions) {
      if (transaction.transaction.state != MevaCoin::WalletTransactionState::DELETED && filter.checkTransaction(transaction)) {
        item.transactions.push_back(transaction);
      }
    }

    result.push_back(std::move(item));
  }

  return result;
}

PaymentService::TransactionRpcInfo convertTransactionWithTransfersToTransactionRpcInfo(
  const MevaCoin::WalletTransactionWithTransfers& transactionWithTransfers) {

  PaymentService::TransactionRpcInfo transactionInfo;

  transactionInfo.state = static_cast<uint8_t>(transactionWithTransfers.transaction.state);
  transactionInfo.transactionHash = Common::podToHex(transactionWithTransfers.transaction.hash);
  transactionInfo.blockIndex = transactionWithTransfers.transaction.blockHeight;
  transactionInfo.timestamp = transactionWithTransfers.transaction.timestamp;
  transactionInfo.isBase = transactionWithTransfers.transaction.isBase;
  transactionInfo.unlockTime = transactionWithTransfers.transaction.unlockTime;
  transactionInfo.amount = transactionWithTransfers.transaction.totalAmount;
  transactionInfo.fee = transactionWithTransfers.transaction.fee;
  transactionInfo.extra = Common::toHex(transactionWithTransfers.transaction.extra.data(), transactionWithTransfers.transaction.extra.size());
  transactionInfo.paymentId = getPaymentIdStringFromExtra(transactionWithTransfers.transaction.extra);

  for (const MevaCoin::WalletTransfer& transfer: transactionWithTransfers.transfers) {
    PaymentService::TransferRpcInfo rpcTransfer;
    rpcTransfer.address = transfer.address;
    rpcTransfer.amount = transfer.amount;
    rpcTransfer.type = static_cast<uint8_t>(transfer.type);

    transactionInfo.transfers.push_back(std::move(rpcTransfer));
  }

  return transactionInfo;
}

std::vector<PaymentService::TransactionsInBlockRpcInfo> convertTransactionsInBlockInfoToTransactionsInBlockRpcInfo(
  const std::vector<MevaCoin::TransactionsInBlockInfo>& blocks) {

  std::vector<PaymentService::TransactionsInBlockRpcInfo> rpcBlocks;
  rpcBlocks.reserve(blocks.size());
  for (const auto& block: blocks) {
    PaymentService::TransactionsInBlockRpcInfo rpcBlock;
    rpcBlock.blockHash = Common::podToHex(block.blockHash);

    for (const MevaCoin::WalletTransactionWithTransfers& transactionWithTransfers: block.transactions) {
      PaymentService::TransactionRpcInfo transactionInfo = convertTransactionWithTransfersToTransactionRpcInfo(transactionWithTransfers);
      rpcBlock.transactions.push_back(std::move(transactionInfo));
    }

    rpcBlocks.push_back(std::move(rpcBlock));
  }

  return rpcBlocks;
}

std::vector<PaymentService::TransactionHashesInBlockRpcInfo> convertTransactionsInBlockInfoToTransactionHashesInBlockRpcInfo(
    const std::vector<MevaCoin::TransactionsInBlockInfo>& blocks) {

  std::vector<PaymentService::TransactionHashesInBlockRpcInfo> transactionHashes;
  transactionHashes.reserve(blocks.size());
  for (const MevaCoin::TransactionsInBlockInfo& block: blocks) {
    PaymentService::TransactionHashesInBlockRpcInfo item;
    item.blockHash = Common::podToHex(block.blockHash);

    for (const MevaCoin::WalletTransactionWithTransfers& transaction: block.transactions) {
      item.transactionHashes.emplace_back(Common::podToHex(transaction.transaction.hash));
    }

    transactionHashes.push_back(std::move(item));
  }

  return transactionHashes;
}

void validateAddresses(const std::vector<std::string>& addresses, const MevaCoin::Currency& currency, Logging::LoggerRef logger) {
  for (const auto& address: addresses) {
    if (!MevaCoin::validateAddress(address, currency)) {
      logger(Logging::WARNING) << "Can't validate address " << address;
      throw std::system_error(make_error_code(MevaCoin::error::BAD_ADDRESS));
    }
  }
}

std::vector<std::string> collectDestinationAddresses(const std::vector<PaymentService::WalletRpcOrder>& orders) {
  std::vector<std::string> result;

  result.reserve(orders.size());
  for (const auto& order: orders) {
    result.push_back(order.address);
  }

  return result;
}

std::vector<MevaCoin::WalletOrder> convertWalletRpcOrdersToWalletOrders(const std::vector<PaymentService::WalletRpcOrder>& orders) {
  std::vector<MevaCoin::WalletOrder> result;
  result.reserve(orders.size());

  for (const auto& order: orders) {
    result.emplace_back(MevaCoin::WalletOrder {order.address, order.amount});
  }

  return result;
}

}

void createWalletFile(std::fstream& walletFile, const std::string& filename) {
  boost::filesystem::path pathToWalletFile(filename);
  boost::filesystem::path directory = pathToWalletFile.parent_path();
  if (!directory.empty() && !Tools::directoryExists(directory.string())) {
    throw std::runtime_error("Directory does not exist: " + directory.string());
  }

  walletFile.open(filename.c_str(), std::fstream::in | std::fstream::out | std::fstream::binary);
  if (walletFile) {
    walletFile.close();
    throw std::runtime_error("Wallet file already exists");
  }

  walletFile.open(filename.c_str(), std::fstream::out);
  walletFile.close();

  walletFile.open(filename.c_str(), std::fstream::in | std::fstream::out | std::fstream::binary);
}

void saveWallet(MevaCoin::IWallet& wallet, std::fstream& walletFile, bool saveDetailed = true, bool saveCache = true) {
  wallet.save(walletFile, saveDetailed, saveCache);
  walletFile.flush();
}

void secureSaveWallet(MevaCoin::IWallet& wallet, const std::string& path, bool saveDetailed = true, bool saveCache = true) {
  std::fstream tempFile;
  std::string tempFilePath = createTemporaryFile(path, tempFile);

  try {
    saveWallet(wallet, tempFile, saveDetailed, saveCache);
  } catch (std::exception&) {
    deleteFile(tempFilePath);
    tempFile.close();
    throw;
  }
  tempFile.close();

  replaceWalletFiles(path, tempFilePath);
}

void generateNewWallet(const MevaCoin::Currency &currency, const WalletConfiguration &conf, Logging::ILogger& logger, System::Dispatcher& dispatcher) {
  Logging::LoggerRef log(logger, "generateNewWallet");

  MevaCoin::INode* nodeStub = NodeFactory::createNodeStub();
  std::unique_ptr<MevaCoin::INode> nodeGuard(nodeStub);

  MevaCoin::IWallet* wallet = WalletFactory::createWallet(currency, *nodeStub, dispatcher);
  std::unique_ptr<MevaCoin::IWallet> walletGuard(wallet);

  log(Logging::INFO) << "Generating new wallet";

  std::fstream walletFile;
  createWalletFile(walletFile, conf.walletFile);

  wallet->initialize(conf.walletPassword);
  auto address = wallet->createAddress();

  log(Logging::INFO) << "New wallet is generated. Address: " << address;

  saveWallet(*wallet, walletFile, false, false);
  log(Logging::INFO) << "Wallet is saved";
}

void importLegacyKeys(const std::string &legacyKeysFile, const WalletConfiguration &conf) {
  std::stringstream archive;

  MevaCoin::importLegacyKeys(legacyKeysFile, conf.walletPassword, archive);

  std::fstream walletFile;
  createWalletFile(walletFile, conf.walletFile);

  archive.flush();
  walletFile << archive.rdbuf();
  walletFile.flush();
}

WalletService::WalletService(const MevaCoin::Currency& currency, System::Dispatcher& sys, MevaCoin::INode& node,
  MevaCoin::IWallet& wallet, const WalletConfiguration& conf, Logging::ILogger& logger) :
    currency(currency),
    wallet(wallet),
    node(node),
    config(conf),
    inited(false),
    logger(logger, "WalletService"),
    dispatcher(sys),
    readyEvent(dispatcher),
    refreshContext(dispatcher)
{
  readyEvent.set();
}

WalletService::~WalletService() {
  if (inited) {
    wallet.stop();
    refreshContext.wait();
    wallet.shutdown();
  }
}

void WalletService::init() {
  loadWallet();
  loadTransactionIdIndex();

  refreshContext.spawn([this] { refresh(); });

  inited = true;
}

void WalletService::saveWallet() {
  PaymentService::secureSaveWallet(wallet, config.walletFile, true, true);
  logger(Logging::INFO) << "Wallet is saved";
}

void WalletService::loadWallet() {
  std::ifstream inputWalletFile;
  inputWalletFile.open(config.walletFile.c_str(), std::fstream::in | std::fstream::binary);
  if (!inputWalletFile) {
    throw std::runtime_error("Couldn't open wallet file");
  }

  logger(Logging::INFO) << "Loading wallet";

  wallet.load(inputWalletFile, config.walletPassword);

  logger(Logging::INFO) << "Wallet loading is finished.";
}

void WalletService::loadTransactionIdIndex() {
  transactionIdIndex.clear();

  for (size_t i = 0; i < wallet.getTransactionCount(); ++i) {
    transactionIdIndex.emplace(Common::podToHex(wallet.getTransaction(i).hash), i);
  }
}

std::error_code WalletService::resetWallet() {
  try {
    System::EventLock lk(readyEvent);

    logger(Logging::INFO) << "Reseting wallet";

    if (!inited) {
      logger(Logging::WARNING) << "Reset impossible: Wallet Service is not initialized";
      return make_error_code(MevaCoin::error::NOT_INITIALIZED);
    }

    reset();
    logger(Logging::INFO) << "Wallet has been reset";
  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while reseting wallet: " << x.what();
    return x.code();
  } catch (std::exception& x) {
    logger(Logging::WARNING) << "Error while reseting wallet: " << x.what();
    return make_error_code(MevaCoin::error::INTERNAL_WALLET_ERROR);
  }

  return std::error_code();
}

std::error_code WalletService::replaceWithNewWallet(const std::string& viewSecretKeyText) {
  try {
    System::EventLock lk(readyEvent);

    Crypto::SecretKey viewSecretKey;
    if (!Common::podFromHex(viewSecretKeyText, viewSecretKey)) {
      logger(Logging::WARNING) << "Cannot restore view secret key: " << viewSecretKeyText;
      return make_error_code(MevaCoin::error::WalletServiceErrorCode::WRONG_KEY_FORMAT);
    }

    Crypto::PublicKey viewPublicKey;
    if (!Crypto::secret_key_to_public_key(viewSecretKey, viewPublicKey)) {
      logger(Logging::WARNING) << "Cannot derive view public key, wrong secret key: " << viewSecretKeyText;
      return make_error_code(MevaCoin::error::WalletServiceErrorCode::WRONG_KEY_FORMAT);
    }

    replaceWithNewWallet(viewSecretKey);
    logger(Logging::INFO) << "The container has been replaced";
  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while replacing container: " << x.what();
    return x.code();
  } catch (std::exception& x) {
    logger(Logging::WARNING) << "Error while replacing container: " << x.what();
    return make_error_code(MevaCoin::error::INTERNAL_WALLET_ERROR);
  }

  return std::error_code();
}

std::error_code WalletService::createAddress(const std::string& spendSecretKeyText, std::string& address) {
  try {
    System::EventLock lk(readyEvent);

    logger(Logging::DEBUGGING) << "Creating address";

    Crypto::SecretKey secretKey;
    if (!Common::podFromHex(spendSecretKeyText, secretKey)) {
      logger(Logging::WARNING) << "Wrong key format: " << spendSecretKeyText;
      return make_error_code(MevaCoin::error::WalletServiceErrorCode::WRONG_KEY_FORMAT);
    }

    address = wallet.createAddress(secretKey);
  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while creating address: " << x.what();
    return x.code();
  }

  logger(Logging::DEBUGGING) << "Created address " << address;

  return std::error_code();
}

std::error_code WalletService::createAddress(std::string& address) {
  try {
    System::EventLock lk(readyEvent);

    logger(Logging::DEBUGGING) << "Creating address";

    address = wallet.createAddress();
  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while creating address: " << x.what();
    return x.code();
  }

  logger(Logging::DEBUGGING) << "Created address " << address;

  return std::error_code();
}

std::error_code WalletService::createTrackingAddress(const std::string& spendPublicKeyText, std::string& address) {
  try {
    System::EventLock lk(readyEvent);

    logger(Logging::DEBUGGING) << "Creating tracking address";

    Crypto::PublicKey publicKey;
    if (!Common::podFromHex(spendPublicKeyText, publicKey)) {
      logger(Logging::WARNING) << "Wrong key format: " << spendPublicKeyText;
      return make_error_code(MevaCoin::error::WalletServiceErrorCode::WRONG_KEY_FORMAT);
    }

    address = wallet.createAddress(publicKey);
  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while creating tracking address: " << x.what();
    return x.code();
  }

  logger(Logging::DEBUGGING) << "Created address " << address;
  return std::error_code();
}

std::error_code WalletService::deleteAddress(const std::string& address) {
  try {
    System::EventLock lk(readyEvent);

    logger(Logging::DEBUGGING) << "Delete address request came";
    wallet.deleteAddress(address);
  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while deleting address: " << x.what();
    return x.code();
  }

  logger(Logging::DEBUGGING) << "Address " << address << " successfully deleted";
  return std::error_code();
}

std::error_code WalletService::getSpendkeys(const std::string& address, std::string& publicSpendKeyText, std::string& secretSpendKeyText) {
  try {
    System::EventLock lk(readyEvent);

    MevaCoin::KeyPair key = wallet.getAddressSpendKey(address);

    publicSpendKeyText = Common::podToHex(key.publicKey);
    secretSpendKeyText = Common::podToHex(key.secretKey);

  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while getting spend key: " << x.what();
    return x.code();
  }

  return std::error_code();
}

std::error_code WalletService::getBalance(const std::string& address, uint64_t& availableBalance, uint64_t& lockedAmount) {
  try {
    System::EventLock lk(readyEvent);
    logger(Logging::DEBUGGING) << "Getting balance for address " << address;

    availableBalance = wallet.getActualBalance(address);
    lockedAmount = wallet.getPendingBalance(address);
  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while getting balance: " << x.what();
    return x.code();
  }

  logger(Logging::DEBUGGING) << address << " actual balance: " << availableBalance << ", pending: " << lockedAmount;
  return std::error_code();
}

std::error_code WalletService::getBalance(uint64_t& availableBalance, uint64_t& lockedAmount) {
  try {
    System::EventLock lk(readyEvent);
    logger(Logging::DEBUGGING) << "Getting wallet balance";

    availableBalance = wallet.getActualBalance();
    lockedAmount = wallet.getPendingBalance();
  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while getting balance: " << x.what();
    return x.code();
  }

  logger(Logging::DEBUGGING) << "Wallet actual balance: " << availableBalance << ", pending: " << lockedAmount;
  return std::error_code();
}

std::error_code WalletService::getBlockHashes(uint32_t firstBlockIndex, uint32_t blockCount, std::vector<std::string>& blockHashes) {
  try {
    System::EventLock lk(readyEvent);
    std::vector<Crypto::Hash> hashes = wallet.getBlockHashes(firstBlockIndex, blockCount);

    blockHashes.reserve(hashes.size());
    for (const auto& hash: hashes) {
      blockHashes.push_back(Common::podToHex(hash));
    }
  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while getting block hashes: " << x.what();
    return x.code();
  }

  return std::error_code();
}

std::error_code WalletService::getViewKey(std::string& viewSecretKey) {
  try {
    System::EventLock lk(readyEvent);
    MevaCoin::KeyPair viewKey = wallet.getViewKey();
    viewSecretKey = Common::podToHex(viewKey.secretKey);
  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while getting view key: " << x.what();
    return x.code();
  }

  return std::error_code();
}

std::error_code WalletService::getTransactionHashes(const std::vector<std::string>& addresses, const std::string& blockHashString,
  uint32_t blockCount, const std::string& paymentId, std::vector<TransactionHashesInBlockRpcInfo>& transactionHashes) {
  try {
    System::EventLock lk(readyEvent);
    validateAddresses(addresses, currency, logger);

    if (!paymentId.empty()) {
      validatePaymentId(paymentId, logger);
    }

    TransactionsInBlockInfoFilter transactionFilter(addresses, paymentId);
    Crypto::Hash blockHash = parseHash(blockHashString, logger);

    transactionHashes = getRpcTransactionHashes(blockHash, blockCount, transactionFilter);
  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while getting transactions: " << x.what();
    return x.code();
  } catch (std::exception& x) {
    logger(Logging::WARNING) << "Error while getting transactions: " << x.what();
    return make_error_code(MevaCoin::error::INTERNAL_WALLET_ERROR);
  }

  return std::error_code();
}

std::error_code WalletService::getTransactionHashes(const std::vector<std::string>& addresses, uint32_t firstBlockIndex,
  uint32_t blockCount, const std::string& paymentId, std::vector<TransactionHashesInBlockRpcInfo>& transactionHashes) {
  try {
    System::EventLock lk(readyEvent);
    validateAddresses(addresses, currency, logger);

    if (!paymentId.empty()) {
      validatePaymentId(paymentId, logger);
    }

    TransactionsInBlockInfoFilter transactionFilter(addresses, paymentId);
    transactionHashes = getRpcTransactionHashes(firstBlockIndex, blockCount, transactionFilter);

  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while getting transactions: " << x.what();
    return x.code();
  } catch (std::exception& x) {
    logger(Logging::WARNING) << "Error while getting transactions: " << x.what();
    return make_error_code(MevaCoin::error::INTERNAL_WALLET_ERROR);
  }

  return std::error_code();
}

std::error_code WalletService::getTransactions(const std::vector<std::string>& addresses, const std::string& blockHashString,
  uint32_t blockCount, const std::string& paymentId, std::vector<TransactionsInBlockRpcInfo>& transactions) {
  try {
    System::EventLock lk(readyEvent);
    validateAddresses(addresses, currency, logger);

    if (!paymentId.empty()) {
      validatePaymentId(paymentId, logger);
    }

    TransactionsInBlockInfoFilter transactionFilter(addresses, paymentId);

    Crypto::Hash blockHash = parseHash(blockHashString, logger);

    transactions = getRpcTransactions(blockHash, blockCount, transactionFilter);
  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while getting transactions: " << x.what();
    return x.code();
  } catch (std::exception& x) {
    logger(Logging::WARNING) << "Error while getting transactions: " << x.what();
    return make_error_code(MevaCoin::error::INTERNAL_WALLET_ERROR);
  }

  return std::error_code();
}

std::error_code WalletService::getTransactions(const std::vector<std::string>& addresses, uint32_t firstBlockIndex,
  uint32_t blockCount, const std::string& paymentId, std::vector<TransactionsInBlockRpcInfo>& transactions) {
  try {
    System::EventLock lk(readyEvent);
    validateAddresses(addresses, currency, logger);

    if (!paymentId.empty()) {
      validatePaymentId(paymentId, logger);
    }

    TransactionsInBlockInfoFilter transactionFilter(addresses, paymentId);

    transactions = getRpcTransactions(firstBlockIndex, blockCount, transactionFilter);
  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while getting transactions: " << x.what();
    return x.code();
  } catch (std::exception& x) {
    logger(Logging::WARNING) << "Error while getting transactions: " << x.what();
    return make_error_code(MevaCoin::error::INTERNAL_WALLET_ERROR);
  }

  return std::error_code();
}

std::error_code WalletService::getTransaction(const std::string& transactionHash, TransactionRpcInfo& transaction) {
  try {
    System::EventLock lk(readyEvent);
    Crypto::Hash hash = parseHash(transactionHash, logger);

    MevaCoin::WalletTransactionWithTransfers transactionWithTransfers = wallet.getTransaction(hash);

    if (transactionWithTransfers.transaction.state == MevaCoin::WalletTransactionState::DELETED) {
      logger(Logging::WARNING) << "Transaction " << transactionHash << " is deleted";
      return make_error_code(MevaCoin::error::OBJECT_NOT_FOUND);
    }

    transaction = convertTransactionWithTransfersToTransactionRpcInfo(transactionWithTransfers);
  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while getting transaction: " << x.what();
    return x.code();
  } catch (std::exception& x) {
    logger(Logging::WARNING) << "Error while getting transaction: " << x.what();
    return make_error_code(MevaCoin::error::INTERNAL_WALLET_ERROR);
  }

  return std::error_code();
}

std::error_code WalletService::getAddresses(std::vector<std::string>& addresses) {
  try {
    System::EventLock lk(readyEvent);

    addresses.clear();
    addresses.reserve(wallet.getAddressCount());

    for (size_t i = 0; i < wallet.getAddressCount(); ++i) {
      addresses.push_back(wallet.getAddress(i));
    }
  } catch (std::exception& e) {
    logger(Logging::WARNING) << "Can't get addresses: " << e.what();
    return make_error_code(MevaCoin::error::INTERNAL_WALLET_ERROR);
  }

  return std::error_code();
}

std::error_code WalletService::sendTransaction(const SendTransaction::Request& request, std::string& transactionHash) {
  try {
    System::EventLock lk(readyEvent);

    validateAddresses(request.sourceAddresses, currency, logger);
    validateAddresses(collectDestinationAddresses(request.transfers), currency, logger);
    if (!request.changeAddress.empty()) {
      validateAddresses({ request.changeAddress }, currency, logger);
    }

    MevaCoin::TransactionParameters sendParams;
    if (!request.paymentId.empty()) {
      addPaymentIdToExtra(request.paymentId, sendParams.extra);
    } else {
      sendParams.extra = Common::asString(Common::fromHex(request.extra));
    }

    sendParams.sourceAddresses = request.sourceAddresses;
    sendParams.destinations = convertWalletRpcOrdersToWalletOrders(request.transfers);
    sendParams.fee = request.fee;
    sendParams.mixIn = request.anonymity;
    sendParams.unlockTimestamp = request.unlockTime;
    sendParams.changeDestination = request.changeAddress;

    size_t transactionId = wallet.transfer(sendParams);
    transactionHash = Common::podToHex(wallet.getTransaction(transactionId).hash);

    logger(Logging::DEBUGGING) << "Transaction " << transactionHash << " has been sent";
  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while sending transaction: " << x.what();
    return x.code();
  } catch (std::exception& x) {
    logger(Logging::WARNING) << "Error while sending transaction: " << x.what();
    return make_error_code(MevaCoin::error::INTERNAL_WALLET_ERROR);
  }

  return std::error_code();
}

std::error_code WalletService::createDelayedTransaction(const CreateDelayedTransaction::Request& request, std::string& transactionHash) {
  try {
    System::EventLock lk(readyEvent);

    validateAddresses(request.addresses, currency, logger);
    validateAddresses(collectDestinationAddresses(request.transfers), currency, logger);
    if (!request.changeAddress.empty()) {
      validateAddresses({ request.changeAddress }, currency, logger);
    }

    MevaCoin::TransactionParameters sendParams;
    if (!request.paymentId.empty()) {
      addPaymentIdToExtra(request.paymentId, sendParams.extra);
    } else {
      sendParams.extra = Common::asString(Common::fromHex(request.extra));
    }

    sendParams.sourceAddresses = request.addresses;
    sendParams.destinations = convertWalletRpcOrdersToWalletOrders(request.transfers);
    sendParams.fee = request.fee;
    sendParams.mixIn = request.anonymity;
    sendParams.unlockTimestamp = request.unlockTime;
    sendParams.changeDestination = request.changeAddress;

    size_t transactionId = wallet.makeTransaction(sendParams);
    transactionHash = Common::podToHex(wallet.getTransaction(transactionId).hash);

    logger(Logging::DEBUGGING) << "Delayed transaction " << transactionHash << " has been created";
  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while creating delayed transaction: " << x.what();
    return x.code();
  } catch (std::exception& x) {
    logger(Logging::WARNING) << "Error while creating delayed transaction: " << x.what();
    return make_error_code(MevaCoin::error::INTERNAL_WALLET_ERROR);
  }

  return std::error_code();
}

std::error_code WalletService::getDelayedTransactionHashes(std::vector<std::string>& transactionHashes) {
  try {
    System::EventLock lk(readyEvent);

    std::vector<size_t> transactionIds = wallet.getDelayedTransactionIds();
    transactionHashes.reserve(transactionIds.size());

    for (auto id: transactionIds) {
      transactionHashes.emplace_back(Common::podToHex(wallet.getTransaction(id).hash));
    }

  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while getting delayed transaction hashes: " << x.what();
    return x.code();
  } catch (std::exception& x) {
    logger(Logging::WARNING) << "Error while getting delayed transaction hashes: " << x.what();
    return make_error_code(MevaCoin::error::INTERNAL_WALLET_ERROR);
  }

  return std::error_code();
}

std::error_code WalletService::deleteDelayedTransaction(const std::string& transactionHash) {
  try {
    System::EventLock lk(readyEvent);

    parseHash(transactionHash, logger); //validate transactionHash parameter

    auto idIt = transactionIdIndex.find(transactionHash);
    if (idIt == transactionIdIndex.end()) {
      return make_error_code(MevaCoin::error::WalletServiceErrorCode::OBJECT_NOT_FOUND);
    }

    size_t transactionId = idIt->second;
    wallet.rollbackUncommitedTransaction(transactionId);

    logger(Logging::DEBUGGING) << "Delayed transaction " << transactionHash << " has been canceled";
  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while deleting delayed transaction hashes: " << x.what();
    return x.code();
  } catch (std::exception& x) {
    logger(Logging::WARNING) << "Error while deleting delayed transaction hashes: " << x.what();
    return make_error_code(MevaCoin::error::INTERNAL_WALLET_ERROR);
  }

  return std::error_code();
}

std::error_code WalletService::sendDelayedTransaction(const std::string& transactionHash) {
  try {
    System::EventLock lk(readyEvent);

    parseHash(transactionHash, logger); //validate transactionHash parameter

    auto idIt = transactionIdIndex.find(transactionHash);
    if (idIt == transactionIdIndex.end()) {
      return make_error_code(MevaCoin::error::WalletServiceErrorCode::OBJECT_NOT_FOUND);
    }

    size_t transactionId = idIt->second;
    wallet.commitTransaction(transactionId);

    logger(Logging::DEBUGGING) << "Delayed transaction " << transactionHash << " has been sent";
  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while sending delayed transaction hashes: " << x.what();
    return x.code();
  } catch (std::exception& x) {
    logger(Logging::WARNING) << "Error while sending delayed transaction hashes: " << x.what();
    return make_error_code(MevaCoin::error::INTERNAL_WALLET_ERROR);
  }

  return std::error_code();
}

std::error_code WalletService::getUnconfirmedTransactionHashes(const std::vector<std::string>& addresses, std::vector<std::string>& transactionHashes) {
  try {
    System::EventLock lk(readyEvent);

    validateAddresses(addresses, currency, logger);

    std::vector<MevaCoin::WalletTransactionWithTransfers> transactions = wallet.getUnconfirmedTransactions();

    TransactionsInBlockInfoFilter transactionFilter(addresses, "");

    for (const auto& transaction: transactions) {
      if (transactionFilter.checkTransaction(transaction)) {
        transactionHashes.emplace_back(Common::podToHex(transaction.transaction.hash));
      }
    }
  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while getting unconfirmed transaction hashes: " << x.what();
    return x.code();
  } catch (std::exception& x) {
    logger(Logging::WARNING) << "Error while getting unconfirmed transaction hashes: " << x.what();
    return make_error_code(MevaCoin::error::INTERNAL_WALLET_ERROR);
  }

  return std::error_code();
}

std::error_code WalletService::getStatus(uint32_t& blockCount, uint32_t& knownBlockCount, std::string& lastBlockHash, uint32_t& peerCount) {
  try {
    System::EventLock lk(readyEvent);

    knownBlockCount = node.getKnownBlockCount();
    peerCount = node.getPeerCount();
    blockCount = wallet.getBlockCount();

    auto lastHashes = wallet.getBlockHashes(blockCount - 1, 1);
    lastBlockHash = Common::podToHex(lastHashes.back());
  } catch (std::system_error& x) {
    logger(Logging::WARNING) << "Error while getting status: " << x.what();
    return x.code();
  } catch (std::exception& x) {
    logger(Logging::WARNING) << "Error while getting status: " << x.what();
    return make_error_code(MevaCoin::error::INTERNAL_WALLET_ERROR);
  }

  return std::error_code();
}

void WalletService::refresh() {
  try {
    logger(Logging::DEBUGGING) << "Refresh is started";
    for (;;) {
      auto event = wallet.getEvent();
      if (event.type == MevaCoin::TRANSACTION_CREATED) {
        size_t transactionId = event.transactionCreated.transactionIndex;
        transactionIdIndex.emplace(Common::podToHex(wallet.getTransaction(transactionId).hash), transactionId);
      }
    }
  } catch (std::system_error& e) {
    logger(Logging::DEBUGGING) << "refresh is stopped: " << e.what();
  } catch (std::exception& e) {
    logger(Logging::WARNING) << "exception thrown in refresh(): " << e.what();
  }
}

void WalletService::reset() {
  PaymentService::secureSaveWallet(wallet, config.walletFile, false, false);
  wallet.stop();
  wallet.shutdown();
  inited = false;
  refreshContext.wait();

  wallet.start();
  init();
}

void WalletService::replaceWithNewWallet(const Crypto::SecretKey& viewSecretKey) {
  wallet.stop();
  wallet.shutdown();
  inited = false;
  refreshContext.wait();

  transactionIdIndex.clear();

  wallet.start();
  wallet.initializeWithViewKey(viewSecretKey, config.walletPassword);
  inited = true;
}

std::vector<MevaCoin::TransactionsInBlockInfo> WalletService::getTransactions(const Crypto::Hash& blockHash, size_t blockCount) const {
  std::vector<MevaCoin::TransactionsInBlockInfo> result = wallet.getTransactions(blockHash, blockCount);
  if (result.empty()) {
    throw std::system_error(make_error_code(MevaCoin::error::WalletServiceErrorCode::OBJECT_NOT_FOUND));
  }

  return result;
}

std::vector<MevaCoin::TransactionsInBlockInfo> WalletService::getTransactions(uint32_t firstBlockIndex, size_t blockCount) const {
  std::vector<MevaCoin::TransactionsInBlockInfo> result = wallet.getTransactions(firstBlockIndex, blockCount);
  if (result.empty()) {
    throw std::system_error(make_error_code(MevaCoin::error::WalletServiceErrorCode::OBJECT_NOT_FOUND));
  }

  return result;
}

std::vector<TransactionHashesInBlockRpcInfo> WalletService::getRpcTransactionHashes(const Crypto::Hash& blockHash, size_t blockCount, const TransactionsInBlockInfoFilter& filter) const {
  std::vector<MevaCoin::TransactionsInBlockInfo> allTransactions = getTransactions(blockHash, blockCount);
  std::vector<MevaCoin::TransactionsInBlockInfo> filteredTransactions = filterTransactions(allTransactions, filter);
  return convertTransactionsInBlockInfoToTransactionHashesInBlockRpcInfo(filteredTransactions);
}

std::vector<TransactionHashesInBlockRpcInfo> WalletService::getRpcTransactionHashes(uint32_t firstBlockIndex, size_t blockCount, const TransactionsInBlockInfoFilter& filter) const {
  std::vector<MevaCoin::TransactionsInBlockInfo> allTransactions = getTransactions(firstBlockIndex, blockCount);
  std::vector<MevaCoin::TransactionsInBlockInfo> filteredTransactions = filterTransactions(allTransactions, filter);
  return convertTransactionsInBlockInfoToTransactionHashesInBlockRpcInfo(filteredTransactions);
}

std::vector<TransactionsInBlockRpcInfo> WalletService::getRpcTransactions(const Crypto::Hash& blockHash, size_t blockCount, const TransactionsInBlockInfoFilter& filter) const {
  std::vector<MevaCoin::TransactionsInBlockInfo> allTransactions = getTransactions(blockHash, blockCount);
  std::vector<MevaCoin::TransactionsInBlockInfo> filteredTransactions = filterTransactions(allTransactions, filter);
  return convertTransactionsInBlockInfoToTransactionsInBlockRpcInfo(filteredTransactions);
}

std::vector<TransactionsInBlockRpcInfo> WalletService::getRpcTransactions(uint32_t firstBlockIndex, size_t blockCount, const TransactionsInBlockInfoFilter& filter) const {
  std::vector<MevaCoin::TransactionsInBlockInfo> allTransactions = getTransactions(firstBlockIndex, blockCount);
  std::vector<MevaCoin::TransactionsInBlockInfo> filteredTransactions = filterTransactions(allTransactions, filter);
  return convertTransactionsInBlockInfoToTransactionsInBlockRpcInfo(filteredTransactions);
}

} //namespace PaymentService
