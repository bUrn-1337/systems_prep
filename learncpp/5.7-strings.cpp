#include <iostream>
#include <string> // For std::string and std::getline

int main()
{
    std::cout << "Enter your full name: ";
    std::string name{};
    std::getline(std::cin >> std::ws, name); // read a full line of text into name

    std::cout << "Enter your favorite color: ";
    std::string color{};
    std::getline(std::cin >> std::ws, color); // read a full line of text into color

    std::cout << "Your name is " << name << " and your favorite color is " << color << '\n';

    return 0;
}
//If using std::getline() to read strings, use std::cin >> std::ws input manipulator to ignore leading whitespace. This needs to be done for each std::getline() call, as std::ws is not preserved across calls
//When extracting to a variable, the extraction operator (>>) ignores leading whitespace. It stops extracting when encountering non-leading whitespace.
//std::getline() does not ignore leading whitespace. If you want it to ignore leading whitespace, pass std::cin >> std::ws as the first argument. It stops extracting when encountering a newline.



//std::string::length() returns an unsigned integral value (most likely of type size_t). If you want to assign the length to an int variable, you should static_cast it to avoid compiler warnings about signed/unsigned conversions:

int length { static_cast<int>(name.length()) };