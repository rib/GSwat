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

#include "gswat-main-window.h"

#include "gedit-document.h"
#include "gedit-view.h"
#include "gswat-view.h"

#include "gswat-session.h"
#include "gswat-debugger.h"
#include "gswat-variable-object.h"



enum {
  GSWAT_WINDOW_STACK_FUNC_COL,
  GSWAT_WINDOW_STACK_N_COLS
};

enum {
  GSWAT_WINDOW_VARIABLE_NAME_COL,
  GSWAT_WINDOW_VARIABLE_VALUE_COL,
  GSWAT_WINDOW_VARIABLES_N_COLS
};

struct _GSwatWindowPrivate
{
    GladeXML        *xml;

    GSwatDebugger   *debugger;

    GSwatView       *source_view;
    GeditDocument   *source_document;

    GtkListStore    *stack_list_store;
    GList           *stack;

    GtkTreeStore    *variables_tree_store;
    GList           *variables;


    //GSwatSession *session;
    //GtkSourceBuffer *source_buffer;
    //GtkSourceView *source_view;

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
static void on_gswat_window_add_break_button_clicked(GtkToolButton   *toolbutton,
                                               gpointer         data);
static void on_debug_button_clicked(GtkToolButton   *toolbutton,
                                    gpointer         data);

static void on_gswat_session_edit_done(GSwatSession *session, GSwatWindow *window);
//static void on_gswat_session_edit_abort(GSwatSession *session);

static void gswat_window_set_toolbar_state(GSwatWindow *self, guint state);

static void gswat_window_update_state(GObject *debugger,
                                    GParamSpec *property,
                                    gpointer data);

static void gswat_window_update_stack(GObject *debugger,
                                    GParamSpec *property,
                                    gpointer data);

static gboolean gswat_window_update_variable_view(gpointer data);

static void gswat_window_update_source_file(GObject *debugger,
                                          GParamSpec *property,
                                          gpointer data);
static void gswat_window_update_source_line(GObject *debugger,
                                          GParamSpec *property,
                                          gpointer data);

static void gswat_window_source_file_loaded(GeditDocument *document,
                                          const GError  *error,
                                          void *user_data);


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
    //GladeXML *xml;
    GnomeApp *main_window = NULL;
    GtkWidget *button;
    GtkWidget *item;
    GtkTreeView *stack_widget;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeView *variable_view;

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

    gtk_widget_show(GTK_WIDGET(main_window));
    
    //XXX
    //create_sourceview(self);
    /* FIXME
    toplevel_win = gdk_window_get_toplevel(//blah);
    */
    //g_object_set_data(G_OBJECT(main_window), "gedit_notebook", notebook);
    

    gswat_window_set_toolbar_state(self, GSWAT_DEBUGGER_NOT_RUNNING);


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



#if 0
    /* load the connect window */
    gswat_connect_window_xml = glade_xml_new(glade_file, "gswat_connect_window", NULL);

    /* in case we can't load the interface, bail */
    if(!gswat_connect_window_xml) {
        g_warning("We could not load the interface!");
        gtk_main_quit();
    } 

    glade_xml_signal_autoconnect(gswat_connect_window_xml);
#endif

    
}


static void
gswat_window_finalize(GObject *object)
{
    GSwatWindow *window;

    window = GSWAT_WINDOW(object);


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
    GtkWidget *main_scroll;
    //int file;

    //source_buffer = gtk_source_buffer_new(NULL);

    /* Create a GtkSourceView in the main pane */
    //source_view = GTK_SOURCE_VIEW(gtk_source_view_new_with_buffer(source_buffer));

    //gtk_text_buffer_set_text (GTK_TEXT_BUFFER(source_buffer), "Hello, this is some text", -1);

    main_scroll = glade_xml_get_widget(
                                       self->priv->xml,
                                       "gswat_window_scrolled_window"
                                      );

    //gtk_container_add(rTK_CONTAINER(main_scroll), GTK_WIDGET(source_view));
    //gtk_widget_show(GTK_WIDGET(source_view));

    self->priv->source_document = gedit_document_new();
    //source_view = gedit_view_new(source_document);

    //XXX
    self->priv->source_view = GSWAT_VIEW(
                    gswat_view_new(self->priv->source_document, self->priv->debugger)
                  );

    g_signal_connect(self->priv->source_document,
                     "loaded",
                     G_CALLBACK(gswat_window_source_file_loaded),
                     self);

    gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(self->priv->source_view), TRUE);
    gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(self->priv->source_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(self->priv->source_view), FALSE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(self->priv->source_view), FALSE);

    gtk_container_add(GTK_CONTAINER (main_scroll), GTK_WIDGET(self->priv->source_view));


}



