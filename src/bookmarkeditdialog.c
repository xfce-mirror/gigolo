/*
 *      bookmarkeditdialog.c
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

/* The code is mainly based on Nautilus' 'Connect to server' dialog, thanks */

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "common.h"
#include "bookmark.h"
#include "settings.h"
#include "backendgvfs.h"
#include "window.h"
#include "bookmarkeditdialog.h"


typedef struct _GigoloBookmarkEditDialogPrivate			GigoloBookmarkEditDialogPrivate;

struct _GigoloBookmarkEditDialogPrivate
{
	GigoloWindow *parent;
	gint dialog_type;

	GtkWidget *type_combo;
	GtkWidget *information_frame;

	GtkWidget *name_label;
	GtkWidget *name_entry;

	GtkWidget *autoconnect_label;
	GtkWidget *autoconnect_switch;

	GtkWidget *uri_label;
	GtkWidget *uri_entry;

	GtkWidget *host_label;
	GtkWidget *host_entry;

	GtkWidget *folder_label;
	GtkWidget *folder_entry;

	GtkWidget *path_label;
	GtkWidget *path_entry;

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
	GtkWidget *share_box;

	GtkWidget *color_label;
	GtkWidget *color_chooser;
	gboolean   color_set;

	GigoloBookmark *bookmark_init;
	GigoloBookmark *bookmark_update;

	gulong browse_host_signal_id;
};

static void gigolo_bookmark_edit_dialog_set_property		(GObject *object, guint prop_id,
															 const GValue *value, GParamSpec *pspec);
static void browse_host_finished_cb							(G_GNUC_UNUSED GigoloBackendGVFS *bnd,
															 GSList *shares,
															 GigoloBookmarkEditDialog *dialog);


struct MethodInfo
{
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
	SHOW_DEVICE    = 0x00000100,
	SHOW_FOLDER    = 0x00000200,
	SHOW_PATH      = 0x00000400
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
	{ "ftp",  21,	SHOW_PORT | SHOW_USER | SHOW_FOLDER },
	{ "sftp", 22,	SHOW_PORT | SHOW_USER | SHOW_FOLDER },
	{ "smb",  0,	SHOW_SHARE | SHOW_USER | SHOW_DOMAIN | SHOW_FOLDER },
	{ "dav",  80,	SHOW_PATH | SHOW_PORT | SHOW_USER | SHOW_FOLDER },
	{ "davs", 443,	SHOW_PATH | SHOW_PORT | SHOW_USER | SHOW_FOLDER },
	{ "obex", 0,	SHOW_DEVICE },
	{ NULL,   0,	0 }
};
static guint methods_len = G_N_ELEMENTS(methods);



G_DEFINE_TYPE_WITH_PRIVATE(GigoloBookmarkEditDialog, gigolo_bookmark_edit_dialog, GTK_TYPE_DIALOG);



static void gigolo_bookmark_edit_dialog_finalize(GObject *object)
{
	GigoloBookmarkEditDialogPrivate *priv = gigolo_bookmark_edit_dialog_get_instance_private(GIGOLO_BOOKMARK_EDIT_DIALOG(object));
	GigoloBackendGVFS *backend;

	backend = gigolo_window_get_backend(priv->parent);
	if (backend != NULL && IS_GIGOLO_BACKEND_GVFS(backend) && priv->browse_host_signal_id > 0)
	{
		g_signal_handler_disconnect(gigolo_window_get_backend(priv->parent), priv->browse_host_signal_id);
		priv->browse_host_signal_id = 0;
	}

	G_OBJECT_CLASS(gigolo_bookmark_edit_dialog_parent_class)->finalize(object);
}


static gboolean check_custom_uri(const gchar *uri)
{
	GigoloBookmark *bm;
	gboolean result;

	bm = gigolo_bookmark_new_from_uri("(validation)", uri);
	result = gigolo_bookmark_is_valid(bm);
	g_object_unref(bm);

	return result;
}


