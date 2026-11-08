# Complete code walkthrough

This guide follows the actual implementation in the modular project. The
single-file equivalent is in `single-file/huffman.cpp`.

## 1. Byte frequencies

The compressor first reads the input as bytes. Each possible byte value is in
the range 0 to 255, so a 256-entry frequency table is sufficient.

```cpp
HuffmanTree::FrequencyTable frequencies{};
for (std::uint8_t byte : data) {
    ++frequencies[byte];
}
```

- `{}` initializes every frequency to zero.
- `byte` is the current byte from the input.
- `frequencies[byte]` selects that byte's counter.
- `++` increments the counter.

## 2. A Huffman node

```cpp
struct HuffmanNode {
    std::uint8_t byte;
    std::uint64_t frequency;
    std::shared_ptr<HuffmanNode> left;
    std::shared_ptr<HuffmanNode> right;
};
```

A leaf represents an original byte. An internal node joins two child nodes.
`shared_ptr` manages node memory automatically.

## 3. The min-priority queue

```cpp
auto first = minHeap.top();
minHeap.pop();

auto second = minHeap.top();
minHeap.pop();
```

The queue is configured so that the least-frequent node is at the top. The
algorithm removes the two smallest nodes and joins them under a new parent.
This is the greedy step in Huffman coding.

## 4. Building binary codes

```cpp
buildCodes(node->left, prefix + "0");
buildCodes(node->right, prefix + "1");
```

Moving left appends `0`; moving right appends `1`. When a leaf is reached, its
current prefix becomes that byte's Huffman code.

## 5. Packing bits

```cpp
buffer = static_cast<unsigned char>(
    (buffer << 1) | (bit == '1' ? 1 : 0)
);
```

The buffer is shifted left to make room for the next bit. The final expression
adds either a zero or one. After eight bits, one byte is written to the output.

## 6. Rebuilding the tree while decoding

The compressed file stores the original byte count and the frequency of each
byte. Decompression reads this table and builds the same deterministic tree.
It then follows each encoded bit: zero means left, one means right. Reaching a
leaf emits that byte and decoding returns to the root.

## 7. Why the original size is stored

The last packed byte may contain unused padding bits. The original byte count
tells the decoder exactly when to stop, so padding is not mistaken for data.

## 8. Input validation

The decoder checks the `HUF1` file marker, reads the header, checks that the
frequency total matches the original size, and detects truncated data.

## 9. Empty and one-symbol files

- Empty input: the header records an original size of zero; no tree traversal is
  needed during decompression.
- One distinct byte: the tree consists of one leaf. The decoder repeats that
  byte for the stored original size.

## 10. Try the code

Build the modular project from the repository root:

```bash
cmake -S . -B build
cmake --build build
```

Then compress and restore a sample:

```bash
./build/huffman compress examples/input.txt output.huff
./build/huffman decompress output.huff restored.txt
cmp examples/input.txt restored.txt
```

For the single-file edition, follow `single-file/README.md`.
