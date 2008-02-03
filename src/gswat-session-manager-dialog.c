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
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <glade/glade.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

//#include "gswat-session.h"
#include "gswat-session-manager-dialog.h"

/* Macros and defines */
#define GSWAT_SESSION_MANAGER_DIALOG_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GSWAT_TYPE_GSWAT_SESSION_MANAGER_DIALOG, GSwatSessionManagerDialogPrivate))

/* Enums/Typedefs */
enum {
    SIG_EDIT_DONE,
    SIG_EDIT_ABORT,
    LAST_SIGNAL
};

enum {
    NAME_LIST_COLUMN_NAME,
    NAME_LIST_COLUMN_SESSION,
    NAME_LIST_N_COLUMNS
};

enum {
    NOTEBOOK_PAGE_RUN_LOCAL,
    NOTEBOOK_PAGE_NETWORK,
    NOTEBOOK_PAGE_SERIAL
};

#if 0
enum {
    PROP_0,
    PROP_NAME,
};
#endif

struct _GSwatSessionManagerDialogPrivate
{
    GladeXML *xml;

    GtkWidget *dialog;

    GtkListStore *name_list_store;
    GtkWidget *name_combo;

    GtkWidget *target_notebook;

    /* run local target tab */
    GtkWidget *command_entry;

    /* network target tab */
    GtkWidget *network_hostname_entry;
    GtkWidget *network_port_entry;
    GtkWidget *network_command_entry;
    /* ... */

    /* serial target tab */
    GtkWidget *serial_combo;

    /* all target stuff */
    GtkWidget *working_dir_entry;
    

    GSwatSessionManager *manager;
    GSwatSession *current_session;
    gint current_name_index;
    GSwatSession *partial_session;

};


/* Function definitions */
static void gswat_session_manager_dialog_class_init(GSwatSessionManagerDialogClass *klass);
static void gswat_session_manager_dialog_get_property(GObject *object,
						      guint id,
						      GValue *value,
						      GParamSpec *pspec);
static void gswat_session_manager_dialog_set_property(GObject *object,
						      guint property_id,
						      const GValue *value,
						      GParamSpec *pspec);
//static void gswat_session_manager_dialog_mydoable_interface_init(gpointer interface,
//                                             gpointer data);
static void gswat_session_manager_dialog_init(GSwatSessionManagerDialog *self);
static void gswat_session_manager_dialog_finalize(GObject *self);

static void construct_dialog(GSwatSessionManagerDialog *self);
static void construct_dialog_name_combo(GSwatSessionManagerDialog *self);
static void destruct_dialog(GSwatSessionManagerDialog *self);
static gint compare_sessions_by_date(GSwatSession *session0,
				     GSwatSession *session1);
static void update_dialog(GSwatSessionManagerDialog *self);
//static void update_current_session_properties(GSwatSessionManagerDialog *self);
static void on_command_entry_changed(GtkEntry *command_entry,
				     GSwatSessionManagerDialog *self);
static void on_working_dir_entry_changed(GtkEntry *working_dir_entry,
					 GSwatSessionManagerDialog *self);

static void on_command_browse_button_clicked(GtkButton *button,
					     GSwatSessionManagerDialog *self);
static void on_working_dir_browse_button_clicked(GtkButton *button,
						 GSwatSessionManagerDialog *self);
static void on_ok_button_clicked(GtkButton *button,
				 GSwatSessionManagerDialog *self);
static gboolean check_field_set(const char *string,
				GtkWidget *focus_on_error_widget,
				const char *error_msg);
static gboolean check_run_local_target(GSwatSessionManagerDialog *self);
static gboolean check_gdbserver_network_target(GSwatSessionManagerDialog *self);
static gboolean check_gdbserver_serial_target(GSwatSessionManagerDialog *self);
static gboolean check_common_fields(GSwatSessionManagerDialog *self);
static void on_cancel_button_clicked(GtkButton *button,
				     GSwatSessionManagerDialog *self);
static gboolean on_delete_event(GtkWidget *widget,
				GdkEvent *event,
				GSwatSessionManagerDialog *self);
//static gboolean on_name_combo_focus_out(GtkWidget *widget,
//					GdkEventFocus *event,
//					GSwatSessionManagerDialog *self);
static void on_name_combo_active_change(GObject *object,
					GParamSpec *property,
					GSwatSessionManagerDialog *self);
