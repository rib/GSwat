#ifndef GSWAT_GDB_VARIABLE_OBJECT_H
#define GSWAT_GDB_VARIABLE_OBJECT_H

#include <glib.h>
#include <glib-object.h>

#include "gswat-variable-object.h"

G_BEGIN_DECLS

#define GSWAT_GDB_VARIABLE_OBJECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSWAT_TYPE_GDB_VARIABLE_OBJECT, GSwatGdbVariableObject))
#define GSWAT_TYPE_GDB_VARIABLE_OBJECT            (gswat_gdb_variable_object_get_type())
#define GSWAT_GDB_VARIABLE_OBJECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSWAT_TYPE_GDB_VARIABLE_OBJECT, GSwatGdbVariableObjectClass))
#define GSWAT_IS_GDB_VARIABLE_OBJECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSWAT_TYPE_GDB_VARIABLE_OBJECT))
#define GSWAT_IS_GDB_VARIABLE_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSWAT_TYPE_GDB_VARIABLE_OBJECT))
#define GSWAT_GDB_VARIABLE_OBJECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSWAT_TYPE_GDB_VARIABLE_OBJECT, GSwatGdbVariableObjectClass))



typedef struct _GSwatGdbVariableObject        GSwatGdbVariableObject;
typedef struct _GSwatGdbVariableObjectClass   GSwatGdbVariableObjectClass;
typedef struct _GSwatGdbVariableObjectPrivate GSwatGdbVariableObjectPrivate;

struct _GSwatGdbVariableObject
{
    /* add your parent type here */
    GObject parent;

    /* add pointers to new members here */
    
	/*< private > */
	GSwatGdbVariableObjectPrivate *priv;
};

struct _GSwatGdbVariableObjectClass
{
    /* add your parent class here */
    GObjectClass parent_class;

    /* add signals here */
    //void (* signal) (GSwatGdbVariableObject *object);
};

GType gswat_gdb_variable_object_get_type(void);

/* add additional methods here */

#if !defined(GSWAT_GDB_DEBUGGER_TYPEDEF)
#define GSWAT_GDB_DEBUGGER_TYPEDEF
typedef struct _GSwatGdbDebugger              GSwatGdbDebugger;
#endif

GSwatGdbVariableObject *gswat_gdb_variable_object_new(GSwatGdbDebugger *debugger,
                                                      const gchar *expression,
                                                      const gchar *cached_value,
                                                      int frame);
char *gswat_gdb_variable_object_get_name(GSwatGdbVariableObject *self);

/* internal, but shared with gswat-gdb-debugger.c */
void _gswat_gdb_variable_object_set_cache_value(GSwatGdbVariableObject *self,
                                                const char *value);

G_END_DECLS

#endif /* GSWAT_GDB_VARIABLE_OBJECT_H */

