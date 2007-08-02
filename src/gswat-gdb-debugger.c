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

/* Macros and defines */
#define GSWAT_GDB_DEBUGGER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GSWAT_TYPE_GDB_DEBUGGER, GSwatGdbDebuggerPrivate))
#define GDBMI_DONE_CALLBACK(X)  ((GDBMIDoneCallback)(X))

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
    PROP_STATE,
    PROP_STACK,
    PROP_LOCALS,
    PROP_BREAKPOINTS,
    PROP_SOURCE_URI,
    PROP_SOURCE_LINE,
    PROP_FRAME
};

typedef struct {
    gboolean in_use;
    GList *names;
    gulong list_arguments_token;
    gulong list_locals_token;
    gboolean list_arguments_done;
}LocalsUpdateMachine;

typedef struct {
    gboolean in_use;
    GQueue *new_stack;
    gulong list_frames_token;
    gulong list_args_token;
    gboolean list_frames_done;
}StackUpdateMachine;

struct _GSwatGdbDebuggerPrivate
{
    GSwatSession            *session;
    
    GSwatDebuggableState    state;
    
    /* Every time the debugger stops, then this
     * is incremented. This is used to validate
     * variable objects. */
    guint                   interrupt_count;
    

    /* A small state machine for tracking the
     * update of the stack frames list which
     * is done using multiple asynchronous GDB/MI
     * requests */
    StackUpdateMachine      stack_machine;
    /* A list of GSwatDebuggableFrames representing
     * the current stack */
    GQueue                  *stack;
    /* When the stack is invalidated, then we have
     * to send a request to GDB for the data */
    gboolean                stack_valid;
    /* The currently active frame level */
    guint                   frame_level;

    gchar                   *current_source_uri;
    gint                    current_line;
    

    GList                   *breakpoints;
    //gchar                   *source_uri;
    //gint                    source_line;
    

    /* A small state machine for tracking the
     * update of the local variables list which
     * is done using two asynchronous GDB/MI
     * requests */
    LocalsUpdateMachine     locals_machine;
    /* A list of variable objects for the current
     * frame's local variables */
    GList                   *locals;
    /* When the locals are invalidated, then we have
     * to send a request to GDB for the data */
    gboolean                locals_valid;


    /* Our GDB conection state */
    gboolean        gdb_connected;
    GIOChannel      *gdb_in;
    GIOChannel      *gdb_out;
    guint           gdb_out_event;
    GIOChannel      *gdb_err;
    guint           gdb_err_event;
    GPid            gdb_pid;
    
    /* Our incrementing source of tokens to
     * use with GDB MI requests */
    unsigned long   gdb_sequence;
    
    /* GDB records are read in one line at a time and
     * entered into the gdb_pending queue. */
    GQueue          *gdb_pending;
    
    GSList          *mi_handlers;

    /* When we add an entry to gdb_pending we queue
     * an idle handler for processing the commands
     * - only if one hasn't already been queued. */
    gboolean        gdb_pending_idle_processor_queued;
    
    /* The job of starting the process being debugged
     * is delegated to a seperate "spawner" program
     * which we currently communicate with via named
     * fifos */
    int             spawn_read_fifo_fd;
    int             spawn_write_fifo_fd;
    GIOChannel      *spawn_read_fifo;
    GIOChannel      *spawn_write_fifo;
    GPid            spawner_pid;
    
    /* Currently we only support debugging local processes,
     * and this is the pid of the process being debugged */
    GPid            target_pid; 
};

typedef struct {
    gulong token;
    GSwatGdbMIRecordCallback result_callback;
    gpointer data;
}GSwatGdbMIHandler;

/* FIXME: this should be removed and records should be added
 * to the pending queue using GSwatGdbMIRecord structs.
 */
typedef struct {
    gulong token;
    GString *record_str;
}GdbPendingRecord;


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
static gboolean gdb_stdout_watcher(GIOChannel* io,
                                   GIOCondition cond,
                                   gpointer data);
static void queue_idle_process_gdb_pending(GSwatGdbDebugger *self);
static void de_queue_idle_process_gdb_pending(GSwatGdbDebugger *self);
static gboolean gdb_stderr_watcher(GIOChannel* io,
                                   GIOCondition cond,
                                   gpointer data);
static void read_next_gdb_line(GSwatGdbDebugger* self, GIOChannel* io);
static void free_pending_record(GdbPendingRecord *pending_record);
static void queue_pending_record(GSwatGdbDebugger *self,
                                 gulong token,
                                 GString *record_str);
static gboolean idle_process_gdb_pending(gpointer data);
static void process_gdb_pending(GSwatGdbDebugger *self);
static void process_gdb_output_record(GSwatGdbDebugger *self,
                                      GdbPendingRecord *record);

static GSwatGdbMIRecordType 
gdb_mi_get_result_record_type(GString *record_str);
static void process_gdb_mi_result_record(GSwatGdbDebugger *self,
                                         gulong token,
                                         GString *record_str);
static void process_gdb_mi_oob_record(GSwatGdbDebugger *self,
                                      gulong token,
                                      GString *record_str);
static void process_gdb_mi_oob_stopped_record(GSwatGdbDebugger *self,
                                              GSwatGdbMIRecord *record);
static void process_gdb_mi_stream_record(GSwatGdbDebugger *self,
                                         gulong token,
                                         GString *record_str);
static void process_frame(const GDBMIValue *val, GSwatDebuggableFrame *frame);
static gchar *uri_from_filename(const gchar *filename);
static void set_source_location(GSwatGdbDebugger *self,
                                const gchar *source_uri,
                                gint line);
static void synchronous_update_stack(GSwatGdbDebugger *self);
static void kick_asynchronous_locals_update(GSwatGdbDebugger *self);
static void
async_locals_update_list_args_mi_callback(GSwatGdbDebugger *self,
                                          const GSwatGdbMIRecord *record,
                                          void *data);
static void async_locals_update_list_locals_mi_callback(GSwatGdbDebugger *self,
                                            const GSwatGdbMIRecord *record,
                                            void *data);
static void update_locals_list_from_name_list(GSwatGdbDebugger *self, GList *names);
static void kick_asynchronous_stack_update(GSwatGdbDebugger *self);
static void async_stack_update_list_frames_mi_callback(GSwatGdbDebugger *self,
                                           const GSwatGdbMIRecord *record,
                                           void *data);
static void async_stack_update_list_args_mi_callback(GSwatGdbDebugger *self,
                                         const GSwatGdbMIRecord *record,
                                         void *data);
static void on_local_variable_object_invalidated(GObject *object,
                                                 GParamSpec *property,
                                                 gpointer data);
static GSwatGdbMIRecord *
find_pending_result_record_for_token(GSwatGdbDebugger *self,
                                     gulong token);
static void basic_runner_mi_callback(GSwatGdbDebugger *self,
                                     const GSwatGdbMIRecord *record,
                                     void *data);
static void break_insert_mi_callback(GSwatGdbDebugger *self,
                                     const GSwatGdbMIRecord *record,
                                     void *data);
static void
restart_running_mi_callback(GSwatGdbDebugger *self,
                            const GSwatGdbMIRecord *record,
                            void *data);
