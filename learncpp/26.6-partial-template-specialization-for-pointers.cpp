#include <iostream>

template <typename T>
class Storage // This is our primary template class (same as previous)
{
private:
    T m_value {};
public:
    Storage(T value)
      : m_value { value }
    {
    }

    void print()
    {
        std::cout << m_value << '\n';
    }
};

template <typename T> // we still have a type template parameter
class Storage<T*> // This is partially specialized for T*
{
private:
    T* m_value {};
public:
    Storage(T* value)
      : m_value { value }
    {
    }

    void print();
};

template <typename T>
void Storage<T*>::print() // This is a non-specialized function of partially specialized class Storage<T*>
{
    if (m_value)
        std::cout << std::scientific << *m_value << '\n';
}

int main()
{
    double d { 1.2 };
    double *ptr { &d };

    Storage s { ptr }; // instantiates Storage<double*> from partially specialized class
    s.print(); // calls Storage<double*>::print()

    return 0;
}
//note that after typename we have used T no T*