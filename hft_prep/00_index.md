# HFT Systems Interview Prep — Master Index

> Target firms: Graviton, QuadEye, QuantBox, DE Shaw, Alphagrep, Salesforce
> Covers: C++ Internals · Memory & OS · Concurrency · Data Structures · Implementations · Output Prediction · Chapter Notes

---

## Study Order (by cross-firm frequency)

| Priority | File | Topics | Firms |
|----------|------|---------|-------|
| 1 | [C++ Internals](01_cpp_internals.md) | vtable/vptr, virtual dtor, EBO, static, lvalue/rvalue, new vs malloc, shared_ptr, unique_ptr, vector, struct padding, C++11-20, templates, SSO, spinlock | All 6 firms |
| 2 | [Memory & OS](02_memory_os.md) | Virtual memory end-to-end, mmap/sbrk/brk, huge pages, ELF layout, paging vs segmentation, process vs thread, context switch, CPU pipeline, scheduling | DE Shaw, Graviton, QuantBox |
| 3 | [Concurrency](03_concurrency.md) | mutex vs semaphore, deadlock + RAG cycle detection, SPSC queue, singleton, memory ordering | DE Shaw, QuadEye, Alphagrep |
| 4 | [Data Structures](04_data_structures.md) | LRU O(1), LFU O(1), binary lifting (LCA), bipartite graph | Graviton + all firms |
| 5 | [Implementations](05_implementations.md) | shared_ptr, unique_ptr, spinlock, memory pool, SSO, SPSC queue, vector — all from scratch, every line defensible | DE Shaw, Alphagrep |
| 6 | [Output Prediction](06_output_prediction.md) | 10 snippets: vtable dispatch, UB, move semantics, padding, static init order | QuantBox, Graviton |
| 7 | [Chapter Notes](07_chapter_notes.md) | Priority table + per-chapter notes for all 169 learncpp.com files | Study reference |

---

## Quick-Access: Most-Asked Topics

| Topic | File | Asked By |
|-------|------|----------|
| Virtual functions + vtable internals | [01_cpp_internals.md](01_cpp_internals.md) | ALL 6 firms |
| shared_ptr from scratch | [05_implementations.md](05_implementations.md) | DE Shaw, Alphagrep, Salesforce |
| Full address translation (VA → PA) | [02_memory_os.md](02_memory_os.md) | DE Shaw, Graviton |
| new vs malloc (every dimension) | [01_cpp_internals.md](01_cpp_internals.md) | QuadEye |
| LRU cache O(1) | [04_data_structures.md](04_data_structures.md) | 5+ firms |
| Deadlock + RAG cycle detection | [03_concurrency.md](03_concurrency.md) | DE Shaw |
| mmap vs sbrk vs brk | [02_memory_os.md](02_memory_os.md) | QuantBox (asked directly) |
| Thread-safe queue with atomics | [03_concurrency.md](03_concurrency.md) + [05_implementations.md](05_implementations.md) | DE Shaw |
| Spinlock implementation | [05_implementations.md](05_implementations.md) | DE Shaw |
| C++ version features (which version = which feature) | [01_cpp_internals.md](01_cpp_internals.md) | DE Shaw |

---

## Implementation Checklist (all covered)

- [x] `shared_ptr` from scratch → [05_implementations.md §1](05_implementations.md)
- [x] `unique_ptr` from scratch → [05_implementations.md §2](05_implementations.md)
- [x] Spinlock → [05_implementations.md §3](05_implementations.md)
- [x] Memory pool → [05_implementations.md §4](05_implementations.md)
- [x] Small string optimization → [05_implementations.md §5](05_implementations.md)
- [x] Thread-safe SPSC queue → [05_implementations.md §6](05_implementations.md)
- [x] `vector::push_back` → [05_implementations.md §7](05_implementations.md)
- [x] LRU cache → [04_data_structures.md §1](04_data_structures.md)
- [x] LFU cache → [04_data_structures.md §2](04_data_structures.md)

---

## Firm-Specific Cheat Sheet

| Firm | Format | What they specifically tested |
|------|--------|-------------------------------|
| **Graviton** | Pen-paper + deep oral grilling | C++ code with vtable errors to *find*, virtual memory end-to-end, CPU pipeline, binary lifting |
| **QuantBox** | Timed sections (must be fast) | Large C++ snippets → predict output, Linux syscall names (mmap/sbrk/brk), BSS/stack/heap structure |
| **QuadEye** | Alternating systems + quant | Virtual memory deep-dive, profiling a running process, templates, new vs malloc (kept going deeper) |
| **DE Shaw** | Resume-driven, whiteboard implementations | shared_ptr/spinlock/memory pool — write it; vtable draw; full address translation; C++ version features |
| **Alphagrep** | C++ + DS implementation heavy | shared_ptr/unique_ptr/push_back — write it; struct padding; TLB calculation; mutex vs semaphore |
| **Salesforce** | Systems basics | shared_ptr internals, stack vs heap OS-level, segfault/stack overflow OS behavior |

---

## Topics to Skip (0 appearances in HFT systems rounds)
- DBMS
- Computer Networks (unless targeting non-HFT roles)
- React / frontend
- Generic sorting algorithms (bubble sort, selection sort, etc.)

> **Flag**: If these appear in your other notes, deprioritize them — they have not appeared in systems rounds for HFT firms.
