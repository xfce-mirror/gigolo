/*
 *      bookmarkeditdialog.h
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


#ifndef __BOOKMARKEDITDIALOG_H__
#define __BOOKMARKEDITDIALOG_H__

G_BEGIN_DECLS

#define SION_BOOKMARK_EDIT_DIALOG_TYPE				(sion_bookmark_edit_dialog_get_type())
#define SION_BOOKMARK_EDIT_DIALOG(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			SION_BOOKMARK_EDIT_DIALOG_TYPE, SionBookmarkEditDialog))
#define SION_BOOKMARK_EDIT_DIALOG_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			SION_BOOKMARK_EDIT_DIALOG_TYPE, SionBookmarkEditDialogClass))
#define IS_SION_BOOKMARK_EDIT_DIALOG(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			SION_BOOKMARK_EDIT_DIALOG_TYPE))
#define IS_SION_BOOKMARK_EDIT_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),\
			SION_BOOKMARK_EDIT_DIALOG_TYPE))

typedef struct _SionBookmarkEditDialog				SionBookmarkEditDialog;
typedef struct _SionBookmarkEditDialogClass			SionBookmarkEditDialogClass;


typedef enum
{
	SION_BE_MODE_CREATE,
	SION_BE_MODE_EDIT,
	SION_BE_MODE_CONNECT
} SionBookmarkEditDialogMode;

struct _SionBookmarkEditDialog
{
	GtkDialog parent;
};

struct _SionBookmarkEditDialogClass
{
	GtkDialogClass parent_class;
};

GType		sion_bookmark_edit_dialog_get_type			(void);
GtkWidget*	sion_bookmark_edit_dialog_new				(GtkWindow *parent, SionSettings *settings, SionBookmarkEditDialogMode mode);
GtkWidget*	sion_bookmark_edit_dialog_new_with_bookmark	(GtkWindow *parent, SionSettings *settings, SionBookmarkEditDialogMode, SionBookmark *bookmark);
gint		sion_bookmark_edit_dialog_run				(SionBookmarkEditDialog *dialog);

G_END_DECLS

#endif /* __BOOKMARKEDITDIALOG_H__ */
