// Copyright (c) 2011-2016 The Mevacoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "MevaCoinSerialization.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

#include "Serialization/ISerializer.h"
#include "Serialization/SerializationOverloads.h"
#include "Serialization/BinaryInputStreamSerializer.h"
#include "Serialization/BinaryOutputStreamSerializer.h"

#include "Common/StringOutputStream.h"
#include "crypto/crypto.h"

#include "Account.h"
#include "MevaCoinConfig.h"
#include "MevaCoinFormatUtils.h"
#include "MevaCoinTools.h"
#include "TransactionExtra.h"

using namespace Common;

namespace {

using namespace MevaCoin;
using namespace Common;

size_t getSignaturesCount(const TransactionInput& input) {
  struct txin_signature_size_visitor : public boost::static_visitor < size_t > {
    size_t operator()(const BaseInput& txin) const { return 0; }
    size_t operator()(const KeyInput& txin) const { return txin.outputIndexes.size(); }
    size_t operator()(const MultisignatureInput& txin) const { return txin.signatureCount; }
  };

  return boost::apply_visitor(txin_signature_size_visitor(), input);
}

struct BinaryVariantTagGetter: boost::static_visitor<uint8_t> {
  uint8_t operator()(const MevaCoin::BaseInput) { return  0xff; }
  uint8_t operator()(const MevaCoin::KeyInput) { return  0x2; }
  uint8_t operator()(const MevaCoin::MultisignatureInput) { return  0x3; }
  uint8_t operator()(const MevaCoin::KeyOutput) { return  0x2; }
  uint8_t operator()(const MevaCoin::MultisignatureOutput) { return  0x3; }
  uint8_t operator()(const MevaCoin::Transaction) { return  0xcc; }
  uint8_t operator()(const MevaCoin::Block) { return  0xbb; }
};

struct VariantSerializer : boost::static_visitor<> {
  VariantSerializer(MevaCoin::ISerializer& serializer, const std::string& name) : s(serializer), name(name) {}

  template <typename T>
  void operator() (T& param) { s(param, name); }

  MevaCoin::ISerializer& s;
  std::string name;
};

void getVariantValue(MevaCoin::ISerializer& serializer, uint8_t tag, MevaCoin::TransactionInput& in) {
  switch(tag) {
  case 0xff: {
    MevaCoin::BaseInput v;
    serializer(v, "value");
    in = v;
    break;
  }
  case 0x2: {
    MevaCoin::KeyInput v;
    serializer(v, "value");
    in = v;
    break;
  }
  case 0x3: {
    MevaCoin::MultisignatureInput v;
    serializer(v, "value");
    in = v;
    break;
  }
  default:
    throw std::runtime_error("Unknown variant tag");
  }
}

void getVariantValue(MevaCoin::ISerializer& serializer, uint8_t tag, MevaCoin::TransactionOutputTarget& out) {
  switch(tag) {
  case 0x2: {
    MevaCoin::KeyOutput v;
    serializer(v, "data");
    out = v;
    break;
  }
  case 0x3: {
    MevaCoin::MultisignatureOutput v;
    serializer(v, "data");
    out = v;
    break;
  }
  default:
    throw std::runtime_error("Unknown variant tag");
  }
}

template <typename T>
bool serializePod(T& v, Common::StringView name, MevaCoin::ISerializer& serializer) {
  return serializer.binary(&v, sizeof(v), name);
}

bool serializeVarintVector(std::vector<uint32_t>& vector, MevaCoin::ISerializer& serializer, Common::StringView name) {
  size_t size = vector.size();
  
  if (!serializer.beginArray(size, name)) {
    vector.clear();
    return false;
  }

  vector.resize(size);

  for (size_t i = 0; i < size; ++i) {
    serializer(vector[i], "");
  }

  serializer.endArray();
  return true;
}

}

namespace Crypto {

bool serialize(PublicKey& pubKey, Common::StringView name, MevaCoin::ISerializer& serializer) {
  return serializePod(pubKey, name, serializer);
}

bool serialize(SecretKey& secKey, Common::StringView name, MevaCoin::ISerializer& serializer) {
  return serializePod(secKey, name, serializer);
}

bool serialize(Hash& h, Common::StringView name, MevaCoin::ISerializer& serializer) {
  return serializePod(h, name, serializer);
}

bool serialize(KeyImage& keyImage, Common::StringView name, MevaCoin::ISerializer& serializer) {
  return serializePod(keyImage, name, serializer);
}

bool serialize(chacha8_iv& chacha, Common::StringView name, MevaCoin::ISerializer& serializer) {
  return serializePod(chacha, name, serializer);
}

bool serialize(Signature& sig, Common::StringView name, MevaCoin::ISerializer& serializer) {
  return serializePod(sig, name, serializer);
}

bool serialize(EllipticCurveScalar& ecScalar, Common::StringView name, MevaCoin::ISerializer& serializer) {
  return serializePod(ecScalar, name, serializer);
}

bool serialize(EllipticCurvePoint& ecPoint, Common::StringView name, MevaCoin::ISerializer& serializer) {
  return serializePod(ecPoint, name, serializer);
}

}

