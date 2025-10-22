#include <gtk/gtk.h>

#if GTK_CHECK_VERSION(3,0,0)

#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include <stdbool.h>
#include <math.h>

extern DB_functions_t *deadbeef;
extern ddb_gtkui_t *gtkui_plugin;

struct ratingscale{
	ddb_gtkui_widget_t base;
	ddb_gtkui_widget_extended_api_t exapi;
	unsigned char granularity;
	unsigned char marks;
	guint change_callback_id;
	float min_value;
	float max_value;
	char meta_name[100];
};

const double granularity_values[] = { 1.0 , 0.1 , 0.01 , 0.001 };
const char *granularity_strings[] = {"1.0","0.1","0.01","0.001"};
const char *css_style =
	"scale{padding:16px;}"
	"scale trough{margin:0;border:0;background:-gtk-icontheme('non-starred-symbolic') repeat-x; min-width:calc(32px*5); min-height:32px;}"
	"scale trough highlight{margin:0;border:0;background:-gtk-icontheme('starred-symbolic') repeat-x; min-width:32px; min-height:32px;}"
	"scale trough highlight{min-width:0; background-size: 32px;}"
	"scale slider{background:transparent;min-width:0;min-height:0;margin:0;}"
;

static void ratingscale_init(ddb_gtkui_widget_t *w);

static void ratingscale_init_granularity(struct ratingscale *data){
	gtk_range_set_round_digits(GTK_RANGE(data->base.widget),data->granularity);
	gtk_range_set_increments(GTK_RANGE(data->base.widget),granularity_values[data->granularity],data->granularity == 0? granularity_values[data->granularity] : granularity_values[data->granularity]*5.0);
	gtk_scale_set_digits(GTK_SCALE(data->base.widget),data->granularity);
}

static void ratingscale_init_marks(struct ratingscale *data){
	gtk_scale_clear_marks(GTK_SCALE(data->base.widget));
	for(unsigned char i=0; i<=data->marks; i+=1){
		gtk_scale_add_mark(GTK_SCALE(data->base.widget),((double)i)*5.0/((double)data->marks),GTK_POS_BOTTOM,NULL);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Configuration.
//

static void ratingscale_on_configure_close(GtkWidget* self,__attribute__((unused)) void* user_data){
	gtk_widget_destroy(GTK_WIDGET(self)); //TODO: Is this necessary?
}
static void on_marks_spin_changed(GtkSpinButton* self,gpointer user_data){
	struct ratingscale *data = (struct ratingscale*)user_data;
	data->marks = (unsigned char)gtk_spin_button_get_value_as_int(self);
	ratingscale_init_marks(data);
}
static void on_granularity_combo_changed(GtkComboBox* self,gpointer user_data){
	struct ratingscale *data = (struct ratingscale*)user_data;
	data->granularity = (unsigned char)gtk_combo_box_get_active(self);
	ratingscale_init_granularity(data);
}
static void ratingscale_on_configure_activate(__attribute__((unused)) GtkMenuItem* self,struct ratingscale *data){
	GtkWidget *dialog = gtk_dialog_new_with_buttons(
		"Rating Scale Configuration",
		NULL, //TODO: What is the parent of this?
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		NULL,
		NULL
	);
	g_signal_connect(dialog,"close",G_CALLBACK(ratingscale_on_configure_close),data);
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,10);
		gtk_container_set_border_width(GTK_CONTAINER(vbox),10);
		{
			GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);
				gtk_container_add(GTK_CONTAINER(hbox),GTK_WIDGET(gtk_label_new("Marks:")));

				GtkSpinButton *marks_spin = GTK_SPIN_BUTTON(gtk_spin_button_new_with_range(1.0,20.0,1.0));
					g_signal_connect(marks_spin,"value-changed",G_CALLBACK(on_marks_spin_changed),data);
					gtk_spin_button_set_value(marks_spin,(double)data->marks);
				gtk_container_add(GTK_CONTAINER(hbox),GTK_WIDGET(marks_spin));
			gtk_container_add(GTK_CONTAINER(vbox),GTK_WIDGET(hbox));
		}

		{
			GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);
				gtk_container_add(GTK_CONTAINER(hbox),GTK_WIDGET(gtk_label_new("Granularity:")));

				GtkComboBoxText *granularity_combo = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
					for(unsigned char i=0; i<sizeof(granularity_strings)/sizeof(granularity_strings[0]); i+=1){
						gtk_combo_box_text_append_text(granularity_combo,granularity_strings[i]);
					}
					gtk_combo_box_set_active(GTK_COMBO_BOX(granularity_combo),data->granularity);
					g_signal_connect(granularity_combo,"changed",G_CALLBACK(on_granularity_combo_changed),data);
				gtk_container_add(GTK_CONTAINER(hbox),GTK_WIDGET(granularity_combo));
			gtk_container_add(GTK_CONTAINER(vbox),GTK_WIDGET(hbox));
		}
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),vbox);
	gtk_widget_show_all(dialog);
}

void ratingscale_initmenu(struct ddb_gtkui_widget_s *w,GtkWidget *menu){
	GtkWidget *item = gtk_menu_item_new_with_label("Configure");
	g_signal_connect(G_OBJECT(item),"activate",G_CALLBACK(ratingscale_on_configure_activate),w);
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu),item);
}

///////////////////////////////////////////////////////////////////////////////
// Save/load.
//

static void ratingscale_deserialize_from_keyvalues(ddb_gtkui_widget_t *base,const char **keyvalues){
	struct ratingscale *data = (struct ratingscale*)base;
	for(size_t i=0; keyvalues[i]!=NULL; i+=2){
		if(strcmp(keyvalues[i],"granularity") == 0){
			data->granularity = atoi(keyvalues[i+1]);
		}else if(strcmp(keyvalues[i],"marks") == 0){
			data->marks = atoi(keyvalues[i+1]);
		}else if(strcmp(keyvalues[i],"min_value") == 0){
			data->min_value = atof(keyvalues[i+1]);
		}else if(strcmp(keyvalues[i],"max_value") == 0){
			data->max_value = atof(keyvalues[i+1]);
		}else if(strcmp(keyvalues[i],"meta_name") == 0){
			strlcpy(data->meta_name,keyvalues[i+1],sizeof(data->meta_name));
		}
	}
}
#define KEYVALUES_COUNT 5
static const char **ratingscale_serialize_to_keyvalues(ddb_gtkui_widget_t *base){
	struct ratingscale *data = (struct ratingscale*)base;
	char const **kv = calloc(KEYVALUES_COUNT*2+1,sizeof(char *));

	kv[0] = "granularity";
	kv[1] = g_strdup_printf("%u",(unsigned int)data->granularity);

	kv[2] = "marks";
	kv[3] = g_strdup_printf("%u",(unsigned int)data->marks);

	kv[4] = "min_value";
	kv[5] = g_strdup_printf("%f",data->min_value);

	kv[6] = "max_value";
	kv[7] = g_strdup_printf("%f",data->max_value);

	if(data->meta_name[0]){
		kv[8] = "meta_name";
		kv[9] = data->meta_name;
	}

	return kv;
}
static void ratingscale_free_serialized_keyvalues(__attribute__((unused)) ddb_gtkui_widget_t *w,char const **keyvalues){
	free((char*)(keyvalues[1]));
	free((char*)(keyvalues[3]));
	free((char*)(keyvalues[5]));
	free((char*)(keyvalues[7]));
	free(keyvalues);
}

///////////////////////////////////////////////////////////////////////////////
// Main logic.
//

static gboolean ratingscale_change(gpointer user_data){
	struct ratingscale *data = (struct ratingscale*)user_data;

	DB_playItem_t * track = deadbeef->streamer_get_playing_track_safe();
	if(track){
		float rating = deadbeef->pl_find_meta_float(track,data->meta_name,NAN);
		deadbeef->pl_item_unref(track);

		gtk_widget_set_sensitive(data->base.widget,true);
		gtk_range_set_value(GTK_RANGE(data->base.widget),isfinite(rating)? 5.0 * ((double)rating - (double)data->min_value) / (fabs(data->min_value) + fabs(data->max_value)) : 0.0);
	}else{
		gtk_range_set_value(GTK_RANGE(data->base.widget),0.0);
		gtk_widget_set_sensitive(data->base.widget,false);
	}
	return G_SOURCE_REMOVE;
}
static void ratingscale_change_on_callback_end(void *user_data){
	struct ratingscale *data = (struct ratingscale*)user_data;
	data->change_callback_id = 0;
}
static int ratingscale_message(struct ddb_gtkui_widget_s *w,uint32_t id,__attribute__((unused)) uintptr_t ctx,__attribute__((unused)) uint32_t p1,__attribute__((unused)) uint32_t p2){
	struct ratingscale *data = (struct ratingscale*)w;
	if(data->meta_name[0] == '\0') return 0;
	switch(id){
		case DB_EV_SONGSTARTED:
		case DB_EV_SONGFINISHED:
			if(data->change_callback_id == 0){
				data->change_callback_id = g_timeout_add_seconds_full(G_PRIORITY_LOW,1,ratingscale_change,data,ratingscale_change_on_callback_end);
			}
			break;
	}
	return 0;
}

static void ratingscale_init(ddb_gtkui_widget_t *w){
	struct ratingscale *data = (struct ratingscale*)w;

	ratingscale_init_granularity(data);
	ratingscale_init_marks(data);
	gtk_scale_set_draw_value(GTK_SCALE(data->base.widget),TRUE);
	gtk_scale_set_value_pos(GTK_SCALE(data->base.widget),GTK_POS_BOTTOM);
	ratingscale_change(data);
}

ddb_gtkui_widget_t *ratingscale_create(){
	struct ratingscale *w = calloc(1,sizeof(struct ratingscale));
	w->base.widget = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL,NULL);
	w->base.init = ratingscale_init;
	w->base.initmenu = ratingscale_initmenu;
	w->base.message = ratingscale_message;
	w->exapi._size = sizeof(ddb_gtkui_widget_extended_api_t);
	w->exapi.deserialize_from_keyvalues = ratingscale_deserialize_from_keyvalues;
	w->exapi.serialize_to_keyvalues     = ratingscale_serialize_to_keyvalues;
	w->exapi.free_serialized_keyvalues  = ratingscale_free_serialized_keyvalues;
	w->granularity = 1;
	w->marks = 5;
	w->change_callback_id = 0;
	w->min_value = 0.0;
	w->max_value = 5.0;
	w->meta_name[0] = '\0';

	gtk_range_set_range(GTK_RANGE(w->base.widget),0.0,5.0);
	gtk_widget_show(w->base.widget);
	gtkui_plugin->w_override_signals(w->base.widget,w);

	GtkCssProvider *provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data(provider,css_style,-1,NULL);
	gtk_style_context_add_provider(gtk_widget_get_style_context(w->base.widget),GTK_STYLE_PROVIDER(provider),GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	g_object_unref(provider);

	return (ddb_gtkui_widget_t*)w;
}

#endif
