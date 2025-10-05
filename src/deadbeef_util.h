#ifndef __DDB_CUSTOMHEADERBAR_DEADBEEF_UTIL_H
#define __DDB_CUSTOMHEADERBAR_DEADBEEF_UTIL_H

#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>

void gtkui_exec_action_14(DB_plugin_action_t *action,int cursor);
void action_call(DB_plugin_action_t *db_action,ddb_action_context_t ctx);
GtkWidget* gtkui_lookup_widget(GtkWidget *widget,const gchar *widget_name);

#endif
