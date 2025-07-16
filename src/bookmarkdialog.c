/*
 *      bookmarkdialog.c
 *
 *      Copyright 2008-2011 Enrico Tröger <enrico(at)xfce(dot)org>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; version 2 of the License.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "bookmark.h"
#include "settings.h"
#include "backendgvfs.h"
#include "window.h"
#include "common.h"
#include "bookmarkdialog.h"
#include "bookmarkeditdialog.h"

typedef struct _GigoloBookmarkDialogPrivate			GigoloBookmarkDialogPrivate;

struct _GigoloBookmarkDialogPrivate
{
	GigoloWindow *parent;

	GtkWidget *tree;
	GtkListStore *store;

	GtkWidget *button_edit;
	GtkWidget *button_delete;

	GtkWidget *popup_menu;
	GtkWidget *edit_item;
	GtkWidget *delete_item;
};

enum
{
	COL_NAME,
	COL_SCHEME,
	COL_HOST,
	COL_PORT,
	COL_AUTOMOUNT,
	COL_USERNAME,
	COL_OTHER,
	COL_COLOR,
	COL_BMREF,
	N_COLUMNS,
	ACTION_ADD,
	ACTION_EDIT,
	ACTION_DELETE
};


G_DEFINE_TYPE_WITH_PRIVATE(GigoloBookmarkDialog, gigolo_bookmark_dialog, GTK_TYPE_DIALOG);


static void gigolo_bookmark_dialog_class_init(GigoloBookmarkDialogClass *klass)
{
}


static void update_row_in_model(GigoloBookmarkDialog *dialog, GtkTreeIter *iter, GigoloBookmark *bm)
{
	GigoloBookmarkDialogPrivate *priv = gigolo_bookmark_dialog_get_instance_private(dialog);
	gchar port[6];
	GString *other_text = g_string_new(NULL);
	const gchar *tmp;

	if (gigolo_bookmark_get_port(bm) > 0)
		g_snprintf(port, sizeof(port), "%d", gigolo_bookmark_get_port(bm));
	else
		port[0] = '\0';

	if (NZV(tmp = gigolo_bookmark_get_domain(bm)))
		g_string_append_printf(other_text, _("Domain: %s"), tmp);
	if (NZV(tmp = gigolo_bookmark_get_share(bm)))
	{
		if (other_text->len > 0)
			g_string_append(other_text, ", ");
		g_string_append_printf(other_text, _("Share: %s"), tmp);
	}
	if (NZV(tmp = gigolo_bookmark_get_folder(bm)))
	{
		if (other_text->len > 0)
			g_string_append(other_text, ", ");
		g_string_append_printf(other_text, _("Folder: %s"), tmp);
	}
	if (NZV(tmp = gigolo_bookmark_get_path(bm)))
	{
		if (other_text->len > 0)
			g_string_append(other_text, ", ");
		g_string_append_printf(other_text, _("Path: %s"), tmp);
	}

	gtk_list_store_set(priv->store, iter,
			COL_NAME, gigolo_bookmark_get_name(bm),
			COL_SCHEME, gigolo_describe_scheme(gigolo_bookmark_get_scheme(bm)),
			COL_HOST, gigolo_bookmark_get_host(bm),
			COL_PORT, port,
			COL_AUTOMOUNT, gigolo_bookmark_get_autoconnect(bm),
			COL_USERNAME, gigolo_bookmark_get_user(bm),
			COL_OTHER, other_text->str,
			COL_COLOR, gigolo_bookmark_get_color(bm),
			COL_BMREF, bm,
			-1);
	g_string_free(other_text, TRUE);
}


static void add_button_click_cb(G_GNUC_UNUSED GtkButton *button, GtkWindow *dialog)
{
	GigoloBookmarkDialogPrivate *priv = gigolo_bookmark_dialog_get_instance_private(GIGOLO_BOOKMARK_DIALOG(dialog));
	GtkWidget *edit_dialog = gigolo_bookmark_edit_dialog_new(priv->parent, GIGOLO_BE_MODE_CREATE);
	GigoloBookmark *bm = NULL;

	if (gigolo_bookmark_edit_dialog_run(GIGOLO_BOOKMARK_EDIT_DIALOG(edit_dialog)) == GTK_RESPONSE_OK)
	{
		GtkTreeIter iter;
		GigoloSettings *settings = gigolo_window_get_settings(GIGOLO_WINDOW(priv->parent));

		bm = gigolo_bookmark_new();
		/* this fills the values of the dialog into 'bm' */
		g_object_set(edit_dialog, "bookmark-update", bm, NULL);

		g_ptr_array_add(gigolo_settings_get_bookmarks(settings), bm);
		gtk_list_store_append(priv->store, &iter);

		update_row_in_model(GIGOLO_BOOKMARK_DIALOG(dialog), &iter, bm);
		gigolo_window_update_bookmarks(GIGOLO_WINDOW(priv->parent));
		gigolo_window_do_autoconnect(GIGOLO_WINDOW(priv->parent));
	}
	gtk_widget_destroy(edit_dialog);
}


