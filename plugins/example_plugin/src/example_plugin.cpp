// ============================================================
// ExamplePlugin — "File Info" reference implementation
//
// Purpose
//   This file is the canonical example of how to write a plugin
//   for Linux File Manager.  It implements one real operation
//   ("file_info") so there is working, testable code to study.
//
// How to adapt this for your own plugin
//   1. Copy the entire plugins/example_plugin directory.
//   2. Rename every occurrence of "ExamplePlugin" / "example_plugin"
//      to your own class / directory name.
//   3. Replace the "file_info" case in execute() with your logic.
//   4. Update metadata.json with your plugin's details.
//   5. Add your directory to the root CMakeLists.txt (or the
//      plugins/CMakeLists.txt if one exists).
// ============================================================

#include "example_plugin.hpp"

#include <sys/stat.h>   // stat, S_IS* macros
#include <cerrno>       // errno
#include <cstring>      // std::strerror
#include <ctime>        // strftime, localtime
#include <iomanip>      // std::setw
#include <iostream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Constructor / boilerplate
// ---------------------------------------------------------------------------

ExamplePlugin::ExamplePlugin() {}

std::string ExamplePlugin::name() const {
    return "Example Plugin";
}

std::string ExamplePlugin::version() const {
    return "1.0.0";
}

std::string ExamplePlugin::description() const {
    return "Reference plugin: reports size, permissions, type and "
           "modification time for one or more file-system paths.";
}

std::vector<std::string> ExamplePlugin::operations() const {
    // Add more operation names here as your plugin grows.
    return {"file_info"};
}

// ---------------------------------------------------------------------------
// execute() — dispatcher
// ---------------------------------------------------------------------------

bool ExamplePlugin::execute(const std::string& operation,
                            const std::vector<std::string>& args) {
    if (operation == "file_info") {
        return fileInfo(args);
    }

    // Unknown operation — return false so the plugin manager can
    // report a meaningful error instead of silently succeeding.
    std::cerr << "[example_plugin] Unknown operation: " << operation << '\n';
    return false;
}

// ---------------------------------------------------------------------------
// fileInfo() — real implementation
// ---------------------------------------------------------------------------

bool ExamplePlugin::fileInfo(const std::vector<std::string>& paths) {
    if (paths.empty()) {
        std::cerr << "[example_plugin] file_info: no paths provided.\n";
        return false;
    }

    bool allSucceeded = true;

    for (const std::string& path : paths) {
        struct stat st{};

        if (::stat(path.c_str(), &st) != 0) {
            std::cerr << "[example_plugin] Cannot stat '" << path
                      << "': " << std::strerror(errno) << '\n';
            allSucceeded = false;
            continue;
        }

        // ---- file type ----
        std::string type;
        if      (S_ISREG(st.st_mode))  type = "regular file";
        else if (S_ISDIR(st.st_mode))  type = "directory";
        else if (S_ISLNK(st.st_mode))  type = "symbolic link";
        else if (S_ISFIFO(st.st_mode)) type = "named pipe (FIFO)";
        else if (S_ISSOCK(st.st_mode)) type = "socket";
        else if (S_ISBLK(st.st_mode))  type = "block device";
        else if (S_ISCHR(st.st_mode))  type = "character device";
        else                            type = "unknown";

        // ---- permissions (rwxrwxrwx style) ----
        const mode_t m = st.st_mode;
        char perms[10];
        perms[0] = (m & S_IRUSR) ? 'r' : '-';
        perms[1] = (m & S_IWUSR) ? 'w' : '-';
        perms[2] = (m & S_IXUSR) ? 'x' : '-';
        perms[3] = (m & S_IRGRP) ? 'r' : '-';
        perms[4] = (m & S_IWGRP) ? 'w' : '-';
        perms[5] = (m & S_IXGRP) ? 'x' : '-';
        perms[6] = (m & S_IROTH) ? 'r' : '-';
        perms[7] = (m & S_IWOTH) ? 'w' : '-';
        perms[8] = (m & S_IXOTH) ? 'x' : '-';
        perms[9] = '\0';

        // ---- modification time ----
        char timeBuf[32];
        std::tm* tmPtr = std::localtime(&st.st_mtime);
        std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", tmPtr);

        // ---- print summary ----
        std::cout << "Path        : " << path            << '\n'
                  << "Type        : " << type            << '\n'
                  << "Size        : " << st.st_size << " bytes\n"
                  << "Permissions : " << perms           << '\n'
                  << "Modified    : " << timeBuf         << '\n'
                  << '\n';
    }

    return allSucceeded;
}

// ---------------------------------------------------------------------------
// Factory function — required by the plugin loader.
// Do NOT rename or change the signature.
// ---------------------------------------------------------------------------

extern "C" IFileManagerPlugin* create_plugin() {
    return new ExamplePlugin();
}