gint gigolo_bookmark_edit_dialog_run(GigoloBookmarkEditDialog *dialog)
{
	gint res;
	gboolean error = FALSE;
	GigoloBookmarkEditDialogPrivate *priv = gigolo_bookmark_edit_dialog_get_instance_private(dialog);
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
			if (gtk_widget_is_visible(priv->name_entry))
			{	/* check the name only if we are creating/editing a bookmark */
				tmp = gtk_entry_get_text(GTK_ENTRY(priv->name_entry));
				if (! *tmp)
				{
					error = TRUE;
					gigolo_message_dialog(dialog, GTK_MESSAGE_ERROR, _("Error"),
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
							gigolo_message_dialog(dialog, GTK_MESSAGE_ERROR, _("Error"),
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
					gigolo_message_dialog(dialog, GTK_MESSAGE_ERROR, _("Error"),
						_("You must enter a server address or name."), NULL);
					gtk_widget_grab_focus(priv->host_entry);
				}
			}
			if (gtk_widget_is_visible(priv->share_combo) && ! error && gtk_widget_get_parent(priv->share_combo) != NULL)
			{
				tmp = gtk_entry_get_text(GTK_ENTRY(priv->share_entry));
				if (! *tmp)
				{
					error = TRUE;
					gigolo_message_dialog(dialog, GTK_MESSAGE_ERROR, _("Error"),
						_("You must enter a share name."), NULL);
					gtk_widget_grab_focus(priv->share_combo);
				}
			}
			if (gtk_widget_is_visible(priv->uri_entry) && ! error && gtk_widget_get_parent(priv->uri_entry) != NULL)
			{
				tmp = gtk_entry_get_text(GTK_ENTRY(priv->uri_entry));
				if (! *tmp || ! check_custom_uri(tmp))
				{
					error = TRUE;
					gigolo_message_dialog(dialog, GTK_MESSAGE_ERROR, _("Error"),
						_("You must enter a valid URI for the connection."), NULL);
					gtk_widget_grab_focus(priv->uri_entry);
				}
			}
			if (! error && gtk_widget_get_parent(priv->path_entry) != NULL)
			{
				tmp = gtk_entry_get_text(GTK_ENTRY(priv->path_entry));
				if (tmp[0] == '/')
				{	/* remove leading slashes */
					gchar *path = g_strdup(tmp);
					gtk_entry_set_text(GTK_ENTRY(priv->path_entry), path + 1);
					g_free(path);
				}
			}
			if (! error && gtk_widget_get_parent(priv->share_entry) != NULL)
			{
				tmp = gtk_entry_get_text(GTK_ENTRY(priv->share_entry));
				if (tmp[0] == '/')
				{	/* remove leading slashes */
					gchar *share = g_strdup(tmp);
					gtk_entry_set_text(GTK_ENTRY(priv->share_entry), share + 1);
					g_free(share);
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
	GObjectClass *g_object_class = G_OBJECT_CLASS(klass);

	g_object_class->finalize = gigolo_bookmark_edit_dialog_finalize;

	g_object_class->set_property = gigolo_bookmark_edit_dialog_set_property;

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
	GigoloBookmarkEditDialogPrivate *priv = gigolo_bookmark_edit_dialog_get_instance_private(dialog);
	gchar *uri, *user;
	const gchar *tmp;
	guint port;
	guint idx;

	/* Name */
	tmp = gigolo_bookmark_get_name(priv->bookmark_init);
	if (tmp != NULL)
		gtk_entry_set_text(GTK_ENTRY(priv->name_entry), tmp);
	/* Color */
	tmp = gigolo_bookmark_get_color(priv->bookmark_init);
	if (tmp != NULL)
	{
		GdkRGBA color;
		if (gdk_rgba_parse(&color, tmp))
			gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(priv->color_chooser), &color);
	}
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
	/* Folder */
	tmp = gigolo_bookmark_get_folder(priv->bookmark_init);
	if (tmp != NULL)
		gtk_entry_set_text(GTK_ENTRY(priv->folder_entry), tmp);
	/* Share */
	tmp = gigolo_bookmark_get_share(priv->bookmark_init);
	if (tmp != NULL)
		gtk_entry_set_text(GTK_ENTRY(priv->share_entry), tmp);
	/* Domain */
	tmp = gigolo_bookmark_get_domain(priv->bookmark_init);
	if (tmp != NULL)
		gtk_entry_set_text(GTK_ENTRY(priv->domain_entry), tmp);
	/* Path */
	tmp = gigolo_bookmark_get_path(priv->bookmark_init);
	if (tmp != NULL)
		gtk_entry_set_text(GTK_ENTRY(priv->path_entry), tmp);
	/* Port */
	port = gigolo_bookmark_get_port(priv->bookmark_init);

	idx = scheme_to_index(gigolo_bookmark_get_scheme(priv->bookmark_init));

	gtk_switch_set_active(
		GTK_SWITCH(priv->autoconnect_switch),
		gigolo_bookmark_get_autoconnect(priv->bookmark_init));

	if (port == 0)
		port = methods[idx].port;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(priv->port_spin), port);

	combo_set_active(priv->type_combo, idx);
}


static void setup_for_type(GigoloBookmarkEditDialog *dialog)
{
	struct MethodInfo *meth;
	guint idx;
	GtkWidget *table;
	GtkTreeIter iter;
	GigoloBookmarkEditDialogPrivate *priv = gigolo_bookmark_edit_dialog_get_instance_private(dialog);

	if (! gtk_combo_box_get_active_iter(GTK_COMBO_BOX(priv->type_combo), &iter))
		return;

	gtk_tree_model_get(gtk_combo_box_get_model(GTK_COMBO_BOX(priv->type_combo)),
			    &iter, COLUMN_INDEX, &idx, -1);
	g_return_if_fail(idx < methods_len);
	meth = &(methods[idx]);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->port_spin), meth->port);

	gtk_widget_hide (priv->uri_label);
	gtk_widget_hide (priv->uri_entry);

	gtk_widget_hide (priv->host_label);
	gtk_widget_hide (priv->host_entry);

	gtk_widget_hide (priv->folder_label);
	gtk_widget_hide (priv->folder_entry);

	gtk_widget_hide (priv->port_label);
	gtk_widget_hide (priv->port_spin);

	gtk_widget_hide (priv->user_label);
	gtk_widget_hide (priv->user_entry);

	gtk_widget_hide (priv->domain_label);
	gtk_widget_hide (priv->domain_entry);

	gtk_widget_hide (priv->path_label);
	gtk_widget_hide (priv->path_entry);

	gtk_widget_hide (priv->share_label);
	gtk_widget_hide (priv->share_box);

	gtk_widget_hide (priv->information_frame);

	if (meth->scheme == NULL)
	{
		gtk_label_set_xalign(GTK_LABEL(priv->uri_label), 0);
		gtk_widget_show(priv->uri_label);

		gtk_label_set_mnemonic_widget(GTK_LABEL(priv->uri_label), priv->uri_entry);
		gtk_widget_show(priv->uri_entry);
	}
	else
	{
		if (meth->flags & SHOW_DEVICE)
			gtk_label_set_text_with_mnemonic(GTK_LABEL(priv->host_label), _("_Device:"));
		else
			gtk_label_set_text_with_mnemonic(GTK_LABEL(priv->host_label), _("_Server:"));

		gtk_label_set_xalign(GTK_LABEL(priv->host_label), 0);
		gtk_widget_show(priv->host_label);

		gtk_label_set_mnemonic_widget(GTK_LABEL(priv->host_label), priv->host_entry);
		gtk_widget_show(priv->host_entry);

		if (meth->flags & SHOW_SHARE)
		{
			gtk_label_set_xalign(GTK_LABEL(priv->share_label), 0);
			gtk_widget_show(priv->share_label);

			gtk_label_set_mnemonic_widget(GTK_LABEL(priv->share_label), priv->share_combo);
			gtk_widget_show(priv->share_box);
		}
		if (meth->flags & SHOW_PATH)
		{
			gtk_label_set_xalign(GTK_LABEL(priv->path_label), 0);
			gtk_widget_show(priv->path_label);

			gtk_label_set_mnemonic_widget(GTK_LABEL(priv->path_label), priv->path_entry);
			gtk_widget_show(priv->path_entry);
		}
	}

	if (meth->flags & (SHOW_PORT | SHOW_DOMAIN | SHOW_USER))
	{
		gtk_widget_show(priv->information_frame);

		if (meth->flags & SHOW_PORT)
		{
			gtk_label_set_xalign(GTK_LABEL(priv->port_label), 0);
			gtk_widget_show(priv->port_label);

			gtk_label_set_mnemonic_widget(GTK_LABEL(priv->port_label), priv->port_spin);
			gtk_widget_show(priv->port_spin);
		}

		if (meth->flags & SHOW_FOLDER && priv->dialog_type != GIGOLO_BE_MODE_CONNECT)
		{
			gtk_label_set_xalign(GTK_LABEL(priv->folder_label), 0);
			gtk_widget_show(priv->folder_label);

			gtk_label_set_mnemonic_widget(GTK_LABEL(priv->folder_label), priv->folder_entry);
			gtk_widget_show(priv->folder_entry);
		}

		if (meth->flags & SHOW_DOMAIN)
		{
			gtk_label_set_xalign(GTK_LABEL(priv->domain_label), 0);
			gtk_widget_show(priv->domain_label);

			gtk_label_set_mnemonic_widget(GTK_LABEL(priv->domain_label), priv->domain_entry);
			gtk_widget_show(priv->domain_entry);
		}

		if (meth->flags & SHOW_USER)
		{
			gtk_label_set_xalign(GTK_LABEL(priv->user_label), 0);
			gtk_widget_show(priv->user_label);

			gtk_label_set_mnemonic_widget(GTK_LABEL(priv->user_label), priv->user_entry);
			gtk_widget_show(priv->user_entry);
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
	GigoloBookmarkEditDialogPrivate *priv = gigolo_bookmark_edit_dialog_get_instance_private(dialog);

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

		gtk_list_store_insert_with_values(store, &iter, -1,
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


static void update_bookmark_color(GigoloBookmarkEditDialog *dialog)
{
	GigoloBookmarkEditDialogPrivate *priv;
	GdkRGBA color;
	gchar *color_string;

	priv = gigolo_bookmark_edit_dialog_get_instance_private(dialog);

	if (! priv->color_set)
		/* if no colour has been chosen by the user, don't set the default colour (black) */
		return;

	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(priv->color_chooser), &color);
	color_string = gdk_rgba_to_string(&color);
	gigolo_bookmark_set_color(priv->bookmark_update, color_string);
	g_free(color_string);
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

	priv = gigolo_bookmark_edit_dialog_get_instance_private(dialog);
	g_return_if_fail(priv->bookmark_update != NULL);
	g_return_if_fail(gigolo_bookmark_is_valid(priv->bookmark_update));

	if (! gtk_combo_box_get_active_iter(GTK_COMBO_BOX(priv->type_combo), &iter))
		return;

	gtk_tree_model_get(gtk_combo_box_get_model(GTK_COMBO_BOX(priv->type_combo)),
			    &iter, COLUMN_INDEX, &idx, -1);

	tmp = gtk_entry_get_text(GTK_ENTRY(priv->name_entry));
	if (*tmp)	/* the name might be empty if the dialog is used as a Connect dialog */
		gigolo_bookmark_set_name(priv->bookmark_update, tmp);
	else
		gigolo_bookmark_set_name(priv->bookmark_update, GIGOLO_BOOKMARK_NAME_NONE);

	update_bookmark_color(dialog);

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
		tmp = gtk_entry_get_text(GTK_ENTRY(priv->folder_entry));
		gigolo_bookmark_set_folder(priv->bookmark_update, tmp);
		tmp = gtk_entry_get_text(GTK_ENTRY(priv->user_entry));
		gigolo_bookmark_set_user(priv->bookmark_update, tmp);
		tmp = gtk_entry_get_text(GTK_ENTRY(priv->path_entry));
		gigolo_bookmark_set_path(priv->bookmark_update, tmp);
		tmp = gtk_entry_get_text(GTK_ENTRY(priv->domain_entry));
		gigolo_bookmark_set_domain(priv->bookmark_update, tmp);
		tmp = gtk_entry_get_text(GTK_ENTRY(priv->share_entry));
		gigolo_bookmark_set_share(priv->bookmark_update, tmp);
		gigolo_bookmark_set_port(priv->bookmark_update,
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(priv->port_spin)));
	}

	autoconnect = gtk_switch_get_active(GTK_SWITCH(priv->autoconnect_switch));
	gigolo_bookmark_set_autoconnect(priv->bookmark_update, autoconnect);
}


static void gigolo_bookmark_edit_dialog_set_property(GObject *object, guint prop_id,
												   const GValue *value, GParamSpec *pspec)
{
 	GigoloBookmarkEditDialog *dialog = GIGOLO_BOOKMARK_EDIT_DIALOG(object);
 	GigoloBookmarkEditDialogPrivate *priv = gigolo_bookmark_edit_dialog_get_instance_private(dialog);

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
		gint mode = g_value_get_int(value);

		switch (mode)
		{
			case GIGOLO_BE_MODE_CREATE:
			{
				title = _("Create Bookmark");
				button_stock_id = stock_id = "gtk-add";
				combo_set_active(priv->type_combo, 0);
				break;
			}
			case GIGOLO_BE_MODE_EDIT:
			{
				title = _("Edit Bookmark");
				stock_id = "gtk-edit";
				button_stock_id = "gtk-ok";
				break;
			}
			case GIGOLO_BE_MODE_CONNECT:
			default:
			{
				title = _("Connect to Server");
				button_stock_id = stock_id = "gtk-connect";
				combo_set_active(priv->type_combo, 0);
				gtk_widget_hide(priv->name_label);
				gtk_widget_hide(priv->name_entry);
				gtk_widget_hide(priv->color_label);
				gtk_widget_hide(priv->color_chooser);
				gtk_widget_hide(priv->autoconnect_label);
				gtk_widget_hide(priv->autoconnect_switch);
				break;
			}
		}
		gtk_window_set_title(GTK_WINDOW(dialog), title);
		gtk_window_set_icon_name(GTK_WINDOW(dialog), stock_id);
		gtk_dialog_add_buttons(GTK_DIALOG(dialog), button_stock_id, GTK_RESPONSE_OK, NULL);
		priv->dialog_type = mode;

		setup_for_type(dialog);
        break;
    }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}


