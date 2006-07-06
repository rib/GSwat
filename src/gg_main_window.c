#include <stdio.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gtksourceview/gtksourceview.h>

#include "global.h"
#include "gg_debugger.h"
#include "gdb.h"

static GladeXML *gg_main_window_xml;

static GladeXML *gg_connect_window_xml;

static GladeXML *gg_run_dialog_xml;

static GtkSourceView *source_view;

static GGDebugger* debugger;


void
gg_main_window_init(gchar *glade_file)
{

    /* load the main window */
    gg_main_window_xml = glade_xml_new(glade_file, "gg_main_window", NULL);
    
    /* in case we can't load the interface, bail */
    if(!gg_main_window_xml) {
        g_warning("We could not load the interface!");
        gtk_main_quit();
    } 

    glade_xml_signal_autoconnect(gg_main_window_xml);

    /* Create a GtkSourceView in the main pane */
    source_view = GTK_SOURCE_VIEW(gtk_source_view_new());
     
    debugger = gg_debugger_new();

#if 0
    /* load the connect window */
    gg_connect_window_xml = glade_xml_new(glade_file, "gg_connect_window", NULL);
    
    /* in case we can't load the interface, bail */
    if(!gg_connect_window_xml) {
        g_warning("We could not load the interface!");
        gtk_main_quit();
    } 
    
    glade_xml_signal_autoconnect(gg_connect_window_xml);
#endif

    /* load the connect window */
    gg_run_dialog_xml = glade_xml_new(glade_file, "gg_run_dialog", NULL);
    
    /* in case we can't load the interface, bail */
    if(!gg_run_dialog_xml) {
        g_warning("We could not load the interface!");
        gtk_main_quit();
    } 
    
    glade_xml_signal_autoconnect(gg_run_dialog_xml);

}



void
on_gg_main_connect_button_clicked      (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
    GtkWidget *gg_run_dialog;

    gg_run_dialog = glade_xml_get_widget(
                                    gg_run_dialog_xml,
                                    "gg_run_dialog"
                        );

    gtk_widget_show(gg_run_dialog);

#if 0
    GtkWidget *gg_connect_window;

    gg_connect_window = glade_xml_get_widget(
                                    gg_connect_window_xml,
                                    "gg_connect_window"
                        );
    
    gtk_widget_show(gg_connect_window);
#endif
}

void
on_gg_run_dialog_ok_button_clicked     (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkEntry *command_entry;

    command_entry = GTK_ENTRY(
                        glade_xml_get_widget(
                                gg_run_dialog_xml,
                                "gg_run_dialog_text_entry"
                        )
                    );
}

void
on_gg_connect_connect_button_clicked(GtkButton       *button,
                                     gpointer         user_data)
{

    GtkEntry *command_entry;

    command_entry = GTK_ENTRY(
                        glade_xml_get_widget(
                                gg_connect_window_xml,
                                "gg_connect_program_text"
                        )
                    );
    
    //const gchar *command = gtk_entry_get_text(command_entry);

/*
    g_shell_parse_argv(command,
                       &command_argc,
                       &command_argv,
                       NULL);
*/
    //gg_gdb_connect(command_argc, command_argv);

    gg_gdb_send_command();
}


void
gg_main_on_quit_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gtk_main_quit();
}

