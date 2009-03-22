/*
 *      bookmarkeditdialog.c
 *
 *      Copyright 2008-2009 Enrico Tröger <enrico(at)xfce(dot)org>
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

/* The code is mainly based on Nautilus' 'Connect to server' dialog, thanks */

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "common.h"
#include "compat.h"
#include "settings.h"
#include "bookmark.h"
#include "window.h"
#include "backendgvfs.h"
#include "bookmarkeditdialog.h"


typedef struct _GigoloBookmarkEditDialogPrivate			GigoloBookmarkEditDialogPrivate;

#define GIGOLO_BOOKMARK_EDIT_DIALOG_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE((obj),\
			GIGOLO_BOOKMARK_EDIT_DIALOG_TYPE, GigoloBookmarkEditDialogPrivate))

struct _GigoloBookmarkEditDialogPrivate
{
	GtkWindow *parent;

	GtkWidget *table;

	GtkWidget *type_combo;
	GtkWidget *information_label;

	GtkWidget *name_label;
	GtkWidget *name_entry;

	GtkWidget *autoconnect_label;
	GtkWidget *autoconnect_checkbtn;

	GtkWidget *uri_label;
	GtkWidget *uri_entry;

	GtkWidget *host_label;
	GtkWidget *host_entry;

	GtkWidget *port_label;
	GtkWidget *port_spin;

	GtkWidget *user_label;
	GtkWidget *user_entry;

	GtkWidget *domain_label;
	GtkWidget *domain_entry;

	GtkWidget *share_label;
	GtkWidget *share_combo;
	GtkWidget *share_button;
	GtkWidget *share_entry;

	GigoloBookmark *bookmark_init;
	GigoloBookmark *bookmark_update;
};

static void gigolo_bookmark_edit_dialog_set_property		(GObject *object, guint prop_id,
															 const GValue *value, GParamSpec *pspec);


struct MethodInfo {
	const gchar *scheme;
	guint port;
	guint flags;
};

enum
{
    PROP_0,
    PROP_MODE,
    PROP_BOOKMARK_INIT,
    PROP_BOOKMARK_UPDATE
};

enum {
	/* Widgets to display in setup_for_type */
	SHOW_SHARE     = 0x00000010,
	SHOW_PORT      = 0x00000020,
	SHOW_USER      = 0x00000040,
	SHOW_DOMAIN    = 0x00000080,
	SHOW_DEVICE    = 0x00000100
};

enum {
	COLUMN_INDEX,
	COLUMN_VISIBLE,
	COLUMN_DESC
};

/* this enum must be in sync with the 'methods' array below */
enum {
	SCHEME_FTP,
	SCHEME_SFTP,
	SCHEME_SMB,
	SCHEME_DAV,
	SCHEME_DAVS,
	SCHEME_OBEX,
	SCHEME_CUSTOM
};

static struct MethodInfo methods[] = {
	{ "ftp",  21,	SHOW_PORT | SHOW_USER },
	{ "sftp", 22,	SHOW_PORT | SHOW_USER },
	{ "smb",  0,	SHOW_SHARE | SHOW_USER | SHOW_DOMAIN },
	{ "dav",  80,	SHOW_PORT | SHOW_USER },
	{ "davs", 443,	SHOW_PORT | SHOW_USER },
	{ "obex", 0,	SHOW_DEVICE },
	{ NULL,   0,	0 }
};
static guint methods_len = G_N_ELEMENTS(methods);



G_DEFINE_TYPE(GigoloBookmarkEditDialog, gigolo_bookmark_edit_dialog, GTK_TYPE_DIALOG);



static void gigolo_bookmark_edit_dialog_destroy(GtkObject *object)
{
	GigoloBookmarkEditDialogPrivate *priv = GIGOLO_BOOKMARK_EDIT_DIALOG_GET_PRIVATE(object);

	gtk_widget_destroy(priv->uri_entry);
	gtk_widget_destroy(priv->uri_label);
	gtk_widget_destroy(priv->host_entry);
	gtk_widget_destroy(priv->host_label);
	gtk_widget_destroy(priv->port_label);
	gtk_widget_destroy(priv->port_spin);
	gtk_widget_destroy(priv->user_entry);
	gtk_widget_destroy(priv->user_label);
	gtk_widget_destroy(priv->domain_entry);
	gtk_widget_destroy(priv->domain_label);
	gtk_widget_destroy(priv->share_combo);
	gtk_widget_destroy(priv->share_button);
	gtk_widget_destroy(priv->share_label);
	gtk_widget_destroy(priv->information_label);

	GTK_OBJECT_CLASS(gigolo_bookmark_edit_dialog_parent_class)->destroy(object);
}


