# Current-folder search — 2026-10-04

Continue the approved Figma app implementation on feat/figma-workspace, from 54fe4c7. Figma workspace 24:1087 and Search results 24:2037 in qNyVuZx39G2xR9wX65e380 define the search field, scope and clear action. High-fidelity references: build/figma-workspace-reference.tsx and build/figma-search-reference.tsx. This milestone implements PRD FM-12 for direct children of the current folder; recursive result cards are deferred.

## Behavior
- Add the 340px search field beside the File Manager heading, using existing 24px search.svg, semantic surface-container, outline and 8px radius; adapt width at 800px.
- Search matches literal case-insensitive substrings of file and directory names, including punctuation such as brackets, stars and question marks where filenames support them. Do not scan file contents or recurse.
- Label the scope clearly as This folder; show result counts and an inline No matching files state with Clear search. An empty query restores ordinary browsing and counts.
- Ctrl+F focuses/selects the search field; Escape in the field clears it and returns focus to the active file view. A visible clear control also works with pointer/keyboard.
- Each tab preserves its own search query when switching. New tabs begin unfiltered. Navigating to another directory clears that tab's query, including history navigation; invalid path entry does not.
- Search works in Details and List modes, preserves sorting, and file operations must target the correct visible filesystem entries. Clear selections that become hidden so actions cannot affect an invisible selected item.
- Updates from filesystem loading/changes must refresh filtered rows/counts. Do not present no-results while loading, or mistake unavailable/inaccessible folders for an empty search; show a suitable loading/error state.
- Preserve current clipboard/tab/navigation behavior and named actions. Use native Qt model/view facilities; no new dependencies and no co-author trailers.

## Tasks
- [x] Add focused failing tests for literal filtering, tab-local query/reset, selection/action safety, asynchronous/live updates and clearing/no-results.
- [x] Implement minimal FileView filtering and MainWindow search UI using existing models and tokens.
- [x] Build all targets; run focused tests and full CTest; inspect actual Qt search/no-results/narrow captures.

Baseline: full build passed; 154/157 CTest cases passed. Existing failures: FileSystemTest.CopyOverwritesExistingWhenFlagSet, PluginManagerTests, CopyPluginIntegration. Explicit UTF-8 file IO is required; avoid PowerShell pipelines that decode git output implicitly.

## Delivery and verification
The search field filters the current tab with literal, case-insensitive filename matching. It includes directories, preserves native size sorting, and does not recurse. Queries survive tab switches and reset on successful navigation. Ctrl+F focuses the field; Escape clears and returns focus to files; Clear search provides a visible action. Loading, unavailable-folder and no-match states are distinct, and background tabs cannot overwrite active results.

Six new Workspace tests cover matching, tab/query behavior, hidden selection and copy safety, live additions, keyboard clearing, loading/missing folders, list-mode size sorting, non-recursion and background-tab isolation. An existing activation test now passes the displayed proxy index, as real UI activation does, instead of passing an underlying filesystem-model index into a proxy slot.

All-target Windows build passed. Focused checks passed 98/98; full CTest passed 160/163, with only the same three baseline failures. Logs are build/search-build.log, build/search-focused-tests.log and build/search-final-tests.log. Actual desktop, no-results and 800px screenshots were inspected under build/search-screenshots/.

The implementation and reviewer subagents hit a usage limit. The reviewer identified a missing Unix directory traversal-permission check, which was fixed. The main session completed implementation, manual review and verification; a complete independent review was not available. Linux runtime/permission behavior remains untested on this Windows host. Recursive search and the Figma recursive-result cards remain deferred.
