# Huffman File Compression (C++17)

A lossless file-compression learning project using Huffman Coding. The project
contains **two equivalent ways to study and run the implementation**:

1. **Modular version (repository root):** separate `.cpp` implementation files
   and `.h` headers, organized by responsibility.
2. **Single-file version (`single-file/huffman.cpp`):** the complete program
   combined into one C++ source file for easier reading, compiling, and sharing.

The original modular source files from the supplied project are preserved.
The single-file edition combines their behavior in one standalone source file.

## Features

- Counts byte frequencies (all 256 possible byte values).
- Builds a Huffman tree using a min-priority queue.
- Assigns shorter codes to more frequent bytes.
- Packs encoded bits into bytes.
- Stores a `HUF1` header and frequency table.
- Rebuilds the tree to decompress the file.
- Reports original size, compressed size, and space saved.
- Reads and writes in binary mode, so it can process text and binary files.

## Requirements

- C++17 compiler
- CMake 3.16+ for the modular version
- Or a direct C++ compiler command for the single-file version

## Option A: Build the modular version

Run these commands from the repository root:

```bash
cmake -S . -B build
cmake --build build
```

Compress:

```bash
./build/huffman compress examples/input.txt output.huff
```

Decompress:

```bash
./build/huffman decompress output.huff restored.txt
```

On Windows, the executable may be in `build/Debug` or `build/Release`:

```powershell
.\build\Debug\huffman.exe compress examples\input.txt output.huff
.\build\Debug\huffman.exe decompress output.huff restored.txt
```

## Option B: Build the single-file version

Change to the single-file folder:

```bash
cd single-file
g++ -std=c++17 -O2 -Wall -Wextra -pedantic huffman.cpp -o huffman
```

Compress and decompress the sample:

```bash
./huffman compress ../examples/input.txt compressed.huff
./huffman decompress compressed.huff restored.txt
```

On Windows with MinGW:

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -pedantic huffman.cpp -o huffman.exe
.\huffman.exe compress ..\examples\input.txt compressed.huff
.\huffman.exe decompress compressed.huff restored.txt
```

## Verify lossless restoration

From the repository root, for the modular version:

```bash
cmp examples/input.txt restored.txt
```

For the single-file version, compare `examples/input.txt` with
`single-file/restored.txt` if you used that output path, or run `cmp` from
inside `single-file`:

```bash
cmp ../examples/input.txt restored.txt
```

No output from `cmp` means the files match.

## Repository layout

```text
.
├── include/                 # Header declarations and HuffmanNode
├── src/                     # Modular C++ implementation
├── single-file/
│   ├── huffman.cpp           # Complete combined implementation
│   └── README.md             # Single-file build/run guide
├── docs/                     # Algorithm and beginner documentation
├── examples/                 # Sample input files
├── tests/                    # Round-trip test
├── output/                   # Place for generated files
├── CMakeLists.txt
├── LICENSE
└── README.md
```

## Documentation

- `docs/01_huffman_coding.md` through `docs/11_build_and_run.md` — topic notes
  supplied with the project.
- `docs/CODE_WALKTHROUGH.md` — explanations of the important code sections.
- `single-file/README.md` — exact single-file compilation and run instructions.

## How the algorithm works

1. Read the input bytes and count each byte's frequency.
2. Insert each used byte into a min-priority queue.
3. Repeatedly remove the two least-frequent nodes and join them.
4. Traverse the final tree to assign `0` and `1` codes.
5. Encode the input and pack the bits into output bytes.
6. During decompression, rebuild the tree from the stored frequency table.
7. Decode until the recorded original byte count is restored.

## Notes and limitations

- This is a project-specific `HUF1` format; it is not a ZIP-compatible format.
- The header stores a 64-bit original size and 256 64-bit frequency values.
  Header overhead means very small files can become larger after compression.
- The program loads file data into memory, so available memory limits the
  practical input size.
- Keep generated `build/` files out of Git; they are created by CMake.

## License

MIT. See `LICENSE`.
