/*
 *      mountoperation.c
 *
 *      Copyright 2009-2010 Enrico Tröger <enrico(at)xfce(dot)org>
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

#include <config.h>

#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <string.h>

#include "compat.h"
#include "mountoperation.h"


/* This is a "light" version of GtkMountOperation for the case when Gigolo is compiled
 * against GTK 2.12. When compild against GTK 2.14 or later, the native GTK dialog is used.
 * Changes: removed the properties which we don't use and adjust coding style.
 * Strings are not marked as translatable on purpose to ease the life of translators because
 * the whole code in here is only used on GTK 2.12, GTK 2.14 and later provide the same dialog
 * with translated strings.
 */

#if ! GTK_CHECK_VERSION(2, 14, 0)

typedef struct _GigoloMountOperationPrivate			GigoloMountOperationPrivate;

#define GIGOLO_MOUNT_OPERATION_GET_PRIVATE(obj)		(G_TYPE_INSTANCE_GET_PRIVATE((obj),\
			GIGOLO_MOUNT_OPERATION_TYPE, GigoloMountOperationPrivate))

struct _GigoloMountOperation
{
	GMountOperation parent;
};

struct _GigoloMountOperationClass
{
	GMountOperationClass parent_class;
};

struct _GigoloMountOperationPrivate
{
	GtkWindow *parent_window;
	GtkDialog *dialog;

	/* for the ask-password dialog */
	GtkWidget *entry_container;
	GtkWidget *username_entry;
	GtkWidget *domain_entry;
	GtkWidget *password_entry;
	GtkWidget *anonymous_toggle;

	GAskPasswordFlags ask_flags;
	GPasswordSave     password_save;
	gboolean          anonymous;
};


static void gigolo_mount_operation_ask_password(GMountOperation *op,
                                                const gchar *message,
                                                const gchar *default_user,
                                                const gchar *default_domain,
                                                GAskPasswordFlags flags);
static void gigolo_mount_operation_ask_question(GMountOperation *op,
                                                const gchar *message,
												const gchar *choices[]);
#if GLIB_CHECK_VERSION(2, 20, 0)
static void gigolo_mount_operation_aborted(GMountOperation *op);
#endif


G_DEFINE_TYPE(GigoloMountOperation, gigolo_mount_operation, G_TYPE_MOUNT_OPERATION);


static void gigolo_mount_operation_finalize(GObject *object)
{
	GigoloMountOperation *self;

	g_return_if_fail(object != NULL);
	g_return_if_fail(IS_GIGOLO_MOUNT_OPERATION(object));

	self = GIGOLO_MOUNT_OPERATION(object);

	G_OBJECT_CLASS(gigolo_mount_operation_parent_class)->finalize(object);
}


static void gigolo_mount_operation_init(G_GNUC_UNUSED GigoloMountOperation *self)
{
	/* nothing to do */
}


static void remember_button_toggled(GtkToggleButton *button, GigoloMountOperation *operation)
{
	if (gtk_toggle_button_get_active(button))
	{
		gpointer data;
		GigoloMountOperationPrivate *priv = GIGOLO_MOUNT_OPERATION_GET_PRIVATE(operation);

		data = g_object_get_data(G_OBJECT(button), "password-save");
		priv->password_save = GPOINTER_TO_INT(data);
	}
}


static void pw_dialog_got_response(GtkDialog *dialog, gint response_id, GigoloMountOperation *g_op)
{
	GigoloMountOperationPrivate *priv = GIGOLO_MOUNT_OPERATION_GET_PRIVATE(g_op);
	GMountOperation *op = G_MOUNT_OPERATION(g_op);

	if (response_id == GTK_RESPONSE_OK)
	{
		const char *text;

		if (priv->ask_flags & G_ASK_PASSWORD_ANONYMOUS_SUPPORTED)
			g_mount_operation_set_anonymous(op, priv->anonymous);

		if (priv->username_entry)
		{
			text = gtk_entry_get_text(GTK_ENTRY(priv->username_entry));
			g_mount_operation_set_username(op, text);
		}

		if (priv->domain_entry)
		{
			text = gtk_entry_get_text(GTK_ENTRY(priv->domain_entry));
			g_mount_operation_set_domain(op, text);
		}

		if (priv->password_entry)
		{
			text = gtk_entry_get_text(GTK_ENTRY(priv->password_entry));
			g_mount_operation_set_password(op, text);
		}

		if (priv->ask_flags & G_ASK_PASSWORD_SAVING_SUPPORTED)
			g_mount_operation_set_password_save(op, priv->password_save);

		g_mount_operation_reply(op, G_MOUNT_OPERATION_HANDLED);
	}
	else
		g_mount_operation_reply(op, G_MOUNT_OPERATION_ABORTED);

	priv->dialog = NULL;
	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_object_unref(op);
}


