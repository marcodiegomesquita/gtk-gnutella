/*
 * $Id$
 *
 * Copyright (c) 2009, Raphael Manfredi
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
 * @ingroup core
 * @file
 *
 * Gnutella DHT "publish" interface.
 *
 * @author Raphael Manfredi
 * @date 2009
 */

#include "common.h"

RCSID("$Id$")

#include "pdht.h"
#include "gdht.h"
#include "share.h"
#include "ggep.h"
#include "ggep_type.h"
#include "sockets.h"			/* For socket_listen_port() */
#include "tls_common.h"			/* For tls_enabled() */

#include "if/dht/lookup.h"
#include "if/dht/knode.h"
#include "if/dht/value.h"
#include "if/dht/publish.h"
#include "if/core/fileinfo.h"

#include "if/gnet_property_priv.h"

#include "lib/atoms.h"
#include "lib/cq.h"
#include "lib/misc.h"
#include "lib/walloc.h"
#include "lib/override.h"		/* Must be the last header included */

#define PDHT_ALOC_MAJOR		0	/**< We generate v0.1 "ALOC" values */
#define PDHT_ALOC_MINOR		1

/**
 * Hash table holding all the pending publishes by KUID.
 */
static GHashTable *aloc_publishes;		/* KUID -> pdht_publish_t */

typedef enum { PDHT_PUBLISH_MAGIC = 0x680182c5U } pdht_magic_t;

typedef enum {
	PDHT_T_ALOC = 0,			/**< ALOC value: shared files */
	PDHT_T_PROX,				/**< PROX value: push-proxies */

	PDHT_T_MAX					/**< Amount of publishing types */
} pdht_type_t;

/**
 * Publishing context.
 */
typedef struct pdht_publish {
	pdht_magic_t magic;
	pdht_type_t type;
	pdht_cb_t cb;				/**< Callback to invoke when finished */
	gpointer arg;				/**< Callback argument */
	const kuid_t *id;			/**< Publishing key (atom) */
	union {
		struct pdht_aloc {			/**< Context for ALOC publishing */
			const sha1_t *sha1;		/**< SHA1 of the file being published */
			shared_file_t *sf;		/**< Published file entry, for logs */
		} aloc;
	} u;
} pdht_publish_t;

static inline void
pdht_publish_check(const pdht_publish_t *pp)
{
	g_assert(pp != NULL);
	g_assert(PDHT_PUBLISH_MAGIC == pp->magic);
}

/**
 * English version of the publish type.
 */
static const char *
pdht_type_to_string(pdht_type_t type)
{
	switch (type) {
	case PDHT_T_ALOC:	return "ALOC";
	case PDHT_T_PROX:	return "PROX";
	case PDHT_T_MAX:	break;
	}

	return "UNKNOWN";
}

/**
 * Free publishing context.
 */
static void
pdht_free_publish(pdht_publish_t *pp, gboolean do_remove)
{
	GHashTable *ht = NULL;

	pdht_publish_check(pp);

	switch (pp->type) {
	case PDHT_T_ALOC:
		atom_sha1_free(pp->u.aloc.sha1);
		shared_file_unref(&pp->u.aloc.sf);
		ht = aloc_publishes;
		break;
	case PDHT_T_PROX:
		/* XXX */
		break;
	case PDHT_T_MAX:
		g_assert_not_reached();
	}

	if (do_remove)
		g_hash_table_remove(ht, pp->id);

	kuid_atom_free(pp->id);
	wfree(pp, sizeof *pp);
}

static const char *pdht_errstr[] = {
	"OK",									/**< PDHT_E_OK */
	"Value is popular",						/**< PDHT_E_POPULAR */
	"Error during node lookup",				/**< PDHT_E_LOOKUP */
	"Node lookup expired",					/**< PDHT_E_LOOKUP_EXPIRED */
	"SHA1 of shared file not available",	/**< PDHT_E_SHA1 */
	"Value publishing still pending",		/**< PDHT_E_PENDING */
	"File no longer shared",				/**< PDHT_E_NOT_SHARED */
	"Could not build GGEP DHT value",		/**< PDHT_E_GGEP */
	"Got no acknowledgement at all",		/**< PDHT_E_NONE */
	"Cancelled",							/**< PDHT_E_CANCELLED */
	"UDP queue clogged",					/**< PDHT_E_UDP_CLOGGED */
	"Publish expired",						/**< PDHT_E_PUBLISH_EXPIRED */
	"Publish error",						/**< PDHT_E_PUBLISH_ERROR */
};

/**
 * English representation of an error code.
 */
