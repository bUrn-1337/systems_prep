// Global variable forward declarations (extern w/ no initializer):
extern int g_y;                 // forward declaration for non-constant global variable
extern const int g_y;           // forward declaration for const global variable
extern constexpr int g_y;       // not allowed: constexpr variables can't be forward declared

// External global variable definitions (no extern)
int g_x;                        // defines non-initialized external global variable (zero initialized by default)
int g_x { 1 };                  // defines initialized external global variable

// External const global variable definitions (extern w/ initializer)
extern const int g_x { 2 };     // defines initialized const external global variable
extern constexpr int g_x { 3 }; // defines initialized constexpr external global variable


//Variables forward declarations do need the extern keyword to help differentiate uninitialized variables definitions from variable forward declarations 

// Although constexpr variables can be given external linkage via the extern keyword, they can not be forward declared as constexpr. This is because the compiler needs to know the value of the constexpr variable (at compile time). If that value is defined in some other file, the compiler has no visibility on what value was defined in that other file.
// However, you can forward declare a constexpr variable as const, which the compiler will treat as a runtime const. This isn’t particularly useful.