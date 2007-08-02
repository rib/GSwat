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

#include <string.h>
#include <glib/gi18n.h>

#include "gedit-prefs-manager.h"
#include "gedit-document.h"
#include "gedit-view.h"

#include "gswat-variable-object.h"
#include "gswat-context.h"
#include "gswat-src-view-tab.h"
#include "gswat-notebook.h"
#include "gswat-control-flow-debugging-context.h"

/* Macros and defines */
#define GSWAT_CONTROL_FLOW_DEBUGGING_CONTEXT_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GSWAT_TYPE_CONTROL_FLOW_DEBUGGING_CONTEXT, GSwatControlFlowDebuggingContextPrivate))

/* Enums/Typedefs */
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
    STACK_FUNC_COL,
    STACK_N_COLS
};

enum {
    VARIABLE_NAME_COL,
    VARIABLE_VALUE_COL,
    VARIABLE_OBJECT_COL,
    GSWAT_WINDOW_VARIABLES_N_COLS
};


struct _GSwatControlFlowDebuggingContextPrivate
{
    gboolean        active;
    
    GSwatWindow *window;
    GSwatDebuggable *debuggable;

    GSwatSrcViewTab *src_view_tab;
    GtkScrolledWindow *variable_view_scrolled_window;
    GtkScrolledWindow *stack_view_scrolled_window;

    GtkTreeView *stack_view;
    GtkTreeView *variable_view;
    
    GQueue          *debuggable_stack;
    GtkListStore    *stack_list_store;
    GList           *display_stack;
    GtkTreeStore    *variable_store;
};

typedef struct {
    GString *display;
}GSwatControlFlowDebuggingContextFrame;

typedef struct {
    GSwatVariableObject *variable_object;
    GString *name;
    GString *value;
}GSwatControlFlowDebuggingContextVariable;


/* Function definitions */
static void gswat_control_flow_debugging_context_class_init(GSwatControlFlowDebuggingContextClass *klass);
static void gswat_control_flow_debugging_context_get_property(GObject *object,
                                                              guint id,
                                                              GValue *value,
                                                              GParamSpec *pspec);
static void gswat_control_flow_debugging_context_set_property(GObject *object,
                                                              guint property_id,
                                                              const GValue *value,
                                                              GParamSpec *pspec);
static void gswat_control_flow_debugging_context_context_interface_init(gpointer interface,
                                                                        gpointer data);
static void gswat_control_flow_debugging_context_init(GSwatControlFlowDebuggingContext *self);
static void gswat_control_flow_debugging_context_finalize(GObject *self);

static gboolean gswat_control_flow_debugging_context_activate(GSwatContext *object);
static void gswat_control_flow_debugging_context_deactivate(GSwatContext *object);
static void optimise_gui_for_control_flow_debugging(GSwatControlFlowDebuggingContext *self);

static void connect_debuggable_signals(GSwatControlFlowDebuggingContext *self);
static void on_gswat_window_debuggable_notify(GObject *object,
                                              GParamSpec *property,
                                              gpointer data);
static void on_variable_view_row_expanded(GtkTreeView *view,
                                          GtkTreeIter *iter,
                                          GtkTreePath *iter_path,
                                          gpointer data);
static void on_variable_view_row_collapsed(GtkTreeView *view,
                                           GtkTreeIter *iter,
                                           GtkTreePath *iter_path,
                                           gpointer data);
static void on_gswat_debuggable_source_uri_notify(GObject *debuggable,
                                                  GParamSpec *property,
                                                  gpointer data);
static void on_gswat_debuggable_source_line_notify(GObject *debuggable,
                                                   GParamSpec *property,
                                                   gpointer data);
static void on_gswat_debuggable_breakpoints_notify(GObject *debuggable,
                                                   GParamSpec *property,
                                                   gpointer data);
static void update_source_view(GSwatControlFlowDebuggingContext *self);
static void on_gedit_document_loaded(GeditDocument *doc,
                                     const GError *error,
                                     void *data);
static void update_line_highlights(GSwatControlFlowDebuggingContext *self);
static void update_variable_view(gpointer data);
static gboolean search_for_variable_store_entry(GtkTreeStore *variable_store,
                                                GSwatVariableObject *variable_object,
                                                GtkTreeIter *iter);
static void add_variable_object_to_tree_store(GSwatVariableObject *variable_object,
                                              GtkTreeStore *variable_store,
                                              GtkTreeIter *parent);
static void prune_variable_store(GtkTreeStore *variable_store,
                                 GList *variables);
static void on_variable_object_value_notify(GObject *object,
                                            GParamSpec *property,
                                            gpointer data);
static void on_variable_object_invalidated(GObject *object,
                                           GParamSpec *property,
                                           gpointer data);
static void on_variable_object_child_count_notify(GObject *object,
                                                  GParamSpec *property,
                                                  gpointer data);
static void on_gswat_debuggable_stack_notify(GObject *object,
                                             GParamSpec *property,
                                             gpointer data);
static void free_current_stack_view_entries(GSwatControlFlowDebuggingContext *self);
static void on_gswat_debuggable_locals_notify(GObject *object,
                                              GParamSpec *property,
                                              gpointer data);
static void
on_stack_frame_activated(GtkTreeView *tree_view,
                         GtkTreePath *path,
                         GtkTreeViewColumn *column,
                         gpointer data);


/* Variables */
static GObjectClass *parent_class = NULL;
//static guint gswat_control_flow_debugging_context_signals[LAST_SIGNAL] = { 0 };

