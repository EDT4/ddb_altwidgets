//TODO: Drag and drop from playlist. See deadbeef/plugins/gtkui/playlist/ddblistview.c:ddb_listview_list_drag_data_received
//TODO: Jump to item menu. Requires saving item
//TODO: Save column width

#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include <stdbool.h>
#include "../gtk2.h"

extern DB_functions_t *deadbeef;
extern ddb_gtkui_t *gtkui_plugin;

struct column_data{
	char title[100];
	char format[1000];
	char *tf;
	unsigned int width;
};

struct queueview{
	ddb_gtkui_widget_t base;
    ddb_gtkui_widget_extended_api_t exapi;
    GtkTreeView *tree_view;
	guint callback_id;
	int column_count;
	int column_cap;
	struct column_data *columns;
};

static void model_init(struct queueview *data);
static gboolean model_update(struct queueview *data);

void set_column_count(struct queueview *data,int count){
	if(count > data->column_cap){
		int i = data->column_cap;
		data->column_cap = count+5;
		data->columns = realloc(data->columns,data->column_cap*sizeof(struct column_data));
		for(; i<data->column_cap; i+=1){
			data->columns[i].title[0]  = '\0';
			data->columns[i].format[0] = '\0';
			data->columns[i].tf = NULL;
			data->columns[i].width = 0;
		}
	}
	data->column_count = count;
}

static void on_add_column(__attribute__((unused)) GtkWidget *menu_item,struct queueview *data){
	set_column_count(data,data->column_count+1);
	model_init(data);
	model_update(data);
}

static void on_remove_column(__attribute__((unused)) GtkWidget *menu_item,struct queueview *data){
	set_column_count(data,data->column_count-1);
	model_init(data);
	model_update(data);
}

static void on_edit_column(GtkWidget *menu_item,struct queueview *data){
	GtkTreeViewColumn *column = GTK_TREE_VIEW_COLUMN(g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(menu_item,GTK_TYPE_MENU)),"column"));
	if(!column) return;
	GtkWidget *dialog = gtk_dialog_new_with_buttons("Edit Column",GTK_WINDOW(gtk_widget_get_toplevel(data->base.widget)),GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		"Cancel",GTK_RESPONSE_CANCEL,
		"OK",GTK_RESPONSE_OK,
		NULL
	);
		GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
		GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,8);
		gtk_container_set_border_width(GTK_CONTAINER(vbox),12);
		GtkWidget *hbox;
			intptr_t i = (intptr_t)g_object_get_data(G_OBJECT(column),"index");

			hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,8);
				gtk_container_add(GTK_CONTAINER(hbox),gtk_label_new("Title:"));
				GtkWidget *title_entry = gtk_entry_new();
					gtk_entry_set_text(GTK_ENTRY(title_entry),data->columns[i].title);
				gtk_container_add(GTK_CONTAINER(hbox),title_entry);
			gtk_container_add(GTK_CONTAINER(vbox),hbox);

			hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
				gtk_container_add(GTK_CONTAINER(hbox),gtk_label_new("Format:"));
				GtkWidget *format_entry = gtk_entry_new();
					gtk_entry_set_text(GTK_ENTRY(format_entry),data->columns[i].format);
				gtk_container_add(GTK_CONTAINER(hbox),format_entry);
			gtk_container_add(GTK_CONTAINER(vbox),hbox);
		gtk_container_add(GTK_CONTAINER(content),vbox);
	gtk_widget_show_all(dialog);

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK){
		strcpy(data->columns[i].title,gtk_entry_get_text(GTK_ENTRY(title_entry)));
		strcpy(data->columns[i].format,gtk_entry_get_text(GTK_ENTRY(format_entry)));
		deadbeef->tf_free(data->columns[i].tf);
		data->columns[i].tf = deadbeef->tf_compile(data->columns[i].format);
		gtk_label_set_text(GTK_LABEL(gtk_tree_view_column_get_widget(column)),data->columns[i].title);
		model_update(data);
	}
	gtk_widget_destroy(dialog);
}

