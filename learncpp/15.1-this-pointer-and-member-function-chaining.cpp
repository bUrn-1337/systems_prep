void print() const { std::cout << m_id; }       // implicit use of this
void print() const { std::cout << this->m_id; } // explicit use of this
//compiler changes the top one into the bottom one

simple.setID(2);
Simple::setID(&simple, 2); // note that simple has been changed from an object prefix to a function argument!
//commpiler rewrites the top into the bottom

void setID(int id) { m_id = id; }
static void setID(Simple* const this, int id) { this->m_id = id; }
//same here 
//this is a const pointer, cant be repointed but can change the value
//All non-static member functions have a this const pointer that holds the address of the implicit object.



struct Something
{
    int data{}; // not using m_ prefix because this is a struct

    void setData(int data)
    {
        this->data = data; // this->data is the member, data is the local parameter
    }
};
//can use this pointer to disambiguate



//we can return *this to do method chaining
class Calc
{
private:
    int m_value{};

public:
    Calc& add(int value) { m_value += value; return *this; }
    Calc& sub(int value) { m_value -= value; return *this; }
    Calc& mult(int value) { m_value *= value; return *this; }

    int getValue() const { return m_value; }
};
#include <iostream>

int main()
{
    Calc calc{};
    calc.add(5).sub(3).mult(4); // method chaining

    std::cout << calc.getValue() << '\n';

    return 0;
}