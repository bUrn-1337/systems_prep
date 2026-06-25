---
## LRU Cache — O(1) get and put

**One-line definition**
A fixed-capacity cache that evicts the least-recently-used entry when full, with O(1) get and put.

**What the interviewer actually asked**
"Implement an LRU cache with O(1) get and put." — asked by 5+ HFT/systems firms including Jane Street, Hudson River, Citadel, Two Sigma, and Optiver.

**The shallow answer** (gets you rejected)
"Use a map and track timestamps, evict the minimum timestamp on overflow." — O(n) eviction, wrong data structure, demonstrates no understanding of the constraint.

**The deep answer** (gets you through)
Two structures working together:
- `std::list<pair<int,int>>` (doubly linked list): stores (key, value) pairs. Most-recently-used sits at the front, least-recently-used at the back. O(1) splice to front, O(1) erase of any node.
- `unordered_map<int, list<pair<int,int>>::iterator>`: maps each key to the exact iterator in the list. O(1) lookup.

On **get**: key found → splice node to front (O(1) via iterator), return value. Key not found → return -1.

On **put**: key found → update value, splice to front. Key not found → insert at front, store iterator in map. If over capacity → read back iterator from map for `list.back()`, erase from map, then pop from list.

The doubly linked list is mandatory — `std::list::splice` and `std::list::erase` both require a pointer to the previous node. A singly linked list makes middle removal O(n).

**Implementation requirement?**
YES

```cpp
// WHY std::list: guarantees iterator stability on splice/erase — iterators stored in the
// map remain valid after any operation. std::deque or std::vector invalidate iterators on
// modification, making the map stale.
//
// WHY doubly linked: splice() and erase() on std::list are O(1) precisely because each
// node knows its predecessor. Removing a node from a singly linked list requires walking
// from head to find the previous node — O(n).
//
// WHY store iterator not pointer: iterators are the idiomatic handle into std::list.
// Storing raw node pointers is UB if you ever copy or move the list.
//
// WHY pair<int,int> in list not just value: on eviction, we must erase the key from the
// map. The only O(1) way to know *which* key to erase is to store the key inside the
// list node itself, so when we pop_back() we can look it up without an extra scan.

#include <list>
#include <unordered_map>
#include <stdexcept>

class LRUCache {
public:
    explicit LRUCache(int capacity) : capacity_(capacity) {
        if (capacity <= 0) throw std::invalid_argument("capacity must be > 0");
        map_.reserve(capacity);  // pre-allocate buckets to avoid rehash during use
    }

    int get(int key) {
        auto it = map_.find(key);
        if (it == map_.end()) return -1;
        // move accessed node to front — it is now most-recently-used
        list_.splice(list_.begin(), list_, it->second);
        return it->second->second;  // return value from (key, value) pair
    }

    void put(int key, int value) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            // key exists: update value in-place and promote to front
            it->second->second = value;
            list_.splice(list_.begin(), list_, it->second);
            return;
        }
        // new key: evict LRU if at capacity before inserting
        if (static_cast<int>(map_.size()) == capacity_) {
            // list_.back() is the least-recently-used node
            int lru_key = list_.back().first;
            map_.erase(lru_key);   // remove from map first (need the key)
            list_.pop_back();      // then remove from list
        }
        // insert new entry at front
        list_.emplace_front(key, value);
        map_[key] = list_.begin();
    }

    // Convenience for debugging/testing — not part of the interview interface
    int size() const { return static_cast<int>(map_.size()); }

private:
    int capacity_;
    std::list<std::pair<int, int>> list_;                       // (key, value), MRU at front
    std::unordered_map<int, std::list<std::pair<int,int>>::iterator> map_;
};
```

**The follow-up trap**
"Why is it doubly linked and not singly?"

Answer: `std::list::splice` and `std::list::erase` are O(1) because the node holds a `prev` pointer. Without `prev`, to unlink a node you must scan from head to find its predecessor — O(n). The entire O(1) guarantee collapses with a singly linked list.

Second follow-up: "This has terrible cache performance. How would you fix it for HFT?"

Answer: `std::list` nodes are heap-allocated individually, scattered across memory. Each access causes a cache miss. In a low-latency setting, replace the list with a fixed-size array used as a circular buffer or a pool-allocated doubly linked list (nodes stored in a contiguous array, linked by index). This gives cache-line locality. The algorithmic complexity stays O(1) but the constant factor drops dramatically. Pre-allocate `capacity` nodes at construction time so there are zero heap allocations during operation.

