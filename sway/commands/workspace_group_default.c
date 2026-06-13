#include <stdlib.h>
#include <string.h>
#include "sway/commands.h"
#include "sway/config.h"
#include "sway/tree/workspace.h"

struct cmd_results *cmd_workspace_group_default(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "workspace_group_default", EXPECTED_EQUAL_TO, 1))) {
		return error;
	}

	if (strchr(argv[0], ':')) {
		return cmd_results_new(CMD_INVALID,
				"Invalid workspace group name '%s'", argv[0]);
	}
	if (workspace_groups_enabled() && !workspace_group_exists(argv[0])) {
		return cmd_results_new(CMD_INVALID,
				"Workspace group '%s' is not configured", argv[0]);
	}

	char *group = strdup(argv[0]);
	if (!group) {
		return cmd_results_new(CMD_FAILURE,
				"Unable to allocate default workspace group");
	}
	free(config->workspace_group_default);
	config->workspace_group_default = group;
	if ((config->reading || !config->active_workspace_group) &&
			workspace_groups_enabled()) {
		workspace_group_set_active(group);
	}

	return cmd_results_new(CMD_SUCCESS, NULL);
}
