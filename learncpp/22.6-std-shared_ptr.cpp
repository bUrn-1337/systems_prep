//it is fine to have multiple std::shared_ptr pointing to the same resource. 
//Internally, std::shared_ptr keeps track of how many std::shared_ptr are sharing
// the resource. As long as at least one std::shared_ptr is pointing to the resource,
// the resource will not be deallocated, even if individual std::shared_ptr are destroyed. 
//As soon as the last std::shared_ptr managing the resource goes out of scope (or is
// reassigned to point at something else), the resource will be deallocated.



#include <iostream>
#include <memory> // for std::shared_ptr

class Resource
{
public:
	Resource() { std::cout << "Resource acquired\n"; }
	~Resource() { std::cout << "Resource destroyed\n"; }
};

int main()
{
	// allocate a Resource object and have it owned by std::shared_ptr
	Resource* res { new Resource };
	std::shared_ptr<Resource> ptr1{ res };
	{
		std::shared_ptr<Resource> ptr2 { ptr1 }; // make another std::shared_ptr pointing to the same thing

		std::cout << "Killing one shared pointer\n";
	} // ptr2 goes out of scope here, but nothing happens

	std::cout << "Killing another shared pointer\n";

	return 0;
} // ptr1 goes out of scope here, and the allocated Resource is destroyed
// this is the correct way of using

#include <iostream>
#include <memory> // for std::shared_ptr

class Resource
{
public:
	Resource() { std::cout << "Resource acquired\n"; }
	~Resource() { std::cout << "Resource destroyed\n"; }
};

int main()
{
	Resource* res { new Resource };
	std::shared_ptr<Resource> ptr1 { res };
	{
		std::shared_ptr<Resource> ptr2 { res }; // create ptr2 directly from res (instead of ptr1)

		std::cout << "Killing one shared pointer\n";
	} // ptr2 goes out of scope here, and the allocated Resource is destroyed

	std::cout << "Killing another shared pointer\n";

	return 0;
} // ptr1 goes out of scope here, and the allocated Resource is destroyed again


//this program will crash because ptr1 and ptr2 are not aware of each other
// co managing the resource, always copy the ptr2 to ptr1 to make shared ptrs


//use make_shared, it is more performant, if we make shared ptr ourselves, then 
//control block and the object are allocated in two blocks, but when using
//make_shared, they are allocated in a single block

#include <iostream>
#include <memory> // for std::shared_ptr

class Resource
{
public:
	Resource() { std::cout << "Resource acquired\n"; }
	~Resource() { std::cout << "Resource destroyed\n"; }
};

int main()
{
	// allocate a Resource object and have it owned by std::shared_ptr
	auto ptr1 { std::make_shared<Resource>() };
	{
		auto ptr2 { ptr1 }; // create ptr2 using copy of ptr1

		std::cout << "Killing one shared pointer\n";
	} // ptr2 goes out of scope here, but nothing happens

	std::cout << "Killing another shared pointer\n";

	return 0;
} // ptr1 goes out of scope here, and the allocated Resource is destroyed


//A std::unique_ptr can be converted into a std::shared_ptr via a special 
//std::shared_ptr constructor that accepts a std::unique_ptr r-value

//if any single one of the shared ptrs is not properly disposed of, then a leak will result