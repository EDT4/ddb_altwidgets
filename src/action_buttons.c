#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include <stdbool.h>
#include "deadbeef_util.h"

extern DB_functions_t *deadbeef;
extern ddb_gtkui_t *gtkui_plugin;

enum{
	BUTTON_PREV,
	BUTTON_PLAYPAUSE,
	BUTTON_STOP,
	BUTTON_NEXT,
	BUTTON_NEXT_SHUFFLE,
	BUTTON_JUMP_TO_CURRENT,

	BUTTON_COUNT
};

struct actionbuttons{
	ddb_gtkui_widget_t base;
	GtkWidget *buttons[BUTTON_COUNT];
	guint callback_id;
	GHashTable *db_action_map;
};

#define PLAY_IMAGE_NEW  gtk_image_new_from_icon_name("media-playback-start-symbolic",GTK_ICON_SIZE_SMALL_TOOLBAR)
#define PAUSE_IMAGE_NEW gtk_image_new_from_icon_name("media-playback-pause-symbolic",GTK_ICON_SIZE_SMALL_TOOLBAR)

static gboolean actionbuttons_on_play(gpointer user_data){
	struct actionbuttons *data = user_data;
	gtk_widget_set_sensitive(data->buttons[BUTTON_JUMP_TO_CURRENT],true);
	gtk_widget_set_sensitive(data->buttons[BUTTON_STOP],true);
	gtk_widget_set_sensitive(data->buttons[BUTTON_NEXT],true);
	gtk_widget_set_sensitive(data->buttons[BUTTON_PREV],true);
	gtk_button_set_image(GTK_BUTTON(data->buttons[BUTTON_PLAYPAUSE]),PAUSE_IMAGE_NEW);
	return G_SOURCE_REMOVE;
}
static gboolean actionbuttons_on_stop(gpointer user_data){
	struct actionbuttons *data = user_data;
	gtk_widget_set_sensitive(data->buttons[BUTTON_JUMP_TO_CURRENT],false);
	gtk_widget_set_sensitive(data->buttons[BUTTON_STOP],false);
	gtk_widget_set_sensitive(data->buttons[BUTTON_NEXT],false);
	gtk_widget_set_sensitive(data->buttons[BUTTON_PREV],false);
	gtk_button_set_image(GTK_BUTTON(data->buttons[BUTTON_PLAYPAUSE]),PLAY_IMAGE_NEW);
	return G_SOURCE_REMOVE;
}
static gboolean actionbuttons_on_unpause(gpointer user_data){
	struct actionbuttons *data = user_data;
	gtk_button_set_image(GTK_BUTTON(data->buttons[BUTTON_PLAYPAUSE]),PAUSE_IMAGE_NEW);
	return G_SOURCE_REMOVE;
}
static gboolean actionbuttons_on_pause(gpointer user_data){
	struct actionbuttons *data = user_data;
	gtk_button_set_image(GTK_BUTTON(data->buttons[BUTTON_PLAYPAUSE]),PLAY_IMAGE_NEW);
	return G_SOURCE_REMOVE;
}
static void actionbuttons_on_callback_end(void *user_data){
	struct actionbuttons *data = user_data;
	data->callback_id = 0;
}

static void actionbuttons_init(ddb_gtkui_widget_t *w){
	struct actionbuttons *data = (struct actionbuttons*)w;

	//Initial state.
	struct DB_output_s* output = deadbeef->get_output();
	if(output){
		switch(output->state()){
			case DDB_PLAYBACK_STATE_STOPPED:
				if(data->callback_id != 0) g_source_remove(data->callback_id);
				data->callback_id = g_idle_add_full(G_PRIORITY_LOW,actionbuttons_on_stop,data,actionbuttons_on_callback_end);
				break;
			case DDB_PLAYBACK_STATE_PLAYING:
				if(data->callback_id != 0) g_source_remove(data->callback_id);
				data->callback_id = g_idle_add_full(G_PRIORITY_LOW,actionbuttons_on_play,data,actionbuttons_on_callback_end);
				break;
			case DDB_PLAYBACK_STATE_PAUSED:
				if(data->callback_id != 0) g_source_remove(data->callback_id);
				data->callback_id = g_idle_add_full(G_PRIORITY_LOW,actionbuttons_on_pause,data,actionbuttons_on_callback_end);
				break;
		}
	}
}

static void actionbuttons_destroy(ddb_gtkui_widget_t *w){
	struct actionbuttons *data = (struct actionbuttons*)w;
	g_hash_table_remove_all(data->db_action_map);
	g_hash_table_unref(data->db_action_map);
}

static int actionbuttons_message(struct ddb_gtkui_widget_s *w,uint32_t id,__attribute__((unused)) uintptr_t ctx,uint32_t p1,__attribute__((unused)) uint32_t p2){
	struct actionbuttons *data = (struct actionbuttons*)w;
	//TODO: MAybe instead of sentivitiy, set enabled on actions.
	switch(id){
		case DB_EV_SONGFINISHED:
			if(data->callback_id != 0) g_source_remove(data->callback_id);
			data->callback_id = g_idle_add_full(G_PRIORITY_LOW,actionbuttons_on_stop,data,actionbuttons_on_callback_end);
			break;
		case DB_EV_SONGSTARTED:
			if(data->callback_id != 0) g_source_remove(data->callback_id);
			data->callback_id = g_idle_add_full(G_PRIORITY_LOW,actionbuttons_on_play,data,actionbuttons_on_callback_end);
			break;
		case DB_EV_PAUSED:
			if(data->callback_id != 0) g_source_remove(data->callback_id);
			data->callback_id = g_idle_add_full(G_PRIORITY_LOW,p1? actionbuttons_on_pause : actionbuttons_on_unpause,data,actionbuttons_on_callback_end);
			break;
	}
	return 0;
}

