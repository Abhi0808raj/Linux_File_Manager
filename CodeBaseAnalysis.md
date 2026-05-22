# Codebase Analysis & Production Roadmap
## Qt C++ Cross-Platform File Manager

> **Document Date:** 2026-05-19
> **Analyzed Revision:** `4b0702b` (branch: `main`)
> **Analyst:** Claude Code (Sonnet 4.6)
> **Target Platforms:** Linux · Windows · macOS

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Architecture Overview](#2-architecture-overview)
3. [Module-by-Module Analysis](#3-module-by-module-analysis)
4. [Cross-Platform Readiness](#4-cross-platform-readiness)
5. [Known Bugs & Code Quality Issues](#5-known-bugs--code-quality-issues)
6. [Production Readiness Checklist](#6-production-readiness-checklist)
7. [Phased Roadmap](#7-phased-roadmap)

---

## 1. Executive Summary

This project is a **Qt6/C++17 cross-platform file manager** with a plugin architecture for extensible file operations. It is in active early development with a working GUI shell, a solid filesystem abstraction layer, a functional plugin loading system, and a professional error handling framework.

### What Works Today

| Component | Status |
|---|---|
| Qt6 main window with toolbar, sidebar, table view | Working |
| Back / Forward / Up navigation with history | Working |
| Directory listing via `QFileSystemModel` | Working |
| Double-click to open directories and files | Working |
| Status bar (file/dir count) | Working |
| `FileSystem` class (copy, move, delete, read/write, metadata) | Working |
| Plugin loader (dynamic `.so` / `.dll` loading) | Working |
| Copy, Move, Delete plugins | Working (in isolation) |
| Error handler with typed exceptions and `Result<T>` | Working |
| Thread-safe logger with timestamps | Working |
| CMake build system with modular test flags | Working |
| Modern light stylesheet (Google Material-inspired) | Working |

### What Is Broken or Missing

- **GUI and plugin system are completely disconnected.** No user action in the UI invokes any plugin or `FileSystem` operation. The app can browse directories but cannot perform any file operation through the interface.
- **`FileView` is a placeholder.** The class exists but contains only a label widget — it is not used in the main window.
- **`error_handler.hpp` has duplicate method declarations** (`warning`, `critical`) that will cause compile errors in strict mode.
- **`Logger` and `ErrorHandler` are not connected.** The logger writes to a file; the error handler writes to `stderr`. They are independent and cannot be configured together.
- **`test.cpp` is empty** and is compiled into the core library, polluting the build.
- **Sidebar navigation is fully hardcoded** — only Home, Desktop, Downloads, Bookmarks, and Filesystem are supported, with no dynamic drives or bookmarks.
- **No right-click context menu** of any kind.
- **No rename, new folder, delete, or cut/copy/paste** from the UI.
- **No search functionality.**
- **No file icons** per file type in the table view.
- **No undo/redo** for any operation.
- **Vendor/company metadata is placeholder** (`"Your Company"`, `"yourcompany"`).
- **No CI/CD pipeline, no release packaging.**

### Production Readiness Score

```
Overall:  ~22% toward a deployable v1.0
  Core Logic:        ████████░░░░░░░░  50%
  GUI / UX:          ████░░░░░░░░░░░░  25%
  Plugin Integration: ██░░░░░░░░░░░░░░  10%
  Testing:           ███░░░░░░░░░░░░░  15%
  Packaging/Release: ░░░░░░░░░░░░░░░░   0%
```

The foundation is genuinely well-designed. The architecture decisions (plugin interface, `Result<T>` wrapper, static `FileSystem` utility, clean header/source separation) are professional-grade. The path to v1.0 is clear, but significant work remains.

---

## 2. Architecture Overview

### Intended Layered Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    app/main.cpp                         │
│              (Entry point, QApplication)                │
└─────────────────────────┬───────────────────────────────┘
                          │
┌─────────────────────────▼───────────────────────────────┐
│                  GUI Layer                              │
│  MainWindow  ←→  FileView  ←→  (dialogs, panels)       │
│        [include/gui/]  [file_manager/gui/]              │
└──────────────┬──────────────────────────────────────────┘
               │  (currently NO connection here ↓)
┌──────────────▼──────────────────────────────────────────┐
│                  Core Layer                             │
│   FileSystem (static utility)   PluginManager          │
│        [include/core/]  [file_manager/core/]            │
└──────────────┬──────────────────┬──────────────────────┘
               │                  │
┌──────────────▼──────┐  ┌────────▼────────────────────  ┐
│  Utilities Layer    │  │   Plugin Layer                 │
│  ErrorHandler       │  │   IFileManagerPlugin (ABI)     │
│  Logger             │  │   copy_plugin.so               │
│  [include/utilities]│  │   move_plugin.so               │
└─────────────────────┘  │   delete_plugin.so             │
                         │   [plugins/basic_operations/]  │
                         └────────────────────────────────┘
```

### Current Reality

The GUI layer has **zero wiring** to the Core layer. `MainWindow` only uses `QFileSystemModel` (Qt's built-in) and `QDir` for display. `FileSystem`, `PluginManager`, and all plugins are compiled but never instantiated from the UI.

### Build System Structure

```
CMakeLists.txt (root)
├── file_manager_core  (STATIC library)
│   ├── file_manager/core/*.cpp
│   ├── file_manager/gui/*.cpp
│   └── file_manager/utilities/*.cpp
├── file_manager  (executable)
│   ├── app/main.cpp
│   ├── resources/resources.qrc
│   └── links → file_manager_core
├── plugins/basic_operations/  (3 SHARED libraries)
│   ├── copy_plugin.so
│   ├── move_plugin.so
│   └── delete_plugin.so
├── plugins/example_plugin/  (SHARED library)
└── tests/  (4 independent test targets, flag-gated)
```

**Build issue:** `file_manager/core/test.cpp` (empty file) is included in `CORE_SOURCES` via `GLOB_RECURSE` and compiled into `file_manager_core`. This is unintentional.

---

## 3. Module-by-Module Analysis

### 3.1 `FileSystem` — `include/core/file_system.hpp` / `file_manager/core/file_system.cpp`

**Purpose:** Static utility class wrapping `std::filesystem` for all file and directory operations.

**Strengths:**
- Clean static interface — easy to call from anywhere without instantiation.
- Uses `std::error_code` throughout to avoid exception-based control flow.
- `std::optional<T>` return types correctly model operations that may return no value.
- Covers all fundamental operations: `exists`, `isDirectory`, `isFile`, `listDirectory`, `createDirectory`, `remove`, `copy`, `move`, `fileSize`, `lastWriteTime`, `readFile`, `writeFile`.

**Issues:**

| File | Line | Issue |
|---|---|---|
| `file_system.cpp` | 49 | `fs::create_directory(path, ec) > 0` — `create_directory` returns `bool`, not a count. This comparison works but is semantically wrong and will cause a compiler warning. Should be `fs::create_directories(path, ec)` (note plural) to support nested paths. |
| `file_system.cpp` | 35–38 | Errors in `listDirectory` are printed to `std::cerr` directly, bypassing the `ErrorHandler`. |
| `file_system.cpp` | All | All error paths print to `std::cerr` directly instead of routing through `FM_ERROR` / `FM_WARNING` macros. |
| `file_system.hpp` | — | No support for symlinks, hidden files filter, or permissions querying — all needed for a production file manager. |
| `file_system.cpp` | 71–84 | `copy` does not handle directory-recursive copying. `fs::copy` with `copy_options::none` will fail silently on directories unless `copy_options::recursive` is added. |

---

### 3.2 `PluginManager` — `include/core/plugin_manager.hpp` / `file_manager/core/plugin_manager.cpp`

**Purpose:** Dynamically loads `.so`/`.dll` plugin files at runtime, manages their lifecycle, and provides name-based lookup.

**Strengths:**
- Correct use of `dlopen`/`FreeLibrary` with proper `RTLD_LAZY | RTLD_LOCAL` flags.
- `unique_ptr<IFileManagerPlugin>` for plugin instance ownership — no memory leaks.
- Copy/move semantics disabled on `PluginManager` — correct for a resource-owning class.
- Name-to-pointer map for O(1) plugin lookup.
- Proper unload order: instance destroyed before `dlclose`.

**Issues:**

| File | Line | Issue |
|---|---|---|
| `plugin_manager.cpp` | 47–57 | `plugins()` returns a reference to a `static` local vector. This is not thread-safe — concurrent calls will race on `pluginPtrs.clear()`. Should return by value or use a member vector. |
| `plugin_manager.cpp` | — | No version compatibility check. A plugin built against an older `IFileManagerPlugin` ABI will crash at runtime with no useful error. |
| `plugin_manager.hpp` | — | No `reloadPlugin(name)` method — needed for development and future hot-reload support. |
| `plugin_manager.cpp` | — | Plugin directory path is hardcoded at call sites. Should have a configurable default (e.g., next to the executable). |
| Root `CMakeLists.txt` | 94–97 | Plugins are built to `${CMAKE_BINARY_DIR}/plugins/` but `PluginManager::loadPlugins()` is never called with this path from `main.cpp`. The plugins exist but are never loaded at runtime. |

---

### 3.3 `ErrorHandler` — `include/utilities/error_handler.hpp` / `file_manager/utilities/error_handler.cpp`

**Purpose:** Singleton error handling framework with typed exceptions, a `Result<T>` monad, logging macros, and `safeExecute` wrapper.

**Strengths:**
- Well-designed `Result<T>` template avoids exception-based control flow for expected failures.
- Variadic template macros (`FM_ERROR`, `FM_WARNING`, etc.) are clean and ergonomic.
- `safeExecute` wrapper pattern is excellent for wrapping risky operations with automatic logging.
- `handleSystemError` correctly maps `errno` values to typed exceptions.
- Singleton with deleted copy/move constructors — correct pattern.

**Issues:**

| File | Line | Issue |
|---|---|---|
| `error_handler.hpp` | 177–180 | `warning(const TArgs&... args)` declared twice — once as a non-forwarding template (line 177) and once as a forwarding-reference template (line 204). This is a **redefinition error** that will fail to compile with certain compilers or ODR-checking tools. |
| `error_handler.hpp` | 185–189 | `critical(const TArgs&... args)` declared twice — same issue. The second `critical` (line 214) calls `critical(format(...))` which calls itself — **infinite recursion**. |
| `error_handler.hpp` | 199–201 | `info()` routes through `logError(ErrorSeverity::WARNING, ...)` — an INFO-level message is incorrectly logged as WARNING severity. Needs an `INFO` entry in the `ErrorSeverity` enum. |
| `error_handler.cpp` | 64–80 | `logError` writes only to `std::cerr` and an optional callback. It does not write to the `Logger`. The two systems are completely decoupled. |
| `error_handler.hpp` | 99–108 | `ErrorCode` enum is defined but `OUT_OF_MEMORY` and `GUI_LOAD_FAILED` are never used anywhere in the codebase. |

---

### 3.4 `Logger` — `include/utilities/logger.hpp` / `file_manager/utilities/logger.cpp`

**Purpose:** Thread-safe file logger with timestamps and severity labels.

**Strengths:**
- `std::lock_guard<std::mutex>` for correct thread safety.
- Append mode on log file — does not destroy previous sessions.
- Timestamp format is ISO 8601-like and machine-parseable.
- RAII file management — destructor closes the file.

**Issues:**

| File | Line | Issue |
|---|---|---|
| `logger.hpp` | — | `Logger` is never instantiated anywhere in the running application. It exists but is unused at runtime. |
| `logger.hpp` | — | No `INFO` severity level — the `ErrorSeverity` enum used by Logger only has `WARNING`, `ERROR`, `CRITICAL`. Normal operations cannot be logged. |
| `logger.cpp` | 33 | `std::localtime` is not thread-safe on all platforms. Should use `localtime_r` (POSIX) or `localtime_s` (Windows) for a cross-platform solution. |
| `logger.hpp` | — | Log file path defaults to `"file_manager.log"` — now resolved to predictable platform-appropriate location (`QStandardPaths::AppDataLocation`) at launch for relative paths. |
| `logger.hpp` | — | No log rotation — a long-running instance will grow the log file indefinitely. |

---

### 3.5 `MainWindow` — `include/gui/main_window.hpp` / `file_manager/gui/main_window.cpp`

**Purpose:** Qt main window — the primary user interface.

**Strengths:**
- Clean separation of `setupModernUI()`, `setupModels()`, `setupConnections()` in the constructor.
- Navigation history with back/forward correctly managed with `QList` and an index.
- `QFileSystemModel` used correctly with `QDir::Hidden` to show hidden files.
- Status bar updates on directory load signal.
- Double-click correctly distinguishes files vs. directories.

**Issues:**

| File | Line | Issue |
|---|---|---|
| `main_window.cpp` | 173–196 | Sidebar navigation locations (`"Home"`, `"Desktop"`, `"Downloads"`, `"Bookmarks"`, `"Filesystem"`) are matched by string comparison against hardcoded labels. Adding a new location requires changes in both the `.ui` file and this switch block — fragile. |
| `main_window.cpp` | 183 | `"Bookmarks"` navigates to `QDir::homePath()` — this is a stub, not real bookmark functionality. |
| `main_window.cpp` | — | No right-click `QMenu` / context menu at all. |
| `main_window.cpp` | — | No keyboard shortcuts (F2 rename, Delete key, Ctrl+C/X/V, F5 refresh). |
| `main_window.cpp` | — | `QFileSystemModel` shows no custom icons per file type — all entries use Qt default icons. |
| `main_window.cpp` | — | `PluginManager` is never instantiated or connected to any UI action. |
| `main_window.cpp` | 108 | `m_historyIndex < m_history.size() - 1` — `m_historyIndex` is `int`, `m_history.size()` returns `qsizetype` (signed 64-bit). Comparison is safe but mixed-sign comparisons will produce warnings. |
| `app/main.cpp` | 13 | `app.setOrganizationName("Your Company")` — placeholder not replaced. |

---

### 3.6 `FileView` — `include/gui/file_view.hpp` / `file_manager/gui/file_view.cpp`

**Purpose:** Intended as a reusable file view component (likely for a dual-pane or tabbed view).

**Current State:** Completely unimplemented. The `.cpp` contains only a `QLabel("FileView placeholder")`. The class is compiled into `file_manager_core` but is never instantiated or used anywhere.

**What it should become:** A self-contained widget encapsulating `QFileSystemModel` + `QTableView` (or `QListView` + `QTableView` toggle), selection state, and context menu — so `MainWindow` can host one or two `FileView` instances for single/dual-pane modes.

---

### 3.7 Plugins — `plugins/basic_operations/` · `plugins/example_plugin/`

**Purpose:** Demonstrate and provide the core file operations through the plugin ABI.

**Strengths:**
- All three plugins (`CopyPlugin`, `MovePlugin`, `DeletePlugin`) correctly implement `IFileManagerPlugin`.
- Each exports `create_plugin()` with C linkage.
- They delegate to `FileSystem::copy/move/remove` — no duplicated logic.
- `metadata.json` exists per plugin for self-description.

**Issues:**

| File | Issue |
|---|---|
| All plugins | `metadata.json` contains `"yourcompany"` and `"Your Company Name"` placeholders. |
| `example_plugin` | Has no `.cpp` source — only a header. It is a header-only stub that cannot be built into a working shared library as-is. |
| All plugins | `metadata.json` is never read by `PluginManager` — version and compatibility information is ignored at load time. |
| `IFileManagerPlugin` | No `initialize(config)` / `shutdown()` lifecycle methods — plugins cannot receive configuration or perform cleanup. |
| Plugin ABI | No versioning in `IFileManagerPlugin` — a plugin built against v1 of the interface and loaded against v2 will silently misbehave or crash. |

---

## 4. Cross-Platform Readiness

### Current Status

| Platform | Compiles | Runs | Packaged |
|---|---|---|---|
| Linux (X11/Wayland) | Yes | Yes | No |
| Windows (MSVC/MinGW) | Likely | Untested | No |
| macOS | Likely | Untested | No |

### Platform-Specific Assessment

**Linux**
- The codebase uses `dlfcn.h` correctly for plugin loading on Linux.
- No hard-coded POSIX paths in core logic (uses `std::filesystem` and `QDir`).
- Missing: `.desktop` file for application menu integration, AppStream metadata, icon in standard resolutions.
- Packaging: AppImage is the easiest first target; `.deb` and AUR require more work.

**Windows**
- `plugin_manager.hpp` correctly `#ifdef`s `windows.h` and `HMODULE`.
- `LoadLibraryW` takes a wide-character path — correct for Unicode file paths.
- Missing: `WinMain` entry point is not needed (Qt handles this), but a Windows manifest file (`.rc`) is needed for proper UAC and DPI awareness.
- `logger.cpp` uses `std::localtime` which is thread-safe on Windows (unlike Linux) but should still be replaced with `localtime_s` for consistency.
- Missing: NSIS or WiX installer, Windows application icon (`.ico`).

**macOS**
- No macOS-specific code exists anywhere — it will use the Linux code path.
- `dlopen`/`dlsym` are available on macOS so plugin loading will work.
- Missing: `.app` bundle structure, `Info.plist`, `.icns` icon, notarization requirements for Gatekeeper, macOS-specific sidebar items (iCloud, AirDrop).
- Qt6 on macOS requires code signing for distribution.

### What Needs to Happen for All Three Platforms

1. Replace `std::localtime` with a thread-safe cross-platform wrapper in `logger.cpp`.
2. Add a platform-appropriate default log file path (using `QStandardPaths`).
3. Add platform-appropriate plugin search path (next to executable, or `~/.config/file_manager/plugins/`).
4. Add CMake install targets for each platform (already started but incomplete).
5. Set up GitHub Actions CI with Linux, Windows, and macOS build matrix.

---

## 5. Known Bugs & Code Quality Issues

### Critical (Will cause crashes or compile failures)

| # | File | Description |
|---|---|---|
| C1 | `error_handler.hpp:177,204` | `warning()` defined twice — redefinition. Will fail to compile with ODR-checking or MSVC. |
| C2 | `error_handler.hpp:185,214` | `critical()` defined twice. The second overload calls itself via `critical(format(...))` — **infinite recursion and stack overflow at runtime**. |
| C3 | `plugin_manager.cpp:47` | `plugins()` returns a reference to a `static` local vector — **data race** under concurrent access. |

### Major (Wrong behavior, silent failures)

| # | File | Description |
|---|---|---|
| M1 | `file_system.cpp:49` | `fs::create_directory` (singular) used instead of `fs::create_directories` — creating nested paths (e.g., `a/b/c`) silently fails if parent doesn't exist. |
| M2 | `file_system.cpp:71` | `FileSystem::copy` does not pass `copy_options::recursive` — copying directories silently fails. |
| M3 | `error_handler.hpp:199` | `info()` logs at `WARNING` severity — INFO messages appear as warnings in the log. |
| M4 | `logger.cpp:33` | `std::localtime` is not thread-safe on Linux/macOS — concurrent logging can produce garbled timestamps or crash. |
| M5 | `CMakeLists.txt:33-35` | `GLOB_RECURSE` on `file_manager/core/` picks up `test.cpp` (empty file) and compiles it into `file_manager_core` — pollutes the production library with test code. |

### Minor (Code quality, warnings, maintainability)

| # | File | Description |
|---|---|---|
| m1 | `main_window.cpp:108` | Mixed signed/unsigned comparison (`int` vs `qsizetype`) — compiler warning. |
| m2 | `main_window.cpp:173` | Sidebar location matching by string comparison — fragile, breaks if UI labels change. Use an enum or `QVariant` data on tree items. |
| m3 | `app/main.cpp:13` | `"Your Company"` placeholder left in `setOrganizationName`. |
| m4 | `plugins/*/metadata.json` | All metadata files contain placeholder vendor information. |
| m5 | `file_system.cpp:all` | All error paths bypass `ErrorHandler` and write directly to `std::cerr`. |
| m6 | `error_handler.hpp:99` | `OUT_OF_MEMORY`, `GUI_LOAD_FAILED` error codes defined but never used. |
| m7 | `include/core/file_system.hpp:all` | Excessive inline comments explaining standard library basics — appropriate for learning, should be reduced before public release. |
| m8 | `file_manager/gui/file_view.cpp` | Entirely placeholder — stub widget with a label. |
| m9 | Root `CMakeLists.txt` | Two separate build directories exist (`build/` and `cmake-build-debug/`) — should be one. Add both to `.gitignore`. |

---

## 6. Production Readiness Checklist

### Core Functionality

| Feature | Status |
|---|---|
| Browse filesystem | Done |
| Navigate (back/forward/up) | Done |
| Open files with system default app | Done |
| Copy files/directories | Not wired to UI |
| Move / rename files | Not wired to UI |
| Delete files | Not wired to UI |
| Create new folder | Not implemented |
| Right-click context menu | Not implemented |
| Keyboard shortcuts (F2, Del, Ctrl+C/X/V) | Not implemented |
| Cut/Copy/Paste with clipboard | Not implemented |
| Drag and drop | Not implemented |
| Undo / Redo | Not implemented |
| File search / filter | Not implemented |
| Bookmarks / favorites | Stub only |
| Hidden files toggle | Done (model configured) |

### UI / UX

| Feature | Status |
|---|---|
| File type icons in list | Not implemented |
| File preview panel | Not implemented |
| Sort by name/size/date/type | Done (QTableView) |
| Column resizing | Done |
| Dual-pane view | Not implemented |
| Tabs | Not implemented |
| Address bar (editable path) | Not implemented |
| Breadcrumb navigation | Not implemented |
| Dark mode / theme switching | Not implemented (light only) |
| Zoom (icon size toggle list/grid view) | Not implemented |

### Quality & Stability

| Feature | Status |
|---|---|
| Unit tests for FileSystem | Framework exists, needs expansion |
| Unit tests for PluginManager | Framework exists, needs expansion |
| Unit tests for ErrorHandler | Framework exists, needs expansion |
| Integration tests (GUI + core) | Not implemented |
| No known crash-level bugs | Failing (C1, C2, C3 above) |
| ErrorHandler + Logger connected | Not connected |
| Thread-safe logging | Partial (mutex present, localtime unsafe) |

### Packaging & Distribution

| Feature | Status |
|---|---|
| Linux AppImage | Not implemented |
| Linux .deb package | Not implemented |
| Linux AUR PKGBUILD | Not implemented |
| Windows installer (NSIS/WiX) | Not implemented |
| macOS .dmg / .app bundle | Not implemented |
| Application icon (all platforms) | Not implemented |
| CI/CD pipeline (GitHub Actions) | Not implemented |
| Versioning strategy (semver) | Not implemented |
| Release notes / changelog | Not implemented |
| Code signing (Windows/macOS) | Not implemented |

---

## 7. Phased Roadmap

### Phase 1 — Stability & Quality
**Goal:** A crash-free, correctly-behaving codebase with a solid test foundation. No new features until this is done.
**Estimated scope:** 2–3 weeks

#### 1.1 Fix Critical Bugs
- [x] Fix duplicate `warning()` and `critical()` declarations in `error_handler.hpp` (remove the second, forwarding-ref overloads or merge them)
- [x] Fix `critical()` infinite recursion in `error_handler.hpp`
- [x] Fix `plugins()` data race in `plugin_manager.cpp` — return by value or use a member vector
- [x] Fix `FileSystem::createDirectory` — switch to `fs::create_directories`
- [x] Fix `FileSystem::copy` — add `copy_options::recursive` for directory support
- [x] Fix `Logger::log` — replace `std::localtime` with `localtime_r` / `localtime_s` cross-platform wrapper
- [x] Fix `ErrorHandler::info()` — add `INFO` to `ErrorSeverity` enum, route correctly

#### 1.2 Clean Up Build System
- [x] Remove `file_manager/core/test.cpp` from `GLOB_RECURSE` scope (move tests to `tests/` or exclude explicitly)
- [x] Consolidate to one build directory; update `.gitignore`
- [x] Fix `PluginConfig.cmake` include path (`../include` is fragile relative path)

#### 1.3 Connect Logger ↔ ErrorHandler
- [x] Add `INFO` severity to `ErrorSeverity` enum
- [x] Instantiate `Logger` as a member of (or dependency injected into) `ErrorHandler`
- [x] Route all `logError` calls through `Logger` in addition to `stderr`
- [x] Configure log file path using `QStandardPaths::AppDataLocation`

#### 1.4 Route FileSystem through ErrorHandler
- [x] Replace all `std::cerr` calls in `file_system.cpp` with `FM_ERROR` / `FM_WARNING` macros

#### 1.5 Expand Tests (Unit + Integration)
- [x] Choose and add a testing framework (GoogleTest or Catch2) to CMake
- [x] Unit tests for `FileSystem`: copy, move, delete, createDirectory (nested), edge cases
- [x] Unit tests for `ErrorHandler`: `Result<T>`, `safeExecute`, exception types
- [x] Unit tests for `Logger`: thread-safe concurrent writes, severity labels, timestamps
- [x] Unit tests for `PluginManager`: load valid plugin, reject invalid `.so`, name lookup
- [x] Integration test: load `copy_plugin.so` → call `execute()` → verify file copied on disk
- [x] Set up CMake `enable_testing()` and `ctest` integration
- [x] Set up GitHub Actions CI (Linux build + test on every push)

#### 1.6 Replace Placeholders
- [x] Replace `"Your Company"` / `"yourcompany"` in `main.cpp` and `metadata.json` files
- [ ] Fill in `example_plugin` with a real `.cpp` source file or document it as a template

---

### Phase 2 — Core UX
**Goal:** The application behaves like a real file manager. A user can do everything they'd expect: browse, copy, move, rename, delete, create folders, and open files — all from the keyboard or mouse.
**Estimated scope:** 4–6 weeks

#### 2.1 Refactor FileView
- [ ] Implement `FileView` as a self-contained widget: `QFileSystemModel` + `QTableView` + `QListView` (toggle)
- [ ] Move file-model logic out of `MainWindow` into `FileView`
- [ ] `MainWindow` hosts a `FileView` instance (prep for dual-pane: host two)

#### 2.2 Context Menu
- [ ] Add `customContextMenuRequested` signal handler to `FileView`
- [ ] Context menu actions: Open, Open With, Copy, Cut, Paste, Rename, Delete, New Folder, Properties
- [ ] Disable irrelevant actions based on selection state (nothing selected, read-only path, etc.)

#### 2.3 File Operations from UI
- [ ] Wire Copy / Cut / Paste to clipboard with `QClipboard` + internal selection tracking
- [ ] Wire Delete to `FileSystem::remove` (with confirmation dialog)
- [ ] Wire Rename to inline editing or `QInputDialog`
- [ ] Wire New Folder to `FileSystem::createDirectory` + inline rename

#### 2.4 Keyboard Shortcuts
- [ ] F2 → Rename
- [ ] Delete → Delete (with confirmation)
- [ ] Ctrl+C / Ctrl+X / Ctrl+V → Copy / Cut / Paste
- [ ] F5 → Refresh
- [ ] Alt+Left / Alt+Right → Back / Forward
- [ ] Alt+Up → Up
- [ ] Ctrl+L → Focus address bar (once address bar exists)
- [ ] Ctrl+H → Toggle hidden files

#### 2.5 Address Bar
- [ ] Add an editable `QLineEdit` path bar between toolbar and file view
- [ ] Clicking a directory in the view updates the address bar
- [ ] Typing a path in the address bar and pressing Enter navigates to it
- [ ] Breadcrumb-style display (clickable path segments) as a stretch goal

#### 2.6 File Icons
- [ ] Use `QFileIconProvider` to display per-file-type icons in the table view
- [ ] Replace placeholder SVG icons in `resources/icons/` with a complete icon set (or use system icons via `QIcon::fromTheme`)

#### 2.7 Sidebar Enhancement
- [ ] Replace hardcoded string matching with enum-based or `QVariant`-tagged tree items
- [ ] Dynamically detect and list mounted drives / volumes (using `QStorageInfo`)
- [ ] Persist and restore user bookmarks (using `QSettings`)
- [ ] Drag a folder onto the sidebar to add a bookmark

#### 2.8 Search
- [ ] Add a search bar (Ctrl+F) that filters the current directory view
- [ ] Live filter as user types (filter on `QFileSystemModel` via `QSortFilterProxyModel`)
- [ ] Recursive search as a separate dialog/panel (stretch goal)

#### 2.9 Properties Dialog
- [ ] Right-click → Properties shows: full path, size, type, permissions, created/modified dates
- [ ] On Linux: show octal permissions with a toggle UI

#### 2.10 Undo / Redo
- [ ] Implement a `FileOperation` command class (copy, move, delete, rename each as a reversible command)
- [ ] `QUndoStack` for undo/redo history
- [ ] Ctrl+Z / Ctrl+Y for undo/redo

---

### Phase 3 — Plugin Integration
**Goal:** The plugin architecture is fully proven end-to-end. The GUI loads plugins at startup and exposes plugin actions to the user. Third-party developers can write a plugin following the documented ABI.
**Estimated scope:** 2–3 weeks

#### 3.1 Wire PluginManager to Application Startup
- [ ] Instantiate `PluginManager` in `MainWindow` (or an application-level controller)
- [ ] Call `loadPlugins` with the platform-appropriate plugin directory at startup
- [ ] Log loaded plugins to status bar or a startup splash

#### 3.2 Add Plugin ABI Versioning
- [ ] Add `virtual int apiVersion() const = 0` to `IFileManagerPlugin`
- [ ] `PluginManager::loadPluginFile` checks ABI version before registering — rejects incompatible plugins with a clear error

#### 3.3 Plugin Lifecycle Methods
- [ ] Add `virtual bool initialize(const PluginContext& ctx) = 0` and `virtual void shutdown() = 0` to the interface
- [ ] `PluginContext` provides access to current path, selected files, and app config
- [ ] `PluginManager` calls `initialize` on load and `shutdown` on unload

#### 3.4 Read Plugin Metadata
- [ ] `PluginManager` reads `metadata.json` alongside each `.so` / `.dll` at load time
- [ ] Metadata drives display name, version, and description shown in the Plugin Manager UI

#### 3.5 Plugin Manager UI Panel
- [ ] Add a Plugins menu or Settings > Plugins panel
- [ ] Shows all loaded plugins: name, version, status (loaded / failed), enable/disable toggle
- [ ] Clicking a plugin shows its description and supported operations

#### 3.6 Expose Plugin Operations in Context Menu
- [ ] Context menu dynamically adds a "Plugins" submenu listing applicable plugin operations
- [ ] Selecting a plugin operation calls `plugin->execute(operation, selectedFilePaths)`
- [ ] Result (`bool`) reported as status bar message or toast notification

#### 3.7 Update Plugin Documentation
- [ ] Write a `PLUGIN_DEVELOPMENT.md` guide with the full ABI contract, versioning rules, and a step-by-step tutorial using `example_plugin` as the base

---

### Phase 4 — Packaging & Release
**Goal:** A tagged v1.0 release downloadable by anyone on Linux, Windows, and macOS.
**Estimated scope:** 2–3 weeks

#### 4.1 Application Identity
- [ ] Choose and set a final application name, version (1.0.0 semver), organization, and icon
- [ ] Create application icons: `.ico` (Windows), `.icns` (macOS), `.png` at 16/32/48/64/128/256px (Linux)
- [ ] Replace all placeholder metadata throughout the codebase

#### 4.2 CI/CD Pipeline (GitHub Actions)
- [ ] Build + test matrix: `ubuntu-latest`, `windows-latest`, `macos-latest`
- [ ] Cache Qt install across runs
- [ ] Upload build artifacts on every push to `main`
- [ ] Create a release workflow triggered by version tags (`v*.*.*`)

#### 4.3 Linux Packaging
- [ ] AppImage: use `linuxdeploy` + `linuxdeploy-plugin-qt`
- [ ] `.deb`: write a CPack `DEB` configuration
- [ ] AUR PKGBUILD: write a `PKGBUILD` for Arch Linux
- [ ] Add `.desktop` file and AppStream metainfo XML

#### 4.4 Windows Packaging
- [ ] Bundle Qt DLLs with `windeployqt`
- [ ] Write NSIS or WiX installer script
- [ ] Add Windows application manifest (DPI awareness, UAC level)
- [ ] Add Windows application icon to the executable via `.rc` resource file

#### 4.5 macOS Packaging
- [ ] Create `.app` bundle with `macdeployqt`
- [ ] Write `Info.plist` with proper bundle identifier, version, and icon
- [ ] Create `.dmg` with `create-dmg` or `hdiutil`
- [ ] Document code signing and notarization steps (required for Gatekeeper)

#### 4.6 Release Process
- [ ] Write `CHANGELOG.md` following Keep a Changelog format
- [ ] Tag v1.0.0 in git; CI publishes binaries to GitHub Releases automatically
- [ ] Write a public-facing release announcement (README update + GitHub Release notes)

---

## Appendix: File Inventory

| Path | Purpose | Status |
|---|---|---|
| `app/main.cpp` | Entry point | Working |
| `file_manager/core/file_system.cpp` | FileSystem implementation | Working (bugs) |
| `file_manager/core/plugin_manager.cpp` | Plugin loader | Working (bugs) |
| `file_manager/core/test.cpp` | Empty file | Removed |
| `file_manager/gui/main_window.cpp` | Main UI | Working (incomplete) |
| `file_manager/gui/file_view.cpp` | File view widget | Stub |
| `file_manager/utilities/error_handler.cpp` | Error handling | Working (bugs) |
| `file_manager/utilities/logger.cpp` | File logger | Working (not used) |
| `include/core/file_system.hpp` | FileSystem API | Good |
| `include/core/plugin_interface.hpp` | Plugin ABI | Good (needs versioning) |
| `include/core/plugin_manager.hpp` | PluginManager API | Good |
| `include/gui/main_window.hpp` | MainWindow header | Good |
| `include/gui/file_view.hpp` | FileView header | Stub |
| `include/utilities/error_handler.hpp` | Error handler API | Has critical bugs |
| `include/utilities/logger.hpp` | Logger API | Good |
| `plugins/basic_operations/src/copy_plugin.cpp` | Copy plugin | Working |
| `plugins/basic_operations/src/move_plugin.cpp` | Move plugin | Working |
| `plugins/basic_operations/src/delete_plugin.cpp` | Delete plugin | Working |
| `plugins/example_plugin/src/example_plugin.cpp` | Example plugin source | File does not exist — header only |
| `resources/stylesheet.qss` | UI stylesheet | Good |
| `resources/resources.qrc` | Qt resource bundle | Working |
| `CMakeLists.txt` | Build system | Working (issues) |
| `cmake/PluginConfig.cmake` | Plugin build macro | Working |
| `README.md` | Project documentation | Good start |

---

*This document should be updated at the start of each phase to reflect current status.*
