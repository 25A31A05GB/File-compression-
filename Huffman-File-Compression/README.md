# Huffman File Compression

This is my ADSAA project. I am using C++ to make a small file compression program with Huffman Coding.

The program can compress a file and decompress it again. Huffman Coding is a lossless method, so the restored file should have the same data as the original file.

## What the program does

- Counts how many times each byte appears in a file.
- Builds a Huffman Tree from those counts.
- Gives a binary code to each byte.
- Saves the encoded data in a `.huff` file.
- Reads the `.huff` file and restores the original file.
- Shows the original and compressed file sizes.

It can work with text files and binary files such as images and PDFs.

## What is Huffman Coding?

Huffman Coding is a compression method. It gives shorter codes to values that appear more often and longer codes to values that appear less often.

It uses the Greedy Method. At each step, it joins the two nodes that have the smallest frequencies.

## Requirements

- C++ compiler with C++17 support
- CMake 3.16 or newer

## Build the project

Open a terminal in this folder.

```bash
cmake -S . -B build
cmake --build build
```

## Run the program

To compress a file:

```bash
huffman compress examples/input.txt output.huff
```

To get the original file back:

```bash
huffman decompress output.huff restored.txt
```

On Windows, the program may be inside `build/Release`. For example:

```powershell
.\build\Release\huffman.exe compress examples\input.txt output.huff
.\build\Release\huffman.exe decompress output.huff restored.txt
```

## Check the result

Compare the original file and the restored file. They should be the same.

On Windows PowerShell:

```powershell
Get-FileHash examples\input.txt
Get-FileHash restored.txt
```

On Linux or macOS:

```bash
cmp examples/input.txt restored.txt
```

## Folder details

- `include` has the header files.
- `src` has the C++ program files.
- `examples` has sample input files.
- `tests` has a small test script.
- `docs` has notes about the algorithm.
- `output` is for files created while trying the program.

## Main steps of Huffman Coding

1. Count the frequency of each byte.
2. Put the bytes into a min-priority queue.
3. Take out the two nodes with the smallest frequencies.
4. Join them under a new node.
5. Put the new node back into the queue.
6. Repeat until one node is left.
7. Use left for `0` and right for `1` to make the codes.

## Complexity

Here, `N` is the number of bytes in the file and `K` is the number of different byte values.

| Operation | Time |
|---|---|
| Count frequencies | O(N) |
| Build the tree | O(K log K) |
| Make the codes | O(K) |
| Encode the file | O(N + number of encoded bits) |
| Decode the file | O(number of encoded bits) |

There can be at most 256 different byte values.

## Note

The compressed file stores a frequency table so that the tree can be built again during decompression. This table takes space. Because of that, a small file may become bigger after compression.

This is a learning project and the `.huff` format is made for this project. It is not the same as ZIP.

## License

MIT License. See the `LICENSE` file.
