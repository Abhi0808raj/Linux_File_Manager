#include <gtest/gtest.h>
// <gtest/gtest.h> transitively includes <cassert>, which defines assert() as
// a C macro. That macro conflicts with the ErrorHandler::assert() static
// template method defined in error_handler.hpp. Undefine it here so the
// header parses correctly and our tests can call ErrorHandler::assert().
#ifdef assert
#  undef assert
#endif
// Logger must be fully defined before error_handler.hpp so that
// unique_ptr<Logger>'s destructor can be instantiated (it's forward-declared
// in error_handler.hpp but setLogger(nullptr) requires the complete type).
#include "utilities/logger.hpp"
#include "utilities/error_handler.hpp"

class ErrorHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        ErrorHandler::instance().setErrorCallback(nullptr);
        ErrorHandler::instance().setLogger(nullptr);
    }
    void TearDown() override {
        ErrorHandler::instance().setErrorCallback(nullptr);
        ErrorHandler::instance().setLogger(nullptr);
    }
};

// --- Result<T>: success path ---

TEST_F(ErrorHandlerTest, ResultSuccessIsSuccess) {
    auto r = ErrorHandler::Result<int>(42);
    EXPECT_TRUE(r.isSuccess());
    EXPECT_FALSE(r.isFailure());
}

TEST_F(ErrorHandlerTest, ResultSuccessValueReturnsStoredValue) {
    auto r = ErrorHandler::Result<int>(99);
    EXPECT_EQ(r.value(), 99);
}

TEST_F(ErrorHandlerTest, ResultMoveValueExtractsString) {
    auto r = ErrorHandler::Result<std::string>(std::string("hello"));
    EXPECT_EQ(std::move(r).moveValue(), "hello");
}

// --- Result<T>: failure path ---

TEST_F(ErrorHandlerTest, ResultFailureIsFailure) {
    auto r = ErrorHandler::Result<int>(
        ErrorHandler::ErrorCode::FILE_NOT_FOUND, "not found");
    EXPECT_FALSE(r.isSuccess());
    EXPECT_TRUE(r.isFailure());
}

TEST_F(ErrorHandlerTest, ResultFailureStoresErrorCode) {
    auto r = ErrorHandler::Result<int>(
        ErrorHandler::ErrorCode::PERMISSION_DENIED, "denied");
    EXPECT_EQ(r.errorCode(), ErrorHandler::ErrorCode::PERMISSION_DENIED);
}

TEST_F(ErrorHandlerTest, ResultFailureStoresErrorMessage) {
    auto r = ErrorHandler::Result<std::string>(
        ErrorHandler::ErrorCode::UNKNOWN_ERROR, "oops");
    EXPECT_EQ(r.errorMessage(), "oops");
}

TEST_F(ErrorHandlerTest, ResultValueThrowsOnFailure) {
    auto r = ErrorHandler::Result<int>(
        ErrorHandler::ErrorCode::UNKNOWN_ERROR, "fail");
    EXPECT_THROW(r.value(), FileManagerException);
}

// --- safeExecute ---

TEST_F(ErrorHandlerTest, SafeExecuteReturnsSuccessForNormalCallable) {
    auto r = ErrorHandler::safeExecute([]() { return 7; }, "op");
    EXPECT_TRUE(r.isSuccess());
    EXPECT_EQ(r.value(), 7);
}

TEST_F(ErrorHandlerTest, SafeExecuteReturnsFailureOnStdException) {
    auto r = ErrorHandler::safeExecute([]() -> int {
        throw std::runtime_error("boom");
    }, "risky_op");
    EXPECT_TRUE(r.isFailure());
    EXPECT_EQ(r.errorCode(), ErrorHandler::ErrorCode::UNKNOWN_ERROR);
}

TEST_F(ErrorHandlerTest, SafeExecuteReturnsFailureOnUnknownException) {
    auto r = ErrorHandler::safeExecute([]() -> int {
        throw 42;
    }, "unknown_throw");
    EXPECT_TRUE(r.isFailure());
    EXPECT_EQ(r.errorCode(), ErrorHandler::ErrorCode::UNKNOWN_ERROR);
}

// --- Exception hierarchy ---

TEST_F(ErrorHandlerTest, RaiseErrorThrowsFileManagerException) {
    EXPECT_THROW(
        ErrorHandler::raiseError<FileManagerException>("test"),
        FileManagerException);
}

TEST_F(ErrorHandlerTest, RaiseErrorThrowsFileSystemExceptionCastableToBase) {
    EXPECT_THROW(
        ErrorHandler::raiseError<FileSystemException>("fs"),
        FileManagerException);
}

TEST_F(ErrorHandlerTest, RaiseErrorThrowsPluginException) {
    EXPECT_THROW(
        ErrorHandler::raiseError<PluginException>("plugin"),
        PluginException);
}

TEST_F(ErrorHandlerTest, ExceptionPreservesMessage) {
    try {
        throw FileManagerException("stored message");
    } catch (const FileManagerException& e) {
        EXPECT_STREQ(e.what(), "stored message");
        EXPECT_EQ(e.message(), "stored message");
    }
}

// --- assert ---

TEST_F(ErrorHandlerTest, AssertDoesNotThrowOnTrueCondition) {
    EXPECT_NO_THROW(ErrorHandler::assert(true, "ok"));
}

TEST_F(ErrorHandlerTest, AssertThrowsOnFalseCondition) {
    EXPECT_THROW(ErrorHandler::assert(false, "bad"), FileManagerException);
}

// --- callback ---

TEST_F(ErrorHandlerTest, CallbackReceivesCorrectSeverityAndMessage) {
    ErrorSeverity gotSeverity = ErrorSeverity::INFO;
    std::string gotMessage;
    ErrorHandler::instance().setErrorCallback(
        [&](ErrorSeverity s, const std::string& m) {
            gotSeverity = s;
            gotMessage = m;
        });
    ErrorHandler::instance().warning("ping");
    EXPECT_EQ(gotSeverity, ErrorSeverity::WARNING);
    EXPECT_EQ(gotMessage, "ping");
}
