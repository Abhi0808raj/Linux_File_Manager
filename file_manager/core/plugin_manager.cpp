#include "plugin_manager.hpp"
#include <iostream>
#include <stdexcept>
#include <system_error>
#include "error_handler.hpp"

// Constructor: Can initialize required state (currently empty)
PluginManager::PluginManager() {
    // Initialize any required resources (currently no special init)
}

// Destructor: Ensures all loaded plugins are properly unloaded
PluginManager::~PluginManager() {
    unloadPlugins();  // Clean up on destruction
}

bool PluginManager::loadPlugins(const std::string& directory) {
    namespace fs = std::filesystem;
    try {
        //Iterate through all files in given directory of plugin
        for (const auto& entry : fs::directory_iterator(directory)) {
            // If its a file and has shared library extension(.dll/.so)
            if (entry.is_regular_file()&& is_shared_library(entry.path())) {
                loadPluginFile(entry.path()); //try loading the plugin
            }
        }
        return true;
    } catch (const fs::filesystem_error& e) {
        FM_ERROR("File System error loading plugins: ",e.what());
    } catch (const std::exception& e) {
        FM_ERROR("Error loading plugins: ",e.what());
    }
    return false;
}