static void browse_host_finished_cb(G_GNUC_UNUSED GigoloBackendGVFS *bnd, GSList *shares,
									GigoloBookmarkEditDialog *dialog)
{
	GigoloBookmarkEditDialogPrivate *priv = gigolo_bookmark_edit_dialog_get_instance_private(dialog);

	if (shares != NULL)
	{
		GSList *node;
		for (node = shares; node != NULL; node = node->next)
		{
			gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(priv->share_combo), NULL, node->data);
		}
		gtk_combo_box_set_active(GTK_COMBO_BOX(priv->share_combo), 0);

	}
	gtk_widget_set_sensitive(priv->share_button, TRUE);
}


static void share_button_clicked_cb(GtkWidget *btn, GigoloBookmarkEditDialog *dialog)
{
	GigoloBookmarkEditDialogPrivate *priv = gigolo_bookmark_edit_dialog_get_instance_private(dialog);
	const gchar *hostname;

	gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(priv->share_combo))));

	hostname = gtk_entry_get_text(GTK_ENTRY(priv->host_entry));
	if (! NZV(hostname))
		return;

	gtk_widget_set_sensitive(btn, FALSE);

	gigolo_backend_gvfs_browse_host(gigolo_window_get_backend(priv->parent),
		GTK_WINDOW(priv->parent), hostname);
}


