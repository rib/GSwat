#include <stdio.h>
#include <gtk/gtk.h>
#include <libgnomeui/libgnomeui.h>
#include <glade/glade.h>
#include <gtksourceview/gtksourceview.h>

#include <config.h>

#include "gswat-main-window.h"

#include "gedit-document.h"
#include "gedit-view.h"
#include "gswat-view.h"

#include "gswat-session.h"
#include "gswat-debugger.h"


enum {
  GSWAT_WINDOW_STACK_FUNC_COL,
  GSWAT_WINDOW_STACK_N_COLS
};


struct _GSwatWindowPrivate
{
    GSwatDebugger* debugger;
    GeditDocument *source_document;
    GtkListStore *stack_list_store;

    GList *stack;

    GladeXML *xml;

    GSwatView *source_view;
    //GSwatSession *session;
    //GtkSourceBuffer *source_buffer;
    //GtkSourceView *source_view;

    /* toolbar buttons */
    GtkWidget *step_button;
    GtkWidget *next_button;
    GtkWidget *continue_button;
    GtkWidget *finish_button;
    GtkWidget *break_button;
    GtkWidget *restart_button;
    GtkWidget *add_break_button;
};

typedef struct {
    GSwatDebuggerFrame *dbg_frame;
    GString *display;
}GSwatWindowFrame;


G_DEFINE_TYPE(GSwatWindow, gswat_window, G_TYPE_OBJECT);




static void gswat_window_class_init(GSwatWindowClass *klass);
static void gswat_window_init(GSwatWindow *self);
static void gswat_window_finalize(GObject *object);

static void setup_sourceview(GSwatWindow *self);
static void on_gswat_window_new_button_clicked(GtkToolButton   *toolbutton,
                                               gpointer         data);
static void on_gswat_window_step_button_clicked(GtkToolButton   *toolbutton,
                                               gpointer         data);
static void on_gswat_window_next_button_clicked(GtkToolButton   *toolbutton,
                                               gpointer         data);
static void on_gswat_window_continue_button_clicked(GtkToolButton   *toolbutton,
                                               gpointer         data);
static void on_gswat_window_finish_button_clicked(GtkToolButton   *toolbutton,
                                               gpointer         data);
static void on_gswat_window_break_button_clicked(GtkToolButton   *toolbutton,
                                               gpointer         data);
static void on_gswat_window_run_button_clicked(GtkToolButton   *toolbutton,
                                               gpointer         data);
static void on_gswat_window_add_break_button_clicked(GtkToolButton   *toolbutton,
                                               gpointer         data);
static void on_debug_button_clicked(GtkToolButton   *toolbutton,
                                    gpointer         data);

static void on_gswat_session_edit_done(GSwatSession *session, GSwatWindow *window);
//static void on_gswat_session_edit_abort(GSwatSession *session);

static void gswat_window_set_toolbar_state(GSwatWindow *self, guint state);

static void gswat_window_update_state(GObject *debugger,
                                    GParamSpec *property,
                                    gpointer data);

static void gswat_window_update_stack(GObject *debugger,
                                    GParamSpec *property,
                                    gpointer data);

static void gswat_window_update_source_file(GObject *debugger,
                                          GParamSpec *property,
                                          gpointer data);
static void gswat_window_update_source_line(GObject *debugger,
                                          GParamSpec *property,
                                          gpointer data);

static void gswat_window_source_file_loaded(GeditDocument *document,
                                          const GError  *error,
                                          void *user_data);


static void
gswat_window_class_init(GSwatWindowClass *klass)
{
    GObjectClass     *object_class = G_OBJECT_CLASS (klass);
    
    object_class->finalize = gswat_window_finalize;


    g_type_class_add_private(klass, sizeof(GSwatWindowPrivate));

}



