#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include "eosio.system_tester.hpp"
#include <eosio/chain/exceptions.hpp>

#include "Runtime/Runtime.h"

#include <fc/variant_object.hpp>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;

using mvo = fc::mutable_variant_object;

constexpr auto FORK = "Istanbul";

class eosio_evm_tester : public tester {
public:
   eosio_evm_tester() {
      produce_blocks( 2 );
      create_accounts( { N(evm), N(bob), N(carol), N(eosio.evm), N(eosio.token), N(1234test1111) } );
      produce_blocks( 2 );

      // Deploy evm
      set_code( N(eosio.evm), contracts::evm_wasm() );
      set_abi( N(eosio.evm), contracts::evm_abi().data() );
      produce_blocks();

      const auto& wasm_accnt = control->db().get<account_object, by_name>( N(eosio.evm) );
      abi_def wasm_abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(wasm_accnt.abi, wasm_abi), true);
      abi_ser.set_abi(wasm_abi, abi_serializer_max_time);

      // Deploy token
      set_code( N(eosio.token), contracts::token_wasm() );
      set_abi( N(eosio.token), contracts::token_abi().data() );
      produce_blocks();

      const auto& token_acct = control->db().get<account_object, by_name>( N(eosio.evm) );
      abi_def token_abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(token_acct.abi, token_abi), true);
      abi_ser.set_abi(token_abi, abi_serializer_max_time);
   }

   action_result push_action( const account_name& signer, const action_name &name, const variant_object &data )
   {
      string action_type_name = abi_ser.get_action_type(name);

      action act;
      act.account = N(eosio.evm);
      act.name    = name;
      act.data    = abi_ser.variant_to_binary( action_type_name, data, abi_serializer_max_time );

      return base_tester::push_action( std::move(act), signer.to_uint64_t() );
   }

   fc::variant get_account( account_name acc )
   {
      vector<char> data = get_row_by_account( N(eosio.evm), N(eosio.evm), N(account), acc );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "Account", data, abi_serializer_max_time );
   }

   transaction_trace_ptr create( const name& signer, const name& account, const std::string& data )
   {
      return base_tester::push_action( N(eosio.evm), N(create), signer, mvo()
           ( "account", account)
           ( "data", data)
      );
   }

   transaction_trace_ptr raw( const name& signer, const std::string& tx, const fc::ripemd160& sender)
   {
      return base_tester::push_action( N(eosio.evm), N(raw), signer, mvo()
           ( "tx", tx)
           ( "sender", sender)
      );
   }

   transaction_trace_ptr testtx( const name& signer, const std::string& tx)
   {
      return base_tester::push_action( N(eosio.evm), N(testtx), signer, mvo()
           ( "tx", tx)
      );
   }
   transaction_trace_ptr printtx( const name& signer, const std::string& tx)
   {
      return base_tester::push_action( N(eosio.evm), N(printtx), signer, mvo()
           ( "tx", tx)
      );
   }

   action_result withdraw( const name& signer, const name& to, const asset& quantity) {

      return push_action( signer, N(withdraw), mvo()
           ( "to", to)
           ( "quantity", quantity)
      );
   }

   abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(eosio_evm_tests)

