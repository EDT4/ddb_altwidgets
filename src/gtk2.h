#ifndef __DDB_ALTERNATIVEWIDGETS_GTK2_H
#define __DDB_ALTERNATIVEWIDGETS_GTK2_H

#include <gtk/gtk.h>

#if GTK_CHECK_VERSION(3,0,0)
	#define gtk_hbutton_box_new() gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL)
	#define gtk_vbutton_box_new() gtk_button_box_new(GTK_ORIENTATION_VERTICAL)
	#define gtk_hscale_new_with_range(a,b,c) gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,a,b,c)
	#define gtk_vscale_new_with_range(a,b,c) gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL,a,b,c)
#else
	GtkWidget *gtk_box_new(GtkOrientation orientation,gint spacing);
	void gtk_combo_box_text_remove_all(GtkComboBoxText* combo_box);
	GtkWidget *gtk_button_new_from_icon_name(const gchar* icon_name,GtkIconSize size);
	void gtk_menu_popup_at_pointer(GtkMenu* menu,const GdkEvent* trigger_event);
	#define GTK_BUTTONBOX_EXPAND GTK_BUTTONBOX_DEFAULT_STYLE
	#define GDK_BUTTON_PRIMARY   1
	#define GDK_BUTTON_MIDDLE    2
	#define GDK_BUTTON_SECONDARY 3
	#include <gdk/gdkkeysyms.h>
#endif
#endif
