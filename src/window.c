/*
 *      window.c
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
#include "backendgvfs.h"
#include "window.h"
#include "bookmarkdialog.h"
#include "bookmarkeditdialog.h"
#include "menubuttonaction.h"
#include "preferencesdialog.h"
#include "mountdialog.h"
#include "browsenetworkpanel.h"
#include "bookmarkpanel.h"


typedef struct _GigoloWindowPrivate			GigoloWindowPrivate;

#define GIGOLO_WINDOW_GET_PRIVATE(obj)		(G_TYPE_INSTANCE_GET_PRIVATE((obj),\
		GIGOLO_WINDOW_TYPE, GigoloWindowPrivate))

/* Returns: TRUE if @a ptr points to a non-zero value. */
#define NZV(ptr) \
	((ptr) && (ptr)[0])

struct _GigoloWindowPrivate
{
	GigoloSettings	*settings;
	GigoloBackendGVFS	*backend_gvfs;

	GtkWidget		*vbox;
	GtkWidget		*hbox_pane;
	GtkWidget		*hbox_view;

	GtkWidget		*panel_pane;
	GtkWidget		*browse_panel;
	GtkWidget		*bookmark_panel;
	GtkWidget		*notebook_panel;
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
	GtkAction		*action_open_terminal;
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


G_DEFINE_TYPE(GigoloWindow, gigolo_window, GTK_TYPE_WINDOW);


static void remove_autoconnect_timeout(GigoloWindow *window)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);

	if (priv->autoconnect_timeout_id != (guint) -1)
	{
		g_source_remove(priv->autoconnect_timeout_id);
		priv->autoconnect_timeout_id = (guint) -1;
	}
}


static void gigolo_window_destroy(GigoloWindow *window)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	gint geo[5];

	remove_autoconnect_timeout(window);

	if (gigolo_settings_get_boolean(priv->settings, "save-geometry"))
	{
		GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(window));
		gtk_window_get_position(GTK_WINDOW(window), &geo[0], &geo[1]);
		gtk_window_get_size(GTK_WINDOW(window), &geo[2], &geo[3]);
		if (gdk_window != NULL && gdk_window_get_state(gdk_window) & GDK_WINDOW_STATE_MAXIMIZED)
			geo[4] = 1;
		else
			geo[4] = 0;

		gigolo_settings_set_geometry(priv->settings, geo, 5);

		g_object_set(priv->settings, "panel-position",
			gtk_paned_get_position(GTK_PANED(priv->panel_pane)), NULL);
	}
	g_object_set(priv->settings, "last-panel-page",
		gtk_notebook_get_current_page(GTK_NOTEBOOK(priv->notebook_panel)), NULL);

	gtk_widget_destroy(priv->tree_popup_menu);
	gtk_widget_destroy(priv->systray_icon_popup_menu);
	gtk_widget_destroy(priv->swin_treeview);
	gtk_widget_destroy(priv->swin_iconview);
	g_object_unref(priv->toolbar);
	g_object_unref(priv->action_group);
	g_object_unref(priv->systray_icon);
	g_object_unref(priv->systray_icon_popup_menu);
	g_object_unref(priv->backend_gvfs);
	priv->backend_gvfs = NULL;

	gtk_widget_destroy(GTK_WIDGET(window));

	gtk_main_quit();
}


static gboolean gigolo_window_delete_event(GtkWidget *widget, G_GNUC_UNUSED GdkEventAny *event)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(widget);

	if (gigolo_settings_get_boolean(priv->settings, "show-in-systray"))
	{
		gtk_widget_hide(widget);
		return TRUE;
	}
	else
	{
		gigolo_window_destroy(GIGOLO_WINDOW(widget));
		return FALSE;
	}
}


static void gigolo_window_class_init(GigoloWindowClass *klass)
{
	GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS(klass);
	gtkwidget_class->delete_event = gigolo_window_delete_event;

	g_type_class_add_private(klass, sizeof(GigoloWindowPrivate));
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
								   guint activate_time, GigoloWindow *window)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);

	if (button == 3)
		gtk_menu_popup(GTK_MENU(priv->systray_icon_popup_menu), NULL, NULL, NULL, NULL,
			button, activate_time);
}


/* Convenience function to get the selected GtkTreeIter from the icon view or the treeview
 * whichever is currently used for display. */
static void get_selected_iter(GigoloWindow *window, GtkTreeIter *iter)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);

	g_return_if_fail(window != NULL);
	g_return_if_fail(iter != NULL);

	if (gigolo_settings_get_integer(priv->settings, "view-mode") == VIEW_MODE_TREEVIEW)
	{
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));
		gtk_tree_selection_get_selected(selection, NULL, iter);
	}
	else
	{
		GList *items = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(priv->iconview));
		GtkTreeModel *model = gtk_icon_view_get_model(GTK_ICON_VIEW(priv->iconview));

		if (items != NULL)
		{
			/* the selection mode is SINGLE, so the list should never have more than one entry
			 * and we simply choose the first item */
			gtk_tree_model_get_iter(model, iter, items->data);
		}
		g_list_foreach(items, (GFunc) gtk_tree_path_free, NULL);
		g_list_free(items);
	}
}


void gigolo_window_mount_from_bookmark(GigoloWindow *window, GigoloBookmark *bookmark,
									   gboolean show_dialog, gboolean show_errors)
{
	gchar *uri;
	GtkWidget *dialog = NULL;
	GigoloWindowPrivate *priv;

	g_return_if_fail(window != NULL);
	g_return_if_fail(bookmark != NULL);

	priv = GIGOLO_WINDOW_GET_PRIVATE(window);

	uri = gigolo_bookmark_get_uri_escaped(bookmark);

	if (show_dialog)
	{
		const gchar *name = gigolo_bookmark_get_name(bookmark);
		gchar *label;

		if (name == NULL || gigolo_str_equal(name, GIGOLO_BOOKMARK_NAME_NONE))
			name = uri;
		label = g_strdup_printf(_("Connecting to \"%s\""), name);

		dialog = gigolo_mount_dialog_new(GTK_WINDOW(window), label);
		gtk_widget_show_all(dialog);

		g_free(label);
	}

	gigolo_backend_gvfs_mount_uri(priv->backend_gvfs, uri, GTK_WINDOW(window), dialog, show_errors);

	if (gigolo_bookmark_get_autoconnect(bookmark))
		gigolo_bookmark_set_should_not_autoconnect(bookmark, FALSE);

	g_free(uri);
}


