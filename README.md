# Achievements
- 100% Success on Ethereum Transaction Tests
- 100% Success on Ethereum RLP Tests
- Total size less than 200KB (~2MB on-chain)
- REVERT fully supported (challenging on EOS as it required no use of eosio::check after nonce increment)

# Build instructions
Latest eosio.cdt
Latest eosio with EOSVM2

```
cmake .
make -j4
```

# Directory structure
- eosio.evm: contains all contract code
  - src: all sourcefiles
  - include/eosio.evm: all headerfiles
  - external: external code for ecc, intx (bigint), keccak256 and rlp
- eos-evm-js: Full JS SDK for deploying both EVM and Ethereum accounts, contracts, fetching state, etc.
- js-tests: scripts for encoding/decoding transactions for testing
- tests: full Ethereum/EOS tests
  - jsontests: submodule of https://github.com/ethereum/tests
  - system_wasms: eosio.system and eosio.token ABIs/WASMS
  - eosio.evm_tests.cpp: testing suite
- truffle: ERC20 and ERC721 contracts

# Contract Public Actions
ACTION raw      ( const std::vector<int8_t>& tx,
                  const std::optional<eosio::checksum160>& sender);

`tx` will take a raw Ethereum transaction RLP hex encoded without the '0x' prefix
`sender` is an optional parameter used when the `tx` is not signed

ACTION create   ( const eosio::name& account,
                  const std::string& data);
`account` is the EOSIO account creating the new Ethereum account
`data` is an arbitrary string used as a salt to create the new Ethereum account

ACTION withdraw ( const eosio::name& to,
                  const eosio::asset& quantity);
`account` is the EOSIO account associated with an Ethereum account with a balance
`quantity` is an EOSIO asset like "4.0000 SYS" for the amount to withdraw

[[eosio::on_notify("eosio.token::transfer")]]
void transfer( const eosio::name& from,
                const eosio::name& to,
                const eosio::asset& quantity,
                const std::string& memo );
Standard transfer function used to deposit balance into associated Ethereum account. If the depositor does not have an EVM account associated, the transaction will fail to execute.

# Contract Tables
```c++
struct Account {
  uint64_t index;
  eosio::checksum160 address;
  eosio::name account;
  eosio::asset balance;
  uint64_t nonce;
  std::vector<uint8_t> code;
}
```
`index` - auto-incremented counter for accounts, also used as scope index for AccountStates
`address` - Ethereum 160 bit address
`account` - EOSIO account associated with Ethereum account
`balance` - EOSIO asset balance associated with Ethereum account
`nonce` - Current nonce of the account
`code` - Contract code for Ethereum account if present

```c++
struct AccountState {
  uint64_t index;
  eosio::checksum256 key;
  bigint::checksum256 value;
}
```
`index` - auto-incremented counter for account states, only used as primary key
`key` - big-endian encoded key for storage
`value` - big-endian encoded value for storage

# EVM Notes
- We assume that maximum "value" of a transaction is within the limits of a int64_t. Any transaction with a "value" greater than 2^62 - 1 will be considered invalid.
- Account States are scoped by the index of the account. The index of an account never changes, thus this is guaranteed to be unique.
- NUMBER opcode returns tapos_block_num, as that is the only EOSIO block number available to contracts
- The RLP encoding in "create" uses RLP (uint64_t eos_account, uint64_t nonce)
- No patricia merkle tree is used
- A value of 1 represents 0.0001 SYS