// tests/unit/test_properties_dialog.cpp
// Unit + property-based tests for PropertiesDialog.
// Requirements: 10.2, 10.6
//
// This file holds unit tests for the PropertiesDialog (task 5.3) and will
// later receive the property test for Property 13 (task 5.4).

#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <QApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QRegularExpression>

#include "gui/properties_dialog.hpp"

// ---------------------------------------------------------------------------
// PropertiesDialogTestAccess — friend class for accessing private members.
// ---------------------------------------------------------------------------
class PropertiesDialogTestAccess {
public:
    static bool isValid(const PropertiesDialog& dlg) { return dlg.valid_; }
    static const PropertiesView& view(const PropertiesDialog& dlg) { return dlg.view_; }
};

// ---------------------------------------------------------------------------
// QApplication fixture — ensures a single QApplication instance exists for
// all tests in this file.
// ---------------------------------------------------------------------------
class PropertiesDialogTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!QApplication::instance()) {
            static int argc = 1;
            static char appName[] = "test_properties_dialog";
            static char* argv[] = { appName, nullptr };
            app_ = new QApplication(argc, argv);
        }
    }

    static void TearDownTestSuite() {
        // QApplication is intentionally leaked — it must outlive all Qt objects.
    }

    static QApplication* app_;
};

QApplication* PropertiesDialogTest::app_ = nullptr;

// ---------------------------------------------------------------------------
// Test: PropertiesDialog for a regular file renders all fields correctly
//
// Creates a file inside QTemporaryDir, instantiates PropertiesDialog, and
// asserts each rendered field is present and well-formed.
//
// Validates: Requirements 10.2
// ---------------------------------------------------------------------------
TEST_F(PropertiesDialogTest, FileRendersAllFields) {
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());

    // Create a test file with some content
    QString filePath = QDir(tmpDir.path()).filePath("testfile.txt");
    {
        QFile f(filePath);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("Hello, World!");  // 13 bytes
        f.close();
    }

    PropertiesDialog dlg(filePath);

    // Dialog should be valid
    ASSERT_TRUE(PropertiesDialogTestAccess::isValid(dlg));

    const PropertiesView& v = PropertiesDialogTestAccess::view(dlg);

    // Path: must be the absolute path we provided
    EXPECT_EQ(v.path, filePath);

    // Type: must be "File"
    EXPECT_EQ(v.type, QStringLiteral("File"));

    // Size: must match regex like "\\d+\\.\\d+ [KMGT]?B"
    QRegularExpression sizeRegex(QStringLiteral("^\\d+\\.\\d+ [KMGT]?B$"));
    EXPECT_TRUE(sizeRegex.match(v.size).hasMatch())
        << "Size string '" << v.size.toStdString() << "' does not match expected format";

    // Modified: must match "yyyy-MM-dd HH:mm:ss" format
    QRegularExpression dateRegex(QStringLiteral("^\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}$"));
    EXPECT_TRUE(dateRegex.match(v.modified).hasMatch())
        << "Modified string '" << v.modified.toStdString() << "' does not match expected format";

    // Permissions: must be a 4-digit octal string
    QRegularExpression permRegex(QStringLiteral("^\\d{4}$"));
    EXPECT_TRUE(permRegex.match(v.permissions).hasMatch())
        << "Permissions string '" << v.permissions.toStdString() << "' does not match expected format";
}

// ---------------------------------------------------------------------------
// Test: PropertiesDialog for a directory renders all fields correctly
//
// Creates a directory inside QTemporaryDir, instantiates PropertiesDialog,
// and asserts each rendered field is present and well-formed.
//
// Validates: Requirements 10.2
// ---------------------------------------------------------------------------
TEST_F(PropertiesDialogTest, DirectoryRendersAllFields) {
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());

    // Create a subdirectory
    QString dirPath = QDir(tmpDir.path()).filePath("subdir");
    ASSERT_TRUE(QDir().mkpath(dirPath));

    PropertiesDialog dlg(dirPath);

    // Dialog should be valid
    ASSERT_TRUE(PropertiesDialogTestAccess::isValid(dlg));

    const PropertiesView& v = PropertiesDialogTestAccess::view(dlg);

    // Path: must be the absolute path we provided
    EXPECT_EQ(v.path, dirPath);

    // Type: must be "Directory"
    EXPECT_EQ(v.type, QStringLiteral("Directory"));

    // Size: must be em-dash "—" for directories
    EXPECT_EQ(v.size, QStringLiteral("\u2014"));

    // Modified: must match "yyyy-MM-dd HH:mm:ss" format
    QRegularExpression dateRegex(QStringLiteral("^\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}$"));
    EXPECT_TRUE(dateRegex.match(v.modified).hasMatch())
        << "Modified string '" << v.modified.toStdString() << "' does not match expected format";

    // Permissions: must be a 4-digit octal string
    QRegularExpression permRegex(QStringLiteral("^\\d{4}$"));
    EXPECT_TRUE(permRegex.match(v.permissions).hasMatch())
        << "Permissions string '" << v.permissions.toStdString() << "' does not match expected format";
}

// ---------------------------------------------------------------------------
// Test: PropertiesDialog for a non-existent path sets valid_ == false
//
// Validates: Requirements 10.6
// ---------------------------------------------------------------------------
TEST_F(PropertiesDialogTest, NonExistentPathIsInvalid) {
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());

    QString nonExistentPath = QDir(tmpDir.path()).filePath("does_not_exist.txt");

    PropertiesDialog dlg(nonExistentPath);

    EXPECT_FALSE(PropertiesDialogTestAccess::isValid(dlg));
}

