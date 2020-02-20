// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once

#include <optional>

#include "constants.hpp"
#include "util.hpp"
#include "logs.hpp"
#include "tables.hpp"

namespace eosio_evm
{
  /**
   * Transaction Helpers
   */
  inline bool is_pre_eip_155(size_t v) { return v == 27 || v == 28; }

  inline size_t from_ethereum_recovery_id (size_t v)
  {
    if (is_pre_eip_155(v)) {
      return v - PRE_155_V_START;
    }

    constexpr auto min_valid_v = 37u;
    eosio::check(v >= min_valid_v, "Invalid Transaction: Expected v to encode a valid chain ID");

    const size_t rec_id = (v - POST_155_V_START) % 2;
    const size_t chain_id = ((v - rec_id) - POST_155_V_START) / 2;
    eosio::check(chain_id == CURRENT_CHAIN_ID, "Invalid Transaction: Chain ID mismatch");

    return rec_id;
  }

  struct EthereumTransaction {
    uint256_t nonce;           // A scalar value equal to the number of transactions sent by the sender;
    uint256_t gas_price;       // A scalar value equal to the number of Wei to be paid per unit of gas for all computation costs incurred as a result of the execution of this transaction;
    uint256_t gas_limit;       // A scalar value equal to the maximum amount of gas that should be used in executing this transaction
    std::vector<uint8_t> to;   // Currently set as vector of bytes.
    uint256_t value;           // A scalar value equal to the number of Wei to be transferred to the message call’s recipient or, in the case of contract creation, as an endowment to the newly created account; forma
    std::vector<uint8_t> data; // An unlimited size byte array specifying the input data of the message call

    std::optional<eosio::checksum160> sender; // Address recovered from 1) signature or 2) EOSIO Account Table (authorized by EOSIO account in case 2)
    std::optional<Address> to_address;        // Currently set as 256 bit. The 160-bit address of the message call’s recipient or, for a contract creation transaction, ∅, used here to denote the only member of B0 ;
    std::unique_ptr<Account> sender_account;  // Pointer to sender account
    std::vector<Address> selfdestruct_list;   // SELFDESTRUCT List
    LogHandler log_handler = {};              // Log handler for transaction
    eosio::checksum256 hash = {};             // Log handler for transaction

    uint256_t gas_used;  // Gas used in transaction
    uint256_t gas_refunds;  // Refunds processed in transaction
    std::map<uint256_t, uint256_t> original_storage; // Cache for SSTORE

    // Signature data
    uint8_t v;
    uint256_t r;
    uint256_t s;

    // RLP constructor
    EthereumTransaction(const std::vector<int8_t>& encoded)
    {
      // Max Transaction size
      eosio::check(encoded.size() < MAX_TX_SIZE, "Invalid Transaction: Max size of a transaction is 128 KB");

      // Encoded
      // eosio::print("Encoded: ", bin2hex(encoded));

      // Decode
      auto rlp = rlp::decode(encoded);
      auto values = rlp.values;

      nonce     = from_big_endian(&values[0].value[0], values[0].value.size());
      gas_price = from_big_endian(&values[1].value[0], values[1].value.size());
      gas_limit = from_big_endian(&values[2].value[0], values[2].value.size());
      to        = values[3].value;
      value     = from_big_endian(&values[4].value[0], values[4].value.size());
      data      = values[5].value;
      v         = values[6].value[0];
      r         = from_big_endian(&values[7].value[0], values[7].value.size());
      s         = from_big_endian(&values[8].value[0], values[8].value.size());

      // Validate To Address
      eosio::check(to.empty() || to.size() == 20, "Invalid Transaction: to address must be 40 characters long if provided (excluding 0x prefix)");
      if (!to.empty()) {
        to_address = from_big_endian(to.data(), to.size());
      }

      // Validate Value
      eosio::check(value >= 0 && value <= eosio::asset::max_amount, "Invalid Transaction: Max Value in EOS EVM TX is 2^62 - 1, and it must be positive.");

      // Hash
      // TODO do we need this in prod? how many times are we calling encode() too
      hash = keccak_256(encode());

      // Gas
      initialize_base_gas();
    }
    ~EthereumTransaction() = default;

    int64_t get_value() { return static_cast<int64_t>(value); }
    bool is_zero() { return !r && !s; }
    bool is_create() const { return !to_address.has_value(); }
    uint256_t gas_left() const { return gas_limit - gas_used; }
    std::string encode() const { return rlp::encode(nonce, gas_price, gas_limit, to, value, data, v, r, s); }

    void initialize_base_gas () {
      gas_used = GP_TRANSACTION;

      for (auto& i: data) {
        gas_used += i == 0
          ? GP_TXDATAZERO
          : GP_TXDATANONZERO;
      }
      if (is_create()) {
        gas_used += GP_TXCREATE;
      }

      eosio::check(gas_used <= gas_limit, "Gas limit " + to_string(gas_limit) + " is too low for initialization, minimum " + to_string(gas_used) + " required.");
    }

