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
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "gedit-tooltips.h"
#include "gedit-spinner.h"

#include "gswat-utils.h"
#include "gswat-tabable.h"
#include "gswat-notebook.h"

/* Macros and defines */
#define GSWAT_NOTEBOOK_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GSWAT_TYPE_NOTEBOOK, GSwatNotebookPrivate))

/* Enums/Typedefs */
/* add your signals here */
enum {
    TAB_ADDED,
    TAB_CLOSE_REQUEST,
    LAST_SIGNAL
};

#if 0
enum {
    PROP_0,
    PROP_NAME,
};
#endif

struct _GSwatNotebookPrivate
{
	GeditTooltips   *tool_tips;
	gint	        always_show_tabs : 1;
};


/* Function definitions */
static void gswat_notebook_class_init(GSwatNotebookClass *klass);
static void gswat_notebook_get_property(GObject *object,
                                        guint id,
                                        GValue *value,
                                        GParamSpec *pspec);
static void gswat_notebook_set_property(GObject *object,
                                        guint property_id,
                                        const GValue *value,
                                        GParamSpec *pspec);
//static void gswat_notebook_mydoable_interface_init(gpointer interface,
//                                             gpointer data);
static void gswat_notebook_init(GSwatNotebook *self);
static void gswat_notebook_finalize(GObject *self);

static GtkWidget *build_blank_tab_label(GSwatNotebook *self);
static void on_gswat_tabbable_state_flags_update(GObject *object,
                                                 GParamSpec *property,
                                                 gpointer data);
static void on_gswat_tabbable_label_text_update(GObject *object,
                                                GParamSpec *property,
                                                gpointer data);
static void on_gswat_tabbable_icon_update(GObject *object,
                                          GParamSpec *property,
                                          gpointer data);
static void on_gswat_tabbable_tooltip_update(GObject *object,
                                             GParamSpec *property,
                                             gpointer data);
static void update_tabs_visibility(GSwatNotebook *self);
static void on_tab_close_button_clicked(GtkWidget *widget, 
                                        gpointer data);

/* Variables */
static GObjectClass *parent_class = NULL;
static guint gswat_notebook_signals[LAST_SIGNAL] = { 0 };

