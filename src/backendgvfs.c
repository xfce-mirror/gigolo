/*
 *      backendgvfs.c
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


#include <glib-object.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "common.h"
#include "backendgvfs.h"
#include "mountoperation.h"
#include "main.h"

typedef struct _GigoloBackendGVFSPrivate			GigoloBackendGVFSPrivate;

#define GIGOLO_BACKEND_GVFS_GET_PRIVATE(obj)		(G_TYPE_INSTANCE_GET_PRIVATE((obj),\
			GIGOLO_BACKEND_GVFS_TYPE, GigoloBackendGVFSPrivate))

enum
{
    PROP_0,
    PROP_STORE
};

enum
{
	MOUNTS_CHANGED,
	OPERATION_FAILED,

	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL];

typedef struct
{
	GigoloBackendGVFS *self;
	GtkWidget *dialog;
} MountInfo;

struct _GigoloBackendGVFSPrivate
{
	GtkListStore *store;
};

static void gigolo_backend_gvfs_finalize  			(GObject *object);
static void gigolo_backend_gvfs_set_property		(GObject *object, guint prop_id,
													 const GValue *value, GParamSpec *pspec);


G_DEFINE_TYPE(GigoloBackendGVFS, gigolo_backend_gvfs, G_TYPE_OBJECT);



static void gigolo_backend_gvfs_cclosure_marshal_VOID__STRING_STRING(
											GClosure		*closure,
							  G_GNUC_UNUSED GValue			*return_value,
											guint			 n_param_values,
											const GValue	*param_values,
							  G_GNUC_UNUSED gpointer		 invocation_hint,
											gpointer		 marshal_data)
{
	typedef void (*GMarshalFunc_VOID__STRING_STRING) (gpointer		 data1,
													  const gchar	*arg_1,
													  const gchar	*arg_2,
													  gpointer		 data2);
	register GMarshalFunc_VOID__STRING_STRING callback;
	register GCClosure* cc = (GCClosure*) closure;
	register gpointer data1, data2;

	g_return_if_fail(n_param_values == 3);

	if (G_CCLOSURE_SWAP_DATA(closure))
	{
		data1 = closure->data;
		data2 = g_value_peek_pointer(param_values + 0);
	}
	else
	{
		data1 = g_value_peek_pointer(param_values + 0);
		data2 = closure->data;
	}
	callback = (GMarshalFunc_VOID__STRING_STRING) (marshal_data ? marshal_data : cc->callback);
	callback(
		data1,
		g_value_get_string(param_values + 1),
		g_value_get_string(param_values + 2),
		data2);
}


static void gigolo_backend_gvfs_class_init(GigoloBackendGVFSClass *klass)
{
	GObjectClass *g_object_class;

	g_object_class = G_OBJECT_CLASS(klass);

	g_object_class->finalize = gigolo_backend_gvfs_finalize;
	g_object_class->set_property = gigolo_backend_gvfs_set_property;

	g_type_class_add_private(klass, sizeof(GigoloBackendGVFSPrivate));

	g_object_class_install_property(g_object_class,
										PROP_STORE,
										g_param_spec_object(
										"store",
										"Liststore",
										"The list store",
										GTK_TYPE_LIST_STORE,
										G_PARAM_WRITABLE));

	signals[MOUNTS_CHANGED] = g_signal_new("mounts-changed",
										G_TYPE_FROM_CLASS(klass),
										(GSignalFlags) 0,
										0,
										0,
										NULL,
										g_cclosure_marshal_VOID__VOID,
										G_TYPE_NONE, 0);
	signals[OPERATION_FAILED] = g_signal_new("operation-failed",
										G_TYPE_FROM_CLASS(klass),
										(GSignalFlags) 0,
										0,
										0,
										NULL,
										gigolo_backend_gvfs_cclosure_marshal_VOID__STRING_STRING,
										G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);
}


static void gigolo_backend_gvfs_finalize(GObject *object)
{
	GigoloBackendGVFS *self;

	self = GIGOLO_BACKEND_GVFS(object);

	G_OBJECT_CLASS(gigolo_backend_gvfs_parent_class)->finalize(object);
}


static gchar *get_tooltip_text(gpointer ref, gint ref_type, const gchar *type)
{
	gchar *result = NULL;
	switch (ref_type)
	{
		case GIGOLO_WINDOW_REF_TYPE_MOUNT:
		{
			gchar *uri, *name, *clean_uri;

			gigolo_backend_gvfs_get_name_and_uri_from_mount(ref, &name, &uri);
			clean_uri = g_uri_unescape_string(uri, G_URI_RESERVED_CHARS_ALLOWED_IN_USERINFO);
			result = g_strdup_printf(
				_("<b>%s</b>\n\nURI: %s\nConnected: Yes\nService Type: %s"), name, clean_uri, type);

			g_free(clean_uri);
			g_free(uri);
			g_free(name);
			return result;
		}
		case GIGOLO_WINDOW_REF_TYPE_VOLUME:
		default:
		{
			gchar *label = gigolo_backend_gvfs_get_volume_identifier(ref);

			if (NZV(label))
			{
				result = g_strdup_printf(_("<b>Unix device: %s</b>"), label);
			}
			g_free(label);
			return result;
		}
	}
}


static void mount_volume_changed_cb(GVolumeMonitor *vm, G_GNUC_UNUSED GMount *mnt, gpointer backend)
{
	GList *mounts, *volumes, *item;
	GFile *file;
	GMount *mount;
	GVolume *volume;
	GIcon *icon;
	GtkTreeIter iter;
	gchar *vol_name, *scheme, *uri, *tooltip_text;
	const gchar *scheme_name;
	GigoloBackendGVFSPrivate *priv = GIGOLO_BACKEND_GVFS_GET_PRIVATE(backend);

	gtk_list_store_clear(priv->store);

	/* list mounts */
	mounts = g_volume_monitor_get_mounts(vm);
	for (item = mounts; item != NULL; item = g_list_next(item))
	{
		mount = G_MOUNT(item->data);
		vol_name = g_mount_get_name(mount);
		file = g_mount_get_root(mount);
		scheme = g_file_get_uri_scheme(file);
		if (gigolo_str_equal(scheme, "burn"))
		{	/* ignore empty CDs which are listed as mounted to burn:// */
			g_free(vol_name);
			g_free(scheme);
			g_object_unref(file);
			continue;
		}
		scheme_name = gigolo_describe_scheme(scheme);
		uri = g_file_get_uri(file);
		icon = g_mount_get_icon(mount);
		tooltip_text = get_tooltip_text(mount, GIGOLO_WINDOW_REF_TYPE_MOUNT, scheme_name);

		gtk_list_store_append(priv->store, &iter);
		gtk_list_store_set(priv->store, &iter,
				GIGOLO_WINDOW_COL_IS_MOUNTED, TRUE,
				GIGOLO_WINDOW_COL_NAME, vol_name,
				GIGOLO_WINDOW_COL_SCHEME, scheme_name,
				GIGOLO_WINDOW_COL_REF, mount,
				GIGOLO_WINDOW_COL_REF_TYPE, GIGOLO_WINDOW_REF_TYPE_MOUNT,
				GIGOLO_WINDOW_COL_PIXBUF, icon,
				GIGOLO_WINDOW_COL_ICON_NAME, "folder-remote",
				GIGOLO_WINDOW_COL_TOOLTIP, tooltip_text,
				-1);
		g_free(vol_name);
		g_free(scheme);
		g_free(uri);
		g_free(tooltip_text);
		g_object_unref(file);
		g_object_unref(icon);
	}
	g_list_foreach(mounts, (GFunc) g_object_unref, NULL);
	g_list_free(mounts);

	/* list volumes */
	volumes = g_volume_monitor_get_volumes(vm);
	for (item = volumes; item != NULL; item = g_list_next(item))
	{
		volume = G_VOLUME(item->data);
		mount = g_volume_get_mount(volume);
		/* display this volume only if it is not mounted, otherwise it will be listed as mounted */
		if (mount == NULL)
		{
			icon = g_volume_get_icon(volume);
			vol_name = g_volume_get_name(volume);
			tooltip_text = get_tooltip_text(volume, GIGOLO_WINDOW_REF_TYPE_VOLUME, NULL);

			gtk_list_store_append(priv->store, &iter);
			gtk_list_store_set(priv->store, &iter,
					GIGOLO_WINDOW_COL_IS_MOUNTED, FALSE,
					GIGOLO_WINDOW_COL_NAME, vol_name,
					GIGOLO_WINDOW_COL_SCHEME, gigolo_describe_scheme("file"),
					GIGOLO_WINDOW_COL_REF, volume,
					GIGOLO_WINDOW_COL_REF_TYPE, GIGOLO_WINDOW_REF_TYPE_VOLUME,
					GIGOLO_WINDOW_COL_PIXBUF, icon,
					GIGOLO_WINDOW_COL_ICON_NAME, "folder-remote",
					GIGOLO_WINDOW_COL_TOOLTIP, tooltip_text,
					-1);
			g_free(vol_name);
			g_free(tooltip_text);
			g_object_unref(icon);
		}
		else
			g_object_unref(mount);
	}
	g_list_foreach(volumes, (GFunc) g_object_unref, NULL);
	g_list_free(volumes);

	g_signal_emit(backend, signals[MOUNTS_CHANGED], 0);
}


