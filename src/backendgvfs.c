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
#include "bookmark.h"
#include "settings.h"
#include "window.h"
#include "mountoperation.h"

typedef struct _GigoloBackendGVFSPrivate			GigoloBackendGVFSPrivate;

#define GIGOLO_BACKEND_GVFS_GET_PRIVATE(obj)		(G_TYPE_INSTANCE_GET_PRIVATE((obj),\
			GIGOLO_BACKEND_GVFS_TYPE, GigoloBackendGVFSPrivate))

enum
{
    PROP_0,
    PROP_PARENT,
    PROP_STORE
};

enum
{
	MOUNTS_CHANGED,
	OPERATION_FAILED,
	BROWSE_NETWORK_FINISHED,
	BROWSE_HOST_FINISHED,

	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL];

enum
{
    BROWSE_MODE_INVALID,
    BROWSE_MODE_DOMAINS,
    BROWSE_MODE_HOSTS,
    BROWSE_MODE_SHARES,
};

typedef struct
{
	GigoloBackendGVFS *self;
	GtkWidget *dialog;
	gboolean show_errors;
} MountInfo;

typedef struct BrowseData
{
	GigoloBackendGVFS *self;

	gint mode;
	gchar *uri;
	GtkWindow *parent;
	GtkTreeStore *store;
	GtkTreePath *parent_path;

	void (*browse_func) (struct BrowseData *bd);
} BrowseData;


struct _GigoloBackendGVFSPrivate
{
	GtkWindow *parent;
	GtkListStore *store;

	gint browse_counter;
};

static void gigolo_backend_gvfs_finalize  			(GObject *object);
static void gigolo_backend_gvfs_set_property		(GObject *object, guint prop_id,
													 const GValue *value, GParamSpec *pspec);
static void browse_network_real						(BrowseData *bd);
static void browse_host_real						(BrowseData *bd);


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
										PROP_PARENT,
										g_param_spec_object(
										"parent",
										"Parent",
										"Parent window",
										GTK_TYPE_WINDOW,
										G_PARAM_WRITABLE));

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
	signals[BROWSE_NETWORK_FINISHED] = g_signal_new("browse-network-finished",
										G_TYPE_FROM_CLASS(klass),
										(GSignalFlags) 0,
										0,
										0,
										NULL,
										g_cclosure_marshal_VOID__VOID,
										G_TYPE_NONE, 0);
	signals[BROWSE_HOST_FINISHED] = g_signal_new("browse-host-finished",
										G_TYPE_FROM_CLASS(klass),
										(GSignalFlags) 0,
										0,
										0,
										NULL,
										/* FIXME use a proper closure, arg1 is a GList* */
										g_cclosure_marshal_VOID__POINTER,
										G_TYPE_NONE, 1, G_TYPE_POINTER);
}


static void gigolo_backend_gvfs_finalize(GObject *object)
{
	GigoloBackendGVFS *self;

	self = GIGOLO_BACKEND_GVFS(object);

	G_OBJECT_CLASS(gigolo_backend_gvfs_parent_class)->finalize(object);
}


static gchar *get_tooltip_text(GigoloBackendGVFS *backend, gpointer ref, gint ref_type, const gchar *type)
{
	gchar *result = NULL;
	GigoloBackendGVFSPrivate *priv = GIGOLO_BACKEND_GVFS_GET_PRIVATE(backend);
	switch (ref_type)
	{
		case GIGOLO_WINDOW_REF_TYPE_MOUNT:
		{
			gchar *uri, *name, *clean_uri;
			GigoloBookmark *b;
			GigoloSettings *settings;

			gigolo_backend_gvfs_get_name_and_uri_from_mount(ref, &name, &uri);
			clean_uri = g_uri_unescape_string(uri, G_URI_RESERVED_CHARS_ALLOWED_IN_USERINFO);

			settings = gigolo_window_get_settings(GIGOLO_WINDOW(priv->parent));
			b = gigolo_settings_get_bookmark_by_uri(settings, uri);
			if (b != NULL)
			{
				const gchar *folder = gigolo_bookmark_get_folder(b);
				if (NZV(folder))
					setptr(clean_uri, g_build_filename(clean_uri, folder, NULL));
			}

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
		tooltip_text = get_tooltip_text(backend, mount, GIGOLO_WINDOW_REF_TYPE_MOUNT, scheme_name);

		gtk_list_store_insert_with_values(priv->store, &iter, -1,
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
			tooltip_text = get_tooltip_text(backend, volume, GIGOLO_WINDOW_REF_TYPE_VOLUME, NULL);

			gtk_list_store_insert_with_values(priv->store, &iter, -1,
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
	GigoloBackendGVFSPrivate *priv = GIGOLO_BACKEND_GVFS_GET_PRIVATE(object);

	switch (prop_id)
	{
	case PROP_PARENT:
	{
		priv->parent = g_value_get_object(value);
		break;
	}
	case PROP_STORE:
	{
		GVolumeMonitor *gvm;

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


GigoloBackendGVFS *gigolo_backend_gvfs_new(void)
{
	return g_object_new(GIGOLO_BACKEND_GVFS_TYPE, NULL);
}


gboolean gigolo_backend_gvfs_is_mount(gpointer mount)
{
	g_return_val_if_fail(mount != NULL, FALSE);

	return G_IS_MOUNT(mount);
}


void gigolo_backend_gvfs_get_name_and_uri_from_mount(gpointer mount, gchar **name, gchar **uri)
{
	GFile *file;

	g_return_if_fail(mount != NULL);

	file = g_mount_get_root(G_MOUNT(mount));
	if (name != NULL)
		*name = g_mount_get_name(G_MOUNT(mount));
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
		if (! g_error_matches(error, G_IO_ERROR, G_IO_ERROR_FAILED_HANDLED))
		{
			msg = g_strdup_printf(_("Connecting to \"%s\" failed."), name);
			g_signal_emit(backend, signals[OPERATION_FAILED], 0, msg, error->message);
			g_free(msg);
		}
		g_error_free(error);
		g_free(name);
	}
	else
		verbose("Mount finished sucessfully");
}


static void unmount_finished_cb(GObject *src, GAsyncResult *res, gpointer backend)
{
	GError *error = NULL;

#if GLIB_CHECK_VERSION(2, 22, 0)
	if (! g_mount_unmount_with_operation_finish(G_MOUNT(src), res, &error))
#else
	if (! g_mount_unmount_finish(G_MOUNT(src), res, &error))
#endif
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


gboolean gigolo_backend_gvfs_mount_volume(GigoloBackendGVFS *backend, GtkWindow *window, gpointer vol)
{
	g_return_val_if_fail(backend != NULL, FALSE);
	g_return_val_if_fail(window != NULL, FALSE);
	g_return_val_if_fail(vol != NULL, FALSE);

	if (! G_IS_MOUNT(vol) && G_IS_VOLUME(vol) && g_volume_can_mount(G_VOLUME(vol)))
	{
		GMountOperation *op = gigolo_mount_operation_new(window);

		g_volume_mount(G_VOLUME(vol), G_MOUNT_MOUNT_NONE, op, NULL, volume_mount_finished_cb, backend);

		g_object_unref(op);
		return TRUE;
	}

	return FALSE;
}


void gigolo_backend_gvfs_unmount_mount(GigoloBackendGVFS *backend, gpointer mount, GtkWindow *parent)
{
#if GLIB_CHECK_VERSION(2, 22, 0)
	GMountOperation *op;
#endif

	g_return_if_fail(backend != NULL);
	g_return_if_fail(mount != NULL);

#if GLIB_CHECK_VERSION(2, 22, 0)
	op = gigolo_mount_operation_new(parent);
	g_mount_unmount_with_operation(
		G_MOUNT(mount), G_MOUNT_UNMOUNT_NONE, op, NULL, unmount_finished_cb, backend);
	g_object_unref(op);
#else
	g_mount_unmount(G_MOUNT(mount), G_MOUNT_UNMOUNT_NONE, NULL, unmount_finished_cb, backend);
#endif
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
		if (mi->show_errors && ! g_error_matches(error, G_IO_ERROR, G_IO_ERROR_FAILED_HANDLED))
			g_signal_emit(mi->self, signals[OPERATION_FAILED], 0, msg, error->message);
		else
			verbose("%s (%s)", msg, error->message);
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
								   GtkWindow *parent, GtkWidget *dialog, gboolean show_errors)
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
	mi->show_errors = show_errors;

	g_file_mount_enclosing_volume(file, G_MOUNT_MOUNT_NONE, op, NULL,
		(GAsyncReadyCallback) mount_ready_cb, mi);

	g_object_unref(file);
	g_object_unref(op);
}


gchar *gigolo_backend_gvfs_get_volume_identifier(gpointer volume)
{
	g_return_val_if_fail(volume != NULL, NULL);

	return g_volume_get_identifier(G_VOLUME(volume), G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
}


static gboolean browse_network_ready_cb(gpointer backend)
{
	GigoloBackendGVFSPrivate *priv;
	priv = GIGOLO_BACKEND_GVFS_GET_PRIVATE(backend);

	g_return_val_if_fail(backend != NULL, FALSE);

	if (priv->browse_counter <= 0)
	{
		g_signal_emit(backend, signals[BROWSE_NETWORK_FINISHED], 0);
		verbose("Browse Network finished");
		return FALSE;
	}
	return TRUE;
}


static void browse_network_mount_ready_cb(GFile *location, GAsyncResult *res, BrowseData *bd)
{
	gboolean success;
	GError *error = NULL;
	GigoloBackendGVFSPrivate *priv;

	g_return_if_fail(bd != NULL);
	g_return_if_fail(bd->self != NULL);

	success = g_file_mount_enclosing_volume_finish(location, res, &error);

	priv = GIGOLO_BACKEND_GVFS_GET_PRIVATE(bd->self);
	priv->browse_counter--;

	if (error != NULL)
	{
		verbose("%s (%s)", G_STRFUNC, error->message);
		g_error_free(error);

		/* If we are looking only for shares, we need to emit the finished signal otherwise the
		 * caller will never know we are done. */
		if (bd->browse_func == browse_host_real)
			g_signal_emit(bd->self, signals[BROWSE_HOST_FINISHED], 0, NULL);
	}
	else
	{
		bd->browse_func(bd);
	}
}


static void browse_network_real(BrowseData *bd)
{
	GigoloBackendGVFS *backend;
	GigoloBackendGVFSPrivate *priv;
	GFile *file;
	GFileInfo *info;
	GError *error = NULL;
	GFileEnumerator *e;
	GtkTreeStore *store;
	GtkWindow *parent;
	gint mode;

	g_return_if_fail(bd != NULL);
	g_return_if_fail(bd->self != NULL);

	store = bd->store;
	mode = bd->mode;
	parent = bd->parent;
	backend = bd->self;
	priv = GIGOLO_BACKEND_GVFS_GET_PRIVATE(backend);
	priv->browse_counter++;

	file = g_file_new_for_uri(bd->uri);

	e = g_file_enumerate_children(file,
		G_FILE_ATTRIBUTE_STANDARD_TARGET_URI ","
		G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME ","
		G_FILE_ATTRIBUTE_STANDARD_ICON,
		G_FILE_QUERY_INFO_NONE, NULL, &error);

	if (error != NULL)
	{
		if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_NOT_MOUNTED))
		{
			GMountOperation *op = gigolo_mount_operation_new(bd->parent);
			/* if the URI wasn't mounted yet, mount it and try again from the mount ready callback */
			g_file_mount_enclosing_volume(file, G_MOUNT_MOUNT_NONE, op, NULL,
				(GAsyncReadyCallback) browse_network_mount_ready_cb, bd);

			g_error_free(error);
			g_object_unref(file);
			g_object_unref(op);
			return;
		}
		else
		{
			verbose("%s: %s", G_STRFUNC, error->message);
			g_error_free(error);
		}
	}
	else
	{
		const gchar *uri;
		GtkTreeIter iter, parent_iter_tmp;
		GtkTreeIter *parent_iter;
		gint child_mode;

		verbose("Querying \"%s\" for available groups/hosts/shares", bd->uri);

		while ((info = g_file_enumerator_next_file(e, NULL, NULL)) != NULL)
		{
			uri = g_file_info_get_attribute_string(info, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI);
			if (uri != NULL)
			{
				/* set parent item */
				if (bd->parent_path != NULL)
				{
					gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &parent_iter_tmp, bd->parent_path);
					parent_iter = &parent_iter_tmp;
				}
				else
					parent_iter = NULL;

				/* insert the item into the tree store */
				gtk_tree_store_insert_with_values(store, &iter, parent_iter, -1,
					GIGOLO_BROWSE_NETWORK_COL_URI, uri,
					GIGOLO_BROWSE_NETWORK_COL_NAME, g_file_info_get_display_name(info),
					GIGOLO_BROWSE_NETWORK_COL_ICON, g_file_info_get_icon(info),
					GIGOLO_BROWSE_NETWORK_COL_CAN_MOUNT, (mode == BROWSE_MODE_SHARES),
					-1);

				/* set mode for the next level or stop recursion */
				switch (mode)
				{
					case BROWSE_MODE_DOMAINS:
						child_mode = BROWSE_MODE_HOSTS;
						break;
					case BROWSE_MODE_HOSTS:
						child_mode = BROWSE_MODE_SHARES;
						break;
					default:
						child_mode = BROWSE_MODE_INVALID;
				}

				/* setup child data and fire it */
				if (child_mode != BROWSE_MODE_INVALID)
				{
					BrowseData *bd_child = g_new0(BrowseData, 1);
					bd_child->mode = child_mode;
					bd_child->uri = g_strdup(uri);
					bd_child->store = store;
					bd_child->self = backend;
					bd_child->parent = parent;
					bd_child->parent_path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
					bd_child->browse_func = browse_network_real;

					/* recurse into the next level */
					browse_network_real(bd_child);
				}
			}
			g_object_unref(info);
		}
		g_object_unref(e);
	}
	priv->browse_counter--;

	g_object_unref(file);
	g_free(bd->uri);
	gtk_tree_path_free(bd->parent_path);
	g_free(bd);
}


void gigolo_backend_gvfs_browse_network(GigoloBackendGVFS *backend, GtkWindow *parent, GtkTreeStore *store)
{
	BrowseData *bd = g_new0(BrowseData, 1);
	GigoloBackendGVFSPrivate *priv;

	g_return_if_fail(backend != NULL);

	bd->mode = BROWSE_MODE_DOMAINS;
	bd->parent_path = NULL;
	bd->parent = parent;
	bd->store = store;
	bd->uri = g_strdup("smb://");
	bd->self = backend;
	bd->browse_func = browse_network_real;

	priv = GIGOLO_BACKEND_GVFS_GET_PRIVATE(backend);
	priv->browse_counter = 0;

	browse_network_real(bd);

	/* When we are here, we initiated the network browsing. Then check the 'browse_counter' and
	 * once it is 0 again, we are probably done with browsing. */
	g_timeout_add(250, browse_network_ready_cb, backend);
}


static void browse_host_real(BrowseData *bd)
{
	GFile *file;
	GFileInfo *info;
	GError *error = NULL;
	GFileEnumerator *e;
	GSList *list = NULL;

	g_return_if_fail(bd != NULL);
	g_return_if_fail(bd->self != NULL);

	file = g_file_new_for_uri(bd->uri);

	e = g_file_enumerate_children(file,
		G_FILE_ATTRIBUTE_STANDARD_NAME,
		G_FILE_QUERY_INFO_NONE, NULL, &error);

	if (error != NULL)
	{
		if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_NOT_MOUNTED))
		{
			GMountOperation *op = gigolo_mount_operation_new(bd->parent);
			/* if the URI wasn't mounted yet, mount it and try again from the mount ready callback */
			g_file_mount_enclosing_volume(file, G_MOUNT_MOUNT_NONE, op, NULL,
				(GAsyncReadyCallback) browse_network_mount_ready_cb, bd);

			g_error_free(error);
			g_object_unref(file);
			g_object_unref(op);
			return;
		}
		else
		{
			verbose("%s: %s", G_STRFUNC, error->message);
			g_error_free(error);
		}
	}
	else
	{
		verbose("Querying \"%s\" for available shares", bd->uri);

		while ((info = g_file_enumerator_next_file(e, NULL, NULL)) != NULL)
		{
			list = g_slist_append(list, g_strdup(g_file_info_get_name(info)));
			g_object_unref(info);
		}
		g_object_unref(e);
	}

	/* propagate our results */
	g_signal_emit(bd->self, signals[BROWSE_HOST_FINISHED], 0, list);

	g_slist_foreach(list, (GFunc) g_free, NULL);
	g_slist_free(list);

	g_object_unref(file);
	g_free(bd->uri);
	g_free(bd);
}


void gigolo_backend_gvfs_browse_host(GigoloBackendGVFS *backend, GtkWindow *parent, const gchar *hostname)
{
	BrowseData *bd = g_new0(BrowseData, 1);

	g_return_if_fail(backend != NULL);
	g_return_if_fail(NZV(hostname));

	bd->uri = g_strdup_printf("smb://%s", hostname);
	bd->self = backend;
	bd->parent = parent;
	bd->browse_func = browse_host_real;

	browse_host_real(bd);
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
