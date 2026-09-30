# File Manager — design package v0.1

Start with [the Figma design system](https://www.figma.com/design/qNyVuZx39G2xR9wX65e380/Untitled?node-id=7-972) or [the interactive preview](index.html). Open the preview in a desktop browser directly, or serve this directory with `python -m http.server 8765 --bind 127.0.0.1` and visit http://127.0.0.1:8765/.

## Contents

- [PRD](PRD.md): confirmed direction, proposed requirements, user journeys, and release acceptance.
- [Design specification](design-system.md): components, states, tokens, accessibility, and plugin rules.
- [Delivery process](process.md): review, implementation milestones, and contribution process.
- [Token export](tokens.json): theme, typography, density, space, and shape definitions.
- [Palette contrast results](contrast-report.json): checked enabled-text and boundary pairs.
- [Figma builder instructions](figma-builder/README.md): local development-plugin setup and limitations.
- [Validation status](validation.md): what was checked and what remains unverified.

## Review the preview

1. Switch light/dark appearance and comfortable/compact density.
2. Toggle split view, select files in each pane, and search for a missing filename.
3. Open Plugins and Settings from the sidebar; reset appearance/layout.
4. Inspect Components and Foundations; open the conflict example.

Everything uses example data. The preview does not interact with the real filesystem. The displayed controls are design demonstrations, not a completed application.

## Current Figma status

Created and visually reviewed in the new student-account file:

- [Foundations and components](https://www.figma.com/design/qNyVuZx39G2xR9wX65e380/Untitled?node-id=7-972): 85 variables across three collections, Light/Dark semantic modes, seven text styles, 17 component sets with 198 variants, and 24 icon components.
- [Workspace patterns](https://www.figma.com/design/qNyVuZx39G2xR9wX65e380/Untitled?node-id=8-326): light single-pane workspace, dark compact split view, plugin/settings examples, and operations/recovery states. These screens contain 121 linked instances, including nested icons and controls.

The library is local to this file; it has not been published as a team library. Figma screens are static design examples, while the HTML preview supplies limited interactions with sample data. The standalone builder incorporates the canvas corrections; it has not been rerun end-to-end as a desktop plugin. See [validation](validation.md) and the exact node IDs in [figma-state.json](figma-state.json).
