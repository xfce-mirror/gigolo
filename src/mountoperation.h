/*
 *      mountoperation.h
 *
 *      Copyright 2009-2010 Enrico Tröger <enrico(at)xfce(dot)org>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
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


#ifndef __MOUNTOPERATION_H__
#define __MOUNTOPERATION_H__

G_BEGIN_DECLS

#if ! GTK_CHECK_VERSION(2, 14, 0)

#define GIGOLO_MOUNT_OPERATION_TYPE				(gigolo_mount_operation_get_type())
#define GIGOLO_MOUNT_OPERATION(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			GIGOLO_MOUNT_OPERATION_TYPE, GigoloMountOperation))
#define GIGOLO_MOUNT_OPERATION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			GIGOLO_MOUNT_OPERATION_TYPE, GigoloMountOperationClass))
#define IS_GIGOLO_MOUNT_OPERATION(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			GIGOLO_MOUNT_OPERATION_TYPE))
#define IS_GIGOLO_MOUNT_OPERATION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),\
			GIGOLO_MOUNT_OPERATION_TYPE))

typedef struct _GigoloMountOperation					GigoloMountOperation;
typedef struct _GigoloMountOperationClass				GigoloMountOperationClass;


GType				gigolo_mount_operation_get_type		(void);

#endif

GMountOperation*	gigolo_mount_operation_new			(GtkWindow	*parent);

G_END_DECLS

#endif /* __MOUNTOPERATION_H__ */
