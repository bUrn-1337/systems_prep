| Chapter | Topic | HFT Relevance | Feeds Into |
|---------|-------|---------------|------------|
| Ch 25 | Polymorphism / vtable | CRITICAL | vtable → icache miss → branch misprediction → latency spike |
| Ch 22 | Smart pointers & move semantics | CRITICAL | ref-count → atomic CAS → cache-line ping-pong → alloc-free path |
| Ch 12 | References & pointers / lvalue-rvalue | CRITICAL | foundation of move semantics → zero-copy message passing |
| Ch 19 | Dynamic allocation (new/delete) | CRITICAL | operator new → glibc malloc → memory pools → custom allocators |
| Ch 16 | std::vector, move semantics intro | CRITICAL | resize → heap alloc mid-hot-path → pre-reservation pattern |
| Ch 26 | Template specialization | CRITICAL | zero-cost abstraction → compile-time dispatch replaces vtable |
| Ch 15 | Advanced class features (static, dtors) | CRITICAL | memory layout → destructor cost → singleton / static init order |
| Ch 11 | Function overloading & templates | CRITICAL | SFINAE → concepts → policy-based design → no runtime cost |
| Ch 14 | Classes & constructors | CRITICAL | copy elision (RVO/NRVO) → eliminates hidden copies on hot path |
| Ch 21 | Operator overloading | CRITICAL | rule of 3/5/0 → assignment on critical path → deep-copy trap |
| Ch 24 | Inheritance | CRITICAL | vtable construction order → vptr bugs on Graviton / cross-TU |
| Ch 5 | Constants (constexpr, as-if rule) | USEFUL | constexpr → compile-time eval → no load, no branch |
| Ch 7 | Scope & linkage (inline, static local) | USEFUL | ODR, inline variables, static-local init → thread-safety of singletons |
| Ch 10 | Type conversion | USEFUL | implicit narrowing → silent data loss in price/quantity fields |
| Ch 13 | Enums & structs | USEFUL | struct layout → padding → false sharing → cache-line alignment |
| Ch 4 | Data types (fixed-width ints, size_t) | USEFUL | uint32_t vs int → ABI stability, size_t on LP64 → loop index width |
| Ch 17 | std::array & C-style arrays | USEFUL | stack-allocated fixed buffer → no heap → deterministic latency |
| Ch 20 | Advanced functions (lambdas, fn ptrs) | USEFUL | lambda → inline functor → zero overhead vs fn pointer indirection |
| Ch 23 | Object relationships | LOW | composition over inheritance → no vtable, but design-level only |
| Ch 2 | Functions, scope, preprocessor | LOW | basic building blocks; this does not matter for systems roles |
| Ch 1 | Variable assignment & initialization | SKIP | this does not matter for systems roles |

---
## Ch 1 — Variable Assignment and Initialization

**HFT relevance**: SKIP — this does not matter for systems roles.

Understand value-init vs default-init to avoid UB. Nothing here is systems-level.

---
## Ch 2 — Functions, Scope, Preprocessor

**HFT relevance**: LOW — this does not matter for systems roles.

Namespaces matter for ODR; `#pragma once` vs include guards is a build-time concern only. Forward declarations reduce compile times in large TUs.

---
## Ch 4 — Data Types (Fixed-Width Integers, size_t)

**What it is**: Guarantees exact bit-widths for integers across platforms and compilers.

**Minimal syntax**
```cpp
#include <cstdint>
uint32_t price_raw;   // always 32 bits, no surprises
uint64_t seq_no;      // monotonic counter, no sign extension
size_t   idx;         // matches pointer width on LP64
```

**HFT relevance**
USEFUL
- Wire protocols (ITCH, SBE, FAST) define fields in exact byte widths. Using `int` instead of `uint32_t` can silently mismatch on a platform where `int` is 16-bit, or introduce sign-extension bugs when casting to 64-bit. `size_t` is 8 bytes on LP64; using `int` as a loop index over a >2B element array is UB.

**Connected to**
→ Fixed-width types → SBE/ITCH message parsing → no sign-extension surprises → correct price reconstruction

