/*
 *      common.c
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

#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "common.h"
#include "main.h"
#include "settings.h"
#include "window.h"


const gchar *sion_find_icon_name(const gchar *request, const gchar *fallback)
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
gboolean sion_str_equal(const gchar *a, const gchar *b)
{
	if (a == NULL && b == NULL) return TRUE;
	else if (a == NULL || b == NULL) return FALSE;

	while (*a == *b++)
		if (*a++ == '\0')
			return TRUE;

	return FALSE;
}


const gchar *sion_describe_scheme(const gchar *scheme)
{
	if (sion_str_equal(scheme, "file"))
		return _("Unix Device");
	else if (sion_str_equal(scheme, "smb"))
		return _("Windows Share");
	else if (sion_str_equal(scheme, "ftp"))
		return _("FTP");
	else if (sion_str_equal(scheme, "http"))
		return _("HTTP");
	else if (sion_str_equal(scheme, "sftp"))
		return _("SSH");
	else if (sion_str_equal(scheme, "obex"))
		/// TODO find something better
		return _("OBEX");
	else if (sion_str_equal(scheme, "dav"))
		return _("WebDAV");
	else if (sion_str_equal(scheme, "davs"))
		return _("Secure WebDAV");
	else if (sion_str_equal(scheme, "network"))
		return _("Network");

	return NULL;
}


guint sion_get_default_port(const gchar *scheme)
{
	if (sion_str_equal(scheme, "ftp"))
		return 21;
	else if (sion_str_equal(scheme, "sftp"))
		return 22;
	else if (sion_str_equal(scheme, "dav"))
		return 80;
	else if (sion_str_equal(scheme, "davs"))
		return 443;

	return 0;
}


/* Are we running in Xfce? */
gboolean sion_is_desktop_xfce(void)
{
	static gboolean check = TRUE;
	static gboolean is_xfce = FALSE;

	if (check)
	{
		gint result;
		gchar *out = NULL;
		gboolean success;

		success = g_spawn_command_line_sync("xprop -root _DT_SAVE_MODE", &out, NULL, &result, NULL);
		if (success && result == 0 && out != NULL && strstr(out, "xfce4") != NULL)
		{
			is_xfce = TRUE;
		}
		g_free(out);

		check = FALSE;
	}
    return is_xfce;
}


void sion_error_dialog(gpointer *parent, const gchar *text, const gchar *secondary)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(GTK_WINDOW(parent), GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", text);
	if (secondary != NULL)
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", secondary);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Error"));
	gtk_window_set_icon_name(GTK_WINDOW(dialog), sion_window_get_icon_name());
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}


/* Can open URLs and email addresses using xdg/exo/gnome-open */
void sion_show_uri(const gchar *uri)
{
	gchar *cmd;
	gchar *open_cmd = g_find_program_in_path("xdg-open");

	if (open_cmd == NULL)
		open_cmd = g_strdup((sion_is_desktop_xfce()) ? "exo-open" : "gnome-open");

	cmd = g_strconcat(open_cmd, " ", uri, NULL);
	g_spawn_command_line_async(cmd, NULL);
	g_free(cmd);
	g_free(open_cmd);
}
