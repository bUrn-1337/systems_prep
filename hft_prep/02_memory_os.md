```markdown
---
## Virtual Memory Address Translation (End-to-End)

**One-line definition**
The MMU converts a virtual address issued by the CPU into a physical DRAM address by walking a multi-level page table, with the TLB caching recent translations.

**What the interviewer actually asked**
"Walk me through what happens when a process accesses address 0x7fff1234. Start from the CPU instruction and end at the physical RAM byte." (DE Shaw, Graviton)

**The shallow answer**
"The CPU uses virtual memory, the OS translates it to physical memory using page tables."

**The deep answer**
1. CPU issues virtual address 0x7fff1234 during a load/store instruction.
2. MMU splits the address: on x86-64 with 4KB pages, bits [11:0] = page offset (0x234), bits [47:12] = virtual page number.
3. MMU checks TLB (Translation Lookaside Buffer): ~64–1024 entries, fully associative. TLB hit: extracts PFN directly, forms physical address = PFN × 4096 + offset. Cost: ~4 cycles. Done.
4. TLB miss: hardware page table walk begins. CR3 register holds physical base of PGD (Page Global Directory). On x86-64 4-level paging: bits [47:39] → PGD index → PUD → bits [38:30] → PUD index → PMD → bits [29:21] → PMD index → PTE → bits [20:12] → PTE index → PFN. Cost: ~40–100 cycles (4 memory accesses, possibly cached in L1/L2).
5. Each PTE checked: present bit (P=1 else page fault), writable bit (W), NX bit (no-execute), user/supervisor bit.
6. Present bit = 0: hardware raises #PF exception → IDT → do_page_fault() → find_vma() → if valid VMA: demand-page (allocate frame, zero or read from disk, update PTE, restart instruction); if invalid → SIGSEGV.
7. Physical address = PFN × 4096 + 0x234. MMU loads TLB entry, retries.
8. Physical address issued to cache hierarchy: L1 (4 cycles, 32KB, 64B lines) → L2 (12 cycles, 256KB) → L3 (40 cycles, 8–32MB) → DRAM (~300 cycles / 100ns).

**Implementation requirement?**
NO

**The follow-up trap**
"What happens to the TLB when you context-switch to another process?" — CR3 is reloaded with the new process's page table base, which flushes the entire TLB (unless PCID/ASID extensions are used). This is the dominant cost in process context switches. Thread switches within the same process do NOT reload CR3, so the TLB stays warm.

**Key numbers / facts**
- 48-bit virtual address space on x86-64 (57-bit with 5-level paging, LA57)
- 4KB page → 12-bit offset
- TLB: ~64–1024 entries, covers a tiny fraction of address space
- Page table walk: 4 memory references, ~40–100 cycles on miss
- TLB hit: ~4 cycles; DRAM miss: ~300 cycles
- PTE size: 8 bytes; each level table: 512 entries × 8B = 4KB (fits one page)

---
## Linux Syscalls: mmap vs sbrk vs brk

**One-line definition**
brk/sbrk extend the contiguous heap by moving the program break; mmap maps arbitrary anonymous or file-backed memory regions anywhere in the virtual address space.

**What the interviewer actually asked**
"How does malloc work at the OS level? What syscall does it use?" and "What is mmap? When does malloc use mmap vs sbrk?" (QuantBox)

**The shallow answer**
"malloc calls the OS to get memory."

**The deep answer**
brk/sbrk: the program break is the first address past the end of the heap. brk(addr) sets it to addr; sbrk(n) increments it by n bytes. The heap is one contiguous region growing upward from after BSS. Cheap for small allocations: one syscall extends the heap, glibc carves it up. Problem: cannot punch holes — you cannot return a block in the middle of the heap to the OS (the break only moves forward, rarely backward).

mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0): allocates a fresh region anywhere in the VA space. Key properties: independent regions can be munmap'd individually (returns physical pages to OS immediately), backed by nothing (demand-paged), can be MAP_SHARED for IPC, can be MAP_HUGETLB for huge pages, can be MAP_LOCKED to pin in RAM (no swap).

glibc ptmalloc2 policy: allocations < MMAP_THRESHOLD (default 128KB) served from sbrk-grown heap arenas; allocations >= 128KB get their own mmap region (can be returned to OS on free). Per-thread arenas reduce contention. MMAP_THRESHOLD is dynamic (mallopt M_TRIM_THRESHOLD).

For HFT: ptmalloc2 is mediocre. jemalloc and tcmalloc have size classes, per-thread caches, lower lock contention. Best: custom fixed-size pool with zero syscalls after init.

MAP_ANONYMOUS|MAP_HUGE (2MB): allocate huge pages via mmap. MAP_LOCKED: pages pinned in physical RAM, no page faults after first touch, no swap jitter. Combine: mmap(NULL, size, PROT_RW, MAP_ANON|MAP_PRIVATE|MAP_LOCKED|MAP_HUGETLB, -1, 0).

**Implementation requirement?**
NO

**The follow-up trap**
"What happens to the physical memory when you call free()?" — for small allocations (sbrk-backed), free() returns the chunk to glibc's free list; physical pages are NOT returned to the OS unless the heap shrinks past the break (brk trim). For mmap-backed allocations, free() calls munmap(), which immediately releases the physical pages. This asymmetry is why long-running processes can have large VSZ but smaller RSS after freeing small objects.

**Key numbers / facts**
- MMAP_THRESHOLD default: 128KB (glibc)
- sbrk/brk: 1 syscall per arena extension, not per malloc call
- mmap: 1 syscall per large allocation
- MAP_LOCKED requires CAP_IPC_LOCK or sufficient RLIMIT_MEMLOCK
- munmap returns pages to OS immediately; free() of small objects does not
- mmap minimum granularity: 1 page (4KB)

---
## Huge Pages

**One-line definition**
Huge pages are 2MB (or 1GB) memory pages that reduce TLB pressure by covering 512× more address space per TLB entry compared to standard 4KB pages.

**What the interviewer actually asked**
"What are huge pages? Why do they matter?" (DE Shaw)

**The shallow answer**
"They're bigger pages that are faster."

**The deep answer**
TLB is the bottleneck. With 4KB pages and ~1024 TLB entries, you cover 1024 × 4KB = 4MB of address space before TLB misses. A 1GB working set needs 262,144 TLB entries — impossible. Each miss triggers a full 4-level page table walk (~40–100 cycles).

With 2MB huge pages: 1024 TLB entries cover 2GB. A 1GB working set needs only 512 TLB entries — fits entirely. Result: near-zero TLB misses for hot data. For an HFT order book that fits in a few hundred MB, this eliminates a major latency source.

Two mechanisms:

1. Transparent Huge Pages (THP): kernel automatically promotes contiguous 4KB pages to 2MB. Enabled by default on Linux. Problem for HFT: khugepaged background thread causes unpredictable latency spikes during defrag/compaction. Disable with: echo never > /sys/kernel/mm/transparent_hugepage/enabled.

2. Explicit Huge Pages (hugetlbfs): pre-allocate huge pages at boot or via /proc. No runtime overhead, no latency spikes, guaranteed availability.

```bash
# Reserve 512 × 2MB = 1GB of huge pages
echo 512 > /proc/sys/vm/nr_hugepages

