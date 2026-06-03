#include <cstdint> // for fixed-width integers
#include <iostream>

int main()
{
    std::int32_t x { 32767 }; // x is always a 32-bit integer
    x = x + 1;                // so 32768 will always fit
    std::cout << x << '\n';

    return 0;
}
//Due to an oversight in the C++ specification, modern compilers typically treat std::int8_t and std::uint8_t (and the corresponding fast and least fixed-width types, which we’ll introduce in a moment) the same as signed char and unsigned char respectively. Thus on most modern systems, the 8-bit fixed-width integral types will behave like char types.
#include <cstdint> // for fixed-width integers
#include <iostream>

int main()
{
    std::int8_t x { 65 };   // initialize 8-bit integral type with value 65
    std::cout << x << '\n'; // You're probably expecting this to print 65

    return 0;
}

//The fixed-width integers actually don’t define new types -- they are just aliases for existing integral types with the desired size. For each fixed-width type, the implementation (the compiler and standard library) gets to determine which existing type is aliased. As an example, on a platform where int is 32-bits, std::int32_t will be an alias for int. On a system where int is 16-bits (and long is 32-bits), std::int32_t will be an alias for long instead.

// The fast types (std::int_fast#_t and std::uint_fast#_t) provide the fastest signed/unsigned integer type with a width of at least # bits (where # = 8, 16, 32, or 64). For example, std::int_fast32_t will give you the fastest signed integer type that’s at least 32-bits. By fastest, we mean the integral type that can be processed most quickly by the CPU.
// The least types (std::int_least#_t and std::uint_least#_t) provide the smallest signed/unsigned integer type with a width of at least # bits (where # = 8, 16, 32, or 64). For example, std::uint_least32_t will give you the smallest unsigned integer type that’s at least 32-bits.
#include <cstdint> // for fast and least types
#include <iostream>

int main()
{
	std::cout << "least 8:  " << sizeof(std::int_least8_t)  * 8 << " bits\n";
	std::cout << "least 16: " << sizeof(std::int_least16_t) * 8 << " bits\n";
	std::cout << "least 32: " << sizeof(std::int_least32_t) * 8 << " bits\n";
	std::cout << '\n';
	std::cout << "fast 8:  "  << sizeof(std::int_fast8_t)   * 8 << " bits\n";
	std::cout << "fast 16: "  << sizeof(std::int_fast16_t)  * 8 << " bits\n";
	std::cout << "fast 32: "  << sizeof(std::int_fast32_t)  * 8 << " bits\n";

	return 0;
}

//The answer is that sizeof returns a value of type std::size_t. std::size_t is an alias for an implementation-defined unsigned integral type. In other words, the compiler decides if std::size_t is an unsigned int, an unsigned long, an unsigned long long, etc…\
//std::size_t is actually a typedef