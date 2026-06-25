# HFT Prep Knowledge Base Index

## Repository Structure

```
systems_prep/
├── INDEX.md           (this file)
├── README.md
├── COA/               (Computer Organization & Architecture — in progress)
├── OS/                (Operating Systems — in progress)
└── learncpp/          (C++ language fundamentals)
```

---

## learncpp/ — C++ Language Fundamentals

### Chapter 1-2: Basics
- `1.4` Variable assignment and initialization
- `2.2` Value-returning functions
- `2.3` Void functions
- `2.4` Function parameters
- `2.5` Local scope
- `2.9` Namespaces
- `2.10` Introduction to preprocessor

### Chapter 4-5: Types, Constants, Strings
- `4.6` Fixed-width integers and size_t
- `5.1` Constant variables
- `5.2` Literals
- `5.3` Numeral systems
- `5.4` As-if rule and compile-time optimization
- `5.6` constexpr variables
- `5.7` Strings
- `5.8` string_view
- `5.9` More on string_view

### Chapter 7: Linkage and Static Storage
- `7.2` User-defined namespaces
- `7.6` Internal linkage
- `7.7` External linkage and variable forward declarations
- `7.9` Inline functions and variables
- `7.11` Static local variables

### Chapter 10: Type Conversions
- `10.1` Implicit type conversion
- `10.2` Floating-point and integral promotion
- `10.4` Narrowing conversions, list initializers, constexpr initializers
- `10.7` Typedefs and type aliases
- `10.8` Type deduction with auto
- `10.9` Function type definitions

### Chapter 11: Function Overloading and Templates
- `11.1` Function overloading
- `11.2` Function overloading differentiation
- `11.3` Function overload resolution
- `11.4` Function deletion
- `11.5` Default arguments; overloading input/output operators
- `11.6` Function templates
- `11.7` Function template instantiation
- `11.8` Templates with multiple types
- `11.9` Non-type template parameters
- `11.10` Function templates in multiple files

### Chapter 12: References, Pointers, and Pass Semantics
- `12.1` Compound data types
- `12.2` Lvalue and rvalue expressions
- `12.3` Lvalue references
- `12.4` Const lvalue references
- `12.5` Pass by lvalue reference
- `12.6` Pass by const lvalue reference
- `12.7` Pointers
- `12.8` Null pointers
- `12.9` Pointers and const
- `12.10` Pass by address
- `12.11` Pass by address (part 2)
- `12.12` Return by reference and address
- `12.13` In and out parameters
- `12.14` Type deduction with pointers, references, and const
- `12.15` std::optional

### Chapter 13: Structs, Enums, and Class Templates
- `13.1` Program-defined types
- `13.2` Unscoped enumerations
- `13.3` Unscoped enumerator integral conversions
- `13.4` Enumeration to string
- `13.6` Scoped enumerations (enum class)
- `13.7` Intro to structs and struct members
- `13.8` Struct aggregate initialization
- `13.9` Default member initialization
- `13.13` Class templates
- `13.14` Class template argument deduction (CTAD)
- `13.15` Alias templates

### Chapter 14: Classes and Constructors
- `14.3` Member functions
- `14.4` Const class objects and const member functions
- `14.5` Access specifiers
- `14.6` Access functions (getters/setters)
- `14.7` Member functions returning references to data members
- `14.9` Intro to constructors
- `14.10` Constructor member initialization list
- `14.11` Default constructors and default arguments
- `14.12` Delegating constructors
- `14.13` Temporary class objects
- `14.14` Introduction to copy constructor
- `14.15` Class initialization and copy elision
- `14.16` Converting constructors and the explicit keyword
- `14.17` constexpr aggregates and classes

### Chapter 15: Advanced Class Features
- `15.1` this pointer and member function chaining
- `15.2` Classes and header files
- `15.3` Nested types
- `15.4` Introduction to destructors
- `15.5` Class templates with member functions
- `15.6` Static member variables
- `15.7` Static member functions
- `15.8` Friend non-member functions
- `15.9` Friend classes and friend member functions
- `15.10` Ref qualifiers

### Chapter 16: std::vector and Dynamic Arrays
- `16.2` Introduction to vectors and list constructors
- `16.3` std::vector and unsigned length/subscript issues
- `16.4` Passing std::vector
- `16.5` Returning std::vector and intro to move semantics
- `16.7` Arrays, loops, and sign challenge solutions
- `16.8` Range-based for loops
- `16.9` Array indexing and length using enumerators
- `16.10` std::vector resizing and capacity
- `16.11` std::vector and stack behavior
- `16.12` std::vector\<bool\>

