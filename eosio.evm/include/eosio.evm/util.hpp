// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License..

#pragma once

#include "constants.hpp"

namespace eosio_evm
{
  /**
   * Conversions
   */
  static inline std::string bin2hex(const std::vector<uint8_t>& bin)
  {
    std::string res;
    const char hex[] = "0123456789abcdef";
    for(auto byte : bin) {
      res += hex[byte >> 4];
      res += hex[byte & 0xf];
    }

    return res;
  }

  template<unsigned N, typename T>
  static inline std::string bin2hex(const std::array<T, N>& bin)
  {
    std::string res;
    const char hex[] = "0123456789abcdef";
    for(auto byte : bin) {
      res += hex[byte >> 4];
      res += hex[byte & 0xf];
    }

    return res;
  }

  inline constexpr bool is_precompile(uint256_t address) {
    return address >= 1 && address <= 65535;
  }

  inline constexpr int64_t num_words(uint64_t size_in_bytes)
  {
    return (static_cast<int64_t>(size_in_bytes) + (WORD_SIZE - 1)) / WORD_SIZE;
  }

  template <typename T>
  static T shrink(uint256_t i)
  {
    return static_cast<T>(i & std::numeric_limits<T>::max());
  }

  inline std::array<uint8_t, 32u> toBin(const Address& address)
  {
    std::array<uint8_t, 32> address_bytes = {};
    intx::be::unsafe::store(address_bytes.data(), address);
    return address_bytes;
  }

  inline const std::array<uint8_t, 32u> fromChecksum160(const eosio::checksum160 input)
  {
    std::array<uint8_t, 32U> output = {};
    auto input_bytes = input.extract_as_byte_array();
    std::copy(std::begin(input_bytes), std::end(input_bytes), std::begin(output) + 12);
    return output;
  }

  inline eosio::checksum160 toChecksum160(const std::array<uint8_t, 32u>& input)
  {
    std::array<uint8_t, 20> output = {};
    std::copy(std::begin(input) + 12, std::end(input), std::begin(output));
    return eosio::checksum160(output);
  }

  inline eosio::checksum256 toChecksum256(const Address& address)
  {
    return eosio::checksum256( toBin(address) );
  }

  static inline eosio::checksum256 pad160(const eosio::checksum160 input)
  {
    return eosio::checksum256( fromChecksum160(input) );
  }

  static inline Address checksum160ToAddress(const eosio::checksum160& input) {
    const std::array<uint8_t, 32u>& checksum = fromChecksum160(input);
    return intx::be::unsafe::load<uint256_t>(checksum.data());
  }
  static inline eosio::checksum160 addressToChecksum160(const Address& input) {
    return toChecksum160( toBin(input) );
  }

  // Do not use for addresses, only key for Account States
  static inline uint256_t checksum256ToValue(const eosio::checksum256& input) {
    std::array<uint8_t, 32U> output = {};
    auto input_bytes = input.extract_as_byte_array();
    std::copy(std::begin(input_bytes), std::end(input_bytes), std::begin(output));

    return intx::be::unsafe::load<uint256_t>(output.data());
  }

  /**
   * Keccak (SHA3) Functions
   */
  inline void keccak_256(
    const unsigned char* input,
    unsigned int inputByteLen,
    unsigned char* output)
  {
    // Ethereum started using Keccak and called it SHA3 before it was finalised.
    SHA3_CTX context;
    keccak_init(&context);
    keccak_update(&context, input, inputByteLen);
    keccak_final(&context, output);
  }

  using KeccakHash = std::array<uint8_t, 32u>;

  inline KeccakHash keccak_256(const uint8_t* begin, size_t byte_len)
  {
    KeccakHash h;
    keccak_256(begin, byte_len, h.data());
    return h;
  }

  inline KeccakHash keccak_256(const std::string& s)
  {
    return keccak_256((const uint8_t*)s.data(), s.size());
  }

  inline KeccakHash keccak_256(const std::vector<uint8_t>& v)
  {
    return keccak_256(v.data(), v.size());
  }

  template <size_t N>
  inline KeccakHash keccak_256(const std::array<uint8_t, N>& a)
  {
    return keccak_256(a.data(), N);
  }

  /**
   * RLP
   */
  inline Address generate_address(const Address& sender, uint256_t nonce) {
    // RLP encode and keccak hash
    const auto rlp_encoding = rlp::encode(sender, nonce);
    std::array<uint8_t, 32u> buffer = keccak_256(rlp_encoding);

    // Copy right 160 bits from keccak buffer
    uint8_t right_160[32] = {};
    memcpy(right_160 + 12u, buffer.data() + 12u, 20u);

    // Return as address
    return intx::be::load<uint256_t>(right_160);
  };

  inline Address generate_address2(const Address& sender, const uint256_t& salt, const std::vector<uint8_t>& init_code) {
    // Sender as bytes
    auto sender_bytes = toBin(sender);

    // Code hash
    std::array<uint8_t, 32u> code_hash = keccak_256(init_code);

    // Salt array
    uint8_t salt_bytes[32] = {};
    intx::be::store(salt_bytes, salt);

    // Concatenate
    std::array<uint8_t, 85u> final = {0xff};
    std::copy(std::begin(sender_bytes) + 12, std::end(sender_bytes), std::begin(final) + 1);
    std::copy(salt_bytes, salt_bytes + 32, std::begin(final) + 21);
    std::copy(std::begin(code_hash), std::end(code_hash), std::begin(final) + 53);

    // Final hash
    auto final_hash = keccak_256(final);

    // Copy right 160 bits from keccak buffer
    uint8_t right_160[32] = {};
    memcpy(right_160 + 12u, final_hash.data() + 12u, 20u);

    // Return as address
    return intx::be::load<uint256_t>(right_160);
  };
} // namespace eosio_evm
