//Data members are not initialized by default, if we dont put the curly braces, they wont be initialised

//in cpp aggregate data types have 
// i) No user-declared constructors 
// ii) No private or protected non-static data members
// iii) No virtual functions


//two forms of aggregate initialisation:
struct Employee
{
    int id {};
    int age {};
    double wage {};
};

int main()
{
    Employee frank = { 1, 32, 60000.0 }; // copy-list initialization using braced list
    Employee joe { 2, 28, 45000.0 };     // list initialization using braced list (preferred)

    return 0;
}
//in c++20:
Employee robert ( 3, 45, 62500.0 );  // direct initialization using parenthesized list (C++20) (possible for some aggregates)

struct Employee
{
    int id {};
    int age {};
    double wage { 76000.0 }; //here wage will be initialised to this
    double whatever;
};

int main()
{
    Employee joe { 2, 28 }; // joe.whatever will be value-initialized to 0.0

    return 0;
}

//overloading to print :
#include <iostream>

struct Employee
{
    int id {};
    int age {};
    double wage {};
};

std::ostream& operator<<(std::ostream& out, const Employee& e)
{
    out << "id: " << e.id << " age: " << e.age << " wage: " << e.wage;
    return out;
}

int main()
{
    Employee joe { 2, 28 }; // joe.wage will be value-initialized to 0.0
    std::cout << joe << '\n';

    return 0;
}

//like other consts, const structt must be initialised
struct Rectangle
{
    double length {};
    double width {};
};

int main()
{
    const Rectangle unit { 1.0, 1.0 };
    const Rectangle zero { }; // value-initialize all members

    return 0;
}

//designated initialisation
struct Foo
{
    int a{ };
    int b{ };
    int c{ };
};

int main()
{
    Foo f1{ .a{ 1 }, .c{ 3 } }; // ok: f1.a = 1, f1.b = 0 (value initialized), f1.c = 3
    Foo f2{ .a = 1, .c = 3 };   // ok: f2.a = 1, f2.b = 0 (value initialized), f2.c = 3
    Foo f3{ .b{ 2 }, .a{ 1 } }; // error: initialization order does not match order of declaration in struct

    return 0;
}


struct Employee
{
    int id {};
    int age {};
    double wage {};
};

int main()
{
    Employee joe { 1, 32, 60000.0 };
    joe = { joe.id, 33, 66000.0 }; // Joe had a birthday and got a raise
    joe = { .id = joe.id, .age = 33, .wage = 66000.0 }; // Joe had a birthday and got a raise
    return 0;
}

//can also do this 
Foo foo { 1, 2, 3 };
Foo x = foo; // copy-initialization
Foo y(foo);  // direct-initialization
Foo z {foo}; // direct-list-initialization
