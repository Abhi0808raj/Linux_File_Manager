#include "core/file_system.hpp"
#include <iostream>
#include <fstream>

int main() {
    std::filesystem::path testPath = "example.txt";

    // Create the file
    std::ofstream file(testPath);
    file << "Test content.\n";
    file.close();

    // Test existence again
    std::cout << "Exists after creation: " << FileSystem::exists(testPath) << std::endl;

    // Clean up
    std::filesystem::remove(testPath);

    return 0;
}
