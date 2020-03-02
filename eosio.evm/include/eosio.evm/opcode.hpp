// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License.

#pragma once

namespace eosio_evm
{
  /**
   * All opcodes as specified in original Ethereum Yellow Paper
   */
  enum Opcode : uint8_t
  {
    // 0s: Stop and Arithmetic Operations
    STOP       = 0x00, // Halts execution
    ADD        = 0x01, // Addition operation
    MUL        = 0x02, // Multiplication operation
    SUB        = 0x03, // Substraction operation
    DIV        = 0x04, // Integer division operation
    SDIV       = 0x05, // Signed integer division operation (truncated)
    MOD        = 0x06, // Modulo remainder operation
    SMOD       = 0x07, // Signed modulo remainder operation
    ADDMOD     = 0x08, // Modulo addition operation
    MULMOD     = 0x09, // Modulo multiplication operation
    EXP        = 0x0a, // Exponential operation
    SIGNEXTEND = 0x0b, // Extend length of two’s complement signed integer.

    // 10s: Comparison & Bitwise Logic Operations
    LT     = 0x10,   // Less-than comparison
    GT     = 0x11,   // Greater-than comparison
    SLT    = 0x12,   // Signed less-than comparison
    SGT    = 0x13,   // Signed greater-than comparison
    EQ     = 0x14,   // Equality comparison
    ISZERO = 0x15,   // Simple not operator
    AND    = 0x16,   // Bitwise AND operation
    OR     = 0x17,   // Bitwise OR operation
    XOR    = 0x18,   // Bitwise XOR operation
    NOT    = 0x19,   // Bitwise NOT operation
    BYTE   = 0x1a,   // Retrieve single byte from word
    SHL    = 0x1b,   // 256-bit shift left
    SHR    = 0x1c,   // 256-bit shift right
    SAR    = 0x1d,   // int256 shift right

    // 20s: SHA3
    SHA3   = 0x20, // Compute Keccak-256 hash.

    // 30s: Environmental Information
    ADDRESS        = 0x30, // Get address of currently executing account
    BALANCE        = 0x31, // Get balance of the given account.
    ORIGIN         = 0x32, //  Get execution origination address (This is the sender of original transaction; it is never an account with non-emptyassociated code.)
    CALLER         = 0x33, // Get caller address (This is the address of the account that is directly responsible for this execution.)
    CALLVALUE      = 0x34, // Get deposited value by the instruction/transaction responsible for this execution.
    CALLDATALOAD   = 0x35, // Get input data of current environment. (This pertains to the input data passed with the message call instruction or transaction.)
    CALLDATASIZE   = 0x36, // Get size of input data in current environment (This pertains to the input data passed with the message call instruction or transaction.)
    CALLDATACOPY   = 0x37, // Copy input data in current environment to memory. (This pertains to the input data passed with the message call instruction or transaction.)
    CODESIZE       = 0x38, // Get size of code running in current environment.
    CODECOPY       = 0x39, // Copy code running in current environment to memory
    GASPRICE       = 0x3a, // Get price of gas in current environment. (This is gas price specified by the originating transaction.)
    EXTCODESIZE    = 0x3b, // Get size of an account’s code..
    EXTCODECOPY    = 0x3c, // Copy an account’s code to memory
    RETURNDATASIZE = 0x3d, // Get size of output data from the previous call from the current environment.
    RETURNDATACOPY = 0x3e, // Copy output data from the previous call to memory.
    EXTCODEHASH    = 0x3f, // Get hash of external code

    // 40s: Block Information
    BLOCKHASH   = 0x40, // Get the hash of one of the 256 most recent complete blocks.
    COINBASE    = 0x41, // Get the block’s beneficiary address
    TIMESTAMP   = 0x42, // Get the block’s timestamp.
    NUMBER      = 0x43, // Get the block’s number
    DIFFICULTY  = 0x44, // Get the block’s difficulty.
    GASLIMIT    = 0x45, // Get the block’s gas limit
    CHAINID     = 0x46, // Get the chain ID
    SELFBALANCE = 0x47, // Get the balance of current account

