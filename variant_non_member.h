#pragma once

#include "details.h"

template <typename... Types>
class variant;

class bad_variant_access : public std::exception {
public:
  bad_variant_access() noexcept = default;
  bad_variant_access(const char* message) : message(message) {}

  const char* what() const noexcept override {
    return message;
  }

private:
  const char* message = "bad variant access";
};

template <class T>
struct in_place_type_t {
  explicit in_place_type_t() = default;
};

template <class T>
inline constexpr in_place_type_t<T> in_place_type{};

template <std::size_t I>
struct in_place_index_t {
  explicit in_place_index_t() = default;
};

template <std::size_t I>
inline constexpr in_place_index_t<I> in_place_index{};

template <class T>
struct variant_size;

template <class... Types>
struct variant_size<variant<Types...>> : details::storage_size<variant<Types...>> {};
template <class T>
struct variant_size<const T> : variant_size<T> {};
template <class T>
struct variant_size<volatile T> : variant_size<T> {};
template <class T>
struct variant_size<const volatile T> : variant_size<T> {};

template <class T>
inline constexpr std::size_t variant_size_v = variant_size<T>::value;

template <std::size_t I, typename T>
struct variant_alternative;

template <std::size_t I, typename... Types>
struct variant_alternative<I, variant<Types...>> {
  using type = details::get_type_by_index_t<I, Types...>;
};

template <std::size_t I, typename... Types>
struct variant_alternative<I, const variant<Types...>> {
  using type = const details::get_type_by_index_t<I, Types...>;
};

template <std::size_t I, typename... Types>
struct variant_alternative<I, volatile variant<Types...>> {
  using type = volatile details::get_type_by_index_t<I, Types...>;
};

template <std::size_t I, typename... Types>
struct variant_alternative<I, const volatile variant<Types...>> {
  using type = const volatile details::get_type_by_index_t<I, Types...>;
};

template <size_t I, class T>
using variant_alternative_t = typename variant_alternative<I, T>::type;

template <std::size_t I, class... Types>
constexpr variant_alternative_t<I, variant<Types...>>& get(variant<Types...>& v) {
  if (I != v.index()) {
    throw bad_variant_access();
  }
  return v.m_storage.data.template get<I>();
}

template <std::size_t I, class... Types>
constexpr variant_alternative_t<I, variant<Types...>>&& get(variant<Types...>&& v) {
  return std::move(get<I>(v));
}

template <std::size_t I, class... Types>
constexpr const variant_alternative_t<I, variant<Types...>>& get(const variant<Types...>& v) {
  if (I != v.index()) {
    throw bad_variant_access();
  }
  return v.m_storage.data.template get<I>();
}

template <std::size_t I, class... Types>
constexpr const variant_alternative_t<I, variant<Types...>>&& get(const variant<Types...>&& v) {
  return std::move(get<I>(v));
}

template <class T, class... Types>
constexpr T& get(variant<Types...>& v) {
  return get<details::get_index_by_type_v<T, Types...>>(v);
}

template <class T, class... Types>
constexpr T&& get(variant<Types...>&& v) {
  return get<details::get_index_by_type_v<T, Types...>>(v);
}

template <class T, class... Types>
constexpr const T& get(const variant<Types...>& v) {
  return get<details::get_index_by_type_v<T, Types...>>(v);
}

template <class T, class... Types>
constexpr const T&& get(const variant<Types...>&& v) {
  return get<details::get_index_by_type_v<T, Types...>>(v);
}

template <std::size_t I, class... Types>
constexpr std::add_pointer_t<variant_alternative_t<I, variant<Types...>>> get_if(variant<Types...>* pv) noexcept {
  if (pv == nullptr || I != pv->index()) {
    return nullptr;
  }
  return std::addressof(get<I>(*pv));
}

template <std::size_t I, class... Types>
constexpr std::add_pointer_t<const variant_alternative_t<I, variant<Types...>>>
get_if(const variant<Types...>* pv) noexcept {
  if (pv == nullptr || I != pv->index()) {
    return nullptr;
  }
  return std::addressof(get<I>(*pv));
}

template <class T, class... Types>
constexpr std::add_pointer_t<T> get_if(variant<Types...>* pv) noexcept {
  return get_if<details::get_index_by_type_v<T, Types...>>(pv);
}

template <class T, class... Types>
constexpr std::add_pointer_t<const T> get_if(const variant<Types...>* pv) noexcept {
  return get_if<details::get_index_by_type_v<T, Types...>>(pv);
}

template <class T, class... Types>
constexpr bool holds_alternative(const variant<Types...>& v) noexcept {
  return details::get_index_by_type_v<T, Types...> == v.index();
}

