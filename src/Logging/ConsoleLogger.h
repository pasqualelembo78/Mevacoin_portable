// Copyright (c) 2011-2016 The Mevacoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <mutex>
#include "CommonLogger.h"

namespace Logging {

class ConsoleLogger : public CommonLogger {
public:
  ConsoleLogger(Level level = DEBUGGING);

protected:
  virtual void doLogString(const std::string& message) override;

private:
  std::mutex mutex;
};

}