GType
gswat_control_flow_debugging_context_get_type(void) /* Typechecking */
{
    static GType self_type = 0;

    if (!self_type)
    {
        static const GTypeInfo object_info =
        {
            sizeof(GSwatControlFlowDebuggingContextClass), /* class structure size */
            NULL, /* base class initializer */
            NULL, /* base class finalizer */
            (GClassInitFunc)gswat_control_flow_debugging_context_class_init, /* class initializer */
            NULL, /* class finalizer */
            NULL, /* class data */
            sizeof(GSwatControlFlowDebuggingContext), /* instance structure size */
            0, /* preallocated instances */
            (GInstanceInitFunc)gswat_control_flow_debugging_context_init, /* instance initializer */
            NULL /* function table */
        };

        /* add the type of your parent class here */
        self_type = g_type_register_static(G_TYPE_OBJECT, /* parent GType */
                                           "GSwatControlFlowDebuggingContext", /* type name */
                                           &object_info, /* type info */
                                           0 /* flags */
                                          );

        /* add interfaces here */
        static const GInterfaceInfo context_info =
        {
            (GInterfaceInitFunc)
                gswat_control_flow_debugging_context_context_interface_init,
            (GInterfaceFinalizeFunc)NULL,
            NULL /* interface data */
        };

        if(self_type != G_TYPE_INVALID) {
            g_type_add_interface_static(self_type,
                                        GSWAT_TYPE_CONTEXT,
                                        &context_info);
        }

    }

    return self_type;
}

static void
gswat_control_flow_debugging_context_class_init(GSwatControlFlowDebuggingContextClass *klass) /* Class Initialization */
{   
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    //GParamSpec *new_param;

    parent_class = g_type_class_peek_parent(klass);

    gobject_class->finalize = gswat_control_flow_debugging_context_finalize;

    gobject_class->get_property = gswat_control_flow_debugging_context_get_property;
    gobject_class->set_property = gswat_control_flow_debugging_context_set_property;

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

    /* set up signals */
#if 0 /* template code */
    klass->signal_member = signal_default_handler;
    gswat_control_flow_debugging_context_signals[SIGNAL_NAME] =
        g_signal_new("signal_name", /* name */
                     G_TYPE_FROM_CLASS(klass), /* interface GType */
                     G_SIGNAL_RUN_LAST, /* signal flags */
                     G_STRUCT_OFFSET(GSwatControlFlowDebuggingContextClass, signal_member),
                     NULL, /* accumulator */
                     NULL, /* accumulator data */
                     g_cclosure_marshal_VOID__VOID, /* c marshaller */
                     G_TYPE_NONE, /* return type */
                     0 /* number of parameters */
                     /* vararg, list of param types */
                    );
#endif

    g_type_class_add_private(klass, sizeof(GSwatControlFlowDebuggingContextPrivate));
}

static void
gswat_control_flow_debugging_context_get_property(GObject *object,
                                                  guint id,
                                                  GValue *value,
                                                  GParamSpec *pspec)
{
    //GSwatControlFlowDebuggingContext* self = GSWAT_CONTROL_FLOW_DEBUGGING_CONTEXT(object);

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
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
            break;
    }
}

static void
gswat_control_flow_debugging_context_set_property(GObject *object,
                                                  guint property_id,
                                                  const GValue *value,
                                                  GParamSpec *pspec)
{   
    //GSwatControlFlowDebuggingContext* self = GSWAT_CONTROL_FLOW_DEBUGGING_CONTEXT(object);

    switch(property_id)
    {
#if 0 /* template code */
        case PROP_NAME:
            gswat_control_flow_debugging_context_set_property(self, g_value_get_int(value));
            gswat_control_flow_debugging_context_set_property(self, g_value_get_uint(value));
            gswat_control_flow_debugging_context_set_property(self, g_value_get_boolean(value));
            gswat_control_flow_debugging_context_set_property(self, g_value_get_string(value));
            gswat_control_flow_debugging_context_set_property(self, g_value_get_object(value));
            gswat_control_flow_debugging_context_set_property(self, g_value_get_pointer(value));
            break;
#endif
        default:
            g_warning("gswat_control_flow_debugging_context_set_property on unknown property");
            return;
    }
}

/* Initialize interfaces here */

static void
gswat_control_flow_debugging_context_context_interface_init(gpointer interface,
                                                            gpointer data)
{
    GSwatContextIface *context = interface;
    g_assert(G_TYPE_FROM_INTERFACE(context) == GSWAT_TYPE_CONTEXT);

    context->activate = gswat_control_flow_debugging_context_activate;
    context->deactivate = gswat_control_flow_debugging_context_deactivate;
}


/* Instance Construction */
static void
gswat_control_flow_debugging_context_init(GSwatControlFlowDebuggingContext *self)
{
    self->priv = GSWAT_CONTROL_FLOW_DEBUGGING_CONTEXT_GET_PRIVATE(self);
    /* populate your object here */
}

/* Instantiation wrapper */
GSwatControlFlowDebuggingContext*
gswat_control_flow_debugging_context_new(GSwatWindow *window)
{
    GSwatControlFlowDebuggingContext *self;

    self = GSWAT_CONTROL_FLOW_DEBUGGING_CONTEXT(g_object_new(gswat_control_flow_debugging_context_get_type(), NULL));

    self->priv->window = window;

    return self;
}

