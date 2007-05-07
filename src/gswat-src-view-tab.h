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

#ifndef GSWAT_SRC_VIEW_TAB_H
#define GSWAT_SRC_VIEW_TAB_H

#include <glib.h>
#include <glib-object.h>
#include "gswat-view.h"

G_BEGIN_DECLS

#define GSWAT_SRC_VIEW_TAB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSWAT_TYPE_SRC_VIEW_TAB, GSwatSrcViewTab))
#define GSWAT_TYPE_SRC_VIEW_TAB            (gswat_src_view_tab_get_type())
#define GSWAT_SRC_VIEW_TAB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSWAT_TYPE_SRC_VIEW_TAB, GSwatSrcViewTabClass))
#define GSWAT_IS_SRC_VIEW_TAB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSWAT_TYPE_SRC_VIEW_TAB))
#define GSWAT_IS_SRC_VIEW_TAB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSWAT_TYPE_SRC_VIEW_TAB))
#define GSWAT_SRC_VIEW_TAB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSWAT_TYPE_SRC_VIEW_TAB, GSwatSrcViewTabClass))

typedef struct _GSwatSrcViewTab        GSwatSrcViewTab;
typedef struct _GSwatSrcViewTabClass   GSwatSrcViewTabClass;
typedef struct _GSwatSrcViewTabPrivate GSwatSrcViewTabPrivate;

struct _GSwatSrcViewTab
{
    /* add your parent type here */
    GObject parent;

    /* add pointers to new members here */
    
	/*< private > */
	GSwatSrcViewTabPrivate *priv;
};

struct _GSwatSrcViewTabClass
{
    /* add your parent class here */
    GObjectClass parent_class;

    /* add signals here */
    //void (* signal) (GSwatSrcViewTab *object);
};

GType gswat_src_view_tab_get_type(void);

/* add additional methods here */
GSwatSrcViewTab *gswat_src_view_tab_new(gchar *file_uri, gint line);
GSwatView *gswat_src_view_tab_get_view(GSwatSrcViewTab *self);

G_END_DECLS

#endif /* GSWAT_SRC_VIEW_TAB_H */

