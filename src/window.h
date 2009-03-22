/*
 *      window.h
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


#ifndef __WINDOW_H__
#define __WINDOW_H__

G_BEGIN_DECLS

#define GIGOLO_WINDOW_TYPE					(gigolo_window_get_type())
#define GIGOLO_WINDOW(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj),\
		GIGOLO_WINDOW_TYPE, GigoloWindow))
#define GIGOLO_WINDOW_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass),\
		GIGOLO_WINDOW_TYPE, GigoloWindowClass))
#define IS_GIGOLO_WINDOW(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), GIGOLO_WINDOW_TYPE))
#define IS_GIGOLO_WINDOW_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), GIGOLO_WINDOW_TYPE))



typedef struct _GigoloWindow				GigoloWindow;
typedef struct _GigoloWindowClass			GigoloWindowClass;

struct _GigoloWindow
{
	GtkWindow parent;
};

struct _GigoloWindowClass
{
	GtkWindowClass parent_class;
};

GType			gigolo_window_get_type				(void);
GtkWidget*		gigolo_window_new					(GigoloSettings *settings);

void			gigolo_window_update_bookmarks		(GigoloWindow *window);
gboolean 		gigolo_window_do_autoconnect		(gpointer data);

void			gigolo_window_mount_from_bookmark	(GigoloWindow *window,
													 GigoloBookmark *bookmark,
													 gboolean show_dialog);

GigoloBookmark*	gigolo_window_find_bookmark_by_uri	(GigoloWindow *window, const gchar *uri);

GigoloSettings*	gigolo_window_get_settings			(GigoloWindow *window);

G_END_DECLS

#endif /* __WINDOW_H__ */
