//The members of a struct are public by default
//The members of a class are private by default
// the above is the only technical difference between struct and class 

//cant even do aggregate initialisation for private members because if class has private members it doesnt qualify as an aggregate


//A class type is allowed to use any number of access specifiers in any order, and they can be used repeatedly (e.g. you can have some public members, then some private ones, then more public ones).



#include <iostream>
#include <string>
#include <string_view>

class Person
{
private:
    std::string m_name{};

public:
    void kisses(const Person& p) const
    {
        std::cout << m_name << " kisses " << p.m_name << '\n';
    }

    void setName(std::string_view name)
    {
        m_name = name;
    }
};

int main()
{
    Person joe;
    joe.setName("Joe");

    Person kate;
    kate.setName("Kate");

    joe.kisses(kate);

    return 0;
}

//access specifiers are class specific meaning an object can access another objects private members inside the class