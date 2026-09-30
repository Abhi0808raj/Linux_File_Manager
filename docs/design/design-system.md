# File Manager — design system v0.1

Status: v0.1 design library and four workspace pattern frames created and visually reviewed in the [student-account Figma file](https://www.figma.com/design/qNyVuZx39G2xR9wX65e380/Untitled?node-id=7-972). The local preview and exported token file describe the proposed product appearance; application styling has not been changed. Implementation and usability validation remain separate work.

## Direction

Material 3 provides semantic color roles, typography hierarchy, surface hierarchy, and state conventions. Desktop adaptations provide denser file rows, persistent paths and tabs, pointer hover, keyboard focus, and an optional two-pane workspace. This is an application-specific adaptation, not a claim of exact coverage of every Material component.

Use Roboto for the design specimens. A future Qt implementation should bundle an appropriately licensed font or explicitly test system-font fallbacks. Use a blue primary accent with pale blue selected surfaces in light mode, and light blue controls over dark neutral surfaces in dark mode. Error, warning, and success use both text/icons and color.

## Token contract

Figma collections:
- `FM / Primitives`: raw palette values; hidden from normal property pickers.
- `FM / Color`: semantic aliases with Light and Dark modes.
- `FM / Metrics`: spacing and shape values.
- Density: Comfortable and Compact control/row metrics. The local builder places these in `FM / Metrics`; a future migration can expose density as an independent mode collection.

The current Figma file uses native Light/Dark semantic modes. The standalone builder has a fallback for accounts that cannot create a second mode. The previous account's file is retained separately and is not the current source of truth.

Color roles include surface, surface-container, surface-container-high, on-surface, on-surface-variant, outline, outline-variant, primary, on-primary, primary-container, on-primary-container, error, error-container, success, and warning. Components bind to roles rather than literal colors.

Text styles use role names: display, headline, title, body, label, and caption. Caption text is supplementary; essential file names use body styling. Do not use display typography inside dense file lists.

Qt mapping: use the exported JSON token names as the proposed interface for a future theme adapter that generates QPalette/QSS and widget metrics. Figma's web code syntax is a design-token reference, not evidence that Qt QSS supports CSS custom properties. No automatic Code Connect binding is claimed for Qt/C++.

## Component inventory

| Family | Variants and properties |
|---|---|
| Button | Filled, tonal, outlined, text; default, hover, pressed, focus, disabled; editable label |
| Icon button | Default, hover, pressed, focus, disabled; swappable icon; accessible-name requirement |
| Field | Default, hover, focus, error, disabled; editable value and error example |
| Navigation item | Default, hover, selected, focus; editable label; swappable icon |
| Tab | Default, active, hover, focus; editable label |
| File row | Default, hover, selected, focus, cut; editable name/type/modified; swappable icon |
| Checkbox | Off, on, mixed, disabled |
| Switch | Off, on, focus, disabled |
| Status badge | Neutral, success, warning, error |
| Progress | Running, complete, error; state-specific text |
| Menu item | Default, hover, focus, disabled; editable label and shortcut |
| Plugin card | Enabled, disabled, incompatible; editable plugin identity and summary |
| Dialog | Conflict and destructive decision examples built from shared buttons |
| State panel | Empty, no results, access error |

Keep construction components outside review frames. Screens use linked instances. Use component text and instance-swap properties for normal overrides, not detachment. Status, progress, dialog consequences, and the field error example use variant-specific text so a shared default cannot mislabel another state. Edit that nested text when a product-specific message is required. Icon colors inherit the host theme.

## Workspace and plugin rules

- Single pane and tabs form the default workspace. Split view is an explicit user choice.
- The path and active pane remain identifiable. Selecting a file must not silently switch panes.
- The preview area is a host surface. A plugin supplies preview content; it does not rearrange the main workspace in v0.1.
- Plugin actions appear in a contextual menu or actions area only when applicable. Explain unavailable actions where useful.
- Plugin settings use the same field, switch, and validation patterns as application settings.
- Install/enable/disable/compatibility states are distinct. Avoid implying that an enabled plugin is sandboxed or verified.
- Reset appearance/layout is available alongside customization settings.

## Interaction and accessibility

Focus is a visible outline distinct from selection. Hover adds emphasis but does not replace labels or keyboard affordances. Pressed states provide immediate feedback. Disabled states do not imply keyboard focusability. State badges include words.

On macOS, adapt shortcuts to Command where appropriate; on Linux and Windows use platform conventions. Keep navigation controls reachable by keyboard and restore focus to the invoking control when a dialog closes.

Long filenames truncate intentionally in rows with access to the full name in a tooltip/properties view. Do not truncate error messages or destructive-action consequences. The specimen boards show static state examples; transitions and screen-reader behavior require implementation testing.

Motion should clarify transitions and respect reduced-motion preferences. The initial library does not include animated prototypes.

## Review examples

1. Light, comfortable single-pane workspace.
2. Dark, compact workspace with optional split view.
3. Plugin management and appearance/settings patterns.
4. File conflict, operation progress, and empty/error states.

## Sources

- [Material foundations](https://m3.material.io/foundations/)
- [Material theme roles and typography](https://developer.android.com/codelabs/m3-design-theming)
- [MengTo: design-first UI prompting](https://github.com/MengTo/Skills/blob/main/agent-skills/ui/design-first-ui-prompting/SKILL.md)
- [Qt accessibility](https://doc.qt.io/qt-6/accessible.html)
