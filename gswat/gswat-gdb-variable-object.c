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
 *
 * For an overview of variable objects see:
 *   http://sourceware.org/gdb/onlinedocs/gdb_25.html#SEC410
 * For specific details see:
 *   gdb/mi/mi-cmd-var.c and gdb/varobj.c
 */
#include <config.h>

#include <stdlib.h>
#include <string.h>

#include <glib/gi18n-lib.h>

#include "gswat-utils.h"
#include "gswat-gdb-debugger.h"
#include "gswat-gdb-variable-object.h"

static void gswat_gdb_variable_object_class_init (GSwatGdbVariableObjectClass *klass);
static void gswat_gdb_variable_object_get_property (GObject *object,
						    guint id,
						    GValue *value,
						    GParamSpec *pspec);
static void gswat_gdb_variable_object_set_property (GObject *object,
						    guint property_id,
						    const GValue *value,
						    GParamSpec *pspec);
static void
gswat_gdb_variable_object_variable_object_interface_init (gpointer interface,
							  gpointer data);
static void gswat_gdb_variable_object_init (GSwatGdbVariableObject *self);
static void gswat_gdb_variable_object_finalize (GObject *self);

static gboolean create_gdb_variable_object (GSwatGdbVariableObject *self);
static GSwatGdbVariableObject *wrap_child_gdb_variable_object (GSwatGdbDebugger *debugger,
							       GSwatGdbVariableObject *parent,
							       const gchar *gdb_name,
							       const gchar *expression,
							       const gchar *cache_value,
							       gint frame,
							       gint child_count);
static void delete_gdb_variable_object (GSwatGdbVariableObject *self);
static void delete_gdb_variable_object_1 (GSwatGdbVariableObject *self,
					  gboolean the_root);
static void register_variable_object (GSwatGdbDebugger *gdb_debuggable,
				      GSwatGdbVariableObject *variable_object);
static gchar *
gswat_gdb_variable_object_get_expression (GSwatVariableObject *self);
static gchar *
gswat_gdb_variable_object_get_value (GSwatVariableObject *self,
				     GError **error);
static gboolean validate_variable_object (GSwatGdbVariableObject *self);
static gchar *evaluate_gdb_variable_object_expression (GSwatGdbVariableObject *self,
						       GError **error);
static guint
gswat_gdb_variable_object_get_child_count (GSwatVariableObject *self);
static gint
count_gdb_variable_object_children (GSwatGdbVariableObject *self);
static GList *
gswat_gdb_variable_object_get_children (GSwatVariableObject *self);
static void synchronous_update_all (GSwatGdbDebugger *gdb_debugger);
static void
update_variable_objects_mi_callback (GSwatGdbDebugger *gdb_debugger,
				     const GSwatGdbMIRecord *record,
				     void *data);
static void handle_changelist (GSwatGdbDebugger *gdb_debugger,
			       const GSwatGdbMIRecord *record);


#define GSWAT_GDB_VARIABLE_OBJECT_GET_PRIVATE(object) \
  (G_TYPE_INSTANCE_GET_PRIVATE((object), \
			       GSWAT_TYPE_GDB_VARIABLE_OBJECT, \
			       GSwatGdbVariableObjectPrivate))

#if 0
enum {
    SIG_UPDATED,
    LAST_SIGNAL
};
#endif

enum {
    PROP_0,
    PROP_VALID,
    PROP_EXPRESSION,
    PROP_VALUE,
    PROP_CHILD_COUNT,
    PROP_CHILDREN
};

struct _GSwatGdbVariableObjectPrivate
{
  GSwatGdbDebugger        *debugger;

  /* This is a snapshot of the GdbDebuggers
   * interrupt counter. When it doesn't match
   * the debuggers count, then the variable
   * objects cached state must be updated */
  guint                   gdb_interrupt_count;

  /* cache of next value to return */
  gchar                   *cached_value;

  /* When a variable object becomes invalid
   * it should be unref'd by the owner at the
   * first opertunity since it is no longer
   * being managed by the backend debugger
   */
  gboolean                valid;

  gchar                   *expression;