gint gigolo_bookmark_edit_dialog_run(GigoloBookmarkEditDialog *dialog)
{
	gint res;
	gboolean error = FALSE;
	GigoloBookmarkEditDialogPrivate *priv = GIGOLO_BOOKMARK_EDIT_DIALOG_GET_PRIVATE(dialog);
	const gchar *tmp;

	while (TRUE)
	{
		error = FALSE;
		res = gtk_dialog_run(GTK_DIALOG(dialog));

		if (res != GTK_RESPONSE_OK)
			break;
		/* perform some error checking and don't return until entered values are sane */
		else
		{
			if (gigolo_widget_get_flags(priv->name_entry) & GTK_VISIBLE)
			{	/* check the name only if we are creating/editing a bookmark */
				tmp = gtk_entry_get_text(GTK_ENTRY(priv->name_entry));
				if (! *tmp)
				{
					error = TRUE;
					gigolo_message_dialog((gpointer)dialog, GTK_MESSAGE_ERROR, _("Error"),
						_("You must enter a name for the bookmark."), NULL);
					gtk_widget_grab_focus(priv->name_entry);
				}
				else
				{	/* check for duplicate bookmark names */
					GigoloSettings *settings = gigolo_window_get_settings(GIGOLO_WINDOW(priv->parent));
					GigoloBookmarkList *bml = gigolo_settings_get_bookmarks(settings);
					GigoloBookmark *bm;
					guint i;

					for (i = 0; i < bml->len && ! error; i++)
					{
						bm = g_ptr_array_index(bml, i);
						if (bm == priv->bookmark_init)
							continue;
						if (gigolo_str_equal(tmp, gigolo_bookmark_get_name(bm)))
						{
							error = TRUE;
							gigolo_message_dialog((gpointer)dialog, GTK_MESSAGE_ERROR, _("Error"),
			_("The entered bookmark name is already in use. Please choose another one."), NULL);
							gtk_widget_grab_focus(priv->name_entry);
						}
					}
				}
			}
			if (! error && gtk_widget_get_parent(priv->host_entry) != NULL)
			{
				tmp = gtk_entry_get_text(GTK_ENTRY(priv->host_entry));
				if (! *tmp)
				{
					error = TRUE;
					gigolo_message_dialog((gpointer)dialog, GTK_MESSAGE_ERROR, _("Error"),
						_("You must enter a server address or name."), NULL);
					gtk_widget_grab_focus(priv->host_entry);
				}
			}
			if (! error && gtk_widget_get_parent(priv->share_combo) != NULL)
			{
				tmp = gtk_entry_get_text(GTK_ENTRY(priv->share_entry));
				if (! *tmp)
				{
					error = TRUE;
					gigolo_message_dialog((gpointer)dialog, GTK_MESSAGE_ERROR, _("Error"),
						_("You must enter a share name."), NULL);
					gtk_widget_grab_focus(priv->share_combo);
				}
			}
			if (! error && gtk_widget_get_parent(priv->uri_entry) != NULL)
			{
				tmp = gtk_entry_get_text(GTK_ENTRY(priv->uri_entry));
				if (! *tmp)
				{
					error = TRUE;
					gigolo_message_dialog((gpointer)dialog, GTK_MESSAGE_ERROR, _("Error"),
						_("You must enter a valid URI for the connection."), NULL);
					gtk_widget_grab_focus(priv->uri_entry);
				}
			}
			if (! error)
				break;
		}
	}

	return res;
}


