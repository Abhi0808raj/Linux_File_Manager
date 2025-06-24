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
    fs::copy_options options=fs::copy_options::none;
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

bool FileSystem::move(const fs::path& source, const fs::path& destination, bool overwrite) {
    std::error_code ec;
    //if destination already exists and overwrite is true, remove the destination first
    if (overwrite&&fs::exists(destination)) {
        fs::remove_all(destination,ec);
        if (ec) {
            std::cerr<<"Error removing existing destination"<<ec.message()<<std::endl;
            return false;
        }
    }
    //In filesystem renaming is same as moving because in renaming we are
    //renaming the path of the file, So the path of file gets changed
    //this is what moving basically is. We rename the path
    //If we want to move from one device to another we have to manually
    //copy the file and then remove from the origin
    fs::rename(source,destination,ec);
    if (ec) {
        std::cerr<<"Error moving: "<<ec.message()<<std::endl;
        return false;
    }
    return true;
}

//get file size :returns optional to handle errors
//Optional is used because we dont want this function to throw errors
//We return size when successful otherwise we return nullopt which means no value present
std::optional<uintmax_t> FileSystem::fileSize(const fs::path& path) {
    std::error_code ec;
    auto size = fs::file_size(path, ec);
    if (ec) {
        std::cerr << "Error getting file size: " << ec.message() << std::endl;
        return std::nullopt;
    }
    return size;
}

// Get last modified time of a file
std::optional<fs::file_time_type> FileSystem::lastWriteTime(const fs::path& path) {
    std::error_code ec;
    auto time = fs::last_write_time(path, ec);
    if (ec) {
        std::cerr << "Error getting last write time: " << ec.message() << std::endl;
        return std::nullopt;
    }
    return time;
}

//Read contents of a file and return as string
std::optional<std::string> FileSystem::readFile(const fs::path& path) {
    //open the file in given path in binary mode
    //this bitwise or guarantees read access
    //so this line basically means file is opened for reading
    //and it reads as binary
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file for reading " << path << std::endl;
        return std::nullopt;
    }
    //istreambuf_iterator<char>(file) reads the file byte by byte until end of file
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return content;
    //we didnt need to close the file a our destructor will automatically
    //destroy the object and close our file
}

bool FileSystem::writeFile(const fs::path& path, const std::string& content) {
    std::ofstream file(path, std::ios::out | std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file for writing " << path << std::endl;
        return false;
    }
    file.write(content.data(), content.size());
    return file.good(); //means return true if write was success
}



