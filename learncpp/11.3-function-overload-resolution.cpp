//The process of matching function calls to a specific overloaded function is called overload resolution.


void foo(unsigned int)
{
}

void foo(float)
{
}

int main()
{
    foo(0);       // int can be numerically converted to unsigned int or to float
    foo(3.14159); // double can be numerically converted to unsigned int or to float

    return 0;
}

int x{ 0 };
foo(static_cast<unsigned int>(x)); // will call foo(unsigned int)
foo(0u); // will call foo(unsigned int) since 'u' suffix is unsigned int, so this is now an exact match