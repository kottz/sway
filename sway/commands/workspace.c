#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <strings.h>
#include "sway/commands.h"
#include "sway/config.h"
#include "sway/input/seat.h"
#include "sway/tree/workspace.h"
#include "list.h"
#include "log.h"
#include "stringop.h"

static struct workspace_config *workspace_config_find_or_create(char *ws_name) {
	struct workspace_config *wsc = workspace_find_config(ws_name);
	if (wsc) {
		return wsc;
	}
	wsc = calloc(1, sizeof(struct workspace_config));
	if (!wsc) {
		return NULL;
	}
	wsc->workspace = strdup(ws_name);
	wsc->outputs = create_list();
	wsc->gaps_inner = INT_MIN;
	wsc->gaps_outer.top = INT_MIN;
	wsc->gaps_outer.right = INT_MIN;
	wsc->gaps_outer.bottom = INT_MIN;
	wsc->gaps_outer.left = INT_MIN;
	list_add(config->workspace_configs, wsc);
	return wsc;
}

void free_workspace_config(struct workspace_config *wsc) {
	free(wsc->workspace);
	list_free_items_and_destroy(wsc->outputs);
	free(wsc);
}

static void prevent_invalid_outer_gaps(struct workspace_config *wsc) {
	if (wsc->gaps_outer.top != INT_MIN &&
			wsc->gaps_outer.top < -wsc->gaps_inner) {
		wsc->gaps_outer.top = -wsc->gaps_inner;
	}
	if (wsc->gaps_outer.right != INT_MIN &&
			wsc->gaps_outer.right < -wsc->gaps_inner) {
		wsc->gaps_outer.right = -wsc->gaps_inner;
	}
	if (wsc->gaps_outer.bottom != INT_MIN &&
			wsc->gaps_outer.bottom < -wsc->gaps_inner) {
		wsc->gaps_outer.bottom = -wsc->gaps_inner;
	}
	if (wsc->gaps_outer.left != INT_MIN &&
			wsc->gaps_outer.left < -wsc->gaps_inner) {
		wsc->gaps_outer.left = -wsc->gaps_inner;
	}
}

static struct cmd_results *cmd_workspace_gaps(int argc, char **argv,
		int gaps_location) {
	const char expected[] = "Expected 'workspace <name> gaps "
		"inner|outer|horizontal|vertical|top|right|bottom|left <px>'";
	if (gaps_location == 0) {
		return cmd_results_new(CMD_INVALID, "%s", expected);
	}
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "workspace", EXPECTED_EQUAL_TO,
					gaps_location + 3))) {
		return error;
	}
	char *ws_name = join_args(argv, argc - 3);
	struct workspace_config *wsc = workspace_config_find_or_create(ws_name);
	free(ws_name);
	if (!wsc) {
		return cmd_results_new(CMD_FAILURE,
				"Unable to allocate workspace output");
	}

	char *end;
	int amount = strtol(argv[gaps_location + 2], &end, 10);
	if (strlen(end)) {
		return cmd_results_new(CMD_FAILURE, "%s", expected);
	}

	bool valid = false;
	char *type = argv[gaps_location + 1];
	if (!strcasecmp(type, "inner")) {
		valid = true;
		wsc->gaps_inner = (amount >= 0) ? amount : 0;
	} else {
		if (!strcasecmp(type, "outer") || !strcasecmp(type, "vertical")
				|| !strcasecmp(type, "top")) {
			valid = true;
			wsc->gaps_outer.top = amount;
		}
		if (!strcasecmp(type, "outer") || !strcasecmp(type, "horizontal")
				|| !strcasecmp(type, "right")) {
			valid = true;
			wsc->gaps_outer.right = amount;
		}
		if (!strcasecmp(type, "outer") || !strcasecmp(type, "vertical")
				|| !strcasecmp(type, "bottom")) {
			valid = true;
			wsc->gaps_outer.bottom = amount;
		}
		if (!strcasecmp(type, "outer") || !strcasecmp(type, "horizontal")
				|| !strcasecmp(type, "left")) {
			valid = true;
			wsc->gaps_outer.left = amount;
		}
	}
	if (!valid) {
		return cmd_results_new(CMD_INVALID, "%s", expected);
	}

	// Prevent invalid gaps configurations.
	if (wsc->gaps_inner != INT_MIN && wsc->gaps_inner < 0) {
		wsc->gaps_inner = 0;
	}
	prevent_invalid_outer_gaps(wsc);

	return error;
}

static struct cmd_results *cmd_workspace_group(int argc, char **argv,
		struct sway_seat *seat) {
	if (argc != 2) {
		return cmd_results_new(CMD_INVALID,
				"Expected 'workspace group <group|toggle>'");
	}

	struct sway_workspace *current = seat_get_focused_workspace(seat);
	workspace_group_remember_workspace(current);

	const char *group = NULL;
	if (strcasecmp(argv[1], "toggle") == 0) {
		group = workspace_group_toggle_active();
	} else if (workspace_group_set_active(argv[1])) {
		group = workspace_group_get_active();
	}
	if (!group) {
		return cmd_results_new(CMD_INVALID,
				"Workspace group '%s' is not configured", argv[1]);
	}

	const char *display_name =
		workspace_group_get_last_display_name(group);
	if (!display_name || !*display_name) {
		display_name = "1";
	}
	struct sway_workspace *ws =
		workspace_by_display_in_group(display_name, group);
	if (!ws) {
		char *name = workspace_group_make_name(group, display_name);
		if (!name) {
			return cmd_results_new(CMD_FAILURE,
					"Unable to allocate grouped workspace name");
		}
		ws = workspace_create(NULL, name);
		free(name);
	}
	if (!ws) {
		return cmd_results_new(CMD_FAILURE, "No workspace to switch to");
	}
	workspace_switch(ws);
	seat_consider_warp_to_focus(seat);
	return cmd_results_new(CMD_SUCCESS, NULL);
}

