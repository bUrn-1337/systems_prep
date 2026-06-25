---
## Mutex vs Semaphore

**One-line definition**

Mutex: binary lock with ownership semantics. Semaphore: integer counter for signaling or resource counting, no ownership.

**What the interviewer actually asked**

"What's the difference between mutex and semaphore? When would you use each? What about busy waiting?" (Alphagrep, QuadEye, DE Shaw)

**The shallow answer** (gets you rejected)

"A mutex is for mutual exclusion and a semaphore is for signaling."

**The deep answer** (gets you through)

Mutex has ownership: only the thread that acquired the lock may release it. This is enforced by the OS (undefined behavior or error on POSIX if another thread unlocks). Consequence: enables priority inheritance — if a low-priority thread holds a mutex that a high-priority thread is waiting on, the low-priority thread temporarily inherits the high priority to prevent priority inversion. Recursive mutex allows the same thread to lock multiple times (must unlock same number of times). Mutex is not usable across processes without shared memory (`pthread_mutexattr_setpshared`).

Semaphore has no ownership: any thread can call `sem_post`. A binary semaphore is NOT a mutex — it has no ownership so priority inheritance is impossible. Counting semaphore tracks available resources (initialized to N). Classic use: producer calls `sem_post`, consumer calls `sem_wait` — decoupled threads, different roles. POSIX API: `sem_init`, `sem_wait` (decrement, block if zero), `sem_post` (increment, wake waiter), `sem_trywait` (non-blocking).

Busy waiting (spinlock): thread loops checking a flag in userspace, never sleeps. Burns CPU cycles — 100% utilization on that core while spinning. Appropriate when lock hold time is under ~30 ns and you cannot afford the ~1–10 µs kernel context-switch overhead of a sleeping mutex. In HFT: spinlock on the hot-path data structures (order book updates), mutex for initialization, config, or anything not on the critical latency path. `std::atomic_flag::test_and_set` with a `while` loop is the canonical spinlock.

**Implementation requirement?**

YES

```cpp
#include <atomic>
#include <thread>

// Spinlock using atomic_flag
class Spinlock {
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
public:
    void lock() {
        // test_and_set returns previous value; loop until we were the one
        // to flip false->true (i.e., we acquired the lock).
        // memory_order_acquire: all subsequent loads/stores happen after.
        while (flag_.test_and_set(std::memory_order_acquire)) {
            // Emit PAUSE hint: reduces power, avoids memory-order violation
            // pipeline flush on Intel when leaving the spin loop.
            _mm_pause(); // x86; on ARM use __yield()
        }
    }

    void unlock() {
        // memory_order_release: all previous stores are visible before
        // the flag is cleared, so the next thread to acquire sees valid data.
        flag_.clear(std::memory_order_release);
    }
};
```

**The follow-up trap**

"Why can't you use a binary semaphore as a mutex in a real-time system?"

Because there is no ownership, priority inheritance is impossible. If a low-priority thread holds the semaphore and a high-priority thread is blocked on it, a medium-priority thread can preempt and run indefinitely — classic priority inversion (Mars Pathfinder 1997 bug). A proper mutex with priority inheritance prevents this.

**Key numbers / facts**

- Mutex syscall overhead: ~1–10 µs (uncontended futex fast-path ~50–100 ns on Linux)
- Spinlock latency: ~5–30 ns (just an atomic CAS + loop)
- `LOCK XCHG` / `test_and_set` on x86: ~10–40 ns depending on cache state
- Priority inversion: Mars Pathfinder 1997 — VxWorks mutex without priority inheritance

---
## Deadlock — 4 Conditions, Prevention, Detection (RAG)

**One-line definition**

Deadlock: a cycle of threads each holding a resource that another thread in the cycle needs.

**What the interviewer actually asked**

"State the 4 conditions for deadlock. How do you detect it? What is a Resource Allocation Graph?" (DE Shaw)

**The shallow answer** (gets you rejected)

"Two threads waiting for each other's locks."

**The deep answer** (gets you through)

Coffman (1971) conditions — ALL four must hold simultaneously:

