#include <stdio.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gtksourceview/gtksourceview.h>

#include "gedit-document.h"
#include "gedit-view.h"

#include "global.h"
#include "gswat-session.h"
#include "gswat-debugger.h"
#include "gdb.h"



static GladeXML *gswat_main_window_xml;


//static GtkSourceBuffer *source_buffer;
//static GtkSourceView *source_view;
static GeditDocument *source_document;
static GtkWidget *source_view;

static GSwatDebugger* debugger;



static void setup_sourceview(void);

static void on_gswat_session_edit_done(GSwatSession *session);
//static void on_gswat_session_edit_abort(GSwatSession *session);

static void gswat_main_set_toolbar_state(guint state);

static void gswat_main_update_state(GObject *debugger,
                                    GParamSpec *property,
                                    gpointer data);

static void gswat_main_update_source_file(GObject *debugger,
                                          GParamSpec *property,
                                          gpointer data);
static void gswat_main_update_source_line(GObject *debugger,
                                          GParamSpec *property,
                                          gpointer data);

static void gswat_main_source_file_loaded(GeditDocument *document,
                                          const GError  *error,
                                          void *user_data);





void
gswat_main_window_init(GSwatSession *session)
{


    /* load the main window */
    gswat_main_window_xml = glade_xml_new(GSWAT_GLADEDIR "gswat.glade", "gswat_main_window", NULL);

    /* in case we can't load the interface, bail */
    if(!gswat_main_window_xml) {
        g_warning("We could not load the interface!");
        gtk_main_quit();
    } 

    glade_xml_signal_autoconnect(gswat_main_window_xml);

    setup_sourceview();

    gswat_main_set_toolbar_state(GSWAT_DEBUGGER_NOT_RUNNING);

    /* If we have been passed a session, then
     * we start that immediatly
     */
    if(session)
    {
        on_gswat_session_edit_done(session);
    }

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
setup_sourceview(void)
{
    GtkWidget *main_scroll;
    //int file;

    //source_buffer = gtk_source_buffer_new(NULL);

    /* Create a GtkSourceView in the main pane */
    //source_view = GTK_SOURCE_VIEW(gtk_source_view_new_with_buffer(source_buffer));

    //gtk_text_buffer_set_text (GTK_TEXT_BUFFER(source_buffer), "Hello, this is some text", -1);

    main_scroll = glade_xml_get_widget(
                                       gswat_main_window_xml,
                                       "gswat_main_scrolled_window"
                                      );

    //gtk_container_add(rTK_CONTAINER(main_scroll), GTK_WIDGET(source_view));
    //gtk_widget_show(GTK_WIDGET(source_view));

    source_document = gedit_document_new();
    source_view = gedit_view_new(source_document);

    g_signal_connect(source_document,
                     "loaded",
                     G_CALLBACK(gswat_main_source_file_loaded),
                     NULL);

    gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(source_view), TRUE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(source_view), FALSE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(source_view), FALSE);

    gtk_container_add(GTK_CONTAINER (main_scroll), source_view);


}







