/*
 * GSwat
 *
 * An object oriented debugger abstraction library
 *
 * Copyright (C) 2006-2009 Robert Bragg <robert@sixbynine.org>
 *
 * GSwat is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
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

#ifndef GSWAT_VARIABLE_OBJECT_H
#define GSWAT_VARIABLE_OBJECT_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GSWAT_TYPE_VARIABLE_OBJECT           (gswat_variable_object_get_type ())
#define GSWAT_VARIABLE_OBJECT(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSWAT_TYPE_VARIABLE_OBJECT, GSwatVariableObject))
#define GSWAT_IS_VARIABLE_OBJECT(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSWAT_TYPE_VARIABLE_OBJECT))
#define GSWAT_VARIABLE_OBJECT_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GSWAT_TYPE_VARIABLE_OBJECT, GSwatVariableObjectIface))

#define GSWAT_VARIABLE_OBJECT_ERROR (gswat_variable_object_error_quark ())
enum {
    GSWAT_VARIABLE_OBJECT_ERROR_GET_VALUE_FAILED
};

enum {
    GSWAT_VARIABLE_OBJECT_CURRENT_NOW_FRAME = -1,
    GSWAT_VARIABLE_OBJECT_ANY_FRAME = -2
};

typedef struct _GSwatVariableObjectIface GSwatVariableObjectIface;
typedef void GSwatVariableObject; /* dummy typedef */

struct _GSwatVariableObjectIface
{
  GTypeInterface g_iface;

  /* signals: */
  void (* updated)(GSwatVariableObject *object);

  /* VTable: */
  gchar *(*get_expression)(GSwatVariableObject* object);
  gchar *(*get_value)(GSwatVariableObject *object, GError **error);
  guint  (*get_child_count)(GSwatVariableObject *object);
  GList *(*get_children)(GSwatVariableObject* object);
};

GType gswat_variable_object_get_type (void);
GQuark gswat_variable_object_error_quark (void);

/* Interface functions */
gchar *gswat_variable_object_get_expression (GSwatVariableObject* self);
gchar *gswat_variable_object_get_value (GSwatVariableObject *self,
					GError **error);
guint  gswat_variable_object_get_child_count (GSwatVariableObject *self);
GList *gswat_variable_object_get_children (GSwatVariableObject* self);

G_END_DECLS

#endif /* GSWAT_VARIABLE_OBJECT_H */

