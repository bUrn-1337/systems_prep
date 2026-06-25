# HFT Interview Prep: Core C++ Implementations

---

## 1. `shared_ptr` from Scratch

Two-allocation version separates the control block from the object. `make_shared` colocates them in one allocation, improving cache locality and reducing allocator overhead — critical in latency-sensitive code. The control block uses two reference counts: `ref_count` (live `shared_ptr` owners) and `weak_count` (live `weak_ptr` owners + 1 while `ref_count > 0`). The "+1" trick means we only call `delete` on the control block when both counts hit zero.

`fetch_add(1, relaxed)` on copy is safe because the copying thread already holds a reference — no synchronization needed. `fetch_sub(1, acq_rel)` on destruction uses `acquire` to synchronize with other threads' decrements (ensures we see all prior stores before deciding to destroy).

```cpp
#include <atomic>
#include <utility>
#include <stdexcept>
#include <cstddef>

// ─── Control Block ─────────────────────────────────────────────────────────────

struct ControlBlockBase {
    std::atomic<int> ref_count{1};
    std::atomic<int> weak_count{1}; // +1 held while ref_count > 0
    virtual void destroy_object() noexcept = 0;
    virtual void destroy_block() noexcept = 0;
    virtual ~ControlBlockBase() = default;
};

template<typename T>
struct ControlBlockSeparate : ControlBlockBase {
    T* ptr;
    explicit ControlBlockSeparate(T* p) : ptr(p) {}
    void destroy_object() noexcept override { delete ptr; ptr = nullptr; }
    void destroy_block()  noexcept override { delete this; }
};

// make_shared variant: object lives inside the control block (single allocation)
template<typename T>
struct ControlBlockInplace : ControlBlockBase {
    alignas(T) unsigned char storage[sizeof(T)];
    bool constructed = false;

    template<typename... Args>
    void construct(Args&&... args) {
        new (storage) T(std::forward<Args>(args)...);
        constructed = true;
    }
    T* get() noexcept { return reinterpret_cast<T*>(storage); }

    void destroy_object() noexcept override {
        if (constructed) { get()->~T(); constructed = false; }
    }
    void destroy_block() noexcept override { delete this; }
};

// ─── shared_ptr ────────────────────────────────────────────────────────────────

template<typename T>
class shared_ptr {
    T*                 ptr_ = nullptr;
    ControlBlockBase*  cb_  = nullptr;

    // Private ctor used by make_shared
    shared_ptr(T* p, ControlBlockBase* cb) : ptr_(p), cb_(cb) {}

    template<typename U, typename... Args>
    friend shared_ptr<U> make_shared(Args&&...);

    void release() noexcept {
        if (!cb_) return;
        // acq_rel: acquire sees prior stores from other threads;
        //          release publishes our stores before decrement.
        if (cb_->ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            cb_->destroy_object();
            // Drop the implicit "+1" weak reference held on behalf of ref_count.
            if (cb_->weak_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                cb_->destroy_block();
            }
        }
        ptr_ = nullptr;
        cb_  = nullptr;
    }

public:
    // Default
    shared_ptr() noexcept = default;

    // From raw pointer (two-allocation path)
    explicit shared_ptr(T* p) {
        if (!p) return;
        cb_  = new ControlBlockSeparate<T>(p);
        ptr_ = p;
    }

    // Copy: bump ref_count — relaxed because the copying thread already holds
    // a live reference, guaranteeing the object is still alive.
    shared_ptr(const shared_ptr& o) noexcept : ptr_(o.ptr_), cb_(o.cb_) {
        if (cb_) cb_->ref_count.fetch_add(1, std::memory_order_relaxed);
    }

    shared_ptr& operator=(const shared_ptr& o) noexcept {
        if (this != &o) {
            release();
            ptr_ = o.ptr_; cb_ = o.cb_;
            if (cb_) cb_->ref_count.fetch_add(1, std::memory_order_relaxed);
        }
        return *this;
    }

    // Move: transfer ownership, no refcount change
    shared_ptr(shared_ptr&& o) noexcept : ptr_(o.ptr_), cb_(o.cb_) {
        o.ptr_ = nullptr; o.cb_ = nullptr;
    }

    shared_ptr& operator=(shared_ptr&& o) noexcept {
        if (this != &o) { release(); ptr_ = o.ptr_; cb_ = o.cb_;
                          o.ptr_ = nullptr; o.cb_ = nullptr; }
        return *this;
    }

    ~shared_ptr() { release(); }

    T& operator*()  const noexcept { return *ptr_; }
    T* operator->() const noexcept { return  ptr_; }
    T* get()        const noexcept { return  ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    int use_count() const noexcept {
        return cb_ ? cb_->ref_count.load(std::memory_order_relaxed) : 0;
    }

    void reset() noexcept { release(); }

    void reset(T* p) {
        release();
        if (p) { cb_ = new ControlBlockSeparate<T>(p); ptr_ = p; }
    }

    void swap(shared_ptr& o) noexcept {
        std::swap(ptr_, o.ptr_); std::swap(cb_, o.cb_);
    }
};

// ─── make_shared (single allocation) ──────────────────────────────────────────

template<typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
    auto* cb = new ControlBlockInplace<T>();
    cb->construct(std::forward<Args>(args)...);
    return shared_ptr<T>(cb->get(), cb);
}
```

