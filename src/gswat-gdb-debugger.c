#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>

#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "gswat-utils.h"
#include "gswat-session.h"
#include "gswat-gdbmi.h"
#include "gswat-debuggable.h"
#include "gswat-variable-object.h"
#include "gswat-gdb-debugger.h"
#include "gswat-gdb-variable-object.h"

/* Function definitions */
static void gswat_gdb_debugger_class_init(GSwatGdbDebuggerClass *klass);
static void gswat_gdb_debugger_get_property(GObject *object,
                                   guint id,
                                   GValue* value,
                                   GParamSpec* pspec);
static void gswat_gdb_debugger_set_property(GObject *object,
					               guint        property_id,
					               const GValue *value,
					               GParamSpec   *pspec);
static void gswat_gdb_debugger_debuggable_interface_init(gpointer interface,
                                                         gpointer data);
static void gswat_gdb_debugger_init(GSwatGdbDebugger *self);
static void gswat_gdb_debugger_finalize(GObject *self);

static gboolean start_spawner_process(GSwatGdbDebugger *self);



static void spawn_local_process(GSwatGdbDebugger *self);
static gboolean parse_stdout_idle(GIOChannel* io,
                                  GIOCondition cond,
                                  gpointer *data);
static gboolean parse_stderr_idle(GIOChannel* io,
                                  GIOCondition cond,
                                  gpointer *data);
static void parse_stdout(GSwatGdbDebugger* self, GIOChannel* io);
static gboolean process_gdb_pending(gpointer data);
static void process_gdbmi_command(GSwatGdbDebugger *self, GString *command);
static void process_frame(GSwatGdbDebugger *self, GDBMIValue *val);
static void set_location(GSwatGdbDebugger *self,
                         const gchar *filename,
                         guint line,
                         const gchar *address);
static gchar *uri_from_filename(const gchar *filename);
static void on_stack_list_done(struct _GSwatGdbDebugger *self,
                               gulong token,
                               const GDBMIValue *val,
                               GString *command,
                               gpointer *data);
static void update_variable_objects(GSwatGdbDebugger *self);
static void on_update_variable_objects_done(GSwatGdbDebugger *self,
                                            gulong token,
                                            const GDBMIValue *done_val,
                                            GString *command,
                                            gpointer *data);
static void on_break_insert_done(GSwatGdbDebugger *self,
                                 gulong token,
                                 const GDBMIValue *val,
                                 GString *command,
                                 gpointer *data);

/* Macros and defines */
#define GSWAT_GDB_DEBUGGER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GSWAT_TYPE_GDB_DEBUGGER, GSwatGdbDebuggerPrivate))
#define GDBMI_DONE_CALLBACK(X)  ((GDBMIDoneCallback)(X))

/* Enums */
/* add your signals here */
#if 0
enum {
    SIGNAL_NAME,
    LAST_SIGNAL
};
#endif

enum {
    PROP_0,
    PROP_STATE,
    PROP_STACK,
    PROP_BREAKPOINTS,
    PROP_SOURCE_URI,
    PROP_SOURCE_LINE
};

struct _GSwatGdbDebuggerPrivate
{
    GSwatSession            *session;

    GSwatDebuggableState    state;
    guint                   state_stamp;

    GList                   *stack;
    GList                   *breakpoints;
    gchar                   *source_uri;
    gulong                  source_line;

    GList                   *locals;
    guint                   locals_stamp;

    /* gdb specific */

    GIOChannel      *gdb_in;
    GIOChannel      *gdb_out;
    guint           gdb_out_event;
    GIOChannel      *gdb_err;
    guint           gdb_err_event;
    GPid            gdb_pid;
    unsigned long   gdb_sequence;
    GQueue          *gdb_pending;

    int             spawn_read_fifo_fd;
    int             spawn_write_fifo_fd;
    GIOChannel      *spawn_read_fifo;
    GIOChannel      *spawn_write_fifo;
    GPid            spawner_pid;

    GPid            target_pid;

    GSList          *mi_handlers;

    /* A list of weak references to all variable
     * objects */
    GList           *all_variables;
};

/* Variables */
static GObjectClass *parent_class = NULL;
//static guint gswat_gdb_debugger_signals[LAST_SIGNAL] = { 0 };

GType
gswat_gdb_debugger_get_type(void) /* Typechecking */
{
    static GType self_type = 0;

    if (!self_type)
    {
        static const GTypeInfo object_info =
        {
            sizeof(GSwatGdbDebuggerClass), /* class structure size */
            NULL, /* base class initializer */
            NULL, /* base class finalizer */
            (GClassInitFunc)gswat_gdb_debugger_class_init, /* class initializer */
            NULL, /* class finalizer */
            NULL, /* class data */
            sizeof(GSwatGdbDebugger), /* instance structure size */
            0, /* preallocated instances */
            (GInstanceInitFunc)gswat_gdb_debugger_init, /* instance initializer */
            NULL /* function table */
        };

        /* add the type of your parent class here */
        self_type = g_type_register_static(G_TYPE_OBJECT, /* parent GType */
                                           "GSwatGdbDebugger", /* type name */
                                           &object_info, /* type info */
                                           0 /* flags */
                                          );

        /* add interfaces here */
        static const GInterfaceInfo debuggable_info =
        {
            (GInterfaceInitFunc)
                gswat_gdb_debugger_debuggable_interface_init,
            (GInterfaceFinalizeFunc)NULL,
            NULL /* interface data */
        };

        if(self_type != G_TYPE_INVALID) {
            g_type_add_interface_static(self_type,
                                        GSWAT_TYPE_DEBUGGABLE,
                                        &debuggable_info);
        }
    }

    return self_type;
}

static void
gswat_gdb_debugger_class_init(GSwatGdbDebuggerClass *klass) /* Class Initialization */
{   
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GParamSpec *new_param;

    parent_class = g_type_class_peek_parent(klass);

    gobject_class->finalize = gswat_gdb_debugger_finalize;

    gobject_class->get_property = gswat_gdb_debugger_get_property;
    gobject_class->set_property = gswat_gdb_debugger_set_property;

    /* set up properties */
#if 0
    //new_param = g_param_spec_int("name", /* name */
    //new_param = g_param_spec_uint("name", /* name */
    //new_param = g_param_spec_boolean("name", /* name */
    //new_param = g_param_spec_gdb_debugger("name", /* name */
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
                                     GSWAT_TYPE_PARAM_OBJ, /* GType */
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

//FIXME move most of these up into the debuggable interface

    new_param = g_param_spec_uint("state",
                                  _("The Running State"),
                                  _("The State of the debugger"
                                    " (whether or not it is currently running)"),
                                  0,
                                  G_MAXUINT,
                                  GSWAT_DEBUGGABLE_NOT_RUNNING,
                                  G_PARAM_READABLE);
    g_object_class_install_property(gobject_class,
                                    PROP_STATE,
                                    new_param
                                   );

    new_param = g_param_spec_pointer("stack",
                                     _("Stack"),
                                     _("The Stack of called functions"),
                                     G_PARAM_READABLE);
    g_object_class_install_property(gobject_class,
                                    PROP_STACK,
                                    new_param
                                   );

    new_param = g_param_spec_pointer("breakpoints",
                                     _("Breakpoints"),
                                     _("The list of user defined breakpoints"),
                                     G_PARAM_READABLE);
    g_object_class_install_property(gobject_class,
                                    PROP_STACK,
                                    new_param
                                   );

    new_param = g_param_spec_string("source-uri",
                                    _("Source URI"),
                                    _("The location for the source file currently being debuggged"),
                                    NULL,//"file:///home/rob/src/ggdb/src/gswat-debugger.c",
                                    G_PARAM_READABLE);
    g_object_class_install_property(gobject_class,
                                    PROP_SOURCE_URI,
                                    new_param
                                   );

    new_param = g_param_spec_ulong("source-line",
                                   _("Source Line"),
                                   _("The line of your source file currently being debuggged"),
                                   0,
                                   G_MAXULONG,
                                   0,
                                   G_PARAM_READABLE);
    g_object_class_install_property(gobject_class,
                                    PROP_SOURCE_LINE,
                                    new_param
                                   );


    /* set up signals */
#if 0 /* template code */
    klass->signal_member = signal_default_handler;
    my_object_signals[SIGNAL_NAME] =
        g_signal_new("signal_name", /* name */
                     G_TYPE_FROM_CLASS(klass), /* object GType */
                     G_SIGNAL_RUN_LAST, /* signal flags */
                     G_STRUCT_OFFSET(MyObjectClass, signal_member),
                     NULL, /* accumulator */
                     NULL, /* accumulator data */
                     g_cclosure_marshal_VOID__VOID, /* c marshaller */
                     G_TYPE_NONE, /* return type */
                     0, /* number of parameters */
                     /* vararg, list of param types */
                    );
#endif
    //gswat_gdb_debugger_signals[] = g_signal_lookup("", INTERFACE_TYPE);


    g_type_class_add_private(klass, sizeof(GSwatGdbDebuggerPrivate));
}

