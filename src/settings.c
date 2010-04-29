/*
 *      settings.c
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

#include "config.h"

#include <gtk/gtk.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "bookmark.h"
#include "settings.h"
#include "common.h"


typedef struct _GigoloSettingsPrivate			GigoloSettingsPrivate;

#define GIGOLO_SETTINGS_GET_PRIVATE(obj)		(G_TYPE_INSTANCE_GET_PRIVATE((obj),\
		GIGOLO_SETTINGS_TYPE, GigoloSettingsPrivate))

struct _GigoloSettingsPrivate
{
	gchar		*config_path;
	gchar		*config_filename;
	gchar		*bookmarks_filename;

	gboolean	save_geometry;
	gboolean	show_in_systray;
	gboolean	start_in_systray;
	gboolean	show_toolbar;
	gint		toolbar_style;
	gint		toolbar_orientation;
	gint		view_mode;
	gboolean	show_panel;
	guint		last_panel_page;
	gboolean	show_autoconnect_errors;

	gchar		*file_manager;
	gint		 autoconnect_interval;
	gint		*geometry; /* window size and position, field 4 is a flag for maximized state */

	GigoloBookmarkList *bookmarks; /* array of known bookmarks */
};

static void gigolo_settings_finalize			(GObject* object);


/* keyfile section names */
#define SECTION_GENERAL	"general"
#define SECTION_UI		"ui"

/* default values */
#define DEFAULT_AUTOCONNECT_INTERVAL	60

enum
{
	PROP_0,

	PROP_FILE_MANAGER,
	PROP_AUTOCONNECT_INTERVAL,

	PROP_SAVE_GEOMETRY,
	PROP_SHOW_IN_SYSTRAY,
	PROP_START_IN_SYSTRAY,
	PROP_SHOW_TOOLBAR,
	PROP_TOOLBAR_STYLE,
	PROP_TOOLBAR_ORIENTATION,
	PROP_VIEW_MODE,
	PROP_SHOW_PANEL,
	PROP_LAST_PANEL_PAGE,
	PROP_SHOW_AUTOCONNECT_ERRORS
};


G_DEFINE_TYPE(GigoloSettings, gigolo_settings, G_TYPE_OBJECT);