GType
gswat_notebook_get_type(void) /* Typechecking */
{
    static GType self_type = 0;

    if (!self_type)
    {
        static const GTypeInfo object_info =
        {
            sizeof(GSwatNotebookClass), /* class structure size */
            NULL, /* base class initializer */
            NULL, /* base class finalizer */
            (GClassInitFunc)gswat_notebook_class_init, /* class initializer */
            NULL, /* class finalizer */
            NULL, /* class data */
            sizeof(GSwatNotebook), /* instance structure size */
            0, /* preallocated instances */
            (GInstanceInitFunc)gswat_notebook_init, /* instance initializer */
            NULL /* function table */
        };

        /* add the type of your parent class here */
        self_type = g_type_register_static(GTK_TYPE_NOTEBOOK, /* parent GType */
                                           "GSwatNotebook", /* type name */
                                           &object_info, /* type info */
                                           0 /* flags */
                                          );
#if 0
        /* add interfaces here */
        static const GInterfaceInfo mydoable_info =
        {
            (GInterfaceInitFunc)
                gswat_notebook_mydoable_interface_init,
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
gswat_notebook_class_init(GSwatNotebookClass *klass) /* Class Initialization */
{   
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    //GParamSpec *new_param;

    parent_class = g_type_class_peek_parent(klass);

    gobject_class->finalize = gswat_notebook_finalize;

    gobject_class->get_property = gswat_notebook_get_property;
    gobject_class->set_property = gswat_notebook_set_property;

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

    /* set up signals */
#if 0 /* template code */
    klass->signal_member = signal_default_handler;
    gswat_notebook_signals[SIGNAL_NAME] =
        g_signal_new("signal_name", /* name */
                     G_TYPE_FROM_CLASS(klass), /* interface GType */
                     G_SIGNAL_RUN_LAST, /* signal flags */
                     G_STRUCT_OFFSET(GSwatNotebookClass, signal_member),
                     NULL, /* accumulator */
                     NULL, /* accumulator data */
                     g_cclosure_marshal_VOID__VOID, /* c marshaller */
                     G_TYPE_NONE, /* return type */
                     0 /* number of parameters */
                     /* vararg, list of param types */
                    );
#endif
    klass->tab_added = NULL;
    gswat_notebook_signals[TAB_ADDED] =
        g_signal_new("tab-added", /* name */
                     G_TYPE_FROM_CLASS(klass), /* interface GType */
                     G_SIGNAL_RUN_LAST, /* signal flags */
                     G_STRUCT_OFFSET(GSwatNotebookClass, tab_added),
                     NULL, /* accumulator */
                     NULL, /* accumulator data */
                     g_cclosure_marshal_VOID__OBJECT, /* c marshaller */
                     G_TYPE_NONE, /* return type */
                     1, /* number of parameters */
                     /* vararg, list of param types */
                     GSWAT_TYPE_TABABLE
                    );

    klass->tab_close_request = NULL;
    gswat_notebook_signals[TAB_CLOSE_REQUEST] =
        g_signal_new("tab-close-request", /* name */
                     G_TYPE_FROM_CLASS(klass), /* interface GType */
                     G_SIGNAL_RUN_LAST, /* signal flags */
                     G_STRUCT_OFFSET(GSwatNotebookClass, tab_close_request),
                     NULL, /* accumulator */
                     NULL, /* accumulator data */
                     g_cclosure_marshal_VOID__OBJECT, /* c marshaller */
                     G_TYPE_NONE, /* return type */
                     1, /* number of parameters */
                     /* vararg, list of param types */
                     GSWAT_TYPE_TABABLE
                    );

    g_type_class_add_private(klass, sizeof(GSwatNotebookPrivate));
}

static void
gswat_notebook_get_property(GObject *object,
                            guint id,
                            GValue *value,
                            GParamSpec *pspec)
{
    //GSwatNotebook* self = GSWAT_NOTEBOOK(object);

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
gswat_notebook_set_property(GObject *object,
                            guint property_id,
                            const GValue *value,
                            GParamSpec *pspec)
{   
    //GSwatNotebook* self = GSWAT_NOTEBOOK(object);

    switch(property_id)
    {
#if 0 /* template code */
        case PROP_NAME:
            gswat_notebook_set_property(self, g_value_get_int(value));
            gswat_notebook_set_property(self, g_value_get_uint(value));
            gswat_notebook_set_property(self, g_value_get_boolean(value));
            gswat_notebook_set_property(self, g_value_get_string(value));
            gswat_notebook_set_property(self, g_value_get_object(value));
            gswat_notebook_set_property(self, g_value_get_pointer(value));
            break;
#endif
        default:
            g_warning("gswat_notebook_set_property on unknown property");
            return;
    }
}

/* Initialize interfaces here */

#if 0
static void
gswat_notebook_mydoable_interface_init(gpointer interface,
                                       gpointer data)
{
    MyDoableClass *mydoable = interface;
    g_assert(G_TYPE_FROM_INTERFACE(mydoable) = MY_TYPE_MYDOABLE);

    mydoable->method1 = gswat_notebook_method1;
    mydoable->method2 = gswat_notebook_method2;
}
#endif

/* Instance Construction */
static void
gswat_notebook_init(GSwatNotebook *self)
{
    self->priv = GSWAT_NOTEBOOK_GET_PRIVATE(self);

    gtk_notebook_set_scrollable(GTK_NOTEBOOK(self), TRUE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(self), FALSE);
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(self), TRUE);

    self->priv->tool_tips = gedit_tooltips_new();
    g_object_ref(G_OBJECT(self->priv->tool_tips));
    gtk_object_sink(GTK_OBJECT(self->priv->tool_tips));

    self->priv->always_show_tabs = TRUE;
#if 0
    g_signal_connect_after(G_OBJECT(self), 
                           "switch_page",
                           G_CALLBACK(on_gswat_notebook_switch_page),
                           NULL);
#endif

}

/* Instantiation wrapper */
GSwatNotebook*
gswat_notebook_new(void)
{
    return GSWAT_NOTEBOOK(g_object_new(gswat_notebook_get_type(), NULL));
}

/* Instance Destruction */
void
gswat_notebook_finalize(GObject *object)
{
    GSwatNotebook *self = GSWAT_NOTEBOOK(object);

    g_object_unref(self->priv->tool_tips);

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
 * gswat_notebook_get_PROPERTY:
 * @self:  A GSwatNotebook.
 *
 * Fetches the PROPERTY of the GSwatNotebook. FIXME, add more info!
 *
 * Returns: The value of PROPERTY. FIXME, add more info!
 */
PropType
gswat_notebook_get_PROPERTY(GSwatNotebook *self)
{
    g_return_val_if_fail(GSWAT_IS_NOTEBOOK(self), /* FIXME */);

    //return self->priv->PROPERTY;
    //return g_strdup(self->priv->PROPERTY);
    //return g_object_ref(self->priv->PROPERTY);
}

/**
 * gswat_notebook_set_PROPERTY:
 * @self:  A GSwatNotebook.
 * @property:  The value to set. FIXME, add more info!
 *
 * Sets this properties value.
 *
 * This will also clear the properties previous value.
 */
void
gswat_notebook_set_PROPERTY(GSwatNotebook *self, PropType PROPERTY)
{
    g_return_if_fail(GSWAT_IS_NOTEBOOK(self));

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

void
gswat_notebook_insert_page(GSwatNotebook *self,
                          GSwatTabable *tabable,
                          gint position,
                          gboolean jump_to)
{
    GtkWidget *tab_label;
    GtkWidget *close_button;
    GtkWidget *page_widget;
    
    tab_label = build_blank_tab_label(self);

    close_button = g_object_get_data(G_OBJECT(tab_label), "close-button");
    
    g_object_set_data(G_OBJECT(tabable),
                      "gswat-notepad-label",
                      tab_label);

    g_object_set_data(G_OBJECT(tabable),
                      "gswat-notebook",
                      self);


    g_signal_connect(G_OBJECT(close_button),
                     "clicked",
                     G_CALLBACK(on_tab_close_button_clicked),
                     tabable);

    g_signal_connect(G_OBJECT(tabable),
                     "notify::tab-state",
                     G_CALLBACK(on_gswat_tabbable_state_flags_update),
                     tab_label);

    g_signal_connect(G_OBJECT(tabable),
                     "notify::tab-label-text",
                     G_CALLBACK(on_gswat_tabbable_label_text_update),
                     tab_label);

    g_signal_connect(G_OBJECT(tabable),
                     "notify::tab-icon",
                     G_CALLBACK(on_gswat_tabbable_icon_update),
                     tab_label);

    g_signal_connect(G_OBJECT(tabable),
                     "notify::tab-tooltip",
                     G_CALLBACK(on_gswat_tabbable_tooltip_update),
                     self);

    /* synthesize notifications to synchronize the
     * notebooks label state with the tabable */
    g_object_notify(G_OBJECT(tabable), "tab-state");
    g_object_notify(G_OBJECT(tabable), "tab-label-text");
    g_object_notify(G_OBJECT(tabable), "tab-icon");
    g_object_notify(G_OBJECT(tabable), "tab-tooltip");

    page_widget = gswat_tabable_get_page_widget(tabable);

    gtk_notebook_insert_page(GTK_NOTEBOOK(self), 
                             GTK_WIDGET(page_widget),
                             tab_label, 
                             position);

    update_tabs_visibility(self);

    g_signal_emit(G_OBJECT(self),
                  gswat_notebook_signals[TAB_ADDED],
                  0,
                  tabable);

    /* The signal handler may have reordered the tabs */
    position = gtk_notebook_page_num(GTK_NOTEBOOK(self), 
                                     GTK_WIDGET(page_widget));

    if(jump_to)
    {
        GtkWidget *focal_point;
        gtk_notebook_set_current_page(GTK_NOTEBOOK(self), position);
        focal_point  = gswat_tabable_get_focal_widget(tabable);
        if(focal_point)
        {
            gtk_widget_grab_focus(GTK_WIDGET(focal_point));
            g_object_unref(focal_point);
        }
    }

    g_object_unref(page_widget);

}

static GtkWidget *
build_blank_tab_label(GSwatNotebook *self)
{
    GtkWidget *hbox, *label_hbox, *label_ebox;
    GtkWidget *label, *dummy_label;
    GtkWidget *close_button;
    GtkSettings *settings;
    gint w, h;
    GtkWidget *image;
    GtkWidget *spinner;
    GtkWidget *icon;

    hbox = gtk_hbox_new (FALSE, 0);

    label_ebox = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(label_ebox), FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), label_ebox, TRUE, TRUE, 0);

    label_hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(label_ebox), label_hbox);

    /* setup close button */
    close_button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(close_button),
                          GTK_RELIEF_NONE);
    /* don't allow focus on the close button */
    gtk_button_set_focus_on_click(GTK_BUTTON(close_button), FALSE);

    /* fetch the size of an icon */
    settings = gtk_widget_get_settings(GTK_WIDGET(self));
    gtk_icon_size_lookup_for_settings(settings,
                                      GTK_ICON_SIZE_MENU,
                                      &w,
                                      &h);
    image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
    gtk_widget_set_size_request(close_button, w + 2, h + 2);
    gtk_container_add(GTK_CONTAINER(close_button), image);
    gtk_box_pack_start(GTK_BOX(hbox), close_button, FALSE, FALSE, 0);

    gedit_tooltips_set_tip(self->priv->tool_tips,
                           close_button,
                           _("Close document"),
                           NULL);

    /* setup spinner */
    spinner = gedit_spinner_new();
    gedit_spinner_set_size(GEDIT_SPINNER(spinner), GTK_ICON_SIZE_MENU);
    gtk_box_pack_start(GTK_BOX(label_hbox), spinner, FALSE, FALSE, 0);

    /* setup site icon, empty by default */
    icon = gtk_image_new();
    gtk_box_pack_start(GTK_BOX(label_hbox), icon, FALSE, FALSE, 0);

    /* setup label */
    label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC (label), 2, 0);
    gtk_box_pack_start(GTK_BOX(label_hbox), label, FALSE, FALSE, 0);

    dummy_label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(label_hbox), dummy_label, TRUE, TRUE, 0);

    gtk_widget_show(hbox);
    gtk_widget_show(label_ebox);
    gtk_widget_show(label_hbox);
    gtk_widget_show(label);
    gtk_widget_show(dummy_label);	
    gtk_widget_show(image);
    gtk_widget_show(close_button);
    gtk_widget_show(icon);

    g_object_set_data(G_OBJECT(hbox), "label", label);
    g_object_set_data(G_OBJECT(hbox), "label-ebox", label_ebox);
    g_object_set_data(G_OBJECT(hbox), "spinner", spinner);
    g_object_set_data(G_OBJECT(hbox), "icon", icon);
    g_object_set_data(G_OBJECT(hbox), "close-button", close_button);
    g_object_set_data(G_OBJECT(hbox), "tooltips", self->priv->tool_tips);

    return hbox;
}