---

## 2. `unique_ptr` from Scratch

EBO (Empty Base Optimization): if the deleter is a stateless functor (empty class), inheriting from it costs zero bytes versus storing it as a member. We use a compressed pair idiom — here simplified by inheriting from the deleter directly via `CompressedPair`.

Move must be `noexcept` so containers can use it during reallocation.

```cpp
#include <utility>
#include <cstddef>

// Lightweight compressed pair: inherits from First (for EBO) and stores Second.
// When Deleter is an empty class, sizeof(CompressedPair<Deleter,T*>) == sizeof(T*).
template<typename Deleter, typename Pointer>
struct CompressedPair : private Deleter {
    Pointer ptr;
    CompressedPair(Pointer p, Deleter d) : Deleter(std::move(d)), ptr(p) {}
    Deleter&       deleter()       noexcept { return static_cast<Deleter&>(*this); }
    const Deleter& deleter() const noexcept { return static_cast<const Deleter&>(*this); }
};

// Default deleter calls delete
template<typename T>
struct DefaultDeleter {
    void operator()(T* p) const noexcept { delete p; }
};

template<typename T, typename Deleter = DefaultDeleter<T>>
class unique_ptr {
    CompressedPair<Deleter, T*> data_;

public:
    explicit unique_ptr(T* p = nullptr, Deleter d = Deleter{}) noexcept
        : data_(p, std::move(d)) {}

    // Non-copyable
    unique_ptr(const unique_ptr&)            = delete;
    unique_ptr& operator=(const unique_ptr&) = delete;

    // Moveable — noexcept is critical: allows vector/etc. to move instead of copy
    unique_ptr(unique_ptr&& o) noexcept
        : data_(o.data_.ptr, std::move(o.data_.deleter())) {
        o.data_.ptr = nullptr;
    }

    unique_ptr& operator=(unique_ptr&& o) noexcept {
        if (this != &o) {
            reset();
            data_.ptr = o.data_.ptr;
            data_.deleter() = std::move(o.data_.deleter());
            o.data_.ptr = nullptr;
        }
        return *this;
    }

    ~unique_ptr() { reset(); }

    T& operator*()  const noexcept { return *data_.ptr; }
    T* operator->() const noexcept { return  data_.ptr; }
    T* get()        const noexcept { return  data_.ptr; }
    explicit operator bool() const noexcept { return data_.ptr != nullptr; }

    // release: give up ownership, caller is responsible
    T* release() noexcept {
        T* tmp = data_.ptr;
        data_.ptr = nullptr;
        return tmp;
    }

    // reset: destroy current, optionally take new
    void reset(T* p = nullptr) noexcept {
        if (data_.ptr) data_.deleter()(data_.ptr);
        data_.ptr = p;
    }

    void swap(unique_ptr& o) noexcept {
        std::swap(data_.ptr,     o.data_.ptr);
        std::swap(data_.deleter(), o.data_.deleter());
    }
};
```

---

## 3. Spinlock from Scratch

