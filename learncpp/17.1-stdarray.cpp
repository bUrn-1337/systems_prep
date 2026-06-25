#include <array>  // for std::array
#include <vector> // for std::vector

int main()
{
    std::array<int, 5> a {};  // a std::array of 5 ints

    std::vector<int> b(5);    // a std::vector of 5 ints (for comparison)

    return 0;
}
//we generally use it for constexpr arrays
//The length of a std::array must be a constant expression

//std::array is an aggregate. This means it has no constructors, and instead is initialized using aggregate initialization.
#include <array>

int main()
{
    std::array<int, 6> fibonnaci = { 0, 1, 1, 2, 3, 5 }; // copy-list initialization using braced list
    std::array<int, 5> prime { 2, 3, 5, 7, 11 };         // list initialization using braced list (preferred)

    std::array<int, 5> a;   // Members default initialized (int elements are left uninitialized)
    std::array<int, 5> b{}; // Members value initialized (int elements are zero initialized) (preferred)

    std::vector<int> v(5);  // Members value initialized (int elements are zero initialized) (for comparison)
    std::array<int, 4> a { 1, 2, 3, 4, 5 }; // compile error: too many initializers
    std::array<int, 4> b { 1, 2 };          // b[2] and b[3] are value initialized


    constexpr std::array a1 { 9, 7, 5, 3, 1 }; // The type is deduced to std::array<int, 5>
    constexpr std::array a2 { 9.7, 7.31 };     // The type is deduced to std::array<double, 2>


    constexpr std::array<int> a2 { 9, 7, 5, 3, 1 };     // error: too few template arguments (length missing)
    constexpr std::array<5> a2 { 9, 7, 5, 3, 1 };       // error: too few template arguments (type missing)
    return 0;
}