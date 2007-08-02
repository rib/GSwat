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
#include <glib/gi18n.h>
#include "rb-file-helpers.h"

#include <config.h>

#include "dialogs/gedit-preferences-dialog.h"
#include "gedit-prefs-manager.h"

#include "gswat-utils.h"
#include "gswat-notebook.h"
#include "gswat-src-view-tab.h"
#include "gswat-view.h"
#include "gswat-session.h"
#include "gswat-debuggable.h"
#include "gswat-gdb-debugger.h"
#include "gswat-context.h"
#include "gswat-in-limbo-context.h"
#include "gswat-control-flow-debugging-context.h"
#include "gswat-source-browsing-context.h"

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

static void construct_layout_widgets(GSwatWindow *self);
static void destruct_layout_widgets(GSwatWindow *self);
static void set_toolbar_state(GSwatWindow *self, guint state);
static void on_session_new_activate(GtkAction *action,
                                    GSwatWindow *self);
static void clean_up_current_session(GSwatWindow *self);
static void on_gswat_debuggable_state_notify(GObject *object,
                                             GParamSpec *property,
                                             gpointer data);
static void on_gswat_session_edit_done(GSwatSession *session,
                                       GSwatWindow *window);
static void on_session_quit_activate(GtkAction *action,
                                     GSwatWindow *self);
static void on_edit_preferences_activate(GtkAction *action,
                                         GSwatWindow *self);
static void on_help_about_activate(GtkAction *action,
                                   GSwatWindow *self);
static void on_control_flow_debug_context_activate(GtkAction *action,
                                                   GSwatWindow *self);
static void on_browse_source_context_activate(GtkAction *action,
                                              GSwatWindow *self);
static void on_debugger_step_activate(GtkAction *action,
                                      GSwatWindow *self);
static void on_debugger_next_activate(GtkAction *action,
                                      GSwatWindow *self);
static void on_debugger_up_frame_activate(GtkAction *action,
                                          GSwatWindow *self);
static void on_debugger_continue_activate(GtkAction *action,
                                          GSwatWindow *self);
static void on_debugger_interrupt_activate(GtkAction *action,
                                           GSwatWindow *self);
static void on_debugger_restart_activate(GtkAction *action,
                                         GSwatWindow *self);
static void on_debugger_add_break_activate(GtkAction *action,
                                           GSwatWindow *self);
static GSwatView *get_current_view(GSwatWindow *self);
static gboolean on_window_delete_event(GtkWidget *win,
                                       GdkEventAny *event,
                                       GSwatWindow *self);
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

enum {
    PROP_0,
    PROP_SESSION,
    PROP_DEBUGGABLE,
    PROP_CONTEXT_ID
};


struct _GSwatWindowPrivate
{
    /* Per Session Data */
    GSwatSession *session;
    GSwatDebuggable *debuggable;

    /* A context is responsible for optimising the
     * gui to suit the task at hand. */
    GSwatContext *context[GSWAT_WINDOW_CONTEXT_COUNT];
    GSwatWindowContextID current_context_id;

    /* Menu and toolbar state */
    GtkUIManager *ui_manager;
    GtkActionGroup *actiongroup;

    /* Layout widgets */
    GtkWidget *window;
    GtkWidget *top_vbox;
    GtkWidget *center_vpaned;
    GtkWidget *center_hpaned;
    GtkWidget *right_vpaned;
    GSwatNotebook *notebook[GSWAT_WINDOW_CONTAINER_COUNT];
};


/* Variables */
static GObjectClass *parent_class = NULL;
//static guint gswat_window_signals[LAST_SIGNAL] = { 0 };

