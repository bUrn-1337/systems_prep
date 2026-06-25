```markdown
## C++ Output Prediction

These 10 snippets cover categories tested by QuantBox, Graviton, and similar HFT shops. For each, assume a 64-bit Linux system with GCC/Clang unless noted.

---

### 1. vtable dispatch — virtual vs non-virtual through base pointer

```cpp
#include <iostream>
struct Base {
    void nonvirt() { std::cout << "Base::nonvirt\n"; }
    virtual void virt() { std::cout << "Base::virt\n"; }
};
struct Derived : Base {
    void nonvirt() { std::cout << "Derived::nonvirt\n"; }
    void virt() override { std::cout << "Derived::virt\n"; }
};
int main() {
    Derived d;
    Base* p = &d;
    p->nonvirt();
    p->virt();
}
```

**Weak candidate answer:** Both print `Derived::` because `p` points to a `Derived` object.

**Actual output:**
```
Base::nonvirt
Derived::virt
```

**Why:** Non-virtual calls are resolved at compile time using the static type of the pointer (`Base*`), so `nonvirt()` dispatches to `Base::nonvirt` regardless of the actual object. Virtual calls use the vtable pointer embedded in the object at runtime, so `virt()` correctly dispatches to `Derived::virt`. This distinction is the entire basis of runtime polymorphism in C++.

---

### 2. Virtual destructor — UB demo (Graviton-style: find the deliberate error)

```cpp
#include <iostream>
struct Resource {
    int* data;
    Resource() : data(new int[100]) {
        std::cout << "acquired\n";
    }
    ~Resource() {                        // <-- deliberate error: not virtual
        delete[] data;
        std::cout << "released\n";
    }
};
struct Handle : Resource {
    char* name;
    Handle() : name(new char[64]) {}
    ~Handle() {
        delete[] name;
        std::cout << "name released\n";
    }
};
int main() {
    Resource* r = new Handle();
    delete r;
}
```

**Weak candidate answer:** Output is `acquired`, `name released`, `released` — all three destructors run.

**Actual output:** `acquired` then `released` (undefined behavior; `name released` is skipped, `name` leaks, behavior is technically UB)

**Why:** Without a virtual destructor in `Base`, `delete r` calls `Resource::~Resource` directly via static dispatch — `Handle::~Handle` is never invoked. The `name` allocation leaks silently. The C++ standard classifies deleting a derived object through a non-virtual-destructor base pointer as undefined behavior (even if it appears to "work"). The deliberate error is the missing `virtual` on `~Resource`. Graviton asks candidates to identify this exact pattern.

---

### 3. Object slicing — assigning derived to base by value

```cpp
#include <iostream>
struct Animal {
    virtual std::string sound() const { return "..."; }
};
struct Dog : Animal {
    std::string sound() const override { return "woof"; }
};
void speak(Animal a) {          // passed by value, not reference
    std::cout << a.sound() << "\n";
}
int main() {
    Dog d;
    speak(d);
}
```

**Weak candidate answer:** `woof` — the virtual dispatch will still find `Dog::sound`.

**Actual output:** `...`

**Why:** Passing `d` by value to a `Animal` parameter invokes `Animal`'s copy constructor, constructing a new `Animal` object — the `Dog` portion is silently discarded ("sliced off"). The resulting object's vptr points to `Animal`'s vtable, not `Dog`'s, so `sound()` resolves to `Animal::sound`. This is why polymorphic hierarchies should almost always be passed by pointer or reference, never by value.

---

### 4. Static initialization order fiasco — two translation units

```cpp
// --- unit_a.cpp ---
#include <iostream>
int x = 42;

// --- unit_b.cpp ---
extern int x;
int y = x + 1;