static void host_entry_changed_cb(GtkEditable *editable, GtkWidget *btn)
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(editable));

	gtk_widget_set_sensitive(btn, NZV(text));
}


static void entry_activate_cb(G_GNUC_UNUSED GtkEditable *editable, GigoloBookmarkEditDialog *dialog)
{
	gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
}


static void color_chooser_set_cb(G_GNUC_UNUSED GtkColorButton *widget,
								 GigoloBookmarkEditDialog *dialog)
{
	GigoloBookmarkEditDialogPrivate *priv = gigolo_bookmark_edit_dialog_get_instance_private(dialog);

	priv->color_set = TRUE;
}


static GtkWidget *make_frame (const gchar *label)
{
	GtkWidget *frame = gtk_frame_new (label);
	GtkWidget *widget = gtk_frame_get_label_widget (GTK_FRAME (frame));
	gchar     *formatted;

	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

	formatted = g_markup_printf_escaped ("<b>%s</b>", label);
	gtk_label_set_markup (GTK_LABEL (widget), formatted);

	return frame;
}


static GtkWidget *make_table ()
{
	GtkWidget *table = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (table), 6);
	gtk_grid_set_column_spacing (GTK_GRID (table), 12);
	gtk_widget_set_margin_start (GTK_WIDGET (table), 12);
	gtk_widget_set_margin_top (GTK_WIDGET (table), 6);
	return table;
}


