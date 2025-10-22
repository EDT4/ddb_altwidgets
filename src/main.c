#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include <stdbool.h>
#include "deadbeef_util.h"
#include "main.h"

ddb_gtkui_widget_t *playbackbuttonsalt_create();
ddb_gtkui_widget_t *volumescale_create();
ddb_gtkui_widget_t *dspcombo_create();
ddb_gtkui_widget_t *menutoggle_create();
ddb_gtkui_widget_t *buttonalt_create();
ddb_gtkui_widget_t *tftester_create();
ddb_gtkui_widget_t *queueview_create();
ddb_gtkui_widget_t *outputplugincombo_create();
ddb_gtkui_widget_t *outputdevicecombo_create();

#if GTK_CHECK_VERSION(3,0,0)
ddb_gtkui_widget_t *ratingscale_create();
ddb_gtkui_widget_t *popovertoggle_create();
#endif

DB_functions_t *deadbeef;
ddb_gtkui_t *gtkui_plugin;
struct altwidgets altwidgets_data;

static int altwidgets_start(){
	altwidgets_data.db_action_map = g_hash_table_new_full(g_str_hash,g_str_equal,free,NULL);
	altwidgets_data.db_action_group = G_ACTION_GROUP(g_simple_action_group_new());
	return 0;
}

static int altwidgets_stop(){
	if(altwidgets_data.db_action_map){
		g_hash_table_remove_all(altwidgets_data.db_action_map);
		g_hash_table_unref(altwidgets_data.db_action_map);
		altwidgets_data.db_action_map = NULL;
	}
	return 0;
}

static void on_action_activate(__attribute__((unused)) GSimpleAction *act,GVariant *parameter,gpointer user_data){
	action_call((DB_plugin_action_t*)user_data,parameter && g_variant_is_of_type(parameter,G_VARIANT_TYPE_INT32)? g_variant_get_int32(parameter) : DDB_ACTION_CTX_MAIN);
}
static void on_actions_reload(){ //TODO: In which thread should this be executed?
	//Remove old.
	g_hash_table_remove_all(altwidgets_data.db_action_map);
	gchar **actions = g_action_group_list_actions(G_ACTION_GROUP(altwidgets_data.db_action_group));
	for(; *actions; actions+=1){
		g_action_map_remove_action(G_ACTION_MAP(altwidgets_data.db_action_group),*actions);
	}
	g_strfreev(actions);

	//Add new.
	for(DB_plugin_t **plugin = deadbeef->plug_get_list() ; *plugin ; plugin++){
		if(!(*plugin)->get_actions) continue;
		for(DB_plugin_action_t *db_action = (*plugin)->get_actions(NULL) ; db_action; db_action = db_action->next){
			if(db_action->callback2 && db_action->flags & (DB_ACTION_COMMON | DB_ACTION_MULTIPLE_TRACKS)){
				GSimpleAction *action = g_simple_action_new(db_action->name,(db_action->flags & DB_ACTION_MULTIPLE_TRACKS) ? G_VARIANT_TYPE_INT32 : NULL);
				g_hash_table_replace(altwidgets_data.db_action_map,strdup(db_action->name),db_action);
				g_signal_connect(action,"activate",G_CALLBACK(on_action_activate),db_action);
				g_action_map_add_action(G_ACTION_MAP(altwidgets_data.db_action_group),G_ACTION(action));
				g_object_unref(action);
			}
		}
	}
}