# Allocate via mmap
void* p = mmap(NULL, 2<<20, PROT_READ|PROT_WRITE,
               MAP_ANONYMOUS|MAP_PRIVATE|MAP_HUGETLB, -1, 0);
```

1GB pages (MAP_HUGE_1GB): even better for very large working sets — 1 TLB entry covers 1GB. Requires kernel support and NUMA awareness.

HFT usage: market data ring buffers, order book arrays, shared memory for IPC between trading engine and risk — all on explicit huge pages with MAP_LOCKED.

**Implementation requirement?**
NO

**The follow-up trap**
"Why not just always use huge pages?" — They require contiguous physical memory (harder to find under memory pressure). Wasted memory if region is sparse (a 2MB page for 1 byte wastes 2MB-1 physical frames). THP causes latency spikes. Explicit huge pages must be reserved at system startup before memory fragments. Also, huge pages interact poorly with NUMA: a 2MB page must reside on one NUMA node — cross-node access penalty.

**Key numbers / facts**
- 4KB page: 12-bit offset; 2MB page: 21-bit offset; 1GB page: 30-bit offset
- 2MB = 512 × 4KB → 512× TLB coverage improvement per entry
- TLB has ~64–1024 entries (L1 ITLB ~128, L1 DTLB ~64, L2 TLB ~1024, varies)
- TLB miss penalty: ~40–100 cycles (4-level walk)
- Disable THP: echo never > /sys/kernel/mm/transparent_hugepage/enabled
- MAP_HUGETLB flag for mmap; MAP_HUGE_2MB or MAP_HUGE_1GB for explicit size

---
## Memory Pool Implementation

**One-line definition**
A memory pool pre-allocates a large slab and serves fixed-size allocations from an intrusive free list, achieving O(1) alloc/free with zero syscall overhead.

**What the interviewer actually asked**
"Implement a memory pool." (DE Shaw)

**The shallow answer**
"Pre-allocate a big array and hand out chunks."

**The deep answer**
Key insight: use an intrusive free list — when a chunk is free, its first bytes store a pointer to the next free chunk. No separate metadata array needed. Allocation = pop from free list head (one pointer load + one pointer store). Free = push to free list head (two pointer stores). Both O(1), branch-free in fast path, zero syscalls after init.

For HFT: all order objects are the same size (e.g., 64 bytes). All message objects same size. Pool per object type. Pool allocated on huge pages with MAP_LOCKED. Thread-local pools eliminate all contention. Pool never shrinks during trading hours — no fragmentation, no GC pauses.

Variable-size needs: slab allocator (multiple pools, one per size class) or buddy allocator. jemalloc's size classes are essentially this.

**Implementation requirement?**
YES

```cpp
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <new>

class MemoryPool {
public:
    // chunkSize must be >= sizeof(void*); poolSize = total bytes to reserve
    MemoryPool(std::size_t chunkSize, std::size_t poolSize)
        : chunkSize_(chunkSize < sizeof(void*) ? sizeof(void*) : chunkSize)
        , poolSize_(poolSize)
    {
        // Single large allocation — in production: mmap + MAP_HUGETLB + MAP_LOCKED
        pool_ = static_cast<char*>(::operator new(poolSize_));
        freeHead_ = nullptr;

        // Build intrusive free list through the entire slab
        std::size_t count = poolSize_ / chunkSize_;
        for (std::size_t i = 0; i < count; ++i) {
            void* chunk = pool_ + i * chunkSize_;
            // Write pointer to next free chunk into the chunk itself
            *reinterpret_cast<void**>(chunk) = freeHead_;
            freeHead_ = chunk;
        }
    }

    ~MemoryPool() {
        ::operator delete(pool_);
    }

    // O(1), no syscall
    [[nodiscard]] void* allocate() noexcept {
        if (__builtin_expect(freeHead_ == nullptr, 0))
            return nullptr; // pool exhausted
        void* chunk = freeHead_;
        freeHead_ = *reinterpret_cast<void**>(chunk);
        return chunk;
    }

    // O(1), no syscall — caller must ensure ptr came from this pool
    void deallocate(void* ptr) noexcept {
        *reinterpret_cast<void**>(ptr) = freeHead_;
        freeHead_ = ptr;
    }

    // Disable copy
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

private:
    char*       pool_;
    void*       freeHead_;
    std::size_t chunkSize_;
    std::size_t poolSize_;
};

// Usage example
struct Order {
    uint64_t orderId;
    double   price;
    uint32_t qty;
    uint8_t  side;
    // pad to cache line in real code
};