**The one thing beginners get wrong**
Mixing `int` and `uint32_t` in arithmetic; the signed operand gets implicitly converted, potentially wrapping silently.

---
## Ch 5 — Constants (const, constexpr, as-if rule, string_view)

**What it is**: Mechanisms to move computation from runtime to compile time and allow the optimizer to elide loads/branches.

**Minimal syntax**
```cpp
constexpr int LEVELS = 10;           // compile-time constant, no memory load
constexpr double TICK = 0.01;        // inlined as immediate by compiler
const     int x = runtime_val();     // runtime const, still lives in memory
std::string_view sv = "BTC-USD";     // non-owning ref, no heap alloc
```

**HFT relevance**
USEFUL
- `constexpr` values are folded into the instruction stream as immediates — the CPU never loads from memory for them. The as-if rule licenses the compiler to reorder/elide any code whose observable effect is unchanged; understanding it explains why `volatile` is not a synchronization primitive and why "obvious" timing loops get deleted. `string_view` matters because it avoids SSO/heap copies on the hot message-parse path.

**Connected to**
→ `constexpr` → compiler substitutes immediate operand → no cache line loaded → zero latency for the constant access

**The one thing beginners get wrong**
Assuming `const` means the value is baked in. A `const int` initialized from a function call still loads from the stack; only `constexpr` guarantees compile-time folding.

---
## Ch 7 — Scope and Linkage (inline, static local, internal/external linkage)

**What it is**: Rules governing where names are visible and how translation units share or isolate definitions.

**Minimal syntax**
```cpp
inline int g_counter = 0;         // C++17: one definition across TUs
static int file_local = 0;        // internal linkage, not exported
static int& get_singleton() {
    static int instance = 0;      // guaranteed-once init (magic static)
    return instance;
}
```

**HFT relevance**
USEFUL
- `inline` variables (C++17) solve the ODR problem for header-defined globals without the extern/definition split. Static-local ("magic static") initialization is guaranteed thread-safe by the standard (Meyers singleton) but the first call takes a branch + possible mutex; in latency-critical paths the singleton must be warmed up before the trading session. Internal linkage (`static` at file scope) prevents the linker from merging symbols, which matters for LTO correctness.

**Connected to**
→ Static-local init → compiler emits guard variable check → branch + possible lock on first call → warm up before market open

**The one thing beginners get wrong**
Thinking `static` at function scope and `static` at file scope do the same thing. File-scope `static` is about linkage; function-scope `static` is about lifetime.

---
## Ch 10 — Type Conversion (implicit, narrowing, typedefs, auto)

**What it is**: The rules by which the compiler silently converts between arithmetic and pointer types.

**Minimal syntax**
```cpp
uint32_t price = 100500;
int64_t  pnl   = price;           // OK: widening
uint32_t back  = (uint32_t)pnl;   // explicit narrow — verify range
auto     it    = v.begin();       // auto deduces iterator, avoids verbose type
```

**HFT relevance**
USEFUL
- Implicit narrowing in price/quantity arithmetic is a source of silent production bugs. A `double` order price narrowed to `float` loses 7 significant digits — at 5-decimal price precision that is a real mismatch. `auto` reduces verbosity but can mask the actual type (e.g., `auto x = a - b` where both are `uint32_t` yields `uint32_t` — wraps on underflow rather than giving a negative integer).

**Connected to**
→ Narrowing conversion → wrong order price sent to exchange → fill at bad price → P&L event

**The one thing beginners get wrong**
`auto` on arithmetic expressions involving mixed signed/unsigned types; the result type follows promotion rules, not intuition.

---
## Ch 11 — Function Overloading and Templates

**What it is**: Compile-time mechanism to generate type-specialized code with zero runtime dispatch overhead.

**Minimal syntax**
```cpp
template<typename T>
T clamp(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

template<int N>          // non-type param
struct RingBuffer { T data[N]; };   // stack-allocated, size known at compile time

template<typename T>
void process(T&&) = delete;         // deletion blocks unwanted instantiations
```

