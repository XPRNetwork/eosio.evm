#pragma once

#include "model.hpp"
#include "eosio.evm.hpp"
#include "stack.hpp"

namespace evm4eos {
  // Forward Declaration
  class evm;

  enum class ExitReason : uint8_t
  {
    returned = 0,
    halted,
    threw
  };

  struct ExecResult
  {
    ExitReason er = {};
    Exception::Type ex = {};
    std::string exmsg = {};
    std::vector<uint8_t> output = {};

    void print () {
      eosio::print("\n-------------Exec Result------------\n");
      eosio::print("\nExitReason: ", (uint8_t) er);
      eosio::print("\nException Type:", (uint8_t) ex);
      eosio::print("\nException message:" + exmsg);
      eosio::print("\nOutput:", bin2hex(output));
      eosio::print("\n-------------End Exec Result------------\n");
    }
  };

  class Processor
  {
  public:
    /**
     * @brief The main entry point for the EVM.
     *
     * Runs the callee's code in the caller's context. VM exceptions (ie,
     * Exception) will be caught and returned in the result.
     *
     * @param tx the transaction
     * @param caller the caller's address
     * @param callee the callee's account state
     * @param input the raw byte input
     * @param call_value the call value
     * the execution will be collected.
     * @return ExecResult the execution result
     */
    ExecResult run(
      EthereumTransaction& tx,
      const Address& caller,
      const Account& callee,
      evm* contract = nullptr
    );
  };

  /**
   * bytecode program
   */
  class Program
  {
  public:
    const std::vector<uint8_t> code;
    const std::set<uint64_t> jump_dests;

    Program(std::vector<uint8_t>&& c)
      : code(c),
        jump_dests(compute_jump_dests(code)) {}

  private:
    std::set<uint64_t> compute_jump_dests(const std::vector<uint8_t>& code)
    {
      std::set<uint64_t> dests;

      for (uint64_t i = 0; i < code.size(); i++) {
        const auto op = code[i];
        if (op == JUMPDEST) {
          dests.insert(i);
        } else if (op >= PUSH1 && op <= PUSH32) {
          i += op - PUSH1 + 1;
        }
      }
      return dests;
    }
  };
}