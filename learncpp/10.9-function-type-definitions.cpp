auto someFcn(bool b)
{
    if (b)
        return 5; // return type int
    else
        return 6.7; // return type double
}
//When using an auto return type, all return statements within the function must return values of the same type, otherwise an error will result.


//Functions that use an auto return type must be fully defined before they can be used (a forward declaration is not sufficient). For example:
#include <iostream>

auto foo();

int main()
{
    std::cout << foo() << '\n'; // the compiler has only seen a forward declaration at this point

    return 0;
}

auto foo()
{
    return 5;
}
//error C3779: 'foo': a function that returns 'auto' cannot be used before it is defined.


//If we have a function whose return type must be deduced based on the type of the function parameters, a normal return type will not suffice, because the compiler has not yet seen the parameters at that point.
#include <type_traits>
// note: decltype(x) evaluates to the type of x

std::common_type_t<decltype(x), decltype(y)> add(int x, double y);         // Compile error: compiler hasn't seen definitions of x and y yet
auto add(int x, double y) -> std::common_type_t<decltype(x), decltype(y)>; // ok


//Type deduction can’t be used for function parameter types