static void action_mount_cb(G_GNUC_UNUSED GtkAction *action, GigoloWindow *window)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(priv->store);
	gpointer vol;
	gint ref_type;
	gboolean handled = FALSE;

	get_selected_iter(window, &iter);
	if (gtk_list_store_iter_is_valid(priv->store, &iter))
	{
		gtk_tree_model_get(model, &iter,
			GIGOLO_WINDOW_COL_REF_TYPE, &ref_type,
			GIGOLO_WINDOW_COL_REF, &vol, -1);

		if (ref_type == GIGOLO_WINDOW_REF_TYPE_VOLUME)
			handled = gigolo_backend_gvfs_mount_volume(priv->backend_gvfs, GTK_WINDOW(window), vol);
	}

	if (! handled)
	{
		GigoloBookmark *bm = NULL;
		GtkWidget *dialog;

		dialog = gigolo_bookmark_edit_dialog_new(window, GIGOLO_BE_MODE_CONNECT);
		if (gigolo_bookmark_edit_dialog_run(GIGOLO_BOOKMARK_EDIT_DIALOG(dialog)) == GTK_RESPONSE_OK)
		{
			bm = gigolo_bookmark_new();
			/* this fills the values of the dialog into 'bm' */
			g_object_set(dialog, "bookmark-update", bm, NULL);

			gigolo_window_mount_from_bookmark(window, bm, TRUE, TRUE);

			g_object_unref(bm);
		}
		gtk_widget_destroy(dialog);
	}
}


static void action_preferences_cb(G_GNUC_UNUSED GtkAction *action, GigoloWindow *window)
{
	GtkWidget *dialog;
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);

	dialog = gigolo_preferences_dialog_new(GTK_WINDOW(window), priv->settings);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gigolo_window_do_autoconnect(window);
	gigolo_settings_write(priv->settings, GIGOLO_SETTINGS_PREFERENCES);

	gtk_widget_destroy(dialog);
}


static void action_toggle_view_cb(GtkToggleAction *action, GigoloWindow *window)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	gboolean active = gtk_toggle_action_get_active(action);
	const gchar *property = NULL;

	if (gtk_action_get_name(GTK_ACTION(action)) == g_intern_string("ShowPanel"))
		property = "show-panel";
	else if (gtk_action_get_name(GTK_ACTION(action)) == g_intern_string("ShowToolbar"))
		property = "show-toolbar";
	else if (gtk_action_get_name(GTK_ACTION(action)) == g_intern_string("ShowInSystray"))
		property = "show-in-systray";

	if (property != NULL)
		g_object_set(priv->settings, property, active, NULL);
}


static void action_view_mode_change_cb(G_GNUC_UNUSED GtkRadioAction *action,
									   GtkRadioAction *current, GigoloWindow *window)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	gint mode = gtk_radio_action_get_current_value(current);

	g_object_set(priv->settings, "view-mode", mode, NULL);
}


static void action_unmount_cb(G_GNUC_UNUSED GtkAction *action, GigoloWindow *window)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	GtkTreeIter iter;
	GigoloBookmark *bm;

	get_selected_iter(window, &iter);
	if (gtk_list_store_iter_is_valid(priv->store, &iter))
	{
		gpointer mnt;
		GtkTreeModel *model = GTK_TREE_MODEL(priv->store);

		gtk_tree_model_get(model, &iter, GIGOLO_WINDOW_COL_REF, &mnt, -1);
		if (gigolo_backend_gvfs_is_mount(mnt))
		{
			gchar *uri;
			gigolo_backend_gvfs_get_name_and_uri_from_mount(mnt, NULL, &uri);
			bm = gigolo_settings_get_bookmark_by_uri(priv->settings, uri);
			if (bm != NULL && gigolo_bookmark_get_autoconnect(bm))
			{	/* we don't want auto-connection to reconnect this bookmark right
				   after we unmount it. */
				gigolo_bookmark_set_should_not_autoconnect(bm, TRUE);
			}
			g_free(uri);

			gigolo_backend_gvfs_unmount_mount(priv->backend_gvfs, mnt, GTK_WINDOW(window));
		}
	}
}


static void action_quit_cb(G_GNUC_UNUSED GtkAction *action, GigoloWindow *window)
{
    gigolo_window_destroy(window);
}


static void action_bookmark_edit_cb(G_GNUC_UNUSED GtkAction *action, GigoloWindow *window)
{
	GtkWidget *dialog;
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);

	dialog = gigolo_bookmark_dialog_new(window);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gigolo_settings_write(priv->settings, GIGOLO_SETTINGS_BOOKMARKS);

	gtk_widget_destroy(dialog);
}


static void about_activate_link(G_GNUC_UNUSED GtkAboutDialog *dialog,
								const gchar *uri, G_GNUC_UNUSED gpointer data)
{
	gigolo_show_uri(uri);
}


static void action_about_cb(G_GNUC_UNUSED GtkAction *action, GigoloWindow *window)
{
    const gchar *authors[]= { "Enrico Tröger <enrico@xfce.org>", NULL };

	gtk_about_dialog_set_email_hook(about_activate_link, NULL, NULL);
	gtk_about_dialog_set_url_hook(about_activate_link, NULL, NULL);
	gtk_show_about_dialog(GTK_WINDOW(window),
		"authors", authors,
		"logo-icon-name", gigolo_get_application_icon_name(),
		"comments", _("A simple frontend to easily connect/mount to local and remote filesystems"),
		"copyright", _("Copyright 2008-2011 Enrico Tröger"),
		"website", "http://www.uvena.de/gigolo/",
		"version", VERSION,
		"translator-credits", _("translator-credits"),
		"license",  "Copyright 2008-2011 Enrico Tröger <enrico@xfce.org>\n\n"
					"This program is free software; you can redistribute it and/or modify\n"
					"it under the terms of the GNU General Public License as published by\n"
					"the Free Software Foundation; version 2 of the License.\n"
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


static void action_help_cb(G_GNUC_UNUSED GtkAction *action, G_GNUC_UNUSED GigoloWindow *window)
{
	gigolo_show_uri("http://www.uvena.de/gigolo/help.html");
}


static void action_supported_schemes_cb(G_GNUC_UNUSED GtkAction *action, GigoloWindow *window)
{
	const gchar* const *supported;
	const gchar *description;
	GString *str;
	GtkWidget *dialog;
	guint j;

	str = g_string_new(_("Gigolo can use the following protocols provided by GVfs:"));
	g_string_append(str, "\n\n");

	supported = gigolo_backend_gvfs_get_supported_uri_schemes();
	for (j = 0; supported[j] != NULL; j++)
	{
		description = gigolo_describe_scheme(supported[j]);
		if (description != NULL)
		{
			g_string_append_printf(str, "%s (%s)", description, supported[j]);
			g_string_append_c(str, '\n');
		}
	}

	dialog = gtk_message_dialog_new(GTK_WINDOW(window),
		GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
		GTK_BUTTONS_OK, "%s", str->str);

	gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);
	g_string_free(str, TRUE);
}


static void action_copy_uri_cb(G_GNUC_UNUSED GtkAction *action, GigoloWindow *window)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(priv->store);

	get_selected_iter(window, &iter);
	if (gtk_list_store_iter_is_valid(priv->store, &iter))
	{
		gpointer mnt;

		gtk_tree_model_get(model, &iter, GIGOLO_WINDOW_COL_REF, &mnt, -1);
		if (gigolo_backend_gvfs_is_mount(mnt))
		{
			gchar *uri;
			GigoloBookmark *b;

			gigolo_backend_gvfs_get_name_and_uri_from_mount(mnt, NULL, &uri);

			b = gigolo_settings_get_bookmark_by_uri(priv->settings, uri);
			if (b != NULL)
			{
				gchar *folder = gigolo_bookmark_get_folder_expanded(b);
				setptr(uri, g_build_filename(uri, folder, NULL));
				g_free(folder);
			}

			gtk_clipboard_set_text(gtk_clipboard_get(gdk_atom_intern("CLIPBOARD", FALSE)), uri, -1);

			g_free(uri);
		}
	}
}


