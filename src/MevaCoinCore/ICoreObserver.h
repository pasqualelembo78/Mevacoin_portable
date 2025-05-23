// Copyright (c) 2011-2016 The Mevacoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

namespace MevaCoin {

class ICoreObserver {
public:
  virtual ~ICoreObserver() {};
  virtual void blockchainUpdated() {};
  virtual void poolUpdated() {};
};

}
