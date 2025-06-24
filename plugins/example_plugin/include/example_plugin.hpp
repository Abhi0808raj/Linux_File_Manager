#pragma once

#include <core/plugin_interface.hpp>
#include <string>
#include <vector>

class ExamplePlugin : public IFileManagerPlugin {
public:
    ExamplePlugin();
    ~ExamplePlugin() override = default;

    std::string name() const override;
    std::string version() const override;
    std::string description() const override;
    std::vector<std::string> operations() const override;

    bool execute(const std::string& operation, const std::vector<std::string>& args) override;
};