static void synchronous_update_locals_list(GSwatGdbDebugger *self);
static guint gdb_debugger_get_frame(GSwatDebuggable* object);
static void gdb_debugger_set_frame(GSwatDebuggable* object, guint frame);


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
    //GParamSpec *new_param;

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

    g_object_class_override_property(gobject_class,
                                     PROP_STATE,
                                     "state");
    g_object_class_override_property(gobject_class,
                                     PROP_STACK,
                                     "stack");
    g_object_class_override_property(gobject_class,
                                     PROP_LOCALS,
                                     "locals");
    g_object_class_override_property(gobject_class,
                                     PROP_BREAKPOINTS,
                                     "breakpoints");
    g_object_class_override_property(gobject_class,
                                     PROP_SOURCE_URI,
                                     "source-uri");
    g_object_class_override_property(gobject_class,
                                     PROP_SOURCE_LINE,
                                     "source-line");
    g_object_class_override_property(gobject_class,
                                     PROP_FRAME,
                                     "frame");


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
    gchar *source_uri;

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
        case PROP_LOCALS:
            g_value_set_pointer(value,
                                gswat_gdb_debugger_get_locals_list(self_debuggable)
                                );
            break;
        case PROP_BREAKPOINTS:
            g_value_set_pointer(value,
                                gswat_gdb_debugger_get_breakpoints(self_debuggable)
                               );
            break;
        case PROP_SOURCE_URI:
            source_uri = gswat_gdb_debugger_get_source_uri(self_debuggable); 
            g_value_set_string_take_ownership(value, source_uri);
            break;
        case PROP_SOURCE_LINE:
            g_value_set_ulong(value,
                              gswat_gdb_debugger_get_source_line(self_debuggable));
            break;
        case PROP_FRAME:
            g_value_set_uint(value, self->priv->frame_level);
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
    //GSwatGdbDebugger* self = GSWAT_GDB_DEBUGGER(object);
    GSwatDebuggable* self_debuggable = GSWAT_DEBUGGABLE(object);

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
        case PROP_FRAME:
            gdb_debugger_set_frame(self_debuggable, g_value_get_uint(value));
            break;
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
    debuggable->get_stack = gswat_gdb_debugger_get_stack;
    debuggable->get_breakpoints = gswat_gdb_debugger_get_breakpoints;
    debuggable->get_locals_list = gswat_gdb_debugger_get_locals_list;
    debuggable->get_frame = gdb_debugger_get_frame;
    debuggable->set_frame = gdb_debugger_set_frame;

}

/* Instance Construction */
static void
gswat_gdb_debugger_init(GSwatGdbDebugger *self)
{
    self->priv = GSWAT_GDB_DEBUGGER_GET_PRIVATE(self);

    self->priv->gdb_pending = g_queue_new();

    self->priv->stack = g_queue_new();

    self->priv->gdb_sequence = 1;

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


    g_object_unref(self->priv->session);
    self->priv->session=NULL;


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


    terminal_command = g_strdup_printf("gnome-terminal -x " 
                                        GSWAT_BIN_DIR 
                                        "/gswat-spawner --read-fifo %s --write-fifo %s",
                                       fifo1_name,
                                       fifo0_name);

    g_shell_parse_argv(terminal_command, &argc, &argv, NULL);
    g_message(terminal_command);
    g_free(terminal_command);


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

    self->priv->target_pid = -1;

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

    self->priv->target_pid = -1;
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
                           (GIOFunc)gdb_stdout_watcher,
                           self);
        self->priv->gdb_err_event =
            g_io_add_watch(self->priv->gdb_err,
                           G_IO_IN,
                           (GIOFunc)gdb_stderr_watcher,
                           self);
    }

    user_command = gswat_session_get_command(GSWAT_SESSION(self->priv->session),
                                             &user_argc,
                                             &user_argv);

    self->priv->gdb_connected = TRUE;

    /* assume this session is for debugging a local file */
    spawn_local_process(self);

}


void
gswat_gdb_debugger_target_disconnect(GSwatDebuggable* object)
{
    GSwatGdbDebugger *self;
    GList *tmp;
    GString *current_line;

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);

    if(self->priv->state == GSWAT_DEBUGGABLE_DISCONNECTED)
    {
        return;
    }
    
    /* As soon as this is set gswat_gdb_debugger_send_mi_command
     * effectivly becomes a NOP, so gswat_gdb_variable_object_cleanup
     * should be able to cope with that. 
     * We don't actually care to cleanly delete the variable objects
     * gdb side, since we are about to kill gdb anyhow. */
    self->priv->gdb_connected = FALSE;
    
    /* So we don't get lots of call back during 
     * gswat_gdb_variable_object_cleanup: */
    for(tmp=self->priv->locals; tmp!=NULL; tmp=tmp->next)
    {
        g_object_disconnect(tmp->data,
                            "any-signal",
                            G_CALLBACK(on_local_variable_object_invalidated),
                            self,
                            NULL);
        g_object_unref(tmp->data);
    }
    g_list_free(self->priv->locals);
    self->priv->locals=NULL;
    
    /* Invalidate all other variable objects */
    gswat_gdb_variable_object_cleanup(self);
    
    g_source_remove(self->priv->gdb_out_event);
    g_source_remove(self->priv->gdb_err_event);

    if(self->priv->gdb_in) {
        g_io_channel_unref(self->priv->gdb_in);
        self->priv->gdb_in = NULL;
    }
    
    if(self->priv->gdb_out) {
        g_io_channel_unref(self->priv->gdb_out);
        self->priv->gdb_out = NULL;
    }
    
    if(self->priv->gdb_err) {
        g_io_channel_unref(self->priv->gdb_err);
        self->priv->gdb_err = NULL;
    }
    
    stop_spawner_process(self);
    
    while((current_line = g_queue_pop_head(self->priv->gdb_pending)))
    {
        g_string_free(current_line, TRUE);
    }

    if(self->priv->stack)
    {
        gswat_debuggable_stack_free(self->priv->stack);
        self->priv->stack = NULL;
    }

    if(self->priv->breakpoints)
    {
        gswat_debuggable_free_breakpoints(self->priv->breakpoints);
        self->priv->breakpoints = NULL;
    }

    de_queue_idle_process_gdb_pending(self);

    self->priv->state = GSWAT_DEBUGGABLE_DISCONNECTED;
    g_object_notify(G_OBJECT(self), "state");
}

void
gswat_gdb_debugger_nop_mi_callback(GSwatGdbDebugger *self,
                                   const GSwatGdbMIRecord *record,
                                   void *data)
{
    g_message("NOP result callback:");
    if(record->val)
    {
        gdbmi_value_dump(record->val, 0);
    }
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
    gswat_gdb_debugger_send_mi_command(self,
                                       gdb_command,
                                       gswat_gdb_debugger_nop_mi_callback,
                                       NULL);
    g_free(gdb_command);
    g_strfreev(argv);


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
    gswat_gdb_debugger_send_mi_command(self,
                                       "-exec-continue",
                                       gswat_gdb_debugger_nop_mi_callback,
                                       NULL);
    gswat_gdb_debugger_send_mi_command(self,
                                       "-exec-continue",
                                       gswat_gdb_debugger_nop_mi_callback,
                                       NULL);


}


static gboolean
gdb_stdout_watcher(GIOChannel* io, GIOCondition cond, gpointer data)
{
    GSwatGdbDebugger *self = GSWAT_GDB_DEBUGGER(data);

    read_next_gdb_line(self, io);

    queue_idle_process_gdb_pending(self);
    return TRUE;
}


static void
queue_idle_process_gdb_pending(GSwatGdbDebugger *self)
{
    if(!self->priv->gdb_pending_idle_processor_queued)
    {
        /* install a one off idle handler to process
         * the gdb_pending queue */
        g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
                        idle_process_gdb_pending,
                        self,
                        NULL);
        self->priv->gdb_pending_idle_processor_queued = TRUE;
    }
}

