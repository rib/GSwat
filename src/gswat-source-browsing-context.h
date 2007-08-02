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

#ifndef GSWAT_SOURCE_BROWSING_CONTEXT_H
#define GSWAT_SOURCE_BROWSING_CONTEXT_H

#include <glib.h>
#include <glib-object.h>

#include "gswat-window.h"

G_BEGIN_DECLS

#define GSWAT_SOURCE_BROWSING_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSWAT_TYPE_SOURCE_BROWSING_CONTEXT, GSwatSourceBrowsingContext))
#define GSWAT_TYPE_SOURCE_BROWSING_CONTEXT            (gswat_source_browsing_context_get_type())
#define GSWAT_SOURCE_BROWSING_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSWAT_TYPE_SOURCE_BROWSING_CONTEXT, GSwatSourceBrowsingContextClass))
#define GSWAT_IS_SOURCE_BROWSING_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSWAT_TYPE_SOURCE_BROWSING_CONTEXT))
#define GSWAT_IS_SOURCE_BROWSING_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSWAT_TYPE_SOURCE_BROWSING_CONTEXT))
#define GSWAT_SOURCE_BROWSING_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSWAT_TYPE_SOURCE_BROWSING_CONTEXT, GSwatSourceBrowsingContextClass))

typedef struct _GSwatSourceBrowsingContext        GSwatSourceBrowsingContext;
typedef struct _GSwatSourceBrowsingContextClass   GSwatSourceBrowsingContextClass;
typedef struct _GSwatSourceBrowsingContextPrivate GSwatSourceBrowsingContextPrivate;

struct _GSwatSourceBrowsingContext
{
    /* add your parent type here */
    GObject parent;

    /* add pointers to new members here */
    
	/*< private > */
	GSwatSourceBrowsingContextPrivate *priv;
};

struct _GSwatSourceBrowsingContextClass
{
    /* add your parent class here */
    GObjectClass parent_class;

    /* add signals here */
    //void (* signal) (GSwatSourceBrowsingContext *object);
};

GType gswat_source_browsing_context_get_type(void);

/* add additional methods here */
GSwatSourceBrowsingContext *gswat_source_browsing_context_new(GSwatWindow *window);

G_END_DECLS

#endif /* GSWAT_SOURCE_BROWSING_CONTEXT_H */