**HFT relevance**
CRITICAL
- Templates are the mechanism behind zero-cost abstractions. A template function produces a fully specialized, inlinable definition per type; the compiler sees through it and can vectorize, constant-fold, and eliminate branches that a virtual dispatch cannot. Non-type template parameters allow ring-buffer sizes and message-type IDs to be compile-time constants, enabling the compiler to use immediate-mode addressing. SFINAE and `if constexpr` allow policy selection at compile time — the runtime binary has no branching overhead for features that are statically disabled.

**Connected to**
→ Template instantiation → fully specialized machine code → compiler inlines & vectorizes → no indirect call, no icache miss → 1-3 ns vs 5-15 ns for virtual call

**The one thing beginners get wrong**
Thinking templates are "just macros." The instantiated function is a real function with its own debug symbols, mangled name, and can be inlined across TU boundaries under LTO.

---
## Ch 12 — References and Pointers (lvalue/rvalue, const refs, pass by address/ref)

**What it is**: The value-category system that underpins move semantics and zero-copy design.

**Minimal syntax**
```cpp
int&  lref = x;          // binds to named object
int&& rref = std::move(x); // binds to expiring value
void f(Widget&& w);      // sink: takes ownership, no copy
const std::string& s;    // read-only view, no copy, extends temporary lifetime
```

**HFT relevance**
CRITICAL

