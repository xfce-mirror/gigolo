/*
 *      window.h
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


#ifndef __WINDOW_H__
#define __WINDOW_H__

G_BEGIN_DECLS

#define SION_WINDOW_TYPE				(sion_window_get_type())
#define SION_WINDOW(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
		SION_WINDOW_TYPE, SionWindow))
#define SION_WINDOW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
		SION_WINDOW_TYPE, SionWindowClass))
#define IS_SION_WINDOW(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), SION_WINDOW_TYPE))
#define IS_SION_WINDOW_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), SION_WINDOW_TYPE))


typedef struct _SionWindow				SionWindow;
typedef struct _SionWindowClass			SionWindowClass;

struct _SionWindow
{
	GtkWindow parent;
};

struct _SionWindowClass
{
	GtkWindowClass parent_class;
};

GType		sion_window_get_type			(void);
GtkWidget*	sion_window_new					(SionSettings *settings);

void		sion_window_update_bookmarks	(SionWindow *window);


const gchar* sion_window_get_icon_name		(void);

G_END_DECLS

#endif /* __WINDOW_H__ */