static void
gswat_gdb_debugger_get_property(GObject *object,
                                guint id,
                                GValue *value,
                                GParamSpec *pspec)
{
    GSwatGdbDebugger* self = GSWAT_GDB_DEBUGGER(object);
    GSwatDebuggable* self_debuggable = GSWAT_DEBUGGABLE(object);
    char *tmp;

    switch(id) {
#if 0 /* template code */
        case PROP_NAME:
            g_value_set_int(value, self->priv->property);
            g_value_set_uint(value, self->priv->property);
            g_value_set_boolean(value, self->priv->property);
            /* don't forget that this will dup the string... */
            g_value_set_string(value, self->priv->property);
            g_value_set_gdb_debugger(value, self->priv->property);
            g_value_set_pointer(value, self->priv->property);
            break;
#endif
        case PROP_STATE:
            g_value_set_uint(value, self->priv->state);
            break;
        case PROP_STACK:
            g_value_set_pointer(value,
                                gswat_gdb_debugger_get_stack(self_debuggable)
                               );
            break;
        case PROP_BREAKPOINTS:
            g_value_set_pointer(value,
                                gswat_gdb_debugger_get_breakpoints(self_debuggable)
                               );
            break;
        case PROP_SOURCE_URI:
            tmp = gswat_gdb_debugger_get_source_uri(self_debuggable);
            g_value_set_string(value, self->priv->source_uri);
            g_free(tmp);
            break;
        case PROP_SOURCE_LINE:
            g_value_set_ulong(value,
                              gswat_gdb_debugger_get_source_line(self_debuggable)
                             );
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
            break;
    }
}

static void
gswat_gdb_debugger_set_property(GObject *object,
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
            self->priv->property = g_value_dup_gdb_debugger(value);
            /* g_free(self->priv->property)? */
            self->priv->property = g_value_get_pointer(value);
            break;
#endif
        default:
            g_warning("gswat_gdb_debugger_set_property on unknown property");
            return;
    }
}

/* Initialize interfaces here */

void
gswat_gdb_debugger_debuggable_interface_init(gpointer interface,
                                             gpointer data)
{
    GSwatDebuggableIface *debuggable = interface;

    g_assert((G_TYPE_FROM_INTERFACE(debuggable) = GSWAT_TYPE_DEBUGGABLE));

    debuggable->target_connect = gswat_gdb_debugger_target_connect;
    debuggable->request_line_breakpoint 
        = gswat_gdb_debugger_request_line_breakpoint;
    debuggable->request_function_breakpoint 
        = gswat_gdb_debugger_request_function_breakpoint;
    debuggable->get_source_uri = gswat_gdb_debugger_get_source_uri;
    debuggable->get_source_line = gswat_gdb_debugger_get_source_line;
    debuggable->cont = gswat_gdb_debugger_continue;
    debuggable->finish = gswat_gdb_debugger_finish;
    debuggable->next = gswat_gdb_debugger_next;
    debuggable->step_into = gswat_gdb_debugger_step_into;
    debuggable->interrupt = gswat_gdb_debugger_interrupt;
    debuggable->restart = gswat_gdb_debugger_restart;
    debuggable->get_state = gswat_gdb_debugger_get_state;
    debuggable->get_state_stamp = gswat_gdb_debugger_get_state_stamp;
    debuggable->get_stack = gswat_gdb_debugger_get_stack;
    debuggable->get_breakpoints = gswat_gdb_debugger_get_breakpoints;
    debuggable->get_locals_list = gswat_gdb_debugger_get_locals_list;

}

/* Instance Construction */
static void
gswat_gdb_debugger_init(GSwatGdbDebugger *self)
{
    self->priv = GSWAT_GDB_DEBUGGER_GET_PRIVATE(self);

    self->priv->gdb_pending = g_queue_new();

    if(!start_spawner_process(self))
    {
        g_critical("Failed to start spawner process");
    }
}

/* Instantiation wrapper */
GSwatGdbDebugger*
gswat_gdb_debugger_new(GSwatSession *session)
{
    GSwatGdbDebugger *debugger;

    g_return_val_if_fail(session != NULL, NULL);

    debugger = g_object_new(GSWAT_TYPE_GDB_DEBUGGER, NULL);

    g_object_ref(session);
    debugger->priv->session = session;

    return debugger;
}


/* Instance Destruction */
void
gswat_gdb_debugger_finalize(GObject *object)
{
    GSwatGdbDebugger *self = GSWAT_GDB_DEBUGGER(object);

    gswat_gdb_debugger_target_disconnect(GSWAT_DEBUGGABLE(self));

    /* FIXME - free arguments */
    if(self->priv->stack) {
        g_list_foreach(self->priv->stack, (GFunc)g_free, NULL);
        g_list_free(self->priv->stack);
        self->priv->stack = NULL;
    }

    g_object_unref(self->priv->session);
    self->priv->session=NULL;

    g_queue_free(self->priv->gdb_pending);

    /* FIXME - free all breakpoints */

    /* FIXME - Unref all variable objects */

    /* FIXME - This is where we should stop the spawner process */

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
start_spawner_process(GSwatGdbDebugger *self)
{
    gchar *fifo0_name;
    gchar *fifo1_name;
    int read_fifo_fd;
    int write_fifo_fd;
    GIOChannel *read_fifo;
    GIOChannel *write_fifo;
    gchar *terminal_command;
    gint argc;
    gchar **argv;
    gboolean retval;
    GError  *error = NULL;
    GString *fifo_data;

    fifo0_name=g_strdup_printf("/tmp/gswat%d0", getpid());
    fifo1_name=g_strdup_printf("/tmp/gswat%d1", getpid());

    /* create named fifos */
    mkfifo(fifo0_name, S_IRUSR|S_IWUSR);
    mkfifo(fifo1_name, S_IRUSR|S_IWUSR);


    terminal_command = g_strdup_printf("gnome-terminal -x gswat-spawner --read-fifo %s --write-fifo %s",
                                       fifo1_name,
                                       fifo0_name);

    g_shell_parse_argv(terminal_command, &argc, &argv, NULL);


    /* run gnome-terminal -x argv[0] -magic /path/to/fifo */
    retval = g_spawn_async(NULL,
                           argv,
                           NULL,
                           G_SPAWN_SEARCH_PATH,
                           NULL,
                           NULL,
                           NULL,
                           &error);
    g_strfreev(argv);


    if(error) {
        g_warning("%s: %s", _("Could not start terminal"), error->message);
        g_error_free(error);
        error = NULL;

        g_free(fifo0_name);
        g_free(fifo1_name);
        return FALSE;

    } else if(!retval) {
        g_warning("%s", _("Could not start terminal"));

        g_free(fifo0_name);
        g_free(fifo1_name);
        return FALSE;
    }

    read_fifo_fd = open(fifo0_name, O_RDONLY);
    write_fifo_fd = open(fifo1_name, O_WRONLY);

    g_free(fifo0_name);
    g_free(fifo1_name);

    /* attach g_io_channels to the fifos */
    read_fifo = g_io_channel_unix_new(read_fifo_fd);
    write_fifo = g_io_channel_unix_new(write_fifo_fd);


    self->priv->spawn_read_fifo_fd = read_fifo_fd;
    self->priv->spawn_write_fifo_fd = write_fifo_fd;

    self->priv->spawn_read_fifo = read_fifo;
    self->priv->spawn_write_fifo = write_fifo;


    fifo_data = g_string_new("");

    g_io_channel_read_line_string(self->priv->spawn_read_fifo,
                                  fifo_data,
                                  NULL,
                                  NULL);

    self->priv->spawner_pid = strtoul(fifo_data->str, NULL, 10);


    return TRUE;
}


static void
stop_spawner_process(GSwatGdbDebugger *self)
{
    kill(self->priv->spawner_pid, SIGKILL);

    g_io_channel_shutdown(self->priv->spawn_read_fifo, FALSE, NULL);
    g_io_channel_shutdown(self->priv->spawn_write_fifo, FALSE, NULL);
    g_io_channel_unref(self->priv->spawn_read_fifo);
    g_io_channel_unref(self->priv->spawn_write_fifo);

    close(self->priv->spawn_read_fifo_fd);
    close(self->priv->spawn_write_fifo_fd);
}

/* Connect to the target as appropriate for the session */
void
gswat_gdb_debugger_target_connect(GSwatDebuggable* object)
{
    GSwatGdbDebugger *self;
    GError  * error = NULL;
    gchar *gdb_argv[] = {
        "gdb", "--interpreter=mi", NULL
    };
    gint fd_in, fd_out, fd_err;
    const gchar *charset;
    gboolean  retval;
    const gchar *user_command;
    gint user_argc;
    gchar **user_argv;

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);

    /* Currently we assume a local gdb session */

    retval = g_spawn_async_with_pipes(NULL, gdb_argv, NULL,
                                      G_SPAWN_SEARCH_PATH,
                                      NULL, NULL,
                                      &self->priv->gdb_pid, &fd_in,
                                      &fd_out, &fd_err,
                                      &error);

    if(error) {
        g_warning("%s: %s", _("Could not start debugger"), error->message);
        g_error_free(error);
        error = NULL;
    } else if(!retval) {
        g_warning("%s", _("Could not start debugger"));
    } else {
        // everything went fine
        self->priv->gdb_in  = g_io_channel_unix_new(fd_in);
        self->priv->gdb_out = g_io_channel_unix_new(fd_out);
        self->priv->gdb_err = g_io_channel_unix_new(fd_err);

        /* what is the current locale charset? */
        g_get_charset(&charset);

        g_io_channel_set_encoding(self->priv->gdb_in, charset, NULL);
        g_io_channel_set_encoding(self->priv->gdb_out, charset, NULL);
        g_io_channel_set_encoding(self->priv->gdb_err, charset, NULL);

        //g_io_channel_set_flags(self->priv->gdb_in, G_IO_FLAG_NONBLOCK, NULL);
        //g_io_channel_set_flags(self->priv->gdb_out, G_IO_FLAG_NONBLOCK, NULL);
        //g_io_channel_set_flags(self->priv->gdb_err, G_IO_FLAG_NONBLOCK, NULL);

        self->priv->gdb_out_event = 
            g_io_add_watch(self->priv->gdb_out,
                           G_IO_IN,
                           (GIOFunc)parse_stdout_idle,
                           self);
        self->priv->gdb_err_event =
            g_io_add_watch(self->priv->gdb_err,
                           G_IO_IN,
                           (GIOFunc)parse_stderr_idle,
                           self);
    }

    user_command = gswat_session_get_command(GSWAT_SESSION(self->priv->session),
                                             &user_argc,
                                             &user_argv);

    /* assume this session is for debugging a local file */
    spawn_local_process(self);

}


