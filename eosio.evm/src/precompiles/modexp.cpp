// // Copyright (c) 2020 Syed Jafri. All rights reserved.
// // Licensed under the MIT License..

// #include <eosio.evm/eosio.evm.hpp>

// namespace eosio_evm
// {

//   void Processor::precompile_modexp()
//   {
//     // Get b,e,m
//     uint8_t b_bytes[32];
//     uint8_t e_bytes[32];
//     uint8_t m_bytes[32];
//     std::copy(ctx->input.begin() + 0, ctx->input.begin() + 32, std::begin(b_bytes));
//     std::copy(ctx->input.begin() + 32, ctx->input.begin() + 64, std::begin(e_bytes));
//     std::copy(ctx->input.begin() + 64, ctx->input.begin() + 96, std::begin(m_bytes));

//     uint256_t b = intx::be::load<uint256_t>(b_bytes);
//     uint256_t e = intx::be::load<uint256_t>(e_bytes);
//     uint256_t m = intx::be::load<uint256_t>(m_bytes);

//         // Charge gas
//     auto gas_cost = GP_SHA256 + (GP_SHA256_WORD * num_words(ctx->input.size()));
//     bool error = use_gas(gas_cost);
//     if (error) return;


//        typedef typename detail::double_integer<I1>::type double_type;

//    I1 x(1), y(a);
//    double_type result;

//    while(b > 0)
//    {
//       if(b & 1)
//       {
//          multiply(result, x, y);
//          x = integer_modulus(result, c);
//       }
//       multiply(result, y, y);
//       y = integer_modulus(result, c);
//       b >>= 1;
//    }
//    return x % c;


//     // Execute
//     eosio::checksum256 checksum = eosio::sha256(
//       const_cast<char*>(reinterpret_cast<const char*>(ctx->input.data())),
//       ctx->input.size()
//     );
//     auto result = checksum.extract_as_byte_array();

//     // Return the result.
//     precompile_return({ result.begin(), result.end() });
//   }
// } // namespace eosio_evm
