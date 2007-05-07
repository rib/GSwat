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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>

#include <gdk/gdkkeysyms.h>

#include <glib/gi18n.h>

#include "gswat-utils.h"
#include "gswat-view.h"

/* Function definitions */
static void gswat_view_class_init(GSwatViewClass *klass);
static void gswat_view_get_property(GObject *object,
                                   guint id,
                                   GValue *value,
                                   GParamSpec *pspec);
static void gswat_view_set_property(GObject *object,
                                   guint property_id,
                                   const GValue *value,
                                   GParamSpec *pspec);
//static void gswat_view_mydoable_interface_init(gpointer interface,
//                                             gpointer data);
static void gswat_view_init(GSwatView *self);
static void gswat_view_destroy(GtkObject *object);
static void gswat_view_finalize(GObject *self);

static gboolean gswat_view_expose(GtkWidget *widget,
                                  GdkEventExpose *event);

/* Macros and defines */
#define GSWAT_VIEW_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GSWAT_TYPE_VIEW, GSwatViewPrivate))

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

struct _GSwatViewPrivate
{
    GList *line_highlights;
};

typedef struct {
    gint line;
}GSwatViewBreakpoint;

/* Variables */
static GeditViewClass *parent_class = NULL;
//static guint gswat_view_signals[LAST_SIGNAL] = { 0 };