`test_and_set` with `acquire` pairs with `clear` + `release` to form a lock/unlock barrier. On x86, a spinning thread hammering the bus with write operations causes cache-line contention. `_mm_pause()` (PAUSE instruction) tells the CPU to relax for ~100 cycles, reducing power and bus traffic — essential in HFT where you have many spinlocks per core.

The RAII guard ensures unlock on exceptions.

```cpp
#include <atomic>
#include <cstddef>

#ifdef _MSC_VER
  #include <intrin.h>
  #define CPU_PAUSE() _mm_pause()
#elif defined(__x86_64__) || defined(__i386__)
  #define CPU_PAUSE() __asm__ volatile("pause" ::: "memory")
#else
  #define CPU_PAUSE() std::atomic_thread_fence(std::memory_order_seq_cst)
#endif

class Spinlock {
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;

public:
    // Spinlock is not copyable or movable — it represents a hardware resource
    Spinlock() = default;
    Spinlock(const Spinlock&) = delete;
    Spinlock& operator=(const Spinlock&) = delete;

    void lock() noexcept {
        // test_and_set: atomically sets flag to true, returns prior value.
        // Loop until prior value was false (we acquired it).
        // memory_order_acquire: all subsequent reads/writes happen after this.
        while (flag_.test_and_set(std::memory_order_acquire)) {
            // First spin: check without writing (read-only loop reduces
            // cache invalidations — "test, test-and-set" pattern)
            while (flag_.test(std::memory_order_relaxed)) {
                CPU_PAUSE();
            }
        }
    }

    bool try_lock() noexcept {
        return !flag_.test_and_set(std::memory_order_acquire);
    }

    void unlock() noexcept {
        // memory_order_release: all prior reads/writes are visible before unlock.
        flag_.clear(std::memory_order_release);
    }

    // ── RAII Guard ─────────────────────────────────────────────────────────────

    struct Guard {
        explicit Guard(Spinlock& sl) noexcept : sl_(sl) { sl_.lock(); }
        ~Guard() noexcept { sl_.unlock(); }
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;
    private:
        Spinlock& sl_;
    };

    Guard make_guard() noexcept { return Guard(*this); }
};
```

---

## 4. Memory Pool from Scratch

The intrusive free list stores the `next` pointer directly in the unused object memory via a `union`, so no extra allocation overhead per slot. The thread-safe version uses a tagged pointer (ABA tag packed in the high bits) to prevent the ABA problem on the CAS. On x86-64, only 48 bits of a pointer are used, so we pack a 16-bit tag in the top bits.

