// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.
// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License..
//  - Modified error handling and sources modified

#include <eosio.evm/eosio.evm.hpp>

namespace eosio_evm
{
  // Return true if error
  bool Processor::access_mem(const uint256_t& offset, const uint256_t& size)
  {
    if (size == 0) {
      return false;
    }

    if (size > MAX_BUFFER_SIZE) {
      return throw_error(Exception(ET::overflow, "overflow in buffer"), {});
    }

    return prepare_mem_access(offset, static_cast<uint64_t>(size));
  }

    // Return true if error
  bool Processor::prepare_mem_access(const uint256_t& offset, const uint64_t& size)
  {
    if (offset > MAX_BUFFER_SIZE) {
      return throw_error(Exception(ET::overflow, "overflow in buffer"), {});
    }

    const auto new_size = static_cast<uint64_t>(offset) + size;
    const auto current_size = ctx->mem.size();

    if (new_size > current_size)
    {
      const auto new_words = num_words(new_size);
      const auto current_words = static_cast<int64_t>(current_size / 32);
      const auto new_cost = 3 * new_words + new_words * new_words / 512;
      const auto current_cost = 3 * current_words + current_words * current_words / 512;
      const auto cost = new_cost - current_cost;

      // Gas
      bool error = use_gas(cost);
      if (error) return true;

      // Resize
      const auto end = static_cast<size_t>(new_words * WORD_SIZE);
      if (end >= MAX_MEM_SIZE) {
        return throw_error(Exception(ET::OOB, "Memory limit exceeded"), {});
      }

      ctx->mem.resize(end);
    }

    // Success
    return false;
  }

  void Processor::calldatacopy()
  {
    const auto mem_index   = ctx->s.pop();
    const auto input_index = ctx->s.pop();
    const auto size        = ctx->s.pop();
    if (ctx->s.stack_error) return throw_stack();

    // Memory access + gas cost
    bool memory_error = access_mem(mem_index, size);
    if (memory_error) return;

    // Determine destination index
    auto destination_index = static_cast<size_t>(mem_index);

    // Determine source index
    auto input_size = ctx->input.size();
    auto bounded_size = static_cast<size_t>(size);
    auto source_index = input_size < input_index ? input_size : static_cast<size_t>(input_index);
    auto copy_size = std::min(bounded_size, input_size - source_index);

    // Copy gas calculation (copy cost is 3)
    bool error = use_gas(num_words(bounded_size) * GP_COPY);
    if (error) return;

    // Set memory
    if (copy_size > 0) {
      std::memcpy(&ctx->mem[destination_index], &ctx->input[source_index], copy_size);
    }

    if (bounded_size - copy_size > 0) {
      std::memset(&ctx->mem[destination_index + copy_size], 0, bounded_size - copy_size);
    }
  }

  void Processor::extcodecopy()
  {
    const auto address     = ctx->s.pop_addr();
    const auto mem_index   = ctx->s.pop();
    const auto input_index = ctx->s.pop();
    const auto size        = ctx->s.pop();
    if (ctx->s.stack_error) return throw_stack();

    // Memory access + gas cost
    bool memory_error = access_mem(mem_index, size);
    if (memory_error) return;

    // Determine destination index
    auto destination_index = static_cast<size_t>(mem_index);

    // Determine source index
    auto bounded_size = static_cast<size_t>(size);
    auto source_index = MAX_BUFFER_SIZE < input_index ? MAX_BUFFER_SIZE : static_cast<size_t>(input_index);

    // Copy gas calculation (copy cost is 3)
    bool error = use_gas(num_words(bounded_size) * GP_COPY);
    if (error) return;

    // Write code to memory
    auto code = get_account(address).get_code();
    auto code_size = code.size();

    uint64_t bytes_copied = 0;
    const auto bytes_to_copy = std::min(bounded_size, code_size - source_index);
    if (source_index < code_size && bytes_to_copy > 0) {
      bytes_copied = bytes_to_copy;
      std::memcpy(&ctx->mem[destination_index], &code[source_index], bytes_to_copy);
    }

    // Pad zeros
    if (bounded_size - bytes_copied > 0) {
      std::memset(&ctx->mem[destination_index + bytes_copied], 0, bounded_size - bytes_copied);
    }
  }

  void Processor::returndatacopy()
  {
    const auto mem_index   = ctx->s.pop();
    const auto input_index = ctx->s.pop();
    const auto size        = ctx->s.pop();
    if (ctx->s.stack_error) return throw_stack();

    // Memory access + gas cost
    bool memory_error = access_mem(mem_index, size);
    if (memory_error) return;

    // Determine destination index
    auto destination_index = static_cast<size_t>(mem_index);

    // Validate return data
    auto return_data_size = ctx->last_return_data.size();
    if (return_data_size < input_index) {
      throw_error(Exception(ET::OOB, "Invalid memory access"), {});
      return;
    }

    // Determine source index
    auto bounded_size = static_cast<size_t>(size);
    auto source_index = static_cast<size_t>(input_index);

    // Validate source index
    if (source_index + bounded_size > return_data_size) {
      throw_error(Exception(ET::OOB, "Invalid memory access"), {});
      return;
    }

    // Copy gas calculation (copy cost is 3)
    bool error = use_gas(num_words(bounded_size) * GP_COPY);
    if (error) return;

    // Write to memory
    if (bounded_size > 0) {
      std::memcpy(&ctx->mem[destination_index], &ctx->last_return_data[source_index], bounded_size);
    }
  }
} // namespace eosio_evm
