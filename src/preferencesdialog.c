/*
 *      preferencesdialog.c
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

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>

#include "common.h"
#include "compat.h"
#include "settings.h"
#include "window.h"
#include "preferencesdialog.h"


typedef struct _SionPreferencesDialogPrivate			SionPreferencesDialogPrivate;

#define SION_PREFERENCES_DIALOG_GET_PRIVATE(obj)		(G_TYPE_INSTANCE_GET_PRIVATE((obj),\
			SION_PREFERENCES_DIALOG_TYPE, SionPreferencesDialogPrivate))

struct _SionPreferencesDialogPrivate
{
	SionWindow	*window;
};

static void sion_preferences_dialog_class_init			(SionPreferencesDialogClass *klass);
static void sion_preferences_dialog_init      			(SionPreferencesDialog *dialog);

static GtkDialogClass *parent_class = NULL;

enum
{
    PROP_0,
    PROP_SETTINGS
};

GType sion_preferences_dialog_get_type(void)
{
	static GType self_type = 0;
	if (! self_type)
	{
		static const GTypeInfo self_info =
		{
			sizeof(SionPreferencesDialogClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc)sion_preferences_dialog_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(SionPreferencesDialog),
			0,
			(GInstanceInitFunc)sion_preferences_dialog_init,
			NULL /* value_table */
		};

		self_type = g_type_register_static(GTK_TYPE_DIALOG, "SionPreferencesDialog", &self_info, 0);
	}

	return self_type;
}


/* Create an Xfce header with icon and title (only used on Xfce).
 * (Code from Midori, thanks to Christian Dywan.) */
static GtkWidget *xfce_header_new(const gchar *icon, const gchar *title)
{
	GtkWidget *entry;
	gchar *markup;
	GtkWidget *xfce_heading;
	GtkWidget *hbox;
	GtkWidget *image;
	GtkWidget *label;
	GdkColor *color;

	xfce_heading = gtk_event_box_new();
	entry = gtk_entry_new();
	color = &(gtk_widget_get_style(entry)->base[GTK_STATE_NORMAL]);
	gtk_widget_modify_bg(xfce_heading, GTK_STATE_NORMAL, color);
	hbox = gtk_hbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
	image = gtk_image_new_from_icon_name(icon, GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	label = gtk_label_new(NULL);
	color = &(gtk_widget_get_style(entry)->text[GTK_STATE_NORMAL]);
	gtk_widget_modify_fg(label, GTK_STATE_NORMAL, color);
	markup = g_strdup_printf("<span size='large' weight='bold'>%s</span>", title);
	gtk_label_set_markup(GTK_LABEL(label), markup);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(xfce_heading), hbox);
	g_free(markup);

	gtk_widget_destroy(entry);

	return xfce_heading;
}


/* Create a frame with no actual frame but a bold label and indentation.
 * (Code from Midori, thanks to Christian Dywan.) */
static GtkWidget *hig_frame_new(const gchar* title)
{
	GtkWidget *frame = gtk_frame_new(NULL);
	gchar *title_bold = g_strdup_printf("<b>%s</b>", title);
	GtkWidget *label = gtk_label_new(NULL);

	gtk_label_set_markup(GTK_LABEL(label), title_bold);
	g_free(title_bold);
	gtk_frame_set_label_widget(GTK_FRAME(frame), label);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 4);

	return frame;
}


static void vm_imple_toggle_cb(GtkToggleButton *button, SionSettings *settings)
{
	sion_settings_set_vm_impl(settings, g_object_get_data(G_OBJECT(button), "impl"));
}


static void check_button_toggle_cb(GtkToggleButton *button, SionSettings *settings)
{
    gboolean toggled = gtk_toggle_button_get_active(button);
    const gchar* property = g_object_get_data(G_OBJECT(button), "property");

	g_object_set(settings, property, toggled, NULL);
}


