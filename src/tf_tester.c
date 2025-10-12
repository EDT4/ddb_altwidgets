#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include <stdbool.h>
#include "deadbeef_util.h"
#include "main.h"

extern DB_functions_t *deadbeef;
extern ddb_gtkui_t *gtkui_plugin;

struct tftester{
	ddb_gtkui_widget_t base;
    ddb_gtkui_widget_extended_api_t exapi;
	GtkWidget *output_text_view;
	GtkTextBuffer *input_buffer;
	GtkTextBuffer *output_buffer;
	int callback_id;
	char *tf_code;
	char buffer[2000];
};

static void on_callback_end(void *user_data){
	struct tftester *data = user_data;
	data->callback_id = 0;
}
static bool output_update(struct tftester *data){
	gtk_text_buffer_set_text(data->output_buffer,"",0);
	ddb_playlist_t *plt = deadbeef->plt_get_curr();
	if(plt){
		ddb_playItem_t **items = NULL;
		ssize_t count = deadbeef->plt_get_selected_items(plt,&items);
		for(ssize_t i=0; i<count; i++) {
			ddb_tf_context_t ctx = {
				._size = sizeof(ddb_tf_context_t),
				.flags = DDB_TF_CONTEXT_MULTILINE,
				.plt = plt,
				.iter = PL_MAIN,
				.it = items[i],
			};
			size_t len = deadbeef->tf_eval(&ctx,data->tf_code,data->buffer,sizeof(data->buffer));
			if(len > 0){
				GtkTextIter end; gtk_text_buffer_get_end_iter(data->output_buffer,&end);
				gtk_text_buffer_insert(data->output_buffer,&end,data->buffer,len);
			}
			deadbeef->pl_item_unref(items[i]);
		}
		free(items);
		deadbeef->plt_unref(plt);
	}
	return G_SOURCE_REMOVE;
}

static void on_input_changed(GtkTextBuffer *buffer,gpointer user_data){
	struct tftester *data = (struct tftester*)user_data;

	//Input.
	GtkTextIter start,end;
	gtk_text_buffer_get_bounds(buffer,&start,&end);
	gchar *text = gtk_text_buffer_get_text(buffer,&start,&end,TRUE);

	//Text formatting.
	deadbeef->tf_free(data->tf_code);
	data->tf_code = deadbeef->tf_compile(text);

	//Output.
	output_update(data);
	g_free(text);
}

static int tftester_message(struct ddb_gtkui_widget_s *w,uint32_t id,__attribute__((unused)) uintptr_t ctx,uint32_t p1,__attribute__((unused)) uint32_t p2){
	struct tftester *data = (struct tftester*)w;
	if(id == DB_EV_PLAYLISTSWITCHED || (id == DB_EV_PLAYLISTCHANGED && p1 == (DDB_PLAYLIST_CHANGE_CONTENT || p1 == DDB_PLAYLIST_CHANGE_SELECTION))){
		if(data->callback_id == 0){
			data->callback_id = g_idle_add_full(G_PRIORITY_LOW,G_SOURCE_FUNC(output_update),data,on_callback_end);
		}
	}
	return 0;
}

static void tftester_deserialize_from_keyvalues(ddb_gtkui_widget_t *base,const char **keyvalues){
	struct tftester *data = (struct tftester*)base;
	for(size_t i=0; keyvalues[i]!=NULL; i+=2){
		if(strcmp(keyvalues[i],"input") == 0){
			gtk_text_buffer_set_text(data->input_buffer,keyvalues[i+1],-1);
		}
	}
}
static const char **tftester_serialize_to_keyvalues(ddb_gtkui_widget_t *base){
	struct tftester *data = (struct tftester*)base;
	char const **kv = calloc(3,sizeof(char *));
	kv[0] = "input";

	GtkTextIter start,end;
	gtk_text_buffer_get_bounds(data->input_buffer,&start,&end);
	kv[1] = gtk_text_buffer_get_text(data->input_buffer,&start,&end,TRUE);

	return kv;
}
static void tftester_free_serialized_keyvalues(__attribute__((unused)) ddb_gtkui_widget_t *w,char const **keyvalues){
	g_free((void*)(keyvalues[1]));
	free(keyvalues);
}

ddb_gtkui_widget_t *tftester_create(){
	struct tftester *w = calloc(1,sizeof(struct tftester));
	w->base.message = tftester_message;
	w->base.widget = gtk_box_new(GTK_ORIENTATION_VERTICAL,10);
		GtkWidget *input_scrolled = gtk_scrolled_window_new(NULL, NULL);
			gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(input_scrolled),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
			gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(input_scrolled),100);

			GtkWidget *input_text_view = gtk_text_view_new();
				gtk_text_view_set_editable(GTK_TEXT_VIEW(input_text_view),TRUE);
			gtk_container_add(GTK_CONTAINER(input_scrolled),input_text_view);
		gtk_box_pack_start(GTK_BOX(w->base.widget),input_scrolled,TRUE,TRUE,0);

		GtkWidget *output_scrolled = gtk_scrolled_window_new(NULL, NULL);
			gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(output_scrolled),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
			gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(output_scrolled),100);

			w->output_text_view = gtk_text_view_new();
				gtk_text_view_set_editable(GTK_TEXT_VIEW(w->output_text_view),FALSE);
			gtk_container_add(GTK_CONTAINER(output_scrolled),w->output_text_view);
		gtk_box_pack_start(GTK_BOX(w->base.widget),output_scrolled,TRUE,TRUE,0);

		w->input_buffer  = gtk_text_view_get_buffer(GTK_TEXT_VIEW(input_text_view));
		w->output_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w->output_text_view));
		g_signal_connect(w->input_buffer,"changed",G_CALLBACK(on_input_changed),w);

	w->exapi._size = sizeof(ddb_gtkui_widget_extended_api_t);
	w->exapi.deserialize_from_keyvalues = tftester_deserialize_from_keyvalues;
	w->exapi.serialize_to_keyvalues     = tftester_serialize_to_keyvalues;
	w->exapi.free_serialized_keyvalues  = tftester_free_serialized_keyvalues;

	w->tf_code = NULL;

	gtkui_plugin->w_override_signals(w->base.widget,w);
	gtk_widget_show_all(w->base.widget);

	return(ddb_gtkui_widget_t*)w;
}
