#include <gtk/gtk.h>

#if GTK_CHECK_VERSION(3,0,0)

#include <assert.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include <stdbool.h>
#include <math.h>

extern DB_functions_t *deadbeef;
extern ddb_gtkui_t *gtkui_plugin;

struct popovertoggle{
	ddb_gtkui_widget_t base;
	ddb_gtkui_widget_extended_api_t exapi;
	GtkWidget *popover;
	char icon_name[200];
	char label[200];
	char tooltip[200];
};

static void on_popover_closed(__attribute__((unused)) GtkPopover* self,gpointer user_data){
	struct popovertoggle *data = (struct popovertoggle*)user_data;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->base.widget),FALSE);
}

static void popovertoggle_on_toggled(GtkToggleButton *widget,gpointer user_data){
	struct popovertoggle *data = (struct popovertoggle*)user_data;
	if(gtk_toggle_button_get_active(widget)){
		gtk_popover_popup(GTK_POPOVER(data->popover));
	}
}

static void popovertoggle_append(ddb_gtkui_widget_t *w,ddb_gtkui_widget_t *child){
	struct popovertoggle *data = (struct popovertoggle*)w;
	gtk_container_add(GTK_CONTAINER(data->popover),child->widget);
	gtk_widget_show(child->widget);
}

static void popovertoggle_remove(ddb_gtkui_widget_t *w,ddb_gtkui_widget_t *child){
	struct popovertoggle *data = (struct popovertoggle*)w;
	gtk_container_remove(GTK_CONTAINER(data->popover),child->widget);
}

static void popovertoggle_deserialize_from_keyvalues(ddb_gtkui_widget_t *base,const char **keyvalues){
	struct popovertoggle *data = (struct popovertoggle*)base;
	for(size_t i=0; keyvalues[i]!=NULL; i+=2){
		if(strcmp(keyvalues[i],"iconname") == 0){
			strlcpy(data->icon_name,keyvalues[i+1],sizeof(data->icon_name));
		}else if(strcmp(keyvalues[i],"label") == 0){
			strlcpy(data->label,keyvalues[i+1],sizeof(data->label));
		}else if(strcmp(keyvalues[i],"tooltip") == 0){
			strlcpy(data->tooltip,keyvalues[i+1],sizeof(data->tooltip));
		}
	}
}
#define KEYVALUES_COUNT 3
static const char **popovertoggle_serialize_to_keyvalues(ddb_gtkui_widget_t *base){
	struct popovertoggle *data = (struct popovertoggle*)base;
	char const **kv = calloc(KEYVALUES_COUNT*2+1,sizeof(char *));
	size_t i=0;

	if(data->icon_name[0]){
		kv[i++] = "iconname";
		kv[i++] = data->icon_name;
	}

	if(data->label[0]){
		kv[i++] = "label";
		kv[i++] = data->label;
	}

	if(data->tooltip[0]){
		kv[i++] = "tooltip";
		kv[i++] = data->tooltip;
	}

	assert(i <= KEYVALUES_COUNT*2+1);

	return kv;
}
static void popovertoggle_free_serialized_keyvalues(__attribute__((unused)) ddb_gtkui_widget_t *w,char const **keyvalues){
	free(keyvalues);
}

static void popovertoggle_init(ddb_gtkui_widget_t *w){
	struct popovertoggle *data = (struct popovertoggle*)w;
	if(data->icon_name[0]) gtk_button_set_image(GTK_BUTTON(data->base.widget),gtk_image_new_from_icon_name(data->icon_name,GTK_ICON_SIZE_SMALL_TOOLBAR));
	if(data->label[0])     gtk_button_set_label(GTK_BUTTON(data->base.widget),data->label);
	if(data->tooltip[0])   gtk_widget_set_tooltip_text(data->base.widget,data->tooltip);
}

ddb_gtkui_widget_t *popovertoggle_create(){
	struct popovertoggle *w = calloc(1,sizeof(struct popovertoggle));
	w->base.widget = gtk_toggle_button_new();
	w->base.init   = popovertoggle_init;
	w->base.append = popovertoggle_append;
	w->base.remove = popovertoggle_remove;
	w->exapi._size = sizeof(ddb_gtkui_widget_extended_api_t);
	w->exapi.deserialize_from_keyvalues = popovertoggle_deserialize_from_keyvalues;
	w->exapi.serialize_to_keyvalues     = popovertoggle_serialize_to_keyvalues;
	w->exapi.free_serialized_keyvalues  = popovertoggle_free_serialized_keyvalues;
	w->icon_name[0] = '\0';
	w->label[0] = '\0';
	w->tooltip[0] = '\0';

	g_signal_connect(w->base.widget,"toggled",G_CALLBACK(popovertoggle_on_toggled),w);
	gtk_widget_show(w->base.widget);
	gtkui_plugin->w_override_signals(w->base.widget,w);

	w->popover = gtk_popover_new(w->base.widget);
	g_signal_connect(GTK_POPOVER(w->popover),"closed",G_CALLBACK(on_popover_closed),w);

	ddb_gtkui_widget_t *ph = gtkui_plugin->w_create("placeholder");
	gtkui_plugin->w_append(&w->base,ph);

	return (ddb_gtkui_widget_t*)w;
}

#endif
