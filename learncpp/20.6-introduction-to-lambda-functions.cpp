// [ captureClause ] ( parameters ) -> returnType
// {
//     statements;
// }
// this is the format for a lambda functon, if return type not give, it will be auto, deduced 
// capture clause is optional too
#include <iostream>

int main()
{
  [] {}; // a lambda with an omitted return type, no captures, and omitted parameters.

  return 0;
}
//eg
return std::all_of(array.begin(), array.end(), [](int i){ return ((i % 2) == 0); });
//can also do 
// we can store the lambda in a named variable and pass it to the function.
auto isEven{
  [](int i)
  {
    return (i % 2) == 0;
  }
};

return std::all_of(array.begin(), array.end(), isEven);

//In actuality, lambdas aren’t functions (which is part of how they avoid the
//limitation of C++ not supporting nested functions). They’re a special kind 
//of object called a functor. Functors are objects that contain an overloaded 
//operator() that make them callable like a function.


//If return type deduction is used, a lambda’s return type is deduced from the return-statements inside the lambda, and all return statements in the lambda must return the same type

//just read the chapter, better than writing everything here