#include <iostream>
#include <string_view>

class Base
{
protected:
    int m_value{};

public:
    Base(int value)
        : m_value{ value }
    {
    }

    virtual ~Base() = default;

    virtual std::string_view getName() const { return "Base"; }
    int getValue() const { return m_value; }
};

class Derived: public Base
{
public:
    Derived(int value)
        : Base{ value }
    {
    }

   std::string_view getName() const override { return "Derived"; }
};
int main()
{
    Derived derived{ 5 };
    Base base{ derived }; // just the base portion of derived will get copied in base
    std::cout << "base is a " << base.getName() << " and has value " << base.getValue() << '\n';

    return 0;
}

// the assigning of a Derived class object to a Base class object is called object slicing
//Because base was and still is just a Base, Base’s virtual pointer still points to Base. Thus, base.getName() resolves to Base::getName().


//slicing can also occur if we pass by value to functions
void printName(const Base base); //like here if we pass derived
//fix is to pass by reference


//can also happens with vectors
#include <vector>

int main()
{
	std::vector<Base> v{};
	v.push_back(Base{ 5 });    // add a Base object to our vector
	v.push_back(Derived{ 6 }); // add a Derived object to our vector

        // Print out all of the elements in our vector
	for (const auto& element : v)
		std::cout << "I am a " << element.getName() << " with value " << element.getValue() << '\n';

	return 0;
}
// I am a Base with value 5
// I am a Base with value 6

//fix is to make vector of pointers or vector of reference wrapper



//slicing can also result in making a frankenobject
int main()
{
    Derived d1{ 5 };
    Derived d2{ 6 };
    Base& b{ d2 };

    b = d1; // this line is problematic

    return 0;
}

//here only the base portion of d1 will be copied in d2, here it is fine, but it can result in a Frankenobject, composed of parts of multiple objects.