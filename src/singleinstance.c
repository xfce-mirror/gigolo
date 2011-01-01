/*
 *      singleinstance.c
 *
 *      Copyright 2009-2011 Enrico Tröger <enrico(at)xfce(dot)org>
 *      Copyright 2006      Darren Salt
 *      Copyright 2002-2006 Olivier Fourdan
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
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */


/*
 * This is very simple detection of an already running by using XSelections.
 */

#include "config.h"

#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>

#include "singleinstance.h"
#include "common.h"
#include "compat.h"
#include "main.h"


#define GIGOLO_SI_NAME "GIGOLO_SEL"
#define GIGOLO_SI_CMD "gigolo_show_window"


enum
{
	PROP_0,

	PROP_PARENT
};


struct _GigoloSingleInstance
{
	GObject parent;
};

struct _GigoloSingleInstanceClass
{
	GObjectClass parent_class;
};

typedef struct _GigoloSingleInstancePrivate			GigoloSingleInstancePrivate;

#define GIGOLO_SINGLE_INSTANCE_GET_PRIVATE(obj)		(G_TYPE_INSTANCE_GET_PRIVATE((obj),\
			GIGOLO_SINGLE_INSTANCE_TYPE, GigoloSingleInstancePrivate))


struct _GigoloSingleInstancePrivate
{
	gboolean	 found_instance;
	Window		 id;
	GtkWidget	*window;

	GtkWindow	*parent;
};

static void gigolo_single_instance_finalize  			(GObject *object);


G_DEFINE_TYPE(GigoloSingleInstance, gigolo_single_instance, G_TYPE_OBJECT);


void gigolo_single_instance_set_parent(GigoloSingleInstance *gis, GtkWindow *parent)
{
	GigoloSingleInstancePrivate *priv;

	g_return_if_fail(gis != NULL);
	g_return_if_fail(parent != NULL);

	priv = GIGOLO_SINGLE_INSTANCE_GET_PRIVATE(gis);
	priv->parent = parent;
}


