# Folder tabs implementation — 2026-10-04

Continue the approved Figma implementation on feat/figma-workspace. Source: workspace frame 24:1087 / Folder tabs 24:1139 in Figma qNyVuZx39G2xR9wX65e380; high-fidelity reference saved in build/figma-workspace-reference.tsx. Implements the tab portion of PRD FM-02.

## Scope and approach
Use a QTabBar above the path controls and a QStackedWidget containing one FileView per tab. Keep each FileView's history, selection, view mode and sort settings. Route the existing shared toolbar and Places to the active tab; inactive signals must not overwrite active controls/status. Reuse semantic tokens and the exported folder/close SVGs. Qt layouts must retain 800px usability and scroll overflowing tabs.

New tab (+ Tab, Ctrl+T) opens the current directory with fresh history. Close (tab close button, Ctrl+W) activates a neighboring tab; keep the last tab open and disable its close action. Ctrl+Tab and Ctrl+Shift+Tab cycle tabs. Titles follow folder names, elide long labels and expose full paths in tooltips. Active tab is identifiable; tab selection has visible keyboard focus. Existing named navigation actions remain compatible.

Copy/cut/paste must work across tabs, including after closing the source tab. Share clipboard state at the window level without changing standalone FileView behavior; refresh cut indicators when a tab becomes active. No changes to filesystem operation semantics, no new dependencies, no co-author commit trailers. Search, split view, settings and session restoration remain separate milestones.

## Implementation and verification
- [x] Add tests first for independent history, active toolbar routing, view/sort/selection restoration, closing active/inactive/last tabs and cross-tab clipboard.
- [x] Implement the tab row, active-tab routing and shared clipboard; keep changes within MainWindow/FileView, theme QSS and focused tests.
- [x] Build all targets; run focused tests and full CTest, comparing against known baseline failures (overwrite, plugin manager, copy plugin integration).
- [x] Capture actual Qt desktop/narrow screenshots with multiple tabs, review changes and fix findings.

Baseline: clean commit 79de583; full build passed; 148/151 CTest cases passed, with only the three known baseline failures. Use UTF-8 explicitly when reading/writing source files on Windows.

## Delivered and verified
Tabs now preserve independent location, history, selection, view mode and sort order. Shared controls route to the active FileView. Window-owned clipboard state survives tab switches and source-tab closure, while standalone FileView instances keep their own clipboard. Opening a tab from an externally deleted location leaves the current tab intact and shows an inline recovery message.

Independent review and scoped re-review completed with findings addressed. The tab row uses 24px local Figma folder/close icons, a neighboring + Tab control, and scrolling overflow. Actual 1280x800 and 800x600 windows, including many-tab overflow, were inspected in build/tabs-screenshots/workspace-desktop.png, workspace-narrow.png and workspace-overflow.png.

Windows all-target build passed. Final CTest: 154/157 passed, including all 14 Workspace cases; only FileSystemTest.CopyOverwritesExistingWhenFlagSet, PluginManagerTests and CopyPluginIntegration remain failing as in the baseline. Logs: build/tabs-final-build.log and build/tabs-final-tests.log. Six new tests cover independent history/view/sort, active/inactive/last closure, selection, copy/cut after source closure, keyboard tab controls, and missing starting directories.

No new dependencies. Session persistence, split panes, search, settings, and Linux runtime/accessibility verification remain outside this milestone.
