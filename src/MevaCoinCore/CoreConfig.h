// Copyright (c) 2011-2016 The Mevacoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include <boost/program_options.hpp>

namespace MevaCoin {

class CoreConfig {
public:
  CoreConfig();

  static void initOptions(boost::program_options::options_description& desc);
  void init(const boost::program_options::variables_map& options);

  std::string configFolder;
  bool configFolderDefaulted = true;
};

} //namespace MevaCoin
