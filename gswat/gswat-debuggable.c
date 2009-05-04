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
#include <config.h>

#include <glib/gi18n-lib.h>

#include "gswat-utils.h"
#include "gswat-debuggable.h"

static void gswat_debuggable_base_init (GSwatDebuggableIface *interface);

#if 0
enum {
    SIGNAL_NAME,
    LAST_SIGNAL
};
#endif

//static guint gswat_debuggable_signals[LAST_SIGNAL] = { 0 };
static guint gswat_debuggable_base_init_count = 0;


GType
gswat_debuggable_get_type (void)
{
  static GType self_type = 0;

  if  (!self_type)
    {
      static const GTypeInfo interface_info = {
	  sizeof  (GSwatDebuggableIface),
	  (GBaseInitFunc)gswat_debuggable_base_init,
	  NULL,//gswat_debuggable_base_finalize
      };

      self_type = g_type_register_static (G_TYPE_INTERFACE,
					  "GSwatDebuggable",
					  &interface_info, 0);

      g_type_interface_add_prerequisite (self_type, G_TYPE_OBJECT);
      //g_type_interface_add_prerequisite (self_type, G_TYPE_);
    }

  return self_type;
}

static void
gswat_debuggable_base_init (GSwatDebuggableIface *interface)
{
  gswat_debuggable_base_init_count++;
  GParamSpec *new_param;

  if (gswat_debuggable_base_init_count == 1)
    {
      new_param = g_param_spec_uint ("state",
				     _ ("The Running State"),
				     _ ("The State of the debuggable"
					"  (whether or not it is currently "
					"running)"),
				     0,
				     G_MAXUINT,
				     GSWAT_DEBUGGABLE_DISCONNECTED,
				     GSWAT_PARAM_READABLE);
      g_object_interface_install_property (interface, new_param);

      new_param = g_param_spec_pointer ("stack",
					_ ("Stack"),
					_ ("The Stack of called functions"),
					GSWAT_PARAM_READABLE);
      g_object_interface_install_property (interface, new_param);

      new_param = g_param_spec_pointer ("locals",
					_ ("Locals"),
					_ ("The current frames local "
					   "variables"),
					GSWAT_PARAM_READABLE);
      g_object_interface_install_property (interface, new_param);

      new_param = g_param_spec_pointer ("breakpoints",
					_ ("Breakpoints"),
					_ ("The list of user defined "
					   "breakpoints"),
					GSWAT_PARAM_READABLE);
      g_object_interface_install_property (interface, new_param);

      new_param = g_param_spec_string ("source-uri",
				       _ ("Source URI"),
				       _ ("The location for the source file "
					  "currently being debuggged"),
				       NULL,
				       GSWAT_PARAM_READABLE);
      g_object_interface_install_property (interface, new_param);

      new_param = g_param_spec_ulong ("source-line",
				      _ ("Source Line"),
				      _ ("The line of your source file "
					 "currently being debuggged"),
				      0,
				      G_MAXULONG,
				      0,
				      GSWAT_PARAM_READABLE);
      g_object_interface_install_property (interface, new_param);

      new_param = g_param_spec_uint ("frame",
				     _ ("Active Frame"),
				     _ ("The debuggable's currently active "
					"stack frame"),
				     0,
				     G_MAXUINT,
				     0,
				     GSWAT_PARAM_READWRITE);
      g_object_interface_install_property (interface, new_param);

    }
}

#if 0
static void
gswat_debuggable_base_finalize (gpointer g_class)
{
  if (gswat_debuggable_base_init_count == 0)
    {

    }
}
#endif

GQuark
gswat_debuggable_error_quark (void)
{
  static GQuark q = 0;
  if (q==0)
    {
      q = g_quark_from_static_string ("gswat-debuggable-error");
    }
  return q;
}

/**
 * gswat_debuggable_target_connect:
 * @object: The debuggable that should connect to it's target
 *
 * This should make the debuggable object connect to
 * its target and marks the start of a debug session.
 *
 * Returns: sucess status
 */
/* FIXME - there should a synchronous and asynchronous
 * way to connect the debugger to its target.
 * GUIs should use the asychronous and we should use a
 * signal to reflect completion. Simple script bindings
 * might prefer to use a synchronous connect.
 */
gboolean
gswat_debuggable_connect (GSwatDebuggable* object, GError **error)
{
  GSwatDebuggableIface *debuggable;
  gboolean ret;

  g_return_val_if_fail (GSWAT_IS_DEBUGGABLE (object), FALSE);
  g_return_val_if_fail  (error == NULL || *error == NULL, FALSE);

  debuggable = GSWAT_DEBUGGABLE_GET_IFACE (object);

  g_object_ref (object);
  if (gswat_debuggable_get_state (object) == GSWAT_DEBUGGABLE_DISCONNECTED)
    {
      ret = debuggable->connect (object, error);
    }
  g_object_unref (object);

  return ret;
}

