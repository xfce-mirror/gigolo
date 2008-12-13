/*
 *      preferencesdialog.h
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


#ifndef __PREFERENCESDIALOG_H__
#define __PREFERENCESDIALOG_H__

G_BEGIN_DECLS

#define SION_PREFERENCES_DIALOG_TYPE				(sion_preferences_dialog_get_type())
#define SION_PREFERENCES_DIALOG(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			SION_PREFERENCES_DIALOG_TYPE, SionPreferencesDialog))
#define SION_PREFERENCES_DIALOG_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			SION_PREFERENCES_DIALOG_TYPE, SionPreferencesDialogClass))
#define IS_SION_PREFERENCES_DIALOG(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			SION_PREFERENCES_DIALOG_TYPE))
#define IS_SION_PREFERENCES_DIALOG_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass),\
			SION_PREFERENCES_DIALOG_TYPE))

typedef struct _SionPreferencesDialog				SionPreferencesDialog;
typedef struct _SionPreferencesDialogClass			SionPreferencesDialogClass;

struct _SionPreferencesDialog
{
	GtkDialog parent;
};

struct _SionPreferencesDialogClass
{
	GtkDialogClass parent_class;
};

GType		sion_preferences_dialog_get_type		(void);
GtkWidget*	sion_preferences_dialog_new				(GtkWindow *parent, SionSettings *settings);

G_END_DECLS

#endif /* __PREFERENCESDIALOG_H__ */
