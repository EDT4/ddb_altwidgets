#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include <stdbool.h>
#include "gtk2.h"
#include "gtk_util.h"

extern DB_functions_t *deadbeef;
extern ddb_gtkui_t *gtkui_plugin;

struct dspcombo{
	ddb_gtkui_widget_t base;
    ddb_gtkui_widget_extended_api_t exapi;
	bool skip_dsp_change; //TODO: Is there a better way?
	int max_width;
};

static int dsp_preset_scandir_filter(const struct dirent *ent){
	char *ext = strrchr(ent->d_name,'.');
	return ext && !strcasecmp(ext,".txt");
}

//When the combo box is changed.
static void on_dspcombo_change(GtkComboBox* self,gpointer user_data){
	struct dspcombo *data = (struct dspcombo*)user_data;
	if(gtk_combo_box_get_active(self) <= 0) return;

	const char *name = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(self));

	char path[PATH_MAX];
	if(snprintf(path,sizeof(path),"%s/presets/dsp/%s.txt",deadbeef->get_system_dir(DDB_SYS_DIR_CONFIG),name) < 0) goto Fail;

	ddb_dsp_context_t *chain = NULL;
	if(deadbeef->dsp_preset_load(path,&chain)) goto Fail;

	deadbeef->conf_set_str("dsp.preset",name);
	data->skip_dsp_change = true;
	deadbeef->streamer_set_dsp_chain(chain);
    deadbeef->dsp_preset_free(chain);
    return;

    Fail: {
	    const GSignalMatchType mask = (GSignalMatchType)(G_SIGNAL_MATCH_FUNC);
	    g_signal_handlers_block_matched(self,mask,0,0,NULL,on_dspcombo_change,data);
	    gtk_combo_box_set_active(self,-1);
	    g_signal_handlers_unblock_matched(self,mask,0,0,NULL,on_dspcombo_change,data);
	}
}

static void dspcombo_rebuild(struct dspcombo *data){
	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(data->base.widget));

	char path[PATH_MAX];
	if(snprintf(path,sizeof(path),"%s/presets/dsp",deadbeef->get_system_dir(DDB_SYS_DIR_CONFIG)) >= 0){
		struct dirent **namelist = NULL;
		int n = scandir(path,&namelist,dsp_preset_scandir_filter,alphasort);

		//Get active DSP preset name.
		deadbeef->conf_lock();
		const char *prev_dsp_name = deadbeef->conf_get_str_fast("dsp.preset","");
		int selected = -1; //Default is -1, meaning the "unknown" DSP preset.

		for(int i=0; i<n; i++){
			{ //Modify the filename given by scandir by removing the file extension.
				char *c;
				for(c=namelist[i]->d_name; *c!='\0' && c<namelist[i]->d_name+PATH_MAX; c+=1);
				if(namelist[i]->d_name+4 < c) *(c-4) = '\0';
			}

			//Find index of active DSP preset by name if not found yet.
			if(selected == -1 && strcmp(prev_dsp_name,namelist[i]->d_name) == 0){selected = i+1;}

			//Postpend name of DSP preset.
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->base.widget),namelist[i]->d_name);

			free(namelist[i]);
		}

		deadbeef->conf_unlock();

		free(namelist);

		//Select the active DSP preset if it was found. Otherwise the "unknown" DSP preset.
		const GSignalMatchType mask = (GSignalMatchType)(G_SIGNAL_MATCH_FUNC);
		g_signal_handlers_block_matched(G_OBJECT(data->base.widget),mask,0,0,NULL,(gpointer)on_dspcombo_change,data);
		gtk_combo_box_set_active(GTK_COMBO_BOX(data->base.widget),selected);
		g_signal_handlers_unblock_matched(G_OBJECT(data->base.widget),mask,0,0,NULL,(gpointer)on_dspcombo_change,data);
	}
}

//When something else changes the DSP preset.
static int on_dsp_change(gpointer user_data){
	struct dspcombo *data = (struct dspcombo*)user_data;

	deadbeef->conf_lock();
		const char *preset = deadbeef->conf_get_str_fast("dsp.preset",NULL);
		if(preset){
			int pos = gtk_combo_box_text_position_of(GTK_COMBO_BOX_TEXT(data->base.widget),preset);
			if(pos >= 0){
				//Found position of DSP preset, set it as active.
				const GSignalMatchType mask = (GSignalMatchType)(G_SIGNAL_MATCH_FUNC);
				g_signal_handlers_block_matched(G_OBJECT(data->base.widget),mask,0,0,NULL,(gpointer)on_dspcombo_change,data);
				gtk_combo_box_set_active(GTK_COMBO_BOX(data->base.widget),pos);
				g_signal_handlers_unblock_matched(G_OBJECT(data->base.widget),mask,0,0,NULL,(gpointer)on_dspcombo_change,data);
			}else{
				//DSP preset not found in combo box. Rebuild/rescan the combo box. Maybe there is a new one.
				dspcombo_rebuild(data);
			}
		}
	deadbeef->conf_unlock();
	return G_SOURCE_REMOVE;
}

