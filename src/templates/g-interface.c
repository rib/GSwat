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

#include "my-doable.h"

/* Function definitions */
static void my_doable_base_init(MyDoableIface *interface);
static void my_doable_base_finalize(MyDoableIface *interface);

/* Macros and defines */

/* Enums */
#if 0
enum {
    SIGNAL_NAME,
	LAST_SIGNAL
};
#endif

/* Variables */
//static guint my_doable_signals[LAST_SIGNAL] = { 0 };
static guint my_doable_base_init_count = 0;


GType
my_doable_get_type(void)
{
    static GType self_type = 0;

    if (!self_type) {
        static const GTypeInfo interface_info = {
            sizeof (MyDoableIface),
            (GBaseInitFunc)my_doable_base_init,
            (GBaseFinalizeFunc)NULL,//my_doable_base_finalize
        };

        self_type = g_type_register_static(G_TYPE_INTERFACE,
                                           "MyDoable",
                                           &interface_info, 0);

        g_type_interface_add_prerequisite(self_type, G_TYPE_OBJECT);
        //g_type_interface_add_prerequisite(self_type, G_TYPE_);
    }

    return self_type;
}

static void
my_doable_base_init(MyDoableIface *interface)
{
    my_doable_base_init_count++;
    //GParamSpec *new_param;

    if(my_doable_base_init_count == 1) {

        /* register signals */
#if 0 /* template code */
        interface->signal_member = signal_default_handler;
        my_doable_signals[SIGNAL_NAME] =
            g_signal_new("signal_name", /* name */
                         G_TYPE_FROM_INTERFACE(interface), /* interface GType */
                         G_SIGNAL_RUN_LAST, /* signal flags */
                         G_STRUCT_OFFSET(MyDoableIface, signal_member),
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
                                         MY_PARAM_READABLE /* flags */
                                         MY_PARAM_WRITEABLE /* flags */
                                         MY_PARAM_READWRITE /* flags */
                                         | G_PARAM_CONSTRUCT
                                         | G_PARAM_CONSTRUCT_ONLY
                                         );
        g_object_interface_install_property(interface, new_param);
#endif

    }
}

#if 0
static void
my_doable_base_finalize(MyDoableIface *interface)
{
    if(my_doable_base_init_count == 0)
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

void
my_doable_method1(MyDoable *object)
{
    MyDoableIface *doable;

    g_return_if_fail(MY_IS_DOABLE(object));
    doable = MY_DOABLE_GET_IFACE(object);

    g_object_ref(object);
    doable->method1(object);
    g_object_unref(object);
}

