int* ptr1, ptr2;   // incorrect: ptr1 is a pointer to an int, but ptr2 is just a plain int!
int* ptr3, * ptr4; // correct: ptr3 and ptr4 are both pointers to an int
// A pointer that has not been initialized is sometimes called a wild pointer. Wild pointers contain a garbage address, and dereferencing a wild pointer will result in undefined behavior.
int main()
{
    int x{ 5 };
    int* ptr;        // an uninitialized pointer (holds a garbage address)
    int* ptr2{};     // a null pointer (we'll discuss these in the next lesson)
    int* ptr3{ &x }; // a pointer initialized with the address of variable x
    return 0;
}
int main()
{
    int i{ 5 };
    double d{ 7.0 };

    int* iPtr{ &i };     // ok: a pointer to an int can point to an int object
    int* iPtr2 { &d };   // not okay: a pointer to an int can't point to a double object
    double* dPtr{ &d };  // ok: a pointer to a double can point to a double object
    double* dPtr2{ &i }; // not okay: a pointer to a double can't point to an int object

    return 0;
}

int* ptr{ 5 }; // not okay
int* ptr{ 0x0012FF7C }; // not okay, 0x0012FF7C is treated as an integer 

// References must be initialized, pointers are not required to be initialized (but should be).
// References are not objects, pointers are.
// References can not be reseated (changed to reference something else), pointers can change what they are pointing at.
// References must always be bound to an object, pointers can point to nothing (we’ll see an example of this in the next lesson).
// References are “safe” (outside of dangling references), pointers are inherently dangerous (we’ll also discuss this in the next lesson).

//address-of operator (&) doesn’t return the address of its operand as a literal (as C++ doesn’t support address literals). Instead, it returns a pointer to the operand (whose value is the address of the operand).


//Dereferencing an invalid pointer will lead to undefined behavior. Any other use of an invalid pointer value is implementation-defined