    // 50s: Stack, Memory, Storage and Flow Operations
    POP      = 0x50, // Remove item from stack.
    MLOAD    = 0x51, // Load word from memory.
    MSTORE   = 0x52, // Save word to memory
    MSTORE8  = 0x53, // Save byte to memory
    SLOAD    = 0x54, // Load word from storage.
    SSTORE   = 0x55, // Save word to storage
    JUMP     = 0x56, // Alter the program counter.
    JUMPI    = 0x57, // Conditionally alter the program counter.
    PC       = 0x58, // Get the value of the program counter prior to the increment corresponding to this instruction.
    MSIZE    = 0x59, // Get the size of active memory in bytes.
    GAS      = 0x5a, // Get the amount of available gas, including the corresponding reduction for the cost of this instruction.
    JUMPDEST = 0x5b, // Mark a valid destination for jumps. This operation has no effect on machine state during execution

    // 60s & 70s: Push Operations
    PUSH1  = 0x60, // Place 1 byte item on stack.
    PUSH2  = 0x61, // Place 2 byte item on stack.
    PUSH3  = 0x62, // Place 3 byte item on stack.
    PUSH4  = 0x63, // Place 4 byte item on stack.
    PUSH5  = 0x64, // Place 5 byte item on stack.
    PUSH6  = 0x65, // Place 6 byte item on stack.
    PUSH7  = 0x66, // Place 7 byte item on stack.
    PUSH8  = 0x67, // Place 8 byte item on stack.
    PUSH9  = 0x68, // Place 9 byte item on stack.
    PUSH10 = 0x69, // Place 10 byte item on stack.
    PUSH11 = 0x6a, // Place 11 byte item on stack.
    PUSH12 = 0x6b, // Place 12 byte item on stack.
    PUSH13 = 0x6c, // Place 13 byte item on stack.
    PUSH14 = 0x6d, // Place 14 byte item on stack.
    PUSH15 = 0x6e, // Place 15 byte item on stack.
    PUSH16 = 0x6f, // Place 16 byte item on stack.
    PUSH17 = 0x70, // Place 17 byte item on stack.
    PUSH18 = 0x71, // Place 18 byte item on stack.
    PUSH19 = 0x72, // Place 19 byte item on stack.
    PUSH20 = 0x73, // Place 20 byte item on stack.
    PUSH21 = 0x74, // Place 21 byte item on stack.
    PUSH22 = 0x75, // Place 22 byte item on stack.
    PUSH23 = 0x76, // Place 23 byte item on stack.
    PUSH24 = 0x77, // Place 24 byte item on stack.
    PUSH25 = 0x78, // Place 25 byte item on stack.
    PUSH26 = 0x79, // Place 26 byte item on stack.
    PUSH27 = 0x7a, // Place 27 byte item on stack.
    PUSH28 = 0x7b, // Place 28 byte item on stack.
    PUSH29 = 0x7c, // Place 29 byte item on stack.
    PUSH30 = 0x7d, // Place 30 byte item on stack.
    PUSH31 = 0x7e, // Place 31 byte item on stack.
    PUSH32 = 0x7f, // Place 32 byte item on stack.

    // 80s: Duplication Operation
    DUP1  = 0x80, // Duplicate 1st stack item
    DUP2  = 0x81, // Duplicate 2nd stack item
    DUP3  = 0x82, // Duplicate 3rd stack item
    DUP4  = 0x83, // Duplicate 4th stack item
    DUP5  = 0x84, // Duplicate 5th stack item
    DUP6  = 0x85, // Duplicate 6th stack item
    DUP7  = 0x86, // Duplicate 7th stack item
    DUP8  = 0x87, // Duplicate 8th stack item
    DUP9  = 0x88, // Duplicate 9th stack item
    DUP10 = 0x89, // Duplicate 10th stack item
    DUP11 = 0x8a, // Duplicate 11th stack item
    DUP12 = 0x8b, // Duplicate 12th stack item
    DUP13 = 0x8c, // Duplicate 13th stack item
    DUP14 = 0x8d, // Duplicate 14th stack item
    DUP15 = 0x8e, // Duplicate 15th stack item
    DUP16 = 0x8f, // Duplicate 16th stack item

