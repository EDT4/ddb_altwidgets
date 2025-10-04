#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include <stdbool.h>

DB_functions_t *deadbeef;
ddb_gtkui_t *gtkui_plugin;

ddb_gtkui_widget_t *actionbuttons_create();
ddb_gtkui_widget_t *volumescale_create();
ddb_gtkui_widget_t *dspcombo_create();
//ddb_gtkui_widget_t *iconbutton_create();

static int altwidgets_connect(){
	gtkui_plugin = (ddb_gtkui_t*) deadbeef->plug_get_for_id(DDB_GTKUI_PLUGIN_ID);
	if(!gtkui_plugin) return -1;

	gtkui_plugin->w_reg_widget("Action Buttons",0,actionbuttons_create,"actionbuttons",NULL);
	gtkui_plugin->w_reg_widget("Volume Scale"  ,0,volumescale_create  ,"volumescale"  ,NULL);
	gtkui_plugin->w_reg_widget("DSP Combo"     ,0,dspcombo_create     ,"dspcombo"     ,NULL);
	//gtkui_plugin->w_reg_widget("Button (Icon)" ,0,iconbutton_create   ,"iconbutton"   ,NULL);

	return 0;
}

static int altwidgets_disconnect(){
	if(gtkui_plugin){
		gtkui_plugin->w_unreg_widget("actionbuttons");
		gtkui_plugin->w_unreg_widget("volumescale");
		gtkui_plugin->w_unreg_widget("dspcombo");
		gtkui_plugin = NULL;
	}
	return 0;
}

static DB_misc_t plugin ={
	.plugin.api_vmajor = DB_API_VERSION_MAJOR,
	.plugin.api_vminor = DB_API_VERSION_MINOR,
	.plugin.version_major = 1,
	.plugin.version_minor = 0,
	.plugin.type = DB_PLUGIN_MISC,
	.plugin.id = "altwidgets-gtk3",
	.plugin.name = "Alternative Widgets",
	.plugin.descr =
		"Alternative small widgets.\n"
		"\n"
		"List of the widgets provided by this plugin:\n"
		"- Action buttons: Playback buttons and more with tooltips and state.\n"
		"- Volume Scale: Volume selector by a scale widget.\n"
		"- DSP Combo: Selecting a saved DSP preset.\n"
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
	.plugin.connect = altwidgets_connect,
	.plugin.disconnect = altwidgets_disconnect,
};

__attribute__((visibility("default")))
DB_plugin_t * altwidgets_gtk3_load(DB_functions_t *api){
	deadbeef = api;
	return DB_PLUGIN(&plugin);
}