// ---------------------------------------------------------------------------
// Test: PropertiesDialog::showFor returns false for a non-existent path
//
// Validates: Requirements 10.6
// ---------------------------------------------------------------------------
TEST_F(PropertiesDialogTest, ShowForReturnsFalseForNonExistentPath) {
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());

    QString nonExistentPath = QDir(tmpDir.path()).filePath("ghost_file.dat");

    bool result = PropertiesDialog::showFor(nonExistentPath, nullptr);
    EXPECT_FALSE(result);
}

// ---------------------------------------------------------------------------
// Test: PropertiesDialog::showFor returns true for a valid file
//
// Validates: Requirements 10.2
// ---------------------------------------------------------------------------
TEST_F(PropertiesDialogTest, ShowForReturnsTrueForValidFile) {
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());

    QString filePath = QDir(tmpDir.path()).filePath("valid_file.txt");
    {
        QFile f(filePath);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("content");
        f.close();
    }

    bool result = PropertiesDialog::showFor(filePath, nullptr);
    EXPECT_TRUE(result);
}

// ---------------------------------------------------------------------------
// Feature: fileview-context-menu, Property 13: PropertiesDialog renders all required fields
//
// For arbitrary file/directory entries created under QTemporaryDir, assert
// the dialog's rendered contents include the absolute path, the type string
// ("File" or "Directory"), a 1024-based size string with one decimal (or
// "—" for directories), a yyyy-MM-dd HH:mm:ss date, and a 4-digit octal
// permissions string.
//
// Uses RC_GTEST_FIXTURE_PROP with the QApplication fixture.
// Design § PBT iteration count recommends 50 for filesystem-touching tests;
// RapidCheck defaults to 100 — acceptable since each iteration is lightweight.
//
// **Validates: Requirements 10.2**
// ---------------------------------------------------------------------------

RC_GTEST_FIXTURE_PROP(PropertiesDialogTest,
                      RendersAllRequiredFields,
                      ()) {
    // Ensure QApplication exists (SetUpTestSuite handles this for the fixture).
    if (!QApplication::instance()) {
        static int argc = 1;
        static char appName[] = "test_properties_dialog";
        static char* argv[] = { appName, nullptr };
        new QApplication(argc, argv);
    }

    // Generate: file vs directory
    const bool isFile = *rc::gen::arbitrary<bool>();

    // Generate a random filename filtered to valid filesystem chars.
    // Use alphanumeric + underscore + hyphen + dot to avoid OS issues.
    auto validCharGen = rc::gen::oneOf(
        rc::gen::inRange(static_cast<char>('a'), static_cast<char>('z' + 1)),
        rc::gen::inRange(static_cast<char>('A'), static_cast<char>('Z' + 1)),
        rc::gen::inRange(static_cast<char>('0'), static_cast<char>('9' + 1)),
        rc::gen::element('_', '-', '.'));

    // Generate a random length between 1 and 19 characters.
    std::size_t nameLen = *rc::gen::inRange<std::size_t>(1, 20);
    std::string rawName = *rc::gen::container<std::string>(nameLen, validCharGen);
    // Ensure non-empty (generator range starts at 1, but guard anyway)
    RC_PRE(!rawName.empty());
    // Avoid filenames that are just dots (e.g. "." or "..")
    RC_PRE(rawName != "." && rawName != "..");

    QString filename = QString::fromStdString(rawName);

    // Create a temporary directory for this iteration.
    QTemporaryDir tmpDir;
    RC_ASSERT(tmpDir.isValid());

    QString entryPath;
    if (isFile) {
        entryPath = QDir(tmpDir.path()).filePath(filename);
        QFile f(entryPath);
        RC_ASSERT(f.open(QIODevice::WriteOnly));
        // Write some random content (1 to 4096 bytes)
        int contentSize = *rc::gen::inRange(1, 4097);
        QByteArray content(contentSize, 'x');
        f.write(content);
        f.close();
    } else {
        entryPath = QDir(tmpDir.path()).filePath(filename);
        RC_ASSERT(QDir().mkpath(entryPath));
    }

    // Instantiate PropertiesDialog
    PropertiesDialog dlg(entryPath);

    // Dialog must be valid for an existing entry
    RC_ASSERT(PropertiesDialogTestAccess::isValid(dlg));

    const PropertiesView& v = PropertiesDialogTestAccess::view(dlg);

    // 1. Path: must equal the absolute path we created
    RC_ASSERT(v.path == entryPath);

    // 2. Type: must be "File" or "Directory" matching what we created
    if (isFile) {
        RC_ASSERT(v.type == QStringLiteral("File"));
    } else {
        RC_ASSERT(v.type == QStringLiteral("Directory"));
    }

    // 3. Size: for files, must match "\\d+\\.\\d+ [KMGT]?B" (1024-based, 1 decimal)
    //          for directories, must be em-dash "—"
    if (isFile) {
        QRegularExpression sizeRegex(QStringLiteral("^\\d+\\.\\d+ [KMGT]?B$"));
        RC_ASSERT(sizeRegex.match(v.size).hasMatch());
    } else {
        RC_ASSERT(v.size == QStringLiteral("\u2014"));
    }

    // 4. Modified: must match "yyyy-MM-dd HH:mm:ss" format
    QRegularExpression dateRegex(QStringLiteral("^\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}$"));
    RC_ASSERT(dateRegex.match(v.modified).hasMatch());

    // 5. Permissions: must be a 4-digit octal string (e.g. "0755")
    QRegularExpression permRegex(QStringLiteral("^\\d{4}$"));
    RC_ASSERT(permRegex.match(v.permissions).hasMatch());
}
