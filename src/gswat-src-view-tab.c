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

#include <gedit-document.h>
#include <gedit-utils.h>

#include "gswat-utils.h"
#include "gswat-tabable.h"
#include "gswat-src-view-tab.h"

/* Macros and defines */
#define GSWAT_SRC_VIEW_TAB_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GSWAT_TYPE_SRC_VIEW_TAB, GSwatSrcViewTabPrivate))

#define MAX_DOC_NAME_LENGTH 40

/* Enums/Typedefs */
/* add your signals here */
#if 0
enum {
    SIGNAL_NAME,
    LAST_SIGNAL
};
#endif


enum {
    PROP_0,
    PROP_TAB_LABEL_TEXT,
    PROP_TAB_ICON,
    PROP_TAB_TOOLTIP,
    PROP_TAB_STATE,
    PROP_TAB_PAGE_WIDGET,
    PROP_TAB_FOCAL_WIDGET
};


struct _GSwatSrcViewTabPrivate
{
    gchar       *tab_label_text;
    GtkPixmap   *tab_icon;
    gchar       *tab_tooltip;
    gulong      tab_state;
    GtkWidget   *tab_page_widget; /* vbox */
    GtkWidget   *tab_focal_widget; /* GswatView */

    GtkWidget   *view_scrolled_window;
    GtkWidget   *view;
};


/* Function definitions */
static void gswat_src_view_tab_class_init(GSwatSrcViewTabClass *klass);
static void gswat_src_view_tab_get_property(GObject *object,
                                   guint id,
                                   GValue *value,
                                   GParamSpec *pspec);
static void gswat_src_view_tab_set_property(GObject *object,
                                   guint property_id,
                                   const GValue *value,
                                   GParamSpec *pspec);
static void gswat_src_view_tab_tabable_interface_init(gpointer interface,
                                                       gpointer data);
static void gswat_src_view_tab_init(GSwatSrcViewTab *self);
static void gswat_src_view_tab_finalize(GObject *self);
static void on_gedit_document_loaded(GeditDocument *doc,
                                     const GError *error,
                                     void *data);

/* Variables */
static GObjectClass *parent_class = NULL;
//static guint gswat_src_view_tab_signals[LAST_SIGNAL] = { 0 };

GType
gswat_src_view_tab_get_type(void) /* Typechecking */
{
    static GType self_type = 0;

    if (!self_type)
    {
        static const GTypeInfo object_info =
        {
            sizeof(GSwatSrcViewTabClass), /* class structure size */
            NULL, /* base class initializer */
            NULL, /* base class finalizer */
            (GClassInitFunc)gswat_src_view_tab_class_init, /* class initializer */
            NULL, /* class finalizer */
            NULL, /* class data */
            sizeof(GSwatSrcViewTab), /* instance structure size */
            0, /* preallocated instances */
            (GInstanceInitFunc)gswat_src_view_tab_init, /* instance initializer */
            NULL /* function table */
        };

        /* add the type of your parent class here */
        self_type = g_type_register_static(G_TYPE_OBJECT, /* parent GType */
                                           "GSwatSrcViewTab", /* type name */
                                           &object_info, /* type info */
                                           0 /* flags */
                                          );

        /* add interfaces here */
        static const GInterfaceInfo tabable_info =
        {
            (GInterfaceInitFunc)
                gswat_src_view_tab_tabable_interface_init,
            (GInterfaceFinalizeFunc)NULL,
            NULL /* interface data */
        };

        if(self_type != G_TYPE_INVALID) {
            g_type_add_interface_static(self_type,
                                        GSWAT_TYPE_TABABLE,
                                        &tabable_info);
        }

    }

    return self_type;
}

static void
gswat_src_view_tab_class_init(GSwatSrcViewTabClass *klass) /* Class Initialization */
{   
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    //GParamSpec *new_param;

    parent_class = g_type_class_peek_parent(klass);

    gobject_class->finalize = gswat_src_view_tab_finalize;

    gobject_class->get_property = gswat_src_view_tab_get_property;
    gobject_class->set_property = gswat_src_view_tab_set_property;

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
                                     GSWAT_PARAM_READABLE /* flags */
                                     GSWAT_PARAM_WRITABLE /* flags */
                                     GSWAT_PARAM_READWRITE /* flags */
                                     | G_PARAM_CONSTRUCT
                                     | G_PARAM_CONSTRUCT_ONLY
                                     );
    g_object_class_install_property(gobject_class,
                                    PROP_NAME,
                                    new_param);
