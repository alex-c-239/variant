#pragma once

#include "variant_storage.h"

template <typename... Types>
class variant {
private:
  static_assert(sizeof...(Types) > 0, "variant must have at least one alternative");
  static_assert((!std::is_reference_v<Types> && ...), "variant cannot store a reference type");
  static_assert((!std::is_array_v<Types> && ...), "variant cannot store an array type");

  using storage_t = details::variant_storage<details::AllTrivialDestructible<Types...>, Types...>;
  using first_t = variant_alternative_t<0, variant<Types...>>;

  static constexpr std::size_t end_index() {
    return sizeof...(Types);
  }

public:
  constexpr variant() noexcept(std::is_nothrow_default_constructible_v<first_t>)
      requires(std::is_default_constructible_v<first_t>) = default;

  template <std::size_t I, typename... Args, typename T = variant_alternative_t<I, variant<Types...>>>
  constexpr explicit variant(in_place_index_t<I>, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
      requires(details::SizeCheck<I, Types...>&& std::is_constructible_v<T, Args...>)
      : m_storage(in_place_index<I>, std::forward<Args>(args)...) {}

  template <class T, typename... Args, std::size_t I = details::get_index_by_type_v<T, Types...>>
  constexpr explicit variant(in_place_type_t<T>, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
      requires(details::OneInTypes<T, Types...>&& std::is_constructible_v<T, Args...>)
      : variant(in_place_index<I>, std::forward<Args>(args)...) {}

  template <typename T, typename U = details::get_best_match_t<T, Types...>>
  constexpr variant(T&& t) noexcept(std::is_nothrow_constructible_v<U, T>)
      requires(!details::SameWithoutSvref<T, variant> && !details::InPlaceTypeT<T> && !details::InPlaceIndexT<T> &&
               details::OneInTypes<U, Types...>)
      : variant(in_place_type<U>, std::forward<T>(t)) {}

  constexpr variant(const variant& other)
      requires(details::AllTrivialCopyConstructible<Types...>&& details::AllCopyConstructible<Types...>) = default;

  constexpr variant(const variant& other) requires(details::AllCopyConstructible<Types...>)
      : m_storage(in_place_index<end_index()>) {
    if (other.valueless_by_exception()) {
      m_storage.m_index = variant_npos;
      return;
    }
    details::visit_at([&, this]<std::size_t I>(in_place_index_t<I>) { this->template emplace<I>(get<I>(other)); },
                      other);
  }

  constexpr variant(variant&& other) noexcept((std::is_nothrow_move_constructible_v<Types> && ...))
      requires(details::AllTrivialMoveConstructible<Types...>&& details::AllMoveConstructible<Types...>) = default;

  constexpr variant(variant&& other) noexcept((std::is_nothrow_move_constructible_v<Types> && ...))
      requires(details::AllMoveConstructible<Types...>)
      : m_storage(in_place_index<end_index()>) {
    if (other.valueless_by_exception()) {
      m_storage.m_index = variant_npos;
      return;
    }
    details::visit_at(
        [&, this]<std::size_t I>(in_place_index_t<I>) { this->template emplace<I>(get<I>(std::move(other))); }, other);
  }

  template <class T, typename U = details::get_best_match_t<T, Types...>,
            std::size_t I = details::get_index_by_type_v<U, Types...>>
  constexpr variant&
  operator=(T&& t) noexcept(std::is_nothrow_assignable_v<U&, T>&& std::is_nothrow_constructible_v<U, T>)
      requires(!details::SameWithoutSvref<T, variant> && details::OneInTypes<U, Types...>) {
    if (index() == I) {
      get<I>(*this) = std::forward<T>(t);
    } else if (std::is_nothrow_constructible_v<U, T>) {
      emplace<I>(std::forward<T>(t));
    } else {
      emplace<I>(U(std::forward<T>(t)));
    }
    return *this;
  }

  constexpr variant& operator=(const variant& other)
      requires(details::AllTrivialCopyAssignable<Types...>&& details::AllTrivialCopyConstructible<Types...>&&
                   details::AllCopyAssignable<Types...>&& details::AllCopyConstructible<Types...>) = default;

  constexpr variant& operator=(const variant& other)
      requires(details::AllCopyAssignable<Types...>&& details::AllCopyConstructible<Types...>) {
    if (index() == other.index() && !other.valueless_by_exception()) {
      details::visit_at([&, this]<std::size_t I>(in_place_index_t<I>) { *this = get<I>(other); }, other);
    } else if (!other.valueless_by_exception()) {
      try {
        details::visit_at([&, this]<std::size_t I>(in_place_index_t<I>) { this->template emplace<I>(get<I>(other)); },
                          other);
      } catch (...) {
        this->m_storage.reset();
        throw;
      }
    } else if (!valueless_by_exception()) {
      m_storage.reset();
    }
    return *this;
  }

  constexpr variant& operator=(variant&& other) noexcept(((std::is_nothrow_move_constructible_v<Types> &&
                                                           std::is_nothrow_move_assignable_v<Types>)&&...))
      requires(details::AllTrivialMoveAssignable<Types...>&& details::AllTrivialMoveConstructible<Types...>&&
                   details::AllMoveAssignable<Types...>&& details::AllMoveConstructible<Types...>) = default;

  constexpr variant& operator=(variant&& other) noexcept(((std::is_nothrow_move_constructible_v<Types> &&
                                                           std::is_nothrow_move_assignable_v<Types>)&&...))
      requires(details::AllMoveAssignable<Types...>&& details::AllMoveConstructible<Types...>) {
    if (index() == other.index() && !other.valueless_by_exception()) {
      details::visit_at([&, this]<std::size_t I>(in_place_index_t<I>) { *this = get<I>(std::move(other)); }, *this);
    } else if (!other.valueless_by_exception()) {
      try {
        details::visit_at(
            [&, this]<std::size_t I>(in_place_index_t<I>) { this->template emplace<I>(get<I>(std::move(other))); },
            other);
      } catch (...) {
        this->m_storage.reset();
        throw;
      }
    } else if (!valueless_by_exception()) {
      m_storage.reset();
    }
    return *this;
  }

  template <std::size_t I, typename... Args, typename U = variant_alternative_t<I, variant>>
  constexpr U& emplace(Args&&... args) requires(details::SizeCheck<I, Types...>&& std::is_constructible_v<U, Args...>) {
    return m_storage.template emplace<I>(std::forward<Args>(args)...);
  }

  template <typename T, typename... Args, std::size_t I = details::get_index_by_type_v<T, Types...>>
  constexpr T& emplace(Args&&... args)
      requires(details::OneInTypes<T, Types...>&& std::is_constructible_v<T, Args...>) {
    return emplace<I>(std::forward<Args>(args)...);
  }

  constexpr void swap(variant& other) noexcept(((std::is_nothrow_move_constructible_v<Types> &&
                                                 std::is_nothrow_swappable_v<Types>)&&...))
      requires(details::AllMoveConstructible<Types...>&& details::AllMoveAssignable<Types...>) {
    if (valueless_by_exception() && other.valueless_by_exception()) {
      return;
    } else if (index() == other.index()) {
      details::visit_at(
          [&, this]<std::size_t I>(in_place_index_t<I>) {
            using std::swap;
            swap(get<I>(*this), get<I>(other));
          },
          other);
    } else {
      std::swap(other, *this);
    }
  }

  constexpr std::size_t index() const noexcept {
    return m_storage.index();
  }
  constexpr bool valueless_by_exception() const noexcept {
    return m_storage.index() == variant_npos;
  }

private:
  template <std::size_t I, typename... Ty>
  friend constexpr variant_alternative_t<I, variant<Ty...>>& get(variant<Ty...>& v);
  template <std::size_t I, typename... Ty>
  friend constexpr variant_alternative_t<I, variant<Ty...>>&& get(variant<Ty...>&& v);
  template <std::size_t I, typename... Ty>
  friend constexpr const variant_alternative_t<I, variant<Ty...>>& get(const variant<Ty...>& v);
  template <std::size_t I, typename... Ty>
  friend constexpr const variant_alternative_t<I, variant<Ty...>>&& get(const variant<Ty...>&& v);

  template <class... Ty>
  friend constexpr bool operator==(const variant<Ty...>& v, const variant<Ty...>& w);
  template <class... Ty>
  friend constexpr bool operator!=(const variant<Ty...>& v, const variant<Ty...>& w);
  template <class... Ty>
  friend constexpr bool operator<(const variant<Ty...>& v, const variant<Ty...>& w);
  template <class... Ty>
  friend constexpr bool operator>(const variant<Ty...>& v, const variant<Ty...>& w);
  template <class... Ty>
  friend constexpr bool operator<=(const variant<Ty...>& v, const variant<Ty...>& w);
  template <class... Ty>
  friend constexpr bool operator>=(const variant<Ty...>& v, const variant<Ty...>& w);

  storage_t m_storage;
};
