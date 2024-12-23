
#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

#include <cstdint>
#include <string>
#include <variant>
#include <climits>

template<template< typename > typename Allocator = std::pmr::polymorphic_allocator>
struct basic_generic_string
{
	using u8string = std::basic_string<char8_t, std::char_traits<char8_t>, Allocator<char8_t>>;
	using string = std::basic_string<char, std::char_traits<char>, Allocator<char>>;
	using wstring = std::basic_string<wchar_t, std::char_traits<wchar_t>, Allocator<wchar_t>>;
	std::variant<u8string, wstring, string> str;

	template<typename T>
	basic_generic_string(T&& t)
		: str(std::forward<T>(t))
	{}

	inline operator string() const
	{
		return to_string();
	}

	inline operator u8string() const
	{
		return to_u8string();
	}

	inline operator wstring() const
	{
		return to_wstring();
	}

	string to_string() const
	{
		if (std::holds_alternative<string>(str))
		{
			return std::get<string>(str);
		}
		else if (std::holds_alternative<wstring>(str))
		{
			return wstring_to_string(std::get<wstring>(str));
		}
		else if (std::holds_alternative<u8string>(str))
		{
			return u8string_to_string(std::get<u8string>(str));
		}
		return {};
	}

	u8string to_u8string() const
	{
		if (std::holds_alternative<string>(str))
		{
			return string_to_u8string(std::get<string>(str));
		}
		else if (std::holds_alternative<wstring>(str))
		{
			return wstring_to_u8string(std::get<wstring>(str));
		}
		else if (std::holds_alternative<u8string>(str))
		{
			return std::get<u8string>(str);
		}
		return {};
	}

	wstring to_wstring() const
	{
		if (std::holds_alternative<string>(str))
		{
			return string_to_wstring(std::get<string>(str));
		}
		else if (std::holds_alternative<wstring>(str))
		{
			return std::get<wstring>(str);
		}
		else if (std::holds_alternative<u8string>(str))
		{
			return u8string_to_wstring(std::get<u8string>(str));
		}
		return {};
	}

	static string u8string_to_string(const u8string& u8str)
	{
// TODO convert u8string to string
#ifdef _WIN32
		wstring wbuf{u8str.get_allocator()};
		wbuf.resize(u8str.size());
		auto wchar_count = MultiByteToWideChar(CP_UTF8,
			MB_ERR_INVALID_CHARS,
			reinterpret_cast<const char*>(u8str.c_str()),
			u8str.size(),
			&wbuf[0],
			wbuf.size());
		wbuf.resize(wchar_count);

		string ansistr{u8str.get_allocator()};
		ansistr.resize(wchar_count * 4);
		auto utf8_len
			= WideCharToMultiByte(CP_ACP, 0, wbuf.c_str(), wbuf.size(), &u8str[0], u8str.size(), nullptr, nullptr);
		ansistr.resize(utf8_len);

		return ansistr;
#else
		// for Linux, string is by default just u8string
		return string(reinterpret_cast<const char*>(u8str.c_str()), u8str.size(), u8str.get_allocator());
#endif
	}

	static u8string string_to_u8string(const string& str)
	{
#ifdef _WIN32
		wstring wbuf{str.get_allocator()};
		wbuf.resize(str.size());
		auto wchar_count
			= MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, str.c_str(), str.size(), &wbuf[0], wbuf.size());
		wbuf.resize(wchar_count);

		u8string u8str{str.get_allocator()};
		u8str.resize(wchar_count * 4);
		auto utf8_len = WideCharToMultiByte(
			CP_UTF8, 0, wbuf.c_str(), wbuf.size(), reinterpret_cast<char*>(&u8str[0]), u8str.size(), nullptr, nullptr);
		u8str.resize(utf8_len);

