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

#define SION_SETTINGS_TYPE				(sion_settings_get_type())
#define SION_SETTINGS(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
		SION_SETTINGS_TYPE, SionSettings))
#define SION_SETTINGS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
		SION_SETTINGS_TYPE, SionSettingsClass))
#define IS_SION_SETTINGS(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), SION_SETTINGS_TYPE))
#define IS_SION_SETTINGS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), SION_SETTINGS_TYPE))

typedef struct _SionSettings			SionSettings;
typedef struct _SionSettingsClass		SionSettingsClass;
typedef 	   GPtrArray				SionBookmarkList;

typedef enum
{
	SION_SETTINGS_PREFERENCES	= (1 << 0),
	SION_SETTINGS_BOOKMARKS		= (1 << 1)
} SionSettingsFlags;

struct _SionSettings
{
	GObject parent;
};

struct _SionSettingsClass
{
	GObjectClass parent_class;
};

GType				sion_settings_get_type			(void);
SionSettings*		sion_settings_new				(void);

void				sion_settings_write				(SionSettings *settings, SionSettingsFlags flags);

const gchar*		sion_settings_get_vm_impl		(SionSettings *settings);
void				sion_settings_set_vm_impl		(SionSettings *settings, const gchar *impl);

const gint*			sion_settings_get_geometry		(SionSettings *settings);
void				sion_settings_set_geometry		(SionSettings *settings, const gint *geometry, gsize len);

SionBookmarkList*	sion_settings_get_bookmarks		(SionSettings *settings);
gboolean			sion_settings_has_file_manager	(SionSettings *settings);

gboolean			sion_settings_get_boolean		(SionSettings *settings, const gchar *property);
gint				sion_settings_get_integer		(SionSettings *settings, const gchar *property);
gchar*				sion_settings_get_string		(SionSettings *settings, const gchar *property);


G_END_DECLS

#endif /* __SETTINGS_H__ */