static void
on_gswat_tabbable_state_flags_update(GObject *object,
                                     GParamSpec *property,
                                     gpointer data)
{
    GSwatTabable *tabable = GSWAT_TABABLE(object);
    GtkWidget *label = GTK_WIDGET(data);
    GtkWidget *spinner;
    GtkImage *icon;
    gulong flags;

    spinner = GTK_WIDGET(g_object_get_data(G_OBJECT(label),
                                           "spinner"));
    icon = GTK_IMAGE(g_object_get_data(G_OBJECT(label), "icon"));

    flags = gswat_tabable_get_state_flags(tabable);
    if(flags & GSWAT_TABABLE_BUSY)
    {
        gtk_widget_hide(GTK_WIDGET(icon));

        gtk_widget_show(spinner);
        gedit_spinner_start(GEDIT_SPINNER(spinner));
    }
    else
    {
        gtk_widget_hide(spinner);
        gedit_spinner_stop(GEDIT_SPINNER(spinner));

        gtk_widget_show(GTK_WIDGET (icon));
    }
}


static void
on_gswat_tabbable_label_text_update(GObject *object,
                                    GParamSpec *property,
                                    gpointer data)
{
    GSwatTabable *tabable = GSWAT_TABABLE(object);
    GtkWidget *tab_label = GTK_WIDGET(data);
    GtkWidget *text_label;
    gchar *label_str;

    text_label = GTK_WIDGET(g_object_get_data(G_OBJECT(tab_label), "label"));
    label_str = gswat_tabable_get_label_text(tabable);
    if(label_str)
    {
        gtk_label_set_text(GTK_LABEL(text_label), label_str);
        g_free(label_str);
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(text_label), "");
    }
}


