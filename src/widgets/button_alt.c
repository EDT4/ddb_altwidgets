#include <assert.h>
#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include <stdbool.h>
#include "../deadbeef_util.h"
#include "../gtk2.h"
#include "../main.h"
#include "../util.h"

extern DB_functions_t *deadbeef;
extern ddb_gtkui_t *gtkui_plugin;
extern struct altwidgets altwidgets_data;

struct buttonalt{
	ddb_gtkui_widget_t base;
	ddb_gtkui_widget_extended_api_t exapi;
	DB_plugin_action_t *action;
	char icon_name[200];
	char label[200];
	char *label_tf;
	uint32_t event_updates[10];
	guint callback_id;
};

static size_t buttonalt_eventupdates_length(struct buttonalt *data){
	size_t len = 0; for(uint32_t *end=&data->event_updates[0]; end[len]; len+=1);
	return len;
}

//Requirement: data->label_tf != NULL
static void buttonalt_update_label_by_tf(struct buttonalt *data){
	char buffer[200];
	ddb_tf_context_t ctx = {
		._size = sizeof(ddb_tf_context_t),
		.flags = DDB_TF_CONTEXT_NO_DYNAMIC,
		.iter = PL_MAIN,
	};
	gtk_button_set_label(GTK_BUTTON(data->base.widget),deadbeef->tf_eval(&ctx,data->label_tf,buffer,sizeof(buffer)) > 0? buffer : data->label);
}

static void on_set_label(struct buttonalt *data){
	if(data->label[0] =='\0'){
		gtk_button_set_label(GTK_BUTTON(data->base.widget),NULL);
	}else if(strchr(data->label,'$') || strchr(data->label,'%')){
		deadbeef->tf_free(data->label_tf);
		data->label_tf = deadbeef->tf_compile(data->label);
		buttonalt_update_label_by_tf(data);
	}else{
		gtk_button_set_label(GTK_BUTTON(data->base.widget),data->label);
	}
}

static void on_button_clicked(__attribute__((unused)) GtkToggleButton *button,gpointer user_data){
	struct buttonalt *data = (struct buttonalt*) user_data;
	if(data->action) action_call(data->action,DDB_ACTION_CTX_MAIN);
}

static void buttonalt_deserialize_from_keyvalues(ddb_gtkui_widget_t *base,const char **keyvalues){
	struct buttonalt *data = (struct buttonalt*)base;
	for(size_t i=0; keyvalues[i]!=NULL; i+=2){
		if(strcmp(keyvalues[i],"action") == 0){
			data->action = g_hash_table_lookup(altwidgets_data.db_action_map,keyvalues[i+1]);
			if(data->action) gtk_widget_set_tooltip_text(data->base.widget,data->action->title);
		}else if(strcmp(keyvalues[i],"iconname") == 0){
			strlcpy(data->icon_name,keyvalues[i+1],sizeof(data->icon_name));
			if(data->icon_name[0]) gtk_button_set_image(GTK_BUTTON(data->base.widget),gtk_image_new_from_icon_name(data->icon_name,GTK_ICON_SIZE_SMALL_TOOLBAR));
		}else if(strcmp(keyvalues[i],"label") == 0){
			strlcpy(data->label,keyvalues[i+1],sizeof(data->label));
			on_set_label(data);
		}else if(strcmp(keyvalues[i],"eventupdates") == 0){
			const char *s = keyvalues[i+1];
			parse_u32s(&s,data->event_updates,sizeof(data->event_updates)/sizeof(data->event_updates[0]),';');
		}
	}
}
#define KEYVALUES_COUNT 4
static const char **buttonalt_serialize_to_keyvalues(ddb_gtkui_widget_t *base){
	struct buttonalt *data = (struct buttonalt*)base;
	char const **kv = calloc(KEYVALUES_COUNT*2+1,sizeof(char *));

	size_t i=0;
	if(data->action && data->action->name && data->action->name[0]){
		kv[i++] = "action";
		kv[i++] = data->action->name;
	}

	if(data->icon_name[0]){
		kv[i++] = "iconname";
		kv[i++] = data->icon_name;
	}

	if(data->label[0]){
		kv[i++] = "label";
		kv[i++] = data->label;
	}

	{
		const size_t len = buttonalt_eventupdates_length(data);
		if(len > 0){
			kv[i++] = "eventupdates";
			size_t size = (20+1)*len; //20 is the maximum length of a uint32_t in base 10. Add 1 for the delimiter and NULL-terminator.
			char *buf = malloc(size);
			buf[write_u32s(data->event_updates,len,';',buf,size-1)] = '\0';
			kv[i++] = buf;
		}
	}

	assert(i <= KEYVALUES_COUNT*2+1);

	return kv;
}
static void buttonalt_free_serialized_keyvalues(__attribute__((unused)) ddb_gtkui_widget_t *w,char const **keyvalues){
	for(const char **s = keyvalues; s[0]; s+=2){
		if(strcmp("eventupdates",s[0]) == 0){
			g_free((char*)(s[1]));
			break;
		}
	}
	free(keyvalues);
}

