/*
 * $Id$
 *
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

#include "monitor_cb.h"

#include "search_gui.h"

RCSID("$Id$");

gboolean on_clist_monitor_button_press_event
    (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    gint row;
    gint col;
    GtkCList *clist_monitor = GTK_CLIST(widget);

	if (event->button != 3)
		return FALSE;

    if (GTK_CLIST(clist_monitor)->selection == NULL)
        return FALSE;

  	if (!gtk_clist_get_selection_info
		(GTK_CLIST(clist_monitor), event->x, event->y, &row, &col))
		return FALSE;

	gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON
            (lookup_widget(main_window, "checkbutton_monitor_enable")), 
        FALSE);
	gtk_menu_popup(GTK_MENU(popup_monitor), NULL, NULL, NULL, NULL, 
                  event->button, event->time);

	return TRUE;
}

void on_popup_monitor_add_search_activate 
    (GtkMenuItem *menuitem, gpointer user_data)
{
	GList *l;
	gchar *titles[1];
	gchar *e;
    GtkCList *clist_monitor = GTK_CLIST
        (lookup_widget(main_window, "clist_monitor"));

	for (
        l = GTK_CLIST(clist_monitor)->selection; 
        l != NULL; 
        l = GTK_CLIST(clist_monitor)->selection 
    ) {		
        gtk_clist_get_text(GTK_CLIST(clist_monitor), (gint) l->data, 0, titles);
        gtk_clist_unselect_row(GTK_CLIST(clist_monitor), (gint) l->data, 0);
     
		e = g_strdup(titles[0]);

		g_strstrip(e);
		if (*e)
			search_gui_new_search(e, 0, NULL);

		g_free(e);
	}	
}

