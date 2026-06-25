#include <iostream>
#include <vector>

template <typename T>
void printReverse(const std::vector<T>& arr)
{
    for (std::size_t index{ arr.size() - 1 }; index >= 0; --index) // index is unsigned
    {
        std::cout << arr[index] << ' ';
    }

    std::cout << '\n';
}

int main()
{
    std::vector arr{ 4, 6, 7, 3, 8, 2, 1, 9 };

    printReverse(arr);

    return 0;
}
//here there is an issue, once index reaches 0, it will then wrap around because it is an unsigned type,
//then undefined behavior will result



//we might think of using size_type as the type of index variable, but its 
//too much to type, even more so for template functions
#include <iostream>
#include <vector>

template <typename T>
void printArray(const std::vector<T>& arr)
{
	// typename keyword prefix required for dependent type
	for (typename std::vector<T>::size_type index { 0 }; index < arr.size(); ++index)
		std::cout << arr[index] << ' ';
}

int main()
{
	std::vector arr { 9, 7, 5, 3, 1 };

	printArray(arr);

	return 0;
}
//Any name that depends on a type containing a template parameter is called a dependent name. 
//Dependent names must be prefixed with the keyword typename in order to be used as a type.



//can also do this
using arrayi = std::vector<int>;
for (arrayi::size_type index { 0 }; index < arr.size(); ++index)
//this is a more general approach for the above:
// arr is some non-reference type
for (decltype(arr)::size_type index { 0 }; index < arr.size(); ++index) // decltype(arr) resolves to std::vector<int>
//but if arr is a refernce type(means it is passed by refernce, this will fail), need to do this:
template <typename T>
void printArray(const std::vector<T>& arr)
{
	// arr can be a reference or non-reference type
	for (typename std::remove_reference_t<decltype(arr)>::size_type index { 0 }; index < arr.size(); ++index)
		std::cout << arr[index] << ' ';
}
//since size_type is almost always a typedef for size_t, just use this
for (std::size_t index { 0 }; index < arr.size(); ++index)
//this will work unless you are using custom allocators


//we can also used these 
using Index = std::ptrdiff_t;

// Sample loop using index
for (Index index{ 0 }; index < static_cast<Index>(arr.size()); ++index)