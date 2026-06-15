//A copy constructor is a constructor that is used to initialize an object with an
// existing object of the same type. After the copy constructor executes, the newly 
//created object should be a copy of the object passed in as the initializer.


//If you do not provide a copy constructor for your classes, C++ will create a public implicit copy constructor for you.
//Fraction fCopy { f };
// this^ is invoking the implicit copy constructor to initialize fCopy with f.

//we can also define copy constructor
#include <iostream>

class Fraction
{
private:
    int m_numerator{ 0 };
    int m_denominator{ 1 };

public:
    // Default constructor
    Fraction(int numerator=0, int denominator=1)
        : m_numerator{numerator}, m_denominator{denominator}
    {
    }

    // Copy constructor
    Fraction(const Fraction& fraction)
        // Initialize our members using the corresponding member of the parameter
        : m_numerator{ fraction.m_numerator }
        , m_denominator{ fraction.m_denominator }
    {
        std::cout << "Copy constructor called\n"; // just to prove it works
    }

    void print() const
    {
        std::cout << "Fraction(" << m_numerator << ", " << m_denominator << ")\n";
    }
};

int main()
{
    Fraction f { 5, 3 };  // Calls Fraction(int, int) constructor
    Fraction fCopy { f }; // Calls Fraction(const Fraction&) copy constructor

    f.print();
    fCopy.print();

    return 0;
}
//A copy constructor should not do anything other than copy an object. 
//This is because the compiler may optimize the copy constructor out in certain cases. 


//The copy constructor’s parameter must be a reference
//when doing pass by value or return by value and the types are same of the temporary object 
//and return type or parameter copy constructor is implicitly called


 // Explicitly request default copy constructor
    Fraction(const Fraction& fraction) = default;


 // Delete the copy constructor so no copies can be made
    Fraction(const Fraction& fraction) = delete;
//the objects will then not be copyable, when try to copy, compile error