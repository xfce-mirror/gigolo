/*
 *      menubuttonaction.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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



G_DEFINE_TYPE(GigoloMenubuttonAction, gigolo_menu_button_action, GTK_TYPE_MENU);


static void delegate_item_activated(GtkMenuItem *item, GigoloMenubuttonAction *action)
{
	g_signal_emit(action, signals[ITEM_CLICKED], 0, item);
}


static void gigolo_widget_destroy(GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy(widget);
}


static void update_menus(GigoloMenubuttonAction *menu, GigoloSettings *settings)
{
	guint i;
	GtkWidget *item;
	GigoloBookmark *bm;
	GigoloBookmarkList *bml = gigolo_settings_get_bookmarks(settings);

	gtk_container_foreach(GTK_CONTAINER(menu), gigolo_widget_destroy, NULL);

	for (i = 0; i < bml->len; i++)
	{
		bm = g_ptr_array_index(bml, i);
		item = gtk_menu_item_new_with_label(gigolo_bookmark_get_name(bm));
		g_object_set_data(G_OBJECT(item), "bookmark", bm);
		gtk_container_add(GTK_CONTAINER(menu), item);
		gtk_widget_show(item);
		g_signal_connect(item, "activate", G_CALLBACK(delegate_item_activated), menu);
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

	g_object_class->set_property = gigolo_menu_button_action_set_property;

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


GtkMenu *gigolo_menu_button_action_new(const gchar *name)
{
	GtkMenu *menu = g_object_new(GIGOLO_MENU_BUTTON_ACTION_TYPE,
							     "name", name,
								 NULL);

	return menu;
}