1. **Mutual exclusion**: at least one resource is held in a non-shareable mode (only one thread can use it at a time).
2. **Hold and wait**: a thread holds at least one resource and is waiting to acquire additional resources held by other threads.
3. **No preemption**: resources cannot be forcibly taken; they must be released voluntarily.
4. **Circular wait**: there exists a set {T1, T2, ..., Tn} such that T1 waits for T2, T2 waits for T3, ..., Tn waits for T1.

**Prevention** — break any one condition:
- Break circular wait (most practical in practice): impose a global lock ordering. Always acquire locks in the same order across all threads. If every thread acquires mutex_A before mutex_B, a cycle is impossible.
- Break hold-and-wait: acquire all resources atomically at once (`std::lock(m1, m2)` uses a deadlock-avoidance algorithm internally).
- Banker's algorithm: before granting a request, simulate allocation and check if the system remains in a "safe state" (there exists an ordering in which all threads can finish). Too expensive for HFT.

**Detection** — Resource Allocation Graph (RAG):
- Nodes: processes (circles) and resource types (squares).
- Edges: request edge P→R (process P is waiting for resource R), assignment edge R→P (resource R is assigned to process P).
- For single-instance resources: deadlock exists if and only if a cycle exists in the RAG.
- For multi-instance resources: cycle is necessary but not sufficient; use the Banker's detection algorithm.
- Cycle detection: DFS with two state arrays — `visited[]` (node ever visited) and `recStack[]` (node on current DFS path). If a back edge is found (neighbor is in recStack), a cycle exists.

**Implementation requirement?**

YES

```cpp
#include <vector>
#include <iostream>

// RAG cycle detection for single-instance resources.
// Graph represented as adjacency list (processes + resources as unified nodes).
class RAGDetector {
    int n_; // total nodes (processes + resources)
    std::vector<std::vector<int>> adj_;
    std::vector<bool> visited_;
    std::vector<bool> recStack_;

    bool dfsHasCycle(int u) {
        visited_[u] = true;
        recStack_[u] = true;

        for (int v : adj_[u]) {
            if (!visited_[v] && dfsHasCycle(v))
                return true;
            if (recStack_[v])   // back edge found — cycle
                return true;
        }

        recStack_[u] = false;   // backtrack
        return false;
    }

public:
    RAGDetector(int n) : n_(n), adj_(n), visited_(n, false), recStack_(n, false) {}

    // Add directed edge u -> v
    // Process Pi waiting for resource Rj: addEdge(process_i, resource_j)
    // Resource Rj held by process Pi: addEdge(resource_j, process_i)
    void addEdge(int u, int v) {
        adj_[u].push_back(v);
    }

    bool hasDeadlock() {
        // Reset state
        std::fill(visited_.begin(), visited_.end(), false);
        std::fill(recStack_.begin(), recStack_.end(), false);

        for (int i = 0; i < n_; ++i)
            if (!visited_[i] && dfsHasCycle(i))
                return true;
        return false;
    }
};

// Example:
// P0 holds R0 (R0->P0), P0 waits for R1 (P0->R1)
// P1 holds R1 (R1->P1), P1 waits for R0 (P1->R0)
// Nodes: P0=0, P1=1, R0=2, R1=3
// Edges: R0->P0 (2->0), P0->R1 (0->3), R1->P1 (3->1), P1->R0 (1->2)
// Cycle: 0->3->1->2->0 — deadlock detected.
```

**The follow-up trap**

"How do you prevent deadlock in practice in an HFT system with multiple shared data structures?"

Lock ordering by address: `if (&m1 < &m2) { lock(m1); lock(m2); } else { lock(m2); lock(m1); }` — universally consistent ordering without a hardcoded hierarchy. `std::lock(m1, m2)` does this internally using a try-and-backoff strategy (not strictly address ordering but deadlock-free).

**Key numbers / facts**

- `std::lock(m1, m2)` uses the "ight-hand rule" / try-lock-and-backoff — O(1) amortized, deadlock-free
- Coffman conditions: 1971 paper
- Banker's algorithm: O(n²r) per request where n = processes, r = resource types — unusable in low-latency code
- In HFT: lock ordering is the only practical approach; often prefer lock-free structures to avoid the problem entirely

---
## Thread-Safe Queue with Atomics (SPSC Ring Buffer)

**One-line definition**

