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

#include <stdio.h>
#include <gtk/gtk.h>
#include <libgnomeui/libgnomeui.h>
#include <glade/glade.h>
#include <gtksourceview/gtksourceview.h>

#include <config.h>

#include "dialogs/gedit-preferences-dialog.h"
#include "gedit-prefs-manager.h"
#include "gedit-document.h"
#include "gedit-view.h"

#include "gswat-utils.h"
#include "gswat-notebook.h"
#include "gswat-src-view-tab.h"
#include "gswat-view.h"
#include "gswat-session.h"
#include "gswat-debuggable.h"
#include "gswat-gdb-debugger.h"
#include "gswat-variable-object.h"
#include "gswat-window.h"

/* Function definitions */
static void gswat_window_class_init(GSwatWindowClass *klass);
static void gswat_window_get_property(GObject *object,
                                   guint id,
                                   GValue *value,
                                   GParamSpec *pspec);
static void gswat_window_set_property(GObject *object,
                                   guint property_id,
                                   const GValue *value,
                                   GParamSpec *pspec);
//static void gswat_window_mydoable_interface_init(gpointer interface,
//                                             gpointer data);
static void gswat_window_init(GSwatWindow *self);
static void gswat_window_finalize(GObject *self);

static void create_sourceview(GSwatWindow *self);
static void destroy_sourceview(GSwatWindow *self);
static void set_toolbar_state(GSwatWindow *self, guint state);
static void on_gswat_window_new_button_clicked(GtkWidget *widget,
                                               gpointer data);
static void on_gswat_session_edit_done(GSwatSession *session,
                                       GSwatWindow *window);
static void on_gswat_window_next_button_clicked(GtkToolButton *toolbutton,
                                                gpointer data);
static void on_gswat_window_step_button_clicked(GtkToolButton *toolbutton,
                                                gpointer data);
static void on_gswat_window_finish_button_clicked(GtkToolButton *toolbutton,
                                                  gpointer data);
static void on_gswat_window_continue_button_clicked(GtkToolButton *toolbutton,
                                                    gpointer data);
static void on_gswat_window_break_button_clicked(GtkToolButton *toolbutton,
                                                 gpointer data);
static void on_gswat_window_run_button_clicked(GtkToolButton *toolbutton,
                                               gpointer data);
static void
on_gswat_window_add_break_button_clicked(GtkToolButton *toolbutton,
                                                     gpointer data);
static GSwatView *get_current_view(GSwatWindow *self);
static void on_gswat_window_debug_button_clicked(GtkToolButton *toolbutton,
                                                 gpointer data);
static void on_gswat_debuggable_source_uri_notify(GObject *debuggable,
                                                  GParamSpec *property,
                                                  gpointer data);
static void on_gswat_debuggable_source_line_notify(GObject *debuggable,
                                                   GParamSpec *property,
                                                   gpointer data);
static void update_source_view(GSwatWindow *self);
static void on_gedit_document_loaded(GeditDocument *doc,
                                     const GError *error,
                                     void *data);
static void update_line_highlights(GSwatWindow *self);
static void on_gswat_debuggable_state_notify(GObject *object,
                                             GParamSpec *property,
                                             gpointer data);
static gboolean update_variable_view(gpointer data);
static
gboolean search_for_variable_store_entry(GtkTreeStore *variable_store,
                                         GSwatVariableObject *variable_object,
                                         GtkTreeIter *iter);
static void prune_variable_store(GtkTreeStore *variable_store,
                                 GList *variables);
static void on_variable_object_updated(GObject *object);
static void on_gswat_debuggable_stack_notify(GObject *object,
                                             GParamSpec *property,
                                             gpointer data);

static void
on_gswat_window_variable_view_row_expanded(GtkTreeView *view,
                                           GtkTreeIter *iter,
                                           GtkTreePath *iter_path,
                                           gpointer    data);

/* Macros and defines */
#define GSWAT_WINDOW_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GSWAT_TYPE_WINDOW, GSwatWindowPrivate))

/* Enums */
/* add your signals here */
#if 0
enum {
    SIGNAL_NAME,
    LAST_SIGNAL
};
#endif

#if 0
enum {
    PROP_0,
    PROP_NAME,
};
#endif

enum {
    GSWAT_WINDOW_STACK_FUNC_COL,
    GSWAT_WINDOW_STACK_N_COLS
};

enum {
    GSWAT_WINDOW_VARIABLE_NAME_COL,
    GSWAT_WINDOW_VARIABLE_VALUE_COL,
    GSWAT_WINDOW_VARIABLE_OBJECT_COL,
    GSWAT_WINDOW_VARIABLES_N_COLS
};

struct _GSwatWindowPrivate
{
    GladeXML        *xml;

    GSwatDebuggable *debuggable;

    GSwatNotebook   *notebook;

    GtkListStore    *stack_list_store;
    GList           *stack;

    GtkTreeView     *variable_view;
    GtkTreeStore    *variable_store;
    //GtkTreeStore    *variables_tree_store;
    //GList           *variables;

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
    GSwatDebuggableFrame *dbg_frame;
    GString *display;
}GSwatWindowFrame;

typedef struct {
    GSwatVariableObject *variable_object;
    GString *name;
    GString *value;
}GSwatWindowVariable;

