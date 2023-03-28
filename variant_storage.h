#pragma once

#include "variant_non_member.h"

namespace details {

template <bool trivial, typename... Types>
union recursive_union;

template <typename Head, typename... Tail>
union recursive_union<true, Head, Tail...> {
  constexpr recursive_union() noexcept(std::is_nothrow_default_constructible_v<Head>)
      requires(std::is_default_constructible_v<Head>)
      : head() {}

  template <std::size_t I, typename... Args>
  constexpr recursive_union(in_place_index_t<I>, Args&&... args)
      : tail(in_place_index<I - 1>, std::forward<Args>(args)...) {}
  template <typename... Args>
  constexpr recursive_union(in_place_index_t<0>, Args&&... args) : head(std::forward<Args>(args)...) {}

  template <std::size_t I>
  constexpr auto& get() {
    if constexpr (I == 0) {
      return head;
    } else {
      return tail.template get<I - 1>();
    }
  }
  template <std::size_t I>
  constexpr const auto& get() const {
    if constexpr (I == 0) {
      return head;
    } else {
      return tail.template get<I - 1>();
    }
  }

  template <std::size_t I, typename... Args>
  constexpr void emplace(Args&&... args) {
    if constexpr (I == 0) {
      std::construct_at(std::addressof(head), std::forward<Args>(args)...);
    } else {
      tail.template emplace<I - 1>(std::forward<Args>(args)...);
    }
  }

  constexpr ~recursive_union() = default;

  Head head;
  recursive_union<true, Tail...> tail;
};

template <typename Head, typename... Tail>
union recursive_union<false, Head, Tail...> {
  constexpr recursive_union() noexcept(std::is_nothrow_default_constructible_v<Head>)
      requires(std::is_default_constructible_v<Head>)
      : head() {}

  template <std::size_t I, typename... Args>
  constexpr recursive_union(in_place_index_t<I>, Args&&... args)
      : tail(in_place_index<I - 1>, std::forward<Args>(args)...) {}
  template <typename... Args>
  constexpr recursive_union(in_place_index_t<0>, Args&&... args) : head(std::forward<Args>(args)...) {}

  template <std::size_t I>
  constexpr auto& get() {
    if constexpr (I == 0) {
      return head;
    } else {
      return tail.template get<I - 1>();
    }
  }
  template <std::size_t I>
  constexpr const auto& get() const {
    if constexpr (I == 0) {
      return head;
    } else {
      return tail.template get<I - 1>();
    }
  }

  template <std::size_t I, typename... Args>
  constexpr void emplace(Args&&... args) {
    if constexpr (I == 0) {
      std::construct_at(std::addressof(head), std::forward<Args>(args)...);
    } else {
      tail.template emplace<I - 1>(std::forward<Args>(args)...);
    }
  }

  template <std::size_t I>
  constexpr void reset() {
    if constexpr (I == 0) {
      head.~Head();
    } else {
      tail.template reset<I - 1>();
    }
  }

  constexpr ~recursive_union() {}

  Head head;
  recursive_union<false, Tail...> tail;
};

template <bool trivial>
union recursive_union<trivial> {
  constexpr recursive_union() = default;
  constexpr recursive_union(in_place_index_t<0>) {}

  constexpr ~recursive_union() = default;
};

template <bool trivial, typename... Types>
struct variant_storage {
  constexpr variant_storage() = default;

  template <std::size_t I, typename... Args>
  constexpr variant_storage(in_place_index_t<I> ind, Args&&... args)
      : data(ind, std::forward<Args>(args)...), m_index(I) {}

  template <std::size_t I, class... Args>
  constexpr auto& emplace(Args&&... args) {
    reset();
    data.template emplace<I>(std::forward<Args>(args)...);
    m_index = I;
    return data.template get<I>();
  }

  constexpr std::size_t index() const noexcept {
    return m_index;
  }

  constexpr void reset() {
    if (m_index < sizeof...(Types)) {
      visit_at([&]<std::size_t I>(in_place_index_t<I>) { data.template reset<I>(); }, *this);
    }
    m_index = variant_npos;
  }

  constexpr ~variant_storage() {
    reset();
  }

  std::size_t m_index = 0;
  recursive_union<trivial, Types...> data;
};

template <typename... Types>
struct variant_storage<true, Types...> {
  constexpr variant_storage() = default;

  template <std::size_t I, typename... Args>
  constexpr variant_storage(in_place_index_t<I>, Args&&... args)
      : m_index(I), data(in_place_index<I>, std::forward<Args>(args)...) {}

  template <std::size_t I, class... Args>
  constexpr auto& emplace(Args&&... args) {
    reset();
    data.template emplace<I>(std::forward<Args>(args)...);
    m_index = I;
    return data.template get<I>();
  }

  constexpr std::size_t index() const noexcept {
    return m_index;
  }

  constexpr void reset() {
    m_index = variant_npos;
  }

  constexpr ~variant_storage() = default;

  std::size_t m_index = 0;
  recursive_union<true, Types...> data;
};

} // namespace details
