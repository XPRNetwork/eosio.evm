- Transaction gas used
- Transaction gas refunds
- Transaction logs
- Transaction selfdestruct list
- Transaction original storage

loadkv
storekv
increment_nonce
transfer_internal
set_code
get_account

enum StateModificationType {
  USE_GAS
  REFUND_GAS
  LOG
  SELFDESTRUCT
  ORIGINAL_STORAGE
  STORE_KV
  INCREMENT_NONCE
  TRANSFER
  SET_CODE
}

struct StateModification {
  uint256_t index;
  uint256_t key;
  uint256_t value;
}

vector<StateModification> state_modifications;
Reversal:

Substract transaction gas (amount)
Add gas refund (amount)
Pop log ()
Pop selfdestruct list ()
originalstorage (key, oldvalue)
remove/modify kv (index, key, oldvalue)
decrement nonce (address, oldnonce)
reverse transfer (amount, from, to)
remove code (address)
