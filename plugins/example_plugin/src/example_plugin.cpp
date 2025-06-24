#include "example_plugin.hpp"
#include <iostream>

ExamplePlugin::ExamplePlugin() {}

std::string ExamplePlugin::name() const {
    return "Example Plugin";
}

std::string ExamplePlugin::version() const {
    return "1.0";
}

std::string ExamplePlugin::description() const {
    return "An example plugin demonstrating plugin structure and implementation.";
}

std::vector<std::string> ExamplePlugin::operations() const {
    return {"example_operation"};
}

bool ExamplePlugin::execute(const std::string& operation, const std::vector<std::string>& args) {
    if (operation != "example_operation") {
        return false;
    }
    std::cout << "Executing example operation with args:";
    for (const auto& arg : args) {
        std::cout << " " << arg;
    }
    std::cout << std::endl;
    return true;
}

extern "C" IFileManagerPlugin* create_plugin() {
    return new ExamplePlugin();
}
