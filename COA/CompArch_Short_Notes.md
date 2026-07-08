# Computer Architecture — Detailed Revision Notes (with Examples & Code)

Scoped to Ch 3 (Assembly/ISA), 9 (Processor Design), 10 (Pipelining), 11 (Memory System), 12 (Multiprocessors), plus the extras the docs surface: **caches, write policies, TLB/AMAT timing, SIMD/systolic, false sharing**. **Ch 11 (memory system) is the most tested — study it deepest.** Here "code" = the worked timing calculations and C++ demos interviews actually use.

---

# 1. Assembly Language & ISA (Ch 3)

## 1.1 ISA = hardware/software contract
Defines: instruction set, register set, data types, addressing modes, memory model, exceptions. Microarchitecture = *how* a given ISA is implemented (pipeline depth, caches, OOO) — many microarchitectures per ISA.

## 1.2 RISC vs CISC
| RISC (ARM, RISC-V, MIPS) | CISC (x86) |
|---|---|
| Few, **fixed-length**, simple instrs | Many, **variable-length**, complex |
| **Load/store** arch — only loads/stores touch memory | Instructions can operate directly on memory |
| Many registers | Historically fewer |
| Easy to pipeline/decode | Decodes into internal **micro-ops** (RISC-like) |

Modern x86 chips are CISC ISA on a RISC-style core (µop decode + OOO). RISC-V is the clean modern teaching/industry RISC ISA.

## 1.3 Instruction classes & addressing modes
- **Classes:** arithmetic/logic (ADD, SUB, AND, SHL), data transfer (LOAD, STORE, MOV), control (BEQ, JMP, CALL, RET).
- **Addressing modes:** immediate, register, register-indirect, **base+displacement** (`ld r1, 8(r2)`), indexed (`base+index*scale`), PC-relative (branches).

## 1.4 Registers & the stack frame
Special regs: **PC/IP**, **SP** (stack pointer), **FP/BP** (frame/base pointer), status/flags.
Calling convention splits registers into **caller-saved** (scratch) and **callee-saved** (must preserve), plus argument/return registers.
**CALL** pushes return address; the callee sets up a **stack frame** (saved regs, locals, spilled args); **RET** tears it down. Stack grows toward lower addresses on x86/ARM.

*Interview relevance:* pipeline questions often ask you to trace a small assembly snippet through stages (Anjalika @ Salesforce traced RISC-V add/sub through 5 stages).

---

# 2. Processor Design (Ch 9)

## 2.1 The 5 datapath stages
1. **IF** — fetch instruction at PC; PC += 4.
2. **ID** — decode; read register file; sign-extend immediates.
3. **EX** — ALU computes result or an effective address; branch comparison.
4. **MEM** — data memory access (loads/stores only).
5. **WB** — write result back to the register file.

## 2.2 Datapath vs Control
- **Datapath:** PC, instruction & data memory, register file, ALU, sign-extender, muxes — the "roads."
- **Control unit:** decodes opcode → signals (`RegWrite`, `ALUSrc`, `ALUOp`, `MemRead/Write`, `MemToReg`, `Branch`, mux selects) — the "traffic lights."
- **Hardwired control** (combinational, fast, fixed) vs **microprogrammed** (a control ROM of micro-instructions, flexible — used for complex CISC).

## 2.3 Single-cycle limitation → motivation for pipelining
Clock period fixed by the **slowest instruction** (a load uses all 5 stages). Simple instructions waste the rest of the cycle. Pipelining reuses that idle hardware by overlapping instructions.

---

# 3. Pipelining (Ch 10)

## 3.1 Idea & throughput
Overlap instructions like an assembly line — one per stage. **Latency** per instruction stays ~5 stages, but **throughput** approaches **1 instruction/cycle** (CPI → 1). Ideal speedup ≈ number of stages; **pipeline registers** (IF/ID, ID/EX, EX/MEM, MEM/WB) carry state between stages.

## 3.2 Hazards
**(a) Structural** — two stages want the same unit simultaneously (e.g., one memory port for both fetch and load). Fix: split I-cache/D-cache, multi-port register file.

**(b) Data** — an instruction needs a not-yet-written result.
- **RAW** (read-after-write) — the real dependency.
- **WAR / WAW** — matter under out-of-order / register reuse; solved by **register renaming**.
- **Fixes:** **forwarding/bypassing** — route the EX/MEM result straight to a later instruction's ALU input instead of waiting for WB. **Load-use hazard:** a load's value isn't ready until end of MEM, so an instruction using it in the *next* cycle needs **one stall (bubble)** even with forwarding.

```
Load-use hazard (needs 1 bubble):
  lw  r1, 0(r2)   IF ID EX ME WB
  add r3, r1, r4     IF ID -- EX ME WB   <- stall 1 cycle, then forward from MEM
```

**(c) Control (branch)** — next PC unknown until the branch resolves.
- Fixes: **early resolution** (decide in ID → smaller penalty), **branch prediction** + flush on misprediction, **delayed branch** (older MIPS).
- **Misprediction penalty** = number of stages between fetch and where the branch resolves.

