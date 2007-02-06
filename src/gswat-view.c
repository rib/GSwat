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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>

#include <gdk/gdkkeysyms.h>

#include <glib/gi18n.h>

#include "gswat-view.h"
#include "gswat-debugger.h"
#include "gedit-prefs-manager.h"


#define GSWAT_VIEW_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GSWAT_TYPE_VIEW, GSwatViewPrivate))



struct _GSwatViewPrivate
{
    //GList *breakpoints;
    GSwatDebugger *debugger;
};


typedef struct {
    gint line;
}GSwatViewBreakpoint;

static void	gswat_view_destroy(GtkObject       *object);
static void	gswat_view_finalize(GObject         *object);
static gint gswat_view_focus_out(GtkWidget       *widget,
                                 GdkEventFocus   *event);

static void debugger_state_update(GObject *object,
                                  GParamSpec *property,
                                  gpointer data);
static gboolean gswat_view_expose(GtkWidget *widget,
                                  GdkEventExpose *event);

G_DEFINE_TYPE(GSwatView, gswat_view, GEDIT_TYPE_VIEW);

/* Signals */
/*
   enum
   {
   GSWAT_VIEW_PLACEHOLDER_SIGNAL,
   LAST_SIGNAL
   };
   */

//static guint view_signals [LAST_SIGNAL] = { 0 };



static void
gswat_view_class_init(GSwatViewClass *klass)
{
    GObjectClass     *object_class = G_OBJECT_CLASS (klass);
    GtkObjectClass   *gtkobject_class = GTK_OBJECT_CLASS (klass);
    //GtkTextViewClass *textview_class = GTK_TEXT_VIEW_CLASS (klass);
    GtkWidgetClass   *widget_class = GTK_WIDGET_CLASS (klass);

    //GtkBindingSet    *binding_set;

    gtkobject_class->destroy = gswat_view_destroy;
    object_class->finalize = gswat_view_finalize;

    widget_class->focus_out_event = gswat_view_focus_out;
    widget_class->expose_event = gswat_view_expose;

    g_type_class_add_private (klass, sizeof (GSwatViewPrivate));

    //binding_set = gtk_binding_set_by_class (klass);

}
/*
static gboolean
test_expose(GtkWidget *widget,
            GdkEventExpose *event,
            gpointer user_data)
{
    gswat_view_expose(widget, event);
    return FALSE;
}
*/

static void 
gswat_view_init(GSwatView *view)
{	

    view->priv = GSWAT_VIEW_GET_PRIVATE(view);

    //g_signal_connect(view, "expose-event", G_CALLBACK(test_expose), NULL);

}

static void
gswat_view_destroy (GtkObject *object)
{
    GSwatView *view;

    view = GSWAT_VIEW (object);


    (* GTK_OBJECT_CLASS (gswat_view_parent_class)->destroy) (object);
}

static void
gswat_view_finalize (GObject *object)
{
    GSwatView *view;

    view = GSWAT_VIEW (object);
    
    gtk_widget_hide_all(GTK_WIDGET(view));

    g_object_unref(view->priv->debugger);

    (* G_OBJECT_CLASS (gswat_view_parent_class)->finalize) (object);
}

static gint
gswat_view_focus_out (GtkWidget *widget, GdkEventFocus *event)
{
    //GSwatView *view = GSWAT_VIEW (widget);

    gtk_widget_queue_draw (widget);

    (* GTK_WIDGET_CLASS (gswat_view_parent_class)->focus_out_event) (widget, event);

    return FALSE;
}


GtkWidget *
gswat_view_new(GeditDocument *doc, GSwatDebugger *debugger)
{
    GSwatView *view;
    //GSwatViewBreakpoint *breakpoint;
    
    g_return_val_if_fail(GEDIT_IS_DOCUMENT (doc), NULL);

    g_object_ref(debugger);

    view = g_object_new(GSWAT_TYPE_VIEW, NULL);

    gtk_text_view_set_buffer(GTK_TEXT_VIEW(view),
                             GTK_TEXT_BUFFER(doc));

    gtk_text_view_set_editable(GTK_TEXT_VIEW (view), 
                               FALSE);					  

    view->priv->debugger = debugger;

    /* add a dummy breakpoint */
    /*
       breakpoint = g_new0(GSwatViewBreakpoint, 1);
       breakpoint->line = 3;
       view->priv->breakpoints=g_list_append(view->priv->breakpoints, breakpoint);
       */

    /* so we can make sure the view always tracks changes to the
     * debugger state */
    g_signal_connect(debugger, "notify::source-line", G_CALLBACK(debugger_state_update), view);

    gtk_widget_show_all(GTK_WIDGET(view));

    return GTK_WIDGET(view);
}


static void
debugger_state_update(GObject *object,
                      GParamSpec *property,
                      gpointer data)
{
    GSwatView *self = GSWAT_VIEW(data);
    GdkWindow *window = gtk_text_view_get_window(GTK_TEXT_VIEW(self), GTK_TEXT_WINDOW_TEXT);


    gdk_window_invalidate_rect(window,
                               NULL,
                               FALSE);
                               
}



/*
   void
   gswat_view_remove_breakpoint(GSwatView *view, gint line)
   {
   GList *tmp;
   GSwatViewBreakpoint *current_breakpoint;

   for(tmp=view->priv->breakpoints; tmp != NULL; tmp=tmp->next)
   {
   current_breakpoint = (GSwatViewBreakpoint *)tmp->data;

   if(current_breakpoint->line == line)
   {
   view->priv->breakpoints = 
   g_list_remove(view->priv->breakpoints, current_breakpoint);
   g_free(current_breakpoint);
   return;
   }
   }
   }

   void
   gswat_view_add_breakpoint(GSwatView *view, gint line)
   {
   GSwatViewBreakpoint *new_breakpoint;

   gswat_view_remove_breakpoint(view, line);

   new_breakpoint = g_new(GSwatViewBreakpoint, 1);
   new_breakpoint->line = line;

   view->priv->breakpoints = 
   g_list_prepend(view->priv->breakpoints, new_breakpoint);
   }
   */

