/*
 * Huffman File Compression and Decompression
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <queue>
#include <string>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <iomanip>
#include <sstream>

struct HuffmanNode {
    unsigned char symbol;
    std::uint64_t frequency;
    int minSymbol;
    std::uint64_t order;
    HuffmanNode* left;
    HuffmanNode* right;

    HuffmanNode(unsigned char ch, std::uint64_t count, std::uint64_t seq)
        : symbol(ch), frequency(count), minSymbol(ch), order(seq),
          left(NULL), right(NULL) {}

    HuffmanNode(HuffmanNode* a, HuffmanNode* b, std::uint64_t seq)
        : symbol(0), frequency(a->frequency + b->frequency),
          minSymbol(a->minSymbol < b->minSymbol ? a->minSymbol : b->minSymbol),
          order(seq), left(a), right(b) {}

    bool isLeaf() const { return left == NULL && right == NULL; }
};

struct NodeCompare {
    bool operator()(const HuffmanNode* a, const HuffmanNode* b) const {
        if (a->frequency != b->frequency)
            return a->frequency > b->frequency;
        if (a->minSymbol != b->minSymbol)
            return a->minSymbol > b->minSymbol;
        return a->order > b->order;
    }
};

class HuffmanTree {
private:
    std::vector<HuffmanNode*> nodes;
    HuffmanNode* root;

    HuffmanNode* makeLeaf(unsigned char symbol, std::uint64_t frequency,
                          std::uint64_t order) {
        HuffmanNode* node = new HuffmanNode(symbol, frequency, order);
        nodes.push_back(node);
        return node;
    }

    HuffmanNode* makeParent(HuffmanNode* left, HuffmanNode* right,
                            std::uint64_t order) {
        HuffmanNode* node = new HuffmanNode(left, right, order);
        nodes.push_back(node);
        return node;
    }

    void buildCodes(HuffmanNode* node, const std::string& prefix,
                    std::array<std::string, 256>& codes) const {
        if (node == NULL) return;
        if (node->isLeaf()) {
            codes[node->symbol] = prefix.empty() ? "0" : prefix;
            return;
        }
        buildCodes(node->left, prefix + "0", codes);
        buildCodes(node->right, prefix + "1", codes);
    }

public:
    HuffmanTree() : root(NULL) {}

    ~HuffmanTree() {
        clear();
    }

    void clear() {
        for (std::size_t i = 0; i < nodes.size(); ++i)
            delete nodes[i];
        nodes.clear();
        root = NULL;
    }

    HuffmanNode* build(const std::array<std::uint64_t, 256>& frequencies) {
        clear();
        std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>,
                            NodeCompare> heap;
        std::uint64_t order = 0;

        for (int i = 0; i < 256; ++i) {
            if (frequencies[i] != 0)
                heap.push(makeLeaf(static_cast<unsigned char>(i),
                                   frequencies[i], order++));
        }

        while (heap.size() > 1) {
            HuffmanNode* first = heap.top(); heap.pop();
            HuffmanNode* second = heap.top(); heap.pop();
            if (first->frequency >
                std::numeric_limits<std::uint64_t>::max() - second->frequency)
                throw std::runtime_error("Frequency total overflow.");
            heap.push(makeParent(first, second, order++));
        }

        root = heap.empty() ? NULL : heap.top();
        return root;
    }

    void getCodes(std::array<std::string, 256>& codes) const {
        codes.fill("");
        buildCodes(root, "", codes);
    }

    HuffmanNode* getRoot() const { return root; }
};

static void writeU16(std::ostream& out, std::uint16_t value) {
    out.put(static_cast<char>(value & 0xFF));
    out.put(static_cast<char>((value >> 8) & 0xFF));
    if (!out) throw std::runtime_error("Failed writing compressed header.");
}

static void writeU64(std::ostream& out, std::uint64_t value) {
    for (int i = 0; i < 8; ++i)
        out.put(static_cast<char>((value >> (8 * i)) & 0xFF));
    if (!out) throw std::runtime_error("Failed writing compressed header.");
}

static std::uint16_t readU16(std::istream& in) {
    int a = in.get(), b = in.get();
    if (a == EOF || b == EOF)
        throw std::runtime_error("Compressed header is incomplete.");
    return static_cast<std::uint16_t>(
        static_cast<unsigned int>(static_cast<unsigned char>(a)) |
        (static_cast<unsigned int>(static_cast<unsigned char>(b)) << 8));
}

static std::uint64_t readU64(std::istream& in) {
    std::uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        int ch = in.get();
        if (ch == EOF)
            throw std::runtime_error("Compressed header is incomplete.");
        value |= static_cast<std::uint64_t>(
            static_cast<unsigned char>(ch)) << (8 * i);
    }
    return value;
}

static std::uint64_t getFileSize(const std::string& path) {
    std::ifstream in(path.c_str(), std::ios::binary | std::ios::ate);
    if (!in) throw std::runtime_error("Cannot open file: " + path);
    std::streamoff size = in.tellg();
    if (size < 0) throw std::runtime_error("Cannot determine file size.");
    return static_cast<std::uint64_t>(size);
}

static std::vector<unsigned char> readFile(const std::string& path) {
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open input file: " + path);

    std::vector<unsigned char> data;
    char ch;
    while (in.get(ch))
        data.push_back(static_cast<unsigned char>(ch));

    if (in.bad()) throw std::runtime_error("Error while reading input file.");
    return data;
}

static void writeFile(const std::string& path,
                      const std::vector<unsigned char>& data) {
    std::ofstream out(path.c_str(), std::ios::binary);
    if (!out) throw std::runtime_error("Cannot create output file: " + path);

    if (!data.empty())
        out.write(reinterpret_cast<const char*>(&data[0]),
                  static_cast<std::streamsize>(data.size()));

    if (!out) throw std::runtime_error("Error while writing output file.");
}

static void ensureDifferentPaths(const std::string& input,
                                 const std::string& output) {
    if (input == output)
        throw std::runtime_error("Input and output paths must be different.");
}

static void printStats(std::uint64_t original, std::uint64_t compressed) {
    std::cout << "\n--- Compression statistics ---\n";
    std::cout << "Original size:   " << original << " bytes\n";
    std::cout << "Compressed size: " << compressed << " bytes\n";

    if (original == 0) {
        std::cout << "Space saved:     Not applicable (empty file)\n";
        return;
    }

    double saved = (1.0 - static_cast<double>(compressed) /
                    static_cast<double>(original)) * 100.0;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Space saved:     " << saved << "%\n";
    if (compressed >= original)
        std::cout << "Note: This file did not become smaller; the header "
                     "and coding overhead can exceed the savings.\n";
}

static void compressFile(const std::string& inputPath,
                         const std::string& outputPath) {
    ensureDifferentPaths(inputPath, outputPath);
    std::vector<unsigned char> data = readFile(inputPath);

    std::array<std::uint64_t, 256> frequencies;
    frequencies.fill(0);
    for (std::size_t i = 0; i < data.size(); ++i) {
        if (frequencies[data[i]] == std::numeric_limits<std::uint64_t>::max())
            throw std::runtime_error("Input file is too large.");
        ++frequencies[data[i]];
    }

    HuffmanTree tree;
    tree.build(frequencies);
    std::array<std::string, 256> codes;
    tree.getCodes(codes);

    std::ofstream out(outputPath.c_str(), std::ios::binary);
    if (!out) throw std::runtime_error("Cannot create compressed file: " + outputPath);

    out.write("HUF2", 4);
    writeU64(out, static_cast<std::uint64_t>(data.size()));

    std::uint16_t symbolCount = 0;
    for (int i = 0; i < 256; ++i)
        if (frequencies[i] != 0) ++symbolCount;
    writeU16(out, symbolCount);

    // Store only symbols that occur: one symbol byte + one 64-bit frequency.
    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] != 0) {
            out.put(static_cast<char>(i));
            writeU64(out, frequencies[i]);
        }
    }

    unsigned char buffer = 0;
    int bitsInBuffer = 0;

    for (std::size_t i = 0; i < data.size(); ++i) {
        const std::string& code = codes[data[i]];
        for (std::size_t j = 0; j < code.size(); ++j) {
            buffer = static_cast<unsigned char>(
                (buffer << 1) | (code[j] == '1' ? 1 : 0));
            ++bitsInBuffer;
            if (bitsInBuffer == 8) {
                out.put(static_cast<char>(buffer));
                buffer = 0;
                bitsInBuffer = 0;
            }
        }
    }

    if (bitsInBuffer != 0)
        out.put(static_cast<char>(buffer << (8 - bitsInBuffer)));

    if (!out) throw std::runtime_error("Error writing compressed data.");
    out.close();

    printStats(static_cast<std::uint64_t>(data.size()),
               getFileSize(outputPath));
    std::cout << "Output file: " << outputPath << "\n";
}

static void decompressFile(const std::string& inputPath,
                           const std::string& outputPath) {
    ensureDifferentPaths(inputPath, outputPath);
    std::ifstream in(inputPath.c_str(), std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open compressed file: " + inputPath);

    char magic[4];
    in.read(magic, 4);
    if (!in || magic[0] != 'H' || magic[1] != 'U' ||
        magic[2] != 'F' || magic[3] != '2')
        throw std::runtime_error("Invalid file format. Expected HUF2.");

    std::uint64_t originalSize = readU64(in);
    std::uint16_t symbolCount = readU16(in);
    if (symbolCount > 256)
        throw std::runtime_error("Invalid symbol count in compressed header.");

    std::array<std::uint64_t, 256> frequencies;
    frequencies.fill(0);
    std::uint64_t total = 0;

    for (std::uint16_t i = 0; i < symbolCount; ++i) {
        int symbol = in.get();
        if (symbol == EOF)
            throw std::runtime_error("Compressed frequency table is incomplete.");
        unsigned char ch = static_cast<unsigned char>(symbol);
        if (frequencies[ch] != 0)
            throw std::runtime_error("Duplicate symbol in compressed header.");

        std::uint64_t frequency = readU64(in);
        if (frequency == 0 ||
            frequency > std::numeric_limits<std::uint64_t>::max() - total)
            throw std::runtime_error("Invalid frequency table.");
        frequencies[ch] = frequency;
        total += frequency;
    }

    if (total != originalSize)
        throw std::runtime_error("Header frequency total does not match original size.");
    if ((originalSize == 0) != (symbolCount == 0))
        throw std::runtime_error("Invalid empty-file header.");

    HuffmanTree tree;
    HuffmanNode* root = tree.build(frequencies);
    std::vector<unsigned char> restored;

    if (originalSize > static_cast<std::uint64_t>(restored.max_size()))
        throw std::runtime_error("Original file is too large for this system.");
    restored.reserve(static_cast<std::size_t>(originalSize));

    if (originalSize > 0 && root->isLeaf()) {
        restored.assign(static_cast<std::size_t>(originalSize), root->symbol);
    } else if (originalSize > 0) {
        HuffmanNode* current = root;
        while (restored.size() < originalSize) {
            int packed = in.get();
            if (packed == EOF)
                throw std::runtime_error("Compressed data ended before decoding completed.");

            unsigned char byte = static_cast<unsigned char>(packed);
            for (int bit = 7; bit >= 0 && restored.size() < originalSize; --bit) {
                bool value = ((byte >> bit) & 1U) != 0;
                current = value ? current->right : current->left;
                if (current == NULL)
                    throw std::runtime_error("Invalid encoded bit sequence.");
                if (current->isLeaf()) {
                    restored.push_back(current->symbol);
                    current = root;
                }
            }
        }
    }

    writeFile(outputPath, restored);
    std::cout << "\nDecompression completed successfully.\n";
    std::cout << "Restored size: " << restored.size() << " bytes\n";
    std::cout << "Output file: " << outputPath << "\n";
}

static void printUsage(const char* program) {
    std::cout << "\nHUFFMAN FILE COMPRESSION (C++11)\n"
              << "Usage:\n"
              << "  " << program << " compress <input-file> <output.huff>\n"
              << "  " << program << " decompress <input.huff> <output-file>\n"
              << "Run without arguments for the interactive menu.\n\n";
}

static void interactiveMenu() {
    while (true) {
        std::cout << "\n====================================\n"
                  << "       HUFFMAN FILE COMPRESSION\n"
                  << "====================================\n"
                  << "1. Compress a file\n"
                  << "2. Decompress a file\n"
                  << "3. Exit\n"
                  << "Choose an option: ";

        std::string choice;
        if (!std::getline(std::cin, choice)) return;

        try {
            if (choice == "1") {
                std::string input, output;
                std::cout << "Enter input file path: ";
                std::getline(std::cin, input);
                std::cout << "Enter output compressed file path: ";
                std::getline(std::cin, output);
                compressFile(input, output);
            } else if (choice == "2") {
                std::string input, output;
                std::cout << "Enter compressed file path: ";
                std::getline(std::cin, input);
                std::cout << "Enter restored output file path: ";
                std::getline(std::cin, output);
                decompressFile(input, output);
            } else if (choice == "3") {
                std::cout << "Exiting program.\n";
                return;
            } else {
                std::cout << "Invalid choice. Please select 1, 2, or 3.\n";
            }
        } catch (const std::exception& error) {
            std::cerr << "Error: " << error.what() << "\n";
        }
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc == 1) {
            interactiveMenu();
            return 0;
        }

        if (argc != 4) {
            printUsage(argv[0]);
            return 1;
        }

        std::string command = argv[1];
        if (command == "compress") {
            compressFile(argv[2], argv[3]);
        } else if (command == "decompress") {
            decompressFile(argv[2], argv[3]);
        } else {
            printUsage(argv[0]);
            return 1;
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }

    return 0;
}
