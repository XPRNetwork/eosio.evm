cmake_minimum_required(VERSION 3.5)

list( APPEND CMAKE_MODULE_PATH ${EOSIO_CDT_BIN} )
include( EosioCDTMacros )

add_native_executable( evm_test evm_test.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/../src/eosio.evm.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/../src/processor.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/../src/state.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/../src/transfer.cpp
)
target_include_directories(evm_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include ${CMAKE_CURRENT_SOURCE_DIR}/../external)