/* Variables */
static GObjectClass *parent_class = NULL;
//static guint gswat_window_signals[LAST_SIGNAL] = { 0 };

GType
gswat_window_get_type(void) /* Typechecking */
{
    static GType self_type = 0;

    if (!self_type)
    {
        static const GTypeInfo object_info =
        {
            sizeof(GSwatWindowClass), /* class structure size */
            NULL, /* base class initializer */
            NULL, /* base class finalizer */
            (GClassInitFunc)gswat_window_class_init, /* class initializer */
            NULL, /* class finalizer */
            NULL, /* class data */
            sizeof(GSwatWindow), /* instance structure size */
            0, /* preallocated instances */
            (GInstanceInitFunc)gswat_window_init, /* instance initializer */
            NULL /* function table */
        };

        /* add the type of your parent class here */
        self_type = g_type_register_static(G_TYPE_OBJECT, /* parent GType */
                                           "GSwatWindow", /* type name */
                                           &object_info, /* type info */
                                           0 /* flags */
                                          );
#if 0
        /* add interfaces here */
        static const GInterfaceInfo mydoable_info =
        {
            (GInterfaceInitFunc)
                gswat_window_mydoable_interface_init,
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
gswat_window_class_init(GSwatWindowClass *klass) /* Class Initialization */
{   
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    //GParamSpec *new_param;

    parent_class = g_type_class_peek_parent(klass);

    gobject_class->finalize = gswat_window_finalize;

    gobject_class->get_property = gswat_window_get_property;
    gobject_class->set_property = gswat_window_set_property;

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
                                     GSWAT_PARAM_READABLE /* flags */
                                     GSWAT_PARAM_WRITABLE /* flags */
                                     GSWAT_PARAM_READWRITE /* flags */
                                     | G_PARAM_CONSTRUCT
                                     | G_PARAM_CONSTRUCT_ONLY
                                     );
    g_object_class_install_property(gobject_class,
                                    PROP_NAME,
                                    new_param);
#endif

    /* set up signals */
#if 0 /* template code */
    klass->signal_member = signal_default_handler;
    gswat_window_signals[SIGNAL_NAME] =
        g_signal_new("signal_name", /* name */
                     G_TYPE_FROM_CLASS(klass), /* interface GType */
                     G_SIGNAL_RUN_LAST, /* signal flags */
                     G_STRUCT_OFFSET(GSwatWindowClass, signal_member),
                     NULL, /* accumulator */
                     NULL, /* accumulator data */
                     g_cclosure_marshal_VOID__VOID, /* c marshaller */
                     G_TYPE_NONE, /* return type */
                     0 /* number of parameters */
                     /* vararg, list of param types */
                    );
#endif

    g_type_class_add_private(klass, sizeof(GSwatWindowPrivate));
}

static void
gswat_window_get_property(GObject *object,
                          guint id,
                          GValue *value,
                          GParamSpec *pspec)
{
    //GSwatWindow* self = GSWAT_WINDOW(object);

    switch(id) {
#if 0 /* template code */
        case PROP_NAME:
            g_value_set_int(value, self->priv->property);
            g_value_set_uint(value, self->priv->property);
            g_value_set_boolean(value, self->priv->property);
            /* don't forget that this will dup the string... */
            g_value_set_string(value, self->priv->property);
            g_value_set_object(value, self->priv->property);
            g_value_set_pointer(value, self->priv->property);
            break;
#endif
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
            break;
    }
}

static void
gswat_window_set_property(GObject *object,
                          guint property_id,
                          const GValue *value,
                          GParamSpec *pspec)
{   
    switch(property_id)
    {
#if 0 /* template code */
        case PROP_NAME:
            self->priv->property = g_value_get_int(value);
            self->priv->property = g_value_get_uint(value);
            self->priv->property = g_value_get_boolean(value);
            g_free(self->priv->property);
            self->priv->property = g_value_dup_string(value);
            if(self->priv->property)
                g_object_unref(self->priv->property);
            self->priv->property = g_value_dup_object(value);
            /* g_free(self->priv->property)? */
            self->priv->property = g_value_get_pointer(value);
            break;
#endif
        default:
            g_warning("gswat_window_set_property on unknown property");
            return;
    }
}

/* Initialize interfaces here */

#if 0
static void
gswat_window_mydoable_interface_init(gpointer interface,
                                     gpointer data)
{
    MyDoableClass *mydoable = interface;
    g_assert(G_TYPE_FROM_INTERFACE(mydoable) = MY_TYPE_MYDOABLE);

    mydoable->method1 = gswat_window_method1;
    mydoable->method2 = gswat_window_method2;
}
#endif

/* Instance Construction */
static void
gswat_window_init(GSwatWindow *self)
{
    GnomeApp *main_window = NULL;
    GtkWidget *button;
    GtkWidget *item;
    GtkTreeView *stack_widget;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeView *variable_view;
    GtkTreeStore *variable_store;

    self->priv = GSWAT_WINDOW_GET_PRIVATE(self);

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
    item = glade_xml_get_widget(self->priv->xml, "gswat_window_file_new_menu_item");
    g_signal_connect(item,
                     "activate",
                     G_CALLBACK(on_gswat_window_new_button_clicked),
                     self
                    );

    button = glade_xml_get_widget(self->priv->xml, "gswat_window_next_button");

    self->priv->next_button = button;
    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(on_gswat_window_next_button_clicked),
                     self
                    );

    button = glade_xml_get_widget(self->priv->xml, "gswat_window_step_button");
    self->priv->step_button = button;
    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(on_gswat_window_step_button_clicked),
                     self
                    );

    button = glade_xml_get_widget(self->priv->xml, "gswat_window_finish_button");
    self->priv->finish_button = button;
    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(on_gswat_window_finish_button_clicked),
                     self
                    );

    button = glade_xml_get_widget(self->priv->xml, "gswat_window_continue_button");
    self->priv->continue_button = button;
    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(on_gswat_window_continue_button_clicked),
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
                     G_CALLBACK(on_gswat_window_debug_button_clicked),
                     self
                    );


    main_window = GNOME_APP(glade_xml_get_widget(self->priv->xml,
                                                 "gswat_main_window")
                           );

    if(!main_window)
    {
        g_critical("Could not find main window in gswat.glade");
    }


    create_sourceview(self);


    set_toolbar_state(self, GSWAT_DEBUGGABLE_NOT_RUNNING);


    /* Setup the stack view */
    stack_widget = 
        (GtkTreeView *)glade_xml_get_widget(self->priv->xml, 
                                            "gswat_window_stack_widget"
                                           );

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Stack",
                                                      renderer,
                                                      "markup", GSWAT_WINDOW_STACK_FUNC_COL,
                                                      NULL);
    gtk_tree_view_append_column(stack_widget, column);



    /* Setup the variable view */
    variable_view = 
        (GtkTreeView *)glade_xml_get_widget(self->priv->xml, 
                                            "gswat_window_variable_view"
                                           );
    gtk_tree_view_set_rules_hint(variable_view, TRUE);


    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Variable",
                                                      renderer,
                                                      "markup", GSWAT_WINDOW_VARIABLE_NAME_COL,
                                                      NULL);
    gtk_tree_view_append_column(variable_view, column);


    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Value",
                                                      renderer,
                                                      "markup", GSWAT_WINDOW_VARIABLE_VALUE_COL,
                                                      NULL);
    gtk_tree_view_append_column(variable_view, column);

    g_signal_connect(variable_view,
                     "row_expanded",
                     G_CALLBACK(on_gswat_window_variable_view_row_expanded),
                     self);
    self->priv->variable_view = variable_view;

    variable_store = gtk_tree_store_new(GSWAT_WINDOW_VARIABLES_N_COLS,
                                        G_TYPE_STRING,
                                        G_TYPE_STRING,
                                        G_TYPE_OBJECT);
    gtk_tree_view_set_model(variable_view, GTK_TREE_MODEL(variable_store));
    self->priv->variable_store = variable_store;


    gtk_widget_show(GTK_WIDGET(main_window));
}

