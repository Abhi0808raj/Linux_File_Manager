#pragma once

// ============================================================
// ExamplePlugin — "File Info" reference implementation
//
// This plugin is intentionally kept simple so it can serve as
// a copy-paste starting point for new plugin authors.
//
// Supported operations
//   "file_info"  – prints size, permissions, type and mtime
//                  for every path supplied as an argument.
// ============================================================

#include <core/plugin_interface.hpp>
#include <string>
#include <vector>

class ExamplePlugin : public IFileManagerPlugin {
public:
    ExamplePlugin();
    ~ExamplePlugin() override = default;

    // ---- IFileManagerPlugin interface ----

    /// Human-readable name shown in the plugin manager UI.
    std::string name() const override;

    /// Semantic version string (MAJOR.MINOR.PATCH).
    std::string version() const override;

    /// Short description shown next to the plugin name.
    std::string description() const override;

    /// List of operation identifiers this plugin handles.
    /// Each string must be unique across all loaded plugins.
    std::vector<std::string> operations() const override;

    /// Dispatch an operation by name.
    /// @param operation  One of the strings returned by operations().
    /// @param args       Operation-specific positional arguments.
    /// @return true on success, false on any error.
    bool execute(const std::string& operation,
                 const std::vector<std::string>& args) override;

private:
    /// Implementation for the "file_info" operation.
    bool fileInfo(const std::vector<std::string>& paths);
};
