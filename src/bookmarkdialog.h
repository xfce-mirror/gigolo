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

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GIGOLO_BOOKMARK_DIALOG_TYPE				(gigolo_bookmark_dialog_get_type())
G_DECLARE_FINAL_TYPE(GigoloBookmarkDialog, gigolo_bookmark_dialog, GIGOLO, BOOKMARK_DIALOG, GtkDialog)

GtkWidget*	gigolo_bookmark_dialog_new			(GigoloWindow *parent);

G_END_DECLS

#endif /* __BOOKMARKDIALOG_H__ */
