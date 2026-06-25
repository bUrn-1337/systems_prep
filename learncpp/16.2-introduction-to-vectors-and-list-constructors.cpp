#include <vector>

int main()
{
	// List construction (uses list constructor)
	std::vector<int> primes{ 2, 3, 5, 7 };          // vector containing 4 int elements with values 2, 3, 5, and 7
	std::vector vowels { 'a', 'e', 'i', 'o', 'u' }; // vector containing 5 char elements with values 'a', 'e', 'i', 'o', and 'u'.  Uses CTAD (C++17) to deduce element type char (preferred).

	return 0;
}
std::vector<int> data( 10 ); // vector containing 10 int elements, value-initialized to 0
//this constructor must be called using direct initialisation

// Copy init
std::vector<int> v1 = 10;     // 10 not an initializer list, copy init won't match explicit constructor: compilation error

// Direct init
std::vector<int> v2(10);      // 10 not an initializer list, matches explicit single-argument constructor

// List init
std::vector<int> v3{ 10 };    // { 10 } interpreted as initializer list, matches list constructor

// Copy list init
std::vector<int> v4 = { 10 }; // { 10 } interpreted as initializer list, matches list constructor
std::vector<int> v5({ 10 });  // { 10 } interpreted as initializer list, matches list constructor

// Default init
std::vector<int> v6 {};       // {} is empty initializer list, matches default constructor
std::vector<int> v7 = {};     // {} is empty initializer list, matches default constructor

//v5 is the same as v4, it is an alternate syntax for copy list initialisation



#include <vector>

int main()
{
    const std::vector<int> prime { 2, 3, 5, 7, 11 }; // prime and its elements cannot be modified

    return 0;
}
//but we cant do std::vector<const int>, it is not allowed, standard library containers were not desgined to have 
//const elements
//vectors cant be made constexpr