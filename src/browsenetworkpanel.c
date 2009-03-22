/*
 *      browsenetworkpanel.c
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
#include "browsenetworkpanel.h"

typedef struct _GigoloBrowseNetworkPanelPrivate			GigoloBrowseNetworkPanelPrivate;

#define GIGOLO_BROWSE_NETWORK_PANEL_GET_PRIVATE(obj)		(G_TYPE_INSTANCE_GET_PRIVATE((obj),\
			GIGOLO_BROWSE_NETWORK_PANEL_TYPE, GigoloBrowseNetworkPanelPrivate))

struct _GigoloBrowseNetworkPanelPrivate
{
	GigoloWindow *parent;

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
	COLUMN_ICON,
	COLUMN_CAN_MOUNT,
	N_COLUMNS,
	ACTION_BOOKMARK,
	ACTION_CONNECT
};



static void tree_selection_changed_cb(GtkTreeSelection *selection, GigoloBrowseNetworkPanel *panel);


G_DEFINE_TYPE(GigoloBrowseNetworkPanel, gigolo_browse_network_panel, GTK_TYPE_VBOX);


static void gigolo_browse_network_panel_finalize(GObject *object)
{
	GigoloBrowseNetworkPanelPrivate *priv = GIGOLO_BROWSE_NETWORK_PANEL_GET_PRIVATE(object);

	gtk_widget_destroy(priv->popup_menu);
	gdk_cursor_unref(priv->wait_cursor);

	G_OBJECT_CLASS(gigolo_browse_network_panel_parent_class)->finalize(object);
}


static void gigolo_browse_network_panel_class_init(GigoloBrowseNetworkPanelClass *klass)
{
	GObjectClass *g_object_class;

	g_object_class = G_OBJECT_CLASS(klass);
	g_object_class->finalize = gigolo_browse_network_panel_finalize;

	gigolo_browse_network_panel_parent_class = (GtkVBoxClass*) g_type_class_peek(GTK_TYPE_VBOX);
	g_type_class_add_private(klass, sizeof(GigoloBrowseNetworkPanelPrivate));
}


static void mount_share(GigoloBrowseNetworkPanel *panel, GigoloBookmarkEditDialogMode mode)
{
	GigoloBrowseNetworkPanelPrivate *priv = GIGOLO_BROWSE_NETWORK_PANEL_GET_PRIVATE(panel);
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

			edit_dialog = gigolo_bookmark_edit_dialog_new_with_bookmark(priv->parent, mode, bm);

			if (gigolo_bookmark_edit_dialog_run(GIGOLO_BOOKMARK_EDIT_DIALOG(edit_dialog)) == GTK_RESPONSE_OK)
			{
				/* this fills the values of the panel into 'bm' */
				g_object_set(edit_dialog, "bookmark-update", bm, NULL);

				if (mode == GIGOLO_BE_MODE_CONNECT)
				{
					gigolo_window_mount_from_bookmark(GIGOLO_WINDOW(priv->parent), bm, TRUE);
				}
				else
				{
					GigoloSettings *settings = gigolo_window_get_settings(GIGOLO_WINDOW(priv->parent));
					g_ptr_array_add(gigolo_settings_get_bookmarks(settings), g_object_ref(bm));
					gigolo_window_update_bookmarks(GIGOLO_WINDOW(priv->parent));
					gigolo_settings_write(settings, GIGOLO_SETTINGS_BOOKMARKS);

					tree_selection_changed_cb(NULL, panel);
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


static void button_bookmark_click_cb(G_GNUC_UNUSED GtkToolButton *btn, GigoloBrowseNetworkPanel *panel)
{
	mount_share(panel, GIGOLO_BE_MODE_EDIT);
}


static void button_connect_click_cb(G_GNUC_UNUSED GtkToolButton *btn, GigoloBrowseNetworkPanel *panel)
{
	mount_share(panel, GIGOLO_BE_MODE_CONNECT);
}


static void gigolo_browse_network_panel_refresh(GigoloBrowseNetworkPanel *panel)
{
	GigoloBrowseNetworkPanelPrivate *priv = GIGOLO_BROWSE_NETWORK_PANEL_GET_PRIVATE(panel);
	GigoloHostUri **groups;
	GigoloHostUri **hosts;
	guint g, i, j;
	GtkTreeIter iter_group, iter_host, iter_share;
	gchar **shares;
	gpointer icon_share = NULL;

	gtk_widget_set_sensitive(priv->button_refresh, FALSE);
	gtk_tree_store_clear(priv->store);

	/* TODO make this async? */

	/* Iterate over workgroups */
	groups = gigolo_backend_gvfs_browse_network(NULL);
	if (groups != NULL)
	{
		icon_share = gigolo_backend_gvfs_get_share_icon();
		for (g = 0; groups[g] != NULL; g++)
		{
			gtk_tree_store_append(priv->store, &iter_group, NULL);
			gtk_tree_store_set(priv->store, &iter_group,
				COLUMN_URI, groups[g]->uri,
				COLUMN_NAME, groups[g]->name,
				COLUMN_ICON, groups[g]->icon,
				COLUMN_CAN_MOUNT, FALSE,
				-1);

			/* Iterate over hosts */
			hosts = gigolo_backend_gvfs_browse_network(groups[g]->uri);
			if (hosts != NULL)
			{
				for (i = 0; hosts[i] != NULL; i++)
				{
					gtk_tree_store_append(priv->store, &iter_host, &iter_group);
					gtk_tree_store_set(priv->store, &iter_host,
						COLUMN_URI, hosts[i]->uri,
						COLUMN_NAME, hosts[i]->name,
						COLUMN_ICON, hosts[i]->icon,
						COLUMN_CAN_MOUNT, FALSE,
						-1);

					/* Iterate over shares */
					shares = gigolo_backend_gvfs_get_smb_shares_from_uri(hosts[i]->uri);
					if (shares != NULL)
					{
						for (j = 0; shares[j] != NULL; j++)
						{
							gtk_tree_store_append(priv->store, &iter_share, &iter_host);
							gtk_tree_store_set(priv->store, &iter_share,
								COLUMN_NAME, shares[j],
								COLUMN_URI, hosts[i]->uri,
								COLUMN_ICON, icon_share,
								COLUMN_CAN_MOUNT, TRUE,
								-1);
						}
						g_strfreev(shares);
					}
					else
						verbose("No Shares found for %s", hosts[i]->name);

					g_object_unref(hosts[i]->icon);
					g_free(hosts[i]->uri);
					g_free(hosts[i]->name);
					g_free(hosts[i]);
				}
				g_free(hosts);
			}
			else
				verbose("No Hosts found for %s", groups[g]->name);

			g_object_unref(groups[g]->icon);
			g_free(groups[g]->uri);
			g_free(groups[g]->name);
			g_free(groups[g]);
		}
		g_free(groups);
		if (icon_share !=  NULL)
			g_object_unref(icon_share);
	}
	else
		verbose("No Workgroups found");

	if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(priv->store), NULL) == 0)
	{
		GtkTreeIter iter;
		gtk_tree_store_append(priv->store, &iter, NULL);
		gtk_tree_store_set(priv->store, &iter,
			COLUMN_NAME, _("No Workgroups found"),
			COLUMN_CAN_MOUNT, FALSE,
			-1);
	}

	gtk_tree_view_expand_all(GTK_TREE_VIEW(priv->tree));

	gdk_window_set_cursor(gigolo_widget_get_window(GTK_WIDGET(panel)), NULL);
	gtk_widget_set_sensitive(priv->button_refresh, TRUE);

	tree_selection_changed_cb(NULL, panel);
}


