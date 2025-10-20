#include <gtk/gtk.h>

#if GTK_CHECK_VERSION(3,0,0)

#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include <stdbool.h>
#include <math.h>

extern DB_functions_t *deadbeef;
extern ddb_gtkui_t *gtkui_plugin;

struct ratingscale{
	ddb_gtkui_widget_t base;
};

ddb_gtkui_widget_t *ratingscale_create(){
	struct ratingscale *w = calloc(1,sizeof(struct ratingscale));
	w->base.widget = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,0.0,5.0,0.01);
	gtk_widget_show(w->base.widget);
	gtkui_plugin->w_override_signals(w->base.widget,w);

	gtk_range_set_increments(GTK_RANGE(w->base.widget),0.01,0.05);
	gtk_scale_set_draw_value(GTK_SCALE(w->base.widget),TRUE);
	gtk_scale_set_value_pos(GTK_SCALE(w->base.widget),GTK_POS_BOTTOM);
	for(int i=0; i<=10; i+=1){
		gtk_scale_add_mark(GTK_SCALE(w->base.widget),((double)i)/2.0,GTK_POS_BOTTOM,NULL);
	}

	GtkCssProvider *provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data(provider,
		"scale{padding:16px;}"
		"scale trough{margin:0;border:0;background:-gtk-icontheme('non-starred-symbolic') repeat-x; min-width:calc(32px*5); min-height:32px;}"
		"scale trough highlight{margin:0;border:0;background:-gtk-icontheme('starred-symbolic') repeat-x; min-width:32px; min-height:32px;}"
		"scale trough highlight{min-width:0; background-size: 32px;}"
		"scale slider{background:transparent;min-width:0;min-height:0;margin:0;}"
		,-1,NULL
	);
	gtk_style_context_add_provider(gtk_widget_get_style_context(w->base.widget),GTK_STYLE_PROVIDER(provider),GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	g_object_unref(provider);

	return (ddb_gtkui_widget_t*)w;
}

#endif
