/*
 *      bookmarkdialog.h
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


#ifndef __BOOKMARKDIALOG_H__
#define __BOOKMARKDIALOG_H__

G_BEGIN_DECLS

#define SION_BOOKMARK_DIALOG_TYPE				(sion_bookmark_dialog_get_type())
#define SION_BOOKMARK_DIALOG(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			SION_BOOKMARK_DIALOG_TYPE, SionBookmarkDialog))
#define SION_BOOKMARK_DIALOG_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			SION_BOOKMARK_DIALOG_TYPE, SionBookmarkDialogClass))
#define IS_SION_BOOKMARK_DIALOG(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			SION_BOOKMARK_DIALOG_TYPE))
#define IS_SION_BOOKMARK_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),\
			SION_BOOKMARK_DIALOG_TYPE))

typedef struct _SionBookmarkDialog				SionBookmarkDialog;
typedef struct _SionBookmarkDialogClass			SionBookmarkDialogClass;

struct _SionBookmarkDialog
{
	GtkDialog parent;
};

struct _SionBookmarkDialogClass
{
	GtkDialogClass parent_class;
};

GType		sion_bookmark_dialog_get_type		(void);
GtkWidget*	sion_bookmark_dialog_new			(GtkWidget *parent, SionSettings *settings);

G_END_DECLS

#endif /* __BOOKMARKDIALOG_H__ */
