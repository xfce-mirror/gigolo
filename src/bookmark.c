/*
 *      bookmark.c
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

#include <string.h>
#include <stdlib.h>
#include <glib-object.h>

#include "bookmark.h"
#include "common.h"
#include "main.h"


typedef struct _SionBookmarkPrivate			SionBookmarkPrivate;

#define SION_BOOKMARK_GET_PRIVATE(obj)		(G_TYPE_INSTANCE_GET_PRIVATE((obj),\
			SION_BOOKMARK_TYPE, SionBookmarkPrivate))

struct _SionBookmarkPrivate
{
	gchar	*name;
	gchar	*scheme;
	gchar	*host;
	gchar	*domain;
	gchar	*share;
	guint	 port;
	gchar	*user;

	gboolean is_valid;
};

static void sion_bookmark_class_init		(SionBookmarkClass *klass);
static void sion_bookmark_init      		(SionBookmark *self);
static void sion_bookmark_finalize  		(GObject *object);

/* Local data */
static GObjectClass *parent_class = NULL;

GType sion_bookmark_get_type(void)
{
	static GType self_type = 0;
	if (! self_type)
	{
		static const GTypeInfo self_info =
		{
			sizeof(SionBookmarkClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc)sion_bookmark_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(SionBookmark),
			0,
			(GInstanceInitFunc)sion_bookmark_init,
			NULL /* value_table */
		};

		self_type = g_type_register_static(G_TYPE_OBJECT, "SionBookmark", &self_info, 0);
	}

	return self_type;
}


static void bookmark_clear(SionBookmark *self)
{
	SionBookmarkPrivate *priv = SION_BOOKMARK_GET_PRIVATE(self);

	g_free(priv->name);
	g_free(priv->scheme);
	g_free(priv->host);
	g_free(priv->domain);
	g_free(priv->share);
	g_free(priv->user);

	priv->name = NULL;
	priv->scheme = NULL;
	priv->host = NULL;
	priv->port = 0;
	priv->domain = NULL;
	priv->share = NULL;
	priv->user = NULL;

	priv->is_valid = TRUE;
}


static void sion_bookmark_class_init(SionBookmarkClass *klass)
{
	GObjectClass *g_object_class;

	g_object_class = G_OBJECT_CLASS(klass);

	g_object_class->finalize = sion_bookmark_finalize;

	parent_class = (GObjectClass*)g_type_class_peek(G_TYPE_OBJECT);
	g_type_class_add_private((gpointer)klass, sizeof(SionBookmarkPrivate));
}


static void sion_bookmark_finalize(GObject *object)
{
	bookmark_clear(SION_BOOKMARK(object));

	G_OBJECT_CLASS(parent_class)->finalize(object);
}


static gboolean parse_uri(SionBookmark *bm, const gchar *uri)
{
	gchar *s, *t, *x, *end;
	guint l;
	SionBookmarkPrivate *priv = SION_BOOKMARK_GET_PRIVATE(bm);

	priv->scheme = g_uri_parse_scheme(uri);

	s = strstr(uri, "://");
	if (priv->scheme == NULL || s == NULL)
	{
		verbose("Error parsing URI %s while reading URI scheme", uri);
		bookmark_clear(bm);
		return FALSE;
	}
	s += 3;

	/* find end of host/port, this is the first slash after the initial double slashes */
	end = strchr(s, '/');
	if (end == NULL)
		end = s + strlen(s); // there is no trailing '/', so use the whole remaining string

	/* find username */
	t = strchr(s, '@');
	if (t != NULL)
	{
		l = 0;
		x = s;
		while (*x != '\0' && x < t && *x != ':')
		{
			l++; // count the len of the username
			x++;
		}
		if (l == 0)
		{
			verbose("Error parsing URI %s while reading username", uri);
			bookmark_clear(bm);
			return FALSE;
		}
		priv->user = g_strndup(s, l);
	}

	/* find hostname */
	s = (t) ? t + 1 : s;
	if (*s == '[') // obex://[00:12:D1:94:1B:28]/ or http://[1080:0:0:0:8:800:200C:417A]/index.html
	{
		gchar *hostend;

		s++; // skip the found '['
		hostend = strchr(s, ']');
		if (! hostend || hostend > end)
		{
			verbose("Error parsing URI %s, missing ']'", uri);
			bookmark_clear(bm);
			return FALSE;
		}
		l = 0;
		x = s;
		while (*x != '\0' && x < end && *x != ']')
		{
			l++; // count the len of the hostname
			x++;
		}
		priv->host = g_strndup(s, l);
		s = hostend;
	}
	else
	{
		l = 0;
		x = s;
		while (*x != '\0' && x < end && *x != ':')
		{
			l++; // count the len of the hostname
			x++;
		}
		priv->host = g_strndup(s, l);
	}

	/* find port */
	t = strchr(s, ':');
	if (t != NULL)
	{
		gchar *tmp;

		t++; // skip the found ':'
		l = 0;
		x = t;
		while (*x != '\0' && x < end)
		{
			l++; // count the len of the port
			x++;
		}
		// atoi should be enough as it returns simply 0 if there are any errors and 0 marks an
		// invalid port
		tmp = g_strndup(t, l);
		priv->port = (guint) atoi(tmp);
		g_free(tmp);
	}
	if (NZV(end))
		priv->share = g_strdup(end + 1);

	return TRUE;
}