/* Instance Destruction */
void
gswat_control_flow_debugging_context_finalize(GObject *object)
{
    //GSwatControlFlowDebuggingContext *self = GSWAT_CONTROL_FLOW_DEBUGGING_CONTEXT(object);

    /* destruct your object here */
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


#if 0 // getter/setter templates
/**
 * gswat_control_flow_debugging_context_get_PROPERTY:
 * @self:  A GSwatControlFlowDebuggingContext.
 *
 * Fetches the PROPERTY of the GSwatControlFlowDebuggingContext. FIXME, add more info!
 *
 * Returns: The value of PROPERTY. FIXME, add more info!
 */
PropType
gswat_control_flow_debugging_context_get_PROPERTY(GSwatControlFlowDebuggingContext *self)
{
    g_return_val_if_fail(GSWAT_IS_CONTROL_FLOW_DEBUGGING_CONTEXT(self), /* FIXME */);

    //return self->priv->PROPERTY;
    //return g_strdup(self->priv->PROPERTY);
    //return g_object_ref(self->priv->PROPERTY);
}

/**
 * gswat_control_flow_debugging_context_set_PROPERTY:
 * @self:  A GSwatControlFlowDebuggingContext.
 * @property:  The value to set. FIXME, add more info!
 *
 * Sets this properties value.
 *
 * This will also clear the properties previous value.
 */
void
gswat_control_flow_debugging_context_set_PROPERTY(GSwatControlFlowDebuggingContext *self, PropType PROPERTY)
{
    g_return_if_fail(GSWAT_IS_CONTROL_FLOW_DEBUGGING_CONTEXT(self));

    //if(self->priv->PROPERTY == PROPERTY)
    //if(self->priv->PROPERTY == NULL
    //   || strcmp(self->priv->PROPERTY, PROPERTY) != 0)
    {
        //    self->priv->PROPERTY = PROPERTY;
        //    g_free(self->priv->PROPERTY);
        //    self->priv->PROPERTY = g_strdup(PROPERTY);
        //    g_object_unref(self->priv->PROPERTY);
        //    self->priv->PROPERTY = g_object_ref(PROPERTY);
        //    g_object_notify(G_OBJECT(self), "PROPERTY");
    }
}
#endif


static gboolean
gswat_control_flow_debugging_context_activate(GSwatContext *object)
{
    GSwatControlFlowDebuggingContext *self;
    GSwatDebuggable *debuggable;

    g_return_val_if_fail(GSWAT_IS_CONTROL_FLOW_DEBUGGING_CONTEXT(object), FALSE);
    self = GSWAT_CONTROL_FLOW_DEBUGGING_CONTEXT(object);

    if(self->priv->active == TRUE)
    {
        return TRUE;
    }

    g_assert(self->priv->window);

    g_message("gswat_control_flow_debugging_context_activate");

    if(!self->priv->debuggable)
    {
        debuggable = gswat_window_get_debuggable(self->priv->window);
        if(!debuggable)
        {
            g_warning("You can only activate the control flow debuggin context\n"
                      " when you have a valid debuggable.");
            return FALSE;
        }
        self->priv->debuggable = debuggable;
        g_signal_connect(self->priv->window,
                         "notify::debuggable",
                         G_CALLBACK(on_gswat_window_debuggable_notify),
                         self);
        /*
           g_signal_connect(window,
           "notify::session",
           G_CALLBACK(on_gswat_window_session_notify),
           self);
           */
        connect_debuggable_signals(self);
    }

    optimise_gui_for_control_flow_debugging(self);

    self->priv->active=TRUE;
    return TRUE;
}


static void
gswat_control_flow_debugging_context_deactivate(GSwatContext *object)
{
    GSwatControlFlowDebuggingContext *self;

    g_return_if_fail(GSWAT_IS_CONTROL_FLOW_DEBUGGING_CONTEXT(object));
    self = GSWAT_CONTROL_FLOW_DEBUGGING_CONTEXT(object);
    
    /* Hide context related widgets */
    if(!self->priv->active)
    {
        return;
    }

    GtkContainer *container;

    container = gswat_window_get_container(self->priv->window,
                                           GSWAT_WINDOW_CONTAINER_MAIN);
    gtk_container_remove(container, GTK_WIDGET(self->priv->src_view_tab));

    container = gswat_window_get_container(self->priv->window,
                                           GSWAT_WINDOW_CONTAINER_RIGHT0);
    gtk_container_remove(container,
                         GTK_WIDGET(self->priv->variable_view_scrolled_window));

    container = gswat_window_get_container(self->priv->window,
                                           GSWAT_WINDOW_CONTAINER_RIGHT1);
    gtk_container_remove(container,
                         GTK_WIDGET(self->priv->stack_view_scrolled_window));

    self->priv->active=FALSE;

    g_message("gswat_control_flow_debugging_context_deactivate");
}


static void
optimise_gui_for_control_flow_debugging(GSwatControlFlowDebuggingContext *self)
{
    GtkTreeView *stack_view;
    GtkTreeView *variable_view;
    GtkTreeStore *variable_store;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;


    if(self->priv->src_view_tab)
    {
        gswat_window_insert_tabable(self->priv->window,
                                    GTK_WIDGET(self->priv->src_view_tab),
                                    GSWAT_WINDOW_CONTAINER_MAIN);
    }
    else
    {
        GSwatSrcViewTab *src_view_tab;
        GSwatView *gswat_view;
        GeditDocument *gedit_document;

        src_view_tab = gswat_src_view_tab_new(NULL, 0);
        /* When switching away from this context we will be
         * removing this widget rom the gui but we don't want it
         * to be destroyed at that point */
        g_object_ref(src_view_tab);

        gswat_window_insert_tabable(self->priv->window,
                                    GSWAT_TABABLE(src_view_tab),
                                    GSWAT_WINDOW_CONTAINER_MAIN);

        gswat_view = 
            gswat_src_view_tab_get_view(GSWAT_SRC_VIEW_TAB(src_view_tab));

        gedit_document = gswat_view_get_document(gswat_view);
        g_object_unref(gswat_view);

        g_signal_connect(gedit_document,
                         "loaded",
                         G_CALLBACK(on_gedit_document_loaded),
                         self);

        self->priv->src_view_tab = src_view_tab;
    }
    gtk_widget_show(gswat_tabable_get_page_widget(self->priv->src_view_tab));


    /* Setup the local variables view */
    if(self->priv->variable_view_scrolled_window)
    {
        gswat_window_insert_widget(self->priv->window,
                                   GTK_WIDGET(self->priv->variable_view_scrolled_window),
                                   GSWAT_WINDOW_CONTAINER_RIGHT0);
    }
    else
    {
        GtkWidget *scrolled_window;

        scrolled_window = gtk_scrolled_window_new(NULL, NULL);
        /* When switching away from this context we will be
         * removing this widget from the gui but we don't want it
         * to be destroyed at that point */
        g_object_ref(scrolled_window);
        self->priv->variable_view_scrolled_window = 
            GTK_SCROLLED_WINDOW(scrolled_window);
        gswat_window_insert_widget(self->priv->window,
                                   scrolled_window,
                                   GSWAT_WINDOW_CONTAINER_RIGHT0);
        variable_view = GTK_TREE_VIEW(gtk_tree_view_new());
        self->priv->variable_view = variable_view;
        gtk_container_add(GTK_CONTAINER(scrolled_window),
                          GTK_WIDGET(variable_view));
        gtk_tree_view_set_rules_hint(variable_view, TRUE);

        renderer = gtk_cell_renderer_text_new();
        column = 
            gtk_tree_view_column_new_with_attributes(_("Variable"),
                                                     renderer,
                                                     "markup",
                                                     VARIABLE_NAME_COL,
                                                     NULL);
        gtk_tree_view_append_column(variable_view, column);


        renderer = gtk_cell_renderer_text_new();
        column = 
            gtk_tree_view_column_new_with_attributes(_("Value"),
                                                     renderer,
                                                     "markup",
                                                     VARIABLE_VALUE_COL,
                                                     NULL);
        gtk_tree_view_append_column(variable_view, column);

        g_signal_connect(variable_view,
                         "row-expanded",
                         G_CALLBACK(on_variable_view_row_expanded),
                         self);

        g_signal_connect(variable_view,
                         "row-collapsed",
                         G_CALLBACK(on_variable_view_row_collapsed),
                         self);

        variable_store = gtk_tree_store_new(GSWAT_WINDOW_VARIABLES_N_COLS,
                                            G_TYPE_STRING,
                                            G_TYPE_STRING,
                                            G_TYPE_OBJECT);
        gtk_tree_view_set_model(variable_view, GTK_TREE_MODEL(variable_store));
        self->priv->variable_store = variable_store;

    }
    gtk_widget_show_all(GTK_WIDGET(self->priv->variable_view_scrolled_window));


    /* Setup the stack view */
    if(self->priv->stack_view_scrolled_window)
    {
        gswat_window_insert_widget(self->priv->window,
                                   GTK_WIDGET(self->priv->stack_view_scrolled_window),
                                   GSWAT_WINDOW_CONTAINER_RIGHT1);
    }
    else
    {
        GtkWidget *scrolled_window;

        scrolled_window = gtk_scrolled_window_new(NULL, NULL);
        /* When switching away from this context we will be
         * removing this widget from the gui but we don't want it
         * to be destroyed at that point */
        g_object_ref(scrolled_window);
        self->priv->stack_view_scrolled_window = 
            GTK_SCROLLED_WINDOW(scrolled_window);
        gswat_window_insert_widget(self->priv->window,
                                   scrolled_window,
                                   GSWAT_WINDOW_CONTAINER_RIGHT1);
        stack_view = GTK_TREE_VIEW(gtk_tree_view_new());
        self->priv->stack_view = stack_view;
        gtk_container_add(GTK_CONTAINER(scrolled_window),
                          GTK_WIDGET(stack_view));
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes(_("Stack"),
                                                          renderer,
                                                          "markup",
                                                          STACK_FUNC_COL,
                                                          NULL);
        gtk_tree_view_append_column(stack_view, column);
        g_signal_connect(G_OBJECT(stack_view),
                         "row-activated",
                         G_CALLBACK(on_stack_frame_activated),
                         self);
    }
    gtk_widget_show_all(GTK_WIDGET(self->priv->stack_view_scrolled_window));
}


static void
connect_debuggable_signals(GSwatControlFlowDebuggingContext *self)
{

    g_signal_connect(G_OBJECT(self->priv->debuggable),
                     "notify::stack",
                     G_CALLBACK(on_gswat_debuggable_stack_notify),
                     self);

    g_signal_connect(G_OBJECT(self->priv->debuggable),
                     "notify::source-uri",
                     G_CALLBACK(on_gswat_debuggable_source_uri_notify),
                     self);

    g_signal_connect(G_OBJECT(self->priv->debuggable),
                     "notify::source-line",
                     G_CALLBACK(on_gswat_debuggable_source_line_notify),
                     self);

    g_signal_connect(G_OBJECT(self->priv->debuggable),
                     "notify::breakpoints",
                     G_CALLBACK(on_gswat_debuggable_breakpoints_notify),
                     self);

    g_signal_connect(G_OBJECT(self->priv->debuggable),
                     "notify::locals",
                     G_CALLBACK(on_gswat_debuggable_locals_notify),
                     self);

}



static void
on_gswat_window_debuggable_notify(GObject *object,
                                  GParamSpec *property,
                                  gpointer data)
{
    GSwatControlFlowDebuggingContext *self =
        GSWAT_CONTROL_FLOW_DEBUGGING_CONTEXT(data);
    GSwatWindow *window = GSWAT_WINDOW(object);
    GSwatDebuggable *debuggable;

    debuggable = gswat_window_get_debuggable(window);
    if(debuggable == self->priv->debuggable)
    {
        return;
    }

    g_object_unref(self->priv->debuggable);
    free_current_stack_view_entries(self);
    /* FIXME - what else needs to be cleared up? */

    self->priv->debuggable = debuggable;

    if(self->priv->debuggable)
    {
        connect_debuggable_signals(self);
    }

    if(self->priv->active && self->priv->debuggable)
    {
        update_source_view(self);
    }
}


static void
on_variable_view_row_expanded(GtkTreeView *view,
                              GtkTreeIter *iter,
                              GtkTreePath *iter_path,
                              gpointer data)
{
    //GSwatControlFlowDebuggingContext *self = 
    //  GSWAT_CONTROL_FLOW_DEBUGGING_CONTEXT(data);
    GtkTreeStore *store;
    GtkTreeIter expanded, child;
    GSwatVariableObject *variable_object, *current_child;
    GList *children, *tmp;
    GList *row_refs;
    GtkTreeRowReference *row_ref;
    GtkTreePath *path;


    store = GTK_TREE_STORE(gtk_tree_view_get_model(view));

    /* Deleting multiple rows in one go is a little tricky. We need to
     * take row references before they are deleted */
    row_refs = NULL;
    if(gtk_tree_model_iter_children(GTK_TREE_MODEL(store), &child, iter))
    {
        /* Get row references */
        do
        {
            path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &child);
            row_ref = gtk_tree_row_reference_new (GTK_TREE_MODEL (store), path);
            row_refs = g_list_prepend (row_refs, row_ref);
            gtk_tree_path_free (path);
        }
        while(gtk_tree_model_iter_next(GTK_TREE_MODEL (store), &child));
    }

    gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &expanded, iter_path);

    gtk_tree_model_get(GTK_TREE_MODEL(store),
                       &expanded,
                       VARIABLE_OBJECT_COL,
                       &variable_object,
                       -1);

    children=gswat_variable_object_get_children(variable_object);
    for(tmp=children; tmp!=NULL; tmp=tmp->next)
    {
        current_child = (GSwatVariableObject *)tmp->data;

        add_variable_object_to_tree_store(current_child,
                                          store,
                                          &expanded);
        g_object_unref(G_OBJECT(current_child));
    }
    g_object_unref(G_OBJECT(variable_object));


    /* Delete the referenced rows */
    for(tmp=row_refs; tmp!=NULL; tmp=tmp->next)
    {
        row_ref = tmp->data;
        path = gtk_tree_row_reference_get_path(row_ref);
        if(path)
        {
            gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &child, path);
            gtk_tree_store_remove(store, &child);
            gtk_tree_path_free(path);
        }
        gtk_tree_row_reference_free(row_ref);
    }
    if(row_refs)
    {
        g_list_free(row_refs);
    }

}