// In practice: static or thread_local pool per object type
// MemoryPool orderPool(sizeof(Order), 1 << 20); // 1MB slab
// Order* o = static_cast<Order*>(orderPool.allocate());
// new (o) Order{...};          // placement new
// o->~Order();
// orderPool.deallocate(o);
```

**The follow-up trap**
"What happens if two threads call allocate() simultaneously?" — the naive implementation above has a data race on freeHead_. Solutions: (1) thread-local pool (zero overhead, preferred in HFT — one pool per core), (2) CAS on freeHead_ for lock-free LIFO stack (ABA problem — use tagged pointer or std::atomic with version counter), (3) mutex (too slow for HFT hot path). In HFT, thread-local pools pinned to cores are the standard answer.

**Key numbers / facts**
- Pool allocate: ~1–2 cycles (load + store), no syscall
- malloc (glibc, hot path): ~50–100ns
- malloc (cold, OS call): ~1–10μs
- Pool free: ~1–2 cycles
- Intrusive free list overhead: zero extra memory beyond the pool slab itself
- In-cache pool alloc approaches register-rename rename speed (~1 cycle)

---
## Process Binary Structure: ELF Sections

**One-line definition**
A Linux process's virtual address space is divided into well-defined segments — text, data, BSS, heap, mmap region, stack — derived from the ELF binary layout at load time.

**What the interviewer actually asked**
"Describe the memory layout of a process. What's in each section?" (QuantBox, DE Shaw)

**The shallow answer**
"There's a stack and a heap, and the code lives somewhere."

**The deep answer**
From low to high virtual address (typical x86-64 layout):

TEXT (0x400000): machine code (compiled instructions). Read-only + executable (PROT_READ|PROT_EXEC). Shared between all processes running the same binary — kernel maps it copy-on-write; only one physical copy needed. Loaded directly from ELF .text section.

DATA: initialized global and static variables with non-zero values (e.g., int x = 5;). Read-write (PROT_READ|PROT_WRITE). Has physical storage in the ELF file (the values must be persisted on disk).

BSS (Block Started by Symbol): uninitialized or zero-initialized global/static variables (e.g., int arr[1000];). No physical storage in ELF binary — only a size field. Kernel allocates anonymous zero pages on load. Trick: a large global array costs zero bytes in the binary file, only virtual address space + physical memory when touched.

HEAP: starts immediately after BSS, grows upward via brk/sbrk. Managed by malloc. Grows on demand (demand paging): kernel only allocates physical pages on first write.

MMAP REGION: between heap and stack. Shared libraries (.so files) mapped here, anonymous mmap regions, huge page allocations, shared memory segments. Dynamic linker ld.so maps .so ELFs here at startup (or lazily).

STACK: grows downward from near top of user space (0x7fff... region). Per-thread. Default 8MB (ulimit -s). Function call frame: saved registers, local variables, return address. Guard page (one 4KB page, PROT_NONE) below bottom of stack — triggers SIGSEGV on overflow.

KERNEL SPACE: above 0x7fffffffffff (canonical address boundary on 48-bit x86-64). Inaccessible; any access → immediate SIGSEGV. Kernel code, kernel stacks, kernel data live here (not accessible to user space).

ELF file structure: ELF header (magic bytes \x7fELF, arch, entry point) → Program Headers (tells kernel/dynamic linker how to map segments into memory: LOAD, DYNAMIC, INTERP) → Section Headers (for linker: .text, .data, .bss, .rodata, .plt, .got, .symtab, .strtab, .rela.text). Program headers = runtime view; section headers = link-time view.

**Implementation requirement?**
NO

**The follow-up trap**
"What's the difference between .data and .rodata?" — .rodata (read-only data) holds string literals and const globals; mapped PROT_READ only (no PROT_WRITE), so any write attempt → SIGSEGV. .data holds non-const initialized globals, mapped PROT_READ|PROT_WRITE. Follow-up: "Where does a C++ vtable live?" — .rodata (const, generated by compiler, read-only).

**Key numbers / facts**
- ELF magic: 0x7f 'E' 'L' 'F'
- TEXT typically starts at 0x400000 (non-PIE) or randomized (PIE/ASLR)
- BSS: zero physical bytes in binary, full physical allocation at runtime
- Stack default: 8MB (RLIMIT_STACK); max addressable per thread
- ASLR (Address Space Layout Randomization): randomizes base of stack, mmap, heap at load time — security feature, slight overhead
- /proc/PID/maps shows every VMA with address ranges, permissions, and backing

---
## Stack vs Heap: Allocation Mechanics and Failure Modes

**One-line definition**
Stack allocation is a single SP register decrement (~1 cycle, no OS involvement); heap allocation may invoke the kernel and can take microseconds.

**What the interviewer actually asked**
"When does stack allocation fail? What's the difference in performance?" (DE Shaw, Salesforce)

**The shallow answer**
"Stack is faster, heap is bigger."

**The deep answer**
Stack allocation mechanics: at function entry, the compiler emits sub rsp, N (N = total bytes of locals + saved registers). This is a single instruction executing in 1 cycle — no OS call, no locking, no metadata. Deallocation: add rsp, N (or ret, which restores saved SP) at function exit. Stack memory is always in L1/L2 cache because it was recently used. Stack is a single contiguous LIFO structure — no fragmentation possible.

Stack failure mode: the stack has a guard page (one PROT_NONE page) at the bottom. When SP decrements past the bottom of the mapped stack (e.g., deep recursion or a massive local array), the next access hits the guard page → CPU raises #PF → kernel's do_page_fault() → check VMA → no VMA (guard page is not a valid VMA) → send SIGSEGV. The kernel does NOT automatically extend the stack past the guard page (unlike some OSes). Result: process crashes. Default limit 8MB.

Heap allocation mechanics: malloc() first checks its free lists (per-thread arena). If a suitable free chunk exists: O(1) list removal, ~50–100ns. If not: may call sbrk() or mmap() → kernel context switch, TLB/cache disruption, potentially page fault on first use. Free chunk coalescing and bin management add overhead. Heap can be gigabytes; limited by virtual address space and physical RAM + swap.

Heap failure mode: sbrk/mmap returns MAP_FAILED → malloc returns nullptr; operator new throws std::bad_alloc. Can be caught and handled (unlike stack overflow which is a signal). Also: OOM killer (Linux) may kill the process entirely if system memory is exhausted (regardless of whether you check return values).

Performance comparison:
- Stack alloc: ~1 cycle / ~0.3ns
- Heap alloc (cached free chunk): ~50–100ns
- Heap alloc (new sbrk/mmap): ~1–10μs
- Stack dealloc: ~1 cycle
- Heap dealloc (free): ~50–100ns (+ possible brk trim)

**Implementation requirement?**
NO

**The follow-up trap**
"Can you increase the stack size?" — Yes: ulimit -s unlimited (per process) or pthread_attr_setstacksize() (per thread). But unlimited is dangerous (runaway recursion eats RAM silently). Alternative: convert deep recursion to explicit heap-allocated stack (iteration). Also: signal handlers need their own stack (sigaltstack()) because the regular stack may be exhausted when SIGSEGV fires.

**Key numbers / facts**
- Stack alloc: 1 instruction (sub rsp, N), ~1 cycle
- Heap alloc (fast path): ~50–100ns
- Heap alloc (syscall path): ~1–10μs
- Default stack size: 8MB (Linux)
- Guard page: 4KB PROT_NONE page below stack bottom
- Stack overflow signal: SIGSEGV (not a C++ exception, cannot be caught by try/catch)
- new/delete overhead vs malloc/free: minimal (new calls malloc internally in most implementations)

---
## Segfault and Stack Overflow at the OS Level

**One-line definition**
Both segfault and stack overflow are hardware page faults that propagate through the kernel's fault handler to deliver SIGSEGV to the offending process.

**What the interviewer actually asked**
"What happens at the OS level when you have a stack overflow or segfault?" (Salesforce, QuadEye)

**The shallow answer**
"The program crashes with a segmentation fault."

**The deep answer**
At the hardware level: any invalid memory access causes the CPU to raise exception vector 14 (#PF, Page Fault). CR2 register is loaded with the faulting virtual address. CPU pushes error code (present/write/user bits) and saves RIP onto the kernel stack, then jumps to the IDT entry for #PF.

Kernel path (Linux): arch/x86/mm/fault.c: exc_page_fault() → handle_page_fault() → do_user_addr_fault(). Key steps:
1. Read faulting address from CR2.
2. Call find_vma(mm, address): search the process's mm_struct for a VMA covering the address.
3. If VMA found and permissions match fault type (write to writable VMA, read to readable VMA): legitimate fault → handle_mm_fault() → demand page or copy-on-write → fix PTE → return (process continues).
4. If no VMA found, or VMA found but permissions wrong (write to read-only, exec on NX page): bad_area() → force_sig_fault(SIGSEGV, SEGV_MAPERR or SEGV_ACCERR, address).
5. SIGSEGV delivered to process → default handler: print "Segmentation fault (core dumped)", write core file (if enabled), call _exit(139).

Stack overflow path: SP decrements below the lowest mapped stack page → access hits guard page (PROT_NONE VMA or no VMA below stack) → #PF → find_vma() finds no valid VMA (or finds the guard page VMA with no permissions) → SIGSEGV. The kernel does NOT extend the stack (unlike Windows which has stack growth on guard page hit). The guard page exists precisely to make overflow detectable rather than silently corrupting adjacent memory.

COW fault (legitimate): fork() marks all pages copy-on-write (PROT_READ in both parent and child). First write → #PF → kernel allocates new physical page, copies content, updates PTE → process continues. This is a valid/handled page fault, not a crash.

**Implementation requirement?**
NO

**The follow-up trap**
"Can you catch SIGSEGV and recover?" — Technically yes: install a signal handler with sigaction(SIGSEGV, ...). Can use sigaltstack() so the handler runs on a separate stack (necessary if stack overflowed). But recovery is dangerous: the process state is undefined after a segfault (likely a bug). Legitimate use: JIT compilers use SIGSEGV handlers to implement bounds checks lazily; garbage collectors use it for write barriers. For production HFT: catch SIGSEGV, log a backtrace (async-signal-safe: write() + backtrace_symbols_fd()), then abort() — never continue execution.

**Key numbers / facts**
- CPU exception vector for page fault: #14 (#PF)
- CR2: holds faulting virtual address during #PF handler
- Error code bits: P (present), W/R (write), U/S (user/supervisor), I/D (instruction fetch)
- SIGSEGV si_code values: SEGV_MAPERR (no mapping), SEGV_ACCERR (wrong permissions)
- Default SIGSEGV action: core dump + exit with signal 11 (exit code 139)
- Guard page: typically 1 × 4KB page (PROT_NONE) at bottom of each thread stack
- sigaltstack() required for SIGSEGV handler when stack may be exhausted

---
## Page Replacement Algorithms: LRU and LFU

**One-line definition**
Page replacement algorithms decide which physical page to evict to disk when RAM is full; LRU evicts the least recently used page, LFU evicts the least frequently used.

**What the interviewer actually asked**
"How does the OS decide which page to evict?" (background for virtual memory questions); LRU/LFU cache are also asked as standalone DS/algo questions.

**The shallow answer**
"LRU removes the oldest page."

**The deep answer**
LRU (Least Recently Used): evict the page not used for the longest time. Optimal under temporal locality. Exact implementation requires a timestamp per page — prohibitive at OS scale (millions of pages). Linux uses the Clock Algorithm (second-chance): accessed bit (A bit) in each PTE is set by hardware on any access. Kernel's page reclaim (kswapd): sweeps pages in circular order; if A=1, clear it and move on (give second chance); if A=0 on second sweep → candidate for eviction. Two-list variant: active list (recently accessed) + inactive list (candidate for eviction). Page promotion/demotion between lists based on A bits.

LFU (Least Frequently Used): evict the page accessed the fewest times. Requires per-page counters. Problem: pages popular in the distant past keep high counts and resist eviction (frequency doesn't decay). Fix: aging — periodically right-shift all counters. Better for workloads with stable hot sets. Not used in Linux kernel but relevant for cache design questions.

OPT / Bélády's Algorithm: evict the page that will not be used for the longest time in the future. Optimal (minimum page faults) but requires future knowledge → only useful as theoretical benchmark.

As a data structures interview question, LRU and LFU caches are canonical problems. O(1) get/put is required.

**Implementation requirement?**
YES

```cpp
#include <unordered_map>
#include <list>
#include <utility>