static gboolean
gswat_view_expose(GtkWidget *widget,
                  GdkEventExpose *event)
{
    GtkTextView *text_view = GTK_TEXT_VIEW(widget);
    GeditView *gedit_view = GEDIT_VIEW(widget);
    GSwatView *gswat_view = GSWAT_VIEW(widget);
    GList *tmp = NULL;
    GSwatDebugger *debugger;
    GList *breakpoints;

    GdkRectangle visible_rect;
    GdkRectangle redraw_rect;
    GtkTextIter cur;
    gint y;
    gint height;
    gint win_y;
    gulong line;
    GdkColor color;
    GdkGC *gc;
    

    debugger = gswat_view->priv->debugger;
    if(!debugger)
    {
        goto parent_expose;
    }


    breakpoints = gswat_debugger_get_breakpoints(debugger);


    if ((event->window != gtk_text_view_get_window(text_view, GTK_TEXT_WINDOW_TEXT)))
    {
        goto parent_expose;
    }

    gtk_text_view_get_visible_rect(text_view, &visible_rect);
    
    gc = gdk_gc_new(GDK_DRAWABLE(event->window));
    
    for(tmp=breakpoints; tmp != NULL; tmp=tmp->next)
    {
        GSwatDebuggerBreakpoint *current_breakpoint;
        gchar *current_doc_uri;

        g_message("breakpoint");
        current_breakpoint = (GSwatDebuggerBreakpoint *)tmp->data;
        current_doc_uri = gedit_document_get_uri(
                                gedit_view_get_document(gedit_view)
                                );
    
        if(current_doc_uri)
            g_message("current_doc_uri=%s", current_doc_uri);
        if(current_breakpoint->source_uri)
            g_message("current_breakpoint->source_uri=%s",
                        current_breakpoint->source_uri);

        if( !current_doc_uri ||
            strcmp(current_breakpoint->source_uri,
                   current_doc_uri
                  ) != 0
          )
        {
            continue;
        }

        gtk_text_buffer_get_iter_at_line(text_view->buffer,
                                         &cur,
                                         current_breakpoint->line);

        gtk_text_view_get_line_yrange(text_view, &cur, &y, &height);

        gtk_text_view_buffer_to_window_coords(text_view,
                                              GTK_TEXT_WINDOW_TEXT,
                                              visible_rect.x,
                                              visible_rect.y,
                                              &redraw_rect.x,
                                              &redraw_rect.y);

        redraw_rect.width = visible_rect.width;
        redraw_rect.height = visible_rect.height;

        gtk_text_view_buffer_to_window_coords(text_view,
                                               GTK_TEXT_WINDOW_TEXT,
                                               0,
                                               y,
                                               NULL,
                                               &win_y);

        if(win_y > (redraw_rect.y + redraw_rect.height)
           || (win_y + height) < redraw_rect.y
          )
        {
            /* line not visible */
            continue;
        }
    
        color = gedit_prefs_manager_get_breakpoint_bg_color();

        gdk_gc_set_rgb_fg_color(gc, &color);
        gdk_draw_rectangle(event->window,
                           gc,
                           TRUE,
                           redraw_rect.x + MAX(0, gtk_text_view_get_left_margin(text_view) - 1),
                           win_y,
                           redraw_rect.width,
                           height);
    }

    /* highlight the current line */
    /* FIXME - this clearly duplicates a big chunk of code from above! */
    line = gswat_debugger_get_source_line(debugger) - 1;
    gtk_text_buffer_get_iter_at_line(text_view->buffer,
                                     &cur,
                                     line);

    gtk_text_view_get_line_yrange(text_view, &cur, &y, &height);

    gtk_text_view_buffer_to_window_coords(text_view,
                                          GTK_TEXT_WINDOW_TEXT,
                                          visible_rect.x,
                                          visible_rect.y,
                                          &redraw_rect.x,
                                          &redraw_rect.y);

    redraw_rect.width = visible_rect.width;
    redraw_rect.height = visible_rect.height;

    g_message("current line=%lu, x=%d, y=%d, width=%d, height=%d",
                line,
                redraw_rect.x,
                redraw_rect.y,
                redraw_rect.width,
                redraw_rect.height);

    gtk_text_view_buffer_to_window_coords (text_view,
                                           GTK_TEXT_WINDOW_TEXT,
                                           0,
                                           y,
                                           NULL,
                                           &win_y);

    if(win_y <= (redraw_rect.y + redraw_rect.height)
       && (win_y + height) >= redraw_rect.y
      )
    {
        color = gedit_prefs_manager_get_current_line_bg_color();
        gdk_gc_set_rgb_fg_color(gc, &color);

        gdk_draw_rectangle(event->window,
                           gc,
                           TRUE,
                           redraw_rect.x + MAX(0, gtk_text_view_get_left_margin(text_view) - 1),
                           win_y,
                           redraw_rect.width,
                           height);

    }else{
        g_message("CLIPPED!!");
    }

    g_object_unref(gc);

parent_expose:
    {
        GtkWidgetClass *widget_class;
        widget_class = GTK_WIDGET_CLASS(gswat_view_parent_class);
        return widget_class->expose_event(widget, event);
    }
    //return (* GTK_WIDGET_CLASS (gswat_view_parent_class)->expose_event)(widget, event);

}


