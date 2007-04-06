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

#include <glib/gi18n.h>

#include "gswat-gdb-debugger.h"
#include "gswat-gdb-variable-object.h"

/* Function definitions */
static void gswat_gdb_variable_object_class_init(GSwatGdbVariableObjectClass *klass);
static void gswat_gdb_variable_object_get_property(GObject *object,
                                                   guint id,
                                                   GValue *value,
                                                   GParamSpec *pspec);
static void gswat_gdb_variable_object_set_property(GObject *object,
                                                   guint property_id,
                                                   const GValue *value,
                                                   GParamSpec *pspec);
static void 
gswat_gdb_variable_object_variable_object_interface_init(gpointer interface,
                                                         gpointer data);
static void gswat_gdb_variable_object_init(GSwatGdbVariableObject *self);
static void gswat_gdb_variable_object_finalize(GObject *self);

static void create_gdb_variable_object(GSwatGdbVariableObject *self);
static GSwatGdbVariableObject *
wrap_gdb_variable_object(GSwatGdbDebugger *debugger,
                         const gchar *gdb_name,
                         const gchar *expression,
                         const gchar *cache_value,
                         int frame);
static void destroy_gdb_variable_object(GSwatGdbVariableObject *self);
static gchar *
gswat_gdb_variable_object_get_expression(GSwatVariableObject *self);
static gchar *
gswat_gdb_variable_object_get_value(GSwatVariableObject *self,
                                    GError **error);
static guint
gswat_gdb_variable_object_get_child_count(GSwatVariableObject *self);
static GList *
gswat_gdb_variable_object_get_children(GSwatVariableObject *self);


/* Macros and defines */
#define GSWAT_GDB_VARIABLE_OBJECT_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GSWAT_TYPE_GDB_VARIABLE_OBJECT, GSwatGdbVariableObjectPrivate))

/* Enums */
/* add your signals here */
enum {
    SIG_UPDATED,
    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_EXPRESSION,
    PROP_VALUE,
    PROP_CHILDREN
};

struct _GSwatGdbVariableObjectPrivate
{
    GSwatGdbDebugger   *debugger;
    guint           debugger_state_stamp;

    gchar           *cached_value;          /* cache of next value to return */
    

    gchar           *expression;
    int             frame;                  /* what frame the expression
                                               must be evaluated in 
                                               (-1 = current) */
    guint           child_count;
    GList           *children;

    /* gdb specific */
    gchar           *gdb_name;
};

/* Variables */
static GObjectClass *parent_class = NULL;
static guint gswat_gdb_variable_object_signals[LAST_SIGNAL] = { 0 };

static int global_variable_object_index = 0;


GType
gswat_gdb_variable_object_get_type(void) /* Typechecking */
{
    static GType self_type = 0;

    if (!self_type)
    {
        static const GTypeInfo gdb_variable_object_info =
        {
            sizeof(GSwatGdbVariableObjectClass), /* class structure size */
            NULL, /* base class initializer */
            NULL, /* base class finalizer */
            (GClassInitFunc)gswat_gdb_variable_object_class_init, /* class initializer */
            NULL, /* class finalizer */
            NULL, /* class data */
            sizeof(GSwatGdbVariableObject), /* instance structure size */
            0, /* preallocated instances */
            (GInstanceInitFunc)gswat_gdb_variable_object_init, /* instance initializer */
            NULL /* function table */
        };

        /* add the type of your parent class here */
        self_type = g_type_register_static(G_TYPE_OBJECT, /* parent GType */
                                           "GSwatGdbVariableObject", /* type name */
                                           &gdb_variable_object_info, /* type info */
                                           0 /* flags */
                                          );

        /* add interfaces here */
        static const GInterfaceInfo variable_object_info =
        {
            (GInterfaceInitFunc)
                gswat_gdb_variable_object_variable_object_interface_init,
            (GInterfaceFinalizeFunc)NULL,
            NULL /* interface data */
        };

        if (self_type != G_TYPE_INVALID) {
            g_type_add_interface_static(self_type,
                                        GSWAT_TYPE_VARIABLE_OBJECT,
                                        &variable_object_info);
        }

    }

    return self_type;
}

