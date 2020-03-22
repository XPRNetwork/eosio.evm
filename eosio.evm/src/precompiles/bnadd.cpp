// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License...

#define GMP_LIMB_BITS 64
#define GMP_NUMB_BITS GMP_LIMB_BITS
#define MINI_GMP_LIMB_TYPE long long int

#include <eosio.evm/eosio.evm.hpp>
#include <mini-gmp/mini-gmp.c>
#include <libff/algebra/curves/alt_bn128/alt_bn128_init.cpp>
#include <libff/algebra/curves/alt_bn128/alt_bn128_pairing.cpp>
#include <libff/algebra/curves/alt_bn128/alt_bn128_pp.cpp>
#include <libff/algebra/curves/alt_bn128/alt_bn128_g1.cpp>
#include <libff/algebra/curves/alt_bn128/alt_bn128_g2.cpp>

libff::bigint<libff::alt_bn128_q_limbs> toLibsnarkBigint(std::array<uint8_t, 32>& _x)
{
	libff::bigint<libff::alt_bn128_q_limbs> b;
	auto const N = b.N;
	constexpr size_t L = sizeof(b.data[0]);
	static_assert(sizeof(mp_limb_t) == L, "Unexpected limb size in libff::bigint.");
	for (size_t i = 0; i < N; i++)
		for (size_t j = 0; j < L; j++)
			b.data[N - 1 - i] |= mp_limb_t(_x[i * L + j]) << (8 * (L - 1 - j));
	return b;
}

std::array<uint8_t, 32> fromLibsnarkBigint(libff::bigint<libff::alt_bn128_q_limbs> const& _b)
{
	static size_t const N = static_cast<size_t>(_b.N);
	static size_t const L = sizeof(_b.data[0]);
	static_assert(sizeof(mp_limb_t) == L, "Unexpected limb size in libff::bigint.");
	std::array<uint8_t, 32> x;
	for (size_t i = 0; i < N; i++)
		for (size_t j = 0; j < L; j++)
			x[i * L + j] = uint8_t(_b.data[N - 1 - i] >> (8 * (L - 1 - j)));
	return x;
}

namespace eosio_evm
{
  void Processor::precompile_bnadd()
  {
    // Check gas
    bool error = use_gas(GP_BNADD);
    if (error) return;

		// Get x and y
    std::array<uint8_t, 32> x1_bytes = {};
    std::array<uint8_t, 32> y1_bytes = {};
    std::array<uint8_t, 32> x2_bytes = {};
    std::array<uint8_t, 32> y2_bytes = {};

		// Copy
    std::copy(ctx->input.begin(), ctx->input.begin() + 32, std::begin(x1_bytes));
    std::copy(ctx->input.begin() + 32, ctx->input.begin() + 64, std::begin(y1_bytes));
    std::copy(ctx->input.begin() + 64, ctx->input.begin() + 96, std::begin(x2_bytes));
    std::copy(ctx->input.begin() + 96, ctx->input.begin() + 128, std::begin(y2_bytes));

		// Debug
		// eosio::print("\nBNADD X1: ", bin2hex(x1_bytes));
		// eosio::print("\nBNADD Y1: ", bin2hex(y1_bytes));
		// eosio::print("\nBNADD X2: ", bin2hex(x2_bytes));
		// eosio::print("\nBNADD Y2: ", bin2hex(y2_bytes));

		libff::alt_bn128_pp::init_public_params();
		libff::alt_bn128_Fq x1 = toLibsnarkBigint(x1_bytes);
		libff::alt_bn128_Fq y1 = toLibsnarkBigint(y1_bytes);
		libff::alt_bn128_G1 p1(x1, y1, libff::alt_bn128_Fq::one());
		libff::alt_bn128_Fq x2 = toLibsnarkBigint(x2_bytes);
		libff::alt_bn128_Fq y2 = toLibsnarkBigint(y2_bytes);
		libff::alt_bn128_G1 p2(x2, y2, libff::alt_bn128_Fq::one());

		eosio::print("\nBNADD X1: ", bin2hex(fromLibsnarkBigint(p1.X.as_bigint())));
		eosio::print("\nBNADD Y1: ", bin2hex(fromLibsnarkBigint(p1.Y.as_bigint())));
		eosio::print("\nBNADD Z1: ", bin2hex(fromLibsnarkBigint(p1.Z.as_bigint())));
		eosio::print("\nBNADD X2: ", bin2hex(fromLibsnarkBigint(p2.X.as_bigint())));
		eosio::print("\nBNADD Y2: ", bin2hex(fromLibsnarkBigint(p2.Y.as_bigint())));
		eosio::print("\nBNADD Z2: ", bin2hex(fromLibsnarkBigint(p2.Z.as_bigint())));

		// Empty result
		std::vector<uint8_t> result(64);

		// Add points
		libff::alt_bn128_G1 point = p1 + p2;

		// Return early if zero
		if (point.is_zero()) precompile_return(result);

		// AFFINE
		eosio::print("\nBNADD XX1: ", bin2hex(fromLibsnarkBigint(point.X.as_bigint())));
		eosio::print("\nBNADD YY1: ", bin2hex(fromLibsnarkBigint(point.Y.as_bigint())));
		eosio::print("\nBNADD ZZ1: ", bin2hex(fromLibsnarkBigint(point.Z.as_bigint())));
		point.to_affine_coordinates();
		eosio::print("\nBNADD XX1: ", bin2hex(fromLibsnarkBigint(point.X.as_bigint())));
		eosio::print("\nBNADD YY1: ", bin2hex(fromLibsnarkBigint(point.Y.as_bigint())));
		eosio::print("\nBNADD ZZ1: ", bin2hex(fromLibsnarkBigint(point.Z.as_bigint())));

		// Convert to result
		auto res1 = fromLibsnarkBigint(point.X.as_bigint());
		auto res2 = fromLibsnarkBigint(point.Y.as_bigint());

		// Copy Result
    std::copy(std::begin(res1), std::begin(res1) + 32, result.begin());
    std::copy(std::begin(res2), std::begin(res2) + 32, result.begin() + 32);
		eosio::print("\nBNADD R: ", bin2hex(result), "\n");
		eosio::print("\nFsi", sizeof(mp_limb_t));
		// Return
		precompile_return(result);
  }
} // namespace eosio_evm
