/*
 *      bookmarkeditdialog.c
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

/* The code is mainly based on Nautilus' 'Connect to server' dialog, thanks */

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "common.h"
#include "compat.h"
#include "bookmark.h"
#include "bookmarkeditdialog.h"

typedef struct _SionBookmarkEditDialogPrivate			SionBookmarkEditDialogPrivate;

#define SION_BOOKMARK_EDIT_DIALOG_GET_PRIVATE(obj)		(G_TYPE_INSTANCE_GET_PRIVATE((obj),\
			SION_BOOKMARK_EDIT_DIALOG_TYPE, SionBookmarkEditDialogPrivate))

struct _SionBookmarkEditDialogPrivate
{
	GtkWidget *table;

	GtkWidget *type_combo;
	GtkWidget *information_label;

	GtkWidget *name_label;
	GtkWidget *name_entry;

	GtkWidget *uri_label;
	GtkWidget *uri_entry;

	GtkWidget *server_label;
	GtkWidget *server_entry;

	GtkWidget *port_label;
	GtkWidget *port_spin;

	GtkWidget *user_label;
	GtkWidget *user_entry;

	GtkWidget *domain_label;
	GtkWidget *domain_entry;

	GtkWidget *share_label;
	GtkWidget *share_entry;
/*
	GtkWidget *share_entry;
	GtkWidget *domain_entry;
	GtkWidget *folder_entry;
*/
	SionBookmark *bookmark_init;
	SionBookmark *bookmark_update;
};

static void sion_bookmark_edit_dialog_class_init			(SionBookmarkEditDialogClass *klass);
static void sion_bookmark_edit_dialog_set_property			(GObject *object, guint prop_id,
															 const GValue *value, GParamSpec *pspec);
static void sion_bookmark_edit_dialog_init      			(SionBookmarkEditDialog *dialog);


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
	SHOW_DOMAIN    = 0x00000080
};

enum {
	COLUMN_INDEX,
	COLUMN_VISIBLE,
	COLUMN_DESC,
};

static struct MethodInfo methods[] = {
	{ "ftp",  21,	SHOW_PORT | SHOW_USER },
	{ "sftp", 22,	SHOW_PORT | SHOW_USER },
	{ "smb",  0,	SHOW_SHARE | SHOW_USER | SHOW_DOMAIN },
	{ "davs", 443,	SHOW_PORT | SHOW_USER },
	{ "dav",  80,	SHOW_PORT | SHOW_USER },
	{ NULL,   0,	0 }
};
static guint methods_len = G_N_ELEMENTS(methods);


static GtkDialogClass *parent_class = NULL;


GType sion_bookmark_edit_dialog_get_type(void)
{
	static GType self_type = 0;
	if (! self_type)
	{
		static const GTypeInfo self_info =
		{
			sizeof(SionBookmarkEditDialogClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc)sion_bookmark_edit_dialog_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(SionBookmarkEditDialog),
			0,
			(GInstanceInitFunc)sion_bookmark_edit_dialog_init,
			NULL /* value_table */
		};

		self_type = g_type_register_static(GTK_TYPE_DIALOG, "SionBookmarkEditDialog", &self_info, 0);
	}

	return self_type;
}


static void sion_bookmark_edit_dialog_destroy(GtkObject *object)
{
	SionBookmarkEditDialogPrivate *priv = SION_BOOKMARK_EDIT_DIALOG_GET_PRIVATE(object);

	gtk_widget_destroy(priv->uri_entry);
	gtk_widget_destroy(priv->uri_label);
	gtk_widget_destroy(priv->server_entry);
	gtk_widget_destroy(priv->server_label);
	gtk_widget_destroy(priv->port_label);
	gtk_widget_destroy(priv->port_spin);
	gtk_widget_destroy(priv->user_entry);
	gtk_widget_destroy(priv->user_label);
	gtk_widget_destroy(priv->domain_entry);
	gtk_widget_destroy(priv->domain_label);
	gtk_widget_destroy(priv->share_entry);
	gtk_widget_destroy(priv->share_label);
	gtk_widget_destroy(priv->information_label);

	GTK_OBJECT_CLASS(parent_class)->destroy(object);
}


