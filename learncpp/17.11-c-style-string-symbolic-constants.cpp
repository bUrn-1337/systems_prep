auto s1{ "Alex" };  // type deduced as const char*
auto* s2{ "Alex" }; // type deduced as const char*
auto& s3{ "Alex" }; // type deduced as const char(&)[5]


//if you want to print address of char, cant do this 
#include <iostream>

int main()
{
    char c{ 'Q' };
    std::cout << &c;

    return 0;
}

//do this instead 
#include <iostream>

int main()
{
    const char* ptr{ "Alex" };

    std::cout << ptr << '\n';                           // print ptr as C-style string
    std::cout << static_cast<const void*>(ptr) << '\n'; // print address held by ptr

    return 0;
}