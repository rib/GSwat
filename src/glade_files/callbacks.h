#include <gnome.h>


void
gg_main_on_quit_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_gg_connect_program_browse_button_activate
                                        (GtkButton       *button,
                                        gpointer         user_data);

void
on_gg_connect_wkdir_browse_button_activate
                                        (GtkButton       *button,
                                        gpointer         user_data);

void
on_gg_connect_connect_button_activate  (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbutton3_clicked                 (GtkToolButton   *toolbutton,
                                        gpointer         user_data);

void
on_gg_main_connect_button_clicked      (GtkToolButton   *toolbutton,
                                        gpointer         user_data);

void
on_gg_connect_connect_button_activate  (GtkButton       *button,
                                        gpointer         user_data);

void
on_gg_connect_connect_button_clicked   (GtkButton       *button,
                                        gpointer         user_data);
