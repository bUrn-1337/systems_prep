#include <iostream>

struct Foo
{
    void printValue(int value)
    {
        std::cout << value;
    }
};

void printValue(int value)
{
    std::cout << value;
}

int main()
{
    printValue(5);   // direct function call to printValue(int)

    Foo f{};
    f.printValue(5); // direct function call to Foo::printValue(int)
    return 0;
}
// when a direct call is made to a non-member function or a non-virtual member function,
// the compiler can determine which function definition should be matched to the call. 
//This is sometimes called early binding (or static binding), as it can be performed at
/// compile-time. The compiler (or linker) can then generate machine language instructions
// that tells the CPU to jump directly to the address of the function.


//Calls to overloaded functions and function templates can also be resolved at compile-time



#include <iostream>

void printValue(int value)
{
    std::cout << value << '\n';
}

int main()
{
    auto fcn { printValue }; // create a function pointer and make it point to function printValue
    fcn(5);                  // invoke printValue indirectly through the function pointer

    return 0;
}

//Calling a function via a function pointer is also known as an indirect function call.
// At the point where fcn(5) is actually called, the compiler does not know at compile-time 
//what function is being called. Instead, at runtime, an indirect function call is made to 
//whatever function exists at the address held by the function pointer.