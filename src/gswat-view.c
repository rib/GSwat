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


#define GSWAT_VIEW_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GSWAT_TYPE_VIEW, GSwatViewPrivate))



struct _GSwatViewPrivate
{
    GList *line_highlights;
};


typedef struct {
    gint line;
}GSwatViewBreakpoint;

static void	gswat_view_destroy(GtkObject       *object);
static void	gswat_view_finalize(GObject         *object);
static gint gswat_view_focus_out(GtkWidget       *widget,
                                 GdkEventFocus   *event);
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
gswat_view_init(GSwatView *self)
{	

    self->priv = GSWAT_VIEW_GET_PRIVATE(self);

    //g_signal_connect(view, "expose-event", G_CALLBACK(test_expose), NULL);

}

static void
gswat_view_destroy (GtkObject *object)
{
    GSwatView *self;

    self = GSWAT_VIEW (object);


    (* GTK_OBJECT_CLASS (gswat_view_parent_class)->destroy) (object);
}

static void
gswat_view_finalize (GObject *object)
{
    GSwatView *self;

    self = GSWAT_VIEW (object);
    
    gtk_widget_hide_all(GTK_WIDGET(self));

    (* G_OBJECT_CLASS (gswat_view_parent_class)->finalize) (object);
}

static gint
gswat_view_focus_out (GtkWidget *widget, GdkEventFocus *event)
{
    //GSwatView *self = GSWAT_VIEW (widget);

    gtk_widget_queue_draw (widget);

    (* GTK_WIDGET_CLASS (gswat_view_parent_class)->focus_out_event) (widget, event);

    return FALSE;
}


GtkWidget *
gswat_view_new(GeditDocument *doc)
{
    GSwatView *self;
    
    g_return_val_if_fail(GEDIT_IS_DOCUMENT (doc), NULL);
    
    self = g_object_new(GSWAT_TYPE_VIEW, NULL);
    
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(self),
                             GTK_TEXT_BUFFER(doc));
    
    gtk_text_view_set_editable(GTK_TEXT_VIEW(self), 
                               FALSE);					  
    
    gtk_widget_show_all(GTK_WIDGET(self));
    
    return GTK_WIDGET(self);
}


void
gswat_view_set_line_highlights(GSwatView *self, const GList *line_highlights)
{
    GList *tmp, *copy;
    GdkWindow *window;
    
    /* why doesn't g_list_copy take (const GList *) input? */
    copy=g_list_copy((GList *)line_highlights);

    /* Do a deep copy of the list */
    for(tmp=copy; tmp!=NULL; tmp=tmp->next)
    {
        GSwatViewLineHighlight *current_highlight;
        GSwatViewLineHighlight *new_line_highlight;

        current_highlight=(GSwatViewLineHighlight *)tmp->data;

        new_line_highlight = g_new(GSwatViewLineHighlight, 1);
        memcpy(new_line_highlight, current_highlight, sizeof(GSwatViewLineHighlight));
        tmp->data=new_line_highlight;
    }
    self->priv->line_highlights = copy;
    
    /* trigger an expose to draw any new highlighted lines */
    window = gtk_text_view_get_window(GTK_TEXT_VIEW(self),
                                        GTK_TEXT_WINDOW_TEXT);
    gdk_window_invalidate_rect(window,
                               NULL,
                               FALSE);
}


GeditDocument *
gswat_view_get_document(GSwatView *self)
{
    return  GEDIT_DOCUMENT(
                gtk_text_view_get_buffer(GTK_TEXT_VIEW (self))
                );
}


static gboolean
gswat_view_expose(GtkWidget *widget,
                  GdkEventExpose *event)
{
    GSwatView *self = GSWAT_VIEW(widget);
    GtkTextView *text_view = GTK_TEXT_VIEW(widget);
    GList *tmp = NULL;
    GdkRectangle visible_rect;
    GdkRectangle redraw_rect;
    GtkTextIter cur;
    gint y;
    gint height;
    gint win_y;
    GdkGC *gc;
    
    if ((event->window != gtk_text_view_get_window(text_view, GTK_TEXT_WINDOW_TEXT)))
    {
        goto parent_expose;
    }

    gtk_text_view_get_visible_rect(text_view, &visible_rect);
    
    gc = gdk_gc_new(GDK_DRAWABLE(event->window));
    
    for(tmp=self->priv->line_highlights; tmp!=NULL; tmp=tmp->next)
    {
        GSwatViewLineHighlight *current_line_highlight;
        current_line_highlight = (GSwatViewLineHighlight *)tmp->data;
        
        gtk_text_buffer_get_iter_at_line(text_view->buffer,
                                         &cur,
                                         current_line_highlight->line - 1);

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
    
        gdk_gc_set_rgb_fg_color(gc, &(current_line_highlight->color));
        gdk_draw_rectangle(event->window,
                           gc,
                           TRUE,
                           redraw_rect.x + MAX(0, gtk_text_view_get_left_margin(text_view) - 1),
                           win_y,
                           redraw_rect.width,
                           height);
       
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


