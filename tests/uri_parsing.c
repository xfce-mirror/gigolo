/*
 *      uri_parsing.c
 *
 *      Copyright 2009-2011 Enrico Tröger <enrico(at)xfce(dot)org>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib-object.h>
#include <gtk/gtk.h>

#include "common.h"
#include "bookmark.h"

typedef struct
{
	const gchar *input;
	const gchar	*output;

	const gchar	*scheme;
	const gchar	*host;
	const gchar	*domain;
	const gchar	*share;
	const gchar	*path;
	const guint	 port;
	const gchar	*user;
} TestURI;


static gboolean report_fail(const TestURI *tu, const gchar *name, const gchar *result, const gchar *expected)
{
	g_print("Input: %s\nExpected %s: %s\nResult %s: %s\n\n", tu->input, name, expected, name, result);
	return FALSE;
}


static gboolean check_if_details_equal(GigoloBookmark *bm, const TestURI *tu)
{
	const gchar *val;
	guint port;

	val = gigolo_bookmark_get_host(bm);
	if (! gigolo_str_equal(val, tu->host))
		return report_fail(tu, "host", val, tu->host);

	val = gigolo_bookmark_get_scheme(bm);
	if (! gigolo_str_equal(val, tu->scheme))
		return report_fail(tu, "scheme", val, tu->scheme);

	val = gigolo_bookmark_get_share(bm);
	if (! gigolo_str_equal(val, tu->share))
		return report_fail(tu, "share", val, tu->share);

	val = gigolo_bookmark_get_path(bm);
	if (! gigolo_str_equal(val, tu->path))
		return report_fail(tu, "path", val, tu->path);

	val = gigolo_bookmark_get_domain(bm);
	if (! gigolo_str_equal(val, tu->domain))
		return report_fail(tu, "domain", val, tu->domain);

	val = gigolo_bookmark_get_user(bm);
	if (! gigolo_str_equal(val, tu->user))
		return report_fail(tu, "user", val, tu->user);

	port = gigolo_bookmark_get_port(bm);
	if (port != tu->port)
	{
		g_print("Input: %s\nExpected port: %d\nResult port: %d\n\n", tu->input, tu->port, port);
		return FALSE;
	}

	return TRUE;
}


gint main(gint argc, gchar **argv)
{
	const TestURI tests[] =
	{
		{ "http://localhost", "http://localhost/", "http", "localhost", NULL, NULL, NULL, 0, NULL },
		{ "http://localhost:8080/", "http://localhost:8080/", "http", "localhost", NULL, NULL, NULL, 8080, NULL },
		{ "sftp://user@localhost:22", "sftp://user@localhost/", "sftp", "localhost", NULL, NULL, NULL, 22, "user" },
		{ "sftp://user@localhost:8022", "sftp://user@localhost:8022/", "sftp", "localhost", NULL, NULL, NULL, 8022, "user" },
		{ "ftp://localhost", "ftp://localhost/", "ftp", "localhost", NULL, NULL, NULL, 0, NULL },
		{ "ftp://user@localhost:21/a", "ftp://user@localhost/", "ftp", "localhost", NULL, NULL, NULL, 21, "user" },
		{ "ftp://user@usershost@localhost:8021/a", "ftp://user@usershost@localhost:8021/", "ftp", "localhost", NULL, NULL, NULL, 8021, "user@usershost" },
		{ "smb://user@localhost", "smb://user@localhost/", "smb", "localhost", NULL, NULL, NULL, 0, "user"},
		{ "smb://user@localhost/share", "smb://user@localhost/share/", "smb", "localhost", NULL, "share", NULL, 0, "user"},
		{ "smb://user@localhost/share/", "smb://user@localhost/share/", "smb", "localhost", NULL, "share", NULL, 0, "user"},
		{ "smb://user@name@localhost/share/and/", "smb://user@name@localhost/share/", "smb", "localhost", NULL, "share", NULL, 0, "user@name"},
		{ "smb://domain;user@localhost/share/and/more", "smb://domain;user@localhost/share/", "smb", "localhost", "domain", "share", NULL, 0, "user"},
		{ "dav://localhost.localdomain/", "dav://localhost.localdomain/", "dav", "localhost.localdomain", NULL, NULL, NULL, 0, NULL},
		{ "dav://localhost.localdomain", "dav://localhost.localdomain/", "dav", "localhost.localdomain", NULL, NULL, NULL, 0, NULL},
		{ "dav://user@dav.domain.tld/owncloud/files/webdav.php", "dav://user@dav.domain.tld/owncloud/files/webdav.php", "dav", "dav.domain.tld", NULL, NULL, "owncloud/files/webdav.php", 0, "user"},
		{ "davs://user@localhost.localdomain/path/path2/more//", "davs://user@localhost.localdomain/path/path2/more", "davs", "localhost.localdomain", NULL, NULL, "path/path2/more", 0, "user"},
		{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL }
	};
	GigoloBookmark *bm;
	gchar *new_uri;
	gsize i;

	gtk_init_check(&argc, &argv);

	bm = gigolo_bookmark_new();

	for (i = 0; tests[i].input != NULL; i++)
	{
		gigolo_bookmark_clear(bm);
		gigolo_bookmark_parse_uri(bm, tests[i].input);
		new_uri = gigolo_bookmark_get_uri(bm);
		if (! gigolo_str_equal(tests[i].output, new_uri))
		{
			g_print("Input: %s\nExpected: %s\nResult: %s\n\n", tests[i].input, tests[i].output, new_uri);
			return 1;
		}
		if (! gigolo_str_equal(tests[i].output, new_uri))
			return 1;
		if (! check_if_details_equal(bm, &tests[i]))
			return 1;
		g_free(new_uri);
	}

	return 0;
}