void
gswat_debuggable_disconnect (GSwatDebuggable* object)
{
  GSwatDebuggableIface *debuggable;

  g_return_if_fail (GSWAT_IS_DEBUGGABLE (object));

  debuggable = GSWAT_DEBUGGABLE_GET_IFACE (object);

  g_object_ref (object);
  if (gswat_debuggable_get_state (object) != GSWAT_DEBUGGABLE_DISCONNECTED)
    {
      debuggable->disconnect (object);
    }
  g_object_unref (object);
}

void
gswat_debuggable_request_line_breakpoint (GSwatDebuggable* object,
					  gchar *uri,
					  guint line)
{
  GSwatDebuggableIface *debuggable;

  g_return_if_fail (GSWAT_IS_DEBUGGABLE (object));
  debuggable = GSWAT_DEBUGGABLE_GET_IFACE (object);

  g_object_ref (object);
  if (gswat_debuggable_get_state (object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
      debuggable->request_line_breakpoint (object, uri, line);
    }
  g_object_unref (object);
}

void
gswat_debuggable_request_function_breakpoint (GSwatDebuggable* object,
					      gchar *symbol)
{
  GSwatDebuggableIface *debuggable;

  g_return_if_fail (GSWAT_IS_DEBUGGABLE (object));
  debuggable = GSWAT_DEBUGGABLE_GET_IFACE (object);

  g_object_ref (object);
  if (gswat_debuggable_get_state (object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
      debuggable->request_function_breakpoint (object, symbol);
    }
  g_object_unref (object);
}

gchar *
gswat_debuggable_get_source_uri (GSwatDebuggable* object)
{
  GSwatDebuggableIface *debuggable;
  gchar *ret;

  g_return_val_if_fail (GSWAT_IS_DEBUGGABLE (object), NULL);
  debuggable = GSWAT_DEBUGGABLE_GET_IFACE (object);

  g_object_ref (object);
  if (gswat_debuggable_get_state (object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
      ret = debuggable->get_source_uri (object);
    }
  else
    {
      ret = NULL;
    }
  g_object_unref (object);

  return ret;
}

gint
gswat_debuggable_get_source_line (GSwatDebuggable* object)
{
  GSwatDebuggableIface *debuggable;
  gulong ret;

  g_return_val_if_fail (GSWAT_IS_DEBUGGABLE (object), 0);
  debuggable = GSWAT_DEBUGGABLE_GET_IFACE (object);

  g_object_ref (object);
  if (gswat_debuggable_get_state (object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
      ret = debuggable->get_source_line (object);
    }
  else
    {
      ret = 0;
    }
  g_object_unref (object);

  return ret;
}

void
gswat_debuggable_continue (GSwatDebuggable* object)
{
  GSwatDebuggableIface *debuggable;

  g_return_if_fail (GSWAT_IS_DEBUGGABLE (object));
  debuggable = GSWAT_DEBUGGABLE_GET_IFACE (object);

  g_object_ref (object);
  if (gswat_debuggable_get_state (object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
      debuggable->cont (object);
    }
  g_object_unref (object);
}

void
gswat_debuggable_finish (GSwatDebuggable* object)
{
  GSwatDebuggableIface *debuggable;

  g_return_if_fail (GSWAT_IS_DEBUGGABLE (object));
  debuggable = GSWAT_DEBUGGABLE_GET_IFACE (object);

  g_object_ref (object);
  if (gswat_debuggable_get_state (object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
      debuggable->finish (object);
    }
  g_object_unref (object);
}

void
gswat_debuggable_next (GSwatDebuggable* object)
{
  GSwatDebuggableIface *debuggable;

  g_return_if_fail (GSWAT_IS_DEBUGGABLE (object));
  debuggable = GSWAT_DEBUGGABLE_GET_IFACE (object);

  g_object_ref (object);
  if (gswat_debuggable_get_state (object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
      debuggable->next (object);
    }
  g_object_unref (object);
}

void
gswat_debuggable_step (GSwatDebuggable* object)
{
  GSwatDebuggableIface *debuggable;

  g_return_if_fail (GSWAT_IS_DEBUGGABLE (object));
  debuggable = GSWAT_DEBUGGABLE_GET_IFACE (object);

  g_object_ref (object);
  if (gswat_debuggable_get_state (object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
      debuggable->step (object);
    }
  g_object_unref (object);
}

void
gswat_debuggable_interrupt (GSwatDebuggable* object)
{
  GSwatDebuggableIface *debuggable;

  g_return_if_fail (GSWAT_IS_DEBUGGABLE (object));
  debuggable = GSWAT_DEBUGGABLE_GET_IFACE (object);

  g_object_ref (object);
  if (gswat_debuggable_get_state (object) == GSWAT_DEBUGGABLE_RUNNING)
    {
      debuggable->interrupt (object);
    }
  g_object_unref (object);
}

void
gswat_debuggable_restart (GSwatDebuggable* object)
{
  GSwatDebuggableIface *debuggable;

  g_return_if_fail (GSWAT_IS_DEBUGGABLE (object));
  debuggable = GSWAT_DEBUGGABLE_GET_IFACE (object);

  g_object_ref (object);
  if (gswat_debuggable_get_state (object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
      debuggable->restart (object);
    }
  g_object_unref (object);
}

GSwatDebuggableState
gswat_debuggable_get_state (GSwatDebuggable* object)
{
  GSwatDebuggableIface *debuggable;
  GSwatDebuggableState ret;

  g_return_val_if_fail (GSWAT_IS_DEBUGGABLE (object), 0);
  debuggable = GSWAT_DEBUGGABLE_GET_IFACE (object);

  g_object_ref (object);
  ret = debuggable->get_state (object);
  g_object_unref (object);

  return ret;
}

GQueue *
gswat_debuggable_get_stack (GSwatDebuggable* object)
{
  GSwatDebuggableIface *debuggable;
  GQueue *ret;

  g_return_val_if_fail (GSWAT_IS_DEBUGGABLE (object), NULL);
  debuggable = GSWAT_DEBUGGABLE_GET_IFACE (object);

  g_object_ref (object);
  if (gswat_debuggable_get_state (object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
      ret = debuggable->get_stack (object);
    }
  else
    {
      ret = NULL;
    }
  g_object_unref (object);

  return ret;
}

void
gswat_debuggable_frame_free (GSwatDebuggableFrame *frame)
{
  GList *tmp;

  g_free (frame->function);
  g_free (frame->source_uri);

  for (tmp=frame->arguments; tmp!=NULL; tmp=tmp->next)
    {
      GSwatDebuggableFrameArgument *arg =
	(GSwatDebuggableFrameArgument *)tmp->data;

      g_free (arg->name);
      g_free (arg->value);
      g_free (arg);
    }
  g_list_free (frame->arguments);

  g_free (frame);
}

void
gswat_debuggable_stack_free (GQueue *stack)
{
  GList *tmp;

  if (!stack)
    {
      return;
    }

  for (tmp=stack->head; tmp!=NULL; tmp=tmp->next)
    {
      gswat_debuggable_frame_free (tmp->data);
    }
  g_queue_free (stack);
}

GList *
gswat_debuggable_get_breakpoints (GSwatDebuggable* object)
{
  GSwatDebuggableIface *debuggable;
  GList *ret;

  g_return_val_if_fail (GSWAT_IS_DEBUGGABLE (object), NULL);
  debuggable = GSWAT_DEBUGGABLE_GET_IFACE (object);

  g_object_ref (object);
  if (gswat_debuggable_get_state (object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
      ret = debuggable->get_breakpoints (object);
    }
  else
    {
      ret = NULL;
    }
  g_object_unref (object);

  return ret;
}

void
gswat_debuggable_free_breakpoints (GList *breakpoints)
{
  GList *tmp;

  for (tmp=breakpoints; tmp!=NULL; tmp=tmp->next)
    {
      GSwatDebuggableBreakpoint *current_breakpoint =
	(GSwatDebuggableBreakpoint *)tmp->data;
      g_free (current_breakpoint->source_uri);
    }
  g_list_free (breakpoints);
}

GList *
gswat_debuggable_get_locals_list (GSwatDebuggable* object)
{
  GSwatDebuggableIface *debuggable;
  GList *ret;

  g_return_val_if_fail (GSWAT_IS_DEBUGGABLE (object), NULL);
  debuggable = GSWAT_DEBUGGABLE_GET_IFACE (object);

  g_object_ref (object);
  if (gswat_debuggable_get_state (object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
      ret = debuggable->get_locals_list (object);
    }
  else
    {
      ret = NULL;
    }
  g_object_unref (object);

  return ret;
}

guint
gswat_debuggable_get_frame (GSwatDebuggable* object)
{
  GSwatDebuggableIface *debuggable;
  guint ret;

  g_return_val_if_fail (GSWAT_IS_DEBUGGABLE (object), 0);
  debuggable = GSWAT_DEBUGGABLE_GET_IFACE (object);

  g_object_ref (object);
  if (gswat_debuggable_get_state (object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
      ret = debuggable->get_frame (object);
    }
  else
    {
      ret = 0;
    }
  g_object_unref (object);

  return ret;
}

void
gswat_debuggable_set_frame (GSwatDebuggable* object, guint frame)
{
  GSwatDebuggableIface *debuggable;

  g_return_if_fail (GSWAT_IS_DEBUGGABLE (object));
  debuggable = GSWAT_DEBUGGABLE_GET_IFACE (object);

  g_object_ref (object);
  if (gswat_debuggable_get_state (object) == GSWAT_DEBUGGABLE_INTERRUPTED)
    {
      debuggable->set_frame (object, frame);
    }
  g_object_unref (object);
}

