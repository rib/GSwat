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

#ifndef GSWAT_GDB_VARIABLE_OBJECT_H
#define GSWAT_GDB_VARIABLE_OBJECT_H

#include <glib.h>
#include <glib-object.h>

#include "gswat-variable-object.h"

G_BEGIN_DECLS

#define GSWAT_GDB_VARIABLE_OBJECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSWAT_TYPE_GDB_VARIABLE_OBJECT, GSwatGdbVariableObject))
#define GSWAT_TYPE_GDB_VARIABLE_OBJECT            (gswat_gdb_variable_object_get_type ())
#define GSWAT_GDB_VARIABLE_OBJECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSWAT_TYPE_GDB_VARIABLE_OBJECT, GSwatGdbVariableObjectClass))
#define GSWAT_IS_GDB_VARIABLE_OBJECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSWAT_TYPE_GDB_VARIABLE_OBJECT))
#define GSWAT_IS_GDB_VARIABLE_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSWAT_TYPE_GDB_VARIABLE_OBJECT))
#define GSWAT_GDB_VARIABLE_OBJECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSWAT_TYPE_GDB_VARIABLE_OBJECT, GSwatGdbVariableObjectClass))



typedef struct _GSwatGdbVariableObject        GSwatGdbVariableObject;
typedef struct _GSwatGdbVariableObjectClass   GSwatGdbVariableObjectClass;
typedef struct _GSwatGdbVariableObjectPrivate GSwatGdbVariableObjectPrivate;

struct _GSwatGdbVariableObject
{
    GObject parent;

    /*< private > */
    GSwatGdbVariableObjectPrivate *priv;
};

struct _GSwatGdbVariableObjectClass
{
    GObjectClass parent_class;
};

GType gswat_gdb_variable_object_get_type (void);

#if !defined (GSWAT_GDB_DEBUGGER_TYPEDEF)
#define GSWAT_GDB_DEBUGGER_TYPEDEF
typedef struct _GSwatGdbDebugger              GSwatGdbDebugger;
#endif

GSwatGdbVariableObject *gswat_gdb_variable_object_new (GSwatGdbDebugger *debugger,
                                                      const gchar *expression,
                                                      const gchar *cached_value,
                                                      int frame);
char *gswat_gdb_variable_object_get_name (GSwatGdbVariableObject *self);

/* These should probably only be used by gswat-gdb-debugger.c */
void gswat_gdb_variable_object_async_update_all (GSwatGdbDebugger *self);
void gswat_gdb_variable_object_cleanup (GSwatGdbDebugger *gdb_debugger);

G_END_DECLS

#endif /* GSWAT_GDB_VARIABLE_OBJECT_H */

