/*
 *      common.c
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
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#ifdef GDK_WINDOWING_X11
 #include <gdk/gdkx.h>
#endif

#include "common.h"

gboolean verbose_mode;

const gchar *gigolo_get_application_icon_name(void)
{
	static const gchar *icon_name = NULL;

	if (icon_name == NULL)
		icon_name = gigolo_find_icon_name("org.xfce.gigolo", "org.xfce.gigolo");

	return icon_name;
}


const gchar *gigolo_find_icon_name(const gchar *request, const gchar *fallback)
{
	GtkIconTheme *theme = gtk_icon_theme_get_default();

	if (gtk_icon_theme_has_icon(theme, request))
		return request;
	else
	{
		debug("icon %s not found, using fallback %s", request, fallback);
		return fallback;
	}
}


/* NULL-safe string comparison */
gboolean gigolo_str_equal(const gchar *a, const gchar *b)
{
	if (a == NULL && b == NULL) return TRUE;
	else if (a == NULL || b == NULL) return FALSE;

	while (*a == *b++)
		if (*a++ == '\0')
			return TRUE;

	return FALSE;
}


const gchar *gigolo_describe_scheme(const gchar *scheme)
{
	if (gigolo_str_equal(scheme, "file"))
		return _("Unix Device");
	else if (gigolo_str_equal(scheme, "smb"))
		return _("Windows Share");
	else if (gigolo_str_equal(scheme, "ftp"))
		return _("FTP");
	else if (gigolo_str_equal(scheme, "http"))
		return _("HTTP");
	else if (gigolo_str_equal(scheme, "sftp"))
		return _("SSH / SFTP");
	else if (gigolo_str_equal(scheme, "obex"))
		return _("Obex");
	else if (gigolo_str_equal(scheme, "dav"))
		return _("WebDAV");
	else if (gigolo_str_equal(scheme, "davs"))
		return _("WebDAV (secure)");
	else if (gigolo_str_equal(scheme, "network"))
		return _("Network");
	else if (gigolo_str_equal(scheme, "archive"))
		return _("Archive");
	else if (gigolo_str_equal(scheme, "gphoto2"))
		return _("Photos");
	else if (gigolo_str_equal(scheme, "custom"))
		return _("Custom Location");

	return NULL;
}


guint gigolo_get_default_port(const gchar *scheme)
{
	if (gigolo_str_equal(scheme, "ftp"))
		return 21;
	else if (gigolo_str_equal(scheme, "sftp"))
		return 22;
	else if (gigolo_str_equal(scheme, "dav"))
		return 80;
	else if (gigolo_str_equal(scheme, "davs"))
		return 443;

	return 0;
}


/* Are we running in Xfce? */
gboolean gigolo_is_desktop_xfce(void)
{
	static gboolean check = TRUE;
	static gboolean is_xfce = FALSE;

	if (check)
	{
		/* Are we running in Xfce? */
		GdkDisplay *display = gdk_display_get_default();
		Display *xdisplay = GDK_DISPLAY_XDISPLAY(display);
		Window root_window = RootWindow(xdisplay, 0);
		Atom save_mode_atom = gdk_x11_get_xatom_by_name("_DT_SAVE_MODE");
		Atom actual_type;
		gint actual_format;
		gulong n_items, bytes;
		gchar *value;
		gint status = XGetWindowProperty(xdisplay, root_window, save_mode_atom, 0, (~0L),
			False, AnyPropertyType, &actual_type, &actual_format, &n_items, &bytes,
			(guchar**) &value);
		if (status == Success)
		{
			if (n_items == 6 && !strncmp (value, "xfce4", 6))
				is_xfce = TRUE;
			XFree(value);
		}
		check = FALSE;
	}
	return is_xfce;
}


gboolean gigolo_message_dialog(gpointer parent, gint type, const gchar *title,
							   const gchar *text, const gchar *secondary)
{
	gboolean ret = FALSE;
	GtkWidget *dialog;
	GtkButtonsType button_type = (type == GTK_MESSAGE_QUESTION) ?
		GTK_BUTTONS_YES_NO : GTK_BUTTONS_OK;

	dialog = gtk_message_dialog_new(GTK_WINDOW(parent), GTK_DIALOG_DESTROY_WITH_PARENT,
                                  type, button_type, "%s", text);

	if (secondary != NULL)
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", secondary);
	gtk_window_set_title(GTK_WINDOW(dialog), title);
	gtk_window_set_icon_name(GTK_WINDOW(dialog), gigolo_get_application_icon_name());

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES)
		ret = TRUE;

	gtk_widget_destroy(dialog);

	return ret;
}


/* Can open URLs and email addresses using xdg/exo/gnome-open */
void gigolo_show_uri(const gchar *uri)
{
	gchar *cmd;
	gchar *open_cmd = g_find_program_in_path("xdg-open");

	if (open_cmd == NULL)
		open_cmd = g_strdup((gigolo_is_desktop_xfce()) ? "exo-open" : "gnome-open");

	cmd = g_strconcat(open_cmd, " ", uri, NULL);
	g_spawn_command_line_async(cmd, NULL);
	g_free(cmd);
	g_free(open_cmd);
}


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

