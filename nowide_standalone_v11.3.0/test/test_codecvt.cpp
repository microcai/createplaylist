//
// Copyright (c) 2015 Artyom Beilis (Tonkikh)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <nowide/utf8_codecvt.hpp>

#include <nowide/convert.hpp>
#include "test.hpp"
#include "test_sets.hpp"
#include <cstring>
#include <iomanip>
#include <iostream>
#include <locale>
#include <vector>

// MSVC has problems with an undefined symbol std::codecvt::id in some versions if the utf char types are used. See
// https://social.msdn.microsoft.com/Forums/vstudio/en-US/8f40dcd8-c67f-4eba-9134-a19b9178e481/vs-2015-rc-linker-stdcodecvt-error?forum=vcgeneral
// Workaround: use int16_t instead of char16_t
#if defined(_MSC_VER) && _MSC_VER >= 1900 && _MSC_VER <= 1916
#define NOWIDE_REQUIRE_UTF_CHAR_WORKAROUND 1
#else
#define NOWIDE_REQUIRE_UTF_CHAR_WORKAROUND 0
#endif

static const char* utf8_name =
  "\xf0\x9d\x92\x9e-\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82-\xE3\x82\x84\xE3\x81\x82.txt";
static const std::wstring wide_name_str = nowide::widen(utf8_name);
static const wchar_t* wide_name = wide_name_str.c_str();

using cvt_type = std::codecvt<wchar_t, char, std::mbstate_t>;

#if NOWIDE_REQUIRE_UTF_CHAR_WORKAROUND
using utf16_char_t = int16_t;
using utf32_char_t = int32_t;
#else
using utf16_char_t = char16_t;
using utf32_char_t = char32_t;
#endif

NOWIDE_SUPPRESS_UTF_CODECVT_DEPRECATION_BEGIN
using cvt_type16 = std::codecvt<utf16_char_t, char, std::mbstate_t>;
using cvt_type32 = std::codecvt<utf32_char_t, char, std::mbstate_t>;
using utf8_utf16_codecvt = nowide::utf8_codecvt<utf16_char_t>;
using utf8_utf32_codecvt = nowide::utf8_codecvt<utf32_char_t>;
NOWIDE_SUPPRESS_UTF_CODECVT_DEPRECATION_END

void test_codecvt_basic()
{
    // UTF-16
    {
        NOWIDE_SUPPRESS_UTF_CODECVT_DEPRECATION_BEGIN
        std::locale l(std::locale::classic(), new utf8_utf16_codecvt());
        const cvt_type16& cvt = std::use_facet<cvt_type16>(l);
        NOWIDE_SUPPRESS_UTF_CODECVT_DEPRECATION_END
        TEST_EQ(cvt.encoding(), 0);   // Characters have a variable width
        TEST_EQ(cvt.max_length(), 4); // At most 4 UTF-8 code units are one internal char (one or two UTF-16 code units)
        TEST(!cvt.always_noconv());   // Always convert
    }
    // UTF-32
    {
        NOWIDE_SUPPRESS_UTF_CODECVT_DEPRECATION_BEGIN
        std::locale l(std::locale::classic(), new utf8_utf32_codecvt());
        const cvt_type32& cvt = std::use_facet<cvt_type32>(l);
        NOWIDE_SUPPRESS_UTF_CODECVT_DEPRECATION_END
        TEST_EQ(cvt.encoding(), 0);   // Characters have a variable width
        TEST_EQ(cvt.max_length(), 4); // At most 4 UTF-8 code units are one internal char (one UTF-32 code unit)
        TEST(!cvt.always_noconv());   // Always convert
    }
}

