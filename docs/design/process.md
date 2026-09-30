# Design and delivery process

## 1. Product definition
Maintain PRD.md as the product source of truth. Mark confirmed decisions, proposals, implementation status, and open questions separately. Scope changes should identify the affected requirement IDs.

## 2. Design system v0.1
Create foundations before dependent components. Name semantic roles consistently in Figma and tokens.json. Validate state coverage, variable bindings, typography, layout, and contrast. Keep the original test page for reference.

## 3. Workflow review
Review browsing, customization, copying with conflicts, and plugin use as complete journeys. Use actual file names, long names, empty folders, denied access, and partial-operation failures. Ask users to complete tasks without coaching; record findings before adding features.

## 4. Technical design
After reviewing the design, specify the Qt theme adapter, command registration, selection model, operation jobs, plugin lifecycle, and preview/settings contribution interfaces. Write platform behavior decisions before calling an API stable.

## 5. Implementation milestones
1. Theme and shared controls, including focus and density.
2. Navigation, tabs, selection, and optional split view.
3. File operation progress and recovery.
4. One external plugin contributing an action, a preview, and settings.
5. Platform verification, packaging, contributor documentation, and an alpha release.

Each milestone should connect PRD requirements to Figma components, implementation work, and acceptance evidence. The sequence is a proposed delivery roadmap, not a claim that estimates or an implementation plan are complete.

## 6. Contribution and maintenance
For a component change, document purpose, anatomy, states, keyboard behavior, token usage, and examples. Review theme/density combinations. Increment the design-system version for meaningful changes and record migrations for renamed tokens or component properties.

## Initial design acceptance
- All agreed families exist as editable components or documented compositions.
- Review screens use linked instances and semantic tokens.
- Light/dark and comfortable/compact examples are present.
- Enabled text and important focus/control boundaries meet the documented contrast targets.
- No unintended clipping or overlapping text in review frames.
- Local documentation states limitations and distinguishes design from implemented features.

