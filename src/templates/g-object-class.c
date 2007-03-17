#include "my-object.h"

/* Function definitions */
static void my_object_class_init(MyObjectClass *klass);
static void my_object_get_property(GObject* object,
                                   guint id,
                                   GValue* value,
                                   GParamSpec* pspec);
static void my_object_set_property(GObject      *object,
					               guint        property_id,
					               const GValue *value,
					               GParamSpec   *pspec);
//staic void my_object_mydoable_interface_init(gpointer interface,
//                                             gpointer data);
static void my_object_init(MyObject *self);

/* Macros and defines */
#define MY_OBJECT_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), MY_TYPE_OBJECT, MyObjectPrivate))

/* Enums */
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

/* Variables */
static GObjectClass *parent_class = NULL;
static guint my_object_signals[LAST_SIGNAL] = { 0 };

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
        self_type = g_type_register_static(MY_OBJECT_PARENT,
                                           "MyObject",
                                           &object_info,
                                           0);
#if 0
        /* add interfaces here */
        static const GInterfaceInfo mydoable_info =
        {
            (GInterfaceInitFunc)
                my_object_mydoable_interface_init,
            (GInterfaceFinalizeFunc)NULL,
            NULL /* interface data */
        }

        if (type != G_TYPE_INVALID) {
            g_type_add_interface_static(
				self_type, MY_TYPE_MYDOABLE, &mydoable_info);
        }
#endif
    }

    return self_type;
}

static void
my_object_class_init(MyObjectClass *klass) /* Class Initialization */
{   
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GParamSpec *new_param;

    parent_class = g_type_class_peek_parent(klass);

    gobject_class->finalize = my_object_finalize;

    gobject_class->get_property = my_object_get_property;
    gobject_class->set_property = my_object_set_property;
    
    /* set up properties */
#if 0
    new_param = g_param_spec_int("name", /* name */
    new_param = g_param_spec_uint("name", /* name */
    new_param = g_param_spec_boolean("name", /* name */
    new_param = g_param_spec_object("name", /* name */
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
#if 0
    klass->signal_member = signal_default_handler;
    my_object_signals[SIGNAL_NAME] 
        = g_signal_new ("signal_name",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                        G_STRUCT_OFFSET (MyObjectClass, signal_member),
                        NULL,
                        NULL,
                        g_cclosure_marshal_VOID__VOID,
                        G_TYPE_NONE, 0);
#endif

    g_type_class_add_private(klass, sizeof(struct MyObjectPrivate));
}

static void
my_object_get_property(GObject* object,
                       guint id,
                       GValue* value,
                       GParamSpec* pspec)
{
    MyObject* self = MY_OBJECT(object);

    switch(id) {
        case PROP_STATE:
#if 0 /* template code */
            g_value_set_int(value, self->priv->property);
            g_value_set_uint(value, self->priv->property);
            g_value_set_boolean(value, self->priv->property);
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
#error fixme set_property

static void
my_object_set_property(GObject      *object,
					   guint        property_id,
					   const GValue *value,
					   GParamSpec   *pspec)
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
		    g_warning("my_object_set_property on unknown property");
		    return;
    }
}

/* Initialize interfaces here */

#if 0
void
my_object_mydoable_interface_init(gpointer interface,
                                  gpointer data)
{
    MyDoableClass *mydoable = interface;
    g_assert(G_TYPE_FROM_INTERFACE(mydoable) = MY_TYPE_MYDOABLE);

    mydoable->method1 = my_object_method1;
    mydoable->method2 = my_object_method2;
}
#endif

static void
my_object_init(MyObject *self) /* Instance Construction */
{
    self->priv = MY_OBJECT_GET_PRIVATE(self);
    /* populate your widget here */
}

MyObject*
my_object_new(void) /* Construction */
{
#error fixme
    return MY_OBJECT(g_object_new(my_object_get_type(), NULL));
}

void
my_object_finalize(MyObject *self) /* Destruction */
{
    /* destruct your object here */
#error fixme
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