#define BUTTON_ACTION_INIT(button,name) \
	gtk_actionable_set_action_name(GTK_ACTIONABLE(button),"db." name);\
	{\
		DB_plugin_action_t *db_action = g_hash_table_lookup(w->db_action_map,name);\
		if(db_action) gtk_widget_set_tooltip_text(button,db_action->title);\
	}

ddb_gtkui_widget_t *actionbuttons_create(){
	struct actionbuttons *w = calloc(1,sizeof(struct actionbuttons));
	w->base.widget = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	w->base.init    = actionbuttons_init;
	w->base.destroy = actionbuttons_destroy;
	w->base.message = actionbuttons_message;
	w->callback_id = 0;
	w->db_action_map = g_hash_table_new_full(g_str_hash,g_str_equal,free,NULL); //TODO: Should this be in create or init (here)?

	GActionGroup *action_group = deadbeef_action_group(w->db_action_map);

		#define button w->buttons[BUTTON_PREV]
		button = gtk_button_new_from_icon_name("media-skip-backward-symbolic",GTK_ICON_SIZE_SMALL_TOOLBAR);
			BUTTON_ACTION_INIT(button,"prev_or_restart");
			gtk_widget_show(button);
		gtk_box_pack_start(GTK_BOX(w->base.widget),button,false,false,0);
		#undef button

		#define button w->buttons[BUTTON_PLAYPAUSE]
		button = gtk_button_new();
			gtk_button_set_image(GTK_BUTTON(button),PLAY_IMAGE_NEW);
			BUTTON_ACTION_INIT(button,"play_pause");
			gtk_widget_show(button);
		gtk_box_pack_start(GTK_BOX(w->base.widget),button,false,false,0);
		#undef button

		#define button w->buttons[BUTTON_STOP]
		button = gtk_button_new_from_icon_name("media-playback-stop-symbolic",GTK_ICON_SIZE_SMALL_TOOLBAR);
			BUTTON_ACTION_INIT(button,"stop");
			gtk_widget_show(button);
		gtk_box_pack_start(GTK_BOX(w->base.widget),button,false,false,0);
		#undef button

		#define button w->buttons[BUTTON_NEXT]
		button = gtk_button_new_from_icon_name("media-skip-forward-symbolic",GTK_ICON_SIZE_SMALL_TOOLBAR);
			BUTTON_ACTION_INIT(button,"next");
			gtk_widget_show(button);
		gtk_box_pack_start(GTK_BOX(w->base.widget),button,false,false,0);
		#undef button

		#define button w->buttons[BUTTON_NEXT_SHUFFLE]
		button = gtk_button_new_from_icon_name("media-playlist-shuffle-symbolic",GTK_ICON_SIZE_SMALL_TOOLBAR);
			BUTTON_ACTION_INIT(button,"playback_random");
			gtk_widget_show(button);
		gtk_box_pack_start(GTK_BOX(w->base.widget),button,false,false,0);
		#undef button

		#define button w->buttons[BUTTON_JUMP_TO_CURRENT]
		button = gtk_button_new_from_icon_name("edit-select-symbolic",GTK_ICON_SIZE_SMALL_TOOLBAR);
			BUTTON_ACTION_INIT(button,"jump_to_current_track");
			gtk_widget_show(button);
		gtk_box_pack_start(GTK_BOX(w->base.widget),button,false,false,0);
		#undef button

		//GtkWidget *button;
		//button = gtk_button_new_from_icon_name("preferences-other-symbolic",GTK_ICON_SIZE_SMALL_TOOLBAR);
		//	BUTTON_ACTION_INIT(button,"preferences");
		//	gtk_style_context_add_class(gtk_widget_get_style_context(button),"flat");
		//	gtk_widget_show(button);
		//gtk_box_pack_start(GTK_BOX(w->base.widget),button,false,false,0);
		//
		//data->menu_button = gtk_button_new_from_icon_name("open-menu-symbolic",GTK_ICON_SIZE_SMALL_TOOLBAR);
		//	g_signal_connect(data->menu_button,"clicked",G_CALLBACK(on_menubar_toggle),window);
		//	gtk_widget_set_tooltip_text(data->menu_button,"Toggle menu bar"); //TODO: action_toggle_menu? toggle_menu? See all actions in deadbeef/plugins/gtkui/gtkui.c and all the DB_plugin_action_t.
		//	gtk_style_context_add_class(gtk_widget_get_style_context(data->menu_button),"flat");
		//	gtk_widget_show(data->menu_button);
		//gtk_box_pack_start(GTK_BOX(w->base.widget),data->menu_button,false,false,0);

		gtk_button_box_set_layout(GTK_BUTTON_BOX(w->base.widget),GTK_BUTTONBOX_EXPAND);

	gtk_widget_insert_action_group(GTK_WIDGET(w->base.widget),"db",action_group);

	gtk_widget_show(w->base.widget);
	gtkui_plugin->w_override_signals(w->base.widget,w);

	return (ddb_gtkui_widget_t*)w;
}
