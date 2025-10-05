#ifndef __DDB_ALTERNATIVEWIDGETS_MAIN_H
#define __DDB_ALTERNATIVEWIDGETS_MAIN_H

#include <gtk/gtk.h>

struct altwidgets{
	GHashTable *db_action_map;
	GActionGroup *db_action_group;
};

#endif
