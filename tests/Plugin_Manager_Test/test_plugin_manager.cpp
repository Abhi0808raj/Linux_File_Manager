#include "core/plugin_manager.hpp"
#include <iostream>
#include <filesystem>

int main() {
    std::cout << "🔍 [Debug] Starting Plugin Manager Test\n";

    // Print current working directory for context
    std::filesystem::path cwd = std::filesystem::current_path();
    std::cout << "📍 [Debug] Current Working Directory: " << cwd << "\n";

    PluginManager manager;
    bool success = false;

    // Step 4 levels up to reach the project root
    std::filesystem::path projectRoot = std::filesystem::current_path()
        .parent_path()   // Plugin_Manager_Test
        .parent_path()   // tests
        .parent_path();  // ✅ /media/.../file_manager

    std::cout << "📁 [Debug] Resolved Project Root: " << projectRoot << "\n";

    // Absolute plugin search paths
    const std::vector<std::filesystem::path> pluginPaths = {
        projectRoot / "cmake-build-plugin_manager_test/built_plugin",
        projectRoot / "cmake-build-plugin_manager_test/plugins",
        projectRoot / "cmake-build-debug/plugins",
        projectRoot / "cmake-build-debug/built_plugin"
    };

    for (const auto& fullPath : pluginPaths) {
        std::cout << "\n📂 [Debug] Attempting to load plugins from: " << fullPath << "\n";

        if (!std::filesystem::exists(fullPath)) {
            std::cout << "❌ [Debug] Directory does not exist: " << fullPath << "\n";
            continue;
        }

        if (manager.loadPlugins(fullPath.string())) {
            std::cout << "✅ [Debug] Plugins loaded successfully from: " << fullPath << "\n";
            success = true;
            break;
        } else {
            std::cout << "⚠️ [Debug] Failed to load plugins from: " << fullPath << "\n";
        }
    }

    if (!success) {
        std::cerr << "❌ [Result] No plugins were loaded.\n";
        return 1;
    }

    size_t count = manager.pluginCount();
    std::cout << "\n✅ [Result] Loaded " << count << " plugin(s).\n";

    for (auto* plugin : manager.plugins()) {
        if (!plugin) {
            std::cout << "⚠️ [Debug] Encountered null plugin pointer.\n";
            continue;
        }

        std::cout << "\n🔌 Plugin Name:        " << plugin->name();
        std::cout << "\n📦 Plugin Version:     " << plugin->version();
        std::cout << "\n📄 Plugin Description: " << plugin->description();

        std::cout << "\n⚙️  Supported Operations:";
        for (const auto& op : plugin->operations()) {
            std::cout << " " << op;
        }

        std::cout << "\n🚀 [Debug] Trying to execute `example_operation`...\n";
        bool executed = plugin->execute("example_operation", {"arg1", "arg2"});
        if (executed) {
            std::cout << "✅ [Debug] Operation executed successfully.\n";
        } else {
            std::cout << "⚠️ [Debug] Plugin failed to execute operation.\n";
        }
    }

    std::cout << "\n🏁 [Debug] Plugin Manager Test Finished.\n";
    return 0;
}
