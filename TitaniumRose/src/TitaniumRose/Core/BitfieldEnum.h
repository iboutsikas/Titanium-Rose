#pragma once
#include <type_traits>

template<typename Enum>
struct EnableBitMaskOperators
{
	static const bool enable = false;

	constexpr explicit operator bool() const 
	{
		return *this != 0;
	}
};

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
constexpr operator |(Enum lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<Enum> (
		static_cast<underlying>(lhs) |
		static_cast<underlying>(rhs)
		);
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
constexpr operator |=(Enum lhs, Enum rhs)
{
	lhs = lhs | rhs;
	return lhs;
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
constexpr operator &(Enum lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<Enum> (
		static_cast<underlying>(lhs) &
		static_cast<underlying>(rhs)
		);
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
constexpr operator &=(Enum lhs, Enum rhs)
{
	lhs = lhs & rhs;
	return lhs;
}

#define ENABLE_BITMASK_OPERATORS(x)	\
template<>                           \
struct EnableBitMaskOperators<x>     \
{                                    \
static const bool enable = true; \
};
