#include <gtk/gtk.h>

#include <assert.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include <stdbool.h>
#include <math.h>
#include "../deadbeef_util.h"
#include "../main.h"

//TODO: Reuse button_alt.c here

extern DB_functions_t *deadbeef;
extern ddb_gtkui_t *gtkui_plugin;
extern struct altwidgets altwidgets_data;

struct togglestatedata{
	char icon_name[256];
	char label[256];
	char tooltip[256];
	char config_value[64];
	GtkImage *icon_image;
};

struct configkeytoggle{
	ddb_gtkui_widget_t base;
	ddb_gtkui_widget_extended_api_t exapi;
	DB_plugin_action_t *action;
	char config_key[256];
	struct togglestatedata state[2];
	int on_config_load_callback_id;
	char css_classes[256];
};

static void configkeytoggle_init(ddb_gtkui_widget_t *w){
	struct configkeytoggle *data = (struct configkeytoggle*)w;
	struct togglestatedata *state = &data->state[gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->base.widget))];

	if(state->icon_image) gtk_button_set_image(GTK_BUTTON(data->base.widget),GTK_WIDGET(state->icon_image));

	if(state->label[0])   gtk_button_set_label(GTK_BUTTON(data->base.widget),state->label);

	if(state->tooltip[0]) gtk_widget_set_tooltip_text(data->base.widget,state->tooltip);
	else if(data->action) gtk_widget_set_tooltip_text(data->base.widget,data->action->title);
}

static void on_toggled(GtkToggleButton *widget,struct configkeytoggle *data){
	if(data->action){
		action_call(data->action,DDB_ACTION_CTX_MAIN);
		return;
	}

	struct togglestatedata *state = &data->state[gtk_toggle_button_get_active(widget)];
	if(data->config_key[0] && state->config_value[0]){
		deadbeef->conf_set_str(data->config_key,state->config_value);
		deadbeef->sendmessage(DB_EV_CONFIGCHANGED,0,0,0);
	}

	configkeytoggle_init(&data->base);
}

static void configkeytoggle_deserialize_from_keyvalues(ddb_gtkui_widget_t *base,const char **keyvalues){
	struct configkeytoggle *data = (struct configkeytoggle*)base;
	for(size_t i=0; keyvalues[i]!=NULL; i+=2){
		if(strcmp(keyvalues[i],"action") == 0){
			data->action = g_hash_table_lookup(altwidgets_data.db_action_map,keyvalues[i+1]);
		}else if(strcmp(keyvalues[i],"iconname0") == 0){
			strlcpy(data->state[0].icon_name,keyvalues[i+1],sizeof(data->state[0].icon_name));
			if(data->state[0].icon_name[0]){
				if(data->state[0].icon_image) g_object_unref(G_OBJECT(data->state[0].icon_image));
				data->state[0].icon_image = GTK_IMAGE(gtk_image_new_from_icon_name(data->state[0].icon_name,GTK_ICON_SIZE_SMALL_TOOLBAR));
				g_object_ref(G_OBJECT(data->state[0].icon_image)); //Increase ref count due to the handling of set_image.
			}
		}else if(strcmp(keyvalues[i],"iconname1") == 0){
			strlcpy(data->state[1].icon_name,keyvalues[i+1],sizeof(data->state[1].icon_name));
			if(data->state[1].icon_name[0]){
				if(data->state[1].icon_image) g_object_unref(G_OBJECT(data->state[1].icon_image));
				data->state[1].icon_image = GTK_IMAGE(gtk_image_new_from_icon_name(data->state[1].icon_name,GTK_ICON_SIZE_SMALL_TOOLBAR));
				g_object_ref(G_OBJECT(data->state[1].icon_image)); //Increase ref count due to the handling of set_image.
			}
		}else if(strcmp(keyvalues[i],"label0") == 0){
			strlcpy(data->state[0].label,keyvalues[i+1],sizeof(data->state[0].label));
		}else if(strcmp(keyvalues[i],"label1") == 0){
			strlcpy(data->state[1].label,keyvalues[i+1],sizeof(data->state[1].label));
		}else if(strcmp(keyvalues[i],"tooltip0") == 0){
			strlcpy(data->state[0].tooltip,keyvalues[i+1],sizeof(data->state[0].tooltip));
		}else if(strcmp(keyvalues[i],"tooltip1") == 0){
			strlcpy(data->state[1].tooltip,keyvalues[i+1],sizeof(data->state[1].tooltip));
		}else if(strcmp(keyvalues[i],"configvalue0") == 0){
			strlcpy(data->state[0].config_value,keyvalues[i+1],sizeof(data->state[0].config_value));
		}else if(strcmp(keyvalues[i],"configvalue1") == 0){
			strlcpy(data->state[1].config_value,keyvalues[i+1],sizeof(data->state[1].config_value));
		}else if(strcmp(keyvalues[i],"configkey") == 0){
			strlcpy(data->config_key,keyvalues[i+1],sizeof(data->config_key));
		}else if(strcmp(keyvalues[i],"cssclasses") == 0){
			strlcpy(data->css_classes,keyvalues[i+1],sizeof(data->css_classes));

			#if GTK_CHECK_VERSION(3,0,0)
			if(data->css_classes[0]){
				gtk_style_context_add_class(gtk_widget_get_style_context(data->base.widget),data->css_classes);
			}
			#endif
		}
	}
}
#define KEYVALUES_COUNT 11
static const char **configkeytoggle_serialize_to_keyvalues(ddb_gtkui_widget_t *base){
	struct configkeytoggle *data = (struct configkeytoggle*)base;
	char const **kv = calloc(KEYVALUES_COUNT*2+1,sizeof(char *));
	size_t i=0;

	if(data->action && data->action->name && data->action->name[0]){
		kv[i++] = "action";
		kv[i++] = data->action->name;
	}

	if(data->state[0].icon_name[0]){
		kv[i++] = "iconname0";
		kv[i++] = data->state[0].icon_name;
	}

	if(data->state[1].icon_name[0]){
		kv[i++] = "iconname1";
		kv[i++] = data->state[1].icon_name;
	}

	if(data->state[0].label[0]){
		kv[i++] = "label0";
		kv[i++] = data->state[0].label;
	}

	if(data->state[1].label[0]){
		kv[i++] = "label1";
		kv[i++] = data->state[1].label;
	}

	if(data->state[0].tooltip[0]){
		kv[i++] = "tooltip0";
		kv[i++] = data->state[0].tooltip;
	}

	if(data->state[1].tooltip[0]){
		kv[i++] = "tooltip1";
		kv[i++] = data->state[1].tooltip;
	}

	if(data->state[0].config_value[0]){
		kv[i++] = "configvalue0";
		kv[i++] = data->state[0].config_value;
	}

	if(data->state[1].config_value[0]){
		kv[i++] = "configvalue1";
		kv[i++] = data->state[1].config_value;
	}

	if(data->config_key[0]){
		kv[i++] = "configkey";
		kv[i++] = data->config_key;
	}

	if(data->config_key[0]){
		kv[i++] = "cssclasses";
		kv[i++] = data->css_classes;
	}

	assert(i <= KEYVALUES_COUNT*2+1);

	return kv;
}
static void configkeytoggle_free_serialized_keyvalues(__attribute__((unused)) ddb_gtkui_widget_t *w,char const **keyvalues){
	free(keyvalues);
}

