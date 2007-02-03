/*
 * <preamble>
 * gswat - A graphical program variable_object for Gnome
 * Copyright (C) 2006  Robert Bragg
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
 * </preamble>
 */


#ifndef GSWAT_VARIABLE_OBJECT_H
#define GSWAT_VARIABLE_OBJECT_H

#include <glib-object.h>

#include "gswat-debugger.h"


G_BEGIN_DECLS

typedef struct _GSwatVariableObject   GSwatVariableObject;
typedef GObjectClass            GSwatVariableObjectClass;

#define GSWAT_TYPE_VARIABLE_OBJECT         (gswat_variable_object_get_type())
#define GSWAT_VARIABLE_OBJECT(i)           (G_TYPE_CHECK_INSTANCE_CAST((i), GSWAT_TYPE_VARIABLE_OBJECT, GSwatVariableObject))
#define GSWAT_VARIABLE_OBJECT_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST((c), GSWAT_TYPE_VARIABLE_OBJECT, GSwatVariableObjectClass))
#define GSWAT_IS_VARIABLE_OBJECT(i)        (G_TYPE_CHECK_INSTANCE_TYPE((i), GSWAT_TYPE_VARIABLE_OBJECT))
#define GSWAT_IS_VARIABLE_OBJECT_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE((c), GSWAT_TYPE_VARIABLE_OBJECT))
#define GSWAT_VARIABLE_OBJECT_GET_CLASS(i) (G_TYPE_INSTANCE_GET_CLASS((i), GSWAT_TYPE_VARIABLE_OBJECT, GSwatVariableObjectClass))




struct _GSwatVariableObject {
	GObject base_instance;
	struct GSwatVariableObjectPrivate* priv;
};


/* Public methods */

GType gswat_variable_object_get_type(void);

GSwatVariableObject*
gswat_variable_object_new(GSwatDebugger *debugger,
                          const gchar *expression,
                          const gchar *cached_value,
                          int frame);

gchar       *gswat_variable_object_get_expression(GSwatVariableObject* self);
gchar       *gswat_variable_object_get_value(GSwatVariableObject* self);
GList       *gswat_variable_object_get_children(GSwatVariableObject* self);

G_END_DECLS

#endif /* !GSWAT_VARIABLE_OBJECT_H */

