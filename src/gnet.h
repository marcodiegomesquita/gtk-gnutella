/*
 * Copyright (c) 2001-2002, Raphael Manfredi, Richard Eckart
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

#ifndef __gnet_h__
#define __gnet_h__

#include "gnutella.h"
#include "listener.h"
#include "prop.h"

/***
 *** Proxy protocols
 ***/
enum {
    PROXY_HTTP = 1,
    PROXY_SOCKSV4 = 4,
    PROXY_SOCKSV5 = 5
};

/***
 *** Properties
 ***/
#include "gnet_property.h"

/***
 *** Gnet nodes
 ***/

/*
 * Gnet node specific types
 */
typedef guint32 gnet_node_t;

typedef struct gnet_node_info {
    gnet_node_t node_handle;    /* Internal node handle */

    gchar *error_str;           /* To sprintf() error strings with vars */
	gint proto_major;			/* Protocol major number */
	gint proto_minor;			/* Protocol minor number */
	gchar *vendor;				/* Vendor information */
	guchar vcode[4];			/* Vendor code (vcode[0] == NUL when unknown) */

	guchar status;			    /* See possible values below */
	guchar peermode;		    /* See possible values below */
	guint32 flags;			    /* See possible values below */
	guint32 attrs;			    /* See possible values below */

	guint32 sent;				/* Number of sent packets */
	guint32 received;			/* Number of received packets */
	guint32 tx_dropped;			/* Number of packets dropped at TX time */
	guint32 rx_dropped;			/* Number of packets dropped at RX time */
	guint32 n_bad;				/* Number of bad packets received */
	guint16 n_dups;				/* Number of dup messages received (bad) */
	guint16 n_hard_ttl;			/* Number of hard_ttl exceeded (bad) */
	guint32 n_weird;			/* Number of weird messages from that node */

    gint squeue_sent;
    gint squeue_count;
    gint mqueue_count;
    gint mqueue_percent_used;
    gboolean in_tx_flow_control;

	time_t last_update;			/* Last update of the node in the GUI */
	time_t connect_date;		/* When we got connected (after handshake) */
	time_t tx_flowc_date;		/* When we entered in TX flow control */
	time_t shutdown_date;		/* When we entered in shutdown mode */
	guint32 shutdown_delay;		/* How long we can stay in shutdown mode */

    gchar *remove_msg;          /* Reason of removing */

	guint32 ip;					/* ip of the node */
	guint16 port;				/* port of the node */

	/*
	 * Traffic statistics -- RAM, 13/05/2002.
	 */

	gint32 tx_given;			/* Bytes fed to the TX stack (from top) */
	gint32 tx_deflated;			/* Bytes deflated by the TX stack */
	gint32 tx_written;			/* Bytes written by the TX stack */
    gboolean tx_compressed;     /* Is tx traffic compressed */
    float  tx_compression_ratio;/* Tx compression ratio */
	
	gint32 rx_given;			/* Bytes fed to the RX stack (from bottom) */
	gint32 rx_inflated;			/* Bytes inflated by the RX stack */
	gint32 rx_read;				/* Bytes read from the RX stack */
    gboolean rx_compressed;     /* Is rx traffic compressed */
    float  rx_compression_ratio;/* Rx compression ratio */

} gnet_node_info_t;

/*
 * Node states.
 */
#define GTA_NODE_CONNECTING			1	/* Making outgoing connection */
#define GTA_NODE_HELLO_SENT			2	/* Sent 0.4 hello */
#define GTA_NODE_WELCOME_SENT		3	/* Hello accepted, remote welcomed */
#define GTA_NODE_CONNECTED			4	/* Connected at the Gnet level */
#define GTA_NODE_REMOVING			5	/* Removing node */
#define GTA_NODE_RECEIVING_HELLO	6	/* Receiving 0.6 headers */
#define GTA_NODE_SHUTDOWN			7	/* Connection being shutdown */

/*
 * Nodes callback definitions
 */
typedef void (*node_added_listener_t)   (gnet_node_t, const gchar *);
typedef void (*node_removed_listener_t) (gnet_node_t);
typedef void (*node_changed_listener_t) (gnet_node_t, gboolean);

#define node_add_listener(signal, callback) \
    node_add_##signal##_listener(callback);

#define node_remove_listener(signal, callback) \
    node_remove_##signal##_listener(callback);

void node_add_node_added_listener(node_added_listener_t);
void node_remove_node_added_listener(node_added_listener_t);
void node_add_node_removed_listener(node_removed_listener_t);
void node_remove_node_removed_listener(node_removed_listener_t);
void node_add_node_changed_listener(node_changed_listener_t);
void node_remove_node_changed_listener(node_changed_listener_t);

/*
 * Nodes public interface
 */
void node_add(guint32, guint16);
void node_remove_by_handle(gnet_node_t n);
void node_remove_nodes_by_handle(GList *node_list);
gnet_node_info_t *node_get_info(const gnet_node_t n);
void node_free_info(gnet_node_info_t *info);





/***
 *** Sharing
 ***/

/*
 * Sharing callbacks
 */
typedef void (*search_request_listener_t) (const gchar *);

void share_add_search_request_listener(search_request_listener_t l);
void share_remove_search_request_listener(search_request_listener_t l);





/***
 *** Searches
 ***/
typedef guint32 gnet_search_t;

typedef struct gnet_search_info {
    gnet_search_t search_handle;

	gchar *query;				/* The search query */
	guint16 speed;				/* Minimum speed for the results of query */
	time_t time;				/* Time when this search was started */
	guint32 items;				/* Total number of items for this search */

	time_t last_update_time;	/* the last time the notebook tab was updated */
	guint32 last_update_items;	/* Number of items included in last update */

	gboolean passive;			/* Is this a passive search? */
	gboolean frozen;			/* True => don't update window */

	guint reissue_timeout;		/* timeout per search, 0 = search stopped */
} gnet_search_info_t;

gnet_search_info_t *search_get_info(const gnet_search_t);
void search_free_info(gnet_search_info_t *);





/***
 *** Filters
 ***/
enum rule_type {
    RULE_TEXT = 0,
    RULE_IP,
    RULE_SIZE,
    RULE_JUMP,
    RULE_SHA1,
    RULE_FLAG,
    RULE_STATE
};

enum rule_text_type {
    RULE_TEXT_PREFIX,
    RULE_TEXT_WORDS,
    RULE_TEXT_SUFFIX,
    RULE_TEXT_SUBSTR,
    RULE_TEXT_REGEXP,
    RULE_TEXT_EXACT
};

enum rule_flag_action {
    RULE_FLAG_SET = 0,
    RULE_FLAG_UNSET = 1,
    RULE_FLAG_IGNORE = 2
};

/*
 * MAX_FILTER_PROP is used to know how many FILTER_PROPS there are.
 */
typedef enum filter_prop {
    FILTER_PROP_DISPLAY = 0,
    FILTER_PROP_DOWNLOAD,
    MAX_FILTER_PROP
} filter_prop_t;

/*
 * The states a filter_property. I chose 0 for UNKNOWN because that
 * makes it easy to initialize the property array with g_new0 and
 * it's easy to check if the state is still unset by !.
 * FILTER_PROP_IGNORE is needed because we also want filter rules
 * that allow to act only on one property and ignores the other.
 */
typedef enum filter_prop_state {
    FILTER_PROP_STATE_UNKNOWN = 0,
    FILTER_PROP_STATE_DO,
    FILTER_PROP_STATE_DONT,
    MAX_FILTER_PROP_STATE,
    FILTER_PROP_STATE_IGNORE
} filter_prop_state_t;
#endif /* __gnet_h__ */

