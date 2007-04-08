/*
 * <preamble>
 * gswat - A graphical program debugger for Gnome
 * Copyright (C) 2006  Robert Bragg
 * </preamble>
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
 */

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glade/glade.h>
#include <libxml/xmlreader.h>
#include <libgnome/gnome-util.h>

#include "gswat-session.h"


/* Macros and defines */
#define GSWAT_SESSION_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GSWAT_TYPE_SESSION, GSwatSessionPrivate))

#define SESSIONS_FILE "gswat-sessions.xml"

/* Enums/Typedefs */
enum {
    SIG_EDIT_DONE,
    SIG_EDIT_ABORT,
    LAST_SIGNAL
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
    SESSION_NAME_COMBO_STRING,
    SESSION_NAME_COMBO_N_COLUMNS
};


typedef struct {
    GladeXML *dialog_xml;

    //GtkListStore *name_list_store;
    GtkWidget *name_combo;
    GtkWidget *command_entry;
    GtkWidget *workdir_entry;
    GtkWidget *serial_combo;
}GSwatSessionEditor;

typedef struct {
    gchar   *name;
    gchar   *target;
    GPid    pid;
    gchar   *command;
    gint    argc;
    gchar   **argv;
    gchar   *working_dir;
    GList   *environment;
}GSwatSessionState;

struct _GSwatSessionPrivate
{
    guint   type;
    gchar   *name;
    gchar   *target;
    GPid    pid;
    gchar   *command;
    gint    argc;
    gchar   **argv;
    gchar   *working_dir;
    GList   *environment;
    GSwatSessionEditor *editor;
};

/* Function definitions */
static void gswat_session_class_init(GSwatSessionClass *klass);
static void gswat_session_get_property(GObject *object,
                                   guint id,
                                   GValue *value,
                                   GParamSpec *pspec);
static void gswat_session_set_property(GObject *object,
                                   guint property_id,
                                   const GValue *value,
                                   GParamSpec *pspec);
//static void gswat_session_mydoable_interface_init(gpointer interface,
//                                             gpointer data);
static void gswat_session_init(GSwatSession *self);
static void gswat_session_finalize(GObject *self);

static void set_session_dialog_defaults(GSwatSession *session, gchar *name);
static GSwatSessionState *find_session_save_state(gchar *session_name);
static void free_session_state_members(GSwatSessionState *session_state);
static GList *read_sessions(void);
static GSwatSessionState *parse_session(xmlDocPtr doc, xmlNodePtr cur);
static void free_sessions(void);
static void on_gswat_new_session_name_combo_change(GtkComboBoxEntry *name_combo,
                                                   GSwatSession *session);
static void on_gswat_session_ok_button_clicked(GtkButton *button,
                                               gpointer user_data);
static void save_sessions(void);
static void save_session(xmlNodePtr parent,
                         GSwatSessionState *session_state);
static void on_gswat_session_cancel_button_clicked(GtkButton *button,
                                       gpointer user_data);
static gboolean on_gswat_new_session_dialog_delete_event(GtkWidget *widget,
                                         GdkEvent *event,
                                         gpointer user_data);
static void gswat_session_abort_edit(GSwatSession *session);



/* Variables */
static GObjectClass *parent_class = NULL;
static guint gswat_session_signals[LAST_SIGNAL] = { 0 };

/* TODO - we need a lock for this */
static GList *sessions = NULL;


GType
gswat_session_get_type(void) /* Typechecking */
{
    static GType self_type = 0;

    if (!self_type)
    {
        static const GTypeInfo object_info =
        {
            sizeof(GSwatSessionClass), /* class structure size */
            NULL, /* base class initializer */
            NULL, /* base class finalizer */
            (GClassInitFunc)gswat_session_class_init, /* class initializer */
            NULL, /* class finalizer */
            NULL, /* class data */
            sizeof(GSwatSession), /* instance structure size */
            0, /* preallocated instances */
            (GInstanceInitFunc)gswat_session_init, /* instance initializer */
            NULL /* function table */
        };

        /* add the type of your parent class here */
        self_type = g_type_register_static(G_TYPE_OBJECT, /* parent GType */
                                           "GSwatSession", /* type name */
                                           &object_info, /* type info */
                                           0 /* flags */
                                          );
#if 0
        /* add interfaces here */
        static const GInterfaceInfo mydoable_info =
        {
            (GInterfaceInitFunc)
                gswat_session_mydoable_interface_init,
            (GInterfaceFinalizeFunc)NULL,
            NULL /* interface data */
        };

        if(self_type != G_TYPE_INVALID) {
            g_type_add_interface_static(self_type,
                                        MY_TYPE_MYDOABLE,
                                        &mydoable_info);
        }
#endif
    }

    return self_type;
}

