//Many new programmers are confused about whether constructors create the objects or not. They do not -- the compiler sets up the memory allocation for the object prior to the constructor call. The constructor is then called on the uninitialized object.
//aggregates are not allowed to have constructors -- so if you add a constructor to an aggregate, it is no longer an aggregate.
//constructors must have same name as class and no return type
//constructor must be non const
// a non-const member function can’t be invoked on a const object. However, the C++ standard explicitly states
// that const doesn’t apply to an object under construction, and only comes into effect after the constructor ends.