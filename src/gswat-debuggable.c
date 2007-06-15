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

#include <glib/gi18n.h>

#include "gswat-utils.h"
#include "gswat-debuggable.h"

/* Function definitions */
static void gswat_debuggable_base_init(GSwatDebuggableIface *interface);

/* Macros and defines */

/* Enums */
#if 0
enum {
    SIGNAL_NAME,
	LAST_SIGNAL
};
#endif

/* Variables */
//static guint gswat_debuggable_signals[LAST_SIGNAL] = { 0 };
static guint gswat_debuggable_base_init_count = 0;


GType
gswat_debuggable_get_type(void)
{
    static GType self_type = 0;

    if (!self_type) {
        static const GTypeInfo interface_info = {
            sizeof (GSwatDebuggableIface),
            (GBaseInitFunc)gswat_debuggable_base_init,
            NULL,//gswat_debuggable_base_finalize
        };

        self_type = g_type_register_static(G_TYPE_INTERFACE,
                                           "GSwatDebuggable",
                                           &interface_info, 0);

        g_type_interface_add_prerequisite(self_type, G_TYPE_OBJECT);
        //g_type_interface_add_prerequisite(self_type, G_TYPE_);
    }

    return self_type;
}

static void
gswat_debuggable_base_init(GSwatDebuggableIface *interface)
{
    gswat_debuggable_base_init_count++;
    GParamSpec *new_param;

    if(gswat_debuggable_base_init_count == 1) {

        /* register signals */
#if 0 /* template code */
        interface->signal_member = signal_default_handler;
        gswat_tabable_signals[SIGNAL_NAME] =
            g_signal_new("signal_name", /* name */
                         G_TYPE_FROM_INTERFACE(interface), /* interface GType */
                         G_SIGNAL_RUN_LAST, /* signal flags */
                         G_STRUCT_OFFSET(GSwatTabableIface, signal_member),
                         NULL, /* accumulator */
                         NULL, /* accumulator data */
                         g_cclosure_marshal_VOID__VOID, /* c marshaller */
                         G_TYPE_NONE, /* return type */
                         0 /* number of parameters */
                         /* vararg, list of param types */
                        );
#endif
           
        /* register properties */
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
        g_object_interface_install_property(interface, new_param);
#endif

        new_param = g_param_spec_uint("state",
                                      _("The Running State"),
                                      _("The State of the debuggable"
                                        " (whether or not it is currently running)"),
                                      0,
                                      G_MAXUINT,
                                      GSWAT_DEBUGGABLE_DISCONNECTED,
                                      GSWAT_PARAM_READABLE);
        g_object_interface_install_property(interface, new_param);

        new_param = g_param_spec_pointer("stack",
                                         _("Stack"),
                                         _("The Stack of called functions"),
                                         GSWAT_PARAM_READABLE);
        g_object_interface_install_property(interface, new_param);

        new_param = g_param_spec_pointer("locals",
                                         _("Locals"),
                                         _("The current frames local variables"),
                                         GSWAT_PARAM_READABLE);
        g_object_interface_install_property(interface, new_param);

        new_param = g_param_spec_pointer("breakpoints",
                                         _("Breakpoints"),
                                         _("The list of user defined breakpoints"),
                                         GSWAT_PARAM_READABLE);
        g_object_interface_install_property(interface, new_param);

        new_param = g_param_spec_string("source-uri",
                                        _("Source URI"),
                                        _("The location for the source file currently being debuggged"),
                                        NULL,
                                        GSWAT_PARAM_READABLE);
        g_object_interface_install_property(interface, new_param);
        
        new_param = g_param_spec_ulong("source-line",
                                       _("Source Line"),
                                       _("The line of your source file currently being debuggged"),
                                       0,
                                       G_MAXULONG,
                                       0,
                                       GSWAT_PARAM_READABLE);
        g_object_interface_install_property(interface, new_param);
        
        new_param = g_param_spec_uint("frame",
                                      _("Active Frame"),
                                      _("The debuggable's currently active stack frame"),
                                      0,
                                      G_MAXUINT,
                                      0,
                                      GSWAT_PARAM_READWRITE);
        g_object_interface_install_property(interface, new_param);

    }
}

#if 0
static void
gswat_debuggable_base_finalize(gpointer g_class)
{
    if(gswat_debuggable_base_init_count == 0)
    {

    }
}
#endif

/* Interface functions */

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

/**
 * gswat_debuggable_target_connect:
 * @object: The debuggable that should connect to it's target
 *
 * This should make the debuggable object connect to
 * its target and marks the start of a debug session.
 *
 * Returns: nothing
 */
/* FIXME - there should a synchronous and asynchronous
 * way to connect the debugger to its target.
 * GUIs should use the asychronous and we should use a
 * signal to reflect completion. Simple script bindings
 * might prefer to use a synchronous connect.
 */
