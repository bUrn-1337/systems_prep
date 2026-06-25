//when a C-style array is used in an expression, the array will be implicitly converted into a pointer to the element type, initialized with the address of the first element (with index 0). Colloquially, this is called array decay
//A decayed array pointer does not know how long the array it is pointing to is. The term “decay” indicates this loss of length type information.
//Subscripting a C-style array actually applies operator[] to the decayed pointer
#include <iostream>
//also if we give length info in function param, it will be ignored
void printArraySize(int arr[])
{
    std::cout << sizeof(arr) << '\n'; // prints 4 (assuming 32-bit addresses)
    std::cout << std::size(arr) << '\n'; // compile error: std::size() won't work on a pointer

}

int main()
{
    int arr[]{ 3, 2, 1 };

    std::cout << sizeof(arr) << '\n'; // prints 12 (assuming 4 byte ints)

    printArraySize(arr);

    return 0;
}