static void gigolo_bookmark_edit_dialog_class_init(GigoloBookmarkEditDialogClass *klass)
{
	GtkObjectClass *gtk_object_class = (GtkObjectClass *) klass;
	GObjectClass *g_object_class = G_OBJECT_CLASS(klass);

	gtk_object_class->destroy = gigolo_bookmark_edit_dialog_destroy;

	g_object_class->set_property = gigolo_bookmark_edit_dialog_set_property;

	g_type_class_add_private(klass, sizeof(GigoloBookmarkEditDialogPrivate));

	g_object_class_install_property(g_object_class,
									PROP_MODE,
									g_param_spec_int(
									"mode",
									"Mode",
									"Operation mode",
									0, G_MAXINT, GIGOLO_BE_MODE_CREATE,
									G_PARAM_WRITABLE));
	g_object_class_install_property(g_object_class,
									PROP_BOOKMARK_INIT,
									g_param_spec_object(
									"bookmark-init",
									"Bookmark-init",
									"Bookmark instance to provide default values",
									GIGOLO_BOOKMARK_TYPE,
									G_PARAM_WRITABLE));
	g_object_class_install_property(g_object_class,
									PROP_BOOKMARK_UPDATE,
									g_param_spec_object(
									"bookmark-update",
									"Bookmark-update",
									"Bookmark instance",
									GIGOLO_BOOKMARK_TYPE,
									G_PARAM_WRITABLE));
}


static guint scheme_to_index(const gchar *scheme)
{
	guint i;

	for (i = 0; i < methods_len; i++)
	{
		if (gigolo_str_equal(scheme, methods[i].scheme))
		{
			return i;
		}
	}
	/* if no matching scheme was found, fall back to the Custom method */
	return methods_len - 1;
}


static gboolean combo_foreach(GtkTreeModel *model, G_GNUC_UNUSED GtkTreePath *path,
							  GtkTreeIter *iter, gpointer data)
{
	guint idx = GPOINTER_TO_UINT(data);
	guint i;

	gtk_tree_model_get(model, iter, COLUMN_INDEX, &i, -1);

	if (i == idx)
	{
		GObject *combo = g_object_get_data(G_OBJECT(model), "combobox");
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo), iter);

		return TRUE;
	}
	return FALSE;
}


static void combo_set_active(GtkWidget *combo, guint idx)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
	gboolean visible = FALSE;
	gboolean is_valid;

	/* Don't do anything if the initial scheme has been set already. */
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter))
		return;

	gtk_tree_model_foreach(model, combo_foreach, GUINT_TO_POINTER(idx));

	/* Prevent setting an invisible/unsupported scheme, fallback to Custom Location. */
	is_valid = gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter);
	if (is_valid)
		gtk_tree_model_get(model, &iter, COLUMN_VISIBLE, &visible, -1);
	if (! visible || ! is_valid)
		gtk_tree_model_foreach(model, combo_foreach, GUINT_TO_POINTER(SCHEME_CUSTOM));
}


static void init_values(GigoloBookmarkEditDialog *dialog)
{
	GigoloBookmarkEditDialogPrivate *priv = GIGOLO_BOOKMARK_EDIT_DIALOG_GET_PRIVATE(dialog);
	gchar *uri, *user;
	const gchar *tmp;
	guint port;
	guint idx;

	/* Name */
	tmp = gigolo_bookmark_get_name(priv->bookmark_init);
	if (tmp != NULL)
		gtk_entry_set_text(GTK_ENTRY(priv->name_entry), tmp);
	/* URI */
	uri = gigolo_bookmark_get_uri(priv->bookmark_init);
	if (uri != NULL)
	{
		gtk_entry_set_text(GTK_ENTRY(priv->uri_entry), uri);
		g_free(uri);
	}
	/* Host */
	tmp = gigolo_bookmark_get_host(priv->bookmark_init);
	if (tmp != NULL)
	{
		gchar *host;
		if (tmp[0] == '[' && gigolo_str_equal("obex", gigolo_bookmark_get_scheme(priv->bookmark_init)))
		{
			gsize len = strlen(tmp);
			/* tmp is something like [00:00:00:00:00] and we want to strip the brackets */
			host = g_strndup(tmp + 1, len - 2);
		}
		else
			host = (gchar *) tmp;

		gtk_entry_set_text(GTK_ENTRY(priv->host_entry), host);
		if (tmp != host)
			g_free(host);
	}
	/* Host */
	user = gigolo_bookmark_get_user_unescaped(priv->bookmark_init);
	if (user != NULL)
	{
		gtk_entry_set_text(GTK_ENTRY(priv->user_entry), user);
		g_free(user);
	}
	/* Share */
	tmp = gigolo_bookmark_get_share(priv->bookmark_init);
	if (tmp != NULL)
		gtk_entry_set_text(GTK_ENTRY(priv->share_entry), tmp);
	/* Domain */
	tmp = gigolo_bookmark_get_domain(priv->bookmark_init);
	if (tmp != NULL)
		gtk_entry_set_text(GTK_ENTRY(priv->domain_entry), tmp);
	/* Port */
	port = gigolo_bookmark_get_port(priv->bookmark_init);

	idx = scheme_to_index(gigolo_bookmark_get_scheme(priv->bookmark_init));

	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(priv->autoconnect_checkbtn),
		gigolo_bookmark_get_autoconnect(priv->bookmark_init));

	if (port == 0)
		port = methods[idx].port;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(priv->port_spin), port);

	combo_set_active(priv->type_combo, idx);
}


