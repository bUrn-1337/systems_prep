double division{ 4.0 / 3 }; // int value 3 implicitly converted to type double


//The following program actually prints int value 3 as if it were a float
#include <iostream>
#include <cstring>

int main()
{
    int n { 3 };                        // here's int value 3
    float f {};                         // here's our float variable
    std::memcpy(&f, &n, sizeof(float)); // copy the bits from n into f
    std::cout << f << '\n';             // print f (containing the bits from n)
    //4.2039e-45
    return 0;
}