static GtkWidget *make_label (const gchar *text, GtkSizeGroup *sg)
{
	GtkWidget *label = gtk_label_new_with_mnemonic(text);
	gtk_label_set_xalign(GTK_LABEL(label), 0);
	gtk_size_group_add_widget(sg, label);
	return label;
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
	GtkWidget *frame;
	GtkSizeGroup *sg;
	GtkCellRenderer *renderer;
	GigoloBookmarkEditDialogPrivate *priv = gigolo_bookmark_edit_dialog_get_instance_private(dialog);

	sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	priv->browse_host_signal_id = 0;
	priv->dialog_type = GIGOLO_BE_MODE_EDIT;

	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
	gtk_box_set_spacing(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), 2);

	gtk_dialog_add_buttons(GTK_DIALOG(dialog), "gtk-cancel", GTK_RESPONSE_CANCEL, NULL);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 18);
	gtk_container_set_border_width(GTK_CONTAINER (vbox), 12);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		vbox, FALSE, TRUE, 0);

	/* Bookmark Settings */
	frame = make_frame (_("Bookmark Settings"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);

	table = make_table ();
	gtk_container_add (GTK_CONTAINER (frame), table);

	priv->name_label = make_label (_("_Name:"), sg);
	gtk_grid_attach(GTK_GRID(table), priv->name_label, 0, 0, 1, 1);

	priv->name_entry = entry = gtk_entry_new();
	/* will make sure column 1 expands if the window is resized */
	gtk_widget_set_hexpand(GTK_WIDGET(entry), TRUE);
	gtk_label_set_mnemonic_widget(GTK_LABEL(priv->name_label), entry);
	gtk_grid_attach(GTK_GRID(table), entry, 1, 0, 1, 1);

	priv->color_label = make_label (_("_Color:"), sg);
	gtk_grid_attach(GTK_GRID(table), priv->color_label, 0, 1, 1, 1);

	priv->color_set = FALSE;
	priv->color_chooser = gtk_color_button_new();
	g_signal_connect(priv->color_chooser, "color-set", G_CALLBACK(color_chooser_set_cb), dialog);
	gtk_label_set_mnemonic_widget(GTK_LABEL(priv->color_label), priv->color_chooser);
	gtk_grid_attach(GTK_GRID(table), priv->color_chooser, 1, 1, 1, 1);

	priv->autoconnect_label = make_label (_("Au_to-Connect:"), sg);
	gtk_grid_attach(GTK_GRID(table), priv->autoconnect_label, 0, 2, 1, 1);

	priv->autoconnect_switch = gtk_switch_new();
	gtk_widget_set_hexpand (GTK_WIDGET (priv->autoconnect_switch), FALSE);
	gtk_widget_set_halign (GTK_WIDGET (priv->autoconnect_switch), GTK_ALIGN_END);
	gtk_widget_set_valign (GTK_WIDGET (priv->autoconnect_switch), GTK_ALIGN_CENTER);
	gtk_label_set_mnemonic_widget(GTK_LABEL(priv->autoconnect_label), priv->autoconnect_switch);
	gtk_grid_attach(GTK_GRID(table), priv->autoconnect_switch, 1, 2, 1, 1);

	/* Connection Settings */
	frame = make_frame (_("Connection Settings"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);

	table = make_table ();
	gtk_container_add (GTK_CONTAINER (frame), table);

	label = make_label (_("Service t_ype:"), sg);
	gtk_grid_attach(GTK_GRID(table), label, 0, 0, 1, 1);

	priv->type_combo = combo = gtk_combo_box_new();
	gtk_grid_attach(GTK_GRID(table), combo, 1, 0, 1, 1);
	gtk_widget_set_hexpand(GTK_WIDGET(priv->type_combo), TRUE);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(combo), renderer, "text", COLUMN_DESC);

	fill_method_combo_box(dialog);

	gtk_label_set_mnemonic_widget(GTK_LABEL(label), combo);
	g_signal_connect(combo, "changed", G_CALLBACK(combo_changed_callback), dialog);

	priv->uri_label = make_label (_("_Location (URI):"), sg);
	gtk_grid_attach(GTK_GRID(table), priv->uri_label, 0, 1, 1, 1);

	priv->uri_entry = gtk_entry_new();
	gtk_grid_attach(GTK_GRID(table), priv->uri_entry, 1, 1, 1, 1);

	priv->host_label = make_label (_("_Server:"), sg);
	gtk_grid_attach(GTK_GRID(table), priv->host_label, 0, 2, 1, 1);

	priv->host_entry = gtk_entry_new();
	gtk_grid_attach(GTK_GRID(table), priv->host_entry, 1, 2, 1, 1);

	priv->path_label = make_label (_("P_ath:"), sg);
	gtk_grid_attach(GTK_GRID(table), priv->path_label, 0, 3, 1, 1);

	priv->path_entry = gtk_entry_new();
	gtk_grid_attach(GTK_GRID(table), priv->path_entry, 1, 3, 1, 1);

	priv->share_label = make_label (_("_Share:"), sg);
	gtk_grid_attach(GTK_GRID(table), priv->share_label, 0, 4, 1, 1);

	priv->share_combo = gtk_combo_box_text_new_with_entry();
	gtk_widget_set_hexpand (priv->share_combo, TRUE);
	priv->share_entry = gtk_bin_get_child(GTK_BIN(priv->share_combo));

	priv->share_button = gtk_button_new();
	gtk_button_set_image(GTK_BUTTON(priv->share_button),
						 gtk_image_new_from_icon_name("gtk-refresh", GTK_ICON_SIZE_MENU));
	gtk_widget_set_sensitive(priv->share_button, FALSE);
	g_signal_connect(priv->share_button, "clicked", G_CALLBACK(share_button_clicked_cb), dialog);

	priv->share_box = hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start(GTK_BOX(hbox), priv->share_combo, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), priv->share_button, FALSE, FALSE, 0);
	gtk_grid_attach(GTK_GRID(table), hbox, 1, 4, 1, 1);

	/* Optional Information */
	priv->information_frame = frame = make_frame (_("Optional Information"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);

	table = make_table ();
	gtk_container_add(GTK_CONTAINER(frame), table);

	priv->port_label = make_label (_("_Port:"), sg);
	gtk_grid_attach(GTK_GRID(table), priv->port_label, 0, 0, 1, 1);

	priv->port_spin = gtk_spin_button_new_with_range(0, 65535, 1);
	gtk_grid_attach(GTK_GRID(table), priv->port_spin, 1, 0, 1, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(priv->port_spin), 0.0);
	gtk_widget_set_tooltip_text(priv->port_spin, _("Set the port to 0 to use the default port"));

	priv->folder_label = make_label (_("_Folder:"), sg);
	gtk_grid_attach(GTK_GRID(table), priv->folder_label, 0, 1, 1, 1);

	priv->folder_entry = gtk_entry_new();
	gtk_grid_attach(GTK_GRID(table), priv->folder_entry, 1, 1, 1, 1);
	gtk_widget_set_tooltip_text(priv->folder_entry,
		_("This is not used for the actual mount, only necessary for opening the mount point in a file browser"));
	gtk_widget_set_hexpand(priv->folder_entry, TRUE);

	priv->domain_label = make_label (_("_Domain:"), sg);
	gtk_grid_attach(GTK_GRID(table), priv->domain_label, 0, 2, 1, 1);

	priv->domain_entry = gtk_entry_new();
	gtk_grid_attach(GTK_GRID(table), priv->domain_entry, 1, 2, 1, 1);

	priv->user_label = make_label (_("_User Name:"), sg);
	gtk_grid_attach(GTK_GRID(table), priv->user_label, 0, 3, 1, 1);

	priv->user_entry = gtk_entry_new();
	gtk_grid_attach(GTK_GRID(table), priv->user_entry, 1, 3, 1, 1);
	g_signal_connect(priv->host_entry, "changed",
		G_CALLBACK(host_entry_changed_cb), priv->share_button);

	gtk_entry_set_activates_default(GTK_ENTRY(priv->name_entry), TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(priv->uri_entry), TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(priv->folder_entry), TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(priv->path_entry), TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(priv->host_entry), TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(priv->port_spin), TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(priv->user_entry), TRUE);

	g_signal_connect(priv->name_entry, "activate", G_CALLBACK(entry_activate_cb), dialog);
	g_signal_connect(priv->uri_entry, "activate", G_CALLBACK(entry_activate_cb), dialog);
	g_signal_connect(priv->host_entry, "activate", G_CALLBACK(entry_activate_cb), dialog);
	g_signal_connect(priv->folder_entry, "activate", G_CALLBACK(entry_activate_cb), dialog);
	g_signal_connect(priv->path_entry, "activate", G_CALLBACK(entry_activate_cb), dialog);
	g_signal_connect(priv->user_entry, "activate", G_CALLBACK(entry_activate_cb), dialog);

	gtk_widget_show_all(vbox);
}


GtkWidget *gigolo_bookmark_edit_dialog_new(GigoloWindow *parent, GigoloBookmarkEditDialogMode mode)
{
	GigoloBookmarkEditDialog *dialog;
	GigoloBookmarkEditDialogPrivate *priv;

	dialog = g_object_new(GIGOLO_BOOKMARK_EDIT_DIALOG_TYPE,
		"transient-for", parent,
		"mode", mode,
		NULL);

	priv = gigolo_bookmark_edit_dialog_get_instance_private(dialog);
	priv->parent = parent;

	priv->browse_host_signal_id = g_signal_connect(gigolo_window_get_backend(parent),
		"browse-host-finished", G_CALLBACK(browse_host_finished_cb), dialog);

	return GTK_WIDGET(dialog);
}


GtkWidget *gigolo_bookmark_edit_dialog_new_with_bookmark(GigoloWindow *parent,
		GigoloBookmarkEditDialogMode mode, GigoloBookmark *bookmark)
{
	GtkWidget *dialog;

	dialog = gigolo_bookmark_edit_dialog_new(parent, mode);
	g_object_set(dialog, "bookmark-init", bookmark, NULL);

	return dialog;
}