## 3.3 Branch prediction
- **Static:** always-taken / always-not-taken / **BTFN** (backward-taken, forward-not-taken — good for loops).
- **Dynamic:**
  - **1-bit:** remembers last outcome; mispredicts twice per loop (entry + exit).
  - **2-bit saturating counter:** states Strongly/Weakly Taken/Not-Taken; needs **two** consecutive misses to flip → tolerates the single loop-exit blip.
  - **BTB** (Branch Target Buffer) caches target addresses; **correlating/tournament** predictors combine histories.

```
2-bit saturating counter FSM (T=taken, N=not-taken):
  [Strong NT] --T--> [Weak NT] --T--> [Weak T] --T--> [Strong T]
      ^  |N              |N               |N              |  ^
      +--+  <----N-------+   <----N-------+   <---N-------+  | (T self-loop)
  Predict "not taken" in the two left states, "taken" in the two right states.
```

## 3.4 CPI & deep pipelines
`CPI_actual = 1 + stalls_per_instr`. Deeper pipelines → higher clock but larger misprediction/hazard penalties → diminishing returns. **Superscalar** issues multiple instructions/cycle; **out-of-order (OOO)** execution with **register renaming** and reorder buffers hides latency (modern cores).

---

# 4. The Memory System (Ch 11) — **study deepest**

## 4.1 Hierarchy & locality
Registers → L1 → L2 → L3 → DRAM → SSD/HDD. Faster = smaller & pricier per byte. Works because of **locality**:
- **Temporal** — reused soon (loop variables).
- **Spatial** — nearby addresses used soon (arrays) → fetch a whole **cache line** (e.g., 64 B).

## 4.2 Cache organization & address decode
- Data moves in **lines/blocks**.
- Address = **Tag | Index | Block-offset**:
  - `offset bits = log2(block size)`
  - `index bits  = log2(number of sets)`
  - `tag bits    = address bits − index − offset`
- **Direct-mapped** (1 line/set), **fully associative** (line anywhere), **N-way set-associative** (N lines/set — the practical choice).

**Worked example:** 32-bit addresses, 32 KB cache, 64 B lines, 4-way set-associative.
```
Lines total          = 32 KB / 64 B          = 512 lines
Sets                 = 512 / 4 (ways)         = 128 sets
offset bits          = log2(64)               = 6
index  bits          = log2(128)              = 7
tag    bits          = 32 - 7 - 6             = 19
```
So a physical address splits as `[ tag:19 | index:7 | offset:6 ]`.

## 4.3 The 3 C's (miss types)
- **Compulsory (cold):** first-ever access. Mitigate with larger blocks, prefetching.
- **Capacity:** working set exceeds cache size. Mitigate with a bigger cache.
- **Conflict:** too many blocks map to one set (low associativity). Mitigate with more ways.

## 4.4 Write policies — "hit-based / miss-based" (Sarvasva @ DE Shaw)
**On a write HIT:**
- **Write-through:** update cache *and* memory. Simple, always coherent, more bus traffic. Usually buffered by a **write buffer**.
- **Write-back:** update cache only, mark **dirty**; write to memory on **eviction**. Less traffic, needs dirty bit + writeback logic. (Default for most caches.)

**On a write MISS:**
- **Write-allocate:** load the line into cache, then write (natural pair with write-back).
- **No-write-allocate:** write straight to memory, don't cache (natural pair with write-through).

## 4.5 AMAT — memory-access timing (Alphagrep asked TLB/timing math)
```
AMAT = Hit time + Miss rate × Miss penalty
```
Multi-level (compute inside-out):
```
AMAT = HitT_L1 + MR_L1 × ( HitT_L2 + MR_L2 × MissPenalty_mem )
```
**Worked example:** L1 hit 1 cyc, L1 miss 5%; L2 hit 10 cyc, L2 miss 20%; DRAM 100 cyc.
```
AMAT = 1 + 0.05 × (10 + 0.20 × 100)
     = 1 + 0.05 × (10 + 20)
     = 1 + 0.05 × 30
     = 1 + 1.5  =  2.5 cycles
```

## 4.6 Cache-aware code: traversal order & blocking
Row-major arrays: iterate the **last index innermost** so consecutive accesses share cache lines.
```cpp
// BAD: column-major traversal of a row-major array -> a cache miss almost every step
for (int j = 0; j < N; ++j)
    for (int i = 0; i < N; ++i)
        sum += A[i][j];        // strides by N*sizeof, poor spatial locality

// GOOD: row-major traversal -> sequential, prefetcher-friendly
for (int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j)
        sum += A[i][j];
```
**Loop blocking/tiling** (e.g., matrix multiply) keeps a sub-block resident in cache to reuse it before eviction — turns capacity misses into hits. This is the practical payoff of understanding the 3 C's.

