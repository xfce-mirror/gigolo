/*
 *      browsenetworkdialog.c
 *
 *      Copyright 2009 Enrico Tröger <enrico(at)xfce(dot)org>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
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

#include "config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "common.h"
#include "compat.h"
#include "main.h"
#include "backendgvfs.h"
#include "settings.h"
#include "bookmark.h"
#include "window.h"
#include "bookmarkeditdialog.h"
#include "browsenetworkdialog.h"

typedef struct _GigoloBrowseNetworkDialogPrivate			GigoloBrowseNetworkDialogPrivate;

#define GIGOLO_BROWSE_NETWORK_DIALOG_GET_PRIVATE(obj)		(G_TYPE_INSTANCE_GET_PRIVATE((obj),\
			GIGOLO_BROWSE_NETWORK_DIALOG_TYPE, GigoloBrowseNetworkDialogPrivate))

struct _GigoloBrowseNetworkDialogPrivate
{
	GtkWindow *parent;
	GigoloSettings *settings;

	GtkWidget *button_refresh;
	GtkWidget *button_connect;
	GtkWidget *button_bookmark;
	GtkWidget *item_connect;
	GtkWidget *item_bookmark;

	GdkCursor *wait_cursor;

	GtkWidget *popup_menu;
	GtkWidget *tree;
	GtkTreeStore *store;
};

enum
{
	COLUMN_NAME,
	COLUMN_URI,
	COLUMN_SHARE,
	COLUMN_ICON,
	N_COLUMNS,
	ACTION_BOOKMARK,
	ACTION_CONNECT
};



G_DEFINE_TYPE(GigoloBrowseNetworkDialog, gigolo_browse_network_dialog, GTK_TYPE_DIALOG);


static void gigolo_browse_network_dialog_finalize(GObject *object)
{
	GigoloBrowseNetworkDialogPrivate *priv = GIGOLO_BROWSE_NETWORK_DIALOG_GET_PRIVATE(object);

	gtk_widget_destroy(priv->popup_menu);
	gdk_cursor_unref(priv->wait_cursor);

	G_OBJECT_CLASS(gigolo_browse_network_dialog_parent_class)->finalize(object);
}


static void gigolo_browse_network_dialog_response(GtkDialog *dialog, G_GNUC_UNUSED gint response)
{
	gtk_widget_destroy(GTK_WIDGET(dialog));
}


static void gigolo_browse_network_dialog_class_init(GigoloBrowseNetworkDialogClass *klass)
{
	GObjectClass *g_object_class;
	GtkDialogClass *gtk_dialog_class;

	gtk_dialog_class = GTK_DIALOG_CLASS(klass);
	gtk_dialog_class->response = gigolo_browse_network_dialog_response;

	g_object_class = G_OBJECT_CLASS(klass);
	g_object_class->finalize = gigolo_browse_network_dialog_finalize;

	gigolo_browse_network_dialog_parent_class = (GtkDialogClass*) g_type_class_peek(GTK_TYPE_DIALOG);
	g_type_class_add_private(klass, sizeof(GigoloBrowseNetworkDialogPrivate));
}


static void mount_share(GigoloBrowseNetworkDialog *dialog, GigoloBookmarkEditDialogMode mode)
{
	GigoloBrowseNetworkDialogPrivate *priv = GIGOLO_BROWSE_NETWORK_DIALOG_GET_PRIVATE(dialog);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->tree));
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected(selection, &model, &iter) &&
		! gtk_tree_model_iter_has_child(model, &iter))
	{
		gchar *uri, *share, *full_uri;
		GigoloBookmark *bm;

		gtk_tree_model_get(model, &iter, COLUMN_NAME, &share, COLUMN_URI, &uri, -1);

		full_uri = g_strconcat(uri, share, NULL);

		bm = gigolo_bookmark_new_from_uri(share, full_uri);
		if (gigolo_bookmark_is_valid(bm))
		{
			GtkWidget *edit_dialog;

			edit_dialog = gigolo_bookmark_edit_dialog_new_with_bookmark(
				GTK_WINDOW(dialog), priv->settings, mode, bm);

			if (gigolo_bookmark_edit_dialog_run(GIGOLO_BOOKMARK_EDIT_DIALOG(edit_dialog)) == GTK_RESPONSE_OK)
			{
				/* this fills the values of the dialog into 'bm' */
				g_object_set(edit_dialog, "bookmark-update", bm, NULL);

				if (mode == GIGOLO_BE_MODE_CONNECT)
				{
					gigolo_window_mount_from_bookmark(GIGOLO_WINDOW(priv->parent), bm, TRUE);
				}
				else
				{
					g_ptr_array_add(gigolo_settings_get_bookmarks(priv->settings), g_object_ref(bm));
					gigolo_window_update_bookmarks(GIGOLO_WINDOW(priv->parent));
					gigolo_settings_write(priv->settings, GIGOLO_SETTINGS_BOOKMARKS);
				}
			}
			gtk_widget_destroy(edit_dialog);
		}

		g_object_unref(bm);
		g_free(uri);
		g_free(full_uri);
		g_free(share);
	}
}


