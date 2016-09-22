
#include "Printf.h"

#include <iostream>
#include <chrono>

template <class R, int Count, class F>
R time_code(F func) {

	// test the timing of our printf
	auto then = std::chrono::system_clock::now();
	
	for(int i = 0; i < Count; ++i) {
		func();
	}
	
	auto now = std::chrono::system_clock::now();
	auto dur = now - then;
	
	return std::chrono::duration_cast<R>(dur);
}

int main() {

	int Foo = 1234;

	// first test correctness
	cxx11::stdout_writer ctx;
	cxx11::Printf(ctx, "hello %*s, %c, %d, %08x %p %016u %02x\n", 10, "world", 0x41, -123, 0x1234, static_cast<void *>(&Foo), -4, -1);
	printf(               "hello %*s, %c, %d, %08x %p %016u %02x\n", 10, "world", 0x41, -123, 0x1234, static_cast<void *>(&Foo), -4, -1);
	
	typedef std::chrono::microseconds ms;
	
	constexpr int count = 1000000;
	
	auto time1 = time_code<ms, count>([&Foo]() {
		char buf[256];
		cxx11::buffer_writer ctx(buf, 256);
		cxx11::Printf(ctx, "hello %*s, %c, %d, %08x %p %016u %02x\n", 10, "world", 0x41, -123, 0x1234, &Foo, -4, -1);	
	});
	
	auto time2 = time_code<ms, count>([&Foo]() {
		char buf[256];
		snprintf(buf, 256, "hello %*s, %c, %d, %08x %p %016u %02x\n", 10, "world", 0x41, -123, 0x1234, static_cast<void *>(&Foo), -4, -1);
	});	
	
	std::cerr << "First Took:  " << time1.count() << " \xC2\xB5s to execute." << std::endl;	
	std::cerr << "Second Took: " << time2.count() << " \xC2\xB5s to execute." << std::endl;	
}
