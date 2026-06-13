#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "sway/commands.h"
#include "sway/config.h"
#include "sway/tree/workspace.h"
#include "list.h"

static bool valid_group_name(const char *name) {
	return strchr(name, ':') == NULL && strcasecmp(name, "toggle") != 0;
}

static bool contains_group(char **groups, int count, const char *group) {
	for (int i = 0; i < count; ++i) {
		if (strcasecmp(groups[i], group) == 0) {
			return true;
		}
	}
	return false;
}

static void clear_workspace_groups(void) {
	while (config->workspace_groups->length) {
		free(config->workspace_groups->items[0]);
		list_del(config->workspace_groups, 0);
	}
}

struct cmd_results *cmd_workspace_groups(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "workspace_groups", EXPECTED_AT_LEAST, 1))) {
		return error;
	}

	for (int i = 0; i < argc; ++i) {
		if (!valid_group_name(argv[i])) {
			return cmd_results_new(CMD_INVALID,
					"Invalid workspace group name '%s'", argv[i]);
		}
		if (contains_group(argv, i, argv[i])) {
			return cmd_results_new(CMD_INVALID,
					"Duplicate workspace group '%s'", argv[i]);
		}
	}

	if (config->workspace_group_default &&
			!contains_group(argv, argc, config->workspace_group_default)) {
		return cmd_results_new(CMD_INVALID,
				"Default workspace group '%s' is not configured",
				config->workspace_group_default);
	}

	clear_workspace_groups();
	for (int i = 0; i < argc; ++i) {
		char *group = strdup(argv[i]);
		if (!group) {
			return cmd_results_new(CMD_FAILURE,
					"Unable to allocate workspace group");
		}
		list_add(config->workspace_groups, group);
	}

	const char *active = config->workspace_group_default ?
		config->workspace_group_default : config->workspace_groups->items[0];
	if (!workspace_group_set_active(active)) {
		return cmd_results_new(CMD_FAILURE,
				"Unable to activate workspace group '%s'", active);
	}
	if (!config->reading) {
		workspace_group_refresh_metadata();
	}
	return cmd_results_new(CMD_SUCCESS, NULL);
}
