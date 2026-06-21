#include <iostream>

class Base
{
protected:
    int m_value {};

public:
    Base(int value)
        : m_value { value }
    {
    }

    void identify() const { std::cout << "I am a Base\n"; }
};

class Derived: public Base
{
public:
    Derived(int value)
        : Base { value }
    {
    }

    int getValue() const { return m_value; } //just add the function in derived class
};

int main()
{
    Derived derived { 5 };
    std::cout << "derived has value " << derived.getValue() << '\n';
    Base base { 5 };
    std::cout << "base has value " << base.getValue() << '\n';//doesnt work

    return 0;
}