const char *
pdht_strerror(pdht_error_t code)
{
	STATIC_ASSERT(G_N_ELEMENTS(pdht_errstr) == PDHT_E_MAX);

	if (UNSIGNED(code) >= G_N_ELEMENTS(pdht_errstr))
		return "invalid PDHT error code";

	return pdht_errstr[code];
}

/**
 * Report publishing error.
 */
static void
pdht_publish_error(pdht_publish_t *pp, pdht_error_t code)
{
	pdht_publish_check(pp);

	if (GNET_PROPERTY(publisher_debug) > 1) {
		g_message("PDHT aborting %s publish for %s: %s",
			pdht_type_to_string(pp->type), kuid_to_string(pp->id),
			pdht_strerror(code));
	}

	(*pp->cb)(pp->arg, code, 0);
	pdht_free_publish(pp, pp->id != NULL);
}

/**
 * Callback when publish_value() is done.
 */
static void
pdht_publish_done(gpointer arg, publish_error_t code, unsigned published)
{
	pdht_publish_t *pp = arg;
	pdht_error_t status;

	pdht_publish_check(pp);

	switch (code) {
	case PUBLISH_E_OK:			status = PDHT_E_OK; break;
	case PUBLISH_E_CANCELLED:	status = PDHT_E_CANCELLED; break;
	case PUBLISH_E_UDP_CLOGGED:	status = PDHT_E_UDP_CLOGGED; break;
	case PUBLISH_E_EXPIRED:		status = PDHT_E_PUBLISH_EXPIRED; break;
	case PUBLISH_E_POPULAR:		status = PDHT_E_POPULAR; break;
	case PUBLISH_E_NONE:		status = PDHT_E_NONE; break;
	case PUBLISH_E_ERROR:
	case PUBLISH_E_MAX:
		status = PDHT_E_PUBLISH_ERROR;
		break;
	}

	if (GNET_PROPERTY(publisher_debug) > 1) {
		g_message("PDHT ending %s publish for %s (%u publish%s): %s",
			pdht_type_to_string(pp->type), kuid_to_string(pp->id),
			published, 1 == published ? "" : "es", publish_strerror(code));
	}

	(*pp->cb)(pp->arg, status, published);
	pdht_free_publish(pp, TRUE);
}

/**
 * Generate a DHT "ALOC" value to publish the shared file.
 *
 * @return NULL if problems during GGEP encoding, the DHT value otherwise.
 */
static dht_value_t *
pdht_get_aloc(const shared_file_t *sf, const kuid_t *key)
{
	void *value;
	ggep_stream_t gs;
	int ggep_len;
	gboolean ok;
	const struct tth *tth;
	dht_value_t *aloc;
	knode_t *our_knode;

	/*
	 * An ALOC value bears the following GGEP keys:
	 *
	 * client-id		the servent's GUID as raw 16 bytes
	 * firewalled		1 byte boolean: whether we are TCP-firewalled
	 * port				the port where file can be requested
	 * length			the length of the file, coded as in "LF"
	 * avail			length available, only set when file is partial
	 * tls				no payload, presence means TLS support
	 * ttroot			the TTH root of the file, as raw data
	 * HNAME			the host's DNS name, if known (as in query hits)
	 *
	 * For LimeWire, the first 4 keys are mandatory for ALOC v0.1.
	 * Stupidly enough for the "firewalled" key, which could have been made
	 * optional like "tls", but this is LimeWire's design choice (interns?).
	 */

	value = walloc(DHT_VALUE_MAX_LEN);
	ggep_stream_init(&gs, value, DHT_VALUE_MAX_LEN);
	
	ok = ggep_stream_pack(&gs, GGEP_NAME(client_id),
		GNET_PROPERTY(servent_guid), GUID_RAW_SIZE, 0);

	{
		guint8 fw = booleanize(GNET_PROPERTY(is_firewalled));
		ok = ok && ggep_stream_pack(&gs, GGEP_NAME(firewalled), &fw, 1, 0);
	}

	{
		char buf[sizeof(guint64)];
		int len;

		len = ggept_filesize_encode(shared_file_size(sf), buf);
		g_assert(len > 0 && UNSIGNED(len) <= sizeof buf);
		ok = ok && ggep_stream_pack(&gs, GGEP_NAME(length), buf, len, 0);
	}

	if (shared_file_is_partial(sf)) {
		fileinfo_t *fi = shared_file_fileinfo(sf);

		if (shared_file_size(sf) != fi->done) {
			char buf[sizeof(guint64)];
			int len;

			len = ggept_filesize_encode(fi->done, buf);
			g_assert(len > 0 && UNSIGNED(len) <= sizeof buf);
			ok = ok && ggep_stream_pack(&gs, GGEP_NAME(avail), buf, len, 0);
		}
	}

	{
		char buf[sizeof(guint16)];
		guint16 port = socket_listen_port();

		poke_be16(buf, port);
		ok = ok && ggep_stream_pack(&gs, GGEP_NAME(port), buf, sizeof buf, 0);
	}

	if (tls_enabled()) {
		ok = ok && ggep_stream_pack(&gs, GGEP_NAME(tls), NULL, 0, 0);
	}

	tth = shared_file_tth(sf);
	if (tth != NULL) {
		ok = ok && ggep_stream_pack(&gs, GGEP_NAME(ttroot),
			tth->data, sizeof tth->data, 0);
	}

	if (
		!GNET_PROPERTY(is_firewalled) &&
		GNET_PROPERTY(give_server_hostname) &&
		!is_null_or_empty(GNET_PROPERTY(server_hostname))
	) {
		ok = ok && ggep_stream_pack(&gs, GGEP_NAME(HNAME),
			GNET_PROPERTY(server_hostname),
			strlen(GNET_PROPERTY(server_hostname)), 0);
	}

	ggep_len = ggep_stream_close(&gs);

	g_assert(ggep_len <= DHT_VALUE_MAX_LEN);

	if (!ok) {
		if (GNET_PROPERTY(publisher_debug)) {
			g_warning("PDHT ALOC cannot construct DHT value for %s \"%s\"",
				shared_file_is_partial(sf) ? "partial" : "shared",
				shared_file_name_canonic(sf));
		}

		wfree(value, DHT_VALUE_MAX_LEN);
		return NULL;
	}

	/*
	 * DHT value becomes the owner of the walloc()-ed GGEP block.
	 */

	g_assert(ggep_len > 0);

	value = wrealloc(value, DHT_VALUE_MAX_LEN, ggep_len);
	our_knode = get_our_knode();
	aloc = dht_value_make(our_knode, key, DHT_VT_ALOC,
		PDHT_ALOC_MAJOR, PDHT_ALOC_MINOR, value, ggep_len);
	knode_refcnt_dec(our_knode);

	return aloc;
}

