// Move semantics is an optimization that allows us, under certain circumstances, 
//to inexpensively transfer ownership of some data members from one object to another
// object (rather than making a more expensive copy).
// Data members that can’t be moved are copied instead.



//We can return move-capable types (like std::vector and std::string) by value.
// Such types will inexpensively move their values instead of making an expensive copy.
//Such types should still be passed by const reference.