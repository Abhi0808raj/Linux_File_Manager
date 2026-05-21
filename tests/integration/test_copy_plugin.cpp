#ifdef assert
#  undef assert
#endif

#include <gtest/gtest.h>
#include "core/plugin_manager.hpp"
#include "core/file_system.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

#ifndef PLUGIN_BUILD_DIR
#  error "PLUGIN_BUILD_DIR must be set via target_compile_definitions"
#endif

class CopyPluginIntegrationTest : public ::testing::Test {
protected:
    PluginManager manager;
    fs::path testDir;

    void SetUp() override {
        testDir = fs::temp_directory_path() / "fm_integration_copy";
        fs::remove_all(testDir);
        fs::create_directories(testDir);

        fs::path pluginDir{PLUGIN_BUILD_DIR};
        ASSERT_TRUE(fs::exists(pluginDir))
            << "Plugin build dir not found: " << pluginDir
            << "\nRun cmake --build first.";
        ASSERT_TRUE(manager.loadPlugins(pluginDir.string()))
            << "Failed to load plugins from: " << pluginDir;
    }

    void TearDown() override {
        manager.unloadPlugins();
        fs::remove_all(testDir);
    }
};

TEST_F(CopyPluginIntegrationTest, CopyPluginIsLoadedByName) {
    EXPECT_NE(manager.getPluginByName("Copy Plugin"), nullptr);
}

TEST_F(CopyPluginIntegrationTest, ExecuteCopyCreatesDestinationWithCorrectContent) {
    auto src = testDir / "source.txt";
    auto dst = testDir / "destination.txt";
    {
        std::ofstream f(src, std::ios::binary);
        f.write("integration test payload", 24);
        f.close();
        ASSERT_TRUE(f.good()) << "Failed to create source file";
    }

    auto* plugin = manager.getPluginByName("Copy Plugin");
    ASSERT_NE(plugin, nullptr);

    bool ok = plugin->execute("copy", {src.string(), dst.string()});
    EXPECT_TRUE(ok);
    EXPECT_TRUE(fs::exists(dst));

    auto content = FileSystem::readFile(dst);
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(*content, "integration test payload");
}

TEST_F(CopyPluginIntegrationTest, ExecuteReturnsFalseForWrongOperation) {
    auto* plugin = manager.getPluginByName("Copy Plugin");
    ASSERT_NE(plugin, nullptr);
    EXPECT_FALSE(plugin->execute("delete", {"/any/src", "/any/dst"}));
}

TEST_F(CopyPluginIntegrationTest, ExecuteReturnsFalseWithTooFewArgs) {
    auto* plugin = manager.getPluginByName("Copy Plugin");
    ASSERT_NE(plugin, nullptr);
    EXPECT_FALSE(plugin->execute("copy", {"/only/one/arg"}));
}
