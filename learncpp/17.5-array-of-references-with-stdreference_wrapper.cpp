//because references are not objects, you cannot make an array of references. The elements of an array must also be assignable, and references can’t be reseated.
#include <array>
#include <iostream>

int main()
{
    int x { 1 };
    int y { 2 };

    [[maybe_unused]] std::array<int&, 2> refarr { x, y }; // compile error: cannot define array of references

    int& ref1 { x };
    int& ref2 { y };
    [[maybe_unused]] std::array valarr { ref1, ref2 }; // ok: this is actually a std::array<int, 2>, not an array of references

    return 0;
}

//we can use std::reference_wrapper
// Operator= will reseat a std::reference_wrapper (change which object is being referenced).
// std::reference_wrapper<T> will implicitly convert to T&.
// The get() member function can be used to get a T&. This is useful when we want to update the value of the object being referenced.

#include <array>
#include <functional> // for std::reference_wrapper
#include <iostream>

int main()
{
    int x { 1 };
    int y { 2 };
    int z { 3 };

    std::array<std::reference_wrapper<int>, 3> arr { x, y, z };

    arr[1].get() = 5; // modify the object in array element 1

    std::cout << arr[1] << y << '\n'; // show that we modified arr[1] and y, prints 55

    return 0;
}



//std::ref() and std::cref() functions were provided as shortcuts to create std::reference_wrapper and const std::reference_wrapper wrapped objects
int x { 5 };
auto ref { std::ref(x) };   // C++11, deduces to std::reference_wrapper<int>
auto cref { std::cref(x) }; // C++11, deduces to std::reference_wrapper<const int>