static gpointer get_selected_mount(GigoloWindow *window)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(priv->store);

	get_selected_iter(window, &iter);
	if (gtk_list_store_iter_is_valid(priv->store, &iter))
	{
		gpointer mnt;
		gtk_tree_model_get(model, &iter, GIGOLO_WINDOW_COL_REF, &mnt, -1);
		if (gigolo_backend_gvfs_is_mount(mnt))
		{
			return mnt;
		}
	}
	return NULL;
}


static void action_open_cb(G_GNUC_UNUSED GtkAction *action, GigoloWindow *window)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	gpointer mnt;

	if (! gigolo_settings_has_file_manager(priv->settings))
		return;

	mnt = get_selected_mount(window);
	if (mnt != NULL)
	{
		GError *error = NULL;
		gchar *uri;
		gchar *file_manager;
		gchar *cmd;
		GigoloBookmark *b;

		file_manager = gigolo_settings_get_string(priv->settings, "file-manager");
		gigolo_backend_gvfs_get_name_and_uri_from_mount(mnt, NULL, &uri);
		b = gigolo_settings_get_bookmark_by_uri(priv->settings, uri);
		if (b != NULL)
		{
			gchar *folder = gigolo_bookmark_get_folder_expanded(b);
			setptr(uri, g_build_filename(uri, folder, NULL));
			g_free(folder);
		}
		/* escape spaces and similar */
		setptr(uri, g_uri_unescape_string(uri, G_URI_RESERVED_CHARS_ALLOWED_IN_USERINFO));

		cmd = g_strconcat(file_manager, " \"", uri, "\"", NULL);
		verbose("Executing open command \"%s\"", cmd);
		if (! g_spawn_command_line_async(cmd, &error))
		{
			gchar *msg = g_strdup_printf(_("The command '%s' failed"), cmd);
			gigolo_message_dialog(window, GTK_MESSAGE_ERROR, _("Error"), msg, error->message);
			verbose("%s: %s", msg, error->message);
			g_error_free(error);
			g_free(msg);
		}

		g_free(cmd);
		g_free(file_manager);
		g_free(uri);
	}
}


static void action_open_terminal_cb(G_GNUC_UNUSED GtkAction *action, GigoloWindow *window)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	gpointer mnt;

	if (! gigolo_settings_has_terminal(priv->settings))
		return;

	mnt = get_selected_mount(window);
	if (mnt != NULL)
	{
		GError *error = NULL;
		gchar **argv;
		gchar *path;
		gchar *terminal;

		terminal = gigolo_settings_get_string(priv->settings, "terminal");
		if (! g_shell_parse_argv(terminal, NULL, &argv, &error))
		{
			gigolo_message_dialog(window, GTK_MESSAGE_ERROR,
				_("Error"), _("Invalid terminal command"), error->message);
			verbose("Invalid erminal command: %s", error->message);
			g_error_free(error);
			return;
		}

		path = gigolo_backend_gvfs_get_mount_path(mnt);
		if (path == NULL)
		{
			gchar *mount_name;
			gchar *msg;
			gigolo_backend_gvfs_get_name_and_uri_from_mount(mnt, &mount_name, NULL);
			msg = g_strdup_printf(_("No default location available for \"%s\""), mount_name);
			gigolo_message_dialog(window, GTK_MESSAGE_ERROR, _("Error"), msg, NULL);
			verbose("Mount has no default path: %s", mount_name);
			g_free(msg);
			g_free(mount_name);
			g_free(terminal);
			g_strfreev(argv);
			return;
		}

		verbose("Executing terminal command \"%s\" in \"%s\"", terminal, path);
		if (! g_spawn_async(path, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error))
		{
			gchar *msg = g_strdup_printf(_("The command '%s' failed"), terminal);
			gigolo_message_dialog(window, GTK_MESSAGE_ERROR, _("Error"), msg, error->message);
			verbose("%s: %s", msg, error->message);
			g_error_free(error);
			g_free(msg);
		}

		g_free(path);
		g_free(terminal);
		g_strfreev(argv);
	}
}


static void tree_realize_cb(GtkWidget *widget)
{
	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(widget));
}


static gboolean iter_is_bookmark(GigoloWindow *window, GtkTreeModel *model, GtkTreeIter *iter)
{
	gint ref_type;
	gpointer ref;

	gtk_tree_model_get(model, iter, GIGOLO_WINDOW_COL_REF_TYPE, &ref_type,
									GIGOLO_WINDOW_COL_REF, &ref, -1);

	if (ref_type == GIGOLO_WINDOW_REF_TYPE_MOUNT && gigolo_backend_gvfs_is_mount(ref))
	{
		gchar *uri;
		gboolean found = FALSE;
		GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);

		gigolo_backend_gvfs_get_name_and_uri_from_mount(ref, NULL, &uri);

		found = (gigolo_settings_get_bookmark_by_uri(priv->settings, uri) != NULL);

		g_free(uri);
		return found;
	}

	return TRUE;
}


static gboolean iter_is_mount(GtkTreeModel *model, GtkTreeIter *iter)
{
	gint ref_type;
	gpointer ref;

	gtk_tree_model_get(model, iter, GIGOLO_WINDOW_COL_REF_TYPE, &ref_type,
									GIGOLO_WINDOW_COL_REF, &ref, -1);

	if (ref_type == GIGOLO_WINDOW_REF_TYPE_MOUNT)
	{
		return gigolo_backend_gvfs_is_mount(ref);
	}

	return FALSE;
}


static void update_create_edit_bookmark_action_label(GtkAction *action, gboolean is_bookmark)
{
	gtk_action_set_sensitive(action, TRUE);
	if (is_bookmark)
		gtk_action_set_label(action, _("Edit _Bookmark"));
	else
		gtk_action_set_label(action, _("Create _Bookmark"));
}


