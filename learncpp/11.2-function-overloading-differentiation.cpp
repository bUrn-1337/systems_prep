//number of params:
int add(int x, int y);
int add(int x, int y, int z);

//types of params 
int add(int x, int y); // integer version
double add(double x, double y); // floating point version
double add(int x, double y); // mixed version
double add(double x, int y); // mixed version


typedef int Height; // typedef
using Age = int; // type alias

void print(int value);
void print(Age value); // not differentiated from print(int)
void print(Height value); // not differentiated from print(int)

void print(int);
void print(const int); // not differentiated from print(int)

//The return type of a function is not considered for differentiation


//A function’s type signature (generally called a signature) is defined as the parts of the function header that are used for differentiation of the function. In C++, this includes the function name, number of parameters, parameter type, and function-level qualifiers. It notably does not include the return type.


//When the compiler compiles a function, it performs name mangling, which means the compiled name of the function is altered (“mangled”) based on various criteria, such as the number and type of parameters, so that the linker has unique names to work with.
//For example, a function with prototype int fcn() might compile to mangled name __fcn_v, whereas int fcn(int) might compile to mangled name __fcn_i. So while in the source code, the two overloaded functions share the name fcn(), in compiled code, the mangled names are unique (__fcn_v vs __fcn_i).
//There is no standardization on how names should be mangled, so different compilers will produce different mangled names.