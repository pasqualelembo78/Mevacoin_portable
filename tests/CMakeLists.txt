add_definitions(-DSTATICLIB)

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} ../version)

file(GLOB_RECURSE CoreTests CoreTests/*)
file(GLOB_RECURSE CryptoTests crypto/*)
file(GLOB_RECURSE FunctionalTests FunctionalTests/*)
file(GLOB_RECURSE IntegrationTestLibrary IntegrationTestLib/*)
file(GLOB_RECURSE IntegrationTests IntegrationTests/*)
file(GLOB_RECURSE NodeRpcProxyTests NodeRpcProxyTests/*)
file(GLOB_RECURSE PerformanceTests PerformanceTests/*)
file(GLOB_RECURSE SystemTests System/*)
file(GLOB_RECURSE TestGenerator TestGenerator/*)
file(GLOB_RECURSE TransfersTests TransfersTests/*)
file(GLOB_RECURSE UnitTests UnitTests/*)

file(GLOB_RECURSE MevaCoinProtocol ../src/MevaCoinProtocol/*)
file(GLOB_RECURSE P2p ../src/P2p/*)

source_group("" FILES ${CoreTests} ${CryptoTests} ${FunctionalTests} ${IntegrationTestLibrary} ${IntegrationTests} ${NodeRpcProxyTests} ${PerformanceTests} ${SystemTests} ${TestGenerator} ${TransfersTests} ${UnitTests})
source_group("" FILES ${MevaCoinProtocol} ${P2p})

add_library(IntegrationTestLibrary ${IntegrationTestLibrary})
add_library(TestGenerator ${TestGenerator})

add_executable(CoreTests ${CoreTests})
add_executable(CryptoTests ${CryptoTests})
add_executable(IntegrationTests ${IntegrationTests})
add_executable(NodeRpcProxyTests ${NodeRpcProxyTests})
add_executable(PerformanceTests ${PerformanceTests})
add_executable(SystemTests ${SystemTests})
add_executable(TransfersTests ${TransfersTests})
add_executable(UnitTests ${UnitTests})

add_executable(DifficultyTests Difficulty/Difficulty.cpp)
add_executable(HashTargetTests HashTarget.cpp)
add_executable(HashTests Hash/main.cpp)

target_link_libraries(CoreTests TestGenerator MevaCoinCore Serialization System Logging Common Crypto BlockchainExplorer ${Boost_LIBRARIES})
target_link_libraries(IntegrationTests IntegrationTestLibrary Wallet NodeRpcProxy InProcessNode P2P Rpc Http Transfers Serialization System MevaCoinCore Logging Common Crypto BlockchainExplorer gtest upnpc-static ${Boost_LIBRARIES})
target_link_libraries(NodeRpcProxyTests NodeRpcProxy MevaCoinCore Rpc Http Serialization System Logging Common Crypto ${Boost_LIBRARIES})
target_link_libraries(PerformanceTests MevaCoinCore Serialization Logging Common Crypto ${Boost_LIBRARIES})
target_link_libraries(SystemTests System gtest_main)
if (MSVC)
  target_link_libraries(SystemTests ws2_32)
  target_link_libraries(NodeRpcProxyTests ws2_32)
  target_link_libraries(CoreTests ws2_32)
endif ()

target_link_libraries(TransfersTests IntegrationTestLibrary Wallet gtest_main InProcessNode NodeRpcProxy P2P Rpc Http BlockchainExplorer MevaCoinCore Serialization System Logging Transfers Common Crypto upnpc-static ${Boost_LIBRARIES})
target_link_libraries(UnitTests gtest_main PaymentGate Wallet TestGenerator InProcessNode NodeRpcProxy Rpc Http Transfers Serialization System Logging BlockchainExplorer Common MevaCoinCore Crypto ${Boost_LIBRARIES})

target_link_libraries(DifficultyTests MevaCoinCore Serialization Crypto Logging Common ${Boost_LIBRARIES})
target_link_libraries(HashTargetTests MevaCoinCore Crypto)
target_link_libraries(HashTests Crypto)

if(NOT MSVC)
  set_property(TARGET gtest gtest_main IntegrationTestLibrary IntegrationTests TestGenerator UnitTests SystemTests HashTargetTests TransfersTests APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-undef -Wno-sign-compare")
endif()

add_custom_target(tests DEPENDS CoreTests IntegrationTests NodeRpcProxyTests PerformanceTests SystemTests TransfersTests UnitTests DifficultyTests HashTargetTests)

set_property(TARGET
  tests

  IntegrationTestLibrary
  TestGenerator

  CoreTests
  CryptoTests
  IntegrationTests
  NodeRpcProxyTests
  PerformanceTests
  SystemTests
  TransfersTests
  UnitTests

  DifficultyTests
  HashTargetTests
  HashTests
PROPERTY FOLDER "tests")

add_dependencies(IntegrationTestLibrary version)

set_property(TARGET CoreTests PROPERTY OUTPUT_NAME "core_tests")
set_property(TARGET CryptoTests PROPERTY OUTPUT_NAME "crypto_tests")
set_property(TARGET IntegrationTests PROPERTY OUTPUT_NAME "integration_tests")
set_property(TARGET NodeRpcProxyTests PROPERTY OUTPUT_NAME "node_rpc_proxy_tests")
set_property(TARGET PerformanceTests PROPERTY OUTPUT_NAME "performance_tests")
set_property(TARGET SystemTests PROPERTY OUTPUT_NAME "system_tests")
set_property(TARGET TransfersTests PROPERTY OUTPUT_NAME "transfers_tests")
set_property(TARGET UnitTests PROPERTY OUTPUT_NAME "unit_tests")
set_property(TARGET DifficultyTests PROPERTY OUTPUT_NAME "difficulty_tests")
set_property(TARGET HashTargetTests PROPERTY OUTPUT_NAME "hash_target_tests")
set_property(TARGET HashTests PROPERTY OUTPUT_NAME "hash_tests")

add_test(CoreTests core_tests --generate_and_play_test_data)
add_test(CryptoTests crypto_tests ${CMAKE_CURRENT_SOURCE_DIR}/crypto/tests.txt)
add_test(DifficultyTests difficulty_tests ${CMAKE_CURRENT_SOURCE_DIR}/Difficulty/data.txt)
foreach(hash IN ITEMS fast slow tree extra-blake extra-groestl extra-jh extra-skein)
  add_test(hash-${hash} hash_tests ${hash} ${CMAKE_CURRENT_SOURCE_DIR}/Hash/tests-${hash}.txt)
endforeach(hash)
add_test(HashTargetTests hash_target_tests)
add_test(SystemTests system_tests)
add_test(UnitTests unit_tests)