static void
on_variable_view_row_collapsed(GtkTreeView *view,
                               GtkTreeIter *iter,
                               GtkTreePath *iter_path,
                               gpointer data)
{
    GtkTreeStore *store;
    GtkTreeIter collapsed, child;
    GSwatVariableObject *variable_object;
    GList *row_refs;
    GtkTreeRowReference *row_ref;
    GtkTreePath *path;
    guint child_count;
    GList *tmp;

    store = GTK_TREE_STORE(gtk_tree_view_get_model(view));


    /* Deleting multiple rows in one go is a little tricky. We need to
     * take row references before they are deleted */
    row_refs = NULL;
    if(gtk_tree_model_iter_children(GTK_TREE_MODEL(store), &child, iter))
    {
        /* Get row references */
        do
        {
            path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &child);
            row_ref = gtk_tree_row_reference_new (GTK_TREE_MODEL (store), path);
            row_refs = g_list_prepend (row_refs, row_ref);
            gtk_tree_path_free (path);
        }
        while(gtk_tree_model_iter_next(GTK_TREE_MODEL (store), &child));
    }

    gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &collapsed, iter_path);

    gtk_tree_model_get(GTK_TREE_MODEL(store),
                       &collapsed,
                       VARIABLE_OBJECT_COL,
                       &variable_object,
                       -1);
    child_count = 
        gswat_variable_object_get_child_count(variable_object);

    if(child_count)
    {
        gtk_tree_store_append(store, &child, &collapsed);
        gtk_tree_store_set(store, &child,
                           VARIABLE_NAME_COL,
                           _("Retrieving Children..."),
                           VARIABLE_VALUE_COL,
                           "",
                           VARIABLE_OBJECT_COL,
                           NULL,
                           -1);
    }


    /* Delete the referenced rows */
    for(tmp=row_refs; tmp!=NULL; tmp=tmp->next)
    {
        row_ref = tmp->data;
        path = gtk_tree_row_reference_get_path(row_ref);
        if(path)
        {
            gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &child, path);

#if 0
            /* DEBUG: so you can inspect the iterated variables
             * in a debugger */
            gtk_tree_model_get(GTK_TREE_MODEL(store),
                               &child,
                               VARIABLE_OBJECT_COL,
                               &variable_object,
                               -1);
            g_object_unref(variable_object);
#endif

            gtk_tree_store_remove(store, &child);
            gtk_tree_path_free(path);
        }
        gtk_tree_row_reference_free(row_ref);
    }
    if(row_refs)
    {
        g_list_free(row_refs);
    }
}

