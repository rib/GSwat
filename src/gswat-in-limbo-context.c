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

#include "gswat-context.h"

#include "gswat-in-limbo-context.h"

/* Macros and defines */
#define GSWAT_IN_LIMBO_CONTEXT_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GSWAT_TYPE_IN_LIMBO_CONTEXT, GSwatInLimboContextPrivate))

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

struct _GSwatInLimboContextPrivate
{
    GSwatWindow *window;
};


/* Function definitions */
static void gswat_in_limbo_context_class_init(GSwatInLimboContextClass *klass);
static void gswat_in_limbo_context_get_property(GObject *object,
                                   guint id,
                                   GValue *value,
                                   GParamSpec *pspec);
static void gswat_in_limbo_context_set_property(GObject *object,
                                   guint property_id,
                                   const GValue *value,
                                   GParamSpec *pspec);
static void gswat_in_limbo_context_context_interface_init(gpointer interface,
                                                          gpointer data);
static void gswat_in_limbo_context_init(GSwatInLimboContext *self);
static void gswat_in_limbo_context_finalize(GObject *self);

static gboolean gswat_in_limbo_context_activate(GSwatContext *object);
static void gswat_in_limbo_context_deactivate(GSwatContext *object);


/* Variables */
static GObjectClass *parent_class = NULL;
//static guint gswat_in_limbo_context_signals[LAST_SIGNAL] = { 0 };