void
gswat_gdb_debugger_target_disconnect(GSwatDebuggable* object)
{
    GSwatGdbDebugger *self;

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);

    /* FIXME - this shouldn't be stopped here.
     * A spawner process should be reused. */
    stop_spawner_process(self);

    g_source_remove(self->priv->gdb_out_event);
    g_source_remove(self->priv->gdb_err_event);


    if(self->priv->gdb_out) {
        g_io_channel_unref(self->priv->gdb_out);
        self->priv->gdb_out = NULL;
    }

    if(self->priv->gdb_err) {
        g_io_channel_unref(self->priv->gdb_err);
        self->priv->gdb_err = NULL;
    }

    if(self->priv->gdb_in) {
        gswat_gdb_debugger_send_mi_command(self, "-gdb-exit\n");
        //parse_stdout_idle(self->priv->gdb_out, G_IO_OUT, self);
        g_io_channel_unref(self->priv->gdb_in);
        self->priv->gdb_in = NULL;
    }

    //kill(self->priv->gdb_pid, SIGTERM);
}


static void
spawn_local_process(GSwatGdbDebugger *self)
{
    gchar **argv;
    GString *fifo_data;

    gsize written;
    gchar *cwd;
    gchar *command;
    gchar *gdb_command;



    fifo_data = g_string_new("");

    /* wait for ACK */
    g_io_channel_read_line_string(self->priv->spawn_read_fifo,
                                  fifo_data,
                                  NULL,
                                  NULL);

    if(strcmp(fifo_data->str, "ACK\n") != 0)
    {
        g_critical("Didn't get an expected \"ACK\" from target fifo\n");
    }


    command = gswat_session_get_command(self->priv->session, NULL, &argv);
    g_string_printf(fifo_data, "%s\n", command);
    g_free(command);

    /* Send the command to run */
    g_io_channel_write_chars(self->priv->spawn_write_fifo,
                             fifo_data->str,
                             fifo_data->len,
                             &written,
                             NULL);
    g_io_channel_flush(self->priv->spawn_write_fifo, NULL);

    /* Send working directory */
    cwd = gswat_session_get_working_dir(self->priv->session);
    g_string_printf(fifo_data, "%s\n", cwd);
    g_free(cwd);
    g_io_channel_write_chars(self->priv->spawn_write_fifo,
                             fifo_data->str,
                             fifo_data->len,
                             &written,
                             NULL);
    g_io_channel_flush(self->priv->spawn_write_fifo, NULL);

    /* Send the session environment */
    /* TODO */

    /* Terminate environment data */
    g_io_channel_write_chars(self->priv->spawn_write_fifo,
                             "*** END ENV ***\n",
                             strlen("*** END ENV ***\n"),
                             &written,
                             NULL);
    g_io_channel_flush(self->priv->spawn_write_fifo, NULL);


    /* The command we sent should now get started in the
     * terminal using ptrace so SIGSTOP can be sent
     * before main() is reached.
     * a ptrace detach is then done and we get sent
     * the PID of the new process
     *
     * Note: The reason for all this hassle is
     * so we can have our debug app run in a nice
     * terminal (so you can e.g. debug ncurses apps)
     *
     * If there is a neater way, please tell me.
     */


    g_io_channel_read_line_string(self->priv->spawn_read_fifo,
                                  fifo_data,
                                  NULL,
                                  NULL);
    self->priv->target_pid = strtoul(fifo_data->str, NULL, 10);
    g_message("process PID=%d",self->priv->target_pid);

    g_string_free(fifo_data, TRUE);


    /* load the symbols into gdb and attach to the suspended
     * process
     */

    gdb_command=g_strdup_printf("-file-exec-and-symbols %s", argv[0]);
    gswat_gdb_debugger_send_mi_command(self, gdb_command);
    g_free(gdb_command);
    g_strfreev(argv);


    /* AARRRgh GDB/MI is a pain in the arse! */
    //gdb_command=g_strdup_printf("-target-attach %d", self->priv->target_pid);
    gdb_command=g_strdup_printf("attach %d", self->priv->target_pid);
    g_message("SENDING = \"%s\"", gdb_command);
    //gswat_gdb_debugger_send_mi_command(self, gdb_command);
    gswat_gdb_debugger_send_cli_command(self, gdb_command);
    g_free(gdb_command);


    gswat_gdb_debugger_request_function_breakpoint(GSWAT_DEBUGGABLE(self),
                                                   "main");


    /* easier than sending a SIGCONT... */
    //gswat_gdb_debugger_run(self);
    gswat_gdb_debugger_send_cli_command(self, "signal SIGCONT");
    gswat_gdb_debugger_send_mi_command(self, "-exec-continue");
    gswat_gdb_debugger_send_mi_command(self, "-exec-continue");


}


static gboolean
parse_stdout_idle(GIOChannel* io, GIOCondition cond, gpointer *data)
{
    GSwatGdbDebugger *self = GSWAT_GDB_DEBUGGER(data);

    parse_stdout(self, io);

    return TRUE;
}


static gboolean
parse_stderr_idle(GIOChannel* io, GIOCondition cond, gpointer *data)
{
    //GSwatGdbDebugger *self = GSWAT_GDB_DEBUGGER(data);
    GIOStatus status;
    GString *line;

    line = g_string_new("");

    status = g_io_channel_read_line_string(io,
                                           line,
                                           NULL,
                                           NULL);

    g_warning("gdb error: %s\n", line->str);

    g_string_free(line, TRUE);

    return TRUE;
}


static void
parse_stdout(GSwatGdbDebugger* self, GIOChannel* io)
{
    /* read new gdb output up to the next (gdb) prompt */
    while(TRUE)
    {
        GString *gdb_string = g_string_new("");
        GError *error = NULL;

        g_io_channel_read_line_string(io, gdb_string, NULL, &error);
        if(error)
        {
            g_error_free(error);
            g_assert_not_reached();
        }

        if(gdb_string->len >= 7 
           && (strncmp(gdb_string->str, "(gdb) \n", 7) == 0))
        {
            g_string_free(gdb_string, TRUE);
            break;
        }

        /* remove the trailing newline */
        gdb_string = g_string_truncate(gdb_string, gdb_string->len-1);

        g_message("adding to pending - \"%s\"", gdb_string->str);
        g_queue_push_tail(self->priv->gdb_pending, gdb_string);
    }

    process_gdb_pending(self);

    return;
}