// --- main.cpp ---
#include <iostream>
extern int y;
int main() {
    std::cout << y << "\n";
}
```

**Weak candidate answer:** `43` — `x` is 42 so `y` is 43.

**Actual output:** `43` or `1` depending on link order (undefined behavior across translation units)

**Why:** The C++ standard does not specify the initialization order of static-duration variables across translation units. If `unit_b.cpp` initializes before `unit_a.cpp`, `x` is still zero-initialized (its dynamic initialization hasn't run yet), so `y` becomes `0 + 1 = 1`. The canonical fix is the "construct on first use" idiom: wrap `x` in a function returning a static local, guaranteeing it is initialized before first use.

---

### 5. Unsigned integer underflow — classic loop bug

```cpp
#include <iostream>
#include <vector>
int main() {
    std::vector<int> v = {10, 20, 30};
    for (std::size_t i = v.size() - 1; i >= 0; --i) {
        std::cout << v[i] << " ";
    }
}
```

**Weak candidate answer:** `30 20 10 ` — iterates backwards and stops.

**Actual output:** undefined behavior (infinite loop, then out-of-bounds access, likely crash or garbage output)

**Why:** `std::size_t` is unsigned. After printing `v[0]`, `i` is decremented from `0` to `std::numeric_limits<size_t>::max()` (typically `18446744073709551615` on 64-bit), which is always `>= 0` — the loop condition is always true. The subsequent `v[i]` is a massive out-of-bounds access causing UB. The fix is to use a signed index, a reverse iterator, or a range-based loop.

---

### 6. Signed integer overflow — UB, not wrap

```cpp
#include <iostream>
#include <limits>
int main() {
    int x = std::numeric_limits<int>::max();
    int y = x + 1;
    std::cout << (y > 0 ? "positive" : "non-positive") << "\n";
    std::cout << y << "\n";
}
```

**Weak candidate answer:** `non-positive` then `-2147483648` — signed overflow wraps to `INT_MIN` like two's complement.

**Actual output:** undefined behavior — compiler may print `positive` and any value, or optimize the branch away entirely

**Why:** Signed integer overflow is undefined behavior in C++, not guaranteed two's-complement wraparound (until C++20 mandates two's-complement representation, but overflow itself remains UB). GCC with `-O2` may see `x + 1 > 0` as trivially true (since `x` is a signed int and addition of a positive constant to a signed int is "always positive" under no-UB assumptions), eliminating the branch entirely. With `-fwrapv` the output would be `non-positive -2147483648`, but that's a non-standard extension.

---

### 7. Dangling reference — returning ref to local

```cpp
#include <iostream>
const int& get() {
    int local = 99;
    return local;        // returns reference to stack variable
}
int main() {
    const int& r = get();
    std::cout << r << "\n";
}
```

**Weak candidate answer:** `99` — the const reference extends the lifetime of the temporary.

**Actual output:** undefined behavior — likely `99`, `0`, or a crash depending on stack activity

**Why:** Lifetime extension via `const` reference binding only applies when the reference is bound directly to a temporary at the call site, not when the reference is returned from a function. Here `local` is destroyed when `get()` returns; `r` is a dangling reference. Reading through `r` is UB. GCC/Clang will warn with `-Wall`. In practice the stack frame may be intact (printing `99`) or immediately overwritten — neither outcome is guaranteed.

---

### 8. Move semantics — moved-from object state

```cpp
#include <iostream>
#include <string>
int main() {
    std::string a = "hello";
    std::string b = std::move(a);
    std::cout << a.size() << "\n";
    std::cout << (a == "") << "\n";
    a = "world";
    std::cout << a << "\n";
}
```

**Weak candidate answer:** crash or UB because `a` was moved from and is now invalid.

**Actual output:**
```
0
1
world
```

**Why:** After a move, `a` is in a "valid but unspecified state" per the standard. For `std::string`, virtually all implementations leave the moved-from object as an empty string (`size() == 0`), because the small-string optimization or heap pointer is transferred to `b`. Crucially, the moved-from object is still destructible and reassignable — using it after reassignment (`a = "world"`) is perfectly well-defined. The first two lines reflect implementation behavior (empty string), not a guarantee.

---

### 9. Struct padding — sizeof with mixed types

```cpp
#include <iostream>
struct A {
    char  c;    // 1 byte
    int   i;    // 4 bytes
    char  d;    // 1 byte
};
struct B {
    int   i;    // 4 bytes
    char  c;    // 1 byte
    char  d;    // 1 byte
};
int main() {
    std::cout << sizeof(A) << "\n";
    std::cout << sizeof(B) << "\n";
}
```

**Weak candidate answer:** Both are `6` (1+4+1).

**Actual output:**
```
12
8
```

**Why:** The compiler aligns `int` to a 4-byte boundary. In `A`, `c` (1 byte) is followed by 3 bytes of padding before `i`, and `d` (1 byte) is followed by 3 bytes of tail padding to make the total a multiple of `alignof(int)=4`, giving `1+3+4+1+3=12`. In `B`, `i` is first (already aligned), then `c` and `d` pack into 2 bytes with 2 bytes of tail padding to align to 4, giving `4+1+1+2=8`. Reordering fields from largest to smallest minimizes padding.

---

### 10. Multiple inheritance virtual dispatch — QuantBox chain prediction

```cpp
#include <iostream>
struct A {
    virtual void f() { std::cout << "A"; }
    virtual void g() { std::cout << "A::g "; f(); }
};
struct B : virtual A {
    void f() override { std::cout << "B"; }
};
struct C : virtual A {
    void f() override { std::cout << "C"; }
    void g() override { std::cout << "C::g "; f(); }
};
struct D : B, C {
    // does not override f() or g()
};
int main() {
    D d;
    A* a = &d;
    a->g();
    std::cout << "\n";
    a->f();
    std::cout << "\n";
}
```

**Weak candidate answer:** `A::g B` then `B` — `B` comes first in the inheritance list so it wins.

**Actual output:**
```
C::g D::f-candidate ambiguity... 
```

Actually:
```
C::g C
D
```

Wait — compiling resolves it: `C::g` wins over `A::g` in the vtable of `D` because `C`'s override is more derived in the override chain. Inside `C::g`, `f()` is a virtual call on `this` (type `D`), and `D` must have a unique final overrider for `f()` — but `B::f` and `C::f` are both candidates. This code **fails to compile** with: `error: no unique final overrider for 'virtual void A::f()' in 'D'`.

**Actual output:** Compilation error

**Why:** With virtual inheritance, the single shared `A` subobject requires exactly one final overrider for each virtual function in the most-derived class. `D` inherits `B::f` and `C::f` — both override `A::f` — creating an ambiguity. The compiler rejects the program. The fix is for `D` to explicitly override `f()`. This is a classic QuantBox trap: candidates who reason about vtable dispatch without knowing the "unique final overrider" rule will predict output for code that never compiles.
```
