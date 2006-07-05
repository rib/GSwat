#include <stdio.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "global.h"
#include "gdb.h"

static GladeXML *gg_main_window_xml;

static GladeXML *gg_connect_window_xml;



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



    /* load the connect window */
    gg_connect_window_xml = glade_xml_new(glade_file, "gg_connect_window", NULL);
    
    /* in case we can't load the interface, bail */
    if(!gg_connect_window_xml) {
        g_warning("We could not load the interface!");
        gtk_main_quit();
    } 
    
    glade_xml_signal_autoconnect(gg_connect_window_xml);


}

void
on_gg_main_connect_button_clicked      (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
    GtkWidget *gg_connect_window;

    gg_connect_window = glade_xml_get_widget(
                                    gg_connect_window_xml,
                                    "gg_connect_window"
                        );
    
    gtk_widget_show(gg_connect_window);
     
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
    
    const gchar *command = gtk_entry_get_text(command_entry);

    g_shell_parse_argv(command,
                       &command_argc,
                       &command_argv,
                       NULL);

    //gg_gdb_connect(command_argc, command_argv);

    gg_gdb_send_command();
}


void
gg_main_on_quit_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gtk_main_quit();
}

