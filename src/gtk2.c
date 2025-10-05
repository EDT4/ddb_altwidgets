#include "gtk2.h"

#if !GTK_CHECK_VERSION(3,0,0)

void gtk_combo_box_text_remove_all(GtkComboBoxText* combo_box){
	gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(combo_box))));
}

GtkWidget *gtk_button_new_from_icon_name(const gchar* icon_name,GtkIconSize size){
	GtkWidget *w = gtk_button_new();
	gtk_button_set_image(GTK_BUTTON(w),gtk_image_new_from_icon_name(icon_name,size));
	return w;
}

#endif
