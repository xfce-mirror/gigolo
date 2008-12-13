/*
 *      passworddialog.c
 *
 *      Copyright 2008 Enrico Tröger <enrico(at)xfce(dot)org>
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
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "main.h"
#include "compat.h"
#include "passworddialog.h"

typedef struct _SionPasswordDialogPrivate			SionPasswordDialogPrivate;

#define SION_PASSWORD_DIALOG_GET_PRIVATE(obj)		(G_TYPE_INSTANCE_GET_PRIVATE((obj),\
			SION_PASSWORD_DIALOG_TYPE, SionPasswordDialogPrivate))

struct _SionPasswordDialogPrivate
{
	GtkWidget *box_domain;
	GtkWidget *box_username;
	GtkWidget *box_password;

	GtkWidget *entry_domain;
	GtkWidget *entry_username;
	GtkWidget *entry_password;
};

static void sion_password_dialog_class_init			(SionPasswordDialogClass *klass);
static void sion_password_dialog_init      			(SionPasswordDialog *dialog);

/* Local data */
static GtkDialogClass *parent_class = NULL;

GType sion_password_dialog_get_type(void)
{
	static GType self_type = 0;
	if (! self_type)
	{
		static const GTypeInfo self_info =
		{
			sizeof(SionPasswordDialogClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc)sion_password_dialog_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(SionPasswordDialog),
			0,
			(GInstanceInitFunc)sion_password_dialog_init,
			NULL /* value_table */
		};

		self_type = g_type_register_static(GTK_TYPE_DIALOG, "SionPasswordDialog", &self_info, 0);
	}

	return self_type;
}


static void sion_password_dialog_class_init(SionPasswordDialogClass *klass)
{
	parent_class = (GtkDialogClass*)g_type_class_peek(GTK_TYPE_DIALOG);
	g_type_class_add_private((gpointer)klass, sizeof(SionPasswordDialogPrivate));
}


static void entry_activate_cb(GtkEntry *entry, gpointer user_data)
{
	gtk_dialog_response(GTK_DIALOG(user_data), GTK_RESPONSE_OK);
}


static void sion_password_dialog_init(SionPasswordDialog *dialog)
{
	GtkWidget *vbox;
	GtkWidget *dialog_vbox;
	GtkWidget *label;
	GtkSizeGroup *size_group;
	SionPasswordDialogPrivate *priv = SION_PASSWORD_DIALOG_GET_PRIVATE(dialog);

	dialog_vbox = sion_dialog_get_content_area(GTK_DIALOG(dialog));

	gtk_window_set_title(GTK_WINDOW(dialog), _("Authentication information needed"));
	//~ gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
	gtk_box_set_spacing(GTK_BOX(dialog_vbox), 2);
	//~ gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

	gtk_dialog_add_buttons(GTK_DIALOG(dialog),
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		NULL);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_box_pack_start(GTK_BOX(dialog_vbox), vbox, FALSE, TRUE, 0);

	gtk_widget_show_all(GTK_WIDGET(dialog));

	size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	/* Domain */
	priv->entry_domain = gtk_entry_new();
	g_signal_connect(priv->entry_domain, "activate", G_CALLBACK(entry_activate_cb), dialog);
	label = gtk_label_new_with_mnemonic(_("_Domain:"));
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), priv->entry_domain);
	priv->box_domain = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(priv->box_domain), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(priv->box_domain), priv->entry_domain, FALSE, FALSE, 0);
	gtk_widget_show(label);
	gtk_widget_show(priv->entry_domain);
	gtk_size_group_add_widget(size_group, label);

	/* Username */
	priv->entry_username = gtk_entry_new();
	g_signal_connect(priv->entry_username, "activate", G_CALLBACK(entry_activate_cb), dialog);
	label = gtk_label_new_with_mnemonic(_("_Username:"));
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), priv->entry_username);
	priv->box_username = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(priv->box_username), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(priv->box_username), priv->entry_username, FALSE, FALSE, 0);
	gtk_widget_show(label);
	gtk_widget_show(priv->entry_username);
	gtk_size_group_add_widget(size_group, label);

	/* Password */
	priv->entry_password = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(priv->entry_password), FALSE);
	g_signal_connect(priv->entry_password, "activate", G_CALLBACK(entry_activate_cb), dialog);
	label = gtk_label_new_with_mnemonic(_("_Password:"));
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), priv->entry_password);
	priv->box_password = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(priv->box_password), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(priv->box_password), priv->entry_password, FALSE, FALSE, 0);
	gtk_widget_show(label);
	gtk_widget_show(priv->entry_password);
	gtk_size_group_add_widget(size_group, label);

	gtk_box_pack_start(GTK_BOX(vbox), priv->box_domain, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), priv->box_username, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), priv->box_password, FALSE, FALSE, 0);

	g_object_unref(size_group);
}


GtkWidget *sion_password_dialog_new(GAskPasswordFlags flags)
{
	GtkWidget *dialog = g_object_new(SION_PASSWORD_DIALOG_TYPE, NULL);
	SionPasswordDialogPrivate *priv = SION_PASSWORD_DIALOG_GET_PRIVATE(dialog);

	/** Implement G_ASK_PASSWORD_SAVING_SUPPORTED */
	if (flags & G_ASK_PASSWORD_NEED_PASSWORD)
	{
		gtk_widget_show(priv->box_password);
		gtk_widget_grab_focus(priv->entry_password);
	}
	if (flags & G_ASK_PASSWORD_NEED_USERNAME)
	{
		gtk_widget_show(priv->box_username);
		gtk_widget_grab_focus(priv->entry_username);
	}
	if (flags & G_ASK_PASSWORD_NEED_DOMAIN)
	{
		gtk_widget_show(priv->box_domain);
		gtk_widget_grab_focus(priv->entry_domain);
	}

	return dialog;
}


const gchar *sion_password_dialog_get_domain(SionPasswordDialog *dialog)
{
	g_return_val_if_fail(dialog != NULL, NULL);

	return gtk_entry_get_text(GTK_ENTRY(
		SION_PASSWORD_DIALOG_GET_PRIVATE(dialog)->entry_domain));
}


const gchar *sion_password_dialog_get_username(SionPasswordDialog *dialog)
{
	g_return_val_if_fail(dialog != NULL, NULL);

	return gtk_entry_get_text(GTK_ENTRY(
		SION_PASSWORD_DIALOG_GET_PRIVATE(dialog)->entry_username));
}


const gchar *sion_password_dialog_get_password(SionPasswordDialog *dialog)
{
	g_return_val_if_fail(dialog != NULL, NULL);

	return gtk_entry_get_text(GTK_ENTRY(
		SION_PASSWORD_DIALOG_GET_PRIVATE(dialog)->entry_password));
}