**Key numbers / facts**
- Cache line: 64 bytes. A `pair<int,int>` is 8 bytes — eight fit in one cache line if contiguous; `std::list` nodes scatter them one-per-allocation.
- `unordered_map::reserve(n)` at construction prevents rehash; default load factor 1.0 means rehash at n+1 insertions otherwise.
- `std::list::splice` is O(1) and does not invalidate any iterators or references — this is the core invariant the design depends on.
- Thread safety: none. For concurrent HFT use, shard by key-hash or use a seqlock per bucket.

---
## LFU Cache — O(1) get and put

**One-line definition**
A fixed-capacity cache that evicts the least-frequently-used entry (ties broken by least-recently-used) in O(1) per operation.

**What the interviewer actually asked**
"Now implement LFU — same O(1) constraint." — typically asked immediately after LRU; tests whether you can compose three interacting data structures under a tight invariant.

**The shallow answer** (gets you rejected)
"Use a min-heap on frequency." — O(log n) eviction. Slightly better: "Use a map from frequency to list." — correct structure but candidate cannot maintain `min_freq` in O(1), fumbles the `put` reset logic.

**The deep answer** (gets you through)
Three structures, one invariant:

1. `key_map`: `unordered_map<int, pair<int,int>>` — maps key → (value, frequency). O(1) lookup.
2. `freq_map`: `unordered_map<int, list<int>>` — maps frequency → doubly-linked list of keys at that frequency, ordered MRU-to-LRU (front = MRU). O(1) per-frequency LRU eviction.
3. `key_iter`: `unordered_map<int, list<int>::iterator>` — maps key → iterator in its frequency list. O(1) removal from the middle of any frequency bucket.
4. `min_freq`: single integer tracking the current minimum frequency.

**min_freq maintenance** is the hard part:
- On **get**: key's frequency goes from f to f+1. If f == min_freq and `freq_map[f]` is now empty, `min_freq++`. Otherwise min_freq unchanged (another key may still have frequency f).
- On **put** (new key): frequency is 1, so `min_freq = 1` unconditionally. This is the only time min_freq can decrease.
- On **put** (existing key): same as get — promote frequency, conditionally increment min_freq.

**Eviction**: remove the front of `freq_map[min_freq]` — that list is the LRU ordering within the minimum frequency bucket, and front = MRU, back = LRU, so evict `freq_map[min_freq].back()`.

**Implementation requirement?**
YES

```cpp
// WHY three maps instead of two: key_map gives O(1) value+frequency lookup.
// freq_map gives O(1) access to all keys sharing a frequency. key_iter gives O(1)
// removal of a specific key from its frequency bucket — without it you'd need O(n)
// scan of the frequency list to find the key before promoting it.
//
// WHY list<int> (keys only, not key-value): the value lives in key_map. Duplicating
// it in freq_map creates a consistency hazard. The list is purely for ordering within
// a frequency bucket; it stores keys and delegates value lookup to key_map.
//
// WHY min_freq resets to 1 on new put: new keys always start at frequency 1. Any
// existing key has frequency >= 1. Therefore the global minimum after a new insertion
// is always 1. No search needed.
//
// WHY min_freq only increments by 1 on get: get increases one key's frequency from
// f to f+1. The global minimum can only rise if the just-promoted key was the last
// key at frequency f. It cannot jump by more than 1 because no other key changed.
//
// WHY front=MRU, back=LRU within each frequency list: ties in frequency are broken
// by recency per LFU spec. New or promoted keys are pushed to front. Eviction
// always takes from back. This mirrors the LRU substructure inside each bucket.

#include <list>
#include <unordered_map>
#include <stdexcept>

class LFUCache {
public:
    explicit LFUCache(int capacity) : capacity_(capacity), min_freq_(0), size_(0) {
        if (capacity < 0) throw std::invalid_argument("capacity must be >= 0");
    }

    int get(int key) {
        auto it = key_map_.find(key);
        if (it == key_map_.end()) return -1;
        promote(key);  // increment frequency, maintain min_freq
        return key_map_[key].first;
    }

    void put(int key, int value) {
        if (capacity_ == 0) return;

        auto it = key_map_.find(key);
        if (it != key_map_.end()) {
            // key exists: update value, promote frequency
            it->second.first = value;
            promote(key);
            return;
        }
        // new key: evict if at capacity
        if (size_ == capacity_) {
            evict_lfu();
        }
        // insert with frequency 1
        key_map_[key] = {value, 1};
        freq_map_[1].push_front(key);
        key_iter_[key] = freq_map_[1].begin();
        min_freq_ = 1;  // new key has frequency 1 — global min is now 1
        ++size_;
    }

private:
    // promote: move key from frequency f to f+1, update min_freq if needed
    void promote(int key) {
        int freq = key_map_[key].second;
        // remove key from its current frequency bucket
        freq_map_[freq].erase(key_iter_[key]);
        // if this bucket is now empty and it was the minimum, min_freq rises by 1
        if (freq_map_[freq].empty()) {
            freq_map_.erase(freq);
            if (min_freq_ == freq) ++min_freq_;
        }
        // insert into the next frequency bucket at the front (most recent)
        int new_freq = freq + 1;
        freq_map_[new_freq].push_front(key);
        key_map_[key].second = new_freq;
        key_iter_[key] = freq_map_[new_freq].begin();
    }

    // evict_lfu: remove the LRU entry from the minimum-frequency bucket
    void evict_lfu() {
        auto& lfu_list = freq_map_[min_freq_];
        int evict_key = lfu_list.back();  // back = least recently used in this bucket
        lfu_list.pop_back();
        if (lfu_list.empty()) freq_map_.erase(min_freq_);
        key_iter_.erase(evict_key);
        key_map_.erase(evict_key);
        --size_;
        // NOTE: do NOT update min_freq_ here. evict_lfu is only called from put() on
        // a new key, which immediately sets min_freq_ = 1 after eviction. Updating it
        // here would be wasted work and could corrupt state if called from elsewhere.
    }

    int capacity_;
    int min_freq_;
    int size_;
    // key -> (value, frequency)
    std::unordered_map<int, std::pair<int, int>> key_map_;
    // frequency -> list of keys at that frequency, MRU at front
    std::unordered_map<int, std::list<int>> freq_map_;
    // key -> iterator into its frequency list (for O(1) mid-list removal)
    std::unordered_map<int, std::list<int>::iterator> key_iter_;
};
```

