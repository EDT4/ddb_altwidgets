#ifndef STRUCT_NAME
#error "STRUCT_NAME must be defined before importing combo.h."
#endif

#ifndef CREATE_FN_NAME
#error "CREATE_FN_NAME must be defined before importing combo.h."
#endif

#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include <stdbool.h>
#include "../gtk2.h"
#include "../gtk_util.h"

///////////////////////////////////////////////////////////////////////////////
//The following functions must also be defined:
//

//Should get the current value from the model, returning the label to be shown in the combo box.
static const char *get();

//Runs after handling `get`, when its value is not needed anymore.
static void post_get(const char* label);

//Should set the current value in the model from the combo box label.
//If a message is triggered by setting the value, resulting in a call to `on_change`, then `skip_change` should be called before actually setting the value in the model to avoid that.
static bool set(const char *label,void *data,void (*skip_change)(void *data));

//Builds the combo box items.
//Call `add` for every item that should be added.
//`selected` should be set to an index if a particular item should be selected. Its default is -1, which means that no item is selected.
static bool build(void *combo,int *selected,void (*add)(void *combo,const char *label));

static int on_message(struct ddb_gtkui_widget_s *w,uint32_t id,uintptr_t ctx,uint32_t p1,uint32_t p2);

///////////////////////////////////////////////////////////////////////////////

extern DB_functions_t *deadbeef;
extern ddb_gtkui_t *gtkui_plugin;

struct STRUCT_NAME{
	ddb_gtkui_widget_t base;
    ddb_gtkui_widget_extended_api_t exapi;
	bool skip_change; //TODO: Is there a better way?
	int max_width;
};

static void skip_change_cb(void *data){
	((struct STRUCT_NAME*)data)->skip_change = true;
}

//When the combo box is changed.
static void w_on_combo_change(GtkComboBox* self,gpointer user_data){
	struct STRUCT_NAME *data = (struct STRUCT_NAME*)user_data;
	if(gtk_combo_box_get_active(self) < 0) return;
	if(!set(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(self)),data,skip_change_cb)){
	    const GSignalMatchType mask = (GSignalMatchType)(G_SIGNAL_MATCH_FUNC);
	    g_signal_handlers_block_matched(self,mask,0,0,NULL,w_on_combo_change,data);
	    gtk_combo_box_set_active(self,-1);
	    g_signal_handlers_unblock_matched(self,mask,0,0,NULL,w_on_combo_change,data);
	}
}

static void w_rebuild_combo(struct STRUCT_NAME *data){
	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(data->base.widget));

	int selected = -1; //Default is -1, meaning unselected.
	if(build(GTK_COMBO_BOX_TEXT(data->base.widget),&selected,(void(*)(void*,const char*))gtk_combo_box_text_append_text)){
		//If the combo box items were successfully built, then select the active index if it was found.
		const GSignalMatchType mask = (GSignalMatchType)(G_SIGNAL_MATCH_FUNC);
		g_signal_handlers_block_matched(G_OBJECT(data->base.widget),mask,0,0,NULL,(gpointer)w_on_combo_change,data);
		gtk_combo_box_set_active(GTK_COMBO_BOX(data->base.widget),selected);
		g_signal_handlers_unblock_matched(G_OBJECT(data->base.widget),mask,0,0,NULL,(gpointer)w_on_combo_change,data);
		gtk_widget_set_sensitive(data->base.widget,true);
	}else{
		gtk_widget_set_sensitive(data->base.widget,false);
	}
}

//When something else changes the value that the combo box keeps track on.
static int w_on_change(gpointer user_data){
	struct STRUCT_NAME *data = (struct STRUCT_NAME*)user_data;

	const char *label = get();
	if(label){
		int pos = gtk_combo_box_text_position_of(GTK_COMBO_BOX_TEXT(data->base.widget),label);
		if(pos >= 0){
			//Found position of label. Set it as active.
			const GSignalMatchType mask = (GSignalMatchType)(G_SIGNAL_MATCH_FUNC);
			g_signal_handlers_block_matched(G_OBJECT(data->base.widget),mask,0,0,NULL,(gpointer)w_on_combo_change,data);
			gtk_combo_box_set_active(GTK_COMBO_BOX(data->base.widget),pos);
			g_signal_handlers_unblock_matched(G_OBJECT(data->base.widget),mask,0,0,NULL,(gpointer)w_on_combo_change,data);
		}else{
			//Label not found in combo box. Rebuild/rescan the combo box. Maybe there is a new one.
			w_rebuild_combo(data);
		}
	}else{
		//No label at all. Rebuild/rescan the combo box.
		w_rebuild_combo(data);
	}
	post_get(label);
	return G_SOURCE_REMOVE;
}

