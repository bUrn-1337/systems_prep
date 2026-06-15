#include <iostream>

class Foo
{
private:
    int m_x {};
    int m_y {};

public:

    // You must use a direct form of initialization here (preferably using braces, but parentheses works as well)
    // -- using copy initialization (with an equals) does not work here.
    Foo(int x, int y)
        : m_x { x }, m_y { y } // here's our member initialization list, doesnt matter where you put commas, colons, can be in next line also
    {
        std::cout << "Foo(" << x << ", " << y << ") constructed\n";
    }

    void print() const
    {
        std::cout << "Foo(" << m_x << ", " << m_y << ")\n";
    }
};

int main()
{
    Foo foo{ 6, 7 };
    foo.print();

    return 0;
}


//members are initialised in the order they are defined in the definition of the class, this will produce unexpted results
#include <algorithm> // for std::max
#include <iostream>

class Foo
{
private:
    int m_x{};
    int m_y{};

public:
    Foo(int x, int y)
        : m_y { std::max(x, y) }, m_x { m_y } // issue on this line
    {
    }

    void print() const
    {
        std::cout << "Foo(" << m_x << ", " << m_y << ")\n";//Foo(-858993460, 7)
    }
};

int main()
{
    Foo foo { 6, 7 };
    foo.print();

    return 0;
}


//this is wrong for a constructor
 Foo(int x, int y)
    {
        m_x = x; // incorrect: this is an assignment, not an initialization
        m_y = y; // incorrect: this is an assignment, not an initialization
    }