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

#ifndef GSWAT_CONTEXT_H
#define GSWAT_CONTEXT_H

#include <glib-object.h>


#define GSWAT_TYPE_CONTEXT           (gswat_context_get_type ())
#define GSWAT_CONTEXT(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSWAT_TYPE_CONTEXT, GSwatContext))
#define GSWAT_IS_CONTEXT(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSWAT_TYPE_CONTEXT))
#define GSWAT_CONTEXT_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GSWAT_TYPE_CONTEXT, GSwatContextIface))

typedef struct _GSwatContextIface GSwatContextIface;
typedef void GSwatContext; /* dummy typedef */

struct _GSwatContextIface
{
	GTypeInterface g_iface;

	/* signals: */

    //void (* signal_member)(GSwatContext *self);

    /* VTable: */
    gboolean (*activate)(GSwatContext *self);
    void (*deactivate)(GSwatContext *self);
};

GType gswat_context_get_type(void);

/* Interface functions */
gboolean gswat_context_activate(GSwatContext *self);
void gswat_context_deactivate(GSwatContext *self);

G_END_DECLS

#endif /* GSWAT_CONTEXT_H */