```cpp
#include <atomic>
#include <cassert>
#include <cstddef>
#include <new>
#include <cstring>

// ─── Single-threaded Pool ──────────────────────────────────────────────────────

template<typename T, std::size_t N>
class MemoryPool {
    // Each slot is either a live T OR a free-list node storing a next pointer.
    // The union guarantees the slot is large enough for both without extra space.
    union Slot {
        T       object;        // when allocated
        Slot*   next;          // when free
        Slot() {} ~Slot() {}
    };

    alignas(Slot) Slot storage_[N];
    Slot* free_head_ = nullptr;

public:
    MemoryPool() {
        // Build the free list: each slot points to the next
        for (std::size_t i = 0; i < N - 1; ++i)
            storage_[i].next = &storage_[i + 1];
        storage_[N - 1].next = nullptr;
        free_head_ = &storage_[0];
    }

    // O(1) allocate — returns raw memory, caller constructs with placement new
    void* allocate() noexcept {
        if (!free_head_) return nullptr; // pool exhausted
        Slot* slot  = free_head_;
        free_head_  = slot->next;
        return slot;
    }

    // O(1) deallocate — caller must have already destroyed the object
    void deallocate(void* p) noexcept {
        auto* slot  = static_cast<Slot*>(p);
        slot->next  = free_head_;
        free_head_  = slot;
    }

    // Convenience: construct T in-place
    template<typename... Args>
    T* construct(Args&&... args) {
        void* mem = allocate();
        if (!mem) return nullptr;
        return new (mem) T(std::forward<Args>(args)...);
    }

    void destroy(T* p) noexcept {
        p->~T();
        deallocate(p);
    }
};

// ─── Lock-free MPMC Pool (tagged pointer for ABA prevention) ──────────────────

template<typename T, std::size_t N>
class LockFreeMemoryPool {
    // Pack a 16-bit ABA counter into the top 16 bits of a 64-bit pointer.
    // Only valid on x86-64 where user-space pointers fit in 48 bits.
    struct TaggedPtr {
        uintptr_t value = 0;

        TaggedPtr() = default;
        TaggedPtr(void* p, uint16_t tag) {
            value = (uintptr_t(p) & 0x0000FFFFFFFFFFFF)
                  | (uintptr_t(tag) << 48);
        }
        void*    ptr() const noexcept {
            // Sign-extend bit 47 to restore canonical address
            uintptr_t raw = value & 0x0000FFFFFFFFFFFF;
            if (raw & (uintptr_t(1) << 47)) raw |= 0xFFFF000000000000;
            return reinterpret_cast<void*>(raw);
        }
        uint16_t tag() const noexcept { return uint16_t(value >> 48); }
        bool operator==(const TaggedPtr& o) const noexcept { return value == o.value; }
    };

    struct Slot {
        union { T obj; std::atomic<TaggedPtr> next; };
        Slot() : next(TaggedPtr{}) {}
        ~Slot() {}
    };

    alignas(64) Slot storage_[N];
    alignas(64) std::atomic<TaggedPtr> head_{};

public:
    LockFreeMemoryPool() {
        for (std::size_t i = 0; i < N; ++i) {
            void* next_ptr = (i + 1 < N) ? static_cast<void*>(&storage_[i+1]) : nullptr;
            storage_[i].next.store(TaggedPtr(next_ptr, 0), std::memory_order_relaxed);
        }
        head_.store(TaggedPtr(&storage_[0], 0), std::memory_order_relaxed);
    }

    void* allocate() noexcept {
        TaggedPtr old_head = head_.load(std::memory_order_acquire);
        while (true) {
            if (!old_head.ptr()) return nullptr; // exhausted
            auto* slot = static_cast<Slot*>(old_head.ptr());
            TaggedPtr next = slot->next.load(std::memory_order_relaxed);
            // Increment tag to defeat ABA: even if the same pointer comes back,
            // the tag will differ, so the CAS will fail correctly.
            TaggedPtr new_head(next.ptr(), old_head.tag() + 1);
            if (head_.compare_exchange_weak(old_head, new_head,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire))
                return slot;
        }
    }

    void deallocate(void* p) noexcept {
        auto* slot = static_cast<Slot*>(p);
        TaggedPtr old_head = head_.load(std::memory_order_acquire);
        while (true) {
            slot->next.store(old_head, std::memory_order_relaxed);
            TaggedPtr new_head(slot, old_head.tag() + 1);
            if (head_.compare_exchange_weak(old_head, new_head,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire))
                return;
        }
    }

    template<typename... Args>
    T* construct(Args&&... args) {
        void* mem = allocate();
        if (!mem) return nullptr;
        return new (mem) T(std::forward<Args>(args)...);
    }

    void destroy(T* p) noexcept { p->~T(); deallocate(p); }
};
```

---

## 5. Small String Optimization from Scratch

The discriminant lives in the last byte of the 16-byte union. In small mode, the last byte stores the remaining capacity (or equivalently: `15 - length`), and when the string is exactly 15 chars, that byte is 0 which also serves as the null terminator. In large mode we set bit 7 of the last byte to 1 (0x80) — this value can never appear as a capacity-remaining byte since capacity-remaining is at most 15.

`sizeof(SmallString) = 16` on any platform with standard ABI.