static gboolean delay_refresh(GigoloBrowseNetworkPanel *panel)
{
	GigoloBrowseNetworkPanelPrivate *priv = GIGOLO_BROWSE_NETWORK_PANEL_GET_PRIVATE(panel);

	gdk_window_set_cursor(gigolo_widget_get_window(GTK_WIDGET(panel)), priv->wait_cursor);
	/* Force the update of the cursor */
	while (g_main_context_iteration(NULL, FALSE));

	gigolo_browse_network_panel_refresh(panel);
	return FALSE;
}


static void button_refresh_click_cb(G_GNUC_UNUSED GtkToolButton *btn, GigoloBrowseNetworkPanel *panel)
{
	g_idle_add((GSourceFunc) delay_refresh, panel);
}


static void button_close_click_cb(G_GNUC_UNUSED GtkToolButton *btn, GigoloBrowseNetworkPanel *panel)
{
	GigoloBrowseNetworkPanelPrivate *priv = GIGOLO_BROWSE_NETWORK_PANEL_GET_PRIVATE(panel);
	GigoloSettings *settings = gigolo_window_get_settings(GIGOLO_WINDOW(priv->parent));
	/* hide the panel by setting the property to FALSE */
	g_object_set(settings, "show-panel", FALSE, NULL);
}


