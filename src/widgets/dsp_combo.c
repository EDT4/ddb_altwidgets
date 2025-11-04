#define STRUCT_NAME dspcombo
#define CREATE_FN_NAME dspcombo_create
#include "../template/combo.h"

#include "../deadbeef_util.h"
#include "../main.h"
extern struct altwidgets altwidgets_data;

static const char *get(){
	deadbeef->conf_lock();
	return deadbeef->conf_get_str_fast("dsp.preset",NULL);
}

static void post_get(__attribute__((unused)) const char *output_plugin){
	deadbeef->conf_unlock();
}

static bool set(const char *value,const char *label,void *data,void (*skip_change)(void *data)){
	ddb_dsp_context_t *chain = NULL;
	if(deadbeef->dsp_preset_load(value,&chain)) return false;

	deadbeef->conf_set_str("dsp.preset",label);
	skip_change(data);
	deadbeef->streamer_set_dsp_chain(chain);
    deadbeef->dsp_preset_free(chain);
    return true;
}

static int dsp_preset_scandir_filter(const struct dirent *ent){
	char *ext = strrchr(ent->d_name,'.');
	return ext && !strcasecmp(ext,".txt");
}

static bool build(void *combo,int *selected,void (*add)(void *combo,const char *value,const char *label)){
	char path[PATH_MAX];
	int len = snprintf(path,sizeof(path),"%s/presets/dsp",deadbeef->get_system_dir(DDB_SYS_DIR_CONFIG));
	if(len >= 0){
		struct dirent **namelist = NULL;
		int n = scandir(path,&namelist,dsp_preset_scandir_filter,alphasort);
		path[len++] = '/';

		//Get active DSP preset name.
		deadbeef->conf_lock();
		const char *prev_dsp_name = deadbeef->conf_get_str_fast("dsp.preset","");

		for(int i=0; i<n; i++){
			//Build path.
			strlcpy(&path[len],namelist[i]->d_name,sizeof(path)-len);

			{ //Modify the filename given by scandir by removing the file extension.
				char *c;
				for(c=namelist[i]->d_name; *c!='\0' && c<namelist[i]->d_name+PATH_MAX; c+=1);
				if(namelist[i]->d_name+4 < c) *(c-4) = '\0';
			}

			//Find index of active DSP preset by name if not found yet.
			if(*selected < 0 && strcmp(prev_dsp_name,namelist[i]->d_name) == 0){*selected = i;}

			//Postpend name of DSP preset.
			add(combo,path,namelist[i]->d_name);

			free(namelist[i]);
		}

		deadbeef->conf_unlock();
		free(namelist);
		return true;
	}
	return false;
}

static int on_message(struct ddb_gtkui_widget_s *w,uint32_t id,__attribute__((unused)) uintptr_t ctx,__attribute__((unused)) uint32_t p1,__attribute__((unused)) uint32_t p2){
	switch(id){
		case DB_EV_DSPCHAINCHANGED:
			on_change(w);
			break;
	}
	return 0;
}

static void on_dsp_preferences_activate(__attribute__((unused)) void *w,DB_plugin_action_t *action){
	action_call(action,DDB_ACTION_CTX_MAIN);
}
static void init_menu(GtkMenu *menu){
	DB_plugin_action_t *action = g_hash_table_lookup(altwidgets_data.db_action_map,"dsp_preferences");
	if(action){
		GtkWidget *item = gtk_menu_item_new_with_label("DSP Preferences");
			g_signal_connect(item,"activate",G_CALLBACK(on_dsp_preferences_activate),action);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);
	}
}