static gboolean
process_gdb_pending(gpointer data)
{
    GSwatGdbDebugger *self = GSWAT_GDB_DEBUGGER(data);
    GString *current_line;

    while((current_line = g_queue_pop_head(self->priv->gdb_pending)))
    {
        process_gdbmi_command(self, current_line);
        g_string_free(current_line, TRUE);
    }

    return FALSE;
}


static void
process_gdbmi_command(GSwatGdbDebugger *self, GString *command)
{
    GString *tmp = NULL;
    gulong command_token;
    gchar *remainder;
    //guint token;
    //guint lineno;
    //gchar *filename;

    //gchar *line = command->str;
    //const size_t len = command->len;

    if (command->len == 0)
    {
        return;
    }


    /* read any gdbmi "token" */
    command_token = strtol(command->str, &remainder, 10);
    if(!(command_token == 0 && command->str == remainder))
    {
        tmp = g_string_new(remainder);
        command = tmp;
    }

#if 0
    token_str = g_string_new("");
    for(i=0; i<command->len; i++)
    {
        if(isdigit(command->str[i])){
            g_string_append_c(token_str, command->str[i]);
        }else{
            break;
        }
    }
    g_assert(i != (command->len-1));
    command_token = strtol(token_str->str, NULL, 10);

    g_string_erase(command, 0, i);
#endif


    g_message("gdbmi_command: token=%ld, command=%s", command_token, command->str);

    if (command->str[0] == '\032' && command->str[1] == '\032')
    {
        g_assert_not_reached();
#if 0
        gdb_util_parse_error_line (&(line[2]), &filename, &lineno);
        if (filename)
        {
            debugger_change_location (debugger, filename, lineno, NULL);
            g_free (filename);
        }
#endif
    }
    else if (strncasecmp (command->str, "^error", 6) == 0)
    {
        g_warning(command->str);

#if 0
        /* GDB reported error */
        if (debugger->priv->current_cmd.suppress_error == FALSE)
        {
            GDBMIValue *val = gdbmi_value_parse (line);
            if (val)
            {
                const GDBMIValue *message;
                message = gdbmi_value_hash_lookup (val, "msg");

                anjuta_util_dialog_error (debugger->priv->parent_win,
                                          "%s",
                                          gdbmi_value_literal_get (message));
                gdbmi_value_free (val);
            }
        }
#endif
    }
    else if (strncasecmp(command->str, "^running", 8) == 0)
    {
        self->priv->state = GSWAT_DEBUGGABLE_RUNNING;
        self->priv->state_stamp++;
        g_object_notify(G_OBJECT(self), "state");
#if 0
        /* Program started running */
        debugger->priv->prog_is_running = TRUE;
        debugger->priv->debugger_is_ready = TRUE;
        debugger->priv->skip_next_prompt = TRUE;
        g_signal_emit_by_name (debugger, "program-running");
#endif
    }
    else if(strncasecmp(command->str, "*stopped", 8) == 0)
    {
        /* FIXME- move this into a seperate function */
        GDBMIValue *val = gdbmi_value_parse(command->str);
        if(val)
        {
            const GDBMIValue *reason;
            const gchar *str = NULL;

            gdbmi_value_dump(val, 0);

            process_frame(self, val);
            update_variable_objects(self);

            reason = gdbmi_value_hash_lookup(val, "reason");
            if(reason){
                str = gdbmi_value_literal_get(reason);
            }

            if (str && strcmp (str, "exited-normally") == 0)
            {
                self->priv->state = GSWAT_DEBUGGABLE_NOT_RUNNING;
            }
            else if (str && strcmp (str, "exited") == 0)
            {
                /* TODO exited with an error code */
                g_assert_not_reached();
            }
            else if (str && strcmp (str, "exited-signalled") == 0)
            {
                /* TODO */
                g_assert_not_reached();
            }
            else if (str && strcmp (str, "signal-received") == 0)
            {
                self->priv->state = GSWAT_DEBUGGABLE_INTERRUPTED;
            }
            else if (str && strcmp (str, "breakpoint-hit") == 0)
            {
                self->priv->state = GSWAT_DEBUGGABLE_INTERRUPTED;
            }
            else if (str && strcmp (str, "function-finished") == 0)
            {
                self->priv->state = GSWAT_DEBUGGABLE_INTERRUPTED;
            }
            else if (str && strcmp (str, "end-stepping-range") == 0)
            {
                self->priv->state = GSWAT_DEBUGGABLE_INTERRUPTED;
            }
            else if (str && strcmp (str, "location-reached") == 0)
            {
                self->priv->state = GSWAT_DEBUGGABLE_INTERRUPTED;
            }

            self->priv->state_stamp++;
            g_object_notify(G_OBJECT(self), "state");

            gdbmi_value_free(val);
        }
#if 0
        /* Process has stopped */
        gboolean program_exited = FALSE;

        GDBMIValue *val = gdbmi_value_parse (line);

        debugger->priv->debugger_is_ready = TRUE;

        /* Check if program has exited */
        if (val)
        {
            const GDBMIValue *reason;
            const gchar *str = NULL;

            debugger_process_frame (debugger, val);

            reason = gdbmi_value_hash_lookup (val, "reason");
            if (reason)
                str = gdbmi_value_literal_get (reason);

            if (str && strcmp (str, "exited-normally") == 0)
            {
                program_exited = TRUE;
                debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
                                                 _("Program exited normally\n"),
                                                 debugger->priv->output_user_data);
            }
            else if (str && strcmp (str, "exited") == 0)
            {
                const GDBMIValue *errcode;
                const gchar *errcode_str;
                gchar *msg;

                program_exited = TRUE;
                errcode = gdbmi_value_hash_lookup (val, "exit-code");
                errcode_str = gdbmi_value_literal_get (errcode);
                msg = g_strdup_printf (_("Program exited with error code %s\n"),
                                       errcode_str);
                debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
                                                 msg, debugger->priv->output_user_data);
                g_free (msg);
            }
            else if (str && strcmp (str, "exited-signalled") == 0)
            {
                const GDBMIValue *signal_name, *signal_meaning;
                const gchar *signal_str, *signal_meaning_str;
                gchar *msg;

                program_exited = TRUE;
                signal_name = gdbmi_value_hash_lookup (val, "signal-name");
                signal_str = gdbmi_value_literal_get (signal_name);
                signal_meaning = gdbmi_value_hash_lookup (val,
                                                          "signal-meaning");
                signal_meaning_str = gdbmi_value_literal_get (signal_meaning);
                msg = g_strdup_printf (_("Program received signal %s (%s) and exited\n"),
                                       signal_str, signal_meaning_str);
                debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
                                                 msg, debugger->priv->output_user_data);
                g_free (msg);
            }
            else if (str && strcmp (str, "signal-received") == 0)
            {
                const GDBMIValue *signal_name, *signal_meaning;
                const gchar *signal_str, *signal_meaning_str;
                gchar *msg;

                signal_name = gdbmi_value_hash_lookup (val, "signal-name");
                signal_str = gdbmi_value_literal_get (signal_name);
                signal_meaning = gdbmi_value_hash_lookup (val,
                                                          "signal-meaning");
                signal_meaning_str = gdbmi_value_literal_get (signal_meaning);

                msg = g_strdup_printf (_("Program received signal %s (%s)\n"),
                                       signal_str, signal_meaning_str);
                debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
                                                 msg, debugger->priv->output_user_data);
                g_free (msg);
            }
            else if (str && strcmp (str, "breakpoint-hit") == 0)
            {
                const GDBMIValue *bkptno;
                const gchar *bkptno_str;
                gchar *msg;

                bkptno = gdbmi_value_hash_lookup (val, "bkptno");
                bkptno_str = gdbmi_value_literal_get (bkptno);

                msg = g_strdup_printf (_("Breakpoint number %s hit\n"),
                                       bkptno_str);
                debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
                                                 msg, debugger->priv->output_user_data);
                g_free (msg);
            }
            else if (str && strcmp (str, "function-finished") == 0)
            {
                debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
                                                 _("Function finished\n"),
                                                 debugger->priv->output_user_data);
            }
            else if (str && strcmp (str, "end-stepping-range") == 0)
            {
                debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
                                                 _("Stepping finished\n"),
                                                 debugger->priv->output_user_data);
            }
            else if (str && strcmp (str, "location-reached") == 0)
            {
                debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
                                                 _("Location reached\n"),
                                                 debugger->priv->output_user_data);
            }
        }

        if (program_exited)
        {
            debugger->priv->prog_is_running = FALSE;
            debugger->priv->prog_is_attached = FALSE;
            debugger->priv->debugger_is_ready = TRUE;
            debugger_stop_terminal (debugger);
            g_signal_emit_by_name (debugger, "program-exited", val);
            debugger_handle_post_execution (debugger);
        }
        else
        {
            debugger->priv->debugger_is_ready = TRUE;
            g_signal_emit_by_name (debugger, "program-stopped", val);
        }

        debugger->priv->stdo_lines = g_list_reverse (debugger->priv->stdo_lines);
        if (debugger->priv->current_cmd.cmd[0] != '\0' &&
            debugger->priv->current_cmd.parser != NULL)
        {
            debugger->priv->debugger_is_ready = TRUE;
            debugger->priv->current_cmd.parser (debugger, val,
                                                debugger->priv->stdo_lines,
                                                debugger->priv->current_cmd.user_data);
            debugger->priv->command_output_sent = TRUE;
            DEBUG_PRINT ("In function: Sending output...");
        }

        if (val)
            gdbmi_value_free (val);
