#ifndef __DDB_ALTERNATIVEWIDGETS_GTK2_H
#define __DDB_ALTERNATIVEWIDGETS_GTK2_H

#include <gtk/gtk.h>

#if GTK_CHECK_VERSION(3,0,0)
	#define gtk_hbutton_box_new() gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL)
	#define gtk_vbutton_box_new() gtk_button_box_new(GTK_ORIENTATION_VERTICAL)
	#define gtk_hscale_new_with_range(a,b,c) gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,a,b,c)
	#define gtk_vscale_new_with_range(a,b,c) gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL,a,b,c)
#else

	void gtk_combo_box_text_remove_all(GtkComboBoxText* combo_box);
	GtkWidget *gtk_button_new_from_icon_name(const gchar* icon_name,GtkIconSize size);
	#define GTK_BUTTONBOX_EXPAND GTK_BUTTONBOX_DEFAULT_STYLE
#endif
#endif
