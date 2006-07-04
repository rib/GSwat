#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include <gtk/gtk.h>

#include "global.h"

void
gg_main_on_quit_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gtk_main_quit();
}

