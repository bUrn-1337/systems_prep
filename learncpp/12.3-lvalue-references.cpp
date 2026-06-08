// regular types
int        // a normal int type (not an reference)
int&       // an lvalue reference to an int object
double&    // an lvalue reference to a double object
const int& // an lvalue reference to a const int object
//lvalue reference also called just refeernce, it is of two types:
//An lvalue reference to a non-const is commonly just called an “lvalue reference”, but may also be referred to as an lvalue reference to non-const or a non-const lvalue reference (since it isn’t defined using the const keyword).
//An lvalue reference to a const is commonly called either an lvalue reference to const or a const lvalue reference.

int main()
{
    int& invalidRef;   // error: references must be initialized

    int x { 5 };
    int& ref { x }; // okay: reference to int is bound to int variable

    return 0;
}
//When a reference is initialized with an object (or function), we say it is bound to that object (or function). The process by which such a reference is bound is called reference binding. The object (or function) being referenced is sometimes called the referent.

int main()
{
    int x { 5 };
    int& ref { x };         // okay: non-const lvalue reference bound to a modifiable lvalue

    const int y { 5 };
    int& invalidRef { y };  // invalid: non-const lvalue reference can't bind to a non-modifiable lvalue
    int& invalidRef2 { 0 }; // invalid: non-const lvalue reference can't bind to an rvalue

    return 0;
}

int main()
{
    int x { 5 };
    int& ref { x };            // okay: referenced type (int) matches type of initializer

    double d { 6.0 };
    int& invalidRef { d };     // invalid: conversion of double to int is narrowing conversion, disallowed by list initialization
    double& invalidRef2 { x }; // invalid: non-const lvalue reference can't bind to rvalue (result of converting x to double)

    return 0;
}
//Once initialized, a reference in C++ cannot be reseated, meaning it cannot be changed to reference another object.

int& ref { x }; // ref is now an alias for x
ref = y; // assigns 6 (the value of y) to x (the object being referenced by ref)



//Reference variables follow the same scoping and duration rules that normal variables do

//A reference can be destroyed before the object it is referencing.
//The object being referenced can be destroyed before the reference.
//When an object being referenced is destroyed before a reference to it, the reference is left referencing an object that no longer exists. Such a reference is called a dangling reference. Accessing a dangling reference leads to undefined behavior.



//references are not objects in C++. A reference is not required to exist or occupy storage. If possible, the compiler will optimize references away by replacing all occurrences of a reference with the referent.


int var{};
int& ref1{ var };  // an lvalue reference bound to var
int& ref2{ ref1 }; // an lvalue reference bound to var
//C++ doesn’t support references to references