static void button_bookmark_click_cb(G_GNUC_UNUSED GtkButton *btn, GigoloBrowseNetworkDialog *dialog)
{
	mount_share(dialog, GIGOLO_BE_MODE_EDIT);
}


static void button_connect_click_cb(G_GNUC_UNUSED GtkButton *btn, GigoloBrowseNetworkDialog *dialog)
{
	mount_share(dialog, GIGOLO_BE_MODE_CONNECT);
}


static void gigolo_browse_network_dialog_refresh(GigoloBrowseNetworkDialog *dialog)
{
	GigoloBrowseNetworkDialogPrivate *priv = GIGOLO_BROWSE_NETWORK_DIALOG_GET_PRIVATE(dialog);
	GigoloHostUri **hosts;

	gtk_widget_set_sensitive(priv->button_refresh, FALSE);
	gtk_tree_store_clear(priv->store);

	/* TODO make this async? */
	hosts = gigolo_backend_gvfs_browse_network();
	if (hosts != NULL)
	{
		guint i;
		GtkTreeIter iter_host, iter_share;
		GtkTreeIter *iter_root = NULL;
		gpointer icon_share = NULL;

		if (gtk_check_version(2, 14, 0) == NULL)
			icon_share = gigolo_backend_gvfs_get_share_icon();

		for (i = 0; hosts[i] != NULL; i++)
		{
			gtk_tree_store_append(priv->store, &iter_host, iter_root);
			gtk_tree_store_set(priv->store, &iter_host,
				COLUMN_URI, hosts[i]->uri,
				COLUMN_NAME, hosts[i]->name,
				COLUMN_ICON, hosts[i]->icon,
				-1);

			if (gigolo_str_equal(hosts[i]->uri, "smb:///"))
			{	/* Cache the root element("Windows Network") and use it as parent for found hosts. */
				if (iter_root == NULL)
					iter_root = gtk_tree_iter_copy(&iter_host);
			}
			else
			{	/* Now we have a host, look for shares. */
				gchar **shares = gigolo_backend_gvfs_get_smb_shares_from_uri(hosts[i]->uri);
				if (shares != NULL)
				{
					guint j;
					for (j = 0; shares[j] != NULL; j++)
					{
						gtk_tree_store_append(priv->store, &iter_share, &iter_host);
						gtk_tree_store_set(priv->store, &iter_share,
							COLUMN_NAME, shares[j],
							COLUMN_URI, hosts[i]->uri,
							COLUMN_ICON, icon_share,
							-1);
					}
					g_strfreev(shares);
				}
			}
			g_object_unref(hosts[i]->icon);
			g_free(hosts[i]->uri);
			g_free(hosts[i]->name);
			g_free(hosts[i]);
		}
		gtk_tree_iter_free(iter_root);
		if (icon_share !=  NULL)
			g_object_unref(icon_share);
		g_free(hosts);
	}
	gtk_tree_view_expand_all(GTK_TREE_VIEW(priv->tree));

	gdk_window_set_cursor(gigolo_widget_get_window(GTK_WIDGET(dialog)), NULL);
	gtk_widget_set_sensitive(priv->button_refresh, TRUE);
}


static gboolean delay_refresh(GigoloBrowseNetworkDialog *dialog)
{
	GigoloBrowseNetworkDialogPrivate *priv = GIGOLO_BROWSE_NETWORK_DIALOG_GET_PRIVATE(dialog);

	gdk_window_set_cursor(gigolo_widget_get_window(GTK_WIDGET(dialog)), priv->wait_cursor);
	/* Force the update of the cursor */
	while (g_main_context_iteration(NULL, FALSE));

	gigolo_browse_network_dialog_refresh(dialog);
	return FALSE;
}