BOOST_FIXTURE_TEST_CASE( test_create, eosio_evm_tester ) try {
   // Create first time
   create( N(1234test1111), N(1234test1111), std::string("hi") );
   auto stats = get_account( N(0) );

   REQUIRE_MATCHING_OBJECT( stats, mvo()
      ("index", 0)
      ("address", "bc5c5b389d1bd6b0e356bfcb2b3d748a98304a0c")
      ("account", "1234test1111")
      ("balance", "0.0000 SYS")
      ("nonce", 1)
      ("code", vector<uint8_t>{})
   );
   produce_blocks(2);

   // Create same time (same arbitrary string)
   BOOST_CHECK_EXCEPTION(
      create( N(1234test1111), N(1234test1111), std::string("hi") ),
      eosio_assert_message_exception,
      [](const eosio_assert_message_exception& e) {
         return expect_assert_message(e, "an EVM account is already linked to this EOS account");
      }
   );
   produce_blocks(2);

   // Create same time (different arbitrary string)
   BOOST_CHECK_EXCEPTION(
      create( N(1234test1111), N(1234test1111), std::string("him") ),
      eosio_assert_message_exception,
      [](const eosio_assert_message_exception& e) {
         return expect_assert_message(e, "an EVM account is already linked to this EOS account");
      }
   );
   produce_blocks(1);
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( test_create_wrong_auth, eosio_evm_tester ) try {
   BOOST_CHECK_EXCEPTION(
      create( N(evm), N(1234test1111), std::string("hi") ),
      missing_auth_exception,
      [](const missing_auth_exception& e) {
         return expect_assert_message(e, "missing authority of");
      }
   );
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( transaction_tests, eosio_evm_tester ) try {
   auto testDirectory = "jsontests/TransactionTests";

   // For each folder in TransactionTests
   for (const auto & entry : fs::directory_iterator(testDirectory)) {
      std::string subFolderPath = entry.path();

      vector<string> subFolderPathSplit;
      boost::split(subFolderPathSplit, subFolderPath, boost::is_any_of("/"));
      std::string category = subFolderPathSplit[2];

      // For each test in ttAddress
      for (const auto& testPathItr : fs::directory_iterator(subFolderPath)) {
         std::string testPath = testPathItr.path();

         // Get test name
         vector<string> testPathSplit;
         boost::split(testPathSplit, testPath, boost::is_any_of("/"));
         std::string testName = testPathSplit[3];

         // Remove .json extension
         size_t extIdx = testName.find_last_of(".");
         testName = testName.substr(0, extIdx);

         // Get the json
         auto json = fc::json::from_file(testPath, fc::json::relaxed_parser);

         // Find out if it is valid
         auto fork_res = json.get_object()[testName][FORK].get_object();
         auto is_valid = fork_res.find("hash") != fork_res.end();

         // Get rlp and erase 0x
         std::string rlp = json.get_object()[testName]["rlp"].get_string();
         rlp.erase(0, 2);

         // Skip
         // String10MbData: EOSIO doesnt support data of 10 MB
         // TransactionWithHighValue: Max value is 2^62 - 1
         // dataTx_bcValidBlockTestFrontier: For frontier
         if (testName == "String10MbData" || testName == "TransactionWithHighValue" || testName == "dataTx_bcValidBlockTestFrontier") {
            // NO OP
         }
         // If valid
         else if (is_valid)
         {
            auto res             = testtx( N(1234test1111), rlp);
            auto console         = fc::json::from_string(res->action_traces[0].console);
            auto hash            = *(console.get_object().find("hash"));
            auto sender          = *(console.get_object().find("sender"));
            auto expected_hash   = *(fork_res.find("hash"));
            auto expected_sender = *(fork_res.find("sender"));

            if (hash != expected_hash || sender != expected_sender) {
               std::cout << "\033[1;32m" << category << " - " << testName << " (Expected: " << is_valid << ")\"\033[0m" << std::endl;
               BOOST_REQUIRE_EQUAL( hash, expected_hash );
               BOOST_REQUIRE_EQUAL( sender, expected_sender );
            }
         }
         // If not valid
         else
         {
            try {
               testtx( N(1234test1111), rlp);

               std::cout << "\033[1;32m" << category << " - " << testName << " (Expected: " << is_valid << ")\033[0m"
                         << "\033[1;31m (Actual: 1) \033[0m" << std::endl << std::endl;
            } catch(const fc::exception& e) {
               auto matches = expect_assert_message(e, "Invalid Transaction") || expect_assert_message(e, "secp256k1_ecdsa_recover_compact") || expect_assert_message(e, "Invalid hex character");
               // std::cout << "ABCD:" << e.get_log().at(0).get_message() << std::endl;
               if (matches) {
                  // std::cout << "\033[1;32m" << e.top_message() << "\033[0m" << std::endl;
               } else {
                  std::cout << "\033[1;31m" << category << " - " << testName << " (Expected: " << is_valid << ")\033[0m" << std::endl;
                  std::cout << "\033[1;31m" << e.to_string() << "\033[0m" << std::endl;
               }
            }
         }

         produce_blocks(1);
      }
   }
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( general_state_tests, eosio_evm_tester ) try {
   auto testDirectory = "jsontests/GeneralStateTests";

   // For each folder in TransactionTests
   for (const auto & entry : fs::directory_iterator(testDirectory)) {
      std::string subFolderPath = entry.path();

      vector<string> subFolderPathSplit;
      boost::split(subFolderPathSplit, subFolderPath, boost::is_any_of("/"));
      std::string category = subFolderPathSplit[2];

      // For each test in ttAddress
      for (const auto& testPathItr : fs::directory_iterator(subFolderPath)) {
         std::string testPath = testPathItr.path();

         // Get test name
         vector<string> testPathSplit;
         boost::split(testPathSplit, testPath, boost::is_any_of("/"));
         std::string testName = testPathSplit[3];

         // Remove .json extension
         size_t extIdx = testName.find_last_of(".");
         testName = testName.substr(0, extIdx);

         // Get the json
         auto json = fc::json::from_file(testPath, fc::json::relaxed_parser);

         // Find out if it is valid
         auto fork_res = json.get_object()[testName][FORK].get_object();
         auto is_valid = fork_res.find("hash") != fork_res.end();

         // Get rlp and erase 0x
         std::string rlp = json.get_object()[testName]["rlp"].get_string();
         rlp.erase(0, 2);

         // Skip
         if (testName == "") {
            // NO OP
         }
         // If valid
         else if (is_valid)
         {
            auto res             = testtx( N(1234test1111), rlp);
            auto console         = fc::json::from_string(res->action_traces[0].console);
            auto hash            = *(console.get_object().find("hash"));
            auto sender          = *(console.get_object().find("sender"));
            auto expected_hash   = *(fork_res.find("hash"));
            auto expected_sender = *(fork_res.find("sender"));

            if (hash != expected_hash || sender != expected_sender) {
               std::cout << "\033[1;32m" << category << " - " << testName << " (Expected: " << is_valid << ")\"\033[0m" << std::endl;
               BOOST_REQUIRE_EQUAL( hash, expected_hash );
               BOOST_REQUIRE_EQUAL( sender, expected_sender );
            }
         }
         // If not valid
         else
         {
            try {
               testtx( N(1234test1111), rlp);

               std::cout << "\033[1;32m" << category << " - " << testName << " (Expected: " << is_valid << ")\033[0m"
                         << "\033[1;31m (Actual: 1) \033[0m" << std::endl << std::endl;
            } catch(const fc::exception& e) {
               auto matches = expect_assert_message(e, "Invalid Transaction") || expect_assert_message(e, "secp256k1_ecdsa_recover_compact") || expect_assert_message(e, "Invalid hex character");
               // std::cout << "ABCD:" << e.get_log().at(0).get_message() << std::endl;
               if (matches) {
                  // std::cout << "\033[1;32m" << e.top_message() << "\033[0m" << std::endl;
               } else {
                  std::cout << "\033[1;31m" << category << " - " << testName << " (Expected: " << is_valid << ")\033[0m" << std::endl;
                  std::cout << "\033[1;31m" << e.to_string() << "\033[0m" << std::endl;
               }
            }
         }

         produce_blocks(1);
      }
   }
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
