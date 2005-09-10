/*
 * $Id$
 *
 * Copyright (c) 2002-2003, Raphael Manfredi
 *
 *----------------------------------------------------------------------
 * This file is part of gtk-gnutella.
 *
 *  gtk-gnutella is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  gtk-gnutella is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with gtk-gnutella; if not, write to the Free Software
 *  Foundation, Inc.:
 *      59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *----------------------------------------------------------------------
 */

/**
 * @ingroup lib
 * @file
 *
 * URL handling of specific formats.
 *
 * @author Raphael Manfredi
 * @date 2002-2003
 */

#include "common.h"

RCSID("$Id$");

#include "host_addr.h"
#include "url.h"
#include "misc.h"
#include "walloc.h"
#include "override.h"		/* Must be the last header included */

/**
 * @bug XXX -- needs to initialze the debug level of the lib functions -- RAM
 */
static gboolean url_debug = FALSE;

#define ESCAPE_CHAR		'%'
#define TRANSPARENT_CHAR(x,m) \
	((x) >= 32 && (x) < 128 && (is_transparent[(x)-32] & (m)))

/**
 * - Reserved chars: ";", "/", "?", ":", "@", "=" and "&"
 * - Unsafe chars  : " ", '"', "<", ">", "#", and "%"
 * - Misc chars    : "{", "}", "|", "\", "^", "~", "[", "]" and "`"
 *
 * - Bit 0 encodes regular transparent set (pathnames, '/' is transparent).
 * - Bit 1 encodes regular transparent set minus '+' (query string).
 */
static const guint8 is_transparent[96] = {
/*  0 1 2 3 4 5 6 7 8 9 a b c d e f */	/* 0123456789abcdef -            */
    0,3,0,0,3,0,0,3,3,3,3,1,3,3,3,3,	/*  !"#$%&'()*+,-./ -  32 -> 47  */
    3,3,3,3,3,3,3,3,3,3,0,0,0,0,0,0,	/* 0123456789:;<=>? -  48 -> 63  */
    0,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,	/* @ABCDEFGHIJKLMNO -  64 -> 79  */
    3,3,3,3,3,3,3,3,3,3,3,0,0,0,0,3,	/* PQRSTUVWXYZ[\]^_ -  80 -> 95  */
    0,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,	/* `abcdefghijklmno -  96 -> 111 */
    3,3,3,3,3,3,3,3,3,3,3,0,0,0,0,0,	/* pqrstuvwxyz{|}~  - 112 -> 127 */
};

#define PATH_MASK		0x1
#define QUERY_MASK		0x2

static const char hex_alphabet[] = "0123456789ABCDEF";

/**
 * Escape undesirable characters using %xx, where xx is an hex code.
 *
 * @param `url' no brief description.
 * @param `mask' tells us whether we're escaping an URL path or a query string.
 *
 * @return argument if no escaping is necessary, or a new string otherwise.
 */
static gchar *
url_escape_mask(const gchar *url, guint8 mask)
{
	const gchar *p;
	gchar *q;
	int need_escape = 0;
	guchar c;
	gchar *new;

	for (p = url, c = *p++; c; c = *p++)
		if (!TRANSPARENT_CHAR(c, mask))
			need_escape++;

	if (need_escape == 0)
		return deconstify_gchar(url);

	new = g_malloc(p - url + (need_escape << 1));

	for (p = url, q = new, c = *p++; c; c = *p++) {
		if (TRANSPARENT_CHAR(c, mask))
			*q++ = c;
		else {
			*q++ = ESCAPE_CHAR;
			*q++ = hex_alphabet[c >> 4];
			*q++ = hex_alphabet[c & 0xf];
		}
	}
	*q = '\0';

	return new;
}

/**
 * Escape undesirable characters using %xx, where xx is an hex code.
 * This is done in the `target' buffer, whose size is `len'.
 * `mask' tells us whether we're escaping an URL path or a query string.
 *
 * @return amount of characters written into buffer (not counting trailing
 * NUL), or -1 if the buffer was too small.
 */