static GtkWidget *add_check_button(SionSettings *settings, const gchar *property, const gchar *text)
{
	gboolean toggled;
	GtkWidget *widget;

	g_object_get(settings, property, &toggled, NULL);
	widget = gtk_check_button_new_with_mnemonic(text);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), toggled);

	g_object_set_data(G_OBJECT(widget), "property", (gpointer) property);
	g_signal_connect(widget, "toggled", G_CALLBACK(check_button_toggle_cb), settings);

	return widget;
}


static void combo_box_changed_cb(GtkComboBox* button, SionSettings *settings)
{
    gint value = gtk_combo_box_get_active(button);
    const gchar *property = g_object_get_data(G_OBJECT(button), "property");

	g_object_set(settings, property, value, NULL);
}


static GtkWidget *add_toolbar_style_combo(SionSettings *settings, const gchar *property)
{
	gint value;
	GtkWidget *widget;

	g_object_get(settings, property, &value, NULL);

	widget = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(widget), _("Icons"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(widget), _("Text"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(widget), _("Both"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(widget), _("Both horizontal"));

	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), value);

	g_object_set_data(G_OBJECT(widget), "property", (gpointer) property);
	g_signal_connect(widget, "changed", G_CALLBACK(combo_box_changed_cb), settings);

	return widget;
}


static GtkWidget *add_toolbar_orientation_combo(SionSettings *settings, const gchar *property)
{
	gint value;
	GtkWidget *widget;

	g_object_get(settings, property, &value, NULL);

	widget = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(widget), _("Horizontal"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(widget), _("Vertical"));

	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), value);

	g_object_set_data(G_OBJECT(widget), "property", (gpointer) property);
	g_signal_connect(widget, "changed", G_CALLBACK(combo_box_changed_cb), settings);

	return widget;
}


static GtkWidget *add_view_mode_combo(SionSettings *settings, const gchar *property)
{
	gint value;
	GtkWidget *widget;

	g_object_get(settings, property, &value, NULL);

	widget = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(widget), _("Symbols"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(widget), _("Detailed"));

	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), value);

	g_object_set_data(G_OBJECT(widget), "property", (gpointer) property);
	g_signal_connect(widget, "changed", G_CALLBACK(combo_box_changed_cb), settings);

	return widget;
}


static void entry_check_input(GtkEntry *entry)
{
	const gchar *command;
	gchar *first_space;
	gchar *first_part;
	gchar *path;
	GtkImage *icon = g_object_get_data(G_OBJECT(entry), "image");

	command = gtk_entry_get_text(entry);
	if ((first_space = strstr(command, " ")))
		first_part = g_strndup(command, first_space - command);
	else
		first_part = g_strdup(command);

	path = g_find_program_in_path(first_part);

	if (path != NULL)
	{
		if (gtk_icon_theme_has_icon(gtk_icon_theme_get_for_screen(
				gtk_widget_get_screen(GTK_WIDGET(entry))), first_part))
		{
			gtk_image_set_from_icon_name(icon, first_part, GTK_ICON_SIZE_MENU);
		}
		else
			gtk_image_set_from_stock(icon, GTK_STOCK_EXECUTE, GTK_ICON_SIZE_MENU);
		g_free(path);
	}
	else
		gtk_image_set_from_stock(icon, GTK_STOCK_STOP, GTK_ICON_SIZE_MENU);

	g_free(first_part);

}


static void entry_activate_cb(GtkEntry *entry, SionSettings *settings)
{
	const gchar *text = gtk_entry_get_text(entry);
	const gchar *property = g_object_get_data(G_OBJECT(entry), "property");

	g_object_set(settings, property, text, NULL);
	entry_check_input(entry);
}


static gboolean entry_focus_out_event_cb(GtkEntry *entry, GdkEventFocus *event,
										 SionSettings *settings)
{
	const gchar *text = gtk_entry_get_text(entry);
	const gchar *property = g_object_get_data(G_OBJECT(entry), "property");

	g_object_set(settings, property, text, NULL);
	entry_check_input(entry);

	return FALSE;
}


