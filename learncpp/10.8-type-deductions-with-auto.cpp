int main()
{
    auto d { 5.0 }; // 5.0 is a double literal, so d will be deduced as a double
    auto i { 1 + 2 }; // 1 + 2 evaluates to an int, so i will be deduced as an int
    auto x { i }; // i is an int, so x will be deduced as an int

    return 0;
}

int a { 5 };            // a is an int

const auto b { 5 };     // b is a const int
constexpr auto c { 5 }; // c is a constexpr int


int main()
{
    const int a { 5 }; // a has type const int
    auto b { a };      // b has type int (const dropped)
    const auto b { a }; // b has type const int (const dropped but reapplied)
    return 0;
}

auto s { "Hello, world" }; // s will be type const char*, not std::string