static void
de_queue_idle_process_gdb_pending(GSwatGdbDebugger *self)
{
    if(self->priv->gdb_pending_idle_processor_queued)
    {
        g_idle_remove_by_data(self);
        self->priv->gdb_pending_idle_processor_queued = FALSE;
    }
}



static gboolean
gdb_stderr_watcher(GIOChannel* io, GIOCondition cond, gpointer data)
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
read_next_gdb_line(GSwatGdbDebugger* self, GIOChannel* io)
{
    GString *record_str = g_string_new(""), *tmp;
    GError *error = NULL;
    gulong record_token;
    gchar *remainder;

    g_io_channel_read_line_string(io, record_str, NULL, &error);
    if(error)
    {
        g_message("GDB IO error: %s", error->message);
        g_error_free(error);
        g_string_free(record_str, TRUE);
        return;
    }

    if(record_str->len >= 7 
       && (strncmp(record_str->str, "(gdb) \n", 7) == 0))
    {
        g_string_free(record_str, TRUE);
        return;
    }

    /* remove the trailing newline */
    record_str = g_string_truncate(record_str, record_str->len-1);

    /* read any gdbmi "token" */
    record_token = strtol(record_str->str, &remainder, 10);
    if(!(record_token == 0 && record_str->str == remainder))
    {
        tmp = g_string_new(remainder);
        g_string_free(record_str, TRUE);
        record_str = tmp;
    }
    else
    {
        /* most gdb records should be associated with a token */
        if(record_str->str[0] != '~'
           && record_str->str[0] != '@'
           && record_str->str[0] != '&')
        {
            g_warning("process_gdb_output_record: failed to read a valid token value");
        }
    }

    queue_pending_record(self, record_token, record_str);

    return;
}


static void
queue_pending_record(GSwatGdbDebugger *self, gulong token, GString *record_str)
{
    GdbPendingRecord *pending_record;

    g_message("queueing gdb record - token=\"%lu\", record=\"%s\"",
              token,
              record_str->str);

    pending_record = g_new(GdbPendingRecord, 1);
    pending_record->token = token;
    pending_record->record_str = record_str;

    g_queue_push_tail(self->priv->gdb_pending, pending_record);
}


static void
free_pending_record(GdbPendingRecord *pending_record)
{
    g_string_free(pending_record->record_str, TRUE);
    g_free(pending_record);
}


static gboolean
idle_process_gdb_pending(gpointer data)
{
    GSwatGdbDebugger *self = GSWAT_GDB_DEBUGGER(data);

    self->priv->gdb_pending_idle_processor_queued = FALSE;
    
    g_object_ref(G_OBJECT(self));
    process_gdb_pending(self);
    g_object_unref(G_OBJECT(self));

    return FALSE;
}


static void
process_gdb_pending(GSwatGdbDebugger *self)
{
    GdbPendingRecord *pending_record;
    gint n;
    GQueue *copy;
    GList *tmp;

    /* We have to be carefull since processing
     * records can modify the queue: */
    copy = g_queue_copy(self->priv->gdb_pending);

#if 0
    g_message("Pending queue dump:");
    for(n=0, tmp=copy->head; tmp!=NULL; n++, tmp=tmp->next)
    {
        pending_record = tmp->data;
        g_message("index=%d: %s", n, pending_record->record_str->str); 
    }
#endif

    for(n=0, tmp=copy->head; tmp!=NULL; n++, tmp=tmp->next)
    {
        gboolean dont_un_queue = FALSE;
        pending_record = tmp->data;

        /* if this is a result record... */
        if(pending_record->record_str->str[0] == '^')
        {
            GSList *tmp;
            GSwatGdbMIHandler *current_handler;

            /* if it has a NULL result record callback we must not 
             * handle it here. It will be manually handled via
             * gswat_gdb_debugger_get_mi_result_record
             */
            for(tmp=self->priv->mi_handlers; tmp!=NULL; tmp=tmp->next)
            {
                current_handler = tmp->data;

                if(current_handler->token == pending_record->token){
                    if(current_handler->result_callback == NULL)
                    {
                        dont_un_queue = TRUE;
                    }
                    break;                 
                }
            }
        }

        if(!dont_un_queue)
        {
            g_queue_remove(self->priv->gdb_pending, pending_record);
            process_gdb_output_record(self, pending_record);
            free_pending_record(pending_record);
        }
    }

    g_queue_free(copy);
}


static void
process_gdb_output_record(GSwatGdbDebugger *self, GdbPendingRecord *record)
{
    GString *record_str;

    g_return_if_fail(record != NULL);
    g_return_if_fail(record->record_str != NULL);
    g_return_if_fail(record->record_str->len > 0);

    record_str = record->record_str;

    /* result records... */
    if(record_str->str[0] == '^')
    {
        process_gdb_mi_result_record(self, record->token, record_str);
    }/* out-of-band records... */
    else if(record_str->str[0] == '*' /* exec-async-output */
            || record_str->str[0] == '+' /* status-async-output */
            || record_str->str[0] == '=') /* notify-async-output */
    {
        process_gdb_mi_oob_record(self, record->token, record_str);
    }/* stream records... */
    else if(record_str->str[0] == '~' /* console-stream-output */
            || record_str->str[0] == '@' /* target-stream-output */
            || record_str->str[0] == '&') /* log-stream-output */
    {
        process_gdb_mi_stream_record(self, record->token, record_str);
    }

    return;
}


static void
process_gdb_mi_result_record(GSwatGdbDebugger *self,
                             gulong token,
                             GString *record_str)
{
    GDBMIValue *val;
    GSList *tmp;
    
    val = NULL;
	if(strchr(record_str->str, ','))
    {
        val = gdbmi_value_parse(record_str->str);
        if(val)
        {
            gdbmi_value_dump(val, 0);
        }
        else
        {
            g_warning("process_gdb_mi_result_record: error parsing record");
            return;
        }
    }


    for(tmp=self->priv->mi_handlers; tmp!=NULL; tmp=tmp->next)
    {
        GSwatGdbMIHandler *current_handler;
        current_handler = (GSwatGdbMIHandler *)tmp->data;

        if(current_handler->token == token
           && current_handler->result_callback != NULL)
        {
            GSwatGdbMIRecord *record;

            record = g_new(GSwatGdbMIRecord, 1);            
            record->type = gdb_mi_get_result_record_type(record_str);
            record->val = val;

            current_handler->result_callback(self,
                                             record,
                                             current_handler->data);
            g_free(record);

            self->priv->mi_handlers = 
                g_slist_remove(self->priv->mi_handlers,
                               current_handler);
            g_free(current_handler);

            break;                 
        }

    }
    
    if(val)
    {
        gdbmi_value_free(val);
    }
}

