#include <iostream>

int main()
{
    int x { 5 };          // x is a modifiable lvalue
    const int& ref { x }; // okay: we can bind a const reference to a modifiable lvalue

    std::cout << ref << '\n'; // okay: we can access the object through our const reference
    ref = 7;                  // error: we can not modify an object through a const reference

    x = 6;                // okay: x is a modifiable lvalue, we can still modify it through the original identifier

    return 0;
}

#include <iostream>

int main()
{
    const int& ref { 5 }; // okay: 5 is an rvalue
    std::cout << ref << '\n'; // prints 5
    return 0;
}
//lvalue ref to const can also bound to rvalues
//When this happens, a temporary object is created and initialized with the rvalue, and the reference to const is bound to that temporary object.
//Lvalue references to const can even bind to values of a different type, so long as those values can be implicitly converted to the reference type
//If you try to bind a const lvalue reference to a value of a different type, the compiler will create a temporary object of the same type as the reference, initialize it using the value, and then bind the reference to the temporary.


#include <iostream>

int main()
{
    short bombs { 1 };         // I can has bomb! (note: type is short)

    const int& you { bombs };  // You can has bomb too (note: type is int&)
    --bombs;                   // Bomb all gone

    if (you)                   // You still has?
    {
        std::cout << "Bombs away!  Goodbye, cruel world.\n"; // Para bailar la bomba
    }

    return 0;
}
//We normally assume that a reference is identical to the object it is bound to -- but this assumption is broken when a reference is bound to 
//a temporary copy of the object or a temporary resulting from the conversion of the object instead. Any modifications subsequently made to 
//the original object will not be seen by the reference (as it is referencing a different object), and vice-versa



//When a const lvalue reference is directly bound to a temporary object, the lifetime of the temporary object is extended to match the lifetime of the reference.



//When applied to a reference, constexpr allows the reference to be used in a constant expression. Constexpr references have a particular limitation: they can only be bound to objects with static duration (either globals or static locals).
int g_x { 5 };

int main()
{
    [[maybe_unused]] constexpr int& ref1 { g_x }; // ok, can bind to global

    static int s_x { 6 };
    [[maybe_unused]] constexpr int& ref2 { s_x }; // ok, can bind to static local

    int x { 6 };
    [[maybe_unused]] constexpr int& ref3 { x }; // compile error: can't bind to non-static object

    return 0;
}
//When defining a constexpr reference to a const variable, we need to apply both constexpr (which applies to the reference) and const (which applies to the type being referenced
int main()
{
    static const int s_x { 6 }; // a const int
    [[maybe_unused]] constexpr const int& ref2 { s_x }; // needs both constexpr and const

    return 0;
}