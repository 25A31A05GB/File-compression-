/*
 * Huffman File Compression - 
 * This single-file edition combines the project's modular implementation.
 */

#include <array>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <queue>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>


// 1. HUFFMAN NODE
// A leaf stores one byte and its frequency. Internal nodes join two children.


struct HuffmanNode {
    std::uint8_t byte;
    std::uint64_t frequency;
    int minByte;
    std::uint64_t order;
    std::shared_ptr<HuffmanNode> left;
    std::shared_ptr<HuffmanNode> right;

    HuffmanNode(std::uint8_t value, std::uint64_t count,
                std::uint64_t insertionOrder)
        : byte(value),
          frequency(count),
          minByte(value),
          order(insertionOrder),
          left(nullptr),
          right(nullptr) {}

    HuffmanNode(std::shared_ptr<HuffmanNode> leftChild,
                std::shared_ptr<HuffmanNode> rightChild,
                std::uint64_t insertionOrder)
        : byte(0),
          frequency(leftChild->frequency + rightChild->frequency),
          minByte(leftChild->minByte < rightChild->minByte
                      ? leftChild->minByte
                      : rightChild->minByte),
          order(insertionOrder),
          left(std::move(leftChild)),
          right(std::move(rightChild)) {}

    bool isLeaf() const {
        return !left && !right;
    }
};


// 2. HUFFMAN TREE
// A min-priority queue repeatedly combines the two least-frequent nodes.

class HuffmanTree {
public:
    using FrequencyTable = std::array<std::uint64_t, 256>;

    explicit HuffmanTree(const FrequencyTable& frequencies) {
        struct NodeCompare {
            bool operator()(const std::shared_ptr<HuffmanNode>& a,
                            const std::shared_ptr<HuffmanNode>& b) const {
                return std::tie(a->frequency, a->minByte, a->order) >
                       std::tie(b->frequency, b->minByte, b->order);
            }
        };

        std::priority_queue<
            std::shared_ptr<HuffmanNode>,
            std::vector<std::shared_ptr<HuffmanNode>>,
            NodeCompare
        > minHeap;

        std::uint64_t insertionOrder = 0;

        for (std::size_t value = 0; value < frequencies.size(); ++value) {
            if (frequencies[value] > 0) {
                minHeap.push(std::make_shared<HuffmanNode>(
                    static_cast<std::uint8_t>(value),
                    frequencies[value],
                    insertionOrder++
                ));
            }
        }

        while (minHeap.size() > 1) {
            auto first = minHeap.top();
            minHeap.pop();

            auto second = minHeap.top();
            minHeap.pop();

            minHeap.push(std::make_shared<HuffmanNode>(
                first, second, insertionOrder++
            ));
        }

        if (!minHeap.empty()) {
            root_ = minHeap.top();
            buildCodes(root_, "");
        }
    }

    const std::shared_ptr<HuffmanNode>& root() const {
        return root_;
    }

    const std::array<std::string, 256>& codes() const {
        return codes_;
    }

private:
    std::shared_ptr<HuffmanNode> root_;
    std::array<std::string, 256> codes_{};

    void buildCodes(const std::shared_ptr<HuffmanNode>& node,
                    const std::string& prefix) {
        if (!node) {
            return;
        }

        if (node->isLeaf()) {
            // A one-byte-value file still receives a one-bit code.
            codes_[node->byte] = prefix.empty() ? "0" : prefix;
            return;
        }

        buildCodes(node->left, prefix + "0");
        buildCodes(node->right, prefix + "1");
    }
};

// 3. FILE HANDLER
// Binary mode lets the program handle text, images, PDFs, and other byte data.


class FileHandler {
public:
    static std::vector<std::uint8_t> readAll(const std::string& path) {
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            throw std::runtime_error("Could not open input file: " + path);
        }

        std::vector<std::uint8_t> data;
        char value;
        while (input.get(value)) {
            data.push_back(
                static_cast<std::uint8_t>(static_cast<unsigned char>(value))
            );
        }