static GSwatGdbMIRecordType
gdb_mi_get_result_record_type(GString *record_str)
{
    if(record_str->len >= 5
       && strncasecmp(record_str->str, "^done", 5) == 0)
    {
        return GSWAT_GDB_MI_REC_TYPE_RESULT_DONE;
    }else if(record_str->len >= 8
             && strncasecmp(record_str->str, "^running", 8) == 0)
    {
        return GSWAT_GDB_MI_REC_TYPE_RESULT_RUNNING;
    }else if(record_str->len >= 10
             && strncasecmp(record_str->str, "^connected", 10) == 0)
    {
        return GSWAT_GDB_MI_REC_TYPE_RESULT_CONNECTED;
    }else if(record_str->len >= 6
             && strncasecmp(record_str->str, "^error", 6) == 0)
    {
        return GSWAT_GDB_MI_REC_TYPE_RESULT_ERROR;
    }else if(record_str->len >= 5
             && strncasecmp(record_str->str, "^exit", 5) == 0)
    {
        return GSWAT_GDB_MI_REC_TYPE_RESULT_EXIT;
    }else
    {
        return GSWAT_GDB_MI_REC_TYPE_UNKNOWN;
        g_warning("process_gdb_mi_recordord: unrecognised result record");
    }

}


static void
process_gdb_mi_oob_record(GSwatGdbDebugger *self,
                          gulong token,
                          GString *record_str)
{
    GDBMIValue *val;
    //GSList *tmp;

    val = gdbmi_value_parse(record_str->str);
    if(!val)
    {
        g_warning("process_gdb_mi_oob_record: error parsing record");
        return;
    }

    gdbmi_value_dump(val, 0);

    if(strncasecmp(record_str->str, "*stopped", 8) == 0)
    {   
        GSwatGdbMIRecord *record;

        record = g_new(GSwatGdbMIRecord, 1);
        record->type = GSWAT_GDB_MI_REC_TYPE_OOB_STOPPED;
        record->val = val;

        process_gdb_mi_oob_stopped_record(self, record);

        g_free(record);
    }else
    {
        g_warning("process_gdb_mi_oob_record: unrecognised out-of-band-record");
    }

#if 0
    for(tmp=self->priv->mi_handlers; tmp!=NULL; tmp=tmp->next)
    {
        GSwatGdbMIHandler *current_handler;
        current_handler = (GSwatGdbMIHandler *)tmp->data;

        if(current_handler->token == command_token
           && current_handler->oob_callback != NULL)
        {
            GSwatGdbMIRecord *record;

            record = g_new(GSwatGdbMIResult, 1);

            if(strncasecmp(record_str->str, "*stopped", 8) == 0)
            {
                record->type = GSWAT_GDB_MI_REC_TYPE_OOB_STOPPED;
            }else
            {
                record->type = GSWAT_GDB_MI_REC_TYPE_UNKNOWN;
                g_warning("process_gdb_mi_oob_record: unrecognised out-of-band-record");
            }

            record->val = val;

            current_handler->oob_callback(self,
                                          record,
                                          current_handler->data);
            g_free(record);

            break;                 
        }
    }
#endif

    gdbmi_value_free(val);
}

static void
process_gdb_mi_oob_stopped_record(GSwatGdbDebugger *self,
                                  GSwatGdbMIRecord *record)
{
    GDBMIValue *val;
    const GDBMIValue *reason;
    const gchar *str = NULL;

    val = record->val;

    gdbmi_value_dump(val, 0);

    /* Invalidate all variable objects */
    self->priv->interrupt_count++;

    reason = gdbmi_value_hash_lookup(val, "reason");
    if(reason){
        str = gdbmi_value_literal_get(reason);
    }

    if(str == NULL)
    {
        g_warning("process_gdb_mi_oob_stopped_record: no reason found");
        self->priv->state = GSWAT_DEBUGGABLE_INTERRUPTED;
    }
    else if (strcmp(str, "exited-normally") == 0)
    {
        gswat_gdb_debugger_target_disconnect(GSWAT_DEBUGGABLE(self));
    }
    else if(strcmp(str, "exited") == 0)
    {
        gswat_gdb_debugger_target_disconnect(GSWAT_DEBUGGABLE(self));
    }
    else if(strcmp(str, "exited-signalled") == 0)
    {
        gswat_gdb_debugger_target_disconnect(GSWAT_DEBUGGABLE(self));
    }
    else if(strcmp(str, "signal-received") == 0)
    {
        self->priv->state = GSWAT_DEBUGGABLE_INTERRUPTED;
    }
    else if(strcmp(str, "breakpoint-hit") == 0)
    {
        self->priv->state = GSWAT_DEBUGGABLE_INTERRUPTED;
    }
    else if(strcmp(str, "function-finished") == 0)
    {
        self->priv->state = GSWAT_DEBUGGABLE_INTERRUPTED;
    }
    else if(strcmp(str, "end-stepping-range") == 0)
    {
        self->priv->state = GSWAT_DEBUGGABLE_INTERRUPTED;
    }
    else if(strcmp(str, "location-reached") == 0)
    {
        self->priv->state = GSWAT_DEBUGGABLE_INTERRUPTED;
    }

    if(self->priv->state == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
        GSwatDebuggableFrame *frame;
        
        gdb_debugger_set_frame(GSWAT_DEBUGGABLE(self), 0);
        
        frame = g_new(GSwatDebuggableFrame, 1);
        process_frame(val, frame);
        set_source_location(self, frame->source_uri, frame->line);
        gswat_debuggable_frame_free(frame);
                
        /* The previous, stack is now invalid so delete it.
         * Note we could quite easily re-use the old allocated
         * GSwatDebuggableFrames as an optimisation if needed.
         * We just free it all at the moment for simplicity */
        self->priv->stack_valid=FALSE;
        gswat_debuggable_stack_free(self->priv->stack);
        self->priv->stack = NULL;
        kick_asynchronous_stack_update(self);

        gswat_gdb_variable_object_async_update_all(self);
        
        self->priv->locals_valid=FALSE;
        kick_asynchronous_locals_update(self);

        g_object_notify(G_OBJECT(self), "state");
    }

}


static void
process_gdb_mi_stream_record(GSwatGdbDebugger *self,
                             gulong token,
                             GString *record_str)
{
    g_message("%s", record_str->str);
}


static void
process_frame(const GDBMIValue *val, GSwatDebuggableFrame *frame)
{
    const GDBMIValue *file_val, *line_val, *frame_val;
    const GDBMIValue *level_val, *address_val, *func_val;
    const gchar *file_str;

    g_return_if_fail(val != NULL);
    
    frame_val = gdbmi_value_hash_lookup(val, "frame");
    if(!frame_val)
    {
        frame_val = val;
    }
    

    level_val = gdbmi_value_hash_lookup(frame_val, "level");
    if(level_val)
    {
        const gchar *level_str;
        level_str = gdbmi_value_literal_get(level_val);
        frame->level = strtol(level_str, NULL, 10);
    }
    else
    {
        frame->level = 0;
    }

    
    file_val = gdbmi_value_hash_lookup(frame_val, "fullname");
    if(!file_val)
    {
        file_val = gdbmi_value_hash_lookup(frame_val, "file");
    }
    if(file_val)
    {
        gchar *source_uri;
        file_str = gdbmi_value_literal_get(file_val);
        source_uri = uri_from_filename(file_str);
        frame->source_uri = source_uri;
    }
    else
    {
        frame->source_uri=NULL;
    }
    
    
    line_val = gdbmi_value_hash_lookup(frame_val, "line");
    if(line_val)
    {
        const gchar *line_str;
        line_str = gdbmi_value_literal_get(line_val);
        frame->line = strtol(line_str, NULL, 10);
    }
    else
    {
        frame->line = 0;
    }
    

    address_val = gdbmi_value_hash_lookup(frame_val, "addr");
    if(address_val)
    {
        const gchar *address_str;
        address_str = gdbmi_value_literal_get(address_val);
        frame->address = strtoul(address_str, NULL, 16);
    }

    
    func_val = gdbmi_value_hash_lookup(frame_val, "func");
    if(func_val)
    {
        const gchar *func_str;
        func_str = gdbmi_value_literal_get(func_val);
        frame->function = g_strdup(func_str);
    }

    frame->arguments = NULL;
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
    
    if(!uri)
    {
        g_warning("Could not create uri for file=\"%s\"", filename);
    }

    return uri;
}