static void edit_button_click_cb(G_GNUC_UNUSED GtkButton *button, GtkWindow *dialog)
{
	GtkTreeSelection *treesel;
	GtkTreeIter iter;
	GtkWidget *edit_dialog;
	GigoloBookmarkDialogPrivate *priv = gigolo_bookmark_dialog_get_instance_private(GIGOLO_BOOKMARK_DIALOG(dialog));
	GigoloBookmark *bm;

	treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->tree));

	if (gtk_tree_selection_count_selected_rows(treesel) != 1)
		return;

	gtk_tree_selection_get_selected(treesel, NULL, &iter);

	gtk_tree_model_get(GTK_TREE_MODEL(priv->store), &iter, COL_BMREF, &bm, -1);

	edit_dialog = gigolo_bookmark_edit_dialog_new_with_bookmark(priv->parent, GIGOLO_BE_MODE_EDIT, bm);
	if (gigolo_bookmark_edit_dialog_run(GIGOLO_BOOKMARK_EDIT_DIALOG(edit_dialog)) == GTK_RESPONSE_OK)
	{
		/* this fills the values of the dialog into 'bm' */
		g_object_set(edit_dialog, "bookmark-update", bm, NULL);

		update_row_in_model(GIGOLO_BOOKMARK_DIALOG(dialog), &iter, bm);
		gigolo_window_update_bookmarks(GIGOLO_WINDOW(priv->parent));
		gigolo_window_do_autoconnect(GIGOLO_WINDOW(priv->parent));
	}
	gtk_widget_destroy(edit_dialog);
}


static void delete_button_click_cb(G_GNUC_UNUSED GtkButton *button, gpointer user_data)
{
	GtkTreeSelection *treesel;
	GtkTreeIter iter;
	GigoloBookmarkDialogPrivate *priv = gigolo_bookmark_dialog_get_instance_private(user_data);
	GigoloBookmark *bm;
	GigoloBookmarkList *bml = gigolo_settings_get_bookmarks(
		gigolo_window_get_settings(GIGOLO_WINDOW(priv->parent)));

	treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->tree));

	if (gtk_tree_selection_count_selected_rows(treesel) != 1)
		return;

	gtk_tree_selection_get_selected(treesel, NULL, &iter);

	gtk_tree_model_get(GTK_TREE_MODEL(priv->store), &iter, COL_BMREF, &bm, -1);
	gtk_list_store_remove(priv->store, &iter);
	g_ptr_array_remove(bml, bm);
	gigolo_window_update_bookmarks(GIGOLO_WINDOW(priv->parent));
}


static void tree_fill(GigoloBookmarkDialog *dialog)
{
	guint i;
	GigoloBookmark *bm;
	GigoloBookmarkDialogPrivate *priv = gigolo_bookmark_dialog_get_instance_private(dialog);
	GigoloBookmarkList *bml = gigolo_settings_get_bookmarks(
		gigolo_window_get_settings(GIGOLO_WINDOW(priv->parent)));
	GtkTreeIter iter;

	for (i = 0; i < bml->len; i++)
	{
		bm = g_ptr_array_index(bml, i);

		gtk_list_store_append(priv->store, &iter);
		update_row_in_model(dialog, &iter, bm);
	}
}


static void tree_row_activated_cb(G_GNUC_UNUSED GtkTreeView *treeview,
								  G_GNUC_UNUSED GtkTreePath *path,
								  G_GNUC_UNUSED GtkTreeViewColumn *arg2, gpointer data)
{
	edit_button_click_cb(NULL, data);
}


