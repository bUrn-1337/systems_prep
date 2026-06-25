#include <cstring> // for std::strlen
#include <iostream>

int main()
{
    char str[255]{ "string" }; // 6 characters + null terminator
    std::cout << "length = " << std::strlen(str) << '\n'; // prints length = 6

    char *ptr { str };
    std::cout << "length = " << std::strlen(ptr) << '\n';   // prints length = 6

    return 0;
}
// strlen() -- returns the length of a C-style string
// strcpy(), strncpy(), strcpy_s() -- overwrites one C-style string with another
// strcat(), strncat() -- Appends one C-style string to the end of another
// strcmp(), strncmp() -- Compares two C-style strings (returns 0 if equal)