/**
 * Callback when lookup for STORE roots succeeded for ALOC publishing.
 */
static void
pdht_aloc_roots_found(const kuid_t *kuid, const lookup_rs_t *rs, gpointer arg)
{
	pdht_publish_t *pp = arg;
	struct pdht_aloc *paloc = &pp->u.aloc;
	shared_file_t *sf;
	dht_value_t *value;

	pdht_publish_check(pp);
	g_assert(pp->id == kuid);		/* They are atoms */

	sf = paloc->sf;

	if (GNET_PROPERTY(publisher_debug) > 1) {
		size_t roots = rs->path_len;
		g_message("PDHT ALOC found %lu publish root%s for %s \"%s\"",
			(unsigned long) roots, 1 == roots ? "" : "s",
			shared_file_is_partial(sf) ? "partial" : "shared",
			shared_file_name_canonic(sf));
	}

	/*
	 * Step #2: if file is still publishable, generate the DHT ALOC value
	 *
	 * If shared_file_by_sha1() returns SHARE_REBUILDING, we nonetheless
	 * go on with the publishing because chances are the file will still
	 * be shared anyway.  If no longer shared, it will not be requeued for
	 * publishing at the next period.
	 */

	if (NULL == shared_file_by_sha1(paloc->sha1)) {
		if (GNET_PROPERTY(publisher_debug)) {
			g_warning("PDHT ALOC cannot publish %s \"%s\": no longer shared",
				shared_file_is_partial(sf) ? "partial" : "shared",
				shared_file_name_canonic(sf));
		}

		pdht_publish_error(pp, PDHT_E_NOT_SHARED);
		return;
	}

	value = pdht_get_aloc(sf, pp->id);

	if (NULL == value) {
		pdht_publish_error(pp, PDHT_E_GGEP);
		return;
	}

	g_assert(kuid_eq(dht_value_key(value), pp->id));

	/*
	 * Step #3: issue the STORE on each of the k identified nodes.
	 */

	publish_value(value, rs, pdht_publish_done, pp);
}

/**
 * Callback for errors during ALOC publishing.
 */
