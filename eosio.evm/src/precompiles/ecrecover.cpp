// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License..

#include <eosio.evm/eosio.evm.hpp>
#include <boost/multiprecision/cpp_int.hpp>

namespace eosio_evm
{
  using bmi = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<>>;
  inline void bmi_to_bytes(bmi bigi, std::vector<uint8_t>& bytes) { auto byte_length = bytes.size(); while (byte_length != 0) { bytes[byte_length - 1] = static_cast<uint8_t>(0xff & bigi); bigi >>= 8; byte_length--; } }
  inline bmi bytes_to_bmi(const std::vector<uint8_t>& bytes) { bmi num = 0; for (auto byte: bytes) { num = num << 8 | byte; } return num; }

  void Processor::precompile_ecrecover()
  {
    // Charge gas
    bool error = use_gas(GP_ECRECOVER);
    if (error) return;

    // Get hash
    std::array<uint8_t, 32> output = {};
    std::copy(ctx->input.begin(), ctx->input.begin() + 32, std::begin(output));
    auto hash = eosio::fixed_bytes<32>(output);

    // Get v,r,s
    uint8_t v_bytes[32];
    uint8_t r_bytes[32];
    uint8_t s_bytes[32];
    std::copy(ctx->input.begin() + 32, ctx->input.begin() + 64, std::begin(v_bytes));
    std::copy(ctx->input.begin() + 64, ctx->input.begin() + 96, std::begin(r_bytes));
    std::copy(ctx->input.begin() + 96, ctx->input.begin() + 128, std::begin(s_bytes));

    uint256_t v = intx::be::load<uint256_t>(v_bytes);
    uint256_t r = intx::be::load<uint256_t>(r_bytes);
    uint256_t s = intx::be::load<uint256_t>(s_bytes);

    // Validation
    auto max = intx::from_string<uint256_t>("0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141");
    bool invalid_v = v != PRE_155_V_START && v != (PRE_155_V_START + 1);
    bool invalid_s = s < 1 || s <= 0 || s > max;
    bool invalid_r = r <= 0 || r > max;

    if (invalid_v || invalid_r || invalid_s) {
      precompile_return({});
      return;
    }

    // Convert R from bytes to big int
    std::vector<uint8_t> r_bytes_vec(32);
    std::copy(std::begin(r_bytes), std::begin(r_bytes) + 32, r_bytes_vec.begin());
    bmi x = bytes_to_bmi(r_bytes_vec);

    // y^2 = x^3 + 7 (mod p)
    bmi p("0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2f");
    bmi ysquared = ((x * x * x + 7) % p);
    bmi y = boost::multiprecision::powm(ysquared, (p + 1) / 4, p);
    bmi squared2 = (y * y) % p;
    bool valid = ysquared == squared2;

    if (!valid) {
      precompile_return({});
      return;
    }

    // Get recoverable signature
    std::array<uint8_t, 65> sig;

    // V recovery byte
    uint8_t recovery_id = static_cast<uint8_t>(v - PRE_155_V_START);
    sig[0] = recovery_id + PRE_155_V_START + 4; // + 4 as it is compressed

    // R and S
    intx::be::unsafe::store(sig.data() + 1, r);
    intx::be::unsafe::store(sig.data() + 1 + R_FIXED_LENGTH, s);

    // ECC sig
    eosio::ecc_signature ecc_sig;
    std::memcpy(ecc_sig.data(), &sig, sizeof(sig));
    eosio::signature signature = eosio::signature { std::in_place_index<0>, ecc_sig };

    // Recover
    eosio::public_key recovered_variant = eosio::recover_key(hash, signature);
    eosio::ecc_public_key compressed_public_key = std::get<0>(recovered_variant);
    std::vector<uint8_t> proper_compressed_key( std::begin(compressed_public_key), std::end(compressed_public_key) );

    // Decompress the ETH pubkey
    uint8_t public_key[65];
    public_key[0] = MBEDTLS_ASN1_OCTET_STRING; // 04 prefix
    uECC_decompress(proper_compressed_key.data(), public_key + 1, uECC_secp256k1());

    // Hash key to get address
    std::array<uint8_t, 32u> hashed_key = keccak_256(public_key + 1, 64);

    // Set the result.
    std::vector<uint8_t> result(32);
    std::copy(std::begin(hashed_key) + 12, std::end(hashed_key), std::begin(result) + 12);

    // Return
    precompile_return(result);
  }
} // namespace eosio_evm