//static void on_name_combo_activated(GtkWidget *widget,
//				    GSwatSessionManagerDialog *self);
//static void activate_new_session_from_name(GSwatSessionManagerDialog *self,
//					   const gchar *name);
//static GSwatSession *lookup_session(GSwatSessionManagerDialog *self,
//				    const gchar *name);
//static gboolean on_name_combo_key_press(GtkWidget *widget,
//					GdkEventKey *event,
//					GSwatSessionManagerDialog *self);
//static void on_name_combo_change(GtkComboBoxEntry *name_combo,
//				 GSwatSessionManagerDialog *self);
static void abort_edit(GSwatSessionManagerDialog *self);

/* Variables */
static GObjectClass *parent_class = NULL;
static guint gswat_session_manager_dialog_signals[LAST_SIGNAL] = { 0 };


GType
gswat_session_manager_dialog_get_type(void) /* Typechecking */
{
    static GType self_type = 0;

    if (!self_type)
    {
	static const GTypeInfo object_info =
	{
	    sizeof(GSwatSessionManagerDialogClass), /* class structure size */
	    NULL, /* base class initializer */
	    NULL, /* base class finalizer */
	    (GClassInitFunc)gswat_session_manager_dialog_class_init, /* class initializer */
	    NULL, /* class finalizer */
	    NULL, /* class data */
	    sizeof(GSwatSessionManagerDialog), /* instance structure size */
	    0, /* preallocated instances */
	    (GInstanceInitFunc)gswat_session_manager_dialog_init, /* instance initializer */
	    NULL /* function table */
	};

	/* add the type of your parent class here */
	self_type = g_type_register_static(G_TYPE_OBJECT, /* parent GType */
					   "GSwatSessionManagerDialog", /* type name */
					   &object_info, /* type info */
					   0 /* flags */
					  );
#if 0
	/* add interfaces here */
	static const GInterfaceInfo mydoable_info =
	{
	    (GInterfaceInitFunc)
		gswat_session_manager_dialog_mydoable_interface_init,
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
gswat_session_manager_dialog_class_init(GSwatSessionManagerDialogClass *klass) /* Class Initialization */
{   
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    //GParamSpec *new_param;

    parent_class = g_type_class_peek_parent(klass);

    gobject_class->finalize = gswat_session_manager_dialog_finalize;

    gobject_class->get_property = gswat_session_manager_dialog_get_property;
    gobject_class->set_property = gswat_session_manager_dialog_set_property;

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
    gswat_session_manager_dialog_signals[SIGNAL_NAME] =
	g_signal_new("signal_name", /* name */
		     G_TYPE_FROM_CLASS(klass), /* interface GType */
		     G_SIGNAL_RUN_LAST, /* signal flags */
		     G_STRUCT_OFFSET(GSwatSessionManagerDialogClass, signal_member),
		     NULL, /* accumulator */
		     NULL, /* accumulator data */
		     g_cclosure_marshal_VOID__VOID, /* c marshaller */
		     G_TYPE_NONE, /* return type */
		     0 /* number of parameters */
		     /* vararg, list of param types */
		    );
#endif

    klass->edit_done=NULL;
    klass->edit_abort=NULL;

    gswat_session_manager_dialog_signals[SIG_EDIT_DONE] =
	g_signal_new("edit-done", /* name */
		     G_TYPE_FROM_CLASS(klass), /* interface GType */
		     G_SIGNAL_RUN_LAST, /* signal flags */
		     G_STRUCT_OFFSET(GSwatSessionManagerDialogClass, edit_done),
		     NULL, /* accumulator */
		     NULL, /* accumulator data */
		     g_cclosure_marshal_VOID__VOID, /* c marshaller */
		     G_TYPE_NONE, /* return type */
		     0 /* number of parameters */
		     /* vararg, list of param types */
		    );

    gswat_session_manager_dialog_signals[SIG_EDIT_ABORT] =
	g_signal_new("edit-abort", /* name */
		     G_TYPE_FROM_CLASS(klass), /* interface GType */
		     G_SIGNAL_RUN_LAST, /* signal flags */
		     G_STRUCT_OFFSET(GSwatSessionManagerDialogClass, edit_abort),
		     NULL, /* accumulator */
		     NULL, /* accumulator data */
		     g_cclosure_marshal_VOID__VOID, /* c marshaller */
		     G_TYPE_NONE, /* return type */
		     0 /* number of parameters */
		     /* vararg, list of param types */
		    );


    g_type_class_add_private(klass, sizeof(GSwatSessionManagerDialogPrivate));
}

static void
gswat_session_manager_dialog_get_property(GObject *object,
					  guint id,
					  GValue *value,
					  GParamSpec *pspec)
{
    //GSwatSessionManagerDialog* self = GSWAT_SESSION_MANAGER_DIALOG(object);

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
gswat_session_manager_dialog_set_property(GObject *object,
					  guint property_id,
					  const GValue *value,
					  GParamSpec *pspec)
{   
    //GSwatSessionManagerDialog* self = GSWAT_SESSION_MANAGER_DIALOG(object);

    switch(property_id)
    {
#if 0 /* template code */
	case PROP_NAME:
	    gswat_session_manager_dialog_set_property(self, g_value_get_int(value));
	    gswat_session_manager_dialog_set_property(self, g_value_get_uint(value));
	    gswat_session_manager_dialog_set_property(self, g_value_get_boolean(value));
	    gswat_session_manager_dialog_set_property(self, g_value_get_string(value));
	    gswat_session_manager_dialog_set_property(self, g_value_get_object(value));
	    gswat_session_manager_dialog_set_property(self, g_value_get_pointer(value));
	    break;
#endif
	default:
	    g_warning("gswat_session_manager_dialog_set_property on unknown property");
	    return;
    }
}

/* Initialize interfaces here */

#if 0
static void
gswat_session_manager_dialog_mydoable_interface_init(gpointer interface,
						     gpointer data)
{
    MyDoableIface *mydoable = interface;
    g_assert(G_TYPE_FROM_INTERFACE(mydoable) == MY_TYPE_MYDOABLE);

    mydoable->method1 = gswat_session_manager_dialog_method1;
    mydoable->method2 = gswat_session_manager_dialog_method2;
}
#endif


/* Instance Construction */
static void
gswat_session_manager_dialog_init(GSwatSessionManagerDialog *self)
{
    self->priv = GSWAT_SESSION_MANAGER_DIALOG_GET_PRIVATE(self);
    /* populate your object here */
    self->priv->partial_session = gswat_session_new();
    self->priv->current_session = self->priv->partial_session;
    self->priv->current_name_index = 0;

    gswat_session_set_name(self->priv->partial_session,
			   "New Session");
    gswat_session_set_working_dir(self->priv->partial_session,
				  g_get_current_dir());
}


/* Instantiation wrapper */
GSwatSessionManagerDialog*
gswat_session_manager_dialog_new(GSwatSessionManager *manager)
{
    GSwatSessionManagerDialog *self;

    self = GSWAT_SESSION_MANAGER_DIALOG(g_object_new(gswat_session_manager_dialog_get_type(), NULL));

    self->priv->manager = manager;

    return self;
}


/* Instance Destruction */
void
gswat_session_manager_dialog_finalize(GObject *object)
{
    //GSwatSessionManagerDialog *self = GSWAT_SESSION_MANAGER_DIALOG(object);

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
 * gswat_session_manager_dialog_get_PROPERTY:
 * @self:  A GSwatSessionManagerDialog.
 *
 * Fetches the PROPERTY of the GSwatSessionManagerDialog. FIXME, add more info!
 *
 * Returns: The value of PROPERTY. FIXME, add more info!
 */
PropType
gswat_session_manager_dialog_get_PROPERTY(GSwatSessionManagerDialog *self)
{
    g_return_val_if_fail(GSWAT_IS_GSWAT_SESSION_MANAGER_DIALOG(self), /* FIXME */);

    //return self->priv->PROPERTY;
    //return g_strdup(self->priv->PROPERTY);
    //return g_object_ref(self->priv->PROPERTY);
}

/**
 * gswat_session_manager_dialog_set_PROPERTY:
 * @self:  A GSwatSessionManagerDialog.
 * @property:  The value to set. FIXME, add more info!
 *
 * Sets this properties value.
 *
 * This will also clear the properties previous value.
 */
void
gswat_session_manager_dialog_set_PROPERTY(GSwatSessionManagerDialog *self, PropType PROPERTY)
{
    g_return_if_fail(GSWAT_IS_GSWAT_SESSION_MANAGER_DIALOG(self));

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
gswat_session_manager_dialog_edit(GSwatSessionManagerDialog *self)
{
    if(!self->priv->dialog)
    {
	construct_dialog(self);
    }
    gtk_widget_show(self->priv->dialog);
}


static void
construct_dialog(GSwatSessionManagerDialog *self)
{
    GladeXML  *xml;
    GtkWidget *dialog;
    GtkWidget *ok_button;
    GtkWidget *cancel_button;
    GtkWidget *target_notebook;
    GtkWidget *serial_align;
    GtkWidget *serial_combo;
    GtkWidget *command_entry;
    GtkWidget *command_browse_button;
    GtkWidget *working_dir_entry;
    GtkWidget *working_dir_browse_button;
    GtkWidget *network_hostname_entry;
    GtkWidget *network_port_entry;
    GtkWidget *network_command_entry;

    /* load the session dialog */
    xml = glade_xml_new(GSWAT_GLADEDIR "gswat.glade",
			"gswat_session_manager_dialog",
			NULL);

    /* in case we can't load the interface, bail */
    if(!xml) {
	g_warning("We could not load the interface!");
	gtk_main_quit();
    }

    self->priv->xml = xml;
    
    dialog = glade_xml_get_widget(xml, "gswat_session_manager_dialog");
    self->priv->dialog = dialog;

#define PREFIX(STR) "gswat_session_manager_dialog_" STR
    serial_align =
	glade_xml_get_widget(xml, PREFIX("serial_align"));
    target_notebook =
	glade_xml_get_widget(xml, PREFIX("target_notebook"));
    command_entry =
	glade_xml_get_widget(xml, PREFIX("command_entry"));
    command_browse_button =
	glade_xml_get_widget(xml, PREFIX("command_browse_button"));
    working_dir_entry =
	glade_xml_get_widget(xml, PREFIX("working_dir_entry"));
    working_dir_browse_button =
	glade_xml_get_widget(xml, PREFIX("working_dir_browse_button"));
    ok_button =
	glade_xml_get_widget(xml, PREFIX("ok_button"));
    cancel_button =
	glade_xml_get_widget(xml, PREFIX("cancel_button"));
    network_hostname_entry =
	glade_xml_get_widget(xml, PREFIX("network_hostname_entry"));
    network_port_entry =
	glade_xml_get_widget(xml, PREFIX("network_port_entry"));
    network_command_entry =
	glade_xml_get_widget(xml, PREFIX("network_command_entry"));
#undef PREFIX
    
    self->priv->target_notebook = target_notebook;
    self->priv->command_entry = command_entry;
    self->priv->working_dir_entry = working_dir_entry;
    self->priv->network_hostname_entry = network_hostname_entry;
    self->priv->network_port_entry = network_port_entry;
    self->priv->network_command_entry = network_command_entry;

    g_signal_connect(command_entry,
		     "changed",
		     G_CALLBACK(on_command_entry_changed),
		     self);

    g_signal_connect(command_browse_button,
		     "clicked",
		     G_CALLBACK(on_command_browse_button_clicked),
		     self);

    g_signal_connect(working_dir_entry,
		     "changed",
		     G_CALLBACK(on_working_dir_entry_changed),
		     self);

    g_signal_connect(working_dir_browse_button,
		     "clicked",
		     G_CALLBACK(on_working_dir_browse_button_clicked),
		     self);

    g_signal_connect(ok_button,
		     "clicked",
		     G_CALLBACK(on_ok_button_clicked),
		     self);

    g_signal_connect(cancel_button,
		     "clicked",
		     G_CALLBACK(on_cancel_button_clicked),
		     self);

    g_signal_connect(dialog,
		     "delete-event",
		     G_CALLBACK(on_delete_event),
		     self);


    serial_combo = gtk_combo_box_new_text();
    self->priv->serial_combo = serial_combo;
    gtk_container_add(GTK_CONTAINER(serial_align), serial_combo);
    gtk_widget_show(serial_combo);

    /* Setup the serial port combo box */
    gtk_combo_box_append_text(GTK_COMBO_BOX(serial_combo), "/dev/ttyS0");
    gtk_combo_box_append_text(GTK_COMBO_BOX(serial_combo), "/dev/ttyS1");
    gtk_combo_box_append_text(GTK_COMBO_BOX(serial_combo), "/dev/ttyS2");
    gtk_combo_box_append_text(GTK_COMBO_BOX(serial_combo), "/dev/ttyS3");
    gtk_combo_box_set_active(GTK_COMBO_BOX(serial_combo), 0);

    construct_dialog_name_combo(self);

    update_dialog(self);
}

static void
construct_dialog_name_combo(GSwatSessionManagerDialog *self)
{
    GtkWidget *name_align;
    GtkWidget *name_combo;
    GtkListStore *list_store;
    GtkTreeIter iter;
    GList *sessions, *tmp;
    //GtkEntry *entry;

    name_align =
	glade_xml_get_widget(self->priv->xml,
			     "gswat_session_manager_dialog_name_align");

    list_store = gtk_list_store_new(NAME_LIST_N_COLUMNS,
				    G_TYPE_STRING,
				    G_TYPE_OBJECT);
    self->priv->name_list_store = list_store;

    gtk_list_store_append(list_store, &iter);
    gtk_list_store_set(list_store, &iter,
		       NAME_LIST_COLUMN_NAME, 
		       gswat_session_get_name(self->priv->partial_session),
		       NAME_LIST_COLUMN_SESSION,
		       self->priv->partial_session,
		       -1);

    sessions = gswat_session_manager_get_sessions(self->priv->manager);
    sessions = g_list_sort(sessions, (GCompareFunc)compare_sessions_by_date);
    for(tmp=sessions; tmp!=NULL; tmp=tmp->next)
    {
	GSwatSession *current_session = tmp->data;
	GtkTreeIter iter;
	gchar *name = gswat_session_get_name(current_session);

	gtk_list_store_append(list_store, &iter);
	gtk_list_store_set(list_store, &iter,
			   NAME_LIST_COLUMN_NAME, 
			   name,
			   NAME_LIST_COLUMN_SESSION,
			   current_session,
			   -1);
	g_free(name);
    }

    name_combo =
	gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(list_store),
					   NAME_LIST_COLUMN_NAME);
    self->priv->name_combo = name_combo;

    gtk_widget_add_events(name_combo, GDK_FOCUS_CHANGE_MASK);
    gtk_container_add(GTK_CONTAINER(name_align), name_combo);

    //entry = gtk_bin_get_child(GTK_BIN(name_combo));
    //gtk_widget_add_events(entry, GDK_FOCUS_CHANGE_MASK);

#if 0
    g_signal_connect(name_combo,
		     "changed",
		     G_CALLBACK(on_name_combo_change),
		     self);
#endif

    g_signal_connect(G_OBJECT(name_combo),
		     "notify::active",
		     G_CALLBACK(on_name_combo_active_change),
		     self);

#if 0
    g_signal_connect(entry,
		     "activate",
		     G_CALLBACK(on_name_combo_activated),
		     self);
#endif

#if 0
    g_signal_connect(entry,
		     "focus-out-event",
		     G_CALLBACK(on_name_combo_focus_out),
		     self);
#endif

#if 0
    g_signal_connect(entry,
		     "key-press-event",
		     G_CALLBACK(on_name_combo_key_press),
		     self);
#endif

    gtk_combo_box_set_active(GTK_COMBO_BOX(name_combo), 0);
    gtk_widget_show(name_combo);
    gswat_session_manager_unref_sessions(sessions);

}

static void
destruct_dialog(GSwatSessionManagerDialog *self)
{
    gtk_widget_hide(self->priv->dialog);
    g_object_unref(self->priv->xml);
    gtk_widget_destroy(self->priv->name_combo);
    gtk_widget_destroy(self->priv->serial_combo);

    gtk_widget_destroy(self->priv->dialog);
    self->priv->dialog = NULL;
}


static gint
compare_sessions_by_date(GSwatSession *session0, GSwatSession *session1)
{
    return (gint)(
		  gswat_session_get_access_time(session1)
		  - gswat_session_get_access_time(session0));
}

#if 0
static void
update_dialog(GSwatSessionManagerDialog *self)
{
    g_debug("set current session=%p\n", session);

    if(self->priv->current_session)
    {
	g_object_unref(self->priv->current_session);
    }
    self->priv->current_session = session;
    g_object_ref(self->priv->current_session);

    update_current_session_properties(self);
}
#endif


static void
update_dialog(GSwatSessionManagerDialog *self)
{
    GSwatSession *session = self->priv->current_session;
    gchar *target_type;
    gchar *working_dir;

    g_return_if_fail(session!=NULL);

    /* Give the "command" entry a default */
    target_type = gswat_session_get_target_type(session);
    if(!target_type ||
       strcmp(gswat_session_get_target_type(session), "Run Local") == 0
      )
    {
	gchar *target = gswat_session_get_target(session);
	if(target)
	{
	    gtk_entry_set_text(GTK_ENTRY(self->priv->command_entry),
			       target);
	}
	else
	{
	    gtk_entry_set_text(GTK_ENTRY(self->priv->command_entry),
			       "");
	}
	g_free(target);
    }
    g_free(target_type);

    /* Give the "working dir" entry a default */
    working_dir = gswat_session_get_working_dir(session);
    if(gswat_session_get_working_dir(session))
    {
	gtk_entry_set_text(GTK_ENTRY(self->priv->working_dir_entry),
			   working_dir);
    }
    else
    {
	gtk_entry_set_text(GTK_ENTRY(self->priv->working_dir_entry),
			   "");
    }
    g_free(working_dir);

    /* TODO - populate the list of environment variables */

}


GSwatSession *
gswat_session_manager_dialog_get_current_session(GSwatSessionManagerDialog *self)
{
    if(!self->priv->current_session)
    {
	return NULL;
    }
    return g_object_ref(self->priv->current_session);
}


static void
on_command_entry_changed(GtkEntry *command_entry,
			 GSwatSessionManagerDialog *self)
{
    GSwatSession *session = self->priv->current_session;
    const gchar *target;

    gswat_session_set_target_type(session, "Run Local");

    target = gtk_entry_get_text(GTK_ENTRY(command_entry));
    gswat_session_set_target(session, target);
}


static void
on_working_dir_entry_changed(GtkEntry *working_dir_entry,
			     GSwatSessionManagerDialog *self)
{
    GSwatSession *session = self->priv->current_session;
    const gchar *working_dir;

    working_dir = gtk_entry_get_text(GTK_ENTRY(working_dir_entry));
    gswat_session_set_working_dir(session, working_dir);
}

static void
on_command_browse_button_clicked(GtkButton *button,
				 GSwatSessionManagerDialog *self)
{
    GtkWidget *dialog;
    GtkFileFilter *filter;
    gchar *filename;

    dialog =
	gtk_file_chooser_dialog_new("Select program to debug",
				    NULL,
				    GTK_FILE_CHOOSER_ACTION_OPEN,
				    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				    NULL);
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, "Programs");
    gtk_file_filter_add_mime_type(filter, "application/x-executable");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(dialog), filter);

    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
	filename =
	    gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	gtk_entry_set_text(GTK_ENTRY(self->priv->command_entry),
			   filename);
    }
    gtk_widget_destroy(dialog);
}

static void
on_working_dir_browse_button_clicked(GtkButton *button,
				     GSwatSessionManagerDialog *self)
{
    GtkWidget *dialog;
    gchar *dirname;

    dialog =
	gtk_file_chooser_dialog_new("Select working directory",
				    NULL,
				    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
				    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				    NULL);
    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
	dirname =
	    gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	gtk_entry_set_text(GTK_ENTRY(self->priv->working_dir_entry),
			   dirname);
    }
    gtk_widget_destroy(dialog);
}

static void
on_ok_button_clicked(GtkButton *button,
		     GSwatSessionManagerDialog *self)
{
    GTimeVal time;
    //gint page;

    /* Check that the current session looks sensisble */

    if(self->priv->current_session == self->priv->partial_session)
    {
	GSwatSession *session;
	session =
	    gswat_session_manager_find_session(self->priv->manager,
					       gswat_session_get_name(self->priv->partial_session));
	if(session)
	{
	    GtkWidget *dialog;
	    gint response;

	    g_object_unref(session);

	    dialog =
		gtk_message_dialog_new(NULL,
				       GTK_DIALOG_MODAL,
				       GTK_MESSAGE_QUESTION,
				       GTK_BUTTONS_NONE,
				       _("Your new session name conflicts with "
					 "the name of an existing session. "
					 "Do you want to overwrite the old "
					 "session?"));
	    gtk_dialog_add_button(GTK_DIALOG(dialog),
				  "Keep old session",
				  GTK_RESPONSE_NO);
	    gtk_dialog_add_button(GTK_DIALOG(dialog),
				  "Overwrite old session",
				  GTK_RESPONSE_YES);
	    gtk_dialog_set_default_response(GTK_DIALOG(dialog),
					    GTK_RESPONSE_NO);
	    response = gtk_dialog_run(GTK_DIALOG(dialog));
	    gtk_widget_destroy(dialog);
	    if(response == GTK_RESPONSE_NO)
	    {
		gtk_widget_grab_focus(self->priv->name_combo);
		return;
	    }
	}
    }

    /* determine the type of session (local, network or serial)
     * and perform appropriate sanity checks */
    /* TODO - at some point I want the session types to be
     * extensible via plugins */
    if(strcmp("Run Local",
	      gswat_session_get_target_type(self->priv->current_session)) == 0)
    {
	if(!check_run_local_target(self))
	    return;
    }
    else if(strcmp("gdbserver network",
		   gswat_session_get_target_type(self->priv->current_session)) == 0)
    {
	if(!check_gdbserver_network_target(self))
	    return;
    }
    else if(strcmp("gdbserver serial",
		   gswat_session_get_target_type(self->priv->current_session)) == 0)
    {
	if(!check_gdbserver_serial_target(self))
	    return;
    }
    else
    {
	/* TODO present a dialog, and let the user decide if they wan't
	 * to let this through. */
	g_warning("Un-expected session type\n");
	return;
    }

#if 0    
    page = gtk_notebook_get_current_page(GTK_NOTEBOOK(self->priv->target_notebook));
    switch(session_type)
    {
	case NOTEBOOK_PAGE_RUN_LOCAL:
	    break;
	case NOTEBOOK_PAGE_NETWORK:
	    break;
	case NOTEBOOK_PAGE_SERIAL:
	    break;
	default:
	    g_warning("Un-expected current page; can't determine"
		      " the session type!\n");
	    return;
    }
#endif

    if(!check_common_fields(self))
	return;

    g_get_current_time(&time);
    gswat_session_set_access_time(self->priv->current_session,
				  time.tv_sec);
    gswat_session_manager_save_sessions(self->priv->manager);

    destruct_dialog(self);

    g_signal_emit_by_name(self, "edit-done");
}


static gboolean
check_field_set(const char *string,
		GtkWidget *focus_on_error_widget,
		const char *error_msg)
{
    if(!string || strlen(string)==0)
    {	
	GtkWidget *dialog;

	dialog =
	    gtk_message_dialog_new(NULL,
				   GTK_DIALOG_MODAL,
				   GTK_MESSAGE_ERROR,
				   GTK_BUTTONS_OK,
				   error_msg);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	gtk_widget_grab_focus(focus_on_error_widget);
	return FALSE;
    }
    return TRUE;
}


static gboolean
check_run_local_target(GSwatSessionManagerDialog *self)
{
    gchar *command;
    gboolean set;

    command = gswat_session_get_target(self->priv->current_session);
    set = check_field_set(command,
			    self->priv->command_entry,
			    _("You need to enter a command to run"));
    g_free(command);
    if(!set)
	return FALSE;

    return TRUE;
}


static gboolean
check_gdbserver_network_target(GSwatSessionManagerDialog *self)
{
    const gchar *hostname, *port;
    gchar *command;
    gboolean set;

    hostname = gtk_entry_get_text(GTK_ENTRY(self->priv->network_hostname_entry));
    set = check_field_set(hostname,
			    self->priv->network_hostname_entry,
			    _("You need to enter a hostname"));
    if(!set)
	return FALSE;

    port = gtk_entry_get_text(GTK_ENTRY(self->priv->network_port_entry));
    set = check_field_set(command,
			    self->priv->network_port_entry,
			    _("You need to enter a port to connect to"));
    if(!set)
	return FALSE;

    command = gswat_session_get_target(self->priv->current_session);
    set = check_field_set(command,
			    self->priv->network_command_entry,
			    _("You need to enter a command to run"));
    g_free(command);
    if(!set)
	return FALSE;

    return TRUE;
}


static gboolean
check_gdbserver_serial_target(GSwatSessionManagerDialog *self)
{
    /* TODO stat the device node */
    /* Check we have read/write permissions */

    return TRUE;
}


static gboolean
check_common_fields(GSwatSessionManagerDialog *self)
{
    gchar *working_dir;
    struct stat buf;
    gboolean set;

    working_dir =
	gswat_session_get_working_dir(self->priv->current_session);
    set = check_field_set(working_dir,
			    self->priv->working_dir_entry,
			    _("You need to enter a working_dir"));
    if(!set)
    {
	g_free(working_dir);
	return FALSE;
    }

    if(g_stat(working_dir, &buf) != 0)
    {
	GtkWidget *dialog;

	dialog =
	    gtk_message_dialog_new(NULL,
				   GTK_DIALOG_MODAL,
				   GTK_MESSAGE_ERROR,
				   GTK_BUTTONS_OK,
				   _("Can't find the working directory that you entered"));

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	gtk_widget_grab_focus(self->priv->working_dir_entry);
	g_free(working_dir);
	return FALSE;
    }

    g_free(working_dir);
    return TRUE;
}


static void
on_cancel_button_clicked(GtkButton *button,
			 GSwatSessionManagerDialog *self)
{
    abort_edit(self);
}


static gboolean
on_delete_event(GtkWidget *widget,
		GdkEvent *event,
		GSwatSessionManagerDialog *self)
{
    abort_edit(self);

    return FALSE;
}

static void
on_name_combo_active_change(GObject *object,
			    GParamSpec *property,
			    GSwatSessionManagerDialog *self)
{
    GtkComboBox *combo = GTK_COMBO_BOX(object);
    gchar *name = gtk_combo_box_get_active_text(combo);
    gint active;
    GtkTreeModel *model = GTK_TREE_MODEL(self->priv->name_list_store);
    GtkTreeIter iter;
    gboolean valid;
    GSwatSession *session;

    active = gtk_combo_box_get_active(GTK_COMBO_BOX(object));
    valid = gtk_combo_box_get_active_iter(GTK_COMBO_BOX(object), &iter);
    g_debug("COMBO active_change active=%d name=%s\n", active, name);
    if(!valid)
    {
	valid =
	    gtk_tree_model_iter_nth_child(model,
					  &iter,
					  NULL,
					  self->priv->current_name_index);
	g_assert(valid);

	gtk_tree_model_get(model,
			   &iter,
			   NAME_LIST_COLUMN_SESSION, &session,
			   -1);
	gtk_list_store_set(GTK_LIST_STORE(model),
			   &iter,
			   NAME_LIST_COLUMN_NAME, name);
	gswat_session_set_name(session, name);
	g_object_unref(session);
#if 0
	/* FIXME - we wouldn't have to bother iterating
	 * the model if we just tracked the current index
	 * into the model and used _get_iter_nth() */

	valid = gtk_tree_model_get_iter_first(model, &iter);

	while(valid)
	{
	    gtk_tree_model_get(model,
			       &iter,
			       NAME_LIST_COLUMN_SESSION, &session,
			       -1);
	    valid = gtk_tree_model_get_iter_next(model, &iter);
	    if(session == self->priv->current_session)
	    {
		gtk_list_store(GTK_LIST_STORE(model),
			       &iter,
			       NAME_LIST_COLUMN_NAME, name);
		gswat_session_set_name(session, name);
		g_object_unref(session);
		break;
	    }
	    g_object_unref(session);
	}
#endif
    }
    else
    {
	//valid = gtk_tree_model_iter_nth_child(model, &iter, NULL, active);
	//g_assert(valid);
	gtk_tree_model_get(model,
			   &iter,
			   NAME_LIST_COLUMN_SESSION, &session,
			   -1);
	self->priv->current_session = session;
	self->priv->current_name_index = active;

	/* Update the dialog to reflect the selected session */
	update_dialog(self);
    }

    g_free(name);
}

#if 0
static gboolean
on_name_combo_focus_out(GtkWidget *widget,
			GdkEventFocus *event,
			GSwatSessionManagerDialog *self)
{
    //GtkEntry *name_entry = GTK_ENTRY(widget);
    //const gchar *name = gtk_entry_get_text(name_entry);

    g_debug("COMBO focus_out\n");
    //activate_new_session_from_name(self, name);

    return FALSE;
}
#endif

#if 0
static void
on_name_combo_activated(GtkWidget *widget,
			GSwatSessionManagerDialog *self)
{
    GtkEntry *name_entry = GTK_ENTRY(widget);
    const gchar *name = gtk_entry_get_text(name_entry);

    g_debug("COMBO editing done!\n");

    activate_new_session_from_name(self, name);
}
#endif

#if 0
static void
activate_new_session_from_name(GSwatSessionManagerDialog *self,
			       const gchar *name)
{
    GSwatSession *session;

    session = lookup_session(self, name);
    g_assert(session);

    update_dialog(self, session);

    if(self->priv->current_session == self->priv->partial_session)
    {
	gswat_session_manager_add_session(self->priv->manager,
					  self->priv->partial_session);
	g_object_unref(self->priv->partial_session);
	self->priv->partial_session = gswat_session_new();
    }
}


static GSwatSession *
lookup_session(GSwatSessionManagerDialog *self,
	       const gchar *name)
{
    GSwatSession *session;

    session = gswat_session_manager_find_session(self->priv->manager, name);
    if(session)
    {
	return session;
    }

    if(strcmp(gswat_session_get_name(self->priv->partial_session), name) == 0)
    {
	return self->priv->partial_session;
    }

    return NULL;
}

#endif

#if 0
static gboolean
on_name_combo_key_press(GtkWidget *widget,
			GdkEventKey *event,
			GSwatSessionManagerDialog *self)
{
    //GtkComboBox *name_entry = GTK_ENTRY(widget);
    //gchar *name = gtk_combo_box_get_active_text(name_entry);

    g_debug("COMBO key_press\n");
#if 0
    switch(event->keyval)
    {
	case GDK_KP_Enter:
	case GDK_Return:
	    activate_new_session_from_name(self, name);
	default:
	    break;
    }
    g_free(name);
#endif
    return FALSE;
}
#endif


#if 0
static void
on_name_combo_change(GtkComboBoxEntry *name_combo,
		     GSwatSessionManagerDialog *self)
{
    GSwatSession *session;
    gchar *name;

    g_debug("COMBO change\n");
    name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(name_combo));

    /* lookup the name in the session manager */
    session = gswat_session_manager_find_session(self->priv->manager, name);
    if(session)
    {
	return;
    }
    else
    {
	/* if that fails, then update the name of the partial session */
	gswat_session_set_name(self->priv->partial_session, name);
    }

    g_free(name);
}
#endif


static void
abort_edit(GSwatSessionManagerDialog *self)
{
    gtk_widget_destroy(self->priv->dialog);

    g_object_unref(self->priv->xml);

    g_signal_emit_by_name(self, "edit-abort");
}