**Connected to**
Understanding lvalue vs rvalue categories is the prerequisite for everything in Ch 22. An rvalue reference is not just syntax — it is a contract with the compiler: "this object's resources may be pillaged." When a market-data message (potentially hundreds of bytes) moves through a pipeline of handler objects, the difference between copy-and-pass and move-and-pass is the difference between O(N) heap allocations per message and O(0). On a 10 Gbps feed handler processing 5 million messages per second, every unnecessary `std::string` copy triggers a `malloc`/`free` pair. On a NUMA system those allocations may land on a remote node, adding 100+ ns of memory latency per message. The lvalue/rvalue distinction is also why `const T&` can bind to a temporary (the compiler extends the temporary's lifetime to match the reference) — this is the mechanism that allows chained expressions to avoid intermediate heap objects. Incorrect lifetime extension is also one of the most common dangling-reference bugs in production HFT code.

**The one thing beginners get wrong**
After `std::move(x)`, `x` is in a valid-but-unspecified state — not necessarily empty or zero. Reading from a moved-from object is UB in class types unless the class documents the post-move state.

---
## Ch 13 — Enums and Structs

**What it is**: User-defined aggregate types and strongly-typed enumerations; the basis of message layout.

**Minimal syntax**
```cpp
enum class Side : uint8_t { Buy = 0, Sell = 1 };  // no implicit int, 1 byte
struct alignas(64) OrderEntry {   // align to cache line
    uint64_t order_id;
    uint32_t price_raw;
    uint32_t qty;
    Side     side;
    uint8_t  _pad[7];             // explicit padding to avoid false sharing
};
```

**HFT relevance**
USEFUL
- Struct layout determines cache-line occupancy. Two hot fields on the same 64-byte line = one cache miss. Two unrelated fields that different threads write on the same line = false sharing = silent serialization. `alignas(64)` forces cache-line alignment. Scoped enums with explicit underlying type (`uint8_t`) guarantee wire size and prevent accidental comparison with unrelated integer values.

**Connected to**
→ Struct layout → cache-line packing → false sharing between writer and reader threads → serialization without a mutex

**The one thing beginners get wrong**
The compiler inserts padding silently. `sizeof(struct{ char a; int b; })` is 8, not 5. Always `static_assert` the size of wire-format structs.

---
## Ch 14 — Classes and Constructors

**What it is**: The rules governing object construction, initialization order, and copy elision.

**Minimal syntax**
```cpp
struct Foo {
    int x;
    explicit Foo(int v) : x{v} {}    // member init list, not assignment
};
// Copy elision / RVO
Foo make() { return Foo{42}; }       // object constructed directly in caller's storage
Foo f = make();                      // zero copies — guaranteed in C++17
```

**HFT relevance**
CRITICAL
- Guaranteed copy elision (NRVO/RVO, mandatory since C++17) means factory functions that return large objects by value incur zero copy cost — the object is constructed in place in the caller's frame. The consequence: returning a `Foo` from a factory is as cheap as constructing it directly. `explicit` on single-argument constructors prevents implicit conversion chains that can silently trigger expensive temporaries. Member initializer lists initialize directly (one construction) rather than default-construct-then-assign (two operations). For an object created millions of times per second, this difference accumulates.

**Connected to**
→ RVO → no temporary on stack → no copy constructor call → no extra cache writes → cheaper hot path

**The one thing beginners get wrong**
Assuming the compiler always applies RVO. NRVO (named RVO) is not guaranteed in C++17 — only prvalue elision is. Returning different named objects on different branches defeats NRVO; the compiler falls back to move construction.

---
## Ch 15 — Advanced Class Features (this, destructors, static members, friend)

**What it is**: The lifecycle and storage-class mechanics of class objects: when they die, what storage they occupy, and how static state is managed.

**Minimal syntax**
```cpp
struct Pool {
    static Pool& get() {
        static Pool inst;   // magic static: thread-safe, one init
        return inst;
    }
    ~Pool() { /* flush/release */ }   // destructor: predictable call site
};
static_assert(std::is_trivially_destructible_v<HotMsg>);
```

**HFT relevance**
CRITICAL

**Connected to**
Destructors are the hidden cost of `}`. Every time an object with a non-trivial destructor goes out of scope, the compiler emits a call to that destructor — and on the hot path this call may hit cold code, evict instruction cache lines, and, if the destructor frees memory, trigger `free()` which acquires a lock inside glibc. The standard library itself is full of non-trivially-destructible types: `std::string`, `std::vector`, `std::function`. HFT code either avoids them on the hot path or ensures they are destroyed on a background thread. `static` data members have static storage duration — they live in the BSS or data segment and are accessed via an absolute address, which is typically one instruction cheaper than a stack-relative access. The static-local singleton pattern (Meyers singleton) relies on a compiler-generated guard variable; the first call is not latency-safe because the standard permits a mutex to protect concurrent first-initialization. This means singletons must be eagerly initialized during the warm-up phase before the trading session begins.

**The one thing beginners get wrong**
Assuming a trivial-looking destructor is free. Even `~unique_ptr<T>()` calls `delete`, which calls `operator delete`, which calls `free`, which may spin on a lock.

---
## Ch 16 — std::vector, Move Semantics Intro, Capacity

**What it is**: A heap-allocated contiguous array with amortized O(1) push_back via geometric capacity growth.

**Minimal syntax**
```cpp
std::vector<Order> orders;
orders.reserve(1024);          // ONE allocation up front, no realloc on hot path
orders.push_back(o);           // O(1) amortized; O(N) if realloc triggers
orders.emplace_back(args...);  // construct in place, no move from temporary
```

**HFT relevance**
CRITICAL

**Connected to**
`std::vector` is the canonical example of why "amortized O(1)" is not the same as "deterministic O(1)." When capacity is exhausted, `push_back` calls `operator new` (which calls `malloc`), copies or moves all existing elements to the new buffer, then calls `operator delete` on the old buffer. On a feed handler that processes 5M messages/second, a reallocation at message 1025 introduces a multi-microsecond stall — visible as a latency spike in tick-to-trade measurements. The fix is always `reserve()` before the trading session with the maximum expected depth. Beyond allocation: vectors store data contiguously, so iteration is cache-friendly (sequential prefetcher fires). An `std::list` of the same elements has one heap allocation per node, scattered in virtual address space, defeating the prefetcher. This is why HFT order books are implemented as flat arrays (or `std::array`) rather than linked lists.

**The one thing beginners get wrong**
Calling `resize()` instead of `reserve()`. `resize()` value-initializes elements and changes `size()` — you can accidentally iterate over uninitialized-but-zeroed garbage. `reserve()` only changes capacity; `size()` stays the same.

---
## Ch 17 — std::array and C-Style Arrays

**What it is**: Fixed-size arrays with stack (or static) allocation and no dynamic memory involvement.

**Minimal syntax**
```cpp
std::array<double, 10> bid_px{};   // stack-allocated, zero-initialized
double raw[10];                    // C-style, same layout, no bounds checking
// array decay: raw == &raw[0], loses size info
void f(double* p, int n);          // size lost at call site
```

**HFT relevance**
USEFUL
- `std::array` and C arrays have deterministic allocation cost (zero for stack, paid once at program start for static). For order book levels, tick tables, or any fixed-depth structure, `std::array` is preferable to `std::vector` because there is no heap involvement and the compiler knows the size at compile time, enabling auto-vectorization. C-array decay (`double[]` → `double*`) is dangerous because it loses the size, leading to buffer overruns in message parsers.

**Connected to**
→ `std::array` on stack → no malloc → deterministic latency → compiler can unroll/vectorize fixed-size loops

**The one thing beginners get wrong**
Passing `std::array<T, N>` by value to a function — copies all N elements. Always pass by `const&` or use a `std::span`.

---
## Ch 19 — Dynamic Allocation (new/delete, Memory Pools)

**What it is**: The interface between C++ and the OS memory subsystem; the entry point for custom allocators.

**Minimal syntax**
```cpp
void* operator new(size_t n) {    // override global allocator
    return pool.alloc(n);
}
void operator delete(void* p) noexcept {
    pool.free(p);
}
// placement new: construct into pre-allocated buffer
new (buf) Order{price, qty};
```

**HFT relevance**
CRITICAL
- Every `new` expression ultimately calls `operator new`, which by default calls glibc `malloc`. `malloc` acquires an internal lock, searches free lists, may call `mmap`/`brk` to extend the heap, and touches cold memory. In the worst case this is hundreds of nanoseconds. HFT systems replace `operator new` globally (or per-class) with a slab/arena allocator that never calls into the OS after startup. Placement new allows constructing objects in pre-allocated regions (NUMA-local, huge-page, shared-memory) without any allocator overhead. Understanding the `new`/`delete` contract is the prerequisite for writing lock-free memory pools.

**Connected to**
→ `new` → `malloc` → lock + free-list walk → possible `mmap` → TLB miss → hundreds of ns → pool allocator eliminates all of this

**The one thing beginners get wrong**
Overriding `operator new` but forgetting the matching `operator delete` — results in the default `free` being called on pool memory, corrupting the pool.

---
## Ch 20 — Advanced Functions (Function Pointers, Lambdas, Captures)

**What it is**: First-class callable objects and the mechanics of capturing state without heap allocation.

**Minimal syntax**
```cpp
auto handler = [&book](const MsgHeader& h) noexcept {
    book.apply(h);               // capture by ref: zero overhead
};
// std::function<void(const MsgHeader&)> — AVOID: heap alloc + virtual call
using FnPtr = void(*)(const MsgHeader&);  // bare fn ptr: one indirection
```

**HFT relevance**
USEFUL
- A non-capturing lambda is zero overhead — the compiler turns it into a regular function and inlines it. A capturing lambda that fits in the small-buffer optimization of `std::function` triggers a heap allocation and stores a vtable pointer for type erasure. On the hot message-dispatch path, `std::function` is measurably slower than a template parameter or a bare function pointer. Policy-based design (pass the callable as a template parameter) is the zero-cost alternative.

**Connected to**
→ `std::function` → small-buffer overflow → heap alloc + vtable → indirect call + icache miss → 10–20 ns overhead vs inlined lambda

**The one thing beginners get wrong**
Capturing a local by reference in a lambda that outlives the local's scope — classic dangling reference that only crashes in production under load.

---
## Ch 21 — Operator Overloading

**What it is**: User-defined semantics for built-in operators; the mechanism behind the Rule of 3/5/0.

**Minimal syntax**
```cpp
struct Price {
    Price& operator=(const Price& o) { raw = o.raw; return *this; }  // copy
    Price& operator=(Price&& o) noexcept { raw = o.raw; return *this; } // move
    bool operator<(const Price& o) const noexcept { return raw < o.raw; }
};
```

**HFT relevance**
CRITICAL
- The Rule of 3/5/0 is a latency contract. If you define a destructor (Rule of 3), the compiler will not generate a move constructor — your object copies instead of moves, silently, everywhere. Failing to define `operator=(T&&) noexcept` means `std::vector` cannot move your objects during realloc; it copies them. `noexcept` on move and swap is load-bearing: `std::vector::push_back` checks `std::is_nothrow_move_constructible` and falls back to copy if absent. Every missed `noexcept` on a hot object is a potential O(N) copy cascade.

**Connected to**
→ Missing `noexcept` on move-assign → `std::vector` copies on realloc → O(N) heap traffic → latency spike on capacity growth

**The one thing beginners get wrong**
Defining copy-assign but not move-assign. The implicit move-assign is deleted when you define copy-assign in C++11/14, so the compiler silently falls back to copy everywhere a move was expected.

---
## Ch 22 — Smart Pointers and Move Semantics

**What it is**: RAII ownership wrappers and the rvalue-reference machinery that enables zero-copy transfer of resources.

**Minimal syntax**
```cpp
auto p = std::make_unique<Order>(args...);  // single ownership, no ref count
auto s = std::make_shared<Book>();          // ref-counted; avoid on hot path
std::unique_ptr<Order> q = std::move(p);   // ownership transfer, p is null
// rvalue ref
void sink(Widget&& w) { local = std::move(w); }
```

**HFT relevance**
CRITICAL

**Connected to**
`std::shared_ptr`'s reference count is a `std::atomic<int>` on the control block. Every copy of a `shared_ptr` is an atomic fetch-add; every destructor is an atomic fetch-sub followed by a compare-and-exchange. On x86 these are `LOCK XADD` instructions. On a multi-socket system, if two threads on different NUMA nodes hold copies of the same `shared_ptr`, every increment/decrement bounces the cache line owning the control block between the two sockets' L3 caches. This is a full cache-line invalidation round trip: ~100 ns per operation at 40–80 ns NUMA latency. At 5M messages/second, a single shared_ptr copy/destroy on the hot path adds ~500 µs of pure cache-coherence overhead per second. `std::unique_ptr` has zero overhead — it is a zero-cost abstraction that compiles to the same code as a raw pointer with `delete` — because there is no control block and no atomic operations. `std::move` does not move anything at runtime; it is a cast to rvalue reference that permits the move constructor to be called. The actual "move" is whatever the move constructor does — for `std::vector` it copies three pointers (24 bytes) and nulls the source, regardless of how many elements the vector contains.

**The one thing beginners get wrong**
Using `std::shared_ptr` as the default "safe pointer." In HFT the default should be `unique_ptr` or raw pointer with explicit lifetime; `shared_ptr` must be justified by an actual shared-ownership requirement and its atomic overhead must be profiled.

---
## Ch 23 — Object Relationships (Composition, Aggregation, Association)

**What it is**: Design-level taxonomy of how objects relate and own each other.

**HFT relevance**
LOW — this does not matter for systems roles in terms of runtime performance.

Prefer composition over inheritance to avoid vtables. `std::initializer_list` is a thin view over a stack-allocated array; using it as a parameter type is fine for small collections.

---
## Ch 24 — Inheritance

**What it is**: The mechanism for deriving classes and the rules governing construction/destruction order.

**Minimal syntax**
```cpp
struct Base { virtual ~Base() = default; int x; };
struct Derived : Base { int y; };
// Construction order: Base ctor → Derived ctor
// Destruction order: Derived dtor → Base dtor
// vptr is set by each ctor in the chain — calling virtual fn in ctor = UB
```

**HFT relevance**
CRITICAL
- Construction order matters for vptr initialization. During `Base::Base()`, the vptr points to `Base`'s vtable — not `Derived`'s. Calling a virtual function from a constructor dispatches to the base version, not the override. This is a common interview question targeting Graviton/ARM deployments where the ABI behavior under cross-TU vptr patching differs slightly from x86. Multiple inheritance introduces multiple vptrs and offset adjustments (`thunk` functions), adding indirection cost and complicating the memory layout. Single-inheritance chains are tolerable; diamond inheritance is never justified on a hot path.

**Connected to**
→ Inheritance → vtable → vptr per object → extra pointer in every object → icache pressure from thunks in multiple-inheritance

**The one thing beginners get wrong**
Calling a virtual method in a destructor. Same problem as in the constructor: the vptr has already been reset to the base class version; the override will never be called.

---
## Ch 25 — Polymorphism / Virtual Functions

**What it is**: Runtime dispatch through a per-class table of function pointers (vtable) looked up via a per-object pointer (vptr).

**Minimal syntax**
```cpp
struct Handler {
    virtual void on_trade(const Trade&) = 0;  // pure virtual
    virtual ~Handler() = default;              // virtual dtor mandatory
};
struct FastHandler final : Handler {
    void on_trade(const Trade&) override;      // final: devirtualization hint
};
```

**HFT relevance**
CRITICAL

**Connected to**
Every virtual call follows this chain: load vptr from object (1 cache miss if object is cold) → load function pointer from vtable at computed offset (1 cache miss if vtable is cold) → indirect call to function pointer (branch predictor has no static target → 1 branch-target-buffer lookup → possible misprediction penalty 15–20 cycles on modern x86). The icache now must hold both the dispatch trampoline and the callee. At 1 GHz effective throughput that is 3–5 ns just for the dispatch mechanism, before the function body executes. In contrast, a non-virtual call or an inlined template function has a static call target that the CPU's branch predictor learns after a handful of executions — effectively 0 ns dispatch overhead. The `final` keyword on a class or method is a devirtualization hint: the compiler can prove no further override exists and may convert the virtual call to a direct call and inline it. This is why HFT codebases often mark concrete strategy and handler classes `final`. `dynamic_cast` is worse: it walks the type-info chain at runtime, touching potentially-cold memory proportional to the inheritance depth. It must never appear on a hot path. Object slicing (copying a `Derived` into a `Base` variable) silently discards the derived part and the vptr, causing the wrong virtual functions to be called — a production-incident-level bug that static analysis tools will not always catch.

**The one thing beginners get wrong**
Forgetting `virtual` on the base class destructor. Deleting a `Derived*` through a `Base*` without a virtual destructor is undefined behavior — the derived destructor never runs, leaking resources. The compiler will not warn by default.

---
## Ch 26 — Template Specialization

**What it is**: The ability to provide a custom template implementation for specific types or values, enabling compile-time dispatch.

**Minimal syntax**
```cpp
template<typename T> struct Codec { /* generic */ };
template<> struct Codec<uint32_t> { /* fast path for uint32 */ };

template<int N> struct Factorial { static constexpr int v = N * Factorial<N-1>::v; };
template<>      struct Factorial<0> { static constexpr int v = 1; };

template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
void encode(T val);   // SFINAE: only for integral types
```

**HFT relevance**
CRITICAL
- Full specialization is the mechanism for type-based dispatch with zero runtime cost. A `Codec<uint32_t>` specialization that uses SIMD intrinsics is selected entirely at compile time — the executable contains only the specialized version for each instantiated type. This replaces what would otherwise be a virtual function table with a direct call (or inline). Partial specialization on non-type parameters enables metaprogramming tricks like loop unrolling at compile time. `std::is_trivially_copyable`, `std::is_nothrow_move_constructible`, and similar traits are implemented via specialization — they gate optimizations inside `std::vector`, `std::sort`, and the standard algorithm library.

**Connected to**
→ Explicit specialization → compiler selects correct overload at instantiation → no vtable, no runtime branch → inlinable → SIMD-vectorizable

**The one thing beginners get wrong**
Partially specializing a function template (which is not allowed in C++). Partial specialization works only on class/struct templates. For function templates, use overloading or `if constexpr` inside a primary template.