static void sion_bookmark_edit_dialog_class_init(SionBookmarkEditDialogClass *klass)
{
	GtkObjectClass *gtk_object_class = (GtkObjectClass *)klass;
	GObjectClass *g_object_class = G_OBJECT_CLASS(klass);

	gtk_object_class->destroy = sion_bookmark_edit_dialog_destroy;

	g_object_class->set_property = sion_bookmark_edit_dialog_set_property;

	parent_class = (GtkDialogClass*)g_type_class_peek(GTK_TYPE_DIALOG);
	g_type_class_add_private((gpointer)klass, sizeof(SionBookmarkEditDialogPrivate));

	g_object_class_install_property(g_object_class,
									PROP_MODE,
									g_param_spec_int(
									"mode",
									"Mode",
									"Operation mode",
									0, G_MAXINT, SION_BE_MODE_CREATE,
									G_PARAM_WRITABLE));
	g_object_class_install_property(g_object_class,
									PROP_BOOKMARK_INIT,
									g_param_spec_object(
									"bookmark-init",
									"Bookmark-init",
									"Bookmark instance to provide default values",
									SION_BOOKMARK_TYPE,
									G_PARAM_WRITABLE));
	g_object_class_install_property(g_object_class,
									PROP_BOOKMARK_UPDATE,
									g_param_spec_object(
									"bookmark-update",
									"Bookmark-update",
									"Bookmark instance",
									SION_BOOKMARK_TYPE,
									G_PARAM_WRITABLE));
}


static guint scheme_to_index(const gchar *scheme)
{
	guint i;

	for (i = 0; i < methods_len; i++)
	{
		if (sion_str_equal(scheme, methods[i].scheme))
		{
			return i;
		}
	}
	/* if no matching scheme was found, fall back to the Custom method */
	return methods_len - 1;
}


gboolean combo_foreach(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	gint idx = GPOINTER_TO_INT(data);
	gint i;

	gtk_tree_model_get(model, iter, COLUMN_INDEX, &i, -1);

	if (i == idx)
	{
		GObject *combo = g_object_get_data(G_OBJECT(model), "combobox");
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo), iter);

		return TRUE;
	}
	return FALSE;
}


static void combo_set_active(GtkWidget *combo, gint idx)
{
	gtk_tree_model_foreach(gtk_combo_box_get_model(GTK_COMBO_BOX(combo)),
		combo_foreach, GINT_TO_POINTER(idx));
}


static void init_values(SionBookmarkEditDialog *dialog)
{
	SionBookmarkEditDialogPrivate *priv = SION_BOOKMARK_EDIT_DIALOG_GET_PRIVATE(dialog);
	gchar *uri;
	const gchar *tmp;
	guint port;
	guint idx;

	tmp = sion_bookmark_get_name(priv->bookmark_init);
	if (tmp != NULL)
		gtk_entry_set_text(GTK_ENTRY(priv->name_entry), tmp);
		tmp = sion_bookmark_get_name(priv->bookmark_init);
	uri = sion_bookmark_get_uri(priv->bookmark_init);
	if (uri != NULL)
	{
		gtk_entry_set_text(GTK_ENTRY(priv->uri_entry), uri);
		tmp = sion_bookmark_get_uri(priv->bookmark_init);
		g_free(uri);
	}
	tmp = sion_bookmark_get_host(priv->bookmark_init);
	if (tmp != NULL)
		gtk_entry_set_text(GTK_ENTRY(priv->server_entry), tmp);
		tmp = sion_bookmark_get_name(priv->bookmark_init);
	tmp = sion_bookmark_get_user(priv->bookmark_init);
	if (tmp != NULL)
		gtk_entry_set_text(GTK_ENTRY(priv->user_entry), tmp);
	tmp = sion_bookmark_get_share(priv->bookmark_init);
	if (tmp != NULL)
		gtk_entry_set_text(GTK_ENTRY(priv->share_entry), tmp);
	tmp = sion_bookmark_get_domain(priv->bookmark_init);
	if (tmp != NULL)
		gtk_entry_set_text(GTK_ENTRY(priv->domain_entry), tmp);
	port = sion_bookmark_get_port(priv->bookmark_init);
	idx = scheme_to_index(sion_bookmark_get_scheme(priv->bookmark_init));
	if (port == 0)
		port = methods[idx].port;

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(priv->port_spin), port);
	combo_set_active(priv->type_combo, (gint) idx);
}