static void
on_gswat_tabbable_icon_update(GObject *object,
                              GParamSpec *property,
                              gpointer data)
{
    GSwatTabable *tabable = GSWAT_TABABLE(object);
    GtkWidget *label = GTK_WIDGET(data);
    GtkImage *icon;
    GdkPixbuf *pixbuf;

    icon = GTK_IMAGE(g_object_get_data(G_OBJECT(label), "icon"));

    pixbuf = gswat_tabable_get_icon(tabable);
    if(pixbuf)
    {
        gtk_image_set_from_pixbuf(icon, pixbuf);

        g_object_unref(pixbuf);
    }
    else
    {
        gtk_image_clear(icon);
    }
}


static void
on_gswat_tabbable_tooltip_update(GObject *object,
                                 GParamSpec *property,
                                 gpointer data)
{
    GSwatNotebook *self = GSWAT_NOTEBOOK(data);
    GSwatTabable *tabable = GSWAT_TABABLE(object);
    GtkWidget *label;
    gchar *tooltip;
    
    label = g_object_get_data(G_OBJECT(tabable),
                              "gswat-notepad-label");

    tooltip = gswat_tabable_get_tooltip(tabable);
    gedit_tooltips_set_tip(self->priv->tool_tips,
                           label,
                           tooltip,
                           NULL);
    if(tooltip)
    {
        g_free(tooltip);
    }
}


/*
 * update_tabs_visibility: Hide tabs if there is only one tab
 * and the pref is not set.
 */
static void
update_tabs_visibility(GSwatNotebook *self)
{
    gboolean show_tabs;
    guint num;

    num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(self));

    show_tabs = (self->priv->always_show_tabs || num > 1);

    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(self), show_tabs);
}

#if 0
static void
on_gswat_notebook_switch_page(GtkNotebook *notebook,
                              GtkNotebookPage *page,
                              guint page_num,
                              gpointer data)
{
    GSwatNotebook *self = GSWAT_NOTEBOOK(notebook);
    GtkWidget *child;

    child = gtk_notebook_get_nth_page(notebook, page_num);

    gtk_widget_grab_focus(GTK_WIDGET(child));
}
#endif


static void
on_tab_close_button_clicked(GtkWidget *widget, 
                            gpointer data)
{
    GSwatNotebook *notebook;
    GSwatTabable *tabable = GSWAT_TABABLE(data);

    notebook = g_object_get_data(G_OBJECT(tabable), "gswat-notebook");
    g_signal_emit(notebook,
                  gswat_notebook_signals[TAB_CLOSE_REQUEST],
                  0,
                  tabable);
}

