#include <eosio.evm/eosio.evm.hpp>

namespace eosio_evm {
  void evm::transfer (const eosio::name& from, const eosio::name& to, const eosio::asset& quantity, const std::string& memo) {
    bool outgoing = from == get_self();
    bool system_deposit = from == "eosio.stake"_n || from == "eosio.ram"_n || from == "eosio"_n;
    if (outgoing || system_deposit) {
      return;
    }

    bool valid_symbol = quantity.symbol == TOKEN_SYMBOL;
    bool valid_contract = get_first_receiver() == TOKEN_CONTRACT;
    bool valid_to = to == get_self();
    eosio::check(valid_symbol && valid_contract && valid_to, "Invalid Deposit");

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
  //  - We need arbitrary input from EOS accounts to create RLP(eosaccount, arbitrary)
  void evm::add_balance (
    const eosio::name& user,
    const eosio::asset& quantity
  ) {
    if (quantity.amount == 0) return;

    auto accounts_byaccount = _accounts.get_index<eosio::name("byaccount")>();
    auto account = accounts_byaccount.find(user.value);
    eosio::check(account != accounts_byaccount.end(), "There are no ETH accounts associated with " + user.to_string());
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
      create_account(address, amount);
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

  void evm::transfer_internal(const Address& from, const Address& to, const int64_t amount) {
    if (amount == 0) return;

    sub_balance(from, amount);
    add_balance(to, amount);
  }
}