static void
gswat_session_class_init(GSwatSessionClass *klass) /* Class Initialization */
{   
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GParamSpec *new_param;

    parent_class = g_type_class_peek_parent(klass);

    gobject_class->finalize = gswat_session_finalize;

    gobject_class->get_property = gswat_session_get_property;
    gobject_class->set_property = gswat_session_set_property;

    /* set up properties */
#if 0
    //new_param = g_param_spec_int("name", /* name */
    //new_param = g_param_spec_uint("name", /* name */
    //new_param = g_param_spec_boolean("name", /* name */
    //new_param = g_param_spec_object("name", /* name */
    new_param = g_param_spec_pointer("name", /* name */
                                     "Name", /* nick name */
                                     "Name", /* description */
#if INT/UINT/CHAR/LONG/FLOAT...
                                     10, /* minimum */
                                     100, /* maximum */
                                     0, /* default */
#elif BOOLEAN
                                     FALSE, /* default */
#elif STRING
                                     NULL, /* default */
#elif OBJECT
                                     MY_TYPE_PARAM_OBJ, /* GType */
#elif POINTER
                                     /* nothing extra */
#endif
                                     MY_PARAM_READABLE /* flags */
                                     MY_PARAM_WRITEABLE /* flags */
                                     MY_PARAM_READWRITE /* flags */
                                     | G_PARAM_CONSTRUCT
                                     | G_PARAM_CONSTRUCT_ONLY
                                     );
    g_object_class_install_property(gobject_class,
                                    PROP_NAME,
                                    new_param);
#endif

    new_param = g_param_spec_string("name",
                                    _("Session Name"),
                                    _("The name of the debugging session"),
                                    "Debug0",
                                    G_PARAM_READABLE | G_PARAM_WRITABLE);
    g_object_class_install_property(gobject_class,
                                    PROP_NAME,
                                    new_param);

    new_param = g_param_spec_string("target-type",
                                    _("Target Type"),
                                    _("The type of target you want to debug"),
                                    "Local Command",
                                    G_PARAM_READABLE | G_PARAM_WRITABLE);
    g_object_class_install_property(gobject_class,
                                    PROP_TARGET,
                                    new_param);

    /* Details for local commands */
    new_param = g_param_spec_string("command",
                                    _("Command"),
                                    _("The command to run"),
                                    "./a.out",
                                    G_PARAM_READABLE | G_PARAM_WRITABLE);
    g_object_class_install_property(gobject_class,
                                    PROP_COMMAND,
                                    new_param);

    /*  */
    new_param = g_param_spec_int("pid",
                                 _("Process ID"),
                                 _("The ID of the process to debug"),
                                 G_MININT,
                                 G_MAXINT,
                                 -1,
                                 G_PARAM_READABLE | G_PARAM_WRITABLE);
    g_object_class_install_property(gobject_class,
                                    PROP_PID,
                                    new_param);

    new_param = g_param_spec_string("working-dir",
                                    _("Working Dir"),
                                    _("The working directory for the program being debugged"),
                                    NULL,
                                    G_PARAM_READABLE | G_PARAM_WRITABLE);
    g_object_class_install_property(gobject_class,
                                    PROP_WORKING_DIR,
                                    new_param);


    /* set up signals */
#if 0 /* template code */
    klass->signal_member = signal_default_handler;
    gswat_session_signals[SIGNAL_NAME] =
        g_signal_new("signal_name", /* name */
                     G_TYPE_FROM_CLASS(klass), /* interface GType */
                     G_SIGNAL_RUN_LAST, /* signal flags */
                     G_STRUCT_OFFSET(GSwatSessionClass, signal_member),
                     NULL, /* accumulator */
                     NULL, /* accumulator data */
                     g_cclosure_marshal_VOID__VOID, /* c marshaller */
                     G_TYPE_NONE, /* return type */
                     0 /* number of parameters */
                     /* vararg, list of param types */
                    );
#endif

    klass->edit_done=NULL;
    klass->edit_abort=NULL;

    gswat_session_signals[SIG_EDIT_DONE] =
        g_signal_new("edit-done", /* name */
                     G_TYPE_FROM_CLASS(klass), /* interface GType */
                     G_SIGNAL_RUN_LAST, /* signal flags */
                     G_STRUCT_OFFSET(GSwatSessionClass, edit_done),
                     NULL, /* accumulator */
                     NULL, /* accumulator data */
                     g_cclosure_marshal_VOID__VOID, /* c marshaller */
                     G_TYPE_NONE, /* return type */
                     0 /* number of parameters */
                     /* vararg, list of param types */
                    );

    gswat_session_signals[SIG_EDIT_ABORT] =
        g_signal_new("edit-abort", /* name */
                     G_TYPE_FROM_CLASS(klass), /* interface GType */
                     G_SIGNAL_RUN_LAST, /* signal flags */
                     G_STRUCT_OFFSET(GSwatSessionClass, edit_abort),
                     NULL, /* accumulator */
                     NULL, /* accumulator data */
                     g_cclosure_marshal_VOID__VOID, /* c marshaller */
                     G_TYPE_NONE, /* return type */
                     0 /* number of parameters */
                     /* vararg, list of param types */
                    );

    g_type_class_add_private(klass, sizeof(GSwatSessionPrivate));
}

