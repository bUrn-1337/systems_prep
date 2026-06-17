//to print function's address
#include <iostream>

int foo() // code starts at memory address 0x002717f0
{
    return 5;
}

int main()
{
    std::cout << reinterpret_cast<void*>(foo) << '\n'; // Tell C++ to interpret function foo as a void pointer (implementation-defined behavior)

    return 0;
}


// fcnPtr is a pointer to a function that takes no arguments and returns an integer
int (*fcnPtr)();

// to make const function pointer:
int (*const fcnPtr)();


int foo()
{
    return 5;
}

int goo()
{
    return 6;
}

int main()
{
    int (*fcnPtr)(){ &foo }; // fcnPtr points to function foo
    fcnPtr = &goo; // fcnPtr now points to function goo

    return 0;
}
//C++ will implicitly convert a function into a function pointer if needed
// function pointers will not convert to void pointers, or vice-versa


// function prototypes
int foo();

// function initializations
int (*fcnPtr5)() { foo }; // okay, foo implicitly converts to function pointer to foo
void* vPtr { foo };       // not okay, though some compilers may allow


//can also init with nullptr

int foo(int x)
{
    return x;
}

int main()
{
    int (*fcnPtr)(int){ &foo }; // Initialize fcnPtr with function foo
    (*fcnPtr)(5); // call function foo(5) through fcnPtr.
    //implicit dereference:
    fcnPtr(5); // call function foo(5) through fcnPtr.

    return 0;
}


//When the compiler encounters a normal function call to a function with one or more default arguments, it rewrites the function call to include the default arguments.


//we can give callback functions as arguments
void selectionSort(int* array, int size, bool (*comparisonFcn)(int, int));
//also for function params, this is equivalent to 
void selectionSort(int* array, int size, bool comparisonFcn(int, int))
//can also set default param
// Default the sort to ascending sort
void selectionSort(int* array, int size, bool (*comparisonFcn)(int, int) = ascending);



//can also use std::function
#include <functional>
bool validate(int x, int y, std::function<bool(int, int)> fcn); // std::function method that returns a bool and takes two int parameters

//std::function only allows calling the function via implicit dereference (e.g. fcnPtr()), not explicit dereference (e.g. (*fcnPtr)()).