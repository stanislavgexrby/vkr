## Requirements

- **C++ Compiler**: GCC 9+ or Clang 10+ with C++17 support
- **CMake**: 3.20 or higher
- **Ninja** (optional): For faster builds
- **Google Test**: Auto-downloaded by CMake

### Platform-specific

**Windows(MSYS2/MinGW):**
```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```

# Clone repository
```bash
git clone https://github.com/stanislavgexrby/vkr.git
cd vkr
```

# Install ImGUI
```bash
./setup.bat
```

# Build and run
```bash
./run.bat
```

# Usage
In GUI press help

---

### Build with CMake

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release

# Запустить тесты
ctest -C Release --output-on-failure
```

---