static void add_row(struct queueview *data,int index,DB_playItem_t *item){ //TODO: How do we save the item in the row? g_object_set_data_full with unref would be nice
	GtkListStore * list_store = GTK_LIST_STORE(gtk_tree_view_get_model(data->tree_view));
	GtkTreeIter iter;
	gtk_list_store_append(list_store,&iter);
	gtk_list_store_set(list_store,&iter,0,index,-1);
	char buffer[200];
	ddb_tf_context_t ctx = {
		._size = sizeof(ddb_tf_context_t),
		.flags = DDB_TF_CONTEXT_NO_DYNAMIC,
		//.plt = plt,
		.iter = PL_MAIN,
		.it = item,
	};
	for(int i=0; i<data->column_count; i++){
		if(!data->columns[i].tf) continue;
		deadbeef->tf_eval(&ctx,data->columns[i].tf,buffer,sizeof(buffer));
		gtk_list_store_set(list_store,&iter,i+1,buffer,-1);
	}
}

//TODO: Drag and drop moving. Requires saving item
/*
static void on_row_deleted(GtkTreeModel* model,GtkTreePath* path,gpointer user_data){
	printf("deleted %d\n",gtk_tree_path_get_indices(path)[0]);
}
static void on_row_inserted(GtkTreeModel* model,GtkTreePath* path,GtkTreeIter* iter,gpointer user_data){
	printf("inserted %d\n",gtk_tree_path_get_indices(path)[0]);
}
*/

static gboolean on_column_button_press(GtkWidget *widget,GdkEventButton *event,struct queueview *data){
	if(event->button == GDK_BUTTON_SECONDARY){
	 	GtkTreeViewColumn *column = GTK_TREE_VIEW_COLUMN(g_object_get_data(G_OBJECT(widget),"column"));
	 	GtkWidget *menu = gtk_menu_new();
			g_object_set_data(G_OBJECT(menu),"column",column);

		 	GtkWidget *edit_item = gtk_menu_item_new_with_label("Edit Column");
		 		g_signal_connect(edit_item,"activate",G_CALLBACK(on_edit_column),data);
		 	gtk_menu_shell_append(GTK_MENU_SHELL(menu),edit_item);

		 	GtkWidget *add_item = gtk_menu_item_new_with_label("Add Column");
		 		g_signal_connect(add_item,"activate",G_CALLBACK(on_add_column),data);
		 	gtk_menu_shell_append(GTK_MENU_SHELL(menu),add_item);

		 	GtkWidget *remove_item = gtk_menu_item_new_with_label("Remove Column");
		 		g_signal_connect(remove_item,"activate",G_CALLBACK(on_remove_column),data);
		 	gtk_menu_shell_append(GTK_MENU_SHELL(menu),remove_item);
	 	gtk_widget_show_all(menu);
	 	gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent*)event);
	}
	return TRUE;
}

