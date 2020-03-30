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

set(FLAGS USE_FIELD_5X52 USE_SCALAR_4X64 HAVE_BUILTIN_EXPECT HAVE___INT128 ENABLE_MODULE_RECOVERY USE_FIELD_INV_BUILTIN USE_NUM_NONE USE_SCALAR_INV_BUILTIN )
target_compile_definitions(eosio.evm PRIVATE -DECMULT_WINDOW_SIZE=15 -DECMULT_GEN_PREC_BITS=4 ${FLAGS})

target_link_options(eosio.evm PUBLIC -stack-size 32000 -use-freeing-malloc)

set_target_properties(eosio.evm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

target_compile_options( eosio.evm PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian)