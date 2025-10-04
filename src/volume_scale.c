#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include <stdbool.h>

extern DB_functions_t *deadbeef;
extern ddb_gtkui_t *gtkui_plugin;

struct volumescale{
	ddb_gtkui_widget_t base;
	guint volume_change_callback_id;
};

static void on_volumescale_change(GtkRange* self,__attribute__((unused)) gpointer user_data){
	deadbeef->volume_set_amp(gtk_range_get_value(self));
}

static gboolean volumescale_change(gpointer user_data){
	struct volumescale *data = (struct volumescale*)user_data;
	const GSignalMatchType mask = (GSignalMatchType)(G_SIGNAL_MATCH_FUNC);
	g_signal_handlers_block_matched(G_OBJECT(data->base.widget),mask,0,0,NULL,(gpointer)on_volumescale_change,data);
	gtk_range_set_value(GTK_RANGE(data->base.widget),deadbeef->volume_get_amp());
	g_signal_handlers_unblock_matched(G_OBJECT(data->base.widget),mask,0,0,NULL,(gpointer)on_volumescale_change,data);
	return G_SOURCE_REMOVE;
}
static void volumescale_change_on_callback_end(void *user_data){
	struct volumescale *data = (struct volumescale*)user_data;
	data->volume_change_callback_id = 0;
}

static void volumescale_init(ddb_gtkui_widget_t *w){
	struct volumescale *data = (struct volumescale*)w;
	volumescale_change(data);
}

static int volumescale_message(struct ddb_gtkui_widget_s *w,uint32_t id,__attribute__((unused)) uintptr_t ctx,__attribute__((unused)) uint32_t p1,__attribute__((unused)) uint32_t p2){
	struct volumescale *data = (struct volumescale*)w;
	switch(id){
		case DB_EV_VOLUMECHANGED:
			if(data->volume_change_callback_id == 0){
				data->volume_change_callback_id = g_idle_add_full(G_PRIORITY_LOW,volumescale_change,data,volumescale_change_on_callback_end);
			}
			break;
	}
	return 0;
}

ddb_gtkui_widget_t *volumescale_create(){
	struct volumescale *w = calloc(1,sizeof(struct volumescale));
	w->base.widget = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,0.0,1.0,0.05);
	w->base.init    = volumescale_init;
	w->base.message = volumescale_message;
	w->volume_change_callback_id = 0;

	g_signal_connect(w->base.widget,"value-changed",G_CALLBACK(on_volumescale_change),w);
	gtk_scale_set_draw_value(GTK_SCALE(w->base.widget),false);
	gtk_widget_set_size_request(w->base.widget,100,-1);
	gtk_widget_show(w->base.widget);
	gtkui_plugin->w_override_signals(w->base.widget,w);

	return (ddb_gtkui_widget_t*)w;
}
