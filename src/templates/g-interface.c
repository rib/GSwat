
#include "my-doable.h"

/* Function definitions */
static void my_doable_base_init(gpointer g_class);

/* Macros and defines */

/* Enums */
enum {
    SIGNAL_NAME,
	LAST_SIGNAL
};

/* Variables */
static guint my_doable_signals[LAST_SIGNAL] = { 0 };
static guint my_doable_base_init_count = 0;


GType
my_doable_get_type(void)
{
    static GType self_type = 0;

    if (!self_type) {
        static const GTypeInfo interface_info = {
            sizeof (MyDoableIface),
            my_doable_base_init,
            NULL,//my_doable_base_finalize
        };

        type = g_type_register_static(G_TYPE_INTERFACE,
                                      "MyDoable",
                                      &interface_info, 0);

        g_type_interface_add_prerequisite(self_type, G_TYPE_OBJECT);
        //g_type_interface_add_prerequisite(self_type, G_TYPE_);
    }

    return self_type;
}

static void
my_doable_base_init(gpointer g_class)
{
    my_doable_base_init_count++;
    
	if(my_doable_base_init_count == 1) {

        /* register signals */
        my_doable_signals[SIGNAL_NAME] =
            g_signal_new("signal_name", /* name */
                         MY_TYPE_DOABLE, /* interface GType */
                         G_SIGNAL_RUN_LAST, /* signal flags */
                         G_STRUCT_OFFSET(MyDoableIface, signal_member),
                         NULL, /* accumulator */
                         NULL, /* accumulator data */
                         g_cclosure_marshal_VOID__VOID, /* c marshaller */
                         G_TYPE_NONE, /* return type */
                         0, /* number of parameters */
                         /* vararg, list of param types */
                        );
    }
}

#if 0
static void
my_doable_base_finalize(gpointer g_class)
{
    if(my_doable_base_init_count == 0)
    {

    }
}
#endif

void
my_doable_method1(MyDoable *object)
{
    MyDoableClass *doable;

    g_return_if_fail(MY_IS_DOABLE(object));

    g_object_ref(object);
    doable->method1(object);
    g_object_unref(object);
}