static void model_init(struct queueview *data){
	GType tys[data->column_count+1];
	tys[0] = G_TYPE_INT;
	for(int i=1; i<=data->column_count; i+=1){tys[i] = G_TYPE_STRING;}
	GtkListStore *list_store = gtk_list_store_newv(data->column_count+1,tys);
		//g_signal_connect(list_store,"row-deleted",G_CALLBACK(on_row_deleted),NULL);
		//g_signal_connect(list_store,"row-inserted",G_CALLBACK(on_row_inserted),NULL);

		{//Remove old columns.
			GList *cols = gtk_tree_view_get_columns(data->tree_view);
			for(GList *i=cols; i; i=g_list_next(i)){
				gtk_tree_view_remove_column(data->tree_view,GTK_TREE_VIEW_COLUMN(i->data));
			}
			g_list_free(cols);
		}

		//Create new columns
		GtkCellRenderer *renderer;
		GtkTreeViewColumn *column;

		//Index column.
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes("#",renderer,"text",0,NULL);
		gtk_tree_view_append_column(data->tree_view,column);

		//Data columns.
		for(int i=1; i<= data->column_count; i++){
			renderer = gtk_cell_renderer_text_new();
			g_object_set(renderer,
				"editable",FALSE,
				NULL
			);

			column = gtk_tree_view_column_new_with_attributes("",renderer,"text",i,NULL);
			g_object_set_data(G_OBJECT(column),"index",(void*)(intptr_t)(i-1));
			gtk_tree_view_column_set_resizable(column,true);
			gtk_tree_view_column_set_clickable(column,true);
			if(data->columns[i-1].width == 0){
				gtk_tree_view_column_set_expand(column,true);
			}else{
				gtk_tree_view_column_set_expand(column,false);
				gtk_tree_view_column_set_fixed_width(column,data->columns[i-1].width);
			}

			{
				GtkWidget *label = gtk_label_new(data->columns[i-1].title);
				gtk_tree_view_column_set_widget(column,label);
				gtk_widget_show(label);
				GtkWidget *button = gtk_widget_get_ancestor(label,GTK_TYPE_BUTTON);
				g_object_set_data(G_OBJECT(button),"column",column);
				g_signal_connect(button,"button-press-event",G_CALLBACK(on_column_button_press),data);
			}
			gtk_tree_view_append_column(data->tree_view,column);
		}
	gtk_tree_view_set_model(data->tree_view,GTK_TREE_MODEL(list_store));
}

static gboolean model_update(struct queueview *data){
	GtkListStore * list_store = GTK_LIST_STORE(gtk_tree_view_get_model(data->tree_view));
	//const GSignalMatchType mask = (GSignalMatchType)(G_SIGNAL_MATCH_FUNC);
	//g_signal_handlers_block_matched(G_OBJECT(list_store),mask,0,0,NULL,(gpointer)on_row_deleted,data);
	//g_signal_handlers_block_matched(G_OBJECT(list_store),mask,0,0,NULL,(gpointer)on_row_inserted,data);
		gtk_list_store_clear(list_store);

		for(int i=0,count=deadbeef->playqueue_get_count(); i<count; i+=1){
			DB_playItem_t *item = deadbeef->playqueue_get_item(i);
			add_row(data,i,item);
			deadbeef->pl_item_unref(item);
		}
	//g_signal_handlers_unblock_matched(G_OBJECT(list_store),mask,0,0,NULL,(gpointer)on_row_inserted,data);
	//g_signal_handlers_unblock_matched(G_OBJECT(list_store),mask,0,0,NULL,(gpointer)on_row_deleted,data);
	return G_SOURCE_REMOVE;
}

static gint selection_compare_func(GtkTreePath *a,GtkTreePath *b){
	return gtk_tree_path_get_indices(a)[0] < gtk_tree_path_get_indices(b)[0]? 1 : -1;
}
static void remove_selected_rows(struct queueview *data){
	//GtkListStore * list_store = GTK_LIST_STORE(gtk_tree_view_get_model(data->tree_view));
	GtkTreeSelection *selection = gtk_tree_view_get_selection(data->tree_view);
	GList *selected_rows = gtk_tree_selection_get_selected_rows(selection,NULL);

	//Sort desc to work with multiple selections. TODO: are there any better solutions?
	GList *iter = g_list_sort(selected_rows,(GCompareFunc)selection_compare_func);

	while(iter){
		deadbeef->playqueue_remove_nth(gtk_tree_path_get_indices((GtkTreePath *)iter->data)[0]);
		iter = g_list_next(iter);
	}

	g_list_free_full(selected_rows,(GDestroyNotify)gtk_tree_path_free);
}

static void on_row_activate(){
	puts("double");
}