A lock-free SPSC queue uses two independent atomic indices (head for consumer, tail for producer) with acquire/release ordering to pass data between threads without any mutex.

**What the interviewer actually asked**

"Implement a thread-safe queue. Can you do it without a mutex? What memory ordering do you need?" (DE Shaw)

**The shallow answer** (gets you rejected)

"`std::queue` with a `std::mutex` around every operation."

**The deep answer** (gets you through)

Lock-based: `std::queue<T>` + `std::mutex` + `std::condition_variable`. Simple, correct, but ~1–10 µs overhead per operation due to kernel involvement. Contention causes cache-line ping-pong on the mutex itself.

Lock-free SPSC: exactly one producer thread and one consumer thread. Head and tail are separate atomics — crucially, only the producer writes `tail` and only the consumer writes `head`. This eliminates write contention entirely. Producer: check `tail - head < capacity` (space available), write item, then `tail.store(release)`. Consumer: check `tail - head > 0` (item available), read item, then `head.store(release)`. The acquire on the load pairs with the release on the store to establish a happens-before edge: all writes to `data_[i]` before the tail store are visible after the tail load by the consumer.

Ring buffer: fixed-size, power-of-2 capacity so index masking is `& (N-1)` instead of modulo. No heap allocation after construction. Cache-line padding between head and tail to avoid false sharing (both on same 64-byte cache line → every write to head invalidates the producer's cached tail).

MPMC: requires a "sequence number" per slot (Dmitry Vyukov's bounded MPMC queue). Each slot has an atomic sequence. Producer: CAS on sequence to claim slot; consumer: CAS to consume. Widely used in disruptor-pattern systems.

Memory ordering summary:
- Producer `data_` write: plain (no ordering needed, protected by subsequent release store)
- Producer `tail_.store(release)`: makes the data write visible
- Consumer `tail_.load(acquire)`: sees the data write
- Consumer `data_` read: plain
- Consumer `head_.store(release)`: makes slot available to producer
- Producer `head_.load(acquire)`: sees freed slots

**Implementation requirement?**

YES

```cpp
#include <atomic>
#include <array>
#include <optional>
#include <cstddef>

// SPSC lock-free ring buffer queue.
// T must be trivially copyable for this implementation.
// Capacity must be a power of 2.
template<typename T, std::size_t Capacity>
class SPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0,
                  "Capacity must be a power of 2");
    static constexpr std::size_t MASK = Capacity - 1;

    // Padding prevents false sharing: head_ and tail_ on separate cache lines.
    // Without padding, a producer write to tail_ invalidates the consumer's
    // cached copy of head_ (same 64-byte line), causing unnecessary bus traffic.
    alignas(64) std::atomic<std::size_t> head_{0}; // written by consumer
    alignas(64) std::atomic<std::size_t> tail_{0}; // written by producer
    alignas(64) std::array<T, Capacity> data_{};

public:
    // Called by producer thread ONLY.
    // Returns false if queue is full (non-blocking).
    bool push(const T& item) {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        // Acquire: see consumer's head_ store, so we know freed slots.
        const std::size_t head = head_.load(std::memory_order_acquire);

        if (tail - head >= Capacity)
            return false; // full

        data_[tail & MASK] = item; // plain write; sequenced before release below

        // Release: makes the data_ write visible to the consumer before it
        // observes the new tail value.
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    // Called by consumer thread ONLY.
    // Returns std::nullopt if queue is empty (non-blocking).
    std::optional<T> pop() {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        // Acquire: see producer's tail_ store, so we know new items.
        const std::size_t tail = tail_.load(std::memory_order_acquire);

        if (head == tail)
            return std::nullopt; // empty

        T item = data_[head & MASK]; // plain read; sequenced after acquire above

        // Release: makes the slot available to the producer.
        head_.store(head + 1, std::memory_order_release);
        return item;
    }

    std::size_t size() const {
        const std::size_t head = head_.load(std::memory_order_acquire);
        const std::size_t tail = tail_.load(std::memory_order_acquire);
        return tail - head;
    }

    bool empty() const { return size() == 0; }
};

// Usage:
// SPSCQueue<int, 1024> q;
// Producer: q.push(42);
// Consumer: auto v = q.pop(); // returns std::optional<int>
```

**The follow-up trap**

"Why doesn't `head_.load(relaxed)` in `push()` cause a race condition?"

It would be a race if both threads wrote `head_`. But in SPSC, only the consumer writes `head_`. The producer only reads it. A stale relaxed read means the producer thinks fewer slots are free than actually are — it may spuriously return `false` (queue appears full). This is a false negative, never a false positive: it will never overwrite valid data. Correctness is maintained; throughput may be slightly reduced if we see a stale head_. In practice the cache coherency protocol delivers the updated value quickly. Some implementations use a local cached copy to avoid even the relaxed load on every push.

**Key numbers / facts**

- Disruptor (LMAX): SPSC ring buffer, ~25 million messages/sec sustained at sub-microsecond latency
- False sharing cost: ~100–200 ns per operation (cache line bounces between cores at ~40–60 ns each)
- Power-of-2 mask vs modulo: bitwise AND is 1 cycle; integer divide/modulo is 20–90 cycles
- `std::memory_order_acquire` on x86: free at hardware level (x86 is TSO — all loads are acquire); compiler fence only
- `std::memory_order_release` on x86: free at hardware level; all stores are release; compiler fence only
- On ARM: both acquire and release emit explicit `DMB` (data memory barrier) instructions (~10–20 ns)

---
## Singleton — Thread-Safe Implementation

**One-line definition**

A singleton ensures a class has exactly one instance; thread-safety requires that concurrent first-calls cannot construct two instances simultaneously.

**What the interviewer actually asked**

"Implement a thread-safe singleton in C++." (DE Shaw)

**The shallow answer** (gets you rejected)

Using a global bool flag checked before construction — races on the check-then-act.

**The deep answer** (gets you through)

Three approaches, each with different tradeoffs:

**1. Meyers Singleton (preferred, C++11+):** Function-local static with lazy initialization. The C++11 standard (§6.7) guarantees that initialization of a static local variable is performed exactly once, even if multiple threads reach it simultaneously. The compiler emits the necessary synchronization (typically a guard variable with double-checked locking internally). Zero boilerplate, correct, no overhead after first call.

**2. Double-checked locking with `std::atomic`:** Pre-C++11, double-checked locking was broken on weak memory models (compiler/CPU could reorder the pointer store before the object was fully constructed, another thread would see a non-null pointer and use a partially-constructed object). Fixed in C++11 by using `std::atomic<T*>` with `acquire` load and `release` store. The release store ensures all writes to the object happen-before the pointer is published.

**3. `std::call_once`:** Explicit version using `std::once_flag`. Equivalent in guarantee to Meyers singleton but allows separation of flag storage from function.

Pre-C++11 double-checked locking failure mode: `instance_` pointer write could be visible to another thread before the constructor body completes (write to `instance_` reordered before writes inside the constructor). Thread B sees non-null pointer and dereferences a zombie object.

**Implementation requirement?**

YES

```cpp
#include <atomic>
#include <mutex>

// ============================================================
// 1. Meyers Singleton — preferred, C++11 guaranteed thread-safe
// ============================================================
class MeyersSingleton {
public:
    static MeyersSingleton& getInstance() {
        static MeyersSingleton instance; // C++11: thread-safe, lazy, exactly once
        return instance;
    }

    MeyersSingleton(const MeyersSingleton&) = delete;
    MeyersSingleton& operator=(const MeyersSingleton&) = delete;

private:
    MeyersSingleton() = default;
};


// ============================================================
// 2. Double-checked locking with std::atomic (C++11)
//    Useful when you need a pointer (e.g., heap-allocated singleton)
//    or when you need to understand the pattern for an interview.
// ============================================================
class DCLSingleton {
public:
    static DCLSingleton* getInstance() {
        // First check (no lock): acquire load sees the release store below
        // if instance was already created. After the acquire, if ptr != nullptr,
        // all writes done before the release store are visible to us.
        DCLSingleton* ptr = instance_.load(std::memory_order_acquire);
        if (ptr == nullptr) {
            std::lock_guard<std::mutex> lock(mutex_);
            // Second check (under lock): another thread may have initialized
            // between our first check and acquiring the mutex.
            ptr = instance_.load(std::memory_order_relaxed); // already under mutex
            if (ptr == nullptr) {
                ptr = new DCLSingleton();
                // Release store: all writes to *ptr (constructor body) happen
                // before this store is visible to any thread doing acquire load.
                instance_.store(ptr, std::memory_order_release);
            }
        }
        return ptr;
    }

    DCLSingleton(const DCLSingleton&) = delete;
    DCLSingleton& operator=(const DCLSingleton&) = delete;

private:
    DCLSingleton() = default;
    static std::atomic<DCLSingleton*> instance_;
    static std::mutex mutex_;
};

std::atomic<DCLSingleton*> DCLSingleton::instance_{nullptr};
std::mutex DCLSingleton::mutex_;


// ============================================================
// 3. std::call_once variant
// ============================================================
#include <memory>

class CallOnceSingleton {
public:
    static CallOnceSingleton& getInstance() {
        std::call_once(initFlag_, []() {
            instance_.reset(new CallOnceSingleton());
        });
        return *instance_;
    }

    CallOnceSingleton(const CallOnceSingleton&) = delete;
    CallOnceSingleton& operator=(const CallOnceSingleton&) = delete;

private:
    CallOnceSingleton() = default;
    static std::unique_ptr<CallOnceSingleton> instance_;
    static std::once_flag initFlag_;
};

std::unique_ptr<CallOnceSingleton> CallOnceSingleton::instance_;
std::once_flag CallOnceSingleton::initFlag_;
```

**The follow-up trap**

"Why was double-checked locking broken before C++11 even if you used a volatile pointer?"

`volatile` prevents the compiler from caching the pointer in a register (forces re-read from memory) but does NOT prevent CPU reordering. On x86 with TSO (Total Store Order) you may get away with it, but on ARM or PowerPC the CPU can reorder the pointer store before the constructor's internal writes. `volatile` has no relationship to the C++ memory model's happens-before relation. It was a false solution. The fix requires an atomic release store so the memory model guarantees all constructor writes are visible before the pointer.

**Key numbers / facts**

- Meyers singleton: overhead after first call = one taken branch (predicted, ~0 cycles)
- `std::call_once`: ~same as Meyers; internally uses `pthread_once` or equivalent
- `std::atomic<T*>` acquire load (x86): ~1 cycle (free; just compiler fence)
- Double-checked locking broken: Java pre-1.5 also had this bug (fixed in JMM JSR-133, 2004)

---
## Memory Ordering and Atomics

**One-line definition**

Memory ordering specifies which reorderings the compiler and CPU are permitted to perform around an atomic operation, controlling the visibility of writes across threads.

**What the interviewer actually asked**

Follows from thread-safe queue and spinlock questions; often asked as "explain the memory orderings in C++11" or "why can't you just use relaxed everywhere?"

**The shallow answer** (gets you rejected)

"Use `seq_cst` to be safe."

**The deep answer** (gets you through)

The C++ memory model defines six orderings, reducible to four categories:

**`memory_order_relaxed`**: Atomicity only — no ordering with respect to any other memory operation. The operation is atomic (no torn reads/writes) but may appear to execute out of order relative to other threads. Use for: independent counters, statistics, ref counts where only the final value matters (not the order of side effects).

**`memory_order_acquire`** (load only): All memory operations after this load (in program order) happen after the load. Forms the receiving end of an acquire-release pair. If thread A stores with release and thread B loads with acquire and sees A's value, then B sees all writes A did before its release store.

**`memory_order_release`** (store only): All memory operations before this store (in program order) happen before the store. Forms the publishing end of an acquire-release pair. Think "publish."

**`memory_order_acq_rel`** (RMW operations): Combines acquire and release — the operation is both an acquire load and a release store. Used for CAS (`compare_exchange`) in the middle of a chain, e.g., a lock-free stack's push.

**`memory_order_seq_cst`**: Total sequential consistency across all threads — every `seq_cst` operation appears in a single total order agreed upon by all threads. The strongest and the default. Prevents store-load reordering (the only reorder that `acquire`/`release` doesn't prevent). Cost: on x86, `seq_cst` stores require `MFENCE` or `LOCK XCHG` (~20–40 ns); on ARM, `DMB ISH` + `DMB ISH` (~20 ns).

**Hardware reality:**

x86 (TSO — Total Store Order): Loads are never reordered with other loads. Stores are never reordered with other stores. The only permitted reorder is store-load (a later load can be reordered before an earlier store). Consequence: acquire and release are free at hardware level on x86 — they only need a compiler fence (`asm volatile("" ::: "memory")`). `seq_cst` requires `MFENCE` (or `LOCK XCHG`).

ARM (weakly ordered): All four types of reordering are possible. `acquire` → `DMB ISHLD`, `release` → `DMB ISH`. More expensive but necessary.

**For HFT hot path:**
- Use `relaxed` for per-thread counters aggregated later.
- Use `acquire`/`release` pairs for producer-consumer (free on x86).
- Avoid `seq_cst` in the hot path — the `MFENCE` stalls the store buffer.
- Profile: on x86 the difference between `relaxed` and `acquire`/`release` is zero; difference vs `seq_cst` can be 20–40 ns per operation.

**Implementation requirement?**

NO (covered in SPSC queue above; see also `std::atomic_flag` section below)

**The follow-up trap**

"If acquire/release is free on x86, why not just use seq_cst everywhere on x86?"

`seq_cst` stores require draining the store buffer (`MFENCE`) to establish a total order of stores across all cores. This can be 20–40 ns and can stall subsequent loads waiting for the store buffer to flush. `acquire`/`release` pairs achieve the same happens-before semantics for producer-consumer without draining the store buffer. `seq_cst` only becomes necessary when you need a total order across three or more threads doing independent operations — the specific guarantee that A sees B's store before C's, etc. In two-thread producer-consumer, `acquire`/`release` is both sufficient and faster.

**Key numbers / facts**

- x86 `MFENCE` / `LOCK XCHG` (seq_cst store): 20–40 ns
- x86 `acquire` load / `release` store: ~0 ns hardware cost (compiler fence only)
- ARM `DMB ISH`: ~10–20 ns
- Store buffer depth (Intel): 56 entries (Skylake), 64 (Ice Lake)
- C++ standard: the six orderings were introduced in C++11 (N2427, Boehm/Adve)
- Store-load is the only reorder x86 permits; the other three (load-load, load-store, store-store) are forbidden by TSO

---
## std::atomic_flag and fetch_add

**One-line definition**

`atomic_flag` is the only type the C++ standard guarantees lock-free; `fetch_add` atomically adds and returns the previous value in a single instruction.

**What the interviewer actually asked**

Follows from spinlock and shared_ptr ref count questions; sometimes asked directly as "how is shared_ptr ref count implemented?"

**The shallow answer** (gets you rejected)

"It's just an atomic integer."

**The deep answer** (gets you through)

**`std::atomic_flag`:**
- The only C++ atomic guaranteed to be lock-free (all others are "usually" lock-free but the standard does not mandate it).
- Only two operations: `test_and_set(order)` — atomically sets to `true`, returns old value; `clear(order)` — sets to `false`.
- No `load` operation (until C++20 which added `test()`). Cannot read without modifying.
- Maps directly to `LOCK XCHG` (x86) or `STLXR`/`LDAXR` pair (ARM).
- Canonical spinlock: `while (flag.test_and_set(acquire)) { _mm_pause(); }` / `flag.clear(release)`.

**`fetch_add(n, order)`:**
- Atomically adds `n` to the atomic and returns the value before the add.
- On x86: `LOCK XADD` instruction — single instruction, no CAS loop needed.
- `fetch_add(1, relaxed)`: used in `shared_ptr` control block for the strong ref count increment. "Relaxed" is valid because we only need the final count to be correct — we don't need to order any other memory operations around the increment itself. The release is done separately on destruction.
- `fetch_add(-1, acq_rel)` on decrement: acquire to see all writes done by the previous owner before it decremented; release to publish our writes before giving up ownership. If result is 1 (we held the last ref), we can now safely destroy the object.
- Compare `fetch_add` to `compare_exchange`: `fetch_add` is a single instruction (when the hardware supports it); `compare_exchange` (CAS) is also one instruction on x86 (`LOCK CMPXCHG`) but requires reading the expected value first and may need a retry loop in algorithms where the expected value is computed from the old value.

**Implementation requirement?**

YES — thread-safe `shared_ptr`-style ref count using atomics:

```cpp
#include <atomic>
#include <cstdint>

// Minimal shared_ptr control block demonstrating atomic ref count semantics.
// This is what std::shared_ptr's control block does internally.
template<typename T>
class SharedPtrControlBlock {
    T* ptr_;
    // strong_count: number of shared_ptr instances owning this object.
    // weak_count: number of weak_ptr instances + 1 (the +1 for the strong refs
    //             so the control block stays alive until all weak_ptrs release).
    std::atomic<int32_t> strong_count_{1};
    std::atomic<int32_t> weak_count_{1};

public:
    explicit SharedPtrControlBlock(T* ptr) : ptr_(ptr) {}

    // Called when a shared_ptr is copied.
    void addRef() {
        // Relaxed: we only need the count to be correct; no data ordering needed
        // here. The thread already holds a reference so the object is alive.
        // The new count will be observed by other threads via their own
        // acquire/release operations on their copies.
        strong_count_.fetch_add(1, std::memory_order_relaxed);
    }

    // Called when a shared_ptr is destroyed.
    // Returns true if this was the last strong reference (caller should delete ptr_).
    bool release() {
        // acq_rel: release so our writes to *ptr_ are visible before we
        // decrement (publish "I'm done"). Acquire so we see all writes from
        // other threads that decremented before us (transitively see their
        // writes through the release-sequence).
        //
        // If fetch_sub returns 1, we were the last owner. The acquire here
        // guarantees we see all writes any previous owner made before their
        // release decrement. Safe to destroy the object.
        if (strong_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete ptr_;
            ptr_ = nullptr;
            releaseWeak(); // release the implicit weak ref held by strong refs
            return true;
        }
        return false;
    }

    void addWeakRef() {
        weak_count_.fetch_add(1, std::memory_order_relaxed);
    }

    void releaseWeak() {
        if (weak_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete this; // all weak_ptrs and strong_ptrs gone; free control block
        }
    }

    // For weak_ptr::lock() — attempt to resurrect a strong reference.
    bool tryAddRef() {
        int32_t count = strong_count_.load(std::memory_order_relaxed);
        while (count > 0) {
            // CAS: if count is still what we read, atomically increment.
            // weak: spurious failure allowed, we retry anyway.
            // On success: acq_rel (we now own a reference, need to see prior writes).
            // On failure: relaxed (we'll re-read and retry).
            if (strong_count_.compare_exchange_weak(
                    count, count + 1,
                    std::memory_order_acq_rel,
                    std::memory_order_relaxed))
                return true;
            // count is updated by compare_exchange_weak on failure
        }
        return false; // object already destroyed
    }

    int32_t useCount() const {
        return strong_count_.load(std::memory_order_acquire);
    }

    T* get() const { return ptr_; }
};
```

**The follow-up trap**

"Why is `relaxed` correct for `addRef()` but `acq_rel` required for the decrement in `release()`?"

In `addRef()`, the caller already holds a reference to the object (that's why it's able to copy the `shared_ptr` at all). The object cannot be destroyed while a reference exists, so no ordering of the object's data relative to the increment is needed — we just need the count to go up atomically. In `release()`, we need the acquire to see all writes made by any thread that previously held and released a reference (ensures we see the fully-constructed/modified object before we destroy it), and the release to ensure our own writes are visible to anyone who may observe the count reaching zero and attempt to access the object via a weak lock. The asymmetry is deliberate and correct.

**Key numbers / facts**

- `LOCK XADD` (fetch_add, x86): ~10–15 ns (uncontended), same cost as `LOCK XCHG`
- `LOCK CMPXCHG` (CAS, x86): ~10–15 ns (uncontended); may retry under contention
- `atomic_flag` guaranteed lock-free: only type where `is_lock_free()` must return true per standard
- `shared_ptr` control block: two counters (strong + weak) in libstdc++ and libc++; on separate cache line from stored pointer in some implementations
- C++20 added `atomic_flag::test()` (read without modify) and `wait()`/`notify_one()`/`notify_all()` on all atomics (efficient userspace futex-like blocking)
- `fetch_sub(1, acq_rel) == 1` pattern: canonical way to detect "last reference" in any ref-counted system
