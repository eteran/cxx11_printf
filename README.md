# cxx11_printf

An implementation of `printf` using c++11's variadic templates. It was inspired 
by some of the "safe printf" examples that I've seen. But none of them attempted
to actually implement the `printf` fully with all of its quirks. This code is 
(an attempt) to do that.

An advantage of this, is that we can support ANY type being passed to a format, 
even complex objects. For example, when `CXX11_PRINTF_EXTENSIONS` is enabled
`"%?"` means "call `to_string` on the argument and print the result" This uses
ADL, so it will find the version that you specify in your namespaces, or use 
`std::to_string` as a fallback. If no `to_string` is found, it uses the internal
one which asserts.

NOTE: floating point is not implemented, as it is complex to do correctly, but 
      is on the TODO list.
	  
Usage is similar to `snprintf`, but more robust. Instead of a buffer/size pair
being passed as a parameter, you pass a context object which has two functions
and a data member:

	// writes a single character to the output of your choosing
	void write(char ch);
	
	// called when formatting is complete, useful for ensuring NUL termination
	void done();
	
	// increment this every time we write is called, this is what printf will
	// return.
	size_t written = 0;
	
	
A simple example of the usage of the library is as follows:

	char buf[256];
	cxx11::buffer_writer ctx(buf, sizeof(buf));
	cxx11::Printf(ctx, "[hello %*s %d]", 10, "world", 123);
	
	// buffer now contains "[hello      world 123]"
	
The context uses duck typing, so any object that meets the critera will suffice,
but there are several examples in Formatters.h

--------

Additionally, while the context based interface is very flexible and can 
accomidate essentially any destination stream or buffer. This implementation
also includes more familiar interfaces:

* `int cxx11::sprintf(std::ostream &os, const char *format, const Ts &... ts);`
* `int cxx11::sprintf(char *str, size_t size, const char *format, const Ts &... ts);`
* `int cxx11::printf(const char *format, const Ts &... ts);`

All of which work in the expected ways without the need to manually manage the 
concept of "contexts".


--------

Performance so far, when optimizations are at -O3 is comparable to glibc's 
printf. Here is the included test program's output on my machine:

	hello      world, A, -123, 00001234 0x7ffeab406cc4 0000004294967292 ffffffff
	hello      world, A, -123, 00001234 0x7ffeab406cc4 0000004294967292 ffffffff
	First Took:  410116 µs to execute.
	Second Took: 407768 µs to execute.

I am sure however, that there is room for some optimizations too :-)