static void
pdht_aloc_roots_error(const kuid_t *kuid, lookup_error_t error, gpointer arg)
{
	pdht_publish_t *pp = arg;
	struct pdht_aloc *paloc = &pp->u.aloc;
	pdht_error_t status;

	pdht_publish_check(pp);
	g_assert(pp->id == kuid);		/* They are atoms */

	if (GNET_PROPERTY(publisher_debug)) {
		g_message("PDHT ALOC publish roots lookup failed for %s \"%s\": %s",
			shared_file_is_partial(paloc->sf) ? "partial" : "shared",
			shared_file_name_canonic(paloc->sf), lookup_strerror(error));
	}

	switch (error) {
	case LOOKUP_E_UDP_CLOGGED:		status = PDHT_E_UDP_CLOGGED; break;
	case LOOKUP_E_EXPIRED:			status = PDHT_E_PUBLISH_EXPIRED; break;
	default:						status = PDHT_E_LOOKUP; break;
	}

	pdht_publish_error(pp, status);
}

/**
 * Asynchronous error reporting context.
 */
struct pdht_async {
	pdht_publish_t *pp;
	pdht_error_t code;
};

/**
 * Callout queue callback to report error asynchronously.
 */
static void
pdht_report_async_error(struct cqueue *cq, gpointer udata)
{
	struct pdht_async *pa = udata;

	(void) cq;
	pdht_publish_error(pa->pp, pa->code);
	wfree(pa, sizeof *pa);
}

/**
 * Asynchronously report error.
 */
static void
pdht_publish_error_async(pdht_publish_t *pp, pdht_error_t code)
{
	struct pdht_async *pa;

	pa = walloc(sizeof *pa);
	pa->pp = pp;
	pa->code = code;

	cq_insert(callout_queue, 1, pdht_report_async_error, pa);
}

/**
 * Launch publishing of shared file within the DHT.
 *
 * @param sf		the shared file to publish
 * @param cb		callback to invoke when publish is completed
 * @param arg		argument to supply to callback
 */
void
pdht_publish_file(shared_file_t *sf, pdht_cb_t cb, gpointer arg)
{
	const char *error = NULL;
	const sha1_t *sha1;
	pdht_publish_t *pp;
	struct pdht_aloc *paloc;
	pdht_error_t code;

	g_assert(sf != NULL);

	pp = walloc0(sizeof *pp);
	pp->magic = PDHT_PUBLISH_MAGIC;
	pp->type = PDHT_T_ALOC;
	pp->cb = cb;
	pp->arg = arg;
	paloc = &pp->u.aloc;
	paloc->sf = shared_file_ref(sf);

	if (!sha1_hash_available(sf) || !sha1_hash_is_uptodate(sf)) {
		error = "no SHA1 available";
		code = PDHT_E_SHA1;
		goto error;
	}

	sha1 = shared_file_sha1(sf);
	g_assert(sha1 != NULL);

	pp->id = gdht_kuid_from_sha1(sha1);
	paloc->sha1 = atom_sha1_get(sha1);

	if (g_hash_table_lookup(aloc_publishes, pp->id)) {
		error = "previous publish still pending";
		code = PDHT_E_PENDING;
		goto error;
	}

	gm_hash_table_insert_const(aloc_publishes, pp->id, pp);

	/*
	 * Publishing will occur in three steps:
	 *
	 * #1 locate suitable nodes for publishing the ALOC value
	 * #2 if file is still publishable, generate the DHT ALOC value
	 * #3 issue the STORE on each of the k identified nodes.
	 *
	 * Here we launch step #1.
	 */

	ulq_find_store_roots(pp->id,
		pdht_aloc_roots_found, pdht_aloc_roots_error, pp);

	return;

error:
	if (GNET_PROPERTY(publisher_debug)) {
		g_warning("PDHT will not publish ALOC for %s \"%s\": %s",
			shared_file_is_partial(sf) ? "partial" : "shared",
			shared_file_name_canonic(sf), error);
	}

	/*
	 * Report error asynchronously, to return to caller first.
	 */

	pdht_publish_error_async(pp, code);
}

/**
 * Initialize the Gnutella DHT layer.
 */
void
pdht_init(void)
{
	aloc_publishes = g_hash_table_new(sha1_hash, sha1_eq);
}

/**
 * Hash table iterator to free a pdht_publish_t
 */
static void
free_publish_kv(gpointer unused_key, gpointer val, gpointer unused_x)
{
	pdht_publish_t *pp = val;

	(void) unused_key;
	(void) unused_x;

	pdht_free_publish(pp, FALSE);
}

/**
 * Shutdown the Gnutella DHT layer.
 */
void
pdht_close(void)
{
	g_hash_table_foreach(aloc_publishes, free_publish_kv, NULL);
	g_hash_table_destroy(aloc_publishes);
	aloc_publishes = NULL;
}

/* vi: set ts=4 sw=4 cindent: */