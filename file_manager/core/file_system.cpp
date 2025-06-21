#include "file_system.hpp"
#include <fstream>
#include <iostream>
#include <system_error>

namespace fs = std::filesystem;

//These are all in filesystem Library

// Check if a file or directory exists
bool FileSystem::exists(const fs::path& path) {
    return fs::exists(path);
}

// Check if a path is a directory
bool FileSystem::isDirectory(const fs::path& path) {
    return fs::is_directory(path);
}

// Check if a path is a regular file
bool FileSystem::isFile(const fs::path& path) {
    return fs::is_regular_file(path);
}

// List contents of a directory
std::vector<fs::directory_entry> FileSystem::listDirectory(const fs::path& path) {
    std::vector<fs::directory_entry> entries;
    try {
        // Ensure the path exists and is a directory
        if (fs::exists(path) && fs::is_directory(path)) {
            for (const auto& entry : fs::directory_iterator(path)) {
                entries.push_back(entry);
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error listing directory: " << e.what() << std::endl;
    }
    return entries;
}

//For Creating Directories
//Returns true if successful
bool FileSystem::createDirectory(const fs::path& path) {
    std::error_code ec;     //variable for storing error

    //Create_directories used for creating directories again and again
    //until final complete path is created

    bool result = fs::create_directory(path,ec)>0;
    if (ec) {
        std::cerr << "Error creating directory: " << ec.message() << std::endl;
    }
    return result;
}

//For removing file or directory
//returns true if successful
bool FileSystem::remove(const fs::path& path) {
    std::error_code ec;
    bool result = fs::remove_all(path,ec)>0;
    if (ec) {
        std::cerr << "Error removing path: " << ec.message() << std::endl;
    }
    return result;
}

//For copying file from one path to another
//takes in source path,destination path
//and overwrite variable to decide whether to override the file if already present or not
//returns true if successful
bool FileSystem::copy(const fs::path& source, const fs::path& destination, bool overwrite) {
    std::error_code ec;
    //default copy behaviour from copy_options class in filesystem
    fs::copy_options options=fs::copy_options()::none;
    if (overwrite) {
        options=fs::copy_options::overwrite_existing;
    }
    fs::copy(source,destination,options,ec);
    if (ec) {
        std::cerr<<"Error Copying: "<<ec.message()<<std::endl;
        return false;
    }
    return true;
}