void
gswat_debuggable_target_connect(GSwatDebuggable* object)
{
    GSwatDebuggableIface *debuggable;

    g_return_if_fail(GSWAT_IS_DEBUGGABLE(object));
    debuggable = GSWAT_DEBUGGABLE_GET_IFACE(object);

    g_object_ref(object);
    if(gswat_debuggable_get_state(object) == GSWAT_DEBUGGABLE_DISCONNECTED)
    {
        debuggable->target_connect(object);
    }
    g_object_unref(object);
}

void
gswat_debuggable_request_line_breakpoint(GSwatDebuggable* object,
                                         gchar *uri,
                                         guint line)
{
    GSwatDebuggableIface *debuggable;

    g_return_if_fail(GSWAT_IS_DEBUGGABLE(object));
    debuggable = GSWAT_DEBUGGABLE_GET_IFACE(object);

    g_object_ref(object);
    if(gswat_debuggable_get_state(object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
        debuggable->request_line_breakpoint(object, uri, line);
    }
    g_object_unref(object);
}

void	    
gswat_debuggable_request_function_breakpoint(GSwatDebuggable* object,
                                             gchar *symbol)
{
    GSwatDebuggableIface *debuggable;

    g_return_if_fail(GSWAT_IS_DEBUGGABLE(object));
    debuggable = GSWAT_DEBUGGABLE_GET_IFACE(object);

    g_object_ref(object);
    if(gswat_debuggable_get_state(object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
        debuggable->request_function_breakpoint(object, symbol);
    }
    g_object_unref(object);
}

gchar *
gswat_debuggable_get_source_uri(GSwatDebuggable* object)
{
    GSwatDebuggableIface *debuggable;
    gchar *ret;

    g_return_val_if_fail(GSWAT_IS_DEBUGGABLE(object), NULL);
    debuggable = GSWAT_DEBUGGABLE_GET_IFACE(object);

    g_object_ref(object);
    if(gswat_debuggable_get_state(object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
        ret = debuggable->get_source_uri(object);
    }
    else
    {
        ret = NULL;
    }
    g_object_unref(object);

    return ret;
}

gint
gswat_debuggable_get_source_line(GSwatDebuggable* object)
{
    GSwatDebuggableIface *debuggable;
    gulong ret;

    g_return_val_if_fail(GSWAT_IS_DEBUGGABLE(object), 0);
    debuggable = GSWAT_DEBUGGABLE_GET_IFACE(object);

    g_object_ref(object);
    if(gswat_debuggable_get_state(object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
        ret = debuggable->get_source_line(object);
    }
    else
    {
        ret = 0;
    }
    g_object_unref(object);

    return ret;
}

void
gswat_debuggable_continue(GSwatDebuggable* object)
{
    GSwatDebuggableIface *debuggable;

    g_return_if_fail(GSWAT_IS_DEBUGGABLE(object));
    debuggable = GSWAT_DEBUGGABLE_GET_IFACE(object);

    g_object_ref(object);
    if(gswat_debuggable_get_state(object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
        debuggable->cont(object);
    }
    g_object_unref(object);
}

void
gswat_debuggable_finish(GSwatDebuggable* object)
{
    GSwatDebuggableIface *debuggable;

    g_return_if_fail(GSWAT_IS_DEBUGGABLE(object));
    debuggable = GSWAT_DEBUGGABLE_GET_IFACE(object);

    g_object_ref(object);
    if(gswat_debuggable_get_state(object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
        debuggable->finish(object);
    }
    g_object_unref(object);
}

void
gswat_debuggable_next(GSwatDebuggable* object)
{
    GSwatDebuggableIface *debuggable;

    g_return_if_fail(GSWAT_IS_DEBUGGABLE(object));
    debuggable = GSWAT_DEBUGGABLE_GET_IFACE(object);

    g_object_ref(object);
    if(gswat_debuggable_get_state(object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
        debuggable->next(object);
    }
    g_object_unref(object);
}

void
gswat_debuggable_step_into(GSwatDebuggable* object)
{
    GSwatDebuggableIface *debuggable;

    g_return_if_fail(GSWAT_IS_DEBUGGABLE(object));
    debuggable = GSWAT_DEBUGGABLE_GET_IFACE(object);

    g_object_ref(object);
    if(gswat_debuggable_get_state(object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
        debuggable->step_into(object);
    }
    g_object_unref(object);
}

void
gswat_debuggable_interrupt(GSwatDebuggable* object)
{
    GSwatDebuggableIface *debuggable;

    g_return_if_fail(GSWAT_IS_DEBUGGABLE(object));
    debuggable = GSWAT_DEBUGGABLE_GET_IFACE(object);

    g_object_ref(object);
    if(gswat_debuggable_get_state(object) == GSWAT_DEBUGGABLE_RUNNING)
    {
        debuggable->interrupt(object);
    }
    g_object_unref(object);
}

void
gswat_debuggable_restart(GSwatDebuggable* object)
{
    GSwatDebuggableIface *debuggable;

    g_return_if_fail(GSWAT_IS_DEBUGGABLE(object));
    debuggable = GSWAT_DEBUGGABLE_GET_IFACE(object);

    g_object_ref(object);
    if(gswat_debuggable_get_state(object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
        debuggable->restart(object);
    }
    g_object_unref(object);
}

guint
gswat_debuggable_get_state(GSwatDebuggable* object)
{
    GSwatDebuggableIface *debuggable;
    guint ret;

    g_return_val_if_fail(GSWAT_IS_DEBUGGABLE(object), 0);
    debuggable = GSWAT_DEBUGGABLE_GET_IFACE(object);

    g_object_ref(object);
    ret = debuggable->get_state(object);
    g_object_unref(object);

    return ret;
}

guint
gswat_debuggable_get_state_stamp(GSwatDebuggable* object)
{
    GSwatDebuggableIface *debuggable;
    guint ret;

    g_return_val_if_fail(GSWAT_IS_DEBUGGABLE(object), 0);
    debuggable = GSWAT_DEBUGGABLE_GET_IFACE(object);

    g_object_ref(object);
    ret = debuggable->get_state_stamp(object);
    g_object_unref(object);

    return ret;
}

GList *
gswat_debuggable_get_stack(GSwatDebuggable* object)
{
    GSwatDebuggableIface *debuggable;
    GList *ret;

    g_return_val_if_fail(GSWAT_IS_DEBUGGABLE(object), NULL);
    debuggable = GSWAT_DEBUGGABLE_GET_IFACE(object);

    g_object_ref(object);
    if(gswat_debuggable_get_state(object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
        ret = debuggable->get_stack(object);
    }
    else
    {
        ret = NULL;
    }
    g_object_unref(object);

    return ret;
}

void
gswat_debuggable_free_stack(GList *stack)
{
    GList *tmp, *tmp2;

    for(tmp=stack; tmp!=NULL; tmp=tmp->next)
    {
        GSwatDebuggableFrame *current_frame = 
            (GSwatDebuggableFrame *)tmp->data;
        
        g_free(current_frame->function);
        
        for(tmp2=current_frame->arguments; tmp2!=NULL; tmp2=tmp2->next)
        {
            GSwatDebuggableFrameArgument *current_arg = 
                (GSwatDebuggableFrameArgument *)tmp2->data;
            
            g_free(current_arg->name);
            g_free(current_arg->value);
            g_free(current_arg);
        }
        g_list_free(current_frame->arguments);

        g_free(current_frame);
    }
    g_list_free(stack);
    
}


GList *
gswat_debuggable_get_breakpoints(GSwatDebuggable* object)
{
    GSwatDebuggableIface *debuggable;
    GList *ret;

    g_return_val_if_fail(GSWAT_IS_DEBUGGABLE(object), NULL);
    debuggable = GSWAT_DEBUGGABLE_GET_IFACE(object);

    g_object_ref(object);
    if(gswat_debuggable_get_state(object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
        ret = debuggable->get_breakpoints(object);
    }
    else
    {
        ret = NULL;
    }
    g_object_unref(object);

    return ret;
}

void
gswat_debuggable_free_breakpoints(GList *breakpoints)
{
    GList *tmp;

    for(tmp=breakpoints; tmp!=NULL; tmp=tmp->next)
    {
        GSwatDebuggableBreakpoint *current_breakpoint = 
            (GSwatDebuggableBreakpoint *)tmp->data;
        g_free(current_breakpoint->source_uri);
    }
    g_list_free(breakpoints);
}


GList *
gswat_debuggable_get_locals_list(GSwatDebuggable* object)
{
    GSwatDebuggableIface *debuggable;
    GList *ret;

    g_return_val_if_fail(GSWAT_IS_DEBUGGABLE(object), NULL);
    debuggable = GSWAT_DEBUGGABLE_GET_IFACE(object);

    g_object_ref(object);
    if(gswat_debuggable_get_state(object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
        ret = debuggable->get_locals_list(object);
    }
    else
    {
        ret = NULL;
    }
    g_object_unref(object);

    return ret;
}


guint
gswat_debuggable_get_frame(GSwatDebuggable* object)
{
    GSwatDebuggableIface *debuggable;
    guint ret;

    g_return_val_if_fail(GSWAT_IS_DEBUGGABLE(object), 0);
    debuggable = GSWAT_DEBUGGABLE_GET_IFACE(object);

    g_object_ref(object);
    if(gswat_debuggable_get_state(object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
        ret = debuggable->get_frame(object);
    }
    else
    {
        ret = 0;
    }
    g_object_unref(object);
    
    return ret;
}


void
gswat_debuggable_set_frame(GSwatDebuggable* object, guint frame)
{
    GSwatDebuggableIface *debuggable;

    g_return_if_fail(GSWAT_IS_DEBUGGABLE(object));
    debuggable = GSWAT_DEBUGGABLE_GET_IFACE(object);

    g_object_ref(object);
    if(gswat_debuggable_get_state(object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
        debuggable->set_frame(object, frame);
    }
    g_object_unref(object);
}
