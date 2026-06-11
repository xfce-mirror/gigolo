/*
 *      window.h
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


#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GIGOLO_WINDOW_TYPE					(gigolo_window_get_type())
G_DECLARE_FINAL_TYPE(GigoloWindow, gigolo_window, GIGOLO, WINDOW, GtkWindow)

GtkWidget*		gigolo_window_new					(GigoloSettings *settings);

void			gigolo_window_update_bookmarks		(GigoloWindow *window);
gboolean 		gigolo_window_do_autoconnect		(gpointer data);

void			gigolo_window_mount_from_bookmark	(GigoloWindow *window,
													 GigoloBookmark *bookmark,
													 gboolean show_dialog,
													 gboolean show_errors);

GigoloSettings*	gigolo_window_get_settings			(GigoloWindow *window);

GigoloBackendGVFS*	gigolo_window_get_backend		(GigoloWindow *window);

G_END_DECLS

#endif /* __WINDOW_H__ */
