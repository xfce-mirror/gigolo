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

#include "common.h"
#include "main.h"


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


gchar *sion_beautify_scheme(const gchar *scheme)
{
	gchar *result;

	if (sion_str_equal(scheme, "file"))
	{
		result = g_strdup(scheme);
		/* Capitalise first character */
		result[0] = g_unichar_toupper(scheme[0]);
	}
	else
		result = g_utf8_strup(scheme, -1);

	return result;
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
