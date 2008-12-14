/*
 *      passworddialog.h
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


#ifndef __PASSWORDDIALOG_H__
#define __PASSWORDDIALOG_H__

G_BEGIN_DECLS

#define SION_PASSWORD_DIALOG_TYPE				(sion_password_dialog_get_type())
#define SION_PASSWORD_DIALOG(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			SION_PASSWORD_DIALOG_TYPE, SionPasswordDialog))
#define SION_PASSWORD_DIALOG_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			SION_PASSWORD_DIALOG_TYPE, SionPasswordDialogClass))
#define IS_SION_PASSWORD_DIALOG(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			SION_PASSWORD_DIALOG_TYPE))
#define IS_SION_PASSWORD_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),\
			SION_PASSWORD_DIALOG_TYPE))

typedef struct _SionPasswordDialog			SionPasswordDialog;
typedef struct _SionPasswordDialogClass		SionPasswordDialogClass;

struct _SionPasswordDialog
{
	GtkDialog parent;
};

struct _SionPasswordDialogClass
{
	GtkDialogClass parent_class;
};

GType			sion_password_dialog_get_type		(void);
GtkWidget*		sion_password_dialog_new			(GAskPasswordFlags flags, const gchar *user, const gchar *domain);

const gchar*	sion_password_dialog_get_domain		(SionPasswordDialog *dialog);
const gchar*	sion_password_dialog_get_username	(SionPasswordDialog *dialog);
const gchar*	sion_password_dialog_get_password	(SionPasswordDialog *dialog);

G_END_DECLS

#endif /* __PASSWORDDIALOG_H__ */