GType
gswat_in_limbo_context_get_type(void) /* Typechecking */
{
    static GType self_type = 0;

    if (!self_type)
    {
        static const GTypeInfo object_info =
        {
            sizeof(GSwatInLimboContextClass), /* class structure size */
            NULL, /* base class initializer */
            NULL, /* base class finalizer */
            (GClassInitFunc)gswat_in_limbo_context_class_init, /* class initializer */
            NULL, /* class finalizer */
            NULL, /* class data */
            sizeof(GSwatInLimboContext), /* instance structure size */
            0, /* preallocated instances */
            (GInstanceInitFunc)gswat_in_limbo_context_init, /* instance initializer */
            NULL /* function table */
        };

        /* add the type of your parent class here */
        self_type = g_type_register_static(G_TYPE_OBJECT, /* parent GType */
                                           "GSwatInLimboContext", /* type name */
                                           &object_info, /* type info */
                                           0 /* flags */
                                          );
        /* add interfaces here */
        static const GInterfaceInfo context_info =
        {
            (GInterfaceInitFunc)
                gswat_in_limbo_context_context_interface_init,
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
gswat_in_limbo_context_class_init(GSwatInLimboContextClass *klass) /* Class Initialization */
{   
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    //GParamSpec *new_param;

    parent_class = g_type_class_peek_parent(klass);

    gobject_class->finalize = gswat_in_limbo_context_finalize;

    gobject_class->get_property = gswat_in_limbo_context_get_property;
    gobject_class->set_property = gswat_in_limbo_context_set_property;

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
    gswat_in_limbo_context_signals[SIGNAL_NAME] =
        g_signal_new("signal_name", /* name */
                     G_TYPE_FROM_CLASS(klass), /* interface GType */
                     G_SIGNAL_RUN_LAST, /* signal flags */
                     G_STRUCT_OFFSET(GSwatInLimboContextClass, signal_member),
                     NULL, /* accumulator */
                     NULL, /* accumulator data */
                     g_cclosure_marshal_VOID__VOID, /* c marshaller */
                     G_TYPE_NONE, /* return type */
                     0 /* number of parameters */
                     /* vararg, list of param types */
                    );
#endif

    g_type_class_add_private(klass, sizeof(GSwatInLimboContextPrivate));
}

static void
gswat_in_limbo_context_get_property(GObject *object,
                       guint id,
                       GValue *value,
                       GParamSpec *pspec)
{
    //GSwatInLimboContext* self = GSWAT_IN_LIMBO_CONTEXT(object);

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
gswat_in_limbo_context_set_property(GObject *object,
                       guint property_id,
                       const GValue *value,
                       GParamSpec *pspec)
{   
    //GSwatInLimboContext* self = GSWAT_IN_LIMBO_CONTEXT(object);

    switch(property_id)
    {
#if 0 /* template code */
        case PROP_NAME:
            gswat_in_limbo_context_set_property(self, g_value_get_int(value));
            gswat_in_limbo_context_set_property(self, g_value_get_uint(value));
            gswat_in_limbo_context_set_property(self, g_value_get_boolean(value));
            gswat_in_limbo_context_set_property(self, g_value_get_string(value));
            gswat_in_limbo_context_set_property(self, g_value_get_object(value));
            gswat_in_limbo_context_set_property(self, g_value_get_pointer(value));
            break;
#endif
        default:
            g_warning("gswat_in_limbo_context_set_property on unknown property");
            return;
    }
}

/* Initialize interfaces here */

static void
gswat_in_limbo_context_context_interface_init(gpointer interface,
                                              gpointer data)
{
    GSwatContextIface *context = interface;
    g_assert(G_TYPE_FROM_INTERFACE(context) == GSWAT_TYPE_CONTEXT);

    context->activate = gswat_in_limbo_context_activate;
    context->deactivate = gswat_in_limbo_context_deactivate;
}

/* Instance Construction */
static void
gswat_in_limbo_context_init(GSwatInLimboContext *self)
{
    self->priv = GSWAT_IN_LIMBO_CONTEXT_GET_PRIVATE(self);
    /* populate your object here */
}

/* Instantiation wrapper */
GSwatInLimboContext*
gswat_in_limbo_context_new(GSwatWindow *window)
{
    GSwatInLimboContext *self;
    self = GSWAT_IN_LIMBO_CONTEXT(g_object_new(gswat_in_limbo_context_get_type(), NULL));
    self->priv->window = window;
    return self;
}

/* Instance Destruction */
void
gswat_in_limbo_context_finalize(GObject *object)
{
    //GSwatInLimboContext *self = GSWAT_IN_LIMBO_CONTEXT(object);

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
 * gswat_in_limbo_context_get_PROPERTY:
 * @self:  A GSwatInLimboContext.
 *
 * Fetches the PROPERTY of the GSwatInLimboContext. FIXME, add more info!
 *
 * Returns: The value of PROPERTY. FIXME, add more info!
 */
PropType
gswat_in_limbo_context_get_PROPERTY(GSwatInLimboContext *self)
{
    g_return_val_if_fail(GSWAT_IS_IN_LIMBO_CONTEXT(self), /* FIXME */);

    //return self->priv->PROPERTY;
    //return g_strdup(self->priv->PROPERTY);
    //return g_object_ref(self->priv->PROPERTY);
}

/**
 * gswat_in_limbo_context_set_PROPERTY:
 * @self:  A GSwatInLimboContext.
 * @property:  The value to set. FIXME, add more info!
 *
 * Sets this properties value.
 *
 * This will also clear the properties previous value.
 */
void
gswat_in_limbo_context_set_PROPERTY(GSwatInLimboContext *self, PropType PROPERTY)
{
    g_return_if_fail(GSWAT_IS_IN_LIMBO_CONTEXT(self));

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
gswat_in_limbo_context_activate(GSwatContext *object)
{
    GSwatInLimboContext *self;

    g_return_val_if_fail(GSWAT_IS_IN_LIMBO_CONTEXT(object), FALSE);
    self = GSWAT_IN_LIMBO_CONTEXT(object);
    g_message("gswat_in_limbo_context_activate");

    return TRUE;
}

static void
gswat_in_limbo_context_deactivate(GSwatContext *object)
{
    GSwatInLimboContext *self;

    g_return_if_fail(GSWAT_IS_IN_LIMBO_CONTEXT(object));
    self = GSWAT_IN_LIMBO_CONTEXT(object);
    g_message("gswat_in_limbo_context_deactivate");
}
