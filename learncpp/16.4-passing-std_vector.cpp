#include <iostream>
#include <vector>

template <typename T>
void passByRef(const T& arr) // will accept any type of object that has an overloaded operator[]
//can do this instead of const vector<T>&
//also can use const auto& arr
{
    std::cout << arr[0] << '\n';
}

int main()
{
    std::vector primes{ 2, 3, 5, 7, 11 };
    passByRef(primes); // ok: compiler will instantiate passByRef(const std::vector<int>&)

    std::vector dbl{ 1.1, 2.2, 3.3 };
    passByRef(dbl);    // ok: compiler will instantiate passByRef(const std::vector<double>&)

    return 0;
}