/*
 *      menubuttonaction.h
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


#ifndef __MENU_BUTTON_ACTION_H__
#define __MENU_BUTTON_ACTION_H__

G_BEGIN_DECLS

#define GIGOLO_MENU_BUTTON_ACTION_TYPE				(gigolo_menu_button_action_get_type())
#define GIGOLO_MENU_BUTTON_ACTION(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			GIGOLO_MENU_BUTTON_ACTION_TYPE, GigoloMenubuttonAction))
#define GIGOLO_MENU_BUTTON_ACTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			GIGOLO_MENU_BUTTON_ACTION_TYPE, GigoloMenubuttonActionClass))
#define IS_GIGOLO_MENU_BUTTON_ACTION(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			GIGOLO_MENU_BUTTON_ACTION_TYPE))
#define IS_GIGOLO_MENU_BUTTON_ACTION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),\
			GIGOLO_MENU_BUTTON_ACTION_TYPE))

typedef struct _GigoloMenubuttonAction			GigoloMenubuttonAction;
typedef struct _GigoloMenubuttonActionClass		GigoloMenubuttonActionClass;

struct _GigoloMenubuttonAction
{
	GtkAction parent;
};

struct _GigoloMenubuttonActionClass
{
	GtkActionClass parent_class;
};

GType		gigolo_menu_button_action_get_type	(void);
GtkAction*	gigolo_menu_button_action_new		(const gchar	*name,
												 const gchar	*label,
												 const gchar	*tooltip,
												 const gchar	*icon_name);

G_END_DECLS

#endif /* __MENU_BUTTON_ACTION_H__ */