static GtkActionEntry gswat_window_actions [] =
{
    { "Session", NULL, N_("_Session") },
    { "Edit", NULL, N_("_Edit") },
    { "Context", NULL, N_("_Context") },
    { "Help", NULL, N_("_Help") },

    { "SessionNew", GTK_STOCK_FILE, N_("_New Session..."), NULL,
        N_("Start a new debugging session"),
        G_CALLBACK(on_session_new_activate) },
    { "SessionQuit", GTK_STOCK_QUIT, N_("_Quit"), NULL,
        N_("Quit the debugger"),
        G_CALLBACK(on_session_quit_activate) },

    { "EditPreferences", GTK_STOCK_PREFERENCES, N_("Prefere_nces..."), NULL,
        N_("Edit debugger preferences"),
        G_CALLBACK(on_edit_preferences_activate) },

    { "ControlFlowDebugContext", NULL, N_("_Control Flow Debug"), NULL,
        N_("Switch to the control flow debugging context"),
        G_CALLBACK(on_control_flow_debug_context_activate) },
    { "BrowseSourceContext", NULL, N_("_Browse Source Code"), NULL,
        N_("Switch to to the source browsing context"),
        G_CALLBACK(on_browse_source_context_activate) },

    { "HelpAbout", GTK_STOCK_ABOUT, N_("_About"), NULL,
        N_("Show information about the debugger"),
        G_CALLBACK(on_help_about_activate) },

    /* Toolbar actions */
    { "SessionNew", GTK_STOCK_FILE, N_("New"), NULL,
        N_("Start a new debugging session."),
        G_CALLBACK(on_session_new_activate) },

    { "DebuggerStepIn", GTK_STOCK_JUMP_TO, N_("Step In"), NULL,
        N_("Step program until it reaches a different source line."),
        G_CALLBACK(on_debugger_step_activate) },
    { "DebuggerNext", GTK_STOCK_GO_FORWARD, N_("Next"), NULL,
        N_("Step program, proceeding through subroutine calls."),
        G_CALLBACK(on_debugger_next_activate) },
    { "DebuggerUpFrame", GTK_STOCK_GO_UP, N_("Up Frame"), NULL,
        N_("Step program, proceeding through subroutine calls."),
        G_CALLBACK(on_debugger_up_frame_activate) },
    { "DebuggerContinue", GTK_STOCK_MEDIA_FORWARD, N_("Continue"), NULL,
        N_("Continue program being debugged, after signal or breakpoint."),
        G_CALLBACK(on_debugger_continue_activate) },

    { "DebuggerBreak", GTK_STOCK_STOP, N_("Interrupt"), NULL,
        N_("Interrupt program execution"),
        G_CALLBACK(on_debugger_interrupt_activate) },
    { "DebuggerRestart", GTK_STOCK_REFRESH, N_("Restart"), NULL,
        N_("Restart program execution"),
        G_CALLBACK(on_debugger_restart_activate) },

    { "DebuggerAddBreak", GTK_STOCK_STOP, N_("Add Break"), NULL,
        N_("Set breakpoint at current line"),
        G_CALLBACK(on_debugger_add_break_activate) },
};

static guint gswat_window_n_actions = G_N_ELEMENTS(gswat_window_actions);

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
    GParamSpec *new_param;

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

    new_param = g_param_spec_object("session", /* name */
                                    "Session", /* nick name */
                                    "The current session", /* description */
                                    GSWAT_TYPE_SESSION, /* GType */
                                    GSWAT_PARAM_READABLE /* flags */
                                   );
    g_object_class_install_property(gobject_class,
                                    PROP_SESSION,
                                    new_param);

    new_param = g_param_spec_object("debuggable", /* name */
                                    "Debuggable", /* nick name */
                                    "The current debuggable", /* description */
                                    GSWAT_TYPE_DEBUGGABLE, /* GType */
                                    GSWAT_PARAM_READABLE /* flags */
                                   );
    g_object_class_install_property(gobject_class,
                                    PROP_DEBUGGABLE,
                                    new_param);

    new_param = g_param_spec_uint("context-id", /* name */
                                  "Context ID", /* nick name */
                                  "The current context ID", /* description */
                                  0, /* minimum */
                                  (GSWAT_WINDOW_CONTEXT_COUNT-1), /* maximum */
                                  0, /* default */
                                  GSWAT_PARAM_READABLE /* flags */
                                 );
    g_object_class_install_property(gobject_class,
                                    PROP_CONTEXT_ID,
                                    new_param);

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
    self->priv = GSWAT_WINDOW_GET_PRIVATE(self);

    construct_layout_widgets(self);

    self->priv->context[GSWAT_WINDOW_IN_LIMBO_CONTEXT] = 
        GSWAT_CONTEXT(gswat_in_limbo_context_new(self));
    self->priv->context[GSWAT_WINDOW_CONTROL_FLOW_DEBUGGING_CONTEXT] = 
        GSWAT_CONTEXT(gswat_control_flow_debugging_context_new(self));
    self->priv->context[GSWAT_WINDOW_SOURCE_BROWSING_CONTEXT] =
        GSWAT_CONTEXT(gswat_source_browsing_context_new(self));
    //TODO
    //self->priv->context[GSWAT_WINDOW_NOTE_TAKING_CONTEXT] =
    //    GSWAT_CONTEXT(g_object_new(GSWAT_TYPE_NOTE_TAKING_CONTEXT, NULL));

    self->priv->current_context_id = -1;

    //connect_signals();
    //set_toolbar_state(self, GSWAT_DEBUGGABLE_NOT_RUNNING);
}

