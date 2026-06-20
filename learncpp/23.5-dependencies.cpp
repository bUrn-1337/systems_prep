//A dependency occurs when one object invokes another object’s functionality 
//in order to accomplish some specific task. This is a weaker relationship 
//than an association, but still, any change to object being depended upon
// may break functionality in the (dependent) caller. A dependency is always
// a unidirectional relationship.



//associations are a relationship where one class keeps a direct or indirect “link” to the associated class as a member
//Dependencies typically are not members. Rather, the object being depended on is typically instantiated as needed (like opening a file to write data to), or passed into a function as a parameter


#include <iostream>

class Point
{
private:
    double m_x{};
    double m_y{};
    double m_z{};

public:
    Point(double x=0.0, double y=0.0, double z=0.0): m_x{x}, m_y{y}, m_z{z}
    {
    }

    friend std::ostream& operator<< (std::ostream& out, const Point& point); // Point has a dependency on std::ostream here
};

std::ostream& operator<< (std::ostream& out, const Point& point)
{
    // Since operator<< is a friend of the Point class, we can access Point's members directly.
    out << "Point(" << point.m_x << ", " << point.m_y << ", " << point.m_z << ')';

    return out;
}

int main()
{
    Point point1 { 2.0, 3.0, 4.0 };

    std::cout << point1; // the program has a dependency on std::cout here

    return 0;
}