static void update_sensitive_buttons(GigoloWindow *window, GtkTreeModel *model, GtkTreeIter *iter)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	gint ref_type;
	gboolean is_bookmark,is_mount, open_possible, open_terminal_possible;

	if (iter != NULL && gtk_list_store_iter_is_valid(priv->store, iter))
	{
		gtk_tree_model_get(model, iter, GIGOLO_WINDOW_COL_REF_TYPE, &ref_type, -1);
		is_bookmark = iter_is_bookmark(window, model, iter);
		is_mount = iter_is_mount(model, iter);
		open_possible = is_mount && gigolo_settings_has_file_manager(priv->settings);
		open_terminal_possible = is_mount && gigolo_settings_has_terminal(priv->settings);

		/* gtk_action_set_sensitive(priv->action_connect, (ref_type != GIGOLO_WINDOW_REF_TYPE_MOUNT));*/
		gtk_action_set_sensitive(priv->action_disconnect, (ref_type == GIGOLO_WINDOW_REF_TYPE_MOUNT));
		update_create_edit_bookmark_action_label(priv->action_bookmark_create, is_bookmark);
		gtk_action_set_sensitive(priv->action_open, open_possible);
		gtk_action_set_sensitive(priv->action_open_terminal, open_terminal_possible);
		gtk_action_set_sensitive(priv->action_copyuri, (ref_type == GIGOLO_WINDOW_REF_TYPE_MOUNT));
	}
	else
	{
		/* gtk_action_set_sensitive(priv->action_connect, FALSE); */
		gtk_action_set_sensitive(priv->action_disconnect, FALSE);
		gtk_action_set_sensitive(priv->action_bookmark_create, FALSE);
		gtk_action_set_sensitive(priv->action_open, FALSE);
		gtk_action_set_sensitive(priv->action_open_terminal, FALSE);
		gtk_action_set_sensitive(priv->action_copyuri, FALSE);
	}
}


static void tree_selection_changed_cb(GtkTreeSelection *selection, GigoloWindow *window)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);

	if (selection == NULL)
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));

	gtk_tree_selection_get_selected(selection, &model, &iter);

	update_sensitive_buttons(window, model, &iter);
}


static void iconview_selection_changed_cb(GtkIconView *view, GigoloWindow *window)
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


static void mounts_changed_cb(G_GNUC_UNUSED GigoloBackendGVFS *backend, GigoloWindow *window)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	gint view_mode = gigolo_settings_get_integer(priv->settings, "view-mode");

	if (view_mode == VIEW_MODE_ICONVIEW)
	{
		iconview_selection_changed_cb(GTK_ICON_VIEW(priv->iconview), window);
	}
	else if (view_mode == VIEW_MODE_TREEVIEW)
	{
		tree_selection_changed_cb(NULL, window);
	}
}


static void mount_operation_failed_cb(G_GNUC_UNUSED GigoloBackendGVFS *backend, const gchar *message,
								   const gchar *error_message, GigoloWindow *window)
{
	gigolo_message_dialog(window, GTK_MESSAGE_ERROR, _("Error"), message, error_message);
}


static void tree_row_activated_cb(G_GNUC_UNUSED GtkTreeView *treeview, GtkTreePath *path,
								  G_GNUC_UNUSED GtkTreeViewColumn *arg2, GigoloWindow *window)
{
	GtkTreeIter iter;
	gint ref_type;
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);

	if (gtk_tree_model_get_iter(GTK_TREE_MODEL(priv->store), &iter, path))
	{
		gtk_tree_model_get(GTK_TREE_MODEL(priv->store), &iter,
			GIGOLO_WINDOW_COL_REF_TYPE, &ref_type, -1);
		if (ref_type == GIGOLO_WINDOW_REF_TYPE_MOUNT)
		{	/* action_unmount_cb(NULL, data); */
			action_open_cb(NULL, window);
		}
		else
		{
			action_mount_cb(NULL, window);
		}
	}
}


static void iconview_item_activated_cb(G_GNUC_UNUSED GtkIconView *iconview,
									   GtkTreePath *path, GigoloWindow *window)
{
	tree_row_activated_cb(NULL, path, NULL, window);
}