**The follow-up trap**
"What happens to min_freq when you call get versus put?"

Answer: On **get**, min_freq can only increase by 1, and only if the promoted key was the sole key at that frequency. On **put** of a new key, min_freq unconditionally resets to 1 regardless of current state — because the new key has frequency 1 and 1 is the global minimum by definition. This asymmetry is the most common bug in LFU implementations. Candidates who do not reset min_freq on new insertions pass all easy test cases but fail when a new key is inserted after several gets have raised min_freq above 1.

Second follow-up: "LFU versus LRU — when does LFU hurt you?"

Answer: LFU suffers from frequency pollution — a key accessed 1000 times in the past but never again will hold its slot indefinitely, blocking newer hot keys. Real systems use variants: LFU with aging (decay frequencies periodically), TinyLFU (count-min sketch approximation used in Caffeine/RocksDB), or LIRS. LRU is simpler and handles scan-once workloads better. For HFT order book data, LRU often wins because access patterns are recency-dominated, not frequency-dominated.

**Key numbers / facts**
- Space complexity: O(n) across all three maps — each key has exactly one entry in each of the three maps at all times.
- `list::erase` via stored iterator: O(1). Without the iterator map, finding the key in its frequency list is O(bucket_size), which is O(n) worst case.
- min_freq can never exceed `size_` — every key starts at 1 and the maximum reachable frequency in k operations on a single key is k.
- A capacity-0 LFU must be a no-op on all operations — handle explicitly; otherwise `evict_lfu` on an empty cache causes UB.

---
## Binary Lifting — LCA in O(log N)

**One-line definition**
Precompute 2^j-th ancestors for every node to answer lowest common ancestor queries in O(log N) after O(N log N) preprocessing.

**What the interviewer actually asked**
"What is binary lifting? Implement LCA using binary lifting." — asked by Graviton Research; also appears at systems-adjacent firms testing algorithmic depth.

**The shallow answer** (gets you rejected)
"Walk both nodes up to the root and find the first common node." — O(N) per query, no precomputation, misses the entire point of the technique.

**The deep answer** (gets you through)
Binary lifting exploits the fact that any integer depth difference can be expressed as a sum of powers of two.

**Preprocessing** — build table `up[node][j]` = the 2^j-th ancestor of `node`:
- `up[node][0]` = direct parent (base case, filled during BFS/DFS from root).
- `up[node][j]` = `up[up[node][j-1]][j-1]` — the 2^j-th ancestor is the 2^(j-1)-th ancestor of the 2^(j-1)-th ancestor.
- Fill in order of increasing j so that `up[node][j-1]` is already valid when computing `up[node][j]`.

