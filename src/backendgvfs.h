/*
 *      backendgvfs.h
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


#ifndef __BACKENDGVFS_H__
#define __BACKENDGVFS_H__

G_BEGIN_DECLS

#define SION_BACKEND_GVFS_TYPE				(sion_backend_gvfs_get_type())
#define SION_BACKEND_GVFS(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			SION_BACKEND_GVFS_TYPE, SionBackendGVFS))
#define SION_BACKEND_GVFS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			SION_BACKEND_GVFS_TYPE, SionBackendGVFSClass))
#define IS_SION_BACKEND_GVFS(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			SION_BACKEND_GVFS_TYPE))
#define IS_SION_BACKEND_GVFS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),\
			SION_BACKEND_GVFS_TYPE))


typedef struct _SionBackendGVFS			SionBackendGVFS;
typedef struct _SionBackendGVFSClass	SionBackendGVFSClass;

struct _SionBackendGVFS
{
	GObject parent;
};

struct _SionBackendGVFSClass
{
	GObjectClass parent_class;
};

GType				sion_backend_gvfs_get_type						(void);
SionBackendGVFS*	sion_backend_gvfs_new							(GtkListStore *store);

gboolean			sion_backend_gvfs_is_mount						(gpointer mnt);
void				sion_backend_gvfs_get_name_and_uri_from_mount	(GMount *mount, gchar **name, gchar **uri);

gboolean			sion_backend_gvfs_mount_volume					(SionBackendGVFS *backend, GVolume *vol);
void				sion_backend_gvfs_unmount_mount					(SionBackendGVFS *backend, GMount *mount);

void				sion_backend_gvfs_mount_uri						(SionBackendGVFS *backend, const gchar *uri, const gchar *domain);

gchar*				sion_backend_gvfs_get_volume_identifier			(GVolume *volume);

G_END_DECLS

#endif /* __BACKENDGVFS_H__ */
