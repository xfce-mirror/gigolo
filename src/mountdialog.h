/*
 *      mountdialog.h
 *
 *      Copyright 2009-2011 Enrico Tröger <enrico(at)xfce(dot)org>
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


#ifndef __MOUNTDIALOG_H__
#define __MOUNTDIALOG_H__

G_BEGIN_DECLS

#define GIGOLO_MOUNT_DIALOG_TYPE				(gigolo_mount_dialog_get_type())
#define GIGOLO_MOUNT_DIALOG(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			GIGOLO_MOUNT_DIALOG_TYPE, GigoloMountDialog))
#define GIGOLO_MOUNT_DIALOG_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			GIGOLO_MOUNT_DIALOG_TYPE, GigoloMountDialogClass))
#define IS_GIGOLO_MOUNT_DIALOG(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			GIGOLO_MOUNT_DIALOG_TYPE))
#define IS_GIGOLO_MOUNT_DIALOG_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass),\
			GIGOLO_MOUNT_DIALOG_TYPE))

typedef struct _GigoloMountDialog				GigoloMountDialog;
typedef struct _GigoloMountDialogClass			GigoloMountDialogClass;


GType		gigolo_mount_dialog_get_type		(void);
GtkWidget*	gigolo_mount_dialog_new				(GtkWindow *parent, const gchar *label);

G_END_DECLS

#endif /* __MOUNTDIALOG_H__ */
