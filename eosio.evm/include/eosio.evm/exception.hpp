// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "opcode.hpp"

namespace eosio_evm
{
  class Exception
  {
  public:
    enum class Type
    {
      OOB,
      outOfGas,
      outOfFunds,
      overflow,
      staticStateChange,
      illegalInstruction,
      notImplemented,
      revert
    };
    const Type type;

  private:
    const std::string msg;

  public:
    Exception(Type t, const std::string& m)
      : type(t),
        msg(m) {}

    const char* what() const noexcept { return msg.c_str(); }
  };

  using ET = Exception::Type;

  enum class ExitReason : uint8_t
  {
    threw = 0,
    returned
  };

  struct ExecResult
  {
    ExitReason er = {};
    Exception::Type ex = {};
    std::string exmsg = "EVM Execution Error";
    std::vector<uint8_t> output = {};
    uint256_t gas_used = 0;

    ExecResult() = default;
    ExecResult(const std::string error): exmsg(error) {}

    #if (TESTING == true)
    void print() {
      eosio::print("\n-------------Exec Result------------\n");
      eosio::print("\nExitReason: ", (uint8_t) er);
      eosio::print("\nException Type:", (uint8_t) ex);
      eosio::print("\nException message:" + exmsg);
      eosio::print("\nOutput:", bin2hex(output));
      eosio::print("\n-------------End Exec Result------------\n");
    }
    #endif /* TESTING */
  };
} // namespace eosio_evm
