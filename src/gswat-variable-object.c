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

#include <stdlib.h>

#include <glib-2.0/glib.h>
#include <glib/gi18n.h>

#include "gswat-variable-object.h"




struct GSwatVariableObjectPrivate
{
    GSwatDebugger   *debugger;
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



/* GType */
G_DEFINE_TYPE(GSwatVariableObject, gswat_variable_object, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_EXPRESSION,
    PROP_VALUE,
    PROP_CHILDREN
};


static int global_variable_object_index = 0;


static void
create_gdb_variable_object(GSwatVariableObject *self)
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
    token = gdb_send_mi_command(self->priv->debugger, command); 
    g_free(command);

    /* FIXME - make sure we can cope with junk expressions */
    val=gdb_get_mi_value(self->priv->debugger, token, NULL);
    gdbmi_value_free(val);

    self->priv->gdb_name = g_strdup_printf("v%d",
                                           global_variable_object_index);

    global_variable_object_index++;

    /* find out if this expression has any children */
    command = g_strdup_printf("-var-info-num-children %s",
                              self->priv->gdb_name);
    token = gdb_send_mi_command(self->priv->debugger, command);
    val=gdb_get_mi_value(self->priv->debugger, token, NULL);
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



static void
destroy_gdb_variable_object(GSwatVariableObject *self)
{
    gchar *command;
    GList *tmp;

    if(!self->priv->gdb_name)
    {
        return;
    }

    command = g_strdup_printf("-var-delete %s",
                              self->priv->gdb_name);
    gdb_send_mi_command(self->priv->debugger,
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

gchar *
gswat_variable_object_get_expression(GSwatVariableObject *self)
{
    return g_strdup(self->priv->expression);
}

gchar *
gswat_variable_object_get_value(GSwatVariableObject *self, GError **error)
{
    gulong token;
    GDBMIValue *top_val;
    const GDBMIValue *value;
    gchar *command;
    gchar *value_string;
    guint debugger_state_stamp;


#if 0
    if(!self->priv->gdb_name)
    {
        create_gdb_variable_object(self);
    }
#endif
    g_assert(self->priv->gdb_name);
    
    debugger_state_stamp = gswat_debugger_get_state_stamp(self->priv->debugger);
    if(self->priv->cached_value
       && self->priv->debugger_state_stamp == debugger_state_stamp)
    {
        return g_strdup(self->priv->cached_value);
    }

    command = g_strdup_printf("-var-evaluate-expression %s", 
                              self->priv->gdb_name);

    token = gdb_send_mi_command(self->priv->debugger,
                                           command);

    g_free(command);

    top_val = gdb_get_mi_value(self->priv->debugger, token, NULL);
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



static GSwatVariableObject *
wrap_gdb_variable_object(GSwatDebugger *debugger,
                         const gchar *gdb_name,
                         const gchar *expression,
                         const gchar *cache_value,
                         int frame)
{
    GSwatVariableObject *variable_object;

    variable_object = g_object_new(GSWAT_TYPE_VARIABLE_OBJECT, NULL);
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
                        gswat_debugger_get_state_stamp(debugger);

    return variable_object;
}


guint
gswat_variable_object_get_child_count(GSwatVariableObject *self)
{
    return self->priv->child_count;
}


GList *
gswat_variable_object_get_children(GSwatVariableObject *self)
{
    GList *tmp;
    gchar *command;
    gulong token;
    GDBMIValue *top_val;
    const GDBMIValue *children_val, *child_val;
    int n;
    const gchar *child_value;
    GSwatVariableObject *variable_object;

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

    token = gdb_send_mi_command(self->priv->debugger, command);
    g_free(command);
    top_val = gdb_get_mi_value(self->priv->debugger, token, NULL);


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

        variable_object = wrap_gdb_variable_object(self->priv->debugger,
                                                   gdbmi_value_literal_get(name_val),
                                                   gdbmi_value_literal_get(expression_val),
                                                   child_value,
                                                   self->priv->frame);

        self->priv->children = g_list_prepend(
                                              self->priv->children,
                                              variable_object);
        n++;
    }

    gdbmi_value_free(top_val);


    return g_list_copy(self->priv->children);
}


GSwatVariableObject*
gswat_variable_object_new(GSwatDebugger *debugger,
                          const gchar *expression,
                          const gchar *cached_value,
                          int frame)
{
    GSwatVariableObject *variable_object;

    variable_object = g_object_new(GSWAT_TYPE_VARIABLE_OBJECT, NULL);

    variable_object->priv->debugger = debugger;
    
    variable_object->priv->expression = g_strdup(expression);

    if(cached_value)
    {
        variable_object->priv->cached_value = g_strdup(cached_value);
    }
    else
    {
        variable_object->priv->cached_value = NULL;
    }
    variable_object->priv->frame = frame;

    variable_object->priv->debugger_state_stamp = 
                        gswat_debugger_get_state_stamp(debugger);
    
    create_gdb_variable_object(variable_object);
    

    return variable_object;
}


GQuark
gswat_variable_object_error_quark(void)
{
    static GQuark q = 0;
    if(q==0)
    {
        q = g_quark_from_static_string("gswat-variable-object-error");
    }
    return q;
}


static void
gswat_variable_object_init(GSwatVariableObject* self)
{

    self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, GSWAT_TYPE_VARIABLE_OBJECT, struct GSwatVariableObjectPrivate);

}



static void
gswat_variable_object_finalize(GObject* object)
{
    GSwatVariableObject* self = GSWAT_VARIABLE_OBJECT(object);

    destroy_gdb_variable_object(self);
    g_free(self->priv->expression);

    G_OBJECT_CLASS(gswat_variable_object_parent_class)->finalize(object);
}



static void
gswat_variable_object_get_property(GObject* object, guint id, GValue* value, GParamSpec* pspec)
{
    GSwatVariableObject* self = GSWAT_VARIABLE_OBJECT(object);
    gchar *expression;
    gchar *variable_object_value;
    GList *children;

    switch(id) {
        case PROP_EXPRESSION:
            expression = gswat_variable_object_get_expression(self);
            g_value_set_string(value, expression);
            break;
        case PROP_VALUE:
            variable_object_value = 
                gswat_variable_object_get_value(self, NULL);
            g_value_set_string(value, variable_object_value);
            break;
        case PROP_CHILDREN:
            children = gswat_variable_object_get_children(self);
            g_value_set_pointer(value, children);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
            break;
    }
}



static void
gswat_variable_object_class_init(GSwatVariableObjectClass* self_class)
{
    GObjectClass* go_class = G_OBJECT_CLASS(self_class);

    go_class->finalize     = gswat_variable_object_finalize;
    go_class->get_property = gswat_variable_object_get_property;



    g_object_class_install_property(go_class,
                                    PROP_EXPRESSION,
                                    g_param_spec_string("expression",
                                                        _("The associated expression"),
                                                        _("The expression to evaluate in the specifed frame"),
                                                        NULL,
                                                        G_PARAM_READABLE));
    g_object_class_install_property(go_class,
                                    PROP_VALUE,
                                    g_param_spec_string("value",
                                                        _("The expression value"),
                                                        _("The result of evaluating the expression in the specified frame"),
                                                        NULL,
                                                        G_PARAM_READABLE));

    g_object_class_install_property(go_class,
                                    PROP_CHILDREN,
                                    g_param_spec_pointer("children",
                                                         _("Children"),
                                                         _("The list of chilren for complex data types"),
                                                         G_PARAM_READABLE));


    g_type_class_add_private(self_class, sizeof(struct GSwatVariableObjectPrivate));
}


