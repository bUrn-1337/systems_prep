# Operating Systems — Detailed Revision Notes (with Code)

Scoped to the tested chapters (from the intern-guidance docs): **memory system, concurrency/synchronization, deadlock** are heavy; **scheduling** is light. Each topic has: concept → why → tradeoffs → common follow-ups → code where interviews ask you to *write* it.

---

# 1. Processes & Threads

## 1.1 What a process is
A **process** is a running program instance. Its state:
- **Address space** — code (text), initialized/uninitialized data (BSS), heap (grows up), stack (grows down), memory-mapped regions.
- **CPU context** — PC, SP, general registers, flags.
- **OS-maintained state** in the **PCB (Process Control Block)**: PID, state, register save area, page-table pointer, open-file table, scheduling info, parent/children, accounting.

## 1.2 Process states
`New → Ready → Running → Terminated`, with `Running → Blocked/Waiting → Ready` when it waits on I/O or an event. **Blocked ≠ Running**: a blocked process consumes no CPU. Transition Running→Ready happens on a timer interrupt (preemption).

## 1.3 Thread vs Process (asked constantly — Zepto, DE Shaw, Alphagrep, Uber)
A **thread** is an execution stream inside a process. Threads share the process's address space and open files but each has its **own stack, registers, PC, and thread-local storage**.

| Aspect | Process | Thread |
|---|---|---|
| Address space | Private | Shared within process |
| Communication | IPC (pipe, shared mem, socket) — costly | Shared memory — cheap, but needs locks |
| Context switch | Expensive: page-table swap, TLB flush | Cheaper: no address-space change |
| Fault isolation | Crash is contained | One thread can corrupt/crash all |
| Creation cost | High (`fork`) | Low |

