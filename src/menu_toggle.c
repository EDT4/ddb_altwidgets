#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include <stdbool.h>
#include "deadbeef_util.h"

extern DB_functions_t *deadbeef;
extern ddb_gtkui_t *gtkui_plugin;

struct menutoggle{
	ddb_gtkui_widget_t base;
	int callback_id;
	GHashTable *db_action_map; //TODO: Move this to main?
};

ddb_gtkui_widget_t *menutoggle_create(){
	struct menutoggle *w = calloc(1,sizeof(struct menutoggle));
	w->base.widget = gtk_toggle_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w->base.widget),!!deadbeef->conf_get_int("gtkui.show_menu",1));
	w->db_action_map = g_hash_table_new_full(g_str_hash,g_str_equal,free,NULL);
	GActionGroup *action_group = deadbeef_action_group(w->db_action_map);

	gtk_widget_insert_action_group(GTK_WIDGET(w->base.widget),"db",action_group);
		gtk_actionable_set_action_name(GTK_ACTIONABLE(w->base.widget),"db.toggle_menu");
		{
			DB_plugin_action_t *db_action = g_hash_table_lookup(w->db_action_map,"toggle_menu");
			if(db_action) gtk_widget_set_tooltip_text(w->base.widget,db_action->title);
		}
	gtk_button_set_image(GTK_BUTTON(w->base.widget),gtk_image_new_from_icon_name("open-menu-symbolic",GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_style_context_add_class(gtk_widget_get_style_context(w->base.widget),"flat");
	gtk_widget_show(w->base.widget);
	gtkui_plugin->w_override_signals(w->base.widget,w);

	return (ddb_gtkui_widget_t*)w;
}