        if (input.bad()) {
            throw std::runtime_error("An error occurred while reading: " + path);
        }
        return data;
    }

    static void writeAll(const std::string& path,
                         const std::vector<std::uint8_t>& data) {
        std::ofstream output(path, std::ios::binary);
        if (!output) {
            throw std::runtime_error("Could not create output file: " + path);
        }

        if (!data.empty()) {
            output.write(reinterpret_cast<const char*>(data.data()),
                         static_cast<std::streamsize>(data.size()));
        }

        if (!output) {
            throw std::runtime_error("An error occurred while writing: " + path);
        }
    }

    static std::uint64_t fileSize(const std::string& path) {
        std::ifstream input(path, std::ios::binary | std::ios::ate);
        if (!input) {
            throw std::runtime_error(
                "Could not open file to measure its size: " + path
            );
        }

        const auto position = input.tellg();
        if (position < 0) {
            throw std::runtime_error("Could not determine file size: " + path);
        }
        return static_cast<std::uint64_t>(position);
    }
};

// 4. COMPRESSION STATISTICS


class Statistics {
public:
    static void printCompression(std::uint64_t originalBytes,
                                 std::uint64_t compressedBytes) {
        std::cout << "\n--- Compression results ---\n";
        std::cout << "Original size:   " << originalBytes << " bytes\n";
        std::cout << "Compressed size: " << compressedBytes << " bytes\n";

        if (originalBytes == 0) {
            std::cout << "Space saved:     Not applicable (empty file)\n";
            return;
        }

        const double saved =
            (1.0 - static_cast<double>(compressedBytes) /
                       static_cast<double>(originalBytes)) * 100.0;

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Space saved:     " << saved << "%\n";

        if (compressedBytes > originalBytes) {
            std::cout << "Note: The compressed file is larger. The header "
                         "adds overhead, especially for small files.\n";
        }
    }
};

// 5. COMPRESSOR
// The HUF1 header stores the original size and 256 frequencies. Encoded bits are packed into bytes, most-significant bit first.


class Compressor {
private:
    static void writeU64(std::ostream& output, std::uint64_t value) {
        for (int i = 0; i < 8; ++i) {
            output.put(static_cast<char>((value >> (i * 8)) & 0xFF));
        }
        if (!output) {
            throw std::runtime_error(
                "Could not write compressed file header."
            );
        }
    }

public:
    static void compress(const std::string& inputPath,
                         const std::string& outputPath) {
        if (inputPath == outputPath) {
            throw std::runtime_error(
                "Input and output paths must be different."
            );
        }

        const auto data = FileHandler::readAll(inputPath);

        HuffmanTree::FrequencyTable frequencies{};
        for (std::uint8_t byte : data) {
            ++frequencies[byte];
        }

        HuffmanTree tree(frequencies);

        std::ofstream output(outputPath, std::ios::binary);
        if (!output) {
            throw std::runtime_error(
                "Could not create compressed file: " + outputPath
            );
        }

        output.write("HUF1", 4);
        writeU64(output, static_cast<std::uint64_t>(data.size()));

        for (std::uint64_t frequency : frequencies) {
            writeU64(output, frequency);
        }

        unsigned char buffer = 0;
        int bitsInBuffer = 0;

        for (std::uint8_t byte : data) {
            const std::string& code = tree.codes()[byte];

            for (char bit : code) {
                buffer = static_cast<unsigned char>(
                    (buffer << 1) | (bit == '1' ? 1 : 0)
                );
                ++bitsInBuffer;

                if (bitsInBuffer == 8) {
                    output.put(static_cast<char>(buffer));
                    buffer = 0;
                    bitsInBuffer = 0;
                }
            }
        }

        if (bitsInBuffer > 0) {
            buffer = static_cast<unsigned char>(
                buffer << (8 - bitsInBuffer)
            );
            output.put(static_cast<char>(buffer));
        }

        if (!output) {
            throw std::runtime_error(
                "An error occurred while writing compressed data."
            );
        }
        output.close();

        Statistics::printCompression(
            static_cast<std::uint64_t>(data.size()),
            FileHandler::fileSize(outputPath)
        );
    }
};


// 6. DECOMPRESSOR
// It reads the header, rebuilds the same tree, follows each encoded bit, and writes the original bytes back to a file.

