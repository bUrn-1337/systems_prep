//The length and indices of std::vector have type size_type
//Each of the standard library container classes defines a nested typedef member
// named size_type (sometimes written as T::size_type), which is an alias for the 
//type used for the length (and indices, if supported) of the container.
//size_type is almost always an alias for std::size_t, but can be overridden (in rare cases) to use a different type.


#include <iostream>
#include <vector>

int main()
{
    std::vector prime { 2, 3, 5, 7, 11 };
    std::cout << "length: " << std::size(prime); // C++17, returns length as type `size_type` (alias for `std::size_t`)

    return 0;
}
//can do static cast to store as int

#include <iostream>
#include <vector>

int main()
{
    std::vector prime{ 2, 3, 5, 7, 11 };
    std::cout << "length: " << std::ssize(prime); // C++20, returns length as a large signed integral type

    return 0;
}
//int may be smaller than the type returned by ssize, so do static cast




//operator[] does not do bounds checking
#include <iostream>
#include <vector>

int main()
{
    std::vector prime{ 2, 3, 5, 7, 11 };

    std::cout << prime[3];  // print the value of element with index 3 (7)
    std::cout << prime[9]; // invalid index (undefined behavior)

    return 0;
}
//Accessing array elements using the at() member function does runtime bounds checking
#include <iostream>
#include <vector>

int main()
{
    std::vector prime{ 2, 3, 5, 7, 11 };

    std::cout << prime.at(3); // print the value of element with index 3, also it returns a refernce 
    std::cout << prime.at(9); // invalid index (throws exception)

    return 0;
}



#include <iostream>
#include <vector>

int main()
{
    std::vector prime{ 2, 3, 5, 7, 11 };

    int index { 3 };                          // non-constexpr signed value
    std::cout << prime.data()[index] << '\n'; // okay: no sign conversion warnings

    return 0;
}
// The data() member function returns a pointer to this underlying C-style array, which we can then index.
// Since C-style arrays allow indexing with both signed and unsigned types, we don’t run into any sign conversion issues
//normally we would get warnings