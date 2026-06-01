int main()
{
    constexpr int x { expr }; // Because variable x is constexpr, expr must be evaluatable at compile-time
}

// The use of language features that result in compile-time evaluation is called compile-time programming.
