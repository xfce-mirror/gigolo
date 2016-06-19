/*
 *      main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <string.h>
#include <libintl.h>

#include "common.h"
#include "bookmark.h"
#include "settings.h"
#include "backendgvfs.h"
#include "window.h"


static int gigolo_create(GtkApplication *);

static gboolean show_version = FALSE;
static gboolean list_schemes = FALSE;
static gboolean new_instance = FALSE;
static gboolean auto_connect = FALSE;
extern gboolean verbose_mode;

static GOptionEntry cli_options[] =
{
	{ "auto-connect", 'a', 0, G_OPTION_ARG_NONE, &auto_connect, N_("Connect all bookmarks marked as 'auto connect' and exit"), NULL },
	{ "new-instance", 'i', 0, G_OPTION_ARG_NONE, &new_instance, N_("Ignore running instances, enforce opening a new instance"), NULL },
	{ "list-schemes", 'l', 0, G_OPTION_ARG_NONE, &list_schemes, N_("Print a list of supported URI schemes"), NULL },
	{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose_mode, N_("Be verbose"), NULL },
	{ "version", 'V', 0, G_OPTION_ARG_NONE, &show_version, N_("Show version information"), NULL },
	{ NULL, 0, 0, 0, NULL, NULL, NULL }
};


static gboolean auto_connect_bookmarks(void)
{
	GigoloBackendGVFS *backend_gvfs;
	GigoloSettings *settings;
	GigoloBookmarkList *bookmarks;
	GigoloBookmark *bm;
	guint i;
	gchar *uri;

	backend_gvfs = gigolo_backend_gvfs_new();
	settings = gigolo_settings_new();
	bookmarks = gigolo_settings_get_bookmarks(settings);

	for (i = 0; i < bookmarks->len; i++)
	{
		bm = g_ptr_array_index(bookmarks, i);
		if (gigolo_bookmark_get_autoconnect(bm) && ! gigolo_bookmark_get_should_not_autoconnect(bm))
		{
			uri = gigolo_bookmark_get_uri_escaped(bm);
			/* Mounting happens asynchronously here and so we don't wait until it is finished
			 * nor de we get any feedback or errors.
			 * TODO make this synchronous(looping and checking) and check for errors */
			gigolo_backend_gvfs_mount_uri(backend_gvfs, uri, NULL, NULL, FALSE);
			g_free(uri);
		}
	}

	return TRUE;
}


static void print_supported_schemes(void)
{
	const gchar* const *supported;
	gint j;

	supported = gigolo_backend_gvfs_get_supported_uri_schemes();
	for (j = 0; supported[j] != NULL; j++)
	{
		g_print("%s\n", supported[j]);
	}
}

static int activate (GtkApplication *app, gpointer user_data)
{
	GList *list;

	list = gtk_application_get_windows (app);
	if (list)
	{
		gtk_window_present (GTK_WINDOW (list->data));
	}
	else
		return gigolo_create(app);
	return 0;
}

gint main(gint argc, gchar** argv)
{
	GtkApplication *gis = NULL;
	gint status;
	GOptionContext *context;

	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	context = g_option_context_new(_("- a simple frontend to easily connect to remote filesystems"));
	g_option_context_add_main_entries(context, cli_options, GETTEXT_PACKAGE);
	g_option_group_set_translation_domain(g_option_context_get_main_group(context), GETTEXT_PACKAGE);
	g_option_context_add_group(context, gtk_get_option_group(FALSE));
	g_option_context_parse(context, &argc, &argv, NULL);
	g_option_context_free(context);

	if (show_version)
	{
		g_print("%s %s\n\n", PACKAGE, VERSION);
		g_print("%s\n", "Copyright (c) 2008-2011");
		g_print("\tEnrico Tröger <enrico@xfce.org>\n\n");
		g_print("\n");

		return EXIT_SUCCESS;
	}

	if (list_schemes)
	{
		print_supported_schemes();

		return EXIT_SUCCESS;
	}

	if (auto_connect)
	{
		gboolean ret = auto_connect_bookmarks();

		return ret ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	verbose("Gigolo %s (GTK+ %u.%u.%u, GLib %u.%u.%u)",
		VERSION,
		gtk_major_version, gtk_minor_version, gtk_micro_version,
		glib_major_version, glib_minor_version, glib_micro_version);

	if (! new_instance)
	{
		gis = gtk_application_new("org.xfce.gigolo", G_APPLICATION_FLAGS_NONE);
		g_signal_connect (gis, "activate", G_CALLBACK (activate), NULL);
		status = g_application_run (G_APPLICATION (gis), argc, argv);
	}
	else
	{
		gtk_init(&argc, &argv);
		status = gigolo_create(NULL);
	}

	return status;
}

static int gigolo_create(GtkApplication *gis)
{
	GigoloSettings *settings;
	gchar *accel_filename;
	GtkWidget *window;

	settings = gigolo_settings_new();

	accel_filename = g_build_filename(g_get_user_config_dir(), PACKAGE, "accels", NULL);
	gtk_accel_map_load(accel_filename);

	window = gigolo_window_new(settings);

	if (gis != NULL)
		gtk_application_add_window(gis, GTK_WINDOW(window));

	if (gigolo_settings_get_boolean(settings, "start-in-systray") &&
		gigolo_settings_get_boolean(settings, "show-in-systray"))
	{
		gdk_notify_startup_complete();
	}
	else
		gtk_widget_show(window);

	gtk_main();

	g_object_unref(settings);
	if (gis != NULL)
		g_object_unref(gis);

	gtk_accel_map_save(accel_filename);
	g_free(accel_filename);

	return 0;
}
