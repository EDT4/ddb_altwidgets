#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include <stdbool.h>
#include <math.h>
#include "gtk2.h"

extern DB_functions_t *deadbeef;
extern ddb_gtkui_t *gtkui_plugin;

struct volumescale{
	ddb_gtkui_widget_t base;
    ddb_gtkui_widget_extended_api_t exapi;
	guint volume_change_callback_id;
	struct{
		enum scaling{
			SCALING_LINEAR,
			SCALING_CUBIC,
			SCALING_DECIBEL,
		} scaling;
		double step1;
		double step2;
		int width;
		int height;
	} options;
};
#define SCALING_COUNT 3

static void on_volumescale_change(GtkRange* self,__attribute__((unused)) gpointer user_data){
	struct volumescale *data = (struct volumescale*)user_data;
	double value = gtk_range_get_value(self);
	switch(data->options.scaling){
		case SCALING_CUBIC:
			value = value*value*value;
			__attribute__ ((fallthrough));
		case SCALING_LINEAR:
			deadbeef->volume_set_amp(value);
			break;
		case SCALING_DECIBEL:
			deadbeef->volume_set_db(deadbeef->volume_get_min_db() * (1.0 - value));
			break;
	}
}

static gboolean volumescale_change(gpointer user_data){
	struct volumescale *data = (struct volumescale*)user_data;
	const GSignalMatchType mask = (GSignalMatchType)(G_SIGNAL_MATCH_FUNC);
	const double amp = deadbeef->volume_get_amp();
	const double db  = deadbeef->volume_get_db();
	double value;
	switch(data->options.scaling){
		case SCALING_LINEAR:
			value = amp;
			break;
		case SCALING_CUBIC:
			value = cbrt(amp);
			break;
		case SCALING_DECIBEL:
			value = 1.0 - db / deadbeef->volume_get_min_db();
			break;
	}

	{
		char buffer[40]; //TODO: Display in cubic scale?
		snprintf(buffer,sizeof(buffer),"%.2f%% (%f dB)",amp*100.0,db);
		gtk_widget_set_tooltip_text(data->base.widget,buffer);
	}

	g_signal_handlers_block_matched(G_OBJECT(data->base.widget),mask,0,0,NULL,(gpointer)on_volumescale_change,data);
	gtk_range_set_value(GTK_RANGE(data->base.widget),value);
	g_signal_handlers_unblock_matched(G_OBJECT(data->base.widget),mask,0,0,NULL,(gpointer)on_volumescale_change,data);

	return G_SOURCE_REMOVE;
}
static void volumescale_change_on_callback_end(void *user_data){
	struct volumescale *data = (struct volumescale*)user_data;
	data->volume_change_callback_id = 0;
}

static void volumescale_init(ddb_gtkui_widget_t *w){
	struct volumescale *data = (struct volumescale*)w;
	gtk_scale_set_draw_value(GTK_SCALE(data->base.widget),false);
	gtk_range_set_increments(GTK_RANGE(data->base.widget),data->options.step1,data->options.step2);
	gtk_widget_set_size_request(data->base.widget,data->options.width,data->options.height);
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

static void on_volumebar_scale_select(GtkWidget *item,struct ddb_gtkui_widget_s *w){
	struct volumescale *data = (struct volumescale*)w;
	data->options.scaling = (size_t)g_object_get_data(G_OBJECT(item),"option");
	volumescale_change(data);
}
static const char *volumebar_menuitems[SCALING_COUNT] = {"Linear Scale","Cubic Scale","dB Scale"};
static gboolean on_volumebar_click(__attribute__((unused)) GtkWidget *widget,GdkEventButton *event,gpointer user_data){
	struct volumescale *data = (struct volumescale*)user_data;
	if(event->type == GDK_BUTTON_PRESS && event->button == 3){ //Right-click
		GtkWidget *menu = gtk_menu_new ();
		{
			GtkWidget *item[SCALING_COUNT];
			GSList *group = NULL;
			for(size_t i=0; i<SCALING_COUNT; i+=1){
				item[i] = gtk_radio_menu_item_new_with_label(group,volumebar_menuitems[i]);
					g_signal_connect(item[i],"toggled",G_CALLBACK(on_volumebar_scale_select),user_data);
					g_object_set_data(G_OBJECT(item[i]),"option",(gpointer)i);
					gtk_widget_show(item[i]);
				gtk_container_add(GTK_CONTAINER(menu),item[i]);
				group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item[i]));
			}
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item[data->options.scaling]),true);
		}
		gtk_menu_attach_to_widget(GTK_MENU(menu),data->base.widget,NULL);
		gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,0,gtk_get_current_event_time());
		return true;
	}
	return false;
}

