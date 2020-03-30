cmake_minimum_required( VERSION 3.5 )

project(eosio_evm)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract(eosio.evm eosio.evm
   ${CMAKE_CURRENT_SOURCE_DIR}/src/eosio.evm.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/processor.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/copy.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/account.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/transfer.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/testing.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/stack.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/instructions.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/precompiles/index.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/precompiles/ecrecover.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/precompiles/sha256.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/precompiles/ripemd160.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/precompiles/identity.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/precompiles/blake2b.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/precompiles/expmod.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/precompiles/bncurve.cpp
)

target_include_directories(eosio.evm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/external
   ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(eosio.evm PRIVATE -D_LIBCPP_STRSTREAM -DBOOST_EXCEPTION_DISABLE=1 -D_GLIBCXX_SSTREAM=1 -D_LIBCPP_SSTREAM -DBOOST_NO_IOSTREAM -DBOOST_NO_STRINGSTREAM )

target_link_options(eosio.evm PUBLIC -stack-size 32000 -use-freeing-malloc)

set_target_properties(eosio.evm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

target_compile_options( eosio.evm PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian)