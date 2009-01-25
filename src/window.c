/*
 *      window.c
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


#include "config.h"

#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "common.h"
#include "bookmark.h"
#include "settings.h"
#include "window.h"
#include "bookmarkdialog.h"
#include "bookmarkeditdialog.h"
#include "menubuttonaction.h"
#include "preferencesdialog.h"
#include "backendgvfs.h"
#include "mountdialog.h"
#include "main.h"


typedef struct _SionWindowPrivate			SionWindowPrivate;

#define SION_WINDOW_GET_PRIVATE(obj)		(G_TYPE_INSTANCE_GET_PRIVATE((obj),\
		SION_WINDOW_TYPE, SionWindowPrivate))

/* Returns: TRUE if @a ptr points to a non-zero value. */
#define NZV(ptr) \
	((ptr) && (ptr)[0])

struct _SionWindowPrivate
{
	SionSettings	*settings;
	SionBackendGVFS	*backend_gvfs;

	GtkWidget		*vbox;
	GtkWidget		*hbox;

	GtkWidget		*treeview;
	GtkWidget		*iconview;
	GtkWidget		*swin_treeview;
	GtkWidget		*swin_iconview;
	GtkListStore	*store;
	GtkWidget		*tree_popup_menu;
	GtkAction		*action_connect;
	GtkAction		*action_disconnect;
	GtkAction		*action_bookmarks;
	GtkAction		*action_bookmark_create;
	GtkAction		*action_open;
	GtkAction		*action_copyuri;

	GtkActionGroup	*action_group;

	GtkWidget		*toolbar;
	GtkStatusIcon	*systray_icon;
	GtkWidget		*systray_icon_popup_menu;

	guint			 autoconnect_timeout_id;
};

enum
{
	ACTION_OPEN,
	ACTION_ADD,
	ACTION_CONNECT,
	ACTION_DISCONNECT
};

enum
{
	VIEW_MODE_ICONVIEW,
	VIEW_MODE_TREEVIEW
};

static void sion_window_class_init			(SionWindowClass *klass);
static void sion_window_init				(SionWindow *window);

static GtkWindowClass *parent_class = NULL;


GType sion_window_get_type(void)
{
	static GType self_type = 0;

	if (! self_type)
	{
		static const GTypeInfo self_info =
		{
			sizeof(SionWindowClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc)sion_window_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(SionWindow),
			0,
			(GInstanceInitFunc)sion_window_init,
			NULL /* value_table */
		};

		self_type = g_type_register_static(GTK_TYPE_WINDOW, "SionWindow", &self_info, 0);
	}

	return self_type;
}


static gboolean sion_window_state_event(GtkWidget *widget, GdkEventWindowState *event)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(widget);
	gboolean show_systray_icon = sion_settings_get_boolean(priv->settings, "show-in-systray");

	if (show_systray_icon)
	{
		gboolean window_hidden = FALSE;
		if (event->changed_mask & GDK_WINDOW_STATE_ICONIFIED)
		{
			if (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED)
				window_hidden = TRUE;
			else
				window_hidden = FALSE;
		}
		if (event->changed_mask & GDK_WINDOW_STATE_WITHDRAWN)
		{
			if (event->new_window_state & GDK_WINDOW_STATE_WITHDRAWN)
				window_hidden = TRUE;
			else
				window_hidden = FALSE;
		}

		if (window_hidden && show_systray_icon)
		{
			gtk_window_set_skip_taskbar_hint(GTK_WINDOW(widget), TRUE);
		}
		else if (! window_hidden)
		{
			gtk_window_set_skip_taskbar_hint(GTK_WINDOW(widget), FALSE);
		}
	}
	return FALSE;
}


static void remove_autoconnect_timeout(SionWindow *window)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);

	if (priv->autoconnect_timeout_id != (guint) -1)
	{
		g_source_remove(priv->autoconnect_timeout_id);
		priv->autoconnect_timeout_id = (guint) -1;
	}
}


static gboolean sion_window_delete_event(GtkWidget *widget, G_GNUC_UNUSED GdkEventAny *event)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(widget);
	gint geo[5];

	remove_autoconnect_timeout(SION_WINDOW(widget));

	if (sion_settings_get_boolean(priv->settings, "save-geometry"))
	{
		gtk_window_get_position(GTK_WINDOW(widget), &geo[0], &geo[1]);
		gtk_window_get_size(GTK_WINDOW(widget), &geo[2], &geo[3]);
		if (gdk_window_get_state(sion_widget_get_window(widget)) & GDK_WINDOW_STATE_MAXIMIZED)
			geo[4] = 1;
		else
			geo[4] = 0;

		sion_settings_set_geometry(priv->settings, geo, 5);
	}
	gtk_widget_destroy(priv->tree_popup_menu);
	gtk_widget_destroy(priv->systray_icon_popup_menu);
	gtk_widget_destroy(priv->swin_treeview);
	gtk_widget_destroy(priv->swin_iconview);
	gtk_widget_destroy(priv->toolbar);
	g_object_unref(priv->action_group);
	g_object_unref(priv->systray_icon);
	g_object_unref(priv->systray_icon_popup_menu);
	g_object_unref(priv->backend_gvfs);

	return FALSE;
}


static void sion_window_class_init(SionWindowClass *klass)
{
	GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS(klass);
	gtkwidget_class->delete_event = sion_window_delete_event;
	gtkwidget_class->window_state_event = sion_window_state_event;

	parent_class =(GtkWindowClass*)g_type_class_peek(GTK_TYPE_WINDOW);
	g_type_class_add_private((gpointer)klass, sizeof(SionWindowPrivate));
}


static void systray_icon_activate_cb(G_GNUC_UNUSED GtkStatusIcon *status_icon, GtkWindow *window)
{
	if (gtk_window_is_active(window))
		gtk_widget_hide(GTK_WIDGET(window));
	else
	{
		gtk_window_deiconify(window);
		gtk_window_present(window);
	}
}