static void tree_selection_changed_cb(GtkTreeSelection *selection, gpointer data)
{
	GigoloBookmarkDialogPrivate *priv = gigolo_bookmark_dialog_get_instance_private(data);

	gtk_widget_set_sensitive(priv->button_edit, (selection != NULL));
	gtk_widget_set_sensitive(priv->button_delete, (selection != NULL));
}


static gboolean tree_button_press_event_cb(G_GNUC_UNUSED GtkWidget *widget,
										   GdkEventButton *event, gpointer data)
{
	if (event->button == 3)
	{
		GigoloBookmarkDialogPrivate *priv = gigolo_bookmark_dialog_get_instance_private(data);
		GtkTreeSelection *treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->tree));
		gboolean have_sel = (gtk_tree_selection_count_selected_rows(treesel) > 0);

		gtk_widget_set_sensitive(priv->edit_item, have_sel);
		gtk_widget_set_sensitive(priv->delete_item, have_sel);

		gtk_menu_popup_at_pointer (GTK_MENU(priv->popup_menu),
								   (GdkEvent *)event);
		return TRUE;
	}
	return FALSE;
}


static void tree_popup_activate_cb(GtkCheckMenuItem *item, gpointer user_data)
{
	GtkWindow *dialog = g_object_get_data(G_OBJECT(item), "dialog");

	switch (GPOINTER_TO_INT(user_data))
	{
		case ACTION_ADD:
		{
			add_button_click_cb(NULL, dialog);
			break;
		}
		case ACTION_EDIT:
		{
			edit_button_click_cb(NULL, dialog);
			break;
		}
		case ACTION_DELETE:
		{
			delete_button_click_cb(NULL, dialog);
			break;
		}
	}
}


static void tree_prepare(GigoloBookmarkDialog *dialog)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *sel;
	GtkWidget *item;
	GigoloBookmarkDialogPrivate *priv = gigolo_bookmark_dialog_get_instance_private(dialog);

	priv->tree = gtk_tree_view_new();
	priv->store = gtk_list_store_new(N_COLUMNS,
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
		G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
		_("Name"), renderer, "text", COL_NAME, "cell-background", COL_COLOR, NULL);
	gtk_tree_view_column_set_sort_indicator(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, COL_NAME);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(column), TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->tree), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
		_("Service Type"), renderer, "text", COL_SCHEME, "cell-background", COL_COLOR, NULL);
	gtk_tree_view_column_set_sort_indicator(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, COL_SCHEME);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(column), TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->tree), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
		_("Host"), renderer, "text", COL_HOST, "cell-background", COL_COLOR, NULL);
	gtk_tree_view_column_set_sort_indicator(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, COL_HOST);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(column), TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->tree), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
		_("Port"), renderer, "text", COL_PORT, "cell-background", COL_COLOR, NULL);
	gtk_tree_view_column_set_sort_indicator(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, COL_PORT);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(column), TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->tree), column);

	renderer = gtk_cell_renderer_toggle_new();
	column = gtk_tree_view_column_new_with_attributes(
		_("Auto-Connect"), renderer, "active", COL_AUTOMOUNT, "cell-background", COL_COLOR, NULL);
	gtk_tree_view_column_set_sort_indicator(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, COL_AUTOMOUNT);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(column), TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->tree), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
		_("Username"), renderer, "text", COL_USERNAME, "cell-background", COL_COLOR, NULL);
	gtk_tree_view_column_set_sort_indicator(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, COL_USERNAME);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(column), TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->tree), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
		_("Other information"), renderer, "text", COL_OTHER, "cell-background", COL_COLOR, NULL);
	gtk_tree_view_column_set_sort_indicator(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, COL_OTHER);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(column), TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->tree), column);

	gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(priv->tree), TRUE);

	/* sorting */
	gtk_tree_sortable_set_sort_column_id(
		GTK_TREE_SORTABLE(priv->store), COL_NAME, GTK_SORT_ASCENDING);

	/* selection handling */
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->tree));
	gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);

	gtk_tree_view_set_model(GTK_TREE_VIEW(priv->tree), GTK_TREE_MODEL(priv->store));
	g_object_unref(G_OBJECT(priv->store));

	priv->popup_menu = gtk_menu_new();

	item = gtk_menu_item_new_with_mnemonic (_("_Add"));
	g_object_set_data(G_OBJECT(item), "dialog", dialog);
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(priv->popup_menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(tree_popup_activate_cb),
		GINT_TO_POINTER(ACTION_ADD));

	priv->edit_item = gtk_menu_item_new_with_mnemonic (_("_Edit"));
	g_object_set_data(G_OBJECT(priv->edit_item), "dialog", dialog);
	gtk_widget_show(priv->edit_item);
	gtk_container_add(GTK_CONTAINER(priv->popup_menu), priv->edit_item);
	g_signal_connect(priv->edit_item, "activate", G_CALLBACK(tree_popup_activate_cb),
		GINT_TO_POINTER(ACTION_EDIT));

	priv->delete_item = gtk_menu_item_new_with_mnemonic (_("_Delete"));
	g_object_set_data(G_OBJECT(priv->delete_item), "dialog", dialog);
	gtk_widget_show(priv->delete_item);
	gtk_container_add(GTK_CONTAINER(priv->popup_menu), priv->delete_item);
	g_signal_connect(priv->delete_item, "activate", G_CALLBACK(tree_popup_activate_cb),
		GINT_TO_POINTER(ACTION_DELETE));

	g_signal_connect(priv->tree, "row-activated", G_CALLBACK(tree_row_activated_cb), dialog);
	g_signal_connect(sel, "changed", G_CALLBACK(tree_selection_changed_cb), dialog);
	g_signal_connect(priv->tree, "button-release-event",
		G_CALLBACK(tree_button_press_event_cb), dialog);
}