#endif
    }
    else if (strncasecmp (command->str, "^done", 5) == 0)
    {
        /* GDB command has reported output */
        GDBMIValue *val = gdbmi_value_parse(command->str);
        if (val)
        {
            GSList *tmp;
            GDBMIDoneHandler *current_handler;

            gdbmi_value_dump(val, 0);

            for(tmp=self->priv->mi_handlers; tmp!=NULL; tmp=tmp->next)
            {
                current_handler = (GDBMIDoneHandler *)tmp->data;

                if(current_handler->token == command_token){
                    current_handler->callback(self,
                                              command_token,
                                              val,
                                              command,
                                              current_handler->data);

                    self->priv->mi_handlers = 
                        g_slist_remove(self->priv->mi_handlers,
                                       current_handler);
                    g_free(current_handler);

                    break;                 
                }

            }

            gdbmi_value_free(val);
        }
#if 0

        debugger->priv->debugger_is_ready = TRUE;

        debugger->priv->stdo_lines = g_list_reverse (debugger->priv->stdo_lines);
        if (debugger->priv->current_cmd.cmd[0] != '\0' &&
            debugger->priv->current_cmd.parser != NULL)
        {
            debugger->priv->current_cmd.parser (debugger, val,
                                                debugger->priv->stdo_lines,
                                                debugger->priv->current_cmd.user_data);
            debugger->priv->command_output_sent = TRUE;
            DEBUG_PRINT ("In function: Sending output...");
        }
        else /* if (val) */
        {
            g_signal_emit_by_name (debugger, "results-arrived",
                                   debugger->priv->current_cmd.cmd, val);
        }

        if (val)
        {
            debugger_process_frame (debugger, val);
            gdbmi_value_free (val);
        }
#endif
    }
#if 0
    else if (strncasecmp (line, GDB_PROMPT, strlen (GDB_PROMPT)) == 0)
    {
        if (debugger->priv->skip_next_prompt)
        {
            debugger->priv->skip_next_prompt = FALSE;
        }
        else
        {
            debugger->priv->debugger_is_ready = TRUE;
            if (debugger->priv->starting)
            {
                debugger->priv->starting = FALSE;
                /* Debugger has just started */
                debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
                                                 _("Debugger is ready.\n"),
                                                 debugger->priv->output_user_data);
            }

            if (strcmp (debugger->priv->current_cmd.cmd, "-exec-run") == 0 &&
                debugger->priv->prog_is_running == FALSE)
            {
                /* Program has failed to run */
                debugger_stop_terminal (debugger);
            }
            debugger->priv->debugger_is_ready = TRUE;

            /* If the parser has not yet been called, call it now. */
            if (debugger->priv->command_output_sent == FALSE &&
                debugger->priv->current_cmd.parser)
            {
                debugger->priv->current_cmd.parser (debugger, NULL,
                                                    debugger->priv->stdo_lines,
                                                    debugger->priv->current_cmd.user_data);
                debugger->priv->command_output_sent = TRUE;
            }

            debugger_queue_execute_command (debugger);	/* Next command. Go. */
            return;
        }
    }
#endif

#if 0
    else
    {
        /* FIXME: change message type */
        gchar *proper_msg, *full_msg;

        if (line[1] == '\"' && line [len - 1] == '\"')
        {
            line[len - 1] = '\0';
            proper_msg = g_strcompress (line + 2);
        }
        else
        {
            proper_msg = g_strdup (line);
        }
        if (proper_msg[strlen(proper_msg)-1] != '\n')
        {
            full_msg = g_strconcat (proper_msg, "\n", NULL);
        }
        else
        {
            full_msg = g_strdup (proper_msg);
        }

        if (debugger->priv->current_cmd.parser)
        {
            if (line[0] == '~')
            {
                /* Save GDB CLI output */
                /* Remove trailing newline */
                full_msg[strlen (full_msg) - 1] = '\0';
                debugger->priv->stdo_lines = g_list_prepend (debugger->priv->stdo_lines,
                                                             g_strdup (full_msg));
            }
        }
        else if (line[0] != '&')
        {
            /* Print the CLI output if there is no parser to receive
             * the output and it is not command echo
             */
            debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
                                             full_msg, debugger->priv->output_user_data);
        }
        g_free (proper_msg);
        g_free (full_msg);
    }


    /* Clear the line buffer */
    g_string_assign (debugger->priv->stdo_line, "");
#endif

}


static void
process_frame(GSwatGdbDebugger *self, GDBMIValue *val)
{
    const GDBMIValue *file, *line, *frame, *addr;
    const gchar *file_str, *line_str, *addr_str;

    if(!val)
        return;

    file_str = line_str = addr_str = NULL;

    g_return_if_fail (val != NULL);

    file = gdbmi_value_hash_lookup(val, "fullname");
    if(!file)
    {
        file = gdbmi_value_hash_lookup(val, "file");
    }
    line = gdbmi_value_hash_lookup(val, "line");
    frame = gdbmi_value_hash_lookup(val, "frame");
    addr = gdbmi_value_hash_lookup(val, "addr");

    if(file && line)
    {
        file_str = gdbmi_value_literal_get(file);
        line_str = gdbmi_value_literal_get(line);

        if (addr)
            addr_str = gdbmi_value_literal_get(addr);

        if (file_str && line_str)
        {
            set_location(self, file_str,
                         (guint)atoi(line_str), addr_str);
        }
    }
    else if(frame)
    {
        file = gdbmi_value_hash_lookup(frame, "fullname");
        if(!file)
        {
            file = gdbmi_value_hash_lookup(frame, "file");
        }
        line = gdbmi_value_hash_lookup(frame, "line");

        if (addr)
            addr_str = gdbmi_value_literal_get(addr);

        if (file && line)
        {
            file_str = gdbmi_value_literal_get(file);
            line_str = gdbmi_value_literal_get(line);
            if (file_str && line_str)
            {
                set_location(self, file_str,
                             (guint)atoi(line_str), addr_str);
            }
        }
    } 
}


static void
set_location(GSwatGdbDebugger *self,
             const gchar *filename,
             guint line,
             const gchar *address)
{
    gboolean new_uri=FALSE;
    gboolean new_line=FALSE;
    gchar *tmp_uri;

    tmp_uri = uri_from_filename(filename);
    if(tmp_uri == NULL)
    {
        g_critical("Could not create uri for file=\"%s\"", filename);
        return;
    }

    if(self->priv->source_uri == NULL ||
       strcmp(tmp_uri, self->priv->source_uri)!=0)
    {
        new_uri = TRUE;
        if(self->priv->source_uri != NULL)
        {
            g_free(self->priv->source_uri);
        }
        self->priv->source_uri = tmp_uri;
    }
    else
    {
        g_free(tmp_uri);
    }

    if(self->priv->source_line != line){
        new_line=TRUE;
        self->priv->source_line = line;
    }

    if(new_uri)
    {
        g_object_notify(G_OBJECT(self), "source-uri");
    }
    else if(new_line)
    {
        g_object_notify(G_OBJECT(self), "source-line");
    }

    if(new_uri || new_line){
        gulong token;
        token = gswat_gdb_debugger_send_mi_command(self, "-stack-list-frames");
        gswat_gdb_debugger_mi_done_connect(self,
                                           token,
                                           GDBMI_DONE_CALLBACK(on_stack_list_done)
                                           ,
                                           NULL
                                          );
    }
}


