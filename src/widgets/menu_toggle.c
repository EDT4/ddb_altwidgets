#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include <stdbool.h>
#include "../deadbeef_util.h"
#include "../main.h"

extern DB_functions_t *deadbeef;
extern ddb_gtkui_t *gtkui_plugin;
extern struct altwidgets altwidgets_data;

struct menutoggle{
	ddb_gtkui_widget_t base;

	#if !GTK_CHECK_VERSION(3,0,0)
	DB_plugin_action_t *action;
	#endif
};

//TODO: It is probably unnecessary for the GTK3 variant to insert an action group just for this.
#if !GTK_CHECK_VERSION(3,0,0)
static void on_button_clicked(__attribute__((unused)) GtkToggleButton *button,gpointer user_data){
	struct menutoggle *data = (struct menutoggle*) user_data;
	if(data->action) action_call(data->action,DDB_ACTION_CTX_MAIN);
}
#endif

ddb_gtkui_widget_t *menutoggle_create(){
	struct menutoggle *w = calloc(1,sizeof(struct menutoggle));
	w->base.widget = gtk_toggle_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w->base.widget),!!deadbeef->conf_get_int("gtkui.show_menu",1));
	gtk_button_set_image(GTK_BUTTON(w->base.widget),gtk_image_new_from_icon_name("open-menu-symbolic",GTK_ICON_SIZE_SMALL_TOOLBAR));

	#if GTK_CHECK_VERSION(3,0,0)
		gtk_widget_insert_action_group(GTK_WIDGET(w->base.widget),"db",altwidgets_data.db_action_group);
			gtk_actionable_set_action_name(GTK_ACTIONABLE(w->base.widget),"db.toggle_menu");
			{
				DB_plugin_action_t *db_action = g_hash_table_lookup(altwidgets_data.db_action_map,"toggle_menu");
				if(db_action) gtk_widget_set_tooltip_text(w->base.widget,db_action->title);
			}
		gtk_style_context_add_class(gtk_widget_get_style_context(w->base.widget),"flat");
	#else
		w->action = g_hash_table_lookup(altwidgets_data.db_action_map,"toggle_menu");
	    g_signal_connect(w->base.widget,"toggled",G_CALLBACK(on_button_clicked),w);
	#endif

	gtk_widget_show(w->base.widget);
	gtkui_plugin->w_override_signals(w->base.widget,w);

	return(ddb_gtkui_widget_t*)w;
}