/* Class Initialization */
static void
gswat_gdb_variable_object_class_init(GSwatGdbVariableObjectClass *klass)
{   
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GParamSpec *new_param;

    parent_class = g_type_class_peek_parent(klass);

    gobject_class->finalize = gswat_gdb_variable_object_finalize;

    gobject_class->get_property = gswat_gdb_variable_object_get_property;
    gobject_class->set_property = gswat_gdb_variable_object_set_property;

    /* set up properties */
#if 0
    //new_param = g_param_spec_int("name", /* name */
    //new_param = g_param_spec_uint("name", /* name */
    //new_param = g_param_spec_boolean("name", /* name */
    //new_param = g_param_spec_gdb_variable_object("name", /* name */
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
                                     GSWAT_TYPE_PARAM_OBJ, /* GType */
#elif POINTER
                                     /* nothing extra */
#endif
                                     GSWAT_PARAM_READABLE /* flags */
                                     GSWAT_PARAM_WRITEABLE /* flags */
                                     GSWAT_PARAM_READWRITE /* flags */
                                     | G_PARAM_CONSTRUCT
                                     | G_PARAM_CONSTRUCT_ONLY
                                     );
    g_object_class_install_property(gobject_class,
                                    PROP_NAME,
                                    new_param);
#endif
    new_param = g_param_spec_string("expression",
                                    _("The associated expression"),
                                    _("The expression to evaluate in the specifed frame"),
                                    NULL,
                                    G_PARAM_READABLE);
    g_object_class_install_property(gobject_class,
                                    PROP_EXPRESSION,
                                    new_param
                                   );

    new_param = g_param_spec_string("value",
                                    _("The expression value"),
                                    _("The result of evaluating the expression in the specified frame"),
                                    NULL,
                                    G_PARAM_READABLE);
    g_object_class_install_property(gobject_class,
                                    PROP_VALUE,
                                    new_param
                                   );

    new_param = g_param_spec_pointer("children",
                                     _("Children"),
                                     _("The list of chilren for complex data types"),
                                     G_PARAM_READABLE);
    g_object_class_install_property(gobject_class,
                                    PROP_CHILDREN,
                                    new_param
                                   );

    /* set up signals */
#if 0 /* template code */
    klass->signal_member = signal_default_handler;
    gswat_gdb_variable_object_signals[SIGNAL_NAME] =
        g_signal_new("signal_name", /* name */
                     G_TYPE_FROM_CLASS(klass), /* interface GType */
                     G_SIGNAL_RUN_LAST, /* signal flags */
                     G_STRUCT_OFFSET(GSwatGdbVariableObjectClass, signal_member),
                     NULL, /* accumulator */
                     NULL, /* accumulator data */
                     g_cclosure_marshal_VOID__VOID, /* c marshaller */
                     G_TYPE_NONE, /* return type */
                     0, /* number of parameters */
                     /* vararg, list of param types */
                    );
#endif
    gswat_gdb_variable_object_signals[SIG_UPDATED] = 
            g_signal_lookup("updated", GSWAT_TYPE_VARIABLE_OBJECT);

    g_type_class_add_private(klass, sizeof(GSwatGdbVariableObjectPrivate));
}

static void
gswat_gdb_variable_object_get_property(GObject *object,
                                       guint id,
                                       GValue *value,
                                       GParamSpec *pspec)
{
    GSwatGdbVariableObject* self = GSWAT_GDB_VARIABLE_OBJECT(object);
    char *tmp;

    switch(id) {
#if 0 /* template code */
        case PROP_NAME:
            g_value_set_int(value, self->priv->property);
            g_value_set_uint(value, self->priv->property);
            g_value_set_boolean(value, self->priv->property);
            /* don't forget that this will dup the string... */
            g_value_set_string(value, self->priv->property);
            g_value_set_gdb_variable_object(value, self->priv->property);
            g_value_set_pointer(value, self->priv->property);
            break;
#endif
        case PROP_EXPRESSION:
            tmp = gswat_gdb_variable_object_get_expression(self);
            g_value_set_string(value, tmp);
            g_free(tmp);
            break;
        case PROP_VALUE:
            tmp = gswat_gdb_variable_object_get_value(self, NULL);
            g_value_set_string(value, tmp);
            g_free(tmp);
            break;
        case PROP_CHILDREN:
            g_value_set_pointer(value,
                                gswat_gdb_variable_object_get_children(self));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
            break;
    }
}

