#pragma once

#include <array>
#include <concepts>
#include <type_traits>
#include <utility>

template <typename... Types>
class variant;

inline constexpr std::size_t variant_npos = -1;

template <typename T>
struct in_place_type_t;

template <std::size_t I>
struct in_place_index_t;

namespace details {

template <bool trivial, typename... Types>
struct variant_storage;

template <typename T>
struct storage_size;

template <typename... Types>
struct storage_size<variant<Types...>> : std::integral_constant<std::size_t, sizeof...(Types)> {};

template <bool trivial, typename... Types>
struct storage_size<variant_storage<trivial, Types...>> : std::integral_constant<std::size_t, sizeof...(Types)> {};

template <typename T>
struct storage_size<const T> : storage_size<T> {};
template <typename T>
struct storage_size<volatile T> : storage_size<T> {};
template <typename T>
struct storage_size<const volatile T> : storage_size<T> {};

template <class T>
inline constexpr std::size_t storage_size_v = storage_size<T>::value;

template <typename T, typename... Types>
struct get_index_by_type;

template <typename T, typename Head, typename... Tail>
struct get_index_by_type<T, Head, Tail...>
    : std::integral_constant<std::size_t, (get_index_by_type<T, Tail...>::value != variant_npos)
                                              ? 1 + get_index_by_type<T, Tail...>::value
                                              : variant_npos> {};

template <typename T, typename... Tail>
struct get_index_by_type<T, T, Tail...> : std::integral_constant<std::size_t, 0> {};

template <typename T>
struct get_index_by_type<T> : std::integral_constant<std::size_t, variant_npos> {};

template <typename T, typename... Types>
inline constexpr std::size_t get_index_by_type_v = get_index_by_type<T, Types...>::value;

struct empty_type {};

template <std::size_t index, typename... Types>
struct get_type_by_index;

template <std::size_t index, typename Head, typename... Tail>
struct get_type_by_index<index, Head, Tail...> {
  using type = typename get_type_by_index<index - 1, Tail...>::type;
};

template <typename Head, typename... Tail>
struct get_type_by_index<0, Head, Tail...> {
  using type = Head;
};

template <std::size_t index>
struct get_type_by_index<index> {
  using type = empty_type;
};

template <std::size_t index, typename... Types>
using get_type_by_index_t = typename get_type_by_index<index, Types...>::type;

template <typename T, typename... Types>
struct get_amount;

template <typename T, typename Head, typename... Tail>
struct get_amount<T, Head, Tail...> : std::integral_constant<std::size_t, get_amount<T, Tail...>::value> {};

template <typename T, typename... Tail>
struct get_amount<T, T, Tail...> : std::integral_constant<std::size_t, 1 + get_amount<T, Tail...>::value> {};

template <typename T>
struct get_amount<T> : std::integral_constant<std::size_t, 0> {};

template <typename T, typename... Types>
inline constexpr std::size_t get_amount_v = get_amount<T, Types...>::value;

template <typename T>
struct is_in_place_type : std::false_type {};

template <typename T>
struct is_in_place_type<in_place_type_t<T>> : std::true_type {};

template <typename T>
inline constexpr bool is_in_place_type_v = is_in_place_type<T>::value;

template <typename T>
struct is_in_place_index : std::false_type {};

template <std::size_t X>
struct is_in_place_index<in_place_index_t<X>> : std::true_type {};

template <typename T>
inline constexpr bool is_in_place_index_v = is_in_place_index<T>::value;

template <typename T>
concept CopyConstructible = std::is_copy_constructible_v<T>;

template <typename... Types>
concept AllCopyConstructible = (CopyConstructible<Types> && ...);

template <typename T>
concept TrivialCopyConstructible = std::is_trivially_copy_constructible_v<T>;

template <typename... Types>
concept AllTrivialCopyConstructible = (TrivialCopyConstructible<Types> && ...);

template <typename T>
concept CopyAssignable = std::is_copy_assignable_v<T>;

template <typename... Types>
concept AllCopyAssignable = (CopyAssignable<Types> && ...);

template <typename T>
concept TrivialCopyAssignable = std::is_trivially_copy_assignable_v<T>;

template <typename... Types>
concept AllTrivialCopyAssignable = (TrivialCopyAssignable<Types> && ...);

template <typename T>
concept MoveConstructible = std::is_move_constructible_v<T>;

template <typename... Types>
concept AllMoveConstructible = (MoveConstructible<Types> && ...);

template <typename T>
concept TrivialMoveConstructible = std::is_trivially_move_constructible_v<T>;

template <typename... Types>
concept AllTrivialMoveConstructible = (TrivialMoveConstructible<Types> && ...);

template <typename T>
concept MoveAssignable = std::is_move_assignable_v<T>;

template <typename... Types>
concept AllMoveAssignable = (MoveAssignable<Types> && ...);

template <typename T>
concept TrivialMoveAssignable = std::is_trivially_move_assignable_v<T>;

template <typename... Types>
concept AllTrivialMoveAssignable = (TrivialMoveAssignable<Types> && ...);

template <typename T>
concept TrivialDestructible = std::is_trivially_destructible_v<T>;

template <typename... Types>
concept AllTrivialDestructible = (TrivialDestructible<Types> && ...);

template <std::size_t I, typename... Types>
concept SizeCheck = (I < sizeof...(Types));

template <typename T, typename... Types>
concept OneInTypes = (get_amount_v<T, Types...> == 1);

template <typename U, typename V>
concept SameWithoutSvref = std::is_same_v<std::remove_cvref<U>, std::remove_cvref<V>>;

template <typename T>
concept InPlaceTypeT = is_in_place_type_v<T>;

template <typename T>
concept InPlaceIndexT = is_in_place_index_v<T>;

template <typename From, typename To, typename ToArray = To[]>
concept Overloadable = requires(From && t) {
  ToArray{std::forward<From>(t)};
};

template <typename T, typename... Types>
struct overloader;

template <typename T, typename Head, typename... Tail>
struct overloader<T, Head, Tail...> : overloader<T, Tail...> {
  using overloader<T, Tail...>::operator();

  Head operator()(Head t) requires(Overloadable<T, Head>);
};

template <typename T, typename Last>
struct overloader<T, Last> {
  Last operator()(Last t) requires(Overloadable<T, Last>);
};

template <typename T, typename... Types>
using get_best_match_t = std::invoke_result_t<overloader<T, Types...>, T>;

} // namespace details