static void
gswat_session_get_property(GObject *object,
                           guint id,
                           GValue *value,
                           GParamSpec *pspec)
{
    GSwatSession* self = GSWAT_SESSION(object);
    char *str = NULL;

    switch(id) {
#if 0 /* template code */
        case PROP_NAME:
            g_value_set_int(value, self->priv->property);
            g_value_set_uint(value, self->priv->property);
            g_value_set_boolean(value, self->priv->property);
            /* don't forget that this will dup the string for you: */
            g_value_set_string(value, self->priv->property);
            g_value_set_object(value, self->priv->property);
            g_value_set_pointer(value, self->priv->property);
            break;
#endif
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

    if(str)
    {
        g_free(str);
    }
}

static void
gswat_session_set_property(GObject *object,
                           guint property_id,
                           const GValue *value,
                           GParamSpec *pspec)
{   
    GSwatSession *self = GSWAT_SESSION(object);
    GSwatSessionState *session_state;
    gboolean found = FALSE;

    /* look up the save state for this session 
     * (We do this before making changes so that
     * name changes will cause a rename, not
     * a copy of session save state)
     */
    session_state = find_session_save_state(self->priv->name);
    if(session_state)
    {
        found = TRUE;
    }

    switch(property_id)
    {
#if 0 /* template code */
        case PROP_NAME:
            gswat_session_set_property(self, g_value_get_int(value));
            gswat_session_set_property(self, g_value_get_uint(value));
            gswat_session_set_property(self, g_value_get_boolean(value));
            gswat_session_set_property(self, g_value_get_string(value));
            gswat_session_set_property(self, g_value_get_object(value));
            gswat_session_set_property(self, g_value_get_pointer(value));
            break;
#endif
        case PROP_NAME:
            gswat_session_set_name(self, g_value_get_string(value));
            break;
        case PROP_TARGET:
            gswat_session_set_target(self, g_value_get_string(value));
            break;
        case PROP_COMMAND:
            gswat_session_set_command(self, g_value_get_string(value));
            break;
        case PROP_PID:
            gswat_session_set_pid(self, g_value_get_int(value));
            break;
        case PROP_WORKING_DIR:
            gswat_session_set_working_dir(self, g_value_get_string(value));
        default:
            g_warning("gswat_session_set_property on unknown property");
            return;
    }

    /* FIXME - these save state updates should be triggered by
     * property notifactions, since they currently wont have any
     * effect if a user directly uses the gswat_session_set_*
     * methods
     * Alternativly the set methods each need extending directly.
     */

    /* The name may have been unset previously so we have
     * another search here...
     */
    if(!found)
    {
        session_state = find_session_save_state(self->priv->name);
        if(session_state)
        {
            found = TRUE;
        }
    }

    if(!found)
    {
        session_state = g_new0(GSwatSessionState, 1);
        sessions = g_list_prepend(sessions, session_state);
    }

    free_session_state_members(session_state);

    if(self->priv->name){
        session_state->name = g_strdup(self->priv->name);
    }
    if(self->priv->target){
        session_state->target = g_strdup(self->priv->target);
    }
    if(self->priv->command){
        session_state->command = g_strdup(self->priv->command);
    }
    session_state->pid = self->priv->pid;
    if(self->priv->working_dir){
        session_state->working_dir = g_strdup(self->priv->working_dir);
    }
}

/* Initialize interfaces here */

#if 0
static void
gswat_session_mydoable_interface_init(gpointer interface,
                                      gpointer data)
{
    MyDoableClass *mydoable = interface;
    g_assert(G_TYPE_FROM_INTERFACE(mydoable) = MY_TYPE_MYDOABLE);

    mydoable->method1 = gswat_session_method1;
    mydoable->method2 = gswat_session_method2;
}
#endif

/* Instance Construction */
static void
gswat_session_init(GSwatSession *self)
{
    self->priv = GSWAT_SESSION_GET_PRIVATE(self);

    self->priv->type = GSWAT_SESSION_TYPE_LOCAL_RUN;

    self->priv->pid = -1;

    self->priv->working_dir = g_get_current_dir();
}

/* Instantiation wrapper */
GSwatSession*
gswat_session_new(void)
{
    return GSWAT_SESSION(g_object_new(gswat_session_get_type(), NULL));
}

/* Instance Destruction */
void
gswat_session_finalize(GObject *object)
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

    G_OBJECT_CLASS(parent_class)->finalize(object);
}