static gboolean entry_has_input(GtkWidget *entry_widget)
{
	const gchar *text;

	if (entry_widget == NULL)
		return TRUE;

	text = gtk_entry_get_text(GTK_ENTRY(entry_widget));

	return text != NULL && text[0] != '\0';
}


static gboolean pw_dialog_input_is_valid(GigoloMountOperation *operation)
{
	GigoloMountOperationPrivate *priv = GIGOLO_MOUNT_OPERATION_GET_PRIVATE(operation);
	gboolean is_valid = TRUE;

	/* We don't require password to be non-empty here
	* since there are situations where it is not needed,
	* see bug 578365.
	* We may add a way for the backend to specify that it
	* definitively needs a password.
	*/
	is_valid = entry_has_input(priv->username_entry) && entry_has_input(priv->domain_entry);

	return is_valid;
}


static void pw_dialog_verify_input(G_GNUC_UNUSED GtkEditable *editable, GigoloMountOperation *op)
{
	GigoloMountOperationPrivate *priv = GIGOLO_MOUNT_OPERATION_GET_PRIVATE(op);
	gboolean is_valid;

	is_valid = pw_dialog_input_is_valid(op);
	gtk_dialog_set_response_sensitive(GTK_DIALOG(priv->dialog), GTK_RESPONSE_OK, is_valid);
}


static void pw_dialog_anonymous_toggled(GtkWidget *widget, GigoloMountOperation *operation)
{
	GigoloMountOperationPrivate *priv = GIGOLO_MOUNT_OPERATION_GET_PRIVATE(operation);
	gboolean is_valid;

	priv->anonymous = (widget == priv->anonymous_toggle);

	if (priv->anonymous)
		is_valid = TRUE;
	else
		is_valid = pw_dialog_input_is_valid(operation);

	gtk_widget_set_sensitive(priv->entry_container, (priv->anonymous == FALSE));
	gtk_dialog_set_response_sensitive(GTK_DIALOG(priv->dialog), GTK_RESPONSE_OK, is_valid);
}


static void pw_dialog_cycle_focus(GtkWidget *widget, GigoloMountOperation *operation)
{
	GigoloMountOperationPrivate *priv = GIGOLO_MOUNT_OPERATION_GET_PRIVATE(operation);
	GtkWidget *next_widget = NULL;

	priv = GIGOLO_MOUNT_OPERATION_GET_PRIVATE(operation);

	if (widget == priv->username_entry)
	{
		if (priv->domain_entry != NULL)
			next_widget = priv->domain_entry;
		else if (priv->password_entry != NULL)
			next_widget = priv->password_entry;
	}
	else if (widget == priv->domain_entry && priv->password_entry)
		next_widget = priv->password_entry;

	if (next_widget)
		gtk_widget_grab_focus(next_widget);
	else if (pw_dialog_input_is_valid(operation))
		gtk_window_activate_default(GTK_WINDOW(priv->dialog));
}


static GtkWidget *table_add_entry(GtkWidget *table, gint row, const gchar *label_text,
								  const gchar *value, gpointer user_data)
{
	GtkWidget *entry;
	GtkWidget *label;

	label = gtk_label_new_with_mnemonic(label_text);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

	entry = gtk_entry_new();

	if (value)
		gtk_entry_set_text(GTK_ENTRY(entry), value);

	gtk_table_attach(GTK_TABLE(table), label, 0, 1, row, row + 1, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, row, row + 1);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), entry);

	g_signal_connect(entry, "changed", G_CALLBACK(pw_dialog_verify_input), user_data);
	g_signal_connect(entry, "activate", G_CALLBACK(pw_dialog_cycle_focus), user_data);

	return entry;
}