static void sion_bookmark_init(SionBookmark *self)
{
	bookmark_clear(self);
}


SionBookmark *sion_bookmark_new(void)
{
	return (SionBookmark*) g_object_new(SION_BOOKMARK_TYPE, NULL);
}


SionBookmark *sion_bookmark_new_from_uri(const gchar *name, const gchar *uri)
{
	SionBookmark *bm = g_object_new(SION_BOOKMARK_TYPE, NULL);
	SionBookmarkPrivate *priv = SION_BOOKMARK_GET_PRIVATE(bm);

	sion_bookmark_set_name(bm, name);
	if (! parse_uri(bm, uri))
		priv->is_valid = FALSE;

	return bm;
}


/* Copy the contents of the bookmark 'src' into the existing bookmark 'dest' */
void sion_bookmark_clone(SionBookmark *dst, const SionBookmark *src)
{
	SionBookmarkPrivate *priv_dst;
	const SionBookmarkPrivate *priv_src;

	g_return_if_fail(dst != NULL);
	g_return_if_fail(src != NULL);

	priv_dst = SION_BOOKMARK_GET_PRIVATE(dst);
	priv_src = SION_BOOKMARK_GET_PRIVATE(src);

	/* free existing strings and data */
	bookmark_clear(dst);

	/* copy from src to dst */
	priv_dst->name = g_strdup(priv_src->name);
	priv_dst->host = g_strdup(priv_src->host);
	priv_dst->scheme = g_strdup(priv_src->scheme);
	priv_dst->domain = g_strdup(priv_src->domain);
	priv_dst->share = g_strdup(priv_src->share);
	priv_dst->user = g_strdup(priv_src->user);
	priv_dst->port = priv_src->port;
}


gchar *sion_bookmark_get_uri(SionBookmark *bookmark)
{
	SionBookmarkPrivate *priv = SION_BOOKMARK_GET_PRIVATE(bookmark);
	gchar *result;
	gchar *port = NULL;

	g_return_val_if_fail(bookmark != NULL, NULL);

	if (priv->port > 0)
	{
		port = g_strdup_printf(":%d", priv->port);
	}

	result = g_strdup_printf("%s://%s%s%s%s/%s%s",
		priv->scheme,
		(NZV(priv->user)) ? priv->user : "",
		(NZV(priv->user)) ? "@" : "",
		priv->host,
		(port) ? port : "",
		(NZV(priv->share)) ? priv->share : "",
		(NZV(priv->share)) ? "/" : "");

	g_free(port);
	return result;
}


void sion_bookmark_set_uri(SionBookmark *bookmark, const gchar *uri)
{
	SionBookmarkPrivate *priv;
	SionBookmark *tmp;

	g_return_if_fail(bookmark != NULL);
	g_return_if_fail(NZV(uri));

	priv = SION_BOOKMARK_GET_PRIVATE(bookmark);

	tmp = sion_bookmark_new_from_uri(priv->name, uri);
	if (sion_bookmark_is_valid(tmp))
		sion_bookmark_clone(bookmark, tmp);
}


const gchar *sion_bookmark_get_name(SionBookmark *bookmark)
{
	g_return_val_if_fail(bookmark != NULL, NULL);

	return SION_BOOKMARK_GET_PRIVATE(bookmark)->name;
}