namespace details {

template <bool Ind, typename Visitor, typename... Variants, std::size_t... Is>
constexpr auto make_invoke_matrix(std::index_sequence<Is...>) {
  struct invoker {
    static constexpr decltype(auto) invoke_visitor(Visitor&& vis, Variants&&... vars) {
      if constexpr (Ind) {
        return std::forward<Visitor>(vis)(in_place_index<Is>...);
      } else {
        return std::forward<Visitor>(vis)(get<Is>(std::forward<Variants>(vars))...);
      }
    }
  };
  return &invoker::invoke_visitor;
}

template <bool Ind, typename Visitor, typename... Variants, std::size_t... Is, std::size_t... Js, typename... Rest>
constexpr auto make_invoke_matrix(std::index_sequence<Is...>, std::index_sequence<Js...>, Rest... rest) {
  return std::array{make_invoke_matrix<Ind, Visitor, Variants...>(std::index_sequence<Is..., Js>(), rest...)...};
}

template <bool Ind, typename Visitor, typename... Variants>
constexpr auto make_invoke_matrix() {
  return make_invoke_matrix<Ind, Visitor, Variants...>(
      std::index_sequence<>(), std::make_index_sequence<storage_size_v<std::remove_reference_t<Variants>>>()...);
}

template <typename Invoker>
constexpr Invoker& get_invoker(Invoker& invoker) {
  return invoker;
}

template <typename Matrix, typename... Is>
constexpr auto& get_invoker(Matrix& matrix, std::size_t i, Is... is) {
  return get_invoker(matrix[i], is...);
}

template <typename Visitor, typename... Variants>
inline constexpr auto invoke_matrix = make_invoke_matrix<false, Visitor, Variants...>();

template <typename Visitor, typename... Variants>
inline constexpr auto invoke_matrix_at = make_invoke_matrix<true, Visitor, Variants...>();

template <class Visitor, class... Variants>
constexpr decltype(auto) visit_at(Visitor&& vis, Variants&&... vars) {
  static_assert(sizeof...(Variants) > 0, "no variants for visit was provided");

  return get_invoker(invoke_matrix_at<Visitor, Variants...>, vars.index()...)(std::forward<Visitor>(vis),
                                                                              std::forward<Variants>(vars)...);
}

} // namespace details

template <class Visitor, class... Variants>
constexpr decltype(auto) visit(Visitor&& vis, Variants&&... vars) {
  static_assert(sizeof...(Variants) > 0, "no variants for visit was provided");

  if ((vars.valueless_by_exception() || ...)) {
    throw bad_variant_access("bad variant access: cannot apply visit to valueless by exception variant");
  }
  return details::get_invoker(details::invoke_matrix<Visitor, Variants...>,
                              vars.index()...)(std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
}

template <class... Types>
constexpr bool operator==(const variant<Types...>& v, const variant<Types...>& w) {
  if (v.index() != w.index()) {
    return false;
  } else if (v.valueless_by_exception()) {
    return true;
  }
  return details::visit_at([&]<std::size_t I>(in_place_index_t<I>) { return get<I>(v) == get<I>(w); }, v);
}

template <class... Types>
constexpr bool operator!=(const variant<Types...>& v, const variant<Types...>& w) {
  if (v.index() != w.index()) {
    return true;
  } else if (v.valueless_by_exception()) {
    return false;
  }
  return details::visit_at([&]<std::size_t I>(in_place_index_t<I>) { return get<I>(v) != get<I>(w); }, v);
}

template <class... Types>
constexpr bool operator<(const variant<Types...>& v, const variant<Types...>& w) {
  if (w.valueless_by_exception()) {
    return false;
  } else if (v.valueless_by_exception()) {
    return true;
  } else if (v.index() != w.index()) {
    return v.index() < w.index();
  }
  return details::visit_at([&]<std::size_t I>(in_place_index_t<I>) { return get<I>(v) < get<I>(w); }, v);
}

template <class... Types>
constexpr bool operator>(const variant<Types...>& v, const variant<Types...>& w) {
  if (v.valueless_by_exception()) {
    return false;
  } else if (w.valueless_by_exception()) {
    return true;
  } else if (v.index() != w.index()) {
    return v.index() > w.index();
  }
  return details::visit_at([&]<std::size_t I>(in_place_index_t<I>) { return get<I>(v) > get<I>(w); }, v);
}

template <class... Types>
constexpr bool operator<=(const variant<Types...>& v, const variant<Types...>& w) {
  if (v.valueless_by_exception()) {
    return true;
  } else if (w.valueless_by_exception()) {
    return false;
  } else if (v.index() != w.index()) {
    return v.index() <= w.index();
  }
  return details::visit_at([&]<std::size_t I>(in_place_index_t<I>) { return get<I>(v) <= get<I>(w); }, v);
}

template <class... Types>
constexpr bool operator>=(const variant<Types...>& v, const variant<Types...>& w) {
  if (w.valueless_by_exception()) {
    return true;
  } else if (v.valueless_by_exception()) {
    return false;
  } else if (v.index() != w.index()) {
    return v.index() >= w.index();
  }
  return details::visit_at([&]<std::size_t I>(in_place_index_t<I>) { return get<I>(v) >= get<I>(w); }, v);
}