**Follow-ups you should nail:**
- *Chrome tabs → processes or threads?* Modern Chrome = **process per site/tab** for isolation & security (a crashed renderer doesn't take down the browser). Older/simple designs might use threads.
- *Why multithreading? (Abdullah)* Parallelism on multicore, overlap I/O with compute, responsiveness (UI thread + worker), shared-memory efficiency vs multiprocess.
- *Concurrency vs parallelism:* concurrency = dealing with many tasks (interleaving, even on 1 core); parallelism = literally running simultaneously on multiple cores.

## 1.4 Context switch (mechanics)
1. Trigger: timer interrupt, syscall, or blocking I/O.
2. CPU switches to kernel mode (trap), saves current registers into the PCB.
3. Scheduler picks next process.
4. If it's a **different process**: load its page-table base (e.g., x86 `CR3`) → this may **flush the TLB** (unless ASID/PCID tagging is used).
5. Restore next process's registers, return to user mode.

Cost sources: register save/restore, cache & TLB cold-start on the new working set (the *indirect* cost usually dominates).

## 1.5 CPU-bound vs I/O-bound (ARNAV @ DE Shaw)
- **CPU-bound:** long compute bursts, few I/O waits. Wants throughput.
- **I/O-bound:** short bursts, frequently blocks. Wants low latency; schedulers boost these for responsiveness.

## 1.6 Process API & key syscalls (QuantBox asked mmap/brk/sbrk)
- `fork()` — clone; returns **0 in child, child-PID in parent**. Uses **copy-on-write** so pages aren't physically copied until written.
- `exec()` — replace the current image with a new program (keeps PID).
- `wait()/waitpid()` — parent reaps child, gets exit status; prevents **zombies** (child that exited but not yet reaped). An **orphan** (parent died first) gets re-parented to init.
- `brk`/`sbrk` — move the heap "break"; classic heap growth.
- `mmap` — map a file or anonymous memory into the address space. Modern `malloc` uses `mmap` for large allocations and `brk`/arena for small ones.

---

# 2. CPU Scheduling  *(light — know vocabulary, don't over-study)*

## 2.1 Metrics
- **Turnaround** = completion − arrival.
- **Response** = first-run − arrival (interactivity).
- **Waiting** = time in ready queue.
- **Throughput**, **fairness**, **CPU utilization**.

## 2.2 Algorithms
| Algo | Idea | Pros | Cons |
|---|---|---|---|
| FCFS/FIFO | Run in arrival order | Simple | **Convoy effect** (short jobs behind long) |
| SJF | Shortest job first | Optimal avg turnaround | Needs length; starves long jobs |
| STCF | Preemptive SJF | Great turnaround | Needs remaining-time estimate |
| RR | Time-slice, cycle | Great response, fair | Slice too small → switch overhead |
| MLFQ | Priority queues, demote CPU hogs, periodic **boost** | Approximates SJF w/o knowing lengths; balances turnaround+response | Tuning-sensitive; gameable |
| Priority | Highest priority first | Flexible | Starvation → fix with **aging** |

**Scheduler types:** long-term (admission control), medium-term (swapping in/out), short-term (dispatch to CPU).

---

# 3. Concurrency & Synchronization

## 3.1 Race conditions & critical sections
A **race condition** = result depends on thread interleaving over shared mutable state (e.g., `count++` is read-modify-write, not atomic). The code touching shared state is the **critical section**; we need:
1. **Mutual exclusion** — at most one thread inside.
2. **Progress** — if free, someone can enter.
3. **Bounded waiting** — no starvation.

## 3.2 Hardware atomics
Locks are built on atomic instructions:
- **Test-and-Set (TAS)** — set to 1, return old.
- **Compare-and-Swap (CAS)** — `if (*p == expected) *p = desired` atomically; foundation of lock-free code.
- **Fetch-and-Add** — atomic increment (ticket locks, counters).

## 3.3 Mutex vs Spinlock (Maulik, Alphagrep)
- **Spinlock**: busy-waits (spins) until the lock frees. No context switch, so lowest latency **when the critical section is tiny and you're on a multiprocessor**. Wastes CPU if held long; useless on a uniprocessor (the holder can't run while you spin).
- **Mutex (blocking)**: sleeps the waiter (yields CPU). Better for longer sections; costs ~1–2 context switches.
- **Ownership:** a mutex is owned by the locking thread (only it unlocks); a semaphore is not.

### Spinlock — implement it (Maulik asked this)
```cpp
#include <atomic>
class Spinlock {
    std::atomic<bool> locked{false};
public:
    void lock() {
        bool expected = false;
        // acquire: spin until we flip false -> true
        while (!locked.compare_exchange_weak(
                   expected, true, std::memory_order_acquire)) {
            expected = false;          // CAS overwrites expected on failure
            // optional backoff to reduce cache-line contention:
            // __builtin_ia32_pause();  (x86 PAUSE)  or std::this_thread::yield();
        }
    }
    void unlock() { locked.store(false, std::memory_order_release); }
};
```
`std::atomic_flag` version is even simpler: `while (flag.test_and_set(std::memory_order_acquire));` / `flag.clear(std::memory_order_release);`.

## 3.4 Semaphores (semaphore vs mutex — Alphagrep, Abdullah)
An integer with two atomic ops:
- `wait()` / **P**: decrement; if < 0, block.
- `post()` / **V**: increment; wake a waiter.

Types: **binary** (0/1, mutex-like but no ownership) and **counting** (guards N identical resources). A semaphore is a **signaling** primitive — one thread can `post` what another `wait`s on (e.g., producer signals consumer).

## 3.5 Condition Variables & monitors
A **CV** lets a thread sleep until a predicate becomes true, always paired with a mutex:
```cpp
std::unique_lock<std::mutex> lk(m);
cv.wait(lk, [&]{ return predicate; });  // releases lock while waiting, re-acquires on wake
```
Always re-check in a **`while`** (or the lambda form) — guards against **spurious wakeups** and lost races. Use `notify_one` (one waiter) vs `notify_all`/`broadcast` (all).
A **monitor** = mutex + CVs bundled with the data (Java `synchronized`, C++ class with a private mutex).