static void button_refresh_click_cb(G_GNUC_UNUSED GtkWidget *button, GigoloBrowseNetworkDialog *dialog)
{
	g_idle_add((GSourceFunc) delay_refresh, dialog);
}


static gboolean tree_key_press_event(G_GNUC_UNUSED GtkWidget *widget, GdkEventKey *event,
									 GigoloBrowseNetworkDialog *dialog)
{
	if (event->keyval == GDK_Return ||
		event->keyval == GDK_ISO_Enter ||
		event->keyval == GDK_KP_Enter ||
		event->keyval == GDK_space)
	{
		mount_share(dialog, GIGOLO_BE_MODE_CONNECT);
		return TRUE;
	}
	return FALSE;
}


static gboolean tree_button_release_event(G_GNUC_UNUSED GtkWidget *widget, GdkEventButton *event,
										  GigoloBrowseNetworkDialog *dialog)
{
	if (event->button == 3)
	{
		GigoloBrowseNetworkDialogPrivate *priv = GIGOLO_BROWSE_NETWORK_DIALOG_GET_PRIVATE(dialog);
		gtk_menu_popup(GTK_MENU(priv->popup_menu), NULL, NULL, NULL, NULL,
															event->button, event->time);
		return TRUE;
	}
	return FALSE;
}


static gboolean tree_button_press_event(GtkWidget *widget, GdkEventButton *event,
										GigoloBrowseNetworkDialog *dialog)
{
	if (event->type == GDK_2BUTTON_PRESS)
	{
		GtkTreeSelection *selection;
		GtkTreeModel *model;
		GtkTreeIter iter;

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));

		if (gtk_tree_selection_get_selected(selection, &model, &iter))
		{
			/* double click on parent node expands/collapses it */
			if (gtk_tree_model_iter_has_child(model, &iter))
			{
				GtkTreePath *path = gtk_tree_model_get_path(model, &iter);

				if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(widget), path))
					gtk_tree_view_collapse_row(GTK_TREE_VIEW(widget), path);
				else
					gtk_tree_view_expand_row(GTK_TREE_VIEW(widget), path, FALSE);

				gtk_tree_path_free(path);
			}
			else
			{
				mount_share(dialog, GIGOLO_BE_MODE_CONNECT);
			}
			return TRUE;
		}
	}
	return FALSE;
}


static void tree_popup_activate_cb(GtkCheckMenuItem *item, gpointer user_data)
{
	GigoloBrowseNetworkDialog *dialog = g_object_get_data(G_OBJECT(item), "dialog");

	switch (GPOINTER_TO_INT(user_data))
	{
		case ACTION_CONNECT:
		{
			button_connect_click_cb(NULL, dialog);
			break;
		}
		case ACTION_BOOKMARK:
		{
			button_bookmark_click_cb(NULL, dialog);
			break;
		}
	}
}


static void tree_selection_changed_cb(GtkTreeSelection *selection, GigoloBrowseNetworkDialog *dialog)
{
	GigoloBrowseNetworkDialogPrivate *priv = GIGOLO_BROWSE_NETWORK_DIALOG_GET_PRIVATE(dialog);
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean set;

	set = (selection != NULL && gtk_tree_selection_get_selected(selection, &model, &iter) &&
		   ! gtk_tree_model_iter_has_child(model, &iter));

	gtk_widget_set_sensitive(priv->button_connect, set);
	gtk_widget_set_sensitive(priv->button_bookmark, set);
	gtk_widget_set_sensitive(priv->item_connect, set);
	gtk_widget_set_sensitive(priv->item_bookmark, set);
}


