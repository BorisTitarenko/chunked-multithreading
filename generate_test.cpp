#include <iostream>
#include <fstream>
#include <vector>
#include <random>

constexpr size_t FILE_SIZE_MB = 1000;  // Change this to 1000 for a 1GB file
constexpr size_t FILE_SIZE = FILE_SIZE_MB * 1024 * 1024; // Convert GB to bytes
constexpr size_t AVG_WORD_LENGTH = 5;
constexpr size_t UNIQUE_WORDS = 10000;  // Number of unique words

// Function to generate random words
std::string generate_random_word(std::mt19937& rng) {
    std::string word;
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    std::uniform_int_distribution<size_t> length_dist(3, 10);  // Word length between 3-10
    size_t word_length = length_dist(rng);
    
    for (size_t i = 0; i < word_length; ++i) {
        word += alphabet[rng() % (sizeof(alphabet) - 1)];
    }
    return word;
}

void generate_large_test_file(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file for writing!\n";
        return;
    }

    std::vector<std::string> words;
    std::mt19937 rng(std::random_device{}());

    // Generate a pool of unique words
    for (size_t i = 0; i < UNIQUE_WORDS; ++i) {
        words.push_back(generate_random_word(rng));
    }
    std::cout << words.size() << std::endl;

    std::uniform_int_distribution<size_t> word_dist(0, words.size() - 1);
    size_t bytes_written = 0;

    while (bytes_written < FILE_SIZE) {
        std::string word = words[word_dist(rng)] + " ";
        file << word;
        bytes_written += word.length();
    }

    file.close();
    std::cout << "Test file '" << filename << "' generated successfully!\n";
}

int main() {
    generate_large_test_file("large_test.txt");
    return 0;
}