static gint
url_escape_mask_into(const gchar *url, gchar *target, gint len, guint8 mask)
{
	const gchar *p = url;
	gchar *q;
	guchar c;
	gchar *end = target + len;

	for (q = target, c = *p++; c && q < end; c = *p++) {
		if (TRANSPARENT_CHAR(c, mask))
			*q++ = c;
		else if (end - q >= 3) {
			*q++ = ESCAPE_CHAR;
			*q++ = hex_alphabet[c >> 4];
			*q++ = hex_alphabet[c & 0xf];
		} else
			break;
	}

	g_assert(q <= end);

	if (q == end)
		return -1;

	*q = '\0';

	return q - target;
}

/**
 * Escape undesirable characters using %xx, where xx is an hex code.
 *
 * @return argument if no escaping is necessary, or a new string otherwise.
 */
gchar *
url_escape(const gchar *url)
{
	return url_escape_mask(url, PATH_MASK);
}

/**
 * Same as url_escape(), but '+' are also escaped for the query string.
 *
 * @return argument if no escaping is necessary, or a new string otherwise.
 */
gchar *
url_escape_query(const gchar *url)
{
	return url_escape_mask(url, QUERY_MASK);
}

/**
 * Escape undesirable characters using %xx, where xx is an hex code.
 * This is done in the `target' buffer, whose size is `len'.
 *
 * @return amount of characters written into buffer (not counting trailing
 * NUL), or -1 if the buffer was too small.
 */
gint
url_escape_into(const gchar *url, gchar *target, gint len)
{
	return url_escape_mask_into(url, target, len, PATH_MASK);
}

/**
 * Escape control characters using %xx, where xx is an hex code.
 *
 * @return argument if no escaping is necessary, or a new string otherwise.
 */
gchar *
url_escape_cntrl(gchar *url)
{
	gchar *p;
	gchar *q;
	int need_escape = 0;
	guchar c;
	gchar *new;

	for (p = url, c = *p++; c; c = *p++)
		if (iscntrl(c) || c == ESCAPE_CHAR)
			need_escape++;

	if (need_escape == 0)
		return url;

	new = g_malloc(p - url + (need_escape << 1));

	for (p = url, q = new, c = *p++; c; c = *p++) {
		if (!iscntrl(c) && c != ESCAPE_CHAR)
			*q++ = c;
		else {
			*q++ = ESCAPE_CHAR;
			*q++ = hex_alphabet[c >> 4];
			*q++ = hex_alphabet[c & 0xf];
		}
	}
	*q = '\0';

	return new;
}

/**
 * Unescape string, in-place if `inplace' is TRUE.
 *
 * Returns the argument if un-escaping is NOT necessary, a new string
 * otherwise unless in-place decoding was requested.
 *
 * @return NULL if the argument isn't valid encoded.
 */
gchar *
url_unescape(gchar *url, gboolean inplace)
{
	gchar *p;
	gchar *q;
	gint need_unescape = 0;
	guint unescaped_memory = 0;
	guchar c;
	gchar *new;

	for (p = url; (c = *p) != '\0'; c = *p++)
		if (c == ESCAPE_CHAR) {
			guchar h = *(++p);
			guchar l = *(++p);

			if (
				(h == '0' && l == '0') ||	/* Forbid %00 */
				!(is_ascii_xdigit(h) && is_ascii_xdigit(l))
			) {
				return NULL;
			}
			need_unescape++;
		}

	if (need_unescape == 0)
		return url;

	/*
	 * The "+ 1" in the g_malloc() call below is for the impossible case where
	 * the string would finish on a truncated escape sequence.  In that
	 * case, we would not have enough room for the final trailing NUL.
	 */

	if (inplace)
		new = url;
	else {
		unescaped_memory = p - url - (need_unescape << 1) + 1;
		new = g_malloc(unescaped_memory);
	}

	for (p = url, q = new, c = *p++; c; c = *p++) {
		if (c != ESCAPE_CHAR)
			*q++ = c;
		else {
			if ((c = *p++)) {
				gint v = (hex2dec(c) << 4);
				if ((c = *p++))
					v += hex2dec(c);
				else
					g_assert_not_reached();	/* Handled in pre-scan above */

				g_assert(inplace || new + unescaped_memory >= q);
				*q++ = v;
			} else
				break;
		}
	}

	g_assert(inplace || new + unescaped_memory >= q);

	*q = '\0';

	g_assert(!inplace || new == url);

	return new;
}

/**
 * Parse all the parameters in the URL query string.  All parameter values are
 * stored in their URL-unescaped form, but parameter names are NOT un-escaped.
 *
 * @return an url_params_t object that can be queried for later...
 *         or NULL if the argument isn't valid encoded.
 */