## 3.6 Producer–Consumer (bounded buffer)
```cpp
#include <semaphore>   // C++20
#include <mutex>
#include <queue>

template <int CAP>
class BoundedBuffer {
    std::queue<int> q;
    std::counting_semaphore<CAP> empty{CAP}; // free slots
    std::counting_semaphore<CAP> full{0};    // filled slots
    std::mutex mtx;
public:
    void produce(int x) {
        empty.acquire();                     // wait for a free slot
        { std::lock_guard<std::mutex> lk(mtx); q.push(x); }
        full.release();                      // signal an item is available
    }
    int consume() {
        full.acquire();                      // wait for an item
        int x;
        { std::lock_guard<std::mutex> lk(mtx); x = q.front(); q.pop(); }
        empty.release();                     // signal a slot freed
        return x;
    }
};
```
Order matters: acquire the counting semaphore **before** the mutex to avoid deadlock.

## 3.7 Thread-safe queue with atomics/locks (Maulik)
```cpp
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

template <class T>
class ThreadSafeQueue {
    std::queue<T> q;
    mutable std::mutex m;
    std::condition_variable cv;
public:
    void push(T v) {
        {
            std::lock_guard<std::mutex> lk(m);
            q.push(std::move(v));
        }
        cv.notify_one();                     // notify outside lock (less contention)
    }
    T wait_pop() {                           // blocks until non-empty
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [this]{ return !q.empty(); });
        T v = std::move(q.front());
        q.pop();
        return v;
    }
    std::optional<T> try_pop() {             // non-blocking
        std::lock_guard<std::mutex> lk(m);
        if (q.empty()) return std::nullopt;
        T v = std::move(q.front());
        q.pop();
        return v;
    }
};
```
**Locking granularity:** coarse (one lock for whole structure) → simple, contended. Fine-grained (per-node locks) or **lock-free** (CAS loops) → scalable but hard; watch the **ABA problem** (use tagged pointers / hazard pointers).

---

# 4. Deadlock

## 4.1 Definition & Coffman conditions
Deadlock = each thread in a set waits on a resource held by another; none progresses. Needs **all four**:
1. **Mutual exclusion** — resource is non-shareable.
2. **Hold and wait** — hold one, request another.
3. **No preemption** — can't forcibly reclaim.
4. **Circular wait** — a cycle in the wait-for graph.

## 4.2 Strategies
| Strategy | How | Note |
|---|---|---|
| **Prevention** | Negate one condition | Easiest in practice: **global lock ordering** (kills circular wait); or acquire all locks at once (kills hold-and-wait) |
| **Avoidance** | Only grant if state stays **safe** → Banker's | Needs max-demand known in advance |
| **Detection + recovery** | Let it happen; detect cycle in RAG; kill/rollback/preempt | Used in DBs |
| **Ostrich** | Ignore | Linux/most OSes for app locks |

**Livelock:** threads keep reacting to each other, changing state but never progressing (vs stuck-still deadlock). **Starvation:** a thread never gets a resource (fix with aging).

## 4.3 Banker's Algorithm (Sarvasva, Jay @ DE Shaw — OSTEP omits it)
Matrices: `Available[m]`, `Max[n][m]`, `Allocation[n][m]`, `Need = Max − Allocation`. A state is **safe** if there's an ordering where every process can finish.
```cpp
#include <vector>
using namespace std;

bool isSafe(vector<int> avail,
            const vector<vector<int>>& maxm,
            const vector<vector<int>>& alloc) {
    int n = alloc.size(), m = avail.size();
    vector<vector<int>> need(n, vector<int>(m));
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            need[i][j] = maxm[i][j] - alloc[i][j];

    vector<bool> done(n, false);
    vector<int> work = avail;
    int finished = 0;
    while (finished < n) {
        bool progressed = false;
        for (int i = 0; i < n; i++) {
            if (done[i]) continue;
            bool canRun = true;
            for (int j = 0; j < m; j++)
                if (need[i][j] > work[j]) { canRun = false; break; }
            if (canRun) {                      // assume it finishes, releases resources
                for (int j = 0; j < m; j++) work[j] += alloc[i][j];
                done[i] = true; progressed = true; finished++;
            }
        }
        if (!progressed) return false;         // no one can proceed -> unsafe
    }
    return true;                               // found a safe sequence
}
```
**Resource-request check:** tentatively grant, recompute, run `isSafe`; if unsafe, roll back and make the process wait.

