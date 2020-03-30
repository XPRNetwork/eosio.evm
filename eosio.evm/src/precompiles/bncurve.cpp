// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License...

#include <eosio.evm/eosio.evm.hpp>

#if (BN_CURVE == true)

#define MINI_GMP_LIMB_TYPE long long int
#include <mini-gmp/mini-gmp.c>
#define GMP_NUMB_BITS GMP_LIMB_BITS

#include <libff/algebra/curves/alt_bn128/alt_bn128_init.cpp>
#include <libff/algebra/curves/alt_bn128/alt_bn128_pairing.cpp>
#include <libff/algebra/curves/alt_bn128/alt_bn128_pp.cpp>
#include <libff/algebra/curves/alt_bn128/alt_bn128_g1.cpp>
#include <libff/algebra/curves/alt_bn128/alt_bn128_g2.cpp>

using Fq   = libff::alt_bn128_Fq;
using Fq2  = libff::alt_bn128_Fq2;
using Fq12 = libff::alt_bn128_Fq12;
using G1   = libff::alt_bn128_G1;
using G2   = libff::alt_bn128_G2;
using GT   = libff::alt_bn128_GT;
using PP   = libff::alt_bn128_pp;
using LBN  = libff::bigint<libff::alt_bn128_q_limbs>;

namespace eosio_evm
{
  void Processor::precompile_bnadd()
  {
    // Check gas
    bool error = use_gas(GP_BNADD);
    if (error) return;

		// Input
		std::vector<uint8_t> input(ctx->input);
		input.resize(128);

		// Get x and y
    std::array<uint8_t, 32> x1_bytes = {};
    std::array<uint8_t, 32> y1_bytes = {};
    std::array<uint8_t, 32> x2_bytes = {};
    std::array<uint8_t, 32> y2_bytes = {};

		// Copy
    std::copy(input.begin(), input.begin() + 32, std::begin(x1_bytes));
    std::copy(input.begin() + 32, input.begin() + 64, std::begin(y1_bytes));
    std::copy(input.begin() + 64, input.begin() + 96, std::begin(x2_bytes));
    std::copy(input.begin() + 96, input.begin() + 128, std::begin(y2_bytes));

		// Initialize
		if (G2::fixed_base_exp_window_table.size() != 22) {
			PP::init_public_params();
		}

		// First point
		Fq x1 = LBN(x1_bytes);
		Fq y1 = LBN(y1_bytes);
		bool is_zero1 = x1 == Fq::zero() && y1 == Fq::zero();
		G1 point1 = is_zero1 ? G1::zero() : G1(x1, y1, Fq::one());
		if (!point1.is_well_formed()) {
			throw_error(Exception(ET::outOfGas, "Out of Gas"), {});
      return;
		}

		// Second point
		Fq x2 = LBN(x2_bytes);
		Fq y2 = LBN(y2_bytes);
		bool is_zero2 = x2 == Fq::zero() && y2 == Fq::zero();
		G1 point2 = is_zero2 ? G1::zero() : G1(x2, y2, Fq::one());
		if (!point2.is_well_formed()) {
			throw_error(Exception(ET::outOfGas, "Out of Gas"), {});
      return;
		}

		// Add points
		G1 point = point1 + point2;

		// Empty result
		std::vector<uint8_t> result(64);

		// Return early if zero
		if (point.is_zero()) {
			return precompile_return(result);
		}

		// TO AFFINE
		point.to_affine_coordinates();

		// Convert to result
		auto res1 = point.X.as_bigint().as_array();
		auto res2 = point.Y.as_bigint().as_array();

		// Copy Result
    std::copy(std::begin(res1), std::begin(res1) + 32, result.begin());
    std::copy(std::begin(res2), std::begin(res2) + 32, result.begin() + 32);

		// Return
		return precompile_return(result);
  }

  void Processor::precompile_bnmul()
  {
    // Check gas
    bool error = use_gas(GP_BNMUL);
    if (error) return;

		// Input
		std::vector<uint8_t> input(ctx->input);
		input.resize(96);

		// Get x, y and mul
    std::array<uint8_t, 32> x1_bytes = {};
    std::array<uint8_t, 32> y1_bytes = {};
    std::array<uint8_t, 32> mul_bytes = {};

		// Copy
    std::copy(input.begin(), input.begin() + 32, std::begin(x1_bytes));
    std::copy(input.begin() + 32, input.begin() + 64, std::begin(y1_bytes));
    std::copy(input.begin() + 64, input.begin() + 96, std::begin(mul_bytes));

		// Initialize
		if (G2::fixed_base_exp_window_table.size() != 22) {
			PP::init_public_params();
		}

		// Point
		Fq x1 = LBN(x1_bytes);
		Fq y1 = LBN(y1_bytes);
		bool is_zero1 = x1 == Fq::zero() && y1 == Fq::zero();
		G1 point1 = is_zero1 ? G1::zero() : G1(x1, y1, Fq::one());
		if (!point1.is_well_formed()) {
			throw_error(Exception(ET::outOfGas, "Out of Gas"), {});
      return;
		}

		// Mul
		auto mul = LBN(mul_bytes);

		// Empty result
		std::vector<uint8_t> result(64);

		// Multiply points
		G1 point = mul * point1;

		// Return early if zero
		if (point.is_zero()) {
			return precompile_return(result);
		}

		// AFFINE
		point.to_affine_coordinates();

		// Convert to result
		auto res1 = point.X.as_bigint().as_array();
		auto res2 = point.Y.as_bigint().as_array();

		// Copy Result
    std::copy(std::begin(res1), std::begin(res1) + 32, result.begin());
    std::copy(std::begin(res2), std::begin(res2) + 32, result.begin() + 32);

		// Return
		return precompile_return(result);
  }