/* Instantiation wrapper */
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

/* Instance Destruction */
void
gswat_window_finalize(GObject *object)
{
    GSwatWindow *window;

    window = GSWAT_WINDOW(object);

    destroy_sourceview(window);

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


static void
create_sourceview(GSwatWindow *self)
{
    GtkWidget       *h_panes;

    h_panes = glade_xml_get_widget(self->priv->xml,
                                   "gswat_window_h_panes");

    self->priv->notebook = GSWAT_NOTEBOOK(gswat_notebook_new());

    gtk_widget_show(GTK_WIDGET(self->priv->notebook));

    gtk_container_remove(GTK_CONTAINER(h_panes),
                         gtk_paned_get_child1((GTK_PANED(h_panes))));
    gtk_paned_add1(GTK_PANED(h_panes), GTK_WIDGET(self->priv->notebook));

}


static void
destroy_sourceview(GSwatWindow *self)
{
    gtk_widget_destroy(GTK_WIDGET(self->priv->notebook));
}


static void
set_toolbar_state(GSwatWindow *self, guint state)
{
    switch(state)
    {
        case GSWAT_DEBUGGABLE_RUNNING:
            g_message("******** setting toolbar state GSWAT_DEBUGGABLE_RUNNING");
            //http://bugzilla.gnome.org/show_bug.cgi?id=56070
#if 0 
            gtk_widget_set_sensitive(self->priv->step_button, FALSE);
            gtk_widget_set_sensitive(self->priv->next_button, FALSE);
            gtk_widget_set_sensitive(self->priv->continue_button, FALSE);
            gtk_widget_set_sensitive(self->priv->finish_button, FALSE);
            gtk_widget_set_sensitive(self->priv->break_button, TRUE);
            gtk_widget_set_sensitive(self->priv->restart_button, TRUE);
            gtk_widget_set_sensitive(self->priv->add_break_button, FALSE);
#endif
            break;
        case GSWAT_DEBUGGABLE_NOT_RUNNING:
            g_message("******** setting toolbar state GSWAT_DEBUGGABLE_NOT_RUNNING");
            //http://bugzilla.gnome.org/show_bug.cgi?id=56070
#if 0
            gtk_widget_set_sensitive(self->priv->step_button, FALSE);
            gtk_widget_set_sensitive(self->priv->next_button, FALSE);
            gtk_widget_set_sensitive(self->priv->continue_button, FALSE);
            gtk_widget_set_sensitive(self->priv->finish_button, FALSE);
            gtk_widget_set_sensitive(self->priv->break_button, FALSE);
            gtk_widget_set_sensitive(self->priv->add_break_button, TRUE);
#endif
            break;
        case GSWAT_DEBUGGABLE_INTERRUPTED:
            g_message("******** setting toolbar state GSWAT_DEBUGGABLE_INTERRUPTED");
            //http://bugzilla.gnome.org/show_bug.cgi?id=56070
#if 0
            gtk_widget_set_sensitive(self->priv->step_button, TRUE);
            gtk_widget_set_sensitive(self->priv->next_button, TRUE);
            gtk_widget_set_sensitive(self->priv->continue_button, TRUE);
            gtk_widget_set_sensitive(self->priv->finish_button, TRUE);
            gtk_widget_set_sensitive(self->priv->break_button, FALSE);
            gtk_widget_set_sensitive(self->priv->restart_button, TRUE);
            gtk_widget_set_sensitive(self->priv->add_break_button, TRUE);
#endif
            break;
        default:
            g_warning("unexpected debuggable state");
            //http://bugzilla.gnome.org/show_bug.cgi?id=56070
#if 0
            gtk_widget_set_sensitive(self->priv->step_button, FALSE);
            gtk_widget_set_sensitive(self->priv->next_button, FALSE);
            gtk_widget_set_sensitive(self->priv->continue_button, FALSE);
            gtk_widget_set_sensitive(self->priv->finish_button, FALSE);
            gtk_widget_set_sensitive(self->priv->break_button, FALSE);
            gtk_widget_set_sensitive(self->priv->restart_button, FALSE);
            gtk_widget_set_sensitive(self->priv->add_break_button, FALSE);
#endif
            break;
    }

}


static void
on_gswat_window_new_button_clicked(GtkWidget   *widget,
                                   gpointer     data)
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


static void
on_gswat_session_edit_done(GSwatSession *session, GSwatWindow *window)
{
    GList *tabs, *tmp;

    if(window->priv->debuggable)
    {
        g_object_unref(window->priv->debuggable);

        tabs = gtk_container_get_children(GTK_CONTAINER(window->priv->notebook));
        for(tmp=tabs;tmp!=NULL;tmp=tmp->next)
        {
            gtk_widget_destroy(GTK_WIDGET(tmp->data));
        }
        g_list_free(tabs);
    }

    window->priv->debuggable 
        = GSWAT_DEBUGGABLE(gswat_gdb_debugger_new(session));

    /* currently we don't require a handle to the session
     * from this class..
     */
    g_object_unref(session);


    g_signal_connect(G_OBJECT(window->priv->debuggable),
                     "notify::state",
                     G_CALLBACK(on_gswat_debuggable_state_notify),
                     window);

    g_signal_connect(G_OBJECT(window->priv->debuggable),
                     "notify::stack",
                     G_CALLBACK(on_gswat_debuggable_stack_notify),
                     window);

    g_signal_connect(G_OBJECT(window->priv->debuggable),
                     "notify::source-uri",
                     G_CALLBACK(on_gswat_debuggable_source_uri_notify),
                     window);

    g_signal_connect(G_OBJECT(window->priv->debuggable),
                     "notify::source-line",
                     G_CALLBACK(on_gswat_debuggable_source_line_notify),
                     window);
#if 0
    g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
                    update_variable_view,
                    window,
                    NULL);
#endif

    gswat_debuggable_target_connect(window->priv->debuggable);
}


