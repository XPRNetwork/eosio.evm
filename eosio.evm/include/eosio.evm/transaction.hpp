// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License.
#pragma once

#include <optional>

#include "constants.hpp"
#include "util.hpp"
#include "tables.hpp"

namespace eosio_evm
{
  /**
   * Transaction Helpers
   */
  inline bool is_pre_eip_155(size_t v) { return v == 27 || v == 28; }

  inline size_t from_ethereum_recovery_id (size_t v)
  {
    if (is_pre_eip_155(v)) {
      return v - PRE_155_V_START;
    }

    constexpr auto min_valid_v = 37u;
    eosio::check(v >= min_valid_v, "Invalid Transaction: Expected v to encode a valid chain ID");

    const size_t rec_id = (v - POST_155_V_START) % 2;
    const size_t chain_id = ((v - rec_id) - POST_155_V_START) / 2;
    eosio::check(chain_id == CURRENT_CHAIN_ID, "Invalid Transaction: Chain ID mismatch");

    return rec_id;
  }

  /**
   * EVM Logs
   */
  struct LogEntry
  {
    Address address;
    std::vector<uint8_t> data;
    std::vector<uint256_t> topics;

    #if (PRINT_LOGS == true)
    std::string topics_as_json_string() const {
      std::string output = "[";
      for (auto i = 0; i < topics.size(); i++) {
        std::string topic_string = intx::hex(topics[i]);
        std::string topic_string_final = std::string(64 - topic_string.size(), '0').append(topic_string);

        output += "\"" + topic_string_final + "\"";
        if (i < topics.size() - 1) {
          output += ",";
        }
      }
      output += "]";
      return output;
    }
    #endif /** PRINT_LOGS **/
  };

  struct LogHandler
  {
    std::vector<LogEntry> logs = {};
    inline void add(LogEntry&& e) { logs.emplace_back(e); }
    inline void pop() { if(!logs.empty()) { logs.pop_back(); } }

    #if (PRINT_LOGS == true)
    std::string as_json_string() const {
      std::string output = "[";

      for (auto i = 0; i < logs.size(); i++) {
        std::string address_string = intx::hex(logs[i].address);
        std::string address_string_final = std::string(40 - address_string.size(), '0').append(address_string);

        output += R"({"address": ")" + address_string_final     + "\"," +
                  R"("data": ")"    + bin2hex(logs[i].data)           + "\"," +
                  R"("topics": )"   + logs[i].topics_as_json_string() + "}";