static GtkWidget *add_program_entry(SionSettings *settings, const gchar *property)
{
	GtkWidget *widget;
	gchar *string;

	widget = gtk_entry_new();
	g_object_get(settings, property, &string, NULL);

	if (string != NULL)
		gtk_entry_set_text(GTK_ENTRY(widget), string);

	g_object_set_data(G_OBJECT(widget), "property", (gpointer) property);
	g_signal_connect(widget, "activate", G_CALLBACK(entry_activate_cb), settings);
	g_signal_connect(widget, "focus-out-event",  G_CALLBACK(entry_focus_out_event_cb), settings);

	return widget;
}


static void set_settings(SionPreferencesDialog *dialog, SionSettings *settings)
{
	GtkWidget *frame, *frame_vbox, *align, *vbox, *hbox;
	GtkWidget *radio1, *radio2, *checkbox, *combo, *entry;
	GtkWidget *label1, *label2, *label3, *label4, *image;
	GSList *rlist;
	GtkSizeGroup *sg;

    vbox = sion_dialog_get_content_area(GTK_DIALOG(dialog));

	if (sion_is_desktop_xfce())
    {
		GtkWidget *heading;
		heading = xfce_header_new(
			sion_window_get_icon_name(),
			gtk_window_get_title(GTK_WINDOW(dialog)));
		gtk_box_pack_start(GTK_BOX(vbox), heading, FALSE, FALSE, 0);
	}

	frame = hig_frame_new(_("General"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 6);

	align = gtk_alignment_new(0, 0.5, 0, 0);
	gtk_container_add(GTK_CONTAINER(frame), align);

	frame_vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(frame_vbox), 4); \
	gtk_container_add(GTK_CONTAINER(align), frame_vbox);

	radio1 = gtk_radio_button_new_with_mnemonic(NULL, _("Use _HAL based volume manager"));
	/// TODO fix this string to be more descriptive and clear
	gtk_widget_set_tooltip_text(radio1, _("This option sets the implementation of the volume manager. In general, this should be left to HAL. Please note, this option requires a restart of Sion."));
	rlist = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio1));
	if (strcmp(sion_settings_get_vm_impl(settings), "hal") == 0)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio1), TRUE);
	gtk_box_pack_start(GTK_BOX(frame_vbox), radio1, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(radio1), "impl", "hal");

	radio2 = gtk_radio_button_new_with_mnemonic(rlist, _("Use _Unix based volume manager"));
	gtk_widget_set_tooltip_text(radio2, _("This option sets the implementation of the volume manager. In general, this should be left to HAL. Please note, this option requires a restart of Sion."));
	rlist = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio2));
	if (strcmp(sion_settings_get_vm_impl(settings), "unix") == 0)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio2), TRUE);
	gtk_box_pack_start(GTK_BOX(frame_vbox), radio2, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(radio2), "impl", "unix");

	g_signal_connect(radio1, "toggled", G_CALLBACK(vm_imple_toggle_cb), settings);
	g_signal_connect(radio2, "toggled", G_CALLBACK(vm_imple_toggle_cb), settings);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(frame_vbox), hbox, FALSE, FALSE, 0);

	label1 = gtk_label_new_with_mnemonic(_("_File Manager"));
	gtk_misc_set_alignment(GTK_MISC(label1), 0.0f, 0.5f);
	gtk_box_pack_start(GTK_BOX(hbox), label1, FALSE, FALSE, 0);

	image = gtk_image_new();
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 3);

	entry = add_program_entry(settings, "file-manager");
	gtk_widget_set_tooltip_text(entry, _("Enter the name of a program to use to open or view mount points"));
	g_object_set_data(G_OBJECT(entry), "image", image);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label1), entry);
	gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
	entry_check_input(GTK_ENTRY(entry));

	frame = hig_frame_new(_("Interface"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 6);

	align = gtk_alignment_new(0, 0.5, 0, 0);
	gtk_container_add(GTK_CONTAINER(frame), align);

	frame_vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(frame_vbox), 4); \
	gtk_container_add(GTK_CONTAINER(align), frame_vbox);

	checkbox = add_check_button(settings, "save-geometry", _("_Save window position and geometry"));
	gtk_widget_set_tooltip_text(checkbox, _("Saves the window position and geometry and restores it at the start"));
	gtk_box_pack_start(GTK_BOX(frame_vbox), checkbox, FALSE, FALSE, 0);

	checkbox = add_check_button(settings, "show-trayicon", _("Show tray _icon"));
	gtk_box_pack_start(GTK_BOX(frame_vbox), checkbox, FALSE, FALSE, 0);

	checkbox = add_check_button(settings, "show-toolbar", _("Show _toolbar"));
	gtk_box_pack_start(GTK_BOX(frame_vbox), checkbox, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(frame_vbox), hbox, FALSE, FALSE, 0);

	label2 = gtk_label_new_with_mnemonic(_("Toolbar St_yle"));
	gtk_misc_set_alignment(GTK_MISC(label2), 0.0f, 0.5f);
	gtk_box_pack_start(GTK_BOX(hbox), label2, FALSE, FALSE, 0);

	combo = add_toolbar_style_combo(settings, "toolbar-style");
	gtk_label_set_mnemonic_widget(GTK_LABEL(label2), combo);
	gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(frame_vbox), hbox, FALSE, FALSE, 0);

	label3 = gtk_label_new_with_mnemonic(_("Toolbar _Orientation"));
	gtk_misc_set_alignment(GTK_MISC(label3), 0.0f, 0.5f);
	gtk_box_pack_start(GTK_BOX(hbox), label3, FALSE, FALSE, 0);

	combo = add_toolbar_orientation_combo(settings, "toolbar-orientation");
	gtk_label_set_mnemonic_widget(GTK_LABEL(label3), combo);
	gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(frame_vbox), hbox, FALSE, FALSE, 0);

	label4 = gtk_label_new_with_mnemonic(_("_View Mode"));
	gtk_misc_set_alignment(GTK_MISC(label4), 0.0f, 0.5f);
	gtk_box_pack_start(GTK_BOX(hbox), label4, FALSE, FALSE, 0);

	combo = add_view_mode_combo(settings, "view-mode");
	gtk_label_set_mnemonic_widget(GTK_LABEL(label4), combo);
	gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, FALSE, 0);

	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget(sg, label2);
	gtk_size_group_add_widget(sg, label3);
	gtk_size_group_add_widget(sg, label4);
	g_object_unref(sg);

	gtk_widget_show_all(vbox);
}


