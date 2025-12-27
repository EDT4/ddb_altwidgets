#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include <stdbool.h>
#include "../deadbeef_util.h"
#include "../gtk2.h"
#include "../main.h"

extern DB_functions_t *deadbeef;
extern ddb_gtkui_t *gtkui_plugin;
extern struct altwidgets altwidgets_data;

enum{
	BUTTON_PREV,
	BUTTON_PLAYPAUSE,
	BUTTON_STOP,
	BUTTON_NEXT,
	BUTTON_NEXT_SHUFFLE,
	BUTTON_JUMP_TO_CURRENT,

	BUTTON_COUNT
};

struct playbackbuttonsalt{
	ddb_gtkui_widget_t base;
	GtkWidget *buttons[BUTTON_COUNT];
	guint callback_id;
	GtkWidget *play_image;
	GtkWidget *pause_image;
};

static gboolean g_on_play(gpointer user_data){
	struct playbackbuttonsalt *data = user_data;
	gtk_widget_set_sensitive(data->buttons[BUTTON_JUMP_TO_CURRENT],true);
	gtk_widget_set_sensitive(data->buttons[BUTTON_STOP],true);
	gtk_widget_set_sensitive(data->buttons[BUTTON_NEXT],true);
	gtk_widget_set_sensitive(data->buttons[BUTTON_PREV],true);
	gtk_button_set_image(GTK_BUTTON(data->buttons[BUTTON_PLAYPAUSE]),data->pause_image);
	return G_SOURCE_REMOVE;
}
static gboolean g_on_stop(gpointer user_data){
	struct playbackbuttonsalt *data = user_data;
	gtk_widget_set_sensitive(data->buttons[BUTTON_JUMP_TO_CURRENT],false);
	gtk_widget_set_sensitive(data->buttons[BUTTON_STOP],false);
	gtk_widget_set_sensitive(data->buttons[BUTTON_NEXT],!!deadbeef->playqueue_get_count());
	gtk_widget_set_sensitive(data->buttons[BUTTON_PREV],false);
	gtk_button_set_image(GTK_BUTTON(data->buttons[BUTTON_PLAYPAUSE]),data->play_image);
	return G_SOURCE_REMOVE;
}
static gboolean g_on_unpause(gpointer user_data){
	struct playbackbuttonsalt *data = user_data;
	gtk_button_set_image(GTK_BUTTON(data->buttons[BUTTON_PLAYPAUSE]),data->pause_image);
	return G_SOURCE_REMOVE;
}
static gboolean g_on_queue(gpointer user_data){
	struct playbackbuttonsalt *data = user_data;
	bool b = !!deadbeef->playqueue_get_count();
	struct DB_output_s* output = deadbeef->get_output();
	if(output){b = b || output->state() != DDB_PLAYBACK_STATE_STOPPED;}
	gtk_widget_set_sensitive(data->buttons[BUTTON_NEXT],b);
	return G_SOURCE_REMOVE;
}
static gboolean g_on_pause(gpointer user_data){
	struct playbackbuttonsalt *data = user_data;
	gtk_button_set_image(GTK_BUTTON(data->buttons[BUTTON_PLAYPAUSE]),data->play_image);
	return G_SOURCE_REMOVE;
}
static void g_on_callback_end(void *user_data){
	struct playbackbuttonsalt *data = user_data;
	data->callback_id = 0;
}

static void playbackbuttonsalt_init(ddb_gtkui_widget_t *w){
	struct playbackbuttonsalt *data = (struct playbackbuttonsalt*)w;

	//Initial state.
	struct DB_output_s* output = deadbeef->get_output();
	if(output){
		switch(output->state()){
			case DDB_PLAYBACK_STATE_STOPPED:
				if(data->callback_id != 0) g_source_remove(data->callback_id);
				data->callback_id = g_idle_add_full(G_PRIORITY_LOW,g_on_stop,data,g_on_callback_end);
				break;
			case DDB_PLAYBACK_STATE_PLAYING:
				if(data->callback_id != 0) g_source_remove(data->callback_id);
				data->callback_id = g_idle_add_full(G_PRIORITY_LOW,g_on_play,data,g_on_callback_end);
				break;
			case DDB_PLAYBACK_STATE_PAUSED:
				if(data->callback_id != 0) g_source_remove(data->callback_id);
				data->callback_id = g_idle_add_full(G_PRIORITY_LOW,g_on_pause,data,g_on_callback_end);
				break;
		}
	}
}

