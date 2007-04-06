/*
 * <preamble>
 * gswat - A graphical program debugger for Gnome
 * Copyright (C) 2006  Robert Bragg
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
 * </preamble>
 */

#ifndef __GSWAT_VIEW_H__
#define __GSWAT_VIEW_H__

#include <gtk/gtk.h>

#include "gedit-view.h"
#include "gedit-document.h"

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define GSWAT_TYPE_VIEW            (gswat_view_get_type ())
#define GSWAT_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GSWAT_TYPE_VIEW, GSwatView))
#define GSWAT_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GSWAT_TYPE_VIEW, GSwatViewClass))
#define GSWAT_IS_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GSWAT_TYPE_VIEW))
#define GSWAT_IS_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSWAT_TYPE_VIEW))
#define GSWAT_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GSWAT_TYPE_VIEW, GSwatViewClass))

/* Private structure type */
typedef struct _GSwatViewPrivate	GSwatViewPrivate;

/*
 * Main object structure
 */
typedef struct _GSwatView		GSwatView;
 
struct _GSwatView
{
	GeditView view;
	
	/*< private > */
	GSwatViewPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _GSwatViewClass		GSwatViewClass;
 
struct _GSwatViewClass
{
	GeditViewClass parent_class;
	
};


typedef struct
{
    GdkColor color;
    gint line;
}GSwatViewLineHighlight;


/*
 * Public methods
 */
GtkType	    gswat_view_get_type     	(void) G_GNUC_CONST;

GtkWidget	*gswat_view_new(GeditDocument *doc);

void        gswat_view_set_line_highlights(GSwatView *view, const GList *line_hilights);
void        gswat_view_add_breakpoint(GSwatView *view, gint line);
void        gswat_view_remove_breakpoint(GSwatView *view, gint line);

GeditDocument *gswat_view_get_document(GSwatView *view);

G_END_DECLS

#endif /* __GSWAT_VIEW_H__ */