static void
gswat_gdb_variable_object_set_property(GObject *object,
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
            self->priv->property = g_value_dup_gdb_variable_object(value);
            /* g_free(self->priv->property)? */
            self->priv->property = g_value_get_pointer(value);
            break;
#endif
        default:
            g_warning("gswat_gdb_variable_object_set_property on unknown property");
            return;
    }
}

/* Initialize interfaces here */

static void
gswat_gdb_variable_object_variable_object_interface_init(gpointer interface,
                                                         gpointer data)
{
    GSwatVariableObjectIface *variable_object = interface;
    g_assert(G_TYPE_FROM_INTERFACE(variable_object) == GSWAT_TYPE_VARIABLE_OBJECT);

    variable_object->get_expression = gswat_gdb_variable_object_get_expression;
    variable_object->get_value = gswat_gdb_variable_object_get_value;
    variable_object->get_child_count = gswat_gdb_variable_object_get_child_count;
    variable_object->get_children = gswat_gdb_variable_object_get_children;
}


/* Instance Construction */
static void
gswat_gdb_variable_object_init(GSwatGdbVariableObject *self)
{
    self->priv = GSWAT_GDB_VARIABLE_OBJECT_GET_PRIVATE(self);
    /* populate your widget here */
}

/* Instantiation wrapper */
GSwatGdbVariableObject*
gswat_gdb_variable_object_new(GSwatGdbDebugger *debugger,
                              const gchar *expression,
                              const gchar *cached_value,
                              int frame)
{
    GSwatGdbVariableObject *variable_object;

    variable_object = g_object_new(GSWAT_TYPE_GDB_VARIABLE_OBJECT, NULL);

    variable_object->priv->debugger = debugger;

    variable_object->priv->expression = g_strdup(expression);

    variable_object->priv->cached_value = NULL;

    if(cached_value)
    {
        variable_object->priv->cached_value = g_strdup(cached_value);
    }

    variable_object->priv->frame = frame;

    variable_object->priv->debugger_state_stamp = 
        gswat_debuggable_get_state_stamp(GSWAT_DEBUGGABLE(debugger));

    create_gdb_variable_object(variable_object);

    _gswat_gdb_debugger_register_variable_object(debugger, variable_object);

    return variable_object;
}

