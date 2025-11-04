#include "gtk2.h"

#include <gdk/gdkevents.h>

#if !GTK_CHECK_VERSION(3,0,0)

void gtk_combo_box_text_remove_all(GtkComboBoxText* combo_box){
	gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(combo_box))));
}

GtkWidget *gtk_button_new_from_icon_name(const gchar* icon_name,GtkIconSize size){
	GtkWidget *w = gtk_button_new();
	gtk_button_set_image(GTK_BUTTON(w),gtk_image_new_from_icon_name(icon_name,size));
	return w;
}

GtkWidget *gtk_box_new(GtkOrientation orientation,gint spacing){
	switch(orientation){
		case GTK_ORIENTATION_HORIZONTAL: return gtk_hbox_new(FALSE,spacing);
		case GTK_ORIENTATION_VERTICAL  : return gtk_vbox_new(FALSE,spacing);
		default: return NULL;
	}
}

void gtk_menu_popup_at_pointer(GtkMenu* menu,const GdkEvent* trigger_event){
	gtk_menu_popup(
		menu,
		NULL,
		NULL,
		NULL,
		NULL,
		//TODO
		0, //gdk_button_event_get_button((GdkButtonEvent*)trigger_event) : 0,
		0 //gdk_event_get_time(trigger_event)
	);
}

#endif
