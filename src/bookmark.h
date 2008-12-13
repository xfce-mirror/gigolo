/*
 *      bookmark.h
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


#ifndef __BOOKMARK_H__
#define __BOOKMARK_H__

G_BEGIN_DECLS

#define SION_BOOKMARK_TYPE					(sion_bookmark_get_type())
#define SION_BOOKMARK(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			SION_BOOKMARK_TYPE, SionBookmark))
#define SION_BOOKMARK_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass),\
			SION_BOOKMARK_TYPE, SionBookmarkClass))
#define IS_SION_BOOKMARK(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), SION_BOOKMARK_TYPE))
#define IS_SION_BOOKMARK_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), SION_BOOKMARK_TYPE))

typedef struct _SionBookmark				SionBookmark;
typedef struct _SionBookmarkClass			SionBookmarkClass;

struct _SionBookmark
{
	GObject parent;
};

struct _SionBookmarkClass
{
	GObjectClass parent_class;
};

GType				sion_bookmark_get_type		(void);
SionBookmark*		sion_bookmark_new			(void);
SionBookmark*		sion_bookmark_new_from_uri	(const gchar *name, const gchar *uri);

gboolean			sion_bookmark_is_valid		(SionBookmark *bookmark);

void				sion_bookmark_clone			(SionBookmark *dst, const SionBookmark *src);

gchar*				sion_bookmark_get_uri		(SionBookmark *bookmark);
void				sion_bookmark_set_uri		(SionBookmark *bookmark, const gchar *uri);

const gchar*		sion_bookmark_get_name		(SionBookmark *bookmark);
void				sion_bookmark_set_name		(SionBookmark *bookmark, const gchar *name);

const gchar*		sion_bookmark_get_scheme	(SionBookmark *bookmark);
void				sion_bookmark_set_scheme	(SionBookmark *bookmark, const gchar *scheme);

const gchar*		sion_bookmark_get_host		(SionBookmark *bookmark);
void				sion_bookmark_set_host		(SionBookmark *bookmark, const gchar *host);

guint				sion_bookmark_get_port		(SionBookmark *bookmark);
void				sion_bookmark_set_port		(SionBookmark *bookmark, guint port);

const gchar*		sion_bookmark_get_user		(SionBookmark *bookmark);
void				sion_bookmark_set_user		(SionBookmark *bookmark, const gchar *user);

G_END_DECLS

#endif /* __BOOKMARK_H__ */