static void setup_for_type(SionBookmarkEditDialog *dialog)
{
	struct MethodInfo *meth;
	guint i;
	guint idx;
	GtkWidget *table;
	GtkTreeIter iter;
	SionBookmarkEditDialogPrivate *priv = SION_BOOKMARK_EDIT_DIALOG_GET_PRIVATE(dialog);

	if (! gtk_combo_box_get_active_iter(GTK_COMBO_BOX(priv->type_combo), &iter))
		return;

	gtk_tree_model_get(gtk_combo_box_get_model(GTK_COMBO_BOX(priv->type_combo)),
			    &iter, COLUMN_INDEX, &idx, -1);
	g_return_if_fail(idx < methods_len && idx >= 0);
	meth = &(methods[idx]);

	if (gtk_widget_get_parent(priv->uri_entry) != NULL)
	{
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->uri_label);
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->uri_entry);
	}
	if (gtk_widget_get_parent(priv->server_entry) != NULL)
	{
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->server_label);
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->server_entry);
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
	if (gtk_widget_get_parent(priv->share_entry) != NULL)
	{
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->share_label);
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->share_entry);
	}
	if (gtk_widget_get_parent(priv->information_label) != NULL)
	{
		gtk_container_remove(GTK_CONTAINER(priv->table), priv->information_label);
	}

	i = 3;
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
		gtk_misc_set_alignment(GTK_MISC(priv->server_label), 0.0, 0.5);
		gtk_widget_show(priv->server_label);
		gtk_table_attach(GTK_TABLE(table), priv->server_label,
				  0, 1, i, i+1, GTK_FILL, GTK_FILL, 0, 0);

		gtk_label_set_mnemonic_widget(GTK_LABEL(priv->server_label), priv->server_entry);
		gtk_widget_show(priv->server_entry);
		gtk_table_attach(GTK_TABLE(table), priv->server_entry,
				  1, 2, i, i+1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

		i++;

		if (meth->flags & SHOW_SHARE)
		{
			gtk_misc_set_alignment(GTK_MISC(priv->share_label), 0.0, 0.5);
			gtk_widget_show(priv->share_label);
			gtk_table_attach(GTK_TABLE(table), priv->share_label,
					  0, 1, i, i+1, GTK_FILL, GTK_FILL, 0, 0);

			gtk_label_set_mnemonic_widget(GTK_LABEL(priv->share_label), priv->share_entry);
			gtk_widget_show(priv->share_entry);
			gtk_table_attach(GTK_TABLE(table), priv->share_entry,
					  1, 2, i, i+1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

			i++;
		}

	}

	if (meth->flags)
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


static void combo_changed_callback(GtkComboBox *combo_box, SionBookmarkEditDialog *dialog)
{
	setup_for_type(dialog);
}


static void fill_method_combo_box(SionBookmarkEditDialog *dialog)
{
	guint i, j;
	gboolean visible;
	const gchar* const *supported;
	GtkListStore *store;
	GtkTreeModel *filter;
	GtkTreeIter iter;
	const gchar *scheme;
	SionBookmarkEditDialogPrivate *priv = SION_BOOKMARK_EDIT_DIALOG_GET_PRIVATE(dialog);

	/* 0 - method index, 1 - visible/supported flag, 2 - description */
	store = gtk_list_store_new(3, G_TYPE_INT, G_TYPE_BOOLEAN, G_TYPE_STRING);

	supported = g_vfs_get_supported_uri_schemes(g_vfs_get_default());

	for (i = 0; i < methods_len; i++)
	{
		visible = FALSE;
		for (j = 0; supported[j] != NULL; j++)
		{
			if (methods[i].scheme == NULL || sion_str_equal(methods[i].scheme, supported[j]))
			{
				visible = TRUE;
				break;
			}
		}
		if (methods[i].scheme != NULL)
			scheme = sion_describe_scheme(methods[i].scheme);
		else
			scheme = _("Custom Location");

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
			COLUMN_INDEX, i,
			COLUMN_VISIBLE, visible,
			COLUMN_DESC, scheme,
			-1);
	}

	filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(store), NULL);
	gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(filter), COLUMN_VISIBLE);
	gtk_combo_box_set_model(GTK_COMBO_BOX(priv->type_combo), filter);
	g_object_set_data(G_OBJECT(filter), "combobox", priv->type_combo);
	g_object_unref(G_OBJECT(store));
	g_object_unref(G_OBJECT(filter));
}


