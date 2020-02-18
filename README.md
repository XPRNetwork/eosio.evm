# Achievements
- WIP 100% Success on Ethereum General State Tests
- 100% Success on Ethereum Transaction Tests
- 100% Success on Ethereum RLP Tests
- Total size less than 200KB (~2MB on-chain)

# EVM Notes
- We assume that maximum "value" of a transaction is within the limits of a int64_t.
  - Any transaction with a "value" greater than 2^62 - 1 will be considered invalid.
- For efficient storage, the key for any storage variable is defined as:
  - SHA256(20 bit address + 32 bit storage address)
  - We opted to use SHA256 here for performance due to the availability of it as an intrinsic in EOSIO, and it can be replaced by Keccak 256 at any time.
- NUMBER opcode returns tapos_block_num, as that is the only EOSIO block num available to contracts
- The RLP encoding in "create" uses RLP (uint64_t eos_account, uint64_t nonce)
- No patricia merkle tree is used