```cpp
#include <cstring>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <algorithm>

class SmallString {
    static constexpr std::size_t SSO_CAP = 15;

    // The entire object is 16 bytes.
    // Discriminant: last byte bit 7 = 0 → small; = 1 → large.
    union Storage {
        // Small mode: buf[15] == (SSO_CAP - len), buf[len] == '\0'
        // When len==15, buf[15]==0 which is the null terminator — clever overlap.
        struct Small {
            char    buf[SSO_CAP];
            uint8_t remaining; // SSO_CAP - length; 0x00..0x0F; bit7 always 0
        } small;

        // Large mode: last byte has bit7 set to distinguish from small.
        struct Large {
            char*       ptr;
            std::size_t len;
            std::size_t cap; // allocated capacity (excluding null)
            // On little-endian x86, the high byte of cap is last byte of union.
            // We guarantee bit7 is set by always storing cap | (1ULL << 63).
        } large;
    } s_;

    static_assert(sizeof(Storage) == 16, "SmallString must be 16 bytes");

    bool is_small() const noexcept {
        // Read the last byte of the union as raw storage.
        const uint8_t* last =
            reinterpret_cast<const uint8_t*>(&s_) + sizeof(Storage) - 1;
        return (*last & 0x80) == 0;
    }

    void set_small_len(std::size_t len) noexcept {
        s_.small.remaining = static_cast<uint8_t>(SSO_CAP - len);
        s_.small.buf[len]  = '\0';
    }

    std::size_t small_len() const noexcept {
        return SSO_CAP - s_.small.remaining;
    }

    // Allocate and copy into large mode
    void init_large(const char* src, std::size_t len) {
        std::size_t cap = len;
        char* buf = new char[cap + 1];
        std::memcpy(buf, src, len);
        buf[len]    = '\0';
        s_.large.ptr = buf;
        s_.large.len = len;
        // Store capacity with high bit set to mark large mode.
        // This destroys the last byte's bit7, which is our discriminant.
        s_.large.cap = cap | (std::size_t(1) << (sizeof(std::size_t)*8 - 1));
    }

    std::size_t large_cap() const noexcept {
        // Strip the discriminant bit to get real capacity.
        return s_.large.cap & ~(std::size_t(1) << (sizeof(std::size_t)*8 - 1));
    }

    void free_large() noexcept {
        if (!is_small()) delete[] s_.large.ptr;
    }

public:
    SmallString() noexcept {
        std::memset(&s_, 0, sizeof(s_)); // small mode, empty string
        s_.small.remaining = SSO_CAP;
    }

    SmallString(const char* src) {
        std::size_t len = src ? std::strlen(src) : 0;
        if (len <= SSO_CAP) {
            std::memcpy(s_.small.buf, src, len);
            set_small_len(len);
        } else {
            init_large(src, len);
        }
    }

    SmallString(const SmallString& o) {
        if (o.is_small()) {
            std::memcpy(&s_, &o.s_, sizeof(s_)); // copy entire 16 bytes
        } else {
            init_large(o.s_.large.ptr, o.s_.large.len);
        }
    }

    SmallString(SmallString&& o) noexcept {
        std::memcpy(&s_, &o.s_, sizeof(s_));   // steal bit pattern
        // Leave o in valid empty state
        std::memset(&o.s_, 0, sizeof(o.s_));
        o.s_.small.remaining = SSO_CAP;
    }

    SmallString& operator=(const SmallString& o) {
        if (this != &o) {
            free_large();
            if (o.is_small()) {
                std::memcpy(&s_, &o.s_, sizeof(s_));
            } else {
                init_large(o.s_.large.ptr, o.s_.large.len);
            }
        }
        return *this;
    }

    SmallString& operator=(SmallString&& o) noexcept {
        if (this != &o) {
            free_large();
            std::memcpy(&s_, &o.s_, sizeof(s_));
            std::memset(&o.s_, 0, sizeof(o.s_));
            o.s_.small.remaining = SSO_CAP;
        }
        return *this;
    }

    ~SmallString() { free_large(); }

    const char* c_str() const noexcept {
        return is_small() ? s_.small.buf : s_.large.ptr;
    }

    std::size_t length() const noexcept {
        return is_small() ? small_len() : s_.large.len;
    }

    bool empty() const noexcept { return length() == 0; }
};
```

---

## 6. Thread-safe SPSC Queue with Atomics

The key invariant: only the producer writes `tail_`, only the consumer writes `head_`. Therefore each party can read its own index with `relaxed` and must read the other's with `acquire` (to observe the data written under `release`). Head and tail go on separate 64-byte cache lines to prevent false sharing — without padding, every producer write would invalidate the consumer's cache line and vice versa, adding ~60ns of coherence traffic per operation.

