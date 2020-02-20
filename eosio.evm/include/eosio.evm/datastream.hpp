// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License.

#pragma once

/**
 * Allows storing uint256_t as checksum
 */
namespace bigint {
  using checksum256 = intx::uint256;
}

namespace eosio_evm {
  /**
   *  Serialize a fixed size int
   *
   *  @param ds - The stream to write
   *  @param v - The value to serialize
   *  @tparam Stream - Type of datastream buffer
   *  @tparam T - Type of the object contained in the array
   *  @tparam N - Number of bits
   *  @return datastream<Stream>& - Reference to the datastream
   */
  template<typename Stream>
  eosio::datastream<Stream>& operator<< ( eosio::datastream<Stream>& ds, const bigint::checksum256& v ) {
      auto bytes = toBin(v);
      ds << bytes;
      return ds;
  }

  /**
   *  Deserialize a fixed size int
   *
   *  @brief Deserialize a fixed size std::array
   *  @param ds - The stream to read
   *  @param v - The destination for deserialized value
   *  @tparam Stream - Type of datastream buffer
   *  @tparam N - Number of bits
   *  @return datastream<Stream>& - Reference to the datastream
   */
  template<typename Stream>
  eosio::datastream<Stream>& operator>> ( eosio::datastream<Stream>& ds, bigint::checksum256& v ) {
      std::array<uint8_t, 32> tmp;
      ds >> tmp;
      v = intx::be::unsafe::load<uint256_t>(tmp.data());
      return ds;
  }
}