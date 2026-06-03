//In modern C++, the term inline has evolved to mean “multiple definitions are allowed”. Thus, an inline function is one that is allowed to be defined in multiple translation units (without violating the ODR).
//The following functions are implicitly inline:
//Functions defined inside a class, struct, or union type definition (14.3 -- Member functions).
//Constexpr / consteval functions (F.1 -- Constexpr functions).
//Functions implicitly instantiated from function templates (11.7 -- Function template instantiation).



//When a header containing an inline function is #included into a source file, that function definition will be compiled as part of that translation unit. An inline function #included into 6 translation units will have its definition compiled 6 times (before the linker deduplicates the definitions). Conversely, a function defined in a source file will have its definition compiled only once, no matter how many translation units its forward declaration is included into

//C++17 introduces inline variables, which are variables that are allowed to be defined in multiple files.
//The following variables are implicitly inline:
//Static constexpr data members 15.6 -- Static member variables.
//Unlike constexpr functions, constexpr variables are not inline by default (excepting those noted above)!