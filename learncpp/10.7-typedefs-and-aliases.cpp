using Distance = double; // define Distance as an alias for type double

int main()
{
    using Miles = long; // define Miles as an alias for type long
    using Speed = long; // define Speed as an alias for type long

    Miles distance { 5 }; // distance is actually just a long
    Speed mhz  { 3200 };  // mhz is actually just a long

    // The following is syntactically valid (but semantically meaningless)
    distance = mhz;

    return 0;
}

// The following aliases are identical
typedef long Miles;
using Miles = long;

typedef int (*FcnType)(double, char); // FcnType hard to find
using FcnType = int(*)(double, char); // FcnType easier to find

//The fixed-width integer types (such as std::int16_t and std::uint32_t) and the size_t type (both covered in lesson 4.6 -- Fixed-width integers and size_t) are actually just type aliases to various fundamental types.
//This is also why when you print an 8-bit fixed-width integer using std::cout, you’re likely to get a character value. 
#include <cstdint> // for fixed-width integers
#include <iostream>

int main()
{
    std::int8_t x{ 97 }; // int8_t is usually a typedef for signed char
    std::cout << x << '\n';

    return 0;
}