/*
 *      settings.c
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

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "settings.h"
#include "common.h"
#include "bookmark.h"
#include "main.h"


typedef struct _SionSettingsPrivate			SionSettingsPrivate;

#define SION_SETTINGS_GET_PRIVATE(obj)		(G_TYPE_INSTANCE_GET_PRIVATE((obj),\
		SION_SETTINGS_TYPE, SionSettingsPrivate))

struct _SionSettingsPrivate
{
	gchar		*config_path;
	gchar		*config_filename;
	gchar		*bookmarks_filename;

	gboolean	save_geometry;
	gboolean	show_trayicon;
	gboolean	show_toolbar;
	gint		toolbar_style;
	gint		toolbar_orientation;
	gint		view_mode;

	gchar		*file_manager;
	gchar		*vm_impl; // GVolumeMonitor implementation to use
	gint		*geometry; // window size and position, field 4 is a flag for maximized state

	SionBookmarkList *bookmarks; // array of known bookmarks
};

static void sion_settings_class_init		(SionSettingsClass *klass);
static void sion_settings_init				(SionSettings *self);
static void sion_settings_finalize			(GObject* object);

static GObjectClass *parent_class = NULL;

// keyfile section names
#define SECTION_GENERAL	"general"
#define SECTION_UI		"ui"

enum
{
	PROP_0,

	PROP_FILE_MANAGER,

	PROP_SAVE_GEOMETRY,
	PROP_SHOW_TRAYICON,
	PROP_SHOW_TOOLBAR,
	PROP_TOOLBAR_STYLE,
	PROP_TOOLBAR_ORIENTATION,
	PROP_VIEW_MODE
};

GType sion_settings_get_type(void)
{
	static GType self_type = 0;
	if (! self_type)
	{
		static const GTypeInfo self_info =
		{
			sizeof(SionSettingsClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc)sion_settings_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(SionSettings),
			0,
			(GInstanceInitFunc)sion_settings_init,
			NULL /* value_table */
		};

		self_type = g_type_register_static(G_TYPE_OBJECT, "SionSettings", &self_info, 0);
	}

	return self_type;
}


static void sion_settings_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	SionSettingsPrivate *priv = SION_SETTINGS_GET_PRIVATE(object);

	switch (prop_id)
	{
	case PROP_SAVE_GEOMETRY:
		priv->save_geometry = g_value_get_boolean(value);
		break;
	case PROP_SHOW_TRAYICON:
		priv->show_trayicon = g_value_get_boolean(value);
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
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}


static void sion_settings_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	SionSettingsPrivate *priv = SION_SETTINGS_GET_PRIVATE(object);

	switch (prop_id)
	{
	case PROP_SAVE_GEOMETRY:
		g_value_set_boolean(value, priv->save_geometry);
		break;
	case PROP_SHOW_TRAYICON:
		g_value_set_boolean(value, priv->show_trayicon);
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
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}