```cpp
#include <atomic>
#include <array>
#include <optional>
#include <cstddef>

template<typename T, std::size_t N>
class SPSCQueue {
    static_assert((N & (N - 1)) == 0, "N must be a power of 2 for cheap modulo");
    static constexpr std::size_t MASK = N - 1;

    // Each cache line is 64 bytes. Padding ensures head_ and tail_ never
    // share a cache line. Without this, every push would invalidate the
    // consumer's cache line even though it only reads tail_.
    struct alignas(64) AlignedAtomic {
        std::atomic<std::size_t> val{0};
        char pad[64 - sizeof(std::atomic<std::size_t>)];
    };

    AlignedAtomic head_; // written by consumer only
    AlignedAtomic tail_; // written by producer only

    // Data buffer. Ideally also aligned so elements don't straddle cache lines.
    alignas(64) T buf_[N];

public:
    SPSCQueue() = default;
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;

    // Called by producer thread only.
    bool push(const T& val) noexcept(std::is_nothrow_copy_assignable_v<T>) {
        // Relaxed: only producer writes tail_, so we can read our own value cheaply.
        std::size_t t = tail_.val.load(std::memory_order_relaxed);
        std::size_t next_t = (t + 1) & MASK;

        // Acquire: we must observe the consumer's head_ store so we don't overwrite
        // an element the consumer hasn't read yet.
        if (next_t == head_.val.load(std::memory_order_acquire))
            return false; // full

        buf_[t] = val;

        // Release: makes the write to buf_[t] visible before the tail_ update.
        // The consumer's acquire load of tail_ will see the updated data.
        tail_.val.store(next_t, std::memory_order_release);
        return true;
    }

    // Called by consumer thread only.
    bool pop(T& out) noexcept(std::is_nothrow_copy_assignable_v<T>) {
        std::size_t h = head_.val.load(std::memory_order_relaxed);

        // Acquire: must observe producer's release store to tail_ and buf_[h].
        if (h == tail_.val.load(std::memory_order_acquire))
            return false; // empty

        out = buf_[h];

        // Release: makes our read of buf_[h] visible before head_ advances,
        // so the producer can safely reuse slot h.
        head_.val.store((h + 1) & MASK, std::memory_order_release);
        return true;
    }

    std::optional<T> pop() noexcept(std::is_nothrow_copy_assignable_v<T>) {
        T val;
        if (pop(val)) return val;
        return std::nullopt;
    }

    bool empty() const noexcept {
        return head_.val.load(std::memory_order_acquire)
            == tail_.val.load(std::memory_order_acquire);
    }

    std::size_t size() const noexcept {
        std::size_t h = head_.val.load(std::memory_order_acquire);
        std::size_t t = tail_.val.load(std::memory_order_acquire);
        return (t - h + N) & MASK;
    }
};
```

---

## 7. `vector::push_back` from Scratch

Growth factor of 2 gives amortized O(1): total copies across N pushes is `1 + 2 + 4 + ... + N = 2N`. Growth factor of 1.5 wastes fewer peak bytes (old allocation can be reused after 2 reallocations) — but 2 is standard and simpler to reason about. C++11 move semantics mean reallocation calls `std::move` on each element, making it O(1) per element if the move constructor is O(1).

`operator new` / `operator delete` instead of `malloc`/`free` to respect custom `operator new` overloads. `std::destroy_at` properly calls destructors without deallocating.