static void
on_gswat_debuggable_source_uri_notify(GObject *debuggable,
                                      GParamSpec *property,
                                      gpointer data)
{
    GSwatControlFlowDebuggingContext *self = 
        GSWAT_CONTROL_FLOW_DEBUGGING_CONTEXT(data);
    update_source_view(self);
}


static void
on_gswat_debuggable_source_line_notify(GObject *debuggable,
                                       GParamSpec *property,
                                       gpointer data)
{
    GSwatControlFlowDebuggingContext *self = 
        GSWAT_CONTROL_FLOW_DEBUGGING_CONTEXT(data);
    update_source_view(self);
}

static void
on_gswat_debuggable_breakpoints_notify(GObject *debuggable,
                                       GParamSpec *property,
                                       gpointer data)
{
    GSwatControlFlowDebuggingContext *self = 
        GSWAT_CONTROL_FLOW_DEBUGGING_CONTEXT(data);

    update_line_highlights(self);
}


/* FIXME, this seems fairly heavy weight to happen every
 * time you step a single line while debugging
 */
static void
update_source_view(GSwatControlFlowDebuggingContext *self)
{
    GSwatDebuggable *debuggable;
    gchar *file_uri;
    gint line;
    GSwatView *gswat_view;
    GeditDocument *gedit_document;
    GtkTextIter iter;
    GtkTextMark *mark;
    GSwatSrcViewTab *src_view_tab;
    char *uri;

    debuggable = self->priv->debuggable;

    file_uri = gswat_debuggable_get_source_uri(debuggable);
    if(!file_uri)
    {
        return;
    }
    line = gswat_debuggable_get_source_line(debuggable);

    g_message("gswat-window update_source_view file=%s, line=%d", file_uri, line);

    src_view_tab = self->priv->src_view_tab;
    gswat_view = gswat_src_view_tab_get_view(src_view_tab);
    gedit_document = gswat_view_get_document(gswat_view);
    uri = gedit_document_get_uri(gedit_document);
    if(!uri || strcmp(uri, file_uri)!=0)
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
        g_free(file_uri);
        g_free(uri);
        g_object_unref(gswat_view);
        return;
    }
    g_free(uri);

    gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(gedit_document),
                                     &iter,
                                     line);
    mark = gtk_text_buffer_create_mark(GTK_TEXT_BUFFER(gedit_document),
                                       NULL,
                                       &iter,
                                       TRUE);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(gswat_view),
                                 mark,
                                 0.2,
                                 FALSE,
                                 0.0,
                                 0.5);
    gtk_text_buffer_delete_mark(GTK_TEXT_BUFFER(gedit_document),
                                mark);

    g_object_unref(gswat_view);

    update_line_highlights(self);

    g_free(file_uri);

    return;

}

