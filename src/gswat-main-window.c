/*
 * <preamble>
 * gswat - A graphical program debugger for Gnome
 * Copyright (C) 2006  Robert Bragg
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
 * </preamble>
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

#include "gswat-notebook.h"
#include "gswat-tab.h"
#include "gswat-view.h"
#include "gswat-session.h"
#include "gswat-debugger.h"
#include "gswat-variable-object.h"
#include "gswat-main-window.h"

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

    GSwatDebugger   *debugger;

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
    GSwatDebuggerFrame *dbg_frame;
    GString *display;
}GSwatWindowFrame;

typedef struct {
    GSwatVariableObject *variable_object;
    GString *name;
    GString *value;
}GSwatWindowVariable;

G_DEFINE_TYPE(GSwatWindow, gswat_window, G_TYPE_OBJECT);




static void gswat_window_class_init(GSwatWindowClass *klass);
static void gswat_window_init(GSwatWindow *self);
static void gswat_window_finalize(GObject *object);

static void create_sourceview(GSwatWindow *self);
static void destroy_sourceview(GSwatWindow *self);
static void on_gswat_window_new_button_clicked(GtkWidget       *widget,
                                               gpointer         data);
static void on_gswat_window_step_button_clicked(GtkToolButton   *toolbutton,
                                                gpointer         data);
static void on_gswat_window_next_button_clicked(GtkToolButton   *toolbutton,
                                                gpointer         data);
static void on_gswat_window_continue_button_clicked(GtkToolButton   *toolbutton,
                                                    gpointer         data);
static void on_gswat_window_finish_button_clicked(GtkToolButton   *toolbutton,
                                                  gpointer         data);
static void on_gswat_window_break_button_clicked(GtkToolButton   *toolbutton,
                                                 gpointer         data);
static void on_gswat_window_run_button_clicked(GtkToolButton   *toolbutton,
                                               gpointer         data);
static void update_line_highlights(GSwatWindow *self);
static void on_gswat_window_add_break_button_clicked(GtkToolButton   *toolbutton,
                                                     gpointer         data);
static void on_variable_view_row_expanded(GtkTreeView *view,
                                          GtkTreeIter *iter,
                                          GtkTreePath *iter_path,
                                          gpointer    data);

static void on_debug_button_clicked(GtkToolButton   *toolbutton,
                                    gpointer         data);

static void on_gswat_session_edit_done(GSwatSession *session, GSwatWindow *window);
//static void on_gswat_session_edit_abort(GSwatSession *session);

static void set_toolbar_state(GSwatWindow *self, guint state);

static void on_gswat_debugger_state_notify(GObject *debugger,
                                           GParamSpec *property,
                                           gpointer data);

static void on_gswat_debugger_stack_notify(GObject *debugger,
                                           GParamSpec *property,
                                           gpointer data);

static gboolean update_variable_view(gpointer data);

static void on_gswat_debugger_source_uri_notify(GObject *debugger,
                                                GParamSpec *property,
                                                gpointer data);
static void on_gswat_debugger_source_line_notify(GObject *debugger,
                                                 GParamSpec *property,
                                                 gpointer data);
static void update_source_view(GSwatWindow *self,
                               GSwatDebugger *debugger);
#if 0
static void on_gswat_window_source_file_loaded(GeditDocument *document,
                                               const GError  *error,
                                               void *user_data);
#endif

static GSwatView *get_current_view(GSwatWindow *self);


static void
gswat_window_class_init(GSwatWindowClass *klass)
{
    GObjectClass     *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = gswat_window_finalize;


    g_type_class_add_private(klass, sizeof(GSwatWindowPrivate));

}



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

    self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, GSWAT_TYPE_WINDOW, GSwatWindowPrivate);

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

    button = glade_xml_get_widget(self->priv->xml, "gswat_window_step_button");
    self->priv->step_button = button;
    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(on_gswat_window_step_button_clicked),
                     self
                    );

    button = glade_xml_get_widget(self->priv->xml, "gswat_window_next_button");
    self->priv->next_button = button;
    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(on_gswat_window_next_button_clicked),
                     self
                    );

    button = glade_xml_get_widget(self->priv->xml, "gswat_window_continue_button");
    self->priv->continue_button = button;
    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(on_gswat_window_continue_button_clicked),
                     self
                    );

    button = glade_xml_get_widget(self->priv->xml, "gswat_window_finish_button");
    self->priv->finish_button = button;
    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(on_gswat_window_finish_button_clicked),
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
                     G_CALLBACK(on_debug_button_clicked),
                     self
                    );


    main_window = GNOME_APP(
                            glade_xml_get_widget(self->priv->xml, "gswat_main_window")
                           );

    if(!main_window)
    {
        g_critical("Could not find main window in gswat.glade");
    }


    create_sourceview(self);


    set_toolbar_state(self, GSWAT_DEBUGGER_NOT_RUNNING);


    /* Setup the stack view */
    stack_widget = 
        (GtkTreeView *)glade_xml_get_widget(
                                            self->priv->xml, 
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
        (GtkTreeView *)glade_xml_get_widget(
                                            self->priv->xml, 
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

    g_signal_connect(variable_view, "row_expanded",
                     G_CALLBACK(on_variable_view_row_expanded), self);
    self->priv->variable_view = variable_view;

    variable_store = gtk_tree_store_new(GSWAT_WINDOW_VARIABLES_N_COLS,
                                         G_TYPE_STRING,
                                         G_TYPE_STRING,
                                         G_TYPE_OBJECT);
    gtk_tree_view_set_model(variable_view, GTK_TREE_MODEL(variable_store));
    self->priv->variable_store = variable_store;


    gtk_widget_show(GTK_WIDGET(main_window));

}


static void
gswat_window_finalize(GObject *object)
{
    GSwatWindow *window;

    window = GSWAT_WINDOW(object);

    destroy_sourceview(window);

    G_OBJECT_CLASS(gswat_window_parent_class)->finalize(object);

}



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




#if 0
static void
on_gswat_session_edit_abort(GSwatSession *session)
{

}
#endif




static void
on_gswat_session_edit_done(GSwatSession *session, GSwatWindow *window)
{
    GList *tabs, *tmp;

    if(window->priv->debugger)
    {
        g_object_unref(window->priv->debugger);

        tabs = gtk_container_get_children(GTK_CONTAINER(window->priv->notebook));
        for(tmp=tabs;tmp!=NULL;tmp=tmp->next)
        {
            gtk_widget_destroy(GTK_WIDGET(tmp->data));
        }
        g_list_free(tabs);
    }

    window->priv->debugger = gswat_debugger_new(session);

    /* currently we don't require a handle to the session
     * from this class..
     */
    g_object_unref(session);


    g_signal_connect(G_OBJECT(window->priv->debugger),
                     "notify::state",
                     G_CALLBACK(on_gswat_debugger_state_notify),
                     window);

    g_signal_connect(G_OBJECT(window->priv->debugger),
                     "notify::stack",
                     G_CALLBACK(on_gswat_debugger_stack_notify),
                     window);

    g_signal_connect(G_OBJECT(window->priv->debugger),
                     "notify::source-uri",
                     G_CALLBACK(on_gswat_debugger_source_uri_notify),
                     window);

    g_signal_connect(G_OBJECT(window->priv->debugger),
                     "notify::source-line",
                     G_CALLBACK(on_gswat_debugger_source_line_notify),
                     window);
#if 0
    g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
                    update_variable_view,
                    window,
                    NULL);
#endif

    gswat_debugger_target_connect(window->priv->debugger);
}


