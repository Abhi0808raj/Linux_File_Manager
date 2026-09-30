# File Manager — product requirements, v0.1

Status: working product draft. The audience and design direction below are confirmed; release requirements are proposals for implementation planning, not claims about current functionality.

## Vision and audience

An open-source file manager for people on Linux, Windows, and macOS who want to adapt file workflows through themes, layouts, and community plugins.

Confirmed decisions:
- Working name: File Manager.
- Material 3 is the visual and interaction foundation, adapted to desktop use.
- Contrasting colors, light and dark themes, comfortable and compact density.
- A single file pane with tabs is the default; split view is optional.
- v0.1 plugins contribute actions, previews, and settings through shared controls.
- The existing implementation uses Qt/C++; the design does not imply a framework migration.

## Problems to solve

1. People need a dependable way to navigate and manage files while keeping useful context visible.
2. People want to customize their workspace without assembling an unusable configuration.
3. Plugin authors need clear, stable extension points and reusable interface patterns.

The problem statements are hypotheses to validate with users. No user interviews or usability study have yet been conducted.

## Product principles

- File content takes visual priority over application chrome.
- Selection, keyboard focus, operation status, and destructive actions are distinguishable.
- Customization has a usable default and a visible reset path.
- Plugins share the application's theme, density, controls, and feedback patterns.
- Cross-platform consistency respects native shortcuts, paths, window controls, and filesystem behavior.

## Proposed first-release requirements

| ID | Priority | Requirement | Acceptance example |
|---|---|---|---|
| FM-01 | Must | Navigate directories, move back/forward/up, and edit the path | A user reaches a directory through the sidebar or path field; history reflects the navigation |
| FM-02 | Must | Open and close tabs; optionally split the workspace | Each pane shows its current path and active selection; closing a pane preserves the remaining pane |
| FM-03 | Must | Provide list browsing, sorting, and explicit selection | Keyboard focus remains identifiable independently of single or multiple selection |
| FM-04 | Must | Create folders, rename, copy, move, and remove files | Conflicts and failures identify affected files; permanent deletion requires a clearly described decision |
| FM-05 | Must | Show progress, cancellation, partial completion, and errors | Cancellation never reports unfinished files as copied; completion summarizes successes and failures |
| FM-06 | Must | Support light, dark, and follow-system appearance | Theme changes preserve readable content and state distinctions; the preference survives restart |
| FM-07 | Must | Support comfortable and compact density | Both modes preserve keyboard access and readable file metadata |
| FM-08 | Must | Install a compatible local plugin and enable/disable it | Unsupported API versions explain why a plugin cannot be enabled; any required restart is disclosed |
| FM-09 | Must | Expose applicable plugin actions for the current selection | An action declares when it applies, receives the intended selection, and reports an actionable result |
| FM-10 | Must | Host plugin previews and settings with shared controls | Plugin UI follows the selected theme and density; invalid settings show inline errors |
| FM-11 | Should | Persist layout preferences and restore defaults | A user can recover the standard workspace without editing configuration files |
| FM-12 | Should | Find files by name within a stated search scope | The search UI makes its scope clear and distinguishes no results from an inaccessible directory |

## Key journeys

### Browse and customize
Open the application → navigate to a folder → open a second tab → switch density → optionally enable split view → return to the default layout.

### Copy with a conflict
Select files → choose a destination → inspect an existing-file conflict → skip, keep both, or replace → monitor progress → review the completion summary. Replacement must not be the automatic result of dismissing the dialog.

### Use an extension
Open Plugins → install a local package → inspect identity and compatibility → enable → select an applicable file → run an action → read the result. A preview appears in the host preview surface; settings remain in the host settings interface.

## Quality and platform requirements

- Keep navigation and cancellation responsive during file operations; define numeric performance budgets after measuring the current application on named hardware and directory fixtures.
- Validate keyboard traversal, focus return after dialogs, accessible names, text scaling, and non-color status cues on each supported OS.
- Design contrast targets: 4.5:1 for ordinary enabled text, 3:1 for large text and essential UI boundaries/focus. This target is not a claim of complete accessibility conformance.
- Test permissions, symlinks, filename rules, case sensitivity, removable volumes, and trash behavior on each supported platform.
- Prefer native window decorations and platform shortcut conventions. Figma screens specify the content area.
- A native plugin executes with the application's trust level; metadata alone does not provide sandboxing. Define trust, compatibility, and lifecycle behavior before public distribution.

## Outside v0.1

Plugin-supplied replacement file views or arbitrary layouts; a hosted marketplace; cloud account synchronization; remote filesystem providers; a full animation system; mobile applications. These remain possible future directions, not implied commitments.

## Validation and success

Before calling the first release ready:
1. Test the three key journeys with representative users and record task completion, confusion, and recovery issues.
2. Ask an external developer to build one plugin without changing the application repository, using only the SDK guide.
3. Verify all must-have requirements on Linux, Windows, and macOS using a published support matrix.
4. Record file-operation correctness and interruption results with repeatable fixtures.

Open product questions for implementation planning: supported OS versions, distribution formats, trash/restore behavior, plugin packaging and trust, search depth, shortcut defaults, and measured performance budgets.

## Design artifacts

- [Design system specification](design-system.md)
- [Delivery and validation process](process.md)
- [Current Figma design file](https://www.figma.com/design/qNyVuZx39G2xR9wX65e380/Untitled?node-id=7-972)