static gboolean tree_button_press_event_cb(G_GNUC_UNUSED GtkWidget *widget,
										   GdkEventButton *event, GigoloWindow *window)
{
	if (event->button == 3)
	{
		GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
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


static gboolean iconview_button_press_event_cb(GtkWidget *widget, GdkEventButton *event, GigoloWindow *window)
{
	if (event->button == 3)
	{
		GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
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


static void action_bookmark_activate_cb(G_GNUC_UNUSED GigoloMenubuttonAction *action,
										GtkWidget *item, GigoloWindow *window)
{
	GigoloBookmark *bm = g_object_get_data(G_OBJECT(item), "bookmark");

	gigolo_window_mount_from_bookmark(window, bm, TRUE, TRUE);
}


static gint sort_bookmarks(gconstpointer a, gconstpointer b)
{
	GigoloBookmark *bm_a = GIGOLO_BOOKMARK(((GPtrArray*)a)->pdata);
	GigoloBookmark *bm_b = GIGOLO_BOOKMARK(((GPtrArray*)b)->pdata);
	const gchar *name_a = gigolo_bookmark_get_name(bm_a);
	const gchar *name_b = gigolo_bookmark_get_name(bm_b);

	return g_strcmp0(name_a, name_b);
}


void gigolo_window_update_bookmarks(GigoloWindow *window)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	GigoloBookmarkList *bookmarks = gigolo_settings_get_bookmarks(priv->settings);

	/* sort the bookmarks */
	g_ptr_array_sort(bookmarks, sort_bookmarks);

	/* writing to the 'settings' property will update the menus */
	g_object_set(priv->action_bookmarks, "settings", priv->settings, NULL);
	g_object_set(priv->bookmark_panel, "settings", priv->settings, NULL);

	/* update the popup menu items */
	tree_selection_changed_cb(NULL, window);

	gigolo_backend_gvfs_update_mounts_and_volumes(priv->backend_gvfs);
}


gboolean gigolo_window_do_autoconnect(gpointer data)
{
	GigoloWindow *window = GIGOLO_WINDOW(data);
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	GigoloBookmarkList *bookmarks = gigolo_settings_get_bookmarks(priv->settings);
	static gint old_interval = -1;
	gint interval;
	guint i;
	gboolean show_errors;

	interval = gigolo_settings_get_integer(priv->settings, "autoconnect-interval");
	if (old_interval != interval)
	{
		if (priv->autoconnect_timeout_id != (guint) -1)
			remove_autoconnect_timeout(window);

		priv->autoconnect_timeout_id = g_timeout_add_seconds(
			interval, gigolo_window_do_autoconnect, data);
		old_interval = interval;
	}

	if (interval == 0)
	{
		remove_autoconnect_timeout(window);
		return FALSE;
	}

	show_errors = gigolo_settings_get_boolean(priv->settings, "show-autoconnect-errors");
	for (i = 0; i < bookmarks->len; i++)
	{
		GigoloBookmark *bm = g_ptr_array_index(bookmarks, i);
		if (gigolo_bookmark_get_autoconnect(bm) && ! gigolo_bookmark_get_should_not_autoconnect(bm))
		{
			gigolo_window_mount_from_bookmark(window, bm, FALSE, show_errors);
		}
	}
	return TRUE;
}


static void action_create_bookmark_cb(G_GNUC_UNUSED GtkAction *button, GigoloWindow *window)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(priv->store);

	get_selected_iter(window, &iter);
	if (gtk_list_store_iter_is_valid(priv->store, &iter))
	{
		gpointer mnt;

		gtk_tree_model_get(model, &iter, GIGOLO_WINDOW_COL_REF, &mnt, -1);
		if (gigolo_backend_gvfs_is_mount(mnt))
		{
			gchar *uri, *name;
			GigoloBookmark *bm;
			GtkWidget *edit_dialog;

			gigolo_backend_gvfs_get_name_and_uri_from_mount(mnt, &name, &uri);

			bm = gigolo_settings_get_bookmark_by_uri(priv->settings, uri);
			if (bm == NULL)
			{
				bm = gigolo_bookmark_new_from_uri(name, uri);
				if (gigolo_bookmark_is_valid(bm))
				{
					/* show the bookmark edit dialog and add the bookmark only if it was
					 * not cancelled */
					edit_dialog = gigolo_bookmark_edit_dialog_new_with_bookmark(
						window, GIGOLO_BE_MODE_EDIT, bm);
					if (gigolo_bookmark_edit_dialog_run(
							GIGOLO_BOOKMARK_EDIT_DIALOG(edit_dialog)) == GTK_RESPONSE_OK)
					{
						/* this fills the values of the dialog into 'bm' */
						g_object_set(edit_dialog, "bookmark-update", bm, NULL);

						g_ptr_array_add(gigolo_settings_get_bookmarks(priv->settings),
							g_object_ref(bm));
						gigolo_window_update_bookmarks(window);
						gigolo_settings_write(priv->settings, GIGOLO_SETTINGS_BOOKMARKS);
					}
					gtk_widget_destroy(edit_dialog);
				}
				g_object_unref(bm);
			}
			else
			{
				/* bookmark exists */
				edit_dialog = gigolo_bookmark_edit_dialog_new_with_bookmark(
					window, GIGOLO_BE_MODE_EDIT, bm);
				if (gigolo_bookmark_edit_dialog_run(
						GIGOLO_BOOKMARK_EDIT_DIALOG(edit_dialog)) == GTK_RESPONSE_OK)
				{
					/* this fills the values of the dialog into 'bm' */
					g_object_set(edit_dialog, "bookmark-update", bm, NULL);

					gigolo_window_update_bookmarks(window);
					gigolo_settings_write(priv->settings, GIGOLO_SETTINGS_BOOKMARKS);
				}
				gtk_widget_destroy(edit_dialog);
			}
			g_free(uri);
			g_free(name);
		}
	}
}


static void gigolo_window_show_side_panel(GigoloWindow *window, gboolean show)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);

	if (show)
		gtk_widget_show(priv->hbox_pane);
	else
		gtk_widget_hide(priv->hbox_pane);
}


static void gigolo_window_show_systray_icon(GigoloWindow *window, gboolean show)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);

	gtk_status_icon_set_visible(priv->systray_icon, show);
}


static void gigolo_window_show_toolbar(GigoloWindow *window, gboolean show)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);

	if (show)
		gtk_widget_show(priv->toolbar);
	else
		gtk_widget_hide(priv->toolbar);
}


static void gigolo_window_set_toolbar_style(GigoloWindow *window, gint style)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
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


static void gigolo_window_set_toolbar_orientation(GigoloWindow *window, gint orientation)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);

	gtk_orientable_set_orientation(GTK_ORIENTABLE(priv->toolbar), orientation);
	if (orientation == GTK_ORIENTATION_HORIZONTAL && priv->vbox != gtk_widget_get_parent(priv->toolbar))
	{
		gtk_container_remove(GTK_CONTAINER(priv->hbox_view), priv->toolbar);
		gtk_container_add(GTK_CONTAINER(priv->vbox), priv->toolbar);
		gtk_box_set_child_packing(GTK_BOX(priv->vbox), priv->toolbar, FALSE, FALSE, 0, GTK_PACK_START);
		gtk_box_reorder_child(GTK_BOX(priv->vbox), priv->toolbar, 1);
	}
	else if (orientation == GTK_ORIENTATION_VERTICAL && priv->hbox_view != gtk_widget_get_parent(priv->toolbar))
	{
		gtk_container_remove(GTK_CONTAINER(priv->vbox), priv->toolbar);
		gtk_container_add(GTK_CONTAINER(priv->hbox_view), priv->toolbar);
		gtk_box_set_child_packing(GTK_BOX(priv->hbox_view), priv->toolbar, FALSE, FALSE, 0, GTK_PACK_START);
	}
}


static void gigolo_window_set_view_mode(GigoloWindow *window, gint mode)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);

	if (mode == VIEW_MODE_ICONVIEW && priv->hbox_view != gtk_widget_get_parent(priv->swin_iconview))
	{
		gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview)));
		gtk_container_remove(GTK_CONTAINER(priv->hbox_view), priv->swin_treeview);
		gtk_container_add(GTK_CONTAINER(priv->hbox_view), priv->swin_iconview);
	}
	else if (mode == VIEW_MODE_TREEVIEW &&  priv->hbox_view != gtk_widget_get_parent(priv->swin_treeview))
	{
		gtk_icon_view_unselect_all(GTK_ICON_VIEW(priv->iconview));
		gtk_container_remove(GTK_CONTAINER(priv->hbox_view), priv->swin_iconview);
		gtk_container_add(GTK_CONTAINER(priv->hbox_view), priv->swin_treeview);
	}
}


static void toggle_action_set_active(GigoloWindow *window, const gchar *name, gboolean set)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	GtkAction *action = gtk_action_group_get_action(priv->action_group, name);

	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), set);
}


static void view_mode_action_set_active(GigoloWindow *window, gint val)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	GtkAction *action = gtk_action_group_get_action(priv->action_group, "ViewSymbols");

	gtk_radio_action_set_current_value(GTK_RADIO_ACTION(action), val);
}


