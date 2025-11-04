#define STRUCT_NAME outputplugincombo
#define CREATE_FN_NAME outputplugincombo_create
#include "../template/combo.h"

static const char *get(){
	deadbeef->conf_lock();
	return deadbeef->conf_get_str_fast("output_plugin",NULL);
}

static void post_get(__attribute__((unused)) const char *output_plugin){
	deadbeef->conf_unlock();
}

//See deadbeef/plugins/gtkui/prefwin/prefwinsound.c:on_pref_output_plugin_changed.
static bool set(const char *value,__attribute__((unused)) const char *label,void *data,void (*skip_change)(void *data)){
	//Search for the output plugin (in case it does not exist anymore) before setting it in the config.
	DB_output_t **out_plugs = deadbeef->plug_get_output_list();
	for(size_t i=0; out_plugs[i]; i+=1){
		//If found, set it.
		if(strcmp(out_plugs[i]->plugin.id,value) == 0){
			skip_change(data);
			deadbeef->conf_set_str("output_plugin",value);
			deadbeef->sendmessage(DB_EV_REINIT_SOUND,0,0,0);
			return true;
		}
	}
	return false;
}

static bool build(void *combo,int *selected,void (*add)(void *combo,const char *value,const char *label)){
	DB_output_t **out_plugs = deadbeef->plug_get_output_list();
	deadbeef->conf_lock();
		const char *outplugname = deadbeef->conf_get_str_fast("output_plugin",NULL);
		for(size_t i=0; out_plugs[i]; i+=1){
			add(combo,out_plugs[i]->plugin.id,out_plugs[i]->plugin.name);
			if(*selected < 0 && outplugname && strcmp(out_plugs[i]->plugin.id,outplugname) == 0){
				*selected = i;
			}
		}
	deadbeef->conf_unlock();
	return true;
}

static int on_message(struct ddb_gtkui_widget_s *w,uint32_t id,__attribute__((unused)) uintptr_t ctx,__attribute__((unused)) uint32_t p1,__attribute__((unused)) uint32_t p2){
	switch(id){
		case DB_EV_REINIT_SOUND:
			on_change(w);
			break;
	}
	return 0;
}

static void init_menu(__attribute__((unused)) GtkMenu *w){}
