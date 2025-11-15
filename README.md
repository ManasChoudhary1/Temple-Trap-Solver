# Temple Trap Solver

An optimal solver for the Temple Trap puzzle using the A* search algorithm that finds the shortest solution path for any valid board configuration.

## Tiles

The puzzle uses the following tile set:

![Temple Trap Board Diagram](./assets/tiles.png)

- Each tile is labeled by a letter and an orientation, where orientation is a rotation in 90° steps: 0–3.
- `--` represents the blank (empty) tile.
- The pawn moves through corridors and stairs while tiles slide to form paths.

## Input format

Input is read from `input.txt`.

- First line: `T` (number of test cases).
- Next `T` lines: One 20-character string per test case.

### Test case string (20 chars)

- Board (characters 0–17):
  - The first 18 characters define the 3×3 board as 9 two-character pairs in row-major order (top-left to bottom-right).
  - Example: `A2`, `E3`, `--`, etc.
  - `T0` means tile `T` with orientation `0`, where orientation is 0–3 (clockwise rotation index).
  - `--` is the blank tile.

- Pawn (characters 18–19):
  - The last two characters are `rc`, the pawn’s 0-indexed start position.
  - `r` is the row (0–2), `c` is the column (0–2).

Example `input.txt`:
```
1
A2E3G3B0--D0H2C1F201
```

## Core logic: A* search

The solver implements A* to find optimal (shortest-cost) solutions across both tile slides and pawn movements in a unified search space.

### State (GameState)

A state captures a full puzzle snapshot:
- 3×3 board configuration (BoardConfig).
- Pawn’s exact position: `(row, col)` and floor (`top` or `ground`).
- Blank tile position.
- `cost_so_far` (g): total cost from start to this state.
- `heuristic_cost` (h): admissible estimate to goal.

### Transitions (Moves)

From any state, the solver enumerates two move classes:

- Tile slides (`find_tile_slides`)
  - Swap the blank with an adjacent tile (up to 4 directions), unless the pawn is on that tile.
  - Each slide has cost `1`.

- Pawn moves (`find_pawn_moves`)
  - Run BFS from the pawn’s current cell to discover all reachable “resting spots” (tiles with holes or the final exit) within a single turn.
  - Generate one successor per reachable resting spot; the move cost equals the BFS path length in steps.

### Heuristic (calculate_heuristic)

An admissible heuristic guides A*:
- Manhattan distance from the pawn to the exit at logical coordinate `(0, -1)` (left of top-left cell).
- Penalties if the blank is in a “bad” position, e.g., at `(0,0)` blocking exit flow.
- Penalty if tile `(0,0)` lacks an open top on the left.

The heuristic remains conservative to preserve optimality.

## Optimizations

To avoid re-exploration and reduce overhead:

- CompactState key:
  - Encode board tiles/orientations and pawn/blank positions into a compact hashable key for `min_cost` tracking.
  - This significantly reduces hashing and memory overhead relative to storing full GameState objects.

- Fast containers:
  - `std::unordered_map` for visited/min-cost bookkeeping yielded ~10–12× reduction vs `std::map`.

- Flat board:
  - A 1D array for the 3×3 board produced an additional ~5× speedup over a 2D structure.

- Priority queue note:
  - The remaining bottleneck is the priority queue; further improvements may be possible with a more compact, encoded state representation and a custom comparator or bucketed queues for small integer costs.

Combined, these changes reduced the worst-case test from ~250s to ~2s on the same machine.

## Running guide

1) Prepare input  
- Add the number of test cases and the 20-character test case strings to `input.txt` (one string per line after `T`).

2) Build (C++23)  
```
g++ -O3 -march=native -std=c++23 solver.cpp -o solver
```

3) Run  
```
./solver
```

4) Inspect outputs  
- `output.txt`: full, step-by-step solution path.  
- `error.txt`: total states explored and final runtime.

## Performance and notes

- Solves even complex cases in under ~3 seconds on a modern CPU.
- Heuristic is admissible, so A* solutions are optimal.
- Tough cases (e.g., 200–300 steps):
  - The heuristic’s single-digit estimate is small relative to 3-digit costs, yielding only ~5% state reduction; for very large costs (1000s), removing the heuristic might help by eliminating heuristic computation overhead.
- Easy/elegant cases:
  - Heuristic is highly effective, often reducing explored states by 30–60%.

## File overview

- `solver.cpp`: A* implementation, state encoding, move generation, heuristic.
- `input.txt`: Test cases (see format above).
- `output.txt`: Solution path for each test.
- `error.txt`: Diagnostics (states explored, runtime).
- `assets/tiles.png`: Tile legend/diagram used in this README.

## Example checklist

- [ ] Validate 20-char test case lines and reject malformed inputs early.
- [ ] Ensure pawn is never on the sliding tile during a slide.
- [ ] BFS only yields “resting spots” (holes/exit), not arbitrary intermediate positions.
- [ ] Heuristic never overestimates (admissible).
- [ ] Use CompactState as the unordered_map key for min-cost pruning.
- [ ] Write outputs consistently for post-run analysis.

---
Happy puzzling and fast escapes!
```