static void systray_icon_popup_menu_cb(G_GNUC_UNUSED GtkStatusIcon *status_icon, guint button,
								   guint activate_time, SionWindow *window)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);

	if (button == 3)
		gtk_menu_popup(GTK_MENU(priv->systray_icon_popup_menu), NULL, NULL, NULL, NULL,
			button, activate_time);
}


/* Convenience function to get the selected GtkTreeIter from the icon view or the treeview
 * whichever is currently used for display. */
static void get_selected_iter(SionWindow *window, GtkTreeIter *iter)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);

	g_return_if_fail(window != NULL);
	g_return_if_fail(iter != NULL);

	if (sion_settings_get_integer(priv->settings, "view-mode") == VIEW_MODE_TREEVIEW)
	{
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));
		gtk_tree_selection_get_selected(selection, NULL, iter);
	}
	else
	{
		GList *l, *items = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(priv->iconview));
		GtkTreeModel *model = gtk_icon_view_get_model(GTK_ICON_VIEW(priv->iconview));

		for (l = items; l != NULL; l = l->next)
		{
			gtk_tree_model_get_iter(model, iter, l->data);
			/* the selection mode is SINGLE, so the list should never have more than one entry */
			break;
		}

		g_list_foreach(items, (GFunc) gtk_tree_path_free, NULL);
		g_list_free(items);
	}
}


static SionBookmark *get_bookmark_from_uri(SionWindow *window, const gchar *uri)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);
	SionBookmarkList *bml = sion_settings_get_bookmarks(priv->settings);
	SionBookmark *bm = NULL;
	gboolean found = FALSE;
	gchar *tmp_uri;
	guint i;

	for (i = 0; i < bml->len && ! found; i++)
	{
		bm = g_ptr_array_index(bml, i);
		tmp_uri = sion_bookmark_get_uri(bm);
		if (sion_str_equal(uri, tmp_uri))
			found = TRUE;

		g_free(tmp_uri);
	}
	return bm;
}


static void mount_from_bookmark(SionWindow *window, SionBookmark *bookmark, gboolean show_dialog)
{
	gchar *uri;
	GtkWidget *dialog = NULL;
	SionWindowPrivate *priv;

	g_return_if_fail(window != NULL);
	g_return_if_fail(bookmark != NULL);

	priv = SION_WINDOW_GET_PRIVATE(window);

	uri = sion_bookmark_get_uri(bookmark);

	if (show_dialog)
	{
		const gchar *name = sion_bookmark_get_name(bookmark);
		gchar *label = g_strdup_printf(_("Mounting \"%s\""), (name != NULL) ? name : uri);

		dialog = sion_mount_dialog_new(GTK_WINDOW(window), label);
		gtk_widget_show_all(dialog);

		g_free(label);
	}

	sion_backend_gvfs_mount_uri(priv->backend_gvfs, uri, sion_bookmark_get_domain(bookmark), dialog);

	if (sion_bookmark_get_autoconnect(bookmark))
		sion_bookmark_set_should_not_autoconnect(bookmark, FALSE);

	g_free(uri);
}


static void action_mount_cb(G_GNUC_UNUSED GtkAction *action, SionWindow *window)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(priv->store);
	gpointer vol;
	gint ref_type;
	gboolean handled = FALSE;

	get_selected_iter(window, &iter);
	if (gtk_list_store_iter_is_valid(priv->store, &iter))
	{
		gtk_tree_model_get(model, &iter,
			SION_WINDOW_COL_REF_TYPE, &ref_type,
			SION_WINDOW_COL_REF, &vol, -1);

		if (ref_type == SION_WINDOW_REF_TYPE_VOLUME)
			handled = sion_backend_gvfs_mount_volume(priv->backend_gvfs, vol);
	}

	if (! handled)
	{
		SionBookmark *bm = NULL;
		GtkWidget *dialog;

		dialog = sion_bookmark_edit_dialog_new(GTK_WINDOW(window),
			priv->settings, SION_BE_MODE_CONNECT);
		if (sion_bookmark_edit_dialog_run(SION_BOOKMARK_EDIT_DIALOG(dialog)) == GTK_RESPONSE_OK)
		{
			bm = sion_bookmark_new();
			/* this fills the values of the dialog into 'bm' */
			g_object_set(dialog, "bookmark-update", bm, NULL);

			mount_from_bookmark(window, bm, TRUE);

			g_object_unref(bm);
		}
		gtk_widget_destroy(dialog);
	}
}


static void action_preferences_cb(G_GNUC_UNUSED GtkAction *action, SionWindow *window)
{
	GtkWidget *dialog;
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);

	dialog = sion_preferences_dialog_new(GTK_WINDOW(window), priv->settings);

	gtk_dialog_run(GTK_DIALOG(dialog));
	sion_window_do_autoconnect(window);
	sion_settings_write(priv->settings, SION_SETTINGS_PREFERENCES);

	gtk_widget_destroy(dialog);
}


static void action_unmount_cb(G_GNUC_UNUSED GtkAction *action, SionWindow *window)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);
	GtkTreeIter iter;
	SionBookmark *bm;

	get_selected_iter(window, &iter);
	if (gtk_list_store_iter_is_valid(priv->store, &iter))
	{
		gpointer mnt;
		GtkTreeModel *model = GTK_TREE_MODEL(priv->store);

		gtk_tree_model_get(model, &iter, SION_WINDOW_COL_REF, &mnt, -1);
		if (sion_backend_gvfs_is_mount(mnt))
		{
			gchar *uri;
			sion_backend_gvfs_get_name_and_uri_from_mount(mnt, NULL, &uri);
			bm = get_bookmark_from_uri(window, uri);
			if (bm != NULL && sion_bookmark_get_autoconnect(bm))
			{	/* we don't want auto-connection to reconnect this bookmark right
				   after we unmount it. */
				sion_bookmark_set_should_not_autoconnect(bm, TRUE);
			}
			g_free(uri);

			sion_backend_gvfs_unmount_mount(priv->backend_gvfs, mnt);
		}
	}
}