/* Instance Destruction */
void
gswat_gdb_variable_object_finalize(GObject *object)
{
    GSwatGdbVariableObject *self = GSWAT_GDB_VARIABLE_OBJECT(object);

    _gswat_gdb_debugger_unregister_variable_object(self->priv->debugger,
                                                   self);

    destroy_gdb_variable_object(self);
    g_free(self->priv->expression);

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
create_gdb_variable_object(GSwatGdbVariableObject *self)
{
    gchar *command;
    gulong token;
    GDBMIValue *val;
    const GDBMIValue *numchild_val;

    /* FIXME we only support expressions that are evaluated
     * in the _current_ frame */
    command = g_strdup_printf("-var-create v%d * %s",
                              global_variable_object_index,
                              //self->priv->frame,
                              self->priv->expression);
    token = gswat_gdb_debugger_send_mi_command(self->priv->debugger, command); 
    g_free(command);

    /* FIXME - make sure we can cope with junk expressions */
    val=gswat_gdb_debugger_get_mi_value(self->priv->debugger, token, NULL);
    gdbmi_value_free(val);

    self->priv->gdb_name = g_strdup_printf("v%d",
                                           global_variable_object_index);

    global_variable_object_index++;

    /* find out if this expression has any children */
    command = g_strdup_printf("-var-info-num-children %s",
                              self->priv->gdb_name);
    token = gswat_gdb_debugger_send_mi_command(self->priv->debugger, command);
    val=gswat_gdb_debugger_get_mi_value(self->priv->debugger, token, NULL);
    numchild_val=gdbmi_value_hash_lookup(val, "numchild");
    if(numchild_val)
    {
        const char *child_count;
        child_count = gdbmi_value_literal_get(numchild_val);
        self->priv->child_count = (guint)strtoul(child_count, NULL, 10);
    }
    else
    {
        self->priv->child_count = 0;
    }
    gdbmi_value_free(val);

    g_free(command);


}


static GSwatGdbVariableObject *
wrap_gdb_variable_object(GSwatGdbDebugger *debugger,
                         const gchar *gdb_name,
                         const gchar *expression,
                         const gchar *cache_value,
                         int frame)
{
    GSwatGdbVariableObject *variable_object;

    variable_object = g_object_new(GSWAT_TYPE_GDB_VARIABLE_OBJECT, NULL);
    variable_object->priv->debugger = debugger;
    variable_object->priv->expression = g_strdup(expression);
    if(cache_value)
    {
        variable_object->priv->cached_value = g_strdup(cache_value);
    }
    else
    {
        variable_object->priv->cached_value = NULL;
    }
    variable_object->priv->frame = frame;
    variable_object->priv->gdb_name = g_strdup(gdb_name);


    variable_object->priv->debugger_state_stamp = 
        gswat_debuggable_get_state_stamp(GSWAT_DEBUGGABLE(debugger));

    return variable_object;
}


static void
destroy_gdb_variable_object(GSwatGdbVariableObject *self)
{
    gchar *command;
    GList *tmp;

    if(!self->priv->gdb_name)
    {
        return;
    }

    command = g_strdup_printf("-var-delete %s",
                              self->priv->gdb_name);
    gswat_gdb_debugger_send_mi_command(self->priv->debugger,
                                 command);
    g_free(command);

    g_free(self->priv->cached_value);
    self->priv->cached_value=NULL;

    for(tmp=self->priv->children; tmp!=NULL; tmp=tmp->next)
    {
        g_object_unref(G_OBJECT(tmp->data));
    }
    g_list_free(self->priv->children);
    self->priv->children=NULL;

    g_free(self->priv->gdb_name);
    self->priv->gdb_name=NULL;
}


char *
gswat_gdb_variable_object_get_name(GSwatGdbVariableObject *self)
{
    return g_strdup(self->priv->gdb_name);
}


void
_gswat_gdb_variable_object_set_cache_value(GSwatGdbVariableObject *self,
                                   const char *value)
{
    if(strcmp(self->priv->cached_value, value) != 0)
    {
        self->priv->cached_value=g_strdup(value);
        g_signal_emit(G_OBJECT(self),
                      gswat_gdb_variable_object_signals[SIG_UPDATED],
                      0);
    }
}


static gchar *
gswat_gdb_variable_object_get_expression(GSwatVariableObject *object)
{
    GSwatGdbVariableObject *self;
    
    g_return_val_if_fail(GSWAT_IS_GDB_VARIABLE_OBJECT(object), NULL);
    self = GSWAT_GDB_VARIABLE_OBJECT(object);

    return g_strdup(self->priv->expression);
}


static gchar *
gswat_gdb_variable_object_get_value(GSwatVariableObject *object,
                                    GError **error)
{
    GSwatGdbVariableObject *self;
    gulong token;
    GDBMIValue *top_val;
    const GDBMIValue *value;
    gchar *command;
    gchar *value_string;
    guint debugger_state_stamp;
    
    g_return_val_if_fail(GSWAT_IS_GDB_VARIABLE_OBJECT(object), NULL);
    self = GSWAT_GDB_VARIABLE_OBJECT(object);

#if 0
    if(!self->priv->gdb_name)
    {
        create_gdb_variable_object(self);
    }
#endif
    g_assert(self->priv->gdb_name);

    debugger_state_stamp = 
        gswat_debuggable_get_state_stamp(GSWAT_DEBUGGABLE(self->priv->debugger));
    if(self->priv->cached_value
       && self->priv->debugger_state_stamp == debugger_state_stamp)
    {
        return g_strdup(self->priv->cached_value);
    }

    command = g_strdup_printf("-var-evaluate-expression %s", 
                              self->priv->gdb_name);

    token = gswat_gdb_debugger_send_mi_command(self->priv->debugger,
                                         command);

    g_free(command);

    top_val = gswat_gdb_debugger_get_mi_value(self->priv->debugger,
                                              token,
                                              NULL);
    if(top_val)
    {
        value = gdbmi_value_hash_lookup(top_val, "value");

        g_assert(value);

        value_string = g_strdup(gdbmi_value_literal_get(value));
    }
    else
    {
        g_warning("Unexpected failure when retrieving variable value");
        value_string = g_strdup("Error retrieving value!");
        g_set_error(error,
                    GSWAT_VARIABLE_OBJECT_ERROR,
                    GSWAT_VARIABLE_OBJECT_ERROR_GET_VALUE_FAILED,
                    "Failed to retrieve the variable object's "
                    "expresion value (%s)",
                    self->priv->expression);
    }


    if(self->priv->cached_value)
    {
        g_free(self->priv->cached_value);
    }
    self->priv->cached_value = g_strdup(value_string);
    self->priv->debugger_state_stamp = debugger_state_stamp;

    gdbmi_value_free(top_val);


    return value_string;
}


static guint
gswat_gdb_variable_object_get_child_count(GSwatVariableObject *object)
{
    GSwatGdbVariableObject *self;
    
    g_return_val_if_fail(GSWAT_IS_GDB_VARIABLE_OBJECT(object), 0);
    self = GSWAT_GDB_VARIABLE_OBJECT(object);

    return self->priv->child_count;
}


static GList *
gswat_gdb_variable_object_get_children(GSwatVariableObject *object)
{
    GSwatGdbVariableObject *self;
    GList *tmp;
    gchar *command;
    gulong token;
    GDBMIValue *top_val;
    const GDBMIValue *children_val, *child_val;
    int n;
    const gchar *child_value;
    GSwatGdbVariableObject *variable_object;
    
    g_return_val_if_fail(GSWAT_IS_GDB_VARIABLE_OBJECT(object), NULL);
    self = GSWAT_GDB_VARIABLE_OBJECT(object);

    if(self->priv->children)
    {
        for(tmp=self->priv->children;tmp!=NULL;tmp=tmp->next)
        {
            g_object_ref(G_OBJECT(tmp->data));
        }
        return g_list_copy(self->priv->children);
    }


    command=g_strdup_printf("-var-list-children --simple-values %s",
                            self->priv->gdb_name);

    token = gswat_gdb_debugger_send_mi_command(self->priv->debugger, command);
    g_free(command);
    top_val = gswat_gdb_debugger_get_mi_value(self->priv->debugger,
                                              token,
                                              NULL);


    children_val = gdbmi_value_hash_lookup(top_val, "children");
    n=0;
    while((child_val = gdbmi_value_list_get_nth(children_val, n)))
    {
        const GDBMIValue *name_val, *expression_val, *value_val;

        /* FIXME - gdb automatically creates variable objects
         * the first time you list children, but the docs
         * arn't clear about the frame number or expression
         * that are used.
         *
         * It looks like the expression is just the name
         * of the member (i.e. not a valid expression
         * in the scope of the parent expression)
         *
         * I need to find out about the frame number
         */

        name_val = gdbmi_value_hash_lookup(child_val, "name");
        expression_val = gdbmi_value_hash_lookup(child_val, "exp");

        /* complex types wont have a value */
        value_val = gdbmi_value_hash_lookup(child_val, "value");
        child_value=NULL;
        if(value_val)
        {
            child_value = gdbmi_value_literal_get(value_val);
        }

        variable_object = 
            wrap_gdb_variable_object(self->priv->debugger,
                                     gdbmi_value_literal_get(name_val),
                                     gdbmi_value_literal_get(expression_val),
                                     child_value,
                                     self->priv->frame);

        self->priv->children = g_list_prepend(self->priv->children,
                                              variable_object);

        _gswat_gdb_debugger_register_variable_object(self->priv->debugger,
                                                     variable_object);
        n++;
    }

    gdbmi_value_free(top_val);

    return g_list_copy(self->priv->children);
}