static int altwidgets_connect(){
	gtkui_plugin = (ddb_gtkui_t*) deadbeef->plug_get_for_id(DDB_GTKUI_PLUGIN_ID);
	if(!gtkui_plugin) return -1;

	//Prepare action map and action group
	on_actions_reload();

	gtkui_plugin->w_reg_widget("Playback Buttons (Alt)" ,0                           ,playbackbuttonsalt_create,"playbackbuttonsalt",NULL);
	gtkui_plugin->w_reg_widget("Volume Scale"           ,DDB_WF_SUPPORTS_EXTENDED_API,volumescale_create       ,"volumescale"       ,NULL);
	gtkui_plugin->w_reg_widget("DSP Combo"              ,DDB_WF_SUPPORTS_EXTENDED_API,dspcombo_create          ,"dspcombo"          ,NULL);
	gtkui_plugin->w_reg_widget("Menu Toggle Button"     ,0                           ,menutoggle_create        ,"menutoggle"        ,NULL);
	gtkui_plugin->w_reg_widget("Button (Alt)"           ,DDB_WF_SUPPORTS_EXTENDED_API,buttonalt_create         ,"buttonalt"         ,NULL);
	gtkui_plugin->w_reg_widget("Title Formatting Tester",DDB_WF_SUPPORTS_EXTENDED_API,tftester_create          ,"tftester"          ,NULL);
	gtkui_plugin->w_reg_widget("Queue View"             ,DDB_WF_SUPPORTS_EXTENDED_API,queueview_create         ,"queueview"         ,NULL);
	gtkui_plugin->w_reg_widget("Output Plugin Combo"    ,DDB_WF_SUPPORTS_EXTENDED_API,outputplugincombo_create ,"outputplugincombo" ,NULL);
	gtkui_plugin->w_reg_widget("Output Device Combo"    ,DDB_WF_SUPPORTS_EXTENDED_API,outputdevicecombo_create ,"outputdevicecombo" ,NULL);

	#if GTK_CHECK_VERSION(3,0,0)
	gtkui_plugin->w_reg_widget("Rating Scale"  ,DDB_WF_SUPPORTS_EXTENDED_API,ratingscale_create  ,"ratingscale"  ,NULL);
	gtkui_plugin->w_reg_widget("Popover Toggle",DDB_WF_SUPPORTS_EXTENDED_API,popovertoggle_create,"popovertoggle",NULL);
	#endif

	//TODO: View switcher. Two widgets: one for selecting the view, the other for the view itself.
	//The view should be a container which either hides or unloads (saves layout as json) and then shows/loads.
	//Multiple views should be able to connect to a single view controller.
	//The view selector cannot have a pointer to a widget in case it is destroyed (no ref counting for widgets?), so store the widget as a "path" from a root widget. Example: root -> child 0: hbox -> child 3: vbox -> child 2: view container.
	//The view selector and container is paired by the container having an "id". The selector searches from a root widget for a view container which has the specified "id". The "id" should be an user inputted string in both the selector and the container.
	//It would therefore be beneficial in this approach to extend the GTKUI API, making it possible to register multiple root widgets, meaning returning a NULL-terminated array in w_get_rootwidget for example.
	//An alternative would be to use the pointer approach and extend the GTKUI API to be able to listen to widget creations/removals.
	//If it is not possible to extend the GTKUI API, a plugin could provide API extensions instead. For example a plugin that provides a list of root widgets which every plugin can register into.

	return 0;
}

static int altwidgets_disconnect(){
	//free(altwidgets_data.db_action_group); //TODO: How to free? g_object_unref?
	if(gtkui_plugin){
		gtkui_plugin->w_unreg_widget("playbackbuttonsalt");
		gtkui_plugin->w_unreg_widget("volumescale");
		gtkui_plugin->w_unreg_widget("dspcombo");
		gtkui_plugin->w_unreg_widget("menutoggle");
		gtkui_plugin->w_unreg_widget("buttonalt");
		gtkui_plugin->w_unreg_widget("tftester");
		gtkui_plugin->w_unreg_widget("queueview");
		gtkui_plugin->w_unreg_widget("outputplugincombo");
		gtkui_plugin->w_unreg_widget("outputdevicecombo");

		#if GTK_CHECK_VERSION(3,0,0)
		gtkui_plugin->w_unreg_widget("ratingscale");
		gtkui_plugin->w_unreg_widget("popovertoggle");
		#endif

		gtkui_plugin = NULL;
	}
	return 0;
}

static int altwidgets_message(uint32_t id,__attribute__((unused)) uintptr_t ctx,__attribute__((unused)) uint32_t p1,__attribute__((unused)) uint32_t p2){
	switch(id){
		case DB_EV_ACTIONSCHANGED:
			on_actions_reload();
			break;
	}
	return 0;
}