static void
on_gedit_document_loaded(GeditDocument *doc,
                         const GError *error,
                         void *data)
{
    GSwatControlFlowDebuggingContext *self = 
        GSWAT_CONTROL_FLOW_DEBUGGING_CONTEXT(data);
    update_source_view(self);
}

static void
update_line_highlights(GSwatControlFlowDebuggingContext *self)
{
    /* request all debuggable breakpoints */
    GList *breakpoints;
    GList *tmp;
    GList *highlights=NULL;
    GSwatViewLineHighlight *highlight;
    gchar *doc_uri;
    GdkColor breakpoint_color;
    GSwatSrcViewTab *src_view_tab;
    GSwatView *gswat_view;

    /* FIXME - this is a rather overweight solution
     * we shouldn't need to update the list of line
     * highlights in every view e.g. just to effectivly
     * update the current line highlight in the current
     * view.
     */

    src_view_tab = self->priv->src_view_tab;
    gswat_view = gswat_src_view_tab_get_view(self->priv->src_view_tab);

    doc_uri = gedit_document_get_uri(gswat_view_get_document(gswat_view));
    if(!doc_uri)
    {
        return;
    }
    
    breakpoints = gswat_debuggable_get_breakpoints(self->priv->debuggable);
    
    breakpoint_color = gedit_prefs_manager_get_breakpoint_bg_color();
    
    /* generate a list of breakpount lines to highlight in this view */
    for(tmp=breakpoints; tmp != NULL; tmp=tmp->next)
    {
        GSwatDebuggableBreakpoint *current_breakpoint;

        current_breakpoint = (GSwatDebuggableBreakpoint *)tmp->data;
        if(strcmp(current_breakpoint->source_uri, doc_uri)==0)
        {
            highlight = g_new(GSwatViewLineHighlight, 1);
            highlight->line = current_breakpoint->line;
            highlight->color = breakpoint_color;
            highlights=g_list_prepend(highlights, highlight);
        }
    }

    /* add a line to highlight the current line */
    if(strcmp(gswat_debuggable_get_source_uri(self->priv->debuggable),
              doc_uri) == 0)
    {
        highlight = g_new(GSwatViewLineHighlight, 1);
        highlight->line = gswat_debuggable_get_source_line(self->priv->debuggable);
        highlight->color = gedit_prefs_manager_get_current_line_bg_color();
        /* note we currently need to append here to ensure
         * the current line gets rendered last */
        highlights=g_list_append(highlights, highlight);
    }

    gswat_view_set_line_highlights(gswat_view, highlights);
    g_object_unref(gswat_view);

    /* free the list */
    for(tmp=highlights; tmp!=NULL; tmp=tmp->next)
    {
        g_free(tmp->data);
    }
    g_list_free(highlights);
    highlights=NULL;

    gswat_debuggable_free_breakpoints(breakpoints);
}


