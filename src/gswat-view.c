
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>

#include <gdk/gdkkeysyms.h>

#include <glib/gi18n.h>

#include "gswat-view.h"
#include "gswat-debugger.h"



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
    GtkTextView *text_view;
    GeditView *gedit_view;
    GSwatView *gswat_view;
    GList *tmp = NULL;
    GSwatDebugger *debugger;
    GList *breakpoints;
    //GSwatViewBreakpoint *current_breakpoint;
    GSwatDebuggerBreakpoint *current_breakpoint;
    //GeditDocument *doc;

    GdkRectangle visible_rect;
    GdkRectangle redraw_rect;
    GtkTextIter cur;
    gint y;
    gint height;
    gint win_y;
    gulong line;

    text_view = GTK_TEXT_VIEW(widget);
    gedit_view = GEDIT_VIEW(widget);
    gswat_view = GSWAT_VIEW(widget);

    debugger = gswat_view->priv->debugger;
    if(debugger)
    {
        breakpoints = gswat_debugger_get_breakpoints(debugger);
    }else{
        breakpoints = NULL;
    }


    gchar *current_doc_uri = NULL;

    //doc = GEDIT_DOCUMENT(gtk_text_view_get_buffer(text_view));

    if ((event->window == gtk_text_view_get_window(text_view, GTK_TEXT_WINDOW_TEXT)))
    {
        for(tmp=breakpoints; tmp != NULL; tmp=tmp->next)
        {
            current_breakpoint = (GSwatDebuggerBreakpoint *)tmp->data;
            current_doc_uri = gedit_document_get_uri(gedit_view_get_document(gedit_view));

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

            gtk_text_view_get_visible_rect(text_view, &visible_rect);

            gtk_text_view_buffer_to_window_coords(text_view,
                                                  GTK_TEXT_WINDOW_TEXT,
                                                  visible_rect.x,
                                                  visible_rect.y,
                                                  &redraw_rect.x,
                                                  &redraw_rect.y);

            redraw_rect.width = visible_rect.width;
            redraw_rect.height = visible_rect.height;

            if(y > (redraw_rect.y + redraw_rect.height)
               || (y + height) < redraw_rect.y
              )
            {
                /* line not visible */
                continue;
            }

            gtk_text_view_buffer_to_window_coords (text_view,
                                                   GTK_TEXT_WINDOW_TEXT,
                                                   0,
                                                   y,
                                                   NULL,
                                                   &win_y);

            gdk_draw_rectangle(event->window,
                               widget->style->mid_gc[GTK_WIDGET_STATE (widget)],
                               TRUE,
                               redraw_rect.x + MAX(0, gtk_text_view_get_left_margin(text_view) - 1),
                               win_y,
                               redraw_rect.width,
                               height);
        }

        if(debugger)
        {
            /* highlight the current line */
            /* FIXME - this clearly duplicates a big chunk of code from above! */
            line = gswat_debugger_get_source_line(debugger) - 1;
            g_message("current line=%lu", line);
            gtk_text_buffer_get_iter_at_line(text_view->buffer,
                                             &cur,
                                             line);

            gtk_text_view_get_line_yrange(text_view, &cur, &y, &height);

            gtk_text_view_get_visible_rect(text_view, &visible_rect);

            gtk_text_view_buffer_to_window_coords(text_view,
                                                  GTK_TEXT_WINDOW_TEXT,
                                                  visible_rect.x,
                                                  visible_rect.y,
                                                  &redraw_rect.x,
                                                  &redraw_rect.y);

            redraw_rect.width = visible_rect.width;
            redraw_rect.height = visible_rect.height;

            if(y <= (redraw_rect.y + redraw_rect.height)
               && (y + height) >= redraw_rect.y
              )
            {

                gtk_text_view_buffer_to_window_coords (text_view,
                                                       GTK_TEXT_WINDOW_TEXT,
                                                       0,
                                                       y,
                                                       NULL,
                                                       &win_y);

                gdk_draw_rectangle(event->window,
                                   widget->style->mid_gc[GTK_WIDGET_STATE (widget)],
                                   TRUE,
                                   redraw_rect.x + MAX(0, gtk_text_view_get_left_margin(text_view) - 1),
                                   win_y,
                                   redraw_rect.width,
                                   height);

            }
        }

    }
    
    return (* GTK_WIDGET_CLASS (gswat_view_parent_class)->expose_event)(widget, event);

    //return FALSE;
}