static void gigolo_mount_operation_ask_password(GMountOperation *mount_op,
												const gchar *message,
												const gchar *default_user,
												const gchar *default_domain,
												GAskPasswordFlags flags)
{
	GigoloMountOperation *operation;
	GigoloMountOperationPrivate *priv;
	GtkWidget *widget;
	GtkDialog *dialog;
	GtkWindow *window;
	GtkWidget *entry_container;
	GtkWidget *hbox, *main_vbox, *vbox, *icon;
	GtkWidget *table;
	GtkWidget *message_label;
	gboolean   can_anonymous;
	guint      rows;
	const gchar *secondary;

	operation = GIGOLO_MOUNT_OPERATION(mount_op);
	priv = GIGOLO_MOUNT_OPERATION_GET_PRIVATE(operation);

	priv->ask_flags = flags;

	widget = gtk_dialog_new();
	dialog = GTK_DIALOG(widget);
	window = GTK_WINDOW(widget);

	priv->dialog = dialog;

	/* Set the dialog up with HIG properties */
	gtk_dialog_set_has_separator(dialog, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
	gtk_box_set_spacing(GTK_BOX(gigolo_dialog_get_content_area(dialog)), 2);
	/* 2 * 5 + 2 = 12 */
	gtk_container_set_border_width(GTK_CONTAINER(gigolo_dialog_get_action_area(dialog)), 5);
	gtk_box_set_spacing(GTK_BOX(gigolo_dialog_get_action_area(dialog)), 6);

	gtk_window_set_resizable(window, FALSE);
	gtk_window_set_title(window, "");
	gtk_window_set_icon_name(window, GTK_STOCK_DIALOG_AUTHENTICATION);

	gtk_dialog_add_buttons(dialog, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		"Co_nnect", GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(dialog, GTK_RESPONSE_OK);

	gtk_dialog_set_alternative_button_order(dialog, GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	/* Build contents */
	hbox = gtk_hbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	gtk_box_pack_start(GTK_BOX(gigolo_dialog_get_content_area(dialog)), hbox, TRUE, TRUE, 0);

	icon = gtk_image_new_from_stock(GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_DIALOG);

	gtk_misc_set_alignment(GTK_MISC(icon), 0.5, 0.0);
	gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);

	main_vbox = gtk_vbox_new(FALSE, 18);
	gtk_box_pack_start(GTK_BOX(hbox), main_vbox, TRUE, TRUE, 0);

	secondary = strstr(message, "\n");
	if (secondary != NULL)
	{
		gchar *s;
		gchar *primary;

		primary = g_strndup(message, secondary - message + 1);
		s = g_strdup_printf("<big><b>%s</b></big>%s", primary, secondary);

		message_label = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(message_label), s);
		gtk_misc_set_alignment(GTK_MISC(message_label), 0.0, 0.5);
		gtk_label_set_line_wrap(GTK_LABEL(message_label), TRUE);
		gtk_box_pack_start(GTK_BOX(main_vbox), GTK_WIDGET(message_label), FALSE, TRUE, 0);

		g_free(s);
		g_free(primary);
	}
	else
	{
		message_label = gtk_label_new(message);
		gtk_misc_set_alignment(GTK_MISC(message_label), 0.0, 0.5);
		gtk_label_set_line_wrap(GTK_LABEL(message_label), TRUE);
		gtk_box_pack_start(GTK_BOX(main_vbox), GTK_WIDGET(message_label), FALSE, FALSE, 0);
	}

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(main_vbox), vbox, FALSE, FALSE, 0);

	can_anonymous = flags & G_ASK_PASSWORD_ANONYMOUS_SUPPORTED;

	priv->anonymous_toggle = NULL;
	if (can_anonymous)
	{
		GtkWidget *anon_box;
		GtkWidget *choice;
		GSList    *group;

		anon_box = gtk_vbox_new(FALSE, 6);
		gtk_box_pack_start(GTK_BOX(vbox), anon_box, FALSE, FALSE, 0);

		choice = gtk_radio_button_new_with_mnemonic(NULL, "Connect _anonymously");
		gtk_box_pack_start(GTK_BOX(anon_box), choice, FALSE, FALSE, 0);
		g_signal_connect(choice, "toggled", G_CALLBACK(pw_dialog_anonymous_toggled), operation);
		priv->anonymous_toggle = choice;

		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(choice));
		choice = gtk_radio_button_new_with_mnemonic(group, "Connect as u_ser:");
		gtk_box_pack_start(GTK_BOX(anon_box), choice, FALSE, FALSE, 0);
		g_signal_connect(choice, "toggled", G_CALLBACK(pw_dialog_anonymous_toggled), operation);
	}

	rows = 0;

	if (flags & G_ASK_PASSWORD_NEED_PASSWORD)
		rows++;

	if (flags & G_ASK_PASSWORD_NEED_USERNAME)
		rows++;

	if (flags &G_ASK_PASSWORD_NEED_DOMAIN)
		rows++;

	/* The table that holds the entries */
	entry_container = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);

	gtk_alignment_set_padding(GTK_ALIGNMENT(entry_container), 0, 0, can_anonymous ? 12 : 0, 0);

	gtk_box_pack_start(GTK_BOX(vbox), entry_container, FALSE, FALSE, 0);
	priv->entry_container = entry_container;

	table = gtk_table_new(rows, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_container_add(GTK_CONTAINER(entry_container), table);

	rows = 0;

	priv->username_entry = NULL;
	if (flags & G_ASK_PASSWORD_NEED_USERNAME)
		priv->username_entry = table_add_entry(table, rows++, "_Username:", default_user, operation);

	priv->domain_entry = NULL;
	if (flags & G_ASK_PASSWORD_NEED_DOMAIN)
		priv->domain_entry = table_add_entry(table, rows++, "_Domain:", default_domain, operation);

	priv->password_entry = NULL;
	if (flags & G_ASK_PASSWORD_NEED_PASSWORD)
	{
		priv->password_entry = table_add_entry(table, rows++, "_Password:", NULL, operation);
		gtk_entry_set_visibility(GTK_ENTRY(priv->password_entry), FALSE);
	}

	if (flags & G_ASK_PASSWORD_SAVING_SUPPORTED)
	{
		GtkWidget    *choice;
		GtkWidget    *remember_box;
		GSList       *group;
		GPasswordSave password_save;

		remember_box = gtk_vbox_new(FALSE, 6);
		gtk_box_pack_start(GTK_BOX(vbox), remember_box,	FALSE, FALSE, 0);

		password_save = g_mount_operation_get_password_save(mount_op);

		choice = gtk_radio_button_new_with_mnemonic(NULL, "Forget password _immediately");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(choice),	password_save == G_PASSWORD_SAVE_NEVER);
		g_object_set_data(G_OBJECT(choice), "password-save", GINT_TO_POINTER(G_PASSWORD_SAVE_NEVER));
		g_signal_connect(choice, "toggled",	G_CALLBACK(remember_button_toggled), operation);
		gtk_box_pack_start(GTK_BOX(remember_box), choice, FALSE, FALSE, 0);

		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(choice));
		choice = gtk_radio_button_new_with_mnemonic(group, "Remember password until you _logout");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(choice),	password_save == G_PASSWORD_SAVE_FOR_SESSION);
		g_object_set_data(G_OBJECT(choice), "password-save", GINT_TO_POINTER(G_PASSWORD_SAVE_FOR_SESSION));
		g_signal_connect(choice, "toggled", G_CALLBACK(remember_button_toggled), operation);
		gtk_box_pack_start(GTK_BOX(remember_box), choice, FALSE, FALSE, 0);

		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(choice));
		choice = gtk_radio_button_new_with_mnemonic(group, "Remember _forever");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(choice), password_save == G_PASSWORD_SAVE_PERMANENTLY);
		g_object_set_data(G_OBJECT(choice), "password-save", GINT_TO_POINTER(G_PASSWORD_SAVE_PERMANENTLY));
		g_signal_connect(choice, "toggled",	G_CALLBACK(remember_button_toggled), operation);
		gtk_box_pack_start(GTK_BOX(remember_box), choice, FALSE, FALSE, 0);
	}

	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(pw_dialog_got_response), operation);

	if (can_anonymous)
	{
		/* The anonymous option will be active by default,
		* ensure the toggled signal is emitted for it.
		*/
		gtk_toggle_button_toggled(GTK_TOGGLE_BUTTON(priv->anonymous_toggle));
	}
	else if (! pw_dialog_input_is_valid(operation))
		gtk_dialog_set_response_sensitive(dialog, GTK_RESPONSE_OK, FALSE);

	if (priv->parent_window)
	{
		gtk_window_set_transient_for(window, priv->parent_window);
		gtk_window_set_modal(window, TRUE);
	}

	gtk_widget_show_all(GTK_WIDGET(dialog));
	g_object_ref(operation);
}


