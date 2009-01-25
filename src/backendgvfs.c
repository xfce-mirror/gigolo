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
#include "passworddialog.h"
#include "main.h"

typedef struct _SionBackendGVFSPrivate			SionBackendGVFSPrivate;

#define SION_BACKEND_GVFS_GET_PRIVATE(obj)		(G_TYPE_INSTANCE_GET_PRIVATE((obj),\
			SION_BACKEND_GVFS_TYPE, SionBackendGVFSPrivate))

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
	SionBackendGVFS *self;
	GtkWidget *dialog;
} MountInfo;

struct _SionBackendGVFSPrivate
{
	GtkListStore *store;
};

static void sion_backend_gvfs_class_init			(SionBackendGVFSClass *klass);
static void sion_backend_gvfs_init      			(SionBackendGVFS *self);
static void sion_backend_gvfs_finalize  			(GObject *object);
static void sion_backend_gvfs_set_property			(GObject *object, guint prop_id,
													 const GValue *value, GParamSpec *pspec);


static GObjectClass *parent_class = NULL;

GType sion_backend_gvfs_get_type(void)
{
	static GType self_type = 0;
	if (! self_type)
	{
		static const GTypeInfo self_info =
		{
			sizeof(SionBackendGVFSClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc)sion_backend_gvfs_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(SionBackendGVFS),
			0,
			(GInstanceInitFunc)sion_backend_gvfs_init,
			NULL /* value_table */
		};

		self_type = g_type_register_static(G_TYPE_OBJECT, "SionBackendGVFS", &self_info, 0);
	}

	return self_type;
}


static void sion_backend_gvfs_cclosure_marshal_VOID__STRING_STRING(
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


static void sion_backend_gvfs_class_init(SionBackendGVFSClass *klass)
{
	GObjectClass *g_object_class;

	g_object_class = G_OBJECT_CLASS(klass);

	g_object_class->finalize = sion_backend_gvfs_finalize;
	g_object_class->set_property = sion_backend_gvfs_set_property;

	parent_class = (GObjectClass*)g_type_class_peek(G_TYPE_OBJECT);
	g_type_class_add_private((gpointer)klass, sizeof(SionBackendGVFSPrivate));

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
										sion_backend_gvfs_cclosure_marshal_VOID__STRING_STRING,
										G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);
}


static void sion_backend_gvfs_finalize(GObject *object)
{
	SionBackendGVFS *self;

	self = SION_BACKEND_GVFS(object);

	if (G_OBJECT_CLASS(parent_class)->finalize)
		(* G_OBJECT_CLASS(parent_class)->finalize)(object);
}


