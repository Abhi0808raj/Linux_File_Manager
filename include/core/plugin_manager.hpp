#pragma once

// Include necessary standard libraries
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <filesystem>

// Platform-specific includes and plugin handle definition
//Allows the code to compile in all platforms
#ifdef _WIN32 // if code is being compiled on windows
    #include <windows.h>
    using PluginHandle = HMODULE; // On Windows, plugin handles are of type HMODULE

#else // If system is not windows its assumend its a unix like OS (like Lunix or macOS)
    #include <dlfcn.h>
    using PluginHandle = void*;   // On Unix-like systems, plugin handles are void*

#endif

#include "plugin_interface.hpp"  // Include the plugin interface that all plugins must implement

// Class responsible for managing plugins dynamically loaded from shared libraries
class PluginManager {
public:
    PluginManager();   // Constructor
    ~PluginManager();  // Destructor — unloads all plugins when the manager is destroyed

    // Load all plugins from a given directory (e.g., ./plugins/)
    bool loadPlugins(const std::string& directory);

    // Unload all loaded plugins and release associated memory and handles
    void unloadPlugins();

    // Return a list of raw pointers to the loaded plugin instances
    const std::vector<IFileManagerPlugin*>& plugins() const;

    // Find and return a plugin instance by its name (returns nullptr if not found)
    IFileManagerPlugin* getPluginByName(const std::string& name) const;

    // Return the number of successfully loaded plugins
    size_t pluginCount() const;

private:
    // Structure to keep track of a loaded plugin:
    // - The plugin instance (as a unique_ptr for automatic memory management)
    // - The system-specific handle to the shared library
    // - The name of the plugin
    struct PluginEntry {
        std::unique_ptr<IFileManagerPlugin> instance;
        PluginHandle handle;
        std::string name;
    };

    // Vector of all loaded plugin entries (used for unloading later)
    std::vector<PluginEntry> loadedPlugins_;

    // Fast lookup table mapping plugin name to its instance pointer
    std::unordered_map<std::string, IFileManagerPlugin*> nameToPlugin_;

    // Internal helper function to load a single plugin file
    // Returns true if plugin was loaded successfully, false otherwise
    bool loadPluginFile(const std::filesystem::path& filePath);

    // Internal helper function to unload a single plugin entry
    void unloadPlugin(PluginEntry& entry);

    // Disable copy and move semantics to prevent accidental duplication
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    PluginManager(PluginManager&&) = delete;
    PluginManager& operator=(PluginManager&&) = delete;
};
