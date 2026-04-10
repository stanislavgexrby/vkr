# SynGT C++ — Syntax Graph Transformation

A C++17 port of the SynGT tool for editing and visualizing formal grammars as syntax diagrams (railroad diagrams). Grammars are described in RBNF (Regular Backus-Naur Form); the system supports equivalence-preserving transformations such as left/right recursion elimination, recursion analysis, rule extraction, and substitution.

Originally developed at the Department of Computer Science, Saint Petersburg State University. The original system was written in Object Pascal (Borland Delphi).

---

## Components

| Component | Description | Platforms |
|-----------|-------------|-----------|
| `libsyngt` | Core library: parser, grammar model, transformations, visualization | Windows, Linux |
| `syngt_cli` | Console application for batch grammar processing | Windows, Linux |
| `syngt_gui` | Interactive GUI with diagram editor | Windows (DirectX 11) |

---

## Requirements

### All platforms
- CMake 3.20+
- C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)
- Google Test — fetched automatically by CMake

### Windows (MSYS2/MinGW — recommended)
```bash
pacman -S mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-cmake \
          mingw-w64-ucrt-x86_64-ninja
```

### Linux (Ubuntu/Debian)
```bash
sudo apt install build-essential cmake ninja-build git
```

---

## Build

### Library and CLI only (cross-platform)
```bash
cmake -B build -DBUILD_GUI=OFF -DBUILD_TESTS=OFF
cmake --build build
```

### With tests
```bash
cmake -B build -DBUILD_GUI=OFF -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

### With GUI (Windows only)
```bash
# First run setup to fetch Dear ImGui
setup.bat

cmake -B build -DBUILD_GUI=ON
cmake --build build
```

Binaries are placed in `build/bin/`.

---

## Usage

### GUI

Launch `syngt_gui.exe` (Windows). The application opens with a minimal empty grammar `S : eps.` ready to edit.

**Main operations (via menu or right-click on diagram):**
- `Grammar → Analyze Recursions` — detect direct/indirect left, right, and general recursion for each non-terminal
- `Grammar → Eliminate Left Recursion` — transform left-recursive rules into iterations
- `Grammar → Eliminate Right Recursion` — transform right-recursive rules into iterations
- `Grammar → Eliminate General Recursion` — eliminate both left and right recursion
- `Grammar → Extract Rule` — extract a selected diagram fragment into a new non-terminal (equivalent transformation)
- `Grammar → Substitute` — inline a non-terminal's definition at the selected reference (equivalent transformation)
- `File → Import from GEdit` — load `.grw` grammar files from the GEdit editor

### CLI

```
syngt_cli <command> [options]

Commands:
  info <grammar.grm>                 Show grammar info (non-terminals, terminals, semantics)
  eliminate-left <in.grm> <out.grm>  Eliminate left recursion
  regularize <in.grm> <out.grm>      Eliminate left recursion + left factorization
  factorize <in.grm> <out.grm>       Apply left factorization
  remove-useless <in.grm> <out.grm>  Remove useless symbols
  check-ll1 <grammar.grm>            Check if grammar is LL(1)
  first-follow <grammar.grm>         Print FIRST and FOLLOW sets
  table <grammar.grm>                Generate LL(1) parsing table
```

**Examples:**
```bash
# Show grammar structure
./syngt_cli info examples/grammars/CALC.GRM

# Eliminate left recursion and save result
./syngt_cli eliminate-left examples/grammars/recursion/left_simple.grm result.grm

# Check if grammar is LL(1)
./syngt_cli check-ll1 examples/grammars/CALC.GRM

# Compute FIRST/FOLLOW sets
./syngt_cli first-follow examples/grammars/LANG.GRM
```

---

## Grammar format (RBNF)

Each rule has the form `NonTerminal : RegularExpression .`

```
{ comment }

expr   : term , @*( '+' , term ; '-' , term ) .
term   : factor , @*( '*' , factor ; '/' , factor ) .
factor : '(' , expr , ')' ; ID ; NUM .

EOGram!
```

**Operators:**

| Operator | Meaning | Example |
|----------|---------|---------|
| `,` | Sequence | `'a' , 'b'` |
| `;` | Alternative | `'x' ; 'y'` |
| `@*( A )` | Zero or more | `@*( digit )` |
| `@+( A )` | One or more | `@+( letter )` |
| `A # B` | `A (B A)*` — list with separator | `item # ','` |
| `( A )` | Grouping | `( 'a' ; 'b' )` |
| `eps` | Empty word (epsilon) | `( rule ; eps )` |
| `$name` | Semantic action marker | `expr , $push` |
| `@name` | Macro reference | `@*( @item )` |

---

## Examples

The `examples/grammars/` directory contains ready-to-use grammars:

| File | Description |
|------|-------------|
| `CALC.GRM` | Calculator with semantic actions |
| `LANG.GRM` | A small programming language |
| `basic/hello.grm` | Minimal two-token grammar |
| `basic/identifier.grm` | Identifier: letter followed by letters/digits |
| `basic/list.grm` | Comma-separated list |
| `recursion/left_simple.grm` | Direct left recursion (for testing elimination) |
| `recursion/right_simple.grm` | Direct right recursion |

---

## Running tests

```bash
cmake -B build -DBUILD_GUI=OFF -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

25 test modules, 180+ individual tests covering the parser, transformations, recursion analysis, visualization, and integration scenarios.

---

## Project structure

```
vkr/
├── libsyngt/           # Core library
│   ├── include/syngt/
│   │   ├── core/       # Grammar, NTListItem
│   │   ├── parser/     # RBNF parser
│   │   ├── regex/      # RETree hierarchy + drawing
│   │   ├── transform/  # LeftElimination, RightElimination, Regularize, ...
│   │   └── analysis/   # RecursionAnalyzer, FirstFollow, ParsingTable
│   └── src/
├── syngt_cli/          # Console application
├── syngt_gui/          # GUI application (Windows / DirectX 11)
├── tests/              # Google Test modules
└── examples/           # Sample .grm grammar files
```

---

## License

MIT License. See [LICENSE](LICENSE).