## 4.4 Deadlock detection via RAG cycle detection (Swapnil @ DE Shaw — recursive + iterative)
With one instance per resource, a cycle in the wait-for graph = deadlock. Use DFS with **white/gray/black** coloring (gray = on the current DFS path; a gray→gray edge is a **back edge** = cycle).

Recursive:
```cpp
// color: 0=white(unseen), 1=gray(on stack), 2=black(done)
bool dfs(int u, const vector<vector<int>>& adj, vector<int>& color) {
    color[u] = 1;
    for (int v : adj[u]) {
        if (color[v] == 1) return true;                 // back edge -> cycle
        if (color[v] == 0 && dfs(v, adj, color)) return true;
    }
    color[u] = 2;
    return false;
}
bool hasCycle(int n, const vector<vector<int>>& adj) {
    vector<int> color(n, 0);
    for (int i = 0; i < n; i++)
        if (color[i] == 0 && dfs(i, adj, color)) return true;
    return false;
}
```
Iterative (explicit stack — avoids recursion stack overflow on deep graphs):
```cpp
bool hasCycleIter(int n, const vector<vector<int>>& adj) {
    vector<int> color(n, 0);
    for (int s = 0; s < n; s++) {
        if (color[s] != 0) continue;
        vector<pair<int,int>> stk{{s, 0}};              // {node, next child index}
        color[s] = 1;
        while (!stk.empty()) {
            auto& [u, idx] = stk.back();
            if (idx < (int)adj[u].size()) {
                int v = adj[u][idx++];
                if (color[v] == 1) return true;         // cycle
                if (color[v] == 0) { color[v] = 1; stk.push_back({v, 0}); }
            } else {
                color[u] = 2; stk.pop_back();
            }
        }
    }
    return false;
}
```

---

# 5. Memory: Address Spaces & Translation

## 5.1 Why virtual memory
Isolation (a process can't touch another's memory), the illusion of a large contiguous private space, and controlled sharing. Every process sees the same virtual layout; the OS+MMU map it to different physical frames.

## 5.2 Base & Bounds → Segmentation → Paging
- **Base & Bounds:** `phys = base + virt`, trap if `virt ≥ bound`. Simple hardware, but the whole space must be **contiguous** → poor utilization.
- **Segmentation:** per-segment base/bounds (code, heap, stack). Grows segments independently; suffers **external fragmentation** (variable-size holes) → needs compaction.
- **Paging (core):** fixed-size **pages** (virtual) ↔ **frames** (physical), e.g., 4 KB. No external fragmentation; only **internal** (partial last page).

## 5.3 Paging mechanics
- Virtual address = **VPN | offset**; `offset bits = log2(page size)` (4 KB → 12 bits).
- **Page table** per process maps VPN → PFN. A **PTE** holds: frame number + **valid**, **present**, **protection (R/W/X)**, **dirty**, **accessed/reference**, **user/supervisor** bits.
- Physical address = `PFN * page_size + offset`.

**Worked example:** 32-bit VA, 4 KB pages → 12 offset bits, 20 VPN bits → 2^20 ≈ 1M PTEs. At 4 B/PTE that's a **4 MB page table per process** — too big to keep flat.

