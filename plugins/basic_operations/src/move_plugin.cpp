#include "../include/move_plugin.hpp"

MovePlugin::MovePlugin() {}

std::string MovePlugin::name() const {
    return "Move Plugin";
}

std::string MovePlugin::version() const {
    return "1.0";
}

std::string MovePlugin::description() const {
    return "Provides file and directory move functionality.";
}

std::vector<std::string> MovePlugin::operations() const {
    return {"move"};
}

bool MovePlugin::execute(const std::string& operation, const std::vector<std::string>& args) {
    if (operation != "move" || args.size() < 2) {
        return false;
    }
    const std::string& src = args[0];
    const std::string& dst = args[1];
    // Use your core FileSystem utility for moving
    return FileSystem::move(src, dst, /*overwrite=*/true);
}

// Factory function for dynamic loading
extern "C" IFileManagerPlugin* create_plugin() {
    return new MovePlugin();
}
