/*
 *      passworddialog.h
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


#ifndef __PASSWORDDIALOG_H__
#define __PASSWORDDIALOG_H__

G_BEGIN_DECLS

#define GIGOLO_PASSWORD_DIALOG_TYPE				(gigolo_password_dialog_get_type())
#define GIGOLO_PASSWORD_DIALOG(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			GIGOLO_PASSWORD_DIALOG_TYPE, GigoloPasswordDialog))
#define GIGOLO_PASSWORD_DIALOG_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			GIGOLO_PASSWORD_DIALOG_TYPE, GigoloPasswordDialogClass))
#define IS_GIGOLO_PASSWORD_DIALOG(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			GIGOLO_PASSWORD_DIALOG_TYPE))
#define IS_GIGOLO_PASSWORD_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),\
			GIGOLO_PASSWORD_DIALOG_TYPE))

typedef struct _GigoloPasswordDialog			GigoloPasswordDialog;
typedef struct _GigoloPasswordDialogClass		GigoloPasswordDialogClass;

struct _GigoloPasswordDialog
{
	GtkDialog parent;
};

struct _GigoloPasswordDialogClass
{
	GtkDialogClass parent_class;
};

GType			gigolo_password_dialog_get_type		(void);
GtkWidget*		gigolo_password_dialog_new			(GAskPasswordFlags flags, const gchar *user);

const gchar*	gigolo_password_dialog_get_domain	(GigoloPasswordDialog *dialog);
const gchar*	gigolo_password_dialog_get_username	(GigoloPasswordDialog *dialog);
const gchar*	gigolo_password_dialog_get_password	(GigoloPasswordDialog *dialog);

G_END_DECLS

#endif /* __PASSWORDDIALOG_H__ */
