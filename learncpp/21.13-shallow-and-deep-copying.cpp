//default copy assignment operators and default copy constructors do shallow copying, so 
//if we have some dynamic memory allocation and a pointer member, they will just copy the pointer
//without allocating new memory
#include <cstring> // for strlen()
#include <cassert> // for assert()

class MyString
{
private:
    char* m_data{};
    int m_length{};

public:
    MyString(const char* source = "" )
    {
        assert(source); // make sure source isn't a null string

        // Find the length of the string
        // Plus one character for a terminator
        m_length = std::strlen(source) + 1;

        // Allocate a buffer equal to this length
        m_data = new char[m_length];

        // Copy the parameter string into our internal buffer
        for (int i{ 0 }; i < m_length; ++i)
            m_data[i] = source[i];
    }

    ~MyString() // destructor
    {
        // We need to deallocate our string
        delete[] m_data;
    }

    char* getString() { return m_data; }
    int getLength() { return m_length; }
};




#include <iostream>

int main()
{
    MyString hello{ "Hello, world!" };
    {
        MyString copy{ hello }; // use default copy constructor
    } // copy is a local variable, so it gets destroyed here.  The destructor deletes copy's string, which leaves hello with a dangling pointer

    std::cout << hello.getString() << '\n'; // this will have undefined behavior

    return 0;
}
// shallow copying will cause problems here
//One answer to this problem is to do a deep copy on any non-null pointers being copied.
// A deep copy allocates memory for the copy and then copies the actual value, so that
// the copy lives in distinct memory from the source. This way, the copy and source are
// distinct and will not affect each other in any way. Doing deep copies requires that 
//we write our own copy constructors and overloaded assignment operators.