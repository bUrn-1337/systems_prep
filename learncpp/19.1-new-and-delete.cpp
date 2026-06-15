new int; // dynamically allocate an integer (and discard the result)
int* ptr{ new int }; // dynamically allocate an integer and assign the address to ptr so we can access it later
int* ptr1{ new int (5) }; // use direct initialization
int* ptr2{ new int { 6 } }; // use uniform initialization


// assume ptr has previously been allocated with operator new
delete ptr; // return the memory pointed to by ptr to the operating system
ptr = nullptr; // set ptr to be a null pointer
//delete doesnt actually delete the variable it just returns the memory back to os,
// we can reassingn the variable some other value



//A pointer that is pointing to deallocated memory is called a dangling pointer.
#include <iostream>

int main()
{
    int* ptr{ new int }; // dynamically allocate an integer
    *ptr = 7; // put a value in that memory location

    delete ptr; // return the memory to the operating system.  ptr is now a dangling pointer.

    std::cout << *ptr; // Dereferencing a dangling pointer will cause undefined behavior
    delete ptr; // trying to deallocate the memory again will also lead to undefined behavior.

    return 0;
}



//without no throw it will throw bad_alloc exception
int* value { new (std::nothrow) int }; // value will be set to a null pointer if the integer allocation fails



//memory leaks:

void doSomething()
{
    int* ptr{ new int{} };
}
//Memory leaks happen when your program loses the address of some bit of dynamically
// allocated memory before giving it back to the operating system. When this happens, 
//your program can’t delete the dynamically allocated memory, because it no longer knows
// where it is. The operating system also can’t use this memory, because that memory is 
//considered to be still in use by your program