# Macro Placer — Region-Based Spreading Plan

Working notes for improving my sequence-pair + simulated-annealing macro placer.
Bringing this into Claude Code so it has the plan alongside the actual codebase.

## Problem

Macros all compact toward the **lower-left corner** of the die. I want them
spread out and hugging the **edges/periphery** of the die, leaving the center
open for standard cells.

## Root cause (important — don't fix this the wrong way)

This is a **sequence-pair decoding artifact**, not an annealer convergence
problem. SP decoding places each block at its earliest feasible position
(longest-path / compaction), so *every* sequence pair decodes to a
bottom-left-justified layout. There is no permutation that decodes to a
"spread across the edges" result — the packer always compacts into the corner.

Consequence: **adding a spreading term to the SA cost function alone will not
work.** No move produces a spread layout for the cost to reward. The fix has to
change *where/how* things pack, not just the cost.

## The plan (do in this order — each step is testable on its own)

### 1. Per-region mirrored packing  ← start here, highest value / lowest risk

- Split the die into 4 quadrant regions (SW, SE, NW, NE) that tile the die.
- Run the existing SP + SA decoder independently in each region, in its own
  local coordinate frame.
- **Mirror the pack direction per quadrant** so each region compacts toward its
  *outer* die corner instead of all pointing inward:
  - SW → pack normally (toward SW corner)
  - SE → mirror x
  - NW → mirror y
  - NE → mirror both
- Cheapest implementation: keep the normal decoder, then reflect the packed
  result inside the region rectangle (`W`×`H`):
  - mirror x:  `x' = W - x - w`
  - mirror y:  `y' = H - y - h`
- Test with a hardcoded/dummy region assignment first, before touching
  partitioning. Verify **no overlaps** after reflection.

### 2. Region assignment (which macro goes in which quadrant) — MIN-CUT

**Decision: use the min-cut partitioning approach** (from Kahng, *VLSI Physical
Design: From Graph Partitioning to Timing Closure*). This assigns macros to
regions by connectivity so heavily-connected macros land together and
cross-region wiring is minimized.

(Not using the old OpenROAD MPL rule of "which side of the cut line is the
macro's current center on" — that relies on inheriting a prior **global
placement**, which I don't have.)

#### 2a. Recursive bisection: 1 → 2 → 4

KL / FM partitioning produces a *balanced bipartition*, not a 4-way split. Get
the four quadrant regions by applying min-cut **recursively, two levels deep**:

- Level 1: bisect the whole macro set into two halves (e.g. left / right).
- Level 2: bisect each half again (e.g. top / bottom) → four regions total.
- Use one cut orientation per level (level 1 horizontal split, level 2 vertical,
  or vice versa) so the recursion maps cleanly onto the two die cut lines.
  Structurally this mirrors what the old MPL did, even though the assignment
  rule inside is different.

#### 2b. Build the hypergraph — macro adjacency via logic  ← chosen: option A

Macros are usually connected *through* standard-cell logic, not directly to each
other. A hypergraph of only macro-to-macro nets would be too sparse to cut
meaningfully.

**Chosen approach (A): build macro adjacency by tracing connectivity through the
logic.** For each pair of macros, derive an edge/weight from the logic paths that
connect them — trace fanout from macro output pins forward through combinational
logic (and across register stages up to some depth) until reaching another
macro's input pin, and accumulate a weight per macro pair. This is exactly the
technique the old OpenROAD code used: a BFS forward through the timing/connectivity
graph, unioning fanin sets, and jumping across register D->Q boundaries for a few
levels of depth. The result is a weighted macro-adjacency graph that min-cut can
actually separate.

(Rejected option B: partition the *whole* netlist including std cells, then read
off which region each macro fell into. Simpler conceptually but pulls the entire
design into the partitioner.)

#### 2c. Bias which quadrant a cluster lands in

Add fixed pseudo-terminals at the die edges/corners representing the I/O pins,
and include them in each region's HPWL — a cluster gravitates toward the corner
nearest its I/O.

#### 2d. Area balance

FM's standard balance constraint balances by **node count**, but macros vary
wildly in size, so an even count can still overload a region by **area**.
Two options:

- Weight the FM balance constraint by macro **area** (weighted partitioning,
  cleaner — this is usually the version Kahng discusses).
- Or keep FM as-is and add a per-region area-capacity reject afterward (cf.
  MPL's `macroMacroArea > regionArea` check).

### 3. Parallelism

- Trivial once 1 & 2 are clean — regions are independent.
- Main thing: make sure SA state and the RNG are **not shared** across threads
  (per-thread RNG, no shared mutable placement state).

## Open questions for Claude Code to ask me

1. **Which SP decoder am I using** — LCS-based `O(n log n)`, or the older
   constraint-graph longest-path `O(n²)`? The reflection in step 1 interacts
   differently with each; flipping coordinates without flipping the constraint
   direction can silently introduce overlaps.
2. **What connectivity data do I have for the logic-tracing (option A)?**
   Tracing macro adjacency through the logic needs the full gate-level netlist
   (macro pins → nets → std-cell gates → ... → other macro pins), plus I/O pin
   positions for the edge pseudo-terminals. Confirm I have: the flat/hierarchical
   netlist with instance connectivity, a way to identify register/sequential
   cells (to jump D->Q across pipeline stages), and pin directions. If I only
   have macro-to-macro nets, option A won't work and I'd need to fall back.
3. Language is C++ (confirm).

## Context

The idea was inspired by the old OpenROAD TritonMP / MPL placer, but note that
placer relied on a prior global placement and used a deliberately *inverted*
(maximize-wirelength) objective to spread macros. My approach (pre-split + per-
region SA) is actually closer to the newer OpenROAD hierarchical placer
(mpl2 / RTL-MP). The mirrored-packing trick is my own addition to get edge-
hugging rather than center-pointing clusters.
