/*
 *      bookmark.c
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

#include <string.h>
#include <stdlib.h>
#include <glib-object.h>

#include "bookmark.h"
#include "common.h"


typedef struct _GigoloBookmarkPrivate			GigoloBookmarkPrivate;

#define GIGOLO_BOOKMARK_GET_PRIVATE(obj)		(G_TYPE_INSTANCE_GET_PRIVATE((obj),\
			GIGOLO_BOOKMARK_TYPE, GigoloBookmarkPrivate))

struct _GigoloBookmarkPrivate
{
	gchar	*name;
	gchar	*scheme;
	gchar	*host;
	gchar	*folder;
	gchar	*domain;
	gchar	*share;
	guint	 port;
	gchar	*user;
	gboolean autoconnect;
	gboolean should_not_autoconnect;

	gboolean is_valid;
};

static void gigolo_bookmark_finalize  		(GObject *object);

G_DEFINE_TYPE(GigoloBookmark, gigolo_bookmark, G_TYPE_OBJECT);


void gigolo_bookmark_bookmark_clear(GigoloBookmark *bookmark)
{
	GigoloBookmarkPrivate *priv = GIGOLO_BOOKMARK_GET_PRIVATE(bookmark);

	g_free(priv->name);
	g_free(priv->scheme);
	g_free(priv->host);
	g_free(priv->folder);
	g_free(priv->domain);
	g_free(priv->share);
	g_free(priv->user);

	priv->name = NULL;
	priv->scheme = NULL;
	priv->host = NULL;
	priv->port = 0;
	priv->domain = NULL;
	priv->folder = NULL;
	priv->share = NULL;
	priv->user = NULL;

	priv->is_valid = TRUE;
}


static void gigolo_bookmark_class_init(GigoloBookmarkClass *klass)
{
	GObjectClass *g_object_class;

	g_object_class = G_OBJECT_CLASS(klass);

	g_object_class->finalize = gigolo_bookmark_finalize;

	g_type_class_add_private(klass, sizeof(GigoloBookmarkPrivate));
}


static void gigolo_bookmark_finalize(GObject *object)
{
	gigolo_bookmark_bookmark_clear(GIGOLO_BOOKMARK(object));

	G_OBJECT_CLASS(gigolo_bookmark_parent_class)->finalize(object);
}


gboolean gigolo_bookmark_parse_uri(GigoloBookmark *bookmark, const gchar *uri)
{
	gchar *s, *t, *x, *end, *tmp;
	guint l;
	GigoloBookmarkPrivate *priv = GIGOLO_BOOKMARK_GET_PRIVATE(bookmark);

	priv->scheme = g_uri_parse_scheme(uri);

	s = strstr(uri, "://");
	if (priv->scheme == NULL || s == NULL)
	{
		verbose("Error parsing URI '%s' while reading URI scheme", uri);
		gigolo_bookmark_bookmark_clear(bookmark);
		return FALSE;
	}
	s += 3;

	/* find end of host/port, this is the first slash after the initial double slashes */
	end = strchr(s, '/');
	if (end == NULL)
		end = s + strlen(s); /* there is no trailing '/', so use the whole remaining string */

	/* find username */
	t = strchr(s, '@');
	/* if we found a '@', search for a second one and use the second one as end of the username
	 * as the username itself might contain a '@' */
	if (t != NULL && (x = strchr(t + 1, '@')) != NULL)
		t = x;
	if (t != NULL)
	{
		l = 0;
		x = s;
		while (*x != '\0' && x < t && *x != ':')
		{
			l++; /* count the len of the username */
			x++;
		}
		if (l == 0)
		{
			verbose("Error parsing URI '%s' while reading username", uri);
			gigolo_bookmark_bookmark_clear(bookmark);
			return FALSE;
		}
		tmp = g_strndup(s, l);
		/* look for a domain in the username, e.g. "mydomain;myuser" */
		if (strchr(tmp, ';') != NULL)
		{
			gchar **fields = g_strsplit(tmp, ";", 2);
			priv->domain = fields[0];
			priv->user = fields[1];

			g_free(fields); /* we free only the array, not the elements */
		}
		else
			priv->user = tmp;
	}

	/* find hostname */
	s = (t) ? t + 1 : s;
	if (*s == '[') /* obex://[00:12:D1:94:1B:28]/ or http://[1080:0:0:0:8:800:200C:417A]/index.html */
	{
		gchar *hostend;

		s++; /* skip the found '[' */
		hostend = strchr(s, ']');
		if (! hostend || hostend > end)
		{
			verbose("Error parsing URI '%s', missing ']'", uri);
			gigolo_bookmark_bookmark_clear(bookmark);
			return FALSE;
		}
		l = 0;
		x = s;
		while (*x != '\0' && x < end && *x != ']')
		{
			l++; /* count the len of the hostname */
			x++;
		}
		tmp = g_strndup(s, l);
		priv->host = g_strconcat("[", tmp, "]", NULL);
		g_free(tmp);
		s = hostend;
	}
	else
	{
		l = 0;
		x = s;
		while (*x != '\0' && x < end && *x != ':')
		{
			l++; /* count the len of the hostname */
			x++;
		}
		priv->host = g_strndup(s, l);
	}

	/* find port */
	t = strchr(s, ':');
	if (t != NULL)
	{
		t++; /* skip the found ':' */
		l = 0;
		x = t;
		while (*x != '\0' && x < end)
		{
			l++; /* count the len of the port */
			x++;
		}
		/* atoi should be enough as it returns simply 0 if there are any errors and 0 marks an
		 * invalid port */
		tmp = g_strndup(t, l);
		priv->port = (guint) atoi(tmp);
		g_free(tmp);
	}
	if (NZV(end) && *end == '/' && gigolo_str_equal("smb", priv->scheme))
	{
		end++; /* skip the slash */

		l = 0;
		x = end;
		while (*x != '\0' && *x != '/')
		{
			l++; /* count the len of the share name */
			x++;
		}
		priv->share = g_strndup(end, l);
	}

	return TRUE;
}