/* add new methods here */

/**
 * function_name:
 * @par1:  description of parameter 1. These can extend over more than
 * one line.
 * @par2:  description of parameter 2
 *
 * The function description goes here.
 *
 * Returns: an integer.
 */
#if 0
For more gtk-doc notes, see:
http://developer.gnome.org/arch/doc/authors.html
#endif

void
gswat_session_edit(GSwatSession *session)
{
    GladeXML  *xml;
    GtkWidget *dialog;
    GtkWidget *ok_button;
    GtkWidget *cancel_button;
    GList *tmp;
    GSwatSessionState *current_session;
    GtkWidget *name_align;
    GtkWidget *name_combo;
    GtkWidget *serial_align;
    GtkWidget *serial_combo;
    GtkWidget *command_entry;
    GtkWidget *workdir_entry;
    //GtkListStore *name_list_store;
    //GtkTreeIter iter;
    //GtkCellRenderer *renderer;



    /* load the session dialog */
    xml = glade_xml_new(GSWAT_GLADEDIR "gswat.glade",
                        "gswat_new_session_dialog",
                        NULL);

    /* in case we can't load the interface, bail */
    if(!xml) {
        g_warning("We could not load the interface!");
        gtk_main_quit();
    }
    session->priv->editor = g_new0(GSwatSessionEditor, 1);

    session->priv->editor->dialog_xml = xml;

    glade_xml_signal_autoconnect(xml);


    dialog = glade_xml_get_widget(xml, "gswat_new_session_dialog");
    name_align = glade_xml_get_widget(xml, "gswat_session_name_align");
    serial_align = glade_xml_get_widget(xml, "gswat_session_serial_align");
    command_entry = glade_xml_get_widget(xml, "gswat_session_command_entry");
    workdir_entry = glade_xml_get_widget(xml, "gswat_session_workdir_entry");
    ok_button = glade_xml_get_widget(xml, "gswat_session_ok_button");
    cancel_button = glade_xml_get_widget(xml, "gswat_session_cancel_button");



    g_signal_connect(ok_button,
                     "clicked",
                     G_CALLBACK(on_gswat_session_ok_button_clicked),
                     session);

    g_signal_connect(cancel_button,
                     "clicked",
                     G_CALLBACK(on_gswat_session_cancel_button_clicked),
                     session);

    g_signal_connect(dialog,
                     "delete-event",
                     G_CALLBACK(on_gswat_new_session_dialog_delete_event),
                     session);


    serial_combo = gtk_combo_box_new_text();
    session->priv->editor->serial_combo = serial_combo;
    gtk_container_add(GTK_CONTAINER(serial_align), serial_combo);
    gtk_widget_show(serial_combo);

    /* Setup the serial port combo box */
    gtk_combo_box_append_text(GTK_COMBO_BOX(serial_combo), "/dev/ttyS0");
    gtk_combo_box_append_text(GTK_COMBO_BOX(serial_combo), "/dev/ttyS1");
    gtk_combo_box_append_text(GTK_COMBO_BOX(serial_combo), "/dev/ttyS2");
    gtk_combo_box_append_text(GTK_COMBO_BOX(serial_combo), "/dev/ttyS3");
    gtk_combo_box_set_active(GTK_COMBO_BOX(serial_combo), 0);

    /* populate the dialog with defaults from the first saved
     * session
     */
    if(sessions != NULL)
    {
        free_sessions();
    }
    read_sessions();


    /* Setup the "name" combo box */
    /* FIXME - use gtk_combo_box_entry_new_text ()!! */

    /*
       name_list_store = gtk_list_store_new(SESSION_NAME_COMBO_N_COLUMNS,
       G_TYPE_STRING);
       session->priv->editor->name_list_store = name_list_store;
       name_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(name_list_store));
       session->priv->editor->name_combo = name_combo;
       renderer = gtk_cell_renderer_text_new();
       g_object_set(renderer,
       "mode", GTK_CELL_RENDERER_MODE_EDITABLE,
       NULL);
       gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(name_combo), renderer, TRUE);
       gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(name_combo), renderer,
       "text", SESSION_NAME_COMBO_STRING,
       NULL);
       */
    name_combo = gtk_combo_box_entry_new_text();
    session->priv->editor->name_combo = name_combo;
    gtk_container_add(GTK_CONTAINER(name_align), name_combo);
    gtk_widget_show(name_combo);

    g_signal_connect(name_combo,
                     "changed",
                     G_CALLBACK(on_gswat_new_session_name_combo_change),
                     session);

    for(tmp=sessions; tmp!=NULL; tmp=tmp->next)
    {
        current_session = (GSwatSessionState *)tmp->data;

        gtk_combo_box_append_text(GTK_COMBO_BOX(name_combo),
                                  current_session->name);
        /*
           gtk_list_store_append(name_list_store, &iter);
           gtk_list_store_set(name_list_store, &iter,
           SESSION_NAME_COMBO_STRING, current_session->name,
           -1);
           */
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(name_combo), 0);

    session->priv->editor->command_entry = command_entry;

    session->priv->editor->workdir_entry = workdir_entry;

    set_session_dialog_defaults(session, NULL);

    gtk_widget_show(dialog);

    //gswat_new_session_dialog = session_dialog;
    //gswat_new_session_xml = xml;

}