static void sion_preferences_dialog_set_property(GObject *object, guint prop_id,
												 const GValue *value, GParamSpec *pspec)
{
	SionPreferencesDialog *preferences = SION_PREFERENCES_DIALOG(object);

	switch (prop_id)
	{
	case PROP_SETTINGS:
	{
		set_settings(preferences, g_value_get_object(value));
		break;
	}
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}


static void sion_preferences_dialog_class_init(SionPreferencesDialogClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = sion_preferences_dialog_set_property;

	g_object_class_install_property(gobject_class,
									PROP_SETTINGS,
									g_param_spec_object(
									"settings",
									"Settings",
									"Settings instance to provide properties",
									SION_SETTINGS_TYPE,
									G_PARAM_WRITABLE));

	parent_class = (GtkDialogClass*)g_type_class_peek(GTK_TYPE_DIALOG);
	g_type_class_add_private((gpointer)klass, sizeof(SionPreferencesDialogPrivate));
}


static void sion_preferences_dialog_init(SionPreferencesDialog *dialog)
{
    g_object_set(dialog,
		"icon-name", GTK_STOCK_PREFERENCES,
		"title", _("Preferences"),
		"has-separator", FALSE,
		NULL);
	gtk_dialog_add_buttons(GTK_DIALOG(dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
}


GtkWidget *sion_preferences_dialog_new(GtkWindow *parent, SionSettings *settings)
{
	GtkWidget *dialog;

	dialog = g_object_new(SION_PREFERENCES_DIALOG_TYPE,
		"transient-for", parent,
		"destroy-with-parent", TRUE,
		"settings", settings,
		NULL);


	return dialog;
}