/* Instantiation wrapper */
GSwatWindow *
gswat_window_new(GSwatSession *session)
{
    GSwatWindow *self;

    self = g_object_new(GSWAT_TYPE_WINDOW, NULL);

    /* If we have been passed a session, then
     * we start that immediately
     */
    if(session)
    {
        g_object_ref(session);
        on_gswat_session_edit_done(session, self);
    }
    else
    {
        gswat_window_set_context(self, GSWAT_WINDOW_IN_LIMBO_CONTEXT);
    }

    return self;
}

/* Instance Destruction */
void
gswat_window_finalize(GObject *object)
{
    GSwatWindow *self;

    self = GSWAT_WINDOW(object);

    clean_up_current_session(self);

    if(self->priv->current_context_id != -1)
    {
        gint context_id = self->priv->current_context_id;
        gswat_context_deactivate(self->priv->context[context_id]);
    }

    destruct_layout_widgets(self);

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
construct_layout_widgets(GSwatWindow *self)
{
    GtkWindow *window;
    GtkWidget *hbox;
    GtkWidget *menubar;
    GtkWidget *toolbar;
    GSwatNotebook *notebook;
    //GtkWidget *drawing_area;
    GError *error=NULL;


    /* Create the top level container widgets */
    window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
    gtk_window_set_default_size(window, 1024, 768);
    self->priv->window = GTK_WIDGET(window);
    gtk_window_set_title(window, _("Debugger"));
    self->priv->top_vbox = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(self->priv->top_vbox);
    gtk_container_add(GTK_CONTAINER(window), self->priv->top_vbox);
    gtk_container_set_border_width(GTK_CONTAINER(self->priv->top_vbox), 0);

    g_signal_connect_object(G_OBJECT(window), "delete-event",
                            G_CALLBACK(on_window_delete_event),
                            self,
                            G_CONNECT_AFTER);


    /* Create the menus/toolbar */
    self->priv->actiongroup = gtk_action_group_new("MainActions");
    gtk_action_group_add_actions(self->priv->actiongroup,
                                 gswat_window_actions,
                                 gswat_window_n_actions,
                                 self);
    self->priv->ui_manager = gtk_ui_manager_new(); 
    gtk_ui_manager_insert_action_group(self->priv->ui_manager,
                                       self->priv->actiongroup, 
                                       0);
    gtk_ui_manager_add_ui_from_file(self->priv->ui_manager,
                                    rb_file("toolbar-menu-ui.xml"),
                                    &error);
    if (error != NULL) {
        g_warning("Couldn't merge %s: %s",
                  rb_file("toolbar-menu-ui.xml"), error->message);
        g_clear_error(&error);
    }
    menubar = gtk_ui_manager_get_widget(self->priv->ui_manager, "/MenuBar");
    gtk_box_pack_start(GTK_BOX(self->priv->top_vbox),
                       menubar, FALSE, FALSE, 0);
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(self->priv->top_vbox),
                       hbox, FALSE, FALSE, 0);
    toolbar = gtk_ui_manager_get_widget(self->priv->ui_manager, "/ToolBar");
    gtk_box_pack_start_defaults(GTK_BOX(hbox), toolbar);


    /* Create the central debugger layout widgets */
    /* Top level vpaned (source view etc top, drawing area bottom) */
    self->priv->center_vpaned = gtk_vpaned_new();
    gtk_box_pack_start(GTK_BOX(self->priv->top_vbox),
                       self->priv->center_vpaned, TRUE, TRUE, 0);
    /* A hpaned (source view left, stack/variables right) */
    self->priv->center_hpaned = gtk_hpaned_new();
    gtk_paned_set_position(GTK_PANED(self->priv->center_hpaned), 700);
    gtk_paned_pack1(GTK_PANED(self->priv->center_vpaned),
                    self->priv->center_hpaned, TRUE, TRUE);
    /* A notebook for the main source view */
    notebook = GSWAT_NOTEBOOK(gswat_notebook_new());
    self->priv->notebook[GSWAT_WINDOW_CONTAINER_MAIN] = notebook;
    gtk_paned_pack1(GTK_PANED(self->priv->center_hpaned),
                    GTK_WIDGET(notebook), TRUE, TRUE);
    /* A vpaned+notebooks for the right (variables top, stack bottom) */
    self->priv->right_vpaned = gtk_vpaned_new();
    gtk_paned_pack2(GTK_PANED(self->priv->center_hpaned),
                    self->priv->right_vpaned, TRUE, TRUE);
    notebook = GSWAT_NOTEBOOK(gswat_notebook_new());
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
    self->priv->notebook[GSWAT_WINDOW_CONTAINER_RIGHT0] = notebook;
    gtk_paned_pack1(GTK_PANED(self->priv->right_vpaned),
                    GTK_WIDGET(notebook), FALSE, TRUE);
    notebook = GSWAT_NOTEBOOK(gswat_notebook_new());
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
    self->priv->notebook[GSWAT_WINDOW_CONTAINER_RIGHT1] = notebook;
    gtk_paned_pack2(GTK_PANED(self->priv->right_vpaned),
                    GTK_WIDGET(notebook), FALSE, TRUE);


    /* Put the bottom drawing area in place */
#if 0
    notebook = GSWAT_NOTEBOOK(gswat_notebook_new());
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
    self->priv->notebook[GSWAT_WINDOW_CONTAINER_BOTTOM] = notebook;
    gtk_paned_add2(GTK_PANED(self->priv->center_vpaned),
                   GTK_WIDGET(notebook));
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    //gswat_notebook_insert_page(notebook, /* FIXME */, -1, TRUE);
    gtk_notebook_insert_page(GTK_NOTEBOOK(notebook), scrolled_window, NULL, -1);
    drawing_area = gtk_label_new(_("drawing_area")); //gtk_drawing_area_new();
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),
                                          drawing_area);
    gtk_paned_add2(GTK_PANED(self->priv->center_vpaned),
                   drawing_area);
    gtk_widget_show(drawing_area);