static void
on_gswat_window_next_button_clicked(GtkToolButton   *toolbutton,
                                    gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gswat_debuggable_next(self->priv->debuggable);
}


static void
on_gswat_window_step_button_clicked(GtkToolButton   *toolbutton,
                                    gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gswat_debuggable_step_into(self->priv->debuggable);
}


static void
on_gswat_window_finish_button_clicked(GtkToolButton   *toolbutton,
                                      gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gswat_debuggable_finish(self->priv->debuggable);
}


static void
on_gswat_window_continue_button_clicked  (GtkToolButton   *toolbutton,
                                          gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gswat_debuggable_continue(self->priv->debuggable);
}


static void
on_gswat_window_break_button_clicked(GtkToolButton   *toolbutton,
                                     gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gswat_debuggable_interrupt(self->priv->debuggable);
}


static void
on_gswat_window_run_button_clicked(GtkToolButton   *toolbutton,
                                   gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gswat_debuggable_restart(self->priv->debuggable);
}


static void
on_gswat_window_add_break_button_clicked(GtkToolButton   *toolbutton,
                                         gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    GSwatView *source_view = NULL;
    GtkTextBuffer *source_buffer = NULL;
    GeditDocument *current_document;
    gchar *uri;
    GtkTextIter cur;
    gint line; 

    source_view = get_current_view(self);
    g_return_if_fail(source_view != NULL);

    source_buffer = GTK_TEXT_VIEW(source_view)->buffer;

    current_document = gswat_view_get_document(source_view);
    uri = gedit_document_get_uri(current_document); 

    gtk_text_buffer_get_iter_at_mark(source_buffer, 
                                     &cur, 
                                     gtk_text_buffer_get_insert(source_buffer)
                                    );
    line = gtk_text_iter_get_line(&cur);

    gswat_debuggable_request_line_breakpoint(self->priv->debuggable, uri, line + 1);

    /* FIXME this should be triggered by a signal */
    update_line_highlights(self);
}


static GSwatView *
get_current_view(GSwatWindow *self)
{
    GtkNotebook *notebook = GTK_NOTEBOOK(self->priv->notebook);
    GtkWidget *current_page;
    GSwatSrcViewTab *src_view_tab;

    current_page = 
        gtk_notebook_get_nth_page(notebook,
                                  gtk_notebook_get_current_page(notebook)
                                 );

    src_view_tab = g_object_get_data(G_OBJECT(current_page),
                                     "gswat-src-view-tab");
    if(src_view_tab)
    {
        return gswat_src_view_tab_get_view(src_view_tab);
    }
    else
    {
        return NULL;
    }
}


static void
on_gswat_window_debug_button_clicked(GtkToolButton   *toolbutton,
                                     gpointer         data)
{
#if 0
    GSwatWindow *self = GSWAT_WINDOW(data);
#endif
}


static void
on_gswat_debuggable_source_uri_notify(GObject *debuggable,
                                      GParamSpec *property,
                                      gpointer data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    update_source_view(self);
}


static void
on_gswat_debuggable_source_line_notify(GObject *debuggable,
                                       GParamSpec *property,
                                       gpointer data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    update_source_view(self);
}


/* FIXME, this seems fairly heavy weight to happen every
 * time you step a single line while debugging
 */
static void
update_source_view(GSwatWindow *self)
{
    GSwatDebuggable *debuggable;
    gchar           *file_uri;
    gint            line;
    GList           *tabs, *tmp;
    GSwatView       *gswat_view;
    GeditDocument   *gedit_document;
    GtkTextIter     iter;
    GSwatSrcViewTab *src_view_tab;

    debuggable = self->priv->debuggable;

    file_uri = gswat_debuggable_get_source_uri(debuggable);
    line = gswat_debuggable_get_source_line(debuggable);

    g_message("gswat-window update_source_view file=%s, line=%d", file_uri, line);

    /* Until we support pining/snapshoting then we only
     * support one GSwatSrcViewTab
     */

    /* Look for the source view tab... */
    tabs = gtk_container_get_children(GTK_CONTAINER(self->priv->notebook));
    src_view_tab = NULL;
    for(tmp=tabs; tmp!=NULL; tmp=tmp->next)
    {
        GSwatSrcViewTab *current_tab;

        current_tab = 
            g_object_get_data(G_OBJECT(tmp->data),
                              "gswat-src-view-tab");
        if(!GSWAT_IS_SRC_VIEW_TAB(current_tab))
        {
            continue;
        }
        src_view_tab = current_tab;
    }
    g_list_free(tabs);


    if(src_view_tab)
    {
        GSwatView *gswat_view;
        char *uri;

        gswat_view = gswat_src_view_tab_get_view(src_view_tab);

        gedit_document = gswat_view_get_document(gswat_view);
        uri = gedit_document_get_uri(gedit_document);
        if(strcmp(uri, file_uri)!=0)
        {
            /* When a GeditDocument finishes loading a URI or
             * loading is canceled we get a signal and we will
             * end up re-calling update_source_view.
             *
             * Therefore in both of these cases we are going to
             * bomb out of this update early.
             */
            if(gedit_document_is_loading(gedit_document))
            {
                gedit_document_load_cancel(gedit_document);
            }
            else
            {
                const GeditEncoding *encoding;
                encoding = gedit_document_get_encoding(gedit_document);
                gedit_document_load(gedit_document,
                                    file_uri,
                                    encoding,
                                    line,
                                    FALSE);
            }
            g_free(uri);
            g_object_unref(gswat_view);
            return;
        }
        g_free(uri);

        gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(gedit_document),
                                         &iter,
                                         line);
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(gswat_view),
                                     &iter,
                                     0.2,
                                     FALSE,
                                     0,
                                     0);
        //gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(gedit_document), &iter);

        g_object_unref(gswat_view);
    }
    else
    {
        /* If we dont already have a source view tab... */

        src_view_tab = gswat_src_view_tab_new(file_uri, line);

        g_object_set_data(G_OBJECT(gswat_tabable_get_page_widget(src_view_tab)),
                          "gswat-src-view-tab",
                          src_view_tab);

        gswat_notebook_insert_page(self->priv->notebook,
                                   GSWAT_TABABLE(src_view_tab),
                                   -1,
                                   TRUE);

        gswat_view = 
            gswat_src_view_tab_get_view(GSWAT_SRC_VIEW_TAB(src_view_tab));

        gedit_document = gswat_view_get_document(gswat_view);
        g_signal_connect(gedit_document,
                         "loaded",
                         G_CALLBACK(on_gedit_document_loaded),
                         self);
        gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(gedit_document),
                                         &iter,
                                         line);
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(gswat_view),
                                     &iter,
                                     0.2,
                                     FALSE,
                                     0,
                                     0);
        //gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(gedit_document), &iter);
    }

    update_line_highlights(self);

    return;

}

static void
on_gedit_document_loaded(GeditDocument *doc,
                         const GError *error,
                         void *data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    update_source_view(self);
}