static void 
gswat_window_init(GSwatWindow *self)
{	
    //GladeXML *xml;
    GnomeApp *main_window = NULL;
    GtkWidget *button;
    GtkTreeView *stack_widget;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, GSWAT_TYPE_WINDOW, GSwatWindowPrivate);

    /* load the main window */
    self->priv->xml = glade_xml_new(GSWAT_GLADEDIR "gswat.glade", "gswat_main_window", NULL);
    
    
    /* in case we can't load the interface, bail */
    if(!self->priv->xml)
    {
        g_critical("We could not load the interface!");
    } 

    glade_xml_signal_autoconnect(self->priv->xml);


    button = glade_xml_get_widget(self->priv->xml, "gswat_window_new_button");
    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(on_gswat_window_new_button_clicked),
                     self
                     );

    button = glade_xml_get_widget(self->priv->xml, "gswat_window_step_button");
    self->priv->step_button = button;
    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(on_gswat_window_step_button_clicked),
                     self
                     );

    button = glade_xml_get_widget(self->priv->xml, "gswat_window_next_button");
    self->priv->next_button = button;
    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(on_gswat_window_next_button_clicked),
                     self
                     );

    button = glade_xml_get_widget(self->priv->xml, "gswat_window_continue_button");
    self->priv->continue_button = button;
    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(on_gswat_window_continue_button_clicked),
                     self
                     );

    button = glade_xml_get_widget(self->priv->xml, "gswat_window_finish_button");
    self->priv->finish_button = button;
    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(on_gswat_window_finish_button_clicked),
                     self
                     );

    button = glade_xml_get_widget(self->priv->xml, "gswat_window_break_button");
    self->priv->break_button = button;
    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(on_gswat_window_break_button_clicked),
                     self
                     );

    button = glade_xml_get_widget(self->priv->xml, "gswat_window_run_button");
    self->priv->restart_button = button;
    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(on_gswat_window_run_button_clicked),
                     self
                     );
    button = glade_xml_get_widget(self->priv->xml, "gswat_window_add_break_button");
    self->priv->add_break_button = button;
    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(on_gswat_window_add_break_button_clicked),
                     self
                     );
    button = glade_xml_get_widget(self->priv->xml, "debug_button");
    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(on_debug_button_clicked),
                     self
                     );


    main_window = GNOME_APP(
                    glade_xml_get_widget(self->priv->xml, "gswat_main_window")
                  );

    if(!main_window)
    {
        g_critical("Could not find main window in gswat.glade");
    }

    gtk_widget_show(GTK_WIDGET(main_window));
    
    //XXX
    //setup_sourceview(self);
    /* FIXME
    toplevel_win = gdk_window_get_toplevel(//blah);
    */
    //g_object_set_data(G_OBJECT(main_window), "gedit_notebook", notebook);
    

    gswat_window_set_toolbar_state(self, GSWAT_DEBUGGER_NOT_RUNNING);


    /* Setup the stack view */
    stack_widget = 
        (GtkTreeView *)glade_xml_get_widget(
                            self->priv->xml, 
                            "gswat_window_stack_widget"
                            );
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("",
                                                      renderer,
                                                      "markup", GSWAT_WINDOW_STACK_FUNC_COL,
                                                      NULL);
    gtk_tree_view_append_column(stack_widget, column);



#if 0
    /* load the connect window */
    gswat_connect_window_xml = glade_xml_new(glade_file, "gswat_connect_window", NULL);

    /* in case we can't load the interface, bail */
    if(!gswat_connect_window_xml) {
        g_warning("We could not load the interface!");
        gtk_main_quit();
    } 

    glade_xml_signal_autoconnect(gswat_connect_window_xml);
#endif

    
}


static void
gswat_window_finalize(GObject *object)
{
    GSwatWindow *window;

    window = GSWAT_WINDOW(object);


    G_OBJECT_CLASS(gswat_window_parent_class)->finalize(object);

}



GSwatWindow *
gswat_window_new(GSwatSession *session)
{
    GSwatWindow *self;
    
    self = g_object_new(GSWAT_TYPE_WINDOW, NULL);
    
    /* If we have been passed a session, then
     * we start that immediatly
     */
    if(session)
    {
        g_object_ref(session);
        on_gswat_session_edit_done(session, self);
    }
    
    return self;
}





