
#ifndef PRINTF_20160922_H_
#define PRINTF_20160922_H_

#include "Formatters.h"

#include <algorithm>
#include <cassert>
#include <cstring>

#define CXX11_PRINT_EXTENSIONS

namespace cxx11 {
namespace detail {

enum class Modifiers {
	MOD_NONE,
	MOD_CHAR,
	MOD_SHORT,
	MOD_LONG,
	MOD_LONG_LONG,
	MOD_LONG_DOUBLE,
	MOD_INTMAX_T,
	MOD_SIZE_T,
	MOD_PTRDIFF_T
};

struct Flags {
	uint8_t justify  : 1;
	uint8_t sign     : 1;
	uint8_t space    : 1;
	uint8_t prefix   : 1;
	uint8_t padding  : 1;
	uint8_t reserved : 3;
};

static_assert(sizeof(Flags) == sizeof(uint8_t), "");

//------------------------------------------------------------------------------
// Name: itoa
// Desc: returns the value of d as a C-string, formatted based on Base, Divisor,
//       and flagsplaces the length of the resultant string in *rlen
//------------------------------------------------------------------------------
template <char Base, unsigned int Divisor, class T>
const char *itoa_internal(char *buf, int precision, T d, int width, Flags flags, const char *alphabet, size_t *rlen) {

	const char *const buf_ptr               = buf;
	char *p                                 = buf;
	typename std::make_unsigned<T>::type ud = d;

	// If %d is specified and d is negative, put `-' in the head.
	switch(Base) {
	case 'd':
	case 'i':
		if(d < 0) {
			*p++ = '-';
			ud = -d;
			width -= 1;
		} else if(flags.space) {
			*p++ = ' ';
			width -= 1;
		} else if(flags.sign) {
			*p++ = '+';
			width -= 1;
		}
		break;

	case 'X':
	case 'x':
		if(flags.prefix) {
			*p++ = '0';
			*p++ = Base;
			width -= 2;
		}
		break;

	case 'o':
		if(flags.prefix) {
			*p++ = '0';
			width -= 1;
		}
		break;

	default:
		break;
	}

	// this is the point we will start reversing the string at after conversion
	buf = p;

	// Divide UD by Divisor until UD == 0.
	do {
		const int remainder = (ud % Divisor);
		*p++ = alphabet[remainder];
		if(width > 0) {
			--width;
		}
	} while (ud /= Divisor);

	while(flags.padding && width > 0) {
		*p++ = '0';
		--width;
	}

	if(precision > (p - buf)) {
		precision -= (p - buf);
		while(precision--) {
			*p++ = '0';
		}
	}

	// terminate buffer 
	*p = '\0';
	
	*rlen = (p - buf_ptr);

	std::reverse(buf, p);

	return buf_ptr;
}

//------------------------------------------------------------------------------
// Name: itoa
// Desc: as a minor optimization, let's determine a few things up front and pass 
//       them as template parameters enabling some more aggressive optimizations
//       when the division can use more efficient operations
//------------------------------------------------------------------------------
template <class T>
const char *itoa(char *buf, char base, int precision, T d, int width, Flags flags, size_t *rlen) {

	if(d == 0 && precision == 0) {
		*buf = '\0';
		return buf;
	}
	
	static const char alphabet_l[] = "0123456789abcdef";
	static const char alphabet_u[] = "0123456789ABCDEF";
	
	switch(base) {
	case 'i':
		return itoa_internal<'i', 10>(buf, precision, d, width, flags, alphabet_l, rlen);
	case 'd':
		return itoa_internal<'d', 10>(buf, precision, d, width, flags, alphabet_l, rlen);
	case 'u':
		return itoa_internal<'u', 10>(buf, precision, d, width, flags, alphabet_l, rlen);
#ifdef CXX11_PRINT_EXTENSIONS
	case 'b':
		return itoa_internal<'B', 2>(buf, precision, d, width, flags, alphabet_l, rlen);
#endif
	case 'X':
		return itoa_internal<'X', 16>(buf, precision, d, width, flags, alphabet_u, rlen);
	case 'x':
		return itoa_internal<'x', 16>(buf, precision, d, width, flags, alphabet_l, rlen);
	case 'o':
		return itoa_internal<'o', 8>(buf, precision, d, width, flags, alphabet_l, rlen);
	default:
		return itoa_internal<'d', 10>(buf, precision, d, width, flags, alphabet_l, rlen);
	}
}

//------------------------------------------------------------------------------
// Name: output_string
// Desc: prints a string to the Context object, taking into account padding flags
// Note: ch is the current format specifier
//------------------------------------------------------------------------------
template <class Context>
void output_string(char ch, const char *s_ptr, int precision, long int width, Flags flags, int len, Context &ctx) {

	if((ch == 's' && precision >= 0 && precision < len)) {
		len = precision;
	}

	// if not left justified padding goes first...
	if(!flags.justify) {
		// spaces go before the prefix...
		while(width > len) {
			ctx.write(' ');
			--width;
		}
	}

	// output the string
	// NOTE(eteran): len is at most strlen, possible is less
	// so we can just loop len times
	width -= len;
	while(len--) {
		ctx.write(*s_ptr++);
	}
	

	// if left justified padding goes last...
	if(flags.justify) {
		while(width > 0) {
			ctx.write(' ');
			--width;
		}
	}
}

// NOTE(eteran): Here is some code to fetch arguments of specific types. We also need a few 
//               default handlers, this code should never really be encountered, but
//               but we need it to keep the linker happy.

template <class T>
const char *formatted_string(T s, typename std::enable_if<std::is_convertible<T, const char *>::value>::type* = 0) {
	return s;
}

template <class T>
const char *formatted_string(T s, typename std::enable_if<!std::is_convertible<T, const char *>::value>::type* = 0) {
	(void)s;
	assert(!"Non-String Argument For String Format");
	return nullptr;
}

template <class R, class T>
R formatted_pointer(T p, typename std::enable_if<std::is_convertible<T, const void *>::value>::type* = 0) {
	return reinterpret_cast<R>(reinterpret_cast<uintptr_t>(p));
}

template <class R, class T>
R formatted_pointer(T p, typename std::enable_if<!std::is_convertible<T, const void *>::value>::type* = 0) {
	(void)p;
	assert(!"Non-Pointer Argument For Pointer Format");
	return 0;
}

template <class R, class T>
R formatted_integer(T n, typename std::enable_if<std::is_integral<T>::value>::type* = 0) {
	return static_cast<R>(n);
}

template <class R, class T>
R formatted_integer(T n, typename std::enable_if<!std::is_integral<T>::value>::type* = 0) {
	(void)n;
	assert(!"Non-Integer Argument For Integer Format");
	return 0;
}


//------------------------------------------------------------------------------
// Name: process_format
// Desc: default handler that should never be called at runtime
//------------------------------------------------------------------------------
template <class Context>
int process_format(Context &ctx, const char *format, Flags flags, long int width, long int precision, Modifiers modifier) {
	(void)ctx;
	(void)format;
	(void)flags;
	(void)width;
	(void)precision;
	(void)modifier;
	assert(!"Should Never Happen");
	return 0;
}

//------------------------------------------------------------------------------
// Name: get_modifier
// Desc: default handler that should never be called at runtime
//------------------------------------------------------------------------------
template <class Context>
int get_modifier(Context &ctx, const char *format, Flags flags, long int width, long int precision) {
	(void)ctx;
	(void)format;
	(void)flags;
	(void)width;
	(void)precision;
	assert(!"Should Never Happen");
	return 0;
}

//------------------------------------------------------------------------------
// Name: get_precision
// Desc: default handler that should never be called at runtime
//------------------------------------------------------------------------------
template <class Context>
int get_precision(Context &ctx, const char *format, Flags flags, long int width) {
	(void)ctx;
	(void)format;
	(void)flags;
	(void)width;
	assert(!"Should Never Happen");
	return 0;
}

//------------------------------------------------------------------------------
// Name: process_format
// Desc: prints the next argument to the Context taking into account the flags, 
//       width, precision, and modifiers collected along the way. Then will 
//       recursively continue processing the string
//------------------------------------------------------------------------------
template <class Context, class T, class... Ts>
int process_format(Context &ctx, const char *format, Flags flags, long int width, long int precision, Modifiers modifier, const T &arg, const Ts &... ts) {

	// enough to contain a 64-bit number in bin notation
	char num_buf[65];
	
	size_t slen;
	const char *s_ptr  = nullptr;
	
	char ch = *format;
	switch(ch) {
	case 'e':
	case 'E':
	case 'f':
	case 'F':
	case 'a':
	case 'A':
	case 'g':
	case 'G':
		// TODO(eteran): implement float formatting... for now, just consume the argument
		return Printf(ctx, format + 1, ts...);

	case 'p':
		precision    = 1;
		ch           = 'x';
		flags.prefix = 1;
		// NOTE(eteran): GNU printf prints "(nil)" for NULL pointers, we print 0x0
		s_ptr        = itoa(num_buf, ch, precision, formatted_pointer<uintptr_t>(arg), width, flags, &slen);

		output_string(ch, s_ptr, precision, width, flags, slen, ctx);
		return Printf(ctx, format + 1, ts...);

	case 'x':
	case 'X':
	case 'u':
	case 'o':
#ifdef CXX11_PRINT_EXTENSIONS
	case 'b': // extension, BINARY mode
#endif
		if(precision < 0) {
			precision = 1;
		}

		switch(modifier) {
		case Modifiers::MOD_CHAR:
			s_ptr = itoa(num_buf, ch, precision, formatted_integer<unsigned char>(arg), width, flags, &slen);
			break;
		case Modifiers::MOD_SHORT:
			s_ptr = itoa(num_buf, ch, precision, formatted_integer<unsigned short int>(arg), width, flags, &slen);
			break;
		case Modifiers::MOD_LONG:
			s_ptr = itoa(num_buf, ch, precision, formatted_integer<unsigned long int>(arg), width, flags, &slen);
			break;
		case Modifiers::MOD_LONG_LONG:
			s_ptr = itoa(num_buf, ch, precision, formatted_integer<unsigned long long int>(arg), width, flags, &slen);
			break;
		case Modifiers::MOD_INTMAX_T:
			s_ptr = itoa(num_buf, ch, precision, formatted_integer<uintmax_t>(arg), width, flags, &slen);
			break;
		case Modifiers::MOD_SIZE_T:
			s_ptr = itoa(num_buf, ch, precision, formatted_integer<size_t>(arg), width, flags, &slen);
			break;
		case Modifiers::MOD_PTRDIFF_T:
			s_ptr = itoa(num_buf, ch, precision, formatted_integer<std::make_unsigned<ptrdiff_t>::type>(arg), width, flags, &slen);
			break;
		default:
			s_ptr = itoa(num_buf, ch, precision, formatted_integer<unsigned int>(arg), width, flags, &slen);
			break;
		}

		output_string(ch, s_ptr, precision, width, flags, slen, ctx);
		return Printf(ctx, format + 1, ts...);

	case 'i':
	case 'd':
		if(precision < 0) {
			precision = 1;
		}

		switch(modifier) {
		case Modifiers::MOD_CHAR:
			s_ptr = itoa(num_buf, ch, precision, formatted_integer<signed char>(arg), width, flags, &slen);
			break;
		case Modifiers::MOD_SHORT:
			s_ptr = itoa(num_buf, ch, precision, formatted_integer<short int>(arg), width, flags, &slen);
			break;
		case Modifiers::MOD_LONG:
			s_ptr = itoa(num_buf, ch, precision, formatted_integer<long int>(arg), width, flags, &slen);
			break;
		case Modifiers::MOD_LONG_LONG:
			s_ptr = itoa(num_buf, ch, precision, formatted_integer<long long int>(arg), width, flags, &slen);
			break;
		case Modifiers::MOD_INTMAX_T:
			s_ptr = itoa(num_buf, ch, precision, formatted_integer<intmax_t>(arg), width, flags, &slen);
			break;
		case Modifiers::MOD_SIZE_T:
			s_ptr = itoa(num_buf, ch, precision, formatted_integer<std::make_signed<size_t>::type>(arg), width, flags, &slen);
			break;
		case Modifiers::MOD_PTRDIFF_T:
			s_ptr = itoa(num_buf, ch, precision, formatted_integer<ptrdiff_t>(arg), width, flags, &slen);
			break;
		default:
			s_ptr = itoa(num_buf, ch, precision, formatted_integer<int>(arg), width, flags, &slen);
			break;
		}

		output_string(ch, s_ptr, precision, width, flags, slen, ctx);
		return Printf(ctx, format + 1, ts...);

	case 'c':
		// char is promoted to an int when pushed on the stack
		num_buf[0] = formatted_integer<char>(arg);
		num_buf[1] = '\0';
		s_ptr = num_buf;
		output_string('c', s_ptr, precision, width, flags, 1, ctx);
		return Printf(ctx, format + 1, ts...);

	case 's':
		s_ptr = formatted_string(arg);
		if(!s_ptr) {
			s_ptr = "(null)";
		}
		output_string('s', s_ptr, precision, width, flags, strlen(s_ptr), ctx);
		return Printf(ctx, format + 1, ts...);

	case 'n':
		switch(modifier) {
		case Modifiers::MOD_CHAR:
			*formatted_pointer<signed char *>(arg) = ctx.written;
			break;
		case Modifiers::MOD_SHORT:
			*formatted_pointer<short int *>(arg) = ctx.written;
			break;
		case Modifiers::MOD_LONG:
			*formatted_pointer<long int *>(arg) = ctx.written;
			break;
		case Modifiers::MOD_LONG_LONG:
			*formatted_pointer<long long int *>(arg) = ctx.written;
			break;
		case Modifiers::MOD_INTMAX_T:
			*formatted_pointer<intmax_t *>(arg) = ctx.written;
			break;
		case Modifiers::MOD_SIZE_T:
			*formatted_pointer<std::make_signed<size_t>::type *>(arg) = ctx.written;
			break;
		case Modifiers::MOD_PTRDIFF_T:
			*formatted_pointer<ptrdiff_t *>(arg) = ctx.written;
			break;
		default:
			*formatted_pointer<int *>(arg) = ctx.written;
			break;
		}

		return Printf(ctx, format + 1, ts...);

	default:
		ctx.write('%');
		// FALL THROUGH
	case '\0':
	case '%':
		ctx.write(ch);
		break;
	}

	return Printf(ctx, format + 1, ts...);
}

//------------------------------------------------------------------------------
// Name: get_modifier
// Desc: gets the modifier, if any, from the format string, then calls 
//       process_format
//------------------------------------------------------------------------------
template <class Context, class T, class... Ts>
int get_modifier(Context &ctx, const char *format, Flags flags, long int width, long int precision, const T &arg, const Ts &... ts) {

	Modifiers modifier = Modifiers::MOD_NONE;

	switch(*format) {
	case 'h':
		modifier = Modifiers::MOD_SHORT;
		++format;
		if(*format == 'h') {
			modifier = Modifiers::MOD_CHAR;
			++format;
		}
		break;
	case 'l':
		modifier = Modifiers::MOD_LONG;
		++format;
		if(*format == 'l') {
			modifier = Modifiers::MOD_LONG_LONG;
			++format;
		}
		break;
	case 'L':
		modifier = Modifiers::MOD_LONG_DOUBLE;
		++format;
		break;
	case 'j':
		modifier = Modifiers::MOD_INTMAX_T;
		++format;
		break;
	case 'z':
		modifier = Modifiers::MOD_SIZE_T;
		++format;
		break;
	case 't':
		modifier = Modifiers::MOD_PTRDIFF_T;
		++format;
		break;
	default:
		break;
	}

	return process_format(ctx, format, flags, width, precision, modifier, arg, ts...);
}

//------------------------------------------------------------------------------
// Name: get_precision
// Desc: gets the precision, if any, either from the format string or as an arg 
//       as needed, then calls get_modifier
//------------------------------------------------------------------------------
template <class Context, class T, class... Ts>
int get_precision(Context &ctx, const char *format, Flags flags, long int width, const T &arg, const Ts &... ts) {

	// default to non-existant
	long int p = -1;

	if(*format == '.') {

		++format;
		if(*format == '*') {
			++format;
			// pull an int off the stack for processing
			p = formatted_integer<long int>(arg);
			return get_modifier(ctx, format, flags, width, p, ts...);
		} else {
			char *endptr;
			p = strtol(format, &endptr, 10);
			format = endptr;
			return get_modifier(ctx, format, flags, width, p, arg, ts...);
		}
	}
	
	return get_modifier(ctx, format, flags, width, p, arg, ts...);
}

//------------------------------------------------------------------------------
// Name: get_width
// Desc: gets the width if any, either from the format string or as an arg as 
//       needed, then calls get_precision
//------------------------------------------------------------------------------
template <class Context, class T, class... Ts>
int get_width(Context &ctx, const char *format, Flags flags, const T &arg, const Ts &... ts) {

	int width = 0;

	if(*format == '*') {
		++format;
		// pull an int off the stack for processing
		width = formatted_integer<long int>(arg);
		return get_precision(ctx, format, flags, width, ts...);
	} else {
		char *endptr;
		width = strtol(format, &endptr, 10);
		format = endptr;		
		return get_precision(ctx, format, flags, width, arg, ts...);
	}
}

//------------------------------------------------------------------------------
// Name: get_flags
// Desc: gets the flags, if any, from the format string, then calls get_width
//------------------------------------------------------------------------------
template <class Context, class... Ts>
int get_flags(Context &ctx, const char *format, const Ts &... ts) {

	Flags f   = { 0, 0, 0, 0, 0, 0 };
	bool done = false;

	// skip past the % char
	++format;

	while(!done) {

		char ch = *format++;

		switch(ch) {
		case '-':
			// justify, overrides padding
			f.justify = 1;
			f.padding = 0;
			break;
		case '+':
			// sign, overrides space
			f.sign  = 1;
			f.space = 0;
			break;
		case ' ':
			if(!f.sign) {
				f.space = 1;
			}
			break;
		case '#':
			f.prefix = 1;
			break;
		case '0':
			if(!f.justify) {
				f.padding = 1;
			}
			break;
		default:
			done = true;
			--format;
		}
	}

	return get_width(ctx, format, f, ts...);
}

}

//------------------------------------------------------------------------------
// Name: Printf
// Desc: 0 argument version of Printf. Asserts on any format character found
//------------------------------------------------------------------------------
template <class Context>
int Printf(Context &ctx, const char *format) {

	assert(format);

	for (; *format; ++format) {
		if (*format != '%' || *++format == '%') {
			ctx.write(*format);
			continue;
		}

		assert(!"Bad Format");
	}

	// this will usually null terminate the string
	ctx.done();

	// return the amount of bytes that should have been written if there was sufficient space
	return ctx.written;
}

//------------------------------------------------------------------------------
// Name: Printf
// Desc: 1+ argument version of Printf.
//------------------------------------------------------------------------------
template <class Context, class... Ts>
int Printf(Context &ctx, const char *format, const Ts &... ts) {

	assert(format);

	while(*format != '\0') {
		if(*format == '%') {
			// %[flag][width][.precision][length]char
			
			// this recurses into get_width -> get_precision -> get_length -> process_format
			return detail::get_flags(ctx, format, ts...);
		} else {
			ctx.write(*format);
		}

		++format;
	}

	// clean up any trailing stuff
	return Printf(ctx, format + 1, ts...);
}

}


#endif
