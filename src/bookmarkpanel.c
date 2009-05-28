/*
 *      bookmarkpanel.c
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
#include <gio/gio.h>

#include "common.h"
#include "backendgvfs.h"
#include "settings.h"
#include "bookmark.h"
#include "window.h"

#include "bookmarkpanel.h"

typedef struct _GigoloBookmarkPanelPrivate			GigoloBookmarkPanelPrivate;

#define GIGOLO_BOOKMARK_PANEL_GET_PRIVATE(obj)		(G_TYPE_INSTANCE_GET_PRIVATE((obj),\
			GIGOLO_BOOKMARK_PANEL_TYPE, GigoloBookmarkPanelPrivate))


enum
{
	GIGOLO_BOOKMARK_PANEL_COL_NAME,
	GIGOLO_BOOKMARK_PANEL_COL_REF,
	GIGOLO_BOOKMARK_PANEL_N_COLUMNS
};

enum
{
	PROP_0,
	PROP_SETTINGS
};


struct _GigoloBookmarkPanel
{
	GtkVBox parent;
};

struct _GigoloBookmarkPanelClass
{
	GtkVBoxClass parent_class;
};

struct _GigoloBookmarkPanelPrivate
{
	GigoloWindow *parent;

	GtkWidget *button_connect;

	GtkWidget *tree;
	GtkListStore *store;
};

G_DEFINE_TYPE(GigoloBookmarkPanel, gigolo_bookmark_panel, GTK_TYPE_VBOX);



static void update_store(GigoloBookmarkPanel *panel, GigoloSettings *settings)
{
	guint i;
	GigoloBookmark *bm;
	GigoloBookmarkList *bml = gigolo_settings_get_bookmarks(settings);
	GigoloBookmarkPanelPrivate *priv = GIGOLO_BOOKMARK_PANEL_GET_PRIVATE(panel);

	gtk_list_store_clear(priv->store);

	if (bml->len == 0)
	{
		gtk_list_store_insert_with_values(priv->store, NULL, -1,
			GIGOLO_BOOKMARK_PANEL_COL_NAME, _("No bookmarks"), -1);
		return;
	}

	for (i = 0; i < bml->len; i++)
	{
		bm = g_ptr_array_index(bml, i);

		gtk_list_store_insert_with_values(priv->store, NULL, -1,
			GIGOLO_BOOKMARK_PANEL_COL_NAME, gigolo_bookmark_get_name(bm),
			GIGOLO_BOOKMARK_PANEL_COL_REF, bm,
			 -1);
	}
}


static void gigolo_bookmark_panel_set_property(GObject *object, guint prop_id,
											   const GValue *value, GParamSpec *pspec)
{
	GigoloBookmarkPanel *panel = GIGOLO_BOOKMARK_PANEL(object);

	switch (prop_id)
	{
	case PROP_SETTINGS:
		update_store(panel, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}


static void gigolo_bookmark_panel_class_init(GigoloBookmarkPanelClass *klass)
{
	GObjectClass *g_object_class;

	g_object_class = G_OBJECT_CLASS(klass);

	g_object_class->set_property = gigolo_bookmark_panel_set_property;

	g_object_class_install_property(g_object_class,
										PROP_SETTINGS,
										g_param_spec_object (
										"settings",
										"Settings",
										"The associated settings",
										GIGOLO_SETTINGS_TYPE,
										G_PARAM_WRITABLE));


	g_type_class_add_private(klass, sizeof(GigoloBookmarkPanelPrivate));
}


static void button_close_click_cb(G_GNUC_UNUSED GtkToolButton *btn, GigoloBookmarkPanel *panel)
{
	GigoloBookmarkPanelPrivate *priv = GIGOLO_BOOKMARK_PANEL_GET_PRIVATE(panel);
	GigoloSettings *settings = gigolo_window_get_settings(GIGOLO_WINDOW(priv->parent));

	g_object_set(settings, "show-panel", FALSE, NULL);
}


static void button_connect_click_cb(G_GNUC_UNUSED GtkToolButton *btn, GigoloBookmarkPanel *panel)
{
	GigoloBookmarkPanelPrivate *priv = GIGOLO_BOOKMARK_PANEL_GET_PRIVATE(panel);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->tree));
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		GigoloBookmark *bm;

		gtk_tree_model_get(model, &iter, GIGOLO_BOOKMARK_PANEL_COL_REF, &bm, -1);
		gigolo_window_mount_from_bookmark(priv->parent, bm, TRUE, TRUE);
	}
}


static void tree_selection_changed_cb(GtkTreeSelection *selection, GigoloBookmarkPanel *panel)
{
	gboolean set;
	GigoloBookmarkPanelPrivate *priv = GIGOLO_BOOKMARK_PANEL_GET_PRIVATE(panel);

	set = (selection != NULL) ? gtk_tree_selection_get_selected(selection, NULL, NULL) : FALSE;

	gtk_widget_set_sensitive(priv->button_connect, set);
}


static void tree_row_activated_cb(GtkTreeView *view, GtkTreePath *path,
								  G_GNUC_UNUSED GtkTreeViewColumn *col,
								  GigoloBookmarkPanel *panel)
{
	GigoloBookmarkPanelPrivate *priv = GIGOLO_BOOKMARK_PANEL_GET_PRIVATE(panel);
	GtkTreeModel *model;
	GtkTreeIter iter;

	model = gtk_tree_view_get_model(view);
	if (gtk_tree_model_get_iter(model, &iter, path))
	{
		GigoloBookmark *bm;

		gtk_tree_model_get(model, &iter, GIGOLO_BOOKMARK_PANEL_COL_REF, &bm, -1);
		gigolo_window_mount_from_bookmark(priv->parent, bm, TRUE, TRUE);
	}
}


static void tree_prepare(GigoloBookmarkPanel *panel)
{
	GtkCellRenderer *text_renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;
	GtkWidget *tree;
	GtkListStore *store;
	GigoloBookmarkPanelPrivate *priv = GIGOLO_BOOKMARK_PANEL_GET_PRIVATE(panel);

	tree = gtk_tree_view_new();
	store = gtk_list_store_new(GIGOLO_BOOKMARK_PANEL_N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);

    column = gtk_tree_view_column_new();

	text_renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree), FALSE);
	gtk_tree_view_column_pack_start(column, text_renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, text_renderer,
		"text", GIGOLO_BOOKMARK_PANEL_COL_NAME, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);

	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	g_object_unref(store);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

	g_signal_connect(tree, "row-activated", G_CALLBACK(tree_row_activated_cb), panel);
	g_signal_connect(selection, "changed", G_CALLBACK(tree_selection_changed_cb), panel);

	priv->tree = tree;
	priv->store = store;

	tree_selection_changed_cb(NULL, panel);
}


static void gigolo_bookmark_panel_init(GigoloBookmarkPanel *self)
{
	GtkWidget *swin, *toolbar;
	GtkToolItem *toolitem;
	GigoloBookmarkPanelPrivate *priv = GIGOLO_BOOKMARK_PANEL_GET_PRIVATE(self);

	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH_HORIZ);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_BUTTON);

	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_CONNECT);
	gtk_widget_set_tooltip_text(GTK_WIDGET(toolitem), _("Connect to the selected bookmark"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	g_signal_connect(toolitem, "clicked", G_CALLBACK(button_connect_click_cb), self);
	priv->button_connect = GTK_WIDGET(toolitem);

	toolitem = gtk_separator_tool_item_new();
	gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(toolitem), FALSE);
	gtk_tool_item_set_expand(toolitem, TRUE);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);

	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_widget_set_tooltip_text(GTK_WIDGET(toolitem), _("Close panel"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	g_signal_connect(toolitem, "clicked", G_CALLBACK(button_close_click_cb), self);

	tree_prepare(self);

	swin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(swin), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(swin), priv->tree);

	gtk_box_pack_start(GTK_BOX(self), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(self), swin, TRUE, TRUE, 0);

	gtk_widget_show_all(toolbar);
	gtk_widget_show_all(swin);
}


GtkWidget *gigolo_bookmark_panel_new(GigoloWindow *parent)
{
	GtkWidget *self;
	GigoloBookmarkPanelPrivate *priv;

	self = g_object_new(GIGOLO_BOOKMARK_PANEL_TYPE, NULL);

	priv = GIGOLO_BOOKMARK_PANEL_GET_PRIVATE(self);
	priv->parent = parent;

	return self;
}

