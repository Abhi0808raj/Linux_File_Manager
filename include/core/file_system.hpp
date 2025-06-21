#pragma once

#include <string>
#include<vector>
#include <filesystem>    // For file and directory operations
#include <optional>      // For std::optional, used when returning values that may not be available

namespace fs=std::filesystem; //Alias for filesystem


//These are just declarations
//Implementation will be there in file_system.cpp
class FileSystem {
public:
    //path class is pre-defined in std::filesystem
    //Function for telling if path exists
    static bool exists(const fs::path& path);

    // Checks if the given path is a directory
    static bool isDirectory(const fs::path& path);

    // Checks if the given path is a regular file (not a directory or symlink)
    static bool isFile(const fs::path& path);

    // Returns a list of all files and directories inside the given directory
    static std::vector<fs::directory_entry> listDirectory(const fs::path& path);

    // Creates a directory at the given path, including any intermediate directories
    static bool createDirectory(const fs::path& path);

    // Removes the file or directory at the given path
    // If it's a directory, deletes it recursively
    static bool remove(const fs::path& path);

    // Copies a file or directory from source to destination
    // If it's a directory, copies contents recursively
    // `overwrite = true` allows replacing existing destination
    static bool copy(const fs::path& source, const fs::path& destination, bool overwrite = false);

    // Moves or renames a file or directory
    // `overwrite = true` allows replacing the destination if it exists
    static bool move(const fs::path& source, const fs::path& destination, bool overwrite = false);

    // Returns the size of a file in bytes, if it exists and is a file
    static std::optional<uintmax_t> fileSize(const fs::path& path);

    // Returns the last write time of a file or directory
    static std::optional<fs::file_time_type> lastWriteTime(const fs::path& path);

    // Reads the content of a text file and returns it as a string
    static std::optional<std::string> readFile(const fs::path& path);

    // Writes the given string content to the specified file
    // Overwrites the file if it already exists
    static bool writeFile(const fs::path& path, const std::string& content);

private:
    //We delete the object  to disallow anyone to create an object of this class
    //It is like a toolbox defined
    //User is supposed to use it Not build it again
    FileSystem() = delete;
};
