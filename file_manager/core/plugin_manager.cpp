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

//Unloading all plugins and clean internal containers
void PluginManager::unloadPlugins() {
    for (&auto entry:loadedPlugins_) {
        unloadPlugin(entry);   //Unload each plugin
    }
    loadedPlugins_.clear();       // Clear Plugin List
    nameToPlugins_.clear();       // Clear name to pointer map
}

//Function to return list of all pointers to all currently loaded plugin instances
//It stores the references to avoid memory copying
const std::vector<IFileManagerPlugin*>& PluginManager::plugins() const {
    // IFileManagerPlugin object(defined in plugin_interface.hpp)
    //Vector to store plugin pointers
    static std::vector<IFileManagerPlugin*> pluginPtrs;
    pluginPtrs.clear(); //To clear any previous pointers stored from earlier cells

    for (const auto& entry : loadedPlugins_) {
        //.get() used to return raw pointer
        pluginPtrs.push_back(entry.instance.get());
    }
    return pluginPtrs;
}

// Retrieve a plugin instance by its registered name
IFileManagerPlugin* PluginManager::getPluginByName(const std::string& name) const {
    //find the plugin name in nameToPlugin map
    auto it = nameToPlugin_.find(name);
    return (it != nameToPlugin_.end()) ? it->second : nullptr;  // Return plugin if found
}

// Returns number of loaded plugins
size_t PluginManager::pluginCount() const {
    return loadedPlugins_.size();
}

// PRIVATE METHODS

// Check if the file has shared library extension based on platform
bool PluginManager::is_shared_library(const std::filesystem::path& path) {
    #ifdef _WIN32
        return path.extension() == ".dll";
    #else
        return path.extension() == ".so";
    #endif
}

// Dynamically loads a plugin file, call its factory function(create_plugin())
//according to operating system, register it into system
// and handle failure clearly
bool PluginManager::loadPluginFile(const std::filesystem::path& filePath) {
    // Platform-specific way to load a shared library
    #ifdef _WIN32
        PluginHandle handle = LoadLibraryW(filePath.c_str());  // Windows
    #else
        PluginHandle handle = dlopen(filePath.c_str(), RTLD_LAZY | RTLD_LOCAL);  // Linux
    #endif

    if (!handle) {
        // Print error if loading fails
        #ifdef _WIN32
            FM_WARNING("Failed to load plugin (Windows error ", GetLastError(), "): ", filePath.string());
        #else
            FM_WARNING("Failed to load plugin: ", dlerror(), " (", filePath.string(), ")");
        #endif
        return false;
    }

    // Define expected function signature for plugin creation
    using CreatePluginFunc = IFileManagerPlugin*(*)();
    CreatePluginFunc createFunc = nullptr;

    // Try to get "create_plugin" symbol from the shared library
    #ifdef _WIN32
        createFunc = reinterpret_cast<CreatePluginFunc>(
            GetProcAddress(handle, "create_plugin")
        );
    #else
        createFunc = reinterpret_cast<CreatePluginFunc>(
            dlsym(handle, "create_plugin")
        );
    #endif

    if (!createFunc) {
        // If the function doesn't exist, unload and warn
        #ifdef _WIN32
            FM_WARNING("Plugin missing create_plugin function: ", filePath.string());
            FreeLibrary(handle);
        #else
            FM_WARNING("Plugin missing create_plugin function: ", dlerror());
            dlclose(handle);
        #endif
        return false;
    }

    // Try to create the plugin instance
    try {
        auto plugin = std::unique_ptr<IFileManagerPlugin>(createFunc());  // Call the factory function
        const std::string pluginName = plugin->name();  // Get plugin name for registration

        // Add to internal plugin list
        loadedPlugins_.push_back({
            std::move(plugin),
            handle,
            pluginName
        });

        // Register name-to-pointer for fast access
        nameToPlugin_[pluginName] = loadedPlugins_.back().instance.get();

        FM_INFO("Loaded plugin: ", pluginName, " (", filePath.filename().string(), ")");
        return true;
    } catch (const std::exception& e) {
        // On error, unload and report
        FM_ERROR("Plugin initialization failed: ", e.what(), " (", filePath.string(), ")");
        #ifdef _WIN32
            FreeLibrary(handle);
        #else
            dlclose(handle);
        #endif
        return false;
    }
}

void PluginManager::unloadPlugin(PluginEntry& entry) {
    entry.instance.reset();  // Delete plugin instance

    // Unload the shared library
    #ifdef _WIN32
        if (entry.handle) FreeLibrary(entry.handle);
    #else
        if (entry.handle) dlclose(entry.handle);
    #endif

    FM_INFO("Unloaded plugin: ", entry.name);
}