static void
update_variable_view(gpointer data)
{
    GSwatControlFlowDebuggingContext *self = 
        GSWAT_CONTROL_FLOW_DEBUGGING_CONTEXT(data);
    GSwatDebuggable *debuggable = self->priv->debuggable;
    GList *variables, *tmp;
    GtkTreeStore *variable_store;


    variables = gswat_debuggable_get_locals_list(debuggable);
    variable_store = self->priv->variable_store;

    /* iterate the current tree store and see if there
     * is already an entry for each variable object.
     * If not, then add one. */
    for(tmp=variables; tmp!=NULL; tmp=tmp->next)
    {
        GSwatVariableObject *variable_object = tmp->data;
        GtkTreeIter iter;

        if(search_for_variable_store_entry(variable_store,
                                           variable_object,
                                           &iter))
        {
            continue;
        }

        add_variable_object_to_tree_store(variable_object,
                                          variable_store,
                                          NULL);
    }

    /* Remove tree store entries that don't exist in
     * the current list of variables */
    prune_variable_store(variable_store, variables);

    for(tmp=variables; tmp!=NULL; tmp=tmp->next)
    {
        g_object_unref(G_OBJECT(tmp->data));
    }
    g_list_free(variables);

    return;
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
                           VARIABLE_OBJECT_COL,
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
add_variable_object_to_tree_store(GSwatVariableObject *variable_object,
                                  GtkTreeStore *variable_store,
                                  GtkTreeIter *parent)
{
    char *expression, *value;
    GtkTreeIter iter, child;
    GtkTreePath *path;
    GtkTreeRowReference *row_ref;
    guint child_count;

    if(g_object_get_data(G_OBJECT(variable_object), "gswat_tree_store")!=NULL)
    {
        /* FIXME - Do we really need to object to this? */
        g_warning("add_variable_object_to_tree_store: BUG: variable "
                  "object is already in a tree store!");
        return;
    }

    expression = 
        gswat_variable_object_get_expression(variable_object);
    value = gswat_variable_object_get_value(variable_object, NULL);

    gtk_tree_store_append(variable_store, &iter, parent);

    gtk_tree_store_set(variable_store, &iter,
                       VARIABLE_NAME_COL,
                       expression,
                       VARIABLE_VALUE_COL,
                       value,
                       VARIABLE_OBJECT_COL,
                       G_OBJECT(variable_object),
                       -1);
    g_object_set_data(G_OBJECT(variable_object),
                      "gswat_tree_store",
                      variable_store);

    path = gtk_tree_model_get_path(GTK_TREE_MODEL(variable_store), &iter);
    row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(variable_store), path);
    gtk_tree_path_free(path);
    g_assert(gtk_tree_row_reference_get_path(row_ref)!=NULL);
    g_object_set_data(G_OBJECT(variable_object),
                      "gswat_row_reference",
                      row_ref);

    g_signal_connect(variable_object,
                     "notify::value",
                     G_CALLBACK(on_variable_object_value_notify),
                     NULL
                    );

    g_signal_connect(variable_object,
                     "notify::valid",
                     G_CALLBACK(on_variable_object_invalidated),
                     NULL
                    );

    child_count = 
        gswat_variable_object_get_child_count(variable_object);

    g_signal_connect(variable_object,
                     "notify::child-count",
                     G_CALLBACK(on_variable_object_child_count_notify),
                     NULL
                    );
    if(child_count)
    {
        gtk_tree_store_append(variable_store, &child, &iter);
        gtk_tree_store_set(variable_store, &child,
                           VARIABLE_NAME_COL,
                           "Retrieving Children...",
                           VARIABLE_VALUE_COL,
                           "",
                           VARIABLE_OBJECT_COL,
                           NULL,
                           -1);
    }

    g_free(expression);
    g_free(value);

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
                           VARIABLE_OBJECT_COL,
                           &current_variable_object,
                           -1);

        if(g_list_find(variables, current_variable_object))
        {
            g_object_unref(G_OBJECT(current_variable_object));
            continue;
        }
        /* else: mark this tree store entry for removal */

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
on_variable_object_value_notify(GObject *object,
                                GParamSpec *property,
                                gpointer data)
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
        g_warning("on_variable_object_invalidated: no tree store for variable");
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
                       VARIABLE_VALUE_COL,
                       value,
                       -1);
    g_free(value);
}

static void
on_variable_object_invalidated(GObject *object,
                               GParamSpec *property,
                               gpointer data)
{
    GSwatVariableObject *variable_object;
    GtkTreeStore *store;
    GtkTreeIter iter;

    variable_object = GSWAT_VARIABLE_OBJECT(object);
    g_message("on_variable_object_invalidated");
    store = g_object_get_data(object, "gswat_tree_store");
    if(!store)
    {
        g_warning("on_variable_object_invalidated: no tree store for variable");
        return;
    }

    if(search_for_variable_store_entry(store,
                                       variable_object,
                                       &iter))
    {
        GtkTreeRowReference *row_ref;

        row_ref = g_object_get_data(G_OBJECT(variable_object),
                                    "gswat_row_reference");
        gtk_tree_row_reference_free(row_ref);

        gtk_tree_store_remove(store, &iter);
    }
}