    // 90s: Exchange Operation
    SWAP1  = 0x90, // Exchange 1st and 2nd stack item
    SWAP2  = 0x91, // Exchange 1st and 3rd stack item
    SWAP3  = 0x92, // Exchange 1st and 4th stack item
    SWAP4  = 0x93, // Exchange 1st and 5th stack item
    SWAP5  = 0x94, // Exchange 1st and 6th stack item
    SWAP6  = 0x95, // Exchange 1st and 7th stack item
    SWAP7  = 0x96, // Exchange 1st and 8th stack item
    SWAP8  = 0x97, // Exchange 1st and 9th stack item
    SWAP9  = 0x98, // Exchange 1st and 10th stack item
    SWAP10 = 0x99, // Exchange 1st and 11th stack item
    SWAP11 = 0x9a, // Exchange 1st and 12th stack item
    SWAP12 = 0x9b, // Exchange 1st and 13th stack item
    SWAP13 = 0x9c, // Exchange 1st and 14th stack item
    SWAP14 = 0x9d, // Exchange 1st and 15th stack item
    SWAP15 = 0x9e, // Exchange 1st and 16th stack item
    SWAP16 = 0x9f, // Exchange 1st and 17th stack item

    // a0s: Logging Operations
    LOG0 = 0xa0, // Append log record with no topics
    LOG1 = 0xa1, // Append log record with 1 topic
    LOG2 = 0xa2, // Append log record with 2 topics
    LOG3 = 0xa3, // Append log record with 3 topics
    LOG4 = 0xa4, // Append log record with 4 topics

    // f0s: System operations
    CREATE       = 0xf0, // Create a new account with associated code
    CALL         = 0xf1, // Message-call into an account
    CALLCODE     = 0xf2, // Message-call into this account with an alternative account’s code
    RETURN       = 0xf3, // Halt execution returning output data
    DELEGATECALL = 0xf4, // Message-call into this account with an alternative account’s code, but persisting the current values for sender and value.
    CREATE2      = 0xf5,
    STATICCALL   = 0xfa, // Static message-call into an account. Exactly equivalent to CALL except: The argument µs is replaced with 0.
    REVERT       = 0xfd, // Halt execution reverting state changes but returning data and remaining gas
    INVALID      = 0xfe, // Designated invalid instruction
    SELFDESTRUCT = 0xff
  };

