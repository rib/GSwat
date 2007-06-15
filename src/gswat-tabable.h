/*
 * <preamble>
 * gswat - A graphical program debugger for Gnome
 * Copyright (C) 2006  Robert Bragg
 * </preamble>
 * 
 * <license>
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * </license>
 *
 */

#ifndef GSWAT_TABABLE_H
#define GSWAT_TABABLE_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GSWAT_TYPE_TABABLE           (gswat_tabable_get_type ())
#define GSWAT_TABABLE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSWAT_TYPE_TABABLE, GSwatTabable))
#define GSWAT_IS_TABABLE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSWAT_TYPE_TABABLE))
#define GSWAT_TABABLE_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GSWAT_TYPE_TABABLE, GSwatTabableIface))


typedef struct _GSwatTabableIface GSwatTabableIface;
typedef void GSwatTabable; /* dummy typedef */

struct _GSwatTabableIface
{
	GTypeInterface g_iface;

	/* signals: */

    //void (* signal_member)(GSwatTabable *self);

    /* VTable: */
};

/* state flags */
#define GSWAT_TABABLE_BUSY          (1UL<<0)
#define GSWAT_TABABLE_CLOSEABLE     (1UL<<1)


GType gswat_tabable_get_type(void);

/* Interface functions */
gchar *gswat_tabable_get_label_text(GSwatTabable *object);
GdkPixbuf *gswat_tabable_get_icon(GSwatTabable *object);
gchar *gswat_tabable_get_tooltip(GSwatTabable *object);
gulong gswat_tabable_get_state_flags(GSwatTabable *object);
GtkWidget *gswat_tabable_get_page_widget(GSwatTabable *object);
GtkWidget *gswat_tabable_get_focal_widget(GSwatTabable *object);

G_END_DECLS

#endif /* GSWAT_TABABLE_H */