static DB_misc_t plugin ={
	.plugin.api_vmajor = DB_API_VERSION_MAJOR,
	.plugin.api_vminor = DB_API_VERSION_MINOR,
	.plugin.version_major = 1,
	.plugin.version_minor = 14,
	.plugin.type = DB_PLUGIN_MISC,
	#if GTK_CHECK_VERSION(3,0,0)
	.plugin.id = "altwidgets-gtk3",
	#else
	.plugin.id = "altwidgets-gtk2",
	#endif
	.plugin.name = "Alternative Widgets",
	.plugin.descr =
		"Alternative small widgets.\n"
		"\n"
		"List of widgets provided:\n"
		"\n"
		"- Playback Buttons (Alt):\n"
		"Playback buttons and more with tooltips and state.\n"
		"Buttons will be disabled depending on the state that the button controls.\n"
		"This widget is unfortunately not configurable as of writing.\n"
		"\n"
		"- Volume Scale:\n"
		"Volume selector by a scale widget.\n"
		"Right-click to select either a linear, cubic or dB scale,\n"
		"similar to the official volume widget.\n"
		"Configuration keys: scale: str, step1: float, step2: float, width: int, height: int.\n"
		"\n"
		"- DSP Combo:\n"
		"Selecting a saved DSP preset.\n"
		"Configuration keys: maxwidth: int.\n"
		"\n"
		"- Menu Toggle Button:\n"
		"Toggles the visibility of the menu bar.\n"
		"\n"
		"- Button (Alt):\n"
		"Provides a button using an icon based on gtk_image_new_from_icon_name.\n"
		"gtk-icon-browser usually provides a way to see a list of icons together with their name,\n"
		"but a GTK theme can also provide additional ones.\n"
		"The label allows for title formatting and will update depending on deadbeef message event IDs if specified.\n"
		"See deadbeef.h:DB_EV_* for a list of them.\n"
		"The action is identified by the deadbeef action names.\n"
		"A list of the default deadbeef actions can be found in deadbeef/src/coreplugin.c:action_*\n"
		"Plugins are also able to provide additional actions.\n"
		"deadbeef.h:DB_plugin_action_t::name is what to look for.\n"
		"Configuration keys: action: str, iconname: str, label: str, eventupdate0: uint, eventupdate1: uint, eventupdate2: uint.\n"
		"\n"
		"- Title Formatting Tester:\n"
		"Displays the title formatted string based on track selection.\n"
		"Useful for testing and formatting track metadata.\n"
		"Configuration keys: input: str.\n"
		"\n"
		"- Queue View:\n"
		"Displays a list of play items that currently are in the play queue.\n"
		"Play items in the queue can be removed by either Right-click > Unqueue\n"
		"or by selecting and pressing the Delete key.\n"
		"It is currently very bare-bones and lack a lot of other useful features.\n"
		"Configuration keys: title: list[str], format: list[str], width: list[uint].\n"
		"\n"
		"- Output Plugin Combo:\n"
		"Selecting an output plugin.\n"
		"Configuration keys: maxwidth: int.\n"
		"\n"
		"- Output Device Combo:\n"
		"Selecting an output device of the output plugin.\n"
		"Configuration keys: maxwidth: int.\n"
		"\n"
		"- Rating Scale (GTK3):\n"
		"Displays a scale with stars that can be selected.\n"
		"It is currently only a proof of concept and does nothing.\n"
		"Configuration keys: granularity: uint, marks: uint, min_value: float, max_value: float, meta_name: str.\n"
		"\n"
		"- Popover Toggle (GTK3):\n"
		"A button which displays a popover containing a widget when pressed.\n"
		"Configuration keys: width: int, height: int, iconname: str, label: str, tooltip: str, padding: uint.\n"
	,
	.plugin.copyright =
		"MIT License\n"
		"\n"
		"Copyright 2025 EDT4\n"
		"\n"
		"Permission is hereby granted,free of charge,to any person obtaining a copy\n"
		"of this software and associated documentation files(the \"Software\"),to deal\n"
		"in the Software without restriction,including without limitation the rights\n"
		"to use,copy,modify,merge,publish,distribute,sublicense,and/or sell\n"
		"copies of the Software,and to permit persons to whom the Software is\n"
		"furnished to do so,subject to the following conditions:\n"
		"\n"
		"The above copyright notice and this permission notice shall be included in all\n"
		"copies or substantial portions of the Software.\n"
		"\n"
		"THE SOFTWARE IS PROVIDED \"AS IS\",WITHOUT WARRANTY OF ANY KIND,EXPRESS OR\n"
		"IMPLIED,INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
		"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
		"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,DAMAGES OR OTHER\n"
		"LIABILITY,WHETHER IN AN ACTION OF CONTRACT,TORT OR OTHERWISE,ARISING FROM,\n"
		"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
		"SOFTWARE.\n"
	,
	.plugin.website = "https://github.org/EDT4/ddb_altwidgets",
	.plugin.connect    = altwidgets_connect,
	.plugin.disconnect = altwidgets_disconnect,
	.plugin.start      = altwidgets_start,
	.plugin.stop       = altwidgets_stop,
	.plugin.message    = altwidgets_message,
};

__attribute__((visibility("default")))
DB_plugin_t *
#if GTK_CHECK_VERSION(3,0,0)
altwidgets_gtk3_load
#else
altwidgets_gtk2_load
#endif
(DB_functions_t *api){
	deadbeef = api;
	return DB_PLUGIN(&plugin);
}
