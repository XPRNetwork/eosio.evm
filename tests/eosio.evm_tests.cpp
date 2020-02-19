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
      create_accounts( { N(evm), N(bob), N(carol), N(eosio.evm), N(eosio.token), N(1234test1111), N(erc20) } );
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

   transaction_trace_ptr testtx( const std::string& tx )
   {
      return base_tester::push_action( N(eosio.evm), N(testtx), N(eosio.evm), mvo()
           ( "tx", tx)
      );
   }
   transaction_trace_ptr printtx( const std::string& tx )
   {
      return base_tester::push_action( N(eosio.evm), N(printtx), N(eosio.evm), mvo()
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

/**
 * ERC20
 */
BOOST_FIXTURE_TEST_CASE( erc_20, eosio_evm_tester ) try {
   base_tester::push_action( N(eosio.evm), N(devnewacct), N(eosio.evm), mvo()
      ( "address", "f79b834a37f3143f4a73fc3934edac67fd3a01cd")
      ( "balance", 0)
      ( "code", std::vector<uint8_t>{})
      ( "nonce", 0)
      ( "account", "")
   );
   base_tester::push_action( N(eosio.evm), N(raw), N(erc20), mvo()
      ( "tx", "f917858001831e84808080b9173760806040523480156200001157600080fd5b506040516200165738038062001657833981810160405260608110156200003757600080fd5b81019080805160405193929190846401000000008211156200005857600080fd5b838201915060208201858111156200006f57600080fd5b82518660018202830111640100000000821117156200008d57600080fd5b8083526020830192505050908051906020019080838360005b83811015620000c3578082015181840152602081019050620000a6565b50505050905090810190601f168015620000f15780820380516001836020036101000a031916815260200191505b50604052602001805160405193929190846401000000008211156200011557600080fd5b838201915060208201858111156200012c57600080fd5b82518660018202830111640100000000821117156200014a57600080fd5b8083526020830192505050908051906020019080838360005b838110156200018057808201518184015260208101905062000163565b50505050905090810190601f168015620001ae5780820380516001836020036101000a031916815260200191505b50604052602001805190602001909291905050508282828260039080519060200190620001dd929190620004a7565b508160049080519060200190620001f6929190620004a7565b5080600560006101000a81548160ff021916908360ff16021790555050505062000232338260ff16600a0a620f4240026200023b60201b60201c565b50505062000556565b600073ffffffffffffffffffffffffffffffffffffffff168273ffffffffffffffffffffffffffffffffffffffff161415620002df576040517f08c379a000000000000000000000000000000000000000000000000000000000815260040180806020018281038252601f8152602001807f45524332303a206d696e7420746f20746865207a65726f20616464726573730081525060200191505060405180910390fd5b620002f3600083836200041960201b60201c565b6200030f816002546200041e60201b62000f2d1790919060201c565b6002819055506200036d816000808573ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020546200041e60201b62000f2d1790919060201c565b6000808473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020819055508173ffffffffffffffffffffffffffffffffffffffff16600073ffffffffffffffffffffffffffffffffffffffff167fddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef836040518082815260200191505060405180910390a35050565b505050565b6000808284019050838110156200049d576040517f08c379a000000000000000000000000000000000000000000000000000000000815260040180806020018281038252601b8152602001807f536166654d6174683a206164646974696f6e206f766572666c6f77000000000081525060200191505060405180910390fd5b8091505092915050565b828054600181600116156101000203166002900490600052602060002090601f016020900481019282601f10620004ea57805160ff19168380011785556200051b565b828001600101855582156200051b579182015b828111156200051a578251825591602001919060010190620004fd565b5b5090506200052a91906200052e565b5090565b6200055391905b808211156200054f57600081600090555060010162000535565b5090565b90565b6110f180620005666000396000f3fe608060405234801561001057600080fd5b50600436106100a95760003560e01c80633950935111610071578063395093511461025f57806370a08231146102c557806395d89b411461031d578063a457c2d7146103a0578063a9059cbb14610406578063dd62ed3e1461046c576100a9565b806306fdde03146100ae578063095ea7b31461013157806318160ddd1461019757806323b872dd146101b5578063313ce5671461023b575b600080fd5b6100b66104e4565b6040518080602001828103825283818151815260200191508051906020019080838360005b838110156100f65780820151818401526020810190506100db565b50505050905090810190601f1680156101235780820380516001836020036101000a031916815260200191505b509250505060405180910390f35b61017d6004803603604081101561014757600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff16906020019092919080359060200190929190505050610586565b604051808215151515815260200191505060405180910390f35b61019f6105a4565b6040518082815260200191505060405180910390f35b610221600480360360608110156101cb57600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff169060200190929190803573ffffffffffffffffffffffffffffffffffffffff169060200190929190803590602001909291905050506105ae565b604051808215151515815260200191505060405180910390f35b610243610687565b604051808260ff1660ff16815260200191505060405180910390f35b6102ab6004803603604081101561027557600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff1690602001909291908035906020019092919050505061069e565b604051808215151515815260200191505060405180910390f35b610307600480360360208110156102db57600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff169060200190929190505050610751565b6040518082815260200191505060405180910390f35b610325610799565b6040518080602001828103825283818151815260200191508051906020019080838360005b8381101561036557808201518184015260208101905061034a565b50505050905090810190601f1680156103925780820380516001836020036101000a031916815260200191505b509250505060405180910390f35b6103ec600480360360408110156103b657600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff1690602001909291908035906020019092919050505061083b565b604051808215151515815260200191505060405180910390f35b6104526004803603604081101561041c57600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff16906020019092919080359060200190929190505050610908565b604051808215151515815260200191505060405180910390f35b6104ce6004803603604081101561048257600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff169060200190929190803573ffffffffffffffffffffffffffffffffffffffff169060200190929190505050610926565b6040518082815260200191505060405180910390f35b606060038054600181600116156101000203166002900480601f01602080910402602001604051908101604052809291908181526020018280546001816001161561010002031660029004801561057c5780601f106105515761010080835404028352916020019161057c565b820191906000526020600020905b81548152906001019060200180831161055f57829003601f168201915b5050505050905090565b600061059a6105936109ad565b84846109b5565b6001905092915050565b6000600254905090565b60006105bb848484610bac565b61067c846105c76109ad565b6106778560405180606001604052806028815260200161102660289139600160008b73ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020600061062d6109ad565b73ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002054610e6d9092919063ffffffff16565b6109b5565b600190509392505050565b6000600560009054906101000a900460ff16905090565b60006107476106ab6109ad565b8461074285600160006106bc6109ad565b73ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002060008973ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002054610f2d90919063ffffffff16565b6109b5565b6001905092915050565b60008060008373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020549050919050565b606060048054600181600116156101000203166002900480601f0160208091040260200160405190810160405280929190818152602001828054600181600116156101000203166002900480156108315780601f1061080657610100808354040283529160200191610831565b820191906000526020600020905b81548152906001019060200180831161081457829003601f168201915b5050505050905090565b60006108fe6108486109ad565b846108f98560405180606001604052806025815260200161109760259139600160006108726109ad565b73ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002060008a73ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002054610e6d9092919063ffffffff16565b6109b5565b6001905092915050565b600061091c6109156109ad565b8484610bac565b6001905092915050565b6000600160008473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002060008373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002054905092915050565b600033905090565b600073ffffffffffffffffffffffffffffffffffffffff168373ffffffffffffffffffffffffffffffffffffffff161415610a3b576040517f08c379a00000000000000000000000000000000000000000000000000000000081526004018080602001828103825260248152602001806110736024913960400191505060405180910390fd5b600073ffffffffffffffffffffffffffffffffffffffff168273ffffffffffffffffffffffffffffffffffffffff161415610ac1576040517f08c379a0000000000000000000000000000000000000000000000000000000008152600401808060200182810382526022815260200180610fde6022913960400191505060405180910390fd5b80600160008573ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002060008473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020819055508173ffffffffffffffffffffffffffffffffffffffff168373ffffffffffffffffffffffffffffffffffffffff167f8c5be1e5ebec7d5bd14f71427d1e84f3dd0314c0f7b2291e5b200ac8c7c3b925836040518082815260200191505060405180910390a3505050565b600073ffffffffffffffffffffffffffffffffffffffff168373ffffffffffffffffffffffffffffffffffffffff161415610c32576040517f08c379a000000000000000000000000000000000000000000000000000000000815260040180806020018281038252602581526020018061104e6025913960400191505060405180910390fd5b600073ffffffffffffffffffffffffffffffffffffffff168273ffffffffffffffffffffffffffffffffffffffff161415610cb8576040517f08c379a0000000000000000000000000000000000000000000000000000000008152600401808060200182810382526023815260200180610fbb6023913960400191505060405180910390fd5b610cc3838383610fb5565b610d2e81604051806060016040528060268152602001611000602691396000808773ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002054610e6d9092919063ffffffff16565b6000808573ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002081905550610dc1816000808573ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002054610f2d90919063ffffffff16565b6000808473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020819055508173ffffffffffffffffffffffffffffffffffffffff168373ffffffffffffffffffffffffffffffffffffffff167fddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef836040518082815260200191505060405180910390a3505050565b6000838311158290610f1a576040517f08c379a00000000000000000000000000000000000000000000000000000000081526004018080602001828103825283818151815260200191508051906020019080838360005b83811015610edf578082015181840152602081019050610ec4565b50505050905090810190601f168015610f0c5780820380516001836020036101000a031916815260200191505b509250505060405180910390fd5b5060008385039050809150509392505050565b600080828401905083811015610fab576040517f08c379a000000000000000000000000000000000000000000000000000000000815260040180806020018281038252601b8152602001807f536166654d6174683a206164646974696f6e206f766572666c6f77000000000081525060200191505060405180910390fd5b8091505092915050565b50505056fe45524332303a207472616e7366657220746f20746865207a65726f206164647265737345524332303a20617070726f766520746f20746865207a65726f206164647265737345524332303a207472616e7366657220616d6f756e7420657863656564732062616c616e636545524332303a207472616e7366657220616d6f756e74206578636565647320616c6c6f77616e636545524332303a207472616e736665722066726f6d20746865207a65726f206164647265737345524332303a20617070726f76652066726f6d20746865207a65726f206164647265737345524332303a2064656372656173656420616c6c6f77616e63652062656c6f77207a65726fa2646970667358221220d2db32fd070f5d5c2299e3157a25d9184a233cd35690d15eab1cf135b8bdfb3364736f6c63430006010033000000000000000000000000000000000000000000000000000000000000006000000000000000000000000000000000000000000000000000000000000000a0000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000000000000000000000000000000000000000000453796564000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004535945440000000000000000000000000000000000000000000000000000000025a033f0ab1e47ce324cb8108b521015cda2bb8fb467385005d13d241aaba229bf9ca00a42d52dfee23996be64616c56f95d4db6dfd5080a9710b4c330651c81b289fb")
      ( "sender", "424a26f6de36eb738762cead721bac23c62a724e")
   );
} FC_LOG_AND_RETHROW()

/**
 * Transaction tests
 */
// BOOST_FIXTURE_TEST_CASE( transaction_tests, eosio_evm_tester ) try {
//    auto testDirectory = "jsontests/TransactionTests";

//    // For each folder in Transact=][pnTests
//    for (const auto & entry : fs::directory_iterator(testDirectory)) {
//       std::string subFolderPath = entry.path();

//       vector<string> subFolderPathSplit;
//       boost::split(subFolderPathSplit, subFolderPath, boost::is_any_of("/"));
//       std::string category = subFolderPathSplit[2];

//       // For each test in ttAddress
//       for (const auto& testPathItr : fs::directory_iterator(subFolderPath)) {
//          std::string testPath = testPathItr.path();

//          // Get test name
//          vector<string> testPathSplit;
//          boost::split(testPathSplit, testPath, boost::is_any_of("/"));
//          std::string testName = testPathSplit[3];

//          // Remove .json extension
//          size_t extIdx = testName.find_last_of(".");
//          testName = testName.substr(0, extIdx);

//          // Get the json
//          auto json = fc::json::from_file(testPath, fc::json::relaxed_parser);

//          // Find out if it is valid
//          auto fork_res = json.get_object()[testName][FORK].get_object();
//          auto is_valid = fork_res.find("hash") != fork_res.end();

//          // Get rlp and erase 0x
//          std::string rlp = json.get_object()[testName]["rlp"].get_string();
//          rlp.erase(0, 2);

//          // Skip
//          // String10MbData: EOSIO doesnt support data of 10 MB
//          // TransactionWithHighValue: Max value is 2^62 - 1
//          // dataTx_bcValidBlockTestFrontier: For frontier
//          if (testName == "String10MbData" || testName == "TransactionWithHighValue" || testName == "dataTx_bcValidBlockTestFrontier") {
//             // NO OP
//          }
//          // If valid
//          else if (is_valid)
//          {
//             auto res             = testtx(rlp);
//             auto console         = fc::json::from_string(res->action_traces[0].console);
//             auto hash            = *(console.get_object().find("hash"));
//             auto sender          = *(console.get_object().find("sender"));
//             auto expected_hash   = *(fork_res.find("hash"));
//             auto expected_sender = *(fork_res.find("sender"));

//             if (hash != expected_hash || sender != expected_sender) {
//                std::cout << "\033[1;32m" << category << " - " << testName << " (Expected: " << is_valid << ")\"\033[0m" << std::endl;
//                BOOST_REQUIRE_EQUAL( hash, expected_hash );
//                BOOST_REQUIRE_EQUAL( sender, expected_sender );
//             }
//          }
//          // If not valid
//          else
//          {
//             try {
//                testtx(rlp);

//                std::cout << "\033[1;32m" << category << " - " << testName << " (Expected: " << is_valid << ")\033[0m"
//                          << "\033[1;31m (Actual: 1) \033[0m" << std::endl << std::endl;
//             } catch(const fc::exception& e) {
//                auto matches = expect_assert_message(e, "Invalid Transaction") || expect_assert_message(e, "secp256k1_ecdsa_recover_compact") || expect_assert_message(e, "Invalid hex character");
//                if (matches) {
//                   // std::cout << "\033[1;32m" << e.top_message() << "\033[0m" << std::endl;
//                } else {
//                   std::cout << "\033[1;31m" << category << " - " << testName << " (Expected: " << is_valid << ")\033[0m" << std::endl;
//                   std::cout << "\033[1;31m" << e.to_string() << "\033[0m" << std::endl;
//                }
//             }
//          }

//          produce_blocks(1);
//       }
//    }
// } FC_LOG_AND_RETHROW()

/**
 * Debugging
 */
// BOOST_FIXTURE_TEST_CASE( debugging, eosio_evm_tester ) try {
//    try {
//       base_tester::push_action( N(eosio.evm), N(devnewacct), N(eosio.evm), mvo()
//          ( "address", "a94f5374fce5edbc8e2a8697c15331677e6ebf0b")
//          ( "balance", "1000000000000000000")
//          ( "code", std::vector<uint8_t>{})
//          ( "nonce", 0)
//          ( "account", "")
//       );
//       base_tester::push_action( N(eosio.evm), N(devnewacct), N(eosio.evm), mvo()
//          ( "address", "095e7baea6a6c7c4c2dfeb977efac326af552d87")
//          ( "balance", 0)
//          ( "code", eosio_system::HexToBytes("73095e7baea6a6c7c4c2dfeb977efac326af552d873173095e7baea6a6c7c4c2dfeb977efac326af552d873173095e7baea6a6c7c4c2dfeb977efac326af552d8731f060005500"))
//          ( "nonce", 0)
//          ( "account", "")
//       );
//       base_tester::push_action( N(eosio.evm), N(raw), N(eosio.evm), mvo()
//          ( "tx", "f85f800183061a8094095e7baea6a6c7c4c2dfeb977efac326af552d870180259ffc52968a0cfe417d4f0d046e3d4763d78e5e53932ea3b3b7aa987e5a631cf7a021e496f759eafe0c921834b2397439e518ac04e6a6654e714d8000cd12e44a55")
//          ( "sender", "424a26f6de36eb738762cead721bac23c62a724e")
//       );
//    } catch(const fc::exception& e) {
//       std::cout << "\033[1;31m" << e.to_string() << "\033[0m" << std::endl;
//    }
//    produce_blocks(1);
// } FC_LOG_AND_RETHROW()

/**
 * General state tests
 */
// BOOST_FIXTURE_TEST_CASE( general_state_tests, eosio_evm_tester ) try {
//    auto testDirectory = "jsontests/GeneralStateTestsEOS";

//    // For each folder in GeneralStateTestsEOS
//    auto totalTests = 0;
//    for (const auto & entry : fs::directory_iterator(testDirectory)) {
//       std::string subFolderPath = entry.path();

//       vector<string> subFolderPathSplit;
//       boost::split(subFolderPathSplit, subFolderPath, boost::is_any_of("/"));
//       std::string category = subFolderPathSplit[2];

//       // For each test in ttAddress
//       for (const auto& testPathItr : fs::directory_iterator(subFolderPath)) {
//          std::string testPath = testPathItr.path();

//          // Get test name
//          vector<string> testPathSplit;
//          boost::split(testPathSplit, testPath, boost::is_any_of("/"));
//          std::string testName = testPathSplit[3];

//          // Remove .json extension
//          size_t extIdx = testName.find_last_of(".");
//          testName = testName.substr(0, extIdx);

//          // Get the json
//          auto json = fc::json::from_file(testPath, fc::json::relaxed_parser);
//          auto testJson = json.get_object()[testName];

//          // Deconstruct
//          auto env = testJson["env"].get_object();
//          auto pre = testJson["pre"];
//          auto post = testJson["post"][FORK];
//          auto transactions = testJson["transactions"];

//          // For each expected transaction post
//          for (std::size_t i = 0; i < post.size(); ++i) {
//             auto results = post[i]["result"];

//             // PREPOPULATE
//             for (std::size_t j = 0; j < pre.size(); ++j) {
//                // Get address (only necessary for prepopulate, the rest are not 0x encoded)
//                std::string address = fc::json::to_string(pre[j]["address"], fc::time_point::maximum());
//                address = address.substr(3, address.length() - 4);

//                // Code
//                auto code_string = pre[j]["code"].get_string();
//                code_string.erase(0, 2);
//                auto code = eosio_system::HexToBytes(code_string);

//                // Convert to uint64_t
//                std::cout << testName << ": " << pre[j]["balance"].get_string() << std::endl;

//                // TODO handle high balance
//                unsigned long long balance = 0;
//                unsigned long long nonce = 0;
//                try {
//                   balance = std::stoull(pre[j]["balance"].get_string(), 0, 16);
//                   nonce = std::stoull(pre[j]["nonce"].get_string(), 0, 16);
//                } catch (const std::exception& e) {
//                   goto next_test;
//                }

//                // Create pre accounts
//                base_tester::push_action( N(eosio.evm), N(devnewacct), N(eosio.evm), mvo()
//                   ( "address", address )
//                   ( "balance", balance )
//                   ( "code", code )
//                   ( "nonce", nonce )
//                   ( "account", "" )
//                );
//                produce_blocks(2);


//                // Create pre storage
//                auto store = pre[j]["storage"];
//                for (std::size_t h = 0; h < store.size(); ++h) {
//                   // Create pre key values
//                   auto key = store[h]["key"].get_string();
//                   key.erase(0, 2);
//                   auto value = store[h]["value"].get_string();
//                   value.erase(0, 2);

//                   base_tester::push_action( N(eosio.evm), N(devnewstore), N(eosio.evm), mvo()
//                      ( "address", address)
//                      ( "key", key )
//                      ( "value", value )
//                   );
//                   produce_blocks(1);
//                }
//             }

//             // Print test name
//             std::cout << totalTests << ". " << testName << ": " << i << std::endl;

//             // Execute transaction
//             base_tester::push_action( N(eosio.evm), N(teststatetx), N(eosio.evm), mvo()
//                ( "tx", transactions[i].get_string())
//                ( "env", env)
//             );
//             produce_blocks(2);

//             // For each result
//             for (std::size_t x = 0; x < results.size(); ++x) {
//                // Get state
//                auto res = base_tester::push_action( N(eosio.evm), N(printstate), N(eosio.evm), mvo()
//                   ( "address", results[x]["address"].get_string() )
//                );
//                produce_blocks(1);

//                // Check console against storage
//                auto expected_storage = fc::json::to_string(results[x]["storage"], fc::time_point::maximum());
//                auto console          = res->action_traces[0].console;

//                if (expected_storage != console) {
//                   // std::cout << "Failed Test: " << testName << ": " << i << std::endl;
//                   BOOST_REQUIRE_EQUAL( console, expected_storage );
//                }

//                // Get state
//                // const auto& db = control->db();
//                // namespace chain = eosio::chain;
//                // const auto* secondary_table_id = db.find<eosio::chain::table_id_object, chain::by_code_scope_table>(
//                //    boost::make_tuple( N(eosio.evm), N(eosio.evm), name(N(accountstate).to_uint64_t() | 2))
//                // );
//                // const auto& secondary_index = db.get_index<index_double_index>().indices();
//                // const auto& secondary_index_by_secondary = secondary_index.get<by_secondary>();
//                // auto itr = secondary_index_by_secondary.lower_bound(address);

//                // while (itr != secondary_index_by_secondary.end()) {
//                //    std::cout << fc::json::to_string(*itr, fc::time_point::maximum()) << std::endl;
//                //    itr++;
//                // }

//                // For each storage
//                // for (std::size_t k = 0; k < storage.size(); ++k) {
//                //    auto value = storage[x];
//                // }
//             }

//             // Reset chain
//             base_tester::push_action( N(eosio.evm), N(clearall), N(eosio.evm), mvo());
//          }
//    next_test:
//          // Reset chain
//          base_tester::push_action( N(eosio.evm), N(clearall), N(eosio.evm), mvo());
//          produce_blocks(1);
//          totalTests++;
//       }
//    }
// } FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
