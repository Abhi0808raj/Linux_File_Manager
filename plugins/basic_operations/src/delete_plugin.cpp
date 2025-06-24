#include "../include/delete_plugin.hpp"

DeletePlugin::DeletePlugin() {}

std::string DeletePlugin::name() const {
    return "Delete Plugin";
}

std::string DeletePlugin::version() const {
    return "1.0";
}

std::string DeletePlugin::description() const {
    return "Provides file and directory delete functionality.";
}

std::vector<std::string> DeletePlugin::operations() const {
    return {"delete"};
}

bool DeletePlugin::execute(const std::string& operation, const std::vector<std::string>& args) {
    if (operation != "delete" || args.empty()) {
        return false;
    }
    const std::string& target = args[0];
    // Use your core FileSystem utility for removal
    return FileSystem::remove(target);
}

// Factory function for dynamic loading
extern "C" IFileManagerPlugin* create_plugin() {
    return new DeletePlugin();
}
