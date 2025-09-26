#!/bin/bash

# ============================================================
#  SynGT C++ - Run Script for Linux/Mac/MSYS2
# ============================================================

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXE_PATH="${PROJECT_ROOT}/build/bin/syngt_cli"

if [ ! -f "${EXE_PATH}" ]; then
    echo "[X] Executable not found: ${EXE_PATH}"
    echo "[*] Please build the project first: ./scripts/build.sh"
    exit 1
fi

if [ $# -eq 0 ]; then
    "${EXE_PATH}" --help
else
    "${EXE_PATH}" "$@"
fi