static void
set_toolbar_state(GSwatWindow *self, guint state)
{
    switch(state)
    {
        case GSWAT_DEBUGGER_RUNNING:
            g_message("******** setting toolbar state GSWAT_DEBUGGER_RUNNING");
            gtk_widget_set_sensitive(self->priv->step_button, FALSE);
            gtk_widget_set_sensitive(self->priv->next_button, FALSE);
            gtk_widget_set_sensitive(self->priv->continue_button, FALSE);
            gtk_widget_set_sensitive(self->priv->finish_button, FALSE);
            gtk_widget_set_sensitive(self->priv->break_button, TRUE);
            gtk_widget_set_sensitive(self->priv->restart_button, TRUE);
            gtk_widget_set_sensitive(self->priv->add_break_button, FALSE);
            break;
        case GSWAT_DEBUGGER_NOT_RUNNING:
            g_message("******** setting toolbar state GSWAT_DEBUGGER_NOT_RUNNING");
            gtk_widget_set_sensitive(self->priv->step_button, FALSE);
            gtk_widget_set_sensitive(self->priv->next_button, FALSE);
            gtk_widget_set_sensitive(self->priv->continue_button, FALSE);
            gtk_widget_set_sensitive(self->priv->finish_button, FALSE);
            gtk_widget_set_sensitive(self->priv->break_button, FALSE);
            gtk_widget_set_sensitive(self->priv->add_break_button, TRUE);
            break;
        case GSWAT_DEBUGGER_INTERRUPTED:
            g_message("******** setting toolbar state GSWAT_DEBUGGER_INTERRUPTED");
            gtk_widget_set_sensitive(self->priv->step_button, TRUE);
            gtk_widget_set_sensitive(self->priv->next_button, TRUE);
            gtk_widget_set_sensitive(self->priv->continue_button, TRUE);
            gtk_widget_set_sensitive(self->priv->finish_button, TRUE);
            gtk_widget_set_sensitive(self->priv->break_button, FALSE);
            gtk_widget_set_sensitive(self->priv->restart_button, TRUE);
            gtk_widget_set_sensitive(self->priv->add_break_button, TRUE);
            break;
        default:
            g_warning("unexpected debugger state");
            gtk_widget_set_sensitive(self->priv->step_button, FALSE);
            gtk_widget_set_sensitive(self->priv->next_button, FALSE);
            gtk_widget_set_sensitive(self->priv->continue_button, FALSE);
            gtk_widget_set_sensitive(self->priv->finish_button, FALSE);
            gtk_widget_set_sensitive(self->priv->break_button, FALSE);
            gtk_widget_set_sensitive(self->priv->restart_button, FALSE);
            gtk_widget_set_sensitive(self->priv->add_break_button, FALSE);
            break;
    }

}



