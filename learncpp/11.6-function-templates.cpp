template <typename T> // this is the template parameter declaration defining T as a type template parameter
T max(T x, T y) // this is the function template definition for max<T>
{
    return (x < y) ? y : x;
}
//The scope of a template parameter declaration is strictly limited to the function template (or class template) that follows
//can also do template <class T> above