static void
destroy_sourceview(GSwatWindow *self)
{
    gtk_widget_destroy(GTK_WIDGET(self->priv->source_view));

    g_object_unref(self->priv->source_document);
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
    if(window->priv->debugger)
    {
        g_object_unref(window->priv->debugger);
        destroy_sourceview(window);
    }

    window->priv->debugger = gswat_debugger_new(session);

    /* currently we don't require a handle to the session
     * from this class..
     */
    g_object_unref(session);

    create_sourceview(window);

    g_signal_connect(G_OBJECT(window->priv->debugger),
                     "notify::state",
                     G_CALLBACK(gswat_window_update_state),
                     window);

    g_signal_connect(G_OBJECT(window->priv->debugger),
                     "notify::stack",
                     G_CALLBACK(gswat_window_update_stack),
                     window);

    g_signal_connect(G_OBJECT(window->priv->debugger),
                     "notify::source-uri",
                     G_CALLBACK(gswat_window_update_source_file),
                     window);

    g_signal_connect(G_OBJECT(window->priv->debugger),
                     "notify::source-line",
                    G_CALLBACK(gswat_window_update_source_line),
                     window);

    g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
                    gswat_window_update_variable_view,
                    window,
                    NULL);

    gswat_debugger_target_connect(window->priv->debugger);

}


static void
gswat_window_set_toolbar_state(GSwatWindow *self, guint state)
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
gswat_window_update_state(GObject *object,
                          GParamSpec *property,
                          gpointer data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    GSwatDebugger *debugger = GSWAT_DEBUGGER(object);
    guint state;

    state = gswat_debugger_get_state(debugger);

    gswat_window_set_toolbar_state(self, state);
}



