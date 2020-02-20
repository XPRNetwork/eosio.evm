# Achievements
- 100% Success on Ethereum Transaction Tests
- 100% Success on Ethereum RLP Tests
- Total size less than 200KB (~2MB on-chain)

# Build instructions
Latest eosio.cdt on develop branch is required
Latest eosio with EOSVM2

```
cmake .
make -j4
```

Note: CMakesList.txt expects eosio to be installed at ~/eosio/2.0, if installed in a different directory, please edit "eosio.evm/CMakeLists.txt"

# Directory structure
- eosio.evm: contains all contract code
  - src: all sourcefiles
  - include/eosio.evm: all headerfiles
  - external: external code for ecc, intx (bigint), keccak256 and rlp
- js: scripts for encoding/decoding transactions for testing
- tests: full Ethereum/EOS tests
  - jsontests: submodule of https://github.com/ethereum/tests
  - system_wasms: eosio.system and eosio.token ABIs/WASMS
  - eosio.evm_tests.cpp: testing suite
- truffle: ERC20 and ERC721 contracts

# EVM Notes
- We assume that maximum "value" of a transaction is within the limits of a int64_t. Any transaction with a "value" greater than 2^62 - 1 will be considered invalid.
- Account States are scoped by the index of the account. The index of an account never changes, thus this is guaranteed to be unique.
- NUMBER opcode returns tapos_block_num, as that is the only EOSIO block number available to contracts
- The RLP encoding in "create" uses RLP (uint64_t eos_account, uint64_t nonce)
- No patricia merkle tree is used
- A value of 1 represents 0.0001 SYS