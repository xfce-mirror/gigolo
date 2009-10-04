/*
 *      settings.h
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


#ifndef __SETTINGS_H__
#define __SETTINGS_H__

G_BEGIN_DECLS

#define GIGOLO_SETTINGS_TYPE				(gigolo_settings_get_type())
#define GIGOLO_SETTINGS(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
		GIGOLO_SETTINGS_TYPE, GigoloSettings))
#define GIGOLO_SETTINGS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
		GIGOLO_SETTINGS_TYPE, GigoloSettingsClass))
#define IS_GIGOLO_SETTINGS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), GIGOLO_SETTINGS_TYPE))
#define IS_GIGOLO_SETTINGS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), GIGOLO_SETTINGS_TYPE))

typedef struct _GigoloSettings			GigoloSettings;
typedef struct _GigoloSettingsClass		GigoloSettingsClass;
typedef 	   GPtrArray				GigoloBookmarkList;

typedef enum
{
	GIGOLO_SETTINGS_PREFERENCES		= (1 << 0),
	GIGOLO_SETTINGS_BOOKMARKS		= (1 << 1)
} GigoloSettingsFlags;

struct _GigoloSettings
{
	GObject parent;
};

struct _GigoloSettingsClass
{
	GObjectClass parent_class;
};

GType				gigolo_settings_get_type			(void);
GigoloSettings*		gigolo_settings_new					(void);

void				gigolo_settings_write				(GigoloSettings *settings, GigoloSettingsFlags flags);

const gchar*		gigolo_settings_get_vm_impl			(GigoloSettings *settings);
void				gigolo_settings_set_vm_impl			(GigoloSettings *settings, const gchar *impl);

const gint*			gigolo_settings_get_geometry		(GigoloSettings *settings);
void				gigolo_settings_set_geometry		(GigoloSettings *settings, const gint *geometry, gsize len);

GigoloBookmarkList*	gigolo_settings_get_bookmarks		(GigoloSettings *settings);
gboolean			gigolo_settings_has_file_manager	(GigoloSettings *settings);

gboolean			gigolo_settings_get_boolean			(GigoloSettings *settings, const gchar *property);
gint				gigolo_settings_get_integer			(GigoloSettings *settings, const gchar *property);
gchar*				gigolo_settings_get_string			(GigoloSettings *settings, const gchar *property);

GigoloBookmark*		gigolo_settings_get_bookmark_by_uri	(GigoloSettings *settings, const gchar *uri);

G_END_DECLS

#endif /* __SETTINGS_H__ */
