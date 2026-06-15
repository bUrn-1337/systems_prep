#include <iostream>

struct Date
{
    int year {};
    int month {};
    int day {};

    void print()
    {
        std::cout << year << '/' << month << '/' << day;
    }
};

int main()
{
    const Date today { 2020, 10, 14 }; // const

    today.print();  // compile error: can't call non-const member function

    return 0;
}




//A const member function is a member function that guarantees it will not
// modify the object or call any non-const member functions (as they may modify the object).
//A const member function that attempts to change a data member or call a non-const member
// function will cause a compiler error to occur.
#include <iostream>

struct Date
{
    int year {};
    int month {};
    int day {};

    void print() const // now a const member function
    {
        std::cout << year << '/' << month << '/' << day;
    }
};

int main()
{
    const Date today { 2020, 10, 14 }; // const

    today.print();  // ok: const object can call const member function

    return 0;
}

//when passing object by const reference cant call non const member functions


//it is possible to overload a member function to have a const and non-const version of the same function. 
//This works because the const qualifier is considered part of the function’s signature, so two functions which 
//differ only in their const-ness are considered distinct