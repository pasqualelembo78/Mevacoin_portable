// Copyright (c) 2011-2016 The Mevacoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "ITransaction.h"
#include <functional>
#include <cstring>

namespace MevaCoin {

inline bool operator==(const AccountPublicAddress &_v1, const AccountPublicAddress &_v2) {
  return memcmp(&_v1, &_v2, sizeof(AccountPublicAddress)) == 0;
}

}

namespace std {

template<>
struct hash < MevaCoin::AccountPublicAddress > {
  size_t operator()(const MevaCoin::AccountPublicAddress& val) const {
    size_t spend = *(reinterpret_cast<const size_t*>(&val.spendPublicKey));
    size_t view = *(reinterpret_cast<const size_t*>(&val.viewPublicKey));
    return spend ^ view;
  }
};

}
