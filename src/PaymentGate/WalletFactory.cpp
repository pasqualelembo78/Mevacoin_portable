// Copyright (c) 2011-2016 The Mevacoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "WalletFactory.h"

#include "NodeRpcProxy/NodeRpcProxy.h"
#include "Wallet/WalletGreen.h"
#include "MevaCoinCore/Currency.h"

#include <stdlib.h>
#include <future>

namespace PaymentService {

WalletFactory WalletFactory::factory;

WalletFactory::WalletFactory() {
}

WalletFactory::~WalletFactory() {
}

MevaCoin::IWallet* WalletFactory::createWallet(const MevaCoin::Currency& currency, MevaCoin::INode& node, System::Dispatcher& dispatcher) {
  MevaCoin::IWallet* wallet = new MevaCoin::WalletGreen(dispatcher, currency, node);
  return wallet;
}

}