**LCA query** for nodes u, v:
1. Ensure `depth[u] >= depth[v]`. If not, swap.
2. Lift u by `depth[u] - depth[v]` using binary representation of the difference — bring both nodes to the same depth.
3. If u == v after equalization, v is the LCA (one was an ancestor of the other).
4. Otherwise, binary-lift both simultaneously from the highest bit down. Invariant: move both up only if their 2^j-th ancestors differ (if they are the same, the LCA is at or below that level — do not overshoot). After the loop, `up[u][0]` is the LCA.

**Implementation requirement?**
YES

```cpp
#include <vector>
#include <queue>
#include <cmath>

// Binary lifting LCA
// Preprocessing: O(N log N) time and space
// Query: O(log N)
// Constraint: rooted tree, 1-indexed nodes, root = 1

class LCA {
public:
    LCA(int n, const std::vector<std::vector<int>>& adj, int root = 1)
        : n_(n), LOG_(static_cast<int>(std::log2(n)) + 1),
          depth_(n + 1, 0),
          up_(n + 1, std::vector<int>(static_cast<int>(std::log2(n)) + 1, 0))
    {
        bfs(adj, root);
        build();
    }

    // query: returns the LCA of nodes u and v
    int query(int u, int v) const {
        // step 1: bring u to the same depth as v
        if (depth_[u] < depth_[v]) std::swap(u, v);
        int diff = depth_[u] - depth_[v];
        for (int j = 0; j < LOG_; ++j)
            if ((diff >> j) & 1)
                u = up_[u][j];

        // step 2: if now equal, one was ancestor of the other
        if (u == v) return u;

        // step 3: lift both simultaneously — move only when ancestors differ
        for (int j = LOG_ - 1; j >= 0; --j)
            if (up_[u][j] != up_[v][j]) {
                u = up_[u][j];
                v = up_[v][j];
            }

        // u and v are now the direct children of the LCA
        return up_[u][0];
    }

    int depth(int u) const { return depth_[u]; }

private:
    void bfs(const std::vector<std::vector<int>>& adj, int root) {
        std::queue<int> q;
        std::vector<bool> visited(n_ + 1, false);
        q.push(root);
        visited[root] = true;
        up_[root][0] = root;  // root's parent is itself (sentinel)
        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (int v : adj[u]) {
                if (!visited[v]) {
                    visited[v] = true;
                    depth_[v] = depth_[u] + 1;
                    up_[v][0] = u;  // direct parent
                    q.push(v);
                }
            }
        }
    }

    void build() {
        // fill table in order of increasing j
        for (int j = 1; j < LOG_; ++j)
            for (int i = 1; i <= n_; ++i)
                up_[i][j] = up_[up_[i][j-1]][j-1];
    }

    int n_, LOG_;
    std::vector<int> depth_;
    std::vector<std::vector<int>> up_;  // up[node][j] = 2^j-th ancestor
};
```

**The follow-up trap**
"Why do you move both nodes simultaneously in step 3, and why from high bit to low?"

Answer: Moving from high bit to low is greedy — you want to take the largest possible jumps first without overshooting the LCA. If you moved from low to high you might undershoot repeatedly and converge incorrectly. Moving simultaneously preserves the invariant: if `up[u][j] != up[v][j]`, the LCA is strictly above that level, so it is safe to jump both. If they are equal, the LCA is at or below — do not move, because you would jump past it.

**Key numbers / facts**
- Table size: N × log₂(N). For N = 10^5, that is ~10^5 × 17 ≈ 1.7M integers — fits comfortably in L3 cache.
- LOG must be at least floor(log₂(N)) + 1. Off-by-one here causes wrong answers on paths of length exactly a power of two.
- Root's parent sentinel (`up[root][0] = root`) ensures lifting never goes out of bounds — the root is its own ancestor at all levels.
- Alternative: Euler tour + sparse table RMQ gives O(1) query at the same O(N log N) preprocessing, but higher constant; binary lifting is simpler to implement under time pressure.

---
## Bipartite Graph Check + DP on Bipartite Graphs

**One-line definition**
A graph is bipartite if its vertices can be 2-colored such that no edge connects same-colored vertices; equivalently, it contains no odd-length cycle.

**What the interviewer actually asked**
"Check whether a graph is bipartite. Then: given a bipartite graph with weighted edges, find the maximum weight matching." — Graviton Research.

**The shallow answer** (gets you rejected)
"BFS and color nodes alternately." — correct start but stops there; cannot explain the DP angle or matching.

**The deep answer** (gets you through)
**Bipartite check** via BFS 2-coloring: assign color 0 to the source, color 1 to all its neighbors, and so on. If you ever encounter an edge between two same-colored nodes, the graph is not bipartite. Handles disconnected graphs by running BFS from every unvisited node.

**DP on bipartite graphs — assignment problem basics:**

