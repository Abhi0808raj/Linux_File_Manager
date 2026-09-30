# Validation status — v0.1

## Passed locally

- JavaScript syntax: local Figma builder and preview inline script.
- JSON parsing: token export, contrast report, and manifest fields.
- Token consistency: the preview/builder embedded definitions match tokens.json.
- Palette contrast: all 18 checked text combinations exceed 4.5:1; the two checked outline/surface combinations exceed 3:1. Minimum checked text ratio is 5.47:1. Disabled text is outside this check.
- Eleven detached-DOM checks: initial six rows; one selected row; dark theme; compact row/control heights; two panes; selection confined to one pane; no-results selection/status reset; three plugin states; appearance/layout reset; component section navigation; semantic color specimens.

The DOM checks ran against the actual preview script using LinkeDOM 0.18.12 in a temporary validation directory. They do not prove CSS layout, browser-native dialog behavior, screen-reader behavior, or visual quality. The validation dependency is not part of the application or design package.

## Verified in Figma

- New student-account file: `qNyVuZx39G2xR9wX65e380`; the earlier Starter file was not reused.
- Two pages: foundations/components and workspace patterns.
- 85 variables in three collections; Light and Dark semantic color modes; seven text styles.
- 17 component sets, 198 variants, and 24 icon components. Four workspace pattern frames contain 121 instances, including nested controls/icons.
- Reviewed rendered screenshots of foundations, component specimens, light workspace, dark split workspace, plugins/settings, and operations/recovery.
- Corrected container heights, shared state-label defaults, dark icon contrast, file-type examples, and header/row alignment. Reviewed post-fix screenshots of affected compositions.
- Final screen bounds: 1280 × 656 (light), 1280 × 552 (dark split), 1280 × 604 (plugins/settings), 1280 × 1006 (operations/recovery).

This is a static design review, not an exhaustive audit of every variant or a tested interactive Figma prototype. State labels are variant content; common control labels, file metadata, and plugin identity retain editable properties. The standalone builder incorporates the fixes but has not been rerun end-to-end as a desktop plugin.

## Still required

- Browser visual review at representative desktop sizes. No browser surface was available to the agent.
- Responsive layout and long-content checks beyond these desktop specimens.
- Keyboard traversal and focus-return review in a real browser/Figma prototype and ultimately Qt.
- Cross-platform Qt implementation and accessibility testing.
- Usability testing with representative users and external plugin authors.

## Implementation boundary

The Qt application was not modified. New files are design artifacts and a local builder. The existing user edits in application/build files were preserved. The .gitignore change permits docs/design to be versioned while retaining the ignore rule for other docs content.