## 5.4 Multi-level page tables
Split the VPN into indices for a **page directory → page table** hierarchy. Only allocate second-level tables for regions actually used → sparse address spaces cost little. Cost: **more memory refs per translation** (a 4-level walk = 4 dependent loads on x86-64). This is exactly what the **TLB** hides.

## 5.5 TLB — high-value topic
- Hardware **cache of VPN→PFN translations** in the MMU.
- **Hit:** translate in ~1 cycle. **Miss:** walk the page table (HW-managed on x86, or SW-managed via trap), install the entry.
- Uses locality: a 4 KB page touched many times = 1 miss + many hits.
- **ASID/PCID** tags entries per address space → avoid full flush on context switch.
- **Coverage** = entries × page size. **Huge pages (2 MB / 1 GB)** raise coverage and cut misses (Maulik asked huge pages) — at the cost of coarser granularity and possible internal waste.

## 5.6 Full "address → data" path (Maulik asked end-to-end)
```
CPU issues virtual address
      │
      ▼
   TLB lookup ──hit──► PFN ──┐
      │ miss                  │
      ▼                       ▼
 page-table walk        form physical address
      │                       │
   present? ──no──► PAGE FAULT (OS loads page from disk/swap, updates PTE, retries)
      │ yes                   ▼
      └──────────────►   L1 → L2 → L3 cache lookup ──miss──► DRAM
```

---

# 6. Free-Space Management & Allocators
*(HFT-relevant: "new vs malloc, max depth" @ Quadeye; memory-pool @ Maulik; allocator resume projects.)*

## 6.1 Stack vs Heap
| | Stack | Heap |
|---|---|---|
| Management | Automatic (compiler) | Manual (`malloc/new` … `free/delete`) |
| Layout | LIFO frames per call | Arbitrary, allocator-managed |
| Speed | Very fast (bump SP) | Slower (search free list) |
| Size | Small, fixed limit | Large |
| Failure | **Stack overflow** (deep recursion) | Fragmentation, leaks, `bad_alloc` |
| Lifetime | Ends at function return | Until freed |

Deep recursive DFS → stack overflow; convert to **iterative + explicit stack / BFS** (recurring theme in your CP too).

## 6.2 `new` vs `malloc` (asked a lot)
| `new` | `malloc` |
|---|---|
| Operator (overloadable) | Library function |
| **Calls constructor** | Raw bytes, no construction |
| Returns **typed** pointer | Returns `void*` |
| Throws `std::bad_alloc` | Returns `NULL` on failure |
| Size deduced from type | Needs `sizeof` |
| Freed with `delete` (calls dtor) | Freed with `free` |

`new[]`/`delete[]` for arrays; mismatching them is UB. `new` typically calls `operator new` (which often calls `malloc`) then constructs.

## 6.3 Fragmentation & free-list strategies
- **External fragmentation:** free memory scattered in unusable small holes.
- **Internal fragmentation:** allocated block larger than requested (rounding/metadata).
- **Placement:** first-fit (fast), best-fit (tighter, slower, tiny leftovers), worst-fit, next-fit.
- **Coalescing:** merge adjacent free blocks on `free` (needs boundary tags / size headers).

## 6.4 Buddy & Slab
- **Buddy:** block sizes are powers of two; split on demand, merge with the "buddy" on free → fast coalescing, some internal waste. Used for kernel page allocation.
- **Slab:** per-type caches of pre-constructed fixed-size objects → no repeated init, low fragmentation for hot kernel structs.

