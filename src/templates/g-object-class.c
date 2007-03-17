#include "my-object.h"

/* add your signals here */
#if 0
enum {
    SIGNAL_NAME,
    LAST_SIGNAL
};
#endif


#define MY_OBJECT_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), MY_TYPE_OBJECT, MyObjectPrivate))

static void my_object_class_init (MyObjectClass *klass);
static void my_object_init (MyObject      *self);
//staic void my_object_mydoable_interface_init ();
#error fixme

static guint my_object_signals[LAST_SIGNAL] = { 0 };

GType
my_object_get_type (void) /* Typechecking */
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

        /* fixme add templace interface code here */
        static const 
#error fixme

            /* add the type of your parent class here */
            self_type = g_type_register_static(MY_OBJECT_PARENT, "MyObject", &object_info, 0);
    }

    return self_type;
}

static void
my_object_get_property(GObject* object, guint id, GValue* value, GParamSpec* pspec)
{
    MyObject* self = MY_OBJECT(object);

    switch(id) {
        case PROP_STATE:
#if 0 /* template code */
            g_value_set_int(value, self->priv->property);
            g_value_set_uint(value, self->priv->property);
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
my_object_class_init(MyObjectClass *klass) /* Class Initialization */
{   
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    parent_class = g_type_class_peek_parent(klass);

    gobject_class->finalize = my_object_finalize;

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
my_object_init(MyObject *self) /* Instance Construction */
{
    /* populate your widget here */
}

MyObject*
my_object_new() /* Construction */
{
    return MY_OBJECT(g_object_new(my_object_get_type(), NULL));
}

void
my_object_finalize(MyObject *self) /* Destruction */
{
    /* destruct your widgets here */
}


/* add new methods here */
#error fixme gtk-doc notes