#endif


    gtk_widget_show_all(self->priv->center_vpaned);

    /* Create the footer widgets */
    //self->priv->statusbar = gswat_statusbar_new();


    /* Finally display it all */
    gtk_ui_manager_ensure_update(self->priv->ui_manager);
    gtk_widget_show(self->priv->window);
}


static void
destruct_layout_widgets(GSwatWindow *self)
{
    gtk_widget_destroy(self->priv->window);
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
        case GSWAT_DEBUGGABLE_DISCONNECTED:
            g_message("******** setting toolbar state GSWAT_DEBUGGABLE_DISCONNECTED");
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


static void on_session_new_activate(GtkAction *action,
                                    GSwatWindow *self)
{
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
on_gswat_session_edit_done(GSwatSession *session, GSwatWindow *self)
{
    clean_up_current_session(self);

    self->priv->session = session;
    /* Give others an opportunity to connect to the debuggable's
     * signals before we connect to the target */
    g_object_notify(G_OBJECT(self), "session");

    self->priv->debuggable 
        = GSWAT_DEBUGGABLE(gswat_gdb_debugger_new(session));

    g_signal_connect(G_OBJECT(self->priv->debuggable),
                     "notify::state",
                     G_CALLBACK(on_gswat_debuggable_state_notify),
                     self);

    gswat_debuggable_target_connect(self->priv->debuggable);

    gswat_window_set_context(self, GSWAT_WINDOW_CONTROL_FLOW_DEBUGGING_CONTEXT);
}


static void 
clean_up_current_session(GSwatWindow *self)
{
    if(self->priv->debuggable)
    {
        g_object_unref(self->priv->debuggable);
        self->priv->debuggable = NULL;
        g_object_notify(G_OBJECT(self), "debuggable");
    }

    if(self->priv->session)
    {
        g_object_unref(self->priv->session);
    }
    self->priv->session = NULL;
    g_object_notify(G_OBJECT(self), "session");

    /* FIXME - clear all other per session data */
}


static void
on_gswat_debuggable_state_notify(GObject *object,
                                 GParamSpec *property,
                                 gpointer data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    GSwatDebuggable *debuggable = GSWAT_DEBUGGABLE(object);
    guint state;

    state = gswat_debuggable_get_state(debuggable);

    if(state == GSWAT_DEBUGGABLE_DISCONNECTED)
    {
        clean_up_current_session(self);
    }

    set_toolbar_state(self, state);

}




static void
on_session_quit_activate(GtkAction *action,
                         GSwatWindow *self)
{
    g_object_unref(self->priv->debuggable);
    self->priv->debuggable = NULL;
    gtk_main_quit();
}


static void
on_edit_preferences_activate(GtkAction *action,
                             GSwatWindow *self)
{
    gedit_show_preferences_dialog();
}


static void
on_help_about_activate(GtkAction *action,
                       GSwatWindow *self)
{

}


static void
on_control_flow_debug_context_activate(GtkAction *action,
                                       GSwatWindow *self)
{
    gswat_window_set_context(self, GSWAT_WINDOW_CONTROL_FLOW_DEBUGGING_CONTEXT);
}


static void
on_browse_source_context_activate(GtkAction *action,
                                  GSwatWindow *self)
{
    gswat_window_set_context(self, GSWAT_WINDOW_SOURCE_BROWSING_CONTEXT);
}


static void
on_debugger_step_activate(GtkAction *action,
                          GSwatWindow *self)
{
    if(self->priv->debuggable)
    {
        gswat_debuggable_step_into(self->priv->debuggable);
    }
}


static void
on_debugger_next_activate(GtkAction *action,
                          GSwatWindow *self)
{
    if(self->priv->debuggable)
    {
        gswat_debuggable_next(self->priv->debuggable);
    }
}


static void
on_debugger_up_frame_activate(GtkAction *action,
                              GSwatWindow *self)
{
    if(self->priv->debuggable)
    {
        gswat_debuggable_finish(self->priv->debuggable);
    }
}


static void
on_debugger_continue_activate(GtkAction *action,
                              GSwatWindow *self)
{
    if(self->priv->debuggable)
    {
        gswat_debuggable_continue(self->priv->debuggable);
    }
}


static void
on_debugger_interrupt_activate(GtkAction *action,
                               GSwatWindow *self)
{
    if(self->priv->debuggable)
    {
        gswat_debuggable_interrupt(self->priv->debuggable);
    }
}


static void
on_debugger_restart_activate(GtkAction *action,
                             GSwatWindow *self)
{
    if(self->priv->debuggable)
    {
        gswat_debuggable_restart(self->priv->debuggable);
    }
}


static void
on_debugger_add_break_activate(GtkAction *action,
                               GSwatWindow *self)
{
    GSwatView *source_view = NULL;
    GtkTextBuffer *source_buffer = NULL;
    GeditDocument *current_document;
    gchar *uri;
    GtkTextIter cur;
    gint line; 

    if(!self->priv->debuggable)
    {
        return;
    }

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

}


static GSwatView *
get_current_view(GSwatWindow *self)
{
    GSwatNotebook *notebook;
    GtkWidget *current_page;
    GSwatSrcViewTab *src_view_tab;

    notebook = self->priv->notebook[GSWAT_WINDOW_CONTAINER_MAIN];

    current_page = 
        gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                                  gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook))
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


#if 0
static void
on_gswat_session_edit_abort(GSwatSession *session)
{

}
#endif


static gboolean
on_window_delete_event(GtkWidget *win,
                       GdkEventAny *event,
                       GSwatWindow *self)
{
    gtk_main_quit();

    return TRUE;
}


gboolean
gswat_window_set_context(GSwatWindow *self, GSwatWindowContainerID context_id)
{
    GSwatWindowContainerID prev_id;
    gboolean success;

    prev_id = self->priv->current_context_id;
    success = gswat_context_activate(self->priv->context[context_id]);

    if(success)
    {
        if(self->priv->current_context_id != prev_id)
        {
            if(prev_id != -1)
            {
                gswat_context_deactivate(self->priv->context[prev_id]);
            }
            g_object_notify(G_OBJECT(self), "context-id");
        }
    }

    return success;
}


GSwatDebuggable *
gswat_window_get_debuggable(GSwatWindow *self)
{
    if(self->priv->debuggable)
    {
        g_object_ref(G_OBJECT(self->priv->debuggable));
        return self->priv->debuggable;
    }
    else
    {
        return NULL;
    }
}


GtkContainer *
gswat_window_get_container(GSwatWindow *self, GSwatWindowContainerID container_id)
{
    if(container_id >= GSWAT_WINDOW_CONTAINER_COUNT)
    {
        g_warning("%s: Invalid container id (%d)", __FUNCTION__, container_id);
        return NULL;
    }

    return GTK_CONTAINER(self->priv->notebook[container_id]);
}


GSwatWindowContextID
gswat_window_get_context_id(GSwatWindow *self)
{
    g_assert(self->priv->current_context_id != -1);

    return self->priv->current_context_id;
}


void
gswat_window_insert_widget(GSwatWindow *self,
                           GtkWidget *widget,
                           GSwatWindowContainerID container_id)
{
    GtkNotebook *notebook;
    notebook = GTK_NOTEBOOK(self->priv->notebook[container_id]);
    gtk_notebook_insert_page(notebook, widget, NULL, -1);
}

void
gswat_window_insert_tabable(GSwatWindow *self,
                            GSwatTabable *tabable,
                            GSwatWindowContainerID container_id)
{
    g_assert(GSWAT_IS_TABABLE(tabable));

    GSwatNotebook *notebook = self->priv->notebook[container_id];
    gswat_notebook_insert_page(notebook,
                               tabable,
                               -1,
                               TRUE);
}