static void gigolo_window_systray_notify_cb(GtkStatusIcon *sicon, GParamSpec *pspec, GtkWindow *window)
{
	const gchar *name;
	GValue *value;

	/* nothing to do if the main window is visible */
	if (gtk_window_is_active(window))
		return;

	name = g_intern_string(pspec->name);
	value = g_new0(GValue, 1);
	g_value_init(value, pspec->value_type);

	if (name == g_intern_string("embedded"))
	{
		g_object_get_property(G_OBJECT(sicon), name, value);
		/* if the icon is not embedded anymore, we display the main window otherwise
		 * it will be lost */
		if (! g_value_get_boolean(value))
		{
			gtk_window_present(window);
		}
	}
}


static void gigolo_window_settings_notify_cb(GigoloSettings *settings, GParamSpec *pspec, GigoloWindow *window)
{
	const gchar *name;
	GValue *value;

	name = g_intern_string(pspec->name);
	value = g_new0(GValue, 1);
	g_value_init(value, pspec->value_type);
	g_object_get_property(G_OBJECT(settings), name, value);

	if (name == g_intern_string("show-toolbar"))
	{
		gboolean state = g_value_get_boolean(value);
		gigolo_window_show_toolbar(window, state);
		toggle_action_set_active(window, "ShowToolbar", state);
	}
	else if (name == g_intern_string("show-in-systray"))
	{
		gboolean state = g_value_get_boolean(value);
		gigolo_window_show_systray_icon(window, state);
		toggle_action_set_active(window, "ShowInSystray", state);
	}
	else if (name == g_intern_string("toolbar-style"))
		gigolo_window_set_toolbar_style(window, g_value_get_int(value));
	else if (name == g_intern_string("toolbar-orientation"))
		gigolo_window_set_toolbar_orientation(window, g_value_get_int(value));
	else if (name == g_intern_string("view-mode"))
	{
		gint mode = g_value_get_int(value);
		gigolo_window_set_view_mode(window, mode);
		view_mode_action_set_active(GIGOLO_WINDOW(window), mode);
	}
	else if (name == g_intern_string("show-panel"))
	{
		gboolean state = g_value_get_boolean(value);
		gigolo_window_show_side_panel(window, state);
		toggle_action_set_active(window, "ShowPanel", state);
	}
	else if (! g_object_class_find_property(G_OBJECT_GET_CLASS(settings), name))
		 verbose("Unexpected setting '%s'", name);
	g_value_unset(value);
	g_free(value);
}


static void create_ui_elements(GigoloWindow *window, GtkUIManager *ui_manager)
{
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
			"<menu action='View'>"
				"<menuitem action='ShowToolbar'/>"
				"<menuitem action='ShowPanel'/>"
				"<menuitem action='ShowInSystray'/>"
				"<separator/>"
				"<menuitem action='ViewDetailed'/>"
				"<menuitem action='ViewSymbols'/>"
			"</menu>"
			"<menu action='Actions'>"
				"<menuitem action='Connect'/>"
				"<menuitem action='Disconnect'/>"
				"<menuitem action='Bookmarks'/>"
				"<separator/>"
				"<menuitem action='Open'/>"
				"<menuitem action='OpenTerminal'/>"
				"<menuitem action='CopyURI'/>"
			"</menu>"
			"<menu action='Help'>"
				"<menuitem action='OnlineHelp'/>"
				"<menuitem action='SupportedSchemes'/>"
				"<separator/>"
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
			"<menuitem action='OpenTerminal'/>"
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
			"<toolitem action='OpenTerminal'/>"
			"<separator/>"
			"<toolitem action='Quit'/>"
		"</toolbar>"
	"</ui>";
	const GtkActionEntry entries[] = {
		{ "File", NULL, N_("_File"), NULL, NULL, NULL },
		{ "Edit", NULL, N_("_Edit"), NULL, NULL, NULL },
		{ "Actions", NULL, N_("_Actions"), NULL, NULL, NULL },
		{ "View", NULL, N_("_View"), NULL, NULL, NULL },
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
			N_("Open the selected resource with a file manager"), G_CALLBACK(action_open_cb) },
		{ "OpenTerminal", NULL, _("Open in _Terminal"), "<Ctrl>t",
			N_("Start a terminal from here"), G_CALLBACK(action_open_terminal_cb) },
		{ "CopyURI", GTK_STOCK_COPY, N_("Copy _URI"), "<Ctrl>c", NULL, G_CALLBACK(action_copy_uri_cb) },
		{ "Quit", GTK_STOCK_QUIT, NULL, "<Ctrl>q", N_("Quit Gigolo"), G_CALLBACK(action_quit_cb) },
		{ "OnlineHelp", GTK_STOCK_HELP, _("Online Help"), NULL, NULL, G_CALLBACK(action_help_cb) },
		{ "SupportedSchemes", NULL, _("Supported Protocols"), NULL, NULL, G_CALLBACK(action_supported_schemes_cb) },
		{ "About", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK(action_about_cb) }
	};
	const guint entries_n = G_N_ELEMENTS(entries);

	const GtkToggleActionEntry toggle_entries[] = {
		{ "ShowPanel", NULL, N_("Side _Panel"),
			NULL, NULL, G_CALLBACK(action_toggle_view_cb), FALSE },
		{ "ShowToolbar", NULL, N_("_Toolbar"),
			NULL, NULL, G_CALLBACK(action_toggle_view_cb), FALSE },
		{ "ShowInSystray", NULL, N_("Status _Icon"),
			NULL, NULL, G_CALLBACK(action_toggle_view_cb), FALSE }
	};
	const guint toggle_entries_n = G_N_ELEMENTS(toggle_entries);

	const GtkRadioActionEntry radio_entries[] = {
		{ "ViewDetailed", NULL, N_("View as _Symbols"), NULL, NULL, 0 },
		{ "ViewSymbols", NULL, N_("View as _Detailed List"), NULL, NULL, 1 },
	};
	const guint radio_entries_n = G_N_ELEMENTS(radio_entries);
	GError *error = NULL;
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	GtkAction *open_terminal;
	GtkIconTheme *icon_theme;

	priv->action_bookmarks = gigolo_menu_button_action_new(
		"Bookmarks", _("_Bookmarks"), _("Choose a bookmark to connect to"),
		gigolo_find_icon_name("bookmark-new", GTK_STOCK_EDIT));
	g_signal_connect(priv->action_bookmarks, "item-clicked",
		G_CALLBACK(action_bookmark_activate_cb), window);
	g_signal_connect(priv->action_bookmarks, "button-clicked", G_CALLBACK(action_mount_cb), window);

	priv->action_group = gtk_action_group_new("UI");
	gtk_action_group_set_translation_domain(priv->action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(priv->action_group, entries, entries_n, window);
	gtk_action_group_add_toggle_actions(priv->action_group, toggle_entries, toggle_entries_n, window);
	gtk_action_group_add_radio_actions(priv->action_group, radio_entries, radio_entries_n, -1,
		G_CALLBACK(action_view_mode_change_cb), window);
	gtk_action_group_add_action(priv->action_group, priv->action_bookmarks);
	gtk_ui_manager_insert_action_group(ui_manager, priv->action_group, 0);
	gtk_window_add_accel_group(GTK_WINDOW(window), gtk_ui_manager_get_accel_group(ui_manager));

	/* set terminal icon for the "OpenTerminal" action */
	open_terminal = gtk_action_group_get_action(priv->action_group, "OpenTerminal");
	icon_theme = gtk_icon_theme_get_for_screen(gtk_widget_get_screen(GTK_WIDGET(window)));
	if (gtk_icon_theme_has_icon(icon_theme, "utilities-terminal"))
		gtk_action_set_icon_name(open_terminal, "utilities-terminal");
	else
		gtk_action_set_stock_id(open_terminal, GTK_STOCK_OPEN);

	if (! gtk_ui_manager_add_ui_from_string(ui_manager, ui_markup, -1, &error))
	{
		verbose("User interface couldn't be created: %s", error->message);
		g_error_free(error);
	}
}


