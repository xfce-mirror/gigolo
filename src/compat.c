/*
 *      compat.c
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

#include <gtk/gtk.h>

#include "compat.h"


GdkWindow *sion_widget_get_window(GtkWidget *widget)
{
#if GTK_CHECK_VERSION(2, 14, 0)
	return gtk_widget_get_window(widget);
#else
	return widget->window;
#endif
}


GtkWidget *sion_dialog_get_content_area(GtkDialog *dialog)
{
#if GTK_CHECK_VERSION(2, 14, 0)
	return gtk_dialog_get_content_area(dialog);
#else
	return dialog->vbox;
#endif
}


void sion_status_icon_set_tooltip_text(GtkStatusIcon *status_icon, const gchar *tooltip_text)
{
#if GTK_CHECK_VERSION(2, 16, 0)
	gtk_status_icon_set_tooltip_text(status_icon, tooltip_text);
#else
	gtk_status_icon_set_tooltip(status_icon, tooltip_text);
#endif
}