struct cmd_results *cmd_workspace(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "workspace", EXPECTED_AT_LEAST, 1))) {
		return error;
	}

	int output_location = -1;
	int gaps_location = -1;

	for (int i = 0; i < argc; ++i) {
		if (strcasecmp(argv[i], "output") == 0) {
			output_location = i;
			break;
		}
	}
	for (int i = 0; i < argc; ++i) {
		if (strcasecmp(argv[i], "gaps") == 0) {
			gaps_location = i;
			break;
		}
	}
	if (output_location == 0) {
		return cmd_results_new(CMD_INVALID,
			"Expected 'workspace <name> output <output>'");
	} else if (output_location > 0) {
		if ((error = checkarg(argc, "workspace", EXPECTED_AT_LEAST,
						output_location + 2))) {
			return error;
		}
		char *ws_name = join_args(argv, output_location);
		struct workspace_config *wsc = workspace_config_find_or_create(ws_name);
		free(ws_name);
		if (!wsc) {
			return cmd_results_new(CMD_FAILURE,
					"Unable to allocate workspace output");
		}
		for (int i = output_location + 1; i < argc; ++i) {
			list_add(wsc->outputs, strdup(argv[i]));
		}
	} else if (gaps_location >= 0) {
		if ((error = cmd_workspace_gaps(argc, argv, gaps_location))) {
			return error;
		}
	} else {
		if (config->reading || !config->active) {
			return cmd_results_new(CMD_DEFER, NULL);
		} else if (!root->outputs->length) {
			return cmd_results_new(CMD_INVALID,
					"Can't run this command while there's no outputs connected.");
		}

		if (root->fullscreen_global) {
			return cmd_results_new(CMD_FAILURE,
				"Can't switch workspaces while fullscreen global");
		}

		bool auto_back_and_forth = true;
		while (strcasecmp(argv[0], "--no-auto-back-and-forth") == 0) {
			auto_back_and_forth = false;
			if ((error = checkarg(--argc, "workspace", EXPECTED_AT_LEAST, 1))) {
				return error;
			}
			++argv;
		}

		struct sway_seat *seat = config->handler_context.seat;

		struct sway_workspace *ws = NULL;
		if (strcasecmp(argv[0], "group") == 0 && workspace_groups_enabled()) {
			return cmd_workspace_group(argc, argv, seat);
		} else if (strcasecmp(argv[0], "number") == 0) {
			if (argc < 2) {
				return cmd_results_new(CMD_INVALID,
						"Expected workspace number");
			}
			if (!isdigit(argv[1][0])) {
				return cmd_results_new(CMD_INVALID,
						"Invalid workspace number '%s'", argv[1]);
			}
			const char *group = workspace_group_get_active();
			if (!(ws = group ?
						workspace_by_number_in_group(argv[1], group) :
						workspace_by_number(argv[1]))) {
				char *display_name = join_args(argv + 1, argc - 1);
				char *name = group ?
					workspace_group_make_name(group, display_name) :
					strdup(display_name);
				ws = name ? workspace_create(NULL, name) : NULL;
				free(display_name);
				free(name);
			}
			if (ws && auto_back_and_forth) {
				ws = workspace_auto_back_and_forth(ws);
			}
		} else if (strcasecmp(argv[0], "next") == 0 ||
				strcasecmp(argv[0], "prev") == 0 ||
				strcasecmp(argv[0], "next_on_output") == 0 ||
				strcasecmp(argv[0], "prev_on_output") == 0 ||
				strcasecmp(argv[0], "current") == 0) {
			ws = workspace_by_name(argv[0]);
		} else if (strcasecmp(argv[0], "back_and_forth") == 0) {
			if (!seat->prev_workspace_name) {
				return cmd_results_new(CMD_INVALID,
						"There is no previous workspace");
			}
			if (!(ws = workspace_by_name(argv[0]))) {
				ws = workspace_create(NULL, seat->prev_workspace_name);
			}
		} else {
			char *name = join_args(argv, argc);
			if (!(ws = workspace_by_name(name))) {
				ws = workspace_create(NULL, name);
			}
			free(name);
			if (ws && auto_back_and_forth) {
				ws = workspace_auto_back_and_forth(ws);
			}
		}
		if (!ws) {
			return cmd_results_new(CMD_FAILURE, "No workspace to switch to");
		}
		workspace_switch(ws);
		seat_consider_warp_to_focus(seat);
	}
	return cmd_results_new(CMD_SUCCESS, NULL);
}