static void
setup_sourceview(GSwatWindow *self)
{
    GtkWidget *main_scroll;
    //int file;

    //source_buffer = gtk_source_buffer_new(NULL);

    /* Create a GtkSourceView in the main pane */
    //source_view = GTK_SOURCE_VIEW(gtk_source_view_new_with_buffer(source_buffer));

    //gtk_text_buffer_set_text (GTK_TEXT_BUFFER(source_buffer), "Hello, this is some text", -1);

    main_scroll = glade_xml_get_widget(
                                       self->priv->xml,
                                       "gswat_window_scrolled_window"
                                      );

    //gtk_container_add(rTK_CONTAINER(main_scroll), GTK_WIDGET(source_view));
    //gtk_widget_show(GTK_WIDGET(source_view));

    self->priv->source_document = gedit_document_new();
    //source_view = gedit_view_new(source_document);

    //XXX
    self->priv->source_view = GSWAT_VIEW(
                    gswat_view_new(self->priv->source_document, self->priv->debugger)
                  );

    g_signal_connect(self->priv->source_document,
                     "loaded",
                     G_CALLBACK(gswat_window_source_file_loaded),
                     self);

    gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(self->priv->source_view), TRUE);
    gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(self->priv->source_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(self->priv->source_view), FALSE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(self->priv->source_view), FALSE);

    gtk_container_add(GTK_CONTAINER (main_scroll), GTK_WIDGET(self->priv->source_view));


}






static void
on_gswat_window_new_button_clicked(GtkToolButton   *toolbutton,
                                   gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    GSwatSession *gswat_session;


    /* Create a new session */
    gswat_session = gswat_session_new();

    g_signal_connect(gswat_session,
                     "edit-done",
                     G_CALLBACK(on_gswat_session_edit_done),
                     self);
    /*
       g_signal_connect(gswat_session,
       "edit-abort",
       G_CALLBACK(on_gswat_session_edit_done),
       NULL);
       */
    gswat_session_edit(gswat_session);

}




#if 0
static void
on_gswat_session_edit_abort(GSwatSession *session)
{

}
#endif




static void
on_gswat_session_edit_done(GSwatSession *session, GSwatWindow *window)
{
    if(window->priv->debugger)
    {
        g_object_unref(window->priv->debugger);
    }

    window->priv->debugger = gswat_debugger_new(session);
    
    setup_sourceview(window);

    g_signal_connect(G_OBJECT(window->priv->debugger),
                     "notify::state",
                     G_CALLBACK(gswat_window_update_state),
                     window);

    g_signal_connect(G_OBJECT(window->priv->debugger),
                     "notify::stack",
                     G_CALLBACK(gswat_window_update_stack),
                     window);

    g_signal_connect(G_OBJECT(window->priv->debugger),
                     "notify::source-uri",
                     G_CALLBACK(gswat_window_update_source_file),
                     window);

    g_signal_connect(G_OBJECT(window->priv->debugger),
                     "notify::source-line",
                     G_CALLBACK(gswat_window_update_source_line),
                     window);

    gswat_debugger_target_connect(window->priv->debugger);

}


static void
gswat_window_set_toolbar_state(GSwatWindow *self, guint state)
{
    switch(state)
    {
        case GSWAT_DEBUGGER_RUNNING:
            gtk_widget_hide(self->priv->step_button);
            gtk_widget_hide(self->priv->next_button);
            gtk_widget_hide(self->priv->continue_button);
            gtk_widget_hide(self->priv->finish_button);
            gtk_widget_show(self->priv->break_button);
            gtk_widget_show(self->priv->restart_button);
            gtk_widget_hide(self->priv->add_break_button);
            break;
        case GSWAT_DEBUGGER_NOT_RUNNING:
            gtk_widget_hide(self->priv->step_button);
            gtk_widget_hide(self->priv->next_button);
            gtk_widget_hide(self->priv->continue_button);
            gtk_widget_hide(self->priv->finish_button);
            gtk_widget_hide(self->priv->break_button);
            gtk_widget_show(self->priv->add_break_button);
            break;
        case GSWAT_DEBUGGER_INTERRUPTED:
            gtk_widget_show(self->priv->step_button);
            gtk_widget_show(self->priv->next_button);
            gtk_widget_show(self->priv->continue_button);
            gtk_widget_show(self->priv->finish_button);
            gtk_widget_hide(self->priv->break_button);
            gtk_widget_show(self->priv->restart_button);
            gtk_widget_show(self->priv->add_break_button);
            break;
        default:
            g_warning("unexpected debugger state");
            gtk_widget_show(self->priv->step_button);
            gtk_widget_show(self->priv->next_button);
            gtk_widget_show(self->priv->continue_button);
            gtk_widget_show(self->priv->finish_button);
            gtk_widget_show(self->priv->break_button);
            gtk_widget_show(self->priv->restart_button);
            gtk_widget_show(self->priv->add_break_button);
            break;
    }

}



