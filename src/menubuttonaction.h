/*
 *      menubuttonaction.h
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


#ifndef __MENU_BUTTON_ACTION_H__
#define __MENU_BUTTON_ACTION_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GIGOLO_MENU_BUTTON_ACTION_TYPE				(gigolo_menu_button_action_get_type())
G_DECLARE_FINAL_TYPE(GigoloMenubuttonAction, gigolo_menu_button_action, GIGOLO, MENU_BUTTON_ACTION, GtkMenu)

GtkMenu*	gigolo_menu_button_action_new		(const gchar	*name);

G_END_DECLS

#endif /* __MENU_BUTTON_ACTION_H__ */
