## Virtual Functions, vtable, vptr

**The mechanism by which C++ achieves runtime polymorphism through a per-class table of function pointers**

**What the interviewer actually asked**
"Draw the vtable for this class. Where does vptr live in memory? When is vptr set?" (DE Shaw, Graviton, Alphagrep)

**The shallow answer**
"vtable is an array of function pointers and vptr points to it."

**The deep answer**
- vptr lives at offset 0 of the object (on all major ABIs: Itanium ABI used by GCC/Clang, MSVC ABI). It is the first 8 bytes of every polymorphic object on 64-bit.
- vptr is set by the constructor — specifically, before any constructor body code runs, the compiler injects code to set vptr to the vtable of the class being constructed. If you have `Derived : Base`, the Base constructor runs first and sets vptr to Base's vtable, then Derived's constructor sets vptr to Derived's vtable. This is why calling a virtual function from a constructor does NOT dispatch to the derived override — the vptr is still pointing to the base vtable at that moment.
- vtable is per-class, not per-object. All objects of the same class share one vtable. vtable lives in the read-only data segment (.rodata).
- Virtual dispatch cost: one indirect load through vptr to get vtable address, then another indexed load to get function pointer, then an indirect call. If vtable is cold (not in L1/L2 cache), this is ~100 cycles. If hot, ~3-5 cycles. No virtual call can ever be devirtualized or inlined by the compiler (unless the compiler can prove the dynamic type statically — devirtualization is possible but rare).
- sizeof(Base) with any virtual function increases by exactly 8 bytes (one pointer) vs a non-polymorphic class.
- Diamond inheritance: each inheritance path gets its own vtable. Virtual inheritance adds a vbase offset into the vtable (or a vbtable on MSVC) so the object layout can be resolved at runtime.
- Multiple inheritance: object has multiple vptrs, one per base class that introduces virtual functions, at different offsets.

**Implementation requirement?**
YES — show the equivalent manual C layout:

```cpp
// C++ class with vtable
class Animal {
public:
    virtual void speak() { /* ... */ }
    virtual void move()  { /* ... */ }
    virtual ~Animal()    { /* ... */ }
    int age;             // offset 8 (after vptr)
};

class Dog : public Animal {
public:
    void speak() override { /* ... */ }
    // move() inherited from Animal
    ~Dog() override { /* ... */ }
    int breed_id;    // offset 12 (after vptr + age)
};

// -------------------------------------------------------
// Equivalent manual C (illustrates what the compiler generates)
// -------------------------------------------------------
struct AnimalVTable {
    void (*speak)(void* self);
    void (*move)(void* self);
    void (*dtor)(void* self);
};

struct Animal_C {
    const AnimalVTable* vptr;  // offset 0, 8 bytes
    int age;                   // offset 8, 4 bytes
    // 4 bytes padding to align to 8
};

// vtable stored in .rodata — one per class, not per object
void animal_speak(void* self) { /* base impl */ }
void animal_move(void* self)  { /* base impl */ }
void animal_dtor(void* self)  { /* base impl */ }

const AnimalVTable Animal_vtable = {
    animal_speak, animal_move, animal_dtor
};

// Dog's vtable — separate object in .rodata
void dog_speak(void* self) { /* Dog override */ }
void dog_dtor(void* self)  { /* Dog override */ }

const AnimalVTable Dog_vtable = {
    dog_speak,    // overridden
    animal_move,  // inherited — same pointer as Animal_vtable.move
    dog_dtor      // overridden
};

// Construction: Dog d; compiles to roughly:
// d.vptr = &Animal_vtable; Animal_C::Animal_C(&d);  // base ctor runs
// d.vptr = &Dog_vtable;    Dog_C::Dog_C(&d);        // derived ctor runs

// Virtual dispatch: d.speak(); compiles to roughly:
// d.vptr->speak(&d);
// Which is: (*((AnimalVTable*)*(uintptr_t*)&d)).speak(&d)
```

**The follow-up trap**
"What happens if you call a virtual function from the constructor?"
Answer: It does NOT call the derived override. vptr is set to the current class's vtable at the time the constructor runs. Base constructor sets vptr to Base's vtable. Calling virtual from Base constructor calls Base's implementation, even if a Derived object is being constructed. This is defined behavior (not UB) but almost always a logic bug.

**Key numbers / facts**
- vptr size: 8 bytes on 64-bit, 4 bytes on 32-bit
- vtable lookup on cold cache: ~100 cycles (L3 miss)
- vtable lookup on hot cache: ~3-5 cycles
- vtable lives in .rodata (read-only data segment)
- Itanium ABI (GCC/Clang): vptr at offset 0
- MSVC ABI: vptr at offset 0 for single inheritance, varies for multiple

---

## Virtual Destructors

**A virtual destructor ensures that deleting through a base pointer calls the derived class destructor**

**What the interviewer actually asked**
"What happens if the base class destructor is not virtual?" (DE Shaw, QuadEye)

**The shallow answer**
"The derived destructor won't be called, causing a memory leak."

**The deep answer**
- This is undefined behavior per the C++ standard (not merely "implementation-defined"). The behavior of `delete base_ptr` where base_ptr points to a Derived object and Base's destructor is not virtual is explicitly UB.
- In practice on all real ABIs: only `~Base()` is called. If Derived allocates heap memory, holds file handles, or manages any resource, those resources are leaked. If Derived's destructor has any side effects, they don't run.
- Destruction order with virtual dtor: Derived destructor runs FIRST, then Base destructor. This is always reverse construction order (Base ctor → Derived ctor → Derived dtor → Base dtor). The virtual dispatch ensures the most-derived destructor is called first via the vtable, then each base dtor is chained automatically by the compiler.
- Making any destructor virtual makes the class polymorphic: if the class had no other virtual functions, adding a virtual dtor introduces a vptr, adding 8 bytes to every instance.
- Rule of thumb (Scott Meyers): if a class has any virtual function, give it a virtual destructor. If a class is not meant to be a base class, do NOT make the destructor virtual (no cost, prevents accidental polymorphism).
- `= default` virtual dtor: `virtual ~Base() = default;` — explicitly defaulted, still virtual, still has correct behavior.
- Pure virtual destructor: a class can have `virtual ~Base() = 0;` (making Base abstract) but MUST still provide a definition (unlike other pure virtuals), because derived dtors will chain-call it.

**Implementation requirement?**
NO — but here is the illustrative code:

```cpp
struct Base {
    // BAD: no virtual destructor
    ~Base() { /* only this runs when deleting via Base* */ }
};

struct Derived : Base {
    int* data;
    Derived() : data(new int[100]) {}
    ~Derived() { delete[] data; }  // NEVER CALLED — resource leak + UB
};

Base* p = new Derived();
delete p;  // UB: only ~Base() called, data leaked

// -------------------------------------------------------

struct BaseGood {
    virtual ~BaseGood() = default;  // correct
};

struct DerivedGood : BaseGood {
    int* data;
    DerivedGood() : data(new int[100]) {}
    ~DerivedGood() override { delete[] data; }  // called correctly
};

BaseGood* p2 = new DerivedGood();
delete p2;  // correct: ~DerivedGood() then ~BaseGood()
```

**The follow-up trap**
"What if I store a `Derived` by value in a `Base` variable (not a pointer)?" 
Answer: That's object slicing — the Derived portion is silently discarded at construction time. The destructor issue doesn't apply here because there is no pointer indirection, but you've already lost the derived data. This is a different bug, equally dangerous.

**Key numbers / facts**
- UB (not implementation-defined) to delete Derived via non-virtual Base*
- Adding virtual dtor adds 8 bytes if no other virtual members exist (first vptr)
- Pure virtual dtor still requires a definition — compiler chains it automatically
- Destruction always: most-derived first, then each base in reverse construction order

---

## Empty Base Optimization (EBO)

**The compiler is permitted to give a base class subobject zero size when it is an empty class, provided it doesn't conflict with object identity rules**

**What the interviewer actually asked**
"sizeof(EmptyClass) — what is it and why?" and "What is Empty Base Optimization?" (DE Shaw, Alphagrep)

**The shallow answer**
"sizeof is 1 because the standard says so."

