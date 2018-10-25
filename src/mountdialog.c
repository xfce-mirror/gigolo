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

static void gigolo_mount_dialog_finalize			(GObject *widget);


G_DEFINE_TYPE_WITH_PRIVATE(GigoloMountDialog, gigolo_mount_dialog, GTK_TYPE_DIALOG);



static void gigolo_mount_dialog_class_init(GigoloMountDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = gigolo_mount_dialog_finalize;
}


static void gigolo_mount_dialog_finalize(GObject *widget)
{
	GigoloMountDialogPrivate *priv = gigolo_mount_dialog_get_instance_private(GIGOLO_MOUNT_DIALOG(widget));

	if (priv->timer_id != (guint) -1)
	{
		g_source_remove(priv->timer_id);
		priv->timer_id = -1;
	}

	G_OBJECT_CLASS(gigolo_mount_dialog_parent_class)->finalize(widget);
}


static gboolean do_pulse(gpointer data)
{
	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(data));

	return TRUE;
}


static void gigolo_mount_dialog_init(GigoloMountDialog *self)
{
	GtkWidget *vbox, *progress;
	GigoloMountDialogPrivate *priv = gigolo_mount_dialog_get_instance_private(self);

	priv->timer_id = (guint) -1;

	gtk_window_set_destroy_with_parent(GTK_WINDOW(self), TRUE);
	gtk_window_set_default_size(GTK_WINDOW(self), 200, -1);
	gtk_window_set_title(GTK_WINDOW(self), _("Connecting"));

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(self))), vbox);

	priv->label = gtk_label_new(NULL);
	gtk_label_set_xalign(GTK_LABEL(priv->label), 0.1);
	gtk_box_pack_start(GTK_BOX(vbox), priv->label, FALSE, FALSE, 0);

	progress = gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(vbox), progress, FALSE, FALSE, 0);

	priv->timer_id = g_timeout_add(250, do_pulse, GTK_PROGRESS_BAR(progress));
}


GtkWidget *gigolo_mount_dialog_new(GtkWindow *parent, const gchar *label)
{
	GtkWidget *dialog = g_object_new(GIGOLO_MOUNT_DIALOG_TYPE,
		"transient-for", parent,
		"icon-name", gigolo_get_application_icon_name(),
		NULL);
	GigoloMountDialogPrivate *priv = gigolo_mount_dialog_get_instance_private(GIGOLO_MOUNT_DIALOG(dialog));

	gtk_label_set_text(GTK_LABEL(priv->label), label);

	return dialog;
}