// LRU Cache: O(1) get and put
// Doubly-linked list (most-recent at front) + hashmap to list iterators
class LRUCache {
public:
    explicit LRUCache(int capacity) : cap_(capacity) {}

    int get(int key) {
        auto it = map_.find(key);
        if (it == map_.end()) return -1;
        // Move accessed node to front
        list_.splice(list_.begin(), list_, it->second);
        return it->second->second;
    }

    void put(int key, int value) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second->second = value;
            list_.splice(list_.begin(), list_, it->second);
            return;
        }
        if (static_cast<int>(list_.size()) == cap_) {
            // Evict LRU (back of list)
            map_.erase(list_.back().first);
            list_.pop_back();
        }
        list_.emplace_front(key, value);
        map_[key] = list_.begin();
    }

private:
    int cap_;
    std::list<std::pair<int,int>> list_; // {key, value}, front = MRU
    std::unordered_map<int, std::list<std::pair<int,int>>::iterator> map_;
};

// LFU Cache: O(1) get and put
// Three structures: key→value, key→freq, freq→list of keys (min-freq tracked)
class LFUCache {
public:
    explicit LFUCache(int capacity) : cap_(capacity), minFreq_(0) {}

    int get(int key) {
        auto it = keyVal_.find(key);
        if (it == keyVal_.end()) return -1;
        touch(key);
        return it->second;
    }