		return u8str;
#else
		// for Linux, string is by default just u8string
		return u8string(reinterpret_cast<const char8_t*>(str.c_str()), str.size(), str.get_allocator());
#endif
	}

	static string wstring_to_string(const wstring& wbuf)
	{
#ifdef _WIN32

		string u8str{wbuf.get_allocator()};
		u8str.resize(wbuf.length() * 4);
		auto utf8_len = WideCharToMultiByte(
			CP_UTF8, 0, wbuf.c_str(), wbuf.size(), reinterpret_cast<char*>(&u8str[0]), u8str.size(), nullptr, nullptr);
		u8str.resize(utf8_len);

		return u8str;
#else
		// for Linux, convert wstring to utf8
		return to_utf8<string>(wbuf, wbuf.get_allocator());
#endif
	}

	static wstring string_to_wstring(const string& str)
	{
#ifdef _WIN32
		wstring wbuf{str.get_allocator()};
		wbuf.resize(str.size());
		auto wchar_count
			= MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, str.c_str(), str.size(), &wbuf[0], wbuf.size());
		wbuf.resize(wchar_count);

		return wbuf;
#else
		// for Linux, string is by default just u8string
		return from_utf8(str, str.get_allocator());
#endif
	}

	static wstring u8string_to_wstring(const u8string& u8str)
	{
#ifdef _WIN32
		wstring wbuf{u8str.get_allocator()};
		wbuf.resize(u8str.size());
		auto wchar_count
			= MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, u8str.c_str(), u8str.size(), &wbuf[0], wbuf.size());
		wbuf.resize(wchar_count);

		return wbuf;
#else
		return from_utf8(u8str, u8str.get_allocator());
#endif
	}

	static u8string wstring_to_u8string(const wstring& wstr)
	{
#ifdef _WIN32
		u8string u8str{wstr.get_allocator()};
		u8str.resize(wstr.length() * 4);
		auto utf8_len = WideCharToMultiByte(
			CP_UTF8, 0, wstr.c_str(), wstr.size(), reinterpret_cast<char*>(&u8str[0]), u8str.size(), nullptr, nullptr);
		u8str.resize(utf8_len);

		return u8str;

#else
		return to_utf8<u8string>(wstr, wstr.get_allocator());
#endif
	}