GType
gswat_view_get_type(void) /* Typechecking */
{
    static GType self_type = 0;

    if (!self_type)
    {
        static const GTypeInfo object_info =
        {
            sizeof(GSwatViewClass), /* class structure size */
            NULL, /* base class initializer */
            NULL, /* base class finalizer */
            (GClassInitFunc)gswat_view_class_init, /* class initializer */
            NULL, /* class finalizer */
            NULL, /* class data */
            sizeof(GSwatView), /* instance structure size */
            0, /* preallocated instances */
            (GInstanceInitFunc)gswat_view_init, /* instance initializer */
            NULL /* function table */
        };

        /* add the type of your parent class here */
        self_type = g_type_register_static(GEDIT_TYPE_VIEW, /* parent GType */
                                           "GSwatView", /* type name */
                                           &object_info, /* type info */
                                           0 /* flags */
                                          );
#if 0
        /* add interfaces here */
        static const GInterfaceInfo mydoable_info =
        {
            (GInterfaceInitFunc)
                gswat_view_mydoable_interface_init,
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
gswat_view_class_init(GSwatViewClass *klass) /* Class Initialization */
{   
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
    //GParamSpec *new_param;

    parent_class = g_type_class_peek_parent(klass);

    gobject_class->finalize = gswat_view_finalize;

    gobject_class->get_property = gswat_view_get_property;
    gobject_class->set_property = gswat_view_set_property;

    gtkobject_class->destroy = gswat_view_destroy;

    widget_class->expose_event = gswat_view_expose;

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
    gswat_view_signals[SIGNAL_NAME] =
        g_signal_new("signal_name", /* name */
                     G_TYPE_FROM_CLASS(klass), /* interface GType */
                     G_SIGNAL_RUN_LAST, /* signal flags */
                     G_STRUCT_OFFSET(GSwatViewClass, signal_member),
                     NULL, /* accumulator */
                     NULL, /* accumulator data */
                     g_cclosure_marshal_VOID__VOID, /* c marshaller */
                     G_TYPE_NONE, /* return type */
                     0 /* number of parameters */
                     /* vararg, list of param types */
                    );
#endif

    g_type_class_add_private(klass, sizeof(GSwatViewPrivate));
}

static void
gswat_view_get_property(GObject *object,
                        guint id,
                        GValue *value,
                        GParamSpec *pspec)
{
    //GSwatView* self = GSWAT_VIEW(object);

    switch(id) {
#if 0 /* template code */
        case PROP_NAME:
            g_value_set_int(value, self->priv->property);
            g_value_set_uint(value, self->priv->property);
            g_value_set_boolean(value, self->priv->property);
            /* don't forget that this will dup the string... */
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
gswat_view_set_property(GObject *object,
                        guint property_id,
                        const GValue *value,
                        GParamSpec *pspec)
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
            g_warning("gswat_view_set_property on unknown property");
            return;
    }
}

/* Initialize interfaces here */

#if 0
static void
gswat_view_mydoable_interface_init(gpointer interface,
                                   gpointer data)
{
    MyDoableClass *mydoable = interface;
    g_assert(G_TYPE_FROM_INTERFACE(mydoable) = MY_TYPE_MYDOABLE);

    mydoable->method1 = gswat_view_method1;
    mydoable->method2 = gswat_view_method2;
}
#endif

/* Instance Construction */
static void
gswat_view_init(GSwatView *self)
{
    self->priv = GSWAT_VIEW_GET_PRIVATE(self);
    /* populate your widget here */
}

/* Instantiation wrapper */
GtkWidget *
gswat_view_new(GeditDocument *doc)
{
    GSwatView *self;

    g_return_val_if_fail(GEDIT_IS_DOCUMENT(doc), NULL);

    self = g_object_new(GSWAT_TYPE_VIEW, NULL);

    gtk_text_view_set_buffer(GTK_TEXT_VIEW(self),
                             GTK_TEXT_BUFFER(doc));

    gtk_text_view_set_editable(GTK_TEXT_VIEW(self), 
                               FALSE);					  

    gtk_widget_show_all(GTK_WIDGET(self));

    return GTK_WIDGET(self);
}

static void
gswat_view_destroy(GtkObject *object)
{
    GSwatView *self;

    self = GSWAT_VIEW(object);

    (* GTK_OBJECT_CLASS(parent_class)->destroy) (object);
}

/* Instance Destruction */
void
gswat_view_finalize(GObject *object)
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



static gboolean
gswat_view_expose(GtkWidget *widget,
                  GdkEventExpose *event)
{
    GSwatView *self = GSWAT_VIEW(widget);
    GtkTextView *text_view = GTK_TEXT_VIEW(widget);
    GList *tmp = NULL;
    GdkRectangle visible_rect;
    GdkRectangle redraw_rect;
    GtkTextIter cur;
    gint y;
    gint height;
    gint win_y;
    GdkGC *gc;

    if ((event->window != gtk_text_view_get_window(text_view, GTK_TEXT_WINDOW_TEXT)))
    {
        goto parent_expose;
    }

    gtk_text_view_get_visible_rect(text_view, &visible_rect);

    gc = gdk_gc_new(GDK_DRAWABLE(event->window));

    for(tmp=self->priv->line_highlights; tmp!=NULL; tmp=tmp->next)
    {
        GSwatViewLineHighlight *current_line_highlight;
        current_line_highlight = (GSwatViewLineHighlight *)tmp->data;

        gtk_text_buffer_get_iter_at_line(text_view->buffer,
                                         &cur,
                                         current_line_highlight->line - 1);

        gtk_text_view_get_line_yrange(text_view, &cur, &y, &height);

        gtk_text_view_buffer_to_window_coords(text_view,
                                              GTK_TEXT_WINDOW_TEXT,
                                              visible_rect.x,
                                              visible_rect.y,
                                              &redraw_rect.x,
                                              &redraw_rect.y);

        redraw_rect.width = visible_rect.width;
        redraw_rect.height = visible_rect.height;

        gtk_text_view_buffer_to_window_coords(text_view,
                                              GTK_TEXT_WINDOW_TEXT,
                                              0,
                                              y,
                                              NULL,
                                              &win_y);

        if(win_y > (redraw_rect.y + redraw_rect.height)
           || (win_y + height) < redraw_rect.y
          )
        {
            /* line not visible */
            continue;
        }

        gdk_gc_set_rgb_fg_color(gc, &(current_line_highlight->color));
        gdk_draw_rectangle(event->window,
                           gc,
                           TRUE,
                           redraw_rect.x + MAX(0, gtk_text_view_get_left_margin(text_view) - 1),
                           win_y,
                           redraw_rect.width,
                           height);

    }

    g_object_unref(gc);


parent_expose:
    {
        GtkWidgetClass *widget_class;
        widget_class = GTK_WIDGET_CLASS(parent_class);
        return widget_class->expose_event(widget, event);
    }
    //return (* GTK_WIDGET_CLASS (gswat_view_parent_class)->expose_event)(widget, event);

}


void
gswat_view_set_line_highlights(GSwatView *self,
                               const GList *line_highlights)
{
    GList *tmp, *copy;
    GdkWindow *window;

    /* why doesn't g_list_copy take (const GList *) input? */
    copy=g_list_copy((GList *)line_highlights);

    /* Do a deep copy of the list */
    for(tmp=copy; tmp!=NULL; tmp=tmp->next)
    {
        GSwatViewLineHighlight *current_highlight;
        GSwatViewLineHighlight *new_line_highlight;

        current_highlight=(GSwatViewLineHighlight *)tmp->data;

        new_line_highlight = g_new(GSwatViewLineHighlight, 1);
        memcpy(new_line_highlight, current_highlight, sizeof(GSwatViewLineHighlight));
        tmp->data=new_line_highlight;
    }
    self->priv->line_highlights = copy;

    /* trigger an expose to draw any new highlighted lines */
    window = gtk_text_view_get_window(GTK_TEXT_VIEW(self),
                                      GTK_TEXT_WINDOW_TEXT);
    gdk_window_invalidate_rect(window,
                               NULL,
                               FALSE);
}


GeditDocument *
gswat_view_get_document(GSwatView *self)
{
    return  GEDIT_DOCUMENT(
                           gtk_text_view_get_buffer(GTK_TEXT_VIEW (self))
                          );
}