    void to_recoverable_signature(std::array<uint8_t, 65>& sig) const
    {
      // V
      uint8_t recovery_id = from_ethereum_recovery_id(v);
      sig[0] = recovery_id + 27 + 4; // + 4 as it is compressed

      // R and S valdiation
      eosio::check(s >= 1 && s <= intx::from_string<uint256_t>("0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF5D576E7357A4501DDFE92F46681B20A0"), "Invalid Transaction: s value in signature must be between 1 and 0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF5D576E7357A4501DDFE92F46681B20A0, inclusive.");

      // R and S
      intx::be::unsafe::store(sig.data() + 1, r);
      intx::be::unsafe::store(sig.data() + 1 + R_FIXED_LENGTH, s);
    }

    const KeccakHash to_be_signed()
    {
      if (is_pre_eip_155(v))
      {
        return keccak_256(rlp::encode(nonce, gas_price, gas_limit, to, value, data));
      }
      else
      {
        return keccak_256(rlp::encode( nonce, gas_price, gas_limit, to, value, data, CURRENT_CHAIN_ID, 0, 0 ));
      }
    }

    eosio::checksum160 get_sender()
    {
      // Return early if already exists
      if (sender.has_value()) {
        return *sender;
      }

      // Get transaction hash as checksum256 message
      const std::array<uint8_t, 32u> message = to_be_signed();
      eosio::checksum256 messageChecksum = eosio::fixed_bytes<32>(message);

      // Get recoverable signature
      std::array<uint8_t, 65> arr;
      to_recoverable_signature(arr);

      eosio::ecc_signature ecc_sig;
      std::memcpy(ecc_sig.data(), &arr, sizeof(arr));
      eosio::signature signature = eosio::signature { std::in_place_index<0>, ecc_sig };

      // Recover
      eosio::public_key recovered_variant = eosio::recover_key(messageChecksum, signature);
      eosio::ecc_public_key compressed_public_key = std::get<0>(recovered_variant);
      std::vector<uint8_t> proper_compressed_key( std::begin(compressed_public_key), std::end(compressed_public_key) );

      // Decompress the ETH pubkey
      uint8_t public_key[65];
      public_key[0] = MBEDTLS_ASN1_OCTET_STRING; // 04 prefix
      uECC_decompress(proper_compressed_key.data(), public_key + 1, uECC_secp256k1());

      // Hash key to get address
      std::array<uint8_t, 32u> hashed_key = keccak_256(public_key + 1, 64);

      // Set sender
      sender = toChecksum160(hashed_key);

      return *sender;
    }

    uint256_t from_big_endian(const uint8_t* begin, size_t size = 32u)
    {
        // Size validation
        eosio::check(size <= 32, "Invalid Transaction: Calling from_big_endian with oversized array");

        if (size == 32) {
            return intx::be::unsafe::load<uint256_t>(begin);
        } else {
            uint8_t tmp[32] = {};
            const auto offset = 32 - size;
            memcpy(tmp + offset, begin, size);

            return intx::be::load<uint256_t>(tmp);
        }
    }

    void print() const
    {
      eosio::print(
        "\x1b[36m\n",
        "sender ",   *sender,                           "\n",
        // "data ",      bin2hex(data),                    "\n",
        "data size: ",  data.size(),                    "\n",
        "gasPrice ", static_cast<int128_t>(gas_price), "\n",
        "gasLimit ", static_cast<int128_t>(gas_limit), "\n",
        "nonce ",     intx::hex(nonce),                 "\n",
        "to ",        bin2hex(to),                      "\n",
        "value ",     static_cast<int64_t>(value),      "\n",
        "v ",         v,                                "\n"
        "r ",         intx::hex(r),                     "\n",
        "s ",         intx::hex(s),                     "\n",
        "hash ",      hash,                             "\n",
        "\x1b[0m"
      );
    }
    void printhex() const
    {
      eosio::print(
        // "data ",      bin2hex(data),        "\n",
        "gasLimit ",  intx::hex(gas_limit), "\n",
        "gasPrice ",  intx::hex(gas_price), "\n",
        "nonce ",     intx::hex(nonce),     "\n",
        "to ",        bin2hex(to),          "\n",
        "value ",     intx::hex(value),     "\n",
        "v ",         v,                    "\n"
        "r ",         intx::hex(r),         "\n",
        "s ",         intx::hex(s),         "\n\n",
        "sender ",    *sender,              "\n",
        "hash ",      hash,                 "\n",
        "rlp ",       encode(),             "\n"
      );
    }
    void printEncoded() const
    {
      eosio::print("\n", encode() );
    }
  };
} // namespace eosio_evm