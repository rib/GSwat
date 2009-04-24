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

#include "gswat-utils.h"

#include "gswat-variable-object.h"

static void gswat_variable_object_base_init (GSwatVariableObjectIface *interface);
/* static void gswat_variable_object_base_finalize (GSwatVariableObjectIface *interface); */

enum {
    SIG_UPDATED,
    LAST_SIGNAL
};

static guint gswat_variable_object_signals[LAST_SIGNAL] = { 0 };
static guint gswat_variable_object_base_init_count = 0;


GType
gswat_variable_object_get_type (void)
{
    static GType self_type = 0;

    if  (!self_type) {
        static const GTypeInfo interface_info = {
            sizeof  (GSwatVariableObjectIface),
             (GBaseInitFunc)gswat_variable_object_base_init,
             (GBaseFinalizeFunc)NULL,/* gswat_variable_object_base_finalize */
        };

        self_type = g_type_register_static (G_TYPE_INTERFACE,
                                           "GSwatVariableObject",
                                           &interface_info, 0);

        g_type_interface_add_prerequisite (self_type, G_TYPE_OBJECT);
    }

    return self_type;
}

static void
gswat_variable_object_base_init (GSwatVariableObjectIface *interface)
{
    gswat_variable_object_base_init_count++;
    GParamSpec *new_param;

    if (gswat_variable_object_base_init_count == 1)
      {
	/* FIXME this should be removed in-place of
	 * properties
	 */
	interface->updated = NULL;
	gswat_variable_object_signals[SIG_UPDATED] =
	  g_signal_new ("updated",
			G_TYPE_FROM_INTERFACE (interface),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (GSwatVariableObjectIface, updated),
			NULL, /* accumulator */
			NULL, /* accumulator data */
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, /* return type */
			0 /* number of parameters */
			/* vararg, list of param types */
	  );

	new_param = g_param_spec_boolean ("valid",
					  "Valid",
					  "Does this object still exist "
					  "according to the backend "
					  "debuggable",
					  TRUE,
					  GSWAT_PARAM_READABLE);
	g_object_interface_install_property (interface, new_param);


	new_param = g_param_spec_string ("expression",
					 "Expression",
					 "The underlying expression whos value"
					 " this object tracks",
					 NULL,
					 GSWAT_PARAM_READABLE);
	g_object_interface_install_property (interface, new_param);


	new_param = g_param_spec_string ("value",
					 "Value",
					 "The value of this objects "
					 "expression",
					 NULL,
					 GSWAT_PARAM_READABLE);
	g_object_interface_install_property (interface, new_param);

	new_param = g_param_spec_uint ("child-count",
				       "Child Count",
				       "The number of children this "
				       "varaible object has",
				       0,
				       G_MAXUINT,
				       0,
				       GSWAT_PARAM_READABLE);
	g_object_interface_install_property (interface, new_param);

	new_param = g_param_spec_pointer ("children",
					  "Children",
					  "A List of this variable objects "
					  "children",
					  GSWAT_PARAM_READABLE);
	g_object_interface_install_property (interface, new_param);
      }
}

gchar *
gswat_variable_object_get_expression (GSwatVariableObject* object)
{
  GSwatVariableObjectIface *variable_object;
  gchar *ret;

  g_return_val_if_fail (GSWAT_IS_VARIABLE_OBJECT (object), NULL);
  variable_object = GSWAT_VARIABLE_OBJECT_GET_IFACE (object);

  g_object_ref (object);
  ret = variable_object->get_expression (object);
  g_object_unref (object);

  return ret;
}

gchar *
gswat_variable_object_get_value (GSwatVariableObject *object, GError **error)
{
  GSwatVariableObjectIface *variable_object;
  gchar *ret;

  g_return_val_if_fail (GSWAT_IS_VARIABLE_OBJECT (object), NULL);
  variable_object = GSWAT_VARIABLE_OBJECT_GET_IFACE (object);

  g_object_ref (object);
  ret = variable_object->get_value (object, error);
  g_object_unref (object);

  return ret;
}

guint
gswat_variable_object_get_child_count (GSwatVariableObject *object)
{
  GSwatVariableObjectIface *variable_object;
  guint ret;

  g_return_val_if_fail (GSWAT_IS_VARIABLE_OBJECT (object), 0);
  variable_object = GSWAT_VARIABLE_OBJECT_GET_IFACE (object);

  g_object_ref (object);
  ret = variable_object->get_child_count (object);
  g_object_unref (object);

  return ret;
}

GList *
gswat_variable_object_get_children (GSwatVariableObject* object)
{
  GSwatVariableObjectIface *variable_object;
  GList *ret;

  g_return_val_if_fail (GSWAT_IS_VARIABLE_OBJECT (object), NULL);
  variable_object = GSWAT_VARIABLE_OBJECT_GET_IFACE (object);

  g_object_ref (object);
  ret = variable_object->get_children (object);
  g_object_unref (object);

  return ret;
}

GQuark
gswat_variable_object_error_quark (void)
{
  static GQuark q = 0;
  if (q==0)
    {
      q = g_quark_from_static_string ("gswat-variable-object-error");
    }
  return q;
}