static void
set_source_location(GSwatGdbDebugger *self,
                    const gchar *source_uri,
                    gint line)
{
    if(source_uri && self->priv->current_source_uri
       && strcmp(source_uri,
                 self->priv->current_source_uri) != 0)
    {
        g_free(self->priv->current_source_uri);
        self->priv->current_source_uri = g_strdup(source_uri);
        g_object_notify(G_OBJECT(self), "source-uri");
    }
    else if(source_uri || self->priv->current_source_uri)
    {
        g_free(self->priv->current_source_uri);
        if(source_uri)
        {
            self->priv->current_source_uri = g_strdup(source_uri);
        }
        else
        {
            self->priv->current_source_uri = NULL;
        }
        g_object_notify(G_OBJECT(self), "source-uri");
    }
    if(line != self->priv->current_line)
    {
        self->priv->current_line = line;
        g_object_notify(G_OBJECT(self), "source-line");
    }
}


/* TODO: Add a flush_ function that can take a locals_machine
 * and block until complete. */
static void
kick_asynchronous_locals_update(GSwatGdbDebugger *self)
{
    LocalsUpdateMachine *locals_machine;
    gchar *gdb_command;
    
    if( (self->priv->state & GSWAT_DEBUGGABLE_RUNNING)
        || self->priv->locals_valid
      )
    {
        return;
    }
    
    locals_machine = &self->priv->locals_machine;
    if(locals_machine->in_use == TRUE)
    {
        return;
    }

    memset(locals_machine, 0, sizeof(LocalsUpdateMachine));
    locals_machine->in_use = TRUE;

    gdb_command = g_strdup_printf("-stack-list-arguments 0 %d %d",
                                  self->priv->frame_level,
                                  self->priv->frame_level);
    locals_machine->list_arguments_token = 
        gswat_gdb_debugger_send_mi_command(self,
                                           gdb_command,
                                           async_locals_update_list_args_mi_callback,
                                           &self->priv->locals_machine);
    g_free(gdb_command);

    locals_machine->list_locals_token =
        gswat_gdb_debugger_send_mi_command(self,
                                           "-stack-list-locals 0",
                                           async_locals_update_list_locals_mi_callback,
                                           &self->priv->locals_machine);
}


static void
async_locals_update_list_args_mi_callback(GSwatGdbDebugger *self,
                                         const GSwatGdbMIRecord *record,
                                         void *data)
{
    LocalsUpdateMachine *locals_machine;
    const GDBMIValue *stack, *frame, *args, *var;
    guint i;

    locals_machine = (LocalsUpdateMachine *)data;
    if(locals_machine->in_use == FALSE)
    {
        g_warning("%s: called out of order", __FUNCTION__);
    }

    /* If someone jumped in and did a synchronous update 
     * already then we can bomb out now. */
    if(self->priv->locals_valid == TRUE)
    {
        return;
    }

    stack = gdbmi_value_hash_lookup(record->val, "stack-args");
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
                        const gchar *name;
                        name = gdbmi_value_literal_get(var);
                        locals_machine->names = 
                            g_list_prepend(locals_machine->names, g_strdup(name));
                    }
                }

            }
        }
    }

    locals_machine->list_arguments_done = TRUE;
}


static void
async_locals_update_list_locals_mi_callback(GSwatGdbDebugger *self,
                                            const GSwatGdbMIRecord *record,
                                            void *data)
{
    LocalsUpdateMachine *locals_machine;
    const GDBMIValue *local, *var;
    guint i;

    locals_machine = (LocalsUpdateMachine *)data;
    if(locals_machine->list_arguments_done != TRUE)
    {
        g_warning("%s: called out of order", __FUNCTION__);
    }

    /* If someone jumped in and did a synchronous update 
     * already then we can bomb out now. */
    if(self->priv->locals_valid == TRUE)
    {
        g_list_foreach(locals_machine->names, (GFunc)g_free, NULL);
        g_list_free(locals_machine->names);
        locals_machine->names = NULL;
        locals_machine->in_use = FALSE;
        return;
    }

    local = gdbmi_value_hash_lookup(record->val, "locals");
    if(local)
    {
        for(i = 0; i < gdbmi_value_get_size(local); i++)
        {
            var = gdbmi_value_list_get_nth(local, i);
            if(var)
            {
                const gchar *name;
                name = gdbmi_value_literal_get(var);
                locals_machine->names = 
                    g_list_prepend(locals_machine->names, g_strdup(name));
            }
        }
    }

    update_locals_list_from_name_list(self, locals_machine->names);
    g_list_foreach(locals_machine->names, (GFunc)g_free, NULL);
    g_list_free(locals_machine->names);
    locals_machine->names = NULL;

    locals_machine->in_use = FALSE;
}


static void
update_locals_list_from_name_list(GSwatGdbDebugger *self, GList *names)
{
    GList *tmp, *tmp2, *locals_copy;
    gboolean list_changed = FALSE;
    
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
                gchar *tmp_value;
                
                /* We retrieve the value here, to make sure the
                 * variable object state is is in synch with the
                 * current debugger state */
                /* Alternativly; perhaps we could have the variable
                 * object code set up a callback for our state
                 * updates, so this is done automatically. The
                 * problem with that is, here, we want the update
                 * to be synchronous. */
                tmp_value =
                    gswat_variable_object_get_value(variable_object, NULL);
                g_free(tmp_value);

                found = TRUE;
                g_free(current_name);
                break;
            }
            g_free(current_name);
        }
        
        if(!found)
        {
            GSwatVariableObject *variable_object;

            variable_object = 
                gswat_gdb_variable_object_new(self,
                                              (char *)tmp->data,
                                              NULL,
                                              GSWAT_VARIABLE_OBJECT_ANY_FRAME);

            g_signal_connect(variable_object,
                             "notify::valid",
                             G_CALLBACK(on_local_variable_object_invalidated),
                             self
                            );

            self->priv->locals = 
                g_list_prepend(self->priv->locals, variable_object);
            list_changed = TRUE;
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
            list_changed = TRUE;
        }
    }
    g_list_free(locals_copy);
    
    /* Update this before notification so that if a listener
     * decides to call gswat_debuggable_get_locals_list, we
     * wont go recursive */
    self->priv->locals_valid = TRUE;
    if(list_changed)
    {
        g_object_notify(G_OBJECT(self), "locals"); 
    }
}

static void
kick_asynchronous_stack_update(GSwatGdbDebugger *self)
{
    StackUpdateMachine *stack_machine;

    if( (self->priv->state & GSWAT_DEBUGGABLE_RUNNING)
        || self->priv->stack_valid
      )
    {
        return;
    }
    g_assert(self->priv->stack == NULL);

    stack_machine = &self->priv->stack_machine;
    if(stack_machine->in_use == TRUE)
    {
        return;
    }

    memset(stack_machine, 0, sizeof(StackUpdateMachine));
    stack_machine->in_use = TRUE;

    stack_machine->new_stack = g_queue_new();
    
    stack_machine->list_frames_token = 
        gswat_gdb_debugger_send_mi_command(self,
                                           "-stack-list-frames",
                                           async_stack_update_list_frames_mi_callback,
                                           stack_machine);
    
    stack_machine->list_args_token =
        gswat_gdb_debugger_send_mi_command(self,
                                           "-stack-list-arguments 1",
                                           async_stack_update_list_args_mi_callback,
                                           stack_machine);
}

