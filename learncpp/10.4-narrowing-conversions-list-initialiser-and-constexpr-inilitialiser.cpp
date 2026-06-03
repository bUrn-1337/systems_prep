//a conversion is narrowing when it is “From an integral type to another integral type that cannot represent all values of the original type, unless the value being converted is constexpr and whose value can be stored exactly in the destination type.”

#include <iostream>

int main()
{
    constexpr int n1{ 5 };   // note: constexpr
    unsigned int u1 { n1 };  // okay: conversion is not narrowing due to exclusion clause

    constexpr int n2 { -5 }; // note: constexpr
    unsigned int u2 { n2 };  // compile error: conversion is narrowing due to value change

    return 0;
}


constexpr double d { 0.1 };
float f { d }; // not narrowing, even though loss of precision results

// initializing a narrower or lesser ranked floating point type with a constexpr value is allowed as long as the value is in range of the destination type, even if the destination type doesn’t have enough precision to precisely store the value!    
int main()
{
    float f { 1.23456789 }; // not a narrowing conversion, even though precision lost!

    return 0;
}