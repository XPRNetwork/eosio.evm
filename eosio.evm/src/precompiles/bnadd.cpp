// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License...

#define CYBOZU_DONT_USE_EXCEPTION
#define MCL_USE_VINT
#define CYBOZU_DONT_USE_STRING 1
#define MCL_DONT_USE_CSPRNG 1
#define MCL_DONT_USE_OPENSSL 1

#include <eosio.evm/eosio.evm.hpp>
#include <mcl/bn256.hpp>
#include <mcl/src/fp.cpp>

using namespace mcl::bn256;

void Hash(G1& P, const std::string& m)
{
	Fp t;
	t.setHashOf(m.data(), m.size());
	bool b;
	mapToG1(&b, P, t);
}

void KeyGen(Fr& s, G2& pub, const G2& Q)
{
	// s.setRand();
	G2::mul(pub, Q, s); // pub = sQ
}

void Sign(G1& sign, const Fr& s, const std::string& m)
{
	G1 Hm;
	Hash(Hm, m);
	G1::mul(sign, Hm, s); // sign = s H(m)
}

bool Verify(const G1& sign, const G2& Q, const G2& pub, const std::string& m)
{
	Fp12 e1, e2;
	G1 Hm;
	Hash(Hm, m);
	pairing(e1, sign, Q); // e1 = e(sign, Q)
	pairing(e2, Hm, pub); // e2 = e(Hm, sQ)
	return e1 == e2;
}

int main()
{
	std::string m = "hello mcl";

	// setup parameter
	bool b;
	initPairing(&b, mcl::BN_SNARK1);
	G2 Q;
	bool b2;
	mapToG2(&b2, Q, 1);

	// generate secret key and public key
	Fr s;
	G2 pub;
	KeyGen(s, pub, Q);
	// eosio::print("secret key " + s);
	// eosio::print("public key " + pub);

	// sign
	G1 sign;
	Sign(sign, s, m);
	// eosio::print("msg " + m);
	// eosio::print("sign " + sign);

	// verify
	bool ok = Verify(sign, Q, pub, m);
	// eosio::print("verify " + (ok ? "ok" : "ng"));
}

void main2()
{
	// setup curve
	bool b;
	initPairing(&b, mcl::BN_SNARK1);

	// Setup
	// G1
	// G1 Q;
	// bool b2;
	// mapToG2(&b2, Q, 1);

	// Setup

}

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

		// G1 R, A, B;
		// A.set(&success, x1, y1);
		// B.set(&success, x2, y2);
		// G1::add(R, A, B);

		// mpz_class final_x, final_y;
		// R.x.getMpz(&success, final_x);
		// R.y.getMpz(&success, final_y);

		// // Copy result
		// uint8_t final_x_array[32];
		// final_x.getArray(&success, &final_x_array, 32);

		// std::vector<uint8_t> test(32);
		// std::copy(std::begin(final_x_array), std::begin(final_x_array) + 32, test.begin());
		// eosio::print("\nBN ADD: ", bin2hex(test));

    // Return the result.
    precompile_return({});
  }
} // namespace eosio_evm