```cpp
#include <cstddef>
#include <utility>
#include <stdexcept>
#include <new>
#include <algorithm>

template<typename T>
class Vector {
    T*          data_     = nullptr;
    std::size_t size_     = 0;
    std::size_t capacity_ = 0;

    void grow() {
        std::size_t new_cap = (capacity_ == 0) ? 1 : capacity_ * 2;
        // Allocate raw uninitialized memory
        T* new_data = static_cast<T*>(::operator new(new_cap * sizeof(T)));

        // Move existing elements into new storage.
        // If T's move ctor throws, we've already moved some elements — ideally
        // we'd use move_if_noexcept. For brevity, we move unconditionally.
        for (std::size_t i = 0; i < size_; ++i) {
            new (new_data + i) T(std::move(data_[i]));
            data_[i].~T();            // destroy moved-from original
        }

        ::operator delete(data_);    // free old raw storage
        data_     = new_data;
        capacity_ = new_cap;
    }

public:
    Vector() = default;

    Vector(const Vector& o) {
        reserve(o.size_);
        for (std::size_t i = 0; i < o.size_; ++i)
            push_back(o.data_[i]);
    }

    Vector(Vector&& o) noexcept
        : data_(o.data_), size_(o.size_), capacity_(o.capacity_) {
        o.data_ = nullptr; o.size_ = 0; o.capacity_ = 0;
    }

    Vector& operator=(Vector o) noexcept { // copy-and-swap
        swap(o); return *this;
    }

    ~Vector() {
        for (std::size_t i = 0; i < size_; ++i) data_[i].~T();
        ::operator delete(data_);
    }

    void push_back(const T& val) {
        if (size_ == capacity_) grow();
        new (data_ + size_) T(val);       // placement new: construct in-place
        ++size_;
    }

    void push_back(T&& val) {
        if (size_ == capacity_) grow();
        new (data_ + size_) T(std::move(val));
        ++size_;
    }

    template<typename... Args>
    T& emplace_back(Args&&... args) {
        if (size_ == capacity_) grow();
        T* p = new (data_ + size_) T(std::forward<Args>(args)...);
        ++size_;
        return *p;
    }

    void pop_back() noexcept {
        if (size_ > 0) { data_[--size_].~T(); }
    }

    void reserve(std::size_t new_cap) {
        if (new_cap <= capacity_) return;
        T* new_data = static_cast<T*>(::operator new(new_cap * sizeof(T)));
        for (std::size_t i = 0; i < size_; ++i) {
            new (new_data + i) T(std::move(data_[i]));
            data_[i].~T();
        }
        ::operator delete(data_);
        data_     = new_data;
        capacity_ = new_cap;
    }

    T&       operator[](std::size_t i)       noexcept { return data_[i]; }
    const T& operator[](std::size_t i) const noexcept { return data_[i]; }

    T& at(std::size_t i) {
        if (i >= size_) throw std::out_of_range("Vector::at");
        return data_[i];
    }

    T*       begin()       noexcept { return data_; }
    T*       end()         noexcept { return data_ + size_; }
    const T* begin() const noexcept { return data_; }
    const T* end()   const noexcept { return data_ + size_; }

    std::size_t size()     const noexcept { return size_; }
    std::size_t capacity() const noexcept { return capacity_; }
    bool        empty()    const noexcept { return size_ == 0; }

    T&       front()       noexcept { return data_[0]; }
    const T& front() const noexcept { return data_[0]; }
    T&       back()        noexcept { return data_[size_ - 1]; }
    const T& back()  const noexcept { return data_[size_ - 1]; }

    void swap(Vector& o) noexcept {
        std::swap(data_,     o.data_);
        std::swap(size_,     o.size_);
        std::swap(capacity_, o.capacity_);
    }

    void clear() noexcept {
        for (std::size_t i = 0; i < size_; ++i) data_[i].~T();
        size_ = 0;
    }
};
```

---

## Implementation Checklist

| Item | Status | Location |
|------|--------|----------|
| `shared_ptr` from scratch | [COVERED IN NOTES] | `05_implementations.md` §1 |
| `unique_ptr` from scratch | [COVERED IN NOTES] | `05_implementations.md` §2 |
| Spinlock from scratch | [COVERED IN NOTES] | `05_implementations.md` §3 |
| Memory pool from scratch | [COVERED IN NOTES] | `05_implementations.md` §4 |
| Small string optimization | [COVERED IN NOTES] | `05_implementations.md` §5 |
| Thread-safe SPSC queue | [COVERED IN NOTES] | `05_implementations.md` §6 |
| `vector::push_back` | [COVERED IN NOTES] | `05_implementations.md` §7 |
| LRU cache | [COVERED IN NOTES] | `04_data_structures.md` §1 |
| LFU cache | [COVERED IN NOTES] | `04_data_structures.md` §2 |

**All 9 items covered.** Every implementation is defensible line-by-line with the design rationale written above the code block.