static void question_dialog_button_clicked(GtkDialog *dialog, gint button_number, GMountOperation *op)
{
	GigoloMountOperationPrivate *priv;
	GigoloMountOperation *operation;

	operation = GIGOLO_MOUNT_OPERATION(op);
	priv = GIGOLO_MOUNT_OPERATION_GET_PRIVATE(priv);

	if (button_number >= 0)
	{
		g_mount_operation_set_choice(op, button_number);
		g_mount_operation_reply(op, G_MOUNT_OPERATION_HANDLED);
	}
	else
		g_mount_operation_reply(op, G_MOUNT_OPERATION_ABORTED);

	priv->dialog = NULL;
	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_object_unref(op);
}


static void gigolo_mount_operation_ask_question(GMountOperation *op, const gchar *message,
												const gchar *choices[])
{
	GigoloMountOperationPrivate *priv;
	GtkWidget *dialog;
	const gchar *secondary = NULL;
	gchar *primary;
	gint count, len = 0;

	g_return_if_fail(IS_GIGOLO_MOUNT_OPERATION(op));
	g_return_if_fail(message != NULL);
	g_return_if_fail(choices != NULL);

	priv = GIGOLO_MOUNT_OPERATION_GET_PRIVATE(priv);

	primary = strstr(message, "\n");
	if (primary)
	{
		secondary = primary + 1;
		primary = g_strndup(message, primary - message);
	}

	dialog = gtk_message_dialog_new(priv->parent_window, 0,
								   GTK_MESSAGE_QUESTION,
								   GTK_BUTTONS_NONE, "%s",
								   primary != NULL ? primary : message);
	g_free(primary);

	if (secondary)
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", secondary);

	/* First count the items in the list then add the buttons in reverse order */
	while (choices[len] != NULL)
		len++;

	for (count = len - 1; count >= 0; count--)
		gtk_dialog_add_button(GTK_DIALOG(dialog), choices[count], count);

	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(question_dialog_button_clicked), op);

	priv->dialog = GTK_DIALOG(dialog);

	gtk_widget_show(dialog);
	g_object_ref(op);
}