static void setup_for_type(GigoloBookmarkEditDialog *dialog)
{
	struct MethodInfo *meth;
	guint i;
	guint idx;
	GtkWidget *table;
	GtkTreeIter iter;
	GigoloBookmarkEditDialogPrivate *priv = GIGOLO_BOOKMARK_EDIT_DIALOG_GET_PRIVATE(dialog);

	if (! gtk_combo_box_get_active_iter(GTK_COMBO_BOX(priv->type_combo), &iter))
		return;

	gtk_tree_model_get(gtk_combo_box_get_model(GTK_COMBO_BOX(priv->type_combo)),
			    &iter, COLUMN_INDEX, &idx, -1);
	g_return_if_fail(idx < methods_len);
	meth = &(methods[idx]);

	if (gtk_widget_get_parent(priv->uri_entry) != NULL)
	{
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->uri_label);
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->uri_entry);
	}
	if (gtk_widget_get_parent(priv->host_entry) != NULL)
	{
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->host_label);
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->host_entry);
	}
	if (gtk_widget_get_parent(priv->port_spin) != NULL)
	{
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->port_label);
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->port_spin);
	}
	if (gtk_widget_get_parent(priv->user_entry) != NULL)
	{
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->user_label);
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->user_entry);
	}
	if (gtk_widget_get_parent(priv->domain_entry) != NULL)
	{
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->domain_label);
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->domain_entry);
	}
	if (gtk_widget_get_parent(priv->share_combo) != NULL)
	{
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->share_label);
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->share_combo);
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->share_button);
	}
	if (gtk_widget_get_parent(priv->information_label) != NULL)
	{
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->information_label);
	}

	i = 4;
	table = priv->table;

	if (meth->scheme == NULL)
	{
		gtk_misc_set_alignment(GTK_MISC(priv->uri_label), 0.0, 0.5);
		gtk_widget_show(priv->uri_label);
		gtk_table_attach(GTK_TABLE(table), priv->uri_label,
				  0, 1, i, i+1, GTK_FILL, GTK_FILL, 0, 0);

		gtk_label_set_mnemonic_widget(GTK_LABEL(priv->uri_label), priv->uri_entry);
		gtk_widget_show(priv->uri_entry);
		gtk_table_attach(GTK_TABLE(table), priv->uri_entry,
				  1, 2, i, i+1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

		i++;
	}
	else
	{
		if (meth->flags & SHOW_DEVICE)
			gtk_label_set_text_with_mnemonic(GTK_LABEL(priv->host_label), _("_Device:"));
		else
			gtk_label_set_text_with_mnemonic(GTK_LABEL(priv->host_label), _("_Server:"));

		gtk_misc_set_alignment(GTK_MISC(priv->host_label), 0.0, 0.5);
		gtk_widget_show(priv->host_label);
		gtk_table_attach(GTK_TABLE(table), priv->host_label,
				  0, 1, i, i+1, GTK_FILL, GTK_FILL, 0, 0);

		gtk_label_set_mnemonic_widget(GTK_LABEL(priv->host_label), priv->host_entry);
		gtk_widget_show(priv->host_entry);
		gtk_table_attach(GTK_TABLE(table), priv->host_entry,
				  1, 2, i, i+1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

		i++;

		if (meth->flags & SHOW_SHARE)
		{
			gtk_misc_set_alignment(GTK_MISC(priv->share_label), 0.0, 0.5);
			gtk_widget_show(priv->share_label);
			gtk_table_attach(GTK_TABLE(table), priv->share_label,
					  0, 1, i, i+1, GTK_FILL, GTK_FILL, 0, 0);

			gtk_label_set_mnemonic_widget(GTK_LABEL(priv->share_label), priv->share_combo);
			gtk_widget_show(priv->share_combo);
			gtk_table_attach(GTK_TABLE(table), priv->share_combo,
					  1, 2, i, i+1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

			gtk_widget_show(priv->share_button);
			gtk_table_attach(GTK_TABLE(table), priv->share_button,
					  2, 3, i, i+1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

			i++;
		}
	}

	if (meth->flags & (SHOW_PORT | SHOW_DOMAIN | SHOW_USER))
	{
		gtk_misc_set_alignment(GTK_MISC(priv->information_label), 0.0, 0.5);
		gtk_widget_show(priv->information_label);
		gtk_table_attach(GTK_TABLE(table), priv->information_label,
			0, 2, i, i+1, GTK_FILL, GTK_FILL, 0, 0);

		i++;

		if (meth->flags & SHOW_PORT)
		{
			gtk_misc_set_alignment(GTK_MISC(priv->port_label), 0.0, 0.5);
			gtk_widget_show(priv->port_label);
			gtk_table_attach(GTK_TABLE(table), priv->port_label,
					  0, 1, i, i+1, GTK_FILL, GTK_FILL, 0, 0);

			gtk_label_set_mnemonic_widget(GTK_LABEL(priv->port_label), priv->port_spin);
			gtk_widget_show(priv->port_spin);
			gtk_table_attach(GTK_TABLE(table), priv->port_spin,
					  1, 2, i, i+1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

			i++;
		}

		if (meth->flags & SHOW_DOMAIN)
		{
			gtk_misc_set_alignment(GTK_MISC(priv->domain_label), 0.0, 0.5);
			gtk_widget_show(priv->domain_label);
			gtk_table_attach(GTK_TABLE(table), priv->domain_label,
					  0, 1, i, i+1, GTK_FILL, GTK_FILL, 0, 0);

			gtk_label_set_mnemonic_widget(GTK_LABEL(priv->domain_label), priv->domain_entry);
			gtk_widget_show(priv->domain_entry);
			gtk_table_attach(GTK_TABLE(table), priv->domain_entry,
					  1, 2, i, i+1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

			i++;
		}

		if (meth->flags & SHOW_USER)
		{
			gtk_misc_set_alignment(GTK_MISC(priv->user_label), 0.0, 0.5);
			gtk_widget_show(priv->user_label);
			gtk_table_attach(GTK_TABLE(table), priv->user_label,
					  0, 1, i, i+1, GTK_FILL, GTK_FILL, 0, 0);

			gtk_label_set_mnemonic_widget(GTK_LABEL(priv->user_label), priv->user_entry);
			gtk_widget_show(priv->user_entry);
			gtk_table_attach(GTK_TABLE(table), priv->user_entry,
					  1, 2, i, i+1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

			i++;
		}
	}
}


static void combo_changed_callback(G_GNUC_UNUSED GtkComboBox *combo_box,
								   GigoloBookmarkEditDialog *dialog)
{
	setup_for_type(dialog);
}


static void fill_method_combo_box(GigoloBookmarkEditDialog *dialog)
{
	guint i, j;
	gboolean visible;
	gboolean have_webdav = FALSE;
	const gchar* const *supported;
	GtkListStore *store;
	GtkTreeModel *filter;
	GtkTreeIter iter;
	const gchar *scheme;
	GigoloBookmarkEditDialogPrivate *priv = GIGOLO_BOOKMARK_EDIT_DIALOG_GET_PRIVATE(dialog);

	/* 0 - method index, 1 - visible/supported flag, 2 - description */
	store = gtk_list_store_new(3, G_TYPE_UINT, G_TYPE_BOOLEAN, G_TYPE_STRING);

	supported = gigolo_backend_gvfs_get_supported_uri_schemes();

	for (i = 0; i < methods_len; i++)
	{
		visible = FALSE;
		for (j = 0; supported[j] != NULL; j++)
		{
			/* Hack: list 'davs://' even if GVfs reports to not support it.
			 * See http://bugzilla.gnome.org/show_bug.cgi?id=538461 */
			if (i == SCHEME_DAV && gigolo_str_equal(methods[i].scheme, supported[j]))
			{
				visible = TRUE;
				have_webdav = TRUE;
				break;
			}
			if (i == SCHEME_DAVS && have_webdav)
			{
				visible = TRUE;
				break;
			}
			else if (methods[i].scheme == NULL || gigolo_str_equal(methods[i].scheme, supported[j]))
			{
				visible = TRUE;
				break;
			}
		}
		scheme = gigolo_describe_scheme((methods[i].scheme != NULL) ? methods[i].scheme : "custom");

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
			COLUMN_INDEX, i,
			COLUMN_VISIBLE, visible,
			COLUMN_DESC, scheme,
			-1);
	}

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), COLUMN_DESC, GTK_SORT_ASCENDING);

	filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(store), NULL);
	gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(filter), COLUMN_VISIBLE);
	gtk_combo_box_set_model(GTK_COMBO_BOX(priv->type_combo), filter);
	g_object_set_data(G_OBJECT(filter), "combobox", priv->type_combo);
	g_object_unref(G_OBJECT(store));
	g_object_unref(G_OBJECT(filter));
}


/* Update the contents of the bookmark with the values from the dialog. */
static void update_bookmark(GigoloBookmarkEditDialog *dialog)
{
	GigoloBookmarkEditDialogPrivate *priv;
	const gchar *tmp;
	gint idx;
	GtkTreeIter iter;
	gboolean autoconnect;

	g_return_if_fail(dialog != NULL);

	priv = GIGOLO_BOOKMARK_EDIT_DIALOG_GET_PRIVATE(dialog);
	g_return_if_fail(priv->bookmark_update != NULL);
	g_return_if_fail(gigolo_bookmark_is_valid(priv->bookmark_update));

	if (! gtk_combo_box_get_active_iter(GTK_COMBO_BOX(priv->type_combo), &iter))
		return;

	gtk_tree_model_get(gtk_combo_box_get_model(GTK_COMBO_BOX(priv->type_combo)),
			    &iter, COLUMN_INDEX, &idx, -1);

	tmp = gtk_entry_get_text(GTK_ENTRY(priv->name_entry));
	if (*tmp)	/* the name might be empty if the dialog is used as a Connect dialog */
		gigolo_bookmark_set_name(priv->bookmark_update, tmp);

	if (idx == -1)
		idx = 0;
	if (methods[idx].scheme == NULL)
	{
		gigolo_bookmark_set_uri(priv->bookmark_update, gtk_entry_get_text(GTK_ENTRY(priv->uri_entry)));
	}
	else if (idx == SCHEME_OBEX)
	{
		gchar *host;

		gigolo_bookmark_set_scheme(priv->bookmark_update, methods[idx].scheme);

		tmp = gtk_entry_get_text(GTK_ENTRY(priv->host_entry));

		if (tmp[0] != '[')
			host = g_strconcat("[", tmp, "]", NULL);
		else
			host = (gchar *) tmp;

		gigolo_bookmark_set_host(priv->bookmark_update, host);
		if (tmp != host)
			g_free(host);
	}
	else
	{
		gigolo_bookmark_set_scheme(priv->bookmark_update, methods[idx].scheme);

		tmp = gtk_entry_get_text(GTK_ENTRY(priv->host_entry));
		gigolo_bookmark_set_host(priv->bookmark_update, tmp);
		tmp = gtk_entry_get_text(GTK_ENTRY(priv->user_entry));
		gigolo_bookmark_set_user(priv->bookmark_update, tmp);
		tmp = gtk_entry_get_text(GTK_ENTRY(priv->domain_entry));
		gigolo_bookmark_set_domain(priv->bookmark_update, tmp);
		tmp = gtk_entry_get_text(GTK_ENTRY(priv->share_entry));
		gigolo_bookmark_set_share(priv->bookmark_update, tmp);
		gigolo_bookmark_set_port(priv->bookmark_update,
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(priv->port_spin)));
	}

	autoconnect = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->autoconnect_checkbtn));
	gigolo_bookmark_set_autoconnect(priv->bookmark_update, autoconnect);
}