static void gigolo_bookmark_init(GigoloBookmark *self)
{
	gigolo_bookmark_bookmark_clear(self);
}


GigoloBookmark *gigolo_bookmark_new(void)
{
	return (GigoloBookmark*) g_object_new(GIGOLO_BOOKMARK_TYPE, NULL);
}


GigoloBookmark *gigolo_bookmark_new_from_uri(const gchar *name, const gchar *uri)
{
	GigoloBookmark *bm = g_object_new(GIGOLO_BOOKMARK_TYPE, NULL);
	GigoloBookmarkPrivate *priv = GIGOLO_BOOKMARK_GET_PRIVATE(bm);

	gigolo_bookmark_set_name(bm, name);
	if (! gigolo_bookmark_parse_uri(bm, uri))
		priv->is_valid = FALSE;

	return bm;
}


/* Copy the contents of the bookmark 'src' into the existing bookmark 'dest' */
void gigolo_bookmark_clone(GigoloBookmark *dst, const GigoloBookmark *src)
{
	GigoloBookmarkPrivate *priv_dst;
	const GigoloBookmarkPrivate *priv_src;

	g_return_if_fail(dst != NULL);
	g_return_if_fail(src != NULL);

	priv_dst = GIGOLO_BOOKMARK_GET_PRIVATE(dst);
	priv_src = GIGOLO_BOOKMARK_GET_PRIVATE(src);

	/* free existing strings and data */
	gigolo_bookmark_bookmark_clear(dst);

	/* copy from src to dst */
	priv_dst->name = g_strdup(priv_src->name);
	priv_dst->host = g_strdup(priv_src->host);
	priv_dst->scheme = g_strdup(priv_src->scheme);
	priv_dst->domain = g_strdup(priv_src->domain);
	priv_dst->folder = g_strdup(priv_src->folder);
	priv_dst->share = g_strdup(priv_src->share);
	priv_dst->user = g_strdup(priv_src->user);
	priv_dst->port = priv_src->port;
}


static gchar *gigolo_bookmark_get_uri_real(GigoloBookmark *bookmark, gboolean escaped)
{
	GigoloBookmarkPrivate *priv = GIGOLO_BOOKMARK_GET_PRIVATE(bookmark);
	gchar *result;
	const gchar *domain = NULL;
	gchar *user = NULL;
	gchar *port = NULL;

	g_return_val_if_fail(bookmark != NULL, NULL);

	if (priv->port > 0 && priv->port != gigolo_get_default_port(priv->scheme))
	{
		port = g_strdup_printf(":%d", priv->port);
	}

	/* Escape the userinfo part to allow '@' characters even if RFC3986 doesn't seem to allow them. */
	if (escaped && NZV(priv->user))
		user = g_uri_escape_string(priv->user, G_URI_RESERVED_CHARS_ALLOWED_IN_USERINFO, FALSE);
	else
		user = g_strdup(priv->user);

	domain = gigolo_bookmark_get_domain(bookmark);

	result = g_strdup_printf("%s://%s%s%s%s%s%s/%s%s",
		priv->scheme,
		(NZV(domain)) ? domain : "",
		(NZV(domain)) ? ";" : "",
		(NZV(user)) ? user : "",
		(NZV(user) || NZV(domain)) ? "@" : "",
		priv->host,
		(port) ? port : "",
		(NZV(priv->share)) ? priv->share : "",
		(NZV(priv->share)) ? "/" : "");

	g_free(port);
	g_free(user);
	return result;
}