static void
set_session_dialog_defaults(GSwatSession *session, gchar *name)
{
    GList *tmp;
    GSwatSessionState *current_session;
    gboolean found = FALSE;

    if(!sessions)
    {
        return;
    }

    for(tmp=sessions; tmp!=NULL; tmp=tmp->next)
    {
        current_session = (GSwatSessionState *)tmp->data;

        if(name==NULL || strcmp(current_session->name, name)==0)
        {
            found = TRUE;
            break;
        }
    }

    if(!found)
    {
        current_session = (GSwatSessionState *)sessions->data;
    }

    /* Give the "command" entry a default */
    if(current_session->command)
    {
        gtk_entry_set_text(GTK_ENTRY(session->priv->editor->command_entry),
                           current_session->command);
    }

    /* Give the "working dir" entry a default */
    if(current_session->working_dir)
    {
        gtk_entry_set_text(GTK_ENTRY(session->priv->editor->workdir_entry),
                           current_session->working_dir);
    }

    /* TODO - populate the list of environment variables */

}


static GSwatSessionState *
find_session_save_state(gchar *session_name)
{
    GList *tmp;

    if(!session_name)
    {
        return NULL;
    }

    for(tmp=sessions; tmp!=NULL; tmp=tmp->next)
    {
        GSwatSessionState *current_session = (GSwatSessionState *)tmp->data;
        if(strcmp(current_session->name, session_name) == 0)
        {
            return current_session;
        }
    }
    return NULL;
}


static void
free_session_state_members(GSwatSessionState *session_state)
{
    if(session_state->name){
        g_free(session_state->name);
        session_state->name=NULL;
    }
    if(session_state->target){
        g_free(session_state->target);
        session_state->target=NULL;
    }
    if(session_state->command){
        g_free(session_state->command);
        session_state->command=NULL;
    }
    if(session_state->working_dir){
        g_free(session_state->working_dir);
        session_state->working_dir=NULL;
    }
}