static void
async_stack_update_list_frames_mi_callback(GSwatGdbDebugger *self,
                                           const GSwatGdbMIRecord *record,
                                           void *data)
{
    StackUpdateMachine *stack_machine;
    const GDBMIValue *stack_val, *frame_val;
    int n;
   
    stack_machine = (StackUpdateMachine *)data;
    if(stack_machine->in_use == FALSE)
    {
        g_warning("%s: called out of order", __FUNCTION__);
        return;
    }

    /* If someone jumped in and did a synchronous update 
     * already then we can bomb out now. */
    if(self->priv->stack_valid == TRUE)
    {
        stack_machine->in_use = FALSE;
        return;
    }

    if(record->type == GSWAT_GDB_MI_REC_TYPE_RESULT_ERROR)
    {
        g_warning("%s: error listing frames", __FUNCTION__);
        stack_machine->in_use = FALSE;
        return;
    }
    
    if(record->type != GSWAT_GDB_MI_REC_TYPE_RESULT_DONE)
    {
        g_warning("%s: unexpected result type", __FUNCTION__);
        stack_machine->in_use = FALSE;
        return;
    }
    
    stack_val = gdbmi_value_hash_lookup(record->val, "stack");
    
    n=0;
    while(1)
    {
        GSwatDebuggableFrame *frame;

        frame_val = gdbmi_value_list_get_nth(stack_val, n);
        if(!frame_val)
        {
            break;
        }
        
        frame = g_new0(GSwatDebuggableFrame, 1);
        g_queue_push_tail(stack_machine->new_stack, frame);

        process_frame(frame_val, frame);

        g_assert(frame->level == n);

        n++;
    }

    stack_machine->list_frames_done = TRUE;
}

static void
async_stack_update_list_args_mi_callback(GSwatGdbDebugger *self,
                                         const GSwatGdbMIRecord *record,
                                         void *data)
{
    StackUpdateMachine *stack_machine;
    GList *tmp;
    GSwatDebuggableFrame *current_frame=NULL;
    const GDBMIValue *frame_val;
    const GDBMIValue *stackargs_val, *args_val, *arg_val, *literal_val;
    GSwatDebuggableFrameArgument *arg;

    stack_machine = (StackUpdateMachine *)data;
    if(stack_machine->list_frames_done != TRUE)
    {
        g_warning("%s: called out of order", __FUNCTION__);
        gswat_debuggable_stack_free(stack_machine->new_stack);
        stack_machine->in_use = FALSE;
        return;
    }
    
    /* If someone jumped in and did a synchronous update 
     * already then we can bomb out now. */
    if(self->priv->stack_valid == TRUE)
    {
        gswat_debuggable_stack_free(stack_machine->new_stack);
        stack_machine->in_use = FALSE;
        return;
    }

    if(record->type == GSWAT_GDB_MI_REC_TYPE_RESULT_ERROR)
    {
        g_warning("%s: error listing frames", __FUNCTION__);
        gswat_debuggable_stack_free(stack_machine->new_stack);
        stack_machine->in_use = FALSE;
        return;
    }
    
    if(record->type != GSWAT_GDB_MI_REC_TYPE_RESULT_DONE)
    {
        g_warning("%s: unexpected result type", __FUNCTION__);
        gswat_debuggable_stack_free(stack_machine->new_stack);
        stack_machine->in_use = FALSE;
        return;
    }

    stackargs_val = gdbmi_value_hash_lookup(record->val,
                                            "stack-args");
    
    for(tmp=stack_machine->new_stack->head; tmp!=NULL; tmp=tmp->next)
    {
        guint a;
        current_frame = (GSwatDebuggableFrame *)tmp->data;
        
        frame_val = gdbmi_value_list_get_nth(stackargs_val,
                                             current_frame->level);
        args_val = gdbmi_value_hash_lookup(frame_val, "args");
        
        a=0;
        while(TRUE)
        {
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
    }
    
    gswat_debuggable_stack_free(self->priv->stack);
    self->priv->stack = stack_machine->new_stack;
    self->priv->stack_valid = TRUE;
    stack_machine->in_use = FALSE;

    /* lookup frame 0 */
    current_frame = self->priv->stack->head->data;
    set_source_location(self,
                        current_frame->source_uri,
                        current_frame->line);

    g_object_notify(G_OBJECT(self), "stack");
}


static void
on_local_variable_object_invalidated(GObject *object,
                                     GParamSpec *property,
                                     gpointer data)
{
    GSwatGdbDebugger *self = GSWAT_GDB_DEBUGGER(data);
    GSwatGdbVariableObject *variable_object;

    variable_object = GSWAT_GDB_VARIABLE_OBJECT(object);

    /* I don't think we want to send a notify::locals signal
     * here, because the assumption is that when the debugger's
     * state progresses then an asynchronous update of the
     * locals list is kicked off which should notice the
     * change and issue a signal */

    /* Just remove the invalid variable from the list of local
     * variables and unref it. */
    self->priv->locals = 
        g_list_remove(self->priv->locals, variable_object);
    g_object_unref(variable_object);

}

gulong
gswat_gdb_debugger_send_mi_command(GSwatGdbDebugger* object,
                                   const gchar* command,
                                   GSwatGdbMIRecordCallback result_callback,
                                   void *data)
{
    GSwatGdbDebugger *self;
    GSwatGdbMIHandler *handler;
    gchar *complete_command = NULL;
    gsize written = 0;

    g_return_val_if_fail(GSWAT_IS_GDB_DEBUGGER(object), 0);
    self = GSWAT_GDB_DEBUGGER(object);

    if(!self->priv->gdb_connected)
    {
        return 0;
    }

    handler = g_new0(GSwatGdbMIHandler, 1);
    handler->token = self->priv->gdb_sequence;
    handler->result_callback = result_callback;
    handler->data = data;

    complete_command = g_strdup_printf("%ld%s\n", self->priv->gdb_sequence, command);

    if(g_io_channel_write(self->priv->gdb_in,
                          complete_command,
                          strlen(complete_command),
                          &written) != G_IO_ERROR_NONE)
    {
        g_warning(_("Couldn't send command '%s' to gdb"), command);
        g_free(handler);
        g_free(complete_command);
        return 0;
    }

    self->priv->mi_handlers = g_slist_prepend(self->priv->mi_handlers, handler);

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

    g_io_channel_flush(self->priv->gdb_in, NULL);

    g_message("gdb cli command:%s\n", complete_command);

    g_free(complete_command);

    return;
}


GSwatGdbMIRecord *
gswat_gdb_debugger_get_mi_result_record(GSwatGdbDebugger *object,
                                        gulong token)
{
    GSwatGdbDebugger *self;
    GSwatGdbMIRecord *pending_record;

    g_return_val_if_fail(GSWAT_IS_GDB_DEBUGGER(object), NULL);
    self = GSWAT_GDB_DEBUGGER(object);

    g_io_channel_flush(self->priv->gdb_in, NULL);


    pending_record = find_pending_result_record_for_token(self, token);
    if(pending_record)
    {
        return pending_record;
    }

    while(TRUE)
    {
        read_next_gdb_line(self, self->priv->gdb_out);
        pending_record = find_pending_result_record_for_token(self, token);
        if(pending_record)
        {
            return pending_record;
        }
    }

    g_assert_not_reached();
    return NULL;
}


static GSwatGdbMIRecord *
find_pending_result_record_for_token(GSwatGdbDebugger *self, gulong token)
{
    int n;
    GdbPendingRecord *pending_record;

    for(n=0;
        (pending_record = g_queue_peek_nth(self->priv->gdb_pending, n));
        n++)
    {
        if(pending_record->record_str->str[0] == '^' /* if this is a result record */
           && pending_record->token == token)
        {
            GSwatGdbMIRecord *record;

            record = g_new(GSwatGdbMIRecord, 1);
            record->val = gdbmi_value_parse(pending_record->record_str->str);
            record->type = gdb_mi_get_result_record_type(pending_record->record_str);

            pending_record = g_queue_pop_nth(self->priv->gdb_pending, n);
            free_pending_record(pending_record);

            return record;
        }
    }


    return NULL;
}

void
gswat_gdb_debugger_free_mi_record(GSwatGdbMIRecord *record)
{
    g_return_if_fail(record != NULL);
    
    if(record->val)
    {
        gdbmi_value_free(record->val);
    }
    g_free(record);
}

void
gswat_gdb_debugger_run(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);

    gswat_gdb_debugger_send_mi_command(self,
                                       "-exec-run",
                                       basic_runner_mi_callback,
                                       NULL);
}