namespace MevaCoin {

void serialize(TransactionPrefix& txP, ISerializer& serializer) {
  serializer(txP.version, "version");

  if (CURRENT_TRANSACTION_VERSION < txP.version) {
    throw std::runtime_error("Wrong transaction version");
  }

  serializer(txP.unlockTime, "unlock_time");
  serializer(txP.inputs, "vin");
  serializer(txP.outputs, "vout");
  serializeAsBinary(txP.extra, "extra", serializer);
}

void serialize(Transaction& tx, ISerializer& serializer) {
  serialize(static_cast<TransactionPrefix&>(tx), serializer);

  size_t sigSize = tx.inputs.size();
  //TODO: make arrays without sizes
//  serializer.beginArray(sigSize, "signatures");
  
  if (serializer.type() == ISerializer::INPUT) {
    tx.signatures.resize(sigSize);
  }

  bool signaturesNotExpected = tx.signatures.empty();
  if (!signaturesNotExpected && tx.inputs.size() != tx.signatures.size()) {
    throw std::runtime_error("Serialization error: unexpected signatures size");
  }

  for (size_t i = 0; i < tx.inputs.size(); ++i) {
    size_t signatureSize = getSignaturesCount(tx.inputs[i]);
    if (signaturesNotExpected) {
      if (signatureSize == 0) {
        continue;
      } else {
        throw std::runtime_error("Serialization error: signatures are not expected");
      }
    }

    if (serializer.type() == ISerializer::OUTPUT) {
      if (signatureSize != tx.signatures[i].size()) {
        throw std::runtime_error("Serialization error: unexpected signatures size");
      }

      for (Crypto::Signature& sig : tx.signatures[i]) {
        serializePod(sig, "", serializer);
      }

    } else {
      std::vector<Crypto::Signature> signatures(signatureSize);
      for (Crypto::Signature& sig : signatures) {
        serializePod(sig, "", serializer);
      }

      tx.signatures[i] = std::move(signatures);
    }
  }
//  serializer.endArray();
}

void serialize(TransactionInput& in, ISerializer& serializer) {
  if (serializer.type() == ISerializer::OUTPUT) {
    BinaryVariantTagGetter tagGetter;
    uint8_t tag = boost::apply_visitor(tagGetter, in);
    serializer.binary(&tag, sizeof(tag), "type");

    VariantSerializer visitor(serializer, "value");
    boost::apply_visitor(visitor, in);
  } else {
    uint8_t tag;
    serializer.binary(&tag, sizeof(tag), "type");

    getVariantValue(serializer, tag, in);
  }
}

void serialize(BaseInput& gen, ISerializer& serializer) {
  serializer(gen.blockIndex, "height");
}

void serialize(KeyInput& key, ISerializer& serializer) {
  serializer(key.amount, "amount");
  serializeVarintVector(key.outputIndexes, serializer, "key_offsets");
  serializer(key.keyImage, "k_image");
}

void serialize(MultisignatureInput& multisignature, ISerializer& serializer) {
  serializer(multisignature.amount, "amount");
  serializer(multisignature.signatureCount, "signatures");
  serializer(multisignature.outputIndex, "outputIndex");
}

void serialize(TransactionOutput& output, ISerializer& serializer) {
  serializer(output.amount, "amount");
  serializer(output.target, "target");
}

void serialize(TransactionOutputTarget& output, ISerializer& serializer) {
  if (serializer.type() == ISerializer::OUTPUT) {
    BinaryVariantTagGetter tagGetter;
    uint8_t tag = boost::apply_visitor(tagGetter, output);
    serializer.binary(&tag, sizeof(tag), "type");

    VariantSerializer visitor(serializer, "data");
    boost::apply_visitor(visitor, output);
  } else {
    uint8_t tag;
    serializer.binary(&tag, sizeof(tag), "type");

    getVariantValue(serializer, tag, output);
  }
}

void serialize(KeyOutput& key, ISerializer& serializer) {
  serializer(key.key, "key");
}

void serialize(MultisignatureOutput& multisignature, ISerializer& serializer) {
  serializer(multisignature.keys, "keys");
  serializer(multisignature.requiredSignatureCount, "required_signatures");
}

void serializeBlockHeader(BlockHeader& header, ISerializer& serializer) {
  serializer(header.majorVersion, "major_version");
  if (header.majorVersion > BLOCK_MAJOR_VERSION_1) {
    throw std::runtime_error("Wrong major version");
  }

  serializer(header.minorVersion, "minor_version");
  serializer(header.timestamp, "timestamp");
  serializer(header.previousBlockHash, "prev_id");
  serializer.binary(&header.nonce, sizeof(header.nonce), "nonce");
}

void serialize(BlockHeader& header, ISerializer& serializer) {
  serializeBlockHeader(header, serializer);
}

void serialize(Block& block, ISerializer& serializer) {
  serializeBlockHeader(block, serializer);

  serializer(block.baseTransaction, "miner_tx");
  serializer(block.transactionHashes, "tx_hashes");
}

void serialize(AccountPublicAddress& address, ISerializer& serializer) {
  serializer(address.spendPublicKey, "m_spend_public_key");
  serializer(address.viewPublicKey, "m_view_public_key");
}

void serialize(AccountKeys& keys, ISerializer& s) {
  s(keys.address, "m_account_address");
  s(keys.spendSecretKey, "m_spend_secret_key");
  s(keys.viewSecretKey, "m_view_secret_key");
}

void serialize(KeyPair& keyPair, ISerializer& serializer) {
  serializer(keyPair.secretKey, "secret_key");
  serializer(keyPair.publicKey, "public_key");
}


} //namespace MevaCoin
