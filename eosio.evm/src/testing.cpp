#include <eosio.evm/eosio.evm.hpp>

namespace evm4eos {
  // TODO remove in prod
  void evm::testtx(const std::vector<int8_t>& tx) {
    require_auth(get_self());
    std::vector<uint8_t> properTx( tx.begin(), tx.end() );
    auto transaction = EthereumTransaction{ properTx };
    eosio::print(R"({"hash":")", transaction.hash, R"(", "sender":")", transaction.get_sender(), R"("})");
  }

  // TODO remove in prod
  void evm::printtx(const std::vector<int8_t>& tx) {
    require_auth(get_self());
    std::vector<uint8_t> properTx( tx.begin(), tx.end() );
    auto transaction = EthereumTransaction{ properTx };
    transaction.get_sender();
    transaction.printhex();
  }

  // TODO remove in prod
  void evm::devcreate(const eosio::checksum160& address, const eosio::name& account) {
    require_auth(get_self());
    create_account(checksum160ToAddress(address), 0, {}, account);
  }
}