static void
update_line_highlights(GSwatWindow *self)
{
    /* request all debuggable breakpoints */
    GList *breakpoints;
    GList *tabs;
    GList *tmp, *tmp2;
    GList *highlights=NULL;
    GSwatViewLineHighlight *highlight;
    gchar *current_doc_uri;
    GdkColor breakpoint_color;


    /* FIXME - this is a hugely overweight solution
     * we shouldnt need to update the list of line
     * highlights in every view e.g. just to effectivly
     * update the current line highlight in the current
     * view.
     */

    breakpoints = gswat_debuggable_get_breakpoints(self->priv->debuggable);

    tabs = gtk_container_get_children(GTK_CONTAINER(self->priv->notebook));
    for(tmp=tabs;tmp!=NULL;tmp=tmp->next)
    {
        GSwatSrcViewTab *current_tab;
        GSwatView *current_gswat_view;

        current_tab = 
            g_object_get_data(G_OBJECT(tmp->data),
                              "gswat-src-view-tab");
        if(!GSWAT_IS_SRC_VIEW_TAB(current_tab))
        {
            continue;
        }

        current_gswat_view = gswat_src_view_tab_get_view(current_tab);

        current_doc_uri 
            = gedit_document_get_uri(gswat_view_get_document(current_gswat_view));

        breakpoint_color = gedit_prefs_manager_get_breakpoint_bg_color();

        /* generate a list of breakpount lines to highlight in this view */
        for(tmp2=breakpoints; tmp2 != NULL; tmp2=tmp2->next)
        {
            GSwatDebuggableBreakpoint *current_breakpoint;

            current_breakpoint = (GSwatDebuggableBreakpoint *)tmp2->data;
            if(strcmp(current_breakpoint->source_uri, current_doc_uri)==0)
            {
                highlight = g_new(GSwatViewLineHighlight, 1);
                highlight->line = current_breakpoint->line;
                highlight->color = breakpoint_color;
                highlights=g_list_prepend(highlights, highlight);
            }
        }

        /* add a line to highlight the current line */
        if(strcmp(gswat_debuggable_get_source_uri(self->priv->debuggable),
                  current_doc_uri) == 0)
        {
            highlight = g_new(GSwatViewLineHighlight, 1);
            highlight->line = gswat_debuggable_get_source_line(self->priv->debuggable);
            highlight->color = gedit_prefs_manager_get_current_line_bg_color();
            /* note we currently need to append here to ensure
             * the current line gets rendered last */
            highlights=g_list_append(highlights, highlight);
        }

        gswat_view_set_line_highlights(current_gswat_view, highlights);

        /* free the list */
        for(tmp2=highlights; tmp2!=NULL; tmp2=tmp2->next)
        {
            g_free(tmp2->data);
        }
        g_list_free(highlights);
        highlights=NULL;
    }
}


static void
on_gswat_debuggable_state_notify(GObject *object,
                                 GParamSpec *property,
                                 gpointer data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    GSwatDebuggable *debuggable = GSWAT_DEBUGGABLE(object);
    guint state;

    update_variable_view(self);

    state = gswat_debuggable_get_state(debuggable);

    set_toolbar_state(self, state);

}


static gboolean
update_variable_view(gpointer data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    GSwatDebuggable *debuggable = self->priv->debuggable;
    GList *variables, *tmp;
    GtkTreeStore *variable_store;
    GtkTreeRowReference *row_ref;
    GtkTreePath *path;

    variables = gswat_debuggable_get_locals_list(debuggable);
    variable_store = self->priv->variable_store;

    /* iterate the current tree store and see if there
     * is already an entry for each variable object.
     * If not, then add one. */
    for(tmp=variables; tmp!=NULL; tmp=tmp->next)
    {
        GSwatVariableObject *variable_object;
        variable_object = (GSwatVariableObject *)tmp->data;
        char *expression, *value;
        GtkTreeIter iter, child;
        guint child_count;

        if(search_for_variable_store_entry(variable_store,
                                           variable_object,
                                           &iter))
        {
            continue;
        }

        expression = 
            gswat_variable_object_get_expression(variable_object);
        value = gswat_variable_object_get_value(variable_object, NULL);

        gtk_tree_store_append(variable_store, &iter, NULL);
        gtk_tree_store_set(variable_store, &iter,
                           GSWAT_WINDOW_VARIABLE_NAME_COL,
                           expression,
                           GSWAT_WINDOW_VARIABLE_VALUE_COL,
                           value,
                           GSWAT_WINDOW_VARIABLE_OBJECT_COL,
                           G_OBJECT(variable_object),
                           -1);
        g_object_set_data(G_OBJECT(variable_object),
                          "gswat_tree_store",
                          variable_store);

        path = gtk_tree_model_get_path(GTK_TREE_MODEL(variable_store), &iter);
        row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(variable_store), path);
        gtk_tree_path_free(path);
        g_object_set_data(G_OBJECT(variable_object),
                          "gswat_row_reference",
                          row_ref);

        g_signal_connect(variable_object,
                         "updated",
                         G_CALLBACK(on_variable_object_updated),
                         NULL
                        );

        child_count = 
            gswat_variable_object_get_child_count(variable_object);

        if(child_count)
        {
            gtk_tree_store_append(variable_store, &child, &iter);
            gtk_tree_store_set(variable_store, &child,
                               GSWAT_WINDOW_VARIABLE_NAME_COL,
                               "Retrieving Children...",
                               GSWAT_WINDOW_VARIABLE_VALUE_COL,
                               "",
                               GSWAT_WINDOW_VARIABLE_OBJECT_COL,
                               NULL,
                               -1);

        }

        g_free(expression);
        g_free(value);
    }

    /* Remove tree store entries that don't exist in
     * the current list of variables */
    prune_variable_store(variable_store, variables);


    for(tmp=variables; tmp!=NULL; tmp=tmp->next)
    {
        g_object_unref(G_OBJECT(tmp->data));
    }
    g_list_free(variables);

    return TRUE;
}