The canonical problem: N workers, N jobs, cost[i][j] to assign worker i to job j. Minimize total cost. This is bipartite matching with weights. Three approaches:

1. **Hungarian algorithm**: O(N^3). Maintains dual variables (potentials) for each worker and job. At each step finds an augmenting path in the equality subgraph (where reduced cost = 0), updates potentials to extend the equality subgraph. No DP in the traditional sense, but the potential update is a DP-like relaxation.

2. **DP on bipartite DAG**: If the bipartite graph is a DAG (directed edges from left to right partition), standard shortest/longest path DP applies. `dp[j]` = best value achievable at right-node j considering left-nodes 1..i processed so far. Transition: `dp[j] = max(dp[j], dp[prev] + w(i, j))`.

3. **Bitmask DP for small N**: `dp[mask]` = minimum cost to assign the first `popcount(mask)` workers to the subset `mask` of jobs. Transition: let `i = popcount(mask)`, try all j in mask: `dp[mask] = min over j in mask of dp[mask ^ (1<<j)] + cost[i][j]`. O(N * 2^N). Practical for N ≤ 20.

**Implementation requirement?**
YES — bipartite check and bitmask DP matching

```cpp
#include <vector>
#include <queue>
#include <climits>

// Bipartite check via BFS 2-coloring
// Returns true if graph is bipartite, fills color[] with 0/1 partition
bool isBipartite(const std::vector<std::vector<int>>& adj, int n,
                 std::vector<int>& color) {
    color.assign(n, -1);
    for (int start = 0; start < n; ++start) {
        if (color[start] != -1) continue;  // already visited component
        std::queue<int> q;
        q.push(start);
        color[start] = 0;
        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (int v : adj[u]) {
                if (color[v] == -1) {
                    color[v] = 1 - color[u];
                    q.push(v);
                } else if (color[v] == color[u]) {
                    return false;  // same color on both endpoints — odd cycle found
                }
            }
        }
    }
    return true;
}

// Bitmask DP — minimum cost perfect matching on N x N bipartite graph
// cost[i][j] = cost of assigning worker i to job j
// dp[mask] = min cost to assign workers 0..(popcount(mask)-1) to jobs in mask
// O(N * 2^N) time, O(2^N) space. Practical for N <= 20.
int minCostMatching(const std::vector<std::vector<int>>& cost, int n) {
    int states = 1 << n;
    std::vector<int> dp(states, INT_MAX / 2);
    dp[0] = 0;
    for (int mask = 0; mask < states; ++mask) {
        if (dp[mask] == INT_MAX / 2) continue;
        int worker = __builtin_popcount(mask);  // next worker to assign
        if (worker == n) continue;
        for (int job = 0; job < n; ++job) {
            if (mask & (1 << job)) continue;    // job already assigned
            int next = mask | (1 << job);
            dp[next] = std::min(dp[next], dp[mask] + cost[worker][job]);
        }
    }
    return dp[states - 1];  // all jobs assigned
}
```

**The follow-up trap**
"When does bitmask DP beat Hungarian?"

Answer: Bitmask DP is O(N * 2^N); Hungarian is O(N^3). Crossover is around N = 20 where 2^20 ≈ 10^6 and N^3 = 8000. For N < ~15, bitmask DP is competitive and simpler to implement correctly under pressure. For N > 20, Hungarian wins decisively. In HFT context (portfolio assignment, order routing to venues), N is typically small (≤ 10 venues), so bitmask DP is the practical choice with the added benefit of trivial correctness verification.

Second follow-up: "What property of bipartite graphs makes the LP relaxation of the assignment problem always yield integer solutions?"

Answer: The constraint matrix of the assignment LP is totally unimodular — every square submatrix has determinant in {-1, 0, 1}. This guarantees that the LP optimal is always at an integer vertex of the polytope. Practically: you can solve the assignment problem as an LP and read off an integer matching without branch-and-bound. This is why network flow on bipartite graphs with integer capacities always returns integer flows.

**Key numbers / facts**
- Bipartite iff no odd cycle (König's theorem corollary).
- BFS 2-coloring: O(V + E) time, O(V) space.
- Maximum bipartite matching (unweighted): O(E * sqrt(V)) with Hopcroft-Karp.
- Hungarian algorithm: O(N^3) — the standard for dense weighted bipartite matching in competitive programming and practice.
- Bitmask DP: practical ceiling is N = 20 (2^20 = ~1M states); at N = 25 it becomes 33M states which is borderline; N = 30 is infeasible (1B states).
- `__builtin_popcount` compiles to a single `POPCNT` instruction on x86 — O(1), not a loop.