static gchar *
uri_from_filename(const gchar *filename)
{
    GString *local_path;
    gchar *cwd, *uri;

    if(filename == NULL)
    {
        return NULL;
    }

    local_path = g_string_new("");


    if(filename[0] != '/')
    {
        cwd = g_get_current_dir();
        local_path = g_string_append(local_path, cwd);
        g_free(cwd);
        local_path = g_string_append(local_path, "/");
    }
    /* FIXME - if we arn't given an absolute
     * path, then we need to iterate though
     * gdb's $dir to figure out the correct
     * path
     */
    local_path = g_string_append(local_path, filename);


    uri = gnome_vfs_get_uri_from_local_path(local_path->str);

    g_string_free(local_path, TRUE);

    return uri;
}


void
gswat_gdb_debugger_mi_done_connect(GSwatGdbDebugger *self,
                                   gulong token,
                                   GDBMIDoneCallback callback,
                                   gpointer *data)
{
    GDBMIDoneHandler *handler;

    handler = g_new0(GDBMIDoneHandler, 1);
    handler->token = token;
    handler->callback = callback;
    handler->data = data;

    self->priv->mi_handlers = g_slist_prepend(self->priv->mi_handlers, handler);
}


static void
on_stack_list_done(struct _GSwatGdbDebugger *self,
                   gulong token,
                   const GDBMIValue *val,
                   GString *command,
                   gpointer *data)
{
    GList *tmp, *tmp2;
    GSwatDebuggableFrame *frame=NULL, *current_frame=NULL;
    const GDBMIValue *stack_val, *frame_val;
    const GDBMIValue *stackargs_val, *args_val, *arg_val, *literal_val;
    GDBMIValue *top_val;
    GString *gdbmi_command=g_string_new("");
    GSwatDebuggableFrameArgument *arg;
    gulong tok;
    int n;

    gdbmi_value_dump(val, 0);

    /* Free the current stack */
    for(tmp=self->priv->stack; tmp!=NULL; tmp=tmp->next)
    {
        current_frame = (GSwatDebuggableFrame *)tmp->data;
        g_free(current_frame->function);
        for(tmp2 = current_frame->arguments; tmp2!=NULL; tmp2=tmp2->next)
        {
            arg = (GSwatDebuggableFrameArgument *)tmp2->data;
            g_free(arg->name);
            g_free(arg->value);
            g_free(arg);
        }
        g_list_free(current_frame->arguments);
        g_free(current_frame);
    }
    g_list_free(self->priv->stack);
    self->priv->stack = NULL;


    stack_val = gdbmi_value_hash_lookup(val, "stack");

    n=0;
    while(1){
        g_message("gdbmi_value_list_get_nth 0 - %d", n);
        frame_val = gdbmi_value_list_get_nth(stack_val, n);
        if(frame_val)
        {
            const gchar *literal;

            frame = g_new0(GSwatDebuggableFrame,1);
            frame->level = n;

            literal_val = gdbmi_value_hash_lookup(frame_val, "addr");
            literal = gdbmi_value_literal_get(literal_val);
            frame->address = strtol(literal, NULL, 10);

            literal_val = gdbmi_value_hash_lookup(frame_val, "func");
            literal = gdbmi_value_literal_get(literal_val);
            frame->function = g_strdup(literal);

            self->priv->stack = g_list_prepend(self->priv->stack, frame);

        }else{
            break;
        }

        n++;
    }


    g_string_printf(gdbmi_command, "-stack-list-arguments 1 0 %d", n);
    tok = gswat_gdb_debugger_send_mi_command(self, gdbmi_command->str);
    g_string_free(gdbmi_command, TRUE);


    top_val = gswat_gdb_debugger_get_mi_value(self, tok, NULL);
    if(!top_val && n>0)
    {
        g_object_notify(G_OBJECT(self), "stack");
        return;
        //g_assert_not_reached();
    }

    gdbmi_value_dump(top_val, 0);

    stackargs_val = gdbmi_value_hash_lookup(top_val, "stack-args");

    n--;
    for(tmp=self->priv->stack; tmp!=NULL; tmp=tmp->next)
    {
        guint a;
        current_frame = (GSwatDebuggableFrame *)tmp->data;

        g_message("gdbmi_value_list_get_nth 1 - %d", n);
        frame_val = gdbmi_value_list_get_nth(stackargs_val, n);
        args_val = gdbmi_value_hash_lookup(frame_val, "args");

        a=0;
        while(TRUE)
        {
            g_message("gdbmi_value_list_get_nth 2");
            arg_val = gdbmi_value_list_get_nth(args_val, a);
            if(!arg_val)
            {
                break;
            }

            arg = g_new0(GSwatDebuggableFrameArgument, 1);

            literal_val = gdbmi_value_hash_lookup(arg_val, "name");
            arg->name = g_strdup(gdbmi_value_literal_get(literal_val));

            literal_val = gdbmi_value_hash_lookup(arg_val, "value");
            arg->value = g_strdup(gdbmi_value_literal_get(literal_val));

            current_frame->arguments = 
                g_list_prepend(current_frame->arguments, arg);

            a++;
        }

        n--;
    }

    gdbmi_value_free(top_val);

    g_object_notify(G_OBJECT(self), "stack");
}


static void
update_variable_objects(GSwatGdbDebugger *self)
{
    gulong token;

    token = gswat_gdb_debugger_send_mi_command(self, "-var-update --simple-values *");
    gswat_gdb_debugger_mi_done_connect(self,
                                       token,
                                       (GDBMIDoneCallback)on_update_variable_objects_done,
                                       NULL);

}


static void
on_update_variable_objects_done(GSwatGdbDebugger *self,
                                gulong token,
                                const GDBMIValue *done_val,
                                GString *command,
                                gpointer *data)
{
    int i, changed_count;
    const GDBMIValue *changelist_val;

    changelist_val = gdbmi_value_hash_lookup(done_val, "changelist");

    changed_count = gdbmi_value_get_size(changelist_val);
    for(i=0; i<changed_count; i++)
    {
        const GDBMIValue *change_val, *val;
        const char *variable_gdb_name;
        GList *tmp;

        change_val = gdbmi_value_list_get_nth(changelist_val, i);
        val = gdbmi_value_hash_lookup(change_val, "in_scope");

        if(strcmp(gdbmi_value_literal_get(val), "true") != 0)
        {
            continue;
        }

        val = gdbmi_value_hash_lookup(change_val, "name");
        variable_gdb_name = gdbmi_value_literal_get(val);

        for(tmp=self->priv->all_variables; tmp!=NULL; tmp=tmp->next)
        {
            char *gdb_name;

            gdb_name = gswat_gdb_variable_object_get_name(tmp->data);
            if(strcmp(gdb_name, variable_gdb_name)==0)
            {
                val = gdbmi_value_hash_lookup(change_val, "value");
                if(val)
                {
                    const char *new_value;
                    new_value = gdbmi_value_literal_get(val);
                    _gswat_gdb_variable_object_set_cache_value(tmp->data, new_value);
                }

            }
        }

    }
}


gulong
gswat_gdb_debugger_send_mi_command(GSwatGdbDebugger* object,
                                   gchar const* command)
{
    GSwatGdbDebugger *self;
    gchar *complete_command = NULL;
    gsize written = 0;

    g_return_val_if_fail(GSWAT_IS_GDB_DEBUGGER(object), 0);
    self = GSWAT_GDB_DEBUGGER(object);

    complete_command = g_strdup_printf("%ld%s\n", self->priv->gdb_sequence, command);

    if(g_io_channel_write(self->priv->gdb_in,
                          complete_command,
                          strlen(complete_command),
                          &written) != G_IO_ERROR_NONE)
    {
        g_warning(_("Couldn't send command '%s' to gdb"), command);
    }

    g_message("gdb mi command:%s\n",complete_command);

    g_free(complete_command);

    return self->priv->gdb_sequence++;
}


void
gswat_gdb_debugger_send_cli_command(GSwatGdbDebugger* object,
                                    gchar const* command)
{
    GSwatGdbDebugger *self;
    gchar *complete_command = NULL;
    gsize written = 0;

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);

    complete_command = g_strdup_printf("%s\n", command);

    if(g_io_channel_write(self->priv->gdb_in,
                          complete_command,
                          strlen(complete_command),
                          &written) != G_IO_ERROR_NONE)
    {
        g_warning(_("Couldn't send command '%s' to gdb"), command);
    }

    g_message("gdb cli command:%s\n",complete_command);

    g_free(complete_command);

    return;
}