static void gigolo_bookmark_dialog_init(GigoloBookmarkDialog *dialog)
{
	GtkWidget *vbox, *vbox2, *hbox, *swin, *button_add;
	GigoloBookmarkDialogPrivate *priv = gigolo_bookmark_dialog_get_instance_private(dialog);

	g_object_set(dialog,
		"icon-name", gigolo_find_icon_name("bookmark-new", "accessories-text-editor"),
		"title", _("Edit Bookmarks"),
		NULL);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_box_set_spacing(GTK_BOX(vbox), 2);

	button_add = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_label(GTK_BUTTON(button_add), _("_Close"));
	gtk_button_set_use_underline(GTK_BUTTON(button_add), TRUE);
	gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button_add, GTK_RESPONSE_CLOSE);
	gtk_widget_set_can_default(button_add, TRUE);

	gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 550, 350);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);

	button_add = gtk_button_new_from_icon_name ("list-add", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_label (GTK_BUTTON (button_add), _("_Add"));
	gtk_button_set_use_underline (GTK_BUTTON (button_add), TRUE);
	g_signal_connect(button_add, "clicked", G_CALLBACK(add_button_click_cb), dialog);

	priv->button_edit = gtk_button_new_from_icon_name ("accessories-text-editor", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_label (GTK_BUTTON (priv->button_edit), _("_Edit"));
	gtk_button_set_use_underline (GTK_BUTTON (priv->button_edit), TRUE);
	g_signal_connect(priv->button_edit, "clicked", G_CALLBACK(edit_button_click_cb), dialog);

	priv->button_delete = gtk_button_new_from_icon_name ("edit-delete", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_label (GTK_BUTTON (priv->button_delete), _("_Delete"));
	gtk_button_set_use_underline (GTK_BUTTON (priv->button_delete), TRUE);
	g_signal_connect(priv->button_delete, "clicked", G_CALLBACK(delete_button_click_cb), dialog);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_box_pack_start(GTK_BOX(hbox), button_add, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), priv->button_edit, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), priv->button_delete, FALSE, FALSE, 0);

	tree_prepare(dialog);

	swin = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_vexpand (GTK_WIDGET (swin), TRUE);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(swin), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(swin), priv->tree);

	vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_box_pack_start(GTK_BOX(vbox2), swin, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(vbox), vbox2);
	gtk_widget_show_all(vbox);
}


GtkWidget *gigolo_bookmark_dialog_new(GigoloWindow *parent)
{
	GtkWidget *dialog;
	GigoloBookmarkDialogPrivate *priv;

	dialog = g_object_new(GIGOLO_BOOKMARK_DIALOG_TYPE, "transient-for", parent, NULL);
	priv = gigolo_bookmark_dialog_get_instance_private(GIGOLO_BOOKMARK_DIALOG(dialog));

	priv->parent = parent;

	tree_fill(GIGOLO_BOOKMARK_DIALOG(dialog));

	tree_selection_changed_cb(NULL, dialog);

	return dialog;
}


