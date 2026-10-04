# Workspace implementation — 2026-10-04

Approved scope: begin the existing Qt application's Figma implementation on a separate branch. Source: Figma file qNyVuZx39G2xR9wX65e380, workspace frame 24:1087, within page 23:643; docs/design/design-system.md and tokens.json.

## Constraints
- Preserve existing filesystem, clipboard and context-menu behavior.
- Use Qt Widgets/C++17, native layouts, exported semantic tokens and local Figma icons.
- First delivery: light comfortable single-pane workspace, places, title, path entry, navigation, file list, view/sort actions and status. Implement only working controls; defer tabs, split view, settings and operation-center flows.
- Real filesystem data replaces specimen rows and device values.
- Keyboard access, visible focus, narrow windows and long paths must work.
- No Co-authored-by commit trailers.

## Tasks
- [x] 1. Shared light theme and Figma icon resources: resources, app startup, small theme adapter. Avoid new dependencies.
- [x] 2. Workspace layout and interactions: MainWindow, minimal FileView integration, focused UI tests. Reuse navigation and operations.
- [x] 3. Build, regression tests, offscreen screenshot at desktop/narrow sizes, independent code review and fixes.

## Verification
Use the qt-mingw-debug build/test presets. Add integration tests for path entry/history, invalid locations, sidebar navigation and view switching. Capture the actual Qt window through a temporary offscreen Qt harness. Review this first milestone against the source frame; document deferred flows explicitly.

## Progress
Branch: feat/figma-workspace. Initial working tree clean; baseline build passed.
Baseline tests: 140/143 passed. Existing failures: FileSystemTest.CopyOverwritesExistingWhenFlagSet, PluginManagerTests, CopyPluginIntegration. Baseline failure list saved in build/workspace-baseline-failures.txt.
Task interfaces: theme owns resources/app/theme adapter; workspace owns main_window/file_view/tests. Shared resource names coordinated in agent briefs. Controller integrates CMake/resource changes if needed.

## Delivered and verified
The application uses the light token palette and local Figma Places icons. Native Qt layouts provide real Places, folder title, editable path (Ctrl+L), Back/Forward/Up/Refresh shortcuts, Details/List modes, sorting, New folder and folder/file counts. Invalid path errors stay inline. Existing context menus and filesystem operations are reused.

Independent review fixes: duplicate Places activation no longer adds history; the sort selector is authoritative (table header clicks are disabled); sidebar focus remains visible; invalid theme resources preserve the application theme. Roboto is used when installed, otherwise the application font is retained. Long filenames elide on one line.

Windows Qt 6.10/MinGW build passed. Final CTest: 148/151 passed; the same three baseline failures remain. All eight Workspace tests and all 19 FileView tests passed. Logs: build/workspace-final-tests.log. No new dependencies.

Actual Qt screenshots were inspected at 1280x800 and 800x600: build/workspace-screenshots/workspace-desktop.png and workspace-narrow.png. For Windows offscreen capture, set QT_QPA_PLATFORM=offscreen, QT_QPA_FONTDIR=C:/Windows/Fonts, FM_SCREENSHOT_FONT=C:/Windows/Fonts/segoeui.ttf and FM_SCREENSHOT_DIR to an output directory before running test_workspace. The font environment option is confined to the test harness. Platform-provided file icons differ from the design specimens and may be absent in offscreen captures.

This is the first functional workspace milestone, not the complete Figma board. [Folder tabs](implementation-tabs.md) and [current-folder search](implementation-search.md) were implemented in subsequent milestones. Recursive search, split view, grid, dark/compact modes, settings, operation-center flows and richer selection/storage status remain future work. Linux runtime and screen-reader verification are not covered by this Windows run.
