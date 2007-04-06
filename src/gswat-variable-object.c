
#include "gswat-variable-object.h"

/* Function definitions */
static void gswat_variable_object_base_init(GSwatVariableObjectIface *interface);
//static void gswat_variable_object_base_finalize(GSwatVariableObjectIface *interface);

/* Macros and defines */

/* Enums */
enum {
    SIG_UPDATED,
    LAST_SIGNAL
};

/* Variables */
static guint gswat_variable_object_signals[LAST_SIGNAL] = { 0 };
static guint gswat_variable_object_base_init_count = 0;


GType
gswat_variable_object_get_type(void)
{
    static GType self_type = 0;

    if (!self_type) {
        static const GTypeInfo interface_info = {
            sizeof (GSwatVariableObjectIface),
            (GBaseInitFunc)gswat_variable_object_base_init,
            (GBaseFinalizeFunc)NULL,//gswat_variable_object_base_finalize
        };

        self_type = g_type_register_static(G_TYPE_INTERFACE,
                                           "GSwatVariableObject",
                                           &interface_info, 0);

        g_type_interface_add_prerequisite(self_type, G_TYPE_OBJECT);
        //g_type_interface_add_prerequisite(self_type, G_TYPE_);
    }

    return self_type;
}

static void
gswat_variable_object_base_init(GSwatVariableObjectIface *interface)
{
    gswat_variable_object_base_init_count++;

    if(gswat_variable_object_base_init_count == 1) {

        /* register signals */
#if 0
        gswat_variable_object_signals[SIGNAL_NAME] =
            g_signal_new("signal_name", /* name */
                         GSWAT_TYPE_VARIABLE_OBJECT, /* interface GType */
                         G_SIGNAL_RUN_LAST, /* signal flags */
                         G_STRUCT_OFFSET(GSwatVariableObjectIface, signal_member),
                         NULL, /* accumulator */
                         NULL, /* accumulator data */
                         g_cclosure_marshal_VOID__VOID, /* c marshaller */
                         G_TYPE_NONE, /* return type */
                         0, /* number of parameters */
                         /* vararg, list of param types */
                        );
#endif
        interface->updated = NULL;
        gswat_variable_object_signals[SIG_UPDATED] =
            g_signal_new("updated", /* name */
                         G_TYPE_FROM_INTERFACE(interface), /* interface GType */
                         G_SIGNAL_RUN_LAST, /* signal flags */
                         G_STRUCT_OFFSET(GSwatVariableObjectIface, updated),
                         NULL, /* accumulator */
                         NULL, /* accumulator data */
                         g_cclosure_marshal_VOID__VOID, /* c marshaller */
                         G_TYPE_NONE, /* return type */
                         0 /* number of parameters */
                         /* vararg, list of param types */
                        );

    }
}

#if 0
static void
gswat_variable_object_base_finalize(GSwatVariableObjectIface *interface)
{
    if(gswat_variable_object_base_init_count == 0)
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

gchar *
gswat_variable_object_get_expression(GSwatVariableObject* object)
{
    GSwatVariableObjectIface *variable_object;
    gchar *ret;

    g_return_val_if_fail(GSWAT_IS_VARIABLE_OBJECT(object), NULL);
    variable_object = GSWAT_VARIABLE_OBJECT_GET_IFACE(object);

    g_object_ref(object);
    ret = variable_object->get_expression(object);
    g_object_unref(object);

    return ret;
}

gchar *
gswat_variable_object_get_value(GSwatVariableObject *object, GError **error)
{
    GSwatVariableObjectIface *variable_object;
    gchar *ret;

    g_return_val_if_fail(GSWAT_IS_VARIABLE_OBJECT(object), NULL);
    variable_object = GSWAT_VARIABLE_OBJECT_GET_IFACE(object);

    g_object_ref(object);
    ret = variable_object->get_value(object, error);
    g_object_unref(object);

    return ret;
}

guint
gswat_variable_object_get_child_count(GSwatVariableObject *object)
{
    GSwatVariableObjectIface *variable_object;
    guint ret;

    g_return_val_if_fail(GSWAT_IS_VARIABLE_OBJECT(object), 0);
    variable_object = GSWAT_VARIABLE_OBJECT_GET_IFACE(object);

    g_object_ref(object);
    ret = variable_object->get_child_count(object);
    g_object_unref(object);

    return ret;
}

GList *
gswat_variable_object_get_children(GSwatVariableObject* object)
{
    GSwatVariableObjectIface *variable_object;
    GList *ret;

    g_return_val_if_fail(GSWAT_IS_VARIABLE_OBJECT(object), NULL);
    variable_object = GSWAT_VARIABLE_OBJECT_GET_IFACE(object);

    g_object_ref(object);
    ret = variable_object->get_children(object);
    g_object_unref(object);

    return ret;
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