    void put(int key, int value) {
        if (cap_ <= 0) return;
        if (keyVal_.count(key)) {
            keyVal_[key] = value;
            touch(key);
            return;
        }
        if (static_cast<int>(keyVal_.size()) == cap_) {
            // Evict LFU (and LRU among ties — back of minFreq list)
            auto& lruList = freqList_[minFreq_];
            int evict = lruList.back();
            lruList.pop_back();
            if (lruList.empty()) freqList_.erase(minFreq_);
            keyFreq_.erase(evict);
            keyVal_.erase(evict);
            keyIter_.erase(evict);
        }
        keyVal_[key] = value;
        keyFreq_[key] = 1;
        freqList_[1].push_front(key);
        keyIter_[key] = freqList_[1].begin();
        minFreq_ = 1;
    }

private:
    void touch(int key) {
        int f = keyFreq_[key];
        keyFreq_[key] = f + 1;
        freqList_[f].erase(keyIter_[key]);
        if (freqList_[f].empty()) {
            freqList_.erase(f);
            if (minFreq_ == f) ++minFreq_;
        }
        freqList_[f+1].push_front(key);
        keyIter_[key] = freqList_[f+1].begin();
    }

    int cap_, minFreq_;
    std::unordered_map<int,int>  keyVal_;
    std::unordered_map<int,int>  keyFreq_;
    std::unordered_map<int, std::list<int>> freqList_; // freq → list of keys
    std::unordered_map<int, std::list<int>::iterator> keyIter_;
};
```

**The follow-up trap**
"What's Linux's actual page replacement policy?" — Not pure LRU. Linux kswapd uses a two-list clock algorithm (active/inactive lists) with accessed bits from PTEs. Huge pages complicate eviction (must evict all 512 sub-pages atomically or split the huge page first — called huge page splitting). Also: swap prefetching (readahead on swap-in). For HFT: answer is "we pin all pages with MAP_LOCKED/mlock() so the kernel never considers them for eviction."

**Key numbers / facts**
- Linux: two LRU lists (active + inactive), clock algorithm, accessed bit in PTE
- Exact LRU at OS scale: impractical (millions of pages, hardware sets A-bit but not timestamps)
- LRU cache (interview): doubly-linked list + hashmap, O(1) both ops
- LFU cache (interview): three hashmaps + per-frequency lists, O(1) both ops
- Page fault cost (soft, page in RAM): ~1μs; hard fault (read from disk): ~10ms
- mlock()/MAP_LOCKED: pins pages, bypasses page reclaim entirely

---
## Paging vs Segmentation

**One-line definition**
Segmentation divides memory into variable-size logical segments; paging divides it into fixed-size pages. Modern x86-64 Linux uses paging only — segmentation is vestigial.

**What the interviewer actually asked**
"What is the difference between paging and segmentation?" (DE Shaw context)

**The shallow answer**
"Paging is fixed size, segmentation is variable size."

**The deep answer**
Segmentation (conceptual / old x86): memory is divided into named logical segments — code segment, data segment, stack segment. Each segment has a base address and a limit (size). A logical address is (segment selector, offset). The segment descriptor (from GDT or LDT) holds base + limit + permissions. Physical address = base + offset. Variable sizes mean segments fit the logical structure of programs. Problem: external fragmentation — gaps between segments in physical memory that are too small to use. Also: complex protection (each segment has its own permissions). Used in 8086/80286/80386 real and protected modes.

Paging (modern x86-64): memory is divided into fixed-size pages (4KB). Physical address space divided into frames. Virtual address = (VPN, offset). Page table maps VPN → PFN. No external fragmentation (all pages same size, any free frame fits any page). Internal fragmentation: up to 4KB-1 bytes wasted per allocation. Protection is per-page (R/W/X/user/supervisor bits in PTE). Hardware page table walker (MMU) handles translation.

x86-64 Linux reality: segmentation still exists in hardware but is effectively disabled. All segment base registers (CS, DS, ES, SS) are set to base=0, limit=0xFFFFFFFF (flat model). Exceptions: FS and GS segment registers — FS is used by glibc for thread-local storage (TLS), GS is used by the kernel for per-CPU data. These use the MSR-based FSBASE/GSBASE mechanism (WRGSBASE/WRFSBASE instructions or syscall), not classical segmentation.

Combined segmentation+paging (Intel Protected Mode on 32-bit): logical address → segmentation → linear address → paging → physical address. Linux always set segments to flat (base=0), so linear address = logical address, effectively bypassing segmentation.

**Implementation requirement?**
NO

**The follow-up trap**
"What is FS used for in a typical x86-64 Linux process?" — FS base register points to the Thread Control Block (TCB) / TLS area for the current thread. glibc accesses errno, stack canary, thread-local variables via FS-relative addressing (e.g., mov rax, fs:[0x28] reads the stack canary). Each thread has a different FS base value (set by set_thread_area or arch_prctl(ARCH_SET_FS, ...)). This is the remaining practical use of segmentation on x86-64.

**Key numbers / facts**
- 4KB page: zero external fragmentation, up to 4095 bytes internal fragmentation
- Segmentation: zero internal fragmentation, external fragmentation problem
- x86-64 Linux: all general segments (CS/DS/SS/ES) have base=0 — flat model
- FS/GS: used for TLS (FS) and per-CPU kernel data (GS), not general segmentation
- Linux does NOT use the LDT (Local Descriptor Table) under normal operation
- External fragmentation: unusable gaps between variable-size allocations

---
## Process vs Thread: Every Dimension

**One-line definition**
Threads share the virtual address space of their process; they have independent stacks, register state, and scheduling context but no isolation from each other's memory.

**What the interviewer actually asked**
"What does a thread share with a process? What doesn't it share?" (nearly every company)

**The shallow answer**
"Threads share memory, processes don't."

**The deep answer**
Shared between all threads of a process:
- Virtual address space (identical page tables, same CR3)
- Code (.text), initialized data (.data), BSS, heap
- File descriptors (open files, sockets, pipes) — close() in one thread affects all
- Signal handlers (sigaction settings, but not signal mask)
- Process ID (getpid() returns same value)
- User/group IDs, working directory, umask
- Memory-mapped regions (mmap)

Each thread has its own (not shared):
- Stack (each thread has a separate stack region, typically 8MB)
- Register file: all GP registers (rax, rbx, ... r15), floating-point/SSE/AVX registers, RIP (instruction pointer), RSP (stack pointer), RFLAGS
- Thread ID (gettid() is unique per thread; getpid() is the same)
- errno (thread-local variable in glibc — critical: errno is NOT shared)
- Signal mask (pthread_sigmask per thread)
- Thread-local storage (TLS) — variables declared __thread or thread_local in C++11
- Scheduling state (runnable/blocked), priority, CPU affinity

Kernel implementation: both processes and threads are kernel task_struct objects. pthread_create() calls clone() syscall with flags: CLONE_VM (share address space) | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD. fork() calls clone() without CLONE_VM — kernel marks all page table entries copy-on-write, creates new mm_struct. First write to a COW page triggers #PF → kernel allocates new frame, copies page, updates PTE in child only.

Creation cost: fork() ~1ms (copies page table structures, even with COW); pthread_create() ~10μs (allocates stack, clones task_struct, minimal mm work).

For HFT: threads are the right model (shared address space means zero-copy data sharing, no IPC overhead). Key practices: pin threads to cores with pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset); use SCHED_FIFO to prevent preemption; use thread-local pools to eliminate lock contention.

**Implementation requirement?**
NO

**The follow-up trap**
"If a thread calls exit(), what happens to other threads?" — exit() terminates the entire process (all threads). To terminate only the calling thread: pthread_exit(). If the main thread calls pthread_exit() (rather than return from main()), other threads continue running — the process stays alive until all threads exit or someone calls exit(). This asymmetry is a common bug source.

**Key numbers / facts**
- fork() cost: ~1ms (page table copy + COW setup)
- pthread_create() cost: ~10μs
- Threads per process limit: /proc/sys/kernel/threads-max (typically 32768–unlimited)
- errno is thread-local in glibc (accessed via FS-relative TLS)
- clone() flags for thread: CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD|CLONE_SETTLS
- CLONE_VM: share mm_struct (page tables). Without it: copy mm_struct (fork)

---
## Context Switch Cost: What Is Saved and Restored

**One-line definition**
A context switch saves the full register state and (for process switches) flushes the TLB by reloading CR3, costing 1–100μs depending on whether address spaces change.

**What the interviewer actually asked**
"What happens during a context switch? Why is it expensive?" (Graviton)

**The shallow answer**
"The OS saves registers and loads the next process."

**The deep answer**
Voluntary switch path: running task calls a blocking syscall (read(), futex_wait(), etc.) or yields. Kernel's __schedule() is invoked.

What is saved (outgoing task's context in task_struct):
- All general-purpose registers: RAX, RBX, RCX, RDX, RSI, RDI, R8–R15, RBP, RSP, RIP (via kernel stack frame from syscall entry).
- RFLAGS.
- x87 FPU state, SSE registers (XMM0–XMM15 = 128-bit each), AVX registers (YMM0–YMM15 = 256-bit), AVX-512 registers (ZMM0–ZMM31 = 512-bit each) — saved lazily in Linux (FPU state saved only if next task also uses FPU, using CR0.TS trick). AVX-512: 32 × 64B = 2KB of register state. Saving 2KB on every switch is expensive.
- Segment registers FS/GS (for TLS).
- Debug registers (DR0–DR7) if in use.

What is restored: same set for the incoming task.

CR3 reload (process switch only): loading CR3 with a new physical address flushes the entire TLB (all entries invalidated). This forces all subsequent memory accesses to page-table-walk until TLB refills. This is the dominant cost of process context switches. Mitigation: PCID (Process-Context IDentifiers) — CR3 low bits encode ASID (address space ID), TLB entries tagged with ASID, no full flush needed. Linux uses PCID on supporting CPUs.

Thread switch (same process): no CR3 change → TLB stays intact → much cheaper. But: L1/L2 cache lines for the outgoing thread's working set are evicted as the incoming thread's working set fills them. Cache miss cost is indirect but real.

Cost breakdown:
- Thread → thread (same process): ~1–10μs (register save/restore + cache pollution)
- Process → process: ~10–100μs (above + TLB flush + cold TLB refill)
- Kernel entry/exit alone (syscall overhead): ~100–200ns

For HFT: eliminate context switches entirely. Busy-polling (spin on a flag in shared memory instead of blocking syscall), DPDK (kernel-bypass networking, user-space poll loop), CPU isolation (isolcpus kernel boot parameter removes a core from scheduler), SCHED_FIFO with priority 99 (never preempted by normal tasks). Zero context switches = zero scheduler jitter.

**Implementation requirement?**
NO

**The follow-up trap**
"What is the AVX-512 context switch penalty and how do you mitigate it?" — If any code on a core uses AVX-512 instructions (256- or 512-bit ZMM registers), the OS must save/restore 32 × 64B = 2KB of register state on every context switch for that core. Even if YOUR code doesn't use AVX-512, if another thread on the same core does, you pay the cost. Mitigation: (1) disable AVX-512 in BIOS for latency-critical nodes, (2) use CPU isolation so trading core is never scheduled alongside AVX-512 users, (3) avoid linking libraries that emit AVX-512 code (openssl, glibc on some builds). Linux saves FPU/SIMD state lazily (only when next task touches FPU), but even lazy save/restore adds overhead.

**Key numbers / facts**
- Thread context switch (same process): ~1–10μs
- Process context switch: ~10–100μs
- syscall round-trip (getpid): ~100–200ns
- CR3 reload → full TLB flush (without PCID)
- AVX-512 register state: 32 × 64B = 2KB
- L1 cache: 32KB, ~4-cycle hit. Context switch likely evicts hot lines.
- isolcpus=N: removes CPU N from scheduler entirely — only tasks explicitly pinned to it run there
- SCHED_FIFO: real-time policy, never preempted by SCHED_OTHER (normal) tasks

---
## CPU Pipeline

**One-line definition**
The CPU pipeline overlaps multiple instruction stages (fetch, decode, execute, memory, writeback) to achieve near-one-instruction-per-cycle throughput; hazards cause stalls that destroy this throughput.

**What the interviewer actually asked**
"Describe the CPU pipeline. What is a pipeline stall?" (Graviton)

**The shallow answer**
"The CPU fetches, decodes, and executes instructions in stages."

**The deep answer**
Classic 5-stage RISC pipeline: Fetch (IF) → Decode (ID) → Execute (EX) → Memory access (MEM) → Writeback (WB). At any given cycle, 5 different instructions are in flight simultaneously. Ideal throughput: 1 instruction per cycle (IPC = 1).

Modern out-of-order superscalar (Intel Sunny Cove, AMD Zen 4): 15–20+ pipeline stages, 4–6 instruction decode width, out-of-order execution with 100+ entry reorder buffer (ROB), register renaming eliminates false dependencies (WAR, WAW hazards), speculative execution (execute past branches before branch resolution), hardware branch predictor (~95–99% accuracy), multiple execution units (ALU, FPU, load/store) → IPC of 3–5 on real code.

Pipeline hazards (stalls):

1. Data hazard (RAW — Read After Write): instruction B needs the result of instruction A, but A hasn't finished EX yet. CPU stalls B's issue until A's result is available. Mitigation: operand forwarding (bypass — result forwarded from EX output directly to next EX input, saves WB→ID round trip). With forwarding: 1-cycle stall for load-use (memory latency can't be forwarded as fast). OoO execution: find independent instructions to fill the stall slots.

2. Structural hazard: two instructions need the same execution unit simultaneously (e.g., two FP divides, one unit). Unit is not pipelined. Mitigation: duplicate units (modern CPUs have multiple ALUs).

3. Control hazard (branch misprediction): CPU speculatively executes the predicted path. If prediction wrong: flush the entire pipeline of speculative instructions → ~15–20 cycle penalty. Branch predictor uses: branch history buffer (2-bit saturating counters), path history, neural/TAGE predictors in modern CPUs.

For HFT: branch misprediction = 15–20 cycles = ~5–7ns at 3GHz. Critical path has zero branch tolerance. Techniques: branchless code (conditional moves: cmov), __builtin_expect(cond, 1/0) hints to compiler for likely branch direction, sorted input to enable predictor to learn, lookup tables instead of if/else chains, template specialization to eliminate runtime branches.

Cache miss in execute: load instruction issues to L1 (4 cycles). If L1 miss → L2 (12 cycles). If L2 miss → L3 (40 cycles). If L3 miss → DRAM (~300 cycles). During these cycles, all dependent instructions stall. OoO execution continues with independent instructions but eventually runs out (ROB fills → frontend stalls). Prefetching (__builtin_prefetch) hides latency by issuing the load early.

**Implementation requirement?**
NO

**The follow-up trap**
"What is Spectre/Meltdown and how do they relate to the pipeline?" — Spectre exploits speculative execution: the CPU execulates past a branch speculatively (or across a function call boundary), making memory accesses that leave cache-timing side channels even after the speculative path is squashed. Meltdown exploits the window between a load from kernel memory and the permission check — the data briefly enters the pipeline and pollutes the cache. Mitigations (KPTI for Meltdown, retpolines + microcode for Spectre) have measurable performance cost: KPTI causes TLB flush on kernel/user transitions, ~5–30% overhead on syscall-heavy code. For HFT kernel-bypass architectures: DPDK/RDMA avoid syscalls entirely → KPTI overhead is minimized.

**Key numbers / facts**
- Classic 5-stage pipeline: IF, ID, EX, MEM, WB
- Modern pipeline depth: 15–20 stages (Intel); more stages = higher clock, higher misprediction penalty
- Branch misprediction penalty: ~15–20 cycles (~5–7ns at 3GHz)
- Branch predictor accuracy: ~95–99% on typical code
- Load-use stall (with forwarding): 1–4 cycles depending on hit level
- L1 hit: 4 cycles; L2: 12 cycles; L3: 40 cycles; DRAM: ~300 cycles
- ROB (Reorder Buffer) size: ~200–350 entries (Zen 4: 320, Golden Cove: 512)
- __builtin_expect: compiler hint, arranges likely path as fall-through (no branch instruction taken in predicted case)

---
## Process Scheduling Algorithms

**One-line definition**
Linux uses the Completely Fair Scheduler (CFS) with a red-black tree ordered by virtual runtime; HFT processes use SCHED_FIFO with CPU isolation to eliminate all scheduler-induced latency jitter.

**What the interviewer actually asked**
"How does the OS schedule processes? What algorithm does Linux use?" (Graviton)

**The shallow answer**
"Round robin with priorities."

**The deep answer**
Classical algorithms (theory):
- FCFS (First Come First Served): non-preemptive, convoy effect (long job blocks short jobs), poor interactive response.
- SJF (Shortest Job First): optimal average waiting time, requires knowing burst length in advance (impractical), starvation of long jobs.
- Round Robin: preemptive, time quantum Q (typically 10–100ms). Good interactive response, high context switch overhead if Q too small, low CPU utilization if Q too large.
- Priority Scheduling: static or dynamic priority. Starvation of low-priority tasks (fix: aging — increase priority over time).
- Multilevel Feedback Queue (MLFQ): multiple queues at different priority levels, new jobs start high-priority, demoted if they use full quantum (CPU-bound), promoted if they yield early (I/O-bound). Approximates SJF without advance knowledge.

Linux CFS (Completely Fair Scheduler): goal is to give each runnable task a "fair" share of CPU. Key mechanism:
- Each task has a vruntime (virtual runtime) that increases as it runs, weighted by nice value (priority). Lower nice = higher weight = slower vruntime increase = more CPU time.
- Runqueue: red-black tree (O(log n) insert/delete) ordered by vruntime. Leftmost node = task with minimum vruntime = next to run.
- On each schedule tick: increment current task's vruntime by (actual_time × weight_factor). If another task has lower vruntime by more than a threshold → preempt.
- New task: vruntime set to current minimum (prevents starvation of existing tasks by new arrivals).
- Nice range: -20 (highest priority, ~40% more CPU) to +19 (lowest), default 0.

Real-time scheduling classes (higher priority than CFS):
- SCHED_FIFO: no time quantum. Runs until it blocks or yields voluntarily, or is preempted only by higher-priority SCHED_FIFO task. Priority 1–99.
- SCHED_RR: like SCHED_FIFO but with a time quantum; round-robin among same-priority RT tasks.
- SCHED_DEADLINE: EDF-based, for tasks with hard deadlines (audio, video).

HFT configuration:
```bash
# Set SCHED_FIFO priority 99 for trading process PID
chrt -f -p 99 <PID>

