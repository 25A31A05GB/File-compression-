# Single-file Huffman implementation

This folder contains the complete Huffman compression and decompression program
combined into one C++17 source file: `huffman.cpp`.

It is an alternative to the modular version in the repository root. The modular
source files remain separate and are not replaced by this version.

## Requirements

- A C++17 compiler (GCC, Clang, or MSVC)
- No third-party C++ libraries are required

## Compile

### Linux, macOS, or GitHub Codespaces

From this folder:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic huffman.cpp -o huffman
```

## Run

Compress a file:

```bash
./huffman compress ../examples/input.txt compressed.huff
```

Decompress it:

```bash
./huffman decompress compressed.huff restored.txt
```

Compare the original and restored files:

```bash
cmp ../examples/input.txt restored.txt
```

If `cmp` prints nothing and exits successfully, the files are identical.

### Windows with MinGW

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -pedantic huffman.cpp -o huffman.exe
.\huffman.exe compress ..\examples\input.txt compressed.huff
.\huffman.exe decompress compressed.huff restored.txt
```

## Command format

```text
huffman compress <input-file> <output.huff>
huffman decompress <input.huff> <output-file>
```

The output file extension is a convention; the program uses its `HUF1` header
to identify this project's compressed format.

## Important notes

- Compression is lossless: decompression should reproduce the original bytes.
- The implementation reads and writes files in binary mode.
- Very small files may become larger because the compressed file stores a
  header and a 256-entry frequency table.
- This is a learning project format, not a ZIP-compatible archive.