static void action_quit_cb(G_GNUC_UNUSED GtkAction *action, SionWindow *window)
{
    sion_window_delete_event(GTK_WIDGET(window), NULL);
    gtk_widget_destroy(GTK_WIDGET(window));
}


static void action_bookmark_edit_cb(G_GNUC_UNUSED GtkAction *action, SionWindow *window)
{
	GtkWidget *dialog;
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);

	dialog = sion_bookmark_dialog_new(GTK_WINDOW(window), priv->settings);

	gtk_dialog_run(GTK_DIALOG(dialog));
	sion_settings_write(priv->settings, SION_SETTINGS_BOOKMARKS);

	gtk_widget_destroy(dialog);
}


static void about_activate_link(G_GNUC_UNUSED GtkAboutDialog *dialog,
								const gchar *uri, G_GNUC_UNUSED gpointer data)
{
	sion_show_uri(uri);
}


static void action_about_cb(G_GNUC_UNUSED GtkAction *action, SionWindow *window)
{
    const gchar *authors[]= { "Enrico Tröger <enrico@xfce.org>", NULL };

	gtk_about_dialog_set_email_hook(about_activate_link, NULL, NULL);
	gtk_about_dialog_set_url_hook(about_activate_link, NULL, NULL);
	gtk_show_about_dialog(GTK_WINDOW(window),
		"authors", authors,
		"logo-icon-name", sion_get_application_icon_name(),
		"comments", "A simple frontend to easily connect to remote filesystems",
		"copyright", "Copyright 2008-2009 Enrico Tröger",
		"website", "http://www.uvena.de/sion/",
		"version", VERSION,
		"translator-credits", _("translator-credits"),
		"license",  "Copyright 2008-2009 Enrico Tröger <enrico@xfce.org>\n\n"
					"This program is free software; you can redistribute it and/or modify\n"
					"it under the terms of the GNU General Public License as published by\n"
					"the Free Software Foundation; either version 2 of the License, or\n"
					"(at your option) any later version.\n"
					"\n"
					"This program is distributed in the hope that it will be useful,\n"
					"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
					"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
					"GNU General Public License for more details.\n"
					"\n"
					"You should have received a copy of the GNU General Public License\n"
					"along with this program; if not, write to the Free Software\n"
					"Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.",
		  NULL);
}


static void action_copy_uri_cb(G_GNUC_UNUSED GtkAction *action, SionWindow *window)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(priv->store);

	get_selected_iter(window, &iter);
	if (gtk_list_store_iter_is_valid(priv->store, &iter))
	{
		gpointer mnt;

		gtk_tree_model_get(model, &iter, SION_WINDOW_COL_REF, &mnt, -1);
		if (sion_backend_gvfs_is_mount(mnt))
		{
			gchar *uri;

			sion_backend_gvfs_get_name_and_uri_from_mount(mnt, NULL, &uri);
			gtk_clipboard_set_text(gtk_clipboard_get(gdk_atom_intern("CLIPBOARD", FALSE)), uri, -1);

			g_free(uri);
		}
	}
}


static void action_open_cb(G_GNUC_UNUSED GtkAction *action, SionWindow *window)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(priv->store);

	if (! sion_settings_has_file_manager(priv->settings))
		return;

	get_selected_iter(window, &iter);
	if (gtk_list_store_iter_is_valid(priv->store, &iter))
	{
		gpointer mnt;
		gtk_tree_model_get(model, &iter, SION_WINDOW_COL_REF, &mnt, -1);
		if (sion_backend_gvfs_is_mount(mnt))
		{
#if 1
			GError *error = NULL;
			gchar *uri;
			gchar *file_manager;
			gchar *cmd;

			file_manager = sion_settings_get_string(priv->settings, "file-manager");
			sion_backend_gvfs_get_name_and_uri_from_mount(mnt, NULL, &uri);
			cmd = g_strconcat(file_manager, " ", uri, NULL);

			if (! g_spawn_command_line_async(cmd, &error))
			{
				verbose(error->message);
				g_error_free(error);
			}

			g_free(cmd);
			g_free(file_manager);
			g_free(uri);
#else
/*
			GFile *file = g_mount_get_root(mnt);
			gchar *path = g_file_get_path(file);

			if (path != NULL)
			{
				gchar *cmd = g_strconcat("xdg-open ", path, NULL);
				g_spawn_command_line_async(cmd, NULL);
				g_free(cmd);
				g_free(path);
			}
			else
			{
				// FIXME make the open command configurable or find a better solution
				//       maybe gtk_show_uri() ?
				GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
							GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
							_("Non-local mountpoints can't be opened."));
				gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), _("(not yet implemented)"));
				gtk_dialog_run(GTK_DIALOG (dialog));
				gtk_widget_destroy(dialog);
				verbose("Non-local mountpoints can't be opened.");
			}
			g_object_unref(file);
*/
#endif
		}
	}
}


static void tree_realize_cb(GtkWidget *widget)
{
	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(widget));
}


static gboolean iter_is_bookmark(SionWindow *window, GtkTreeModel *model, GtkTreeIter *iter)
{
	gint ref_type;
	gpointer ref;

	gtk_tree_model_get(model, iter, SION_WINDOW_COL_REF_TYPE, &ref_type,
									SION_WINDOW_COL_REF, &ref, -1);

	if (ref_type == SION_WINDOW_REF_TYPE_MOUNT)
	{
		gchar *uri;
		gboolean found = FALSE;

		sion_backend_gvfs_get_name_and_uri_from_mount(ref, NULL, &uri);

		found = (get_bookmark_from_uri(window, uri) != NULL);

		g_free(uri);
		return found;
	}

	return TRUE;
}


