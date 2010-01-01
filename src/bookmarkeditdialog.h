/*
 *      bookmarkeditdialog.h
 *
 *      Copyright 2008-2010 Enrico Tröger <enrico(at)xfce(dot)org>
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

#define GIGOLO_BOOKMARK_EDIT_DIALOG_TYPE				(gigolo_bookmark_edit_dialog_get_type())
#define GIGOLO_BOOKMARK_EDIT_DIALOG(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			GIGOLO_BOOKMARK_EDIT_DIALOG_TYPE, GigoloBookmarkEditDialog))
#define GIGOLO_BOOKMARK_EDIT_DIALOG_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			GIGOLO_BOOKMARK_EDIT_DIALOG_TYPE, GigoloBookmarkEditDialogClass))
#define IS_GIGOLO_BOOKMARK_EDIT_DIALOG(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			GIGOLO_BOOKMARK_EDIT_DIALOG_TYPE))
#define IS_GIGOLO_BOOKMARK_EDIT_DIALOG_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass),\
			GIGOLO_BOOKMARK_EDIT_DIALOG_TYPE))

typedef struct _GigoloBookmarkEditDialog				GigoloBookmarkEditDialog;
typedef struct _GigoloBookmarkEditDialogClass			GigoloBookmarkEditDialogClass;


typedef enum
{
	GIGOLO_BE_MODE_CREATE,
	GIGOLO_BE_MODE_EDIT,
	GIGOLO_BE_MODE_CONNECT
} GigoloBookmarkEditDialogMode;

struct _GigoloBookmarkEditDialog
{
	GtkDialog parent;
};

struct _GigoloBookmarkEditDialogClass
{
	GtkDialogClass parent_class;
};

GType		gigolo_bookmark_edit_dialog_get_type			(void);
GtkWidget*	gigolo_bookmark_edit_dialog_new					(GigoloWindow *parent,
															 GigoloBookmarkEditDialogMode mode);
GtkWidget*	gigolo_bookmark_edit_dialog_new_with_bookmark	(GigoloWindow *parent,
															 GigoloBookmarkEditDialogMode,
															 GigoloBookmark *bookmark);
gint		gigolo_bookmark_edit_dialog_run					(GigoloBookmarkEditDialog *dialog);

G_END_DECLS

#endif /* __BOOKMARKEDITDIALOG_H__ */