static void tree_mounted_col_toggled_cb(GtkCellRendererToggle *cell, gchar *pth, GigoloWindow *window)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	GtkTreeSelection *selection;
	GtkTreePath *path;

	path = gtk_tree_path_new_from_string(pth);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));

	gtk_tree_selection_select_path(selection, path);

	if (gtk_cell_renderer_toggle_get_active(cell))
		action_unmount_cb(NULL, window);
	else
		action_mount_cb(NULL, window);

	gtk_tree_path_free(path);
}


static void create_tree_view(GigoloWindow *window)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *sel;

	priv->treeview = gtk_tree_view_new();
	gtk_widget_set_has_tooltip(priv->treeview, TRUE);
	gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(priv->treeview), GIGOLO_WINDOW_COL_TOOLTIP);
	gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(priv->treeview), TRUE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(priv->treeview), FALSE);
	gtk_tree_sortable_set_sort_column_id(
		GTK_TREE_SORTABLE(priv->store), GIGOLO_WINDOW_COL_NAME, GTK_SORT_ASCENDING);

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));
	gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);

	if (gtk_check_version(2, 14, 0) == NULL)
	{
		renderer = gtk_cell_renderer_pixbuf_new();
		column = gtk_tree_view_column_new_with_attributes(NULL, renderer,
			"gicon", GIGOLO_WINDOW_COL_PIXBUF, NULL);
		gtk_tree_view_column_set_sort_indicator(column, FALSE);
		gtk_tree_view_column_set_resizable(column, TRUE);
		gtk_tree_view_append_column(GTK_TREE_VIEW(priv->treeview), column);
	}

	renderer = gtk_cell_renderer_toggle_new();
	column = gtk_tree_view_column_new_with_attributes(
		_("Connected"), renderer, "active", GIGOLO_WINDOW_COL_IS_MOUNTED, NULL);
	gtk_tree_view_column_set_sort_indicator(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, GIGOLO_WINDOW_COL_IS_MOUNTED);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->treeview), column);
	g_signal_connect(renderer, "toggled", G_CALLBACK(tree_mounted_col_toggled_cb), window);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
		_("Service Type"), renderer, "text", GIGOLO_WINDOW_COL_SCHEME, NULL);
	gtk_tree_view_column_set_sort_indicator(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, GIGOLO_WINDOW_COL_SCHEME);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->treeview), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
		_("Name"), renderer, "text", GIGOLO_WINDOW_COL_NAME, NULL);
	gtk_tree_view_column_set_sort_indicator(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, GIGOLO_WINDOW_COL_NAME);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->treeview), column);

	gtk_tree_view_set_model(GTK_TREE_VIEW(priv->treeview), GTK_TREE_MODEL(priv->store));

	g_signal_connect(sel, "changed", G_CALLBACK(tree_selection_changed_cb), window);
	g_signal_connect(priv->treeview, "realize", G_CALLBACK(tree_realize_cb), window);
	g_signal_connect(priv->treeview, "button-press-event", G_CALLBACK(tree_button_press_event_cb), window);
	g_signal_connect(priv->treeview, "row-activated", G_CALLBACK(tree_row_activated_cb), window);
}


static void create_icon_view(GigoloWindow *window)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	GtkCellRenderer *renderer;

	priv->iconview = gtk_icon_view_new();
	gtk_widget_set_has_tooltip(priv->iconview, TRUE);
	gtk_icon_view_set_tooltip_column(GTK_ICON_VIEW(priv->iconview), GIGOLO_WINDOW_COL_TOOLTIP);
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
			"gicon", GIGOLO_WINDOW_COL_PIXBUF);
	else
		gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(priv->iconview), renderer,
			"icon-name", GIGOLO_WINDOW_COL_ICON_NAME);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "xalign", 0.5, "yalign", 0.0, NULL);
	gtk_cell_layout_pack_end(GTK_CELL_LAYOUT(priv->iconview), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(priv->iconview), renderer,
		"text", GIGOLO_WINDOW_COL_NAME, NULL);

	gtk_icon_view_set_model(GTK_ICON_VIEW(priv->iconview), GTK_TREE_MODEL(priv->store));

	g_signal_connect(priv->iconview, "selection-changed",
		G_CALLBACK(iconview_selection_changed_cb), window);
	g_signal_connect(priv->iconview, "button-press-event",
		G_CALLBACK(iconview_button_press_event_cb), window);
	g_signal_connect(priv->iconview, "item-activated",
		G_CALLBACK(iconview_item_activated_cb), window);
}


static GtkWidget *gigolo_window_create_panel(GigoloWindow *window)
{
	GtkWidget *panel_pane, *label;
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);

	panel_pane = gtk_hpaned_new();
	gtk_paned_set_position(GTK_PANED(panel_pane), 200);

	priv->notebook_panel = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(priv->notebook_panel), GTK_POS_LEFT);
	gtk_widget_show(priv->notebook_panel);

	label = gtk_label_new(_("Bookmarks"));
	gtk_label_set_angle(GTK_LABEL(label), 90.0);
	gtk_widget_show(label);

	priv->bookmark_panel = gigolo_bookmark_panel_new(window);
	gtk_widget_show(priv->bookmark_panel);
	gtk_notebook_append_page(GTK_NOTEBOOK(priv->notebook_panel), priv->bookmark_panel, label);

	label = gtk_label_new(_("Network"));
	gtk_label_set_angle(GTK_LABEL(label), 90.0);
	gtk_widget_show(label);

	priv->browse_panel = gigolo_browse_network_panel_new(window);
	gtk_widget_show(priv->browse_panel);
	gtk_notebook_append_page(GTK_NOTEBOOK(priv->notebook_panel), priv->browse_panel, label);

	priv->hbox_view = gtk_hbox_new(FALSE, 0);
	priv->hbox_pane = gtk_hbox_new(FALSE, 0);

	gtk_paned_add1(GTK_PANED(panel_pane), priv->hbox_pane);
	gtk_paned_add2(GTK_PANED(panel_pane), priv->hbox_view);
	gtk_box_pack_start(GTK_BOX(priv->hbox_pane), priv->notebook_panel, TRUE, TRUE, 0);

	return panel_pane;
}


