/*
 *      common.h
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



#ifndef __COMMON_H__
#define __COMMON_H__


/* Returns: TRUE if @a ptr points to a non-zero value. */
#define NZV(ptr) \
	((ptr) && (ptr)[0])


enum
{
	GIGOLO_WINDOW_COL_IS_MOUNTED,
	GIGOLO_WINDOW_COL_SCHEME,
	GIGOLO_WINDOW_COL_NAME,
	GIGOLO_WINDOW_COL_REF,
	GIGOLO_WINDOW_COL_REF_TYPE, /* volume or mount, see enum below */
	GIGOLO_WINDOW_COL_PIXBUF,
	GIGOLO_WINDOW_COL_ICON_NAME,
	GIGOLO_WINDOW_COL_TOOLTIP,
	GIGOLO_WINDOW_N_COLUMNS
};

enum
{
	GIGOLO_WINDOW_REF_TYPE_VOLUME,
	GIGOLO_WINDOW_REF_TYPE_MOUNT /* mounted volume */
};


const gchar *gigolo_describe_scheme(const gchar *scheme);

gboolean gigolo_str_equal(const gchar *a, const gchar *b);

const gchar *gigolo_find_icon_name(const gchar *request, const gchar *fallback);

gboolean gigolo_is_desktop_xfce(void);

void gigolo_show_uri(const gchar *uri);

guint gigolo_get_default_port(const gchar *scheme);

gboolean gigolo_message_dialog(gpointer *parent, gint type, const gchar *title,
							   const gchar *text, const gchar *secondary);

const gchar *gigolo_get_application_icon_name(void);

#endif /* __COMMON_H__ */