## 4.7 Cache + virtual memory interaction
- **TLB** (see OS notes) = cache of page-table entries; TLB miss → page walk → possible page fault.
- **VIPT** (virtually-indexed, physically-tagged) L1: index the cache with virtual bits **while** the TLB translates in parallel, then compare using the physical tag — hides translation latency.
- **Huge pages** raise TLB coverage → fewer TLB misses for large working sets (Maulik).

## 4.8 DRAM basics
Organized in banks/rows/columns with a **row buffer**. Access = activate row (RAS) + column read (CAS); an open-row hit is fast, a row conflict pays activate+precharge. Periodic **refresh**. Distinguish **latency** (time for one access) from **bandwidth** (bytes/sec) — they optimize differently.

---

# 5. Multiprocessor Systems (Ch 12)

## 5.1 Why parallel
Single-core frequency scaling hit the **power wall** (~mid-2000s). More performance now comes from more cores + wider SIMD, which pushes complexity onto software.

## 5.2 Flynn's taxonomy
- **SISD** (classic scalar), **SIMD** (one instruction, many data — vector units/GPUs), **MIMD** (multicore/multiprocess), MISD (rare).

## 5.3 Cache coherence — MESI
When multiple cores cache the same line, a write must not leave stale copies. Each cached line has a state:
- **M** odified — dirty, this core has the only valid copy.
- **E** xclusive — clean, only this core has it.
- **S** hared — clean, possibly in other caches.
- **I** nvalid — not valid here.

**Snooping** (cores watch a shared bus) or **directory** (scalable, tracks sharers) enforces it via **write-invalidate**: writing a line invalidates all other copies first. State transitions on local read/write and remote (bus) read/write.

## 5.4 False sharing (very HFT-relevant) — demo
Two cores write **different** variables that land on the **same 64 B line** → the line ping-pongs (invalidations) between caches, destroying performance even with zero logical sharing.
```cpp
#include <atomic>
#include <cstdint>

// BAD: two counters on the same cache line -> false sharing
struct BadCounters {
    std::atomic<uint64_t> a;
    std::atomic<uint64_t> b;          // likely same 64B line as 'a'
};

// GOOD: pad each to its own cache line
struct alignas(64) Padded { std::atomic<uint64_t> v; char pad[64 - sizeof(v)]; };
struct GoodCounters { Padded a; Padded b; };   // a and b on separate lines
```
Fix: `alignas(64)` / `std::hardware_destructive_interference_size`, or separate per-thread structures.

## 5.5 Memory consistency & fences
Coherence = *what value* a location holds; **consistency** = *the order* different cores observe memory ops. **Sequential consistency** is intuitive but slow; real CPUs (x86 = TSO, ARM = weak) reorder → correct concurrent code needs **memory barriers/fences** and properly-ordered atomics (`memory_order_acquire/release/seq_cst`). Ties directly to lock-free structures in the OS notes.

## 5.6 SIMD / systolic (KLA & HPC roles asked SIMD + systolic arrays)
- **SIMD:** one instruction on a vector register — SSE/AVX (x86), NEON/SVE (ARM). Great for data-parallel inner loops; compilers auto-vectorize, or use intrinsics.
```cpp
#include <immintrin.h>            // AVX
// c[i] = a[i] + b[i] for i in [0,8): 8 floats in one instruction
__m256 va = _mm256_loadu_ps(a);
__m256 vb = _mm256_loadu_ps(b);
__m256 vc = _mm256_add_ps(va, vb);
_mm256_storeu_ps(c, vc);
```
- **Systolic array:** a grid of processing elements (PEs) that rhythmically pump data through, each doing a multiply-accumulate — the heart of matrix-multiply / ML accelerators (e.g., Google TPU). Relevant if your resume mentions a RISC processor / HPC (ARNAV was grilled on SIMD + systolic arrays integrated with a RISC project).

## 5.7 Amdahl's Law
Speedup is capped by the serial fraction:
```
Speedup(N) = 1 / ( (1 − p) + p/N )        p = parallel fraction, N = processors
Max (N→∞)  = 1 / (1 − p)
```
**Example:** p = 0.90 → max speedup = 1/0.10 = **10×**, no matter how many cores. Small serial fractions dominate at scale (→ motivates Gustafson's view: scale the problem size too).

---

## Quick interview flags
- **Ch 11 (caches) is the top-tested comparch topic.** Memorize: tag/index/offset decode, the **3 C's**, **write policies (hit + miss)**, and **AMAT** math — practice the numeric examples until instant.
- **TLB + cache + paging** connect straight to the OS memory notes; expect the full **address → data** walk-through.
- **Pipelining:** hazard types + forwarding + branch prediction is the usual depth; be ready to trace a snippet.
- **Multiprocessors:** **MESI, false sharing, Amdahl** are the crowd-pleasers; SIMD/systolic only if your resume invites it.
- Cache-aware coding (traversal order, blocking, false-sharing padding) is a strong signal in HFT systems rounds — mention it proactively.