static void on_change(struct ddb_gtkui_widget_s *w){
	struct STRUCT_NAME *data = (struct STRUCT_NAME*)w;
	if(data->skip_change){
		data->skip_change = false;
	}else{
		g_idle_add(w_on_change,data);
	}
}

static gboolean w_on_combo_click(__attribute__((unused)) GtkWidget *widget,GdkEventButton *event,struct STRUCT_NAME *data){
	if(event->button == GDK_BUTTON_SECONDARY){
		GtkWidget *menu = gtk_menu_new();
			GtkWidget *item;
			item = gtk_menu_item_new_with_label("Refresh");
				g_signal_connect_swapped(item,"activate",G_CALLBACK(w_rebuild_combo),data);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);

			gtk_widget_show_all(menu);
		gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent*)event);
		return TRUE;
	}
	return FALSE;
}

static void on_init(ddb_gtkui_widget_t *w){
	struct STRUCT_NAME *data = (struct STRUCT_NAME*)w;

	//Set max width.
	if(data->max_width > 0){
		GList *renderers = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(data->base.widget));
		for(; renderers != NULL; renderers = renderers->next){
			GtkCellRenderer *renderer = GTK_CELL_RENDERER(renderers->data);
			g_object_set(renderer,"ellipsize",PANGO_ELLIPSIZE_END,"width-chars",data->max_width,"max-width-chars",data->max_width,NULL);
		}
		g_list_free(renderers);
	}

	w_rebuild_combo(data);
}

static void on_deserialize_from_keyvalues(ddb_gtkui_widget_t *base,const char **keyvalues){
	struct STRUCT_NAME *data = (struct STRUCT_NAME*)base;
	for(size_t i=0; keyvalues[i]!=NULL; i+=2){
		if(strcmp(keyvalues[i],"maxwidth") == 0){
			data->max_width = atoi(keyvalues[i+1]);
		}
	}
}
static const char **on_serialize_to_keyvalues(ddb_gtkui_widget_t *base){
	struct STRUCT_NAME *data = (struct STRUCT_NAME*)base;
	#define INT_BUFFER_LEN 20
	#define ENTRIES 1
	char const **kv = calloc(ENTRIES * 2 + 1,sizeof(char *));
	kv[0] = "maxwidth";
	kv[1] = malloc(INT_BUFFER_LEN);
	snprintf((char*)(kv[1]),INT_BUFFER_LEN,"%d",data->max_width);
	return kv;
}
static void on_free_serialized_keyvalues(__attribute__((unused)) ddb_gtkui_widget_t *w,char const **keyvalues){
	free((char*)(keyvalues[1]));
	free(keyvalues);
}

ddb_gtkui_widget_t *CREATE_FN_NAME(){
	struct STRUCT_NAME *data = calloc(1,sizeof(struct STRUCT_NAME));
	data->base.widget = gtk_combo_box_text_new();
	data->base.init    = on_init;
	data->base.message = on_message;
	data->exapi._size = sizeof(ddb_gtkui_widget_extended_api_t);
	data->exapi.deserialize_from_keyvalues = on_deserialize_from_keyvalues;
	data->exapi.serialize_to_keyvalues     = on_serialize_to_keyvalues;
	data->exapi.free_serialized_keyvalues  = on_free_serialized_keyvalues;
	data->max_width = 20;

	g_signal_connect(data->base.widget,"changed",G_CALLBACK(w_on_combo_change),data);
    g_signal_connect(data->base.widget,"button-press-event",G_CALLBACK(w_on_combo_click),data);
	gtk_widget_show(data->base.widget);
	gtkui_plugin->w_override_signals(data->base.widget,&data->base);

	return (ddb_gtkui_widget_t*)data;
}
