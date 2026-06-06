#include <iostream>

template <typename T>
T max(T x, T y)
{
    return (x < y) ? y : x;
}

int main()
{
    std::cout << max<int>(1, 2) << '\n'; // instantiates and calls function max<int>(int, int)

    return 0;
}
//when we call max<int>(1, 2), the function specialization that gets instantiated looks something like this:
template<> // ignore this for now
int max<int>(int x, int y) // the generated function max<int>(int, int)
{
    return (x < y) ? y : x;
}

//A function template is only instantiated the first time a function call is made in each translation unit. Conversely, if no function call is made to a function template, the function template won’t be instantiated in that translation unit.

std::cout << max<>(1, 2) << '\n';
std::cout << max(1, 2) << '\n';//compiler will attempt to deduce the type and generate a max function
// In the top case (with the empty angled brackets), the compiler will only consider max<int> template function overloads when determining which overloaded function to call.
// In the bottom case (with no angled brackets), the compiler will consider both max<int> template function overloads and max non-template function overloads.


#include <iostream>

template <typename T>
T max(T x, T y)
{
    std::cout << "called max<int>(int, int)\n";
    return (x < y) ? y : x;
}

int max(int x, int y)
{
    std::cout << "called max(int, int)\n";
    return (x < y) ? y : x;
}

int main()
{
    std::cout << max<int>(1, 2) << '\n'; // calls max<int>(int, int)
    std::cout << max<>(1, 2) << '\n';    // deduces max<int>(int, int) (non-template functions not considered)
    std::cout << max(1, 2) << '\n';      // calls max(int, int)

    return 0;
}
// When the bottom case results in both a template function and a non-template function that are equally viable, the non-template function will be preferred.


#include <iostream>

template <typename T>
T addOne(T x)
{
    return x + 1;
}

int main()
{
    std::cout << addOne("Hello, world!") << '\n'; //ello, world!

    return 0;
}


//When a static local variable is used in a function template, each function instantiated from that template will have a separate version of the static local variable.