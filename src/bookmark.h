/*
 *      bookmark.h
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


#ifndef __BOOKMARK_H__
#define __BOOKMARK_H__

G_BEGIN_DECLS

#define GIGOLO_BOOKMARK_TYPE					(gigolo_bookmark_get_type())
#define GIGOLO_BOOKMARK(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			GIGOLO_BOOKMARK_TYPE, GigoloBookmark))
#define GIGOLO_BOOKMARK_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass),\
			GIGOLO_BOOKMARK_TYPE, GigoloBookmarkClass))
#define IS_GIGOLO_BOOKMARK(obj)					(G_TYPE_CHECK_INSTANCE_TYPE((obj), GIGOLO_BOOKMARK_TYPE))
#define IS_GIGOLO_BOOKMARK_CLASS(klass)			(G_TYPE_CHECK_CLASS_TYPE((klass), GIGOLO_BOOKMARK_TYPE))

typedef struct _GigoloBookmark					GigoloBookmark;
typedef struct _GigoloBookmarkClass				GigoloBookmarkClass;

struct _GigoloBookmark
{
	GObject parent;
};

struct _GigoloBookmarkClass
{
	GObjectClass parent_class;
};

GType				gigolo_bookmark_get_type		(void);
GigoloBookmark*		gigolo_bookmark_new				(void);
GigoloBookmark*		gigolo_bookmark_new_from_uri	(const gchar *name, const gchar *uri);

gboolean			gigolo_bookmark_is_valid		(GigoloBookmark *bookmark);

void				gigolo_bookmark_clone			(GigoloBookmark *dst, const GigoloBookmark *src);

gchar*				gigolo_bookmark_get_uri			(GigoloBookmark *bookmark);
gchar*				gigolo_bookmark_get_uri_escaped	(GigoloBookmark *bookmark);
void				gigolo_bookmark_set_uri			(GigoloBookmark *bookmark, const gchar *uri);

const gchar*		gigolo_bookmark_get_name		(GigoloBookmark *bookmark);
void				gigolo_bookmark_set_name		(GigoloBookmark *bookmark, const gchar *name);

const gchar*		gigolo_bookmark_get_scheme		(GigoloBookmark *bookmark);
void				gigolo_bookmark_set_scheme		(GigoloBookmark *bookmark, const gchar *scheme);

const gchar*		gigolo_bookmark_get_host		(GigoloBookmark *bookmark);
void				gigolo_bookmark_set_host		(GigoloBookmark *bookmark, const gchar *host);

const gchar*		gigolo_bookmark_get_folder		(GigoloBookmark *bookmark);
void				gigolo_bookmark_set_folder		(GigoloBookmark *bookmark, const gchar *folder);
gchar*				gigolo_bookmark_get_folder_expanded(GigoloBookmark *bookmark);

const gchar*		gigolo_bookmark_get_path		(GigoloBookmark *bookmark);
void				gigolo_bookmark_set_path		(GigoloBookmark *bookmark, const gchar *path);

guint				gigolo_bookmark_get_port		(GigoloBookmark *bookmark);
void				gigolo_bookmark_set_port		(GigoloBookmark *bookmark, guint port);

const gchar*		gigolo_bookmark_get_user		(GigoloBookmark *bookmark);
gchar*				gigolo_bookmark_get_user_unescaped(GigoloBookmark *bookmark);
void				gigolo_bookmark_set_user		(GigoloBookmark *bookmark, const gchar *user);

const gchar*		gigolo_bookmark_get_share		(GigoloBookmark *bookmark);
void				gigolo_bookmark_set_share		(GigoloBookmark *bookmark, const gchar *share);

const gchar*		gigolo_bookmark_get_domain		(GigoloBookmark *bookmark);
void				gigolo_bookmark_set_domain		(GigoloBookmark *bookmark, const gchar *domain);

gboolean			gigolo_bookmark_get_autoconnect	(GigoloBookmark *bookmark);
void				gigolo_bookmark_set_autoconnect	(GigoloBookmark *bookmark, gboolean autoconnect);

gboolean			gigolo_bookmark_get_should_not_autoconnect	(GigoloBookmark *bookmark);
void				gigolo_bookmark_set_should_not_autoconnect	(GigoloBookmark *bookmark, gboolean should_not_autoconnect);

gboolean			gigolo_bookmark_parse_uri		(GigoloBookmark *bookmark, const gchar *uri);

void				gigolo_bookmark_bookmark_clear	(GigoloBookmark *bookmark);

const gchar*		gigolo_bookmark_get_color		(GigoloBookmark *bookmark);
void				gigolo_bookmark_set_color		(GigoloBookmark *bookmark, const gchar *color);

G_END_DECLS

#endif /* __BOOKMARK_H__ */