### Chapter 17: std::array and C-Style Arrays
- `17.1` std::array
- `17.2` std::array length and subscript
- `17.3` Passing and returning std::arrays
- `17.4` std::array of class types and brace elision
- `17.5` Array of references with std::reference_wrapper
- `17.7` C-style arrays
- `17.8` C-style array decay
- `17.10` C-style strings
- `17.11` C-style string symbolic constants
- `17.12` Multidimensional C-style arrays

### Chapter 19: Dynamic Memory
- `19.1` new and delete
- `19.2` Dynamically allocating arrays
- `19.3` Destructors
- `19.4` Pointers to pointers and dynamic multidimensional arrays
- `19.5` Void pointers

### Chapter 20: Advanced Functions
- `20.1` Function pointers
- `20.3` Recursion
- `20.4` Command-line arguments
- `20.5` Ellipsis (variadic functions)
- `20.6` Introduction to lambda functions
- `20.7` Lambda captures

### Chapter 21: Operator Overloading
- `21.2` Operator overloading with friend functions
- `21.3` Overloading with normal functions
- `21.4` Overloading input/output operators
- `21.5` Overloading using member functions
- `21.6` Overloading unary operators
- `21.7` Overloading relational operators
- `21.8` Overloading increment and decrement operators
- `21.9` Overloading the subscript operator
- `21.10` Overloading the parenthesis operator
- `21.11` Overloading typecasts
- `21.12` Overloading the assignment operator
- `21.13` Shallow and deep copying

### Chapter 22: Smart Pointers and Move Semantics
- `22.1` Introduction to smart pointers and move semantics
- `22.2` Rvalue references
- `22.3` Move constructors and move assignment
- `22.4` std::move
- `22.5` std::unique_ptr
- `22.6` std::shared_ptr
- `22.7` Circular dependency issues with std::shared_ptr and weak_ptr

### Chapter 23: Object Relationships
- `23.2` Composition
- `23.3` Aggregation
- `23.4` Association
- `23.5` Dependencies
- `23.7` std::initializer_list

### Chapter 24: Inheritance
- `24.2` Basic inheritance in C++
- `24.3` Order of construction of derived classes
- `24.4` Construction and initialization of derived classes
- `24.5` Inheritance and access specifiers
- `24.6` Adding new functionality to a derived class
- `24.7` Calling inherited functions and overriding behavior
- `24.8` Hiding inherited functionality
- `24.9` Multiple inheritance

### Chapter 25: Virtual Functions and Polymorphism
- `25.1` Pointers and references to base classes of derived objects
- `25.2` Virtual functions and polymorphism
- `25.3` override and final specifiers; covariant return types
- `25.4` Virtual destructors and overriding virtualization
- `25.5` Early binding and late binding
- `25.7` Pure virtual functions, abstract base classes, and interfaces
- `25.9` Object slicing
- `25.10` Dynamic casting
- `25.11` Printing inherited classes

### Chapter 26: Advanced Templates
- `26.1` Template classes
- `26.2` Template non-type parameters
- `26.3` Function template specialization
- `26.4` Class template specialization
- `26.5` Partial template specialization
- `26.6` Partial template specialization for pointers

---

## COA/ — Computer Organization and Architecture
_(directory initialized, content coming soon)_

Topics to cover:
- CPU pipeline stages and hazards
- Cache hierarchy (L1/L2/L3), cache coherence (MESI)
- Memory ordering and memory barriers
- Branch prediction
- SIMD / vectorization
- CPU affinity and NUMA

---

## OS/ — Operating Systems
_(directory initialized, content coming soon)_

Topics to cover:
- Process vs thread, context switching cost
- Kernel vs user space, syscall overhead
- Memory-mapped files and huge pages
- Lock-free data structures, atomics, compare-and-swap
- Spinlocks vs mutexes
- CPU scheduling (SCHED_FIFO, SCHED_RR, isolcpus)
- Interrupt handling and IRQ affinity
- DPDK / kernel bypass networking basics

---

## HFT-Relevant C++ Topics to Prioritize

| Topic | Chapter(s) |
|---|---|
| constexpr / compile-time computation | 5.4, 5.6, 14.17 |
| Move semantics and perfect forwarding | 22.1 - 22.4 |
| Smart pointers (ownership, no GC pauses) | 22.5 - 22.7 |
| Template metaprogramming | 11.6-11.10, 26.1-26.6 |
| Inline functions (zero-overhead abstraction) | 7.9 |
| Static storage / static members | 7.11, 15.6, 15.7 |
| Operator overloading (custom types, no virtual) | 21.x |
| Virtual dispatch cost awareness | 25.2, 25.5 |
| Memory layout: structs, padding, alignment | 13.7 - 13.9 |
| Rvalue refs / avoid copies on hot path | 22.2, 22.3 |