static void update_sensitive_buttons(SionWindow *window, GtkTreeModel *model, GtkTreeIter *iter)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);
	gint ref_type;
	gboolean is_bookmark = FALSE;

	if (iter != NULL && gtk_list_store_iter_is_valid(priv->store, iter))
	{
		gtk_tree_model_get(model, iter, SION_WINDOW_COL_REF_TYPE, &ref_type, -1);
		is_bookmark = iter_is_bookmark(window, model, iter);

		/* gtk_action_set_sensitive(priv->action_connect, (ref_type != SION_WINDOW_REF_TYPE_MOUNT)); */
		/* gtk_action_set_sensitive(priv->action_bookmarks_toolbar, (ref_type != SION_WINDOW_REF_TYPE_MOUNT)); */
		gtk_action_set_sensitive(priv->action_disconnect, (ref_type == SION_WINDOW_REF_TYPE_MOUNT));
		gtk_action_set_sensitive(priv->action_bookmark_create, ! is_bookmark);
		gtk_action_set_sensitive(priv->action_open, sion_settings_has_file_manager(priv->settings));
		gtk_action_set_sensitive(priv->action_copyuri, (ref_type == SION_WINDOW_REF_TYPE_MOUNT));
	}
	else
	{
		/* gtk_action_set_sensitive(priv->action_connect, FALSE); */
		/* gtk_action_set_sensitive(priv->action_bookmarks_toolbar, FALSE); */
		gtk_action_set_sensitive(priv->action_disconnect, FALSE);
		gtk_action_set_sensitive(priv->action_bookmark_create, FALSE);
		gtk_action_set_sensitive(priv->action_open, FALSE);
		gtk_action_set_sensitive(priv->action_copyuri, FALSE);
	}
}


static void tree_selection_changed_cb(GtkTreeSelection *selection, SionWindow *window)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);

	if (selection == NULL)
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));

	gtk_tree_selection_get_selected(selection, &model, &iter);

	update_sensitive_buttons(window, model, &iter);
}


static void iconview_selection_changed_cb(GtkIconView *view, SionWindow *window)
{
	GList *l, *items = gtk_icon_view_get_selected_items(view);
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_icon_view_get_model(view);

	for (l = items; l != NULL; l = l->next)
	{
		gtk_tree_model_get_iter(model, &iter, l->data);
		update_sensitive_buttons(window, model, &iter);
	}
	if (items == NULL)
		update_sensitive_buttons(window, model, NULL);

	g_list_foreach(items, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(items);
}


static void mounts_changed_cb(G_GNUC_UNUSED SionBackendGVFS *backend, SionWindow *window)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);
	gint view_mode = sion_settings_get_integer(priv->settings, "view-mode");

	if (view_mode == VIEW_MODE_ICONVIEW)
	{
		iconview_selection_changed_cb(GTK_ICON_VIEW(priv->iconview), window);
	}
	else if (view_mode == VIEW_MODE_TREEVIEW)
	{
		tree_selection_changed_cb(NULL, window);
	}
}


static void mount_operation_failed_cb(G_GNUC_UNUSED SionBackendGVFS *backend, const gchar *message,
								   const gchar *error_message, SionWindow *window)
{
	sion_error_dialog((gpointer) window, message, error_message);
}


static void tree_row_activated_cb(G_GNUC_UNUSED GtkTreeView *treeview, GtkTreePath *path,
								  G_GNUC_UNUSED GtkTreeViewColumn *arg2, SionWindow *window)
{
	GtkTreeIter iter;
	gint ref_type;
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);

	if (gtk_tree_model_get_iter(GTK_TREE_MODEL(priv->store), &iter, path))
	{
		gtk_tree_model_get(GTK_TREE_MODEL(priv->store), &iter,
			SION_WINDOW_COL_REF_TYPE, &ref_type, -1);
		if (ref_type == SION_WINDOW_REF_TYPE_MOUNT)
		{
			/* action_unmount_cb(NULL, data); */
			action_open_cb(NULL, window);
		}
		else
		{
			action_mount_cb(NULL, window);
		}
	}
}


static void iconview_item_activated_cb(G_GNUC_UNUSED GtkIconView *iconview,
									   GtkTreePath *path, SionWindow *window)
{
	tree_row_activated_cb(NULL, path, NULL, window);
}


static gboolean tree_button_press_event_cb(G_GNUC_UNUSED GtkWidget *widget,
										   GdkEventButton *event, SionWindow *window)
{
	if (event->button == 3)
	{
		SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);
		GtkTreeSelection *treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));
		gboolean have_sel = (gtk_tree_selection_count_selected_rows(treesel) > 0);

		if (have_sel)
			gtk_menu_popup(GTK_MENU(priv->tree_popup_menu), NULL, NULL, NULL, NULL,
																event->button, event->time);
	}
	return FALSE;
}


/* tries to select the item/path specified by wx and wy, GtkIconView doesn't do this by default
 * when right-clicking on an item */
static void iconview_select_item(GtkIconView *view, gdouble wx, gdouble wy)
{
	gint bx, by;
	GtkTreePath *path;

	gtk_icon_view_convert_widget_to_bin_window_coords(view, wx, wy, &bx, &by);
	path = gtk_icon_view_get_path_at_pos(view, bx, by);

	if (path != NULL)
	{
		gtk_icon_view_select_path(view, path);
		gtk_tree_path_free(path);
	}
}


static gboolean iconview_button_press_event_cb(GtkWidget *widget, GdkEventButton *event, SionWindow *window)
{
	if (event->button == 3)
	{
		SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);
		GList *items;
		gboolean have_sel;

		iconview_select_item(GTK_ICON_VIEW(widget), event->x, event->y);

		items = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(widget));
		have_sel = (items != NULL) && (g_list_length(items) > 0);
		g_list_foreach(items, (GFunc) gtk_tree_path_free, NULL);
		g_list_free(items);

		if (have_sel)
			gtk_menu_popup(GTK_MENU(priv->tree_popup_menu), NULL, NULL, NULL, NULL,
																event->button, event->time);
	}
	return FALSE;
}