static void buttonalt_option_action_on_changed(GtkEditable* self,gpointer user_data){
	struct buttonalt *data = (struct buttonalt*)user_data;
	data->action = g_hash_table_lookup(altwidgets_data.db_action_map,gtk_entry_get_text(GTK_ENTRY(self)));
	if(data->action) gtk_widget_set_tooltip_text(data->base.widget,data->action->title);
}
static void buttonalt_option_iconname_on_changed(GtkEditable* self,gpointer user_data){
	struct buttonalt *data = (struct buttonalt*)user_data;
	const char *s = gtk_entry_get_text(GTK_ENTRY(self));
	strlcpy(data->icon_name,s,sizeof(data->icon_name));
	gtk_button_set_image(GTK_BUTTON(data->base.widget),s && s[0]!='\0'? gtk_image_new_from_icon_name(s,GTK_ICON_SIZE_SMALL_TOOLBAR): NULL);
	//TODO: gtk_image_new_from_*
}
static void buttonalt_option_label_on_changed(GtkEditable* self,gpointer user_data){
	struct buttonalt *data = (struct buttonalt*)user_data;
	const char *s = gtk_entry_get_text(GTK_ENTRY(self));
	if(s){
		strlcpy(data->label,s,sizeof(data->label));
	}else{
		data->label[0] = '\0';
	}
	on_set_label(data);
}
static void buttonalt_on_configure_activate(__attribute__((unused)) GtkMenuItem* self,struct buttonalt *data){
	GtkWidget *dialog = gtk_dialog_new_with_buttons(
		"Icon Button Configuration",
		GTK_WINDOW(gtkui_plugin->get_mainwin()),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		NULL,
		NULL
	);
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *control;
	GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
		vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,10);
			gtk_container_set_border_width(GTK_CONTAINER(vbox),10);

			hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
				gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Action:"),false,false,0);
				control = gtk_entry_new();
					if(data->action && data->action->name) gtk_entry_set_text(GTK_ENTRY(control),data->action->name);
					g_signal_connect(control,"changed",G_CALLBACK(buttonalt_option_action_on_changed),data);
				gtk_box_pack_start(GTK_BOX(hbox),control,true,true,0);
			gtk_box_pack_start(GTK_BOX(vbox),hbox,false,false,0);

			hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
				gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Icon Name:"),false,false,0);
				control = gtk_entry_new();
					gtk_entry_set_text(GTK_ENTRY(control),data->icon_name);
					g_signal_connect(control,"changed",G_CALLBACK(buttonalt_option_iconname_on_changed),data);
				gtk_box_pack_start(GTK_BOX(hbox),control,true,true,0);
			gtk_box_pack_start(GTK_BOX(vbox),hbox,false,false,0);

			hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
				gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Label:"),false,false,0);
				control = gtk_entry_new();
					gtk_entry_set_text(GTK_ENTRY(control),data->label);
					g_signal_connect(control,"changed",G_CALLBACK(buttonalt_option_label_on_changed),data);
				gtk_box_pack_start(GTK_BOX(hbox),control,true,true,0);
			gtk_box_pack_start(GTK_BOX(vbox),hbox,false,false,0);

			hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
				gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Label Update Events:"),false,false,0);
				control = gtk_entry_new();
					{
						const size_t len = buttonalt_eventupdates_length(data);
						if(len > 0){
							char buf[(20+1)*len]; //20 is the maximum length of a uint32_t in base 10. Add 1 for the delimiter and NULL-terminator.
							buf[write_u32s(data->event_updates,len,';',buf,sizeof(buf)-1)] = '\0';
							gtk_entry_set_text(GTK_ENTRY(control),buf);
						}
					}
					gtk_widget_set_sensitive(control,false); //TODO
					//g_signal_connect(control,"changed",G_CALLBACK(buttonalt_option_eventupdates_on_changed),data);
				gtk_box_pack_start(GTK_BOX(hbox),control,true,true,0);
			gtk_box_pack_start(GTK_BOX(vbox),hbox,false,false,0);

		gtk_container_add(GTK_CONTAINER(content_area),vbox);
	gtk_widget_show_all(dialog);
}

