#include <eosio.evm/eosio.evm.hpp>

#ifdef TESTING
namespace eosio_evm {
  void evm::testtx(const std::vector<int8_t>& tx) {
    require_auth(get_self());
    std::vector<uint8_t> properTx( tx.begin(), tx.end() );
    auto transaction = EthereumTransaction{ properTx };
    eosio::print(R"({"hash":")", transaction.hash, R"(", "sender":")", transaction.get_sender(), R"("})");
  }

  void evm::printtx(const std::vector<int8_t>& tx) {
    require_auth(get_self());
    std::vector<uint8_t> properTx( tx.begin(), tx.end() );
    auto transaction = EthereumTransaction{ properTx };
    transaction.get_sender();
    transaction.printhex();
  }

  void evm::devnewacct(const eosio::checksum160& address, const uint64_t balance, const std::vector<uint8_t> code, const uint64_t nonce, const eosio::name& account) {
    require_auth(get_self());
    create_account(checksum160ToAddress(address), balance, code, account);

    // Modify nonce
    auto accounts_byaddress = _accounts.get_index<eosio::name("byaddress")>();
    auto existing_address   = accounts_byaddress.find(pad160(address));
    accounts_byaddress.modify(existing_address, eosio::same_payer, [&](auto& a) {
      a.nonce = nonce;
    });
  }

  void evm::devnewstore(const eosio::checksum160& address, const std::string& key, const std::string value) {
    require_auth(get_self());

    // Get account
    auto accounts_byaddress = _accounts.get_index<eosio::name("byaddress")>();
    auto existing_address   = accounts_byaddress.find(pad160(address));
    eosio::check(existing_address != accounts_byaddress.end(), "address does not exist");

    storekv(
      existing_address->index,
      intx::from_string<uint256_t>(key),
      intx::from_string<uint256_t>(value)
    );
  }

  void evm::teststatetx(const std::vector<int8_t>& tx, const Env& env) {
    require_auth(get_self());

    // Set block from env
    set_current_block(env);

    // Execute transaction
    raw(tx, std::nullopt);
  }

  void evm::printstate(const eosio::checksum160& address) {
    eosio::print("[");

    // Get account
    auto accounts_byaddress = _accounts.get_index<eosio::name("byaddress")>();
    auto existing_address   = accounts_byaddress.find(pad160(address));

    if (existing_address != accounts_byaddress.end()) {
      // Get scoped state table for account
      account_state_table accounts_states(get_self(), existing_address->index);
      auto itr = accounts_states.begin();
      while(itr != accounts_states.end()){
        eosio::print(R"({"key":")", intx::hex(to_key(itr->key)), R"(","value":")", intx::hex(itr->value), R"("})");
        itr++;
      }
    }

    eosio::print("]");
  }
}
#endif