  namespace OpFees {
    static uint16_t by_code[256] = {
      // 0s: Stop and Arithmetic Operations
      /* STOP       */  0,  // Halts execution
      /* ADD        */  3,  // Addition operation
      /* MUL        */  5,  // Multiplication operation
      /* SUB        */  3,  // Substraction operation
      /* DIV        */  5,  // Integer division operation
      /* SDIV       */  5,  // Signed integer division operation (truncated)
      /* MOD        */  5,  // Modulo remainder operation
      /* SMOD       */  5,  // Signed modulo remainder operation
      /* ADDMOD     */  8,  // Modulo addition operation
      /* MULMOD     */  8,  // Modulo multiplication operation
      /* EXP        */  10, // Exponential operation
      /* SIGNEXTEND */  5,  // Extend length of two’s complement signed integer.
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,

      // 10s: Comparison & Bitwise Logic Operations
      /* LT     */  3, // Less-than comparison
      /* GT     */  3, // Greater-than comparison
      /* SLT    */  3, // Signed less-than comparison
      /* SGT    */  3, // Signed greater-than comparison
      /* EQ     */  3, // Equality comparison
      /* ISZERO */  3, // Simple not operator
      /* AND    */  3, // Bitwise AND operation
      /* OR     */  3, // Bitwise OR operation
      /* XOR    */  3, // Bitwise XOR operation
      /* NOT    */  3, // Bitwise NOT operation
      /* BYTE   */  3, // Retrieve single byte from word
      /* SHL    */  3, // Shift left
      /* SHR    */  3, // Shift right
      /* SAR    */  3, // int256 shift right
      /* NULL   */  0,
      /* NULL   */  0,

      // 20s: SHA3
      /* SHA3       */  30, // Compute Keccak-256 hash.
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,

      // 30s: Environmental Information
      /* ADDRESS        */  2,   // Get address of currently executing account
      /* BALANCE        */  700, // Get balance of the given account.
      /* ORIGIN         */  2,   // Get execution origination address (This is the sender of original transaction; it is never an account with non-emptyassociated code.)
      /* CALLER         */  2,   // Get caller address (This is the address of the account that is directly responsible for this execution.)
      /* CALLVALUE      */  2,   // Get deposited value by the instruction/transaction responsible for this execution.
      /* CALLDATALOAD   */  3,   // Get input data of current environment. (This pertains to the input data passed with the message call instruction or transaction.)
      /* CALLDATASIZE   */  2,   // Get size of input data in current environment (This pertains to the input data passed with the message call instruction or transaction.)
      /* CALLDATACOPY   */  3,   // Copy input data in current environment to memory. (This pertains to the input data passed with the message call instruction or transaction.)
      /* CODESIZE       */  2,   // Get size of code running in current environment.
      /* CODECOPY       */  3,   // Copy code running in current environment to memory
      /* GASPRICE       */  2,   //  Get price of gas in current environment. (This is gas price specified by the originating transaction.)
      /* EXTCODESIZE    */  700, // Get size of an account’s code.
      /* EXTCODECOPY    */  700, // Copy an account’s code to memory
      /* RETURNDATASIZE */  2,   // Get size of output data from the previous call from the current environment.
      /* RETURNDATACOPY */  3,   // Copy output data from the previous call to memory.
      /* EXTCODEHASH    */  700,

      // 40s: Block Information
      /* BLOCKHASH  */  20, // Get the hash of one of the 256 most recent complete blocks.
      /* COINBASE   */  2,  // Get the block’s beneficiary address
      /* TIMESTAMP  */  2,  // Get the block’s timestamp.
      /* NUMBER     */  2,  // Get the block’s number
      /* DIFFICULTY */  2,  // Get the block’s difficulty.
      /* GASLIMIT   */  2,  // Get the block’s gas limit
      /* CHAINID    */  2,  // Get the block’s gas limit
      /* SELFBALANCE*/  5,
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,
      /* NULL       */  0,

      // 50s: Stack, Memory, Storage and Flow Operations
      /* POP      */  2,   // Remove item from stack.
      /* MLOAD    */  3,   // Load word from memory.
      /* MSTORE   */  3,   // Save word to memory
      /* MSTORE8  */  3,   // Save byte to memory
      /* SLOAD    */  800, // Load word from storage.
      /* SSTORE   */  0,   // Save word to storage
      /* JUMP     */  8,   // Alter the program counter.
      /* JUMPI    */  10,  // Conditionally alter the program counter.
      /* PC       */  2,   // Get the value of the program counter prior to the increment corresponding to this instruction.
      /* MSIZE    */  2,   // Get the size of active memory in bytes.
      /* GAS      */  2,   // Get the amount of available gas, including the corresponding reduction for the cost of this instruction.
      /* JUMPDEST */  1,   // Mark a valid destination for jumps. This operation has no effect on machine state during execution
      /* NULL     */  0,
      /* NULL     */  0,
      /* NULL     */  0,
      /* NULL     */  0,

      // 60s & 70s: Push Operations
      /* PUSH1  */  3, // Place 1 byte item on stack.
      /* PUSH2  */  3, // Place 2 byte item on stack.
      /* PUSH3  */  3, // Place 3 byte item on stack.
      /* PUSH4  */  3, // Place 4 byte item on stack.
      /* PUSH5  */  3, // Place 5 byte item on stack.
      /* PUSH6  */  3, // Place 6 byte item on stack.
      /* PUSH7  */  3, // Place 7 byte item on stack.
      /* PUSH8  */  3, // Place 8 byte item on stack.
      /* PUSH9  */  3, // Place 9 byte item on stack.
      /* PUSH10 */  3, // Place 10 byte item on stack.
      /* PUSH11 */  3, // Place 11 byte item on stack.
      /* PUSH12 */  3, // Place 12 byte item on stack.
      /* PUSH13 */  3, // Place 13 byte item on stack.
      /* PUSH14 */  3, // Place 14 byte item on stack.
      /* PUSH15 */  3, // Place 15 byte item on stack.
      /* PUSH16 */  3, // Place 16 byte item on stack.
      /* PUSH17 */  3, // Place 17 byte item on stack.
      /* PUSH18 */  3, // Place 18 byte item on stack.
      /* PUSH19 */  3, // Place 19 byte item on stack.
      /* PUSH20 */  3, // Place 20 byte item on stack.
      /* PUSH21 */  3, // Place 21 byte item on stack.
      /* PUSH22 */  3, // Place 22 byte item on stack.
      /* PUSH23 */  3, // Place 23 byte item on stack.
      /* PUSH24 */  3, // Place 24 byte item on stack.
      /* PUSH25 */  3, // Place 25 byte item on stack.
      /* PUSH26 */  3, // Place 26 byte item on stack.
      /* PUSH27 */  3, // Place 27 byte item on stack.
      /* PUSH28 */  3, // Place 28 byte item on stack.
      /* PUSH29 */  3, // Place 29 byte item on stack.
      /* PUSH30 */  3, // Place 30 byte item on stack.
      /* PUSH31 */  3, // Place 31 byte item on stack.
      /* PUSH32 */  3, // Place 32 byte item on stack.

      // 80s: Duplication Operation
      /* DUP1  */  3, // Duplicate 1st stack item
      /* DUP2  */  3, // Duplicate 2nd stack item
      /* DUP3  */  3, // Duplicate 3rd stack item
      /* DUP4  */  3, // Duplicate 4th stack item
      /* DUP5  */  3, // Duplicate 5th stack item
      /* DUP6  */  3, // Duplicate 6th stack item
      /* DUP7  */  3, // Duplicate 7th stack item
      /* DUP8  */  3, // Duplicate 8th stack item
      /* DUP9  */  3, // Duplicate 9th stack item
      /* DUP10 */  3, // Duplicate 10th stack item
      /* DUP11 */  3, // Duplicate 11th stack item
      /* DUP12 */  3, // Duplicate 12th stack item
      /* DUP13 */  3, // Duplicate 13th stack item
      /* DUP14 */  3, // Duplicate 14th stack item
      /* DUP15 */  3, // Duplicate 15th stack item
      /* DUP16 */  3, // Duplicate 16th stack item

      // 90s: Exchange Operation
      /* SWAP1  */  3, // Exchange 1st and 2nd stack item
      /* SWAP2  */  3, // Exchange 1st and 3rd stack item
      /* SWAP3  */  3, // Exchange 1st and 4th stack item
      /* SWAP4  */  3, // Exchange 1st and 5th stack item
      /* SWAP5  */  3, // Exchange 1st and 6th stack item
      /* SWAP6  */  3, // Exchange 1st and 7th stack item
      /* SWAP7  */  3, // Exchange 1st and 8th stack item
      /* SWAP8  */  3, // Exchange 1st and 9th stack item
      /* SWAP9  */  3, // Exchange 1st and 10th stack item
      /* SWAP10 */  3, // Exchange 1st and 11th stack item
      /* SWAP11 */  3, // Exchange 1st and 12th stack item
      /* SWAP12 */  3, // Exchange 1st and 13th stack item
      /* SWAP13 */  3, // Exchange 1st and 14th stack item
      /* SWAP14 */  3, // Exchange 1st and 15th stack item
      /* SWAP15 */  3, // Exchange 1st and 16th stack item
      /* SWAP16 */  3, // Exchange 1st and 17th stack item

      // a0s: Logging Operations
      /* LOG0 */  (375), // Append log record with no topics
      /* LOG1 */  (375), // Append log record with 1 topic
      /* LOG2 */  (375), // Append log record with 2 topics
      /* LOG3 */  (375), // Append log record with 3 topics
      /* LOG4 */  (375), // Append log record with 4 topics
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,

      // b0s
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,

      // c0s
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,

      // d0s
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,

      // e0s
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,
      /* NULL */  0,

      // f0s: System operations
      /* CREATE       */  32000, // Create a new account with associated code
      /* CALL         */  700,   // Message-call into an account
      /* CALLCODE     */  700,   // Message-call into this account with an alternative account’s code
      /* RETURN       */  0,     // Halt execution returning output data
      /* DELEGATECALL */  700,   // Message-call into this account with an alternative account’s code, but persisting the current values for sender and value.
      /* CREATE2      */  32000,
      /* NULL         */  0,
      /* NULL         */  0,
      /* NULL         */  0,
      /* NULL         */  0,
      /* STATICCALL   */  700,   // Static message-call into an account. Exactly equivalent to CALL except: The argument µs is replaced with 0.
      /* NULL         */  0,
      /* NULL         */  0,
      /* REVERT       */  0,     // Halt execution reverting state changes but returning data and remaining gas
      /* INVALID      */  0,     // Designated invalid instruction
      /* SELFDESTRUCT */  5000
    };
  }