static void gigolo_backend_gvfs_set_property(GObject *object, guint prop_id,
										   const GValue *value, GParamSpec *pspec)
{
	switch (prop_id)
	{
	case PROP_STORE:
	{
		GVolumeMonitor *gvm;
		GigoloBackendGVFSPrivate *priv = GIGOLO_BACKEND_GVFS_GET_PRIVATE(object);

		priv->store = g_value_get_object(value);

		gvm = g_volume_monitor_get();
		g_signal_connect(gvm, "mount-added", G_CALLBACK(mount_volume_changed_cb), object);
		g_signal_connect(gvm, "mount-changed", G_CALLBACK(mount_volume_changed_cb), object);
		g_signal_connect(gvm, "mount-removed", G_CALLBACK(mount_volume_changed_cb), object);
		g_signal_connect(gvm, "volume-added", G_CALLBACK(mount_volume_changed_cb), object);
		g_signal_connect(gvm, "volume-changed", G_CALLBACK(mount_volume_changed_cb), object);
		g_signal_connect(gvm, "volume-removed", G_CALLBACK(mount_volume_changed_cb), object);

		/* fill the list store once */
		mount_volume_changed_cb(gvm, NULL, object);
		break;
	}
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}


static void gigolo_backend_gvfs_init(G_GNUC_UNUSED GigoloBackendGVFS *self)
{
}


GigoloBackendGVFS *gigolo_backend_gvfs_new(GtkListStore *store)
{
	GigoloBackendGVFS *backend = g_object_new(GIGOLO_BACKEND_GVFS_TYPE, "store", store, NULL);

	return backend;
}


gboolean gigolo_backend_gvfs_is_mount(gpointer mnt)
{
	g_return_val_if_fail(mnt != NULL, FALSE);

	return G_IS_MOUNT(mnt);
}


void gigolo_backend_gvfs_get_name_and_uri_from_mount(GMount *mount, gchar **name, gchar **uri)
{
	GFile *file;

	g_return_if_fail(mount != NULL);

	file = g_mount_get_root(mount);
	if (name != NULL)
		*name = g_mount_get_name(mount);
	if (uri != NULL)
		*uri = g_file_get_uri(file);

	g_object_unref(file);
}


static void volume_mount_finished_cb(GObject *src, GAsyncResult *res, gpointer backend)
{
	GError *error = NULL;

	if (! g_volume_mount_finish(G_VOLUME(src), res, &error))
	{
		gchar *name, *msg;

		if (G_IS_VOLUME(src))
			name = g_volume_get_name(G_VOLUME(src));
		else
		{
			gigolo_backend_gvfs_get_name_and_uri_from_mount(G_MOUNT(src), &name, NULL);
			if (name == NULL)
				name = g_strdup(_("unknown"));
		}

		g_warning("Mounting of \"%s\" failed: %s", name, error->message);
		msg = g_strdup_printf(_("Connecting to \"%s\" failed."), name);

		g_signal_emit(backend, signals[OPERATION_FAILED], 0, msg, error->message);

		g_error_free(error);
		g_free(name);
		g_free(msg);
	}
	else
		verbose("Mount finished sucessfully");
}


static void unmount_finished_cb(GObject *src, GAsyncResult *res, gpointer backend)
{
	GError *error = NULL;

	if (! g_mount_unmount_finish(G_MOUNT(src), res, &error))
	{
		gchar *name, *msg;

		if (G_IS_VOLUME(src))
			name = g_volume_get_name(G_VOLUME(src));
		else
		{
			gigolo_backend_gvfs_get_name_and_uri_from_mount(G_MOUNT(src), &name, NULL);
			if (name == NULL)
				name = g_strdup(_("unknown"));
		}

		g_warning("Unmounting of \"%s\" failed: %s", name, error->message);
		msg = g_strdup_printf(_("Disconnecting from \"%s\" failed."), name);

		g_signal_emit(backend, signals[OPERATION_FAILED], 0, msg, error->message);

		g_error_free(error);
		g_free(name);
		g_free(msg);
	}
}


gboolean gigolo_backend_gvfs_mount_volume(GigoloBackendGVFS *backend, GVolume *vol)
{
	g_return_val_if_fail(backend != NULL, FALSE);
	g_return_val_if_fail(vol != NULL, FALSE);

	if (! G_IS_MOUNT(vol) && G_IS_VOLUME(vol) && g_volume_can_mount(vol))
	{
		g_volume_mount(vol, G_MOUNT_MOUNT_NONE, NULL, NULL, volume_mount_finished_cb, backend);
		return TRUE;
	}

	return FALSE;
}


void gigolo_backend_gvfs_unmount_mount(GigoloBackendGVFS *backend, GMount *mount)
{
	g_return_if_fail(backend != NULL);
	g_return_if_fail(mount != NULL);

	g_mount_unmount(mount, G_MOUNT_UNMOUNT_NONE, NULL, unmount_finished_cb, backend);
}


static void mount_ready_cb(GFile *location, GAsyncResult *res, MountInfo *mi)
{
	gchar *uri;
	gboolean success;
	GError *error = NULL;

	uri = g_file_get_uri(location);
	success = g_file_mount_enclosing_volume_finish(location, res, &error);

	if (error != NULL && ! g_error_matches(error, G_IO_ERROR, G_IO_ERROR_ALREADY_MOUNTED))
	{
		gchar *msg = g_strdup_printf(_("Connecting to \"%s\" failed."), uri);
		g_signal_emit(mi->self, signals[OPERATION_FAILED], 0, msg, error->message);
		g_free(msg);
	}

	if (error != NULL)
		g_error_free(error);

	if (mi->dialog != NULL)
		gtk_widget_destroy(mi->dialog);

	g_free(uri);
	g_free(mi);
}


void gigolo_backend_gvfs_mount_uri(GigoloBackendGVFS *backend, const gchar *uri,
								   GtkWindow *parent, GtkWidget *dialog)
{
	GMountOperation *op;
	GFile *file;
	MountInfo *mi;

	g_return_if_fail(uri != NULL);
	g_return_if_fail(backend != NULL);

	op = gigolo_mount_operation_new(GTK_WINDOW(parent));
	file = g_file_new_for_uri(uri);
	mi = g_new0(MountInfo, 1);
	mi->self = backend;
	mi->dialog = dialog;

	g_file_mount_enclosing_volume(file, G_MOUNT_MOUNT_NONE, op, NULL,
		(GAsyncReadyCallback) mount_ready_cb, mi);

	g_object_unref(file);
}


gchar *gigolo_backend_gvfs_get_volume_identifier(GVolume *volume)
{
	g_return_val_if_fail(volume != NULL, NULL);

	return g_volume_get_identifier(volume, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
}


gchar **gigolo_backend_gvfs_get_smb_shares_from_uri(const gchar *uri)
{
	gchar **shares = NULL;
	GList *l, *shares_list = NULL;
	GFile *file;
	GFileInfo *info;
	GError *error = NULL;
	GFileEnumerator *e;

	g_return_val_if_fail(uri != NULL, NULL);

	verbose("Querying \"%s\" for available shares", uri);

	file = g_file_new_for_uri(uri);

	e = g_file_enumerate_children(file,
		G_FILE_ATTRIBUTE_MOUNTABLE_CAN_MOUNT "," G_FILE_ATTRIBUTE_STANDARD_NAME,
		G_FILE_QUERY_INFO_NONE, NULL, &error);

	if (error != NULL)
	{
		verbose("%s: %s", G_STRFUNC, error->message);
		g_error_free(error);
	}
	else
	{
		guint i, len;

		while ((info = g_file_enumerator_next_file(e, NULL, NULL)) != NULL)
		{
			shares_list = g_list_append(shares_list, g_strdup(g_file_info_get_name(info)));

			g_object_unref(info);
		}
		g_object_unref(e);

		i = 0;
		len = g_list_length(shares_list);
		shares = g_new(gchar*, len + 1);

		for (l = shares_list; l != NULL; l = g_list_next(l))
		{
			shares[i] = l->data;
			i++;
		}
		shares[i] = NULL;
		g_list_free(shares_list);
	}

	g_object_unref(file);

	return shares;
}


gchar **gigolo_backend_gvfs_get_smb_shares(const gchar *hostname, const gchar *user, const gchar *domain)
{
	gchar **shares = NULL;
	gchar *uri;

	g_return_val_if_fail(hostname != NULL, NULL);

	uri = g_strdup_printf("smb://%s%s%s%s%s/",
		(NZV(domain)) ? domain : "",
		(NZV(domain)) ? ";" : "",
		(NZV(user)) ? user : "",
		(NZV(user) || NZV(domain)) ? "@" : "",
		hostname);

	shares = gigolo_backend_gvfs_get_smb_shares_from_uri(uri);
	g_free(uri);

	return shares;
}


GigoloHostUri **gigolo_backend_gvfs_browse_network(void)
{
	GigoloHostUri *h, **hosts = NULL;
	GList *l, *hosts_list = NULL;
	GFile *file;
	GFileInfo *info;
	GError *error = NULL;
	GFileEnumerator *e;

	file = g_file_new_for_uri("network://");

	e = g_file_enumerate_children(file,
		G_FILE_ATTRIBUTE_MOUNTABLE_CAN_MOUNT ","
		G_FILE_ATTRIBUTE_STANDARD_TARGET_URI ","
		G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME ","
		G_FILE_ATTRIBUTE_STANDARD_ICON,
		G_FILE_QUERY_INFO_NONE, NULL, &error);

	if (error != NULL)
	{
		verbose("%s: %s", G_STRFUNC, error->message);
		g_error_free(error);
	}
	else
	{
		guint i, len;
		const gchar *uri;

		while ((info = g_file_enumerator_next_file(e, NULL, NULL)) != NULL)
		{
			uri = g_file_info_get_attribute_string(info, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI);
			if (uri != NULL && g_str_has_prefix(uri, "smb://"))
			{
				h = g_new(GigoloHostUri, 1);
				h->name = g_strdup(g_file_info_get_display_name(info));
				h->uri = g_strdup(uri);
				h->icon = g_object_ref(g_file_info_get_icon(info));
				hosts_list = g_list_append(hosts_list, h);
			}
			g_object_unref(info);
		}
		g_object_unref(e);

		i = 0;
		len = g_list_length(hosts_list);
		hosts = g_new(GigoloHostUri*, len + 1);

		for (l = hosts_list; l != NULL; l = g_list_next(l))
		{
			hosts[i] = (GigoloHostUri*) l->data;
			i++;
		}
		hosts[i] = NULL;
		g_list_free(hosts_list);
	}

	g_object_unref(file);

	return hosts;
}


gpointer gigolo_backend_gvfs_get_share_icon(void)
{
	return g_themed_icon_new("folder-remote");
}


const gchar *const *gigolo_backend_gvfs_get_supported_uri_schemes(void)
{
	return g_vfs_get_supported_uri_schemes(g_vfs_get_default());
}


gboolean gigolo_backend_gvfs_is_scheme_supported(const gchar *scheme)
{
	const gchar *const *schemes = gigolo_backend_gvfs_get_supported_uri_schemes();
	guint i;

	g_return_val_if_fail(scheme != NULL, FALSE);

	for (i = 0; schemes[i] != NULL; i++)
	{
		if (gigolo_str_equal(schemes[i], scheme))
			return TRUE;
	}

	return FALSE;
}