/* Read in all the users saved sessions */
static GList *
read_sessions(void)
{
    xmlDocPtr doc;
    xmlNodePtr cur;
    gchar *file_name;


    /* FIXME: file locking - Paolo */
    file_name = gnome_util_home_file(SESSIONS_FILE);
    if (!g_file_test (file_name, G_FILE_TEST_EXISTS))
    {
        g_free (file_name);
        g_warning("Failed to open saved sessions file: %s\n", file_name);
        return NULL;
    }

    doc = xmlParseFile(file_name);
    g_free(file_name);

    if(doc == NULL)
    {
        g_warning("Failed to parse saved sessions file: %s\n", file_name);
        return NULL;
    }

    cur = xmlDocGetRootElement(doc);
    if(cur == NULL) 
    {
        g_message ("The sessions file '%s' is empty", SESSIONS_FILE);
        xmlFreeDoc (doc);

        return NULL;
    }

    if(xmlStrcmp (cur->name, (const xmlChar *) "sessions")) 
    {
        g_message ("File '%s' is of the wrong type", SESSIONS_FILE);
        xmlFreeDoc (doc);

        return NULL;
    }

    cur = xmlDocGetRootElement(doc);
    cur = cur->xmlChildrenNode;

    g_message("read_sessions");

    /* FIXME take sessions mutex */

    while (cur != NULL)
    {
        GSwatSessionState *next_session = parse_session(doc, cur);
        if(next_session)
        {
            sessions = g_list_prepend(sessions, next_session);
        }

        cur = cur->next;
    }

    /* FIXME release sessions mutex */

    xmlFreeDoc(doc);


    return sessions; 
}


static GSwatSessionState *
parse_session(xmlDocPtr doc, xmlNodePtr cur)
{
    GSwatSessionState *session_state;
    xmlChar *name, *target, *command, *working_dir;

    g_message("parse_session");

    /* ignore blank space */
    if(xmlIsBlankNode(cur))
    {
        return NULL;
    }

    if(xmlStrcmp(cur->name, (const xmlChar *)"session") != 0)
    {
        g_warning("missing \"session\" tag");
        return NULL;
    }

    session_state = g_new0(GSwatSessionState, 1);

    for(cur=cur->xmlChildrenNode; cur!=NULL; cur = cur->next)
    {
        if(xmlIsBlankNode(cur))
        {
            continue;
        }

        if(xmlStrcmp(cur->name, (const xmlChar *)"name") == 0)
        {
            name = xmlGetProp(cur, (const xmlChar *)"value");
            if(!name){
                goto parse_error;
            }

            session_state->name = g_strdup((gchar *)name);
            xmlFree(name); 
        }
        if(xmlStrcmp(cur->name, (const xmlChar *)"target") == 0)
        {
            target = xmlGetProp(cur, (const xmlChar *)"value");
            if(!target){
                goto parse_error;
            }

            session_state->target = g_strdup((gchar *)target);
            xmlFree(target); 
        }
        if(xmlStrcmp(cur->name, (const xmlChar *)"command") == 0)
        {
            command = xmlGetProp(cur, (const xmlChar *)"value");
            if(!command){
                goto parse_error;
            }

            session_state->command = g_strdup((gchar *)command);
            xmlFree(command); 
        }
        if(xmlStrcmp(cur->name, (const xmlChar *)"working_dir") == 0)
        {
            working_dir = xmlGetProp(cur, (const xmlChar *)"value");
            if(!working_dir){
                goto parse_error;
            }

            session_state->working_dir = g_strdup((gchar *)working_dir);
            xmlFree(working_dir); 
        }
    }

    if(session_state->name == NULL)
    {
        goto parse_error;
    }
    if(session_state->target == NULL)
    {
        goto parse_error;
    }
    if(session_state->command == NULL)
    {
        goto parse_error;
    }
    if(session_state->working_dir == NULL)
    {
        goto parse_error;
    }

    g_message("Read in session with name=%s", session_state->name);

    return session_state;

parse_error:

    g_warning("session parse error");

    if(session_state->name)
    {
        g_free(session_state->name);
    }
    if(session_state->target)
    {
        g_free(session_state->target);
    }
    if(session_state->command)
    {
        g_free(session_state->command);
    }
    if(session_state->working_dir)
    {
        g_free(session_state->working_dir);
    }

    g_free(session_state);
    return NULL;
}


static void
free_sessions(void)
{
    GList *tmp;

    /* FIXME take session mutex */

    for(tmp=sessions; tmp!=NULL; tmp=tmp->next)
    {
        GSwatSessionState *current_session = (GSwatSessionState *)tmp->data;
        free_session_state_members(current_session);
        g_free(current_session);
    }
    g_list_free(sessions);
    sessions = NULL;

    /* FIXME release session mutex */
}