static void gigolo_bookmark_edit_dialog_set_property(GObject *object, guint prop_id,
												   const GValue *value, GParamSpec *pspec)
{
 	GigoloBookmarkEditDialog *dialog = GIGOLO_BOOKMARK_EDIT_DIALOG(object);
 	GigoloBookmarkEditDialogPrivate *priv = GIGOLO_BOOKMARK_EDIT_DIALOG_GET_PRIVATE(dialog);

	switch (prop_id)
    {
    case PROP_BOOKMARK_INIT:
		priv->bookmark_init = g_value_get_object(value);
		init_values(dialog);
		break;
    case PROP_BOOKMARK_UPDATE:
		priv->bookmark_update = g_value_get_object(value);
		update_bookmark(dialog);
		break;
    case PROP_MODE:
    {
		const gchar *title;
		const gchar *stock_id;
		const gchar *button_stock_id;

		switch (g_value_get_int(value))
		{
			case GIGOLO_BE_MODE_CREATE:
			{
				title = _("Create Bookmark");
				button_stock_id = stock_id = GTK_STOCK_ADD;
				combo_set_active(priv->type_combo, 0);
				break;
			}
			case GIGOLO_BE_MODE_EDIT:
			{
				title = _("Edit Bookmark");
				stock_id = GTK_STOCK_EDIT;
				button_stock_id = GTK_STOCK_OK;
				break;
			}
			case GIGOLO_BE_MODE_CONNECT:
			default:
			{
				title = _("Connect to Server");
				button_stock_id = stock_id = GTK_STOCK_CONNECT;
				combo_set_active(priv->type_combo, 0);
				gtk_widget_hide(priv->name_label);
				gtk_widget_hide(priv->name_entry);
				gtk_widget_hide(priv->autoconnect_label);
				gtk_widget_hide(priv->autoconnect_checkbtn);
				break;
			}
		}
		gtk_window_set_title(GTK_WINDOW(dialog), title);
		gtk_window_set_icon_name(GTK_WINDOW(dialog), stock_id);
		gtk_dialog_add_buttons(GTK_DIALOG(dialog), button_stock_id, GTK_RESPONSE_OK, NULL);

		setup_for_type(dialog);
        break;
    }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}


static void share_button_clicked_cb(GtkWidget *btn, GigoloBookmarkEditDialog *dialog)
{
	GigoloBookmarkEditDialogPrivate *priv = GIGOLO_BOOKMARK_EDIT_DIALOG_GET_PRIVATE(dialog);
	const gchar *hostname;
	gchar **shares;

	gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(priv->share_combo))));

	hostname = gtk_entry_get_text(GTK_ENTRY(priv->host_entry));
	if (! NZV(hostname))
		return;

	gtk_widget_set_sensitive(btn, FALSE);

	/* FIXME for now we ignore username and domain when browsing for shares as this is not yet
	 * supported by GVfs (1.0.x). */
	shares = gigolo_backend_gvfs_get_smb_shares(hostname, NULL, NULL);

	if (shares != NULL)
	{
		guint i, len = g_strv_length(shares);
		for (i = 0; i < len; i++)
		{
			gtk_combo_box_append_text(GTK_COMBO_BOX(priv->share_combo), shares[i]);
		}
		g_strfreev(shares);
	}
	gtk_widget_set_sensitive(btn, TRUE);
}


