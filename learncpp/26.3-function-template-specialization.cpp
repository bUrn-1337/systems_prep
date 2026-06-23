//look at this template function
#include <iostream>

template <typename T>
void print(const T& t)
{
    std::cout << t << '\n';
}

int main()
{
    print(5);
    print(6.7);

    return 0;
}
//now imagine we want to define special behavior for type double, like print scientific notation
//one simple and preferred way is to define a non template function
void print(double d)
{
    std::cout << std::scientific << d << '\n';
}
//this will take precedence over any template function




//Another way to achieve a similar result is to use explicit template specialization. 
//Explicit template specialization (often shortened to template specialization) is a 
//feature that allows us to explicitly define different implementations of a template
// for specific types or values. When all of the template parameters are specialized,
// it is called a full specialization. When only some of the template parameters are 
//specialized, it is called a partial specialization.

#include <iostream>

// Here's our primary template (must come first)
template <typename T>
void print(const T& t)
{
    std::cout << t << '\n';
}

// A full specialization of primary template print<T> for type double
// Full specializations are not implicitly inline, so make this inline if put in header file
template<>                          // template parameter declaration containing no template parameters
void print<double>(const double& d) // specialized for type double
{
    std::cout << std::scientific << d << '\n';
}

int main()
{
    print(5);
    print(6.7);

    return 0;
}
//note, a non template function will take precedence over specialised template also