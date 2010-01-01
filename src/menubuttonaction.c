/*
 *      menubuttonaction.c
 *
 *      Copyright 2008-2010 Enrico Tröger <enrico(at)xfce(dot)org>
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

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "common.h"
#include "bookmark.h"
#include "settings.h"
#include "menubuttonaction.h"


enum
{
	PROP_0,
	PROP_SETTINGS
};

enum
{
	ITEM_CLICKED,
	BUTTON_CLICKED,

	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL];



G_DEFINE_TYPE(GigoloMenubuttonAction, gigolo_menu_button_action, GTK_TYPE_ACTION);


static void delegate_item_activated(GtkMenuItem *item, GigoloMenubuttonAction *action)
{
	g_signal_emit(action, signals[ITEM_CLICKED], 0, item);
}


static void delegate_button_clicked(G_GNUC_UNUSED GtkToolButton *button, GtkAction *action)
{
	g_signal_emit(action, signals[BUTTON_CLICKED], 0);
}


static GtkWidget *gigolo_menu_button_action_create_menu_item(G_GNUC_UNUSED GtkAction *action)
{
	GtkWidget *menuitem;

	menuitem = g_object_new(GTK_TYPE_IMAGE_MENU_ITEM, NULL);

	return menuitem;
}


static GtkWidget *gigolo_menu_button_action_create_tool_item(GtkAction *action)
{
	GtkWidget *toolitem;

	toolitem = g_object_new(GTK_TYPE_MENU_TOOL_BUTTON, NULL);
	g_signal_connect(toolitem, "clicked", G_CALLBACK(delegate_button_clicked), action);

	return toolitem;
}


static void set_menu(GtkWidget *item, GtkWidget *menu)
{
	if (GTK_IS_MENU_ITEM(item))
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
	else
		gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(item), menu);
}


static GtkWidget *get_menu(GtkWidget *item)
{
	if (GTK_IS_MENU_ITEM(item))
		return gtk_menu_item_get_submenu(GTK_MENU_ITEM(item));
	else
		return gtk_menu_tool_button_get_menu(GTK_MENU_TOOL_BUTTON(item));
}


static void update_menus(GigoloMenubuttonAction *action, GigoloSettings *settings)
{
	GSList *l;
	GtkWidget *menu;
	guint i;
	GtkWidget *item;
	GigoloBookmark *bm;
	GigoloBookmarkList *bml = gigolo_settings_get_bookmarks(settings);

	for (l = gtk_action_get_proxies(GTK_ACTION(action)); l; l = l->next)
	{
		menu = get_menu(l->data);

		if (GTK_IS_MENU_ITEM(l->data))
			gtk_widget_set_sensitive(l->data, (bml->len > 0));

		if (bml->len == 0)
		{
			if (menu != NULL)
				set_menu(l->data, NULL);
			continue;
		}

		if (menu != NULL)
		{	/* clear the old menu items */
			gtk_container_foreach(GTK_CONTAINER(menu), (GtkCallback) gtk_widget_destroy, NULL);
		}
		else
		{	/* create new menu */
			menu = gtk_menu_new();
			set_menu(l->data, menu);
		}

		for (i = 0; i < bml->len; i++)
		{
			bm = g_ptr_array_index(bml, i);
			item = gtk_menu_item_new_with_label(gigolo_bookmark_get_name(bm));
			g_object_set_data(G_OBJECT(item), "bookmark", bm);
			gtk_container_add(GTK_CONTAINER(menu), item);
			gtk_widget_show(item);
			g_signal_connect(item, "activate", G_CALLBACK(delegate_item_activated), action);
		}
	}
}


static void gigolo_menu_button_action_connect_proxy(GtkAction *action, GtkWidget *widget)
{
	GTK_ACTION_CLASS(gigolo_menu_button_action_parent_class)->connect_proxy(action, widget);

	/* Overwrite the icon and label of the toolbar button */
	if (GTK_IS_TOOL_BUTTON(widget))
	{
		gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(widget), GTK_STOCK_CONNECT);
		gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Connect"));
	}
}


static void gigolo_menu_button_action_set_property(GObject *object, guint prop_id,
												 const GValue *value, GParamSpec *pspec)
{
	GigoloMenubuttonAction *action = GIGOLO_MENU_BUTTON_ACTION(object);

	switch (prop_id)
	{
	case PROP_SETTINGS:
		update_menus(action, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}


static void gigolo_menu_button_action_class_init(GigoloMenubuttonActionClass *klass)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS(klass);
	GtkActionClass *action_class = GTK_ACTION_CLASS(klass);

	g_object_class->set_property = gigolo_menu_button_action_set_property;

	action_class->connect_proxy = gigolo_menu_button_action_connect_proxy;
	action_class->create_menu_item = gigolo_menu_button_action_create_menu_item;
	action_class->create_tool_item = gigolo_menu_button_action_create_tool_item;
	action_class->menu_item_type = GTK_TYPE_IMAGE_MENU_ITEM;
	action_class->toolbar_item_type = GTK_TYPE_MENU_TOOL_BUTTON;

	g_object_class_install_property(g_object_class,
										PROP_SETTINGS,
										g_param_spec_object (
										"settings",
										"Settings",
										"The associated settings",
										GIGOLO_SETTINGS_TYPE,
										G_PARAM_WRITABLE));

	signals[ITEM_CLICKED] = g_signal_new("item-clicked",
										G_TYPE_FROM_CLASS(klass),
										(GSignalFlags) 0,
										0,
										0,
										NULL,
										g_cclosure_marshal_VOID__OBJECT,
										G_TYPE_NONE, 1, GTK_TYPE_WIDGET);
	signals[BUTTON_CLICKED] = g_signal_new("button-clicked",
										G_TYPE_FROM_CLASS(klass),
										(GSignalFlags) 0,
										0,
										0,
										NULL,
										g_cclosure_marshal_VOID__VOID,
										G_TYPE_NONE, 0);
}


static void gigolo_menu_button_action_init(G_GNUC_UNUSED GigoloMenubuttonAction *action)
{
}


GtkAction *gigolo_menu_button_action_new(const gchar *name, const gchar *label,
									   const gchar *tooltip, const gchar *icon_name)
{
	GtkAction *action = g_object_new(GIGOLO_MENU_BUTTON_ACTION_TYPE,
		"name", name, "label", label, "tooltip", tooltip, "icon-name", icon_name, NULL);

	return action;
}