static void action_bookmark_activate_cb(G_GNUC_UNUSED SionMenubuttonAction *action,
										GtkWidget *item, SionWindow *window)
{
	SionBookmark *bm = g_object_get_data(G_OBJECT(item), "bookmark");

	mount_from_bookmark(window, bm, TRUE);
}


static gint sort_bookmarks(gconstpointer a, gconstpointer b)
{
	SionBookmark *bm_a = SION_BOOKMARK(((GPtrArray*)a)->pdata);
	SionBookmark *bm_b = SION_BOOKMARK(((GPtrArray*)b)->pdata);
	const gchar *name_a = sion_bookmark_get_name(bm_a);
	const gchar *name_b = sion_bookmark_get_name(bm_b);

	return g_strcmp0(name_a, name_b);
}


void sion_window_update_bookmarks(SionWindow *window)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);
	SionBookmarkList *bookmarks = sion_settings_get_bookmarks(priv->settings);

	/* sort the bookmarks */
	g_ptr_array_sort(bookmarks, sort_bookmarks);

	/* writing to the 'settings' property will update the menus */
	g_object_set(priv->action_bookmarks, "settings", priv->settings, NULL);
}


gboolean sion_window_do_autoconnect(gpointer data)
{
	SionWindow *window = SION_WINDOW(data);
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);
	SionBookmarkList *bookmarks = sion_settings_get_bookmarks(priv->settings);
	static gint old_interval = -1;
	gint interval;
	guint i;

	interval = sion_settings_get_integer(priv->settings, "autoconnect-interval");
	if (old_interval != interval)
	{
		if (priv->autoconnect_timeout_id != (guint) -1)
			remove_autoconnect_timeout(window);

		priv->autoconnect_timeout_id = g_timeout_add_seconds(
			interval, sion_window_do_autoconnect, data);
		old_interval = interval;
	}

	if (interval == 0)
	{
		remove_autoconnect_timeout(window);
		return FALSE;
	}

	for (i = 0; i < bookmarks->len; i++)
	{
		SionBookmark *bm = g_ptr_array_index(bookmarks, i);
		if (sion_bookmark_get_autoconnect(bm) && ! sion_bookmark_get_should_not_autoconnect(bm))
		{
			mount_from_bookmark(window, bm, FALSE);
		}
	}
	return TRUE;
}


static void action_create_bookmark_cb(G_GNUC_UNUSED GtkAction *button, SionWindow *window)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(priv->store);

	get_selected_iter(window, &iter);
	if (gtk_list_store_iter_is_valid(priv->store, &iter))
	{
		gpointer mnt;

		gtk_tree_model_get(model, &iter, SION_WINDOW_COL_REF, &mnt, -1);
		if (sion_backend_gvfs_is_mount(mnt))
		{
			gchar *uri, *name;

			sion_backend_gvfs_get_name_and_uri_from_mount(mnt, &name, &uri);

			if (get_bookmark_from_uri(window, uri) == NULL)
			{
				SionBookmark *bm = sion_bookmark_new_from_uri(name, uri);
				if (sion_bookmark_is_valid(bm))
				{
					GtkWidget *edit_dialog;

					/* show the bookmark edit dialog and add the bookmark only if it was
					 * not cancelled */
					edit_dialog = sion_bookmark_edit_dialog_new_with_bookmark(
						GTK_WINDOW(window), priv->settings, SION_BE_MODE_EDIT, bm);
					if (sion_bookmark_edit_dialog_run(SION_BOOKMARK_EDIT_DIALOG(edit_dialog)) ==
						GTK_RESPONSE_OK)
					{
						/* this fills the values of the dialog into 'bm' */
						g_object_set(edit_dialog, "bookmark-update", bm, NULL);

						g_ptr_array_add(sion_settings_get_bookmarks(priv->settings),
							g_object_ref(bm));
						sion_window_update_bookmarks(window);
						sion_settings_write(priv->settings, SION_SETTINGS_BOOKMARKS);
					}
					gtk_widget_destroy(edit_dialog);
				}
				g_object_unref(bm);
			}
			else
				verbose("Bookmark for %s already exists", uri);

			g_free(uri);
			g_free(name);
		}
	}
}


static void sion_window_show_systray_icon(SionWindow *window, gboolean show)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);

	gtk_status_icon_set_visible(priv->systray_icon, show);
}


static void sion_window_show_toolbar(SionWindow *window, gboolean show)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);

	if (show)
		gtk_widget_show(priv->toolbar);
	else
		gtk_widget_hide(priv->toolbar);
}


static void sion_window_set_toolbar_style(SionWindow *window, gint style)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);
	if (style == -1)
	{
		g_object_get(gtk_widget_get_settings(GTK_WIDGET(window)), "gtk-toolbar-style", &style, NULL);
		g_object_set(priv->settings, "toolbar-style", style, NULL);
	}
	else
	{
		gtk_toolbar_set_style(GTK_TOOLBAR(priv->toolbar), style);
	}
}


static void sion_window_set_toolbar_orientation(SionWindow *window, gint orientation)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);

	sion_toolbar_set_orientation(GTK_TOOLBAR(priv->toolbar), orientation);
	if (orientation == GTK_ORIENTATION_HORIZONTAL && priv->vbox != gtk_widget_get_parent(priv->toolbar))
	{
		gtk_container_remove(GTK_CONTAINER(priv->hbox), priv->toolbar);
		gtk_container_add(GTK_CONTAINER(priv->vbox), priv->toolbar);
		gtk_box_set_child_packing(GTK_BOX(priv->vbox), priv->toolbar, FALSE, FALSE, 0, GTK_PACK_START);
		gtk_box_reorder_child(GTK_BOX(priv->vbox), priv->toolbar, 1);
	}
	else if (orientation == GTK_ORIENTATION_VERTICAL && priv->hbox != gtk_widget_get_parent(priv->toolbar))
	{
		gtk_container_remove(GTK_CONTAINER(priv->vbox), priv->toolbar);
		gtk_container_add(GTK_CONTAINER(priv->hbox), priv->toolbar);
		gtk_box_set_child_packing(GTK_BOX(priv->hbox), priv->toolbar, FALSE, FALSE, 0, GTK_PACK_START);
	}
}


