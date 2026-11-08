#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <queue>
#include <iomanip>
#include <stdexcept>
#include <cstdint>

struct Node {
    unsigned char symbol;
    std::uint64_t frequency;
    int minSymbol;
    std::uint64_t order;
    Node* left;
    Node* right;

    Node(unsigned char ch, std::uint64_t count, std::uint64_t seq)
        : symbol(ch), frequency(count), minSymbol(ch), order(seq),
          left(NULL), right(NULL) {}

    Node(Node* a, Node* b, std::uint64_t seq)
        : symbol(0), frequency(a->frequency + b->frequency),
          minSymbol(a->minSymbol < b->minSymbol ? a->minSymbol : b->minSymbol),
          order(seq), left(a), right(b) {}

    bool isLeaf() const { return left == NULL && right == NULL; }
};

struct CompareNodes {
    bool operator()(const Node* a, const Node* b) const {
        if (a->frequency != b->frequency)
            return a->frequency > b->frequency;
        if (a->minSymbol != b->minSymbol)
            return a->minSymbol > b->minSymbol;
        return a->order > b->order;
    }
};

struct EncodedData {
    std::array<std::uint64_t, 256> frequencies;
    std::vector<unsigned char> bytes;
    std::size_t bitCount;
    std::size_t originalSize;

    EncodedData() : bitCount(0), originalSize(0) {
        frequencies.fill(0);
    }
};

class HuffmanCompressor {
private:
    std::vector<Node*> allocated;

    Node* makeNode(unsigned char ch, std::uint64_t count, std::uint64_t order) {
        Node* n = new Node(ch, count, order);
        allocated.push_back(n);
        return n;
    }

    Node* makeParent(Node* a, Node* b, std::uint64_t order) {
        Node* n = new Node(a, b, order);
        allocated.push_back(n);
        return n;
    }

    Node* buildTree(const std::array<std::uint64_t, 256>& frequencies) {
        std::priority_queue<Node*, std::vector<Node*>, CompareNodes> heap;
        std::uint64_t order = 0;

        for (int i = 0; i < 256; ++i) {
            if (frequencies[i] != 0) {
                heap.push(makeNode(static_cast<unsigned char>(i),
                                   frequencies[i], order++));
            }
        }

        while (heap.size() > 1) {
            Node* first = heap.top();
            heap.pop();
            Node* second = heap.top();
            heap.pop();
            heap.push(makeParent(first, second, order++));
        }

        return heap.empty() ? NULL : heap.top();
    }

    void makeCodes(Node* node, const std::string& prefix,
                   std::array<std::string, 256>& codes) {
        if (node == NULL) return;

        if (node->isLeaf()) {
            codes[node->symbol] = prefix.empty() ? "0" : prefix;
            return;
        }

        makeCodes(node->left, prefix + "0", codes);
        makeCodes(node->right, prefix + "1", codes);
    }

    void releaseTree() {
        for (std::size_t i = 0; i < allocated.size(); ++i)
            delete allocated[i];
        allocated.clear();
    }

public:
    ~HuffmanCompressor() { releaseTree(); }

    EncodedData compress(const std::string& input,
                         std::array<std::string, 256>& codes) {
        releaseTree();

        EncodedData result;
        result.originalSize = input.size();

        for (std::size_t i = 0; i < input.size(); ++i) {
            unsigned char ch = static_cast<unsigned char>(input[i]);
            ++result.frequencies[ch];
        }

        Node* root = buildTree(result.frequencies);
        codes.fill("");

        if (root == NULL) return result;

        makeCodes(root, "", codes);

        unsigned char currentByte = 0;
        int bitsInCurrentByte = 0;

        for (std::size_t i = 0; i < input.size(); ++i) {
            unsigned char ch = static_cast<unsigned char>(input[i]);
            const std::string& code = codes[ch];

            for (std::size_t j = 0; j < code.size(); ++j) {
                currentByte = static_cast<unsigned char>(
                    (currentByte << 1) | (code[j] == '1' ? 1 : 0));
                ++bitsInCurrentByte;
                ++result.bitCount;

                if (bitsInCurrentByte == 8) {
                    result.bytes.push_back(currentByte);
                    currentByte = 0;
                    bitsInCurrentByte = 0;
                }
            }
        }

        if (bitsInCurrentByte != 0) {
            currentByte = static_cast<unsigned char>(
                currentByte << (8 - bitsInCurrentByte));
            result.bytes.push_back(currentByte);
        }

        return result;
    }

