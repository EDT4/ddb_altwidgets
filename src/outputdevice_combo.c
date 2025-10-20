#define STRUCT_NAME outputdevicecombo
#define CREATE_FN_NAME outputdevicecombo_create
#include "template/combo.h"

static const char *get(){
	deadbeef->conf_lock();
	char key[100];
	snprintf(key,sizeof(key),"%s_soundcard",deadbeef->get_output()->plugin.id);
	return deadbeef->conf_get_str_fast(key,NULL);
}

static void post_get(__attribute__((unused)) const char *output_plugin){
	deadbeef->conf_unlock();
}

//See deadbeef/plugins/gtkui/prefwin/prefwinsound.c:on_pref_soundcard_changed.
static bool set(const char *label,void *data,void (*skip_change)(void *data)){
	//TODO: Search for the output device (in case it does not exist anymore) before setting it in the config.
	char key[100];
	snprintf(key,sizeof(key),"%s_soundcard",deadbeef->get_output()->plugin.id);
	skip_change(data);
	deadbeef->conf_set_str(key,label);
	deadbeef->sendmessage(DB_EV_CONFIGCHANGED,0,0,0);
	deadbeef->sendmessage(DB_EV_REINIT_SOUND,0,0,0);
	return true;
}

struct build_enum_soundcards_cb_data{
	void *combo;
	int *selected;
	void (*add)(void *combo,const char *label);
	const char *current;
	size_t index;
};
static void build_enum_soundcards_cb(const char *name,const char *desc,void *user_data){
	struct build_enum_soundcards_cb_data *data = (struct build_enum_soundcards_cb_data*)user_data;
	if(*data->selected < 0 && data->current && strcmp(name,data->current) == 0){
		*data->selected = data->index;
	}
	data->add(data->combo,name);
	data->index+= 1;
}
//See deadbeef/plugins/gtkui/prefwin/prefwinsound.c:prefwin_fill_soundcards.
static bool build(void *combo,int *selected,void (*add)(void *combo,const char *label)){
	if(deadbeef->get_output()->enum_soundcards){
		char key[100];
		snprintf(key,sizeof(key),"%s_soundcard",deadbeef->get_output()->plugin.id);
		deadbeef->conf_lock();
			struct build_enum_soundcards_cb_data data = {combo,selected,add,deadbeef->conf_get_str_fast(key,NULL),0};
			deadbeef->get_output()->enum_soundcards(build_enum_soundcards_cb,&data);
		deadbeef->conf_unlock();
		return true;
	}
	return false;
}

static int on_message(struct ddb_gtkui_widget_s *w,uint32_t id,__attribute__((unused)) uintptr_t ctx,__attribute__((unused)) uint32_t p1,__attribute__((unused)) uint32_t p2){
	switch(id){
		case DB_EV_REINIT_SOUND:
			on_change(w);
			break;
	}
	return 0;
}
