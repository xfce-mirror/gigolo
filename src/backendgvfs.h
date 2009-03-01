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

#define GIGOLO_BACKEND_GVFS_TYPE				(gigolo_backend_gvfs_get_type())
#define GIGOLO_BACKEND_GVFS(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			GIGOLO_BACKEND_GVFS_TYPE, GigoloBackendGVFS))
#define GIGOLO_BACKEND_GVFS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			GIGOLO_BACKEND_GVFS_TYPE, GigoloBackendGVFSClass))
#define IS_GIGOLO_BACKEND_GVFS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			GIGOLO_BACKEND_GVFS_TYPE))
#define IS_GIGOLO_BACKEND_GVFS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass),\
			GIGOLO_BACKEND_GVFS_TYPE))


typedef struct _GigoloBackendGVFS				GigoloBackendGVFS;
typedef struct _GigoloBackendGVFSClass			GigoloBackendGVFSClass;

struct _GigoloBackendGVFS
{
	GObject parent;
};

struct _GigoloBackendGVFSClass
{
	GObjectClass parent_class;
};

GType				gigolo_backend_gvfs_get_type					(void);
GigoloBackendGVFS*	gigolo_backend_gvfs_new							(GtkListStore *store);

gboolean			gigolo_backend_gvfs_is_mount					(gpointer mnt);
void				gigolo_backend_gvfs_get_name_and_uri_from_mount	(GMount *mount, gchar **name, gchar **uri);

gboolean			gigolo_backend_gvfs_mount_volume				(GigoloBackendGVFS *backend, GVolume *vol);
void				gigolo_backend_gvfs_unmount_mount				(GigoloBackendGVFS *backend, GMount *mount);

void				gigolo_backend_gvfs_mount_uri					(GigoloBackendGVFS *backend,
																	 const gchar *uri,
																	 GtkWindow *parent,
																	 GtkWidget *dialog);

gchar*				gigolo_backend_gvfs_get_volume_identifier		(GVolume *volume);

G_END_DECLS

#endif /* __BACKENDGVFS_H__ */
