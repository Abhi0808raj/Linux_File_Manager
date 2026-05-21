#include <gtest/gtest.h>
#include "core/file_system.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class FileSystemTest : public ::testing::Test {
protected:
    fs::path testDir;

    void SetUp() override {
        testDir = fs::temp_directory_path() / "fm_unit_filesystem";
        fs::remove_all(testDir);
        fs::create_directories(testDir);
    }

    void TearDown() override {
        fs::remove_all(testDir);
    }

    fs::path tmp(const std::string& name) { return testDir / name; }

    void makeFile(const fs::path& p, const std::string& content = "data") {
        std::ofstream f(p);
        f << content;
    }
};

// --- exists / isDirectory / isFile ---

TEST_F(FileSystemTest, ExistsReturnsTrueForFile) {
    makeFile(tmp("a.txt"));
    EXPECT_TRUE(FileSystem::exists(tmp("a.txt")));
}

TEST_F(FileSystemTest, ExistsReturnsFalseForMissing) {
    EXPECT_FALSE(FileSystem::exists(tmp("ghost.txt")));
}

TEST_F(FileSystemTest, IsDirectoryReturnsTrueForDir) {
    EXPECT_TRUE(FileSystem::isDirectory(testDir));
}

TEST_F(FileSystemTest, IsFileReturnsTrueForRegularFile) {
    makeFile(tmp("reg.txt"));
    EXPECT_TRUE(FileSystem::isFile(tmp("reg.txt")));
}

TEST_F(FileSystemTest, IsFileReturnsFalseForDirectory) {
    EXPECT_FALSE(FileSystem::isFile(testDir));
}

// --- createDirectory ---

TEST_F(FileSystemTest, CreateDirectoryCreatesNestedPath) {
    auto nested = tmp("a") / "b" / "c";
    EXPECT_TRUE(FileSystem::createDirectory(nested));
    EXPECT_TRUE(fs::is_directory(nested));
}

TEST_F(FileSystemTest, CreateDirectoryReturnsFalseForExistingDir) {
    // create_directories returns 0 for an already-existing path — implementation returns false
    EXPECT_FALSE(FileSystem::createDirectory(testDir));
}

// --- copy ---

TEST_F(FileSystemTest, CopyCreatesDestinationWithSameContent) {
    makeFile(tmp("src.txt"), "hello copy");
    EXPECT_TRUE(FileSystem::copy(tmp("src.txt"), tmp("dst.txt")));
    EXPECT_TRUE(fs::exists(tmp("dst.txt")));
    auto content = FileSystem::readFile(tmp("dst.txt"));
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(*content, "hello copy");
}

TEST_F(FileSystemTest, CopyFailsWithoutOverwriteWhenDestExists) {
    makeFile(tmp("src.txt"), "new");
    makeFile(tmp("dst.txt"), "original");
    EXPECT_FALSE(FileSystem::copy(tmp("src.txt"), tmp("dst.txt"), false));
    // destination content must be unchanged
    auto content = FileSystem::readFile(tmp("dst.txt"));
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(*content, "original");
}

TEST_F(FileSystemTest, CopyOverwritesExistingWhenFlagSet) {
    makeFile(tmp("src.txt"), "updated");
    makeFile(tmp("dst.txt"), "stale");
    EXPECT_TRUE(FileSystem::copy(tmp("src.txt"), tmp("dst.txt"), true));
    auto content = FileSystem::readFile(tmp("dst.txt"));
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(*content, "updated");
}

// --- move ---

TEST_F(FileSystemTest, MoveRemovesSourceAndCreatesDestination) {
    makeFile(tmp("msrc.txt"), "moving");
    EXPECT_TRUE(FileSystem::move(tmp("msrc.txt"), tmp("mdst.txt")));
    EXPECT_FALSE(fs::exists(tmp("msrc.txt")));
    EXPECT_TRUE(fs::exists(tmp("mdst.txt")));
}

TEST_F(FileSystemTest, MoveOverwritesExistingWhenFlagSet) {
    makeFile(tmp("msrc2.txt"), "moved");
    makeFile(tmp("mdst2.txt"), "overwritten");
    EXPECT_TRUE(FileSystem::move(tmp("msrc2.txt"), tmp("mdst2.txt"), true));
    EXPECT_FALSE(fs::exists(tmp("msrc2.txt")));
    auto content = FileSystem::readFile(tmp("mdst2.txt"));
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(*content, "moved");
}

// --- remove ---

TEST_F(FileSystemTest, RemoveDeletesFile) {
    makeFile(tmp("del.txt"));
    EXPECT_TRUE(FileSystem::remove(tmp("del.txt")));
    EXPECT_FALSE(fs::exists(tmp("del.txt")));
}

TEST_F(FileSystemTest, RemoveDeletesDirectoryRecursively) {
    auto dir = tmp("subdir");
    fs::create_directories(dir);
    makeFile(dir / "nested.txt");
    EXPECT_TRUE(FileSystem::remove(dir));
    EXPECT_FALSE(fs::exists(dir));
}

TEST_F(FileSystemTest, RemoveReturnsFalseForNonexistentPath) {
    EXPECT_FALSE(FileSystem::remove(tmp("never_existed.txt")));
}

// --- readFile / writeFile ---

TEST_F(FileSystemTest, ReadFileReturnsContent) {
    makeFile(tmp("read.txt"), "read me");
    auto content = FileSystem::readFile(tmp("read.txt"));
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(*content, "read me");
}

TEST_F(FileSystemTest, ReadFileReturnsNulloptForMissingFile) {
    EXPECT_FALSE(FileSystem::readFile(tmp("nosuchfile.txt")).has_value());
}

TEST_F(FileSystemTest, WriteFilePersistsContent) {
    EXPECT_TRUE(FileSystem::writeFile(tmp("written.txt"), "persist me"));
    auto content = FileSystem::readFile(tmp("written.txt"));
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(*content, "persist me");
}

// --- fileSize ---

TEST_F(FileSystemTest, FileSizeReturnsCorrectByteCount) {
    makeFile(tmp("size.txt"), "12345");
    auto size = FileSystem::fileSize(tmp("size.txt"));
    ASSERT_TRUE(size.has_value());
    EXPECT_EQ(*size, 5u);
}

TEST_F(FileSystemTest, FileSizeReturnsNulloptForMissingFile) {
    EXPECT_FALSE(FileSystem::fileSize(tmp("nofile.txt")).has_value());
}