  #if (OPTRACE == true)
  static std::string opcodeToString (uint8_t op) {
    std::map<uint8_t, std::string> opToStrings = {
      // 0s: Stop and Arithmetic Operations
      { 0x00, "STOP"   }, // Halts execution
      { 0x01, "ADD"    }, // Addition operation
      { 0x02, "MUL"    }, // Multiplication operation
      { 0x03, "SUB"    }, // Substraction operation
      { 0x04, "DIV"    }, // Integer division operation
      { 0x05, "SDIV"   }, // Signed integer division operation (truncated)
      { 0x06, "MOD"    }, // Modulo remainder operation
      { 0x07, "SMOD"   }, // Signed modulo remainder operation
      { 0x08, "ADDMOD" }, // Modulo addition operation
      { 0x09, "MULMOD" }, // Modulo multiplication operation
      { 0x0a, "EXP"    }, // Exponential operation
      { 0x0b, "SIGNEXTEND" }, // Extend length of two’s complement signed integer.

      // 10s: Comparison & Bitwise Logic Operations
      { 0x10, "LT"    }, // Less-than comparison
      { 0x11, "GT"    }, // Greater-than comparison
      { 0x12, "SLT"   }, // Signed less-than comparison
      { 0x13, "SGT"   }, // Signed greater-than comparison
      { 0x14, "EQ"    }, // Equality comparison
      { 0x15, "ISZERO" }, // Simple not operator
      { 0x16, "AND" }, // Bitwise AND operation
      { 0x17, "OR" }, // Bitwise OR operation
      { 0x18, "XOR" }, // Bitwise XOR operation
      { 0x19, "NOT" }, // Bitwise NOT operation
      { 0x1a, "BYTE" }, // Retrieve single byte from word
      { 0x1b, "SHL" }, // Shift left
      { 0x1c, "SHR" }, // Shift right
      { 0x1d, "SAR" }, // int256 shift right

      // 20s: SHA3
      { 0x20, "SHA3" }, // Compute Keccak-256 hash.

      // 30s: Environmental Information
      { 0x30, "ADDRESS" }, // Get address of currently executing account
      { 0x31, "BALANCE" }, // Get balance of the given account.
      { 0x32, "ORIGIN" }, //  Get execution origination address (This is the sender of original transaction; it is never an account with non-emptyassociated code.)
      { 0x33, "CALLER" }, // Get caller address (This is the address of the account that is directly responsible for this execution.)
      { 0x34, "CALLVALUE" }, // Get deposited value by the instruction/transaction responsible for this execution.
      { 0x35, "CALLDATALOAD" }, // Get input data of current environment. (This pertains to the input data passed with the message call instruction or transaction.)
      { 0x36, "CALLDATASIZE" }, // Get size of input data in current environment (This pertains to the input data passed with the message call instruction or transaction.)
      { 0x37, "CALLDATACOPY" }, // Copy input data in current environment to memory. (This pertains to the input data passed with the message call instruction or transaction.)
      { 0x38, "CODESIZE" }, // Get size of code running in current environment.
      { 0x39, "CODECOPY" }, // Copy code running in current environment to memory
      { 0x3a, "GASPRICE" }, // Get price of gas in current environment. (This is gas price specified by the originating transaction.)
      { 0x3b, "EXTCODESIZE" }, // Get size of an account’s code.
      { 0x3c, "EXTCODECOPY" }, // Copy an account’s code to memory
      { 0x3d, "RETURNDATASIZE" }, // Get size of output data from the previous call from the current environment.
      { 0x3e, "RETURNDATACOPY" }, // Copy output data from the previous call to memory.
      { 0x3f, "EXTCODEHASH"}, // External code hash

      // 40s: Block Information
      { 0x40, "BLOCKHASH" }, // Get the hash of one of the 256 most recent complete blocks.
      { 0x41, "COINBASE" }, // Get the block’s beneficiary address
      { 0x42, "TIMESTAMP" }, // Get the block’s timestamp.
      { 0x43, "NUMBER" }, // Get the block’s number
      { 0x44, "DIFFICULTY" }, // Get the block’s difficulty.
      { 0x45, "GASLIMIT" }, // Get the block’s gas limit
      { 0x46, "CHAINID" }, // Get the chain ID
      { 0x47, "SELFBALANCE" }, // Get the balance of the contract

      // 50s: Stack, Memory, Storage and Flow Operations
      { 0x50, "POP" }, // Remove item from stack.
      { 0x51, "MLOAD" }, // Load word from memory.
      { 0x52, "MSTORE" }, // Save word to memory
      { 0x53, "MSTORE8" }, // Save byte to memory
      { 0x54, "SLOAD" }, // Load word from storage.
      { 0x55, "SSTORE" }, // Save word to storage
      { 0x56, "JUMP" }, // Alter the program counter.
      { 0x57, "JUMPI" }, // Conditionally alter the program counter.
      { 0x58, "PC" }, // Get the value of the program counter prior to the increment corresponding to this instruction.
      { 0x59, "MSIZE" }, // Get the size of active memory in bytes.
      { 0x5a, "GAS" }, // Get the amount of available gas, including the corresponding reduction for the cost of this instruction.
      { 0x5b, "JUMPDEST" }, // Mark a valid destination for jumps. This operation has no effect on machine state during execution

      // 60s & 70s: Push Operations
      { 0x60, "PUSH1" }, // Place 1 byte item on stack.
      { 0x61, "PUSH2" }, // Place 2 byte item on stack.
      { 0x62, "PUSH3" }, // Place 3 byte item on stack.
      { 0x63, "PUSH4" }, // Place 4 byte item on stack.
      { 0x64, "PUSH5" }, // Place 5 byte item on stack.
      { 0x65, "PUSH6" }, // Place 6 byte item on stack.
      { 0x66, "PUSH7" }, // Place 7 byte item on stack.
      { 0x67, "PUSH8" }, // Place 8 byte item on stack.
      { 0x68, "PUSH9" }, // Place 9 byte item on stack.
      { 0x69, "PUSH10" }, // Place 10 byte item on stack.
      { 0x6a, "PUSH11" }, // Place 11 byte item on stack.
      { 0x6b, "PUSH12" }, // Place 12 byte item on stack.
      { 0x6c, "PUSH13" }, // Place 13 byte item on stack.
      { 0x6d, "PUSH14" }, // Place 14 byte item on stack.
      { 0x6e, "PUSH15" }, // Place 15 byte item on stack.
      { 0x6f, "PUSH16" }, // Place 16 byte item on stack.
      { 0x70, "PUSH17" }, // Place 17 byte item on stack.
      { 0x71, "PUSH18" }, // Place 18 byte item on stack.
      { 0x72, "PUSH19" }, // Place 19 byte item on stack.
      { 0x73, "PUSH20" }, // Place 20 byte item on stack.
      { 0x74, "PUSH21" }, // Place 21 byte item on stack.
      { 0x75, "PUSH22" }, // Place 22 byte item on stack.
      { 0x76, "PUSH23" }, // Place 23 byte item on stack.
      { 0x77, "PUSH24" }, // Place 24 byte item on stack.
      { 0x78, "PUSH25" }, // Place 25 byte item on stack.
      { 0x79, "PUSH26" }, // Place 26 byte item on stack.
      { 0x7a, "PUSH27" }, // Place 27 byte item on stack.
      { 0x7b, "PUSH28" }, // Place 28 byte item on stack.
      { 0x7c, "PUSH29" }, // Place 29 byte item on stack.
      { 0x7d, "PUSH30" }, // Place 30 byte item on stack.
      { 0x7e, "PUSH31" }, // Place 31 byte item on stack.
      { 0x7f, "PUSH32" }, // Place 32 byte item on stack.

      // 80s: Duplication Operation
      { 0x80, "DUP1" }, // Duplicate 1st stack item
      { 0x81, "DUP2" }, // Duplicate 2nd stack item
      { 0x82, "DUP3" }, // Duplicate 3rd stack item
      { 0x83, "DUP4" }, // Duplicate 4th stack item
      { 0x84, "DUP5" }, // Duplicate 5th stack item
      { 0x85, "DUP6" }, // Duplicate 6th stack item
      { 0x86, "DUP7" }, // Duplicate 7th stack item
      { 0x87, "DUP8" }, // Duplicate 8th stack item
      { 0x88, "DUP9" }, // Duplicate 9th stack item
      { 0x89, "DUP10" }, // Duplicate 10th stack item
      { 0x8a, "DUP11" }, // Duplicate 11th stack item
      { 0x8b, "DUP12" }, // Duplicate 12th stack item
      { 0x8c, "DUP13" }, // Duplicate 13th stack item
      { 0x8d, "DUP14" }, // Duplicate 14th stack item
      { 0x8e, "DUP15" }, // Duplicate 15th stack item
      { 0x8f, "DUP16" }, // Duplicate 16th stack item

      // 90s: Exchange Operation
      { 0x90, "SWAP1" }, // Exchange 1st and 2nd stack item
      { 0x91, "SWAP2" }, // Exchange 1st and 3rd stack item
      { 0x92, "SWAP3" }, // Exchange 1st and 4th stack item
      { 0x93, "SWAP4" }, // Exchange 1st and 5th stack item
      { 0x94, "SWAP5" }, // Exchange 1st and 6th stack item
      { 0x95, "SWAP6" }, // Exchange 1st and 7th stack item
      { 0x96, "SWAP7" }, // Exchange 1st and 8th stack item
      { 0x97, "SWAP8" }, // Exchange 1st and 9th stack item
      { 0x98, "SWAP9" }, // Exchange 1st and 10th stack item
      { 0x99, "SWAP10" }, // Exchange 1st and 11th stack item
      { 0x9a, "SWAP11" }, // Exchange 1st and 12th stack item
      { 0x9b, "SWAP12" }, // Exchange 1st and 13th stack item
      { 0x9c, "SWAP13" }, // Exchange 1st and 14th stack item
      { 0x9d, "SWAP14" }, // Exchange 1st and 15th stack item
      { 0x9e, "SWAP15" }, // Exchange 1st and 16th stack item
      { 0x9f, "SWAP16" }, // Exchange 1st and 17th stack item

      // a0s: Logging Operations
      { 0xa0, "LOG0" }, // Append log record with no topics
      { 0xa1, "LOG1" }, // Append log record with 1 topic
      { 0xa2, "LOG2" }, // Append log record with 2 topics
      { 0xa3, "LOG3" }, // Append log record with 3 topics
      { 0xa4, "LOG4" }, // Append log record with 4 topics

      // f0s: System operations
      { 0xf0, "CREATE" }, // Create a new account with associated code
      { 0xf1, "CALL" }, // Message-call into an account
      { 0xf2, "CALLCODE" }, // Message-call into this account with an alternative account’s code
      { 0xf3, "RETURN" }, // Halt execution returning output data
      { 0xf4, "DELEGATECALL" }, // Message-call into this account with an alternative account’s code, but persisting the current values for sender and value.
      { 0xf5, "CREATE2" },
      { 0xfa, "STATICCALL" }, // Static message-call into an account. Exactly equivalent to CALL except: The argument µs is replaced with 0.
      { 0xfd, "REVERT" }, // Halt execution reverting state changes but returning data and remaining gas
      { 0xfe, "INVALID" }, // Designated invalid instruction
      { 0xff, "SELFDESTRUCT" }
    };

    return opToStrings[op];
  }
  #endif /** OPTRACE **/
} // namespace eosio_evm