        if (i < logs.size() - 1) {
          output += ",";
        }
      }
      output += "]";
      return output;
    }
    #endif /** PRINT_LOGS **/
  };

  /**
   * State modifications: Stored for Reverting
   */
  enum class StateModificationType {
    NONE = 0,
    STORE_KV,
    CREATE_ACCOUNT,
    SET_CODE,
    INCREMENT_NONCE,
    ORIGINAL_STORAGE,
    TRANSFER,
    LOG,
    SELF_DESTRUCT
  };
  using SMT = StateModificationType;

  struct StateModification {
    StateModificationType type;
    uint64_t index;
    uint256_t key;
    uint256_t oldvalue;
    uint256_t amount;
    uint256_t newvalue;

    #if (TESTING == true)
    void print() {
      std::map<StateModificationType, std::string> reverseState = {
        { SMT::STORE_KV, "STORE_KV"   },
        { SMT::CREATE_ACCOUNT, "CREATE_ACCOUNT"    },
        { SMT::SET_CODE, "SET_CODE"    },
        { SMT::INCREMENT_NONCE, "INCREMENT_NONCE"    },
        { SMT::ORIGINAL_STORAGE, "ORIGINAL_STORAGE"    },
        { SMT::TRANSFER, "TRANSFER"    },
        { SMT::LOG, "LOG"    },
        { SMT::SELF_DESTRUCT, "SELF_DESTRUCT"    }
      };

      eosio::print("\nType: ", reverseState[type], ", Index: ", index, ", Key: ", intx::to_string(key), " OldValue: ", intx::to_string(oldvalue), " NewValue: ", intx::to_string(newvalue), " Amount: ", intx::to_string(amount));
    }
    #endif /** Testing **/
  };

  struct EthereumTransaction {
    uint256_t nonce;           // A scalar value equal to the number of transactions sent by the sender;
    uint256_t gas_price;       // A scalar value equal to the number of Wei to be paid per unit of gas for all computation costs incurred as a result of the execution of this transaction;
    uint256_t gas_limit;       // A scalar value equal to the maximum amount of gas that should be used in executing this transaction
    std::vector<uint8_t> to;   // Currently set as vector of bytes.
    uint256_t value;           // A scalar value equal to the number of Wei to be transferred to the message call’s recipient or, in the case of contract creation, as an endowment to the newly created account; forma
    std::vector<uint8_t> data; // An unlimited size byte array specifying the input data of the message call

    std::optional<eosio::checksum160> sender; // Address recovered from 1) signature or 2) EOSIO Account Table (authorized by EOSIO account in case 2)
    std::optional<Address> to_address;        // Currently set as 256 bit. The 160-bit address of the message call’s recipient or, for a contract creation transaction, ∅, used here to denote the only member of B0 ;
    std::optional<Address> created_address;   // If create transaction, the address that was created
    std::unique_ptr<Account> sender_account;  // Pointer to sender account
    std::vector<Address> selfdestruct_list;   // SELFDESTRUCT List
    LogHandler logs = {};                     // Log handler for transaction
    eosio::checksum256 hash = {};             // Hash of transaction
    std::vector<std::string> errors;          // Keeps track of errors

    uint256_t gas_used;     // Gas used in transaction
    uint256_t gas_refunds;  // Refunds processed in transaction
    std::vector<StateModification> state_modifications;                  // State modifications
    std::map<uint64_t, std::map<uint256_t, uint256_t>> original_storage; // Cache for SSTORE

    // Signature data
    uint8_t v;   // Recovery ID
    uint256_t r; // Output
    uint256_t s; // Output

    // Ram Payer
    eosio::name ram_payer;

    // RLP constructor
    EthereumTransaction(const std::vector<int8_t>& encoded, eosio::name ram_payer_account)
    {
      // Max Transaction size
      eosio::check(encoded.size() < MAX_TX_SIZE, "Invalid Transaction: Max size of a transaction is 128 KB");

      // Decode
      auto rlp = rlp::decode(encoded);
      auto values = rlp.values;

      nonce     = from_big_endian(&values[0].value[0], values[0].value.size());
      gas_price = from_big_endian(&values[1].value[0], values[1].value.size());
      gas_limit = from_big_endian(&values[2].value[0], values[2].value.size());
      to        = values[3].value;
      value     = from_big_endian(&values[4].value[0], values[4].value.size());
      data      = values[5].value;
      v         = values[6].value[0];
      r         = from_big_endian(&values[7].value[0], values[7].value.size());
      s         = from_big_endian(&values[8].value[0], values[8].value.size());

      // Validate To Address
      eosio::check(to.empty() || to.size() == 20, "Invalid Transaction: to address must be 40 characters long if provided (excluding 0x prefix)");
      if (!to.empty()) {
        to_address = from_big_endian(to.data(), to.size());
      }

      // Validate Value
      eosio::check(value >= 0, "Invalid Transaction: Value cannot be negative.");

      // Hash
      hash = keccak_256(encode());

      // RAM Payer
      eosio::require_auth(ram_payer_account);
      ram_payer = ram_payer_account;

      // Gas
      initialize_base_gas();
    }
    ~EthereumTransaction() = default;

    bool is_zero() { return !r && !s; }
    bool is_create() const { return !to_address.has_value(); }
    uint256_t gas_left() const { return gas_limit - gas_used; }
    std::string encode() const { return rlp::encode(nonce, gas_price, gas_limit, to, value, data, v, r, s); }

    void initialize_base_gas () {
      gas_used = GP_TRANSACTION;

      for (auto& i: data) {
        gas_used += i == 0
          ? GP_TXDATAZERO
          : GP_TXDATANONZERO;
      }

      if (is_create()) {
        gas_used += GP_TXCREATE;
      }

      eosio::check(gas_used <= gas_limit, "Gas limit " + to_string(gas_limit) + " is too low for initialization, minimum " + to_string(gas_used) + " required.");
    }

    void to_recoverable_signature(std::array<uint8_t, 65>& sig) const
    {
      // V
      uint8_t recovery_id = from_ethereum_recovery_id(v);
      sig[0] = recovery_id + 27 + 4; // + 4 as it is compressed

      // R and S valdiation
      eosio::check(s >= 1 && s <= intx::from_string<uint256_t>("0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF5D576E7357A4501DDFE92F46681B20A0"), "Invalid Transaction: s value in signature must be between 1 and 0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF5D576E7357A4501DDFE92F46681B20A0, inclusive.");

      // R and S
      intx::be::unsafe::store(sig.data() + 1, r);
      intx::be::unsafe::store(sig.data() + 1 + R_FIXED_LENGTH, s);
    }

    const KeccakHash to_be_signed()
    {
      if (is_pre_eip_155(v))
      {
        return keccak_256(rlp::encode(nonce, gas_price, gas_limit, to, value, data));
      }
      else
      {
        return keccak_256(rlp::encode( nonce, gas_price, gas_limit, to, value, data, CURRENT_CHAIN_ID, 0, 0 ));
      }
    }

    eosio::checksum160 get_sender()
    {
      // Return early if already exists
      if (sender.has_value()) {
        return *sender;
      }

      // Get transaction hash as checksum256 message
      const std::array<uint8_t, 32u> message = to_be_signed();
      eosio::checksum256 messageChecksum = eosio::fixed_bytes<32>(message);

      // Get recoverable signature
      std::array<uint8_t, 65> arr;
      to_recoverable_signature(arr);

      eosio::ecc_signature ecc_sig;
      std::memcpy(ecc_sig.data(), &arr, sizeof(arr));
      eosio::signature signature = eosio::signature { std::in_place_index<0>, ecc_sig };

      // Recover
      eosio::public_key recovered_variant = eosio::recover_key(messageChecksum, signature);
      eosio::ecc_public_key compressed_public_key = std::get<0>(recovered_variant);
      std::vector<uint8_t> proper_compressed_key( std::begin(compressed_public_key), std::end(compressed_public_key) );

      // Decompress the ETH pubkey
      uint8_t public_key[65];
      public_key[0] = MBEDTLS_ASN1_OCTET_STRING; // 04 prefix
      uECC_decompress(proper_compressed_key.data(), public_key + 1, uECC_secp256k1());

      // Hash key to get address
      std::array<uint8_t, 32u> hashed_key = keccak_256(public_key + 1, 64);

      // Set sender
      sender = toChecksum160(hashed_key);

      return *sender;
    }

    uint256_t from_big_endian(const uint8_t* begin, size_t size = 32u)
    {
      // Size validation
      eosio::check(size <= 32, "Invalid Transaction: Calling from_big_endian with oversized array");

      if (size == 32)
      {
        return intx::be::unsafe::load<uint256_t>(begin);
      }
      else
      {
        uint8_t tmp[32] = {};
        const auto offset = 32 - size;
        memcpy(tmp + offset, begin, size);

        return intx::be::load<uint256_t>(tmp);
      }
    }

    /**
     * State Modifications and original storage
     */
    inline void add_modification(const StateModification& modification) {
      state_modifications.emplace_back(modification);
    }
    inline uint256_t find_original(const uint64_t& address_index, const uint256_t& key) {
      if (original_storage.count(address_index) == 0 || original_storage[address_index].count(key) == 0) {
        return 0;
      } else {
        return original_storage[address_index][key];
      }
    }
    inline void emplace_original(const uint64_t& address_index, const uint256_t& key, const uint256_t& value) {
      if (original_storage.count(address_index) == 0 || original_storage[address_index].count(key) == 0) {
        original_storage[address_index][key] = value;
      }
    }

    std::string errors_as_json_string() const {
      std::string output = "[";
      for (auto i = 0; i < errors.size(); i++) {
        output += "\"" + errors[i] + "\"";
        if (i < errors.size() - 1) output += ",";
      }
      output += "]";
      return output;
    }

    void print_receipt(const ExecResult& result) const
    {
      auto status = result.er == ExitReason::returned ? "1" : "0";

      eosio::print(
        "{",
          "\"status\": \"", status, "\",",
          "\"from\": \"", *sender, "\",",
          "\"to\": \"", bin2hex(to), "\",",
          "\"value\": ", intx::to_string(value), ",",
          "\"nonce\": \"", intx::hex(nonce), "\",",
          "\"v\": \"", v, "\",",
          "\"r\": \"", intx::hex(r), "\",",
          "\"s\": \"", intx::hex(s), "\",",
          "\"createdAddress\": \"", created_address ? intx::hex(*created_address) : "", "\",",
          "\"cumulativeGasUsed\": \"", intx::hex(gas_used), "\",",
          "\"gasUsed\": \"", intx::hex(gas_used), "\",",
          "\"gasLimit\": \"", intx::hex(gas_limit), "\",",
          "\"gasPrice\": \"", intx::hex(gas_price), "\",",
          #if(PRINT_LOGS == true)
          "\"logs\": ", logs.as_json_string(), ",",
          #endif
          "\"output\": \"", bin2hex(result.output), "\","
          "\"errors\": ", errors_as_json_string(), ","
          "\"transactionHash\": \"", hash, "\"", ","
          "\"transactionIndex\": \"0\""
        "}"
      );
    }

    #if (TESTING == true)
    void printhex() const
    {
      eosio::print(
        // "data ",      bin2hex(data),        "\n",
        "gasLimit ",  intx::hex(gas_limit), "\n",
        "gasPrice ",  intx::hex(gas_price), "\n",
        "data ",      bin2hex(data),        "\n",
        "nonce ",     intx::hex(nonce),     "\n",
        "to ",        bin2hex(to),          "\n",
        "value ",     intx::hex(value),     "\n",
        "v ",         v,                    "\n"
        "r ",         intx::hex(r),         "\n",
        "s ",         intx::hex(s),         "\n\n",
        "sender ",    *sender,              "\n",
        "hash ",      hash,                 "\n"
      );
    }
    #endif /* TESTING */
  };
} // namespace eosio_evm