## 6.5 Memory pool / arena (Maulik) — implement it
Hand out fixed-size slots from a pre-allocated chunk; free = push onto a free list. O(1) alloc/free, no per-object syscall — common in low-latency systems.
```cpp
#include <vector>
#include <cstddef>
#include <algorithm>

class MemoryPool {
    struct Block { Block* next; };            // free slots form an intrusive list
    Block* freeList = nullptr;
    std::vector<char*> chunks;
    size_t objSize, perChunk;
public:
    MemoryPool(size_t objSize_, size_t perChunk_ = 1024)
        : objSize(std::max(objSize_, sizeof(Block))), perChunk(perChunk_) {}

    void* allocate() {
        if (!freeList) refill();
        Block* b = freeList;
        freeList = freeList->next;
        return b;
    }
    void deallocate(void* p) {
        Block* b = static_cast<Block*>(p);
        b->next = freeList;
        freeList = b;
    }
    ~MemoryPool() { for (char* c : chunks) delete[] c; }
private:
    void refill() {
        char* chunk = new char[objSize * perChunk];
        chunks.push_back(chunk);
        for (size_t i = 0; i < perChunk; i++) {
            Block* b = reinterpret_cast<Block*>(chunk + i * objSize);
            b->next = freeList; freeList = b;
        }
    }
};
```

## 6.6 shared_ptr (Maulik, Alphagrep, QuantBox, Salesforce) — implement it
```cpp
#include <atomic>
template <class T>
class SharedPtr {
    T* ptr = nullptr;
    std::atomic<long>* cnt = nullptr;         // atomic -> thread-safe refcount
public:
    explicit SharedPtr(T* p = nullptr)
        : ptr(p), cnt(p ? new std::atomic<long>(1) : nullptr) {}

    SharedPtr(const SharedPtr& o) : ptr(o.ptr), cnt(o.cnt) {
        if (cnt) cnt->fetch_add(1, std::memory_order_relaxed);
    }
    SharedPtr(SharedPtr&& o) noexcept : ptr(o.ptr), cnt(o.cnt) {
        o.ptr = nullptr; o.cnt = nullptr;
    }
    SharedPtr& operator=(SharedPtr o) {       // copy-and-swap handles copy & move
        std::swap(ptr, o.ptr); std::swap(cnt, o.cnt); return *this;
    }
    ~SharedPtr() { release(); }

    T& operator*()  const { return *ptr; }
    T* operator->() const { return ptr; }
    long use_count() const { return cnt ? cnt->load() : 0; }
private:
    void release() {
        if (cnt && cnt->fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete ptr; delete cnt;
        }
    }
};
```
Talking points: the **control block** holds the refcount (real `std::shared_ptr` also has a weak count and deleter); atomic refcount makes copies thread-safe (the *pointee* is not); `weak_ptr` breaks **cyclic references** that would leak.

---

# 7. Virtual Memory: Demand Paging & Replacement

## 7.1 Demand paging
Load pages lazily on first touch. Access to a page with **present = 0** → **page fault** → OS: find a free frame (or evict one), read the page from disk/swap, update the PTE, **restart the faulting instruction**. Minor fault = page already in memory (just fix mapping); major fault = must hit disk.

## 7.2 Swap & the "lag" question (Abdullah @ DE Shaw)
**Swap space** = disk backing store for evicted pages. Many tabs open + low RAM → OS constantly evicts and reloads pages → disk I/O storm → system "lags." That's **thrashing** from over-committed memory.

## 7.3 Page replacement policies (Sprinklr, Jay)
| Policy | Rule | Note |
|---|---|---|
| **OPT (Belady)** | Evict page used farthest in future | Theoretical optimum; unimplementable |
| **FIFO** | Evict oldest loaded | Simple; **Belady's anomaly** (more frames → more faults) |
| **LRU** | Evict least-recently-used | Strong; exact version costly (per-access bookkeeping) |
| **Clock / Second-chance** | Circular scan using **reference bit**; give a second chance | Practical LRU approximation |
| **LFU** | Evict least-frequently-used | Needs counts; can retain stale-hot pages |

