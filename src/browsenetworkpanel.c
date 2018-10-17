/*
 *      browsenetworkpanel.c
 *
 *      Copyright 2009-2011 Enrico Tröger <enrico(at)xfce(dot)org>
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
#include <gio/gio.h>

#include "common.h"
#include "backendgvfs.h"
#include "bookmark.h"
#include "settings.h"
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

	gulong browse_network_signal_id;
};

enum
{
	ACTION_BOOKMARK,
	ACTION_CONNECT
};

static void tree_selection_changed_cb(GtkTreeSelection *selection, GigoloBrowseNetworkPanel *panel);
static void browse_network_finished_cb(G_GNUC_UNUSED GigoloBackendGVFS *bnd, GigoloBrowseNetworkPanel *panel);


G_DEFINE_TYPE(GigoloBrowseNetworkPanel, gigolo_browse_network_panel, GTK_TYPE_BOX);


static void gigolo_browse_network_panel_finalize(GObject *object)
{
	GigoloBrowseNetworkPanelPrivate *priv = GIGOLO_BROWSE_NETWORK_PANEL_GET_PRIVATE(object);
	GigoloBackendGVFS *backend;

	gtk_widget_destroy(priv->popup_menu);
	g_object_unref(priv->wait_cursor);

	backend = gigolo_window_get_backend(priv->parent);
	if (backend != NULL && IS_GIGOLO_BACKEND_GVFS(backend) && priv->browse_network_signal_id > 0)
	{
		g_signal_handler_disconnect(gigolo_window_get_backend(priv->parent),
			priv->browse_network_signal_id);
		priv->browse_network_signal_id = 0;
	}


	G_OBJECT_CLASS(gigolo_browse_network_panel_parent_class)->finalize(object);
}


static void gigolo_browse_network_panel_class_init(GigoloBrowseNetworkPanelClass *klass)
{
	GObjectClass *g_object_class;

	g_object_class = G_OBJECT_CLASS(klass);
	g_object_class->finalize = gigolo_browse_network_panel_finalize;

	gigolo_browse_network_panel_parent_class = (GtkBoxClass*) g_type_class_peek(GTK_TYPE_BOX);
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
		gchar *uri, *share;
		GigoloBookmark *bm;

		gtk_tree_model_get(model, &iter,
			GIGOLO_BROWSE_NETWORK_COL_NAME, &share,
			GIGOLO_BROWSE_NETWORK_COL_URI, &uri,
			-1);

		bm = gigolo_bookmark_new_from_uri(share, uri);
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
					gigolo_window_mount_from_bookmark(GIGOLO_WINDOW(priv->parent), bm, TRUE, TRUE);
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


static void insert_row(GtkTreeStore *store, GtkTreeIter *parent, const gchar *text)
{
	gtk_tree_store_insert_with_values(store, NULL, parent, -1,
		GIGOLO_BROWSE_NETWORK_COL_NAME, text,
		GIGOLO_BROWSE_NETWORK_COL_CAN_MOUNT, FALSE,
		-1);
}


static void find_empty_nodes(GtkTreeModel *model)
{
	GtkTreeIter child, iter;

	if (! gtk_tree_model_get_iter_first(model, &iter))
	{
		insert_row(GTK_TREE_STORE(model), NULL, _("No Workgroups found"));
		return;
	}

	do
	{
		if (gtk_tree_model_iter_children(model, &child, &iter))
		{
			do
			{
				if (! gtk_tree_model_iter_has_child(model, &child))
					insert_row(GTK_TREE_STORE(model), &child, _("No Shares found"));
			}
			while (gtk_tree_model_iter_next(model, &child));
		}
		else
			insert_row(GTK_TREE_STORE(model), &iter, _("No Hosts found"));
	}
	while (gtk_tree_model_iter_next(model, &iter));
}


static void browse_network_finished_cb(G_GNUC_UNUSED GigoloBackendGVFS *bnd, GigoloBrowseNetworkPanel *panel)
{
	GigoloBrowseNetworkPanelPrivate *priv = GIGOLO_BROWSE_NETWORK_PANEL_GET_PRIVATE(panel);

	find_empty_nodes(GTK_TREE_MODEL(priv->store));

	gtk_widget_set_sensitive(priv->button_refresh, TRUE);

	gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(panel)), NULL);
}


static void gigolo_browse_network_panel_refresh(GigoloBrowseNetworkPanel *panel)
{
	GigoloBrowseNetworkPanelPrivate *priv = GIGOLO_BROWSE_NETWORK_PANEL_GET_PRIVATE(panel);

	gtk_widget_set_sensitive(priv->button_refresh, FALSE);
	gtk_tree_store_clear(priv->store);

	gigolo_backend_gvfs_browse_network(gigolo_window_get_backend(priv->parent),
		GTK_WINDOW(priv->parent), priv->store);
}


static gboolean delay_refresh(GigoloBrowseNetworkPanel *panel)
{
	GigoloBrowseNetworkPanelPrivate *priv = GIGOLO_BROWSE_NETWORK_PANEL_GET_PRIVATE(panel);

	gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(panel)), priv->wait_cursor);
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
	if (event->keyval == GDK_KEY_Return ||
		event->keyval == GDK_KEY_ISO_Enter ||
		event->keyval == GDK_KEY_KP_Enter ||
		event->keyval == GDK_KEY_space)
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
		gtk_menu_popup_at_pointer (GTK_MENU(priv->popup_menu),
								   (GdkEvent *)event);
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
			gboolean can_mount;

			gtk_tree_model_get(model, &iter, GIGOLO_BROWSE_NETWORK_COL_CAN_MOUNT, &can_mount, -1);
			/* double click on parent node expands/collapses it */
			if (! can_mount)
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
			GIGOLO_BROWSE_NETWORK_COL_CAN_MOUNT, &set,
			GIGOLO_BROWSE_NETWORK_COL_URI, &uri,
			GIGOLO_BROWSE_NETWORK_COL_NAME, &share,
			-1);

		if (set)
		{
			GigoloSettings *settings = gigolo_window_get_settings(GIGOLO_WINDOW(priv->parent));
			gchar *full_uri = g_strconcat(uri, share, "/", NULL);


			is_bookmark = (gigolo_settings_get_bookmark_by_uri(settings, full_uri) != NULL);

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


static void tree_row_inserted(G_GNUC_UNUSED GtkTreeModel *model, GtkTreePath *path,
							  G_GNUC_UNUSED GtkTreeIter *iter, GtkTreeView *tree)
{
	gtk_tree_view_expand_to_path(tree, path);
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
	store = gtk_tree_store_new(GIGOLO_BROWSE_NETWORK_N_COLUMNS,
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_ICON, G_TYPE_BOOLEAN);

    column = gtk_tree_view_column_new();

	if (gtk_check_version(2, 14, 0) == NULL)
	{
		icon_renderer = gtk_cell_renderer_pixbuf_new();
		gtk_tree_view_column_pack_start(column, icon_renderer, FALSE);
		gtk_tree_view_column_set_attributes(column, icon_renderer,
			"gicon", GIGOLO_BROWSE_NETWORK_COL_ICON, NULL);
		g_object_set(icon_renderer, "xalign", 0.0, NULL);
	}

	text_renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree), FALSE);
	gtk_tree_view_column_pack_start(column, text_renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, text_renderer,
		"text", GIGOLO_BROWSE_NETWORK_COL_NAME, NULL);

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
	g_signal_connect(store, "row-inserted", G_CALLBACK(tree_row_inserted), tree);

	/* popup menu */
	menu = gtk_menu_new();

	priv->item_connect = item = gtk_menu_item_new_with_mnemonic(_("_Connect"));
	g_object_set_data(G_OBJECT(item), "panel", panel);
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(tree_popup_activate_cb),
		GINT_TO_POINTER(ACTION_CONNECT));

	priv->item_bookmark = item = gtk_menu_item_new_with_mnemonic(_("Create _Bookmark"));
	g_object_set_data(G_OBJECT(item), "panel", panel);
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(tree_popup_activate_cb),
		GINT_TO_POINTER(ACTION_BOOKMARK));

	priv->tree = tree;
	priv->store = store;
	priv->popup_menu = menu;

	tree_selection_changed_cb(NULL, panel);
}


