# SynGT C++ - Syntax Grammar Transformation Tool

## Requirements

- **C++ Compiler**: GCC 9+ or Clang 10+ with C++17 support
- **CMake**: 3.20 or higher
- **Ninja** (optional): For faster builds
- **Google Test**: Auto-downloaded by CMake

### Platform-specific

**Windows (MSYS2/MinGW):**
```bash
pacman -S mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-cmake \
          mingw-w64-ucrt-x86_64-ninja
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt install build-essential cmake ninja-build \
                 libglfw3-dev libgl1-mesa-dev git
```

**Linux (Fedora/RHEL):**
```bash
sudo dnf install gcc-c++ cmake ninja-build \
                 glfw-devel mesa-libGL-devel git
```

**macOS:**
```bash
brew install cmake ninja glfw
```

# Clone repository
```bash
git clone https://github.com/stanislavgexrby/vkr.git
cd vkr
```

# Install ImGUI

**Windows (MSYS2/MinGW):**
```bash
./setup.bat
```

**Linux (Ubuntu/Debian):**
```bash
chmod +x setup.sh
./setup.sh
```

# Build

**Wiith GUI:**
```bash
cmake -B build -G Ninja -DBUILD_GUI=ON
cd build
ninja
```

**Only library**
```bash
cmake -B build -G Ninja
cd build
ninja
```

**Tests**
```bash
cmake -B build -G Ninja -DBUILD_TESTS=ON
cd build
ninja
ctest --output-on-failure
```

### Run

**Windows:**
```bash
build\bin\syngt_gui.exe
```

**Linux/macOS:**
```bash
./build/bin/syngt_gui
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

ctest -C Release --output-on-failure
```

---
