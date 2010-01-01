/*
 *      compat.h
 *
 *      Copyright 2008-2010 Enrico Tröger <enrico(at)xfce(dot)org>
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

#ifndef __COMPAT_H__
#define __COMPAT_H__


GdkWindow *gigolo_widget_get_window(GtkWidget *widget);

GtkWidget *gigolo_dialog_get_content_area(GtkDialog *dialog);

GtkWidget *gigolo_dialog_get_action_area(GtkDialog *dialog);

void gigolo_status_icon_set_tooltip_text(GtkStatusIcon *status_icon, const gchar *tooltip_text);

guint32 gigolo_widget_get_flags(GtkWidget *widget);

void gigolo_toolbar_set_orientation(GtkToolbar *toolbar, GtkOrientation orientation);

#endif /* __COMPAT_H__ */