static gboolean on_row_button_press(GtkWidget *widget,GdkEventButton *event,struct queueview *data){
	if(event->button == GDK_BUTTON_SECONDARY){
		GtkTreePath *path = NULL;

		if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget),(gint)event->x,(gint)event->y,&path,NULL,NULL,NULL)){
			GtkWidget *menu = gtk_menu_new();
				GtkWidget *item;
				item = gtk_menu_item_new_with_label("Unqueue");
					g_signal_connect_swapped(item,"activate",G_CALLBACK(remove_selected_rows),data);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);

				item = gtk_menu_item_new_with_label("Jump to");
				gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);

				gtk_widget_show_all(menu);
			gtk_menu_popup_at_pointer(GTK_MENU(menu),(GdkEvent*)event);
			gtk_tree_path_free(path);
		}
		return FALSE;
	}else if(event->button == GDK_BUTTON_PRIMARY && event->type == GDK_2BUTTON_PRESS){
		on_row_activate();
		return TRUE;
	}
	return FALSE;
}

static gboolean on_key_press(__attribute__((unused)) GtkWidget *widget,GdkEventKey *event,struct queueview *data){
	switch(event->keyval){
		case GDK_KEY_Delete:
			remove_selected_rows(data);
			return TRUE;
		case GDK_KEY_Return:
			on_row_activate();
			return TRUE;
	}
	return FALSE;
}

static void queueview_on_callback_end(void *user_data){
	struct queueview *data = (struct queueview*)user_data;
	data->callback_id = 0;
}

static void queueview_init(ddb_gtkui_widget_t *w){
	struct queueview *data = (struct queueview*)w;
	model_init(data);
	data->callback_id = g_idle_add_full(G_PRIORITY_LOW,G_SOURCE_FUNC(model_update),data,queueview_on_callback_end);
}

static int queueview_message(struct ddb_gtkui_widget_s *w,uint32_t id,__attribute__((unused)) uintptr_t ctx,uint32_t p1,__attribute__((unused)) uint32_t p2){
	struct queueview *data = (struct queueview*)w;
	if((id == DB_EV_TRACKINFOCHANGED || id == DB_EV_PLAYLISTCHANGED) && p1 == DDB_PLAYLIST_CHANGE_PLAYQUEUE && data->callback_id == 0){
		data->callback_id = g_idle_add_full(G_PRIORITY_LOW,G_SOURCE_FUNC(model_update),data,queueview_on_callback_end);
	}
	return 0;
}