# Pin to core 3 (no other threads on this core)
taskset -cp 3 <PID>

# At boot: isolate cores 2,3 from scheduler (kernel cmdline)
# isolcpus=2,3 nohz_full=2,3 rcu_nocbs=2,3

# Per-thread affinity in C++
cpu_set_t cpuset;
CPU_ZERO(&cpuset);
CPU_SET(3, &cpuset);
pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);

# Verify scheduling policy
chrt -p <PID>
```

With isolcpus: the kernel never schedules any other task on that core. Only tasks explicitly pinned there via taskset/affinity run. Combined with SCHED_FIFO priority 99: the trading thread is never preempted except by hardware interrupts (IRQs) — and those can be steered away from the core via /proc/irq/*/smp_affinity.

**Implementation requirement?**
NO

**The follow-up trap**
"What is priority inversion and how is it solved?" — Low-priority task L holds a mutex. High-priority task H blocks on the mutex. Medium-priority task M preempts L (because L is low priority and M is medium). H is blocked waiting for L, but L can't run because M runs instead. H effectively runs at L's priority. In HFT: prevents SCHED_FIFO thread from making progress. Solutions: (1) Priority inheritance — mutex holder temporarily inherits the priority of the highest-priority waiter (Linux PI futex: FUTEX_LOCK_PI). (2) Priority ceiling — mutex has a ceiling priority; any holder runs at ceiling. (3) Avoid locks entirely (lock-free data structures, single-writer designs). In practice, HFT hot paths use lock-free or wait-free structures (atomics, SPSC queues) to eliminate the problem entirely.

**Key numbers / facts**
- Linux CFS: O(log n) scheduling, red-black tree ordered by vruntime
- Nice value: -20 to +19; weight ratio between adjacent nice levels: ~1.25×
- SCHED_FIFO priority range: 1–99 (99 = highest user-space RT priority)
- Default time quantum (CFS): ~4–15ms (adaptive, sysctl kernel.sched_min_granularity_ns)
- chrt -f 99: sets SCHED_FIFO at max user priority
- isolcpus= : kernel boot param to remove CPUs from scheduler entirely
- /proc/irq/*/smp_affinity: steer hardware IRQs away from isolated cores
- Priority inversion fix: FUTEX_LOCK_PI (priority-inheriting futex)
```
