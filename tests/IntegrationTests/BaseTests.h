// Copyright (c) 2011-2016 The Mevacoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <gtest/gtest.h>
#include <future>

#include <Logging/ConsoleLogger.h>
#include <System/Dispatcher.h>
#include "MevaCoinCore/Currency.h"

#include "../IntegrationTestLib/TestNetwork.h"

namespace Tests {

class BaseTest : public testing::Test {
public:

  BaseTest() :
    currency(MevaCoin::CurrencyBuilder(logger).testnet(true).currency()),
    network(dispatcher, currency) {
  }

protected:

  virtual void TearDown() override {
    network.shutdown();
  }

  System::Dispatcher& getDispatcher() {
    return dispatcher;
  }

  System::Dispatcher dispatcher;
  Logging::ConsoleLogger logger;
  MevaCoin::Currency currency;
  TestNetwork network;
};

}