static void gigolo_single_instance_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch (prop_id)
	{
	case PROP_PARENT:
		gigolo_single_instance_set_parent(GIGOLO_SINGLE_INSTANCE(object), g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}


static void gigolo_single_instance_class_init(GigoloSingleInstanceClass *klass)
{
	GObjectClass *g_object_class;

	g_object_class = G_OBJECT_CLASS(klass);
	g_object_class->finalize = gigolo_single_instance_finalize;
	g_object_class->set_property = gigolo_single_instance_set_property;

	g_type_class_add_private(klass, sizeof(GigoloSingleInstancePrivate));

	g_object_class_install_property(g_object_class,
									PROP_PARENT,
									g_param_spec_object(
									"parent",
									"Parent",
									"The Gigolo main window which gets popped up by the remote instance.",
									GTK_TYPE_WINDOW,
									G_PARAM_WRITABLE));
}


static void gigolo_single_instance_finalize(GObject *object)
{
	GigoloSingleInstancePrivate *priv = GIGOLO_SINGLE_INSTANCE_GET_PRIVATE(object);

	if (priv->window != NULL)
		gtk_widget_destroy(priv->window);

	G_OBJECT_CLASS(gigolo_single_instance_parent_class)->finalize(object);
}


static gboolean message_received(G_GNUC_UNUSED GtkWidget *widget,
								 GdkEventClient *ev, GigoloSingleInstance *gis)
{
	if (ev->data_format == 8 && gigolo_str_equal(ev->data.b, GIGOLO_SI_CMD))
	{
		GigoloSingleInstancePrivate *priv;

		g_return_val_if_fail(gis != NULL, FALSE);

		priv = GIGOLO_SINGLE_INSTANCE_GET_PRIVATE(gis);
		gtk_window_present(priv->parent);

		return TRUE;
	}

	return FALSE;
}


static void setup_selection(GigoloSingleInstance *gis)
{
	GdkScreen *gscreen;
	gchar      selection_name[32];
	Atom       selection_atom;
	GtkWidget *win;
	Window     xwin;

	win = gtk_invisible_new();
	gtk_widget_realize(win);
	xwin = GDK_WINDOW_XID(gigolo_widget_get_window(GTK_WIDGET(win)));

	gscreen = gtk_widget_get_screen(win);
	g_snprintf(selection_name, sizeof(selection_name),
		GIGOLO_SI_NAME"%d", gdk_screen_get_number(gscreen));
	selection_atom = XInternAtom(GDK_DISPLAY(), selection_name, False);

	if (XGetSelectionOwner(GDK_DISPLAY(), selection_atom))
	{
		gtk_widget_destroy(win);
		return;
	}

	XSelectInput(GDK_DISPLAY(), xwin, PropertyChangeMask);
	XSetSelectionOwner(GDK_DISPLAY(), selection_atom, xwin, GDK_CURRENT_TIME);

	g_signal_connect(G_OBJECT(win), "client-event", G_CALLBACK(message_received), gis);
}


static void gigolo_single_instance_init(GigoloSingleInstance *self)
{
	GigoloSingleInstancePrivate *priv = GIGOLO_SINGLE_INSTANCE_GET_PRIVATE(self);

	priv->found_instance = FALSE;
	priv->window = NULL;
}


gboolean gigolo_single_instance_is_running(GigoloSingleInstance *gis)
{
	GigoloSingleInstancePrivate *priv;

	g_return_val_if_fail(gis != NULL, FALSE);

	priv = GIGOLO_SINGLE_INSTANCE_GET_PRIVATE(gis);

	return priv->found_instance;
}


static gboolean find_running_instance(GigoloSingleInstance *gis)
{
	GdkScreen *gscreen;
	gchar selection_name[32];
	Atom selection_atom;
	GigoloSingleInstancePrivate *priv;

	g_return_val_if_fail(gis != NULL, FALSE);

	priv = GIGOLO_SINGLE_INSTANCE_GET_PRIVATE(gis);
	priv->window = gtk_invisible_new();
	gtk_widget_realize(priv->window);

	gscreen = gtk_widget_get_screen(priv->window);
	g_snprintf(selection_name, sizeof(selection_name),
		GIGOLO_SI_NAME"%d", gdk_screen_get_number(gscreen));
	selection_atom = XInternAtom(GDK_DISPLAY(), selection_name, False);

	priv->id = XGetSelectionOwner(GDK_DISPLAY(), selection_atom);

	gdk_flush();

	return (priv->id != None);
}


void gigolo_single_instance_present(GigoloSingleInstance *gis)
{
	GigoloSingleInstancePrivate *priv;

	g_return_if_fail(gis != NULL);

	priv = GIGOLO_SINGLE_INSTANCE_GET_PRIVATE(gis);
	if (priv->id != None && priv->found_instance)
	{
		GdkEventClient gev;

		gev.type = GDK_CLIENT_EVENT;
		gev.window = gigolo_widget_get_window(priv->window);
		gev.send_event = TRUE;
		gev.message_type = gdk_atom_intern("STRING", FALSE);
		gev.data_format = 8;
		g_strlcpy(gev.data.b, GIGOLO_SI_CMD, sizeof(gev.data.b));

		gdk_event_send_client_message((GdkEvent*) &gev, (GdkNativeWindow) priv->id);
	}
}


/* When creating a new object of this class we search an already running instance of Gigolo
 * and in case we found one, we set the 'found_instance' member to TRUE.
 * Otherwise, we register ourselves as new single running instance. */
GigoloSingleInstance *
gigolo_single_instance_new(void)
{
	GigoloSingleInstance *gis = g_object_new(GIGOLO_SINGLE_INSTANCE_TYPE, NULL);
	GigoloSingleInstancePrivate *priv = GIGOLO_SINGLE_INSTANCE_GET_PRIVATE(gis);

	if (find_running_instance(gis))
	{
		priv->found_instance = TRUE;
	}
	else
	{
		setup_selection(gis);
	}

	return gis;
}

