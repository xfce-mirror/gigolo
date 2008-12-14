/*
 *      common.h
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



#ifndef __COMMON_H__
#define __COMMON_H__


/* Returns: TRUE if @a ptr points to a non-zero value. */
#define NZV(ptr) \
	((ptr) && (ptr)[0])


enum
{
	SION_WINDOW_COL_IS_MOUNTED,
	SION_WINDOW_COL_SCHEME,
	SION_WINDOW_COL_NAME,
	SION_WINDOW_COL_REF,
	SION_WINDOW_COL_REF_TYPE, /* volume or mount, see enum below */
	SION_WINDOW_COL_PIXBUF,
	SION_WINDOW_COL_ICON_NAME,
	SION_WINDOW_COL_TOOLTIP,
	SION_WINDOW_N_COLUMNS
};

enum
{
	SION_WINDOW_REF_TYPE_VOLUME,
	SION_WINDOW_REF_TYPE_MOUNT /* mounted volume */
};


const gchar *sion_describe_scheme(const gchar *scheme);

gboolean sion_str_equal(const gchar *a, const gchar *b);

const gchar *sion_find_icon_name(const gchar *request, const gchar *fallback);

gboolean sion_is_desktop_xfce(void);

void sion_show_uri(const gchar *uri);


#endif /* __COMMON_H__ */