static void realize_cb(GigoloBrowseNetworkPanel *panel, G_GNUC_UNUSED gpointer data)
{
	g_timeout_add(250, (GSourceFunc) delay_refresh, panel);
}


static void gigolo_browse_network_panel_init(GigoloBrowseNetworkPanel *panel)
{
	GtkWidget *swin, *toolbar;
	GtkToolItem *toolitem;
	GigoloBrowseNetworkPanelPrivate *priv = GIGOLO_BROWSE_NETWORK_PANEL_GET_PRIVATE(panel);

	priv->browse_network_signal_id = 0;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (panel), GTK_ORIENTATION_VERTICAL);
	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH_HORIZ);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_BUTTON);

	toolitem = gtk_tool_button_new(gtk_image_new_from_icon_name ("gtk-connect",
		gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar))), NULL);
	gtk_widget_set_tooltip_text(GTK_WIDGET(toolitem), _("Connect to the selected share"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	g_signal_connect(toolitem, "clicked", G_CALLBACK(button_connect_click_cb), panel);
	priv->button_connect = GTK_WIDGET(toolitem);

	toolitem = gtk_tool_button_new(
		gtk_image_new_from_icon_name(gigolo_find_icon_name("bookmark-new", "gtk-edit"), GTK_ICON_SIZE_BUTTON),
		_("Create _Bookmark"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(toolitem), _("Create a bookmark from the selected share"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	g_signal_connect(toolitem, "clicked", G_CALLBACK(button_bookmark_click_cb), panel);
	priv->button_bookmark = GTK_WIDGET(toolitem);

	toolitem = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);

	toolitem = gtk_tool_button_new(gtk_image_new_from_icon_name ("gtk-refresh",
		gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar))), NULL);
	gtk_widget_set_tooltip_text(GTK_WIDGET(toolitem), _("Refresh the network list"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	g_signal_connect(toolitem, "clicked", G_CALLBACK(button_refresh_click_cb), panel);
	priv->button_refresh = GTK_WIDGET(toolitem);

	toolitem = gtk_separator_tool_item_new();
	gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(toolitem), FALSE);
	gtk_tool_item_set_expand(toolitem, TRUE);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);

	toolitem = gtk_tool_button_new(gtk_image_new_from_icon_name ("gtk-close",
		gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar))), NULL);
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

	priv->wait_cursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_WATCH);

	g_signal_connect_after(panel, "realize", G_CALLBACK(realize_cb), NULL);
}


GtkWidget *gigolo_browse_network_panel_new(GigoloWindow *parent)
{
	GtkWidget *self;
	GigoloBrowseNetworkPanelPrivate *priv;
	GigoloBackendGVFS *backend;

	self = g_object_new(GIGOLO_BROWSE_NETWORK_PANEL_TYPE, NULL);

	priv = GIGOLO_BROWSE_NETWORK_PANEL_GET_PRIVATE(self);
	priv->parent = parent;

	backend = gigolo_window_get_backend(parent);
	priv->browse_network_signal_id = g_signal_connect(backend, "browse-network-finished",
		G_CALLBACK(browse_network_finished_cb), self);

	return self;
}


