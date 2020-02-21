#ifndef _C4_CHARCONV_HPP_
#define _C4_CHARCONV_HPP_

/** @file charconv.hpp Low-level conversion functions to/from strings */

#include <stdio.h>
#include <inttypes.h>
#include <type_traits>
#include <stdarg.h>
#include <climits>
#include <limits>
#include <utility>
#if (C4_CPP >= 17) && __has_include(<charconv>) && __cpp_lib_to_chars
#   define C4CORE_HAVE_STD_TOCHARS 1
#   include <charconv>
#else
#   define C4CORE_HAVE_STD_TOCHARS 0
#endif

#include "c4/config.hpp"
#include "c4/substr.hpp"

#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4800) //'int': forcing value to bool 'true' or 'false' (performance warning)
#   pragma warning(disable: 4996) // snprintf/scanf: this function or variable may be unsafe
#endif

namespace c4 {

/** @defgroup formatting Formatting functions */

/** @defgroup lowlevel_tofrom_chars Single value to/from string conversion
 * @brief Low-level functions providing type-specific
 * low-level conversion of values to and from string.
 * @ingroup formatting
 */

/** @defgroup generic_tofrom_chars Generic single value to/from string conversion
 * @brief Lightweight generic type-safe wrappers for
 * converting individual values to/from strings. These functions generally
 * just dispatch to the proper low-level conversion function.
 * @ingroup formatting
 *
 * These are the main functions:
 *
 * @code{.cpp}
 * // Convert the given value, writing into the string.
 * // The resulting string will NOT be null-terminated.
 * // Return the number of characters needed.
 * // This function is safe to call when the string is too small -
 * // no writes will occur beyond the string's last character.
 * template<class T> size_t to_chars(substr buf, T const& C4_RESTRICT val);
 *
 *
 * // Convert the given value to a string using to_chars(), and
 * // return the resulting string, up to and including the last
 * // written character.
 * template<class T> substr to_chars_sub(substr buf, T const& C4_RESTRICT val);
 *
 *
 * // read a value from the string, which must be
 * // trimmed to the value (ie, no leading/trailing whitespace).
 * // return true if the conversion succeeded
 * template<class T> bool from_chars(csubstr buf, T * C4_RESTRICT val);
 *
 *
 * // read the first valid sequence of characters from the string
 * // and convert it using from_chars().
 * // Return the number of characters read for converting.
 * template<class T> size_t from_first_chars(csubstr buf, T * C4_RESTRICT val);
 * @endcode
 */


/** @ingroup lowlevel_tofrom_chars */
typedef enum {
    /** print the real number in floating point format (like %f) */
    FTOA_FLOAT = 0,
    /** print the real number in scientific format (like %e) */
    FTOA_SCIENT = 1,
    /** print the real number in flexible format (like %g) */
    FTOA_FLEX = 2,
    /** print the real number in hexadecimal format (like %a) */
    FTOA_HEXA = 3
} RealFormat_e;


inline C4_CONSTEXPR14 char to_c_fmt(RealFormat_e f)
{
    constexpr const char fmt[] = {
        'f',  // FTOA_FLOAT
        'e',  // FTOA_SCIENT
        'g',  // FTOA_FLEX
        'a',  // FTOA_HEXA
    };
    C4_ASSERT(f >= 0 && f < (typename std::underlying_type<RealFormat_e>::type)sizeof(fmt));
    return fmt[f];
}


#if C4CORE_HAVE_STD_TOCHARS
inline constexpr std::chars_format to_std_fmt(RealFormat_e f)
{
    constexpr const std::chars_format fmt[] = {
        std::chars_format::fixed,       // FTOA_FLOAT
        std::chars_format::scientific,  // FTOA_SCIENT
        std::chars_format::general,     // FTOA_FLEX
        std::chars_format::hex,         // FTOA_HEXA
    };
    C4_ASSERT(f >= 0 && f < (typename std::underlying_type<RealFormat_e>::type)sizeof(fmt));
    return fmt[f];
}
#endif // C4CORE_HAVE_STD_TOCHARS


// generic versions
template<class T> bool atox(csubstr s, T *v);
template<class T> size_t xtoa(substr s, T v);


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// Helper macros, undefined below

#define _c4append(c) { if(pos < buf.len) { buf.str[pos++] = (c); } else { ++pos; } }
#define _c4appendrdx(i) { if(pos < buf.len) { buf.str[pos++] = (radix == 16 ? hexchars[i] : (char)(i) + '0'); } else { ++pos; } }


/** convert an integral signed decimal to a string.
 * The resulting string is NOT zero-terminated.
 * Writing stops at the buffer's end.
 * @return the number of characters needed for the result, even if the buffer size is insufficient
 * @ingroup lowlevel_tofrom_chars */
template<class T>
size_t itoa(substr buf, T v)
{
    C4_STATIC_ASSERT(std::is_integral<T>::value);
    C4_STATIC_ASSERT(std::is_signed<T>::value);
    size_t pos = 0;
    if(v < 0)
    {
        _c4append('-');
        do {
            _c4append('0' - (v % 10));
            v /= 10;
        } while(v);
        if(buf.len > 0)
        {
            buf.reverse_range(1, pos <= buf.len ? pos : buf.len);
        }
    }
    else
    {
        do {
            _c4append('0' + (v % 10));
            v /= 10;
        } while(v);
        buf.reverse_range(0, pos <= buf.len ? pos : buf.len);
    }
    return pos;
}


/** convert an integral signed integer to a string, using a specific
 * radix. The radix must be 2, 8, 10 or 16.
 *
 * The resulting string is NOT zero-terminated.
 * Writing stops at the buffer's end.
 * @return the number of characters needed for the result, even if the buffer size is insufficient
 * @ingroup lowlevel_tofrom_chars */
template<class T>
size_t itoa(substr buf, T v, T radix)
{
    C4_STATIC_ASSERT(std::is_integral<T>::value);
    C4_STATIC_ASSERT(std::is_signed<T>::value);
    constexpr static const char hexchars[] = "0123456789abcdef";
    size_t pos = 0;

    // write the sign prefix
    if(v < 0)
    {
        v = -v;
        _c4append('-');
    }

    // write the radix prefix
    C4_ASSERT(radix == 2 || radix == 8 || radix == 10 || radix == 16);
    switch(radix)
    {
    case 2 : _c4append('0'); _c4append('b'); break;
    case 8 : _c4append('0');                 break;
    case 16: _c4append('0'); _c4append('x'); break;
    }

    // write the number
    size_t pfx = pos;
    do {
        _c4appendrdx(v % radix);
        v /= radix;
    } while(v);
    if(buf.len)
    {
        buf.reverse_range(pfx, pos <= buf.len ? pos : buf.len);
    }

    return pos;
}

//-----------------------------------------------------------------------------

/** convert an integral unsigned decimal to a string.
 * The resulting string is NOT zero-terminated.
 * Writing stops at the buffer's end.
 * @return the number of characters needed for the result, even if the buffer size is insufficient
 * @ingroup lowlevel_tofrom_chars */
template<class T>
size_t utoa(substr buf, T v)
{
    C4_STATIC_ASSERT(std::is_integral<T>::value);
    C4_STATIC_ASSERT(std::is_unsigned<T>::value);
    size_t pos = 0;
    do {
        _c4append((char)(v % 10) + '0');
        v /= 10;
    } while(v);
    buf.reverse_range(0, pos <= buf.len ? pos : buf.len);
    return pos;
}


/** convert an integral unsigned integer to a string, using a specific radix. The radix must be 2, 8, 10 or 16.
 * The resulting string is NOT zero-terminated.
 * Writing stops at the buffer's end.
 * @return the number of characters needed for the result, even if the buffer size is insufficient
 * @ingroup lowlevel_tofrom_chars */
template<class T>
size_t utoa(substr buf, T v, T radix)
{
    C4_STATIC_ASSERT(std::is_integral<T>::value);
    C4_STATIC_ASSERT(std::is_unsigned<T>::value);
    C4_ASSERT(radix == 2 || radix == 8 || radix == 10 || radix == 16);
    static constexpr const char hexchars[] = "0123456789abcdef";
    size_t pos = 0;

    // write the radix prefix
    switch(radix)
    {
    case 2 : _c4append('0'); _c4append('b'); break;
    case 8 : _c4append('0');                 break;
    case 16: _c4append('0'); _c4append('x'); break;
    }

    // write the number
    size_t pfx = pos;
    do {
        _c4appendrdx(v % radix);
        v /= radix;
    } while(v);
    if(buf.len)
    {
        buf.reverse_range(pfx, pos <= buf.len ? pos : buf.len);
    }

    return pos;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


/** Convert a trimmed string to a signed integral value. The value can be
 * formatted as decimal, binary (prefix 0b), octal (prefix 0)
 * or hexadecimal (prefix 0x). Every character in the input string is read
 * for the conversion; it must not contain any leading or trailing whitespace.
 * @return true if the conversion was successful.
 * @see atoi_first() if the string is not trimmed to the value to read.
 * @ingroup lowlevel_tofrom_chars
 */
template<class T>
bool atoi(csubstr str, T * C4_RESTRICT v)
{
    C4_STATIC_ASSERT(std::is_integral<T>::value);
    C4_STATIC_ASSERT(std::is_signed<T>::value);

    C4_ASSERT(str.len > 0);
    C4_ASSERT(str == str.first_int_span());

    T n = 0;
    T sign = 1;
    size_t start = 0;
    if(str[0] == '-')
    {
        ++start;
        sign = -1;
    }

    if(str.str[start] != '0')
    {
        for(size_t i = start; i < str.len; ++i)
        {
            char c = str.str[i];
            if(c < '0' || c > '9') return false;
            n = n*T(10) + (T(c)-T('0'));
        }
    }
    else
    {
        C4_ASSERT(str.len > start);
        if(str.len == start+1)
        {
            *v = 0; // because the first character is 0
            return true;
        }
        else if(str.str[start+1] == 'x' || str.str[start+1] == 'X') // hexadecimal
        {
            C4_ASSERT(str.len > 2);
            start += 2;
            for(size_t i = start; i < str.len; ++i)
            {
                char c = str.str[i];
                T cv;
                if(c >= '0' && c <= '9') cv = T(c) - T('0');
                else if(c >= 'a' && c <= 'f') cv = T(10) + (T(c)-T('a'));
                else if(c >= 'A' && c <= 'F') cv = T(10) + (T(c)-T('A'));
                else return false;
                n = n*T(16) + cv;
            }
        }
        else // octal
        {
            C4_ASSERT(str.len > 1);
            for(size_t i = start; i < str.len; ++i)
            {
                char c = str.str[i];
                if(c < '0' || c > '7') return false;
                n = n*T(8) + (T(c)-T('0'));
            }
        }
    }
    *v = sign * n;
    return true;
}


/** Select the next range of characters in the string that can be parsed
 * as a signed integral value, and convert it using atoi(). Leading
 * whitespace (space, newline, tabs) is skipped.
 * @return the number of characters read for conversion, or csubstr::npos if the conversion failed
 * @see atoi() if the string is already trimmed to the value to read.
 * @see csubstr::first_int_span()
 * @ingroup lowlevel_tofrom_chars
 */
template<class T>
inline size_t atoi_first(csubstr str, T * C4_RESTRICT v)
{
    csubstr trimmed = str.first_int_span();
    if(trimmed.len == 0) return csubstr::npos;
    if(atoi(trimmed, v)) return trimmed.end() - str.begin();
    return csubstr::npos;
}


//-----------------------------------------------------------------------------

/** Convert a trimmed string to an unsigned integral value. The value can be
 * formatted as decimal, binary (prefix 0b), octal (prefix 0)
 * or hexadecimal (prefix 0x). Every character in the input string is read
 * for the conversion; it must not contain any leading or trailing whitespace.
 * @return true if the conversion was successful.
 * @see atou_first() if the string is not trimmed to the value to read.
 * @ingroup lowlevel_tofrom_chars
 */
template<class T>
bool atou(csubstr str, T * C4_RESTRICT v)
{
    C4_STATIC_ASSERT(std::is_integral<T>::value);
    C4_ASSERT(str.len > 0);
    C4_ASSERT_MSG(str.str[0] != '-', "must be positive");
    C4_ASSERT(str == str.first_uint_span());

    T n = 0;

    if(str.str[0] != '0')
    {
        for(size_t i = 0; i < str.len; ++i)
        {
            char c = str.str[i];
            if(c < '0' || c > '9') return false;
            n = n*T(10) + (T(c)-T('0'));
        }
    }
    else
    {
        if(str.len == 1)
        {
            *v = 0; // because the first character is 0
            return true;
        }
        else if(str.str[1] == 'x' || str.str[1] == 'X') // hexadecimal
        {
            C4_ASSERT(str.len > 2);
            for(size_t i = 2; i < str.len; ++i)
            {
                char c = str.str[i];
                T cv;
                if(c >= '0' && c <= '9') cv = T(c) - T('0');
                else if(c >= 'a' && c <= 'f') cv = T(10) + (T(c)-T('a'));
                else if(c >= 'A' && c <= 'F') cv = T(10) + (T(c)-T('A'));
                else return false;
                n = n*T(16) + cv;
            }
        }
        else // octal
        {
            C4_ASSERT(str.len > 1);
            for(size_t i = 1; i < str.len; ++i)
            {
                char c = str.str[i];
                if(c < '0' || c > '7') return false;
                n = n*T(8) + (T(c)-T('0'));
            }
        }
    }
    *v = n;
    return true;
}


/** Select the next range of characters in the string that can be parsed
 * as an unsigned integral value, and convert it using atou(). Leading
 * whitespace (space, newline, tabs) is skipped.
 * @return the number of characters read for conversion, or csubstr::npos if the conversion faileds
 * @see atou() if the string is already trimmed to the value to read.
 * @see csubstr::first_uint_span()
 * @ingroup lowlevel_tofrom_chars
 */
template<class T>
inline size_t atou_first(csubstr str, T *v)
{
    csubstr trimmed = str.first_uint_span();
    if(trimmed.len == 0) return csubstr::npos;
    if(atou(trimmed, v)) return trimmed.end() - str.begin();
    return csubstr::npos;
}


//-----------------------------------------------------------------------------

namespace detail {


/** @see http://www.exploringbinary.com/ for many good examples on float-str conversion */
template<size_t N>
void get_real_format_str(char (& C4_RESTRICT fmt)[N], int precision, RealFormat_e formatting, const char* length_modifier="")
{
    int iret;
    if(precision == -1)
    {
        iret = snprintf(fmt, sizeof(fmt), "%%%s%c", length_modifier, to_c_fmt(formatting));
    }
    else if(precision == 0)
    {
        iret = snprintf(fmt, sizeof(fmt), "%%.%s%c", length_modifier, to_c_fmt(formatting));
    }
    else
    {
        iret = snprintf(fmt, sizeof(fmt), "%%.%d%s%c", precision, length_modifier, to_c_fmt(formatting));
    }
    C4_ASSERT(iret >= 2 && size_t(iret) < sizeof(fmt));
    C4_UNUSED(iret);
}


/** @todo we're depending on snprintf()/sscanf() for converting to/from
 * floating point numbers. Apparently, this increases the binary size
 * by a considerable amount. There are some lightweight printf
 * implementations:
 *
 * @see http://www.sparetimelabs.com/tinyprintf/tinyprintf.php (BSD)
 * @see https://github.com/weiss/c99-snprintf
 * @see https://github.com/nothings/stb/blob/master/stb_sprintf.h
 * @see http://www.exploringbinary.com/
 * @see https://blog.benoitblanchon.fr/lightweight-float-to-string/
 * @see http://www.ryanjuckett.com/programming/printing-floating-point-numbers/
 */
template<class T>
size_t print_one(substr str, const char* full_fmt, T v)
{
#ifdef _MSC_VER
    /** use _snprintf() to prevent early termination of the output
     * for writing the null character at the last position
     * @see https://msdn.microsoft.com/en-us/library/2ts7cx93.aspx */
    int iret = _snprintf(str.str, str.len, full_fmt, v);
    if(iret < 0)
    {
        /* when buf.len is not enough, VS returns a negative value.
         * so call it again with a negative value for getting an
         * actual length of the string */
        iret = snprintf(nullptr, 0, full_fmt, v);
        C4_ASSERT(iret > 0);
    }
    size_t ret = (size_t) iret;
    return ret;
#else
    int iret = snprintf(str.str, str.len, full_fmt, v);
    C4_ASSERT(iret >= 0);
    size_t ret = (size_t) iret;
    if(ret >= str.len)
    {
        ++ret; /* snprintf() reserves the last character to write \0 */
    }
    return ret;
#endif
}

template<class T> struct real_buf;
template<> struct real_buf<float> { using type = uint32_t; };
template<> struct real_buf<double> { using type = uint64_t; };

template<class T>
struct real_type_info
{
    C4_STATIC_ASSERT(std::is_floating_point<T>::value);
    using itype = typename real_buf<T>::type;
    itype buf = {};

    constexpr static inline int get_exponent_bits()
    {
        using nl = std::numeric_limits<T>;
        int range = nl::max_exponent - nl::min_exponent;
        int bits = 0;
        while ((range >> bits) > 0) ++bits;
        return bits;
    }

    constexpr static inline itype get_mask(int start, int end)
    {
        itype r = 0;
        constexpr const itype o = 1;
        for(int i = start; i < end; ++i) r |= (o << i);
        return r;
    }

    enum : int {
        num_bits  = sizeof(T) * CHAR_BIT,
        num_frac_bits = std::numeric_limits<T>::digits - 1,
        num_exp_bits = get_exponent_bits(),
        frac_start = 0, frac_end = num_frac_bits,
        exp_start = frac_end, exp_end = exp_start + num_exp_bits,
        sign_bit = num_bits - 1,
    };
    enum : itype {
        one = 1,
        frac_mask = get_mask(frac_start, frac_end),
        exp_mask = get_mask(exp_start, exp_end),
    };
};

template<class T>
inline size_t scan_one_real(csubstr str, T *v)
{
    C4_STATIC_ASSERT(std::is_floating_point<T>::value);
    C4_ASSERT(str.len > 0);
    C4_ASSERT(str == str.first_real_span());

    using rtype = real_type_info<T>;
    using itype = typename real_type_info<T>::itype;

    size_t pos = 0; // the current buffer position
    rtype r; // the result, initialized to zero
    if(str[0] == '-')
    {
        r.buf |= (rtype::one << rtype::sign_bit);
        ++pos;
    }
    else if(str[0] == '+')
    {
        // no need to clear the sign bit
        ++pos; // other than counting the position
    }

    if(str.str[pos] == '0')
    {
        C4_ASSERT(str.len > pos);
        if(str.len == pos+1)
        {
            *v = T(0);
            return pos+1;
        }
        else if(str.str[pos + 1] == 'x' || str.str[pos + 1] == 'X')
        {   // hexadecimal
            C4_ASSERT(str.len > 2);
            pos += 2;
            return pos;
        }
        while(str.str[++pos] == '0') {;} // skip leading zeroes
        if(str.len == pos+1)
        {
            *v = T(0);
            return pos+1;
        }
    }

    csubstr rem = str.sub(pos);
    size_t exp_pos = rem.first_of_any("e", "E").pos;
    csubstr exponent = rem.right_of(exp_pos);
    csubstr mantissa = rem.left_of(exp_pos);
    size_t dot_pos = mantissa.first_of('.');
    csubstr integral = mantissa.left_of(dot_pos);
    csubstr fractional = mantissa.right_of(dot_pos);

    itype integral_v = 0;
    itype fractional_v = 0;
    itype exponent_v = 0;
    for(char c : integral)
    {
        C4_ASSERT(c >= '0' && c <= '9');
        integral_v = integral_v * itype(10) + (itype(c) - itype('0'));
    }
    for(char c : fractional)
    {
        C4_ASSERT(c >= '0' && c <= '9');
        fractional_v = fractional_v * itype(10) + (itype(c) - itype('0'));
    }

    C4_NOT_IMPLEMENTED();
    return pos;
}


template<typename T> constexpr        const char* get_length_modifier();
template<>           constexpr inline const char* get_length_modifier<float>() { return ""; }
template<>           constexpr inline const char* get_length_modifier<double>() { return "l"; }

/** scans a string using the given type format, while at the same time
 * allowing non-null-terminated strings AND guaranteeing that the given
 * string length is strictly respected, so that no buffer overflows
 * might occur. */
template<typename T>
inline size_t scan_one(csubstr str, const char *type_fmt, T *v)
{
    /* snscanf() is absolutely needed here as we must be sure that
     * str.len is strictly respected, because substr is
     * generally not null-terminated.
     *
     * Alas, there is no snscanf().
     *
     * So we fake it by using a dynamic format with an explicit
     * field size set to the length of the given span.
     * This trick is taken from:
     * https://stackoverflow.com/a/18368910/5875572 */

    /* this is the actual format we'll use for scanning */
    char fmt[16];

    /* write the length into it. Eg "%12f".
     * Also, get the number of characters read from the string.
     * So the final format ends up as "%12f%n"*/
    int iret = snprintf(fmt, sizeof(fmt), "%%" "%zu" "%s" "%%n", str.len, type_fmt);
    /* no nasty surprises, please! */
    C4_ASSERT(iret >= 0 && size_t(iret) < sizeof(fmt));

    /* now we scan with confidence that the span length is respected */
    int num_chars;
    iret = sscanf(str.str, fmt, v, &num_chars);
    /* scanf returns the number of successful conversions */
    if(iret != 1) return csubstr::npos;
    C4_ASSERT(num_chars >= 0);
    return (size_t)(num_chars);
}

} // namespace detail


#if C4CORE_HAVE_STD_TOCHARS
template<class T>
size_t rtoa(substr buf, T v, int precision=-1, RealFormat_e formatting=FTOA_FLEX)
{
    std::to_chars_result result;
    size_t pos = 0;
    if(formatting == FTOA_HEXA)
    {
        _c4append('0');
        _c4append('x');
    }
    if(precision == -1)
    {
        result = std::to_chars(buf.str + pos, buf.str + buf.len, v, to_std_fmt(formatting));
    }
    else
    {
        result = std::to_chars(buf.str + pos, buf.str + buf.len, v, to_std_fmt(formatting), precision);
    }
    if(result.ec == std::errc())
    {
        // all good, no errors.
        C4_ASSERT(result.ptr >= buf.str);
        ptrdiff_t delta = result.ptr - buf.str;
        return static_cast<size_t>(delta);
    }
    C4_ASSERT(result.ec == std::errc::value_too_large);
    // This is unfortunate.
    //
    // When the result can't fit in the given buffer,
    // std::to_chars() returns the end pointer it was originally
    // given, which is useless because here we want to known
    // _exactly_ how many characters the buffer must have to fit
    // the result.
    //
    // So we fall back on printf in this case.
    char fmt[16];
    detail::get_real_format_str(fmt, precision, formatting, detail::get_length_modifier<T>());
    size_t ret = detail::print_one(buf, fmt, v);
    return ret > buf.len ? ret : buf.len + 1;
}
#endif // C4CORE_HAVE_STD_TOCHARS


#undef _c4appendrdx
#undef _c4append


/** Convert a single precision real number to string.
 * The string will in general be NOT null-terminated.
 * For FTOA_FLEX, \p precision is the number of significand digits. Otherwise
 * \p precision is the number of decimals.
 * @ingroup lowlevel_tofrom_chars
 */
inline size_t ftoa(substr str, float v, int precision=-1, RealFormat_e formatting=FTOA_FLEX)
{
#if C4CORE_HAVE_STD_TOCHARS
    return rtoa(str, v, precision, formatting);
#else
    char fmt[16];
    detail::get_real_format_str(fmt, precision, formatting, /*length_modifier*/"");
    return detail::print_one(str, fmt, v);
#endif
}


/** Convert a double precision real number to string.
 * The string will in general be NOT null-terminated.
 * For FTOA_FLEX, \p precision is the number of significand digits. Otherwise
 * \p precision is the number of decimals.
 * @return the number of characters written.
 * @ingroup lowlevel_tofrom_chars
 */
inline size_t dtoa(substr str, double v, int precision=-1, RealFormat_e formatting=FTOA_FLEX)
{
#if C4CORE_HAVE_STD_TOCHARS
    return rtoa(str, v, precision, formatting);
#else
    char fmt[16];
    detail::get_real_format_str(fmt, precision, formatting, /*length_modifier*/"l");
    return detail::print_one(str, fmt, v);
#endif
}


/** Convert a string to a single precision real number.
 * The input string must be trimmed to the value, ie
 * no leading or trailing whitespace can be present.
 * @return true iff the conversion succeeded
 * @ingroup lowlevel_tofrom_chars
 * @see atof_first() if the string is not trimmed
 */
inline bool atof(csubstr str, float * C4_RESTRICT v)
{
    C4_ASSERT(str == str.first_real_span());
#if C4CORE_HAVE_STD_TOCHARS
    std::from_chars_result result;
    result = std::from_chars(str.str, str.str + str.len, *v);
    return result.ec == std::errc();
#else
    size_t ret = detail::scan_one(str, "g", v);
    return ret != csubstr::npos;
#endif
}


/** Convert a string to a double precision real number.
 * The input string must be trimmed to the value, ie
 * no leading or trailing whitespace can be present.
 * @return true iff the conversion succeeded
 * @ingroup lowlevel_tofrom_chars
 * @see atod_first() if the string is not trimmed
 */
inline bool atod(csubstr str, double * C4_RESTRICT v)
{
    C4_ASSERT(str == str.first_real_span());
#if C4CORE_HAVE_STD_TOCHARS
    std::from_chars_result result;
    result = std::from_chars(str.str, str.str + str.len, *v);
    return result.ec == std::errc();
#else
    size_t ret = detail::scan_one(str, "lg", v);
    return ret != csubstr::npos;
#endif
}


/** Convert a string to a single precision real number.
 * Leading whitespace is skipped until valid characters are found.
 * @return true iff the conversion succeeded
 * @ingroup lowlevel_tofrom_chars
 */
inline size_t atof_first(csubstr str, float * C4_RESTRICT v)
{
    csubstr trimmed = str.first_real_span();
    if(trimmed.len == 0) return csubstr::npos;
    if(atof(trimmed, v)) return trimmed.end() - str.begin();
    return csubstr::npos;
}


/** Convert a string to a double precision real number.
 * Leading whitespace is skipped until valid characters are found.
 * @return true iff the conversion succeeded
 * @ingroup lowlevel_tofrom_chars
 */
inline size_t atod_first(csubstr str, double * C4_RESTRICT v)
{
    csubstr trimmed = str.first_real_span();
    if(trimmed.len == 0) return csubstr::npos;
    if(atod(trimmed, v)) return trimmed.end() - str.begin();
    return csubstr::npos;
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// generic versions
C4_ALWAYS_INLINE bool atox(csubstr s,  uint8_t *v) { return atou(s, v); }
C4_ALWAYS_INLINE bool atox(csubstr s,   int8_t *v) { return atoi(s, v); }
C4_ALWAYS_INLINE bool atox(csubstr s, uint16_t *v) { return atou(s, v); }
C4_ALWAYS_INLINE bool atox(csubstr s,  int16_t *v) { return atoi(s, v); }
C4_ALWAYS_INLINE bool atox(csubstr s, uint32_t *v) { return atou(s, v); }
C4_ALWAYS_INLINE bool atox(csubstr s,  int32_t *v) { return atoi(s, v); }
C4_ALWAYS_INLINE bool atox(csubstr s, uint64_t *v) { return atou(s, v); }
C4_ALWAYS_INLINE bool atox(csubstr s,  int64_t *v) { return atoi(s, v); }
C4_ALWAYS_INLINE bool atox(csubstr s,    float *v) { return atof(s, v); }
C4_ALWAYS_INLINE bool atox(csubstr s,   double *v) { return atod(s, v); }

C4_ALWAYS_INLINE size_t xtoa(substr s,  uint8_t v) { return utoa(s, v); }
C4_ALWAYS_INLINE size_t xtoa(substr s,   int8_t v) { return itoa(s, v); }
C4_ALWAYS_INLINE size_t xtoa(substr s, uint16_t v) { return utoa(s, v); }
C4_ALWAYS_INLINE size_t xtoa(substr s,  int16_t v) { return itoa(s, v); }
C4_ALWAYS_INLINE size_t xtoa(substr s, uint32_t v) { return utoa(s, v); }
C4_ALWAYS_INLINE size_t xtoa(substr s,  int32_t v) { return itoa(s, v); }
C4_ALWAYS_INLINE size_t xtoa(substr s, uint64_t v) { return utoa(s, v); }
C4_ALWAYS_INLINE size_t xtoa(substr s,  int64_t v) { return itoa(s, v); }
C4_ALWAYS_INLINE size_t xtoa(substr s,    float v) { return ftoa(s, v); }
C4_ALWAYS_INLINE size_t xtoa(substr s,   double v) { return dtoa(s, v); }


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define _C4_DEFINE_TO_FROM_CHARS_TOA(ty, id)                    \
                                                                \
/** @ingroup generic_tofrom_chars */                            \
inline size_t to_chars(substr buf, ty v)                        \
{                                                               \
    return id##toa(buf, v);                                     \
}                                                               \
                                                                \
/** @ingroup generic_tofrom_chars */                            \
inline bool from_chars(csubstr buf, ty *C4_RESTRICT v)          \
{                                                               \
    return ato##id(buf, v);                                     \
}                                                               \
                                                                \
/** @ingroup generic_tofrom_chars */                            \
inline size_t from_chars_first(csubstr buf, ty *C4_RESTRICT v)  \
{                                                               \
    return ato##id##_first(buf, v);                             \
}

#ifdef _MSC_VER

#define _C4_DEFINE_TO_CHARS(ty, pri_fmt)                                \
/** @ingroup generic_tofrom_chars */                                    \
inline size_t to_chars(substr buf, ty v)                                \
{                                                                       \
    /** use _snprintf() to prevent early termination of the output      \
     * for writing the null character at the last position              \
     * @see https://msdn.microsoft.com/en-us/library/2ts7cx93.aspx */   \
    int iret = _snprintf(buf.str, buf.len, "%" pri_fmt, v);             \
    if(iret < 0)                                                        \
    {                                                                   \
        /* when buf.len is not enough, VS returns a negative value.     \
         * so call it again with a negative value for getting an        \
         * actual length of the string */                               \
        iret = snprintf(nullptr, 0, "%" pri_fmt, v);                    \
        C4_ASSERT(iret > 0);                                            \
    }                                                                   \
    size_t ret = (size_t) iret;                                         \
    return ret;                                                         \
}

#else // not _MSC_VER

#define _C4_DEFINE_TO_CHARS(ty, pri_fmt)                                \
/** @ingroup generic_tofrom_chars */                                    \
inline size_t to_chars(substr buf, ty v)                                \
{                                                                       \
    int iret = snprintf(buf.str, buf.len, "%" pri_fmt, v);              \
    C4_ASSERT(iret >= 0);                                               \
    size_t ret = (size_t) iret;                                         \
    if(ret >= buf.len)                                                  \
    {                                                                   \
        ++ret; /* snprintf() reserves the last character to write \0 */ \
    }                                                                   \
    return ret;                                                         \
}

#endif


/** this macro defines to_chars()/from_chars() pairs for intrinsic types. */ \
#define _C4_DEFINE_TO_FROM_CHARS(ty, pri_fmt, scn_fmt)                  \
                                                                        \
_C4_DEFINE_TO_CHARS(ty, pri_fmt)                                        \
                                                                        \
/** @ingroup generic_tofrom_chars */                                    \
inline size_t from_chars_first(csubstr buf, ty * C4_RESTRICT v)         \
{                                                                       \
    /* snscanf() is absolutely needed here as we must be sure that      \
     * buf.len is strictly respected, because the span string is        \
     * generally not null-terminated.                                   \
     *                                                                  \
     * Alas, there is no snscanf().                                     \
     *                                                                  \
     * So we fake it by using a dynamic format with an explicit         \
     * field size set to the length of the given span.                  \
     * This trick is taken from:                                        \
     * https://stackoverflow.com/a/18368910/5875572 */                  \
                                                                        \
    /* this is the actual format we'll use for scanning */              \
    char fmt[12];                                                       \
    /* write the length into it. Eg "%12d" for an int (scn_fmt="d").    \
     * Also, get the number of characters read from the string.         \
     * So the final format ends up as "%12d%n"*/                        \
    int ret = snprintf(fmt, sizeof(fmt), "%%""%zu" scn_fmt "%%n", buf.len); \
    /* no nasty surprises, please! */                                   \
    C4_ASSERT(size_t(ret) < sizeof(fmt));                               \
    /* now we scan with confidence that the span length is respected */ \
    int num_chars;                                                      \
    ret = sscanf(buf.str, fmt, v, &num_chars);                          \
    /* scanf returns the number of successful conversions */            \
    if(ret != 1) return csubstr::npos;                                  \
    return (size_t)(num_chars);                                         \
}                                                                       \
                                                                        \
/** @ingroup generic_tofrom_chars */                                    \
inline bool from_chars(csubstr buf, ty * C4_RESTRICT v)                 \
{                                                                       \
    size_t num = from_chars_first(buf, v);                              \
    return (num != csubstr::npos);                                      \
}

//_C4_DEFINE_TO_FROM_CHARS(double  , "lg"            , "lg"            )
//_C4_DEFINE_TO_FROM_CHARS(float   , "g"             , "g"             )
//_C4_DEFINE_TO_FROM_CHARS(char    , "c"             , "c"             )
//_C4_DEFINE_TO_FROM_CHARS(  int8_t, PRId8 /*"%hhd"*/, SCNd8 /*"%hhd"*/)
//_C4_DEFINE_TO_FROM_CHARS( uint8_t, PRIu8 /*"%hhu"*/, SCNu8 /*"%hhu"*/)
//_C4_DEFINE_TO_FROM_CHARS( int16_t, PRId16/*"%hd" */, SCNd16/*"%hd" */)
//_C4_DEFINE_TO_FROM_CHARS(uint16_t, PRIu16/*"%hu" */, SCNu16/*"%hu" */)
//_C4_DEFINE_TO_FROM_CHARS( int32_t, PRId32/*"%d"  */, SCNd32/*"%d"  */)
//_C4_DEFINE_TO_FROM_CHARS(uint32_t, PRIu32/*"%u"  */, SCNu32/*"%u"  */)
//_C4_DEFINE_TO_FROM_CHARS( int64_t, PRId64/*"%lld"*/, SCNd64/*"%lld"*/)
//_C4_DEFINE_TO_FROM_CHARS(uint64_t, PRIu64/*"%llu"*/, SCNu64/*"%llu"*/)
_C4_DEFINE_TO_FROM_CHARS(void*   , "p"             , "p"             )
_C4_DEFINE_TO_FROM_CHARS_TOA(   float, f)
_C4_DEFINE_TO_FROM_CHARS_TOA(  double, d)
_C4_DEFINE_TO_FROM_CHARS_TOA(  int8_t, i)
_C4_DEFINE_TO_FROM_CHARS_TOA( int16_t, i)
_C4_DEFINE_TO_FROM_CHARS_TOA( int32_t, i)
_C4_DEFINE_TO_FROM_CHARS_TOA( int64_t, i)
_C4_DEFINE_TO_FROM_CHARS_TOA( uint8_t, u)
_C4_DEFINE_TO_FROM_CHARS_TOA(uint16_t, u)
_C4_DEFINE_TO_FROM_CHARS_TOA(uint32_t, u)
_C4_DEFINE_TO_FROM_CHARS_TOA(uint64_t, u)

#undef _C4_DEFINE_TO_FROM_CHARS
#undef _C4_DEFINE_TO_FROM_CHARS_TOA


//-----------------------------------------------------------------------------
/** call to_chars() and return a substr consisting of the
 * written portion of the input buffer. Ie, same as to_chars(),
 * but return a substr instead of a size_t.
 *
 * @see to_chars()
 * @ingroup generic_tofrom_string */
template<class T>
inline substr to_chars_sub(substr buf, T const& C4_RESTRICT v)
{
    size_t sz = to_chars(buf, v);
    return buf.left_of(sz <= buf.len ? sz : buf.len);
}

//-----------------------------------------------------------------------------
// bool implementation

/** @ingroup generic_tofrom_chars */
inline size_t to_chars(substr buf, bool v)
{
    int val = v;
    return to_chars(buf, val);
}

/** @ingroup generic_tofrom_chars */
inline bool from_chars(csubstr buf, bool * C4_RESTRICT v)
{
    int val = 0;
    bool ret = from_chars(buf, &val);
    *v = (val != 0);
    return ret;
}

/** @ingroup generic_tofrom_chars */
inline size_t from_chars_first(csubstr buf, bool * C4_RESTRICT v)
{
    int val = 0;
    size_t ret = from_chars_first(buf, &val);
    *v = (val != 0);
    return ret;
}


//-----------------------------------------------------------------------------
// single-char implementation

/** @ingroup generic_tofrom_chars */
inline size_t to_chars(substr buf, char v)
{
    if(buf.len > 0) buf[0] = v;
    return 1;
}

/** extract a single character from a substring
 * @note to extract a string instead and not just a single character, use the csubstr overload
 * @ingroup generic_tofrom_chars */
inline bool from_chars(csubstr buf, char * C4_RESTRICT v)
{
    if(buf.len != 1) return false;
    *v = buf[0];
    return true;
}

/** @ingroup generic_tofrom_chars */
inline size_t from_chars_first(csubstr buf, char * C4_RESTRICT v)
{
    if(buf.len < 1) return csubstr::npos;
    *v = buf[0];
    return 1;
}


//-----------------------------------------------------------------------------
// csubstr implementation

/** @ingroup generic_tofrom_chars */
inline size_t to_chars(substr buf, csubstr v)
{
    C4_ASSERT(!buf.contains(v) && !v.contains(buf));
    size_t len = buf.len < v.len ? buf.len : v.len;
    memcpy(buf.str, v.str, len);
    return v.len;
}

/** @ingroup generic_tofrom_chars */
inline bool from_chars(csubstr buf, csubstr *C4_RESTRICT v)
{
    *v = buf;
    return true;
}

/** @ingroup generic_tofrom_chars */
inline size_t from_chars_first(substr buf, csubstr * C4_RESTRICT v)
{
    csubstr trimmed = buf.first_non_empty_span();
    if(trimmed.len == 0) return csubstr::npos;
    *v = trimmed;
    return trimmed.end() - buf.begin();
}


//-----------------------------------------------------------------------------
// substr

/** @ingroup generic_tofrom_chars */
inline size_t to_chars(substr buf, substr v)
{
    C4_ASSERT(!buf.overlaps(v));
    size_t len = buf.len < v.len ? buf.len : v.len;
    memcpy(buf.str, v.str, len);
    return v.len;
}

/** @ingroup generic_tofrom_chars */
inline bool from_chars(csubstr buf, substr * C4_RESTRICT v)
{
    C4_ASSERT(!buf.overlaps(*v));
    bool ok = buf.len <= v->len;
    if(ok)
    {
        memcpy(v->str, buf.str, buf.len);
        v->len = buf.len;
        return true;
    }
    memcpy(v->str, buf.str, buf.len);
    return false;
}

/** @ingroup generic_tofrom_chars */
inline size_t from_chars_first(csubstr buf, substr * C4_RESTRICT v)
{
    csubstr trimmed = buf.first_non_empty_span();
    C4_ASSERT(!trimmed.overlaps(*v));
    if(C4_UNLIKELY(trimmed.len == 0)) return csubstr::npos;
    size_t len = trimmed.len > v->len ? v->len : trimmed.len;
    memcpy(v->str, trimmed.str, len);
    if(C4_UNLIKELY(trimmed.len > v->len)) return csubstr::npos;
    return trimmed.end() - buf.begin();
}


//-----------------------------------------------------------------------------

/** @ingroup generic_tofrom_chars */
template<size_t N>
inline size_t to_chars(substr buf, const char (& C4_RESTRICT v)[N])
{
    csubstr sp(v);
    return to_chars(buf, sp);
}

/** @ingroup generic_tofrom_chars */
inline size_t to_chars(substr buf, const char * C4_RESTRICT v)
{
    return to_chars(buf, to_csubstr(v));
}


} // namespace c4

#ifdef _MSC_VER
#   pragma warning(pop)
#endif

#endif /* _C4_CHARCONV_HPP_ */
