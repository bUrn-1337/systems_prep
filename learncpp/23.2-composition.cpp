// process of building complex objects from simpler ones is called object composition.
//When we build classes with data members, we’re essentially constructing a complex
//object from simpler parts, which is object composition. For this reason, structs
//and classes are sometimes referred to as composite types.
//There are two basic subtypes of object composition: composition(we'll call it object composition) and aggregation.


// To qualify as a composition, an object and a part must have the following relationship:
// The part (member) is part of the object (class)
// The part (member) can only belong to one object (class) at a time
// The part (member) has its existence managed by the object (class)
// The part (member) does not know about the existence of the object (class) (we call this unidirectional relationship

//While object composition models has-a type relationships (a body has-a heart, a
// fraction has-a denominator), we can be more precise and say that composition 
//models “part-of” relationships (a heart is part-of a body, a numerator is part
// of a fraction). Composition is often used to model physical relationships, where
// one object is physically contained inside another.

//examples
class Fraction
{
private:
	int m_numerator;
	int m_denominator;

public:
	Fraction(int numerator=0, int denominator=1)
		: m_numerator{ numerator }, m_denominator{ denominator }
	{
	}
};
//The parts of an object composition can be singular or multiplicative -- for example, 
//a heart is a singular part of the body, but a body contains 10 fingers (which could be modeled as an array).



//Note that composition has nothing to say about the transferability of parts. A heart can be transplanted from one body to another. However, even after being transplanted, it still meets the requirements for a composition (the heart is now owned by the recipient, and can only be part of the recipient object unless transferred again).