static void
basic_runner_mi_callback(GSwatGdbDebugger *self,
                         const GSwatGdbMIRecord *record,
                         void *data)
{
    if(record->type == GSWAT_GDB_MI_REC_TYPE_RESULT_ERROR)
    {
        g_warning("basic_runner_mi_callback: recieved an error");
        return;
    }

    if(record->type != GSWAT_GDB_MI_REC_TYPE_RESULT_RUNNING)
    {
        g_warning("basic_runner_mi_callback: unexpected result type");
        return;
    }

    self->priv->state = GSWAT_DEBUGGABLE_RUNNING;
    g_object_notify(G_OBJECT(self), "state");

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

    if(self->priv->gdb_connected)
    {
        filename = gnome_vfs_get_local_path_from_uri(uri);
        g_assert(filename);

        gdb_command = g_strdup_printf("-break-insert %s:%d", filename, line);
        gswat_gdb_debugger_send_mi_command(self,
                                           gdb_command,
                                           break_insert_mi_callback,
                                           NULL);

        g_free(gdb_command);
        g_free(filename);
    }
}


static void
break_insert_mi_callback(GSwatGdbDebugger *self,
                         const GSwatGdbMIRecord *record,
                         void *data)
{
    GSwatDebuggableBreakpoint *breakpoint;
    GDBMIValue *val;
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

    if(record->type == GSWAT_GDB_MI_REC_TYPE_RESULT_ERROR)
    {
        g_warning("break_insert_mi_callback: error inserting break point");
        return;
    }

    if(record->type != GSWAT_GDB_MI_REC_TYPE_RESULT_DONE)
    {
        g_warning("break_insert_mi_callback: unexpected result type");
        return;
    }

    val = record->val;
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

    return;
}


/* TODO The following requestor functions and control
 * flow operations should probably return a cookie so
 * that users can optionally choose to block until
 * things are complete. Also what amount of error
 * information is usefull to users, without over
 * complicating the API. */

void
gswat_gdb_debugger_request_function_breakpoint(GSwatDebuggable *object,
                                               gchar *function)
{
    GSwatGdbDebugger *self;
    GString *gdb_command = g_string_new("");

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);

    if(self->priv->gdb_connected)
    {
        g_string_printf(gdb_command, "-break-insert %s", function);

        gswat_gdb_debugger_send_mi_command(self,
                                           gdb_command->str,
                                           break_insert_mi_callback,
                                           NULL);

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

    if(self->priv->gdb_connected)
    {
        gdb_command = g_strdup_printf("-break-insert 0x%lx", address);
        gswat_gdb_debugger_send_mi_command(self,
                                           gdb_command,
                                           break_insert_mi_callback,
                                           NULL);
        g_free(gdb_command);

    }
}


void
gswat_gdb_debugger_continue(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);

    gdb_debugger_set_frame(GSWAT_DEBUGGABLE(self), 0);
    
    gswat_gdb_debugger_send_mi_command(self,
                                       "-exec-continue",
                                       basic_runner_mi_callback,
                                       NULL);
}


void
gswat_gdb_debugger_finish(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);

    gdb_debugger_set_frame(GSWAT_DEBUGGABLE(self), 0);

    gswat_gdb_debugger_send_mi_command(self,
                                       "-exec-finish",
                                       basic_runner_mi_callback,
                                       NULL);
}




void
gswat_gdb_debugger_step_into(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);

    gdb_debugger_set_frame(GSWAT_DEBUGGABLE(self), 0);

    gswat_gdb_debugger_send_mi_command(self,
                                       "-exec-step",
                                       basic_runner_mi_callback,
                                       NULL);
}


void
gswat_gdb_debugger_next(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);
    
    gdb_debugger_set_frame(GSWAT_DEBUGGABLE(self), 0);

    gswat_gdb_debugger_send_mi_command(self,
                                       "-exec-next",
                                       basic_runner_mi_callback,
                                       NULL);
}


void
gswat_gdb_debugger_interrupt(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);

    /* For a local process, just use kill(pid, SIGINT) */
    if(self->priv->target_pid != -1)
    {
        if(self->priv->state == GSWAT_DEBUGGABLE_RUNNING)
        {
            kill(self->priv->target_pid, SIGINT);
        }
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
        gswat_gdb_debugger_send_mi_command(self,
                                           "-exec-run",
                                           restart_running_mi_callback,
                                           NULL);
    }
}


static void
restart_running_mi_callback(GSwatGdbDebugger *self,
                            const GSwatGdbMIRecord *record,
                            void *data)
{
    if(record->type == GSWAT_GDB_MI_REC_TYPE_RESULT_ERROR)
    {
        g_warning("restart_running_mi_callback: recieved an error");
        return;
    }

    if(record->type != GSWAT_GDB_MI_REC_TYPE_RESULT_RUNNING)
    {
        g_warning("restart_running_mi_callback: unexpected result type");
        return;
    }

    self->priv->state = GSWAT_DEBUGGABLE_RUNNING;
    g_object_notify(G_OBJECT(self), "state");
}


gchar *
gswat_gdb_debugger_get_source_uri(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;
    
    g_return_val_if_fail(GSWAT_IS_GDB_DEBUGGER(object), NULL);
    self = GSWAT_GDB_DEBUGGER(object);
    
    if(self->priv->current_source_uri)
    {
        return g_strdup(self->priv->current_source_uri);
    }
    else
    {
        return NULL;
    }
}


