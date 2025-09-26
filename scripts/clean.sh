#!/bin/bash
echo "Cleaning build directory..."
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
rm -rf "${PROJECT_ROOT}/build"
echo "Cleaned successfully"