void test_codecvt_unshift()
{
    char buf[256];
    // UTF-16
    {
        const auto name16 =
          nowide::utf::convert_string<utf16_char_t>(utf8_name, utf8_name + std::strlen(utf8_name));

        utf8_utf16_codecvt cvt16;
        // Unshift on initial state does nothing
        std::mbstate_t mb{};
        char* to_next;
        NOWIDE_SUPPRESS_UTF_CODECVT_DEPRECATION_BEGIN
        const cvt_type16& cvt = cvt16;
        TEST_EQ(cvt.unshift(mb, buf, std::end(buf), to_next), cvt_type16::ok);
        TEST(to_next == buf);
        const utf16_char_t* from_next;
        // Convert into a to small buffer
        TEST_EQ(cvt.out(mb, &name16.front(), &name16.back(), from_next, buf, buf + 1, to_next), cvt_type16::partial);
        TEST(from_next == &name16[1]);
        TEST(to_next == buf);
        // Unshift on non-default state is not possible
        TEST_EQ(cvt.unshift(mb, buf, std::end(buf), to_next), cvt_type16::error);
        NOWIDE_SUPPRESS_UTF_CODECVT_DEPRECATION_END
    }
    // UTF-32
    {
        const auto name32 =
          nowide::utf::convert_string<utf32_char_t>(utf8_name, utf8_name + std::strlen(utf8_name));

        utf8_utf32_codecvt cvt32;
        // Unshift on initial state does nothing
        std::mbstate_t mb{};
        char* to_next;
        NOWIDE_SUPPRESS_UTF_CODECVT_DEPRECATION_BEGIN
        const cvt_type32& cvt = cvt32;
        TEST_EQ(cvt.unshift(mb, buf, std::end(buf), to_next), cvt_type32::noconv);
        TEST(to_next == buf);
        const utf32_char_t* from_next;
        // Convert into a too small buffer
        TEST_EQ(cvt.out(mb, &name32.front(), &name32.back(), from_next, buf, buf + 1, to_next), cvt_type32::partial);
        TEST(from_next == &name32.front()); // Noting consumed
        TEST(to_next == buf);
        TEST(std::mbsinit(&mb) != 0); // State unchanged --> Unshift does nothing
        TEST_EQ(cvt.unshift(mb, buf, std::end(buf), to_next), cvt_type32::noconv);
        TEST(to_next == buf);
        NOWIDE_SUPPRESS_UTF_CODECVT_DEPRECATION_END
    }
}

void test_codecvt_in_n_m(const cvt_type& cvt, size_t n, size_t m)
{
    const wchar_t* wptr = wide_name;
    size_t wlen = std::wcslen(wide_name);
    size_t u8len = std::strlen(utf8_name);
    const char* from = utf8_name;
    const char* from_end = from;
    const char* real_end = utf8_name + u8len;
    const char* from_next = from;
    std::mbstate_t mb{};
    while(from_next < real_end)
    {
        if(from == from_end)
        {
            from_end = from + n;
            if(from_end > real_end)
                from_end = real_end;
        }

        wchar_t buf[128];
        wchar_t* to = buf;
        wchar_t* to_end = to + m;
        wchar_t* to_next = to;

        std::mbstate_t mb2 = mb;
        std::codecvt_base::result r = cvt.in(mb, from, from_end, from_next, to, to_end, to_next);

        int count = cvt.length(mb2, from, from_end, to_end - to);
        TEST_EQ(std::memcmp(&mb, &mb2, sizeof(mb)), 0);
        TEST_EQ(count, from_next - from);

        if(r == cvt_type::partial)
        {
            from_end += n;
            if(from_end > real_end)
                from_end = real_end;
        } else
            TEST_EQ(r, cvt_type::ok);
        while(to != to_next)
        {
            TEST(*wptr == *to);
            wptr++;
            to++;
        }
        to = to_next;
        from = from_next;
    }
    TEST(wptr == wide_name + wlen);
    TEST(from == real_end);
}