static void sion_settings_class_init(SionSettingsClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = sion_settings_finalize;
	gobject_class->get_property = sion_settings_get_property;
	gobject_class->set_property = sion_settings_set_property;

	parent_class = (GObjectClass*)g_type_class_peek(G_TYPE_OBJECT);
	g_type_class_add_private((gpointer)klass, sizeof(SionSettingsPrivate));

	g_object_class_install_property(gobject_class,
									PROP_SAVE_GEOMETRY,
									g_param_spec_boolean(
									"save-geometry",
									"Save window position and geometry",
									"Saves the window position and geometry and restores it at the start",
									TRUE,
									G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class,
									PROP_SHOW_TRAYICON,
									g_param_spec_boolean(
									"show-trayicon",
									"show-trayicon",
									"Whether to show the trayicon",
									TRUE,
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


static void write_settings_config(SionSettings *settings)
{
	GKeyFile *k;
	SionSettingsPrivate *priv = SION_SETTINGS_GET_PRIVATE(settings);

	if (! g_file_test(priv->config_path, G_FILE_TEST_IS_DIR))
		g_mkdir_with_parents(priv->config_path, 0700);

	k = g_key_file_new();

	if (priv->vm_impl != NULL)
		g_key_file_set_string(k, SECTION_GENERAL, "vm_impl", priv->vm_impl);
	if (priv->file_manager != NULL)
		g_key_file_set_string(k, SECTION_GENERAL, "file_manager", priv->file_manager);

	if (priv->geometry != NULL)
		g_key_file_set_integer_list(k, SECTION_UI, "geometry", priv->geometry, 5);
	g_key_file_set_boolean(k, SECTION_UI, "save_geometry", priv->save_geometry);
	g_key_file_set_boolean(k, SECTION_UI, "show_trayicon", priv->show_trayicon);
	g_key_file_set_boolean(k, SECTION_UI, "show_toolbar", priv->show_toolbar);
	g_key_file_set_integer(k, SECTION_UI, "toolbar_style", priv->toolbar_style);
	g_key_file_set_integer(k, SECTION_UI, "toolbar_orientation", priv->toolbar_orientation);
	g_key_file_set_integer(k, SECTION_UI, "view_mode", priv->view_mode);

	write_data(k, priv->config_filename);

	g_key_file_free(k);
}


static void write_settings_bookmarks(SionSettings *settings)
{
	GKeyFile *k;
	const gchar *name;
	gsize i;
	SionBookmark *bm;
	SionBookmarkList *bml;
	SionSettingsPrivate *priv = SION_SETTINGS_GET_PRIVATE(settings);

	if (! g_file_test(priv->config_path, G_FILE_TEST_IS_DIR))
		g_mkdir_with_parents(priv->config_path, 0700);

	k = g_key_file_new();

	bml = sion_settings_get_bookmarks(settings);
	for (i = 0; i < bml->len; i++)
	{
		bm = g_ptr_array_index(bml, i);
		if (IS_SION_BOOKMARK(bm))
		{
			name = sion_bookmark_get_name(bm);
			set_setting_string(k, name, "host", sion_bookmark_get_host(bm));
			set_setting_string(k, name, "user", sion_bookmark_get_user(bm));
			set_setting_string(k, name, "scheme", sion_bookmark_get_scheme(bm));
			set_setting_string(k, name, "share", sion_bookmark_get_share(bm));
			set_setting_string(k, name, "domain", sion_bookmark_get_domain(bm));
			set_setting_int(k, name, "port", sion_bookmark_get_port(bm));
		}
	}

	write_data(k, priv->bookmarks_filename);

	g_key_file_free(k);
}


void sion_settings_write(SionSettings *settings)
{
	g_return_if_fail(settings != NULL);

	write_settings_config(settings);
	write_settings_bookmarks(settings);
}


static void sion_settings_finalize(GObject* object)
{
	SionSettingsPrivate *priv = SION_SETTINGS_GET_PRIVATE(object);

	sion_settings_write(SION_SETTINGS(object));

	g_free(priv->vm_impl);
	g_free(priv->geometry);

	g_ptr_array_foreach(priv->bookmarks, (GFunc) g_object_unref, NULL);
	g_ptr_array_free(priv->bookmarks, TRUE);

	g_free(priv->config_filename);
	g_free(priv->bookmarks_filename);
	g_free(priv->config_path);

	G_OBJECT_CLASS(parent_class)->finalize(object);
}


static void load_settings_read_config(SionSettingsPrivate *priv)
{
	GKeyFile *k;
	GError *error = NULL;
	const gchar *default_vm_impl = "hal";
	gsize i;

	k = g_key_file_new();
	if (! g_key_file_load_from_file(k, priv->config_filename, G_KEY_FILE_NONE, &error))
	{
		verbose("Loading configuration file failed (%s).", error->message);
		g_error_free(error);
		error = NULL;
	}

	priv->vm_impl = get_setting_string(k, SECTION_GENERAL, "vm_impl", default_vm_impl);
	priv->file_manager = get_setting_string(k, SECTION_GENERAL, "file_manager", "gvfs-open");

	priv->save_geometry = get_setting_boolean(k, SECTION_UI, "save_geometry", TRUE);
	priv->show_trayicon = get_setting_boolean(k, SECTION_UI, "show_trayicon", TRUE);
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


static void load_settings_read_bookmarks(SionSettingsPrivate *priv)
{
	GKeyFile *k;
	GError *error = NULL;
	gsize len, i;
	gchar **groups;
	gchar *scheme, *host, *user, *domain, *share;
	gint port;
	SionBookmark *bm;

	k = g_key_file_new();
	if (! g_key_file_load_from_file(k, priv->bookmarks_filename, G_KEY_FILE_NONE, &error))
	{
		verbose("Loading bookmarks file failed (%s).", error->message);
		g_error_free(error);
		error = NULL;
	}

	// read groups for bookmarks
	groups = g_key_file_get_groups(k, &len);
	for (i = 0; i < len; i++)
	{
		scheme = get_setting_string(k, groups[i], "scheme", "");
		host = get_setting_string(k, groups[i], "host", "");
		user = get_setting_string(k, groups[i], "user", "");
		domain = get_setting_string(k, groups[i], "domain", "");
		share = get_setting_string(k, groups[i], "share", "");
		port = get_setting_int(k, groups[i], "port", 0);

		bm = sion_bookmark_new();
		sion_bookmark_set_name(bm, groups[i]);
		sion_bookmark_set_scheme(bm, scheme);
		if (NZV(host))
			sion_bookmark_set_host(bm, host);
		if (NZV(user))
			sion_bookmark_set_user(bm, user);
		if (NZV(domain))
			sion_bookmark_set_domain(bm, domain);
		if (NZV(share))
			sion_bookmark_set_share(bm, share);
		sion_bookmark_set_port(bm, port);

		g_ptr_array_add(priv->bookmarks, bm);

		g_free(scheme);
		g_free(host);
		g_free(user);
		g_free(domain);
		g_free(share);
	}
	g_strfreev(groups);

	g_key_file_free(k);
}


static void sion_settings_init(SionSettings *self)
{
	SionSettingsPrivate *priv = SION_SETTINGS_GET_PRIVATE(self);

	priv->config_path = g_build_filename(g_get_user_config_dir(), PACKAGE, NULL);
	priv->config_filename = g_build_filename(priv->config_path, "config", NULL);
	priv->bookmarks_filename = g_build_filename(priv->config_path, "bookmarks", NULL);

	priv->bookmarks = g_ptr_array_new();

	load_settings_read_config(priv);
	load_settings_read_bookmarks(priv);
}


SionSettings *sion_settings_new(void)
{
	return (SionSettings*) g_object_new(SION_SETTINGS_TYPE, NULL);
}


const gchar *sion_settings_get_vm_impl(SionSettings *settings)
{
	g_return_val_if_fail(settings != NULL, NULL);

	return SION_SETTINGS_GET_PRIVATE(settings)->vm_impl;
}


void sion_settings_set_vm_impl(SionSettings *settings, const gchar *impl)
{
	SionSettingsPrivate *priv;

	g_return_if_fail(settings != NULL);

	priv = SION_SETTINGS_GET_PRIVATE(settings);

	if (impl == NULL)
		impl = "hal";

	g_free(priv->vm_impl);
	priv->vm_impl = g_strdup(impl);
}


const gint *sion_settings_get_geometry(SionSettings *settings)
{
	g_return_val_if_fail(settings != NULL, NULL);

	return SION_SETTINGS_GET_PRIVATE(settings)->geometry;
}


void sion_settings_set_geometry(SionSettings *settings, const gint *geometry, gsize len)
{
	SionSettingsPrivate *priv;
	guint i;

	g_return_if_fail(settings != NULL);
	g_return_if_fail(geometry != NULL);
	g_return_if_fail(len > 0);

	priv = SION_SETTINGS_GET_PRIVATE(settings);

	g_free(priv->geometry);
	priv->geometry = g_new(gint, len);

	for (i = 0; i < len; i++)
	{
		priv->geometry[i] = geometry[i];
	}
}


SionBookmarkList *sion_settings_get_bookmarks(SionSettings *settings)
{
	g_return_val_if_fail(settings != NULL, NULL);

	return SION_SETTINGS_GET_PRIVATE(settings)->bookmarks;
}


gboolean sion_settings_get_boolean(SionSettings *settings, const gchar *property)
{
	gboolean value;

	g_return_val_if_fail(settings != NULL, FALSE);
	g_return_val_if_fail(property != NULL, FALSE);

	g_object_get(settings, property, &value, NULL);

	return value;
}


gint sion_settings_get_integer(SionSettings *settings, const gchar *property)
{
	gint value;

	g_return_val_if_fail(settings != NULL, FALSE);
	g_return_val_if_fail(property != NULL, FALSE);

	g_object_get(settings, property, &value, NULL);

	return value;
}


gchar *sion_settings_get_string(SionSettings *settings, const gchar *property)
{
	gchar *value;

	g_return_val_if_fail(settings != NULL, FALSE);
	g_return_val_if_fail(property != NULL, FALSE);

	g_object_get(settings, property, &value, NULL);

	return value;
}


gboolean sion_settings_has_file_manager(SionSettings *settings)
{
	SionSettingsPrivate *priv;

	g_return_val_if_fail(settings != NULL, FALSE);

	priv = SION_SETTINGS_GET_PRIVATE(settings);

	return NZV(priv->file_manager);
}
