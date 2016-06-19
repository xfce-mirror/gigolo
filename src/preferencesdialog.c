/*
 *      preferencesdialog.c
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

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>

#include "common.h"
#include "bookmark.h"
#include "settings.h"
#include "backendgvfs.h"
#include "preferencesdialog.h"


typedef struct _GigoloPreferencesDialogPrivate			GigoloPreferencesDialogPrivate;

#define GIGOLO_PREFERENCES_DIALOG_GET_PRIVATE(obj)		(G_TYPE_INSTANCE_GET_PRIVATE((obj),\
			GIGOLO_PREFERENCES_DIALOG_TYPE, GigoloPreferencesDialogPrivate))


enum
{
    PROP_0,
    PROP_SETTINGS
};

G_DEFINE_TYPE(GigoloPreferencesDialog, gigolo_preferences_dialog, GTK_TYPE_DIALOG);


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
	GtkWidget* vbox;
	GtkWidget* separator;

	xfce_heading = gtk_event_box_new();
	entry = gtk_entry_new();
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
	image = gtk_image_new_from_icon_name(icon, GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	label = gtk_label_new(NULL);
	markup = g_strdup_printf("<span size='large' weight='bold'>%s</span>", title);
	gtk_label_set_markup(GTK_LABEL(label), markup);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(xfce_heading), hbox);
	g_free(markup);

	gtk_widget_destroy(entry);

	vbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(vbox), xfce_heading, FALSE, FALSE, 0);

	separator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 0);

	return vbox;
}


static void check_button_toggle_cb(GtkToggleButton *button, GigoloSettings *settings)
{
    gboolean toggled = gtk_toggle_button_get_active(button);
    const gchar* property = g_object_get_data(G_OBJECT(button), "property");

	g_object_set(settings, property, toggled, NULL);
}


static void check_button_toggle_sensitive_cb(GtkToggleButton *button, GtkWidget *widget)
{
    gboolean toggled = gtk_toggle_button_get_active(button);

	gtk_widget_set_sensitive(widget, toggled);
}


static void check_toolbar_show_toggle_cb(GtkToggleButton *button, G_GNUC_UNUSED gpointer data)
{
    gboolean toggled = gtk_toggle_button_get_active(button);

	gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(button), "combo_toolbar_style"), toggled);
	gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(button), "combo_toolbar_orient"), toggled);
}


static GtkWidget *add_check_button(GigoloSettings *settings, const gchar *property, const gchar *text)
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


static void combo_box_changed_cb(GtkComboBox* button, GigoloSettings *settings)
{
    gint value = gtk_combo_box_get_active(button);
    const gchar *property = g_object_get_data(G_OBJECT(button), "property");

	g_object_set(settings, property, value, NULL);
}


static GtkWidget *add_toolbar_style_combo(GigoloSettings *settings, const gchar *property)
{
	gint value;
	GtkWidget *widget;

	g_object_get(settings, property, &value, NULL);

	widget = gtk_combo_box_text_new();
	gtk_combo_box_text_append(GTK_COMBO_BOX(widget), NULL, _("Icons"));
	gtk_combo_box_text_append(GTK_COMBO_BOX(widget), NULL, _("Text"));
	gtk_combo_box_text_append(GTK_COMBO_BOX(widget), NULL, _("Both"));
	gtk_combo_box_text_append(GTK_COMBO_BOX(widget), NULL, _("Both horizontal"));

	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), value);

	g_object_set_data(G_OBJECT(widget), "property", (gpointer) property);
	g_signal_connect(widget, "changed", G_CALLBACK(combo_box_changed_cb), settings);

	return widget;
}


static GtkWidget *add_toolbar_orientation_combo(GigoloSettings *settings, const gchar *property)
{
	gint value;
	GtkWidget *widget;

	g_object_get(settings, property, &value, NULL);

	widget = gtk_combo_box_text_new();
	gtk_combo_box_text_append(GTK_COMBO_BOX(widget), NULL, _("Horizontal"));
	gtk_combo_box_text_append(GTK_COMBO_BOX(widget), NULL, _("Vertical"));

	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), value);

	g_object_set_data(G_OBJECT(widget), "property", (gpointer) property);
	g_signal_connect(widget, "changed", G_CALLBACK(combo_box_changed_cb), settings);

	return widget;
}


static GtkWidget *add_view_mode_combo(GigoloSettings *settings, const gchar *property)
{
	gint value;
	GtkWidget *widget;

	g_object_get(settings, property, &value, NULL);

	widget = gtk_combo_box_text_new();
	gtk_combo_box_text_append(GTK_COMBO_BOX(widget), NULL, _("Symbols"));
	gtk_combo_box_text_append(GTK_COMBO_BOX(widget), NULL, _("Detailed List"));

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


static void entry_activate_cb(GtkEntry *entry, GigoloSettings *settings)
{
	const gchar *text = gtk_entry_get_text(entry);
	const gchar *property = g_object_get_data(G_OBJECT(entry), "property");

	g_object_set(settings, property, text, NULL);
	entry_check_input(entry);
}


static gboolean entry_focus_out_event_cb(GtkEntry *entry, G_GNUC_UNUSED GdkEventFocus *event,
										 GigoloSettings *settings)
{
	const gchar *text = gtk_entry_get_text(entry);
	const gchar *property = g_object_get_data(G_OBJECT(entry), "property");

	g_object_set(settings, property, text, NULL);
	entry_check_input(entry);

	return FALSE;
}


static GtkWidget *add_program_entry(GigoloSettings *settings, const gchar *property)
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


static void spin_value_changed_cb(GtkSpinButton *spin, GigoloSettings *settings)
{
	gint interval = gtk_spin_button_get_value_as_int(spin);
	const gchar *property = g_object_get_data(G_OBJECT(spin), "property");

	g_object_set(settings, property, interval, NULL);
}


static GtkWidget *add_spinbutton(GigoloSettings *settings, const gchar *property)
{
	GtkWidget *widget;
	gint timeout;

	widget = gtk_spin_button_new_with_range(0, G_MAXUINT, 1);
	g_object_get(settings, property, &timeout, NULL);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), timeout);

	g_object_set_data(G_OBJECT(widget), "property", (gpointer) property);
	g_signal_connect(widget, "activate", G_CALLBACK(spin_value_changed_cb), settings);
	g_signal_connect(widget, "value-changed",  G_CALLBACK(spin_value_changed_cb), settings);

	return widget;
}


static void set_settings(GigoloPreferencesDialog *dialog, GigoloSettings *settings)
{
	GtkWidget *frame_vbox, *notebook_vbox, *vbox, *hbox, *notebook;
	GtkWidget *checkbox, *combo, *entry, *combo_toolbar_style, *tmp_box;
	GtkWidget *combo_toolbar_orient, *spinbutton;
	GtkWidget *label1, *label2, *label3, *label4, *image;
	GtkSizeGroup *sg;

    vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	if (gigolo_is_desktop_xfce())
    {
		GtkWidget *heading;
		heading = xfce_header_new(
			gigolo_get_application_icon_name(),
			gtk_window_get_title(GTK_WINDOW(dialog)));
		gtk_box_pack_start(GTK_BOX(vbox), heading, FALSE, FALSE, 0);
	}

	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), notebook, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(notebook), 5);

#define PAGE_GENERAL
	notebook_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	frame_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_set_border_width(GTK_CONTAINER(frame_vbox), 5);
	gtk_box_pack_start(GTK_BOX(notebook_vbox), frame_vbox, TRUE, TRUE, 5);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), notebook_vbox, gtk_label_new(_("General")));

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start(GTK_BOX(frame_vbox), hbox, FALSE, FALSE, 0);

	label1 = gtk_label_new_with_mnemonic(_("_File Manager"));
	gtk_label_set_xalign(GTK_LABEL(label1), 0);
	gtk_box_pack_start(GTK_BOX(hbox), label1, FALSE, FALSE, 0);

	image = gtk_image_new();
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 3);

	entry = add_program_entry(settings, "file-manager");
	gtk_widget_set_tooltip_text(entry, _("Enter the name of a program to use to open or view mount points"));
	g_object_set_data(G_OBJECT(entry), "image", image);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label1), entry);
	gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
	entry_check_input(GTK_ENTRY(entry));

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start(GTK_BOX(frame_vbox), hbox, FALSE, FALSE, 0);

	label1 = gtk_label_new_with_mnemonic(_("_Terminal"));
	gtk_label_set_xalign(GTK_LABEL(label1), 0);
	gtk_box_pack_start(GTK_BOX(hbox), label1, FALSE, FALSE, 0);

	image = gtk_image_new();
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 3);

	entry = add_program_entry(settings, "terminal");
	gtk_widget_set_tooltip_text(entry, _("Enter the name of a program to open mount points in a terminal"));
	g_object_set_data(G_OBJECT(entry), "image", image);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label1), entry);
	gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
	entry_check_input(GTK_ENTRY(entry));

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start(GTK_BOX(frame_vbox), hbox, FALSE, FALSE, 0);

	label1 = gtk_label_new_with_mnemonic(_("_Bookmark Auto-Connect Interval"));
	gtk_label_set_xalign(GTK_LABEL(label1), 0);
	gtk_box_pack_start(GTK_BOX(hbox), label1, FALSE, FALSE, 0);

	spinbutton = add_spinbutton(settings, "autoconnect-interval");
	gtk_widget_set_tooltip_text(spinbutton,
		_("How often to try auto connecting bookmarks, in seconds. Zero disables checking."));
	gtk_label_set_mnemonic_widget(GTK_LABEL(label1), spinbutton);
	gtk_box_pack_start(GTK_BOX(hbox), spinbutton, FALSE, FALSE, 10);

	gtk_box_pack_start(GTK_BOX(frame_vbox), gtk_label_new(""), FALSE, FALSE, 0);

#define PAGE_INTERFACE
	notebook_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	frame_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_set_border_width(GTK_CONTAINER(frame_vbox), 5);
	gtk_box_pack_start(GTK_BOX(notebook_vbox), frame_vbox, TRUE, TRUE, 5);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), notebook_vbox, gtk_label_new(_("Interface")));

	checkbox = add_check_button(settings, "save-geometry", _("_Save window position and geometry"));
	gtk_widget_set_tooltip_text(checkbox, _("Saves the window position and geometry and restores it at the start"));
	gtk_box_pack_start(GTK_BOX(frame_vbox), checkbox, FALSE, FALSE, 0);

	tmp_box = checkbox = add_check_button(settings, "show-in-systray", _("Show status _icon in the Notification Area"));
	gtk_box_pack_start(GTK_BOX(frame_vbox), checkbox, FALSE, FALSE, 0);

	checkbox = add_check_button(settings, "start-in-systray", _("Start _minimized in the Notification Area"));
	gtk_box_pack_start(GTK_BOX(frame_vbox), checkbox, FALSE, FALSE, 0);
	g_signal_connect(tmp_box, "toggled", G_CALLBACK(check_button_toggle_sensitive_cb), checkbox);
	check_button_toggle_sensitive_cb(GTK_TOGGLE_BUTTON(tmp_box), checkbox);

	checkbox = add_check_button(settings, "show-panel", _("Show side panel"));
	gtk_widget_set_tooltip_text(checkbox, _("Whether to show a side panel for browsing the local network for available Samba/Windows shares and a bookmark list"));
	gtk_box_pack_start(GTK_BOX(frame_vbox), checkbox, FALSE, FALSE, 0);

	checkbox = add_check_button(settings, "show-autoconnect-errors", _("Show auto-connect error messages"));
	gtk_widget_set_tooltip_text(checkbox, _("Whether to show error message dialogs when auto-connecting of bookmarks fails"));
	gtk_box_pack_start(GTK_BOX(frame_vbox), checkbox, FALSE, FALSE, 0);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start(GTK_BOX(frame_vbox), hbox, FALSE, FALSE, 0);

	label4 = gtk_label_new_with_mnemonic(_("_Connection List Mode"));
	gtk_label_set_xalign(GTK_LABEL(label4), 0);
	gtk_box_pack_start(GTK_BOX(hbox), label4, FALSE, FALSE, 0);

	combo = add_view_mode_combo(settings, "view-mode");
	gtk_label_set_mnemonic_widget(GTK_LABEL(label4), combo);
	gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, FALSE, 0);

#define PAGE_TOOLBAR
	notebook_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	frame_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_set_border_width(GTK_CONTAINER(frame_vbox), 5);
	gtk_box_pack_start(GTK_BOX(notebook_vbox), frame_vbox, TRUE, TRUE, 5);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), notebook_vbox, gtk_label_new(_("Toolbar")));

	checkbox = add_check_button(settings, "show-toolbar", _("Show _toolbar"));
	gtk_box_pack_start(GTK_BOX(frame_vbox), checkbox, FALSE, FALSE, 0);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start(GTK_BOX(frame_vbox), hbox, FALSE, FALSE, 0);

	label2 = gtk_label_new_with_mnemonic(_("St_yle"));
	gtk_label_set_xalign(GTK_LABEL(label2), 0);
	gtk_box_pack_start(GTK_BOX(hbox), label2, FALSE, FALSE, 0);

	combo_toolbar_style = add_toolbar_style_combo(settings, "toolbar-style");
	gtk_label_set_mnemonic_widget(GTK_LABEL(label2), combo_toolbar_style);
	gtk_box_pack_start(GTK_BOX(hbox), combo_toolbar_style, FALSE, FALSE, 0);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start(GTK_BOX(frame_vbox), hbox, FALSE, FALSE, 0);

	label3 = gtk_label_new_with_mnemonic(_("_Orientation"));
	gtk_label_set_xalign(GTK_LABEL(label3), 0);
	gtk_box_pack_start(GTK_BOX(hbox), label3, FALSE, FALSE, 0);

	combo_toolbar_orient = add_toolbar_orientation_combo(settings, "toolbar-orientation");
	gtk_label_set_mnemonic_widget(GTK_LABEL(label3), combo_toolbar_orient);
	gtk_box_pack_start(GTK_BOX(hbox), combo_toolbar_orient, FALSE, FALSE, 0);

	g_object_set_data(G_OBJECT(checkbox), "combo_toolbar_style", combo_toolbar_style);
	g_object_set_data(G_OBJECT(checkbox), "combo_toolbar_orient", combo_toolbar_orient);
	g_signal_connect(checkbox, "toggled", G_CALLBACK(check_toolbar_show_toggle_cb), settings);

	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget(sg, label2);
	gtk_size_group_add_widget(sg, label3);
	g_object_unref(sg);


	gtk_widget_show_all(vbox);
}


static void gigolo_preferences_dialog_set_property(GObject *object, guint prop_id,
												 const GValue *value, GParamSpec *pspec)
{
	GigoloPreferencesDialog *preferences = GIGOLO_PREFERENCES_DIALOG(object);

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


static void gigolo_preferences_dialog_class_init(GigoloPreferencesDialogClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = gigolo_preferences_dialog_set_property;

	g_object_class_install_property(gobject_class,
									PROP_SETTINGS,
									g_param_spec_object(
									"settings",
									"Settings",
									"Settings instance to provide properties",
									GIGOLO_SETTINGS_TYPE,
									G_PARAM_WRITABLE));
}


static void gigolo_preferences_dialog_init(GigoloPreferencesDialog *dialog)
{
    g_object_set(dialog,
		"icon-name", GTK_STOCK_PREFERENCES,
		"title", _("Preferences"),
		NULL);
	gtk_dialog_add_buttons(GTK_DIALOG(dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
}


GtkWidget *gigolo_preferences_dialog_new(GtkWindow *parent, GigoloSettings *settings)
{
	GtkWidget *dialog;

	dialog = g_object_new(GIGOLO_PREFERENCES_DIALOG_TYPE,
		"transient-for", parent,
		"destroy-with-parent", TRUE,
		"settings", settings,
		NULL);

	return dialog;
}


