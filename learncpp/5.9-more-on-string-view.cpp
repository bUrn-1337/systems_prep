#include <iostream>
#include <string>
#include <string_view>

std::string getName()
{
    std::string s { "Alex" };
    return s;
}

int main()
{
  std::string_view name { getName() }; // name initialized with return value of function
  std::cout << name << '\n'; // undefined behavior

  return 0;
}

#include <iostream>
#include <string>
#include <string_view>

int main()
{
    using namespace std::string_literals;
    std::string_view name { "Alex"s }; // "Alex"s creates a temporary std::string
    std::cout << name << '\n'; // undefined behavior

    return 0;
}


//Do not initialize a std::string_view with a std::string literal, as this will leave the std::string_view dangling.
//It is okay to initialize a std::string_view with a C-style string literal or a std::string_view literal. It’s also okay to initialize a std::string_view with a C-style string object, a std::string object, or a std::string_view object, as long as that string object outlives the view.



//std::string_view can be used as the return value of a function. However, this is often dangerous.
//Because local variables are destroyed at the end of the function, returning a std::string_view that is viewing a local variable will result in the returned std::string_view being invalid, and further use of that std::string_view will result in undefined behavior.


//because C-style string literals exist for the entire program, it’s fine (and useful) to return C-style string literals from a function that has a return type of std::string_view


//Because std::string_view is a view, it contains functions that let us modify our view by “closing the curtains”. This does not modify the string being viewed in any way, just the view itself.
//The remove_prefix() member function removes characters from the left side of the view.
//The remove_suffix() member function removes characters from the right side of the view.


//std::string_view may or may not be null-terminated