static void
on_variable_object_child_count_notify(GObject *object,
                                      GParamSpec *property,
                                      gpointer data)
{
    GSwatVariableObject *variable_object;
    GtkTreeStore *store;
    GtkTreeIter iter, child;
    GList *tmp;
    GList *row_refs;
    GtkTreeRowReference *row_ref;
    GtkTreePath *path;
    guint child_count;


    variable_object = GSWAT_VARIABLE_OBJECT(object);
    g_message("on_variable_object_invalidated");
    store = g_object_get_data(object, "gswat_tree_store");
    if(!store)
    {
        g_warning("on_variable_object_child_count_notify: no tree store for variable");
        return;
    }

    if(!search_for_variable_store_entry(store,
                                        variable_object,
                                        &iter))
    {
        return;
    }

    /* Deleting multiple rows in one go is a little tricky. We need to
     * take row references before they are deleted */
    row_refs = NULL;
    if(gtk_tree_model_iter_children(GTK_TREE_MODEL(store), &child, &iter))
    {
        /* Get row references */
        do
        {
            path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &child);
            row_ref = gtk_tree_row_reference_new (GTK_TREE_MODEL (store), path);
            row_refs = g_list_prepend (row_refs, row_ref);
            gtk_tree_path_free (path);
        }
        while(gtk_tree_model_iter_next(GTK_TREE_MODEL (store), &child));
    }

    gtk_tree_model_get(GTK_TREE_MODEL(store),
                       &iter,
                       VARIABLE_OBJECT_COL,
                       &variable_object,
                       -1);
    child_count = 
        gswat_variable_object_get_child_count(variable_object);

    if(child_count)
    {
        gtk_tree_store_append(store, &child, &iter);
        gtk_tree_store_set(store, &child,
                           VARIABLE_NAME_COL,
                           _("Retrieving Children..."),
                           VARIABLE_VALUE_COL,
                           "",
                           VARIABLE_OBJECT_COL,
                           NULL,
                           -1);
    }


    /* Delete the referenced rows */
    for(tmp=row_refs; tmp!=NULL; tmp=tmp->next)
    {
        row_ref = tmp->data;
        path = gtk_tree_row_reference_get_path(row_ref);
        if(path)
        {
            gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &child, path);
            gtk_tree_store_remove(store, &child);
            gtk_tree_path_free(path);
        }
        gtk_tree_row_reference_free(row_ref);
    }
    if(row_refs)
    {
        g_list_free(row_refs);
    }

}


static void
on_gswat_debuggable_stack_notify(GObject *object,
                                 GParamSpec *property,
                                 gpointer data)
{
    GSwatControlFlowDebuggingContext *self = 
        GSWAT_CONTROL_FLOW_DEBUGGING_CONTEXT(data);
    GSwatDebuggable *debuggable = GSWAT_DEBUGGABLE(object);
    GList *tmp, *tmp2;
    GSwatDebuggableFrameArgument *arg;
    GtkTreeView *stack_widget=NULL;
    GtkListStore *list_store;
    GtkTreeIter iter;
    GList *disp_stack = NULL;

    gswat_debuggable_stack_free(self->priv->debuggable_stack);
    self->priv->debuggable_stack = gswat_debuggable_get_stack(debuggable);

    /* create new display strings for the stack */
    for(tmp=self->priv->debuggable_stack->head; tmp!=NULL; tmp=tmp->next)
    {
        GSwatDebuggableFrame *frame;
        GSwatControlFlowDebuggingContextFrame *display_frame = g_new0(GSwatControlFlowDebuggingContextFrame, 1);
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

        display_frame->display = display;

        disp_stack = g_list_prepend(disp_stack, display_frame);
    }
    disp_stack = g_list_reverse(disp_stack);


    /* start displaying the new stack frames */
    stack_widget = GTK_TREE_VIEW(self->priv->stack_view);

    list_store = gtk_list_store_new(STACK_N_COLS,
                                    G_TYPE_STRING,
                                    G_TYPE_ULONG);

    for(tmp=disp_stack; tmp!=NULL; tmp=tmp->next)
    {
        GSwatControlFlowDebuggingContextFrame *display_frame = (GSwatControlFlowDebuggingContextFrame *)tmp->data;

        gtk_list_store_append(list_store, &iter);

        gtk_list_store_set(list_store, &iter,
                           STACK_FUNC_COL,
                           display_frame->display->str,
                           -1);

    }

    gtk_tree_view_set_model(stack_widget, GTK_TREE_MODEL(list_store));


    /* free the previously displayed stack */
    free_current_stack_view_entries(self);

    self->priv->stack_list_store = list_store;
    self->priv->display_stack=disp_stack;

}


static void
free_current_stack_view_entries(GSwatControlFlowDebuggingContext *self)
{
    GList *tmp;

    if(self->priv->stack_list_store)
    {
        gtk_list_store_clear(self->priv->stack_list_store);
        g_object_unref(self->priv->stack_list_store);
    }
    self->priv->stack_list_store = NULL;

    /* free the previously displayed stack frames */
    for(tmp=self->priv->display_stack; tmp!=NULL; tmp=tmp->next)
    {
        GSwatControlFlowDebuggingContextFrame *display_frame 
            = (GSwatControlFlowDebuggingContextFrame *)tmp->data;

        g_string_free(display_frame->display, TRUE);

        g_free(display_frame);
    }
    g_list_free(self->priv->display_stack);
    self->priv->display_stack = NULL;
}


static void
on_gswat_debuggable_locals_notify(GObject *object,
                                  GParamSpec *property,
                                  gpointer data)
{
    GSwatControlFlowDebuggingContext *self = 
        GSWAT_CONTROL_FLOW_DEBUGGING_CONTEXT(data);
    update_variable_view(self);
}


static void
on_stack_frame_activated(GtkTreeView *tree_view,
                         GtkTreePath *path,
                         GtkTreeViewColumn *column,
                         gpointer data)
{
    gint path_depth;
    gint *indices;
    GSwatControlFlowDebuggingContext *self = 
        GSWAT_CONTROL_FLOW_DEBUGGING_CONTEXT(data);
    GSwatDebuggableFrame *frame;

    path_depth = gtk_tree_path_get_depth(path);
    g_return_if_fail(path_depth == 1);

    indices = gtk_tree_path_get_indices(path);
    frame = g_queue_peek_nth(self->priv->debuggable_stack, indices[0]);
    g_return_if_fail(frame != NULL);
    g_return_if_fail(frame->level == indices[0]);

    gswat_debuggable_set_frame(GSWAT_DEBUGGABLE(self->priv->debuggable),
                               frame->level);

}


