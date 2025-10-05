#include <gtk/gtk.h>

#if GTK_CHECK_VERSION(3,0,0)

#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include <stdbool.h>
#include <math.h>

extern DB_functions_t *deadbeef;
extern ddb_gtkui_t *gtkui_plugin;

struct ratingtoggle{
	ddb_gtkui_widget_t base;
	GtkWidget *popover;
	GtkWidget *scale;
};

static void on_rating_popover_closed(__attribute__((unused)) GtkPopover* self,gpointer user_data){
	struct ratingtoggle *data = (struct ratingtoggle*)user_data;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->base.widget),FALSE);
	//TODO
	//printf("Closing: %f\n",gtk_range_get_value(GTK_RANGE(data->scale)));
}

static void ratingtoggle_on_toggled(GtkToggleButton *widget,gpointer user_data){
	struct ratingtoggle *data = (struct ratingtoggle*)user_data;
	if(gtk_toggle_button_get_active(widget)){
		//TODO
		//gtk_range_set_value(GTK_RANGE(data->scale),current_rating);
		gtk_popover_popup(GTK_POPOVER(data->popover));
	}
}

ddb_gtkui_widget_t *ratingtoggle_create(){
	struct ratingtoggle *w = calloc(1,sizeof(struct ratingtoggle));
	w->base.widget = gtk_toggle_button_new();
	w->popover = NULL;

	g_signal_connect(w->base.widget,"toggled",G_CALLBACK(ratingtoggle_on_toggled),w);
    gtk_button_set_image(GTK_BUTTON(w->base.widget),gtk_image_new_from_icon_name("semi-starred-symbolic",GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show(w->base.widget);
	gtkui_plugin->w_override_signals(w->base.widget,w);

	w->popover = gtk_popover_new(w->base.widget);
		w->scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,0.0,5.0,0.01);
			gtk_range_set_increments(GTK_RANGE(w->scale),0.01,0.05);
			gtk_scale_set_draw_value(GTK_SCALE(w->scale),TRUE);
			gtk_scale_set_value_pos(GTK_SCALE(w->scale),GTK_POS_BOTTOM);
			gtk_widget_set_name(w->scale,"star-scale");
			for(int i=0; i<=10; i+=1){
				gtk_scale_add_mark(GTK_SCALE(w->scale),((double)i)/2.0,GTK_POS_BOTTOM,NULL);
			}
		gtk_widget_show(w->scale);

		g_signal_connect(GTK_POPOVER(w->popover),"closed",G_CALLBACK(on_rating_popover_closed),w);

		GtkCssProvider *provider = gtk_css_provider_new();
		gtk_css_provider_load_from_data(provider,
			"scale{padding:16px;}"
			"scale trough{margin:0;border:0;background:-gtk-icontheme('non-starred-symbolic') repeat-x; min-width:calc(32px*5); min-height:32px;}"
			"scale trough highlight{margin:0;border:0;background:-gtk-icontheme('starred-symbolic') repeat-x; min-width:32px; min-height:32px;}"
			"scale trough highlight{min-width:0; background-size: 32px;}"
			"scale slider{background:transparent;min-width:0;min-height:0;margin:0;}"
			,-1,NULL
		);
		gtk_style_context_add_provider(gtk_widget_get_style_context(w->scale),GTK_STYLE_PROVIDER(provider),GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		g_object_unref(provider);
	gtk_container_add(GTK_CONTAINER(w->popover),w->scale);

	return (ddb_gtkui_widget_t*)w;
}

#endif
