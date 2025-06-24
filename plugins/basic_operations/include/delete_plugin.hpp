#pragma once

#include <core/plugin_interface.hpp>
#include <core/file_system.hpp>
#include <string>
#include <vector>

// The DeletePlugin implements the "delete" operation for the file manager.
class DeletePlugin : public IFileManagerPlugin {
public:
    DeletePlugin();
    ~DeletePlugin() override = default;

    std::string name() const override;
    std::string version() const override;
    std::string description() const override;
    std::vector<std::string> operations() const override;

    // args[0] = path to delete (file or directory)
    bool execute(const std::string& operation, const std::vector<std::string>& args) override;
};