**The deep answer**
- `sizeof` any complete object type must be at least 1. Reason: if two distinct objects of type `EmptyClass` could have the same address, pointer arithmetic and object identity would break. `char arr[2]` requires `arr[0]` and `arr[1]` to have distinct addresses, so each element must be at least 1 byte.
- EBO: When `Derived` inherits from an empty `Base`, the Base subobject is allowed to overlap with the first byte of the Derived object's own storage. The Base subobject still exists, but takes 0 additional space. `sizeof(Derived)` = `sizeof(Derived's own members)` (rounded up to alignment), not `sizeof(Base) + sizeof(Derived's members)`.
- EBO applies when: the base class is empty (no non-static data members, no virtual functions), and the first member of the derived class is not of the same type as the base (avoids two subobjects at the same address for the same type).
- Real-world use: `std::unique_ptr<T, Deleter>` uses EBO to store the Deleter. When `Deleter` is `std::default_delete<T>` (empty struct), `sizeof(unique_ptr<T>)` == `sizeof(T*)` == 8. Without EBO it would be 16 (pointer + deleter). `std::tuple` uses EBO recursively for all its elements. Allocator-aware containers store their allocators via EBO.
- C++20 `[[no_unique_address]]`: applies the same optimization to data members (not just bases). Allows a member of empty type to share address with another member or the object itself.

**Implementation requirement?**
NO — but illustrative:

```cpp
struct Empty {};
static_assert(sizeof(Empty) == 1);

struct WithMember {
    Empty e;  // 1 byte + padding
    int x;    // 4 bytes
};
static_assert(sizeof(WithMember) == 8);  // 1 (e) + 3 (pad) + 4 (x)

struct DerivedFromEmpty : Empty {
    int x;   // 4 bytes
};
// EBO: Base subobject takes 0 bytes, sizeof == 4
static_assert(sizeof(DerivedFromEmpty) == 4);

// unique_ptr uses EBO for deleter:
// sizeof(std::unique_ptr<int>) == 8  (just the pointer)
// sizeof(std::unique_ptr<int, CustomDeleter>) == 8 if CustomDeleter is empty
// sizeof(std::unique_ptr<int, CustomDeleter>) == 16 if CustomDeleter has state

// C++20: [[no_unique_address]] for members
struct C20Example {
    [[no_unique_address]] Empty e;
    int x;
};
static_assert(sizeof(C20Example) == 4);  // same as EBO
```

**The follow-up trap**
"If EBO saves memory in unique_ptr, why not always inherit from the deleter instead of storing it as a member?"
Answer: You can't inherit from an arbitrary user-supplied type safely — it might have a virtual destructor (making your class polymorphic unintentionally), or it might be final. The `[[no_unique_address]]` attribute in C++20 solves this cleanly without inheritance.

**Key numbers / facts**
- `sizeof` any object: minimum 1 byte
- EBO: empty base subobject collapses to 0 bytes in derived
- `sizeof(std::unique_ptr<T>)` == 8 with stateless deleter (EBO in effect)
- `std::tuple` applies EBO recursively
- C++20: `[[no_unique_address]]` extends this to data members

---

## Static Keyword — All 4 Uses

**The most overloaded keyword in C++, with four entirely distinct meanings depending on context**

**What the interviewer actually asked**
"List every meaning of static in C++." (DE Shaw)

**The shallow answer**
"Static means the variable persists, or it means it belongs to the class."

**The deep answer**

**(a) Static local variable**
Initialized exactly once, on first pass through the declaration. Lifetime extends until program exit. In C++11 and later, initialization is guaranteed thread-safe (the standard mandates a lock around the first initialization — typically implemented with a double-checked lock + a guard byte). After initialization, no locking. Useful for function-local singletons. Destroyed in reverse order of construction at program exit.

**(b) Static member variable**
One copy per class, shared across all instances. Not stored inside any object — lives in the global data segment (.data or .bss). Must be defined (and optionally initialized) outside the class body in exactly one translation unit (before C++17). C++17 inline static allows definition inside the class. Accessed via `ClassName::member` or through an object (but object is irrelevant).

**(c) Static member function**
No `this` pointer. Cannot access non-static members directly. Cannot be `const` or `virtual`. Can be called without an object: `ClassName::func()`. Used for factory functions, utility functions that logically belong to the class but don't operate on an instance, and as callbacks to C APIs that require a function pointer (since non-static member functions have an implicit `this` parameter incompatible with a raw function pointer).

**(d) Static free function or variable (translation-unit scope)**
Gives internal linkage: the name is not visible outside the current translation unit (.cpp file). The linker will not find it from other translation units. Equivalent to putting the declaration inside an anonymous namespace (which is the preferred modern C++ style). Prevents ODR (One Definition Rule) violations when two translation units define a function with the same name.

**Implementation requirement?**
NO — but complete illustration:

```cpp
// (a) Static local variable — thread-safe singleton init (C++11)
Logger& getLogger() {
    static Logger instance;  // initialized once, on first call, thread-safe
    return instance;
    // destroyed at program exit
}

// (b) Static member variable
class Counter {
public:
    static int count;        // declaration — no storage here
    Counter() { ++count; }
};
int Counter::count = 0;      // definition — storage here, one per program
// C++17: can use inline: inline static int count = 0; inside class

// (c) Static member function
class Factory {
public:
    static Factory* create() { return new Factory(); }
    // No 'this'. Compatible with C-style function pointers.
private:
    Factory() = default;
};

// C API callback pattern:
// void register_callback(void (*fn)(void));
// register_callback(&Factory::create_void);  // works: no 'this'

// (d) Static free function — internal linkage
static void helper() { /* not visible outside this .cpp file */ }
// Modern C++ equivalent:
namespace {
    void helper2() { /* also internal linkage, preferred */ }
}
```

**The follow-up trap**
"Is initialization of a static local variable always thread-safe?"
Answer: Yes, since C++11. The standard requires it. Before C++11, it was not guaranteed — you needed explicit synchronization. The runtime cost is: one atomic flag check on each subsequent call (the guard byte test). On x86 with a hot cache, this is ~1-2 cycles. If the guard byte is cold, it's a cache miss. Some HFT code avoids static locals in hot paths for this reason.

**Key numbers / facts**
- C++11: static local init is thread-safe (no explicit lock needed in user code)
- Static member: lives in .data/.bss, not inside any object
- Static function: no `this`, usable as raw function pointer
- Static file-scope: internal linkage, invisible to linker from other TUs
- Preferred modern idiom for (d): anonymous namespace, not `static`

---

## Lvalue vs Rvalue and Move Semantics

**Lvalue has persistent identity (addressable); rvalue is a temporary without persistent identity. Move semantics exploit rvalues to steal resources in O(1).**

**What the interviewer actually asked**
"What is an lvalue? What is an rvalue? Why do rvalue references exist?" (Graviton)

**The shallow answer**
"Lvalue is on the left side of assignment, rvalue is on the right."

**The deep answer**
- Correct definition: lvalue has a persistent memory location — you can take its address with `&`. rvalue is a temporary — no stable address, exists only for the duration of the expression.
- The "left side / right side" mnemonic is wrong: `const int& r = 42;` — 42 is an rvalue but binds to the right side of `=`. `int x; x = 5;` — x is an lvalue on the left, but a function returning int& can appear on the left too.
- C++11 value categories (full): lvalue, xvalue (eXpiring value — result of `std::move`, named rvalue reference), prvalue (pure rvalue — literal, temporary). lvalue + xvalue = glvalue. xvalue + prvalue = rvalue.
- rvalue references (`T&&`): introduced solely to enable move semantics. An rvalue reference binds to temporaries and xvalues. It does NOT bind to lvalues (without `std::move`).
- `std::move(x)`: does NOT move anything. It is a cast to `T&&` (xvalue). It tells the compiler "treat x as a temporary — it's OK to steal its resources." The actual move happens in the move constructor or move assignment operator.
- Move constructor for `std::vector<T>`: copies the 3 pointer-sized fields (pointer, size, capacity) — 3 register ops. Sets the source pointer to null. O(1). No heap allocation, no element copy.
- Copy constructor for `std::vector<T>`: malloc a new buffer, copy or construct each element. O(n) time, O(n) heap allocation, cache-cold reads and writes.
- Hardware consequence: copying a 1M-element vector = 8MB memcpy across cache lines. Moving = 3 register assignments. In a hot trading loop, returning a vector by value used to be catastrophic. With move semantics + RVO, it's free.
- Perfect forwarding: `template<typename T> void f(T&& arg)` — T&& is a forwarding reference (not an rvalue reference) when T is deduced. `std::forward<T>(arg)` preserves the value category of arg. Used in factory functions and `emplace_back`.

**Implementation requirement?**
NO — but illustrative:

```cpp
// lvalue: has address, persists
int x = 42;
int* p = &x;       // OK: x is lvalue
int& ref = x;      // OK: lvalue reference binds to lvalue

// rvalue: temporary, no address
// int* p2 = &42;  // ERROR: cannot take address of rvalue
int&& rref = 42;   // OK: rvalue reference binds to rvalue (extends lifetime)

// Move semantics
struct Buffer {
    char* data;
    size_t size;

    Buffer(size_t n) : data(new char[n]), size(n) {}

    // Copy constructor: O(n) — allocate + copy every byte
    Buffer(const Buffer& o) : data(new char[o.size]), size(o.size) {
        std::memcpy(data, o.data, size);
    }

    // Move constructor: O(1) — steal pointer
    Buffer(Buffer&& o) noexcept : data(o.data), size(o.size) {
        o.data = nullptr;  // prevent double-free
        o.size = 0;
    }

    ~Buffer() { delete[] data; }
};

Buffer a(1'000'000);
Buffer b = std::move(a);  // O(1): pointer swap, a.data is now null
Buffer c = a;             // O(n): full copy of a (which is now empty, but still O(n) path)

// Perfect forwarding
template<typename T>
void emplace(T&& val) {
    // std::forward preserves value category:
    // if val was lvalue, forward passes it as lvalue
    // if val was rvalue (or std::move'd), forward passes it as rvalue
    container.push_back(std::forward<T>(val));
}
```

**The follow-up trap**
"Is a named rvalue reference an lvalue or an rvalue?"
Answer: lvalue. `void f(int&& x) { /* x is an lvalue inside here */ }`. A named rvalue reference is an lvalue because it has a name (and therefore a persistent address). This is why you need `std::move(x)` again inside f if you want to forward it as an rvalue. This surprises many candidates.

**Key numbers / facts**
- `std::move`: zero runtime cost — it's a `static_cast<T&&>`, compiled away
- Move a 1M vector: ~3 register ops
- Copy a 1M vector: malloc + ~8MB memcpy
- `noexcept` on move constructor is critical — `std::vector` will only use move during reallocation if the move constructor is `noexcept`
- C++17 guarantees RVO (Return Value Optimization) — prvalue returned from function is constructed directly in destination, not moved or copied

---

## new vs malloc — Every Dimension

**`new` is a C++ operator that allocates memory AND constructs an object; `malloc` is a C library function that allocates raw memory only**

**What the interviewer actually asked**
"Tell me everything about new vs malloc. Keep going." (QuadEye asked to keep going deeper)

**The shallow answer**
"new calls the constructor, malloc doesn't. new throws on failure, malloc returns null."

**The deep answer**

**(a) Initialization**
`new T` calls `T`'s constructor. `malloc(sizeof(T))` returns uninitialized bytes. Using the memory returned by malloc as a T without constructing it is UB.

**(b) Type safety**
`new T` returns `T*`. `malloc` returns `void*` — requires explicit cast in C++, and you can easily cast to the wrong type silently.

**(c) Overloadability**
`operator new` can be overloaded at global scope or per-class. Per-class override lets you redirect all allocations for a type to a custom memory pool. `malloc` cannot be overloaded (it's a C library function).

**(d) Failure behavior**
`new` throws `std::bad_alloc` on allocation failure (unless you use `new(std::nothrow)` which returns nullptr). `malloc` returns `nullptr`. In HFT, `new(std::nothrow)` or pre-allocated pools are common to avoid exception overhead.

**(e) Alignment**
`new T` guarantees alignment to `alignof(T)`. `malloc` guarantees alignment to `max_align_t` (typically 8 or 16 bytes). For over-aligned types (`alignas(64)` for cache-line alignment), `new` respects it; `malloc` may not. C++17 `operator new` is alignment-aware. Use `std::aligned_alloc` or `_mm_malloc` for aligned raw allocation.

**(f) operator new internals**
`operator new(size_t)` (the default) calls `malloc` under the hood in most standard library implementations. You can replace `::operator new` globally. The call chain for `new T` is: compiler generates call to `operator new(sizeof(T))` → calls malloc → compiler injects constructor call on returned pointer.

**(g) Placement new**
`new(ptr) T(args...)` — constructs T at address `ptr` without any allocation. Ptr must be suitably aligned and have sufficient space. Used in memory pools, std::optional internals, shared_ptr control block. Must be destroyed explicitly via `ptr->~T()` (not delete, which would also try to free). This is the HFT workhorse: allocate a slab once, placement-new objects into it, manually destroy them.

**(h) delete vs free**
`delete ptr` calls `ptr->~T()` then `::operator delete(ptr)`. `free(ptr)` only releases memory, no destructor. Mixing: `delete` on `malloc`'d memory = UB. `free` on `new`'d memory = UB.

**(i) new[] vs new**
`new T[n]` uses a separate allocator (`operator new[]`). Implementations often store the array count at a hidden offset before the returned pointer (to know how many dtors to call on `delete[]`). `delete[]` must be used — using `delete` (non-array) on a `new[]`'d array is UB.

**(j) Custom allocators for HFT**
Override `operator new` and `operator delete` per-class to allocate from a pre-allocated pool. Eliminates malloc overhead (lock contention in glibc malloc, TLB misses from fragmentation). Common pattern: slab allocator for fixed-size objects, bump allocator for short-lived per-event objects reset after each event.

**Implementation requirement?**
NO — but illustrative placement new (the HFT-critical pattern):

```cpp
// Placement new: construct without allocation
alignas(alignof(Order)) char pool[sizeof(Order) * 1000];
int count = 0;

Order* place_order(int price, int qty) {
    Order* o = new (pool + count * sizeof(Order)) Order(price, qty);
    ++count;
    return o;
}

void destroy_order(Order* o) {
    o->~Order();  // explicit destructor, no free()
}

// Per-class operator new — pool allocator
class Order {
public:
    static void* operator new(size_t sz) {
        return g_order_pool.allocate(sz);
    }
    static void operator delete(void* ptr) {
        g_order_pool.deallocate(ptr);
    }
    // ...
};

// Now: new Order(price, qty) uses pool, zero malloc overhead
```

**The follow-up trap**
"What does `delete nullptr` do?"
Answer: Well-defined, does nothing. The standard guarantees `delete nullptr` is a no-op. This is intentional — you don't need to null-check before delete.

**Key numbers / facts**
- `new` = `operator new` (malloc) + constructor call
- `delete` = destructor call + `operator delete` (free)
- Placement new: zero allocation overhead, but manual lifetime management
- `new(std::nothrow)`: returns nullptr on failure instead of throwing
- Mixing new/free or malloc/delete: always UB
- `delete[]` must pair with `new[]`, `delete` must pair with `new`
- glibc malloc: has a per-arena lock — contended in multithreaded HFT code

---

## shared_ptr Implementation

**A reference-counted smart pointer that allows shared ownership of a heap object; last owner destroys the object**

**What the interviewer actually asked**
"Implement shared_ptr from scratch. Now explain thread safety." (DE Shaw, Alphagrep, Salesforce)

**The shallow answer**
"It keeps a reference count and deletes the object when count reaches zero."

**The deep answer**
- Naive implementation: two heap allocations — one for the object, one for the control block. `make_shared` fuses them into one allocation: control block contains the object inline. One allocation, better cache locality, but the object cannot be freed independently of the control block (weak_ptr keeps the control block alive).
- Control block contains: `atomic<int>` strong ref count (shared_ptr copies), `atomic<int>` weak ref count (weak_ptr copies + 1 while any shared_ptr exists), deleter (type-erased function), allocator.
- Thread safety: incrementing/decrementing the ref count is atomic (thread-safe). But the shared_ptr object itself (the pointer + control block pointer pair) is NOT thread-safe to copy/assign from multiple threads without synchronization. Multiple threads can hold their own copies of a shared_ptr (safe). Multiple threads cannot simultaneously read and write the same shared_ptr instance (not safe).
- Copy constructor: atomic increment of strong count.
- Destructor: atomic decrement. If strong count reaches 0: call deleter (destroys object). Then decrement weak count by 1. If weak count also reaches 0: free control block.
- `weak_ptr::lock()`: atomically checks if strong count > 0 and increments it if so. Returns empty shared_ptr if object already destroyed.
- Circular reference: A holds shared_ptr to B, B holds shared_ptr to A. Neither ref count ever hits 0. Solution: one side uses weak_ptr.
- Performance cost in HFT: every copy = atomic increment (memory fence on x86: actually a lock xadd). Every destroy = atomic decrement + conditional branch. On a modern x86, a lock xadd to a shared cache line from multiple cores causes cache-line bouncing (~100-300 cycles contention). Use unique_ptr where possible; if sharing is needed, consider passing raw pointers with explicit lifetime guarantees.

**Implementation requirement?**
YES:

```cpp
#include <atomic>
#include <utility>
#include <cassert>

// -------------------------------------------------------
// Control block base — type-erased destructor
// -------------------------------------------------------
struct ControlBlockBase {
    std::atomic<int> strong_count{1};
    std::atomic<int> weak_count{1};  // +1 while any shared_ptr alive

    virtual void destroy_object() noexcept = 0;   // calls ~T
    virtual void destroy_self()   noexcept = 0;   // frees control block memory
    virtual ~ControlBlockBase() = default;
};

// Control block for separate allocation (new T + new ControlBlock)
template<typename T, typename Deleter = std::default_delete<T>>
struct ControlBlock : ControlBlockBase {
    T*      ptr;
    Deleter deleter;

    ControlBlock(T* p, Deleter d = Deleter{}) : ptr(p), deleter(std::move(d)) {}

    void destroy_object() noexcept override {
        deleter(ptr);
        ptr = nullptr;
    }
    void destroy_self() noexcept override {
        delete this;
    }
};

// -------------------------------------------------------
// Forward declare weak_ptr
// -------------------------------------------------------
template<typename T>
class weak_ptr_impl;

// -------------------------------------------------------
// shared_ptr
// -------------------------------------------------------
template<typename T>
class shared_ptr_impl {
    template<typename U> friend class shared_ptr_impl;
    template<typename U> friend class weak_ptr_impl;

    T*                ptr_{nullptr};
    ControlBlockBase* cb_{nullptr};

    // Private: construct from weak_ptr (lock path)
    shared_ptr_impl(T* ptr, ControlBlockBase* cb) noexcept
        : ptr_(ptr), cb_(cb) {}

public:
    // Default: empty
    shared_ptr_impl() noexcept = default;

    // From raw pointer: takes ownership
    explicit shared_ptr_impl(T* ptr)
        : ptr_(ptr),
          cb_(ptr ? new ControlBlock<T>(ptr) : nullptr) {}

    // From raw pointer with custom deleter
    template<typename Deleter>
    shared_ptr_impl(T* ptr, Deleter d)
        : ptr_(ptr),
          cb_(ptr ? new ControlBlock<T, Deleter>(ptr, std::move(d)) : nullptr) {}

    // Copy constructor: share ownership
    shared_ptr_impl(const shared_ptr_impl& o) noexcept
        : ptr_(o.ptr_), cb_(o.cb_) {
        if (cb_) cb_->strong_count.fetch_add(1, std::memory_order_relaxed);
    }

    // Move constructor: transfer ownership
    shared_ptr_impl(shared_ptr_impl&& o) noexcept
        : ptr_(o.ptr_), cb_(o.cb_) {
        o.ptr_ = nullptr;
        o.cb_  = nullptr;
    }

    // Copy assignment
    shared_ptr_impl& operator=(const shared_ptr_impl& o) noexcept {
        if (this != &o) {
            shared_ptr_impl tmp(o);  // increment o's count
            swap(tmp);               // swap — tmp now holds our old state
        }                            // tmp destructs: decrements old count
        return *this;
    }

    // Move assignment
    shared_ptr_impl& operator=(shared_ptr_impl&& o) noexcept {
        shared_ptr_impl tmp(std::move(o));
        swap(tmp);
        return *this;
    }

    // Destructor: release ownership
    ~shared_ptr_impl() {
        if (!cb_) return;
        // Decrement strong count with release semantics so the object's
        // final writes are visible to the thread that destroys it.
        if (cb_->strong_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            cb_->destroy_object();
            // Now decrement weak count (the +1 held while any shared_ptr alive)
            if (cb_->weak_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                cb_->destroy_self();
            }
        }
    }

    T* get()             const noexcept { return ptr_; }
    T& operator*()       const noexcept { return *ptr_; }
    T* operator->()      const noexcept { return ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    int use_count() const noexcept {
        return cb_ ? cb_->strong_count.load(std::memory_order_relaxed) : 0;
    }

    void swap(shared_ptr_impl& o) noexcept {
        std::swap(ptr_, o.ptr_);
        std::swap(cb_,  o.cb_);
    }

    void reset() noexcept { shared_ptr_impl().swap(*this); }
    void reset(T* ptr)    { shared_ptr_impl(ptr).swap(*this); }
};

// -------------------------------------------------------
// weak_ptr
// -------------------------------------------------------
template<typename T>
class weak_ptr_impl {
    T*                ptr_{nullptr};
    ControlBlockBase* cb_{nullptr};

public:
    weak_ptr_impl() noexcept = default;

    weak_ptr_impl(const shared_ptr_impl<T>& sp) noexcept
        : ptr_(sp.ptr_), cb_(sp.cb_) {
        if (cb_) cb_->weak_count.fetch_add(1, std::memory_order_relaxed);
    }

    weak_ptr_impl(const weak_ptr_impl& o) noexcept
        : ptr_(o.ptr_), cb_(o.cb_) {
        if (cb_) cb_->weak_count.fetch_add(1, std::memory_order_relaxed);
    }

    ~weak_ptr_impl() {
        if (!cb_) return;
        if (cb_->weak_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            cb_->destroy_self();
        }
    }

    // lock(): atomically try to promote to shared_ptr
    shared_ptr_impl<T> lock() const noexcept {
        if (!cb_) return {};
        // CAS loop: increment strong_count only if > 0
        int count = cb_->strong_count.load(std::memory_order_relaxed);
        while (count > 0) {
            if (cb_->strong_count.compare_exchange_weak(
                    count, count + 1,
                    std::memory_order_acq_rel,
                    std::memory_order_relaxed)) {
                return shared_ptr_impl<T>(ptr_, cb_);  // private ctor
            }
            // count updated by CAS failure — retry with new value
        }
        return {};  // object already destroyed
    }

    bool expired() const noexcept {
        return !cb_ || cb_->strong_count.load(std::memory_order_relaxed) == 0;
    }
};
```

**The follow-up trap**
"make_shared allocates object and control block together — does that have any downside?"
Answer: Yes. With separate allocation (`shared_ptr<T>(new T(...))`), when the strong count hits 0, the object is destroyed and its memory freed immediately, even if weak_ptrs keep the control block alive. With `make_shared`, the object's memory cannot be freed until the weak count also hits 0 (because object and control block are one allocation). If you have long-lived weak_ptrs, make_shared keeps the object's memory alive longer than necessary.

**Key numbers / facts**
- make_shared: 1 allocation (control block + object fused)
- new + shared_ptr: 2 allocations
- Atomic ref count increment: ~5ns uncontended on modern x86
- Atomic ref count with cache-line contention from multiple cores: ~100-300ns
- Thread-safe: the ref count. NOT thread-safe: the shared_ptr object itself
- Circular references: always use weak_ptr for back-pointers/observer patterns

---

## unique_ptr Implementation

**A non-copyable, moveable RAII smart pointer that models exclusive ownership with zero overhead over a raw pointer**

**What the interviewer actually asked**
"Implement unique_ptr from scratch." (Alphagrep)

**The shallow answer**
"It's a wrapper that calls delete in the destructor."

**The deep answer**
- Non-copyable: copy constructor and copy assignment are `= delete`. Prevents accidental double-free.
- Moveable: move constructor and move assignment transfer ownership (set source to nullptr).
- EBO for deleter: unique_ptr stores the pointer and deleter. If deleter is an empty class (std::default_delete is empty), EBO collapses deleter storage to 0 bytes. sizeof(unique_ptr<T>) == sizeof(T*) == 8 bytes.
- Zero overhead: with the default deleter, the compiler inlines the destructor to a single `if (ptr) delete ptr;`, which is identical to what you'd write manually. No virtual dispatch, no ref count, no heap allocation.
- Custom deleters: useful for anything that isn't heap-allocated. `unique_ptr<FILE, decltype(&fclose)>` for file handles. `unique_ptr<void, decltype(&munmap)>` for mmap'd regions. The deleter type is part of the unique_ptr type — `unique_ptr<T, D1>` and `unique_ptr<T, D2>` are different types.
- unique_ptr<T[]>: array specialization. Uses `delete[]` instead of `delete`. Provides `operator[]`. Does NOT provide `operator*` or `operator->`.
- Converting to shared_ptr: `shared_ptr<T> sp = std::move(up);` — valid. unique_ptr can be converted to shared_ptr (which takes ownership). The reverse is not possible.

**Implementation requirement?**
YES:

```cpp
#include <utility>    // std::move, std::swap, std::forward
#include <memory>     // std::default_delete

// -------------------------------------------------------
// Primary template: single object
// -------------------------------------------------------
template<typename T, typename Deleter = std::default_delete<T>>
class unique_ptr_impl {
public:
    using pointer      = T*;
    using element_type = T;
    using deleter_type = Deleter;

private:
    // EBO: compress deleter to 0 bytes when it is an empty class.
    // Real implementations use a compressed_pair; we use inheritance here
    // for clarity.
    struct Storage : Deleter {   // inherits from Deleter — EBO applies
        T* ptr;
        explicit Storage(T* p, Deleter d = Deleter{})
            : Deleter(std::move(d)), ptr(p) {}
    } storage_;

    Deleter& get_deleter()             noexcept { return storage_; }
    const Deleter& get_deleter() const noexcept { return storage_; }

public:
    // Constructors
    constexpr unique_ptr_impl() noexcept : storage_(nullptr) {}
    constexpr unique_ptr_impl(std::nullptr_t) noexcept : storage_(nullptr) {}

    explicit unique_ptr_impl(T* ptr) noexcept : storage_(ptr) {}

    unique_ptr_impl(T* ptr, Deleter d) noexcept
        : storage_(ptr, std::move(d)) {}

    // Move constructor: transfer ownership
    unique_ptr_impl(unique_ptr_impl&& o) noexcept
        : storage_(o.release(), std::move(o.get_deleter())) {}

    // Converting move constructor (e.g., unique_ptr<Derived> -> unique_ptr<Base>)
    template<typename U, typename D2,
             typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    unique_ptr_impl(unique_ptr_impl<U, D2>&& o) noexcept
        : storage_(o.release(), std::move(o.get_deleter())) {}

    // No copy
    unique_ptr_impl(const unique_ptr_impl&)            = delete;
    unique_ptr_impl& operator=(const unique_ptr_impl&) = delete;

    // Move assignment
    unique_ptr_impl& operator=(unique_ptr_impl&& o) noexcept {
        if (this != &o) {
            reset(o.release());
            get_deleter() = std::move(o.get_deleter());
        }
        return *this;
    }

    unique_ptr_impl& operator=(std::nullptr_t) noexcept {
        reset();
        return *this;
    }

    // Destructor: RAII cleanup
    ~unique_ptr_impl() noexcept {
        if (storage_.ptr) {
            get_deleter()(storage_.ptr);  // calls delete by default
        }
    }

    // Observers
    T* get()             const noexcept { return storage_.ptr; }
    T& operator*()       const noexcept { return *storage_.ptr; }
    T* operator->()      const noexcept { return storage_.ptr; }
    explicit operator bool() const noexcept { return storage_.ptr != nullptr; }

    // Modifiers

    // release(): give up ownership, return raw pointer, set internal to null
    T* release() noexcept {
        T* tmp = storage_.ptr;
        storage_.ptr = nullptr;
        return tmp;
    }

    // reset(): destroy current object, take ownership of new one
    void reset(T* ptr = nullptr) noexcept {
        T* old = storage_.ptr;
        storage_.ptr = ptr;
        if (old) get_deleter()(old);
    }

    void swap(unique_ptr_impl& o) noexcept {
        std::swap(storage_.ptr,  o.storage_.ptr);
        std::swap(get_deleter(), o.get_deleter());
    }
};

// -------------------------------------------------------
// Array specialization
// -------------------------------------------------------
template<typename T, typename Deleter>
class unique_ptr_impl<T[], Deleter> {
    T*      ptr_;
    Deleter deleter_;
public:
    explicit unique_ptr_impl(T* ptr = nullptr) noexcept
        : ptr_(ptr), deleter_{} {}

    ~unique_ptr_impl() noexcept {
        if (ptr_) deleter_(ptr_);  // default_delete<T[]> calls delete[]
    }

    unique_ptr_impl(unique_ptr_impl&& o) noexcept
        : ptr_(o.release()), deleter_(std::move(o.deleter_)) {}

    unique_ptr_impl(const unique_ptr_impl&)            = delete;
    unique_ptr_impl& operator=(const unique_ptr_impl&) = delete;

    T& operator[](size_t i) const noexcept { return ptr_[i]; }
    T* get()                 const noexcept { return ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    T* release() noexcept { T* t = ptr_; ptr_ = nullptr; return t; }
    void reset(T* ptr = nullptr) noexcept {
        T* old = ptr_; ptr_ = ptr;
        if (old) deleter_(old);
    }
};

// Factory function (like std::make_unique)
template<typename T, typename... Args>
unique_ptr_impl<T> make_unique_impl(Args&&... args) {
    return unique_ptr_impl<T>(new T(std::forward<Args>(args)...));
}

// -------------------------------------------------------
// Usage examples
// -------------------------------------------------------
// unique_ptr_impl<int> p(new int(42));
// auto p2 = std::move(p);      // p is null, p2 owns the int
// p2.reset();                  // int deleted

// Custom deleter for FILE*:
// auto f = unique_ptr_impl<FILE, decltype(&fclose)>(fopen("x","r"), &fclose);
```

**The follow-up trap**
"Can unique_ptr be stored in a standard container like std::vector?"
Answer: Yes — `std::vector<std::unique_ptr<T>>` is valid. You must use move semantics to insert (`push_back(std::move(p))`), never copy. Sorting the vector requires move-capable elements, which unique_ptr satisfies. You cannot use `std::set` or other containers that require copyability — only moveable.

**Key numbers / facts**
- sizeof(unique_ptr<T>) == sizeof(T*) == 8 bytes (with stateless deleter, EBO)
- Zero runtime overhead over raw pointer when inlined
- Non-copyable: prevents all double-free bugs statically at compile time
- release(): relinquishes ownership without destroying — use carefully
- Converting unique_ptr<Derived> to unique_ptr<Base>: safe via move

---

## Vector push_back Implementation

**push_back appends an element; when capacity is exceeded it reallocates with geometric growth to achieve O(1) amortized cost**

**What the interviewer actually asked**
"Implement push_back for vector. What's the amortized complexity and why?" (Alphagrep)

**The shallow answer**
"It doubles the capacity when full, so it's amortized O(1)."

**The deep answer**
- Why geometric growth gives amortized O(1): start with capacity C. Each push_back is O(1) until capacity hit. On reallocation: copy/move n elements. But the next reallocation happens after another n insertions. Total work: 1 + 2 + 4 + ... + n = 2n. Amortized n insertions = 2n work = O(1) per insertion. If you grew by +1 each time: 1+2+3+...+n = O(n²) total.
- Growth factor tradeoff: factor 2.0 = 50% wasted memory in worst case, but previous allocations can never be reused (sum of all previous capacities = current capacity). Factor < 2 (e.g., 1.5 or golden ratio ≈ 1.618) = at some point previous freed memory is large enough to reuse, better for allocators. MSVC uses 1.5, GCC uses 2.0.
- Reallocation calls move constructors (not copy) if `noexcept` move is available (std::move_if_noexcept). If move constructor might throw, it falls back to copy (to maintain strong exception guarantee). This is why `noexcept` on move constructors matters: without it, push_back copies instead of moves on reallocation.
- Iterator invalidation: ANY reallocation invalidates all iterators, pointers, and references into the vector. push_back that causes reallocation = all iterators dangling.
- In HFT: never let a vector reallocate in a hot path. Call `reserve(expected_max_size)` upfront. Better yet, use a fixed-size array or a custom container.

**Implementation requirement?**
YES:

```cpp
#include <memory>       // std::allocator, std::allocator_traits
#include <stdexcept>    // std::out_of_range
#include <algorithm>    // std::max
#include <utility>      // std::move, std::forward

template<typename T, typename Allocator = std::allocator<T>>
class vector_impl {
    using AllocTraits = std::allocator_traits<Allocator>;

    T*        data_     {nullptr};
    size_t    size_     {0};
    size_t    capacity_ {0};
    Allocator alloc_;

    // Allocate raw memory for n elements (no construction)
    T* allocate(size_t n) {
        return AllocTraits::allocate(alloc_, n);
    }

    // Deallocate raw memory (no destruction)
    void deallocate(T* ptr, size_t n) {
        if (ptr) AllocTraits::deallocate(alloc_, ptr, n);
    }

    // Destroy elements [first, last) but don't free memory
    void destroy_range(T* first, T* last) noexcept {
        for (; first != last; ++first)
            AllocTraits::destroy(alloc_, first);
    }

    // Move elements from [src, src+n) to uninitialized dst
    // Uses move if noexcept, copy otherwise (strong exception guarantee)
    void uninitialized_move_or_copy(T* src, size_t n, T* dst) {
        if constexpr (std::is_nothrow_move_constructible_v<T>) {
            for (size_t i = 0; i < n; ++i)
                AllocTraits::construct(alloc_, dst + i, std::move(src[i]));
        } else {
            size_t i = 0;
            try {
                for (; i < n; ++i)
                    AllocTraits::construct(alloc_, dst + i, src[i]);
            } catch (...) {
                destroy_range(dst, dst + i);
                throw;
            }
        }
    }

    // Core reallocation: grow to new_cap
    void reallocate(size_t new_cap) {
        T* new_data = allocate(new_cap);

        // Move/copy existing elements to new buffer
        // If this throws (copy path), new_data is freed in catch
        try {
            uninitialized_move_or_copy(data_, size_, new_data);
        } catch (...) {
            deallocate(new_data, new_cap);
            throw;
        }

        // Destroy old elements and free old buffer
        destroy_range(data_, data_ + size_);
        deallocate(data_, capacity_);

        data_     = new_data;
        capacity_ = new_cap;
        // size_ unchanged
    }

public:
    vector_impl() noexcept = default;

    explicit vector_impl(size_t n, const T& val = T{}) {
        reserve(n);
        for (size_t i = 0; i < n; ++i)
            AllocTraits::construct(alloc_, data_ + i, val);
        size_ = n;
    }

    ~vector_impl() noexcept {
        destroy_range(data_, data_ + size_);
        deallocate(data_, capacity_);
    }

    // No copy for brevity; real impl would deep-copy
    vector_impl(const vector_impl&)            = delete;
    vector_impl& operator=(const vector_impl&) = delete;

    // Move constructor
    vector_impl(vector_impl&& o) noexcept
        : data_(o.data_), size_(o.size_), capacity_(o.capacity_), alloc_(std::move(o.alloc_)) {
        o.data_ = nullptr; o.size_ = 0; o.capacity_ = 0;
    }

    // -------------------------------------------------------
    // reserve: ensure capacity >= n, reallocate if needed
    // -------------------------------------------------------
    void reserve(size_t n) {
        if (n <= capacity_) return;
        reallocate(n);
    }

    // -------------------------------------------------------
    // push_back (copy): O(1) amortized
    // -------------------------------------------------------
    void push_back(const T& val) {
        if (size_ == capacity_) {
            // Geometric growth: factor 2 (GCC/libstdc++ style)
            size_t new_cap = capacity_ == 0 ? 1 : capacity_ * 2;
            reallocate(new_cap);
        }
        // Construct new element at end of initialized region
        AllocTraits::construct(alloc_, data_ + size_, val);
        ++size_;
    }

    // -------------------------------------------------------
    // push_back (move): O(1) amortized, avoids copy for moveable T
    // -------------------------------------------------------
    void push_back(T&& val) {
        if (size_ == capacity_) {
            size_t new_cap = capacity_ == 0 ? 1 : capacity_ * 2;
            reallocate(new_cap);
        }
        AllocTraits::construct(alloc_, data_ + size_, std::move(val));
        ++size_;
    }

    // -------------------------------------------------------
    // emplace_back: construct in-place, forwarding args — avoids extra move
    // -------------------------------------------------------
    template<typename... Args>
    T& emplace_back(Args&&... args) {
        if (size_ == capacity_) {
            size_t new_cap = capacity_ == 0 ? 1 : capacity_ * 2;
            reallocate(new_cap);
        }
        T* slot = data_ + size_;
        AllocTraits::construct(alloc_, slot, std::forward<Args>(args)...);
        ++size_;
        return *slot;
    }

    void pop_back() noexcept {
        assert(size_ > 0);
        AllocTraits::destroy(alloc_, data_ + size_ - 1);
        --size_;
    }

    T&       operator[](size_t i)       noexcept { return data_[i]; }
    const T& operator[](size_t i) const noexcept { return data_[i]; }
    T&       front()                    noexcept { return data_[0]; }
    T&       back()                     noexcept { return data_[size_-1]; }
    T*       begin()                    noexcept { return data_; }
    T*       end()                      noexcept { return data_ + size_; }
    size_t   size()                     const noexcept { return size_; }
    size_t   capacity()                 const noexcept { return capacity_; }
    bool     empty()                    const noexcept { return size_ == 0; }
};
```

**The follow-up trap**
"Why does push_back take the argument by value in some APIs?"
Answer: The "pass by value + move" pattern: `void push_back(T val) { ... move(val) ... }`. For lvalues this costs a copy (same as const&). For rvalues this costs a move (same as &&). Combines both overloads into one function at the cost of an extra move for rvalue callers. For expensive-to-move types this is worse; for cheap-to-move types it's equivalent. The two-overload approach (const& and &&) is strictly more efficient but more verbose.

**Key numbers / facts**
- Amortized O(1) push_back requires geometric growth (any factor > 1)
- GCC libstdc++: growth factor 2.0
- MSVC STL: growth factor 1.5
- realloc uses move constructors only if `noexcept` — always mark move constructors `noexcept`
- push_back causing reallocation invalidates ALL iterators, pointers, references
- HFT: `reserve()` upfront, never reallocate in hot path
- emplace_back: constructs in-place, avoids extra move vs push_back with a newly constructed object

---

## Struct Padding and Memory Layout

**The compiler inserts padding bytes between struct members to satisfy alignment requirements; understanding this is essential for minimizing struct size and cache behavior**

**What the interviewer actually asked**
"What is struct padding? Rearrange this struct to minimize size." (Alphagrep)

**The shallow answer**
"Padding aligns members to their natural alignment."

**The deep answer**
- Each member must be placed at an address divisible by its alignment: `char` aligns to 1, `short` to 2, `int` to 4, `long long`/`double`/pointer to 8.
- The struct itself aligns to the largest alignment of any member (so that arrays of structs work correctly).
- Rule for minimal size: sort members by descending alignment (descending size for primitives). This eliminates interior padding.
- Tail padding: the struct is padded at the end to make sizeof a multiple of the struct's alignment. Example: `struct { int i; char c; }` = 8 bytes (4 + 1 + 3 tail padding) even though data is only 5 bytes.
- `alignas(N)`: force a type or variable to align to N bytes. N must be a power of 2. Common in HFT: `alignas(64)` to put a struct on its own cache line, preventing false sharing. Cache line = 64 bytes on all modern x86 processors.
- `__attribute__((packed))` (GCC) / `#pragma pack(1)` (MSVC): suppress padding entirely. Danger: misaligned access on architectures that require alignment (ARM fault). On x86 misaligned access is allowed but costs ~2x (crosses two cache lines). Never use in hot paths.
- `offsetof(T, member)`: returns byte offset of member within struct T. Defined for standard-layout types.
- Hardware: if a struct straddles a cache line boundary (e.g., 8-byte field starting at byte 60 of a 64-byte cache line), loading or storing it requires fetching two cache lines. On x86 this is handled in hardware but is slower. `alignas(64)` on a frequently accessed struct forces it to start on a cache line boundary.

**Implementation requirement?**
NO — but illustrative layout analysis:

```cpp
// BAD layout — 12 bytes
struct Bad {
    char  a;    // offset 0, 1 byte
                // 3 bytes padding (int needs 4-byte alignment)
    int   b;    // offset 4, 4 bytes
    char  c;    // offset 8, 1 byte
                // 3 bytes tail padding (struct aligns to 4)
};
static_assert(sizeof(Bad) == 12);

// GOOD layout — 8 bytes (same members, sorted by descending size)
struct Good {
    int   b;    // offset 0, 4 bytes
    char  a;    // offset 4, 1 byte
    char  c;    // offset 5, 1 byte
                // 2 bytes tail padding (struct aligns to 4)
};
static_assert(sizeof(Good) == 8);

// More complex example
struct Mixed {
    char   a;    // offset 0, 1 byte
                 // 7 bytes padding (double needs 8-byte alignment)
    double d;    // offset 8, 8 bytes
    int    i;    // offset 16, 4 bytes
    short  s;    // offset 20, 2 bytes
                 // 2 bytes tail padding (struct aligns to 8)
};
static_assert(sizeof(Mixed) == 24);

struct MixedOptimal {
    double d;    // offset 0, 8 bytes
    int    i;    // offset 8, 4 bytes
    short  s;    // offset 12, 2 bytes
    char   a;    // offset 14, 1 byte
                 // 1 byte tail padding (struct aligns to 8)
};
static_assert(sizeof(MixedOptimal) == 16);

// Cache-line alignment to prevent false sharing
struct alignas(64) HotData {
    std::atomic<int> counter;   // 4 bytes
    // 60 bytes padding to fill cache line
    // No other unrelated data shares this cache line
};
static_assert(sizeof(HotData) == 64);
static_assert(alignof(HotData) == 64);

// offsetof usage
struct Packet {
    uint32_t seq_num;    // offset 0
    uint16_t length;     // offset 4
    uint16_t checksum;   // offset 6
    char     payload[8]; // offset 8
};
static_assert(offsetof(Packet, payload) == 8);
```

**The follow-up trap**
"What is false sharing and how do you prevent it?"
Answer: Two variables on the same cache line, written by different cores. Even though cores write different variables (no logical sharing), writing one variable invalidates the entire cache line on all other cores (MESI protocol). All cores must re-fetch the cache line. Fix: pad/align each frequently written variable to its own 64-byte cache line with `alignas(64)`. Common in HFT: per-thread counters, per-thread queues, etc.

**Key numbers / facts**
- Cache line size: 64 bytes on all modern x86 processors
- `alignas(64)`: place struct on its own cache line
- `offsetof`: requires standard-layout type (no virtual, no non-standard bases)
- `sizeof(struct)` is always a multiple of the struct's own alignment
- sizeof increase from padding: up to (largest_member_size - 1) bytes wasted
- `#pragma pack`: defeats alignment, do not use in HFT hot paths on x86

---

## C++ Version Features (C++11/14/17/20)

**Each standard revision added features critical to modern systems programming; interviewers ask "which version introduced X" to gauge depth**

**What the interviewer actually asked**
"Which version introduced X?" (DE Shaw)

**The shallow answer**
"Move semantics and lambdas are C++11."

**The deep answer**

### C++11 (the big bang — transformed the language)
- **Move semantics and rvalue references (`&&`)**: eliminated unnecessary copies, enabled efficient containers and return-by-value. The single most impactful feature.
- **Smart pointers** (`unique_ptr`, `shared_ptr`, `weak_ptr`): replaced manual `new`/`delete`. unique_ptr is zero-overhead; shared_ptr has atomic ref count.
- **Lambda expressions**: `[capture](params) -> ret { body }`. Inline function objects, closures. Enables `std::sort(v.begin(), v.end(), [](int a, int b){ return a < b; })`.
- **`std::thread`, `std::mutex`, `std::atomic`**: first standardized memory model and threading primitives. `std::atomic<T>` for lock-free programming.
- **`auto`**: type inference for variable declarations. `auto x = 3.14;` → x is double. Does NOT change runtime behavior.
- **Range-based for**: `for (auto& x : container)` — calls begin()/end() on container.
- **`nullptr`**: type-safe null pointer constant. Replaces NULL (which is 0, an integer). `nullptr` has type `std::nullptr_t`, prevents ambiguity in overload resolution.
- **`constexpr`**: marks functions/variables as evaluable at compile time. Basic version: single return statement. Variables: `constexpr int N = 42;`.
- **`= delete` and `= default`**: explicitly delete or default special member functions. `Foo(const Foo&) = delete;` prevents copying.
- **Variadic templates**: `template<typename... Args>` — templates taking arbitrary numbers of type arguments. Enables `std::tuple`, `std::function`, perfect forwarding.
- **Initializer lists**: `std::vector<int> v = {1, 2, 3};`. Uniform initialization syntax.
- **`static_assert`**: compile-time assertions. `static_assert(sizeof(int) == 4, "need 32-bit int");`.
- **`override` and `final`**: override confirms a function overrides a virtual (compile error if it doesn't). final prevents further overriding or inheritance.
- **Scoped enumerations** (`enum class`): `enum class Color { Red, Green };` — no implicit conversion to int, no namespace pollution.
- **`std::chrono`**: type-safe time durations and clocks. `std::chrono::high_resolution_clock`.

### C++14 (refinements — filled holes in C++11)
- **Generic lambdas**: `auto` parameters in lambda. `[](auto x, auto y) { return x + y; }` — lambda template.
- **`constexpr` relaxed**: functions can now have if/else, loops, multiple statements. Not just a single return.
- **`std::make_unique`**: was accidentally omitted from C++11. `std::make_unique<T>(args...)`.
- **Variable templates**: `template<typename T> constexpr T pi = T(3.14159...);`
- **Binary literals**: `0b1010` == 10. Digit separators: `1'000'000`.
- **`std::exchange`**: `T old = std::exchange(x, new_val);` — atomically replaces value, returns old.
- **Deprecated `[[deprecated]]`**: attribute to mark deprecated functions.
- **Return type deduction**: `auto f() { return 42; }` — compiler deduces return type.

### C++17 (practical improvements — most widely used today)
- **Structured bindings**: `auto [key, val] = map_entry;`. Destructure pairs, tuples, structs.
- **`if constexpr`**: compile-time if in templates. `if constexpr (std::is_integral_v<T>) { ... }`. Eliminates SFINAE gymnastics for simple cases.
- **`std::optional<T>`**: represents a value that may or may not be present. No heap allocation. `sizeof(optional<T>)` == `sizeof(T) + 1` (rounded to alignment).
- **`std::variant<T1, T2, ...>`**: type-safe union. Replaces raw union + manual discriminant. `std::visit` for pattern matching.
- **`std::any`**: type-erased value container. Heap allocates for large types.
- **`std::string_view`**: non-owning reference to a string buffer (pointer + length). Pass instead of `const std::string&` to avoid construction. Zero-copy parsing.
- **Fold expressions**: `(args + ... )` — expand parameter packs with operators.
- **Guaranteed copy elision (mandatory RVO)**: returning a prvalue from a function is guaranteed to construct directly in the destination. No copy, no move, no NRVO opt-in needed.
- **Parallel algorithms**: `std::sort(std::execution::par, v.begin(), v.end())` — policy-based parallelism.
- **`std::filesystem`**: portable file system operations.
- **Class template argument deduction (CTAD)**: `std::pair p(1, 2.0);` — no need for `std::make_pair`.
- **`std::byte`**: type alias for `unsigned char`, used to represent raw byte data without arithmetic confusion.
- **`[[nodiscard]]`**: warn if return value is ignored.
- **`if`/`switch` with init statement**: `if (auto it = m.find(k); it != m.end()) { ... }`.

### C++20 (major feature release — still being adopted)
- **Concepts and `requires`**: named constraints on template parameters. `template<std::integral T>` — compile errors at the point of use, not deep in instantiation. `requires (T a) { a + a; }`.
- **Coroutines**: `co_await`, `co_yield`, `co_return`. Stackless coroutines for async code, generators, state machines. Framework-level (no standard executor yet — that's C++26).
- **Ranges library** (`std::ranges`): composable range adaptors. `v | std::views::filter(f) | std::views::transform(g)`. Lazy, composable, no intermediate allocations.
- **`std::span<T>`**: non-owning view of a contiguous sequence (like string_view but for any type). `span<int>` refers to a contiguous array of ints without owning it.
- **Modules**: replace `#include` with `import` — faster compilation, no macro leakage, no header ordering issues. Still being adopted (limited compiler/tool support as of 2025).
- **Three-way comparison (`<=>`)**: "spaceship operator". `auto operator<=>(const T&) const = default;` generates all six comparison operators.
- **`consteval`**: functions that MUST be evaluated at compile time (unlike `constexpr` which can be runtime). `consteval int square(int n) { return n*n; }`.
- **`std::jthread`**: joinable thread that automatically joins in destructor and supports cooperative cancellation via `stop_token`.
- **`std::format`**: Python-style string formatting. `std::format("x={}, y={}", x, y)`. Type-safe, no printf format string bugs.
- **`std::latch`, `std::barrier`, `std::counting_semaphore`**: synchronization primitives.
- **`std::atomic_ref`**: atomic operations on non-atomic objects.
- **Aggregate initialization improvements**, **`using enum`**, **lambda in unevaluated contexts**, **`[[likely]]`/`[[unlikely]]` attributes** (actually C++20 formalized; hint to optimizer).

**Key numbers / facts**
- C++11: foundational — move semantics, lambdas, threads, atomic
- C++14: make_unique, generic lambdas, relaxed constexpr
- C++17: string_view, optional, variant, guaranteed RVO, structured bindings
- C++20: concepts, coroutines, ranges, modules, format, span
- Most HFT shops today: target C++17 or C++20

---

## Templates

**Compile-time code generation mechanism that produces a separate function or class for each set of template arguments**

**What the interviewer actually asked**
"Explain templates. What is template instantiation?" (QuadEye)

**The shallow answer**
"Templates allow you to write generic code. The compiler generates code for each type."

**The deep answer**
- Template instantiation: when the compiler sees `vector<int>`, it stamps out a complete copy of the vector code with T=int. This happens per translation unit. The linker merges duplicate instantiations (COMDAT folding). Result: each unique instantiation is a separate set of functions/methods in the binary.
- Template code is not compiled until instantiated — syntax errors in uninstantiated templates may not be caught.
- Template specialization: provide an alternate implementation for specific types. Full specialization: `template<> class vector<bool>` (the infamous space-packing specialization). Partial specialization: `template<typename T> class vector<T*>` — specializes for all pointer types.
- SFINAE (Substitution Failure Is Not An Error): when the compiler tries to substitute template arguments and a type expression fails, it discards that overload silently (rather than error) and tries the next. Used to enable/disable function overloads based on type traits. Replaced by concepts in C++20 for readability.
- Compile-time polymorphism (static dispatch, zero vtable cost): `template<typename T> void process(T& x) { x.execute(); }` — resolved at compile time to the specific type's method. No virtual dispatch, no cache miss, no indirect call. The function can be inlined. This is the CRTP (Curiously Recurring Template Pattern) basis.
- Template metaprogramming (TMP): using template instantiation as a Turing-complete computation at compile time. `factorial<5>::value` computed at compile time. constexpr has replaced most TMP in modern code.
- Template bloat: many instantiations → large binary → icache pressure. If you instantiate `sort<int>`, `sort<long>`, `sort<float>`, `sort<double>` — four copies of sort in the binary. In extreme cases, this causes icache misses that hurt hot-path performance in HFT. Mitigation: explicit instantiation, type-erased implementations, or base-class factoring.
- Extern templates (`extern template class vector<int>;`): prevents instantiation in this translation unit, requires instantiation elsewhere. Reduces compile times and binary size.

**Implementation requirement?**
NO — but key patterns illustrated:

```cpp
// Basic function template
template<typename T>
T max_val(T a, T b) { return a > b ? a : b; }
// Instantiation: max_val<int>(3, 5) -> compiler generates:
// int max_val(int a, int b) { return a > b ? a : b; }

// Class template
template<typename T, size_t N>
class FixedArray {
    T data_[N];
public:
    T& operator[](size_t i) { return data_[i]; }
    constexpr size_t size() const { return N; }
};
// FixedArray<double, 4> is a different type than FixedArray<double, 8>

// SFINAE: enable overload only for integral types (C++11/14 style)
template<typename T>
std::enable_if_t<std::is_integral_v<T>, T>
safe_div(T a, T b) { return b ? a/b : 0; }

// Concepts (C++20): same constraint, readable
template<std::integral T>
T safe_div_c20(T a, T b) { return b ? a/b : 0; }

// CRTP: static polymorphism, zero virtual dispatch
template<typename Derived>
struct Executor {
    void run() {
        static_cast<Derived*>(this)->execute();  // no vtable, fully inlined
    }
};
struct FastExec : Executor<FastExec> {
    void execute() { /* HFT hot path */ }
};

// Variadic template (enables std::tuple, std::function, etc.)
template<typename... Ts>
struct TypeList {};
// TypeList<int, double, char> is a valid type
// sizeof...(Ts) gives count at compile time

// Explicit instantiation (in one .cpp file) to reduce binary bloat:
// template class vector<int>;   // instantiate in THIS TU
// extern template class vector<int>;  // DON'T instantiate in OTHER TUs
```

**The follow-up trap**
"What is the difference between `typename` and `class` in a template parameter list?"
Answer: No semantic difference for template type parameters (`template<typename T>` == `template<class T>`). `typename` is also used inside templates to disambiguate dependent type names: `typename T::iterator` tells the compiler that `T::iterator` is a type (not a static member). `class` cannot be used for this purpose.

**Key numbers / facts**
- Each unique template instantiation = separate code in binary
- SFINAE: substitution failure is not an error — used for overload selection
- CRTP: static dispatch, zero vtable, fully inlineable
- Concepts (C++20): replace SFINAE with readable constraints
- Template bloat: many instantiations hurt icache in hot paths
- `constexpr` has replaced most TMP (template metaprogramming) in modern C++

---

## Small String Optimization (SSO)

**An optimization where short strings are stored in the string object itself (on the stack) rather than heap-allocated, avoiding malloc for common short strings**

**What the interviewer actually asked**
"Implement SSO." (DE Shaw)

**The shallow answer**
"Short strings are stored inside the string object instead of on the heap."

**The deep answer**
- std::string is typically 24 or 32 bytes (depends on implementation). On 64-bit: libstdc++ = 32 bytes, libc++ = 24 bytes, MSVC = 32 bytes.
- Those 24/32 bytes can BE the string data for short strings. No heap allocation needed.
- Two representations (discriminated union):
  - Small mode: inline buffer (SSO buffer), 1 byte for length, flag indicating small mode. libc++ fits 22 chars + null in 23 bytes with the remaining byte for metadata.
  - Large mode: pointer (8 bytes) + size (8 bytes) + capacity (8 bytes) = 24 bytes of metadata, data on heap.
- The discriminant (is_small flag) is typically hidden in a bit of the capacity field, or in the high bit of the length byte, depending on implementation.
- SSO threshold: libc++: 22 chars, libstdc++ (gcc): 15 chars (32-byte object), MSVC: 15 chars.
- Why this matters for HFT: ticker symbols (AAPL, MSFT, BTC-USD), field names, order IDs — typically < 15 chars. SSO means accessing or copying these is cache-local, no malloc, no free, no heap fragmentation.
- Caveat: `data()` pointer of a small string points INTO the string object. Consequence: move of an SSO string must copy the inline buffer (not just swap a pointer), so SSO strings are NOT O(1) to move for small strings — they're O(n) where n ≤ 15. This is still fast (< 15 bytes = fits in a register file), but it's not a pointer swap.

**Implementation requirement?**
YES:

```cpp
#include <cstring>
#include <cassert>
#include <stdexcept>
#include <algorithm>

class SmallString {
    static constexpr size_t SSO_CAPACITY = 15;  // max chars inline (+ 1 for null)

    // -------------------------------------------------------
    // Layout: 24 bytes total
    // Small mode: [data_[0..14]] [length (1 byte, high bit = 0)]
    // Large mode: [ptr_ (8)] [size_ (8)] [cap_ with high bit = 1 (8)]
    // -------------------------------------------------------
    union {
        // Large mode: heap-allocated
        struct {
            char*  ptr;       // 8 bytes: heap pointer
            size_t size;      // 8 bytes: length (not including null)
            size_t cap;       // 8 bytes: capacity; high bit set = large mode
        } large_;

        // Small mode: inline buffer
        struct {
            char   buf[SSO_CAPACITY + 1];  // 16 bytes: inline data (null-terminated)
            // We overload buf[15] as: length in small mode (high bit clear)
            // So we effectively have 15 chars of storage (indices 0..14) plus
            // the length encoded in the last byte.
        } small_;
    };

    // Discriminant: we store the length in small_.buf[SSO_CAPACITY]
    // If high bit of that byte is 0 -> small mode, value = length
    // If high bit is 1               -> large mode (embedded in large_.cap)

    bool is_large() const noexcept {
        // In large mode, large_.cap has its high bit set
        // Check via the raw byte at offset SSO_CAPACITY (same memory)
        return (small_.buf[SSO_CAPACITY] & 0x80) != 0;
    }

    void set_small_size(size_t n) noexcept {
        assert(n <= SSO_CAPACITY);
        // Store length in last byte, high bit = 0 (small marker)
        small_.buf[n] = '\0';                   // null-terminate
        small_.buf[SSO_CAPACITY] = static_cast<char>(n);  // length, high bit clear
    }

    void set_large_size(size_t n) noexcept {
        large_.size = n;
    }

    static constexpr size_t LARGE_FLAG = size_t(1) << (sizeof(size_t)*8 - 1);

public:
    SmallString() noexcept {
        // Default: empty small string
        std::memset(small_.buf, 0, sizeof(small_.buf));
        // small_.buf[SSO_CAPACITY] = 0 -> small mode, length = 0
    }

    SmallString(const char* s) {
        size_t n = std::strlen(s);
        if (n <= SSO_CAPACITY) {
            // Small mode: copy into inline buffer
            std::memcpy(small_.buf, s, n + 1);  // +1 for null
            small_.buf[SSO_CAPACITY] = static_cast<char>(n);  // length, high bit = 0
        } else {
            // Large mode: heap allocate
            large_.ptr  = new char[n + 1];
            large_.size = n;
            large_.cap  = (n + 1) | LARGE_FLAG;  // set high bit = large marker
            std::memcpy(large_.ptr, s, n + 1);
        }
    }

    SmallString(const SmallString& o) {
        if (!o.is_large()) {
            // Small: copy entire 16-byte inline buffer (trivially)
            std::memcpy(small_.buf, o.small_.buf, sizeof(small_.buf));
        } else {
            // Large: allocate and copy
            size_t n    = o.large_.size;
            large_.ptr  = new char[n + 1];
            large_.size = n;
            large_.cap  = o.large_.cap;
            std::memcpy(large_.ptr, o.large_.ptr, n + 1);
        }
    }

    SmallString(SmallString&& o) noexcept {
        if (!o.is_large()) {
            // Small mode: MUST copy the inline buffer — pointer swap impossible
            // because the data IS the object, not a separate heap allocation.
            // This is O(16) = 2 x 64-bit register copies — effectively free.
            std::memcpy(small_.buf, o.small_.buf, sizeof(small_.buf));
            o.small_.buf[SSO_CAPACITY] = 0;  // set source to empty
        } else {
            // Large mode: steal the pointer — O(1)
            large_   = o.large_;
            o.large_.ptr  = nullptr;
            o.large_.size = 0;
            o.large_.cap  = 0;
        }
    }

    SmallString& operator=(const SmallString& o) {
        if (this != &o) {
            SmallString tmp(o);
            swap(tmp);
        }
        return *this;
    }

    SmallString& operator=(SmallString&& o) noexcept {
        if (this != &o) {
            if (is_large()) delete[] large_.ptr;
            new (this) SmallString(std::move(o));  // placement new to reuse storage
        }
        return *this;
    }

    ~SmallString() noexcept {
        if (is_large()) delete[] large_.ptr;
    }

    const char* c_str() const noexcept {
        return is_large() ? large_.ptr : small_.buf;
    }

    const char* data() const noexcept { return c_str(); }

    size_t size() const noexcept {
        if (is_large()) return large_.size;
        return static_cast<unsigned char>(small_.buf[SSO_CAPACITY]);
    }

    bool empty() const noexcept { return size() == 0; }

    size_t capacity() const noexcept {
        if (is_large()) return (large_.cap & ~LARGE_FLAG) - 1;
        return SSO_CAPACITY;
    }

    char operator[](size_t i) const noexcept {
        return c_str()[i];
    }

    void swap(SmallString& o) noexcept {
        // Raw byte swap of entire object — works for both small and large mode
        char tmp[sizeof(SmallString)];
        std::memcpy(tmp,   this,  sizeof(SmallString));
        std::memcpy(this,  &o,    sizeof(SmallString));
        std::memcpy(&o,    tmp,   sizeof(SmallString));
    }
};

static_assert(sizeof(SmallString) == 24 || sizeof(SmallString) == 32,
              "SSO string should be 24 or 32 bytes");
```

**The follow-up trap**
"Does moving a small SSO string invalidate iterators/pointers?"
Answer: Yes. Moving a small string copies the inline buffer to the destination object. Any pointer into the source string's inline buffer now points into the old object, which has been cleared. The destination object's data() returns a pointer into the DESTINATION's inline buffer. This is a critical difference from large strings (where data() returns the heap pointer, which is still valid after move if you kept a copy). General rule: never keep raw pointers into std::string data across any operation that might reallocate or move it.

**Key numbers / facts**
- SSO threshold: libc++: 22 chars, libstdc++: 15 chars, MSVC: 15 chars
- sizeof(std::string): libc++ = 24 bytes, libstdc++ = 32 bytes, MSVC = 32 bytes
- Move of SSO string: copies inline buffer (O(n), n ≤ 15/22) — not O(1)
- Move of large string: pointer swap — O(1)
- SSO eliminates malloc for all strings ≤ threshold — critical for HFT ticker/field name access

---

## Spinlock Implementation

**A synchronization primitive that busy-waits (spins) instead of yielding to the OS, trading CPU cycles for lower latency when contention is brief**

**What the interviewer actually asked**
"Implement a spinlock." (DE Shaw)

**The shallow answer**
"Use an atomic bool and spin in a while loop."

**The deep answer**
- `std::atomic_flag` is the ONLY C++ type guaranteed by the standard to be lock-free on all implementations. `std::atomic<bool>` is lock-free on all real hardware but the standard only guarantees lock-free IF `is_lock_free()` returns true. Use `atomic_flag` for spinlocks.
- `test_and_set()`: atomically sets the flag to true and returns the previous value. If previous value was false (unlocked), we acquired the lock. If true (locked), we loop.
- Memory order: `acquire` on lock (see all writes that happened before the previous `release`), `release` on unlock (publish our writes to the next acquirer). This is the minimal ordering needed for a correct mutex.
- `pause` instruction (x86: `_mm_pause()`, ARM: `__yield()`): hints the CPU that we're in a spin loop. Reduces power consumption. More importantly: reduces memory bus contention in hyperthreaded CPUs (the spinning thread yields execution resources to the sibling thread). On x86, pause delays ~100 cycles. Without it, the spin loop can cause cache-line bouncing between the spinning core and the lock-holding core at full memory bus bandwidth.
- False sharing: if the atomic_flag shares a cache line with other data, every lock/unlock bounces that entire cache line between cores. Solution: `alignas(64)` on the spinlock to give it its own cache line.
- Exponential backoff: on first failure, pause once. On second failure, pause twice. Etc. Reduces contention on the lock variable. Used in high-contention scenarios.
- When spinlock wins vs mutex: mutex has ~100-1000ns syscall overhead (futex on Linux). Spinlock wins if the critical section is held for less than ~30-50ns (less time than the context switch cost). HFT: order book update, ring buffer push — typically < 30ns. Use spinlock. Longer operations: use mutex.
- Recursive spinlocks: a thread that already holds the lock must not call `lock()` again — deadlock (tries to acquire from itself). Spinlocks are NOT reentrant.

**Implementation requirement?**
YES:

```cpp
#include <atomic>
#include <immintrin.h>  // _mm_pause() — x86 pause instruction

// -------------------------------------------------------
// Basic spinlock using std::atomic_flag
// -------------------------------------------------------
class Spinlock {
    // alignas(64): own cache line — prevents false sharing with adjacent data
    alignas(64) std::atomic_flag flag_ = ATOMIC_FLAG_INIT;

public:
    // lock(): spin until we acquire
    void lock() noexcept {
        for (;;) {
            // Fast path: try to acquire immediately (not yet contended)
            // memory_order_acquire: all subsequent reads/writes happen AFTER this
            if (!flag_.test_and_set(std::memory_order_acquire)) {
                return;  // acquired
            }
            // Slow path: already locked — spin with backoff
            // First, test without test_and_set to avoid write-invalidating the
            // cache line on every iteration (test is a read, TAS is a write)
            while (flag_.test(std::memory_order_relaxed)) {
                _mm_pause();  // x86 PAUSE: reduces bus contention on HT cores
                              // ~140 cycles delay, hints spin loop to CPU
            }
            // Cache line now shows unlocked (test returned false) — retry TAS
        }
    }

    // try_lock(): attempt once, return false if not acquired
    bool try_lock() noexcept {
        return !flag_.test_and_set(std::memory_order_acquire);
    }

    // unlock(): release the lock
    void unlock() noexcept {
        // memory_order_release: all preceding reads/writes visible before unlock
        flag_.clear(std::memory_order_release);
    }
};

// -------------------------------------------------------
// Spinlock with exponential backoff — for high-contention scenarios
// -------------------------------------------------------
class SpinlockBackoff {
    alignas(64) std::atomic_flag flag_ = ATOMIC_FLAG_INIT;

    static void spin(int count) noexcept {
        for (int i = 0; i < count; ++i)
            _mm_pause();
    }

public:
    void lock() noexcept {
        int backoff = 1;
        for (;;) {
            if (!flag_.test_and_set(std::memory_order_acquire))
                return;

            // Exponential backoff: 1, 2, 4, 8, ... pauses (capped at 64)
            spin(backoff);
            backoff = std::min(backoff * 2, 64);

            // Test without TAS to avoid thrashing cache line
            while (flag_.test(std::memory_order_relaxed)) {
                spin(backoff);
            }
        }
    }

    bool try_lock() noexcept {
        return !flag_.test_and_set(std::memory_order_acquire);
    }

    void unlock() noexcept {
        flag_.clear(std::memory_order_release);
    }
};

// -------------------------------------------------------
// RAII lock guard (compatible with std::lock_guard interface)
// -------------------------------------------------------
template<typename SpinlockT>
class SpinlockGuard {
    SpinlockT& lock_;
public:
    explicit SpinlockGuard(SpinlockT& l) noexcept : lock_(l) { lock_.lock(); }
    ~SpinlockGuard() noexcept { lock_.unlock(); }

    SpinlockGuard(const SpinlockGuard&)            = delete;
    SpinlockGuard& operator=(const SpinlockGuard&) = delete;
};

// -------------------------------------------------------
// Usage
// -------------------------------------------------------
// Spinlock sl;
// {
//     SpinlockGuard<Spinlock> guard(sl);
//     // critical section — update order book, push to ring buffer, etc.
// }  // automatically unlocked

// -------------------------------------------------------
// Alternative: spinlock from std::atomic<bool>
// (slightly less portable — atomic_flag is always lock-free)
// -------------------------------------------------------
class SpinlockAtomic {
    alignas(64) std::atomic<bool> locked_{false};

public:
    void lock() noexcept {
        bool expected = false;
        while (!locked_.compare_exchange_weak(
                   expected, true,
                   std::memory_order_acquire,
                   std::memory_order_relaxed)) {
            expected = false;  // reset after CAS failure
            while (locked_.load(std::memory_order_relaxed))
                _mm_pause();
        }
    }

    bool try_lock() noexcept {
        bool expected = false;
        return locked_.compare_exchange_strong(
            expected, true,
            std::memory_order_acquire,
            std::memory_order_relaxed);
    }

    void unlock() noexcept {
        locked_.store(false, std::memory_order_release);
    }
};
```

**The follow-up trap**
"Why do you test with `flag_.test()` before retrying `test_and_set()`?"
Answer: This is the "test, test-and-set" (TTAS) pattern. `test_and_set` is a write (RMW operation) — it always issues a write to the cache line, causing cache-line invalidation on all other cores. If you spin with bare `test_and_set`, every iteration bounces the cache line between the spinning core and the holding core, saturating the memory bus. By first using `test` (a read), the spinning core keeps the cache line in Shared state (multiple readers fine). Only when the read shows the lock is free do we attempt `test_and_set`. This reduces cache-line bouncing from O(spinning_iterations) to O(1 per acquire attempt).

**Key numbers / facts**
- `std::atomic_flag`: only type guaranteed lock-free by the C++ standard
- `memory_order_acquire` on lock, `memory_order_release` on unlock — minimum correct ordering
- `_mm_pause()`: ~140 cycles delay on modern x86, reduces HT core bus contention
- Spinlock vs mutex crossover: ~30-50ns critical section duration
- TTAS pattern: read first, then write — reduces cache-line bouncing dramatically
- `alignas(64)`: one cache line per spinlock — prevents false sharing
- Spinlocks are NOT reentrant — same thread calling lock() twice = deadlock
