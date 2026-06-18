class Cents
{
private:
    int m_cents{};
public:
    Cents(int cents=0)
        : m_cents{ cents }
    {
    }

    // Overloaded int cast
    operator int() const { return m_cents; }

    int getCents() const { return m_cents; }
    void setCents(int cents) { m_cents = cents; }
};

// Overloaded typecasts must be non-static members, and
// should be const so they can be used with const objects.
// Overloaded typecasts do not have explicit parameters, as there is no way to pass
// explicit arguments to them. They do still have a hidden *this parameter, pointing 
//to the implicit object (which is the object to be converted).
// Overloaded typecast do not declare a return type. The name of the conversion (e.g. int) 
//is used as the return type, as it is the only return type allowed. This prevents
// redundancy in the declaration



//now we can do this and the compiler wont error if we pass cents to function which expects int
std::cout << static_cast<int>(cents);



#include <iostream>

class Cents
{
private:
    int m_cents{};
public:
    Cents(int cents=0)
        : m_cents{ cents }
    {
    }

    explicit operator int() const { return m_cents; } // now marked as explicit

    int getCents() const { return m_cents; }
    void setCents(int cents) { m_cents = cents; }
};

class Dollars
{
private:
    int m_dollars{};
public:
    Dollars(int dollars=0)
        : m_dollars{ dollars }
    {
    }

    operator Cents() const { return Cents { m_dollars * 100 }; }
};

void printCents(Cents cents)
{
//  std::cout << cents;                   // no longer works because cents won't implicit convert to an int
    std::cout << static_cast<int>(cents); // we can use an explicit cast instead
}

int main()
{
    Dollars dollars{ 9 };
    printCents(dollars); // implicit conversion from Dollars to Cents okay because its not marked as explicit

    std::cout << '\n';

    return 0;
}