static void sion_window_set_view_mode(SionWindow *window, gint mode)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);

	if (mode == VIEW_MODE_ICONVIEW && priv->hbox != gtk_widget_get_parent(priv->swin_iconview))
	{
		gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview)));
		gtk_container_remove(GTK_CONTAINER(priv->hbox), priv->swin_treeview);
		gtk_container_add(GTK_CONTAINER(priv->hbox), priv->swin_iconview);
	}
	else if (mode == VIEW_MODE_TREEVIEW &&  priv->hbox != gtk_widget_get_parent(priv->swin_treeview))
	{
		gtk_icon_view_unselect_all(GTK_ICON_VIEW(priv->iconview));
		gtk_container_remove(GTK_CONTAINER(priv->hbox), priv->swin_iconview);
		gtk_container_add(GTK_CONTAINER(priv->hbox), priv->swin_treeview);
	}
}


static void sion_window_settings_notify_cb(SionSettings *settings, GParamSpec *pspec, SionWindow *window)
{
	const gchar *name;
	GValue *value;

	name = g_intern_string(pspec->name);
	value = g_new0(GValue, 1);
	g_value_init(value, pspec->value_type);
	g_object_get_property(G_OBJECT(settings), name, value);

	if (name == g_intern_string("show-toolbar"))
		sion_window_show_toolbar(window, g_value_get_boolean(value));
	else if (name == g_intern_string("show-in-systray"))
		sion_window_show_systray_icon(window, g_value_get_boolean(value));
	else if (name == g_intern_string("toolbar-style"))
		sion_window_set_toolbar_style(window, g_value_get_int(value));
	else if (name == g_intern_string("toolbar-orientation"))
		sion_window_set_toolbar_orientation(window, g_value_get_int(value));
	else if (name == g_intern_string("view-mode"))
		sion_window_set_view_mode(window, g_value_get_int(value));
	else if (! g_object_class_find_property(G_OBJECT_GET_CLASS(settings), name))
		 verbose("Unexpected setting '%s'", name);
	g_value_unset(value);
	g_free(value);
}


static void create_ui_elements(SionWindow *window, GtkUIManager *ui_manager)
{
	GError *error = NULL;
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);
	const gchar *ui_markup =
	"<ui>"
		"<menubar>"
			"<menu action='File'>"
				"<menuitem action='Quit'/>"
			"</menu>"
			"<menu action='Edit'>"
				"<menuitem action='EditBookmarks'/>"
				"<separator/>"
				"<menuitem action='Preferences'/>"
			"</menu>"
			"<menu action='Actions'>"
				"<menuitem action='Connect'/>"
				"<menuitem action='Disconnect'/>"
				"<menuitem action='Bookmarks'/>"
				"<separator/>"
				"<menuitem action='Open'/>"
				"<menuitem action='CopyURI'/>"
			"</menu>"
			"<menu action='Help'>"
				"<menuitem action='About'/>"
			"</menu>"
		"</menubar>"

		"<popup name='systraymenu'>"
			"<menuitem action='Connect'/>"
			"<menuitem action='Bookmarks'/>"
			"<separator/>"
			"<menuitem action='EditBookmarks'/>"
			"<menuitem action='Preferences'/>"
			"<separator/>"
			"<menuitem action='Quit'/>"
		"</popup>"

		"<popup name='treemenu'>"
			"<menuitem action='Open'/>"
			"<menuitem action='CopyURI'/>"
			"<menuitem action='CreateBookmark'/>"
			"<separator/>"
			"<menuitem action='Connect'/>"
			"<menuitem action='Disconnect'/>"
		"</popup>"

		"<toolbar>"
			"<toolitem action='Bookmarks'/>"
			"<toolitem action='Disconnect'/>"
			"<separator/>"
			"<toolitem action='EditBookmarks'/>"
			"<separator/>"
			"<toolitem action='Open'/>"
			"<separator/>"
			"<toolitem action='Quit'/>"
		"</toolbar>"
	"</ui>";
	const GtkActionEntry entries[] = {
		{ "File", NULL, N_("_File"), NULL, NULL, NULL },
		{ "Edit", NULL, N_("_Edit"), NULL, NULL, NULL },
		{ "Actions", NULL, N_("_Actions"), NULL, NULL, NULL },
		{ "Help", NULL, N_("_Help"), NULL, NULL, NULL },
		{ "Preferences", GTK_STOCK_PREFERENCES,
			NULL, "<Ctrl>p", NULL, G_CALLBACK(action_preferences_cb) },
		{ "CreateBookmark", GTK_STOCK_ADD,
			N_("Create _Bookmark"), "<Ctrl>n", NULL, G_CALLBACK(action_create_bookmark_cb) },
		{ "EditBookmarks", GTK_STOCK_EDIT,
			N_("_Edit Bookmarks"), "<Ctrl>b",
			N_("Open the bookmark manager to add, edit or delete bookmarks"),
			G_CALLBACK(action_bookmark_edit_cb) },
		{ "Connect", GTK_STOCK_CONNECT, NULL, NULL, NULL, G_CALLBACK(action_mount_cb) },
		{ "Disconnect", GTK_STOCK_DISCONNECT, NULL, NULL,
			N_("Disconnect the selected resource"), G_CALLBACK(action_unmount_cb) },
		{ "Open", GTK_STOCK_OPEN, NULL, "<Ctrl>o",
			N_("Open the selection resource with a file manager"), G_CALLBACK(action_open_cb) },
		{ "CopyURI", GTK_STOCK_COPY, N_("Copy URI"), "<Ctrl>c", NULL, G_CALLBACK(action_copy_uri_cb) },
		{ "Quit", GTK_STOCK_QUIT, NULL, "<Ctrl>q", N_("Quit Sion"), G_CALLBACK(action_quit_cb) },
		{ "About", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK(action_about_cb) }
	};
	const guint entries_n = G_N_ELEMENTS(entries);


	priv->action_bookmarks = sion_menu_button_action_new(
		"Bookmarks", _("_Bookmarks"), _("Choose a bookmark to connect to"),
		sion_find_icon_name("bookmark-new", GTK_STOCK_EDIT));
	g_signal_connect(priv->action_bookmarks, "item-clicked", G_CALLBACK(action_bookmark_activate_cb), window);
	g_signal_connect(priv->action_bookmarks, "button-clicked", G_CALLBACK(action_mount_cb), window);

	priv->action_group = gtk_action_group_new("UI");
	gtk_action_group_set_translation_domain(priv->action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(priv->action_group, entries, entries_n, window);
	gtk_action_group_add_action(priv->action_group, priv->action_bookmarks);
	gtk_ui_manager_insert_action_group(ui_manager, priv->action_group, 0);
	gtk_window_add_accel_group(GTK_WINDOW(window), gtk_ui_manager_get_accel_group(ui_manager));

	if (! gtk_ui_manager_add_ui_from_string(ui_manager, ui_markup, -1, &error))
	{
		verbose("User interface couldn't be created: %s", error->message);
		g_error_free(error);
	}
}


