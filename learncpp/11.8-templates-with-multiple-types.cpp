//type conversion is done only when resolving function overloads, not when performing template argument deduction.
#include <iostream>

template <typename T>
T max(T x, T y)
{
    return (x < y) ? y : x;
}

int main()
{
    std::cout << max(2, 3.5) << '\n';  // compile error
    std::cout << max<double>(2, 3.5) << '\n'; //correct
    return 0;
}

#include <iostream>

template <typename T, typename U>
auto max(T x, U y) // ask compiler can figure out what the relevant return type is, better than keeping it U or T
{
    return (x < y) ? y : x;
}// this is *

int main()
{
    std::cout << max(2, 3.5) << '\n';

    return 0;
}
//recall function with an auto return type needs to be fully defined before it can be used (a forward declaration won’t suffice), since the compiler has to inspect the function implementation to determine the return type.
//to do forward declaration:
#include <iostream>
#include <type_traits> // for std::common_type_t

template <typename T, typename U>
auto max(T x, U y) -> std::common_type_t<T, U>; // returns the common type of T and U

int main()
{
    std::cout << max(2, 3.5) << '\n';

    return 0;
}

template <typename T, typename U>
auto max(T x, U y) -> std::common_type_t<T, U>
{
    return (x < y) ? y : x;
}



//since c++20 this is the shorthand for *
auto max(auto x, auto y)
{
    return (x < y) ? y : x;
}


//Just like functions may be overloaded, function templates may also be overloaded. Such overloads can have a different number of template types and/or a different number or type of function parameters
//if more than one template matches, whichever function template is more restrictive/specialized will be preferred