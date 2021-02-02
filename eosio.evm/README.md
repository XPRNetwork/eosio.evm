
## Disclaimer 

The code in this repository has not been audited by a professional audit firm. Please use at your own discretion and risk. The author takes no responsibility for any material or inmaterial losses from any form of operation or deployment of the software within this repository.

# Contract
Configure eosio.evm/include/eosio.evm/constants.hpp as needed with constants such as token symbol.

Please note that setting the JS-SDK will not currently work if OPTRACE or PRINT_STATE are set to true as it will print more information to EOSIO console.

### EVM Notes
- Account and code tables were merged to match the specification in the Ethereum Yellow Paper
- Account States are scoped by the index of the account. The index of an account never changes, thus this is guaranteed to be unique.
- NUMBER opcode returns tapos_block_num, as that is the only EOSIO block number available to contracts
- The RLP encoding in "create" uses RLP (uint64_t eos_account, uint64_t nonce)
- No patricia merkle tree is used
- The balance value is printed in explorers, etc as a big-endian hex value with a precision of 10^18, as specified in Ethereum yellow paper. Therefore, 1 EOS is represented as 1000000000000000000, which is de0b6b3a7640000 in big-endian hex.

### Precompile support
eosio.evm supports all 9 precompiles
1. ec_recover
2. sha256
3. ripemd160
4. identity
5. expmod
6. bn_add
7. bn_mul
8. bn_pairing
9. blake2b

## Constants
All constants are found at [constants.hpp](eosio.evm/include/eosio.evm/constants.hpp)

1. **TESTING** - adds functionality for executing tests, and resetting the contract; default true, remove in production
2. **BN_CURVE** - adds bnadd, bnmul, and bnpair precompiles; default true
3. **CHARGE_SENDER_FOR_GAS** - toggle charging sender for gas; default false, required for ethereum tests
4. **PRINT_LOGS** - prints logs as part of execution receipt; default false
5. **OPTRACE** - prints the opcode trace for the execution; default false
6. **PRINT_STATE** - prints the state when saved or loaded from tables; default false
7. **TOKEN_SYMBOL_CODE_RAW** - the symbol of the core token on-chain; default "EOS"
8. **TOKEN_CONTRACT_RAW** - the contract of the core token on-chain; default "eosio.token"
9. **TOKEN_PRECISION** - the precision of the core symbol on-chain; default 4

**NOTE:** [TESTING, CHARGE_SENDER_FOR_GAS] must be enabled, and [OPTRACE, PRINT_LOGS] must be disabled for ethereum/tests testing to pass successfuly.

**NOTE:** If ec_add, ec_mul and ec_pairing precompiles are required, set BN_CURVE to true (will increase WASM size by 210KB, or ~2MB onchain).

### Contract Public Actions
```c++
ACTION raw ( const eosio::name& ram_payer,
             const std::vector<int8_t>& tx,
             const std::optional<eosio::checksum160>& sender);
```
- `ram_payer` Name of account paying for RAM costs
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

ACTION call( const eosio::name& ram_payer,
             const std::vector<int8_t>& tx,
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
- `balance` - 256 bit balance stored as a bigint (shows as big endian when printed). The precision is 10^18 as specified in Ethereum whitepaper. 1 EOS is thus represented as 1000000000000000000 which is de0b6b3a7640000 in big-endian hex.

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