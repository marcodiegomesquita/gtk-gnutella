/*
 * $Id$
 *
 * Copyright (c) 2003, Christian Biere
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

#ifndef _hashlist_h_
#define _hashlist_h_

#include <glib.h>

typedef struct hash_list_iter hash_list_iter_t;
typedef struct hash_list hash_list_t;

hash_list_t *hash_list_new(void);
void hash_list_free(hash_list_t *hl);
void hash_list_remove(hash_list_t *hl, gpointer data);
void hash_list_append(hash_list_t *hl, gpointer data);
void hash_list_prepend(hash_list_t *hl, gpointer data);
gpointer hash_list_first(const hash_list_t *hl);
gpointer hash_list_last(const hash_list_t *hl);
gulong hash_list_length(const hash_list_t *hl);
gpointer hash_list_get_iter(hash_list_t *hl, hash_list_iter_t **i);
void hash_list_release(hash_list_iter_t *i);
gboolean hash_list_has_next(const hash_list_iter_t *i);
gboolean hash_list_has_previous(const hash_list_iter_t *i);
gboolean hash_list_contains(hash_list_t *hl, gpointer data);
void hash_list_foreach(const hash_list_t *hl, GFunc func, gpointer user_data);

#endif	/* _hashlist_h_ */