#if WCHAR_MAX >= INT32_MAX
	// convert ucs to utf8
	template <typename STRING_TYPE = u8string>
	static STRING_TYPE to_utf8(std::wstring_view wstr, Allocator<typename STRING_TYPE::value_type> alloc = {})
	{
		using CHAR = typename STRING_TYPE::value_type;
		STRING_TYPE ret{alloc};
		ret.reserve(wstr.size()*6);

		for (wchar_t C : wstr)
		{
			if (C <= 0b01111111)
			{
				ret.push_back(static_cast<CHAR>(C));
			}
			else if (C <= 0b11111111111) // two byte encoding
			{
				ret.push_back(
					static_cast<CHAR>((C >> 6) | 0b11000000)
				);

				ret.push_back(
					static_cast<CHAR>(C & 0b111111 | 0b10000000)
				);
			}
			else if (C <= 0b1111111111111111) // 3 byte encoding
			{
				ret.push_back(
					static_cast<CHAR>((C >> 12) | 0b11100000)
				);

				ret.push_back(
					static_cast<CHAR>( (C>>6) & 0b111111 | 0b10000000)
				);

				ret.push_back(
					static_cast<CHAR>( (C) & 0b111111 | 0b10000000)
				);
			}
			else if (C <= 0b111111111111111111111) // 4 byte encoding
			{
				ret.push_back(
					static_cast<CHAR>((C >> 18) | 0b11110000)
				);

				ret.push_back(
					static_cast<CHAR>( (C>>12) & 0b111111 | 0b10000000)
				);

				ret.push_back(
					static_cast<CHAR>( (C>>6) & 0b111111 | 0b10000000)
				);

				ret.push_back(
					static_cast<CHAR>( (C) & 0b111111 | 0b10000000)
				);
			}
			else if (C <= 0b11111111111111111111111111) // 5 byte encoding
			{
				ret.push_back(
					static_cast<CHAR>((C >> 24) | 0b11111000)
				);

				ret.push_back(
					static_cast<CHAR>( (C>>18) & 0b111111 | 0b10000000)
				);

				ret.push_back(
					static_cast<CHAR>( (C>>12) & 0b111111 | 0b10000000)
				);

				ret.push_back(
					static_cast<CHAR>( (C>>6) & 0b111111 | 0b10000000)
				);

				ret.push_back(
					static_cast<CHAR>( (C) & 0b111111 | 0b10000000)
				);
			}
			else if (C <= 0b1111111111111111111111111111111) // 6 byte encoding
			{
				ret.push_back(
					static_cast<CHAR>((C >> 30) | 0b11111100)
				);

				ret.push_back(
					static_cast<CHAR>( (C>>24) & 0b111111 | 0b10000000)
				);

				ret.push_back(
					static_cast<CHAR>( (C>>18) & 0b111111 | 0b10000000)
				);

				ret.push_back(
					static_cast<CHAR>( (C>>12) & 0b111111 | 0b10000000)
				);

				ret.push_back(
					static_cast<CHAR>( (C>>6) & 0b111111 | 0b10000000)
				);

				ret.push_back(
					static_cast<CHAR>( (C) & 0b111111 | 0b10000000)
				);
			}
		}

		return ret;
	}

	template <typename STRING_TYPE = std::u8string_view>
	static wstring from_utf8(STRING_TYPE utf8_str, Allocator<wchar_t> alloc = {})
	{
		wstring ret{alloc};
		ret.reserve(utf8_str.size());

		for (auto p = utf8_str.begin(); p != utf8_str.end(); p++)
		{
			unsigned char c = *p;
			if (c <= 0b01111111)
			{
				ret.push_back(static_cast<wchar_t>(c));
			}
			else if ( c <= 0b11011111 )
			{
				p++;
				unsigned char c2 = *p;
				ret.push_back(static_cast<wchar_t>( ((c&0b11111) << 6) + (c2 & 0b111111)));
			}
			else if ( c <= 0b11101111 )
			{
				p++;
				unsigned char c2 = *p;
				p++;
				unsigned char c3 = *p;
				ret.push_back(static_cast<wchar_t>( ((c&0b1111) << 12) + ((c2 & 0b111111)<< 6) + (c3 &0b111111)));
			}
			else if ( c <= 0b11110111 )
			{
				p++;
				unsigned char c2 = *p;
				p++;
				unsigned char c3 = *p;
				p++;
				unsigned char c4 = *p;
				ret.push_back(static_cast<wchar_t>( ((c&0b111) << 18) + ((c2 & 0b111111)<< 12) + ((c3 &0b111111)<<6) + (c4&0b111111)));
			}
			else if ( c <= 0b11111011 )
			{
				p++;
				unsigned char c2 = *p;
				p++;
				unsigned char c3 = *p;
				p++;
				unsigned char c4 = *p;
				p++;
				unsigned char c5 = *p;
				ret.push_back(static_cast<wchar_t>( ((c&0b11) << 24) + ((c2 & 0b111111)<< 18) + ((c3 &0b111111)<<12) + ((c4&0b111111)<<6) + (c5&0b111111)));
			}
			else if ( c <= 0b11111101 )
			{
				p++;
				unsigned char c2 = *p;
				p++;
				unsigned char c3 = *p;
				p++;
				unsigned char c4 = *p;
				p++;
				unsigned char c5 = *p;
				p++;
				unsigned char c6 = *p;
				ret.push_back(static_cast<wchar_t>( ((c&0b1) << 30) + ((c2 & 0b111111)<< 24) + ((c3 &0b111111)<<18) + ((c4&0b111111)<<12) + ((c5&0b111111)<<6) + (c6&0b111111)));
			}
			else
			{
				continue;
			}
		}
		return ret;
	}
#endif
};

using generic_string = basic_generic_string<std::allocator>;

using pmr_generic_string = basic_generic_string<std::pmr::polymorphic_allocator>;
