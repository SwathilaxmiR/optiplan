# Walkthrough: Network Topology, Routing & Spectrum Management

## 1. What It Does
Phase 2 builds the core network graph, path computation, and spectrum management engine:
- **`Topology`**: Represents nodes and bidirectional physical fiber links loaded from standardized JSON files (`toy4`, `nsfnet`, `nobel-eu`). Builds $O(1)$ adjacency graphs and link lookups, and verifies graph connectivity using Breadth-First Search (BFS).
- **`Routing`**:
  - Implements **Dijkstra's algorithm** with a min-heap priority queue supporting arbitrary excluded node and edge sets.
  - Implements **Yen's loopless $k$-shortest paths algorithm (1971)**.
  - Implements `KShortestPathCache` to precompute and cache all-pairs $k$-shortest paths for simulation speedup.
- **`SpectrumState`**:
  - High-performance bitset-based (`std::bitset<128>`) wavelength slot representation for the 50 GHz optical grid (default 80 channels).
  - Path-level bitwise operations (`findAvailableSlots(Path)`) computing the intersection of free slots along all links in a single bitwise AND.
  - Strict enforcement of the **wavelength continuity constraint** and **non-overlap constraint**.
  - Allocation policies: **First-Fit**, **Most-Used**, and **Random-Fit**.

---

## 2. Why It Is Designed This Way
- **Bitset-Accelerated Spectrum Tracking**: Rather than storing vectors of lightpath objects on each link and checking overlap via nested loops, a 128-bit bitset (`SlotMask`) allows finding slots free on an entire $m$-hop path with just $m$ bitwise AND operations (`free_mask &= ~occupied[link]`). This evaluates in single-digit nanoseconds per candidate.
- **Yen's Algorithm vs. Simple Multi-Path**: Yen's algorithm guarantees that all generated candidate paths are **loopless** and strictly ordered by path length ($L(P_0) \le L(P_1) \le \dots \le L(P_{k-1})$). Deviations in other heuristics can generate loops or missed shortest alternatives.
- **All-Pairs Precomputation Cache (`KShortestPathCache`)**: In static and dynamic optical network planning, routing topologies do not change on every demand arrival. Precomputing the top $k$ shortest paths between all $(s, d)$ pairs upfront turns the candidate routing step into an $O(1)$ lookup during simulation, keeping the simulation inner loop fast.

---

## 3. Key Functions to Read First
1. `core/src/routing.cpp` -> `yenKShortestPaths`:
   - Inspect how the spur node and root path are determined, how branching edges are temporarily excluded to force divergence, and how looplessness is maintained.
2. `core/src/spectrum.cpp` -> `SpectrumState::findAvailableSlots` & `allocateSlot`:
   - See how bitwise masking instantly enforces wavelength continuity across all links of a candidate route, and how non-overlap is strictly validated.
3. `core/src/main.cpp`:
   - The CLI demo showing end-to-end orchestration: loading config $\rightarrow$ loading topology $\rightarrow$ routing candidates $\rightarrow$ QoT evaluation $\rightarrow$ spectrum allocation.

---

## 4. Likely Interview Questions & Answers

### Q1: How does Yen's algorithm guarantee that all $k$ candidate paths are loopless, and what is its computational complexity?
**Answer:**
Yen's algorithm guarantees looplessness by decomposing candidate generation from path $A_{i-1}$ into a *root path* and a *spur path*. For each node $j$ along $A_{i-1}$, the root path is the subpath from source $s$ to spur node $u_j$. To prevent the spur path from looping back onto the root path, **all nodes in the root path (except the spur node itself) are explicitly excluded from the graph** during the Dijkstra search for the spur path to destination $d$.
Additionally, any outgoing edges from the spur node that coincide with previously confirmed paths in $A$ sharing the exact same root path are temporarily removed.
The computational complexity is $O(k \cdot V \cdot (E + V \log V))$, where $V$ is node count and $E$ is edge count. By precomputing and caching all-pairs $k$-shortest paths once during initialization (`KShortestPathCache`), this cost is paid only once upfront.

### Q2: What is the Wavelength Continuity Constraint in transparent optical networks, and how does your bitset design optimize its verification?
**Answer:**
In transparent (all-optical) networks without optical-electrical-optical (OEO) wavelength converters, an optical lightpath must occupy the exact same wavelength slot on every fiber link along its end-to-end path. If slot 4 is occupied on even a single intermediate link, slot 4 cannot be used for that lightpath.
In OptiPlan, each link maintains a `SlotMask` (`std::bitset<128>`) where bit $c = 1$ denotes occupied and $0$ denotes free. Finding all valid wavelengths across a path $P = (e_1, e_2, \dots, e_m)$ is computed via:
$$\text{FreeMask} = \text{ValidMask} \ \& \ (\sim \text{Occupied}(e_1)) \ \& \ (\sim \text{Occupied}(e_2)) \ \& \ \dots \ \& \ (\sim \text{Occupied}(e_m))$$
Because bitwise operations operate directly on 64-bit/128-bit CPU words, finding available wavelength slots across an entire multi-hop path takes only a handful of CPU clock cycles without any dynamic memory allocations.

### Q3: Why is First-Fit usually preferred over Random-Fit for spectrum assignment in optical networks?
**Answer:**
In optical networks, random assignment scatters active lightpaths across the spectrum, causing severe **spectrum fragmentation** (leaving small, non-contiguous free gaps that cannot accommodate wider channels or multi-slot superchannels).
**First-Fit** always packs connections into the lowest available frequency slot index. This packs existing traffic toward lower wavelength channels and keeps the higher frequency spectrum open and contiguous for longer, multi-hop lightpaths arriving in the future. As demonstrated in optical literature, First-Fit achieves significantly lower blocking probability than Random-Fit under dynamic traffic.
