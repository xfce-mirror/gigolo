/*
 *      main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <string.h>
#include <libintl.h>

#include "main.h"
#include "settings.h"
#include "bookmark.h"
#include "window.h"
#include "backendgvfs.h"
#include "singleinstance.h"


static gboolean show_version = FALSE;
static gboolean verbose_mode = FALSE;
static gboolean list_schemes = FALSE;
static gboolean new_instance = FALSE;

static GOptionEntry cli_options[] =
{
	{ "new-instance", 'i', 0, G_OPTION_ARG_NONE, &new_instance, N_("Ignore running instances, enforce opening a new instance"), NULL },
	{ "list-schemes", 'l', 0, G_OPTION_ARG_NONE, &list_schemes, N_("Print a list of supported URI schemes"), NULL },
	{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose_mode, N_("Be verbose"), NULL },
	{ "version", 'V', 0, G_OPTION_ARG_NONE, &show_version, N_("Show version information"), NULL },
	{ NULL, 0, 0, 0, NULL, NULL, NULL }
};


#ifdef DEBUG
void debug(gchar const *format, ...)
{
	va_list args;
	va_start(args, format);
	g_logv(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format, args);
	va_end(args);
}
#endif


void verbose(gchar const *format, ...)
{
#ifndef DEBUG
	if (verbose_mode)
#endif
	{
		va_list args;
		va_start(args, format);
		g_logv(G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE, format, args);
		va_end(args);
	}
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


gint main(gint argc, gchar** argv)
{
	GigoloSettings *settings;
	GigoloSingleInstance *gis = NULL;
	const gchar *vm_impl;
	gchar *accel_filename;
	GOptionContext *context;
	GtkWidget *window;

	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	context = g_option_context_new(_("- a simple frontend to easily connect to remote filesystems"));
	g_option_context_add_main_entries(context, cli_options, GETTEXT_PACKAGE);
	g_option_group_set_translation_domain(g_option_context_get_main_group(context), GETTEXT_PACKAGE);
	g_option_context_add_group(context, gtk_get_option_group(FALSE));
	g_option_context_parse(context, &argc, &argv, NULL);
	g_option_context_free(context);

	gtk_init(&argc, &argv);

	if (show_version)
	{
		g_print("%s %s\n\n", PACKAGE, VERSION);
		g_print("%s\n", "Copyright (c) 2008-2009");
		g_print("\tEnrico Tröger <enrico@xfce.org>\n\n");
		g_print("\n");

		return EXIT_SUCCESS;
	}

	if (list_schemes)
	{
		print_supported_schemes();

		return EXIT_SUCCESS;
	}

	if (! new_instance)
	{
		gis = gigolo_single_instance_new();
		if (gigolo_single_instance_is_running(gis))
		{
			gigolo_single_instance_present(gis);
			g_object_unref(gis);
			exit(0);
		}
	}

	verbose("Gigolo %s (GTK+ %u.%u.%u, GLib %u.%u.%u)",
		VERSION,
		gtk_major_version, gtk_minor_version, gtk_micro_version,
		glib_major_version, glib_minor_version, glib_micro_version);

	settings = gigolo_settings_new();

	accel_filename = g_build_filename(g_get_user_config_dir(), PACKAGE, "accels", NULL);
	gtk_accel_map_load(accel_filename);

	/* GVfs currently depends on gnome-mount for HAL-based GVolumeMonitor implementation,
	 * when gnome-mount is not installed, we can use "unix" as GVolumeMonitor implementation. */
	if ((vm_impl = gigolo_settings_get_vm_impl(settings)) != NULL)
		g_setenv("GIO_USE_VOLUME_MONITOR", vm_impl, 0);

	window = gigolo_window_new(settings);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	if (gis != NULL)
		gigolo_single_instance_set_parent(gis, GTK_WINDOW(window));

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
