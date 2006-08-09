/* This file is part of gnome gdb
 *
 * AUTHORS
 *     Sven Herzberg  <herzi@gnome-de.org>
 *
 * Copyright (C) 2006  Sven Herzberg <herzi@gnome-de.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glade/glade.h>

#include "gswat-session.h"


struct GSwatSessionPrivate {

    guint   type;

    gchar   *name;
    gchar   *target;

    GPid  pid;
    
    gchar   *command;
    gint    argc;
    gchar   **argv;

    gchar *working_dir;

    GList *environment;


    /* For the session editor */
    GladeXML *dialog_xml;

};



enum {
	PROP_0,
	PROP_NAME,
	PROP_TARGET,
    PROP_COMMAND,
	PROP_PID,
    PROP_WORKING_DIR
};

enum {
    SIG_EDIT_DONE,
    SIG_EDIT_ABORT,
	LAST_SIGNAL
};

static guint session_signals[LAST_SIGNAL] = { 0 };

/* GType */
G_DEFINE_TYPE(GSwatSession, gswat_session, G_TYPE_OBJECT);


static void gswat_session_get_property(GObject* object,
                                       guint id,
                                       GValue* value,
                                       GParamSpec* pspec);
static void gswat_session_set_property(GObject *object,
                                       guint id,
                                       const GValue *value,
                                       GParamSpec *pspec);
static void gswat_session_finalize(GObject* object);



static void on_gswat_session_ok_button_clicked(GtkButton *button, gpointer user_data);
static void on_gswat_session_cancel_button_clicked(GtkButton *button, gpointer user_data);
static gboolean on_gswat_new_session_dialog_delete_event(GtkWidget *widget,
                                                  GdkEvent *event,
                                                  gpointer user_data);



GSwatSession*
gswat_session_new(void)
{
    return g_object_new(GSWAT_TYPE_SESSION, NULL);
}


