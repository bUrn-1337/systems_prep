//Unlike normal member variables, static member variables are shared by all objects of the class.

#include <iostream>

struct Something
{
    static int s_value; // declare s_value as static (initializer moved below)
};

int Something::s_value{ 1 }; // define and initialize s_value to 1 (we'll discuss this section below), we must do it in global scope since its a global variable

int main()
{
    Something first{};
    Something second{};

    first.s_value = 2;

    std::cout << first.s_value << '\n'; //2
    std::cout << second.s_value << '\n'; //2
    return 0;
}

//Static members are global variables that live inside the scope region of the class.
//can also access like
Something::s_value = 2;


//static member definition is not subject to access controls: you can define and initialize the value even if it’s declared as private (or protected) in the class (as definitions are not considered to be a form of access).


//if we want to initialize inside the class we can do the following 
class Whatever
{
public:
    static const int s_value{ 4 }; // a static const int can be defined and initialized directly
};

class Whatever
{
public:
    static inline int s_value{ 4 }; // a static inline variable can be defined and initialized directly
};

// in place of inline we can also put constexpr because constexpr are implicitly inline


//we may use this for id generator



#include <utility> // for std::pair<T, U>

class Foo
{
private:
    auto m_x { 5 };           // auto not allowed for non-static members
    std::pair m_v { 1, 2.3 }; // CTAD not allowed for non-static members

    static inline auto s_x { 5 };           // auto allowed for static members
    static inline std::pair s_v { 1, 2.3 }; // CTAD allowed for static members

public:
    Foo() {};
};

int main()
{
    Foo foo{};

    return 0;
}