static void
on_gswat_debugger_state_notify(GObject *object,
                               GParamSpec *property,
                               gpointer data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    GSwatDebugger *debugger = GSWAT_DEBUGGER(object);
    guint state;

    update_variable_view(self);

    state = gswat_debugger_get_state(debugger);

    set_toolbar_state(self, state);

}



static void
on_gswat_debugger_stack_notify(GObject *object,
                               GParamSpec *property,
                               gpointer data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    GSwatDebugger *debugger = GSWAT_DEBUGGER(object);
    GList *stack, *tmp, *tmp2;
    GSwatDebuggerFrameArgument *arg;
    GladeXML *xml;
    GtkTreeView *stack_widget=NULL;
    GtkListStore *list_store;
    GtkTreeIter iter;
    GList *disp_stack = NULL;

    stack = gswat_debugger_get_stack(debugger);

    /* TODO look up the stack tree view widget */
    xml = self->priv->xml;
    stack_widget = (GtkTreeView *)glade_xml_get_widget(xml, "gswat_window_stack_widget");


    list_store = gtk_list_store_new(GSWAT_WINDOW_STACK_N_COLS,
                                    G_TYPE_STRING,
                                    G_TYPE_ULONG);



    /* create new display strings for the stack */
    for(tmp=stack; tmp!=NULL; tmp=tmp->next)
    {
        GSwatDebuggerFrame *frame;
        GSwatWindowFrame *display_frame = g_new0(GSwatWindowFrame, 1);
        GString *display = g_string_new("");

        frame = (GSwatDebuggerFrame *)tmp->data;

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
            arg = (GSwatDebuggerFrameArgument *)tmp2->data;


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
            arg = (GSwatDebuggerFrameArgument *)tmp2->data;

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
        
        
        g_object_unref(G_OBJECT(current_variable_object));

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


static gboolean
update_variable_view(gpointer data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    GSwatDebugger *debugger = self->priv->debugger;
    GList *variables, *tmp;
    GtkTreeStore *variable_store;
    
    variables = gswat_debugger_get_locals_list(debugger);
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


#if 0
static void
on_gswat_window_source_file_loaded(GeditDocument *document,
                                   const GError  *error,
                                   void *user_data)
{

    return;
}
#endif



static void
on_gswat_debugger_source_uri_notify(GObject *debugger,
                                    GParamSpec *property,
                                    gpointer data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    update_source_view(self, GSWAT_DEBUGGER(debugger));
}



static void
on_gswat_debugger_source_line_notify(GObject *debugger,
                                     GParamSpec *property,
                                     gpointer data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    update_source_view(self, GSWAT_DEBUGGER(debugger));
}

/* FIXME, this seems fairly heavy weight to happen every
 * time you step a single line while debugging
 */
static void
update_source_view(GSwatWindow *self,
                   GSwatDebugger *debugger)
{
    gchar           *file_uri;
    gint            line;
    GList           *tabs, *tmp;
    //GeditDocument   *source_document;
    GSwatView       *gswat_view;
    GeditDocument   *gedit_document;
    GtkTextIter     iter;
    GtkWidget       *new_tab;

    file_uri = gswat_debugger_get_source_uri(debugger);
    line = gswat_debugger_get_source_line(debugger);

    g_message("gswat-window update_source_view file=%s, line=%d", file_uri, line);

    tabs = gtk_container_get_children(GTK_CONTAINER(self->priv->notebook));

    for(tmp=tabs; tmp!=NULL; tmp=tmp->next)
    {
        GSwatTab *current_tab;
        GSwatView *current_view;
        GeditDocument *current_document;


        char *uri;
        if(!GSWAT_IS_TAB(tmp->data))
        {
            continue;
        }

        current_tab = GSWAT_TAB(tmp->data);

        /* FIXME I don't think tabs should care that they
         * contain a GSwatView specificlly */
        current_view = gswat_tab_get_view(current_tab);

        current_document = gswat_view_get_document(current_view);
        uri = gedit_document_get_uri(current_document);
        if(strcmp(uri, file_uri)==0)
        {
            gint page;

            page = gtk_notebook_page_num(GTK_NOTEBOOK(self->priv->notebook),
                                         GTK_WIDGET(current_tab));
            gtk_notebook_set_current_page(GTK_NOTEBOOK(self->priv->notebook),
                                          page);

            gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(current_document),
                                             &iter,
                                             line);
            gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(current_view),
                                         &iter,
                                         0.2,
                                         FALSE,
                                         0,
                                         0);
            //gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(gedit_document), &iter);

            g_free(uri);
            g_list_free(tabs);
            goto update_highlights;
        }
        g_free(uri);
    }

    g_list_free(tabs);

    /* If we don't already have a tab with the right file
     * open ...
     */

    new_tab = _gswat_tab_new_from_uri(file_uri,
                                      NULL,
                                      line,			
                                      FALSE);
    gtk_widget_show(new_tab);
    gswat_notebook_add_tab(self->priv->notebook,
                           GSWAT_TAB(new_tab),
                           -1,
                           TRUE);

    gswat_view = gswat_tab_get_view(GSWAT_TAB(new_tab));
    g_assert(GSWAT_IS_VIEW(gswat_view));

    gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(gswat_view), TRUE);
    gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(gswat_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(gswat_view), FALSE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(gswat_view), FALSE);


    gedit_document = gswat_view_get_document(gswat_view);
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

update_highlights:

    update_line_highlights(self);

    return;


}



