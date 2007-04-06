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

#ifndef __GSWAT_MAIN_WINDOW_HEADER
#define __GSWAT_MAIN_WINDOW_HEADER

#include "gswat-session.h"

G_BEGIN_DECLS

typedef GObjectClass            GSwatWindowClass;

/*
 * Type checking and casting macros
 */
#define GSWAT_TYPE_WINDOW            (gswat_window_get_type ())
#define GSWAT_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GSWAT_TYPE_WINDOW, GSwatWindow))
#define GSWAT_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GSWAT_TYPE_WINDOW, GSwatWindowClass))
#define GSWAT_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GSWAT_TYPE_WINDOW))
#define GSWAT_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSWAT_TYPE_WINDOW))
#define GSWAT_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GSWAT_TYPE_WINDOW, GSwatWindowClass))


/* Private structure type */
typedef struct _GSwatWindowPrivate	GSwatWindowPrivate;


/*
 * Main object structure
 */
typedef struct _GSwatWindow		GSwatWindow;


struct _GSwatWindow
{   
    GObject base_instance;
    	
	/*< private > */
	GSwatWindowPrivate *priv;
};


/* Public methods */

GType gswat_window_get_type(void);

GSwatWindow *gswat_window_new(GSwatSession *session);


G_END_DECLS


#endif /* __GSWAT_MAIN_WINDOW_HEADER */