static gboolean on_dspcombo_click(__attribute__((unused)) GtkWidget *widget,GdkEventButton *event,struct dspcombo *data){
	if(event->button == GDK_BUTTON_SECONDARY){
		GtkWidget *menu = gtk_menu_new();
			GtkWidget *item;
			item = gtk_menu_item_new_with_label("Refresh");
				g_signal_connect_swapped(item,"activate",G_CALLBACK(dspcombo_rebuild),data);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);

			gtk_widget_show_all(menu);
		gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent*)event);
		return TRUE;
	}
	return FALSE;
}

static void dspcombo_init(ddb_gtkui_widget_t *w){
	struct dspcombo *data = (struct dspcombo*)w;

	//Set max width.
	if(data->max_width > 0){
		GList *renderers = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(data->base.widget));
		for(; renderers != NULL; renderers = renderers->next){
			GtkCellRenderer *renderer = GTK_CELL_RENDERER(renderers->data);
			g_object_set(renderer,"ellipsize",PANGO_ELLIPSIZE_END,"width-chars",data->max_width,"max-width-chars",data->max_width,NULL);
		}
		g_list_free(renderers);
	}

	dspcombo_rebuild(data);
}

static int dspcombo_message(struct ddb_gtkui_widget_s *w,uint32_t id,__attribute__((unused)) uintptr_t ctx,__attribute__((unused)) uint32_t p1,__attribute__((unused)) uint32_t p2){
	struct dspcombo *data = (struct dspcombo*)w;
	switch(id){
		case DB_EV_DSPCHAINCHANGED:
			if(data->skip_dsp_change) data->skip_dsp_change = false; else g_idle_add(on_dsp_change,data);
			break;
	}
	return 0;
}

static void dspcombo_deserialize_from_keyvalues(ddb_gtkui_widget_t *base,const char **keyvalues){
	struct dspcombo *data = (struct dspcombo*)base;
	for(size_t i=0; keyvalues[i]!=NULL; i+=2){
		if(strcmp(keyvalues[i],"maxwidth") == 0){
			data->max_width = atoi(keyvalues[i+1]);
		}
	}
}
static const char **dspcombo_serialize_to_keyvalues(ddb_gtkui_widget_t *base){
	struct dspcombo *data = (struct dspcombo*)base;
	#define INT_BUFFER_LEN 20
	#define ENTRIES 1
	char const **kv = calloc(ENTRIES * 2 + 1,sizeof(char *));
	kv[0] = "maxwidth";
	kv[1] = malloc(INT_BUFFER_LEN);
	snprintf((char*)(kv[1]),INT_BUFFER_LEN,"%d",data->max_width);
	return kv;
}
static void dspcombo_free_serialized_keyvalues(__attribute__((unused)) ddb_gtkui_widget_t *w,char const **keyvalues){
	free((char*)(keyvalues[1]));
	free(keyvalues);
}

ddb_gtkui_widget_t *dspcombo_create(){
	struct dspcombo *data = calloc(1,sizeof(struct dspcombo));
	data->base.widget = gtk_combo_box_text_new();
	data->base.init    = dspcombo_init;
	data->base.message = dspcombo_message;
	data->exapi._size = sizeof(ddb_gtkui_widget_extended_api_t);
	data->exapi.deserialize_from_keyvalues = dspcombo_deserialize_from_keyvalues;
	data->exapi.serialize_to_keyvalues     = dspcombo_serialize_to_keyvalues;
	data->exapi.free_serialized_keyvalues  = dspcombo_free_serialized_keyvalues;
	data->max_width = 20;

	g_signal_connect(data->base.widget,"changed",G_CALLBACK(on_dspcombo_change),data);
    g_signal_connect(data->base.widget,"button-press-event",G_CALLBACK(on_dspcombo_click),data);
	gtk_widget_show(data->base.widget);
	gtkui_plugin->w_override_signals(data->base.widget,&data->base);

	return (ddb_gtkui_widget_t*)data;
}
