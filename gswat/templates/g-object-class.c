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

#include "my-object.h"

/* Macros and defines */
#define MY_OBJECT_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), MY_TYPE_OBJECT, MyObjectPrivate))

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

struct _MyObjectPrivate
{
#error template_fixme
};


/* Function definitions */
static void my_object_class_init(MyObjectClass *klass);
static void my_object_get_property(GObject *object,
                                   guint id,
                                   GValue *value,
                                   GParamSpec *pspec);
static void my_object_set_property(GObject *object,
                                   guint property_id,
                                   const GValue *value,
                                   GParamSpec *pspec);
//static void my_object_mydoable_interface_init(gpointer interface,
//                                             gpointer data);
static void my_object_init(MyObject *self);
static void my_object_finalize(GObject *self);


/* Variables */
static GObjectClass *parent_class = NULL;
#error template_fixme
//static guint my_object_signals[LAST_SIGNAL] = { 0 };

GType
my_object_get_type(void) /* Typechecking */
{
    static GType self_type = 0;

    if (!self_type)
    {
        static const GTypeInfo object_info =
        {
            sizeof(MyObjectClass), /* class structure size */
            NULL, /* base class initializer */
            NULL, /* base class finalizer */
            (GClassInitFunc)my_object_class_init, /* class initializer */
            NULL, /* class finalizer */
            NULL, /* class data */
            sizeof(MyObject), /* instance structure size */
            0, /* preallocated instances */
            (GInstanceInitFunc)my_object_init, /* instance initializer */
            NULL /* function table */
        };

        /* add the type of your parent class here */
#error template_fixme
        self_type = g_type_register_static(G_TYPE_OBJECT, /* parent GType */
                                           "MyObject", /* type name */
                                           &object_info, /* type info */
                                           0 /* flags */
                                          );
#if 0
        /* add interfaces here */
        static const GInterfaceInfo mydoable_info =
        {
            (GInterfaceInitFunc)
                my_object_mydoable_interface_init,
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
my_object_class_init(MyObjectClass *klass) /* Class Initialization */
{   
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    //GParamSpec *new_param;

    parent_class = g_type_class_peek_parent(klass);

    gobject_class->finalize = my_object_finalize;

    gobject_class->get_property = my_object_get_property;
    gobject_class->set_property = my_object_set_property;

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
    my_object_signals[SIGNAL_NAME] =
        g_signal_new("signal_name", /* name */
                     G_TYPE_FROM_CLASS(klass), /* interface GType */
                     G_SIGNAL_RUN_LAST, /* signal flags */
                     G_STRUCT_OFFSET(MyObjectClass, signal_member),
                     NULL, /* accumulator */
                     NULL, /* accumulator data */
                     g_cclosure_marshal_VOID__VOID, /* c marshaller */
                     G_TYPE_NONE, /* return type */
                     0 /* number of parameters */
                     /* vararg, list of param types */
                    );
#endif

    g_type_class_add_private(klass, sizeof(MyObjectPrivate));
}

static void
my_object_get_property(GObject *object,
                       guint id,
                       GValue *value,
                       GParamSpec *pspec)
{
    //MyObject* self = MY_OBJECT(object);

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
my_object_set_property(GObject *object,
                       guint property_id,
                       const GValue *value,
                       GParamSpec *pspec)
{   
    //MyObject* self = MY_OBJECT(object);

    switch(property_id)
    {
#if 0 /* template code */
        case PROP_NAME:
            my_object_set_property(self, g_value_get_int(value));
            my_object_set_property(self, g_value_get_uint(value));
            my_object_set_property(self, g_value_get_boolean(value));
            my_object_set_property(self, g_value_get_string(value));
            my_object_set_property(self, g_value_get_object(value));
            my_object_set_property(self, g_value_get_pointer(value));
            break;
#endif
        default:
            g_warning("my_object_set_property on unknown property");
            return;
    }
}

/* Initialize interfaces here */

#if 0
static void
my_object_mydoable_interface_init(gpointer interface,
                                  gpointer data)
{
    MyDoableIface *mydoable = interface;
    g_assert(G_TYPE_FROM_INTERFACE(mydoable) == MY_TYPE_MYDOABLE);

    mydoable->method1 = my_object_method1;
    mydoable->method2 = my_object_method2;
}
#endif

/* Instance Construction */
static void
my_object_init(MyObject *self)
{
    self->priv = MY_OBJECT_GET_PRIVATE(self);
    /* populate your object here */
}

/* Instantiation wrapper */
MyObject*
my_object_new(void)
{
    return MY_OBJECT(g_object_new(my_object_get_type(), NULL));
}

/* Instance Destruction */
void
my_object_finalize(GObject *object)
{
    //MyObject *self = MY_OBJECT(object);

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
 * my_object_get_PROPERTY:
 * @self:  A MyObject.
 *
 * Fetches the PROPERTY of the MyObject. FIXME, add more info!
 *
 * Returns: The value of PROPERTY. FIXME, add more info!
 */
PropType
my_object_get_PROPERTY(MyObject *self)
{
    g_return_val_if_fail(MY_IS_OBJECT(self), /* FIXME */);

    //return self->priv->PROPERTY;
    //return g_strdup(self->priv->PROPERTY);
    //return g_object_ref(self->priv->PROPERTY);
}

/**
 * my_object_set_PROPERTY:
 * @self:  A MyObject.
 * @property:  The value to set. FIXME, add more info!
 *
 * Sets this properties value.
 *
 * This will also clear the properties previous value.
 */
void
my_object_set_PROPERTY(MyObject *self, PropType PROPERTY)
{
    g_return_if_fail(MY_IS_OBJECT(self));

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

