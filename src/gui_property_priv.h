/*
 * Copyright (c) 2001-2002, Richard Eckart
 *
 * THIS FILE IS AUTOGENERATED! DO NOT EDIT!
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

#ifndef __gui_property_priv_h__
#define __gui_property_priv_h__

#include <glib.h>

extern gboolean monitor_enabled;
extern guint32  monitor_max_items;
extern gboolean queue_regex_case;
extern gboolean search_autoselect;
extern guint32  nodes_col_widths[7];
extern guint32  dl_active_col_widths[6];
extern guint32  dl_queued_col_widths[5];
extern guint32  search_results_col_visible[6];
extern guint32  search_list_col_widths[3];
extern guint32  search_results_col_widths[6];
extern guint32  search_stats_col_widths[3];
extern guint32  ul_stats_col_widths[5];
extern guint32  uploads_col_widths[6];
extern guint32  filter_rules_col_widths[4];
extern guint32  filter_filters_col_widths[3];
extern guint32  gnet_stats_pkg_col_widths[6];
extern guint32  gnet_stats_byte_col_widths[6];
extern guint32  gnet_stats_drop_reasons_col_widths[2];
extern guint32  window_coords[4];
extern guint32  filter_dlg_coords[4];
extern guint32  downloads_divider_pos;
extern guint32  main_divider_pos;
extern guint32  gnet_stats_divider_pos;
extern guint32  side_divider_pos;
extern guint32  search_max_results;
extern guint32  gui_debug;
extern guint32  filter_main_divider_pos;
extern gboolean search_results_show_tabs;
extern gboolean toolbar_visible;
extern gboolean statusbar_visible;
extern gboolean progressbar_uploads_visible;
extern gboolean progressbar_downloads_visible;
extern gboolean progressbar_connections_visible;
extern gboolean progressbar_bws_in_visible;
extern gboolean progressbar_bws_out_visible;
extern gboolean progressbar_bws_gin_visible;
extern gboolean progressbar_bws_gout_visible;
extern gboolean progressbar_bws_in_avg;
extern gboolean progressbar_bws_out_avg;
extern gboolean progressbar_bws_gin_avg;
extern gboolean progressbar_bws_gout_avg;
extern gboolean search_autoselect_ident;
extern gboolean jump_to_downloads;
extern gboolean show_search_results_settings;
extern gboolean search_autoselect_fuzzy;
extern guint32  filter_default_policy;
extern guint32  default_minimum_speed;
extern guint32  search_stats_mode;
extern guint32  search_stats_update_interval;
extern guint32  search_stats_delcoef;
extern gboolean confirm_quit;
extern gboolean show_tooltips;
extern gboolean expert_mode;
extern gboolean gnet_stats_pkg_perc;
extern gboolean gnet_stats_byte_perc;
extern gboolean gnet_stats_drop_perc;
extern guint32  gnet_stats_general_col_widths[2];
extern gboolean clear_uploads;


prop_set_t *gui_prop_init(void);
void gui_prop_shutdown(void);

#endif /* __gui_property_priv_h__ */

