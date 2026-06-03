#include <iostream>

void incrementAndPrint()
{
    static int s_value{ 1 }; // static duration via static keyword.  This initializer is only executed once.
    ++s_value;
    std::cout << s_value << '\n';
} // s_value is not destroyed here, but becomes inaccessible because it goes out of scope

int main()
{
    incrementAndPrint(); //2
    incrementAndPrint(); //3
    incrementAndPrint(); //4

    return 0;
}

//Static local variables are only initialized the first time the code is executed, not on subsequent calls.
//A static local variable has block scope like a local variable, but its lifetime is until the end of the program like a global variable. 