static const char *volumebar_scaling_str[SCALING_COUNT] = {"linear","cubic","db"};
static void volumescale_deserialize_from_keyvalues(ddb_gtkui_widget_t *base,const char **keyvalues){
	struct volumescale *data = (struct volumescale*)base;
	for(size_t i=0; keyvalues[i]!=NULL; i+=2){
		if(strcmp(keyvalues[i],"scale") == 0){
			for(size_t j=0; j<SCALING_COUNT; j+=1){
				if(strcmp(keyvalues[i+1],volumebar_scaling_str[j]) == 0){
					data->options.scaling = j;
					break;
				}
			}
		}else if(strcmp(keyvalues[i],"step1") == 0){
			data->options.step1 = atof(keyvalues[i+1]);
		}else if(strcmp(keyvalues[i],"step2") == 0){
			data->options.step2 = atof(keyvalues[i+1]);
		}else if(strcmp(keyvalues[i],"width") == 0){
			data->options.width = atoi(keyvalues[i+1]);
		}else if(strcmp(keyvalues[i],"height") == 0){
			data->options.height = atoi(keyvalues[i+1]);
		}
	}
}
static const char **volumescale_serialize_to_keyvalues(ddb_gtkui_widget_t *base){
	struct volumescale *data = (struct volumescale*)base;
	#define BUFFER_LEN 20
	#define ENTRIES 5
	char const **kv = calloc(ENTRIES * 2 + 1,sizeof(char *));

	kv[0] = "scale";
	kv[1] = volumebar_scaling_str[data->options.scaling];

	kv[2] = "step1";
	kv[3] = malloc(BUFFER_LEN);
	snprintf((char*)(kv[3]),BUFFER_LEN,"%f",data->options.step1);

	kv[4] = "step2";
	kv[5] = malloc(BUFFER_LEN);
	snprintf((char*)(kv[5]),BUFFER_LEN,"%f",data->options.step2);

	kv[6] = "width";
	kv[7] = malloc(BUFFER_LEN);
	snprintf((char*)(kv[7]),BUFFER_LEN,"%d",data->options.width);

	kv[8] = "height";
	kv[9] = malloc(BUFFER_LEN);
	snprintf((char*)(kv[9]),BUFFER_LEN,"%d",data->options.height);

	return kv;
}
static void volumescale_free_serialized_keyvalues(__attribute__((unused)) ddb_gtkui_widget_t *w,char const **keyvalues){
	free((char*)(keyvalues[3]));
	free((char*)(keyvalues[5]));
	free((char*)(keyvalues[7]));
	free((char*)(keyvalues[9]));
	free(keyvalues);
}

ddb_gtkui_widget_t *volumescale_create(){
	struct volumescale *w = calloc(1,sizeof(struct volumescale));
	w->base.widget = gtk_hscale_new_with_range(0.0,1.0,0.01);
	w->base.init    = volumescale_init;
	w->base.message = volumescale_message;
	w->exapi._size = sizeof(ddb_gtkui_widget_extended_api_t);
	w->exapi.deserialize_from_keyvalues = volumescale_deserialize_from_keyvalues;
	w->exapi.serialize_to_keyvalues     = volumescale_serialize_to_keyvalues;
	w->exapi.free_serialized_keyvalues  = volumescale_free_serialized_keyvalues;
	w->volume_change_callback_id = 0;
	w->options.scaling = SCALING_LINEAR;
	w->options.step1 = 0.01;
	w->options.step2 = 0.05;
	w->options.width = 100;
	w->options.height = -1;

    g_signal_connect(w->base.widget,"button_press_event",G_CALLBACK(on_volumebar_click),w);
	g_signal_connect(w->base.widget,"value-changed",G_CALLBACK(on_volumescale_change),w);
	gtk_widget_show(w->base.widget);
	gtkui_plugin->w_override_signals(w->base.widget,w);

	return (ddb_gtkui_widget_t*)w;
}
