// Copyright (c) 2011-2016 The Mevacoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "PaymentServiceJsonRpcMessages.h"
#include "Serialization/SerializationOverloads.h"

namespace PaymentService {

void Reset::Request::serialize(MevaCoin::ISerializer& serializer) {
  serializer(viewSecretKey, "viewSecretKey");
}

void Reset::Response::serialize(MevaCoin::ISerializer& serializer) {
}

void GetViewKey::Request::serialize(MevaCoin::ISerializer& serializer) {
}

void GetViewKey::Response::serialize(MevaCoin::ISerializer& serializer) {
  serializer(viewSecretKey, "viewSecretKey");
}

void GetStatus::Request::serialize(MevaCoin::ISerializer& serializer) {
}

void GetStatus::Response::serialize(MevaCoin::ISerializer& serializer) {
  serializer(blockCount, "blockCount");
  serializer(knownBlockCount, "knownBlockCount");
  serializer(lastBlockHash, "lastBlockHash");
  serializer(peerCount, "peerCount");
}

void GetAddresses::Request::serialize(MevaCoin::ISerializer& serializer) {
}

void GetAddresses::Response::serialize(MevaCoin::ISerializer& serializer) {
  serializer(addresses, "addresses");
}

void CreateAddress::Request::serialize(MevaCoin::ISerializer& serializer) {
  bool hasSecretKey = serializer(spendSecretKey, "spendSecretKey");
  bool hasPublicKey = serializer(spendPublicKey, "spendPublicKey");

  if (hasSecretKey && hasPublicKey) {
    //TODO: replace it with error codes
    throw RequestSerializationError();
  }
}

void CreateAddress::Response::serialize(MevaCoin::ISerializer& serializer) {
  serializer(address, "address");
}

void DeleteAddress::Request::serialize(MevaCoin::ISerializer& serializer) {
  if (!serializer(address, "address")) {
    throw RequestSerializationError();
  }
}

void DeleteAddress::Response::serialize(MevaCoin::ISerializer& serializer) {
}

void GetSpendKeys::Request::serialize(MevaCoin::ISerializer& serializer) {
  if (!serializer(address, "address")) {
    throw RequestSerializationError();
  }
}

void GetSpendKeys::Response::serialize(MevaCoin::ISerializer& serializer) {
  serializer(spendSecretKey, "spendSecretKey");
  serializer(spendPublicKey, "spendPublicKey");
}

void GetBalance::Request::serialize(MevaCoin::ISerializer& serializer) {
  serializer(address, "address");
}

void GetBalance::Response::serialize(MevaCoin::ISerializer& serializer) {
  serializer(availableBalance, "availableBalance");
  serializer(lockedAmount, "lockedAmount");
}

void GetBlockHashes::Request::serialize(MevaCoin::ISerializer& serializer) {
  bool r = serializer(firstBlockIndex, "firstBlockIndex");
  r &= serializer(blockCount, "blockCount");

  if (!r) {
    throw RequestSerializationError();
  }
}

void GetBlockHashes::Response::serialize(MevaCoin::ISerializer& serializer) {
  serializer(blockHashes, "blockHashes");
}

void TransactionHashesInBlockRpcInfo::serialize(MevaCoin::ISerializer& serializer) {
  serializer(blockHash, "blockHash");
  serializer(transactionHashes, "transactionHashes");
}

void GetTransactionHashes::Request::serialize(MevaCoin::ISerializer& serializer) {
  serializer(addresses, "addresses");

  if (serializer(blockHash, "blockHash") == serializer(firstBlockIndex, "firstBlockIndex")) {
    throw RequestSerializationError();
  }

  if (!serializer(blockCount, "blockCount")) {
    throw RequestSerializationError();
  }

  serializer(paymentId, "paymentId");
}

void GetTransactionHashes::Response::serialize(MevaCoin::ISerializer& serializer) {
  serializer(items, "items");
}

void TransferRpcInfo::serialize(MevaCoin::ISerializer& serializer) {
  serializer(type, "type");
  serializer(address, "address");
  serializer(amount, "amount");
}

void TransactionRpcInfo::serialize(MevaCoin::ISerializer& serializer) {
  serializer(state, "state");
  serializer(transactionHash, "transactionHash");
  serializer(blockIndex, "blockIndex");
  serializer(timestamp, "timestamp");
  serializer(isBase, "isBase");
  serializer(unlockTime, "unlockTime");
  serializer(amount, "amount");
  serializer(fee, "fee");
  serializer(transfers, "transfers");
  serializer(extra, "extra");
  serializer(paymentId, "paymentId");
}

void GetTransaction::Request::serialize(MevaCoin::ISerializer& serializer) {
  if (!serializer(transactionHash, "transactionHash")) {
    throw RequestSerializationError();
  }
}

void GetTransaction::Response::serialize(MevaCoin::ISerializer& serializer) {
  serializer(transaction, "transaction");
}

void TransactionsInBlockRpcInfo::serialize(MevaCoin::ISerializer& serializer) {
  serializer(blockHash, "blockHash");
  serializer(transactions, "transactions");
}

void GetTransactions::Request::serialize(MevaCoin::ISerializer& serializer) {
  serializer(addresses, "addresses");

  if (serializer(blockHash, "blockHash") == serializer(firstBlockIndex, "firstBlockIndex")) {
    throw RequestSerializationError();
  }

  if (!serializer(blockCount, "blockCount")) {
    throw RequestSerializationError();
  }

  serializer(paymentId, "paymentId");
}

void GetTransactions::Response::serialize(MevaCoin::ISerializer& serializer) {
  serializer(items, "items");
}

void GetUnconfirmedTransactionHashes::Request::serialize(MevaCoin::ISerializer& serializer) {
  serializer(addresses, "addresses");
}

void GetUnconfirmedTransactionHashes::Response::serialize(MevaCoin::ISerializer& serializer) {
  serializer(transactionHashes, "transactionHashes");
}

void WalletRpcOrder::serialize(MevaCoin::ISerializer& serializer) {
  bool r = serializer(address, "address");
  r &= serializer(amount, "amount");

  if (!r) {
    throw RequestSerializationError();
  }
}

void SendTransaction::Request::serialize(MevaCoin::ISerializer& serializer) {
  serializer(sourceAddresses, "addresses");

  if (!serializer(transfers, "transfers")) {
    throw RequestSerializationError();
  }

  serializer(changeAddress, "changeAddress");

  if (!serializer(fee, "fee")) {
    throw RequestSerializationError();
  }

  if (!serializer(anonymity, "anonymity")) {
    throw RequestSerializationError();
  }

  bool hasExtra = serializer(extra, "extra");
  bool hasPaymentId = serializer(paymentId, "paymentId");

  if (hasExtra && hasPaymentId) {
    throw RequestSerializationError();
  }

  serializer(unlockTime, "unlockTime");
}

void SendTransaction::Response::serialize(MevaCoin::ISerializer& serializer) {
  serializer(transactionHash, "transactionHash");
}

void CreateDelayedTransaction::Request::serialize(MevaCoin::ISerializer& serializer) {
  serializer(addresses, "addresses");

  if (!serializer(transfers, "transfers")) {
    throw RequestSerializationError();
  }

  serializer(changeAddress, "changeAddress");

  if (!serializer(fee, "fee")) {
    throw RequestSerializationError();
  }

  if (!serializer(anonymity, "anonymity")) {
    throw RequestSerializationError();
  }

  bool hasExtra = serializer(extra, "extra");
  bool hasPaymentId = serializer(paymentId, "paymentId");

  if (hasExtra && hasPaymentId) {
    throw RequestSerializationError();
  }

  serializer(unlockTime, "unlockTime");
}

void CreateDelayedTransaction::Response::serialize(MevaCoin::ISerializer& serializer) {
  serializer(transactionHash, "transactionHash");
}

void GetDelayedTransactionHashes::Request::serialize(MevaCoin::ISerializer& serializer) {
}

void GetDelayedTransactionHashes::Response::serialize(MevaCoin::ISerializer& serializer) {
  serializer(transactionHashes, "transactionHashes");
}

void DeleteDelayedTransaction::Request::serialize(MevaCoin::ISerializer& serializer) {
  if (!serializer(transactionHash, "transactionHash")) {
    throw RequestSerializationError();
  }
}

void DeleteDelayedTransaction::Response::serialize(MevaCoin::ISerializer& serializer) {
}

void SendDelayedTransaction::Request::serialize(MevaCoin::ISerializer& serializer) {
  if (!serializer(transactionHash, "transactionHash")) {
    throw RequestSerializationError();
  }
}

void SendDelayedTransaction::Response::serialize(MevaCoin::ISerializer& serializer) {
}

}
