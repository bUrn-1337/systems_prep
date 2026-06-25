// std::vector<bool> has a fairly high amount of overhead (sizeof(std::vector<bool>) is 40 bytes on the author’s machine), so you won’t save memory unless you’re allocating more Boolean values than the overhead for your architecture.
//std::vector<bool> is not a vector (it is not required to be contiguous in memory), nor does it hold bool values (it holds a collection of bits), nor does it meet C++’s definition of a container.

template<typename T>
void foo( std::vector<T>& v )
{
    T& first = v[0]; // get a reference to the first element
    // Do something with first
}
//this doesnt work with std::vector<bool>