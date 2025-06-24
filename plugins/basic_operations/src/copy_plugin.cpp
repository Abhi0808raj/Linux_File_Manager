#include "../include/copy_plugin.hpp"

CopyPlugin::CopyPlugin() {}

std::string CopyPlugin::name() const {
    return "Copy Plugin";
}

std::string CopyPlugin::version() const {
    return "1.0";
}

std::string CopyPlugin::description() const {
    return "Provides file and directory copy functionality.";
}

std::vector<std::string> CopyPlugin::operations() const {
    return {"copy"};
}

bool CopyPlugin::execute(const std::string& operation, const std::vector<std::string>& args) {
    if (operation != "copy" || args.size() < 2) {
        return false;
    }
    const std::string& src = args[0];
    const std::string& dst = args[1];
    return FileSystem::copy(src, dst, /*overwrite=*/true);
}

// Factory function for dynamic loading
extern "C" IFileManagerPlugin* create_plugin() {
    return new CopyPlugin();
}