static int playbackbuttonsalt_message(struct ddb_gtkui_widget_s *w,uint32_t id,__attribute__((unused)) uintptr_t ctx,uint32_t p1,__attribute__((unused)) uint32_t p2){
	struct playbackbuttonsalt *data = (struct playbackbuttonsalt*)w;
	//TODO: MAybe instead of sentivitiy, set enabled on actions.
	switch(id){
		case DB_EV_SONGFINISHED:
			if(data->callback_id != 0) g_source_remove(data->callback_id);
			data->callback_id = g_idle_add_full(G_PRIORITY_LOW,g_on_stop,data,g_on_callback_end);
			break;
		case DB_EV_SONGSTARTED:
			if(data->callback_id != 0) g_source_remove(data->callback_id);
			data->callback_id = g_idle_add_full(G_PRIORITY_LOW,g_on_play,data,g_on_callback_end);
			break;
		case DB_EV_PAUSED:
			if(data->callback_id != 0) g_source_remove(data->callback_id);
			data->callback_id = g_idle_add_full(G_PRIORITY_LOW,p1? g_on_pause : g_on_unpause,data,g_on_callback_end);
			break;
		case DB_EV_TRACKINFOCHANGED:
		case DB_EV_PLAYLISTCHANGED:
			if(p1 == DDB_PLAYLIST_CHANGE_PLAYQUEUE){
				if(data->callback_id != 0) g_source_remove(data->callback_id);
				data->callback_id = g_idle_add_full(G_PRIORITY_LOW,g_on_queue,data,g_on_callback_end);
			}
			break;
	}
	return 0;
}

#if GTK_CHECK_VERSION(3,0,0)
	#define BUTTON_ACTION_INIT(button,name) \
		gtk_actionable_set_action_name(GTK_ACTIONABLE(button),"db." name);\
		{\
			DB_plugin_action_t *db_action = g_hash_table_lookup(altwidgets_data.db_action_map,name);\
			if(db_action) gtk_widget_set_tooltip_text(button,db_action->title);\
		}
#else
	static void on_actionbutton_clicked(GtkToggleButton *button,__attribute__((unused)) gpointer user_data){
		DB_plugin_action_t * action = g_object_get_data(G_OBJECT(button),"action");
		if(action) action_call(action,DDB_ACTION_CTX_MAIN);
	}

	#define BUTTON_ACTION_INIT(button,name) \
		{\
			DB_plugin_action_t *db_action = g_hash_table_lookup(altwidgets_data.db_action_map,name);\
			if(db_action){\
				gtk_widget_set_tooltip_text(button,db_action->title);\
				g_object_set_data(G_OBJECT(button),"action",db_action);\
				g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_actionbutton_clicked),NULL);\
			}\
		}
#endif

void playbackbuttonsalt_destroy(struct ddb_gtkui_widget_s *w){
	struct playbackbuttonsalt *data = (struct playbackbuttonsalt*)w;
	g_object_unref(G_OBJECT(data->play_image));
	g_object_unref(G_OBJECT(data->pause_image));
}

ddb_gtkui_widget_t *playbackbuttonsalt_create(){
	struct playbackbuttonsalt *w = calloc(1,sizeof(struct playbackbuttonsalt));
	w->base.widget = gtk_hbutton_box_new();
	w->base.init    = playbackbuttonsalt_init;
	w->base.destroy = playbackbuttonsalt_destroy;
	w->base.message = playbackbuttonsalt_message;
	w->callback_id = 0;
	w->play_image  = gtk_image_new_from_icon_name("media-playback-start-symbolic",GTK_ICON_SIZE_SMALL_TOOLBAR);
	w->pause_image = gtk_image_new_from_icon_name("media-playback-pause-symbolic",GTK_ICON_SIZE_SMALL_TOOLBAR);

	//Increase ref count due to the handling of set_image.
	g_object_ref(G_OBJECT(w->play_image));
	g_object_ref(G_OBJECT(w->pause_image));

	#if GTK_CHECK_VERSION(3,0,0)
	gtk_widget_insert_action_group(GTK_WIDGET(w->base.widget),"db",altwidgets_data.db_action_group);
	#endif

		#define button w->buttons[BUTTON_PREV]
		button = gtk_button_new_from_icon_name("media-skip-backward-symbolic",GTK_ICON_SIZE_SMALL_TOOLBAR);
			BUTTON_ACTION_INIT(button,"prev_or_restart");
			gtk_widget_show(button);
		gtk_box_pack_start(GTK_BOX(w->base.widget),button,false,false,0);
		#undef button

		#define button w->buttons[BUTTON_PLAYPAUSE]
		button = gtk_button_new();
			gtk_button_set_image(GTK_BUTTON(button),w->play_image);
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

		gtk_button_box_set_layout(GTK_BUTTON_BOX(w->base.widget),GTK_BUTTONBOX_EXPAND);

	gtk_widget_show(w->base.widget);
	gtkui_plugin->w_override_signals(w->base.widget,w);

	return (ddb_gtkui_widget_t*)w;
}
