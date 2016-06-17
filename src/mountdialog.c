/*
 *      mountdialog.c
 *
 *      Copyright 2009-2011 Enrico Tröger <enrico(at)xfce(dot)org>
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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "common.h"
#include "main.h"
#include "mountdialog.h"


typedef struct _GigoloMountDialogPrivate			GigoloMountDialogPrivate;

#define GIGOLO_MOUNT_DIALOG_GET_PRIVATE(obj)		(G_TYPE_INSTANCE_GET_PRIVATE((obj),\
			GIGOLO_MOUNT_DIALOG_TYPE, GigoloMountDialogPrivate))

struct _GigoloMountDialog
{
	GtkDialog parent;
};

struct _GigoloMountDialogClass
{
	GtkDialogClass parent_class;
};

struct _GigoloMountDialogPrivate
{
	GtkWidget *label;
	guint timer_id;
};

static void gigolo_mount_dialog_destroy				(GtkObject *widget);


G_DEFINE_TYPE(GigoloMountDialog, gigolo_mount_dialog, GTK_TYPE_DIALOG);



static void gigolo_mount_dialog_class_init(GigoloMountDialogClass *klass)
{
	GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);

	object_class->destroy = gigolo_mount_dialog_destroy;

	g_type_class_add_private(klass, sizeof(GigoloMountDialogPrivate));
}


static void gigolo_mount_dialog_destroy(GtkObject *widget)
{
	GigoloMountDialogPrivate *priv = GIGOLO_MOUNT_DIALOG_GET_PRIVATE(widget);

	if (priv->timer_id != (guint) -1)
	{
		g_source_remove(priv->timer_id);
		priv->timer_id = -1;
	}

	GTK_OBJECT_CLASS(gigolo_mount_dialog_parent_class)->destroy(widget);
}


static gboolean do_pulse(gpointer data)
{
	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(data));

	return TRUE;
}


static void gigolo_mount_dialog_init(GigoloMountDialog *self)
{
	GtkWidget *vbox, *progress;
	GigoloMountDialogPrivate *priv = GIGOLO_MOUNT_DIALOG_GET_PRIVATE(self);

	priv->timer_id = (guint) -1;

	gtk_dialog_set_has_separator(GTK_DIALOG(self), FALSE);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(self), TRUE);
	gtk_window_set_default_size(GTK_WINDOW(self), 200, -1);
	gtk_window_set_title(GTK_WINDOW(self), _("Connecting"));

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(self))), vbox);

	priv->label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(priv->label), 0.1, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), priv->label, FALSE, FALSE, 6);

	progress = gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(vbox), progress, FALSE, FALSE, 6);

	priv->timer_id = g_timeout_add(250, do_pulse, GTK_PROGRESS_BAR(progress));
}


GtkWidget *gigolo_mount_dialog_new(GtkWindow *parent, const gchar *label)
{
	GtkWidget *dialog = g_object_new(GIGOLO_MOUNT_DIALOG_TYPE,
		"transient-for", parent,
		"icon-name", gigolo_get_application_icon_name(),
		NULL);
	GigoloMountDialogPrivate *priv = GIGOLO_MOUNT_DIALOG_GET_PRIVATE(dialog);

	gtk_label_set_text(GTK_LABEL(priv->label), label);

	return dialog;
}


