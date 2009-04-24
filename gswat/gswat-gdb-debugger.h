/*
 * GSwat
 *
 * An object oriented debugger abstraction library
 *
 * Copyright  (C) 2006-2009 Robert Bragg <robert@sixbynine.org>
 *
 * GSwat is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or  (at your option)
 * any later version.
 *
 * GSwat is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with GSwat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GSWAT_GDB_DEBUGGER_H
#define GSWAT_GDB_DEBUGGER_H

#include <glib.h>
#include <glib-object.h>

#include "gswat-debuggable.h"
#include "gswat-gdbmi.h"
#include "gswat-session.h"
#include "gswat-gdb-variable-object.h"

G_BEGIN_DECLS

#define GSWAT_GDB_DEBUGGER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSWAT_TYPE_GDB_DEBUGGER, GSwatGdbDebugger))
#define GSWAT_TYPE_GDB_DEBUGGER             (gswat_gdb_debugger_get_type ())
#define GSWAT_GDB_DEBUGGER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GSWAT_TYPE_GDB_DEBUGGER, GSwatGdbDebuggerClass))
#define GSWAT_IS_GDB_DEBUGGER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSWAT_TYPE_GDB_DEBUGGER))
#define GSWAT_IS_GDB_DEBUGGER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GSWAT_TYPE_GDB_DEBUGGER))
#define GSWAT_GDB_DEBUGGER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), GSWAT_TYPE_GDB_DEBUGGER, GSwatGdbDebuggerClass))

#define GSWAT_GDB_DEBUGGER_ERROR (gswat_gdb_debugger_error_quark ())
enum {
    GSWAT_GDB_DEBUGGER_ERROR_SPAWNER_PROCESS_IO,
    GSWAT_GDB_DEBUGGER_ERROR_GDB_IO
};

/* TODO support dynamically determined workarounds */

/* In gdb 6.5 and 6.6 it looks like there might be a bug related
 * to querying child information for a variable with a NULL value.
 *  (Note it seems to work in 6.4) */
#define GSWAT_GDB_DEBUGGER_CAN_INSPECT_NULL_VAROBJS	FALSE
/* #define GSWAT_GDB_DEBUGGER_CAN_INSPECT_NULL_VAROBJS	TRUE */


#if !defined (GSWAT_GDB_DEBUGGER_TYPEDEF)
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
  void  (* gdb_io_error_signal)  (GSwatGdbDebugger *object, gchar *error_message, gpointer data);
  void  (* gdb_io_timeout_signal)  (GSwatGdbDebugger *object, gpointer data);
};

typedef enum
{
  /* result record types */
  GSWAT_GDB_MI_REC_TYPE_RESULT_DONE,
  GSWAT_GDB_MI_REC_TYPE_RESULT_RUNNING,
  GSWAT_GDB_MI_REC_TYPE_RESULT_CONNECTED,
  GSWAT_GDB_MI_REC_TYPE_RESULT_ERROR,
  GSWAT_GDB_MI_REC_TYPE_RESULT_EXIT,

  /* out of band record types */
  GSWAT_GDB_MI_REC_TYPE_OOB_STOPPED,

  GSWAT_GDB_MI_REC_TYPE_UNKNOWN
}GSwatGdbMIRecordType;

typedef struct {
    GSwatGdbMIRecordType type;
    GDBMIValue *val;
}GSwatGdbMIRecord;

typedef void (*GSwatGdbMIRecordCallback) (GSwatGdbDebugger *self,
					  const GSwatGdbMIRecord *record,
					  void *data);

GType gswat_gdb_debugger_get_type (void);
GQuark gswat_gdb_debugger_error_quark (void);

/* add additional methods here */
GSwatGdbDebugger *gswat_gdb_debugger_new (GSwatSession *session);
gulong gswat_gdb_debugger_send_mi_command (GSwatGdbDebugger* object,
					   const gchar* command,
					   GSwatGdbMIRecordCallback result_callback,
					   void *data);
void gswat_gdb_debugger_nop_mi_callback (GSwatGdbDebugger *self,
					 const GSwatGdbMIRecord *record,
					 void *data);
GSwatGdbMIRecord *gswat_gdb_debugger_get_mi_result_record (GSwatGdbDebugger *object,
							   gulong token);
void gswat_gdb_debugger_free_mi_record (GSwatGdbMIRecord *record);
void gswat_gdb_debugger_send_cli_command (GSwatGdbDebugger* self,
					  gchar const* command);

/* internal, but shared with gswat-gdb-variable-object.c */
void _gswat_gdb_debugger_register_variable_object (GSwatGdbDebugger* self,
						   GSwatGdbVariableObject *variable_object);
void _gswat_gdb_debugger_unregister_variable_object (GSwatGdbDebugger* self,
						     GSwatGdbVariableObject *variable_object);

guint gswat_gdb_debugger_get_interrupt_count (GSwatGdbDebugger *self);

void gswat_gdb_debugger_request_address_breakpoint (GSwatDebuggable *object,
						    unsigned long address);
G_END_DECLS

#endif /* GSWAT_GDB_DEBUGGER_H */