static void host_entry_changed_cb(GtkEditable *editable, GtkWidget *btn)
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(editable));

	gtk_widget_set_sensitive(btn, NZV(text));
}


static void gigolo_bookmark_edit_dialog_init(GigoloBookmarkEditDialog *dialog)
{
	GtkWidget *label;
	GtkWidget *label_tmp;
	GtkWidget *table;
	GtkWidget *combo;
	GtkWidget *entry;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkCellRenderer *renderer;
	GigoloBookmarkEditDialogPrivate *priv = GIGOLO_BOOKMARK_EDIT_DIALOG_GET_PRIVATE(dialog);

	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
	gtk_box_set_spacing(GTK_BOX(gigolo_dialog_get_content_area(GTK_DIALOG(dialog))), 2);

	gtk_dialog_add_buttons(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER (vbox), 5);
	gtk_box_pack_start(GTK_BOX(gigolo_dialog_get_content_area(GTK_DIALOG(dialog))),
		vbox, FALSE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	priv->table = table = gtk_table_new(9, 3, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);
	gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 0);

	priv->name_label = gtk_label_new_with_mnemonic(_("_Bookmark name:"));
	gtk_misc_set_alignment(GTK_MISC(priv->name_label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), priv->name_label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

	priv->name_entry = entry = gtk_entry_new();
	gtk_label_set_mnemonic_widget(GTK_LABEL(priv->name_label), entry);
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

	priv->autoconnect_label = gtk_label_new_with_mnemonic(_("Au_to-Connect"));
	gtk_misc_set_alignment(GTK_MISC(priv->autoconnect_label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), priv->autoconnect_label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

	priv->autoconnect_checkbtn = gtk_check_button_new();
	gtk_label_set_mnemonic_widget(GTK_LABEL(priv->autoconnect_label), priv->autoconnect_checkbtn);
	gtk_table_attach(GTK_TABLE(table), priv->autoconnect_checkbtn, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

	label = gtk_label_new_with_mnemonic(_("Service t_ype:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);

	priv->type_combo = combo = gtk_combo_box_new();
	gtk_table_attach(GTK_TABLE(table), combo, 1, 2, 2, 3, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

	label_tmp = gtk_label_new(" ");
	gtk_table_attach(GTK_TABLE(table), label_tmp, 0, 2, 3, 4, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(combo), renderer, "text", COLUMN_DESC);

	fill_method_combo_box(dialog);

	gtk_label_set_mnemonic_widget(GTK_LABEL(label), combo);
	g_signal_connect(combo, "changed", G_CALLBACK(combo_changed_callback), dialog);

	priv->uri_entry = gtk_entry_new();
	priv->host_entry = gtk_entry_new();
	priv->port_spin = gtk_spin_button_new_with_range(0, 65535, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(priv->port_spin), 0.0);
	gtk_widget_set_tooltip_text(priv->port_spin, _("Set the port to 0 to use the default port"));
	priv->user_entry = gtk_entry_new();
	priv->domain_entry = gtk_entry_new();
	priv->share_combo = gtk_combo_box_entry_new_text();
	priv->share_entry = gtk_bin_get_child(GTK_BIN(priv->share_combo));

	priv->uri_label = gtk_label_new_with_mnemonic(_("_Location (URI):"));
	priv->host_label = gtk_label_new_with_mnemonic(_("_Server:"));
	priv->user_label = gtk_label_new_with_mnemonic(_("_User Name:"));
	priv->information_label = gtk_label_new(_("Optional information:"));
	priv->port_label = gtk_label_new_with_mnemonic(_("_Port:"));
	priv->domain_label = gtk_label_new_with_mnemonic(_("_Domain:"));
	priv->share_label = gtk_label_new_with_mnemonic(_("_Share:"));

	gtk_entry_set_activates_default(GTK_ENTRY(priv->name_entry), TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(priv->uri_entry), TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(priv->host_entry), TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(priv->port_spin), TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(priv->user_entry), TRUE);

	priv->share_button = gtk_button_new();
	gtk_button_set_image(GTK_BUTTON(priv->share_button),
		gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
	gtk_widget_set_sensitive(priv->share_button, FALSE);
	g_signal_connect(priv->share_button, "clicked", G_CALLBACK(share_button_clicked_cb), dialog);

	g_signal_connect(priv->host_entry, "changed",
		G_CALLBACK(host_entry_changed_cb), priv->share_button);

	/* We need an extra ref so we can remove them from the table */
	g_object_ref(priv->uri_entry);
	g_object_ref(priv->uri_label);
	g_object_ref(priv->host_entry);
	g_object_ref(priv->host_label);
	g_object_ref(priv->port_label);
	g_object_ref(priv->port_spin);
	g_object_ref(priv->user_entry);
	g_object_ref(priv->user_label);
	g_object_ref(priv->domain_entry);
	g_object_ref(priv->domain_label);
	g_object_ref(priv->share_combo);
	g_object_ref(priv->share_button);
	g_object_ref(priv->share_label);
	g_object_ref(priv->information_label);

	gtk_widget_show_all(vbox);
}


GtkWidget *gigolo_bookmark_edit_dialog_new(GtkWindow *parent, GigoloBookmarkEditDialogMode mode)
{
	GigoloBookmarkEditDialog *dialog = g_object_new(GIGOLO_BOOKMARK_EDIT_DIALOG_TYPE,
		"transient-for", parent,
		"mode", mode,
		NULL);
	GigoloBookmarkEditDialogPrivate *priv = GIGOLO_BOOKMARK_EDIT_DIALOG_GET_PRIVATE(dialog);
	priv->parent = parent;

	return GTK_WIDGET(dialog);
}


GtkWidget *gigolo_bookmark_edit_dialog_new_with_bookmark(GtkWindow *parent,
		GigoloBookmarkEditDialogMode mode, GigoloBookmark *bookmark)
{
	GigoloBookmarkEditDialog *dialog = g_object_new(GIGOLO_BOOKMARK_EDIT_DIALOG_TYPE,
		"transient-for", parent,
		"bookmark-init", bookmark,
		"mode", mode,
		NULL);
	GigoloBookmarkEditDialogPrivate *priv = GIGOLO_BOOKMARK_EDIT_DIALOG_GET_PRIVATE(dialog);
	priv->parent = parent;

	return GTK_WIDGET(dialog);
}