/* Update the contents of the bookmark with the values from the dialog. */
void update_bookmark(SionBookmarkEditDialog *dialog)
{
	SionBookmarkEditDialogPrivate *priv;
	const gchar *tmp;
	gint idx;
	GtkTreeIter iter;

	g_return_if_fail(dialog != NULL);

	/// TODO do error checking, at the very least, don't allow empty bookmark names

	priv = SION_BOOKMARK_EDIT_DIALOG_GET_PRIVATE(dialog);
	g_return_if_fail(priv->bookmark_update != NULL);

	tmp = gtk_entry_get_text(GTK_ENTRY(priv->name_entry));
	if (*tmp)
		sion_bookmark_set_name(priv->bookmark_update, tmp);

	if (! gtk_combo_box_get_active_iter(GTK_COMBO_BOX(priv->type_combo), &iter))
		return;

	gtk_tree_model_get(gtk_combo_box_get_model(GTK_COMBO_BOX(priv->type_combo)),
			    &iter, COLUMN_INDEX, &idx, -1);

	if (idx == -1)
		idx = 0;
	if (methods[idx].scheme == NULL)
		sion_bookmark_set_uri(priv->bookmark_update, gtk_entry_get_text(GTK_ENTRY(priv->uri_entry)));
	else
	{
		sion_bookmark_set_scheme(priv->bookmark_update, methods[idx].scheme);

		tmp = gtk_entry_get_text(GTK_ENTRY(priv->server_entry));
		if (*tmp)
			sion_bookmark_set_host(priv->bookmark_update, tmp);
		tmp = gtk_entry_get_text(GTK_ENTRY(priv->user_entry));
		sion_bookmark_set_user(priv->bookmark_update, tmp);
		tmp = gtk_entry_get_text(GTK_ENTRY(priv->domain_entry));
		sion_bookmark_set_domain(priv->bookmark_update, tmp);
		tmp = gtk_entry_get_text(GTK_ENTRY(priv->share_entry));
		sion_bookmark_set_share(priv->bookmark_update, tmp);
		sion_bookmark_set_port(priv->bookmark_update,
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(priv->port_spin)));
	}
}


