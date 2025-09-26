## Requirements

- **C++ Compiler**: GCC 9+ or Clang 10+ with C++17 support
- **CMake**: 3.20 or higher
- **Ninja** (optional): For faster builds
- **Google Test**: Auto-downloaded by CMake

### Platform-specific

**Windows(MSYS2/MinGW):**
```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja

**Linux:**
```bach
sudo apt install build-essential cmake ninja-build

# Clone repository
```bash
git clone <repository-url>
cd syngt-cpp```

# Create build directory
```bash
mkdir build && cd build```

# Configure
```bash
cmake .. -G Ninja```

# Build
```bash
ninja```

# Usage
```bash
# From build directory
./bin/syngt_cli path/to/grammar.grm ```

# Run tests
```bash
# From build directory
ctest --output-on-failure

# Or run specific test
./tests/test_Grammar
./tests/test_Parser
```
# Run all tests
```bash
cd build
ctest
```
