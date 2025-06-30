#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <vector>

const int NUM_COMMANDS = 20;         // Total number of commands to generate
const int NUM_BLOCKS = 8;            // Block IDs range from 0 to NUM_BLOCKS-1
const int MAX_VALUE = 1000;          // Max value for write operations

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <output_file> <read_write_ratio (0.0 to 1.0)>\n";
        return 1;
    }

    std::ofstream out(argv[1]);
    if (!out) {
        std::cerr << "Error opening output file\n";
        return 1;
    }

    double read_ratio = std::stod(argv[2]);
    if (read_ratio < 0.0 || read_ratio > 1.0) {
        std::cerr << "Read/write ratio must be between 0.0 and 1.0\n";
        return 1;
    }

    std::srand(std::time(nullptr));

    for (int i = 0; i < NUM_COMMANDS; ++i) {
        double r = (double) std::rand() / RAND_MAX;
        int block = std::rand() % NUM_BLOCKS;

        if (r < read_ratio) {
            out << "R " << block << "\n";
        } else {
            int value = std::rand() % MAX_VALUE;
            out << "W " << block << " " << value << "\n";
        }
    }

    std::cout << "Command file generated: " << argv[1] << "\n";
    return 0;
}
