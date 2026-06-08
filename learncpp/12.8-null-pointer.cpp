int main()
{
    int* ptr { nullptr }; // can use nullptr to initialize a pointer to be a null pointer

    int value { 5 };
    int* ptr2 { &value }; // ptr2 is a valid pointer
    ptr2 = nullptr; // Can assign nullptr to make the pointer a null pointer

    someFunction(nullptr); // we can also pass nullptr to a function that has a pointer parameter

    return 0;
}

//// pointers convert to Boolean false if they are null, and Boolean true if they are non-null
//Conditionals can only be used to differentiate null pointers from non-null pointers. There is no convenient way to determine whether a non-null pointer is pointing to a valid object or dangling (pointing to an invalid object).
//When an object is destroyed, any pointers to the destroyed object will be left dangling (they will not be automatically set to nullptr). It is your responsibility to detect these cases and ensure those pointers are subsequently set to nullptr.



//0 is used to indicate nullptr 
//th also ere is a preprocessor macro named NULL (defined in the <cstddef> header)