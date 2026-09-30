# Local Figma builder

Status: the library was created through page-scoped MCP scripts in the [new student-account file](https://www.figma.com/design/qNyVuZx39G2xR9wX65e380/Untitled?node-id=7-972) and visually reviewed. This standalone builder incorporates the resulting layout, theme, and state-label corrections. Its syntax is checked, but the revised desktop-plugin path has not been run end-to-end. Do not rerun it in the completed file; it is a reproduction aid for a separate empty file. It finds its own pages, collections, variables, and styles by exact name and reuses them.

In Figma's desktop application, create a new development plugin for Figma Design using the Run once template. Name it File Manager Design System Builder. Figma generates a manifest with an assigned plugin ID. Replace the generated code.js with the code.js in this directory; retain the generated ID and merge the supplied manifest fields into the generated manifest. Then run the development plugin in the intended design file. Local plugin creation/execution depends on your account's edit access; this does not grant access or change your plan. See the [official quickstart](https://developers.figma.com/docs/plugins/plugin-quickstart-guide/) and [manifest reference](https://developers.figma.com/docs/plugins/manifest/).

The builder uses no network requests. It creates two named pages, a semantic token library, component variants, and example screens. It does not remove the existing test page. A second run stops if its component or documentation objects already exist; inspect partial results before retrying a failed run. It does not automatically clean up partial nodes.

Where a plan supports multiple variable modes, Light and Dark use semantic modes. Otherwise the builder uses separate light/dark semantic variables and explicit Theme variants. Density values are also exposed as named metrics. This fallback is documented in the completion message.

The local preview is a separate HTML review artifact using the same token definitions. It is not an interactive Figma prototype or an implementation of filesystem operations.
