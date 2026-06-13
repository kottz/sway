---
name: sway-workspace-groups-maintainer
description: Maintain kottz's Sway workspace-groups fork, especially rebasing the feature branch, resolving conflicts in workspace command/state/IPC code, preserving non-grouped compatibility, and validating with the local Sway build/test workflow.
---

# Sway Workspace Groups Maintainer

Use this skill for Sway source work in the `kottz/sway` fork.

## Workflow

1. Read `AGENTS.md` and `CONTRIBUTING.md`.
2. Confirm remotes:
   - `origin`: `git@github.com:kottz/sway.git`
   - `upstream`: `https://github.com/swaywm/sway.git`
3. Fetch upstream tags before rebasing.
4. Rebase `workspace-groups` onto the selected upstream Sway tag or branch.
5. Resolve conflicts by preserving the feature contract in `AGENTS.md`.
6. Run:

```bash
meson compile -C build
meson test -C build
git diff --check
```

## Conflict Hotspots

- Workspace lookup, sorting, and lifecycle code in `sway/tree/workspace.c`.
- Workspace and move command parsing.
- IPC JSON shape in `sway/ipc-json.c`.
- Manual pages for config and IPC fields.
- Runtime reload and rename paths that can stale group metadata.

## Invariants

- Grouping remains opt-in.
- Plain non-grouped workspaces behave as upstream Sway.
- Internal grouped names stay unique, but IPC exposes `display_name` and `group`.
- Current-group workspace switching and moving preserve Mod+number muscle memory.
- Group toggling restores the last used workspace in the target group.
