// override and final are not keywords, they are referred to as specifiers

// a derived class virtual function is only considered an override if its 
// signature and return types match exactly. so we might expect a function 
// to be override but it might turn out to be not

#include <iostream>
#include <string_view>

class A
{
public:
	virtual std::string_view getName1(int x) { return "A"; }
	virtual std::string_view getName2(int x) { return "A"; }
};

class B : public A
{
public:
	virtual std::string_view getName1(short x) { return "B"; } // note: parameter is a short
	virtual std::string_view getName2(int x) const { return "B"; } // note: function is const
};

int main()
{
	B b{};
	A& rBase{ b };
	std::cout << rBase.getName1(1) << '\n'; //A
	std::cout << rBase.getName2(2) << '\n'; //A

	return 0;
}

//const must come before override
//If a function marked as override does not override a base class function 
//(or is applied to a non-virtual function), the compiler will flag the function as an error

#include <string_view>

class A
{
public:
	virtual std::string_view getName1(int x) { return "A"; }
	virtual std::string_view getName2(int x) { return "A"; }
	virtual std::string_view getName3(int x) { return "A"; }
};

class B : public A
{
public:
	std::string_view getName1(short int x) override { return "B"; } // compile error, function is not an override
	std::string_view getName2(int x) const override { return "B"; } // compile error, function is not an override
	std::string_view getName3(int x) override { return "B"; } // okay, function is an override of A::getName3(int)

};

int main()
{
	return 0;
}
// the override specifier implies virtual, there’s no need to tag functions using the override specifier with the virtual keyword.



//if we want the user to not be able to override a function or inherit from a class, we use the final keyword after classname or inplace of override or after override
#include <string_view>

class A
{
public:
	virtual std::string_view getName() const { return "A"; }
};

class B : public A
{
public:
	// note use of final specifier on following line -- that makes this function not able to be overridden in derived classes
	std::string_view getName() const override final { return "B"; } // okay, overrides A::getName()
};

class C : public B
{
public:
	std::string_view getName() const override { return "C"; } // compile error: overrides B::getName(), which is final
};



#include <string_view>

class A
{
public:
	virtual std::string_view getName() const { return "A"; }
};

class B final : public A // note use of final specifier here
{
public:
	std::string_view getName() const override { return "B"; }
};

class C : public B // compile error: cannot inherit from final class
{
public:
	std::string_view getName() const override { return "C"; }
};