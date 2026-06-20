// To qualify as an aggregation, a whole object and its parts must have the following relationship:
// The part (member) is part of the object (class)
// The part (member) can (if desired) belong to more than one object (class) at a time
// The part (member) does not have its existence managed by the object (class)
// The part (member) does not know about the existence of the object (class)


//e.g.
#include <iostream>
#include <string>
#include <string_view>

class Teacher
{
private:
  std::string m_name{};

public:
  Teacher(std::string_view name)
      : m_name{ name }
  {
  }

  const std::string& getName() const { return m_name; }
};

class Department
{
private:
  const Teacher& m_teacher; // This dept holds only one teacher for simplicity, but it could hold many teachers

public:
  Department(const Teacher& teacher)
      : m_teacher{ teacher }
  {
  }
};

int main()
{
  // Create a teacher outside the scope of the Department
  Teacher bob{ "Bob" }; // create a teacher

  {
    // Create a department and use the constructor parameter to pass
    // the teacher to it.
    Department department{ bob };

  } // department goes out of scope here and is destroyed

  // bob still exists here, but the department doesn't

  std::cout << bob.getName() << " still exists!\n";

  return 0;
}

//model relationships in the simplest way possible, need not be related to real life


std::vector<const Teacher&> m_teachers{}; // Illegal
#include <functional> // std::reference_wrapper
#include <iostream>
#include <vector>
#include <string>

int main()
{
  std::string tom{ "Tom" };
  std::string berta{ "Berta" };

  std::vector<std::reference_wrapper<std::string>> names{ tom, berta }; // these strings are stored by reference, not value
  // Vector of const references to std::string
  //std::vector<std::reference_wrapper<const std::string>> names{ tom, berta };
  std::string jim{ "Jim" };

  names.emplace_back(jim);

  for (auto name : names)
  {
    // Use the get() member function to get the referenced string.
    name.get() += " Beam";
  }

  std::cout << jim << '\n'; // prints Jim Beam

  return 0;
}