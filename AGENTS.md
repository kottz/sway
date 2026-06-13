# AGENTS.md

This fork carries kottz's native workspace-groups feature on
`workspace-groups`.

## Remotes

- `origin`: `git@github.com:kottz/sway.git`
- `upstream`: `https://github.com/swaywm/sway.git`

When maintaining this fork, fetch upstream tags before deciding what to rebase
onto.

## Feature Contract

- Workspace groups are opt-in via `workspace_groups`.
- Existing non-grouped Sway configs must keep working.
- `workspace number N` and `move container to workspace number N` target the
  active group when groups are configured.
- `workspace group <group>|toggle` switches group and restores that group's
  last focused workspace.
- IPC exposes group metadata for workspaces and containers without requiring
  clients to parse names like `work:1`.
- Waybar should be able to show plain workspace numbers and a compact W/P
  group indicator.

## Important Files

- `sway/tree/workspace.c`
- `sway/commands/workspace.c`
- `sway/commands/move.c`
- `sway/commands/workspace_groups.c`
- `sway/commands/workspace_group_default.c`
- `sway/ipc-json.c`
- `include/sway/tree/workspace.h`
- `sway/sway.5.scd`
- `sway/sway-ipc.7.scd`

## Verification

```bash
meson compile -C build
meson test -C build
git diff --check
```

Follow Sway's `CONTRIBUTING.md`: kernel-like C style, tabs for indentation,
and concise commit subjects.
