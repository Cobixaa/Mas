### MasChess — Fast, Optimized UCI Chess Engine (C++)

MasChess is a compact, highly-optimized UCI chess engine written in modern C++20. It uses bitboards, iterative deepening alpha-beta with aspiration windows, a transposition table, killer/history heuristics, and a solid evaluation. It speaks UCI, so it works out of the box with GUIs like DroidFish.

No CMake, no Makefile — just a single direct compile command.

---

### Features

- Bitboard move generation (fast knights/kings/pawns; sliding rays)
- Iterative deepening negamax with alpha-beta pruning
- Quiescence search (captures only) with delta pruning
- Transposition table (configurable size), Zobrist hashing
- Move ordering: TT move, MVV-LVA captures, killer moves, history heuristic
- UCI protocol support ("uci", "isready", "position", "go", "stop", "quit")
- Single-binary build: one compile command, no build system

---

### Files

- `src/main.cpp`: program entry; UCI loop
- `src/uci.h/.cpp`: UCI protocol parsing/dispatch
- `src/search.h/.cpp`: search, time management, move ordering
- `src/tt.h/.cpp`: transposition table
- `src/eval.h/.cpp`: evaluation
- `src/movegen.h/.cpp`: legal move generation
- `src/board.h/.cpp`: board state, make/unmake, Zobrist hash
- `src/common.h`: types, constants, utilities

---

### Quick Start (Linux / macOS)

Requirements: Clang or GCC with C++20.

Compile:

```bash
clang++ -O3 -DNDEBUG -flto -march=native -std=c++20 -pthread -s -o maschess src/*.cpp
```

Run in UCI mode (for testing):

```bash
./maschess
uci
isready
position startpos moves e2e4 e7e5 g1f3
go movetime 2000
```

---

### Android (Termux) — Step-by-step

1) Install Termux from F-Droid (`https://f-droid.org`) or Play Store (if available).

2) Open Termux and set up storage access:

```bash
termux-setup-storage
```

3) Install toolchain:

```bash
pkg update -y && pkg upgrade -y
pkg install -y clang git
```

4) Clone or copy this project into Termux. If you already have the files, `cd` into the folder. Otherwise:

```bash
cd ~
git clone https://example.com/your/maschess.git maschess
cd maschess
```

5) Compile (ARM64 phones):

```bash
clang++ -O3 -DNDEBUG -flto -std=c++20 -pthread -march=armv8-a+crypto -mtune=native -s -o maschess src/*.cpp
```

If you have a 32-bit device (rare), try:

```bash
clang++ -O3 -DNDEBUG -flto -std=c++20 -pthread -march=armv7-a -mfpu=neon -mfloat-abi=softfp -s -o maschess src/*.cpp
```

6) Move the binary to a location DroidFish can see (Downloads):

```bash
cp ./maschess ~/storage/downloads/maschess
```

7) In DroidFish:

- Open DroidFish
- Menu → Manage Chess Engines → Add engine → From file
- Browse to Downloads and select `maschess`
- Select the engine; DroidFish should show "UCI engine" and allow play/analysis

Tip: If DroidFish does not see Downloads, ensure Termux storage is granted (step 2) and the file is executable:

```bash
chmod +x ~/storage/downloads/maschess
```

---

### UCI Options

The engine exposes minimal options via UCI:

- `Hash` (MB): Size of transposition table. Default 64. Example from GUI: set to 128 or 256 if you have RAM.

In a GUI (including DroidFish), open Engine options to adjust.

---

### Usage Tips

- Strength scales with time and `Hash` size
- On mobile, 128–256 MB Hash is usually safe; avoid gigabyte sizes
- Single-threaded by design (reliable on all phones). GUI can still run multiple instances for analysis if desired.

---

### Direct CLI usage examples

Play a quick move from start position:

```bash
./maschess <<EOF
uci
isready
position startpos
go movetime 1000
quit
EOF
```

Load a FEN and search to depth 12:

```bash
./maschess <<EOF
uci
isready
position fen r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 2 3
go depth 12
quit
EOF
```

---

### Technical Notes

- Board: bitboards per piece & color, Zobrist hashing
- Move gen: pseudo-legal + legality check; fast precomputed attacks for non-sliders; on-the-fly sliding rays
- Search: principal variation, TT cutoff, LMR, late move pruning on quiets, killers/history
- Quiescence: captures-only with delta pruning and SEE-lite via MVV-LVA ordering
- Evaluation: material, piece-square tables, mobility, bishop pair, pawn structure basics

---

### License

MIT

# Mas
A Prototype Chess Engine.
