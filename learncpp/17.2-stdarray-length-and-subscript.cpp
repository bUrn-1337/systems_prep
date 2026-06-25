//here, sizetype is always an alias for size_t
template<typename T, std::size_t N> // N is a non-type template parameter
struct array;
#include <array>
#include <iostream>

int main()
{
    constexpr std::array arr { 9, 7, 5, 3, 1 };
    std::cout << "length: " << std::ssize(arr); // C++20, returns length as a large signed integral type
                                                //usually std::ptrdiff_t
    return 0;
}


//all of the methods, i.e, .size() member, std::size() and std::ssize(), all will return size as constexpr

//with std::get we can get compile time bounds checking
#include <array>
#include <iostream>

int main()
{
    constexpr std::array prime{ 2, 3, 5, 7, 11 };

    std::cout << std::get<3>(prime); // print the value of element with index 3
    std::cout << std::get<9>(prime); // invalid index (compile error)

    return 0;
}