static gboolean on_config_load(struct configkeytoggle *data){
	if(data->config_key[0]){
		deadbeef->conf_lock();
		const char *value = deadbeef->conf_get_str_fast(data->config_key,NULL);
		if(value){
			if(data->state[0].config_value[0] && strcmp(value,data->state[0].config_value) == 0){
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->base.widget))){
					const GSignalMatchType mask = (GSignalMatchType)(G_SIGNAL_MATCH_FUNC);
					g_signal_handlers_block_matched(G_OBJECT(data->base.widget),mask,0,0,NULL,on_toggled,data);
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->base.widget),false);
					g_signal_handlers_unblock_matched(G_OBJECT(data->base.widget),mask,0,0,NULL,on_toggled,data);
				}
			}else if(data->state[1].config_value[0] && strcmp(value,data->state[1].config_value) == 0){
				if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->base.widget))){
					const GSignalMatchType mask = (GSignalMatchType)(G_SIGNAL_MATCH_FUNC);
					g_signal_handlers_block_matched(G_OBJECT(data->base.widget),mask,0,0,NULL,on_toggled,data);
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->base.widget),true);
					g_signal_handlers_unblock_matched(G_OBJECT(data->base.widget),mask,0,0,NULL,on_toggled,data);
				}
			}
		}
		deadbeef->conf_unlock();
	}
	return G_SOURCE_REMOVE;
}

static void on_config_load_callback_end(struct configkeytoggle *data){
	data->on_config_load_callback_id = 0;
}
static void call_config_load(struct configkeytoggle *data){
	if(data->on_config_load_callback_id == 0){
		data->on_config_load_callback_id = g_idle_add_full(G_PRIORITY_LOW,G_SOURCE_FUNC(on_config_load),data,(GDestroyNotify)on_config_load_callback_end);
	}
}

static int configkeytoggle_message(struct ddb_gtkui_widget_s *w,uint32_t id,__attribute__((unused)) uintptr_t ctx,__attribute__((unused)) uint32_t p1,__attribute__((unused)) uint32_t p2){
	struct configkeytoggle *data = (struct configkeytoggle*)w;
	switch(id){
		case DB_EV_CONFIGCHANGED:
			call_config_load(data);
			break;
	}
	return 0;
}

ddb_gtkui_widget_t *configkeytoggle_create(){
	struct configkeytoggle *w = calloc(1,sizeof(struct configkeytoggle));
	w->base.widget  = gtk_toggle_button_new();
	w->base.init    = configkeytoggle_init;
	w->base.message = configkeytoggle_message;
	w->exapi._size = sizeof(ddb_gtkui_widget_extended_api_t);
	w->exapi.deserialize_from_keyvalues = configkeytoggle_deserialize_from_keyvalues;
	w->exapi.serialize_to_keyvalues     = configkeytoggle_serialize_to_keyvalues;
	w->exapi.free_serialized_keyvalues  = configkeytoggle_free_serialized_keyvalues;

	g_signal_connect(w->base.widget,"toggled",G_CALLBACK(on_toggled),w);
	gtk_widget_show(w->base.widget);
	gtkui_plugin->w_override_signals(w->base.widget,w);
	call_config_load(w);

	return (ddb_gtkui_widget_t*)w;
}
