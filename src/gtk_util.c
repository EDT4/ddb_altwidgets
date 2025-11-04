#include "gtk_util.h"

/*
gchar *gtk_combo_box_text_get_text(GtkComboBoxText *combo_box,gint position){
	if(position < 0) return NULL;

	gint text_column = gtk_combo_box_get_entry_text_column(GTK_COMBO_BOX(combo_box));
	if(text_column < 0) return NULL;

	GtkListStore *model = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(combo_box)));
	if(!gtk_tree_model_get_column_type(model,text_column) != G_TYPE_STRING) return NULL;

	GtkTreeIter iter;
	if(!gtk_tree_model_iter_nth_child(model,&iter,NULL,position)) return NULL;

	gchar *text = NULL;
	gtk_tree_model_get(model,&iter,text_column,&text,-1);

	return text;
}
*/

gint gtk_combo_box_text_position_of(GtkComboBoxText *combo_box,const char *str){
	gint text_column = gtk_combo_box_get_entry_text_column(GTK_COMBO_BOX(combo_box));
	if(text_column < 0) return -1;

	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo_box));
	if(gtk_tree_model_get_column_type(model,text_column) != G_TYPE_STRING) return -1;

	gint i = 0;
	GtkTreeIter iter;
	if(!gtk_tree_model_get_iter_first(model,&iter)) return -1;
	do{
		gchar *text = NULL;
		gtk_tree_model_get(model,&iter,text_column,&text,-1);
		if(strcmp(str,text) == 0) return i;
		i+= 1;
	}while(gtk_tree_model_iter_next(model,&iter));

	return -1;
}
