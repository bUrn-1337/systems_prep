#include <iostream>

class IntList
{
private:
    int m_list[10]{};

public:
    int& operator[] (int index)
    {
        return m_list[index];
    }
};

/*
// Can also be implemented outside the class definition
int& IntList::operator[] (int index)
{
    return m_list[index];
}
*/

int main()
{
    IntList list{};
    list[2] = 3; // set a value
    std::cout << list[2] << '\n'; // get a value

    return 0;
}

//here we must return a reference because [] has higher precedence than =, 
//so it will evaluate first and the left side of = must be lvalue

#include <iostream>

class IntList
{
private:
    int m_list[10]{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }; // give this class some initial state for this example

public:
    // For non-const objects: can be used for assignment
    int& operator[] (int index)
    {
        return m_list[index];
    }

    // For const objects: can only be used for access
    // This function could also return by value if the type is cheap to copy
    const int& operator[] (int index) const
    {
        return m_list[index];
    }
};

int main()
{
    IntList list{};
    list[2] = 3; // okay: calls non-const version of operator[]
    std::cout << list[2] << '\n';

    const IntList clist{};
    // clist[2] = 3; // compile error: clist[2] returns const reference, which we can't assign to
    std::cout << clist[2] << '\n';

    return 0;
}
//for const objects, can just overload


//we can remove the repetition by using const_cast
#include <iostream>
#include <utility> // for std::as_const

class IntList
{
private:
    int m_list[10]{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }; // give this class some initial state for this example

public:
    int& operator[] (int index)
    {
        // use std::as_const to get a const version of `this` (as a reference)
        // so we can call the const version of operator[]
        // then const_cast to discard the const on the returned reference
        return const_cast<int&>(std::as_const(*this)[index]);
    }

    const int& operator[] (int index) const
    {
        return m_list[index];
    }
};

int main()
{
    IntList list{};
    list[2] = 3; // okay: calls non-const version of operator[]
    std::cout << list[2] << '\n';

    const IntList clist{};
    // clist[2] = 3; // compile error: clist[2] returns const reference, which we can't assign to
    std::cout << clist[2] << '\n';

    return 0;
}

//can do this to make sure the index is within bounds, inside the operator
assert(index >= 0 && static_cast<std::size_t>(index) < std::size(m_list));  
//can also use if statment and error handling cause assert is compiled out of non debug build


//If you try to call operator[] on a pointer to an object, C++ will assume you’re trying to index an array of objects of that type.
IntList* list{ new IntList{} };
list [2] = 3; // error: this will assume we're accessing index 2 of an array of IntLists
//instead do this 
IntList* list{ new IntList{} };
(*list)[2] = 3; // get our IntList object, then call overloaded operator[]
delete list;



//the operator can take any type indices, not just int
void operator[] (std::string_view index);