static void
on_gswat_new_session_name_combo_change(GtkComboBoxEntry *name_combo,
                                       GSwatSession *session)
{
    gchar *name;

    name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(name_combo));
    set_session_dialog_defaults(session, name);
    g_free(name);

}


static void
on_gswat_session_ok_button_clicked(GtkButton       *button,
                                   gpointer         user_data)
{
    GSwatSession *session = GSWAT_SESSION(user_data);

    GladeXML *xml;
    GtkWidget *dialog;

    GtkWidget *name_combo;
    GtkWidget *command_entry;
    //GtkWidget *session_hostname_entry;
    //GtkWidget *session_port_entry;


    xml = session->priv->editor->dialog_xml;

    dialog = glade_xml_get_widget(xml, "gswat_new_session_dialog");
    gtk_widget_hide(dialog);

    name_combo = session->priv->editor->name_combo;
    //target_combo = glade_xml_get_widget(xml, "gswat_session_name_combo");
    command_entry = glade_xml_get_widget(xml, "gswat_session_command_entry");


    gchar *name = g_strdup("Test1");//gtk_combo_box_get_active_text(GTK_COMBO_BOX(name_combo));
    //gchar *name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(name_combo));
    g_object_set(session,
                 "name", name,
                 NULL);
    g_free(name);


    gchar *target = g_strdup("localhost");//gtk_combo_box_get_active_text(GTK_COMBO_BOX(target_combo));
    //gtk_combo_box_get_active_text(GTK_COMBO_BOX(target_combo));
    g_object_set(session,
                 "target", target,
                 NULL);
    g_free(target);


    const gchar *command = gtk_entry_get_text(GTK_ENTRY(command_entry));
    g_object_set(session,
                 "command", command,
                 NULL);



    save_sessions();

    g_object_unref(xml);
    gtk_widget_destroy(session->priv->editor->name_combo);
    gtk_widget_destroy(session->priv->editor->serial_combo);
    //g_object_unref(session->priv->editor->name_combo);
    //g_object_unref(session->priv->editor->name_list_store);
    //g_object_unref(session->priv->editor->serial_combo);
    g_free(session->priv->editor);
    session->priv->editor = NULL;

    gtk_widget_destroy(dialog);



    g_signal_emit_by_name(session, "edit-done");
}


static void
save_sessions(void)
{
    xmlDocPtr doc;
    xmlNodePtr root;
    gchar *file_name;
    GList *tmp;

    doc = xmlNewDoc((const xmlChar *)"1.0");
    if (doc == NULL)
    {
        return;
    }

    /* Create metadata root */
    root = xmlNewDocNode(doc, NULL, (const xmlChar *)"sessions", NULL);
    xmlDocSetRootElement(doc, root);

    for(tmp=sessions; tmp!=NULL; tmp=tmp->next)
    {
        GSwatSessionState *current_session = (GSwatSessionState *)tmp->data;
        save_session(root, current_session);

    }

    /* FIXME: lock file - Paolo */
    file_name = gnome_util_home_file(SESSIONS_FILE);
    xmlSaveFormatFile(file_name, doc, 1);
    g_free(file_name);

    xmlFreeDoc (doc);
}


