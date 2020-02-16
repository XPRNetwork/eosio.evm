#include <eosio.evm/eosio.evm.hpp>

namespace evm4eos {
  void evm::transfer (const eosio::name& from, const eosio::name& to, const eosio::asset& quantity, const std::string& memo) {
    auto token_contract = get_first_receiver();

    if (quantity.symbol != TOKEN_SYMBOL) {
      return;
    }

    // Do not process deposits from a contract other than as specified
    if (token_contract != TOKEN_CONTRACT) {
      return;
    }

    // Do not process deposits from system accounts
    if (from == "eosio.stake"_n || from == "eosio.ram"_n || from == "eosio"_n) {
      return;
    }

    // Do not process deposits to anyone but self
    if (to != get_self()) {
      return;
    }

    // process deposit
    add_balance(from, quantity);
  }

  void evm::withdraw (
    const eosio::name& to,
    const eosio::asset& quantity
  ) {
    require_auth( to );

    // Substract account
    sub_balance(to, quantity);

    // Withdraw
    evm::transfer_action t_action( TOKEN_CONTRACT, {get_self(), "active"_n} );
    t_action.send(get_self(), to, quantity, std::string("Withdraw balance for: " + to.to_string()));
  }

  // Does NOT create address if not found
  //  -> we need arbitrary input from EOS accounts to create RLP(eosaccount, address)
  void evm::add_balance (
    const eosio::name& user,
    const eosio::asset& quantity
  ) {
    if (quantity.amount == 0) return;
    auto accounts_byaccount = _accounts.get_index<eosio::name("byaccount")>();
    auto account = accounts_byaccount.find(user.value);
    eosio::check(account != accounts_byaccount.end(), "account does not have a balance (add_balance).");
    eosio::check(quantity.amount >= 0, "amount must not be negative");

    accounts_byaccount.modify(account, eosio::same_payer, [&](auto& a) {
      a.balance += quantity;
    });
  }

  // Creates address if not found
  void evm::add_balance (
    const Address& address,
    const int64_t& amount
  ) {
    if (amount == 0) return;
    auto accounts_byaddress = _accounts.get_index<eosio::name("byaddress")>();
    auto account = accounts_byaddress.find(toChecksum256(address));
    eosio::check(amount >= 0, "amount must not be negative");

    // Exists
    if (account != accounts_byaddress.end()) {
      accounts_byaddress.modify(account, eosio::same_payer, [&](auto& a) {
        a.balance.amount += amount;
      });
    // Does not exist
    } else {
      create_account(address, amount, {}, {});
    }
  }

  void evm::sub_balance (
    const eosio::name& user,
    const eosio::asset& quantity
  ) {
    if (quantity.amount == 0) return;
    auto accounts_byaccount = _accounts.get_index<eosio::name("byaccount")>();
    auto account = accounts_byaccount.find(user.value);
    eosio::check(account != accounts_byaccount.end(), "account does not have a balance (sub_balance)..");
    eosio::check(quantity.amount >= 0, "amount must not be negative");
    eosio::check(account->balance.amount >= quantity.amount, "account balance too low.");

    accounts_byaccount.modify(account, eosio::same_payer, [&](auto& a) {
      a.balance -= quantity;
    });
  }

  void evm::sub_balance (
    const Address& address,
    const int64_t& amount
  ) {
    if (amount == 0) return;
    auto accounts_byaddress = _accounts.get_index<eosio::name("byaddress")>();
    auto account = accounts_byaddress.find(toChecksum256(address));
    eosio::check(account != accounts_byaddress.end(), "account does not have a balance (sub_balance).");
    eosio::check(amount >= 0, "amount must not be negative");
    eosio::check(account->get_balance() >= amount, "account balance too low.");

    accounts_byaddress.modify(account, eosio::same_payer, [&](auto& a) {
      a.balance.amount -= amount;
    });
  }
}