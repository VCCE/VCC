#pragma once
#include <Windows.h>
#include <string>

template<class Type_>
inline LPCSTR ToLPCSTR(std::basic_string<Type_>& str)
{
	return str.c_str();
}

template<class Type_>
inline LPCSTR ToLPCSTR(const std::basic_string<Type_>& str)
{
	return str.c_str();
}

template<class Type_>
LPCSTR ToLPCSTR(const std::basic_filebuf<Type_>&& str)
{
	static_assert("RValue not supported for ToLPCSTR");
}

std::string ToHexString(long value, int length, bool leadingZeros = true);
std::string ToDecimalString(long value, int length, bool leadingZeros);