    std::string decompress(const EncodedData& data) {
        releaseTree();
        Node* root = buildTree(data.frequencies);

        if (data.originalSize == 0) return "";
        if (root == NULL) throw std::runtime_error("Invalid encoded data.");

        if (root->isLeaf()) {
            return std::string(data.originalSize, static_cast<char>(root->symbol));
        }

        std::string output;
        output.reserve(data.originalSize);
        Node* current = root;

        for (std::size_t bit = 0;
             bit < data.bitCount && output.size() < data.originalSize; ++bit) {
            const std::size_t byteIndex = bit / 8;
            const int bitIndex = 7 - static_cast<int>(bit % 8);
            bool value = ((data.bytes[byteIndex] >> bitIndex) & 1U) != 0;

            current = value ? current->right : current->left;
            if (current == NULL)
                throw std::runtime_error("Invalid bit sequence.");

            if (current->isLeaf()) {
                output.push_back(static_cast<char>(current->symbol));
                current = root;
            }
        }

        if (output.size() != data.originalSize)
            throw std::runtime_error("Compressed data is incomplete.");

        return output;
    }
};

int main() {
    std::cout << "========================================\n";
    std::cout << "       HUFFMAN TEXT COMPRESSION\n";
    std::cout << "          Portable C++11 Demo\n";
    std::cout << "========================================\n";
    std::cout << "Enter a line of text to compress:\n> ";

    std::string input;
    std::getline(std::cin, input);

    HuffmanCompressor compressor;
    std::array<std::string, 256> codes;
    EncodedData compressed;

    try {
        compressed = compressor.compress(input, codes);
        std::string restored = compressor.decompress(compressed);

        std::cout << "\n--- Huffman Codes ---\n";
        for (int i = 0; i < 256; ++i) {
            if (!codes[i].empty()) {
                if (i == 32)
                    std::cout << "[space]";
                else if (i == 9)
                    std::cout << "[tab]";
                else if (i >= 32 && i <= 126)
                    std::cout << "'" << static_cast<char>(i) << "'";
                else
                    std::cout << "[byte " << i << "]";
                std::cout << " : " << codes[i] << "\n";
            }
        }

        std::cout << "\n--- Compression Result ---\n";
        std::cout << "Original text: " << input << "\n";
        std::cout << "Original size: " << input.size() << " bytes\n";
        std::cout << "Encoded data:  ";

        if (compressed.bitCount == 0) {
            std::cout << "(empty)";
        } else {
            for (std::size_t bit = 0; bit < compressed.bitCount; ++bit) {
                std::size_t byteIndex = bit / 8;
                int bitIndex = 7 - static_cast<int>(bit % 8);
                std::cout << (((compressed.bytes[byteIndex] >> bitIndex) & 1U)
                              ? '1' : '0');
            }
        }
        std::cout << "\n";
        std::cout << "Encoded bits:  " << compressed.bitCount << "\n";
        std::cout << "Packed bytes:  " << compressed.bytes.size() << "\n";

        if (compressed.bitCount != 0) {
            double ratio = 100.0 * static_cast<double>(compressed.bytes.size()) /
                           static_cast<double>(input.size());
            std::cout << std::fixed << std::setprecision(2);
            std::cout << "Packed-data size relative to input: "
                      << ratio << "%\n";
        }

        std::cout << "\n--- Decompression Result ---\n";
        std::cout << "Restored text: " << restored << "\n";
        std::cout << "Verification: "
                  << (restored == input ? "SUCCESS - original text restored"
                                        : "FAILED")
                  << "\n";

        std::cout << "\nNote: This online-compiler edition compresses text in memory.\n";
        std::cout << "It demonstrates Huffman coding; it does not create a .huff file.\n";
    }
    catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }

    return 0;
}
