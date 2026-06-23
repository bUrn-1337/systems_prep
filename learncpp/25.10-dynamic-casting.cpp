//if we have a base pointer to a derived object, but there is a function only in derived
//it is not a virtual function in base, so how do we access it
//or how do we convert base pointer to derived pointer


//here we do dynamic casting
// the most common use for dynamic casting is for converting base-class pointers into derived-class pointers. This process is called downcasting.

#include <iostream>
#include <string>
#include <string_view>

class Base
{
protected:
	int m_value{};

public:
	Base(int value)
		: m_value{value}
	{
	}

	virtual ~Base() = default;
};

class Derived : public Base
{
protected:
	std::string m_name{};

public:
	Derived(int value, std::string_view name)
		: Base{value}, m_name{name}
	{
	}

	const std::string& getName() const { return m_name; }
};

Base* getObject(bool returnDerived)
{
	if (returnDerived)
		return new Derived{1, "Apple"};
	else
		return new Base{2};
}

int main()
{
	Base* b{ getObject(true) };

	Derived* d{ dynamic_cast<Derived*>(b) }; // use dynamic cast to convert Base pointer into Derived pointer

	std::cout << "The name of the Derived is: " << d->getName() << '\n';

	delete b;

	return 0;
}



//if b is not pointing to a derived object, then dynamic casting will fail and return nullptr, so add this check
if (d) // make sure d is non-null
		std::cout << "The name of the Derived is: " << d->getName() << '\n';


//it fails in these cases
//With protected or private inheritance.
// For classes that do not declare or inherit any virtual functions (and thus don’t have a virtual table).
// In certain cases involving virtual base classes


//we can also use static_cast but it has no runtime type checking, so if the base pointer is not pointing to 
//a derived pointer, undefined behavior will result



//dynamic_cast can be done in same way for base refernce to derived
int main()
{
	Derived apple{1, "Apple"}; // create an apple
	Base& b{ apple }; // set base reference to object
	Derived& d{ dynamic_cast<Derived&>(b) }; // dynamic cast using a reference instead of a pointer

	std::cout << "The name of the Derived is: " << d.getName() << '\n'; // we can access Derived::getName through d

	return 0;
}


//also, converting derived to base is called upcasting