class Decompressor {
private:
    static std::uint64_t readU64(std::istream& input) {
        std::uint64_t value = 0;

        for (int i = 0; i < 8; ++i) {
            const int byte = input.get();
            if (byte == EOF) {
                throw std::runtime_error(
                    "Compressed file header is incomplete."
                );
            }

            value |= static_cast<std::uint64_t>(
                static_cast<unsigned char>(byte)
            ) << (i * 8);
        }
        return value;
    }

public:
    static void decompress(const std::string& inputPath,
                           const std::string& outputPath) {
        if (inputPath == outputPath) {
            throw std::runtime_error(
                "Input and output paths must be different."
            );
        }

        std::ifstream input(inputPath, std::ios::binary);
        if (!input) {
            throw std::runtime_error(
                "Could not open compressed file: " + inputPath
            );
        }

        char magic[4];
        input.read(magic, 4);
        if (!input || magic[0] != 'H' || magic[1] != 'U' ||
            magic[2] != 'F' || magic[3] != '1') {
            throw std::runtime_error(
                "This is not a supported HUF1 compressed file."
            );
        }

        const std::uint64_t originalSize = readU64(input);

        HuffmanTree::FrequencyTable frequencies{};
        std::uint64_t frequencyTotal = 0;

        for (std::size_t i = 0; i < frequencies.size(); ++i) {
            frequencies[i] = readU64(input);

            if (frequencies[i] >
                std::numeric_limits<std::uint64_t>::max() - frequencyTotal) {
                throw std::runtime_error("Invalid frequency table.");
            }
            frequencyTotal += frequencies[i];
        }

        if (frequencyTotal != originalSize) {
            throw std::runtime_error(
                "Compressed file has an invalid frequency table."
            );
        }

        HuffmanTree tree(frequencies);
        std::vector<std::uint8_t> restored;

        if (originalSize >
            static_cast<std::uint64_t>(restored.max_size())) {
            throw std::runtime_error(
                "The original file is too large for this system."
            );
        }

        restored.reserve(static_cast<std::size_t>(originalSize));

        if (originalSize > 0) {
            auto current = tree.root();

            if (!current) {
                throw std::runtime_error(
                    "Compressed file does not contain a valid tree."
                );
            }

            // If the source had only one distinct byte, the tree is one leaf.
            if (current->isLeaf()) {
                restored.assign(
                    static_cast<std::size_t>(originalSize),
                    current->byte
                );
            } else {
                while (restored.size() < originalSize) {
                    const int packed = input.get();
                    if (packed == EOF) {
                        throw std::runtime_error(
                            "Compressed data ended before decoding was complete."
                        );
                    }

                    const unsigned char byte =
                        static_cast<unsigned char>(packed);

                    for (int bitIndex = 7;
                         bitIndex >= 0 && restored.size() < originalSize;
                         --bitIndex) {
                        const bool bit = ((byte >> bitIndex) & 1U) != 0;
                        current = bit ? current->right : current->left;

                        if (!current) {
                            throw std::runtime_error(
                                "Compressed data contains an invalid bit sequence."
                            );
                        }

                        if (current->isLeaf()) {
                            restored.push_back(current->byte);
                            current = tree.root();
                        }
                    }
                }
            }
        }

        FileHandler::writeAll(outputPath, restored);
        std::cout << "Decompression completed.\n";
        std::cout << "Restored size: " << restored.size() << " bytes\n";
    }
};


void printUsage(const char* program) {
    std::cout << "Huffman File Compression (C++17) - Single File Edition\n\n"
              << "Usage:\n"
              << "  " << program << " compress <input-file> <output.huff>\n"
              << "  " << program << " decompress <input.huff> <output-file>\n\n"
              << "Examples:\n"
              << "  " << program
              << " compress examples/input.txt output.huff\n"
              << "  " << program
              << " decompress output.huff restored.txt\n";
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printUsage(argv[0]);
        return 1;
    }

    const std::string command = argv[1];

    try {
        if (command == "compress") {
            Compressor::compress(argv[2], argv[3]);
        } else if (command == "decompress") {
            Decompressor::decompress(argv[2], argv[3]);
        } else {
            printUsage(argv[0]);
            return 1;
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