static void queueview_deserialize_from_keyvalues(ddb_gtkui_widget_t *base,const char **keyvalues){
	struct queueview *data = (struct queueview*)base;
	for(size_t i=0; keyvalues[i]!=NULL; i+=2){
		if(strcmp(keyvalues[i],"title") == 0){
			const char *c = keyvalues[i+1];
			int j = 0;
			int k = 0;
			while(true){switch(*c){
				case ';':
					data->columns[j].title[k] = '\0';
					j+= 1;
					k = 0;
					if(j >= data->column_count) set_column_count(data,j+1);
					break;
				case '\0':
					data->columns[j].title[k] = '\0';
					goto End;
				default:
					data->columns[j].title[k++] = *c;
					break;
			} c+= 1;}
		}else if(strcmp(keyvalues[i],"format") == 0){
			const char *c = keyvalues[i+1];
			int j = 0;
			int k = 0;
			while(true){switch(*c){
				case ';':
					data->columns[j].format[k] = '\0';
					deadbeef->tf_free(data->columns[j].tf);
					data->columns[j].tf = deadbeef->tf_compile(data->columns[j].format);
					j+= 1;
					k = 0;
					if(j >= data->column_count) set_column_count(data,j+1);
					break;
				case '\0':
					data->columns[j].format[k] = '\0';
					deadbeef->tf_free(data->columns[j].tf);
					data->columns[j].tf = deadbeef->tf_compile(data->columns[j].format);
					goto End;
				default:
					data->columns[j].format[k++] = *c;
					break;
			} c+= 1;}
		}else if(strcmp(keyvalues[i],"width") == 0){
			const char *c = keyvalues[i+1];
			int j = 0;
			while(true){
				data->columns[j++].width = (unsigned int)strtoul(c,(char**)&c,10);
				switch(*(c++)){
					case ';':
						if(j >= data->column_count) set_column_count(data,j+1);
						break;
					case '\0':
						goto End;
				}
			}
		}
		End:;
	}
}
static const char **queueview_serialize_to_keyvalues(ddb_gtkui_widget_t *base){
	struct queueview *data = (struct queueview*)base;
	#define INT_BUFFER_LEN 20
	#define ENTRIES 3
	char const **kv = calloc(ENTRIES * 2 + 1,sizeof(char *));
	size_t e = 0;

	if(data->column_count){ //TODO: Change this to just numbered?
		kv[e++] = "title";
		{
			char *v = malloc(data->column_count * (1+sizeof(data->columns[0].title)));
			kv[e++] = v;
			int c = 0;
			Loop1:
			for(char *i = data->columns[c].title; *i; i+=1){if(*v != ';') *(v++) = *i;}
			if(++c < data->column_count){ //Loop or end.
				*(v++) = ';';
				goto Loop1;
			}else{
				*v = '\0';
			}
		}

		kv[e++] = "format";
		{
			char *v = malloc(data->column_count * (1+sizeof(data->columns[0].format)));
			kv[e++] = v;
			int c = 0;
			Loop2:
			for(char *i = data->columns[c].format; *i; i+=1){if(*v != ';') *(v++) = *i;}
			if(++c < data->column_count){ //Loop or end.
				*(v++) = ';';
				goto Loop2;
			}else{
				*v = '\0';
			}
		}

		kv[e++] = "width";
		{
			char *v = malloc(data->column_count * (1+INT_BUFFER_LEN));
			kv[e++] = v;
			int c = 0;
			Loop3:
			v+= snprintf(v,INT_BUFFER_LEN,"%u",data->columns[c].width);
			if(++c < data->column_count){ //Loop or end.
				*(v++) = ';';
				goto Loop3;
			}
		}
	}

	return kv;
}
static void queueview_free_serialized_keyvalues(ddb_gtkui_widget_t *base,char const **keyvalues){
	struct queueview *data = (struct queueview*)base;
	if(data->column_count){
		free((char*)(keyvalues[1]));
		free((char*)(keyvalues[3]));
		free((char*)(keyvalues[5]));
	}
	free(keyvalues);
}

ddb_gtkui_widget_t *queueview_create(){
	struct queueview *w = calloc(1,sizeof(struct queueview));
	w->base.widget = gtk_scrolled_window_new(NULL,NULL);
	w->base.init    = queueview_init;
	w->base.message = queueview_message;
	w->exapi._size = sizeof(ddb_gtkui_widget_extended_api_t);
	w->exapi.deserialize_from_keyvalues = queueview_deserialize_from_keyvalues;
	w->exapi.serialize_to_keyvalues     = queueview_serialize_to_keyvalues;
	w->exapi.free_serialized_keyvalues  = queueview_free_serialized_keyvalues;
	w->tree_view = GTK_TREE_VIEW(gtk_tree_view_new());
	w->callback_id = 0;
	w->column_count = 1;
	w->column_cap = 2;
	w->columns = calloc(w->column_cap,sizeof(struct column_data));
	strcpy(w->columns[0].title,"Title");
	strcpy(w->columns[0].format,"%title%");
	w->columns[0].tf = deadbeef->tf_compile(w->columns[0].format);

    gtk_tree_view_set_reorderable(w->tree_view,TRUE);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(w->tree_view),GTK_SELECTION_MULTIPLE);
    g_signal_connect(GTK_WIDGET(w->tree_view),"button-press-event",G_CALLBACK(on_row_button_press),w);
    g_signal_connect(GTK_WIDGET(w->tree_view),"key-press-event",G_CALLBACK(on_key_press),w);
	gtk_widget_show(GTK_WIDGET(w->tree_view));
	gtk_container_add(GTK_CONTAINER(w->base.widget),GTK_WIDGET(w->tree_view));

	gtk_widget_show(w->base.widget);
	gtkui_plugin->w_override_signals(w->base.widget,w);

	return (ddb_gtkui_widget_t*)w;
}