void test_codecvt_out_n_m(const cvt_type& cvt, size_t n, size_t m)
{
    const char* nptr = utf8_name;
    size_t wlen = std::wcslen(wide_name);
    size_t u8len = std::strlen(utf8_name);

    std::mbstate_t mb{};

    const wchar_t* from_next = wide_name;
    const wchar_t* real_from_end = wide_name + wlen;

    char buf[256];
    char* to = buf;
    char* to_next = to;
    char* to_end = to + n;
    char* real_to_end = buf + sizeof(buf);

    while(from_next < real_from_end)
    {
        const wchar_t* from = from_next;
        const wchar_t* from_end = from + m;
        if(from_end > real_from_end)
            from_end = real_from_end;
        if(to_end == to)
        {
            to_end = to + n;
        }

        std::codecvt_base::result r = cvt.out(mb, from, from_end, from_next, to, to_end, to_next);
        if(r == cvt_type::partial)
        {
            // If those are equal, then "partial" probably means: Need more input
            // Otherwise "Need more output"
            if(from_next != from_end)
            {
                TEST(to_end - to_next < cvt.max_length());
                to_end += n;
                TEST(to_end <= real_to_end); // Should always be big enough
            }
        } else
        {
            TEST_EQ(r, cvt_type::ok);
        }

        while(to != to_next)
        {
            TEST_EQ(*nptr, *to);
            nptr++;
            to++;
        }
        from = from_next;
    }
    TEST(nptr == utf8_name + u8len);
    TEST(from_next == real_from_end);
    const auto expected = (sizeof(wchar_t) == 2) ? cvt_type::ok : cvt_type::noconv; // UTF-32 is not state-dependent
    TEST_EQ(cvt.unshift(mb, to, to + n, to_next), expected);
    TEST(to_next == to);
}

void test_codecvt_conv()
{
    std::cout << "Conversions " << std::endl;
    std::locale l(std::locale::classic(), new nowide::utf8_codecvt<wchar_t>());

    const cvt_type& cvt = std::use_facet<cvt_type>(l);
    const size_t utf8_len = std::strlen(utf8_name);
    const size_t wide_len = std::wcslen(wide_name);

    for(size_t i = 1; i <= utf8_len + 1; i++)
    {
        for(size_t j = 1; j <= wide_len + 1; j++)
        {
            try
            {
                test_codecvt_in_n_m(cvt, i, j);
                test_codecvt_out_n_m(cvt, i, j);
            } catch(...) // LCOV_EXCL_LINE
            {
                std::cerr << "Wlen=" << j << " Nlen=" << i << std::endl; // LCOV_EXCL_LINE
                throw;                                                   // LCOV_EXCL_LINE
            }
        }
    }
}

void test_codecvt_err()
{
    std::cout << "Errors " << std::endl;
    std::locale l(std::locale::classic(), new nowide::utf8_codecvt<wchar_t>());

    const cvt_type& cvt = std::use_facet<cvt_type>(l);

    std::cout << "- UTF-8" << std::endl;
    {
        {
            wchar_t buf[4];
            wchar_t* const to = buf;
            wchar_t* const to_end = buf + 4;
            const char* err_utf = "1\xFF\xFF\xd7\xa9";
            std::mbstate_t mb{};
            const char* from = err_utf;
            const char* from_end = from + std::strlen(from);
            const char* from_next = from;
            wchar_t* to_next = to;
            TEST_EQ(cvt.in(mb, from, from_end, from_next, to, to_end, to_next), cvt_type::ok);
            TEST(from_next == from + 5);
            TEST(to_next == to + 4);
            TEST(std::wstring(to, to_end) == nowide::widen(err_utf));
        }
        {
            wchar_t buf[4];
            wchar_t* const to = buf;
            wchar_t* const to_end = buf + 4;
            const char* err_utf = "1\xd7"; // 1 valid, 1 incomplete UTF-8 char
            std::mbstate_t mb{};
            const char* from = err_utf;
            const char* from_end = from + std::strlen(from);
            const char* from_next = from;
            wchar_t* to_next = to;
            TEST_EQ(cvt.in(mb, from, from_end, from_next, to, to_end, to_next), cvt_type::partial);
            TEST(from_next == from + 1);
            TEST(to_next == to + 1);
            // False positive in GCC 13 in MinGW
#if defined(__GNUC__) && __GNUC__ >= 13
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfree-nonheap-object"
#endif
            TEST(std::wstring(to, to_next) == std::wstring(L"1"));
#if defined(__GNUC__) && __GNUC__ >= 13
#pragma GCC diagnostic pop
#endif
        }
        {
            char buf[4] = {};
            char* const to = buf;
            char* const to_end = buf + 4;
            char* to_next = to;
            const wchar_t* err_utf = L"\xD800"; // Trailing UTF-16 surrogate
            std::mbstate_t mb{};
            const wchar_t* from = err_utf;
            const wchar_t* from_end = from + 1;
            const wchar_t* from_next = from;
            cvt_type::result res = cvt.out(mb, from, from_end, from_next, to, to_end, to_next);
#ifdef NOWIDE_MSVC
#pragma warning(disable : 4127) // Constant expression detected
#endif
            if(sizeof(wchar_t) == 2)
            {
                TEST_EQ(res, cvt_type::partial);
                TEST(from_next == from_end);
                TEST(to_next == to);
                TEST_EQ(buf[0], 0);
            } else
            {
                TEST_EQ(res, cvt_type::ok);
                TEST(from_next == from_end);
                TEST(to_next == to + 3);
                // surrogate is invalid
                TEST_EQ(std::string(to, to_next), nowide::narrow(wreplacement_str));
            }
        }
    }

    std::cout << "- UTF-16/32" << std::endl;
    {
        char buf[32];
        char* to = buf;
        char* to_end = buf + 32;
        char* to_next = to;
        wchar_t err_buf[] = {'1', 0xDC9E, 0}; // second value is invalid for UTF-16 and 32
        const wchar_t* err_utf = err_buf;
        {
            std::mbstate_t mb{};
            const wchar_t* from = err_utf;
            const wchar_t* from_end = from + std::wcslen(from);
            const wchar_t* from_next = from;
            TEST_EQ(cvt.out(mb, from, from_end, from_next, to, to_end, to_next), cvt_type::ok);
            TEST(from_next == from + 2);
            TEST(to_next == to + 4);
            TEST_EQ(std::string(to, to_next), nowide::narrow(err_buf));
        }
    }
}

