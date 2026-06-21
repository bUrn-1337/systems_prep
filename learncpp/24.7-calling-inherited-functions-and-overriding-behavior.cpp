//compiler will first see in the derived class to find a function that matches, if there are multiple
//then the best match, then in its parent, then in its parent and so on



//we can also change the access specifier by overriding in derived class
#include <iostream>

class Base
{
public:
    Base() { }

    void identify() const { std::cout << "Base::identify()\n"; }
};

class Derived: public Base
{
public:
    Derived() { }

    void identify() const
    {
        std::cout << "Derived::identify()\n";
        Base::identify(); // note call to Base::identify() here
    }
};

int main()
{
    Base base {};
    base.identify();

    Derived derived {};
    derived.identify();

    return 0;
}

// Base::identify()
// Derived::identify()
// Base::identify()



//to call a friend function or operator, need to do static cast because scope resolution wont work
#include <iostream>

class Base
{
public:
    Base() { }

	friend std::ostream& operator<< (std::ostream& out, const Base&)
	{
		out << "In Base\n";
		return out;
	}
};

class Derived: public Base
{
public:
    Derived() { }

 	friend std::ostream& operator<< (std::ostream& out, const Derived& d)
	{
		out << "In Derived\n";
		// static_cast Derived to a Base object, so we call the right version of operator<<
		out << static_cast<const Base&>(d);
		return out;
    }
};

int main()
{
    Derived derived {};

    std::cout << derived << '\n';

    return 0;
}



#include <iostream>

class Base
{
public:
    void print(int)    { std::cout << "Base::print(int)\n"; }
    void print(double) { std::cout << "Base::print(double)\n"; }
};

class Derived: public Base
{
public:
    using Base::print; // make all Base::print() functions eligible for overload resolution, without this, Derived::print(double) would be called
    void print(double) { std::cout << "Derived::print(double)"; }
};


int main()
{
    Derived d{};
    d.print(5); // calls Base::print(int), which is the best matching function visible in Derived

    return 0;
}