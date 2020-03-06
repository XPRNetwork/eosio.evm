[![eosio.evm](./evm.png)](#)

### Benchmarks
| Action         | CPU Cost      |
| -------------  |:-------------:|
| ERC20 Transfer | 504µs [(TX)](https://jungle.bloks.io/transaction/eb2d83e1ed04b98d1c7767acae5df174de56ee51a2bf6d1c06a8a863f9b98ca0)|
| ERC20 Deploy   | 764µs [(TX)](https://jungle.bloks.io/transaction/074f2cb4435173293243e4350a9a3faa12e5fb639780aaabb79ad68fb2c813e8)      |
| EVM Transfer   | 325µs [(TX)](https://jungle.bloks.io/transaction/640c061cbd717b08b8af1c28129be1ef7365d1810fc285313a55d44f2271e312)      |
| EVM New Address| 553µs [(TX)](https://jungle.bloks.io/transaction/876ce02ccdc7fd7338fcf9e9fea6ea9e4575211209fe29c88ec33eb63584be84)     |


### Achievements
- Full Javascript SDK for deploying, executing and querying contracts
- 100% Success on Ethereum Transaction Tests
- 100% Success on Ethereum RLP Tests
- REVERT support (challenging on EOS as it requires no use of eosio::check after nonce increment)
- Istanbul support
- Full gas cost calculations (not billed to sender)
- Web3-similar call support (query view functions with no state modifications)
- Total size of ~200KB (With OPTRACE and TESTING set to false)
- ecrecover, sha256, ripdemd160 and identity precompiles supported

NOTE: (TESTING, CHARGE_SENDER_FOR_GAS) must be enabled, and (OPTRACE, PRINT_LOGS) must be disabled in the file eosio.evm/include/eosio.evm/constants.hpp for ethereum/tests testing to pass successfuly. Tests that pass the 32MB RAM limit on EOSIO were disabled.

Some precompiles like ec_mul and ec_add are due to be added as intrinsics to EOSIO soon.

### Usage instructions
Recommended: [eos-evm-js guide](https://github.com/jafri/eosio.evm/tree/master/eos-evm-js)

Basic usage: [cleos guide](https://github.com/jafri/eosio.evm/tree/master/CLEOS-GUIDE.md)


### Build/Manual Deployment instructions
Requires latest eosio.cdt with latest eosio 2 with EOSVM

Change the token symbol in eosio.evm/include/eosio.evm/constants.hpp to reflect your chain

```
cmake .
make -j4
```

No special instructions needed for manual deployment, simply deploy the WASM and ABI from eosio.evm/eosio.evm/ directory


### Directory structure
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
  eosio::asset balance;
  uint64_t nonce;
  std::vector<uint8_t> code;
}
```
- `index` - auto-incremented counter for accounts, also used as scope index for AccountStates
- `address` - Ethereum 160 bit address
- `account` - EOSIO account associated with Ethereum account
- `balance` - EOSIO asset balance associated with Ethereum account
- `nonce` - Current nonce of the account
- `code` - Contract code for Ethereum account if present

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


### EVM Notes
- We assume that maximum "value" of a transaction is within the limits of a int64_t. Any transaction with a "value" greater than 2^62 - 1 will be considered invalid.
- Account States are scoped by the index of the account. The index of an account never changes, thus this is guaranteed to be unique.
- NUMBER opcode returns tapos_block_num, as that is the only EOSIO block number available to contracts
- The RLP encoding in "create" uses RLP (uint64_t eos_account, uint64_t nonce)
- No patricia merkle tree is used
- A value of 1 represents 0.0001 SYS
- Precompiled contracts are not currently supported, many like ec_mul and ec_add are due to be added to EOSIO soon


### Special Mentions
- Eddy Ashton for his work on [enclave-ready EVM (eEVM)](https://github.com/microsoft/eEVM)
- Pawel Bylica for his work on pushing the speed limits of EVMs with [evmone](https://github.com/ethereum/evmone)
- winsvega for his continous work maintaining [Ethereum Tests](https://github.com/ethereum/tests)


### Disclosure
Note that this repository is still in a highly iterative state, if you find any bugs, please open an issue or a pull request.