  /* The expression for a variable object can
   * either be evaluated:
   * - In the frame that was current at the point
   *   of variable-object creation.
   *    (GSWAT_VARIABLE_OBJECT_CURRENT_NOW_FRAME)
   * - In the frame that is current at the point
   *   you request the value.
   *    (GSWAT_VARIABLE_OBJECT_ANY_FRAME)
   * - In any other single specific frame chosen
   *   at the point of variable-object creation.
   *
   * FIXME - we currently don't support the last
   * option.
   */
  int                     frame;

  GSwatGdbVariableObject  *parent;

  guint                   child_count;
  /* This list of children is "consistent" if it
   * represents the _full_ list of children
   * as would be retuned from -var-list-children.
   * It can become inconsistent if some child
   * variable objects are un-ref'd and the gdb
   * side object deleted.
   */
  gboolean               children_consistent;
  /* This is a list of all variable objects that
   * have corresponding gdb side objects that
   * are child nodes of this object.
   * Note: as seen by 'children_consistent' above
   * it is possible to delete gdb side child
   * objects; at which point this doesn't
   * represent the _full_ list of children got
   * via -var-list-children.
   */
  GList                   *children;

  /* The gdb side name for this variable object */
  gchar                   *gdb_name;

};

static GObjectClass *parent_class = NULL;
/* static guint gswat_gdb_variable_object_signals[LAST_SIGNAL] = { 0 }; */

static int global_variable_object_index = 0;


GType
gswat_gdb_variable_object_get_type (void)
{
  static GType self_type = 0;

  if  (!self_type)
    {
      static const GTypeInfo gdb_variable_object_info =
	{
	  sizeof (GSwatGdbVariableObjectClass),
	  NULL, /* base class initializer */
	  NULL, /* base class finalizer */
	  (GClassInitFunc)gswat_gdb_variable_object_class_init,
	  NULL, /* class finalizer */
	  NULL, /* class data */
	  sizeof (GSwatGdbVariableObject), /* instance structure size */
	  0, /* preallocated instances */
	  (GInstanceInitFunc)gswat_gdb_variable_object_init,
	  NULL /* function table */
	};

      self_type = g_type_register_static (G_TYPE_OBJECT,
					  "GSwatGdbVariableObject",
					  &gdb_variable_object_info,
					  0 /* flags */
      );

      static const GInterfaceInfo variable_object_info =
	{
	  (GInterfaceInitFunc)
	    gswat_gdb_variable_object_variable_object_interface_init,
	  (GInterfaceFinalizeFunc)NULL,
	  NULL /* interface data */
	};

      if  (self_type != G_TYPE_INVALID) {
	  g_type_add_interface_static (self_type,
				       GSWAT_TYPE_VARIABLE_OBJECT,
				       &variable_object_info);
      }

    }

  return self_type;
}

static void
gswat_gdb_variable_object_class_init (GSwatGdbVariableObjectClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  /* GParamSpec *new_param; */

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = gswat_gdb_variable_object_finalize;

  gobject_class->get_property = gswat_gdb_variable_object_get_property;
  gobject_class->set_property = gswat_gdb_variable_object_set_property;

  g_object_class_override_property (gobject_class,
				    PROP_VALID,
				    "valid");
  g_object_class_override_property (gobject_class,
				    PROP_EXPRESSION,
				    "expression");
  g_object_class_override_property (gobject_class,
				    PROP_VALUE,
				    "value");
  g_object_class_override_property (gobject_class,
				    PROP_CHILD_COUNT,
				    "child-count");
  g_object_class_override_property (gobject_class,
				    PROP_CHILDREN,
				    "children");

  g_type_class_add_private (klass, sizeof (GSwatGdbVariableObjectPrivate));
}

static void
gswat_gdb_variable_object_get_property (GObject *object,
					guint id,
					GValue *value,
					GParamSpec *pspec)
{
  GSwatGdbVariableObject* self = GSWAT_GDB_VARIABLE_OBJECT (object);
  gchar *value_str;
  guint child_count;
  void *children;

  switch (id) {
    case PROP_VALID:
      g_value_set_boolean (value, self->priv->valid);
    case PROP_EXPRESSION:
      g_value_set_string (value, self->priv->expression);
      break;
    case PROP_VALUE:
      value_str = gswat_gdb_variable_object_get_value (self, NULL);
      g_value_set_string_take_ownership (value, value_str);
      break;
    case PROP_CHILD_COUNT:
      child_count = gswat_gdb_variable_object_get_child_count (self);
      g_value_set_uint (value, child_count);
      break;
    case PROP_CHILDREN:
      children = gswat_gdb_variable_object_get_children (self);
      g_value_set_pointer (value, children);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, id, pspec);
      break;
  }
}