gchar *gigolo_bookmark_get_uri(GigoloBookmark *bookmark)
{
	return gigolo_bookmark_get_uri_real(bookmark, FALSE);
}


gchar *gigolo_bookmark_get_uri_escaped(GigoloBookmark *bookmark)
{
	return gigolo_bookmark_get_uri_real(bookmark, TRUE);
}


void gigolo_bookmark_set_uri(GigoloBookmark *bookmark, const gchar *uri)
{
	GigoloBookmarkPrivate *priv;
	GigoloBookmark *tmp;

	g_return_if_fail(bookmark != NULL);
	g_return_if_fail(NZV(uri));

	priv = GIGOLO_BOOKMARK_GET_PRIVATE(bookmark);

	tmp = gigolo_bookmark_new_from_uri(priv->name, uri);
	if (gigolo_bookmark_is_valid(tmp))
		gigolo_bookmark_clone(bookmark, tmp);
}


const gchar *gigolo_bookmark_get_name(GigoloBookmark *bookmark)
{
	g_return_val_if_fail(bookmark != NULL, NULL);

	return GIGOLO_BOOKMARK_GET_PRIVATE(bookmark)->name;
}


void gigolo_bookmark_set_name(GigoloBookmark *bookmark, const gchar *name)
{
	GigoloBookmarkPrivate *priv;

	g_return_if_fail(bookmark != NULL);
	g_return_if_fail(NZV(name));

	priv = GIGOLO_BOOKMARK_GET_PRIVATE(bookmark);

	g_free(priv->name);
	priv->name = g_strdup(name);
}


const gchar *gigolo_bookmark_get_scheme(GigoloBookmark *bookmark)
{
	g_return_val_if_fail(bookmark != NULL, NULL);

	return GIGOLO_BOOKMARK_GET_PRIVATE(bookmark)->scheme;
}


void gigolo_bookmark_set_scheme(GigoloBookmark *bookmark, const gchar *scheme)
{
	GigoloBookmarkPrivate *priv;

	g_return_if_fail(bookmark != NULL);
	g_return_if_fail(NZV(scheme));

	priv = GIGOLO_BOOKMARK_GET_PRIVATE(bookmark);

	g_free(priv->scheme);
	priv->scheme = g_strdup(scheme);
}


const gchar *gigolo_bookmark_get_host(GigoloBookmark *bookmark)
{
	g_return_val_if_fail(bookmark != NULL, NULL);

	return GIGOLO_BOOKMARK_GET_PRIVATE(bookmark)->host;
}


void gigolo_bookmark_set_host(GigoloBookmark *bookmark, const gchar *host)
{
	GigoloBookmarkPrivate *priv;

	g_return_if_fail(bookmark != NULL);
	g_return_if_fail(NZV(host));

	priv = GIGOLO_BOOKMARK_GET_PRIVATE(bookmark);

	g_free(priv->host);
	priv->host = g_strdup(host);
}


const gchar *gigolo_bookmark_get_folder(GigoloBookmark *bookmark)
{
	g_return_val_if_fail(bookmark != NULL, NULL);

	return GIGOLO_BOOKMARK_GET_PRIVATE(bookmark)->folder;
}


void gigolo_bookmark_set_folder(GigoloBookmark *bookmark, const gchar *folder)
{
	GigoloBookmarkPrivate *priv;

	g_return_if_fail(bookmark != NULL);
	g_return_if_fail(folder != NULL);

	priv = GIGOLO_BOOKMARK_GET_PRIVATE(bookmark);

	g_free(priv->folder);
	priv->folder = g_strdup(folder);
}


guint gigolo_bookmark_get_port(GigoloBookmark *bookmark)
{
	g_return_val_if_fail(bookmark != NULL, 0);

	return GIGOLO_BOOKMARK_GET_PRIVATE(bookmark)->port;
}