## 7.4 LRU cache (Microsoft, Sprinklr, DE Shaw) — O(1) implement it
Hash map (key → list iterator) + doubly linked list (front = most recent).
```cpp
#include <unordered_map>
#include <list>

class LRUCache {
    int cap;
    std::list<std::pair<int,int>> dll;                          // front = MRU {key,val}
    std::unordered_map<int, std::list<std::pair<int,int>>::iterator> mp;
public:
    LRUCache(int c) : cap(c) {}

    int get(int key) {
        auto it = mp.find(key);
        if (it == mp.end()) return -1;
        dll.splice(dll.begin(), dll, it->second);               // move to front, O(1)
        return it->second->second;
    }
    void put(int key, int val) {
        auto it = mp.find(key);
        if (it != mp.end()) {
            it->second->second = val;
            dll.splice(dll.begin(), dll, it->second);
            return;
        }
        if ((int)dll.size() == cap) {                           // evict LRU (back)
            mp.erase(dll.back().first);
            dll.pop_back();
        }
        dll.push_front({key, val});
        mp[key] = dll.begin();
    }
};
```
*Follow-up (Uber): why doubly linked?* Need O(1) removal of an arbitrary node given its iterator (singly-linked can't fix the predecessor's `next` in O(1)).

## 7.5 LFU cache (DE Shaw, Sprinklr follow-up) — O(1) implement it
Bucket keys by frequency; track `minFreq` for O(1) eviction.
```cpp
#include <unordered_map>
#include <list>

class LFUCache {
    int cap, minFreq = 0;
    std::unordered_map<int, std::pair<int,int>> kv;             // key -> {val, freq}
    std::unordered_map<int, std::list<int>::iterator> pos;      // key -> iterator in bucket
    std::unordered_map<int, std::list<int>> bucket;            // freq -> keys (front=MRU)
public:
    LFUCache(int c) : cap(c) {}

    int get(int key) {
        if (!kv.count(key)) return -1;
        touch(key);
        return kv[key].first;
    }
    void put(int key, int val) {
        if (cap == 0) return;
        if (kv.count(key)) { kv[key].first = val; touch(key); return; }
        if ((int)kv.size() == cap) {                            // evict LFU, LRU within freq
            int old = bucket[minFreq].back();
            bucket[minFreq].pop_back();
            kv.erase(old); pos.erase(old);
        }
        kv[key] = {val, 1};
        bucket[1].push_front(key);
        pos[key] = bucket[1].begin();
        minFreq = 1;
    }
private:
    void touch(int key) {                                       // bump frequency
        int f = kv[key].second;
        bucket[f].erase(pos[key]);
        if (bucket[f].empty()) { bucket.erase(f); if (minFreq == f) minFreq++; }
        kv[key].second = f + 1;
        bucket[f + 1].push_front(key);
        pos[key] = bucket[f + 1].begin();
    }
};
```

## 7.6 Copy-on-Write (COW)
After `fork`, parent & child share pages **read-only**; the first write faults → OS copies just that page and makes it writable. Makes `fork` cheap; also used by `mmap` private mappings.

## 7.7 Thrashing & working set
- **Thrashing:** active pages (working set) > physical frames → nonstop faulting, CPU stalls on disk, utilization collapses.
- **Working-set model:** keep each process's recently-used pages (window Δ) resident; suspend processes if their combined working sets don't fit.
- Fixes: add RAM, reduce multiprogramming degree, better locality.
- **"8 GB RAM, 50 GB data" (Jay):** don't load it all — **stream in chunks / external algorithms**, `mmap` the file and let demand paging fetch on access, use memory-mapped + sequential access to leverage prefetching.

---

## Quick interview flags
- **Deadlock (conditions + Banker's + code)** and **memory/paging/TLB** are the highest-frequency OS topics for your targets.
- Expect to **write code**, not just define: spinlock, threadsafe queue, LRU/LFU, shared_ptr, RAG cycle detection (recursive + iterative), memory pool.
- **Scheduling:** know the words, move on.
- When you name a term (e.g., "semaphore", "COW"), expect an immediate "implement it / prove it" follow-up — only say what you can defend (per the DE Shaw tip).
