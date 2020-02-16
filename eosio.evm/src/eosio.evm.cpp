#include <eosio.evm/eosio.evm.hpp>

namespace evm4eos {

void evm::create (
  const eosio::name& account,
  const std::string& data
) {
  // Check that account is authorized
  require_auth(account);

  // Check that account does not already exist
  auto accounts_byaccount = _accounts.get_index<eosio::name("byaccount")>();
  auto existing_account = accounts_byaccount.find(account.value);
  eosio::check(existing_account == accounts_byaccount.end(), "an EVM account is already linked to this EOS account.");

  // Encode using RLP, Hash and get right-most 160 bits (Address)
  const auto rlp_encoding = rlp::encode(account.value, data);
  eosio::checksum160 address = toChecksum160( keccak_256(rlp_encoding) );

  // Create user account
  create_account(checksum160ToAddress(address), 0, {}, account);
}

// TODO remove in prod
void evm::testtx(const std::vector<int8_t>& tx) {
  std::vector<uint8_t> properTx( tx.begin(), tx.end() );
  auto transaction = EthereumTransaction{ properTx };
  eosio::print(R"({"hash":")", transaction.hash, R"(", "sender":")", transaction.get_sender(), R"("})");
}
// TODO remove in prod
void evm::printtx(const std::vector<int8_t>& tx) {
  std::vector<uint8_t> properTx( tx.begin(), tx.end() );
  auto transaction = EthereumTransaction{ properTx };
  transaction.get_sender();
  transaction.printhex();
}

void evm::raw(
  const std::vector<int8_t>& tx,
  const std::optional<eosio::checksum160>& sender
) {
  std::vector<uint8_t> properTx( tx.begin(), tx.end() );

  // Create transaction
  auto transaction = EthereumTransaction{ properTx };
  transaction.print();

  // Index by address
  auto accounts_byaddress = _accounts.get_index<eosio::name("byaddress")>();
  decltype(accounts_byaddress.begin()) from_account;

  // The “R” and “S” values of the transaction are 0 (EOS SPECIAL)
  if (transaction.is_zero())
  {
    // Ensure sender exists
    eosio::check(sender.has_value(), "Invalid Transaction: no signature in transaction and no sender address was provided.");
    from_account = accounts_byaddress.find(pad160(*sender));
    eosio::check(from_account != accounts_byaddress.end(), "Invalid Transaction: sender address does not exist (without signature).");

    // Ensure EOSIO account is associated
    eosio::check(from_account->get_account() != 0, "Invalid Transaction: no EOSIO account is associated with sender's address.");

    // Check authorization of associated EOSIO account
    // TODO maybe charge only the sending EOSIO account of the entire TX (not thereum) for RAM
    eosio::require_auth(from_account->account);

    // Set transaction sender
    transaction.sender = *sender;
  }
  // The “R” and “S” values of the transaction are NOT 0
  else
  {
    // EC RECOVERY
    eosio::checksum256 sender_256 = pad160(transaction.get_sender());

    // Ensure signer exists
    from_account = accounts_byaddress.find(sender_256);
    eosio::check(from_account != accounts_byaddress.end(), "Invalid Transaction: sender address does not exist (with signature).");
  }

  // Check account nonce
  eosio::check(from_account->get_nonce() == transaction.nonce, "Invalid Transaction: incorrect nonce.");
  accounts_byaddress.modify(from_account, eosio::same_payer, [&](auto& a) {
    a.nonce += 1;
  });

  // Initialize base gas
  transaction.initialize_base_gas();

  /**
   *
   *
   * Savepoint:
   * Anything from this point on could be reverted
   * CANT THROW/EOSIO::ASSERT
   *
   *
   */

  Address to_address;
  auto from_160 = checksum160ToAddress(*transaction.sender);

  // CREATE
  if (transaction.is_create()) {
    to_address = generate_address(from_account->get_address(), from_account->get_nonce() - 1);
  // CALL
  } else {
    to_address = *transaction.to_address;
  }

  // Transfer value balance (will make "to" account not if does not exist currently).
  transfer_internal(from_account->get_address(), to_address, transaction.get_value());

  // Execute transaction
  const ExecResult exec_result = Processor().run(transaction, from_160, get_account(to_address), this);

  // Success
  if (exec_result.er == ExitReason::returned)
  {
    // New contract created, result is the code that should be deployed
    if (transaction.is_create()) {
      set_code(to_address, std::move(exec_result.output));
    }

    // Print output (DEBUG).
    // eosio::print("OUTPUT:", bin2hex(exec_result.output) );
  }
  // Error
  else
  {
    eosio::check(exec_result.er != ExitReason::threw, "EVM Execution Error: " + exec_result.exmsg);
    // eosio::check(false, "EVM Execution Error: " + exec_result.exmsg);
  }
}
}