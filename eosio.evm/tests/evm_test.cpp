/**
 *  @file
 *  @copyright defined in eosio.cdt/LICENSE.txt
 */

#include <eosio/tester.hpp>
#include <eosio.evm/eosio.evm.hpp>
#include "json.hpp"

using std::fill;
using std::move;

using eosio::datastream;
using std::string;

// Definitions found in `eosio.cdt/libraries/eosiolib/core/eosio/string.hpp`
EOSIO_TEST_BEGIN(string_test)
   //// constexpr string(const char* str, const size_t n)
   {
      static const char* str0{""};
      static const char* str1{"abc"};
      static const char* str2{"abcdef"};

      static const string eostr0(str0, 0);
      static const string eostr1(str1, 1);
      static const string eostr2(str2, 6);

      CHECK_EQUAL( eostr0.size(), 0 )
      CHECK_EQUAL( strcmp(eostr0.c_str(), ""), 0)

      CHECK_EQUAL( eostr1.size(), 1 )
      CHECK_EQUAL( strcmp(eostr1.c_str(), "a"), 0)

      CHECK_EQUAL( eostr2.size(), 6 )
      CHECK_EQUAL( strcmp(eostr2.c_str(), "abcdef"), 0)
   }
EOSIO_TEST_END

int main(int argc, char* argv[]) {
   bool verbose = false;
   if( argc >= 2 && std::strcmp( argv[1], "-v" ) == 0 ) {
      verbose = true;
   }
   silence_output(!verbose);

   EOSIO_TEST(string_test)
   return has_failed();
}
