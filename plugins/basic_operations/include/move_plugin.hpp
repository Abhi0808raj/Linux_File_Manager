#pragma once

#include <core/plugin_interface.hpp>
#include <core/file_system.hpp>
#include <string>
#include <vector>

// The MovePlugin implements the "move" operation for the file manager.
class MovePlugin : public IFileManagerPlugin {
public:
    MovePlugin();
    ~MovePlugin() override = default;

    std::string name() const override;
    std::string version() const override;
    std::string description() const override;
    std::vector<std::string> operations() const override;

    // args[0] = source path, args[1] = destination path
    bool execute(const std::string& operation, const std::vector<std::string>& args) override;
};
