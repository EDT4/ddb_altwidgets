#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include <stdbool.h>

extern DB_functions_t *deadbeef;
extern ddb_gtkui_t *gtkui_plugin;

struct dspcombo{
	ddb_gtkui_widget_t base;
};

static int dsp_preset_scandir_filter(const struct dirent *ent){
	char *ext = strrchr(ent->d_name,'.');
	return ext && !strcasecmp(ext,".txt");
}

static void on_dspcombo_change(GtkComboBox* self,__attribute__((unused)) gpointer user_data){
	if(gtk_combo_box_get_active(self) == 0) return;

	const char *name = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(self));

	char path[PATH_MAX];
	if(snprintf(path,sizeof(path),"%s/presets/dsp/%s.txt",deadbeef->get_system_dir(DDB_SYS_DIR_CONFIG),name) < 0) return;

	ddb_dsp_context_t *chain = NULL;
	if(deadbeef->dsp_preset_load(path,&chain)) return;

	deadbeef->streamer_set_dsp_chain(chain);
	deadbeef->conf_set_str("gtkui.conf_dsp_preset",name);
}

static void dspcombo_init(ddb_gtkui_widget_t *w){
	struct dspcombo *data = (struct dspcombo*)w;

	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(data->base.widget));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->base.widget),"");

	char path[PATH_MAX];
	if(snprintf(path,sizeof(path),"%s/presets/dsp",deadbeef->get_system_dir(DDB_SYS_DIR_CONFIG)) >= 0){
		struct dirent **namelist = NULL;
		int n = scandir(path,&namelist,dsp_preset_scandir_filter,alphasort);

		//Get active DSP preset name.
		deadbeef->conf_lock();
		const char *prev_dsp_name = deadbeef->conf_get_str_fast("gtkui.conf_dsp_preset","");
		int selected = 0; //Default is 0, meaning the "unknown" DSP preset.

		for(int i=0; i<n; i++){
			{ //Modify the filename given by scandir by removing the file extension.
				char *c;
				for(c=namelist[i]->d_name; *c!='\0' && c<namelist[i]->d_name+PATH_MAX; c+=1);
				if(namelist[i]->d_name+4 < c) *(c-4) = '\0';
			}

			//Find index of active DSP preset by name if not found yet.
			if(selected == 0 && strcmp(prev_dsp_name,namelist[i]->d_name) == 0){selected = i+1;}

			//Postpend name of DSP preset.
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->base.widget),namelist[i]->d_name);

			free(namelist[i]);
		}

		deadbeef->conf_unlock();

		free(namelist);

		//Select the active DSP preset if it was found. Otherwise the "unknown" DSP preset.
		gtk_combo_box_set_active(GTK_COMBO_BOX(data->base.widget),selected);
	}
}

/*
static int dspcombo_message(struct ddb_gtkui_widget_s *w,uint32_t id,__attribute__((unused)) uintptr_t ctx,__attribute__((unused)) uint32_t p1,__attribute__((unused)) uint32_t p2){
	struct dspcombo *data = (struct dspcombo*)w;
	switch(id){
		case DB_EV_DSPCHAINCHANGED:
			g_idle_add(dspcombo_change,NULL);
			break;
	}
	return 0;
}
*/

ddb_gtkui_widget_t *dspcombo_create(){
	struct dspcombo *data = calloc(1,sizeof(struct dspcombo));
	data->base.widget = gtk_combo_box_text_new();
	data->base.init = dspcombo_init;

	g_signal_connect(data->base.widget,"changed",G_CALLBACK(on_dspcombo_change),NULL);
	gtk_widget_set_size_request(data->base.widget,64,-1);
	gtk_widget_show(data->base.widget);
	gtkui_plugin->w_override_signals(data->base.widget,&data->base);

	/*{ TODO: How?
		GtkLabel *dsp_combo_label = GTK_LABEL(gtk_bin_get_child(GTK_BIN(data->base.widget)));
		gtk_label_set_max_width_chars(dsp_combo_label,16);
		gtk_label_set_ellipsize(dsp_combo_label,PANGO_ELLIPSIZE_END);
	}*/
	/*{
		GList *renderers = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(data->base.widget));
		while(renderers != NULL){
			GtkCellRenderer *renderer = GTK_CELL_RENDERER(renderers->data);
			g_object_set(renderer,"ellipsize", PANGO_ELLIPSIZE_END,"width-chars", 10,NULL);
			renderers = renderers->next;
		}
		g_list_free(renderers);
	}*/

	return (ddb_gtkui_widget_t*)data;
}