static void
gswat_gdb_variable_object_set_property (GObject *object,
					guint property_id,
					const GValue *value,
					GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      g_warning ("gswat_gdb_variable_object_set_property on unknown property");
      return;
    }
}

static void
gswat_gdb_variable_object_variable_object_interface_init (gpointer interface,
							  gpointer data)
{
  GSwatVariableObjectIface *variable_object = interface;
  g_assert (G_TYPE_FROM_INTERFACE (variable_object) == GSWAT_TYPE_VARIABLE_OBJECT);

  variable_object->get_expression = gswat_gdb_variable_object_get_expression;
  variable_object->get_value = gswat_gdb_variable_object_get_value;
  variable_object->get_child_count = gswat_gdb_variable_object_get_child_count;
  variable_object->get_children = gswat_gdb_variable_object_get_children;
}

static void
gswat_gdb_variable_object_init (GSwatGdbVariableObject *self)
{
  self->priv = GSWAT_GDB_VARIABLE_OBJECT_GET_PRIVATE (self);

}

GSwatGdbVariableObject *
gswat_gdb_variable_object_new (GSwatGdbDebugger *debugger,
			       const gchar *expression,
			       const gchar *cached_value,
			       gint frame)
{
  GSwatGdbVariableObject *variable_object;

  variable_object = g_object_new (GSWAT_TYPE_GDB_VARIABLE_OBJECT, NULL);
  variable_object->priv->debugger = debugger;
  variable_object->priv->parent = NULL;
  variable_object->priv->expression = g_strdup (expression);
  if (cached_value)
    {
      variable_object->priv->cached_value = g_strdup (cached_value);
    }
  else
    {
      variable_object->priv->cached_value = NULL;
    }
  variable_object->priv->frame = frame;
  variable_object->priv->gdb_interrupt_count =
    gswat_gdb_debugger_get_interrupt_count (debugger);

  if (!create_gdb_variable_object (variable_object))
    {
      g_object_unref (variable_object);
    }

  register_variable_object (debugger, variable_object);

  variable_object->priv->valid = TRUE;

  return variable_object;
}