#endif
    g_object_class_override_property(gobject_class,
                                     PROP_TAB_LABEL_TEXT,
                                     "tab-label-text");
    g_object_class_override_property(gobject_class,
                                     PROP_TAB_ICON,
                                     "tab-icon");
    g_object_class_override_property(gobject_class,
                                     PROP_TAB_TOOLTIP,
                                     "tab-tooltip");
    g_object_class_override_property(gobject_class,
                                     PROP_TAB_STATE,
                                     "tab-state");
    g_object_class_override_property(gobject_class,
                                     PROP_TAB_PAGE_WIDGET,
                                     "tab-page-widget");
    g_object_class_override_property(gobject_class,
                                     PROP_TAB_FOCAL_WIDGET,
                                     "tab-focal-widget");


    /* set up signals */
#if 0 /* template code */
    klass->signal_member = signal_default_handler;
    gswat_src_view_tab_signals[SIGNAL_NAME] =
        g_signal_new("signal_name", /* name */
                     G_TYPE_FROM_CLASS(klass), /* interface GType */
                     G_SIGNAL_RUN_LAST, /* signal flags */
                     G_STRUCT_OFFSET(GSwatSrcViewTabClass, signal_member),
                     NULL, /* accumulator */
                     NULL, /* accumulator data */
                     g_cclosure_marshal_VOID__VOID, /* c marshaller */
                     G_TYPE_NONE, /* return type */
                     0 /* number of parameters */
                     /* vararg, list of param types */
                    );
#endif

    g_type_class_add_private(klass, sizeof(GSwatSrcViewTabPrivate));
}

static void
gswat_src_view_tab_get_property(GObject *object,
                                guint id,
                                GValue *value,
                                GParamSpec *pspec)
{
    GSwatSrcViewTab* self = GSWAT_SRC_VIEW_TAB(object);

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
        case PROP_TAB_LABEL_TEXT:
            g_value_set_string(value, self->priv->tab_label_text);
            break;
        case PROP_TAB_ICON:
            g_value_set_object(value, self->priv->tab_icon);
            break;
        case PROP_TAB_TOOLTIP:
            g_value_set_string(value, self->priv->tab_tooltip);
            break;
        case PROP_TAB_STATE:
            g_value_set_ulong(value, self->priv->tab_state);
            break;
        case PROP_TAB_PAGE_WIDGET:
            g_value_set_object(value, self->priv->tab_page_widget);
            break;
        case PROP_TAB_FOCAL_WIDGET:
            g_value_set_object(value, self->priv->tab_focal_widget);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
            break;
    }
}

static void
gswat_src_view_tab_set_property(GObject *object,
                                guint property_id,
                                const GValue *value,
                                GParamSpec *pspec)
{   
    //GSwatSrcViewTab* self = GSWAT_SRC_VIEW_TAB(object);

    switch(property_id)
    {
#if 0 /* template code */
        case PROP_NAME:
            gswat_src_view_tab_set_property(self, g_value_get_int(value));
            gswat_src_view_tab_set_property(self, g_value_get_uint(value));
            gswat_src_view_tab_set_property(self, g_value_get_boolean(value));
            gswat_src_view_tab_set_property(self, g_value_get_string(value));
            gswat_src_view_tab_set_property(self, g_value_get_object(value));
            gswat_src_view_tab_set_property(self, g_value_get_pointer(value));
            break;
#endif
        default:
            g_warning("gswat_src_view_tab_set_property on unknown property");
            return;
    }
}

/* Initialize interfaces here */


static void
gswat_src_view_tab_tabable_interface_init(gpointer interface,
                                          gpointer data)
{
    GSwatTabableIface *tabable = interface;
    g_assert(G_TYPE_FROM_INTERFACE(tabable) == GSWAT_TYPE_TABABLE);

    /* Currently nothing to do */
}


