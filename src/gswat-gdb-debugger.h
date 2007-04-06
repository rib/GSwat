#ifndef GSWAT_GDB_DEBUGGER_H
#define GSWAT_GDB_DEBUGGER_H

#include <glib.h>
#include <glib-object.h>

#include "gswat-debuggable.h"
#include "gswat-gdbmi.h"
#include "gswat-session.h"
#include "gswat-gdb-variable-object.h"

G_BEGIN_DECLS

#define GSWAT_GDB_DEBUGGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSWAT_TYPE_GDB_DEBUGGER, GSwatGdbDebugger))
#define GSWAT_TYPE_GDB_DEBUGGER            (gswat_gdb_debugger_get_type())
#define GSWAT_GDB_DEBUGGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSWAT_TYPE_GDB_DEBUGGER, GSwatGdbDebuggerClass))
#define GSWAT_IS_GDB_DEBUGGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSWAT_TYPE_GDB_DEBUGGER))
#define GSWAT_IS_GDB_DEBUGGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSWAT_TYPE_GDB_DEBUGGER))
#define GSWAT_GDB_DEBUGGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSWAT_TYPE_GDB_DEBUGGER, GSwatGdbDebuggerClass))

#if !defined(GSWAT_GDB_DEBUGGER_TYPEDEF)
#define GSWAT_GDB_DEBUGGER_TYPEDEF
typedef struct _GSwatGdbDebugger        GSwatGdbDebugger;
#endif
typedef struct _GSwatGdbDebuggerClass   GSwatGdbDebuggerClass;
typedef struct _GSwatGdbDebuggerPrivate GSwatGdbDebuggerPrivate;

struct _GSwatGdbDebugger
{
    /* add your parent type here */
    GObject parent;

    /* add pointers to new members here */
    
	/*< private > */
	GSwatGdbDebuggerPrivate *priv;
};

struct _GSwatGdbDebuggerClass
{
    /* add your parent class here */
    GObjectClass parent_class;

    /* add signals here */
    //void (* signal) (GSwatGdbDebugger *object);
};

/* FIXME - the namespacing here isn't consistent */
typedef void (*GDBMIDoneCallback)(GSwatGdbDebugger *self,
                                  gulong token,
                                  const GDBMIValue *val,
                                  GString *command,
                                  gpointer data);

typedef struct {
    gulong token;
    GDBMIDoneCallback callback;
    gpointer data;
}GDBMIDoneHandler;

GType gswat_gdb_debugger_get_type(void);

/* add additional methods here */
GSwatGdbDebugger *gswat_gdb_debugger_new(GSwatSession *session);
void gswat_gdb_debugger_target_connect(GSwatDebuggable* object);
void gswat_gdb_debugger_target_disconnect(GSwatDebuggable* object);
gulong gswat_gdb_debugger_send_mi_command(GSwatGdbDebugger* self,
                                          gchar const* command);
void gdb_debugger_mi_done_connect(GSwatDebuggable *object,
                                  gulong token,
                                  GDBMIDoneCallback callback,
                                  gpointer *data);
GDBMIValue *gswat_gdb_debugger_get_mi_value(GSwatGdbDebugger *self,
                                            gulong token,
                                            GDBMIValue **error);
void gswat_gdb_debugger_mi_done_connect(GSwatGdbDebugger *self,
                                        gulong token,
                                        GDBMIDoneCallback callback,
                                        gpointer *data);
void gswat_gdb_debugger_send_cli_command(GSwatGdbDebugger* self,
                                         gchar const* command);
void gswat_gdb_debugger_request_line_breakpoint(GSwatDebuggable* object,
                                                gchar *uri,
                                                guint line);
void gswat_gdb_debugger_request_function_breakpoint(GSwatDebuggable* object,
                                                    gchar *symbol);
void gswat_gdb_debugger_continue(GSwatDebuggable* object);
void gswat_gdb_debugger_finish(GSwatDebuggable* object);
void gswat_gdb_debugger_step_into(GSwatDebuggable* object);
void gswat_gdb_debugger_next(GSwatDebuggable* object);
void gswat_gdb_debugger_interrupt(GSwatDebuggable* object);
void gswat_gdb_debugger_restart(GSwatDebuggable* object);
gchar *gswat_gdb_debugger_get_source_uri(GSwatDebuggable* object);
gulong gswat_gdb_debugger_get_source_line(GSwatDebuggable* object);
guint gswat_gdb_debugger_get_state(GSwatDebuggable* object);
guint gswat_gdb_debugger_get_state_stamp(GSwatDebuggable* object);
GList *gswat_gdb_debugger_get_stack(GSwatDebuggable* object);
GList *gswat_gdb_debugger_get_breakpoints(GSwatDebuggable* object);
GList *gswat_gdb_debugger_get_locals_list(GSwatDebuggable* object);

/* internal, but shared with gswat-gdb-variable-object.c */
void _gswat_gdb_debugger_register_variable_object(GSwatGdbDebugger* self,
                                                  GSwatGdbVariableObject *variable_object);
void _gswat_gdb_debugger_unregister_variable_object(GSwatGdbDebugger* self,
                                                    GSwatGdbVariableObject *variable_object);


G_END_DECLS

#endif /* GSWAT_GDB_DEBUGGER_H */