static gboolean tree_key_press_event(G_GNUC_UNUSED GtkWidget *widget, GdkEventKey *event,
									 GigoloBrowseNetworkPanel *panel)
{
	if (event->keyval == GDK_Return ||
		event->keyval == GDK_ISO_Enter ||
		event->keyval == GDK_KP_Enter ||
		event->keyval == GDK_space)
	{
		mount_share(panel, GIGOLO_BE_MODE_CONNECT);
		return TRUE;
	}
	return FALSE;
}


static gboolean tree_button_release_event(G_GNUC_UNUSED GtkWidget *widget, GdkEventButton *event,
										  GigoloBrowseNetworkPanel *panel)
{
	if (event->button == 3)
	{
		GigoloBrowseNetworkPanelPrivate *priv = GIGOLO_BROWSE_NETWORK_PANEL_GET_PRIVATE(panel);
		gtk_menu_popup(GTK_MENU(priv->popup_menu), NULL, NULL, NULL, NULL,
															event->button, event->time);
		return TRUE;
	}
	return FALSE;
}


static gboolean tree_button_press_event(GtkWidget *widget, GdkEventButton *event,
										GigoloBrowseNetworkPanel *panel)
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
				mount_share(panel, GIGOLO_BE_MODE_CONNECT);
			}
			return TRUE;
		}
	}
	return FALSE;
}


static void tree_popup_activate_cb(GtkCheckMenuItem *item, gpointer user_data)
{
	GigoloBrowseNetworkPanel *panel = g_object_get_data(G_OBJECT(item), "panel");

	switch (GPOINTER_TO_INT(user_data))
	{
		case ACTION_CONNECT:
		{
			button_connect_click_cb(NULL, panel);
			break;
		}
		case ACTION_BOOKMARK:
		{
			button_bookmark_click_cb(NULL, panel);
			break;
		}
	}
}


static void tree_selection_changed_cb(GtkTreeSelection *selection, GigoloBrowseNetworkPanel *panel)
{
	GigoloBrowseNetworkPanelPrivate *priv = GIGOLO_BROWSE_NETWORK_PANEL_GET_PRIVATE(panel);
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean set = FALSE;
	gboolean is_bookmark = FALSE;

	if (selection == NULL)
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->tree));

	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *uri, *share;
		gtk_tree_model_get(model, &iter,
			COLUMN_CAN_MOUNT, &set,
			COLUMN_URI, &uri,
			COLUMN_NAME, &share,
			-1);

		if (set)
		{
			gchar *full_uri = g_strconcat(uri, share, "/", NULL);

			is_bookmark = (gigolo_window_find_bookmark_by_uri(
								GIGOLO_WINDOW(priv->parent), full_uri) != NULL);

			g_free(full_uri);
		}
		g_free(share);
		g_free(uri);
	}

	gtk_widget_set_sensitive(priv->button_connect, set);
	gtk_widget_set_sensitive(priv->button_bookmark, set && ! is_bookmark);
	gtk_widget_set_sensitive(priv->item_connect, set);
	gtk_widget_set_sensitive(priv->item_bookmark, set && ! is_bookmark);
}