static void
gswat_session_class_init(GSwatSessionClass* self_class)
{
    GObjectClass* go_class = G_OBJECT_CLASS(self_class);

    go_class->finalize     = gswat_session_finalize;
    go_class->get_property = gswat_session_get_property;
    go_class->set_property = gswat_session_set_property;

    g_object_class_install_property(go_class,
                                    PROP_NAME,
                                    g_param_spec_string("name",
                                                        _("Session Name"),
                                                        _("The name of the debugging session"),
                                                        "Debug0",
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE));

    g_object_class_install_property(go_class,
                                    PROP_TARGET,
                                    g_param_spec_string("target-type",
                                                        _("Target Type"),
                                                        _("The type of target you want to debug"),
                                                        "Local Command",
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE));

    /* Details for local commands */

    g_object_class_install_property(go_class,
                                    PROP_COMMAND,
                                    g_param_spec_string("command",
                                                        _("Command"),
                                                        _("The command to run"),
                                                        "./a.out",
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE));

    /*  */
    g_object_class_install_property(go_class,
                                    PROP_PID,
                                    g_param_spec_int("pid",
                                                     _("Process ID"),
                                                     _("The ID of the process to debug"),
                                                     G_MININT,
                                                     G_MAXINT,
                                                     -1,
                                                     G_PARAM_READABLE | G_PARAM_WRITABLE));
    
    g_object_class_install_property(go_class,
                                    PROP_WORKING_DIR,
                                    g_param_spec_string("working-dir",
                                                        _("Working Dir"),
                                                        _("The working directory for the program being debugged"),
                                                        NULL,
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE));

    

    session_signals[SIG_EDIT_DONE] =
   		g_signal_new ("edit-done",
			      G_OBJECT_CLASS_TYPE(go_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET(GSwatSessionClass, edit_done),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
    
    session_signals[SIG_EDIT_ABORT] =
   		g_signal_new ("edit-abort",
			      G_OBJECT_CLASS_TYPE(go_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET(GSwatSessionClass, edit_abort),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
    
    
    g_type_class_add_private(self_class, sizeof(struct GSwatSessionPrivate));
}



static void
gswat_session_init(GSwatSession* self)
{

    self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, GSWAT_TYPE_SESSION, struct GSwatSessionPrivate);

    self->priv->type = GSWAT_SESSION_TYPE_LOCAL_RUN;

    self->priv->pid = -1;

    self->priv->working_dir = g_get_current_dir();

}



static void
gswat_session_finalize(GObject* object)
{
    GSwatSession* self = GSWAT_SESSION(object);
    
    if(self->priv->name){
        g_free(self->priv->name);
    }

    if(self->priv->target){
        g_free(self->priv->target);
    }

    if(self->priv->command){
        g_free(self->priv->command);
    }

    if(self->priv->argv){
        g_strfreev(self->priv->argv);
    }

    if(self->priv->working_dir){
        g_free(self->priv->working_dir);
    }
    
    /* TODO - free environment */

    G_OBJECT_CLASS(gswat_session_parent_class)->finalize(object);
}




static void
gswat_session_set_property(GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
    GSwatSession *session = GSWAT_SESSION(object);
    const gchar *command, *working_dir;

    switch (prop_id)
    {
        case PROP_NAME:
            session->priv->name = g_value_dup_string(value);
            break;
        case PROP_TARGET:
            session->priv->target = g_value_dup_string(value);
            break;
        case PROP_COMMAND:
            command = g_value_get_string(value);
            gswat_session_set_command(GSWAT_SESSION(object),
                                      command
                                      );

            break;
        case PROP_PID:
            session->priv->pid = g_value_get_int(value);
            break;
        case PROP_WORKING_DIR:
            working_dir = g_value_get_string(value);
            gswat_session_set_working_dir(GSWAT_SESSION(object),
                                          working_dir
                                          );
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}



static void
gswat_session_get_property(GObject* object, guint id, GValue* value, GParamSpec* pspec)
{
    GSwatSession* self = GSWAT_SESSION(object);

    switch(id) 
    {
        case PROP_NAME:
            g_value_set_string(value, self->priv->name);
            break;
        case PROP_TARGET:
            g_value_set_string(value, self->priv->target);
            break;
        case PROP_COMMAND:
            g_value_set_string(value, self->priv->command);
            break;
        case PROP_PID:
            g_value_set_int(value, self->priv->pid);
            break;
        case PROP_WORKING_DIR:
            g_value_set_string(value, self->priv->working_dir);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
            break;
    }
}




gchar *
gswat_session_get_command(GSwatSession* self, gint *argc, gchar ***argv)
{
    if(argc != NULL)
    {
        *argc = self->priv->argc;
    }

    if(argv != NULL)
    {
        *argv = g_strdupv(self->priv->argv);
    }
    
    return g_strdup(self->priv->command);
}




void
gswat_session_set_command(GSwatSession* self, const gchar *command)
{
    GError *error=NULL;

    g_assert(command != NULL);

    if(self->priv->command == NULL ||
       strcmp(command, self->priv->command) != 0)
    {
        self->priv->command = g_strdup(command);
        
        g_strfreev(self->priv->argv);

        if(!g_shell_parse_argv(command,
                               &(self->priv->argc),
                               &(self->priv->argv),
                               &error))
        {
            g_message("Failed to parse command: %s\n", error->message);
            g_error_free(error);
        }

        g_object_notify(G_OBJECT(self), "command");
    }
}




gint
gswat_session_get_pid(GSwatSession* self)
{
    return self->priv->pid;
}



void
gswat_session_set_pid(GSwatSession* self, gint pid)
{
    if(pid != self->priv->pid)
    {
        self->priv->pid = pid;
        g_object_notify(G_OBJECT(self), "pid");
    }
}



gchar *
gswat_session_get_working_dir(GSwatSession* self)
{
    return g_strdup(self->priv->working_dir);
}


void
gswat_session_set_working_dir(GSwatSession* self, const gchar *working_dir)
{
    g_assert(working_dir != NULL);

    if(self->priv->working_dir == NULL ||
       strcmp(working_dir, self->priv->working_dir) != 0)
    {
        self->priv->working_dir = g_strdup(working_dir);

        g_object_notify(G_OBJECT(self), "working-dir");
    }
}


void
gswat_session_edit(GSwatSession *session)
{
    GladeXML  *xml;
    GtkWidget *dialog;
    GtkWidget *session_ok_button;
    GtkWidget *session_cancel_button;

    /* load the session dialog */
    xml = glade_xml_new(GSWAT_GLADEDIR "gswat.glade",
                        "gswat_new_session_dialog",
                        NULL);

    /* in case we can't load the interface, bail */
    if(!xml) {
        g_warning("We could not load the interface!");
        gtk_main_quit();
    }
    session->priv->dialog_xml = xml;


    glade_xml_signal_autoconnect(xml);


    //GtkWidget *session_dialog;
    //GtkWidget *session_name_combo;
    //GtkWidget *session_target_combo;
    //GtkWidget *session_command_entry;
    //GtkWidget *session_hostname_entry;
    //GtkWidget *session_port_entry;

    dialog = glade_xml_get_widget(xml, "gswat_new_session_dialog");
    //session->priv->session_name_combo = glade_xml_get_widget(xml, "gswat_session_name_combo");
    //session->priv->session_target_combo = glade_xml_get_widget(xml, "gswat_session_name_combo");
    //session->priv->session_command_entry = glade_xml_get_widget(xml, "gswat_session_command_entry");

    session_ok_button = glade_xml_get_widget(xml, "gswat_session_ok_button");
    session_cancel_button = glade_xml_get_widget(xml, "gswat_session_cancel_button");


    g_signal_connect(session_ok_button,
                     "clicked",
                     G_CALLBACK(on_gswat_session_ok_button_clicked),
                     session);

    g_signal_connect(session_cancel_button,
                     "clicked",
                     G_CALLBACK(on_gswat_session_cancel_button_clicked),
                     session);
    
    g_signal_connect(dialog,
                     "delete-event",
                     G_CALLBACK(on_gswat_new_session_dialog_delete_event),
                     session);
    
    //gtk_combo_box_set_active(GTK_COMO_BOX(session_target_combo), 0);

    /* look at the default session name */
    /*
       gchar *session_name = g_object_get(session,
       "name",
       &session_name);
       */

    /* make the first name entry active */
    //gtk_combo_box_set_active(GTK_COMO_BOX(session_name_combo), 0);


    gtk_widget_show(dialog);

    //gswat_new_session_dialog = session_dialog;
    //gswat_new_session_xml = xml;

}

#if 0
void
gswat_save_session(GSwatSession* self)
{
    /* read in the session file */

    /* remove any session with the same name */

    /* Add the new session */

    /* write back the session file */
}
#endif




/* 
 * The Session Edit Dialog
 *
 */


static void
on_gswat_session_ok_button_clicked(GtkButton       *button,
                                   gpointer         user_data)
{
    GSwatSession *session = GSWAT_SESSION(user_data);

    GladeXML *xml;
    GtkWidget *dialog;

    GtkWidget *session_name_combo;
    GtkWidget *session_target_combo;
    GtkWidget *session_command_entry;
    //GtkWidget *session_hostname_entry;
    //GtkWidget *session_port_entry;


    xml = session->priv->dialog_xml;

    dialog = glade_xml_get_widget(xml, "gswat_new_session_dialog");
    gtk_widget_hide(dialog);

    session_name_combo = glade_xml_get_widget(xml, "gswat_session_name_combo");
    session_target_combo = glade_xml_get_widget(xml, "gswat_session_name_combo");
    session_command_entry = glade_xml_get_widget(xml, "gswat_session_command_entry");



    const gchar *command = gtk_entry_get_text(GTK_ENTRY(session_command_entry));
    g_object_set(session,
                 "command", command,
                 NULL);


#if 0
    g_object_set(session,
                 "command", "default",
                 NULL);
#endif


    gtk_widget_destroy(dialog);

    g_object_unref(xml);
    session->priv->dialog_xml = NULL;
    
    g_signal_emit_by_name(session, "edit-done");
}




static void
gswat_session_abort_edit(GSwatSession *session)
{
    GladeXML *xml;
    GtkWidget *dialog;
    
    xml = session->priv->dialog_xml;

    dialog = glade_xml_get_widget(xml, "gswat_new_session_dialog");
    gtk_widget_hide(dialog);
    

    gtk_widget_destroy(dialog);

    g_object_unref(xml);
    session->priv->dialog_xml = NULL;

    g_signal_emit_by_name(session, "edit-abort");
}


static void
on_gswat_session_cancel_button_clicked(GtkButton       *button,
                                       gpointer         user_data)
{
    GSwatSession *session = GSWAT_SESSION(user_data);

    gswat_session_abort_edit(session);
}


static gboolean
on_gswat_new_session_dialog_delete_event(GtkWidget       *widget,
                                         GdkEvent        *event,
                                         gpointer         user_data)
{
    GSwatSession *session = GSWAT_SESSION(user_data);

    gswat_session_abort_edit(session);

    return FALSE;
}