std::wstring codecvt_to_wide(const std::string& s)
{
    std::locale l(std::locale::classic(), new nowide::utf8_codecvt<wchar_t>());

    const cvt_type& cvt = std::use_facet<cvt_type>(l);

    std::mbstate_t mb{};
    const char* const from = s.c_str();
    const char* const from_end = from + s.size();
    const char* from_next = from;

    std::vector<wchar_t> buf(s.size() + 2); // +1 for possible incomplete char, +1 for NULL
    wchar_t* const to = &buf[0];
    wchar_t* const to_end = to + buf.size();
    wchar_t* to_next = to;

    const auto expected_consumed = cvt.length(mb, from, from_end, buf.size());
    cvt_type::result res = cvt.in(mb, from, from_end, from_next, to, to_end, to_next);
    TEST_EQ(expected_consumed, from_next - from);
    if(res == cvt_type::partial)
    {
        TEST(to_next < to_end);
        *(to_next++) = NOWIDE_REPLACEMENT_CHARACTER;
    } else
        TEST_EQ(res, cvt_type::ok);

    return std::wstring(to, to_next);
}

std::string codecvt_to_narrow(const std::wstring& s)
{
    std::locale l(std::locale::classic(), new nowide::utf8_codecvt<wchar_t>());

    const cvt_type& cvt = std::use_facet<cvt_type>(l);

    std::mbstate_t mb{};
    const wchar_t* const from = s.c_str();
    const wchar_t* const from_end = from + s.size();
    const wchar_t* from_next = from;

    std::vector<char> buf((s.size() + 1) * 4 + 1); // +1 for possible incomplete char, +1 for NULL
    char* const to = &buf[0];
    char* const to_end = to + buf.size();
    char* to_next = to;

    cvt_type::result res = cvt.out(mb, from, from_end, from_next, to, to_end, to_next);
    if(res == cvt_type::partial)
    {
        TEST(to_next < to_end);
        return std::string(to, to_next) + nowide::narrow(wreplacement_str);
    } else
        TEST_EQ(res, cvt_type::ok);

    return std::string(to, to_next);
}

void test_codecvt_subst()
{
    std::cout << "Substitutions " << std::endl;
    run_all(codecvt_to_wide, codecvt_to_narrow);
}

// coverity[root_function]
void test_main(int, char**, char**)
{
    test_codecvt_basic();
    test_codecvt_unshift();
    test_codecvt_conv();
    test_codecvt_err();
    test_codecvt_subst();
}