static gchar *get_tooltip_text(gpointer ref, gint ref_type, const gchar *type)
{
	gchar *result = NULL;
	switch (ref_type)
	{
		case SION_WINDOW_REF_TYPE_MOUNT:
		{
			gchar *uri, *name;

			sion_backend_gvfs_get_name_and_uri_from_mount(ref, &name, &uri);
			result = g_strdup_printf(
				_("<b>%s</b>\n\nURI: %s\nMounted: Yes\nService Type: %s"), name, uri, type);

			g_free(uri);
			g_free(name);
			return result;
		}
		case SION_WINDOW_REF_TYPE_VOLUME:
		default:
		{
			gchar *label = sion_backend_gvfs_get_volume_identifier(ref);

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
	SionBackendGVFSPrivate *priv = SION_BACKEND_GVFS_GET_PRIVATE(backend);

	gtk_list_store_clear(priv->store);

	/* list mounts */
	mounts = g_volume_monitor_get_mounts(vm);
	for (item = mounts; item != NULL; item = g_list_next(item))
	{
		mount = G_MOUNT(item->data);
		vol_name = g_mount_get_name(mount);
		file = g_mount_get_root(mount);
		scheme = g_file_get_uri_scheme(file);
		scheme_name = sion_describe_scheme(scheme);
		uri = g_file_get_uri(file);
		icon = g_mount_get_icon(mount);
		tooltip_text = get_tooltip_text(mount, SION_WINDOW_REF_TYPE_MOUNT, scheme_name);

		gtk_list_store_append(priv->store, &iter);
		gtk_list_store_set(priv->store, &iter,
				SION_WINDOW_COL_IS_MOUNTED, TRUE,
				SION_WINDOW_COL_NAME, vol_name,
				SION_WINDOW_COL_SCHEME, scheme_name,
				SION_WINDOW_COL_REF, mount,
				SION_WINDOW_COL_REF_TYPE, SION_WINDOW_REF_TYPE_MOUNT,
				SION_WINDOW_COL_PIXBUF, icon,
				SION_WINDOW_COL_ICON_NAME, "folder-remote",
				SION_WINDOW_COL_TOOLTIP, tooltip_text,
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
			tooltip_text = get_tooltip_text(volume, SION_WINDOW_REF_TYPE_VOLUME, NULL);

			gtk_list_store_append(priv->store, &iter);
			gtk_list_store_set(priv->store, &iter,
					SION_WINDOW_COL_IS_MOUNTED, FALSE,
					SION_WINDOW_COL_NAME, vol_name,
					SION_WINDOW_COL_SCHEME, sion_describe_scheme("file"),
					SION_WINDOW_COL_REF, volume,
					SION_WINDOW_COL_REF_TYPE, SION_WINDOW_REF_TYPE_VOLUME,
					SION_WINDOW_COL_PIXBUF, icon,
					SION_WINDOW_COL_ICON_NAME, "folder-remote",
					SION_WINDOW_COL_TOOLTIP, tooltip_text,
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


static void sion_backend_gvfs_set_property(GObject *object, guint prop_id,
										   const GValue *value, GParamSpec *pspec)
{
	switch (prop_id)
	{
	case PROP_STORE:
	{
		GVolumeMonitor *gvm;
		SionBackendGVFSPrivate *priv = SION_BACKEND_GVFS_GET_PRIVATE(object);

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


static void sion_backend_gvfs_init(G_GNUC_UNUSED SionBackendGVFS *self)
{
}


SionBackendGVFS *sion_backend_gvfs_new(GtkListStore *store)
{
	SionBackendGVFS *backend = g_object_new(SION_BACKEND_GVFS_TYPE, "store", store, NULL);

	return backend;
}


gboolean sion_backend_gvfs_is_mount(gpointer mnt)
{
	g_return_val_if_fail(mnt != NULL, FALSE);

	return G_IS_MOUNT(mnt);
}


void sion_backend_gvfs_get_name_and_uri_from_mount(GMount *mount, gchar **name, gchar **uri)
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
			sion_backend_gvfs_get_name_and_uri_from_mount(G_MOUNT(src), &name, NULL);
			if (name == NULL)
				name = g_strdup(_("unknown"));
		}

		g_warning("Mounting of \"%s\" failed (%s)", name, error->message);
		msg = g_strdup_printf(_("Mounting of \"%s\" failed."), name);

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
			sion_backend_gvfs_get_name_and_uri_from_mount(G_MOUNT(src), &name, NULL);
			if (name == NULL)
				name = g_strdup(_("unknown"));
		}

		g_warning("Unmounting of \"%s\" failed: %s", name, error->message);
		msg = g_strdup_printf(_("Unmounting of \"%s\" failed."), name);

		g_signal_emit(backend, signals[OPERATION_FAILED], 0, msg, error->message);

		g_error_free(error);
		g_free(name);
		g_free(msg);
	}
}


gboolean sion_backend_gvfs_mount_volume(SionBackendGVFS *backend, GVolume *vol)
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


void sion_backend_gvfs_unmount_mount(SionBackendGVFS *backend, GMount *mount)
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
		gchar *msg = g_strdup_printf(_("Mounting of \"%s\" failed."), uri);
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


static void set_password_cb(GMountOperation *op, G_GNUC_UNUSED gchar *message, gchar *default_user,
							gchar *default_domain, GAskPasswordFlags flags, const gchar *domain)
{
	GMountOperationResult result;
	GtkWidget *dialog;

	dialog = sion_password_dialog_new(flags, default_user,
		(domain != NULL) ? domain : default_domain);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
	{
		result = G_MOUNT_OPERATION_HANDLED;

		if (flags & G_ASK_PASSWORD_NEED_DOMAIN)
			g_mount_operation_set_domain(op,
				sion_password_dialog_get_domain(SION_PASSWORD_DIALOG(dialog)));
		if (flags & G_ASK_PASSWORD_NEED_USERNAME)
			g_mount_operation_set_username(op,
				sion_password_dialog_get_username(SION_PASSWORD_DIALOG(dialog)));
		if (flags & G_ASK_PASSWORD_NEED_PASSWORD)
		{
			g_mount_operation_set_password(op,
				sion_password_dialog_get_password(SION_PASSWORD_DIALOG(dialog)));
			/* TODO make this configurable? */
			/* g_mount_operation_set_password_save(op, G_PASSWORD_SAVE_FOR_SESSION); */
			/* g_mount_operation_set_password_save(op, G_PASSWORD_SAVE_NEVER); */
			g_mount_operation_set_password_save(op, G_PASSWORD_SAVE_PERMANENTLY);
		}
	}
	else
	{
		result = G_MOUNT_OPERATION_ABORTED;
	}

	gtk_widget_destroy(dialog);

	g_mount_operation_reply(op, result);
}


void sion_backend_gvfs_mount_uri(SionBackendGVFS *backend, const gchar *uri,
								 const gchar *domain, GtkWidget *dialog)
{
	GMountOperation *op;
	GFile *file;
	MountInfo *mi;

	g_return_if_fail(uri != NULL);
	g_return_if_fail(backend != NULL);

	op = g_mount_operation_new();
	file = g_file_new_for_uri(uri);
	mi = g_new0(MountInfo, 1);
	mi->self = backend;
	mi->dialog = dialog;

	g_signal_connect(op, "ask-password", G_CALLBACK(set_password_cb), (gchar*) domain);

	g_file_mount_enclosing_volume(file, G_MOUNT_MOUNT_NONE, op, NULL,
		(GAsyncReadyCallback) mount_ready_cb, mi);

	g_object_unref(file);
}


gchar *sion_backend_gvfs_get_volume_identifier(GVolume *volume)
{
	g_return_val_if_fail(volume != NULL, NULL);

	return g_volume_get_identifier(volume, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
}