static void
gswat_window_update_stack(GObject *object,
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
gswat_window_update_variable_view(gpointer data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    GSwatDebugger *debugger = GSWAT_DEBUGGER(self->priv->debugger);
    GladeXML *xml;
    GtkTreeView *variables_view=NULL;
    GtkTreeStore *variables_store;
    GList *variables, *tmp;
    GtkTreeIter iter;
    GList *display_variables = NULL;


    variables = gswat_debugger_get_locals_list(debugger);

    /* TODO look up the variable tree view widget */
    xml = self->priv->xml;
    variables_view = (GtkTreeView *)glade_xml_get_widget(xml, "gswat_window_variable_view");



    variables_store = gtk_tree_store_new(GSWAT_WINDOW_VARIABLES_N_COLS,
                                         G_TYPE_STRING,
                                         G_TYPE_STRING);



    /* create new display strings for the stack */
    for(tmp=variables; tmp!=NULL; tmp=tmp->next)
    {
        GString *name = g_string_new("");
        GString *value = g_string_new("");
        GSwatVariableObject *variable_object;
        GSwatWindowVariable *display_variable = g_new0(GSwatWindowVariable, 1);

        variable_object = (GSwatVariableObject *)tmp->data;
        g_object_ref(variable_object);

        name = g_string_append(name,
                               gswat_variable_object_get_expression(
                                                                    variable_object)
                              );

        value = g_string_append(value,
                                gswat_variable_object_get_value(
                                                                variable_object)
                               );

        display_variable->variable_object = variable_object;
        display_variable->name = name;
        display_variable->value = value;

        display_variables = g_list_prepend(display_variables, display_variable);
    }
    display_variables = g_list_reverse(display_variables);

    /* start displaying the new stack frames */
    for(tmp=display_variables; tmp!=NULL; tmp=tmp->next)
    {
        GSwatWindowVariable *display_variable = 
            (GSwatWindowVariable *)tmp->data;

        gtk_tree_store_append(variables_store, &iter, NULL);
        gtk_tree_store_set(variables_store, &iter,
                           GSWAT_WINDOW_VARIABLE_NAME_COL,
                           display_variable->name->str,
                           GSWAT_WINDOW_VARIABLE_VALUE_COL,
                           display_variable->value->str,
                           -1);
    }
    if(self->priv->variables_tree_store)
    {
        g_object_unref(self->priv->variables_tree_store);
    }
    self->priv->variables_tree_store = variables_store;

    gtk_tree_view_set_model(variables_view,
                            GTK_TREE_MODEL(self->priv->variables_tree_store));



    /* free the previously displayed stack frames */
    for(tmp=self->priv->variables; tmp!=NULL; tmp=tmp->next)
    {
        GSwatWindowVariable *display_variable = 
            (GSwatWindowVariable *)tmp->data;

        g_string_free(display_variable->name, TRUE);
        g_string_free(display_variable->value, TRUE);
        g_object_unref(display_variable->variable_object);
        g_free(display_variable);
    }
    g_list_free(self->priv->variables);
    self->priv->variables=display_variables;
    

    return TRUE;
}



static void
gswat_window_source_file_loaded(GeditDocument *document,
                                const GError  *error,
                                void *user_data)
{

    return;
}



static void
gswat_window_update_source_file(GObject *debugger,
                                GParamSpec *property,
                                gpointer data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gchar * file_uri;
    gint line;

    /* FIXME - we should have a main_window object to hold this */
    //GtkTextIter iter;


    file_uri = gswat_debugger_get_source_uri(GSWAT_DEBUGGER(debugger));

    line = gswat_debugger_get_source_line(GSWAT_DEBUGGER(debugger));

    g_message("gswat_window_update_source_file %s, line=%d", file_uri, line);
    gedit_document_load(self->priv->source_document, file_uri, NULL, line, FALSE);


#if 0
    line = gswat_debugger_get_source_line(GSWAT_DEBUGGER(debugger));

    gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (source_document),
                                      &iter,
                                      2);
    gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (source_document), &iter);
#endif

}



static void
gswat_window_update_source_line(GObject *debugger,
                                GParamSpec *property,
                                gpointer data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    gulong line;
    GtkTextIter iter;

    line = gswat_debugger_get_source_line(GSWAT_DEBUGGER(debugger));

    g_message("gswat_window_update_source_line %lu\n", line);

    gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER (self->priv->source_document),
                                     &iter,
                                     (line-1));


    gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (self->priv->source_document), &iter);

#if 0
    if(!gedit_document_goto_line(source_document, line))
    {
        g_message("Failed to update line");
    }
    else
    {
        g_message("Main line update\n");
    }
#endif
    //gedit_document_load(source_document, file_uri, NULL, 0, FALSE);

}

void
on_debug_button_clicked(GtkToolButton   *toolbutton,
                        gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    GtkTextIter iter;

    gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (self->priv->source_document),
                                      &iter,
                                      2);


    gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (self->priv->source_document), &iter);


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


void
on_gswat_window_add_break_button_clicked(GtkToolButton   *toolbutton,
                                         gpointer         data)
{
    GSwatWindow *self = GSWAT_WINDOW(data);
    GSwatView *source_view = NULL;
    GtkTextBuffer *source_buffer = NULL;
    GtkTextIter cur;
    gint line; 


    source_view = self->priv->source_view;
    g_assert(source_view != NULL);

    source_buffer = GTK_TEXT_VIEW(source_view)->buffer;


    gchar *uri = gswat_debugger_get_source_uri(self->priv->debugger);


    gtk_text_buffer_get_iter_at_mark(
                                     source_buffer, 
                                     &cur, 
                                     gtk_text_buffer_get_insert(source_buffer)
                                    );
    line = gtk_text_iter_get_line(&cur);

    gswat_debugger_request_line_breakpoint(self->priv->debugger, uri, line);
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





