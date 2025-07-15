# Qt C++ File Manager

A modern, extensible file manager built with Qt and C++17, featuring a plugin architecture for enhanced functionality.

## Features

- **Cross-platform GUI** - Built with Qt for Windows, Linux, and macOS support
- **Plugin Architecture** - Extensible design allowing custom file operations through plugins
- **File System Operations** - Complete file and directory management capabilities
- **Robust Error Handling** - Comprehensive error handling and logging system
- **Thread-Safe Design** - Multi-threaded operations with proper synchronization

## Architecture

### Core Components

- **File System Layer** (`file_manager/core/file_system.cpp`)
    - File and directory operations (create, copy, move, delete)
    - Path validation and file metadata retrieval
    - Cross-platform filesystem abstraction

- **Plugin System** (`file_manager/core/plugin_manager.cpp`)
    - Dynamic plugin loading from shared libraries (.dll/.so)
    - Plugin lifecycle management
    - Operation execution framework

- **GUI Layer** (`file_manager/gui/`)
    - Qt-based main window with file tree view
    - Menu and toolbar integration
    - Status bar for user feedback

- **Utilities** (`file_manager/utilities/`)
    - Error handling and custom exceptions
    - Thread-safe logging system
    - Result wrapper for safe operations

## Building the Project

### Prerequisites

- **Qt 5.15+** or **Qt 6.0+**
- **CMake 3.16+**
- **C++17 compatible compiler**
    - GCC 7+
    - Clang 6+
    - MSVC 2019+

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/Abhi0808raj/Linux_File_Manager
cd file-manager

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
make -j$(nproc)  # Linux/macOS
# or
cmake --build . --config Release  # Windows
```

### Running the Application

```bash
# From build directory
./file_manager
```

## Plugin Development

### Creating a Plugin

1. **Inherit from IFileManagerPlugin**:

```cpp
#include <core/plugin_interface.hpp>

class MyPlugin : public IFileManagerPlugin {
public:
    std::string name() const override { return "My Plugin"; }
    std::string version() const override { return "1.0"; }
    std::string description() const override { return "Description"; }
    std::vector<std::string> operations() const override { return {"my_op"}; }
    
