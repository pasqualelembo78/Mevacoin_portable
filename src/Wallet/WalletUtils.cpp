// Copyright (c) 2011-2016 The Mevacoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "WalletUtils.h"

#include "MevaCoin.h"

namespace MevaCoin {

bool validateAddress(const std::string& address, const MevaCoin::Currency& currency) {
  MevaCoin::AccountPublicAddress ignore;
  return currency.parseAccountAddressString(address, ignore);
}

}