/* Instance Construction */
static void
gswat_src_view_tab_init(GSwatSrcViewTab *self)
{
    GtkWidget *vbox;
    GtkWidget *sw;
    GeditDocument *doc;


    self->priv = GSWAT_SRC_VIEW_TAB_GET_PRIVATE(self);

    /* Create a top level vbox */
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(vbox);

    /* Create a scrolled window */
    sw = gtk_scrolled_window_new(NULL, NULL);
    self->priv->view_scrolled_window = sw;

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (sw),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                        GTK_SHADOW_IN);	
    gtk_widget_show(sw);

    /* Create the view */
    doc = gedit_document_new();

    self->priv->view = gswat_view_new(doc);
    g_object_unref(doc);
    gtk_widget_show(self->priv->view);

    gtk_box_pack_end(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(sw), self->priv->view);


    /* Set up the tabable interface */
    self->priv->tab_label_text = g_strdup("");
    self->priv->tab_icon = NULL;
    self->priv->tab_tooltip = "FIXME: This is a big flibble";
    self->priv->tab_state = 0;
    self->priv->tab_page_widget = vbox;
    self->priv->tab_focal_widget = self->priv->view;
}



/* Instantiation wrapper */
GSwatSrcViewTab*
gswat_src_view_tab_new(gchar *file_uri, gint line)
{
    GSwatSrcViewTab *self;
    GSwatView *view;
    GeditDocument *doc;
    const GeditEncoding *encoding;
    

    self = 
        GSWAT_SRC_VIEW_TAB(g_object_new(gswat_src_view_tab_get_type(),
                                        NULL));

    /* FIXME should this be put in a gobject class constructor? */

    view = gswat_src_view_tab_get_view(self);
    doc = gswat_view_get_document(view);

    g_signal_connect(doc,
                     "loaded",
                     G_CALLBACK(on_gedit_document_loaded),
                     self
                    );

    g_object_unref(view);
    encoding = gedit_document_get_encoding(doc);
    gedit_document_load(doc, file_uri, encoding, line, FALSE);

    return self;
}

/* Instance Destruction */
void
gswat_src_view_tab_finalize(GObject *object)
{
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
 * gswat_src_view_tab_get_PROPERTY:
 * @self:  A GSwatSrcViewTab.
 *
 * Fetches the PROPERTY of the GSwatSrcViewTab. FIXME, add more info!
 *
 * Returns: The value of PROPERTY. FIXME, add more info!
 */
PropType
gswat_src_view_tab_get_PROPERTY(GSwatSrcViewTab *self)
{
    g_return_val_if_fail(GSWAT_IS_SRC_VIEW_TAB(self), /* FIXME */);

    //return self->priv->PROPERTY;
    //return g_strdup(self->priv->PROPERTY);
    //return g_object_ref(self->priv->PROPERTY);
}

/**
 * gswat_src_view_tab_set_PROPERTY:
 * @self:  A GSwatSrcViewTab.
 * @property:  The value to set. FIXME, add more info!
 *
 * Sets this properties value.
 *
 * This will also clear the properties previous value.
 */
void
gswat_src_view_tab_set_PROPERTY(GSwatSrcViewTab *self, PropType PROPERTY)
{
    g_return_if_fail(GSWAT_IS_SRC_VIEW_TAB(self));

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


GSwatView *
gswat_src_view_tab_get_view(GSwatSrcViewTab *self)
{
    return g_object_ref(self->priv->view);
}

static void
on_gedit_document_loaded(GeditDocument *doc,
                         const GError *error,
                         void *data)
{
    GSwatSrcViewTab *self = GSWAT_SRC_VIEW_TAB(data);
    gchar *name;
    gchar *label_text;

    name = gedit_document_get_short_name_for_display(doc);

    /* Truncate the name so it doesn't get insanely wide. */
    label_text = 
        gedit_utils_str_middle_truncate(name, MAX_DOC_NAME_LENGTH);
        
    g_free(name);

    g_free(self->priv->tab_label_text);
    self->priv->tab_label_text = g_strdup(label_text);
    g_object_notify(self, "tab-label-text");
}