    bool execute(const std::string& operation, 
                const std::vector<std::string>& args) override {
        // Implementation
        return true;
    }
};
```

2. **Export the factory function**:

```cpp
extern "C" IFileManagerPlugin* create_plugin() {
    return new MyPlugin();
}
```

3. **Build as shared library**:

```cmake
add_library(my_plugin SHARED src/my_plugin.cpp)
target_include_directories(my_plugin PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(my_plugin PRIVATE file_manager_core)
```

### Available Plugins

The project includes several built-in plugins:

- **Copy Plugin** - File and directory copying
- **Move Plugin** - File and directory moving/renaming
- **Delete Plugin** - File and directory deletion
- **Example Plugin** - Template for plugin development

## File System Operations

### Basic Operations

```cpp
#include <core/file_system.hpp>

// Check if file exists
bool exists = FileSystem::exists("/path/to/file");

// Copy file
bool success = FileSystem::copy("/source", "/destination", true);

// Move file
bool success = FileSystem::move("/old/path", "/new/path");

// Delete file/directory
bool success = FileSystem::remove("/path/to/delete");

// Read file contents
auto content = FileSystem::readFile("/path/to/file");
if (content) {
    std::cout << *content << std::endl;
}
```

### Directory Operations

```cpp
// List directory contents
auto entries = FileSystem::listDirectory("/path/to/directory");
for (const auto& entry : entries) {
    std::cout << entry.path() << std::endl;
}

// Create directory
bool success = FileSystem::createDirectory("/new/directory");

// Get file size
auto size = FileSystem::fileSize("/path/to/file");
if (size) {
    std::cout << "File size: " << *size << " bytes" << std::endl;
}
```

## Error Handling

The project uses a comprehensive error handling system:

### Exception Types

- `FileManagerException` - Base exception class
- `FileSystemException` - File system related errors
- `PluginException` - Plugin loading/execution errors
- `GuiException` - GUI related errors

### Error Handling Macros

```cpp
#include <utilities/error_handler.hpp>

// Assertions
FM_ASSERT(condition, "Error message");
FM_REQUIRE(condition, "Requirement failed");

// Logging
FM_INFO("Information message");
FM_WARNING("Warning message");
FM_ERROR("Error message");
FM_CRITICAL("Critical error");

// Throwing exceptions
FM_THROW("General error");
FM_THROW_FS("Filesystem error");
FM_THROW_PLUGIN("Plugin error");
```

### Safe Execution

```cpp
auto result = ErrorHandler::safeExecute([]() {
    return riskyOperation();
}, "Operation description");

if (result.isSuccess()) {
    auto value = result.value();
    // Use value
} else {
    std::cout << "Error: " << result.errorMessage() << std::endl;
}
```

## Project Structure

```
file_manager/
│
├── structure/
│   └── structure.txt
│
├── include/                              # All public/project headers
│   ├── core/
│   │   ├── file_system.hpp
│   │   ├── plugin_interface.hpp
│   │   └── plugin_manager.hpp
│   │
│   ├── gui/
│   │   ├── main_window.hpp
│   │   └── file_view.hpp
│   │
│   └── utilities/
│       ├── logger.hpp
│       └── error_handler.hpp
│
├── file_manager/                         # Core application code (sources only)
│   ├── core/
│   │   ├── file_system.cpp
│   │   ├── plugin_manager.cpp
│   │
│   ├── gui/
│   │   ├── main_window.cpp
│   │   └── file_view.cpp
│   │
│   └── utilities/
│       ├── logger.cpp
│       └── error_handler.cpp
│
├── plugins/
│   ├── basic_operations/
│   │   ├── include/
│   │   │   ├── copy_plugin.hpp
│   │   │   ├── delete_plugin.hpp
│   │   │   └── move_plugin.hpp
│   │   ├── src/
│   │   │   ├── copy_plugin.cpp
│   │   │   ├── delete_plugin.cpp
│   │   │   └── move_plugin.cpp
│   │   ├── metadata.json
│   │   └── CMakeLists.txt
│   │
│   └── example_plugin/
│       ├── include/
│       │   └── example_plugin.hpp
│       ├── src/
│       │   └── example_plugin.cpp
│       ├── metadata.json
│       └── CMakeLists.txt
├── app/
│   └── main.cpp
│
├── resources/
│   └── icons/
│
├── tests/
│   ├── Error_Handler_Test/
│   │   ├── CMakeLists.txt
│   │   └── error_handler_test.cpp
│   ├── File_system_Test/
│   │   ├── CMakeLists.txt
│   │   └── test_file_system_only.cpp
│   ├── Logger_Test/
│   │   ├── CMakeLists.txt
│   │   └── test_logger.cpp
│   ├── Plugin_Manager_Test/
│        ├── CMakeLists.txt
│        └── test_plugin_manager.cpp
│
├── CMakeLists.txt
│
└── cmake/
    └── PluginConfig.cmake

```

## Configuration

### Plugin Directory

By default, plugins are loaded from the `plugins/` directory. This can be configured programmatically:

```cpp
PluginManager manager;
manager.loadPlugins("/custom/plugin/path");
```

### Logging

Logging is configured through the `Logger` class:

```cpp
Logger logger("custom_log.txt");
logger.log(ErrorSeverity::INFO, "Custom message");
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Submit a pull request

### Code Style

- Follow C++17 best practices
- Use RAII for resource management
- Prefer const correctness
- Use meaningful variable names
- Add comprehensive comments

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Dependencies

- **Qt** - GUI framework
- **C++17 Standard Library** - Core functionality
- **CMake** - Build system
- **Platform-specific libraries**:
    - Windows: `windows.h` for plugin loading
    - Linux/macOS: `dlfcn.h` for plugin loading

## Troubleshooting

### Plugin Loading Issues

1. Ensure plugins are in the correct directory
2. Check that plugins export the `create_plugin()` function
3. Verify plugin dependencies are satisfied
4. Check error logs for specific loading failures

### Build Issues

1. Ensure Qt is properly installed and configured
2. Check CMake version compatibility
3. Verify C++17 compiler support
4. Check for missing dependencies

### Runtime Issues

1. Check log files for error messages
2. Verify file permissions
3. Ensure plugins are compatible with the current version
4. Check for system-specific path issues

## Future Enhancements

- Network file operations
- Archive handling (ZIP, TAR)
- File preview capabilities
- Advanced search functionality
- Bookmarks and favorites
- Multi-tab interface
- Theme support