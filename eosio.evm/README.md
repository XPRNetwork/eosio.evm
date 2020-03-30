# Contract
Configure eosio.evm/include/eosio.evm/constants.hpp as needed with constants such as token symbol.

Please note that setting the JS-SDK will not currently work if OPTRACE or PRINT_STATE or set to true as it will print more information to EOSIO console.

### EVM Notes
- We assume that maximum "value" of a transaction is within the limits of a int64_t. Any transaction with a "value" greater than 2^62 - 1 will be considered invalid.
- Account States are scoped by the index of the account. The index of an account never changes, thus this is guaranteed to be unique.
- NUMBER opcode returns tapos_block_num, as that is the only EOSIO block number available to contracts
- The RLP encoding in "create" uses RLP (uint64_t eos_account, uint64_t nonce)
- Precompiled contracts are not currently supported, many like ec_mul and ec_add are due to be added to EOSIO soon
- No patricia merkle tree is used
- A value of 1 represents 0.0001 SYS
- Account and code tables were merged to match the specification in the Ethereum Yellow Paper

### Contract Public Actions
```c++
ACTION raw ( const std::vector<int8_t>& tx,
             const std::optional<eosio::checksum160>& sender);
```
- `tx` will take a raw Ethereum transaction RLP hex encoded without the '0x' prefix
- `sender` is an optional parameter used when the `tx` is not signed
&nbsp;

```c++
ACTION create ( const eosio::name& account,
                const std::string& data);
```
- `account` is the EOSIO account creating the new Ethereum account
- `data` is an arbitrary string used as a salt to create the new Ethereum account
&nbsp;

```c++
ACTION withdraw ( const eosio::name& to,
                  const eosio::asset& quantity);
```
- `account` is the EOSIO account associated with an Ethereum account with a balance
- `quantity` is an EOSIO asset like "4.0000 SYS" for the amount to withdraw
&nbsp;

```c++
[[eosio::on_notify("eosio.token::transfer")]]
void transfer( const eosio::name& from,
                const eosio::name& to,
                const eosio::asset& quantity,
                const std::string& memo );
```
- Standard transfer function used to deposit balance into associated Ethereum account. If the depositor does not have an EVM account associated, the transaction will fail to execute.
&nbsp;
```c++

ACTION call( const std::vector<int8_t>& tx,
             const std::optional<eosio::checksum160>& sender );
```
- Function to mock execute and view result (no state modifications are persisted), similiar to Web3 call()
&nbsp;

### Contract Tables
```c++
struct Account {
  uint64_t index;
  eosio::checksum160 address;
  eosio::name account;
  uint64_t nonce;
  std::vector<uint8_t> code;
  bigint::checksum256 balance;
}
```
- `index` - auto-incremented counter for accounts, also used as scope index for AccountStates
- `address` - Ethereum 160 bit address
- `account` - EOSIO account associated with Ethereum account
- `nonce` - Current nonce of the account
- `code` - Contract code for Ethereum account if present
- `balance` - 256 bit balance stored as a bigint (shows as big endian when printed)

```c++
struct AccountState {
  uint64_t index;
  eosio::checksum256 key;
  bigint::checksum256 value;
}
```
- `index` - auto-incremented counter for account states, only used as primary key
- `key` - big-endian encoded key for storage
- `value` - big-endian encoded value for storage

### Special Mentions
- Eddy Ashton for his work on [enclave-ready EVM (eEVM)](https://github.com/microsoft/eEVM)
- Pawel Bylica for his work on pushing the speed limits of EVMs with [evmone](https://github.com/ethereum/evmone)
- winsvega for his continous work maintaining [Ethereum Tests](https://github.com/ethereum/tests)

### Disclosure
Note that this repository is still in a highly iterative state, if you find any bugs, please open an issue or a pull request.