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

#ifndef GSWAT_VIEW_H
#define GSWAT_VIEW_H

#include <gtk/gtk.h>

#include "gedit-view.h"
#include "gedit-document.h"

G_BEGIN_DECLS

#define GSWAT_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSWAT_TYPE_VIEW, GSwatView))
#define GSWAT_TYPE_VIEW            (gswat_view_get_type())
#define GSWAT_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSWAT_TYPE_VIEW, GSwatViewClass))
#define GSWAT_IS_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSWAT_TYPE_VIEW))
#define GSWAT_IS_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSWAT_TYPE_VIEW))
#define GSWAT_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSWAT_TYPE_VIEW, GSwatViewClass))

typedef struct _GSwatView        GSwatView;
typedef struct _GSwatViewClass   GSwatViewClass;
typedef struct _GSwatViewPrivate GSwatViewPrivate;

struct _GSwatView
{
    /* add your parent type here */
    GeditView parent;

    /* add pointers to new members here */
    
	/*< private > */
	GSwatViewPrivate *priv;
};

struct _GSwatViewClass
{
    /* add your parent class here */
    GeditViewClass parent_class;

    /* add signals here */
    //void (* signal) (GSwatView *object);
};

typedef struct
{
    GdkColor color;
    gint line;
}GSwatViewLineHighlight;

GType gswat_view_get_type(void);

/* add additional methods here */
GtkWidget *gswat_view_new(GeditDocument *doc);
void gswat_view_set_line_highlights(GSwatView *self,
                                    const GList *line_highlights);
GeditDocument *gswat_view_get_document(GSwatView *self);


G_END_DECLS

#endif /* GSWAT_VIEW_H */