static void
on_variable_view_row_expanded(GtkTreeView *view,
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
on_debug_button_clicked(GtkToolButton   *toolbutton,
                        gpointer         data)
{
#if 0
    GSwatWindow *self = GSWAT_WINDOW(data);
#endif
}




void
on_gswat_window_step_button_clicked(GtkToolButton   *toolbutton,
                                    gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gswat_debugger_step_into(self->priv->debugger);
}


void
on_gswat_window_next_button_clicked(GtkToolButton   *toolbutton,
                                    gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gswat_debugger_next(self->priv->debugger);
}


void
on_gswat_window_break_button_clicked(GtkToolButton   *toolbutton,
                                     gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gswat_debugger_interrupt(self->priv->debugger);
}

void
on_gswat_window_continue_button_clicked  (GtkToolButton   *toolbutton,
                                          gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gswat_debugger_continue(self->priv->debugger);
}


void
on_gswat_window_finish_button_clicked(GtkToolButton   *toolbutton,
                                      gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gswat_debugger_finish(self->priv->debugger);
}


void
on_gswat_window_run_button_clicked(GtkToolButton   *toolbutton,
                                   gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gswat_debugger_restart(self->priv->debugger);
}


static void
update_line_highlights(GSwatWindow *self)
{
    /* request all debugger breakpoints */
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

    breakpoints = gswat_debugger_get_breakpoints(self->priv->debugger);

    tabs = gtk_container_get_children(GTK_CONTAINER(self->priv->notebook));
    for(tmp=tabs;tmp!=NULL;tmp=tmp->next)
    {
        GSwatTab *current_tab;
        GSwatView *current_gswat_view;

        if(!GSWAT_IS_TAB(tmp->data))
        {
            continue;
        }
        current_tab = (GSwatTab *)tmp->data;

        /* FIXME I don't think tabs should care that they
         * contain a GSwatView specificlly */
        current_gswat_view = gswat_tab_get_view(current_tab);


        current_doc_uri = gedit_document_get_uri(
                                                 gswat_view_get_document(current_gswat_view)
                                                );

        breakpoint_color = gedit_prefs_manager_get_breakpoint_bg_color();

        /* generate a list of breakpount lines to highlight in this view */
        for(tmp2=breakpoints; tmp2 != NULL; tmp2=tmp2->next)
        {
            GSwatDebuggerBreakpoint *current_breakpoint;

            current_breakpoint = (GSwatDebuggerBreakpoint *)tmp2->data;
            if(strcmp(current_breakpoint->source_uri, current_doc_uri)==0)
            {
                highlight = g_new(GSwatViewLineHighlight, 1);
                highlight->line = current_breakpoint->line;
                highlight->color = breakpoint_color;
                highlights=g_list_prepend(highlights, highlight);
            }
        }

        /* add a line to highlight the current line */
        if(strcmp(gswat_debugger_get_source_uri(self->priv->debugger),
                  current_doc_uri) == 0)
        {
            highlight = g_new(GSwatViewLineHighlight, 1);
            highlight->line = gswat_debugger_get_source_line(self->priv->debugger);
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



void
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
    g_assert(source_view != NULL);

    source_buffer = GTK_TEXT_VIEW(source_view)->buffer;

    current_document = gswat_view_get_document(source_view);
    uri = gedit_document_get_uri(current_document); 

    gtk_text_buffer_get_iter_at_mark(
                                     source_buffer, 
                                     &cur, 
                                     gtk_text_buffer_get_insert(source_buffer)
                                    );
    line = gtk_text_iter_get_line(&cur);

    gswat_debugger_request_line_breakpoint(self->priv->debugger, uri, line + 1);

    /* FIXME this should be triggered by a signal */
    update_line_highlights(self);
}

void
gswat_window_on_preferences_activate(GtkMenuItem     *menuitem,
                                     gpointer         data)
{
    gedit_show_preferences_dialog();
}


void
gswat_window_on_about_activate(GtkMenuItem     *menuitem,
                               gpointer         user_data)
{

}


void
gswat_window_on_quit_activate(GtkMenuItem     *menuitem,
                              gpointer         data)
{
    gtk_main_quit();
}


static GSwatView *
get_current_view(GSwatWindow *self)
{
    GtkNotebook *notebook = GTK_NOTEBOOK(self->priv->notebook);
    GtkWidget *current_page;

    current_page = gtk_notebook_get_nth_page(
                                             notebook,
                                             gtk_notebook_get_current_page(notebook)
                                            );
    return gswat_tab_get_view(GSWAT_TAB(current_page));
}