static void create_tree_view(SionWindow *window)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *sel;

	priv->treeview = gtk_tree_view_new();
	gtk_widget_set_has_tooltip(priv->treeview, TRUE);
	gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(priv->treeview), SION_WINDOW_COL_TOOLTIP);
	gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(priv->treeview), TRUE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(priv->treeview), FALSE);
	gtk_tree_sortable_set_sort_column_id(
		GTK_TREE_SORTABLE(priv->store), SION_WINDOW_COL_NAME, GTK_SORT_ASCENDING);

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));
	gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);

	if (gtk_check_version(2, 14, 0) == NULL)
	{
		renderer = gtk_cell_renderer_pixbuf_new();
		column = gtk_tree_view_column_new_with_attributes(NULL, renderer,
			"gicon", SION_WINDOW_COL_PIXBUF, NULL);
		gtk_tree_view_column_set_sort_indicator(column, FALSE);
		gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(column), TRUE);
		gtk_tree_view_append_column(GTK_TREE_VIEW(priv->treeview), column);
	}

	renderer = gtk_cell_renderer_toggle_new();
	column = gtk_tree_view_column_new_with_attributes(
		_("Mounted"), renderer, "active", SION_WINDOW_COL_IS_MOUNTED, NULL);
	gtk_tree_view_column_set_sort_indicator(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, SION_WINDOW_COL_IS_MOUNTED);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(column), TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->treeview), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
		_("Service Type"), renderer, "text", SION_WINDOW_COL_SCHEME, NULL);
	gtk_tree_view_column_set_sort_indicator(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, SION_WINDOW_COL_SCHEME);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(column), TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->treeview), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
		_("Name"), renderer, "text", SION_WINDOW_COL_NAME, NULL);
	gtk_tree_view_column_set_sort_indicator(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, SION_WINDOW_COL_NAME);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(column), TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->treeview), column);

	gtk_tree_view_set_model(GTK_TREE_VIEW(priv->treeview), GTK_TREE_MODEL(priv->store));

	g_signal_connect(sel, "changed", G_CALLBACK(tree_selection_changed_cb), window);
	g_signal_connect(priv->treeview, "realize", G_CALLBACK(tree_realize_cb), window);
	g_signal_connect(priv->treeview, "button-press-event", G_CALLBACK(tree_button_press_event_cb), window);
	g_signal_connect(priv->treeview, "row-activated", G_CALLBACK(tree_row_activated_cb), window);
}


static void create_icon_view(SionWindow *window)
{
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);
	GtkCellRenderer *renderer;

	priv->iconview = gtk_icon_view_new();
	gtk_widget_set_has_tooltip(priv->iconview, TRUE);
	gtk_icon_view_set_tooltip_column(GTK_ICON_VIEW(priv->iconview), SION_WINDOW_COL_TOOLTIP);
	gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(priv->iconview), GTK_SELECTION_SINGLE);
	gtk_icon_view_set_spacing(GTK_ICON_VIEW(priv->iconview), 3);
	gtk_icon_view_set_column_spacing(GTK_ICON_VIEW(priv->iconview), 30);
	gtk_icon_view_set_row_spacing(GTK_ICON_VIEW(priv->iconview), 30);

	renderer = gtk_cell_renderer_pixbuf_new();
    g_object_set(renderer,
		"stock-size", GTK_ICON_SIZE_DND,
		"follow-state", TRUE,
		"xalign", 0.5,
		"yalign", 1.0, NULL);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(priv->iconview), renderer, FALSE);
	if (gtk_check_version(2, 14, 0) == NULL)
		gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(priv->iconview), renderer,
			"gicon", SION_WINDOW_COL_PIXBUF);
	else
		gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(priv->iconview), renderer,
			"icon-name", SION_WINDOW_COL_ICON_NAME);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "xalign", 0.5, "yalign", 1.0, NULL);
	gtk_cell_layout_pack_end(GTK_CELL_LAYOUT(priv->iconview), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(priv->iconview), renderer,
		"text", SION_WINDOW_COL_NAME, NULL);

	gtk_icon_view_set_model(GTK_ICON_VIEW(priv->iconview), GTK_TREE_MODEL(priv->store));

	g_signal_connect(priv->iconview, "selection-changed", G_CALLBACK(iconview_selection_changed_cb), window);
	g_signal_connect(priv->iconview, "button-press-event", G_CALLBACK(iconview_button_press_event_cb), window);
	g_signal_connect(priv->iconview, "item-activated", G_CALLBACK(iconview_item_activated_cb), window);
}