void gigolo_bookmark_set_port(GigoloBookmark *bookmark, guint port)
{
	GigoloBookmarkPrivate *priv;

	g_return_if_fail(bookmark != NULL);

	priv = GIGOLO_BOOKMARK_GET_PRIVATE(bookmark);

	priv->port = port;
}


gboolean gigolo_bookmark_get_autoconnect(GigoloBookmark *bookmark)
{
	g_return_val_if_fail(bookmark != NULL, 0);

	return GIGOLO_BOOKMARK_GET_PRIVATE(bookmark)->autoconnect;
}


void gigolo_bookmark_set_autoconnect(GigoloBookmark *bookmark, gboolean autoconnect)
{
	GigoloBookmarkPrivate *priv;

	g_return_if_fail(bookmark != NULL);

	priv = GIGOLO_BOOKMARK_GET_PRIVATE(bookmark);

	priv->autoconnect = autoconnect;
}


gboolean gigolo_bookmark_get_should_not_autoconnect(GigoloBookmark *bookmark)
{
	g_return_val_if_fail(bookmark != NULL, 0);

	return GIGOLO_BOOKMARK_GET_PRIVATE(bookmark)->should_not_autoconnect;
}


void gigolo_bookmark_set_should_not_autoconnect(GigoloBookmark *bookmark, gboolean should_not_autoconnect)
{
	GigoloBookmarkPrivate *priv;

	g_return_if_fail(bookmark != NULL);

	priv = GIGOLO_BOOKMARK_GET_PRIVATE(bookmark);

	priv->should_not_autoconnect = should_not_autoconnect;
}


gchar *gigolo_bookmark_get_user_unescaped(GigoloBookmark *bookmark)
{
	gchar *username;
	GigoloBookmarkPrivate *priv;

	g_return_val_if_fail(bookmark != NULL, NULL);

	priv = GIGOLO_BOOKMARK_GET_PRIVATE(bookmark);

	if (NZV(priv->user))
		username = g_uri_unescape_string(priv->user, G_URI_RESERVED_CHARS_ALLOWED_IN_USERINFO);
	else
		username = g_strdup(priv->user);

	return username;
}


const gchar *gigolo_bookmark_get_user(GigoloBookmark *bookmark)
{
	g_return_val_if_fail(bookmark != NULL, NULL);

	return GIGOLO_BOOKMARK_GET_PRIVATE(bookmark)->user;
}


void gigolo_bookmark_set_user(GigoloBookmark *bookmark, const gchar *user)
{
	GigoloBookmarkPrivate *priv;

	g_return_if_fail(bookmark != NULL);
	g_return_if_fail(user != NULL);

	priv = GIGOLO_BOOKMARK_GET_PRIVATE(bookmark);

	g_free(priv->user);
	priv->user = g_strdup(user);
}


const gchar *gigolo_bookmark_get_share(GigoloBookmark *bookmark)
{
	g_return_val_if_fail(bookmark != NULL, NULL);

	return GIGOLO_BOOKMARK_GET_PRIVATE(bookmark)->share;
}


void gigolo_bookmark_set_share(GigoloBookmark *bookmark, const gchar *share)
{
	GigoloBookmarkPrivate *priv;

	g_return_if_fail(bookmark != NULL);
	g_return_if_fail(share != NULL);

	priv = GIGOLO_BOOKMARK_GET_PRIVATE(bookmark);

	g_free(priv->share);
	priv->share = g_strdup(share);
}


const gchar *gigolo_bookmark_get_domain(GigoloBookmark *bookmark)
{
	g_return_val_if_fail(bookmark != NULL, NULL);

	return GIGOLO_BOOKMARK_GET_PRIVATE(bookmark)->domain;
}


void gigolo_bookmark_set_domain(GigoloBookmark *bookmark, const gchar *domain)
{
	GigoloBookmarkPrivate *priv;

	g_return_if_fail(bookmark != NULL);
	g_return_if_fail(domain != NULL);

	priv = GIGOLO_BOOKMARK_GET_PRIVATE(bookmark);

	g_free(priv->domain);
	priv->domain = g_strdup(domain);
}


gboolean gigolo_bookmark_is_valid(GigoloBookmark *bookmark)
{
	GigoloBookmarkPrivate *priv;

	g_return_val_if_fail(bookmark != NULL, FALSE);

	priv = GIGOLO_BOOKMARK_GET_PRIVATE(bookmark);

	return priv->is_valid;
}

