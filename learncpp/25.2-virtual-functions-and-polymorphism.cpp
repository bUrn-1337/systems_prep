// A virtual function is a special type of member function that, when called, resolves 
// to the most-derived version of the function for the actual type of the object being referenced or pointed to.
// A derived function is considered a match if it has the same signature (name, parameter
// types, and whether it is const) and return type as the base version of the function. Such 
// functions are called overrides.

#include <iostream>
#include <string>
#include <string_view>

class Animal
{
protected:
    std::string m_name {};

    // We're making this constructor protected because
    // we don't want people creating Animal objects directly,
    // but we still want derived classes to be able to use it.
    Animal(std::string_view name)
        : m_name{ name }
    {
    }

public:
    const std::string& getName() const { return m_name; }
    virtual std::string_view speak() const { return "???"; }
};

class Cat: public Animal
{
public:
    Cat(std::string_view name)
        : Animal{ name }
    {
    }

    virtual std::string_view speak() const { return "Meow"; }
};

class Dog: public Animal
{
public:
    Dog(std::string_view name)
        : Animal{ name }
    {
    }

    virtual std::string_view speak() const { return "Woof"; }
};

void report(const Animal& animal)
{
    std::cout << animal.getName() << " says " << animal.speak() << '\n';
}

int main()
{
    Cat cat{ "Fred" };
    Dog dog{ "Garbo" };

    report(cat);
    report(dog);

    return 0;
}
// Fred says Meow
// Garbo says Woof



// In programming, polymorphism refers to the ability of an entity to have multiple
// forms (the term “polymorphism” literally means “many forms”).
// Compile-time polymorphism refers to forms of polymorphism that are resolved by 
// the compiler. These include function overload resolution, as well as template resolution.
// Runtime polymorphism refers to forms of polymorphism that are resolved at runtime.
// This includes virtual function resolution.


#include <iostream>
#include <string_view>

class A
{
public:
    virtual std::string_view getName() const { return "A"; }
};

class B: public A
{
public:
    virtual std::string_view getName() const { return "B"; }
};

class C: public B
{
public:
    virtual std::string_view getName() const { return "C"; }
};

class D: public C
{
public:
    virtual std::string_view getName() const { return "D"; }
};

int main()
{
    C c {};
    A& rBase{ c };
    std::cout << "rBase is a " << rBase.getName() << '\n';// rBase is a C

    return 0;
}
//it is a C class object, so only between A and C will be checked, so D wont be called 



//Virtual function resolution only works when a member function is called through a pointer or reference to a class type object.
C c{};
std::cout << c.getName(); // will always call C::getName

A a { c }; // copies the A portion of c into a (don't do this)
std::cout << a.getName(); // will always call A::getName



//Under normal circumstances, the return type of a virtual function and its override must match. 
class Base
{
public:
    virtual int getValue() const { return 5; }
};

class Derived: public Base
{
public:
    virtual double getValue() const { return 6.78; }
};
//here, Derived::getValue() is not considered a matching override for Base::getValue() and compilation will fail.



//If you call a virtual function in a Base class destructor, it will always resolve to the Base class version of the function
//similarly for constructors
//downside for virtual function: resolving a virtual function call takes longer than resolving a regular one.
//also extra memory cause a new pointer is allocated for each object