static void sion_window_init(SionWindow *window)
{
	GtkWidget *menubar;
	GtkUIManager *ui_manager;
	SionWindowPrivate *priv = SION_WINDOW_GET_PRIVATE(window);

	priv->autoconnect_timeout_id = (guint) -1;

	gtk_window_set_title(GTK_WINDOW(window), _("Sion"));
	gtk_window_set_icon_name(GTK_WINDOW(window), sion_get_application_icon_name());
	gtk_window_set_default_size(GTK_WINDOW(window), 550, 350);

	/* Init liststore */
	priv->store = gtk_list_store_new(SION_WINDOW_N_COLUMNS,
		G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER,
		G_TYPE_INT, G_TYPE_ICON, G_TYPE_STRING, G_TYPE_STRING);

	create_tree_view(window);
	create_icon_view(window);

	priv->swin_treeview = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(priv->swin_treeview),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(priv->swin_treeview), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(priv->swin_treeview), priv->treeview);

	priv->swin_iconview = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(priv->swin_iconview),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(priv->swin_iconview), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(priv->swin_iconview), priv->iconview);

	/* Init the GVfs backend */
	priv->backend_gvfs = sion_backend_gvfs_new(priv->store);
	g_signal_connect(priv->backend_gvfs, "mounts-changed", G_CALLBACK(mounts_changed_cb), window);
	g_signal_connect(priv->backend_gvfs, "operation-failed",
		G_CALLBACK(mount_operation_failed_cb), window);

	/* UI Manager */
	ui_manager = gtk_ui_manager_new();
	create_ui_elements(window, ui_manager);
	menubar = gtk_ui_manager_get_widget(ui_manager, "/menubar");
	priv->toolbar = gtk_ui_manager_get_widget(ui_manager, "/toolbar");
	priv->systray_icon_popup_menu = gtk_ui_manager_get_widget(ui_manager, "/systraymenu");
	priv->tree_popup_menu = gtk_ui_manager_get_widget(ui_manager, "/treemenu");
	/* increase refcount to keep the widgets after the ui manager is destroyed */
	g_object_ref(priv->systray_icon_popup_menu);
	g_object_ref(priv->tree_popup_menu);
	g_object_ref(priv->toolbar);
	g_object_ref(priv->swin_treeview);
	g_object_ref(priv->swin_iconview);

	/* Buttons */
	priv->action_connect = gtk_action_group_get_action(priv->action_group, "Connect");
	priv->action_disconnect = gtk_action_group_get_action(priv->action_group, "Disconnect");
	priv->action_bookmark_create = gtk_action_group_get_action(priv->action_group, "CreateBookmark");
	priv->action_open = gtk_action_group_get_action(priv->action_group, "Open");
	priv->action_copyuri = gtk_action_group_get_action(priv->action_group, "CopyURI");

	/* Set the is-important property for some toolbar actions */
/*
	g_object_set(gtk_action_group_get_action(priv->action_group, "EditBookmarks"), "is-important", TRUE, NULL);
	g_object_set(priv->action_connect, "is-important", TRUE, NULL);
	g_object_set(priv->action_disconnect, "is-important", TRUE, NULL);
	g_object_set(priv->action_bookmarks, "is-important", TRUE, NULL);
*/
	/* Pack the widgets altogether */
	priv->vbox = gtk_vbox_new(FALSE, 0);
	priv->hbox = gtk_hbox_new(FALSE, 0);

	gtk_box_pack_start(GTK_BOX(priv->vbox), menubar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(priv->vbox), priv->toolbar, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(priv->hbox), priv->swin_iconview, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(priv->vbox), priv->hbox, TRUE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(window), priv->vbox);

	/* Show everything */
	gtk_widget_show_all(priv->vbox);
	gtk_widget_show_all(priv->swin_treeview);

	/* Status icon */
	priv->systray_icon = gtk_status_icon_new_from_icon_name(sion_get_application_icon_name());
	sion_status_icon_set_tooltip_text(priv->systray_icon, _("Sion"));
	g_signal_connect(priv->systray_icon, "activate", G_CALLBACK(systray_icon_activate_cb), window);
	g_signal_connect(priv->systray_icon, "popup-menu", G_CALLBACK(systray_icon_popup_menu_cb), window);

	g_object_unref(ui_manager);
}



GtkWidget *sion_window_new(SionSettings *settings)
{
	GtkWidget *window;
	SionWindowPrivate *priv;
	const gint *geo;

	window = g_object_new(SION_WINDOW_TYPE, NULL);
	priv = SION_WINDOW_GET_PRIVATE(window);
	priv->settings = settings;
	g_signal_connect(settings, "notify", G_CALLBACK(sion_window_settings_notify_cb), window);

	g_object_set(priv->action_bookmarks, "settings", settings, NULL);

	sion_window_show_systray_icon(SION_WINDOW(window), sion_settings_get_boolean(settings, "show-in-systray"));
	sion_window_show_toolbar(SION_WINDOW(window), sion_settings_get_boolean(settings, "show-toolbar"));
	sion_window_set_toolbar_style(SION_WINDOW(window), sion_settings_get_integer(settings, "toolbar-style"));
	sion_window_set_toolbar_orientation(SION_WINDOW(window),
		sion_settings_get_integer(settings, "toolbar-orientation"));
	sion_window_set_view_mode(SION_WINDOW(window), sion_settings_get_integer(settings, "view-mode"));

	if (sion_settings_get_boolean(settings, "save-geometry"))
	{
		geo = sion_settings_get_geometry(settings);
		if (geo != NULL && *geo != -1)
		{
			gtk_window_move(GTK_WINDOW(window), geo[0], geo[1]);
			gtk_window_set_default_size(GTK_WINDOW(window), geo[2], geo[3]);
			if (geo[4] == 1)
				gtk_window_maximize(GTK_WINDOW(window));
		}
	}

	mounts_changed_cb(NULL, SION_WINDOW(window));
	sion_window_update_bookmarks(SION_WINDOW(window));
	sion_window_do_autoconnect(SION_WINDOW(window));

	return window;
}