static void
save_session(xmlNodePtr parent, GSwatSessionState *session_state)
{
    xmlNodePtr session_node;
    xmlNodePtr name_node;
    xmlNodePtr target_node;
    xmlNodePtr command_node;
    gchar *pid;
    xmlNodePtr pid_node;
    xmlNodePtr working_dir_node;

    session_node = xmlNewChild(parent, NULL, (const xmlChar *)"session", NULL);


    name_node = xmlNewChild(session_node, NULL, (const xmlChar *)"name", NULL);
    xmlSetProp(name_node,
               (const xmlChar *)"value",
               (const xmlChar *)session_state->name);

    target_node = xmlNewChild(session_node, NULL, (const xmlChar *)"target", NULL);
    xmlSetProp(target_node,
               (const xmlChar *)"value",
               (const xmlChar *)session_state->target);

    command_node = xmlNewChild(session_node, NULL, (const xmlChar *)"command", NULL);
    xmlSetProp(command_node,
               (const xmlChar *)"value",
               (const xmlChar *)session_state->command);

    pid_node = xmlNewChild(session_node, NULL, (const xmlChar *)"pid", NULL);
    pid = g_strdup_printf("%d", session_state->pid);
    xmlSetProp(pid_node,
               (const xmlChar *)"value",
               (const xmlChar *)pid);
    g_free(pid);

    working_dir_node = xmlNewChild(session_node, NULL, (const xmlChar *)"working_dir", NULL);
    xmlSetProp(working_dir_node,
               (const xmlChar *)"value",
               (const xmlChar *)session_state->working_dir);
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


static void
gswat_session_abort_edit(GSwatSession *session)
{
    GladeXML *xml;
    GtkWidget *dialog;

    xml = session->priv->editor->dialog_xml;

    dialog = glade_xml_get_widget(xml, "gswat_new_session_dialog");
    gtk_widget_hide(dialog);


    g_object_unref(xml);
    g_object_unref(session->priv->editor->name_combo);
    //g_object_unref(session->priv->editor->name_list_store);
    g_object_unref(session->priv->editor->serial_combo);
    g_free(session->priv->editor);
    session->priv->editor = NULL;

    gtk_widget_destroy(dialog);

    g_signal_emit_by_name(session, "edit-abort");
}

#if 0 // getter/setter templates
/**
 * gswat_session_get_PROPERTY:
 * @self:  A MyObject.
 *
 * Fetches the PROPERTY of the MyObject. FIXME, add more info!
 *
 * Returns: The value of PROPERTY. FIXME, add more info!
 */
PropType
gswat_session_get_PROPERTY(GSwatSession *self)
{
    g_return_if_fail(MY_IS_OBJECT(self));

    //return self->priv->PROPERTY;
    //return g_strdup(self->priv->PROPERTY);
    //return g_object_ref(self->priv->PROPERTY);
}

/**
 * gswat_session_set_PROPERTY:
 * @self:  A GSwatSession.
 * @property:  The value to set. FIXME, add more info!
 *
 * Sets this properties value.
 *
 * This will also clear the properties previous value.
 */
void
gswat_session_set_PROPERTY(GSwatSession *self, PropType PROPERTY)
{
    g_return_if_fail(MY_IS_OBJECT(self));

    //if(self->priv->PROPERTY == PROPERTY)
    //if(self->priv->PROPERTY == NULL
    //   || strcmp(self->priv->PROPERTY, PROPERTY) != 0)
    {
    //    self->priv->PROPERTY = PROPERTY;
    //    g_free(self->priv->PROPERTY);
    //    self->priv->PROPERTY = g_strdup(PROPERTY);
    //    g_object_unref(self->priv->PROPERTY);
    //    self->priv->PROPERTY = g_object_ref(PROPERTY);
    //    g_object_notify(self, "PROPERTY");
    }
}
#endif

/**
 * gswat_session_get_target:
 * @self:  A MyObject.
 *
 * Fetches the target of the MyObject.
 *
 * Returns: The sessions target name.
 */
gchar *
gswat_session_get_target(GSwatSession *self)
{
    g_return_val_if_fail(GSWAT_IS_SESSION(self), NULL);

    return g_strdup(self->priv->target);
}

/**
 * gswat_session_set_target:
 * @self:  A GSwatSession.
 * @property:  The target name to set.
 *
 * Sets the sessions target name.
 *
 * This will also clear the properties previous value.
 */
void
gswat_session_set_target(GSwatSession *self, const gchar *target)
{
    g_return_if_fail(GSWAT_IS_SESSION(self));

    if(self->priv->target == NULL
       || strcmp(self->priv->target, target) != 0)
    {
        g_free(self->priv->target);
        self->priv->target = g_strdup(target);
        g_object_notify(G_OBJECT(self), "target");
    }
}

/**
 * gswat_session_get_name:
 * @self:  A GSwatSession.
 *
 * Fetches the name of the GSwatSession.
 *
 * Return value: A newly allocated string containing the
 *               sessions name.
 */
gchar *
gswat_session_get_name(GSwatSession *self)
{
    g_return_val_if_fail(GSWAT_IS_SESSION(self), NULL);

    return g_strdup(self->priv->name);
}

/**
 * gswat_session_set_name:
 * @self:  A GSwatSession.
 * @property:  The session name to set.
 *
 * Sets the name of the session.
 *
 * This will also clear the sessions previous name.
 */
void
gswat_session_set_name(GSwatSession* self, const gchar *name)
{
    g_return_if_fail(GSWAT_IS_SESSION(self));

    if(self->priv->name == NULL
       || strcmp(self->priv->name, name) != 0)
    {
        g_free(self->priv->name);
        self->priv->name = g_strdup(name);
        g_object_notify(G_OBJECT(self), "name");
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














