# Project Dependencies

To build and run `lain-day-c`, you need to set up your development environment with the following tools and libraries.

## Core Build Tools

*   **C Compiler:** GCC or Clang with C11 support.
*   **CMake:** Version 3.10 or higher.
*   **Make:** GNU Make (or Ninja).

## Python Environment

The project uses Python 3 scripts for generating code (SSL to C, String IDs).

*   **Python:** Version 3.6 or higher.
*   **Python Libraries:**
    *   `PyYAML`: Required for parsing Scene Scripting Language (`.ssl`) files.

## C Libraries

*   **zlib:** Compression library.
    *   *Note:* `cJSON` and `linenoise` are included in the `external/` directory and do not need to be installed separately.

---

## Installation Guide

### 1. Ubuntu / Debian / WSL

```bash
# Install Build Tools and C Libraries
sudo apt update
sudo apt install build-essential cmake zlib1g-dev

# Install Python and PyYAML
sudo apt install python3 python3-pip python3-yaml
# Alternatively, via pip:
# pip3 install PyYAML
```

### 2. macOS (via Homebrew)

```bash
# Install Build Tools
xcode-select --install  # Installs Clang and Make

# Install CMake and Python
brew install cmake python

# Install PyYAML
pip3 install PyYAML
```

### 3. Android (Termux)

```bash
# Install Build Tools and Libraries
pkg install clang cmake make zlib

# Install Python and PyYAML
pkg install python
pip install PyYAML
```

### 4. Windows (MinGW/MSYS2)

Recommended using MSYS2:

```bash
# Install toolchain
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make

# Install Python and pip
pacman -S mingw-w64-x86_64-python mingw-w64-x86_64-python-pip

# Install PyYAML
pip install PyYAML
```

## Verifying Installation

To check if you have `PyYAML` installed correctly, run:

```bash
python3 -c "import yaml; print('PyYAML is installed')"
```