static gboolean
search_for_variable_store_entry(GtkTreeStore *variable_store,
                                GSwatVariableObject *variable_object,
                                GtkTreeIter *iter)
{
    GSwatVariableObject *current_variable_object;

    if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(variable_store),
                                      iter))
    {
        return FALSE;
    }

    do{
        gtk_tree_model_get(GTK_TREE_MODEL(variable_store),
                           iter,
                           GSWAT_WINDOW_VARIABLE_OBJECT_COL,
                           &current_variable_object,
                           -1);
        if(current_variable_object == variable_object)
        {
            g_object_unref(G_OBJECT(current_variable_object));
            return TRUE;
        }
        g_object_unref(G_OBJECT(current_variable_object));

    }while(gtk_tree_model_iter_next(GTK_TREE_MODEL(variable_store), iter));

    return FALSE;
}


static void
prune_variable_store(GtkTreeStore *variable_store,
                     GList *variables)
{
    GSwatVariableObject *current_variable_object;
    GtkTreeIter iter;
    GList *tmp, *row_refs=NULL;
    GtkTreePath *path;
    GtkTreeRowReference *row_ref;

    if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(variable_store),
                                      &iter))
    {
        return;
    }

    do{
        gtk_tree_model_get(GTK_TREE_MODEL(variable_store),
                           &iter,
                           GSWAT_WINDOW_VARIABLE_OBJECT_COL,
                           &current_variable_object,
                           -1);
        if(g_list_find(variables, current_variable_object))
        {
            g_object_unref(G_OBJECT(current_variable_object));
            continue;
        }

        /* mark this tree store entry for removal */

        path = gtk_tree_model_get_path(GTK_TREE_MODEL(variable_store), &iter);
        row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(variable_store), path);
        row_refs = g_list_prepend(row_refs, row_ref);
        gtk_tree_path_free(path);


        row_ref = g_object_get_data(G_OBJECT(current_variable_object),
                                    "gswat_row_reference");
        gtk_tree_row_reference_free(row_ref);

        g_object_unref(current_variable_object);

    }while(gtk_tree_model_iter_next(GTK_TREE_MODEL(variable_store), &iter));

    /* iterate the list of rows that need to be deleted
     * and actually remove them
     */
    for(tmp=row_refs; tmp!=NULL; tmp=tmp->next)
    {
        row_ref = tmp->data;
        path = gtk_tree_row_reference_get_path(row_ref);
        g_assert(path != NULL);
        gtk_tree_model_get_iter(GTK_TREE_MODEL(variable_store), &iter, path);
        gtk_tree_store_remove(variable_store, &iter);
        gtk_tree_path_free(path);
        gtk_tree_row_reference_free(row_ref);
    }
    if(row_refs)
    {
        g_list_free(row_refs);
    }

    return;
}


static void
on_variable_object_updated(GObject *object)
{
    GSwatVariableObject *variable_object;
    GtkTreeStore *store;
    GtkTreeRowReference *row_ref;
    GtkTreePath *path;
    char *value;
    GtkTreeIter iter;

    variable_object = GSWAT_VARIABLE_OBJECT(object);
    g_message("on_variable_object_updated");
    store = g_object_get_data(object, "gswat_tree_store");
    if(!store)
    {
        g_message("on_variable_object_updated 1");
        return;
    }

    row_ref = g_object_get_data(object, "gswat_row_reference");
    if(!row_ref)
    {
        g_message("on_variable_object_updated 2");
        return;
    }

    g_message("on_variable_object_updated3");
    value = gswat_variable_object_get_value(variable_object, NULL);

    path = gtk_tree_row_reference_get_path(row_ref);
    g_assert(path != NULL);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path);
    gtk_tree_store_set(store, &iter,
                       GSWAT_WINDOW_VARIABLE_VALUE_COL,
                       value,
                       -1);
    g_free(value);
}