static void update_side_panel(GigoloWindow *window)
{
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	guint panel_position;

	if (! gigolo_backend_gvfs_is_scheme_supported("smb"))
		gtk_widget_destroy(priv->browse_panel);

	if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(priv->notebook_panel)) < 2)
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(priv->notebook_panel), FALSE);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->notebook_panel),
		gigolo_settings_get_integer(priv->settings, "last-panel-page"));

	panel_position = gigolo_settings_get_integer(priv->settings, "panel-position");
	if (panel_position <= 0)
		panel_position = 200;
	gtk_paned_set_position(GTK_PANED(priv->panel_pane), panel_position);
}


static void gigolo_window_init(GigoloWindow *window)
{
	GtkWidget *menubar, *panel_pane;
	GtkUIManager *ui_manager;
	GigoloWindowPrivate *priv = GIGOLO_WINDOW_GET_PRIVATE(window);

	priv->autoconnect_timeout_id = (guint) -1;

	gtk_window_set_title(GTK_WINDOW(window), _("Gigolo"));
	gtk_window_set_icon_name(GTK_WINDOW(window), gigolo_get_application_icon_name());
	gtk_window_set_default_size(GTK_WINDOW(window), 650, 350);

	/* Init liststore */
	priv->store = gtk_list_store_new(GIGOLO_WINDOW_N_COLUMNS,
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
	priv->backend_gvfs = gigolo_backend_gvfs_new();
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
	priv->action_open_terminal = gtk_action_group_get_action(priv->action_group, "OpenTerminal");
	priv->action_copyuri = gtk_action_group_get_action(priv->action_group, "CopyURI");

	g_object_set(priv->action_bookmarks, "is-important", TRUE, NULL);
	g_object_set(gtk_action_group_get_action(priv->action_group, "EditBookmarks"),
		"is-important", TRUE, NULL);

	/* Panel */
	priv->panel_pane = panel_pane = gigolo_window_create_panel(window);

	/* Pack the widgets altogether */
	priv->vbox = gtk_vbox_new(FALSE, 0);

	gtk_box_pack_start(GTK_BOX(priv->vbox), menubar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(priv->vbox), priv->toolbar, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(priv->hbox_view), priv->swin_iconview, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(priv->vbox), panel_pane, TRUE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(window), priv->vbox);

	/* Show everything */
	gtk_widget_show_all(priv->vbox);
	gtk_widget_show_all(priv->swin_treeview);

	/* Status icon */
	priv->systray_icon = gtk_status_icon_new_from_icon_name(gigolo_get_application_icon_name());
	gtk_status_icon_set_tooltip_text(priv->systray_icon, _("Gigolo"));
	g_signal_connect(priv->systray_icon, "activate", G_CALLBACK(systray_icon_activate_cb), window);
	g_signal_connect(priv->systray_icon, "popup-menu", G_CALLBACK(systray_icon_popup_menu_cb), window);
	g_signal_connect(priv->systray_icon, "notify", G_CALLBACK(gigolo_window_systray_notify_cb), window);

	g_object_unref(ui_manager);
}


GtkWidget *gigolo_window_new(GigoloSettings *settings)
{
	GtkWidget *window;
	GigoloWindowPrivate *priv;
	const gint *geo;
	gboolean state;
	gint value;

	window = g_object_new(GIGOLO_WINDOW_TYPE, NULL);
	priv = GIGOLO_WINDOW_GET_PRIVATE(window);
	priv->settings = settings;
	g_signal_connect(settings, "notify", G_CALLBACK(gigolo_window_settings_notify_cb), window);

	g_object_set(priv->action_bookmarks, "settings", settings, NULL);

	g_object_set(priv->backend_gvfs, "parent", window, "store", priv->store, NULL);

	gigolo_window_set_toolbar_style(GIGOLO_WINDOW(window),
		gigolo_settings_get_integer(settings, "toolbar-style"));
	gigolo_window_set_toolbar_orientation(GIGOLO_WINDOW(window),
		gigolo_settings_get_integer(settings, "toolbar-orientation"));
	/* Show Panel */
	state = gigolo_settings_get_boolean(settings, "show-panel");
	gigolo_window_show_side_panel(GIGOLO_WINDOW(window), state);
	toggle_action_set_active(GIGOLO_WINDOW(window), "ShowPanel", state);
	update_side_panel(GIGOLO_WINDOW(window));
	/* Show Toolbar */
	state = gigolo_settings_get_boolean(settings, "show-toolbar");
	gigolo_window_show_toolbar(GIGOLO_WINDOW(window), state);
	toggle_action_set_active(GIGOLO_WINDOW(window), "ShowToolbar", state);
	/* Show Status Icon */
	state = gigolo_settings_get_boolean(settings, "show-in-systray");
	gigolo_window_show_systray_icon(GIGOLO_WINDOW(window), state);
	toggle_action_set_active(GIGOLO_WINDOW(window), "ShowInSystray", state);
	/* View Mode */
	value = gigolo_settings_get_integer(settings, "view-mode");
	gigolo_window_set_view_mode(GIGOLO_WINDOW(window), value);
	view_mode_action_set_active(GIGOLO_WINDOW(window), value);

	if (gigolo_settings_get_boolean(settings, "save-geometry"))
	{
		geo = gigolo_settings_get_geometry(settings);
		if (geo != NULL && *geo != -1)
		{
			gtk_window_move(GTK_WINDOW(window), geo[0], geo[1]);
			gtk_window_set_default_size(GTK_WINDOW(window), geo[2], geo[3]);
			if (geo[4] == 1)
				gtk_window_maximize(GTK_WINDOW(window));
		}
	}

	mounts_changed_cb(NULL, GIGOLO_WINDOW(window));
	gigolo_window_update_bookmarks(GIGOLO_WINDOW(window));
	gigolo_window_do_autoconnect(GIGOLO_WINDOW(window));

	return window;
}


GigoloSettings *gigolo_window_get_settings(GigoloWindow *window)
{
	GigoloWindowPrivate *priv;

	g_return_val_if_fail(window != NULL, NULL);

	priv = GIGOLO_WINDOW_GET_PRIVATE(window);

	return priv->settings;
}


GigoloBackendGVFS *gigolo_window_get_backend(GigoloWindow *window)
{
	GigoloWindowPrivate *priv;

	g_return_val_if_fail(window != NULL, NULL);

	priv = GIGOLO_WINDOW_GET_PRIVATE(window);

	return priv->backend_gvfs;
}


