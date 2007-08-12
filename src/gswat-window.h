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

#ifndef GSWAT_WINDOW_H
#define GSWAT_WINDOW_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "gswat-session.h"
#include "gswat-debuggable.h"
#include "gswat-tabable.h"

G_BEGIN_DECLS

#define GSWAT_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSWAT_TYPE_WINDOW, GSwatWindow))
#define GSWAT_TYPE_WINDOW            (gswat_window_get_type())
#define GSWAT_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSWAT_TYPE_WINDOW, GSwatWindowClass))
#define GSWAT_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSWAT_TYPE_WINDOW))
#define GSWAT_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSWAT_TYPE_WINDOW))
#define GSWAT_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSWAT_TYPE_WINDOW, GSwatWindowClass))

typedef struct _GSwatWindow        GSwatWindow;
typedef struct _GSwatWindowClass   GSwatWindowClass;
typedef struct _GSwatWindowPrivate GSwatWindowPrivate;

struct _GSwatWindow
{
    /* add your parent type here */
    GObject parent;

    /* add pointers to new members here */
    
	/*< private > */
	GSwatWindowPrivate *priv;
};

struct _GSwatWindowClass
{
    /* add your parent class here */
    GObjectClass parent_class;

    /* add signals here */
    //void (* signal) (GSwatWindow *object);
};

typedef enum {
    GSWAT_WINDOW_CONTAINER_MAIN,
    GSWAT_WINDOW_CONTAINER_RIGHT0,
    GSWAT_WINDOW_CONTAINER_RIGHT1,
    GSWAT_WINDOW_CONTAINER_BOTTOM,
    GSWAT_WINDOW_CONTAINER_COUNT
}GSwatWindowContainerID;

/* Contexts represent the task that a user is
 * activly focused on.
 */
typedef enum {
    /* When started without a session, the intention is
     * that this limbo context will display your most
     * recent sessions for fast access */
    GSWAT_WINDOW_IN_LIMBO_CONTEXT,
    /* Only valid while connected to a debuggable target;
     * this context is active while you are literally
     * stepping through and debugging code. */
    GSWAT_WINDOW_CONTROL_FLOW_DEBUGGING_CONTEXT,
    /* Valid when you have a session; this context
     * can be activated at any time to browse through
     * source code (e.g. for setting breakpoints)*/
    GSWAT_WINDOW_SOURCE_BROWSING_CONTEXT,
    
    /* TODO: Use somthing like GEGL to provide a no-fuss note
     * taking pad. It would e.g. be nice if it supported
     * graphics tablets for doodling diagramatic notes
     * too. Plugins could be used for exporting data structures
     * from a debuggable as nice graphics (i.e. pictures
     * worth 100 words) */
    /* GSWAT_WINDOW_NOTE_TAKING_CONTEXT, */
    
    /* TODO: we may want to be able to dynamically create
     * new contexts for some complex plugins in the
     * future. 
     * We could also use a tree structure and allow
     * sub-contexts if necissary. */
    GSWAT_WINDOW_CONTEXT_COUNT

}GSwatWindowContextID;


GType gswat_window_get_type(void);

/* add additional methods here */
GSwatWindow *gswat_window_new(GSwatSession *session);

GSwatDebuggable *gswat_window_get_debuggable(GSwatWindow *self);
GtkContainer *gswat_window_get_container(GSwatWindow *self,
                                         GSwatWindowContainerID container_id);
GSwatWindowContextID gswat_window_get_context_id(GSwatWindow *self);
gboolean gswat_window_set_context(GSwatWindow *self, GSwatWindowContainerID context_id);
void gswat_window_insert_widget(GSwatWindow *self,
                                GtkWidget *widget,
                                GSwatWindowContainerID container_id);
void gswat_window_insert_tabable(GSwatWindow *self,
                                 GSwatTabable *tabable,
                                 GSwatWindowContainerID container_id);

GtkUIManager *gswat_window_get_ui_manager(GSwatWindow *self);

G_END_DECLS

#endif /* GSWAT_WINDOW_H */