void sion_bookmark_set_name(SionBookmark *bookmark, const gchar *name)
{
	SionBookmarkPrivate *priv;

	g_return_if_fail(bookmark != NULL);
	g_return_if_fail(NZV(name));

	priv = SION_BOOKMARK_GET_PRIVATE(bookmark);

	g_free(priv->name);
	priv->name = g_strdup(name);
}


const gchar *sion_bookmark_get_scheme(SionBookmark *bookmark)
{
	g_return_val_if_fail(bookmark != NULL, NULL);

	return SION_BOOKMARK_GET_PRIVATE(bookmark)->scheme;
}


void sion_bookmark_set_scheme(SionBookmark *bookmark, const gchar *scheme)
{
	SionBookmarkPrivate *priv;

	g_return_if_fail(bookmark != NULL);
	g_return_if_fail(NZV(scheme));

	priv = SION_BOOKMARK_GET_PRIVATE(bookmark);

	g_free(priv->scheme);
	priv->scheme = g_strdup(scheme);
}


const gchar *sion_bookmark_get_host(SionBookmark *bookmark)
{
	g_return_val_if_fail(bookmark != NULL, NULL);

	return SION_BOOKMARK_GET_PRIVATE(bookmark)->host;
}


void sion_bookmark_set_host(SionBookmark *bookmark, const gchar *host)
{
	SionBookmarkPrivate *priv;

	g_return_if_fail(bookmark != NULL);
	g_return_if_fail(NZV(host));

	priv = SION_BOOKMARK_GET_PRIVATE(bookmark);

	g_free(priv->host);
	priv->host = g_strdup(host);
}


guint sion_bookmark_get_port(SionBookmark *bookmark)
{
	g_return_val_if_fail(bookmark != NULL, 0);

	return SION_BOOKMARK_GET_PRIVATE(bookmark)->port;
}


void sion_bookmark_set_port(SionBookmark *bookmark, guint port)
{
	SionBookmarkPrivate *priv;

	g_return_if_fail(bookmark != NULL);

	priv = SION_BOOKMARK_GET_PRIVATE(bookmark);

	priv->port = port;
}


const gchar *sion_bookmark_get_user(SionBookmark *bookmark)
{
	g_return_val_if_fail(bookmark != NULL, NULL);

	return SION_BOOKMARK_GET_PRIVATE(bookmark)->user;
}


void sion_bookmark_set_user(SionBookmark *bookmark, const gchar *user)
{
	SionBookmarkPrivate *priv;

	g_return_if_fail(bookmark != NULL);
	g_return_if_fail(user != NULL);

	priv = SION_BOOKMARK_GET_PRIVATE(bookmark);

	g_free(priv->user);
	priv->user = g_strdup(user);
}


const gchar *sion_bookmark_get_share(SionBookmark *bookmark)
{
	g_return_val_if_fail(bookmark != NULL, NULL);

	return SION_BOOKMARK_GET_PRIVATE(bookmark)->share;
}


void sion_bookmark_set_share(SionBookmark *bookmark, const gchar *share)
{
	SionBookmarkPrivate *priv;

	g_return_if_fail(bookmark != NULL);
	g_return_if_fail(share != NULL);

	priv = SION_BOOKMARK_GET_PRIVATE(bookmark);

	g_free(priv->share);
	priv->share = g_strdup(share);
}


const gchar *sion_bookmark_get_domain(SionBookmark *bookmark)
{
	g_return_val_if_fail(bookmark != NULL, NULL);

	return SION_BOOKMARK_GET_PRIVATE(bookmark)->domain;
}


void sion_bookmark_set_domain(SionBookmark *bookmark, const gchar *domain)
{
	SionBookmarkPrivate *priv;

	g_return_if_fail(bookmark != NULL);
	g_return_if_fail(domain != NULL);

	priv = SION_BOOKMARK_GET_PRIVATE(bookmark);

	g_free(priv->domain);
	priv->domain = g_strdup(domain);
}


gboolean sion_bookmark_is_valid(SionBookmark *bookmark)
{
	SionBookmarkPrivate *priv;

	g_return_val_if_fail(bookmark != NULL, FALSE);

	priv = SION_BOOKMARK_GET_PRIVATE(bookmark);

	return priv->is_valid;
}

