#define STRUCT_NAME dspcombo
#define CREATE_FN_NAME dspcombo_create
#include "../template/combo.h"

static const char *get(){
	deadbeef->conf_lock();
	return deadbeef->conf_get_str_fast("dsp.preset",NULL);
}

static void post_get(__attribute__((unused)) const char *output_plugin){
	deadbeef->conf_unlock();
}

static bool set(const char *label,void *data,void (*skip_change)(void *data)){
	char path[PATH_MAX];
	if(snprintf(path,sizeof(path),"%s/presets/dsp/%s.txt",deadbeef->get_system_dir(DDB_SYS_DIR_CONFIG),label) < 0) return false;

	ddb_dsp_context_t *chain = NULL;
	if(deadbeef->dsp_preset_load(path,&chain)) return false;

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

static bool build(void *combo,int *selected,void (*add)(void *combo,const char *label)){
	char path[PATH_MAX];
	if(snprintf(path,sizeof(path),"%s/presets/dsp",deadbeef->get_system_dir(DDB_SYS_DIR_CONFIG)) >= 0){
		struct dirent **namelist = NULL;
		int n = scandir(path,&namelist,dsp_preset_scandir_filter,alphasort);

		//Get active DSP preset name.
		deadbeef->conf_lock();
		const char *prev_dsp_name = deadbeef->conf_get_str_fast("dsp.preset","");

		for(int i=0; i<n; i++){
			{ //Modify the filename given by scandir by removing the file extension.
				char *c;
				for(c=namelist[i]->d_name; *c!='\0' && c<namelist[i]->d_name+PATH_MAX; c+=1);
				if(namelist[i]->d_name+4 < c) *(c-4) = '\0';
			}

			//Find index of active DSP preset by name if not found yet.
			if(*selected < 0 && strcmp(prev_dsp_name,namelist[i]->d_name) == 0){*selected = i;}

			//Postpend name of DSP preset.
			add(combo,namelist[i]->d_name);

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