GDBMIValue *
gswat_gdb_debugger_get_mi_value(GSwatGdbDebugger *object,
                                gulong token,
                                GDBMIValue **mi_error)
{
    GSwatGdbDebugger *self;
    GString *current_line;
    gchar *remainder;
    gboolean found = FALSE;
    GDBMIValue *val = NULL;
    int n;

    g_return_val_if_fail(GSWAT_IS_GDB_DEBUGGER(object), NULL);
    self = GSWAT_GDB_DEBUGGER(object);

    g_io_channel_flush(self->priv->gdb_in, NULL);


    n=0;
    while((current_line = g_queue_peek_nth(self->priv->gdb_pending, n)))
    {
        gulong command_token;
        GDBMIValue *val;

        command_token = strtol(current_line->str, &remainder, 10);
        if(command_token == 0 && current_line->str == remainder)
        {
            n++;
            continue;
        }

        if(command_token == token)
        {
            current_line = g_queue_pop_nth(self->priv->gdb_pending, n);

            val = gdbmi_value_parse(remainder);

            g_string_free(current_line, TRUE);

            return val;
        }

        n++;
    }


    while(TRUE)
    {
        GString *gdb_string = g_string_new("");
        GError *error = NULL;
        gulong command_token;

        g_io_channel_read_line_string(self->priv->gdb_out,
                                      gdb_string,
                                      NULL,
                                      &error);
        if(error)
        {
            g_error_free(error);
            g_assert_not_reached();
        }

        if(gdb_string->len >= 7 
           && (strncmp(gdb_string->str, "(gdb) \n", 7) == 0)
          ) 
        {
            if(found)
            {
                g_string_free(gdb_string, TRUE);
                break;
            }
            else
            {
                continue;
            }
        }

        /* remove the trailing newline */
        gdb_string = g_string_truncate(gdb_string, gdb_string->len-1);

        /* We should probably cache line tokens... */
        command_token = strtol(gdb_string->str, NULL, 10);

        if(command_token == token)
        {
            char *error_str;
            g_message("found gdb line match - %s", gdb_string->str);

            found = TRUE;

            error_str=strstr(gdb_string->str, "^error,");
            if(error_str && error_str < strchr(gdb_string->str, ','))
            {
                val = NULL;
                if(mi_error)
                {
                    *mi_error = gdbmi_value_parse(gdb_string->str);
                }
            }
            else
            {
                val = gdbmi_value_parse(gdb_string->str);
                if(mi_error)
                {
                    *mi_error = NULL;
                }
            }
            g_string_free(gdb_string, TRUE);
        }
        else
        {
            g_message("adding to pending - \"%s\"", gdb_string->str);
            g_queue_push_tail(self->priv->gdb_pending, gdb_string);
        }
    }

    if(!g_queue_is_empty(self->priv->gdb_pending))
    {
        /* install a one off idle handler to process any commands
         * that were added to the gdb_pending queue
         */
        g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
                        process_gdb_pending,
                        self,
                        NULL);
    }

    if(found)
    {
        return val;
    }

    g_assert_not_reached();
    return NULL;
}


void
gswat_gdb_debugger_run(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);

    gswat_gdb_debugger_send_mi_command(self, "-exec-run");
}


void
gswat_gdb_debugger_request_line_breakpoint(GSwatDebuggable *object,
                                           gchar *uri,
                                           guint line)
{
    GSwatGdbDebugger *self;
    gchar *gdb_command;
    gchar *filename;

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);

    if(self->priv->state == GSWAT_DEBUGGABLE_NOT_RUNNING
       || self->priv->state == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
        gulong token;

        filename = gnome_vfs_get_local_path_from_uri(uri);
        g_assert(filename);

        gdb_command = g_strdup_printf("-break-insert %s:%d", filename, line);
        token = gswat_gdb_debugger_send_mi_command(self, gdb_command);
        gswat_gdb_debugger_mi_done_connect(self,
                                           token,
                                           (GDBMIDoneCallback)on_break_insert_done,
                                           NULL
                                          );

        g_free(gdb_command);
        g_free(filename);
    }
}


static void
on_break_insert_done(GSwatGdbDebugger *self,
                     gulong token,
                     const GDBMIValue *val,
                     GString *command,
                     gpointer *data)
{
    GSwatDebuggableBreakpoint *breakpoint;
    const GDBMIValue *bkpt_val, *literal_val;
    const char *literal;

    /*
       {
       bkpt = {
       addr = "0x08048397",
       times = "0",
       func = "function2",
       type = "breakpoint",
       number = "2",
       file = "test.c",
       disp = "keep",
       enabled = "y",
       fullname = "/home/rob/local/gswat/bin/test.c",
       line = "16",
       },
       },
       */

    bkpt_val = gdbmi_value_hash_lookup(val, "bkpt");
    g_assert(bkpt_val);

    breakpoint = g_new0(GSwatDebuggableBreakpoint, 1);

    literal_val = gdbmi_value_hash_lookup(bkpt_val, "fullname");
    literal = gdbmi_value_literal_get(literal_val);
    breakpoint->source_uri = uri_from_filename(literal);

    literal_val = gdbmi_value_hash_lookup(bkpt_val, "line");
    literal = gdbmi_value_literal_get(literal_val);
    breakpoint->line = strtol(literal, NULL, 10);

    self->priv->breakpoints =
        g_list_prepend(self->priv->breakpoints, breakpoint);

    g_object_notify(G_OBJECT(self), "breakpoints");

}


void
gswat_gdb_debugger_request_function_breakpoint(GSwatDebuggable *object,
                                               gchar *function)
{
    GSwatGdbDebugger *self;
    GString *gdb_command = g_string_new("");

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);

    if(self->priv->state == GSWAT_DEBUGGABLE_NOT_RUNNING
       || self->priv->state == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
        gulong token;

        g_string_printf(gdb_command, "-break-insert %s", function);

        token = gswat_gdb_debugger_send_mi_command(self, gdb_command->str);
        gswat_gdb_debugger_mi_done_connect(self,
                                           token,
                                           GDBMI_DONE_CALLBACK(
                                                               on_break_insert_done
                                                              )
                                           ,
                                           NULL
                                          );

        g_string_free(gdb_command, TRUE);
    }
}


void
gswat_gdb_debugger_request_address_breakpoint(GSwatDebuggable *object,
                                              unsigned long address)
{
    GSwatGdbDebugger *self;
    gchar *gdb_command;

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);

    if(self->priv->state == GSWAT_DEBUGGABLE_NOT_RUNNING
       || self->priv->state == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
        gulong token;

        gdb_command = g_strdup_printf("-break-insert 0x%lx", address);
        token = gswat_gdb_debugger_send_mi_command(self, gdb_command);
        g_free(gdb_command);
        gswat_gdb_debugger_mi_done_connect(self,
                                           token,
                                           GDBMI_DONE_CALLBACK(
                                                               on_break_insert_done
                                                              )
                                           ,
                                           NULL
                                          );

    }
}


void
gswat_gdb_debugger_continue(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;
    //GString *gdb_command = g_string_new("");

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);

    //g_string_printf(gdb_command, "-exec-continue\n", self->priv->gdb_sequence++);

    gswat_gdb_debugger_send_mi_command(self, "-exec-continue");

    //g_string_free(gdb_command, TRUE);
}


void
gswat_gdb_debugger_finish(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;
    //GString *gdb_command = g_string_new("");

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);

    //g_string_printf(gdb_command, "-exec-continue\n", self->priv->gdb_sequence++);

    gswat_gdb_debugger_send_mi_command(self, "-exec-finish");

    //g_string_free(gdb_command, TRUE);
}




void
gswat_gdb_debugger_step_into(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;
    //GString *gdb_command = g_string_new("");

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);

    //g_string_printf(gdb_command, "-exec-step", self->priv->gdb_sequence++);

    gswat_gdb_debugger_send_mi_command(self, "-exec-step");

    //g_string_free(gdb_command, TRUE);
}

void
gswat_gdb_debugger_next(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;
    //GString *gdb_command = g_string_new("");

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);

    //g_string_printf(gdb_command, "%ld -exec-next\n", self->priv->gdb_sequence++);

    gswat_gdb_debugger_send_mi_command(self, "-exec-next");

    //g_string_free(gdb_command, TRUE);
}


void
gswat_gdb_debugger_interrupt(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);

    /* FIXME - don't assume the process is local. */
    if(self->priv->state == GSWAT_DEBUGGABLE_RUNNING)
    {
        kill(self->priv->target_pid, SIGINT);
    }
}


void
gswat_gdb_debugger_restart(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);

    if(self->priv->state == GSWAT_DEBUGGABLE_RUNNING
       || self->priv->state == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
        //kill(self->priv->target_pid, SIGKILL);
        gswat_gdb_debugger_send_mi_command(self, "-target-detach");

        spawn_local_process(self);
    }
}