static void
gswat_window_update_state(GObject *object,
                        GParamSpec *property,
                        gpointer data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    GSwatDebugger *debugger = GSWAT_DEBUGGER(object);
    guint state;

    state = gswat_debugger_get_state(debugger);

    gswat_window_set_toolbar_state(self, state);
}



static void
gswat_window_update_stack(GObject *object,
                        GParamSpec *property,
                        gpointer data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    GSwatDebugger *debugger = GSWAT_DEBUGGER(object);
    GList *stack, *tmp, *tmp2;
    GSwatDebuggerFrameArgument *arg;
    GladeXML *xml;
    GtkTreeView *stack_widget=NULL;
    GtkListStore *list_store;
    GtkTreeIter iter;
    GList *disp_stack = NULL;

    stack = gswat_debugger_get_stack(debugger);

    /* TODO look up the stack tree view widget */
    xml = self->priv->xml;
    stack_widget = (GtkTreeView *)glade_xml_get_widget(xml, "gswat_window_stack_widget");


    list_store = gtk_list_store_new(GSWAT_WINDOW_STACK_N_COLS,
                                    G_TYPE_STRING,
                                    G_TYPE_ULONG);

    

    /* create new display strings for the stack */
    for(tmp=stack; tmp!=NULL; tmp=tmp->next)
    {
        GSwatDebuggerFrame *frame;
        GSwatWindowFrame *display_frame = g_new0(GSwatWindowFrame, 1);
        GString *display = g_string_new("");

        frame = (GSwatDebuggerFrame *)tmp->data;

        /* note: at this point the list is in reverse,
         * so the last in the list is our current frame
         */
        if(tmp->prev==NULL)
        {
            display = g_string_append(display, "<b>");
            display = g_string_append(display, frame->function);
            display = g_string_append(display, "</b>");
        }else{
            display = g_string_append(display, frame->function);
        }

        display = g_string_append(display, "(");

        for(tmp2 = frame->arguments; tmp2!=NULL; tmp2=tmp2->next)
        {
            arg = (GSwatDebuggerFrameArgument *)tmp2->data;


            display = g_string_append(display, arg->name);
            display = g_string_append(display, "=<b>");
            display = g_string_append(display, arg->value);
            display = g_string_append(display, "</b>, ");
        }

        if(display->str[display->len-2] == ',')
        {
            display = g_string_truncate(display, display->len-2);
        }

        display = g_string_append(display, ")");

        display_frame->dbg_frame = frame;
        display_frame->display = display;

        disp_stack = g_list_prepend(disp_stack, display_frame);
    }
    disp_stack = g_list_reverse(disp_stack);
    

    /* start displaying the new stack frames */
    for(tmp=disp_stack; tmp!=NULL; tmp=tmp->next)
    {
        GSwatWindowFrame *display_frame = (GSwatWindowFrame *)tmp->data;

        gtk_list_store_append(list_store, &iter);

        gtk_list_store_set(list_store, &iter,
                           GSWAT_WINDOW_STACK_FUNC_COL,
                                display_frame->display->str,
                           -1);

    }
    g_object_unref(self->priv->stack_list_store);
    self->priv->stack_list_store = list_store;
    
    gtk_tree_view_set_model(stack_widget,
                            GTK_TREE_MODEL(self->priv->stack_list_store));

    

    /* free the previously displayed stack frames */
    for(tmp=self->priv->stack; tmp!=NULL; tmp=tmp->next)
    {
        GSwatWindowFrame *display_frame = (GSwatWindowFrame *)tmp->data;

        g_string_free(display_frame->display, TRUE);
        for(tmp2=display_frame->dbg_frame->arguments; tmp2!=NULL; tmp2=tmp2->next)
        {   
            arg = (GSwatDebuggerFrameArgument *)tmp2->data;

            g_free(arg->name);
            g_free(arg->value);
            g_free(arg);
        }
        g_list_free(display_frame->dbg_frame->arguments);

        g_free(display_frame);
    }
    g_list_free(self->priv->stack);
    self->priv->stack=disp_stack;
}




