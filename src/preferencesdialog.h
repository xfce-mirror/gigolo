/*
 *      preferencesdialog.h
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


#ifndef __PREFERENCESDIALOG_H__
#define __PREFERENCESDIALOG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GIGOLO_PREFERENCES_DIALOG_TYPE				(gigolo_preferences_dialog_get_type())
G_DECLARE_FINAL_TYPE(GigoloPreferencesDialog, gigolo_preferences_dialog, GIGOLO, PREFERENCES_DIALOG, GtkDialog)

GtkWidget*	gigolo_preferences_dialog_new			(GtkWindow *parent, GigoloSettings *settings);

G_END_DECLS

#endif /* __PREFERENCESDIALOG_H__ */