void
gswat_gdb_variable_object_finalize (GObject *object)
{
  GSwatGdbVariableObject *self = GSWAT_GDB_VARIABLE_OBJECT (object);

  /* delete the variable object from gdb's point
   * of view */
  delete_gdb_variable_object (self);

  g_free (self->priv->expression);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
create_gdb_variable_object (GSwatGdbVariableObject *self)
{
  gchar *command;
  gulong token;
  GSwatGdbMIRecord *result;
  const GDBMIValue *numchild_val;


  if (self->priv->frame == GSWAT_VARIABLE_OBJECT_ANY_FRAME)
    {
      command = g_strdup_printf ("-var-create v%d @ %s",
				 global_variable_object_index,
				 self->priv->expression);
    }
  else if (self->priv->frame == GSWAT_VARIABLE_OBJECT_CURRENT_NOW_FRAME)
    {
      command = g_strdup_printf ("-var-create v%d * %s",
				 global_variable_object_index,
				 self->priv->expression);
    }
  else
    {
      g_warning ("create_gdb_variable_object: doesn't currently "
		 "support arbitrary frame choice");
    }
  token = gswat_gdb_debugger_send_mi_command (self->priv->debugger,
					      command,
					      NULL,
					      NULL);
  g_free (command);

  /* FIXME - make sure we can cope with junk expressions */
  result=gswat_gdb_debugger_get_mi_result_record (self->priv->debugger, token);
  if (!result)
    {
      /* An IO error has occurred */
      return FALSE;
    }

  numchild_val=gdbmi_value_hash_lookup (result->val, "numchild");
  if (numchild_val)
    {
      const char *child_count_str;
      child_count_str = gdbmi_value_literal_get (numchild_val);
      self->priv->child_count =  (gint)strtoul (child_count_str, NULL, 10);
    }
  else
    {
      /* Mark the child_count as "unknown" */
      self->priv->child_count = -1;
    }

  gswat_gdb_debugger_free_mi_record (result);

  self->priv->gdb_name = g_strdup_printf ("v%d",
					  global_variable_object_index);

  global_variable_object_index++;

  return TRUE;
}

static GSwatGdbVariableObject *
wrap_child_gdb_variable_object (GSwatGdbDebugger *debugger,
				GSwatGdbVariableObject *parent,
				const gchar *gdb_name,
				const gchar *expression,
				const gchar *cache_value,
				gint frame,
				gint child_count)
{
  GSwatGdbVariableObject *variable_object;
  GList *siblings;
  GList *tmp;

  variable_object = g_object_new (GSWAT_TYPE_GDB_VARIABLE_OBJECT, NULL);
  variable_object->priv->debugger = debugger;
  variable_object->priv->parent = parent;
  variable_object->priv->expression = g_strdup (expression);
  if (cache_value)
    {
      variable_object->priv->cached_value = g_strdup (cache_value);
    }
  else
    {
      variable_object->priv->cached_value = NULL;
    }
  variable_object->priv->frame = frame;
  variable_object->priv->gdb_name = g_strdup (gdb_name);
  variable_object->priv->child_count = child_count;
  variable_object->priv->gdb_interrupt_count =
    gswat_gdb_debugger_get_interrupt_count (debugger);

  siblings = parent->priv->children;
  for (tmp=siblings; tmp!=NULL; tmp=tmp->next)
    {
      GSwatGdbVariableObject *sibling = tmp->data;

      if (strcmp (sibling->priv->gdb_name,
		  variable_object->priv->gdb_name) == 0)
	{
	  g_warning ("wrap_child_gdb_variable_object: found a conflicting"
		     " sibling object");
	}
    }

  siblings = g_list_prepend (siblings, variable_object);
  parent->priv->children = siblings;

  register_variable_object (debugger, variable_object);

  variable_object->priv->valid = TRUE;

  return variable_object;
}

static void
delete_gdb_variable_object (GSwatGdbVariableObject *self)
{
  delete_gdb_variable_object_1 (self, TRUE);
}

static void
delete_gdb_variable_object_1 (GSwatGdbVariableObject *self,
			      gboolean the_root)
{
  gchar *command;
  GList *tmp;
  GList *all_variables;

  if (!self->priv->gdb_name)
    {
      return;
    }

  for (tmp=self->priv->children; tmp!=NULL; tmp=tmp->next)
    {
      GSwatGdbVariableObject *child;
      child = tmp->data;
      delete_gdb_variable_object_1 (child, FALSE);
    }
  g_list_free (self->priv->children);
  self->priv->children=NULL;

  self->priv->child_count = -1;
  self->priv->children_consistent = FALSE;

  if (the_root)
    {
      /* gdb will automatically delete children so
       * we don't need to send this seperatly for
       * each one. */
      command = g_strdup_printf ("-var-delete %s",
				 self->priv->gdb_name);
      gswat_gdb_debugger_send_mi_command (self->priv->debugger,
					  command,
					  gswat_gdb_debugger_nop_mi_callback,
					  NULL);
      g_free (command);

      /* 'the_root' doesn't imply this variable object
       * has no parent; just that it is the root of
       * what is being deleted. If we are deleting some
       * child then remove the parents reference to it,
       * and mark the parents list of children as
       * in-consistent */
      if (self->priv->parent && self->priv->parent->priv->children)
	{
	  GList *siblings;

	  siblings = self->priv->parent->priv->children;
	  siblings = g_list_remove (siblings, self);
	  self->priv->parent->priv->children = siblings;
	  self->priv->parent->priv->children_consistent = FALSE;
	}
    }

  g_free (self->priv->cached_value);
  self->priv->cached_value=NULL;

  all_variables = g_object_get_data (G_OBJECT (self->priv->debugger),
				     "all-gswat-gdb-variable-objects");
  all_variables = g_list_remove (all_variables, self);
  g_object_set_data (G_OBJECT (self->priv->debugger),
		     "all-gswat-gdb-variable-objects",
		     all_variables);

  g_free (self->priv->gdb_name);
  self->priv->gdb_name=NULL;

  self->priv->valid = FALSE;
  g_object_notify (G_OBJECT (self), "valid");
}

static void
register_variable_object (GSwatGdbDebugger *gdb_debuggable,
			  GSwatGdbVariableObject *variable_object)
{
  GList *all_variables;

  all_variables = g_object_get_data (G_OBJECT (gdb_debuggable),
				     "all-gswat-gdb-variable-objects");

  all_variables = g_list_prepend (all_variables, variable_object);

  g_object_set_data (G_OBJECT (gdb_debuggable),
		     "all-gswat-gdb-variable-objects",
		     all_variables);
}

char *
gswat_gdb_variable_object_get_name (GSwatGdbVariableObject *self)
{
  if (!self->priv->valid)
    {
      g_warning ("gswat_gdb_variable_object_get_name: invalid request");
      return NULL;
    }
  else
    {
      return g_strdup (self->priv->gdb_name);
    }
}

static gchar *
gswat_gdb_variable_object_get_expression (GSwatVariableObject *object)
{
  GSwatGdbVariableObject *self;

  g_return_val_if_fail (GSWAT_IS_GDB_VARIABLE_OBJECT (object), NULL);
  self = GSWAT_GDB_VARIABLE_OBJECT (object);

  return g_strdup (self->priv->expression);
}

static gchar *
gswat_gdb_variable_object_get_value (GSwatVariableObject *object,
				     GError **error)
{
  GSwatGdbVariableObject *self;

  g_return_val_if_fail (GSWAT_IS_GDB_VARIABLE_OBJECT (object), NULL);
  self = GSWAT_GDB_VARIABLE_OBJECT (object);

  if (!validate_variable_object (self))
    {
      return NULL;
    }

  /* Note, that after validate_variable_object, the object
   * may still have a NULL cached_value.
   */

  if (self->priv->cached_value)
    {
      return g_strdup (self->priv->cached_value);
    }

  self->priv->cached_value =
    evaluate_gdb_variable_object_expression (self, error);

  return g_strdup (self->priv->cached_value);
}

static gboolean
validate_variable_object (GSwatGdbVariableObject *self)
{
  GSwatGdbDebugger *gdb_debugger;
  guint interrupt_count;

  if (!self->priv->valid)
    {
      return FALSE;
    }

  g_object_ref (self);

  gdb_debugger = self->priv->debugger;
  interrupt_count
    = gswat_gdb_debugger_get_interrupt_count (gdb_debugger);
  if (self->priv->gdb_interrupt_count < interrupt_count)
    {
      synchronous_update_all (gdb_debugger);

      if (!self->priv->valid)
	{
	  g_object_unref (self);
	  return FALSE;
	}
    }

  g_assert (self->priv->gdb_interrupt_count == interrupt_count);

  g_object_unref (self);
  return TRUE;
}

static gchar *
evaluate_gdb_variable_object_expression (GSwatGdbVariableObject *self,
					 GError **error)
{
  gchar *command;
  gulong token;
  GSwatGdbMIRecord *result;
  const GDBMIValue *value;
  gchar *value_string;

  command = g_strdup_printf ("-var-evaluate-expression %s",
			     self->priv->gdb_name);

  token = gswat_gdb_debugger_send_mi_command (self->priv->debugger,
					      command,
					      NULL,
					      NULL);

  g_free (command);

  result = gswat_gdb_debugger_get_mi_result_record (self->priv->debugger,
						    token);
  if (!result)
    {
      /* An IO error has occurred */
      return g_strdup ("Error retrieving value!");;
    }
  if (result && result->val)
    {
      value = gdbmi_value_hash_lookup (result->val, "value");
    }

  if (value)
    {
      value_string = g_strdup (gdbmi_value_literal_get (value));
    }
  else
    {
      g_warning ("Unexpected failure when retrieving variable value");
      value_string = g_strdup ("Error retrieving value!");
      g_set_error (error,
		   GSWAT_VARIABLE_OBJECT_ERROR,
		   GSWAT_VARIABLE_OBJECT_ERROR_GET_VALUE_FAILED,
		   "Failed to retrieve the variable object's "
		   "expresion value  (%s)",
		   self->priv->expression);
    }

  gswat_gdb_debugger_free_mi_record (result);

  return value_string;
}

static guint
gswat_gdb_variable_object_get_child_count (GSwatVariableObject *object)
{
  GSwatGdbVariableObject *self;

  g_return_val_if_fail (GSWAT_IS_GDB_VARIABLE_OBJECT (object), 0);
  self = GSWAT_GDB_VARIABLE_OBJECT (object);

  if (!validate_variable_object (self))
    {
      return 0;
    }

  if (self->priv->child_count == -1)
    {
      self->priv->child_count = count_gdb_variable_object_children (self);
    }

  return  (guint)self->priv->child_count;
}

static gint
count_gdb_variable_object_children (GSwatGdbVariableObject *self)
{
  gchar *command;
  gulong token;
  GSwatGdbMIRecord *result;
  const GDBMIValue *numchild_val;
  gint child_count;

  /* find out if this expression has any children */
  command = g_strdup_printf ("-var-info-num-children %s",
			     self->priv->gdb_name);
  token = gswat_gdb_debugger_send_mi_command (self->priv->debugger,
					      command,
					      NULL,
					      NULL);
  g_free (command);
  result=gswat_gdb_debugger_get_mi_result_record (self->priv->debugger, token);
  if (!result)
    {
      /* An IO error has occurred */
      return 0;
    }
  numchild_val=gdbmi_value_hash_lookup (result->val, "numchild");
  if (numchild_val)
    {
      const char *child_count_str;
      child_count_str = gdbmi_value_literal_get (numchild_val);
      child_count =  (gint)strtoul (child_count_str, NULL, 10);
    }
  else
    {
      child_count = 0;
    }
  gswat_gdb_debugger_free_mi_record (result);

  return child_count;
}

static GList *
gswat_gdb_variable_object_get_children (GSwatVariableObject *object)
{
  GSwatGdbVariableObject *self;
  GList *tmp;
  gchar *command;
  gulong token;
  GSwatGdbMIRecord *result;
  const GDBMIValue *children_val, *child_val;
  int n;
  GSwatGdbVariableObject *variable_object;

  g_return_val_if_fail (GSWAT_IS_GDB_VARIABLE_OBJECT (object), NULL);
  self = GSWAT_GDB_VARIABLE_OBJECT (object);

  if (!validate_variable_object (self))
    {
      return NULL;
    }

  if (self->priv->children && self->priv->children_consistent)
    {
      g_list_foreach (self->priv->children,  (GFunc)g_object_ref, NULL);
      return g_list_copy (self->priv->children);
    }

  /* --simple-values means print the name and value of
   * simple types, but omit the value for complex
   * types. Below we cache the number of
   * child->children as 0 if we get a value, else we
   * mark the number un-dermined  (-1)
   */
  command=g_strdup_printf ("-var-list-children --simple-values %s",
			   self->priv->gdb_name);

  token = gswat_gdb_debugger_send_mi_command (self->priv->debugger,
					      command,
					      NULL,
					      NULL);
  g_free (command);
  result = gswat_gdb_debugger_get_mi_result_record (self->priv->debugger,
						    token);
  if (!result)
    {
      /* An IO error has occurred */
      return NULL;
    }

  children_val = gdbmi_value_hash_lookup (result->val, "children");
  n=0;
  while ( (child_val = gdbmi_value_list_get_nth (children_val, n)))
    {
      gboolean child_already_exists = FALSE;
      const GDBMIValue *name_val, *expression_val, *value_val;
      const GDBMIValue *numchild_val;
      const gchar *name_str, *expression_str, *child_value_str;
      const gchar *numchild_str;
      gint child_count;

      /* FIXME - gdb automatically creates variable objects
       * the first time you list children, but the docs
       * arn't clear about the frame number or expression
       * that are used.
       *
       * It looks like the expression is just the name
       * of the member  (i.e. not a valid expression
       * in the scope of the parent expression)
       *
       * I need to find out about the frame number
       */

      name_val = gdbmi_value_hash_lookup (child_val, "name");
      name_str = gdbmi_value_literal_get (name_val);

      for (tmp=self->priv->children; tmp!=NULL; tmp=tmp->next)
	{
	  GSwatGdbVariableObject *variable_object = tmp->data;

	  if (strcmp (variable_object->priv->gdb_name, name_str) == 0)
	    {
	      child_already_exists = TRUE;
	    }
	}
      if (child_already_exists)
	{
	  n++;
	  continue;
	}

      expression_val = gdbmi_value_hash_lookup (child_val, "exp");
      expression_str = gdbmi_value_literal_get (expression_val);

      /* complex types wont have a value */
      value_val = gdbmi_value_hash_lookup (child_val, "value");
      child_value_str=NULL;
      if (value_val)
	{
	  child_value_str = gdbmi_value_literal_get (value_val);
	  child_count = 0;
	}
      else
	{
	  child_count = -1;
	}

      numchild_val = gdbmi_value_hash_lookup (child_val, "numchild");
      numchild_str = NULL;
      if (numchild_val)
	{
	  numchild_str = gdbmi_value_literal_get (numchild_val);
	  child_count =  (gint)strtoul (numchild_str, NULL, 10);
	}


      /* Note this function also adds the wrapped child to
       * the parents list of children */
      variable_object =
	wrap_child_gdb_variable_object (self->priv->debugger,
					self, /* parent */
					name_str,
					expression_str,
					child_value_str,
					self->priv->frame,
					child_count);

      n++;
    }

  self->priv->children_consistent = TRUE;

  gswat_gdb_debugger_free_mi_record (result);

  return g_list_copy (self->priv->children);
}

void
gswat_gdb_variable_object_async_update_all (GSwatGdbDebugger *self)
{
  gswat_gdb_debugger_send_mi_command (self,
				      "-var-update --simple-values *",
				      update_variable_objects_mi_callback,
				      NULL);
}

static void
synchronous_update_all (GSwatGdbDebugger *gdb_debugger)
{
  gulong token;
  GSwatGdbMIRecord *result;

  token = gswat_gdb_debugger_send_mi_command (gdb_debugger,
					      "-var-update --simple-values *",
					      NULL,
					      NULL);
  result = gswat_gdb_debugger_get_mi_result_record (gdb_debugger, token);
  if (!result)
    {
      /* An IO error has occcurred */
      return;
    }
  update_variable_objects_mi_callback (gdb_debugger, result, NULL);
  gswat_gdb_debugger_free_mi_record (result);
}

static void
update_variable_objects_mi_callback (GSwatGdbDebugger *gdb_debugger,
				     const GSwatGdbMIRecord *record,
				     void *data)
{
  if (record->type == GSWAT_GDB_MI_REC_TYPE_RESULT_ERROR)
    {
      g_warning ("update_variable_objects_mi_callback: error updating var "
		 "objects");
      return;
    }

  if (record->type != GSWAT_GDB_MI_REC_TYPE_RESULT_DONE)
    {
      g_warning ("update_variable_objects_mi_callback: unexpected result "
		 "type");
      return;
    }

  handle_changelist (gdb_debugger, record);

  return;
}

static void
handle_changelist (GSwatGdbDebugger *gdb_debugger,
		   const GSwatGdbMIRecord *record)
{
  GList *all_variables, *all_variables_copy;
  int i, changed_count;
  GList *tmp;
  GDBMIValue *val;
  const GDBMIValue *changelist_val;

  all_variables = g_object_get_data (G_OBJECT (gdb_debugger),
				     "all-gswat-gdb-variable-objects");
  all_variables_copy = g_list_copy (all_variables);
  g_list_foreach (all_variables_copy,  (GFunc)g_object_ref, NULL);

  val = record->val;
  changelist_val = gdbmi_value_hash_lookup (val, "changelist");

  changed_count = gdbmi_value_get_size (changelist_val);
  for (i=0; i<changed_count; i++)
    {
      const GDBMIValue *change_val, *val;
      const char *variable_gdb_name;
      GSwatGdbVariableObject *variable_object;
      gboolean found = FALSE;
      gboolean child_count_changed = FALSE;
      gboolean type_changed = FALSE;

      change_val = gdbmi_value_list_get_nth (changelist_val, i);

      val = gdbmi_value_hash_lookup (change_val, "name");
      variable_gdb_name = gdbmi_value_literal_get (val);

      for (tmp=all_variables_copy; tmp!=NULL; tmp=tmp->next)
	{
	  char *gdb_name;

	  variable_object = tmp->data;
	  if (!variable_object->priv->gdb_name)
	    {
	      continue;
	    }
	  gdb_name = variable_object->priv->gdb_name;
	  if (strcmp (gdb_name, variable_gdb_name)==0)
	    {
	      found = TRUE;
	      break;
	    }
	}

      if (!found)
	{
	  g_warning ("gswat_gdb_variable_object_handle_changelist: got "
		     "an unexpected change list entry");
	  continue;
	}

      val = gdbmi_value_hash_lookup (change_val, "in_scope");
      if (strcmp (gdbmi_value_literal_get (val), "true") != 0)
	{
	  delete_gdb_variable_object (variable_object);
	  continue;
	}


      /* refering to gdb's varobj.c, then in the case
       * where a varobj can be evaluated in any
       * frame and its evaluated type changes, it
       * and its children are deleted and a new
       * root varobj with the same name is created
       *  (for our purposes that means just the children
       * have been deleted)
       */
      val = gdbmi_value_hash_lookup (change_val, "new_type");
      if (val)
	{
	  if (variable_object->priv->children)
	    {
	      for (tmp=variable_object->priv->children;tmp!=NULL;tmp=tmp->next)
		{
		  delete_gdb_variable_object (tmp->data);
		}
	      g_list_free (variable_object->priv->children);
	      variable_object->priv->children = NULL;
	    }
	  type_changed = TRUE;
	}

      val = gdbmi_value_hash_lookup (change_val, "new_num_children");
      if (val)
	{
	  const gchar *child_count_str;
	  child_count_str = gdbmi_value_literal_get (val);
	  variable_object->priv->child_count =
	    (gint)strtoul (child_count_str, NULL, 10);

	  child_count_changed = TRUE;
	}
      else
	{
	  if (type_changed)
	    {
	      variable_object->priv->child_count = -1;
	      child_count_changed = TRUE;
	    }
	}

      g_free (variable_object->priv->cached_value);
      val = gdbmi_value_hash_lookup (change_val, "value");
      if (val)
	{
	  const char *new_value;
	  new_value = gdbmi_value_literal_get (val);
	  variable_object->priv->cached_value=g_strdup (new_value);
	}
      else
	{
	  variable_object->priv->cached_value
	    = evaluate_gdb_variable_object_expression (variable_object, NULL);
	}

      /* #warning FIXME this workaround probably needs to be put
       * in more places */
      if (!GSWAT_GDB_DEBUGGER_CAN_INSPECT_NULL_VAROBJS)
	{
	  if (strcmp (variable_object->priv->cached_value, "0x0")==0)
	    {
	      variable_object->priv->child_count = 0;
	      child_count_changed = TRUE;
	    }
	  else
	    {
	      variable_object->priv->child_count = -1;
	      child_count_changed = TRUE;
	    }
	}

      if (child_count_changed)
	{
	  g_object_notify (G_OBJECT (variable_object),
			   "child-count");
	}

      g_object_notify (G_OBJECT (variable_object),
		       "value");

    }

  /* Mark all variable objects as up to date
   * and remove our temporary reference */
  for (tmp=all_variables_copy; tmp!=NULL; tmp=tmp->next)
    {
      GSwatGdbVariableObject *variable_object;
      variable_object = tmp->data;
      variable_object->priv->gdb_interrupt_count =
	gswat_gdb_debugger_get_interrupt_count (variable_object->priv->debugger);
      g_object_unref (variable_object);
    }

  g_list_free (all_variables_copy);
}

void
gswat_gdb_variable_object_cleanup (GSwatGdbDebugger *gdb_debugger)
{
  GList *all_variables, *all_variables_copy;
  GList *tmp;

  all_variables = g_object_get_data (G_OBJECT (gdb_debugger),
				     "all-gswat-gdb-variable-objects");
  all_variables_copy = g_list_copy (all_variables);
  g_list_foreach (all_variables_copy,  (GFunc)g_object_ref, NULL);

  for (tmp=all_variables_copy; tmp!=NULL; tmp=tmp->next)
    {
      /* This will delete all the gdb side variable
       * objects and send a valid=FALSE notification
       * to the owners of all the variable objects
       * who will be responsible for final unref'ing
       */
      delete_gdb_variable_object (tmp->data);
    }
  g_list_foreach (all_variables_copy,  (GFunc)g_object_unref, NULL);
  g_list_free (all_variables_copy);


  all_variables = g_object_get_data (G_OBJECT (gdb_debugger),
				     "all-gswat-gdb-variable-objects");
  g_list_free (all_variables);
  g_object_set_data (G_OBJECT (gdb_debugger),
		     "all-gswat-gdb-variable-objects",
		     NULL);
}