void
on_gswat_main_new_button_clicked(GtkToolButton   *toolbutton,
                                 gpointer         user_data)
{
    GSwatSession *gswat_session;


    /* Create a new session */
    gswat_session = gswat_session_new();

    g_signal_connect(gswat_session,
                     "edit-done",
                     G_CALLBACK(on_gswat_session_edit_done),
                     NULL);
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
on_gswat_session_edit_done(GSwatSession *session)
{
    if(debugger)
    {
        g_object_unref(debugger);
    }

    debugger = gswat_debugger_new(session);

    g_signal_connect(G_OBJECT(debugger),
                     "notify::state",
                     G_CALLBACK(gswat_main_update_state),
                     NULL);

    g_signal_connect(G_OBJECT(debugger),
                     "notify::source-uri",
                     G_CALLBACK(gswat_main_update_source_file),
                     NULL);

    g_signal_connect(G_OBJECT(debugger),
                     "notify::source-line",
                     G_CALLBACK(gswat_main_update_source_line),
                     NULL);

    gswat_debugger_target_connect(debugger);

}


static void
gswat_main_set_toolbar_state(guint state)
{

    GladeXML *xml;
    GtkWidget *step_button;
    GtkWidget *next_button;
    GtkWidget *continue_button;
    GtkWidget *finish_button;
    GtkWidget *break_button;
    GtkWidget *restart_button;


    xml = gswat_main_window_xml;

    step_button = glade_xml_get_widget(xml, "gswat_main_step_button");
    next_button = glade_xml_get_widget(xml, "gswat_main_next_button");
    continue_button = glade_xml_get_widget(xml, "gswat_main_continue_button");
    finish_button = glade_xml_get_widget(xml, "gswat_main_finish_button");
    break_button = glade_xml_get_widget(xml, "gswat_main_break_button");
    restart_button = glade_xml_get_widget(xml, "gswat_main_run_button");


    switch(state)
    {
        case GSWAT_DEBUGGER_RUNNING:
            gtk_widget_hide(step_button);
            gtk_widget_hide(next_button);
            gtk_widget_hide(continue_button);
            gtk_widget_hide(finish_button);
            gtk_widget_show(break_button);
            gtk_widget_show(restart_button);
            break;
        case GSWAT_DEBUGGER_NOT_RUNNING:
            gtk_widget_hide(step_button);
            gtk_widget_hide(next_button);
            gtk_widget_hide(continue_button);
            gtk_widget_hide(finish_button);
            gtk_widget_hide(break_button);
            break;
        case GSWAT_DEBUGGER_INTERRUPTED:
            gtk_widget_show(step_button);
            gtk_widget_show(next_button);
            gtk_widget_show(continue_button);
            gtk_widget_show(finish_button);
            gtk_widget_hide(break_button);
            gtk_widget_show(restart_button);
            break;
        default:
            g_warning("unexpected debugger state");
            gtk_widget_show(step_button);
            gtk_widget_show(next_button);
            gtk_widget_show(continue_button);
            gtk_widget_show(finish_button);
            gtk_widget_show(break_button);
            gtk_widget_show(restart_button);
            break;
    }

}



static void
gswat_main_update_state(GObject *object,
                        GParamSpec *property,
                        gpointer data)
{
    GSwatDebugger *debugger = GSWAT_DEBUGGER(object);
    guint state;

    state = gswat_debugger_get_state(debugger);

    gswat_main_set_toolbar_state(state);
}

static void
gswat_main_source_file_loaded(GeditDocument *document,
                              const GError  *error,
                              void *user_data)
{

    return;
}


static void
gswat_main_update_source_file(GObject *debugger,
                              GParamSpec *property,
                              gpointer data)
{
    gchar * file_uri;
    gint line;

    /* FIXME - we should have a main_window object to hold this */
    //GtkTextIter iter;

    
    file_uri = gswat_debugger_get_source_uri(GSWAT_DEBUGGER(debugger));

    line = gswat_debugger_get_source_line(GSWAT_DEBUGGER(debugger));

    g_message("gswat_main_update_source_file %s, line=%d", file_uri, line);
    gedit_document_load(source_document, file_uri, NULL, line, False);


#if 0
    line = gswat_debugger_get_source_line(GSWAT_DEBUGGER(debugger));

    gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (source_document),
                                      &iter,
                                      2);
    gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (source_document), &iter);
#endif

}



static void
gswat_main_update_source_line(GObject *debugger,
                              GParamSpec *property,
                              gpointer data)
{
    gulong line;
    GtkTextIter iter;

    line = gswat_debugger_get_source_line(GSWAT_DEBUGGER(debugger));

    g_message("gswat_main_update_source_line %lu\n", line);

    gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER (source_document),
                                     &iter,
                                     (line-1));


    gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (source_document), &iter);

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
    //gedit_document_load(source_document, file_uri, NULL, 0, False);

}

void
on_debug_button_clicked(GtkToolButton   *toolbutton,
                        gpointer         user_data)
{
    GtkTextIter iter;

    gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (source_document),
                                      &iter,
                                      2);


    gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (source_document), &iter);



}




void
on_gswat_main_step_button_clicked(GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
    gswat_debugger_step_into(debugger);
}


void
on_gswat_main_next_button_clicked(GtkToolButton   *toolbutton,
                                  gpointer         user_data)
{
    gswat_debugger_next(debugger);
}


void
on_gswat_main_break_button_clicked(GtkToolButton   *toolbutton,
                                   gpointer         user_data)
{
    gswat_debugger_interrupt(debugger);
}

void
on_gswat_main_continue_button_clicked  (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
    gswat_debugger_continue(debugger);
}


void
on_gswat_main_finish_button_clicked    (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
    gswat_debugger_finish(debugger);
}


void
on_gswat_main_run_button_clicked     (GtkToolButton   *toolbutton,
                                      gpointer         user_data)
{
    gswat_debugger_restart(debugger);
}



void
gswat_main_on_quit_activate(GtkMenuItem     *menuitem,
                            gpointer         user_data)
{
    gtk_main_quit();
}








