//when there is not enough memory for the vector and we push_back,
//extra capacity is allocated for the vector, but how much depends on the implementation,
//gcc and clang double the current capacity



//reserve function changes the capacity but not length
#include <iostream>
#include <vector>

void printStack(const std::vector<int>& stack)
{
	if (stack.empty()) // if stack.size == 0
		std::cout << "Empty";

	for (auto element : stack)
		std::cout << element << ' ';

	// \t is a tab character, to help align the text
	std::cout << "\tCapacity: " << stack.capacity() << "  Length " << stack.size() << "\n";
}

int main()
{
	std::vector<int> stack{};

	printStack(stack);

	stack.reserve(6); // reserve space for 6 elements (but do not change length)
	printStack(stack);

	stack.push_back(1);
	printStack(stack);

	stack.push_back(2);
	printStack(stack);

	stack.push_back(3);
	printStack(stack);

	std::cout << "Top: " << stack.back() << '\n';

	stack.pop_back();
	printStack(stack);

	stack.pop_back();
	printStack(stack);

	stack.pop_back();
	printStack(stack);

	return 0;
}

// Empty   Capacity: 0  Length: 0
// Empty   Capacity: 6  Length: 0
// 1       Capacity: 6  Length: 1
// 1 2     Capacity: 6  Length: 2
// 1 2 3   Capacity: 6  Length: 3
// Top: 3
// 1 2     Capacity: 6  Length: 2
// 1       Capacity: 6  Length: 1
// Empty   Capacity: 6  Length: 0





//when we are pushing an existing object, push_back and emplace_back are the same in efficiency
//but for temporary variable, emplace is more efficient
//With emplace_back(), we don’t need to create a temporary object to pass. Instead, 
//we pass the arguments that would be used to create the temporary object, and 
//emplace_back() forwards them (using a feature called perfect forwarding) to 
//the vector, where they are used to create and initialize the object inside the 
//vector. This avoids a copy that would have otherwise been made.
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

class Foo
{
private:
    std::string m_a{};
    int m_b{};

public:
    Foo(std::string_view a, int b)
        : m_a { a }, m_b { b }
        {}

    explicit Foo(int b)
        : m_a {}, m_b { b }
        {};
};

int main()
{
	std::vector<Foo> stack{};

	// When we already have an object, push_back and emplace_back are similar in efficiency
	Foo f{ "a", 2 };
	stack.push_back(f);    // prefer this one
	stack.emplace_back(f);

	// When we need to create a temporary object to push, emplace_back is more efficient
	stack.push_back({ "a", 2 }); // creates a temporary object, and then copies it into the vector
	stack.emplace_back("a", 2);  // forwards the arguments so the object can be created directly in the vector (no copy made)

	// push_back won't use explicit constructors, emplace_back will
	stack.push_back({ 2 }); // compile error: Foo(int) is explicit
	stack.emplace_back(2);  // ok

	return 0;
}