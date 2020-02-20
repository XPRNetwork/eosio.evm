#include <eosio.evm/eosio.evm.hpp>

namespace eosio_evm{

void evm::create (
  const eosio::name& account,
  const std::string& data
) {
  // Check that account is authorized
  require_auth(account);

  // Encode using RLP(account.value, data)
  auto accounts_byaccount = _accounts.get_index<eosio::name("byaccount")>();
  auto existing_account = accounts_byaccount.find(account.value);
  eosio::check(existing_account == accounts_byaccount.end(), "an EVM account is already linked to this EOS account.");

  // Encode using RLP, Hash and get right-most 160 bits (Address)
  const auto rlp_encoding = rlp::encode(account.value, data);
  eosio::checksum160 address = toChecksum160( keccak_256(rlp_encoding) );

  // Create user account
  const Account* created_account = create_account(checksum160ToAddress(address), 0, {}, account);
  eosio::check(created_account != NULL, "an EVM account with this address already exists.");
}

void evm::raw(
  const std::vector<int8_t>& tx,
  const std::optional<eosio::checksum160>& sender
) {
  std::vector<uint8_t> properTx( tx.begin(), tx.end() );

  // Create transaction
  auto transaction = EthereumTransaction{ properTx };
  // transaction.print();

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
    eosio::check(from_account->get_account_value() != 0, "Invalid Transaction: no EOSIO account is associated with sender's address.");

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
  eosio::check(from_account->get_nonce() == transaction.nonce, "Invalid Transaction: incorrect nonce, received " + to_string(transaction.nonce) + " expected " + std::to_string(from_account->get_nonce()));
  accounts_byaddress.modify(from_account, eosio::same_payer, [&](auto& a) {
    a.nonce += 1;
  });

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

  const Account* callee;
  std::vector<uint8_t> code;

  // CREATE
  if (transaction.is_create()) {
    to_address = generate_address(from_account->get_address(), from_account->get_nonce() - 1);
    callee = create_account(to_address, 0, {}, {});
    if (callee == NULL) {
      return eosio::print("Execution Error: ", intx::hex(to_address), " already exists.");
    }
    callee->print();
    code = transaction.data;
  // CALL
  } else {
    to_address = *transaction.to_address;
    callee = &get_account(to_address);
    code = callee->get_code();
  }

  // Transfer value balance (will make "to" account not if does not exist currently).
  transfer_internal(from_account->get_address(), to_address, transaction.get_value());

  // Execute transaction
  auto processor = Processor(transaction, this);
  const ExecResult exec_result = processor.run(
    from_160,
    *callee,
    transaction.gas_left(),
    false, // Is Static
    transaction.data,
    code,
    transaction.get_value()
  );

  // clean-up
  for (const auto& addr : transaction.selfdestruct_list) {
    selfdestruct(addr);
  }

  // Success
  if (exec_result.er == ExitReason::returned)
  {
    // New contract created, result is the code that should be deployed
    if (transaction.is_create()) {
      // Validate size
      const auto output_size = exec_result.output.size();
      if (output_size >= MAX_CODE_SIZE) {
        eosio::print("Code is larger than max code size, out of gas!");
        return;
      }

      // Charge create data gas
      transaction.gas_used += output_size * GP_CREATE_DATA;

      // Set code if not empty
      if (output_size > 0) {
        set_code(to_address, std::move(exec_result.output));
      }
    }
  }
  // Error
  else
  {
    eosio::print("EVM Execution Error: ", int(exec_result.er), ", ", exec_result.exmsg);
  }
}

} // namepsace eosio_evm