#ifdef assert
#  undef assert
#endif

#include <gtest/gtest.h>
#include "core/plugin_manager.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

#ifndef PLUGIN_BUILD_DIR
#  error "PLUGIN_BUILD_DIR must be set via target_compile_definitions"
#endif

class PluginManagerTest : public ::testing::Test {
protected:
    PluginManager manager;
    fs::path pluginDir{PLUGIN_BUILD_DIR};
    fs::path tmpDir;

    void SetUp() override {
        tmpDir = fs::temp_directory_path() / "fm_unit_plugins";
        fs::remove_all(tmpDir);
        fs::create_directories(tmpDir);
    }

    void TearDown() override {
        manager.unloadPlugins();
        fs::remove_all(tmpDir);
    }
};

TEST_F(PluginManagerTest, LoadsValidPluginsFromBuildDir) {
    ASSERT_TRUE(fs::exists(pluginDir)) << "Plugin dir missing: " << pluginDir;
    EXPECT_TRUE(manager.loadPlugins(pluginDir.string()));
    EXPECT_GT(manager.pluginCount(), 0u);
}

TEST_F(PluginManagerTest, PluginsVectorSizeMatchesPluginCount) {
    ASSERT_TRUE(fs::exists(pluginDir));
    manager.loadPlugins(pluginDir.string());
    EXPECT_EQ(manager.plugins().size(), manager.pluginCount());
}

TEST_F(PluginManagerTest, GetPluginByNameReturnsCopyPlugin) {
    ASSERT_TRUE(fs::exists(pluginDir));
    manager.loadPlugins(pluginDir.string());
    auto* p = manager.getPluginByName("Copy Plugin");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->name(), "Copy Plugin");
}

TEST_F(PluginManagerTest, GetPluginByNameReturnsNullptrForUnknown) {
    ASSERT_TRUE(fs::exists(pluginDir));
    manager.loadPlugins(pluginDir.string());
    EXPECT_EQ(manager.getPluginByName("Nonexistent Plugin"), nullptr);
}

TEST_F(PluginManagerTest, InvalidSharedLibraryIsSkippedNotLoaded) {
    // Write garbage bytes — dlopen will fail on non-ELF content
    fs::path fakeLib = tmpDir / "fake_plugin.so";
    std::ofstream(fakeLib) << "not a real ELF shared library";
    bool traversed = manager.loadPlugins(tmpDir.string());
    EXPECT_TRUE(traversed);           // directory was accessible
    EXPECT_EQ(manager.pluginCount(), 0u);  // invalid file not registered
}

TEST_F(PluginManagerTest, EmptyDirectoryTraversesWithZeroPlugins) {
    bool ok = manager.loadPlugins(tmpDir.string());
    EXPECT_TRUE(ok);
    EXPECT_EQ(manager.pluginCount(), 0u);
}

TEST_F(PluginManagerTest, NonexistentDirectoryReturnsFalse) {
    EXPECT_FALSE(manager.loadPlugins("/no/such/plugin/path/exists"));
    EXPECT_EQ(manager.pluginCount(), 0u);
}
