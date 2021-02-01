#include <eosio.evm/eosio.evm.hpp>

#if (TESTING == true)
namespace eosio_evm {
  void evm::testtx(const std::vector<int8_t>& tx) {
    require_auth(get_self());
    auto transaction = EthereumTransaction(tx, get_self());
    eosio::print(R"({"hash":")", transaction.hash, R"(", "sender":")", transaction.get_sender(), R"("})");
  }

  void evm::printtx(const std::vector<int8_t>& tx) {
    require_auth(get_self());
    auto transaction = EthereumTransaction(tx, get_self());
    transaction.get_sender();
    transaction.printhex();
  }

  void evm::devnewacct(const eosio::checksum160& address, const std::string balance, const std::vector<uint8_t> code, const uint64_t nonce, const eosio::name& account) {
    require_auth(get_self());

    // Create account
    auto address_256        = pad160(address);
    auto accounts_byaddress = _accounts.get_index<eosio::name("byaddress")>();
    auto existing_address   = accounts_byaddress.find(address_256);
    eosio::check(existing_address == accounts_byaddress.end(), "An account already exists with this address");

    auto ubalance = intx::from_string<uint256_t>(balance);
    eosio::check(ubalance >= 0, "Balance cannot be negative");
    _accounts.emplace(get_self(), [&](auto& a) {
      a.index   = _accounts.available_primary_key();
      a.address = address;
      a.account = account;
      a.balance = ubalance;
      a.nonce   = nonce;
      a.code    = code;
    });
  }

  void evm::devnewstore(const eosio::checksum160& address, const std::string& key, const std::string value) {
    require_auth(get_self());

    // Get account
    auto accounts_byaddress = _accounts.get_index<eosio::name("byaddress")>();
    auto existing_address   = accounts_byaddress.find(pad160(address));
    eosio::check(existing_address != accounts_byaddress.end(), "address does not exist");

    // Key value
    auto checksum_key   = toChecksum256(intx::from_string<uint256_t>(key));
    auto checksum_value = intx::from_string<uint256_t>(value);

    // Store KV
    account_state_table accounts_states(get_self(), existing_address->index);
    auto accounts_states_bykey = accounts_states.get_index<eosio::name("bykey")>();
    auto account_state         = accounts_states_bykey.find(checksum_key);
    accounts_states.emplace(get_self(), [&](auto& a) {
        a.index   = accounts_states.available_primary_key();
        a.key     = checksum_key;
        a.value   = checksum_value;
    });
  }

  void evm::teststatetx(const std::vector<int8_t>& tx, const Env& env) {
    require_auth(get_self());

    // Set block from env
    set_current_block(env);

    // Execute transaction
    raw(get_self(), tx, std::nullopt);
  }

  void evm::printaccount(const eosio::checksum160& address) {
    auto accounts_byaddress = _accounts.get_index<eosio::name("byaddress")>();
    auto existing_address   = accounts_byaddress.find(pad160(address));

    eosio::print("{");
    if (existing_address != accounts_byaddress.end()) {
      auto code = bin2hex(existing_address->get_code());
      code = code.length() % 2 == 0 ? "0x" + code : "0x0" + code;

      auto nonce = intx::hex(intx::from_string<uint256_t>(std::to_string(existing_address->get_nonce())));
      nonce = nonce.length() % 2 == 0 ? "0x" + nonce : "0x0" + nonce;

      auto balance = intx::hex(existing_address->get_balance());
      balance = balance.length() % 2 == 0 ? "0x" + balance : "0x0" + balance;

      eosio::print("\"code\":\"", code, "\",");
      eosio::print("\"nonce\":\"", nonce, "\",");
      eosio::print("\"balance\":\"", balance, "\"");
    }
    eosio::print("}");
  }

  void evm::printstate(const eosio::checksum160& address) {
    eosio::print("{");

    // Get account
    auto accounts_byaddress = _accounts.get_index<eosio::name("byaddress")>();
    auto existing_address   = accounts_byaddress.find(pad160(address));

    if (existing_address != accounts_byaddress.end()) {
      // Get scoped state table for account
      account_state_table accounts_states(get_self(), existing_address->index);
      auto itr = accounts_states.begin();
      while(itr != accounts_states.end()){
        std::string key = intx::hex(checksum256ToValue(itr->key));
        std::string value = intx::hex(itr->value);

        if (key.length() % 2 == 0) {
          key = "0x" + key;
        } else {
          key = "0x0" + key;
        }

        if (value.length() % 2 == 0) {
          value = "0x" + value;
        } else {
          value = "0x0" + value;
        }

        eosio::print("\"" + key + "\":\"" + value + "\"");

        itr++;

        if (itr != accounts_states.end()) {
          eosio::print(",");
        }
      }
    }

    eosio::print("}");
  }
}
#endif