	void Processor::precompile_bnpairing()
  {
		// Input
		std::vector<uint8_t> input(ctx->input);

		// Parameters
		auto INPUT_SIZE = 192;
		auto K = input.size() / INPUT_SIZE;

		// Check gas
    bool error = use_gas(GP_BNPAIR_BASE + (GP_BNPAIR_K * K));
    if (error) return;

		// Must be multiple of 192 bytes
		if (input.size() % INPUT_SIZE != 0) {
			throw_error(Exception(ET::outOfGas, "Out of Gas"), {});
			return;
		}

		// Empty, always 1 as result
		if (input.size() == 0) {
			std::vector<uint8_t> result(32);
			result[31] = 1;
			return precompile_return(result);
		}

		// Initialize
		if (G2::fixed_base_exp_window_table.size() != 22) {
			PP::init_public_params();
		}

		// Accumulator
		Fq12 accumulator = Fq12::one();

		// Loop
		for (auto i = 0; i < K; i++) {
			std::array<uint8_t, 32> x1_bytes = {};
			std::array<uint8_t, 32> y1_bytes = {};
			std::array<uint8_t, 32> x2_bytes = {};
			std::array<uint8_t, 32> y2_bytes = {};
			std::array<uint8_t, 32> x3_bytes = {};
			std::array<uint8_t, 32> y3_bytes = {};

			auto offset = i * INPUT_SIZE;

			// Copy
			std::copy(input.begin() + offset,         input.begin() + (offset + 32),  std::begin(x1_bytes));
			std::copy(input.begin() + (offset + 32),  input.begin() + (offset + 64),  std::begin(y1_bytes));
			std::copy(input.begin() + (offset + 64),  input.begin() + (offset + 96),  std::begin(x2_bytes));
			std::copy(input.begin() + (offset + 96),  input.begin() + (offset + 128), std::begin(y2_bytes));
			std::copy(input.begin() + (offset + 128), input.begin() + (offset + 160), std::begin(x3_bytes));
			std::copy(input.begin() + (offset + 160), input.begin() + (offset + 192), std::begin(y3_bytes));

			// Initialize bigints
			auto x1_bigint = LBN(x1_bytes);
			auto y1_bigint = LBN(y1_bytes);
			auto x2_bigint = LBN(x2_bytes);
			auto y2_bigint = LBN(y2_bytes);
			auto x3_bigint = LBN(x3_bytes);
			auto y3_bigint = LBN(y3_bytes);

			// Check modmax
			auto modmax = Fq::mod;
			if (
				modmax <= x1_bigint || modmax <= y1_bigint ||
				modmax <= x2_bigint || modmax <= y2_bigint ||
				modmax <= x3_bigint || modmax <= y3_bigint
			) {
				throw_error(Exception(ET::outOfGas, "Greater than modmax"), {});
				return;
			}

			// Convert bigint -> Fq
			Fq x1 = x1_bigint;
			Fq y1 = y1_bigint;
			Fq x2 = x2_bigint;
			Fq y2 = y2_bigint;
			Fq x3 = x3_bigint;
			Fq y3 = y3_bigint;
			Fq2 x4 (y2, x2);
			Fq2 y4 (y3, x3);

			// Form G1
			bool is_zero1 = x1 == Fq::zero() && y1 == Fq::zero();
			G1 point1 = is_zero1 ? G1::zero() : G1(x1, y1, Fq::one());
			if (!is_zero1 && !point1.is_well_formed()) {
				throw_error(Exception(ET::outOfGas, "G1 point not well formed"), {});
				return;
			}

			// Form G2
			bool is_zero2 = x4 == Fq2::zero() && y4 == Fq2::zero();
			G2 point2 = is_zero2 ? G2::zero() : G2(x4, y4, Fq2::one());

			// Validate G2
			bool not_well_formed = !is_zero2 && !point2.is_well_formed();
			auto point2Inverse = -G2::scalar_field::one() * point2;
			bool invalid_inverse = (point2 + point2Inverse) != G2::zero();
			if (not_well_formed || invalid_inverse) {
				throw_error(Exception(ET::outOfGas, "G2 point not well formed"), {});
				return;
			}

			// Skip if zero
			if (point2.is_zero() || point1.is_zero()) continue;

			// Accumulate
			accumulator = accumulator * libff::alt_bn128_miller_loop(libff::alt_bn128_precompute_G1(point1), libff::alt_bn128_precompute_G2(point2));
		}

		// Empty result
		std::vector<uint8_t> result(32);
		result[31] = libff::alt_bn128_final_exponentiation(accumulator) == GT::one();

		// Return
		return precompile_return(result);
  }
} // namespace eosio_evm

#endif /** BN_CURVE == true **/