static void
gswat_window_source_file_loaded(GeditDocument *document,
                              const GError  *error,
                              void *user_data)
{

    return;
}



static void
gswat_window_update_source_file(GObject *debugger,
                              GParamSpec *property,
                              gpointer data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gchar * file_uri;
    gint line;

    /* FIXME - we should have a main_window object to hold this */
    //GtkTextIter iter;


    file_uri = gswat_debugger_get_source_uri(GSWAT_DEBUGGER(debugger));

    line = gswat_debugger_get_source_line(GSWAT_DEBUGGER(debugger));

    g_message("gswat_window_update_source_file %s, line=%d", file_uri, line);
    gedit_document_load(self->priv->source_document, file_uri, NULL, line, FALSE);


#if 0
    line = gswat_debugger_get_source_line(GSWAT_DEBUGGER(debugger));

    gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (source_document),
                                      &iter,
                                      2);
    gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (source_document), &iter);
#endif

}



static void
gswat_window_update_source_line(GObject *debugger,
                              GParamSpec *property,
                              gpointer data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gulong line;
    GtkTextIter iter;

    line = gswat_debugger_get_source_line(GSWAT_DEBUGGER(debugger));

    g_message("gswat_window_update_source_line %lu\n", line);

    gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER (self->priv->source_document),
                                     &iter,
                                     (line-1));


    gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (self->priv->source_document), &iter);

#if 0
    if(!gedit_document_goto_line(source_document, line))
    {
        g_message("Failed to update line");
    }
    else
    {
        g_message("Main line update\n");
    }
#endif
    //gedit_document_load(source_document, file_uri, NULL, 0, FALSE);

}

void
on_debug_button_clicked(GtkToolButton   *toolbutton,
                        gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    GtkTextIter iter;

    gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (self->priv->source_document),
                                      &iter,
                                      2);


    gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (self->priv->source_document), &iter);



}




void
on_gswat_window_step_button_clicked(GtkToolButton   *toolbutton,
                                  gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gswat_debugger_step_into(self->priv->debugger);
}


void
on_gswat_window_next_button_clicked(GtkToolButton   *toolbutton,
                                  gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gswat_debugger_next(self->priv->debugger);
}


void
on_gswat_window_break_button_clicked(GtkToolButton   *toolbutton,
                                   gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gswat_debugger_interrupt(self->priv->debugger);
}

void
on_gswat_window_continue_button_clicked  (GtkToolButton   *toolbutton,
                                        gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gswat_debugger_continue(self->priv->debugger);
}


void
on_gswat_window_finish_button_clicked(GtkToolButton   *toolbutton,
                                      gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gswat_debugger_finish(self->priv->debugger);
}


void
on_gswat_window_run_button_clicked(GtkToolButton   *toolbutton,
                                   gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gswat_debugger_restart(self->priv->debugger);
}


void
on_gswat_window_add_break_button_clicked(GtkToolButton   *toolbutton,
                                   gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    GSwatView *source_view = NULL;
    GtkTextBuffer *source_buffer = NULL;
    GtkTextIter cur;
    gint line; 

    
    source_view = self->priv->source_view;
    g_assert(source_view != NULL);

    source_buffer = GTK_TEXT_VIEW(source_view)->buffer;


    gchar *uri = gswat_debugger_get_source_uri(self->priv->debugger);
    

    gtk_text_buffer_get_iter_at_mark(
                            source_buffer, 
							&cur, 
							gtk_text_buffer_get_insert(source_buffer)
                            );
    line = gtk_text_iter_get_line(&cur);

    gswat_debugger_request_line_breakpoint(self->priv->debugger, uri, line);
}



void
gswat_window_on_quit_activate(GtkMenuItem     *menuitem,
                            gpointer         data)
{
    gtk_main_quit();
}