void buttonalt_initmenu(struct ddb_gtkui_widget_s *w,GtkWidget *menu){
	GtkWidget *item = gtk_menu_item_new_with_label("Configure");
	g_signal_connect(G_OBJECT(item),"activate",G_CALLBACK(buttonalt_on_configure_activate),w);
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu),item);
}

static void buttonalt_on_callback_end(struct buttonalt *data){
	data->callback_id = 0;
}

static int buttonalt_message(struct ddb_gtkui_widget_s *w,uint32_t id,__attribute__((unused)) uintptr_t ctx,__attribute__((unused)) uint32_t p1,__attribute__((unused)) uint32_t p2){
	struct buttonalt *data = (struct buttonalt*)w;
	if(data->label_tf){
		for(size_t i=0; i<sizeof(data->event_updates)/sizeof(data->event_updates[0]) && data->event_updates[i] != 0; i+=1){
			if(data->event_updates[i] == id){
				data->callback_id = g_idle_add_full(G_PRIORITY_LOW,G_SOURCE_FUNC(buttonalt_update_label_by_tf),data,(GDestroyNotify)buttonalt_on_callback_end);
				break;
			}
		}
	}
	return 0;
}

ddb_gtkui_widget_t *buttonalt_create(){
	struct buttonalt *w = calloc(1,sizeof(struct buttonalt));
	w->base.widget = gtk_button_new();
	w->base.initmenu = buttonalt_initmenu;
	w->base.message = buttonalt_message;
	w->exapi._size = sizeof(ddb_gtkui_widget_extended_api_t);
	w->exapi.deserialize_from_keyvalues = buttonalt_deserialize_from_keyvalues;
	w->exapi.serialize_to_keyvalues     = buttonalt_serialize_to_keyvalues;
	w->exapi.free_serialized_keyvalues  = buttonalt_free_serialized_keyvalues;
	w->action = NULL;
	w->icon_name[0] = '\0';
	w->label[0] = '\0';
	w->label_tf = NULL;
	w->callback_id = 0;
	w->event_updates[0] = 0;
	w->event_updates[1] = 0;
	w->event_updates[2] = 0;
	g_signal_connect(w->base.widget,"clicked",G_CALLBACK(on_button_clicked),w);
	gtk_widget_show(w->base.widget);
	gtkui_plugin->w_override_signals(w->base.widget,w);
	return(ddb_gtkui_widget_t*)w;
}
