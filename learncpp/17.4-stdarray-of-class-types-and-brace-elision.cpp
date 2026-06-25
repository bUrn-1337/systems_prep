constexpr std::array houses {
        House{ 13, 1, 7 }, // we mention House here
        House{ 14, 2, 5 }, // and here
        House{ 15, 2, 4 }  // and here
    };

// doesn't work
constexpr std::array<House, 3> houses { // we're telling the compiler that each element is a House
        { 13, 1, 7 }, // but not mentioning it here
        { 14, 2, 5 },
        { 15, 2, 4 }
    };
//this doesnt work because std::array is a struct defined like 
template<typename T, std::size_t N>
struct array
{
    T implementation_defined_name[N]; // a C-style array with N elements of type T
}
//so when we do the above thing, compiler thinks { 13, 1, 7 } is the initializer for the first member i.e the c style array
//so we must do this:
// This works as expected
constexpr std::array<House, 3> houses { // initializer for houses
    { // extra set of braces to initialize the C-style array member with implementation_defined_name
        { 13, 4, 30 }, // initializer for array element 0
        { 14, 3, 10 }, // initializer for array element 1
        { 15, 3, 40 }, // initializer for array element 2
     }
};