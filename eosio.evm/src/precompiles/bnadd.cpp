// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License...

#define CYBOZU_DONT_USE_EXCEPTION
#define MCL_USE_VINT
#define CYBOZU_DONT_USE_STRING 1
#define MCL_DONT_USE_CSPRNG 1

#include <eosio.evm/eosio.evm.hpp>
#include <mcl/bn256.hpp>
#include <mcl/src/fp.cpp>

using namespace mcl::bn256;

namespace eosio_evm
{
  void Processor::precompile_bnadd()
  {
    // Check gas
    bool error = use_gas(GP_BNADD);
    if (error) return;

		// Get x and y
    uint8_t x1_bytes[32];
    uint8_t y1_bytes[32];
    uint8_t x2_bytes[32];
    uint8_t y2_bytes[32];

		// Copy
    std::copy(ctx->input.begin(), ctx->input.begin() + 32, std::begin(x1_bytes));
    std::copy(ctx->input.begin() + 32, ctx->input.begin() + 64, std::begin(y1_bytes));
    std::copy(ctx->input.begin() + 64, ctx->input.begin() + 96, std::begin(x2_bytes));
    std::copy(ctx->input.begin() + 96, ctx->input.begin() + 128, std::begin(y2_bytes));

		// Create points
		mpz_class x1_m, y1_m, x2_m, y2_m;
		bool success;
		x1_m.setArray(&success, x1_bytes, 32);
		y1_m.setArray(&success, y1_bytes, 32);
		x2_m.setArray(&success, x2_bytes, 32);
		y2_m.setArray(&success, y2_bytes, 32);

		// Fp
		Fp x1,y1, x2,y2;
		x1.setMpz(&success, x1_m);
		y1.setMpz(&success, y1_m);
		x2.setMpz(&success, x2_m);
		y2.setMpz(&success, y2_m);

		// setup curve
		initPairing(&success, mcl::BN_SNARK1);

		G1 R, A, B;
		A.set(&success, x1, y1);
		B.set(&success, x2, y2);
		G1::add(R, A, B);

		mpz_class final_x, final_y;
		R.x.getMpz(&success, final_x);
		R.y.getMpz(&success, final_y);

		// Copy result
		uint8_t final_x_array[32];
		final_x.getArray(&success, &final_x_array, 32);

		// Test
		// std::vector<uint8_t> test(32);
		// std::copy(std::begin(final_x_array), std::begin(final_x_array) + 32, test.begin());
		// eosio::print(bin2hex(test));

    // Return the result.
    precompile_return({});
  }
} // namespace eosio_evm
