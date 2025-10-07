#include "deadbeef_util.h"

extern DB_functions_t *deadbeef;

//Copied from deadbeef/plugins/gtkui/actions.c:gtkui_exec_action_14.
void gtkui_exec_action_14(DB_plugin_action_t *action,int cursor){
	// Plugin can handle all tracks by itself
	if (action->flags & DB_ACTION_CAN_MULTIPLE_TRACKS)
	{
		action->callback (action, NULL);
		return;
	}

	// For single-track actions just invoke it with first selected track
	if (!(action->flags & DB_ACTION_MULTIPLE_TRACKS))
	{
		if (cursor == -1) {
			cursor = deadbeef->pl_get_cursor (PL_MAIN);
		}
		if (cursor == -1) 
		{
			return;
		}
		DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (cursor, PL_MAIN);
		action->callback (action, it);
		deadbeef->pl_item_unref (it);
		return;
	}

	//We end up here if plugin won't traverse tracks and we have to do it ourselves
	DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
	while (it) {
		if (deadbeef->pl_is_selected (it)) {
			action->callback (action, it);
		}
		DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
		deadbeef->pl_item_unref (it);
		it = next;
	}
}

//Copied with modifications from deadbeef/plugins/gtkui/actions.c:menu_action_cb.
void action_call(DB_plugin_action_t *db_action,ddb_action_context_t ctx){
	if(db_action->callback){
		gtkui_exec_action_14(db_action,-1);
	}else if(db_action->callback2){
		db_action->callback2(db_action,ctx);
	}
}