gchar *
gswat_gdb_debugger_get_source_uri(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;

    g_return_val_if_fail(GSWAT_IS_GDB_DEBUGGER(object), NULL);
    self = GSWAT_GDB_DEBUGGER(object);

    return g_strdup(self->priv->source_uri);
}


gulong
gswat_gdb_debugger_get_source_line(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;

    g_return_val_if_fail(GSWAT_IS_GDB_DEBUGGER(object), 0);
    self = GSWAT_GDB_DEBUGGER(object);

    return self->priv->source_line;
}


guint
gswat_gdb_debugger_get_state(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;

    g_return_val_if_fail(GSWAT_IS_GDB_DEBUGGER(object), 0);
    self = GSWAT_GDB_DEBUGGER(object);

    return self->priv->state;
}


guint
gswat_gdb_debugger_get_state_stamp(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;

    g_return_val_if_fail(GSWAT_IS_GDB_DEBUGGER(object), 0);
    self = GSWAT_GDB_DEBUGGER(object);

    return self->priv->state_stamp;
}


GList *
gswat_gdb_debugger_get_stack(GSwatDebuggable *object)
{ 
    GSwatGdbDebugger *self;
    GSwatDebuggableFrame *current_frame, *new_frame;
    GList *tmp, *tmp2, *new_stack=NULL;
    GSwatDebuggableFrameArgument *current_arg, *new_arg;

    g_return_val_if_fail(GSWAT_IS_GDB_DEBUGGER(object), NULL);

    self = GSWAT_GDB_DEBUGGER(object);

    /* copy the stack list */
    for(tmp=self->priv->stack; tmp!=NULL; tmp=tmp->next)
    {
        current_frame = (GSwatDebuggableFrame *)tmp->data;

        new_frame = g_new(GSwatDebuggableFrame, 1);
        new_frame->level = current_frame->level;
        new_frame->address = current_frame->address;
        new_frame->function = g_strdup(current_frame->function);

        /* deep copy the arguments */
        new_frame->arguments = NULL;
        for(tmp2=current_frame->arguments; tmp2!=NULL; tmp2=tmp2->next)
        {
            current_arg = (GSwatDebuggableFrameArgument *)tmp2->data;

            new_arg = g_new0(GSwatDebuggableFrameArgument, 1);
            new_arg->name = g_strdup(current_arg->name);
            new_arg->value = g_strdup(current_arg->value);
            new_frame->arguments = 
                g_list_prepend(new_frame->arguments, new_arg);
        }

        new_stack = g_list_prepend(new_stack, new_frame);
    }

    return new_stack;
}

GList *
gswat_gdb_debugger_get_breakpoints(GSwatDebuggable *object)
{ 
    GSwatGdbDebugger *self;
    GSwatDebuggableBreakpoint *current_breakpoint, *new_breakpoint;
    GList *tmp, *new_breakpoints=NULL;

    g_return_val_if_fail(GSWAT_IS_GDB_DEBUGGER(object), NULL);
    self = GSWAT_GDB_DEBUGGER(object);

    /* copy the stack list */
    for(tmp=self->priv->breakpoints; tmp!=NULL; tmp=tmp->next)
    {
        current_breakpoint = (GSwatDebuggableBreakpoint *)tmp->data;

        new_breakpoint = g_new(GSwatDebuggableBreakpoint, 1);
        memcpy(new_breakpoint, current_breakpoint, sizeof(GSwatDebuggableBreakpoint));

        new_breakpoints = g_list_prepend(new_breakpoints, new_breakpoint);
    }

    return new_breakpoints;
}


GList *
gswat_gdb_debugger_get_locals_list(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;
    gulong token;
    GDBMIValue *mi_results1, *mi_results2;
    const GDBMIValue *local, *var, *frame, *args, *stack;
    const gchar * name;
    GList *names, *tmp, *tmp2, *locals_copy;
    guint i;

    g_return_val_if_fail(GSWAT_IS_GDB_DEBUGGER(object), NULL);
    self = GSWAT_GDB_DEBUGGER(object);


    if( (self->priv->state & GSWAT_DEBUGGABLE_RUNNING)
        || (self->priv->locals_stamp == self->priv->state_stamp)
      )
    {
        g_list_foreach(self->priv->locals, (GFunc)g_object_ref, NULL);
        return g_list_copy(self->priv->locals);
    }

    self->priv->locals_stamp = self->priv->state_stamp;

    token = gswat_gdb_debugger_send_mi_command(self, "-stack-list-arguments 0 0 0");
    mi_results1 = gswat_gdb_debugger_get_mi_value(self, token, NULL);

    names = NULL;
    /* Add arguments */
    stack = gdbmi_value_hash_lookup(mi_results1, "stack-args");
    if(stack)
    {
        frame = gdbmi_value_list_get_nth(stack, 0);
        if(frame)
        {
            args = gdbmi_value_hash_lookup(frame, "args");
            if(args)
            {
                for(i = 0; i < gdbmi_value_get_size(args); i++)
                {
                    var = gdbmi_value_list_get_nth(args, i);
                    if(var)
                    {
                        name = gdbmi_value_literal_get(var);
                        names = g_list_prepend(names, (gchar *)name);
                    }
                }

            }
        }
    }


    token = gswat_gdb_debugger_send_mi_command(self, "-stack-list-locals 0");
    mi_results2 = gswat_gdb_debugger_get_mi_value(self, token, NULL);

    /* List local variables */	
    local = gdbmi_value_hash_lookup(mi_results2, "locals");
    if(local)
    {
        for(i = 0; i < gdbmi_value_get_size(local); i++)
        {
            var = gdbmi_value_list_get_nth(local, i);
            if(var)
            {
                name = gdbmi_value_literal_get(var);
                names = g_list_prepend(names, (gchar *)name);
            }
        }
    }



    for(tmp=names; tmp!=NULL; tmp=tmp->next)
    {
        gboolean found = FALSE;

        for(tmp2=self->priv->locals; tmp2!=NULL; tmp2=tmp2->next)
        {
            GSwatVariableObject *variable_object;
            char *current_name;

            variable_object = tmp2->data;
            current_name = 
                gswat_variable_object_get_expression(variable_object);

            if(strcmp(tmp->data, current_name) == 0)
            {
                found = TRUE;
                g_free(current_name);
                break;
            }
            g_free(current_name);
        }

        if(!found)
        {
            GSwatVariableObject *variable_object;

            variable_object = gswat_gdb_variable_object_new(self,
                                                            (char *)tmp->data,
                                                            NULL,
                                                            -1);

            self->priv->locals = 
                g_list_prepend(self->priv->locals, variable_object);

        }
    }

    /* prune, extra variable objects from self->priv->locals */
    locals_copy=g_list_copy(self->priv->locals);
    for(tmp=locals_copy; tmp!=NULL; tmp=tmp->next)
    {
        gboolean found = FALSE;
        GSwatVariableObject *variable_object;
        char *current_name;

        variable_object = tmp->data;
        current_name = 
            gswat_variable_object_get_expression(variable_object);

        for(tmp2=names; tmp2!=NULL; tmp2=tmp2->next)
        {
            if(strcmp(current_name, tmp2->data) == 0)
            {
                found = TRUE;
                break;
            }
        }
        g_free(current_name);

        if(!found)
        {
            self->priv->locals = 
                g_list_remove(self->priv->locals, variable_object);
            g_object_unref(variable_object);
        }
    }
    g_list_free(locals_copy);


    g_list_free(names);

    gdbmi_value_free(mi_results1);
    gdbmi_value_free(mi_results2);


    g_list_foreach(self->priv->locals, (GFunc)g_object_ref, NULL);
    return g_list_copy(self->priv->locals);
}


void
_gswat_gdb_debugger_register_variable_object(GSwatGdbDebugger* self,
                                             GSwatGdbVariableObject *variable_object)
{
    GList *item;

    item = g_list_alloc();
    item->data = variable_object;

    g_object_add_weak_pointer(G_OBJECT(variable_object),
                              (gpointer)&item->data);

    self->priv->all_variables = 
        g_list_concat(item, self->priv->all_variables);

    self->priv->all_variables = 
        g_list_remove_all(self->priv->all_variables, NULL);
}

void
_gswat_gdb_debugger_unregister_variable_object(GSwatGdbDebugger* self,
                                               GSwatGdbVariableObject *variable_object)
{
    GList *item;

    item = g_list_find(self->priv->all_variables, variable_object);
    if(item)
    {
        g_object_remove_weak_pointer(G_OBJECT(variable_object),
                                     (gpointer)&item->data);
    }
    self->priv->all_variables = 
        g_list_remove(self->priv->all_variables, variable_object);

    self->priv->all_variables = 
        g_list_remove_all(self->priv->all_variables, NULL);
}