static void gigolo_settings_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GigoloSettingsPrivate *priv = GIGOLO_SETTINGS_GET_PRIVATE(object);

	switch (prop_id)
	{
	case PROP_SAVE_GEOMETRY:
		priv->save_geometry = g_value_get_boolean(value);
		break;
	case PROP_SHOW_IN_SYSTRAY:
		priv->show_in_systray = g_value_get_boolean(value);
		break;
	case PROP_START_IN_SYSTRAY:
		priv->start_in_systray = g_value_get_boolean(value);
		break;
	case PROP_SHOW_TOOLBAR:
		priv->show_toolbar = g_value_get_boolean(value);
		break;
	case PROP_TOOLBAR_STYLE:
		priv->toolbar_style = g_value_get_int(value);
		break;
	case PROP_TOOLBAR_ORIENTATION:
		priv->toolbar_orientation = g_value_get_int(value);
		break;
	case PROP_VIEW_MODE:
		priv->view_mode = g_value_get_int(value);
		break;
	case PROP_FILE_MANAGER:
		g_free(priv->file_manager);
		priv->file_manager = g_value_dup_string(value);
		break;
	case PROP_AUTOCONNECT_INTERVAL:
		priv->autoconnect_interval = g_value_get_int(value);
		break;
	case PROP_SHOW_PANEL:
		priv->show_panel = g_value_get_boolean(value);
		break;
	case PROP_LAST_PANEL_PAGE:
		priv->last_panel_page = g_value_get_uint(value);
		break;
	case PROP_SHOW_AUTOCONNECT_ERRORS:
		priv->show_autoconnect_errors = g_value_get_boolean(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}


static void gigolo_settings_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GigoloSettingsPrivate *priv = GIGOLO_SETTINGS_GET_PRIVATE(object);

	switch (prop_id)
	{
	case PROP_SAVE_GEOMETRY:
		g_value_set_boolean(value, priv->save_geometry);
		break;
	case PROP_SHOW_IN_SYSTRAY:
		g_value_set_boolean(value, priv->show_in_systray);
		break;
	case PROP_START_IN_SYSTRAY:
		g_value_set_boolean(value, priv->start_in_systray);
		break;
	case PROP_SHOW_TOOLBAR:
		g_value_set_boolean(value, priv->show_toolbar);
		break;
	case PROP_TOOLBAR_STYLE:
		g_value_set_int(value, priv->toolbar_style);
		break;
	case PROP_TOOLBAR_ORIENTATION:
		g_value_set_int(value, priv->toolbar_orientation);
		break;
	case PROP_VIEW_MODE:
		g_value_set_int(value, priv->view_mode);
		break;
	case PROP_FILE_MANAGER:
		g_value_set_string(value, priv->file_manager);
		break;
	case PROP_AUTOCONNECT_INTERVAL:
		if (priv->autoconnect_interval < 0)
			g_object_set(object, "autoconnect-interval", DEFAULT_AUTOCONNECT_INTERVAL, NULL);
		g_value_set_int(value, priv->autoconnect_interval);
		break;
	case PROP_SHOW_PANEL:
		g_value_set_boolean(value, priv->show_panel);
		break;
	case PROP_LAST_PANEL_PAGE:
		g_value_set_uint(value, priv->last_panel_page);
		break;
	case PROP_SHOW_AUTOCONNECT_ERRORS:
		g_value_set_boolean(value, priv->show_autoconnect_errors);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}


static void gigolo_settings_class_init(GigoloSettingsClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = gigolo_settings_finalize;
	gobject_class->get_property = gigolo_settings_get_property;
	gobject_class->set_property = gigolo_settings_set_property;

	g_type_class_add_private(klass, sizeof(GigoloSettingsPrivate));

	g_object_class_install_property(gobject_class,
									PROP_SAVE_GEOMETRY,
									g_param_spec_boolean(
									"save-geometry",
									"Save window position and geometry",
									"Saves the window position and geometry and restores it at the start",
									TRUE,
									G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class,
									PROP_SHOW_IN_SYSTRAY,
									g_param_spec_boolean(
									"show-in-systray",
									"show-in-systray",
									"Whether to show an icon in the notification area",
									TRUE,
									G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class,
									PROP_START_IN_SYSTRAY,
									g_param_spec_boolean(
									"start-in-systray",
									"start-in-systray",
									"Whether to start the application minimised in the notification area",
									FALSE,
									G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class,
									PROP_SHOW_TOOLBAR,
									g_param_spec_boolean(
									"show-toolbar",
									"show-toolbar",
									"Whether to show the toolbar",
									TRUE,
									G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class,
									PROP_TOOLBAR_STYLE,
									g_param_spec_int(
									"toolbar-style",
									"toolbar-style",
									"The style of the toolbar",
									-1, G_MAXINT, -1,
									G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class,
									PROP_TOOLBAR_ORIENTATION,
									g_param_spec_int(
									"toolbar-orientation",
									"toolbar-orientation",
									"The orientation of the toolbar",
									-1, G_MAXINT, -1,
									G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class,
									PROP_VIEW_MODE,
									g_param_spec_int(
									"view-mode",
									"view-mode",
									"Whether to use an IconView or a TreeView",
									0, G_MAXINT, 0,
									G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class,
									PROP_FILE_MANAGER,
									g_param_spec_string(
									"file-manager",
									"file-manager",
									"A program for use to open mount points",
									NULL,
									G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class,
									PROP_AUTOCONNECT_INTERVAL,
									g_param_spec_int(
									"autoconnect-interval",
									"autoconnect-interval",
									"Autoconnect interval",
									0, G_MAXINT, 0,
									G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class,
									PROP_SHOW_PANEL,
									g_param_spec_boolean(
									"show-panel",
									"show-panel",
									"Whether to show the side panel",
									TRUE,
									G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class,
									PROP_LAST_PANEL_PAGE,
									g_param_spec_uint(
									"last-panel-page",
									"last-panel-page",
									"Last displayed panel page",
									0, G_MAXUINT, 0,
									G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class,
									PROP_SHOW_AUTOCONNECT_ERRORS,
									g_param_spec_boolean(
									"show-autoconnect-errors",
									"show-autoconnect-errors",
									"Whether to show error messages when auto-connecting bookmarks fails",
									TRUE,
									G_PARAM_READWRITE));
}


static gchar *get_setting_string(GKeyFile *config, const gchar *section, const gchar *key,
								 const gchar *default_value)
{
	gchar *tmp;
	GError *error = NULL;

	if (config == NULL)
		return g_strdup(default_value);

	tmp = g_key_file_get_string(config, section, key, &error);
	if (error != NULL)
	{
		g_error_free(error);
		return (gchar*) g_strdup(default_value);
	}
	return tmp;
}


static gint get_setting_int(GKeyFile *config, const gchar *section, const gchar *key,
							  gint default_value)
{
	gint tmp;
	GError *error = NULL;

	if (config == NULL)
		return default_value;

	tmp = g_key_file_get_integer(config, section, key, &error);
	if (error != NULL)
	{
		g_error_free(error);
		return default_value;
	}
	return tmp;
}


static gboolean get_setting_boolean(GKeyFile *config, const gchar *section, const gchar *key,
									gboolean default_value)
{
	gboolean tmp;
	GError *error = NULL;

	if (config == NULL)
		return default_value;

	tmp = g_key_file_get_boolean(config, section, key, &error);
	if (error != NULL)
	{
		g_error_free(error);
		return default_value;
	}
	return tmp;
}


static void set_setting_string(GKeyFile *config, const gchar *section, const gchar *key,
							   const gchar *value)
{
	if (config != NULL && value != NULL)
		g_key_file_set_string(config, section, key, value);
}


static void set_setting_int(GKeyFile *config, const gchar *section, const gchar *key, gint value)
{
	if (config != NULL && value > 0)
		g_key_file_set_integer(config, section, key, value);
}


static void write_data(GKeyFile *k, const gchar *filename)
{
	gsize len;
	GError *error = NULL;
	gchar *data;

	data = g_key_file_to_data(k, &len, &error);
	if (data == NULL || error != NULL)
	{
		g_warning("Saving configuration file failed (%s).", error->message);
		g_error_free(error);
		g_free(data);
		return;
	}

	if (! g_file_set_contents(filename, data, len, &error))
	{
		g_warning("Writing configuration file to disk failed (%s).", error->message);
		g_error_free(error);
	}
	g_free(data);
}


static void write_settings_config(GigoloSettings *settings)
{
	GKeyFile *k;
	GigoloSettingsPrivate *priv = GIGOLO_SETTINGS_GET_PRIVATE(settings);

	if (! g_file_test(priv->config_path, G_FILE_TEST_IS_DIR))
		g_mkdir_with_parents(priv->config_path, 0700);

	k = g_key_file_new();

	if (priv->file_manager != NULL)
		g_key_file_set_string(k, SECTION_GENERAL, "file_manager", priv->file_manager);
	g_key_file_set_integer(k, SECTION_GENERAL, "autoconnect_interval", priv->autoconnect_interval);

	if (priv->geometry != NULL)
		g_key_file_set_integer_list(k, SECTION_UI, "geometry", priv->geometry, 5);
	g_key_file_set_boolean(k, SECTION_UI, "save_geometry", priv->save_geometry);
	g_key_file_set_boolean(k, SECTION_UI, "show_in_systray", priv->show_in_systray);
	g_key_file_set_boolean(k, SECTION_UI, "start_in_systray", priv->start_in_systray);
	g_key_file_set_boolean(k, SECTION_UI, "show_toolbar", priv->show_toolbar);
	g_key_file_set_integer(k, SECTION_UI, "toolbar_style", priv->toolbar_style);
	g_key_file_set_integer(k, SECTION_UI, "toolbar_orientation", priv->toolbar_orientation);
	g_key_file_set_integer(k, SECTION_UI, "view_mode", priv->view_mode);
	g_key_file_set_boolean(k, SECTION_UI, "show_panel", priv->show_panel);
	g_key_file_set_integer(k, SECTION_UI, "last_panel_page", priv->last_panel_page);
	g_key_file_set_boolean(k, SECTION_UI, "show_autoconnect_errors", priv->show_autoconnect_errors);

	write_data(k, priv->config_filename);

	g_key_file_free(k);
}


static void write_settings_bookmarks(GigoloSettings *settings)
{
	GKeyFile *k;
	const gchar *name;
	gsize i;
	GigoloBookmark *bm;
	GigoloBookmarkList *bml;
	GigoloSettingsPrivate *priv = GIGOLO_SETTINGS_GET_PRIVATE(settings);

	if (! g_file_test(priv->config_path, G_FILE_TEST_IS_DIR))
		g_mkdir_with_parents(priv->config_path, 0700);

	k = g_key_file_new();

	bml = gigolo_settings_get_bookmarks(settings);
	for (i = 0; i < bml->len; i++)
	{
		bm = g_ptr_array_index(bml, i);
		if (IS_GIGOLO_BOOKMARK(bm))
		{
			name = gigolo_bookmark_get_name(bm);
			set_setting_string(k, name, "host", gigolo_bookmark_get_host(bm));
			set_setting_string(k, name, "folder", gigolo_bookmark_get_folder(bm));
			set_setting_string(k, name, "user", gigolo_bookmark_get_user(bm));
			set_setting_string(k, name, "scheme", gigolo_bookmark_get_scheme(bm));
			set_setting_string(k, name, "share", gigolo_bookmark_get_share(bm));
			set_setting_string(k, name, "domain", gigolo_bookmark_get_domain(bm));
			set_setting_string(k, name, "path", gigolo_bookmark_get_path(bm));
			set_setting_int(k, name, "port", gigolo_bookmark_get_port(bm));
			set_setting_int(k, name, "autoconnect", gigolo_bookmark_get_autoconnect(bm));
		}
	}

	write_data(k, priv->bookmarks_filename);

	g_key_file_free(k);
}


void gigolo_settings_write(GigoloSettings *settings, GigoloSettingsFlags flags)
{
	g_return_if_fail(settings != NULL);

	if (flags & GIGOLO_SETTINGS_PREFERENCES)
		write_settings_config(settings);
	if (flags & GIGOLO_SETTINGS_BOOKMARKS)
		write_settings_bookmarks(settings);
}


static void gigolo_settings_finalize(GObject* object)
{
	GigoloSettingsPrivate *priv = GIGOLO_SETTINGS_GET_PRIVATE(object);

	gigolo_settings_write(GIGOLO_SETTINGS(object),
		GIGOLO_SETTINGS_PREFERENCES | GIGOLO_SETTINGS_BOOKMARKS);

	g_free(priv->geometry);

	g_ptr_array_foreach(priv->bookmarks, (GFunc) g_object_unref, NULL);
	g_ptr_array_free(priv->bookmarks, TRUE);

	g_free(priv->config_filename);
	g_free(priv->bookmarks_filename);
	g_free(priv->config_path);

	G_OBJECT_CLASS(gigolo_settings_parent_class)->finalize(object);
}


static void load_settings_read_config(GigoloSettingsPrivate *priv)
{
	GKeyFile *k;
	GError *error = NULL;
	gsize i;

	k = g_key_file_new();
	if (! g_key_file_load_from_file(k, priv->config_filename, G_KEY_FILE_NONE, &error))
	{
		verbose("Loading configuration file failed (%s).", error->message);
		g_error_free(error);
		error = NULL;
	}

	priv->file_manager = get_setting_string(k, SECTION_GENERAL, "file_manager", "gvfs-open");
	priv->autoconnect_interval = get_setting_int(k, SECTION_GENERAL, "autoconnect_interval",
		DEFAULT_AUTOCONNECT_INTERVAL);

	priv->save_geometry = get_setting_boolean(k, SECTION_UI, "save_geometry", TRUE);
	priv->show_in_systray = get_setting_boolean(k, SECTION_UI, "show_in_systray", TRUE);
	priv->start_in_systray = get_setting_boolean(k, SECTION_UI, "start_in_systray", FALSE);
	priv->show_panel = get_setting_boolean(k, SECTION_UI, "show_panel", FALSE);
	priv->last_panel_page = get_setting_int(k, SECTION_UI, "last_panel_page", 0);
	priv->show_autoconnect_errors = get_setting_boolean(k, SECTION_UI, "show_autoconnect_errors", TRUE);
	priv->show_toolbar = get_setting_boolean(k, SECTION_UI, "show_toolbar", TRUE);
	priv->toolbar_style = get_setting_int(k, SECTION_UI, "toolbar_style", -1);
	priv->toolbar_orientation = get_setting_int(k, SECTION_UI, "toolbar_orientation", 0);
	priv->view_mode = get_setting_int(k, SECTION_UI, "view_mode", 0);
	priv->geometry = g_key_file_get_integer_list(k, SECTION_UI, "geometry", NULL, &error);
	if (error)
	{
		g_error_free(error);
		error = NULL;
		priv->geometry = NULL;
	}
	else
	{
		/* don't use insane values: when main windows was maximized last time, pos might be
		 * negative at least on Windows for some reason */
		if (priv->geometry[4] != 1)
		{
			for (i = 0; i < 4; i++)
			{
				if (priv->geometry[i] < -1)
					priv->geometry[i] = -1;
			}
		}
	}

	g_key_file_free(k);
}


static void load_settings_read_bookmarks(GigoloSettingsPrivate *priv)
{
	GKeyFile *k;
	GError *error = NULL;
	gsize len, i;
	gchar **groups;
	gchar *scheme, *host, *user, *domain, *share, *folder, *path;
	gint port;
	gboolean autoconnect;
	GigoloBookmark *bm;

	k = g_key_file_new();
	if (! g_key_file_load_from_file(k, priv->bookmarks_filename, G_KEY_FILE_NONE, &error))
	{
		verbose("Loading bookmarks file failed (%s).", error->message);
		g_error_free(error);
		error = NULL;
	}

	/* read groups for bookmarks */
	groups = g_key_file_get_groups(k, &len);
	for (i = 0; i < len; i++)
	{
		scheme = get_setting_string(k, groups[i], "scheme", "");
		host = get_setting_string(k, groups[i], "host", "");
		folder = get_setting_string(k, groups[i], "folder", "");
		path = get_setting_string(k, groups[i], "path", "");
		user = get_setting_string(k, groups[i], "user", "");
		domain = get_setting_string(k, groups[i], "domain", "");
		share = get_setting_string(k, groups[i], "share", "");
		port = get_setting_int(k, groups[i], "port", 0);
		autoconnect = get_setting_int(k, groups[i], "autoconnect", FALSE);

		bm = gigolo_bookmark_new();
		gigolo_bookmark_set_name(bm, groups[i]);
		gigolo_bookmark_set_scheme(bm, scheme);
		if (NZV(host))
			gigolo_bookmark_set_host(bm, host);
		if (NZV(folder))
			gigolo_bookmark_set_folder(bm, folder);
		if (NZV(user))
			gigolo_bookmark_set_user(bm, user);
		if (NZV(domain))
			gigolo_bookmark_set_domain(bm, domain);
		if (NZV(path))
			gigolo_bookmark_set_path(bm, path);
		if (NZV(share))
			gigolo_bookmark_set_share(bm, share);
		gigolo_bookmark_set_port(bm, port);
		gigolo_bookmark_set_autoconnect(bm, autoconnect);

		g_ptr_array_add(priv->bookmarks, bm);

		g_free(scheme);
		g_free(host);
		g_free(folder);
		g_free(path);
		g_free(user);
		g_free(domain);
		g_free(share);
	}
	g_strfreev(groups);

	g_key_file_free(k);
}


static void check_for_old_dir(GigoloSettingsPrivate *priv)
{
	gchar *old_dir = g_build_filename(g_get_user_config_dir(), "sion", NULL);
	/* move the old config dir if it exists */
	if (g_file_test(old_dir, G_FILE_TEST_EXISTS))
	{
		if (! gigolo_message_dialog(NULL, GTK_MESSAGE_QUESTION, _("Move it now?"),
			_("Gigolo needs to move your old configuration directory before starting."), NULL))
			exit(0);

		if (g_rename(old_dir, priv->config_path) != 0)
		{
			/* for translators: the third %s in brackets is the error message which
			 * describes why moving the dir didn't work */
			gchar *msg = g_strdup_printf(
				_("Your old configuration directory \"%s\" could not be moved to \"%s\" (%s). "
				  "Please move manually the directory to the new location."),
				old_dir, priv->config_path, g_strerror(errno));
			gigolo_message_dialog(NULL, GTK_MESSAGE_WARNING, _("Warning"), msg, NULL);
		}
	}
	g_free(old_dir);
}


static void gigolo_settings_init(GigoloSettings *self)
{
	GigoloSettingsPrivate *priv = GIGOLO_SETTINGS_GET_PRIVATE(self);

	priv->config_path = g_build_filename(g_get_user_config_dir(), PACKAGE, NULL);
	priv->config_filename = g_build_filename(priv->config_path, "config", NULL);
	priv->bookmarks_filename = g_build_filename(priv->config_path, "bookmarks", NULL);

	priv->bookmarks = g_ptr_array_new();

	check_for_old_dir(priv);

	load_settings_read_config(priv);
	load_settings_read_bookmarks(priv);
}


GigoloSettings *gigolo_settings_new(void)
{
	return (GigoloSettings*) g_object_new(GIGOLO_SETTINGS_TYPE, NULL);
}


const gint *gigolo_settings_get_geometry(GigoloSettings *settings)
{
	g_return_val_if_fail(settings != NULL, NULL);

	return GIGOLO_SETTINGS_GET_PRIVATE(settings)->geometry;
}


void gigolo_settings_set_geometry(GigoloSettings *settings, const gint *geometry, gsize len)
{
	GigoloSettingsPrivate *priv;
	guint i;

	g_return_if_fail(settings != NULL);
	g_return_if_fail(geometry != NULL);
	g_return_if_fail(len > 0);

	priv = GIGOLO_SETTINGS_GET_PRIVATE(settings);

	g_free(priv->geometry);
	priv->geometry = g_new(gint, len);

	for (i = 0; i < len; i++)
	{
		priv->geometry[i] = geometry[i];
	}
}


GigoloBookmarkList *gigolo_settings_get_bookmarks(GigoloSettings *settings)
{
	g_return_val_if_fail(settings != NULL, NULL);

	return GIGOLO_SETTINGS_GET_PRIVATE(settings)->bookmarks;
}


gboolean gigolo_settings_get_boolean(GigoloSettings *settings, const gchar *property)
{
	gboolean value;

	g_return_val_if_fail(settings != NULL, FALSE);
	g_return_val_if_fail(property != NULL, FALSE);

	g_object_get(settings, property, &value, NULL);

	return value;
}


gint gigolo_settings_get_integer(GigoloSettings *settings, const gchar *property)
{
	gint value;

	g_return_val_if_fail(settings != NULL, FALSE);
	g_return_val_if_fail(property != NULL, FALSE);

	g_object_get(settings, property, &value, NULL);

	return value;
}


gchar *gigolo_settings_get_string(GigoloSettings *settings, const gchar *property)
{
	gchar *value;

	g_return_val_if_fail(settings != NULL, FALSE);
	g_return_val_if_fail(property != NULL, FALSE);

	g_object_get(settings, property, &value, NULL);

	return value;
}


gboolean gigolo_settings_has_file_manager(GigoloSettings *settings)
{
	GigoloSettingsPrivate *priv;

	g_return_val_if_fail(settings != NULL, FALSE);

	priv = GIGOLO_SETTINGS_GET_PRIVATE(settings);

	return NZV(priv->file_manager);
}


GigoloBookmark *gigolo_settings_get_bookmark_by_uri(GigoloSettings *settings, const gchar *uri)
{
	GigoloBookmarkList *bml;
	GigoloBookmark *bm = NULL;
	gboolean found = FALSE;
	gchar *tmp_uri;
	guint i;

	g_return_val_if_fail(settings != NULL, FALSE);
	g_return_val_if_fail(uri != NULL, FALSE);

	bml = gigolo_settings_get_bookmarks(settings);

	for (i = 0; i < bml->len && ! found; i++)
	{
		bm = g_ptr_array_index(bml, i);
		tmp_uri = gigolo_bookmark_get_uri_escaped(bm);
		if (gigolo_str_equal(uri, tmp_uri))
			found = TRUE;

		g_free(tmp_uri);
	}
	return (found) ? bm : NULL;
}