static void sion_bookmark_edit_dialog_set_property(GObject *object, guint prop_id,
												   const GValue *value, GParamSpec *pspec)
{
 	SionBookmarkEditDialog *dialog = SION_BOOKMARK_EDIT_DIALOG(object);
 	SionBookmarkEditDialogPrivate *priv = SION_BOOKMARK_EDIT_DIALOG_GET_PRIVATE(dialog);

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
			case SION_BE_MODE_CREATE:
			{
				title = _("Create Bookmark");
				button_stock_id = stock_id = GTK_STOCK_ADD;
				combo_set_active(priv->type_combo, 0);
				break;
			}
			case SION_BE_MODE_EDIT:
			{
				title = _("Edit Bookmark");
				stock_id = GTK_STOCK_EDIT;
				button_stock_id = GTK_STOCK_OK;
				break;
			}
			case SION_BE_MODE_CONNECT:
			default:
			{
				title = _("Connect to Server");
				button_stock_id = stock_id = GTK_STOCK_CONNECT;
				combo_set_active(priv->type_combo, 0);
				gtk_widget_hide(priv->name_label);
				gtk_widget_hide(priv->name_entry);
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


static void sion_bookmark_edit_dialog_init(SionBookmarkEditDialog *dialog)
{
	GtkWidget *label;
	GtkWidget *table;
	GtkWidget *combo;
	GtkWidget *entry;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkCellRenderer *renderer;
	SionBookmarkEditDialogPrivate *priv = SION_BOOKMARK_EDIT_DIALOG_GET_PRIVATE(dialog);

	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
	gtk_box_set_spacing(GTK_BOX(sion_dialog_get_content_area(GTK_DIALOG(dialog))), 2);

	gtk_dialog_add_buttons(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER (vbox), 5);
	gtk_box_pack_start(GTK_BOX(sion_dialog_get_content_area(GTK_DIALOG(dialog))),
		vbox, FALSE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	priv->table = table = gtk_table_new(8, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);
	gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 0);

	priv->name_label = gtk_label_new_with_mnemonic(_("_Bookmark name:"));
	gtk_misc_set_alignment(GTK_MISC(priv->name_label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), priv->name_label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

	priv->name_entry = entry = gtk_entry_new();
	gtk_label_set_mnemonic_widget(GTK_LABEL(priv->name_label), entry);
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

	label = gtk_label_new_with_mnemonic(_("Service _type:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

	priv->type_combo = combo = gtk_combo_box_new();
	gtk_table_attach(GTK_TABLE(table), combo, 1, 2, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

	label = gtk_label_new(" ");
	gtk_table_attach(GTK_TABLE(table), label, 0, 2, 2, 3, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(combo), renderer, "text", COLUMN_DESC);

	fill_method_combo_box(dialog);

	gtk_label_set_mnemonic_widget(GTK_LABEL(label), combo);
	g_signal_connect(combo, "changed", G_CALLBACK(combo_changed_callback), dialog);

	priv->uri_entry = gtk_entry_new();
	priv->server_entry = gtk_entry_new();
	priv->port_spin = gtk_spin_button_new_with_range(0, 65536, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(priv->port_spin), 0);
	gtk_widget_set_tooltip_text(priv->port_spin, _("Set the port to 0 to use the default port."));
	priv->user_entry = gtk_entry_new();
	priv->domain_entry = gtk_entry_new();
	priv->share_entry = gtk_entry_new();

	priv->uri_label = gtk_label_new_with_mnemonic(_("_Location (URI):"));
	priv->server_label = gtk_label_new_with_mnemonic(_("_Server:"));
	priv->user_label = gtk_label_new_with_mnemonic(_("_User Name:"));
	priv->information_label = gtk_label_new(_("Optional information:"));
	priv->port_label = gtk_label_new_with_mnemonic(_("_Port:"));
	priv->domain_label = gtk_label_new_with_mnemonic(_("_Domain:"));
	priv->share_label = gtk_label_new_with_mnemonic(_("_Share:"));

	gtk_entry_set_activates_default(GTK_ENTRY(priv->uri_entry), TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(priv->server_entry), TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(priv->port_spin), TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(priv->user_entry), TRUE);

	/* We need an extra ref so we can remove them from the table */
	g_object_ref(priv->uri_entry);
	g_object_ref(priv->uri_label);
	g_object_ref(priv->server_entry);
	g_object_ref(priv->server_label);
	g_object_ref(priv->port_label);
	g_object_ref(priv->port_spin);
	g_object_ref(priv->user_entry);
	g_object_ref(priv->user_label);
	g_object_ref(priv->domain_entry);
	g_object_ref(priv->domain_label);
	g_object_ref(priv->share_entry);
	g_object_ref(priv->share_label);
	g_object_ref(priv->information_label);

	gtk_widget_show_all(vbox);
}


GtkWidget* sion_bookmark_edit_dialog_new(GtkWidget *parent, SionBookmarkEditDialogMode mode)
{
	SionBookmarkEditDialog *dialog = g_object_new(SION_BOOKMARK_EDIT_DIALOG_TYPE,
		"transient-for", parent,
		"mode", mode,
		NULL);


	return GTK_WIDGET(dialog);
}


GtkWidget* sion_bookmark_edit_dialog_new_with_bookmark(GtkWidget *parent, SionBookmarkEditDialogMode mode, SionBookmark *bookmark)
{
	SionBookmarkEditDialog *dialog = g_object_new(SION_BOOKMARK_EDIT_DIALOG_TYPE,
		"transient-for", parent,
		"bookmark-init", bookmark,
		"mode", mode,
		NULL);


	return GTK_WIDGET(dialog);
}


