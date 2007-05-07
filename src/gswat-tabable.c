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

#include "gswat-utils.h"
#include "gswat-tabable.h"

/* Function definitions */
static void gswat_tabable_base_init(GSwatTabableIface *interface);
//static void gswat_tabable_base_finalize(GSwatTabableIface *interface);

/* Macros and defines */

/* Enums */
#if 0
enum {
    SIGNAL_NAME,
	LAST_SIGNAL
};
#endif

/* Variables */
//static guint gswat_tabable_signals[LAST_SIGNAL] = { 0 };
static guint gswat_tabable_base_init_count = 0;


GType
gswat_tabable_get_type(void)
{
    static GType self_type = 0;

    if (!self_type) {
        static const GTypeInfo interface_info = {
            sizeof (GSwatTabableIface),
            (GBaseInitFunc)gswat_tabable_base_init,
            (GBaseFinalizeFunc)NULL,//gswat_tabable_base_finalize
        };

        self_type = g_type_register_static(G_TYPE_INTERFACE,
                                           "GSwatTabable",
                                           &interface_info, 0);

        g_type_interface_add_prerequisite(self_type, G_TYPE_OBJECT);
        //g_type_interface_add_prerequisite(self_type, G_TYPE_);
    }

    return self_type;
}

static void
gswat_tabable_base_init(GSwatTabableIface *interface)
{
    gswat_tabable_base_init_count++;
    GParamSpec *new_param;

    if(gswat_tabable_base_init_count == 1) {

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

        new_param = g_param_spec_ulong("tab-state", /* name */
                                         "State", /* nick name */
                                         "State flags e.g. indicating busy status", /* description */
                                         0, /* minimum */
                                         G_MAXULONG, /* maximum */
                                         0, /* default */
                                         GSWAT_PARAM_READABLE /* flags */
                                         );
        g_object_interface_install_property(interface, new_param);
        
        new_param = g_param_spec_string("tab-label-text", /* name */
                                         "Tab Label Text", /* nick name */
                                         "The text displayed in the tab", /* description */
                                         NULL, /* default */
                                         GSWAT_PARAM_READABLE /* flags */
                                         );
        g_object_interface_install_property(interface, new_param);

        new_param = g_param_spec_object("tab-icon", /* name */
                                         "Tab Icon", /* nick name */
                                         "The icon to display on the tab", /* description */
                                         GTK_TYPE_PIXMAP, /* GType */
                                         GSWAT_PARAM_READABLE /* flags */
                                         );
        g_object_interface_install_property(interface, new_param);

        new_param = g_param_spec_string("tab-tooltip", /* name */
                                         "Tab Tooltip Text", /* nick name */
                                         "The text displayed in the tabs tooltip", /* description */
                                         NULL, /* default */
                                         GSWAT_PARAM_READABLE /* flags */
                                         );
        g_object_interface_install_property(interface, new_param);

        new_param = g_param_spec_object("tab-page-widget", /* name */
                                         "Page Widget", /* nick name */
                                         "The widget representing a page in a tabed notebook", /* description */
                                         GTK_TYPE_WIDGET, /* GType */
                                         GSWAT_PARAM_READABLE /* flags */
                                         );
        g_object_interface_install_property(interface, new_param);

        new_param = g_param_spec_object("tab-focal-widget", /* name */
                                         "Focal Widget", /* nick name */
                                         "The widget to give focus when a tabbable is selected", /* description */
                                         GTK_TYPE_WIDGET, /* GType */
                                         GSWAT_PARAM_READABLE /* flags */
                                         );
        g_object_interface_install_property(interface, new_param);
    }
}

#if 0
static void
gswat_tabable_base_finalize(GSwatTabableIface *interface)
{
    if(gswat_tabable_base_init_count == 0)
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
gswat_tabable_get_label_text(GSwatTabable *object)
{
    gchar *ret;

    g_return_val_if_fail(GSWAT_IS_TABABLE(object), NULL);
    
    g_object_get(G_OBJECT(object), "tab-label-text", &ret, NULL);

    return ret;
}

GdkPixbuf *
gswat_tabable_get_icon(GSwatTabable *object)
{
    GdkPixbuf *ret;

    g_return_val_if_fail(GSWAT_IS_TABABLE(object), NULL);

    g_object_get(G_OBJECT(object), "tab-icon", &ret, NULL);

    return ret;
}

gchar *
gswat_tabable_get_tooltip(GSwatTabable *object)
{
    gchar *ret;

    g_return_val_if_fail(GSWAT_IS_TABABLE(object), NULL);

    g_object_get(G_OBJECT(object), "tab-tooltip", &ret, NULL);

    return ret;
}

gulong
gswat_tabable_get_state_flags(GSwatTabable *object)
{
    gulong ret;

    g_return_val_if_fail(GSWAT_IS_TABABLE(object), 0);
    
    g_object_get(G_OBJECT(object), "tab-state", &ret, NULL);

    return ret;
}

GtkWidget *
gswat_tabable_get_page_widget(GSwatTabable *object)
{
    GtkWidget *ret;
    
    g_return_val_if_fail(GSWAT_IS_TABABLE(object), NULL);
    
    g_object_get(G_OBJECT(object), "tab-page-widget", &ret, NULL);

    return ret;
}

GtkWidget *
gswat_tabable_get_focal_widget(GSwatTabable *object)
{
    GtkWidget *ret;

    g_return_val_if_fail(GSWAT_IS_TABABLE(object), NULL);
    
    g_object_get(G_OBJECT(object), "tab-focal-widget", &ret, NULL);
    
    return ret;
}

