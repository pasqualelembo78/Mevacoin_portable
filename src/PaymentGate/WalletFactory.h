// Copyright (c) 2011-2016 The Mevacoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "IWallet.h"
#include "INode.h"
#include <System/Dispatcher.h>

#include <string>
#include <memory>

namespace MevaCoin {
class Currency;
}

namespace PaymentService {

class WalletFactory {
public:
  static MevaCoin::IWallet* createWallet(const MevaCoin::Currency& currency, MevaCoin::INode& node, System::Dispatcher& dispatcher);
private:
  WalletFactory();
  ~WalletFactory();

  static WalletFactory factory;
};

} //namespace PaymentService
