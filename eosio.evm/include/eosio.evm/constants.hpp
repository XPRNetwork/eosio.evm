// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License.

#pragma once

// Adds superpower testing functions (required for running cpp tests/clearing data in contract)
#define TESTING true

// Adds bnadd, bnmul, and bnpair precompiles and mini-gmp
#define BN_CURVE true

// Only turn this on when doing ethereum/tests (otherwise balances won't match)
#define CHARGE_SENDER_FOR_GAS false

// Keep all of these off unless testing/debugging
#define PRINT_LOGS false // not really helpful on EOSIO, but kept for completeness.
#define OPTRACE false
#define PRINT_STATE false

// Token constants
#define TOKEN_SYMBOL_CODE_RAW "EOS"
#define TOKEN_CONTRACT_RAW "eosio.token"
#define TOKEN_PRECISION 4
#define TOKEN_ADJUSTMENT uint256_t{pow(10, 18 - TOKEN_PRECISION)}

#define TOKEN_SYMBOL_CODE eosio::symbol_code(TOKEN_SYMBOL_CODE_RAW)
#define TOKEN_CONTRACT eosio::name(TOKEN_CONTRACT_RAW)
#define TOKEN_SYMBOL eosio::symbol(TOKEN_SYMBOL_CODE, TOKEN_PRECISION)

// Crypto
#define MBEDTLS_ASN1_OCTET_STRING 0x04

namespace eosio_evm
{
  struct ChainIDs
  {
    static constexpr size_t PRE_EIP_155          = 0;
    static constexpr size_t ETHEREUM_MAINNET     = 1;
    static constexpr size_t EXPANSE_MAINNET      = 2;
    static constexpr size_t ROPSTEN              = 3;
    static constexpr size_t RINKEBY              = 4;
    static constexpr size_t GOERLI               = 5;
    static constexpr size_t KOVAN                = 42;
    static constexpr size_t GETH_PRIVATE_DEFAULT = 1337;
  };

  // Constant chain ID determined at COMPILE time
  static constexpr size_t CURRENT_CHAIN_ID = ChainIDs::ETHEREUM_MAINNET;

  // Processor constants
  static constexpr auto MAX_CALL_DEPTH  = 1024u;
  static constexpr auto WORD_SIZE       = 32u;
  static constexpr auto MAX_MEM_SIZE    = 1ull << 25; // 32 MB
  constexpr auto MAX_BUFFER_SIZE = std::numeric_limits<uint32_t>::max();

  // Signatures: Start depending on EIP 155
  static constexpr size_t PRE_155_V_START  = 27;
  static constexpr size_t POST_155_V_START = 35;

  // Gas
  // static constexpr uint256_t GAS_PRICE = 1; // Not used since gas price is passed in TX

  static constexpr uint64_t GP_TRANSACTION            = 21000;
  static constexpr uint64_t GP_TXDATAZERO             = 4;
  static constexpr uint64_t GP_TXDATANONZERO          = 16;
  static constexpr uint64_t GP_TXCREATE               = 32000;
  static constexpr uint64_t GP_NEW_ACCOUNT            = 25000;
  static constexpr uint64_t GP_COPY                   = 3;
  static constexpr uint64_t GP_CALL_VALUE_TRANSFER    = 9000;
  static constexpr uint64_t GP_CALL_STIPEND           = 2300;
  static constexpr uint64_t GP_SHA3_WORD              = 6;
  static constexpr uint64_t GP_EXP_BYTE               = 50;
  static constexpr uint64_t GP_CREATE_DATA            = 200;
  static constexpr uint64_t GP_LOG_DATA               = 8;
  static constexpr uint64_t GP_SELFDESTRUCT_REFUND    = 24000;
  static constexpr uint64_t GP_SSTORE_MINIMUM         = 2300;
  static constexpr uint64_t GP_SLOAD_GAS              = 800;
  static constexpr uint64_t GP_SSTORE_SET_GAS         = 20000;
  static constexpr uint64_t GP_SSTORE_RESET_GAS       = 5000;
  static constexpr uint64_t GP_SSTORE_CLEARS_SCHEDULE = 15000;
  static constexpr uint64_t GP_EXTRA_PER_LOG          = 375;

  // Precompiles
  static constexpr uint64_t GP_ECRECOVER              = 3000;
  static constexpr uint64_t GP_IDENTITY_BASE          = 15;
  static constexpr uint64_t GP_IDENTITY_WORD          = 3;
  static constexpr uint64_t GP_SHA256                 = 60;
  static constexpr uint64_t GP_SHA256_WORD            = 12;
  static constexpr uint64_t GP_RIPEMD160              = 600;
  static constexpr uint64_t GP_RIPEMD160_WORD         = 120;
  static constexpr uint64_t GP_BLAKE2_ROUND           = 1;
  static constexpr uint64_t GP_MODEXP                 = 20;
  static constexpr uint64_t GP_BNADD                  = 150;
  static constexpr uint64_t GP_BNMUL                  = 6000;
  static constexpr uint64_t GP_BNPAIR_BASE            = 45000;
  static constexpr uint64_t GP_BNPAIR_K               = 34000;

  // TX
  static constexpr size_t R_FIXED_LENGTH = 32u;
  static constexpr size_t MAX_TX_SIZE    = 128 * 1024;  // Currently 128KB, more after EIP-2464
  static constexpr size_t MAX_CODE_SIZE  = 24 * 1024;   // Currently 24KB, unlimited after EIP 1538
  static constexpr size_t MAX_STACK_SIZE = 1024;
} // namespace eosio_evm