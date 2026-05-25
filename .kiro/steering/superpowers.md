---
inclusion: manual
---

# Superpowers Development Methodology

When this steering is active, follow the Superpowers workflow for all development tasks.

## Core Principle

Do NOT jump straight into writing code. Follow the structured process below.

## Workflow

### 1. Brainstorming (before any creative work)

- Explore project context first (files, docs, recent commits)
- Ask clarifying questions one at a time (prefer multiple choice)
- Propose 2-3 approaches with trade-offs and a recommendation
- Present design in sections, get approval after each
- Write design doc to `docs/superpowers/specs/YYYY-MM-DD-<topic>-design.md`
- Do NOT write code until design is approved

### 2. Writing Plans (after design approval)

- Break work into small tasks (2-5 minutes each)
- Each task has: exact file paths, complete code, verification steps
- Tasks are ordered by dependency
- Plan is presented for approval before execution

### 3. Test-Driven Development (during implementation)

- RED: Write a failing test first
- GREEN: Write minimal code to make it pass
- REFACTOR: Clean up while tests stay green
- Commit after each green phase
- Never write implementation code before the test

### 4. Verification Before Completion

- Run the full test suite
- Verify the feature works end-to-end
- Check for regressions
- Evidence over claims — show it works, don't just say it

## Philosophy

- **YAGNI** — Don't build what isn't needed
- **DRY** — Don't repeat yourself
- **Systematic over ad-hoc** — Process over guessing
- **Complexity reduction** — Simplicity is the goal
- **One question at a time** — Don't overwhelm
- **Incremental validation** — Present, approve, then proceed