#if GLIB_CHECK_VERSION(2, 20, 0)
static void gigolo_mount_operation_aborted(GMountOperation *op)
{
	GigoloMountOperationPrivate *priv = GIGOLO_MOUNT_OPERATION_GET_PRIVATE(priv);

	if (priv->dialog != NULL)
	{
		gtk_widget_destroy(GTK_WIDGET(priv->dialog));
		priv->dialog = NULL;
		g_object_unref(op);
	}
}
#endif


static void gigolo_mount_operation_set_parent(GigoloMountOperation *op, GtkWindow *parent)
{
	GigoloMountOperationPrivate *priv;

	g_return_if_fail(IS_GIGOLO_MOUNT_OPERATION(op));
	g_return_if_fail(parent == NULL || GTK_IS_WINDOW(parent));

	priv = GIGOLO_MOUNT_OPERATION_GET_PRIVATE(op);

	if (priv->parent_window == parent)
		return;

	if (priv->parent_window)
	{
		g_signal_handlers_disconnect_by_func(priv->parent_window,
			gtk_widget_destroyed, &priv->parent_window);
		priv->parent_window = NULL;
	}

	if (parent)
	{
		priv->parent_window = g_object_ref(parent);

		g_signal_connect(parent, "destroy", G_CALLBACK(gtk_widget_destroyed), &priv->parent_window);
		if (priv->dialog)
			gtk_window_set_transient_for(GTK_WINDOW(priv->dialog), parent);
	}
}


static void gigolo_mount_operation_class_init(GigoloMountOperationClass *klass)
{
	GObjectClass *g_object_class;
	GMountOperationClass *mount_op_class = G_MOUNT_OPERATION_CLASS(klass);

	g_object_class = G_OBJECT_CLASS(klass);

	g_object_class->finalize = gigolo_mount_operation_finalize;

	g_type_class_add_private(klass, sizeof(GigoloMountOperationPrivate));

	mount_op_class->ask_password = gigolo_mount_operation_ask_password;
	mount_op_class->ask_question = gigolo_mount_operation_ask_question;
#if GLIB_CHECK_VERSION(2, 20, 0)
	mount_op_class->aborted = gigolo_mount_operation_aborted;
#endif
}
#endif


GMountOperation *gigolo_mount_operation_new(GtkWindow *parent)
{
#if GTK_CHECK_VERSION(2, 14, 0)
	return gtk_mount_operation_new(parent);
#else
	GMountOperation *op;

	op = g_object_new(GIGOLO_MOUNT_OPERATION_TYPE, NULL);
	gigolo_mount_operation_set_parent(GIGOLO_MOUNT_OPERATION(op), parent);

	return op;
#endif
}


