//you can do this 
int value { 5 };

int* ptr { &value };
std::cout << *ptr << '\n'; // Dereference pointer to int to get int value

int** ptrptr { &ptr };
std::cout << **ptrptr << '\n'; // dereference to get pointer to int, dereference again to get int value
//but not this 
int value { 5 };
int** ptrptr { &&value }; // not valid
//This is because the address of operator (operator&) requires an lvalue, but &value is an rvalue.
int** ptrptr { nullptr };



//how to make dynamic 2d arrays
int** array { new int*[10] }; // allocate an array of 10 int pointers — these are our rows
for (int count { 0 }; count < 10; ++count)
    array[count] = new int[5]; // these are our columns

//to delete 
for (int count { 0 }; count < 10; ++count)
    delete[] array[count];
delete[] array; // this needs to be done last