static void
on_gswat_debuggable_stack_notify(GObject *object,
                                 GParamSpec *property,
                                 gpointer data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    GSwatDebuggable *debuggable = GSWAT_DEBUGGABLE(object);
    GList *stack, *tmp, *tmp2;
    GSwatDebuggableFrameArgument *arg;
    GladeXML *xml;
    GtkTreeView *stack_widget=NULL;
    GtkListStore *list_store;
    GtkTreeIter iter;
    GList *disp_stack = NULL;

    stack = gswat_debuggable_get_stack(debuggable);

    /* TODO look up the stack tree view widget */
    xml = self->priv->xml;
    stack_widget = (GtkTreeView *)glade_xml_get_widget(xml, "gswat_window_stack_widget");


    list_store = gtk_list_store_new(GSWAT_WINDOW_STACK_N_COLS,
                                    G_TYPE_STRING,
                                    G_TYPE_ULONG);



    /* create new display strings for the stack */
    for(tmp=stack; tmp!=NULL; tmp=tmp->next)
    {
        GSwatDebuggableFrame *frame;
        GSwatWindowFrame *display_frame = g_new0(GSwatWindowFrame, 1);
        GString *display = g_string_new("");

        frame = (GSwatDebuggableFrame *)tmp->data;

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
            arg = (GSwatDebuggableFrameArgument *)tmp2->data;


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
    if(self->priv->stack_list_store)
    {
        g_object_unref(self->priv->stack_list_store);
    }
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
            arg = (GSwatDebuggableFrameArgument *)tmp2->data;

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


#if 0
static void
on_gswat_session_edit_abort(GSwatSession *session)
{

}
#endif


static void
on_gswat_window_variable_view_row_expanded(GtkTreeView *view,
                                           GtkTreeIter *iter,
                                           GtkTreePath *iter_path,
                                           gpointer    data)
{
    //GSwatWindow *self = GSWAT_WINDOW(data);
    GtkTreeStore *store;
    GtkTreeIter expanded, child, child2;
    GSwatVariableObject *variable_object, *current_child;
    GList *children, *tmp;
    GList *row_refs;
    GtkTreeRowReference *row_ref;
    GtkTreePath *path;


    store = GTK_TREE_STORE(gtk_tree_view_get_model(view));

    /* Deleting multiple rows at one go is little tricky. We need to
       take row references before they are deleted
       */
    row_refs = NULL;
    if (gtk_tree_model_iter_children(GTK_TREE_MODEL(store), &child, iter))
    {
        /* Get row references */
        do
        {
            path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &child);
            row_ref = gtk_tree_row_reference_new (GTK_TREE_MODEL (store), path);
            row_refs = g_list_prepend (row_refs, row_ref);
            gtk_tree_path_free (path);
        }
        while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &child));
    }


    gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &expanded, iter_path);

    gtk_tree_model_get(GTK_TREE_MODEL(store),
                       &expanded,
                       GSWAT_WINDOW_VARIABLE_OBJECT_COL,
                       &variable_object,
                       -1);

    children=gswat_variable_object_get_children(variable_object);
    for(tmp=children; tmp!=NULL; tmp=tmp->next)
    {
        current_child = (GSwatVariableObject *)tmp->data;
        char *expression, *value;
        //GtkTreeIter iter, child;
        guint child_count;

        expression = 
            gswat_variable_object_get_expression(current_child);
        value = gswat_variable_object_get_value(current_child, NULL);

        gtk_tree_store_append(store, &child, &expanded);
        gtk_tree_store_set(store, &child,
                           GSWAT_WINDOW_VARIABLE_NAME_COL,
                           expression,
                           GSWAT_WINDOW_VARIABLE_VALUE_COL,
                           value,
                           GSWAT_WINDOW_VARIABLE_OBJECT_COL,
                           G_OBJECT(current_child),
                           -1);
        g_object_set_data(G_OBJECT(current_child),
                          "gswat_tree_store",
                          store);

        path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &child);
        row_ref = gtk_tree_row_reference_new (GTK_TREE_MODEL (store), path);
        gtk_tree_path_free(path);
        g_object_set_data(G_OBJECT(current_child),
                          "gswat_row_reference",
                          row_ref);

        g_signal_connect(current_child,
                         "updated",
                         G_CALLBACK(on_variable_object_updated),
                         NULL
                        );

        child_count = 
            gswat_variable_object_get_child_count(current_child);

        if(child_count)
        {
            gtk_tree_store_append(store, &child2, &child);
            gtk_tree_store_set(store, &child2,
                               GSWAT_WINDOW_VARIABLE_NAME_COL,
                               "Retrieving Children...",
                               GSWAT_WINDOW_VARIABLE_VALUE_COL,
                               "",
                               GSWAT_WINDOW_VARIABLE_OBJECT_COL,
                               NULL,
                               -1);

        }

        g_free(expression);
        g_free(value);
    }
    g_object_unref(G_OBJECT(variable_object));

    /* Delete the referenced rows */
    for(tmp=row_refs; tmp!=NULL; tmp=tmp->next)
    {
        row_ref = tmp->data;
        path = gtk_tree_row_reference_get_path(row_ref);
        g_assert (path != NULL);
        gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &child, path);
        gtk_tree_store_remove(store, &child);
        gtk_tree_path_free(path);
        gtk_tree_row_reference_free(row_ref);
    }
    if(row_refs)
    {
        g_list_free(row_refs);
    }
}


void
on_gswat_window_preferences_activate(GtkMenuItem     *menuitem,
                                     gpointer         data)
{
    gedit_show_preferences_dialog();
}


void
on_gswat_window_about_activate(GtkMenuItem     *menuitem,
                               gpointer         user_data)
{

}


void
on_gswat_window_quit_activate(GtkMenuItem     *menuitem,
                              gpointer         data)
{
    gtk_main_quit();
}


