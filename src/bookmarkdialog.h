/*
 *      bookmarkdialog.h
 *
 *      Copyright 2008-2011 Enrico Tröger <enrico(at)xfce(dot)org>
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


#ifndef __BOOKMARKDIALOG_H__
#define __BOOKMARKDIALOG_H__

G_BEGIN_DECLS

#define GIGOLO_BOOKMARK_DIALOG_TYPE				(gigolo_bookmark_dialog_get_type())
#define GIGOLO_BOOKMARK_DIALOG(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			GIGOLO_BOOKMARK_DIALOG_TYPE, GigoloBookmarkDialog))
#define GIGOLO_BOOKMARK_DIALOG_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			GIGOLO_BOOKMARK_DIALOG_TYPE, GigoloBookmarkDialogClass))
#define IS_GIGOLO_BOOKMARK_DIALOG(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			GIGOLO_BOOKMARK_DIALOG_TYPE))
#define IS_GIGOLO_BOOKMARK_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),\
			GIGOLO_BOOKMARK_DIALOG_TYPE))

typedef struct _GigoloBookmarkDialog			GigoloBookmarkDialog;
typedef struct _GigoloBookmarkDialogClass		GigoloBookmarkDialogClass;

struct _GigoloBookmarkDialog
{
	GtkDialog parent;
};

struct _GigoloBookmarkDialogClass
{
	GtkDialogClass parent_class;
};

GType		gigolo_bookmark_dialog_get_type		(void);
GtkWidget*	gigolo_bookmark_dialog_new			(GigoloWindow *parent);

G_END_DECLS

#endif /* __BOOKMARKDIALOG_H__ */
