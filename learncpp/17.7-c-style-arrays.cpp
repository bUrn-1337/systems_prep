//he length of a C-style array must be at least 1. The compiler will error if the array length is zero, negative, or a non-integral value.
//C-style arrays dynamically allocated on the heap are allowed to have length 0.


//Unlike the standard library container classes (which use unsigned indices of type std::size_t only), the index of a C-style array can be a value of any integral type (signed or unsigned) or an unscoped enumeration
//When declaring an array (e.g. int arr[5]), the use of [] is part of the declaration syntax, not an invocation of the subscript operator operator[].


//Just like std::array, C-style arrays are aggregates, which means they can be initialized using aggregate initialization.


int main()
{
    auto squares[5] { 1, 4, 9, 16, 25 }; // compile error: can't use type deduction on C-style arrays

    return 0;
}


#include <iostream>
#include <iterator> // for std::size and std::ssize

int main()
{
    const int prime[] { 2, 3, 5, 7, 11 };   // the compiler will deduce prime to have length 5

    std::cout << std::size(prime) << '\n';  // C++17, returns unsigned integral value 5
    std::cout << std::ssize(prime) << '\n'; // C++20, returns signed integral value 5

    return 0;
}


int main()
{
    int arr[] { 1, 2, 3 }; // okay: initialization is fine
    arr[0] = 4;            // assignment to individual elements is fine
    arr = { 5, 6, 7 };     // compile error: array assignment not valid

    return 0;
}