gint
gswat_gdb_debugger_get_source_line(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;

    g_return_val_if_fail(GSWAT_IS_GDB_DEBUGGER(object), 0);
    self = GSWAT_GDB_DEBUGGER(object);
    
    if(!self->priv->current_source_uri)
    {
        return -1;
    }

    return self->priv->current_line;
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
gswat_gdb_debugger_get_interrupt_count(GSwatGdbDebugger *self)
{
    g_return_val_if_fail(GSWAT_IS_GDB_DEBUGGER(self), 0);

    return self->priv->interrupt_count;
}


GQueue *
gswat_gdb_debugger_get_stack(GSwatDebuggable *object)
{ 
    GSwatGdbDebugger *self;
    GSwatDebuggableFrame *current_frame, *new_frame;
    GList *tmp, *tmp2;
    GQueue *new_stack;
    GSwatDebuggableFrameArgument *current_arg, *new_arg;
    
    g_return_val_if_fail(GSWAT_IS_GDB_DEBUGGER(object), NULL);
    
    self = GSWAT_GDB_DEBUGGER(object);
    
    synchronous_update_stack(self);
    
    new_stack = g_queue_new();

    /* FIXME: the frames should probably be represented as
     * GObjects, contained in a parent stack object, then 
     * this can be reduced to a single ref of the stack
     * object.
     */
    
    /* copy the stack list */
    for(tmp=self->priv->stack->head; tmp!=NULL; tmp=tmp->next)
    {
        current_frame = (GSwatDebuggableFrame *)tmp->data;

        new_frame = g_new(GSwatDebuggableFrame, 1);
        new_frame->level = current_frame->level;
        new_frame->address = current_frame->address;
        new_frame->function = g_strdup(current_frame->function);
        new_frame->source_uri = g_strdup(current_frame->source_uri);
        new_frame->line = current_frame->line;

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

        g_queue_push_tail(new_stack, new_frame);
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
        new_breakpoint->line = current_breakpoint->line;
        new_breakpoint->source_uri = g_strdup(current_breakpoint->source_uri);

        new_breakpoints = g_list_prepend(new_breakpoints, new_breakpoint);
    }

    return new_breakpoints;
}


GList *
gswat_gdb_debugger_get_locals_list(GSwatDebuggable *object)
{
    GSwatGdbDebugger *self;
    
    g_return_val_if_fail(GSWAT_IS_GDB_DEBUGGER(object), NULL);
    self = GSWAT_GDB_DEBUGGER(object);

    synchronous_update_locals_list(self);

    g_list_foreach(self->priv->locals, (GFunc)g_object_ref, NULL);
    return g_list_copy(self->priv->locals);
}


static void
synchronous_update_locals_list(GSwatGdbDebugger *self)
{
    LocalsUpdateMachine *locals_machine;
    gulong token;
    GSwatGdbMIRecord *record;
    
    if( (self->priv->state & GSWAT_DEBUGGABLE_RUNNING)
        || (self->priv->locals_valid == TRUE)
      )
    {
        return;
    }
    
    locals_machine = g_new0(LocalsUpdateMachine, 1);;
    locals_machine->in_use = TRUE;
    
    token = 
        gswat_gdb_debugger_send_mi_command(self,
                                           "-stack-list-arguments 0 0 0",
                                           NULL,
                                           NULL);
    record = gswat_gdb_debugger_get_mi_result_record(self, token);
    async_locals_update_list_args_mi_callback(self, record, locals_machine);
    gswat_gdb_debugger_free_mi_record(record);
    
    token = 
        gswat_gdb_debugger_send_mi_command(self,
                                           "-stack-list-locals 0",
                                           NULL,
                                           NULL);
    record = gswat_gdb_debugger_get_mi_result_record(self, token);
    async_locals_update_list_locals_mi_callback(self, record, locals_machine);
    gswat_gdb_debugger_free_mi_record(record);
    
    g_free(locals_machine);
}


static void
synchronous_update_stack(GSwatGdbDebugger *self)
{
    StackUpdateMachine *stack_machine;
    gulong token;
    GSwatGdbMIRecord *record;

    if( (self->priv->state & GSWAT_DEBUGGABLE_RUNNING)
        || (self->priv->stack_valid == TRUE)
      )
    {
        return;
    }
    
    stack_machine = g_new0(StackUpdateMachine, 1);
    stack_machine->in_use = TRUE;

    gswat_debuggable_stack_free(self->priv->stack);
    self->priv->stack = NULL;
    
    token = 
        gswat_gdb_debugger_send_mi_command(self,
                                           "-stack-list-frames",
                                           NULL,
                                           NULL);
    record = gswat_gdb_debugger_get_mi_result_record(self, token);
    async_stack_update_list_frames_mi_callback(self,
                                               record,
                                               stack_machine);
    gswat_gdb_debugger_free_mi_record(record);

    token =
        gswat_gdb_debugger_send_mi_command(self,
                                           "-stack-list-arguments 1",
                                           NULL,
                                           NULL);
    record = gswat_gdb_debugger_get_mi_result_record(self, token);
    async_stack_update_list_args_mi_callback(self,
                                             record,
                                             stack_machine);
    gswat_gdb_debugger_free_mi_record(record);
    g_free(stack_machine);
}


static guint
gdb_debugger_get_frame(GSwatDebuggable* object)
{
    GSwatGdbDebugger *self;
    
    g_return_val_if_fail(GSWAT_IS_GDB_DEBUGGER(object), 0);
    self = GSWAT_GDB_DEBUGGER(object);

    return self->priv->frame_level;
}


static void
gdb_debugger_set_frame(GSwatDebuggable* object, guint frame_level)
{
    GSwatGdbDebugger *self;
    gchar *gdb_command;
    gulong token;
    GSwatGdbMIRecord *record;
    GSwatDebuggableFrame *frame;

    g_return_if_fail(GSWAT_IS_GDB_DEBUGGER(object));
    self = GSWAT_GDB_DEBUGGER(object);
    
    if(frame_level == self->priv->frame_level)
    {
        return;
    }
    
    if(!self->priv->stack_valid)
    {
        g_warning("%s: It doesn't make sense to request a frame level"
                  " while there is no valid stack!",
                  __FUNCTION__);
        synchronous_update_stack(self);
    }

    if(frame_level == self->priv->stack->length)
    {
        g_warning("%s: Frame level (%d) is out of range!",
                  __FUNCTION__,
                  frame_level);
        return;
    }

    /* Currently this is a synchronous operation since it's
     * not expected to be slow.
     *
     * Making it asynchronous involves having a level_valid
     * boolean, and gdb_debugger_get_frame would need a
     * way of synchronising the operation. */
     
    gdb_command=g_strdup_printf("-stack-select-frame %d", frame_level);
    token = gswat_gdb_debugger_send_mi_command(self,
                                               gdb_command,
                                               NULL,
                                               NULL);
    g_free(gdb_command);
    
    record = gswat_gdb_debugger_get_mi_result_record(self, token);

    if(record->type != GSWAT_GDB_MI_REC_TYPE_RESULT_DONE)
    {
        g_warning("%s: failed to set frame", __FUNCTION__);
        return;
    }
    gswat_gdb_debugger_free_mi_record(record);
    
    self->priv->frame_level = frame_level;
    
    /* Note: The API does not guarantee that the signals for
     * the corresponding local variable or stack updates will
     * be completed before this function finishes. The
     * user may manually request their value before such
     * a signal is issued (not a recommended style though) and
     * they should get the right data since we mark it invalid
     * here, and any manual request will cause blocking until
     * a result comes back from GDB.
     */

    /* invalidate all variable objects..
     * Note: "interrupt_count" is a bad name.*/
    self->priv->interrupt_count++;  

    self->priv->locals_valid = FALSE;
    
    kick_asynchronous_locals_update(self);
    
    frame = g_queue_peek_nth(self->priv->stack,
                             self->priv->frame_level);
    set_source_location(self,
                        frame->source_uri,
                        frame->line);

    g_object_notify(G_OBJECT(self), "frame");
}