url_params_t *
url_params_parse(gchar *query)
{
	url_params_t *up;
	gchar *q;
	gchar *start;
	gchar *name = NULL;
	gchar *value = NULL;
	gboolean in_value = FALSE;

	up = walloc(sizeof(*up));
	up->params = g_hash_table_new(g_str_hash, g_str_equal);
	up->count = 0;

	for (q = start = query; /* empty */; q++) {
		gchar c = *q;

		if (in_value) {
			if (c == '&' || c == '\0') {		/* End of value */
				*q = '\0';
				value = url_unescape(start, FALSE);
				if (!value) {
					if (name)
						G_FREE_NULL(name);
					url_params_free(up);
					return NULL;
				}
				if (value == start)				/* No unescaping took place */
					value = g_strdup(start);
				*q = c;
				g_hash_table_insert(up->params, name, value);
				up->count++;
				in_value = FALSE;
				name = NULL;
				value = NULL;
				start = q + 1;					/* Name will start there */
			}
		} else {
			if (c == '=') {						/* End of parameter name */
				*q = '\0';
				name = g_strdup(start);
				*q = c;
				in_value = TRUE;
				start = q + 1;					/* Value will start there */
			}
		}

		if (c == '\0')
			break;
	}

	g_assert(name == NULL);
	g_assert(value == NULL);

	return up;
}

/**
 * Get the value of a parameter, or NULL if the parameter is not present.
 * The value returned has already been URL-unescaped.
 */
const gchar *
url_params_get(url_params_t *up, const gchar *name)
{
	g_assert(up != NULL);
	g_assert(up->params != NULL);

	return g_hash_table_lookup(up->params, name);
}

static void
free_params_kv(gpointer key, gpointer value, gpointer unused_udata)
{
	(void) unused_udata;
	g_free(key);
	g_free(value);
}

/**
 * Dispose of the url_params_t structure.
 */
void
url_params_free(url_params_t *up)
{
	g_assert(up != NULL);

	g_hash_table_foreach(up->params, free_params_kv, NULL);
	g_hash_table_destroy(up->params);

	wfree(up, sizeof(*up));
}

static gboolean
url_safe_char(gchar c, url_policy_t p)
{
	if (p & URL_POLICY_GWC_RULES) {
		/* Reject allow anything unreasonable */
		if (!isascii(c)) {
			if (url_debug)
				g_warning("Non-ASCII character in GWC URL; rejected");
			return FALSE;
		}

    	if (!is_ascii_alnum(c) && NULL == strchr("/._-~", c)) {
			if (url_debug)
      			g_warning("Unsafe character in GWC URL; rejected");
      		return FALSE;
    	}
	} else {
		/* XXX: Figure out what's the correct set for this */
		if (!isascii(c) || is_ascii_cntrl(c)) {
			return FALSE;
		}
	}
	return TRUE;
}

/**
 * @attention
 * NB: May modify ``url'' in all cased; pass a copy if necessary!
 *
 * @returns NULL if ``url'' isn't a valid resp. allowed URL. Otherwise,
 * it returns either a pointer to the original URL or a g_malloc()ed
 * modified copy which has to be g_free()ed.
 *
 * The URL is validated according to the specified policy. Unnecessary
 * parts such as ":80" (for the port) and "/./" are removed: the hostname
 * is forced to lowercase; a base URI "/" is appended if missing.
 * This allows comparing different variants of the same URL to detect
 * duplicates.
 */