static void tree_prepare(GigoloBrowseNetworkDialog *dialog)
{
	GtkCellRenderer *text_renderer, *icon_renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;
	GtkWidget *tree, *menu, *item;
	GtkTreeStore *store;
	GigoloBrowseNetworkDialogPrivate *priv = GIGOLO_BROWSE_NETWORK_DIALOG_GET_PRIVATE(dialog);

	tree = gtk_tree_view_new();
	store = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_ICON);

    column = gtk_tree_view_column_new();

	if (gtk_check_version(2, 14, 0) == NULL)
	{
		icon_renderer = gtk_cell_renderer_pixbuf_new();
		gtk_tree_view_column_pack_start(column, icon_renderer, FALSE);
		gtk_tree_view_column_set_attributes(column, icon_renderer, "gicon", COLUMN_ICON, NULL);
		g_object_set(icon_renderer, "xalign", 0.0, NULL);
	}

	text_renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree), FALSE);
	gtk_tree_view_column_pack_start(column, text_renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, text_renderer, "text", COLUMN_NAME, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);

	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	g_object_unref(store);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

	g_signal_connect(tree, "button-press-event", G_CALLBACK(tree_button_press_event), dialog);
	g_signal_connect(tree, "button-release-event", G_CALLBACK(tree_button_release_event), dialog);
	g_signal_connect(tree, "key-press-event", G_CALLBACK(tree_key_press_event), dialog);
	g_signal_connect(selection, "changed", G_CALLBACK(tree_selection_changed_cb), dialog);

	/* popup menu */
	menu = gtk_menu_new();

	priv->item_connect = item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CONNECT, NULL);
	g_object_set_data(G_OBJECT(item), "dialog", dialog);
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(tree_popup_activate_cb),
		GINT_TO_POINTER(ACTION_CONNECT));

	priv->item_bookmark = item = gtk_image_menu_item_new_with_mnemonic(_("Create _Bookmark"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
		gtk_image_new_from_icon_name(
			gigolo_find_icon_name("bookmark-new", GTK_STOCK_EDIT), GTK_ICON_SIZE_BUTTON));
	g_object_set_data(G_OBJECT(item), "dialog", dialog);
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(tree_popup_activate_cb),
		GINT_TO_POINTER(ACTION_BOOKMARK));

	priv->tree = tree;
	priv->store = store;
	priv->popup_menu = menu;
}


static void realize_cb(GtkWidget *dialog, G_GNUC_UNUSED gpointer data)
{
	g_timeout_add(250, (GSourceFunc) delay_refresh, dialog);
}


static void gigolo_browse_network_dialog_init(GigoloBrowseNetworkDialog *dialog)
{
	GtkWidget *vbox, *vbox2, *hbox, *swin;
	GigoloBrowseNetworkDialogPrivate *priv = GIGOLO_BROWSE_NETWORK_DIALOG_GET_PRIVATE(dialog);

	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
	vbox = gigolo_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_box_set_spacing(GTK_BOX(vbox), 2);

	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

	gtk_window_set_title(GTK_WINDOW(dialog), _("Browse Network"));
	gtk_window_set_icon_name(GTK_WINDOW(dialog), GTK_STOCK_FIND);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 450, 350);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	priv->button_connect = gtk_button_new_from_stock(GTK_STOCK_CONNECT);
	gtk_widget_set_sensitive(priv->button_connect, FALSE);
	g_signal_connect(priv->button_connect, "clicked", G_CALLBACK(button_connect_click_cb), dialog);

	priv->button_bookmark = gtk_button_new_with_mnemonic(_("Create _Bookmark"));
	gtk_button_set_image(GTK_BUTTON(priv->button_bookmark),
		gtk_image_new_from_icon_name(
			gigolo_find_icon_name("bookmark-new", GTK_STOCK_EDIT), GTK_ICON_SIZE_BUTTON));
	gtk_widget_set_sensitive(priv->button_bookmark, FALSE);
	g_signal_connect(priv->button_bookmark, "clicked", G_CALLBACK(button_bookmark_click_cb), dialog);

	priv->button_refresh = gtk_button_new_from_stock(GTK_STOCK_REFRESH);
	gtk_widget_set_sensitive(priv->button_refresh, FALSE);
	g_signal_connect(priv->button_refresh, "clicked", G_CALLBACK(button_refresh_click_cb), dialog);

	hbox = gtk_hbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(hbox), priv->button_connect, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), priv->button_bookmark, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), priv->button_refresh, FALSE, FALSE, 0);

	tree_prepare(dialog);

	swin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(swin), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(swin), priv->tree);

	vbox2 = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox2), swin, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(vbox), vbox2);
	gtk_widget_show_all(vbox);

	priv->wait_cursor = gdk_cursor_new(GDK_WATCH);

	g_signal_connect_after(dialog, "realize", G_CALLBACK(realize_cb), NULL);
}


GtkWidget *gigolo_browse_network_dialog_new(GtkWindow *parent, GigoloSettings *settings)
{
	GtkWidget *self;
	GigoloBrowseNetworkDialogPrivate *priv;

	self = g_object_new(GIGOLO_BROWSE_NETWORK_DIALOG_TYPE, "transient-for", parent, NULL);

	priv = GIGOLO_BROWSE_NETWORK_DIALOG_GET_PRIVATE(self);
	priv->parent = parent;
	priv->settings = settings;

	return self;
}


