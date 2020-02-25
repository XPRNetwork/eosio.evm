#include <eosio.evm/eosio.evm.hpp>

namespace eosio_evm {
  uint256_t Stack::pop()
  {
    if (st.empty()) {
      ctx->error_cb(Exception(Exception::Type::outOfBounds, "Stack out of range"), {}, {});
    }

    uint256_t val = st.front();
    st.pop_front();
    return val;
  }

  uint256_t Stack::pop_addr()
  {
    static const uint256_t MASK_160 = (uint256_t(1) << 160) - 1;
    return pop() & MASK_160;
  }

  uint64_t Stack::popu64()
  {
    const auto val = pop();
    if (val > std::numeric_limits<uint64_t>::max()) {
      ctx->error_cb(Exception(Exception::Type::outOfBounds, "Value on stack is larger than 2^64"), {}, {});
    }

    return static_cast<uint64_t>(val);
  }

  int64_t Stack::popAmount()
  {
    const auto val = pop();
    if (val > eosio::asset::max_amount) {
      ctx->error_cb(Exception(Exception::Type::outOfBounds, "Value on stack is larger than 2^62 - 1"), {}, {});
    }

    return static_cast<int64_t>(val);
  }

  void Stack::push(const uint256_t& val)
  {
    if (size() == MAX_STACK_SIZE) {
      ctx->error_cb(Exception(Exception::Type::outOfBounds, "Stack memory exceeded"), {}, {});
    }

    st.push_front(val);
  }

  uint64_t Stack::size() const
  {
    return st.size();
  }

  void Stack::swap(uint64_t i)
  {
    if (i >= size()) {
      ctx->error_cb(Exception(Exception::Type::outOfBounds, "Swap out of range"), {}, {});
    }

    std::swap(st[0], st[i]);
  }

  void Stack::dup(uint64_t a)
  {
    if (a >= size()) {
      ctx->error_cb(Exception(Exception::Type::outOfBounds, "Dup out of range"), {}, {});
    }

    st.push_front(st[a]);
  }

  void Stack::print() {
    eosio::print("\n");

    for (const auto& elem : st) {
      eosio::print(intx::hex(elem), "\n");
    }
  }

  std::string Stack::asArray() {
    std::string base = "[";
    for (int i = st.size() - 1; i >= 0; i--) {
      base += "\"0x" + intx::hex(st[i]) + "\",";
    }
    if (!st.empty()) {
      base.pop_back();
    }
    base += "]";
    return base;
  }
} // Namespace eosio_evm