gchar *
url_normalize(gchar *url, url_policy_t pol)
{
	static const char http_prefix[] = "http://";
	const gchar *p, *uri, *endptr, *tld = NULL;
	gchar c, *q, *warn = NULL;
	host_addr_t addr;
	guint16 port = 0;
	gint dots = 0;

	g_assert(url);

	if (NULL == (q = is_strcaseprefix(url, http_prefix))) {
		if (url_debug)
			g_warning("URL \"%s\" isn't preceded by \"%s\"", url, http_prefix);
		return NULL;
	}
	/* Make sure the prefix is all-lowercase */
	memcpy(url, http_prefix, CONST_STRLEN(http_prefix));

	if (!is_ascii_alnum(*q)) {
		warn = "HTTP prefix MUST be followed by an alphanum";
		goto bad;
	}

	if (
		string_to_host_or_addr(q, &endptr, &addr) &&
		is_host_addr(addr) &&
		('/' == *endptr || ':' == *endptr || '\0' == *endptr)
	) {
		if (!(pol & URL_POLICY_ALLOW_IP_AS_HOST)) {
			warn = "URLs without hostnames have been disabled";
			goto bad;
		}
		q = deconstify_gchar(endptr);

	} else {
		/* The ``host'' part is not an IP address */

		for (/* NOTHING */; *q != '\0'; q++) {

		    for (/* NOTHING */; '\0' != (c = *q); q++) {
        		if (is_ascii_alnum(c)) {
          			*q = ascii_tolower(c);
        		} else if ('-' != c) {
          			break;
        		}
      		}

			if ('\0' == c) {
				if (dots < 1 && !(pol & URL_POLICY_ALLOW_LOCAL_HOSTS)) {
					warn = "current URL policy forbids local hosts";
					goto bad;
				}
				break;
			} else if ('.' == c) {

				if ( !(is_ascii_alnum(*(q - 1)) && is_ascii_alnum(q[1]))) {
					warn = "a dot must be preceded and followed by an alphanum";
					goto bad;
				}
				dots++;
				if ('\0' == q[1] || ':' == q[1] || '/' == q[1])
					break;

				tld = &q[1];
			} else if ('\0' == c || '/' == c || ':' == c) {
				break;
			} else {
				if (url_debug)
					g_warning("Bad URL \"%s\": "
						"invalid character '%c' in ``host:port'' part.",
						url, is_ascii_print(c) ? c : '?');
				return NULL;
			}
		}

		if (!tld || !(is_ascii_alpha(tld[0]) && is_ascii_alpha(tld[1]))) {
			warn = "no or invalid top-level domain";
			goto bad;
		}

	}

	p = q;
	if (':' == *q) {
		guint32 u;
		gint error;

		q++; /* Skip ':' */

		/* Reject port numbers with leading zeros */
		if (!is_ascii_digit(*q) || '0' == *q) {
			error = EINVAL;
			u = 0;
		} else {
			u = parse_uint32(q, &endptr, 10, &error);
		}

		if (error || u < 1 || u > 65535) {
			warn = "':' MUST be followed a by port value (1-65535)";
			goto bad;
		}

		port = (guint16) u;
		p = endptr;
		if (port == /* HTTP_PORT */ 80) {
			/* We don't want the default port in a URL;
			 * this does also prevents duplicates. */
			q--;
		} else {
			q = deconstify_gchar(endptr);
		}
	}

	if ('/' != *p && '\0' != *p) {
		warn = "host must be followed by ':', '/' or NUL";
		goto bad;
	}

	uri = p;

	/* Scan path */
	for (/* NOTHING */; '\0' != (c = *p); p++) {
		if (!url_safe_char(c, pol)) {
			warn = "URL contains characters prohibited by policy";
			goto bad;
		}
	}

	if (q != uri) {
		size_t len;

		len = strlen(uri);
		memmove(q, uri, len);
		q[len] = '\0';
	}
	uri = q;

	if (0 != canonize_path(q, q)) {
		warn = "Could not canonize URI";
		goto bad;
	}

	if (pol & URL_POLICY_GWC_RULES) {
		static const struct {
			const gchar *ext;
			size_t len;
		} static_types[] = {
#define D(x) { (x), CONST_STRLEN(x) }
			D(".html"),
			D(".htm"),
			D(".txt"),
#undef D
		};
		guint i;

		for (i = 0; i < G_N_ELEMENTS(static_types); i++)
    		if (
				0 == ascii_strcasecmp(q - static_types[i].len,
						static_types[i].ext)
			) {
				warn = "URL points probably to static data; rejected";
      			goto bad;
    		}
	}

	/* Add a trailing slash; if the URI is empty (to prevent dupes) */
	if ('\0' == uri[0]) {
		ssize_t len = q - url;
		gchar *s;

		g_assert(len > 0);
		s = g_malloc(len + sizeof "/");
		memcpy(s, url, len);
		s[len] = '/';
		s[len + 1] = '\0';
		url = s;
	}

	if (url_debug)
		g_message("url=\"%s\"", url);

	return url;

bad:
	if (url_debug)
		g_warning("invalid URL \"%s\": %s", url, warn);
	return NULL;
}

/* vi: set ts=4 sw=4 cindent: */