static void tree_prepare(GigoloBrowseNetworkPanel *panel)
{
	GtkCellRenderer *text_renderer, *icon_renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;
	GtkWidget *tree, *menu, *item;
	GtkTreeStore *store;
	GigoloBrowseNetworkPanelPrivate *priv = GIGOLO_BROWSE_NETWORK_PANEL_GET_PRIVATE(panel);

	tree = gtk_tree_view_new();
	store = gtk_tree_store_new(N_COLUMNS,
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_ICON, G_TYPE_BOOLEAN);

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

	g_signal_connect(tree, "button-press-event", G_CALLBACK(tree_button_press_event), panel);
	g_signal_connect(tree, "button-release-event", G_CALLBACK(tree_button_release_event), panel);
	g_signal_connect(tree, "key-press-event", G_CALLBACK(tree_key_press_event), panel);
	g_signal_connect(selection, "changed", G_CALLBACK(tree_selection_changed_cb), panel);

	/* popup menu */
	menu = gtk_menu_new();

	priv->item_connect = item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CONNECT, NULL);
	g_object_set_data(G_OBJECT(item), "panel", panel);
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(tree_popup_activate_cb),
		GINT_TO_POINTER(ACTION_CONNECT));

	priv->item_bookmark = item = gtk_image_menu_item_new_with_mnemonic(_("Create _Bookmark"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
		gtk_image_new_from_icon_name(
			gigolo_find_icon_name("bookmark-new", GTK_STOCK_EDIT), GTK_ICON_SIZE_BUTTON));
	g_object_set_data(G_OBJECT(item), "panel", panel);
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(tree_popup_activate_cb),
		GINT_TO_POINTER(ACTION_BOOKMARK));

	priv->tree = tree;
	priv->store = store;
	priv->popup_menu = menu;
}


static void realize_cb(GtkWidget *panel, G_GNUC_UNUSED gpointer data)
{
	g_timeout_add(250, (GSourceFunc) delay_refresh, panel);
}


static void gigolo_browse_network_panel_init(GigoloBrowseNetworkPanel *panel)
{
	GtkWidget *swin, *toolbar;
	GtkToolItem *toolitem;
	GigoloBrowseNetworkPanelPrivate *priv = GIGOLO_BROWSE_NETWORK_PANEL_GET_PRIVATE(panel);

	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH_HORIZ);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_BUTTON);

	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_CONNECT);
	gtk_widget_set_tooltip_text(GTK_WIDGET(toolitem), _("Connect to the selected share"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	g_signal_connect(toolitem, "clicked", G_CALLBACK(button_connect_click_cb), panel);
	priv->button_connect = GTK_WIDGET(toolitem);

	toolitem = gtk_tool_button_new(
		gtk_image_new_from_icon_name(gigolo_find_icon_name("bookmark-new", GTK_STOCK_EDIT), GTK_ICON_SIZE_BUTTON),
		_("Create _Bookmark"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(toolitem), _("Create a bookmark from the selected share"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	g_signal_connect(toolitem, "clicked", G_CALLBACK(button_bookmark_click_cb), panel);
	priv->button_bookmark = GTK_WIDGET(toolitem);

	toolitem = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);

	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_REFRESH);
	gtk_widget_set_tooltip_text(GTK_WIDGET(toolitem), _("Refresh the network list"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	g_signal_connect(toolitem, "clicked", G_CALLBACK(button_refresh_click_cb), panel);
	priv->button_refresh = GTK_WIDGET(toolitem);

	toolitem = gtk_separator_tool_item_new();
	gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(toolitem), FALSE);
	gtk_tool_item_set_expand(toolitem, TRUE);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);

	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_widget_set_tooltip_text(GTK_WIDGET(toolitem), _("Close panel"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	g_signal_connect(toolitem, "clicked", G_CALLBACK(button_close_click_cb), panel);

	tree_prepare(panel);

	swin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(swin), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(swin), priv->tree);

	gtk_box_pack_start(GTK_BOX(panel), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(panel), swin, TRUE, TRUE, 0);

	gtk_widget_show_all(toolbar);
	gtk_widget_show_all(swin);

	priv->wait_cursor = gdk_cursor_new(GDK_WATCH);

	g_signal_connect_after(panel, "realize", G_CALLBACK(realize_cb), NULL);
}


GtkWidget *gigolo_browse_network_panel_new(GigoloWindow *parent)
{
	GtkWidget *self;
	GigoloBrowseNetworkPanelPrivate *priv;

	self = g_object_new(GIGOLO_BROWSE_NETWORK_PANEL_TYPE, NULL);

	priv = GIGOLO_BROWSE_NETWORK_PANEL_GET_PRIVATE(self);
	priv->parent = parent;

	return self;
}


