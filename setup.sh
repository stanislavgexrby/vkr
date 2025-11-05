#!/bin/bash

echo "========================================"
echo "  SynGT GUI Setup Script"
echo "========================================"
echo

cd "$(dirname "$0")"

if ! command -v git &> /dev/null; then
    echo "ERROR: Git not found! Please install Git first."
    echo "On Ubuntu/Debian: sudo apt install git"
    echo "On Fedora/RHEL: sudo dnf install git"
    read -p "Press Enter to continue..."
    exit 1
fi

if [ ! -d "syngt_gui" ]; then
    mkdir -p syngt_gui
fi
cd syngt_gui

echo "Downloading Dear ImGui..."
git clone --depth 1 https://github.com/ocornut/imgui.git
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to clone ImGui repository"
    read -p "Press Enter to continue..."
    exit 1
fi
cd ..

echo
echo "========================================"
echo "   Setup Complete!"
echo "========================================"
echo
echo "Next steps:"
echo "1. Run: cmake -B build -G Ninja -DBUILD_GUI=ON"
echo "2. Run: cd build"
echo "3. Run: ninja"
echo
echo "Wait for the compilation and then the project will be in build/bin/syngt_gui"
read -p "Press Enter to continue..."