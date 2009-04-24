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

#ifndef GSWAT_DEBUGGABLE_H
#define GSWAT_DEBUGGABLE_H

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * SECTION:gswat-debuggable
 * @short_description: An interface that abstracts over any debugger
 *		       implementation
 *
 * All debuggers implement all the features exposed in the API
 */


#define GSWAT_TYPE_DEBUGGABLE           (gswat_debuggable_get_type())
#define GSWAT_DEBUGGABLE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSWAT_TYPE_DEBUGGABLE, GSwatDebuggable))
#define GSWAT_IS_DEBUGGABLE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSWAT_TYPE_DEBUGGABLE))
#define GSWAT_DEBUGGABLE_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GSWAT_TYPE_DEBUGGABLE, GSwatDebuggableIface))

#define GSWAT_DEBUGGABLE_ERROR (gswat_debuggable_error_quark ())

enum {
    GSWAT_DEBUGGABLE_ERROR_TARGET_CONNECT_FAILED
};

typedef struct _GSwatDebuggableIface GSwatDebuggableIface;
typedef struct _GSwatDebuggable GSwatDebuggable; /* dummy typedef */

struct _GSwatDebuggableIface
{
  GTypeInterface g_iface;

  gboolean (*connect)(GSwatDebuggable *object, GError **error);
  void (*disconnect)(GSwatDebuggable *object);
  void (*request_line_breakpoint)(GSwatDebuggable* object,
				  gchar *uri,
				  guint line);
  void (*request_function_breakpoint)(GSwatDebuggable* object,
				      gchar *symbol);
  gchar *(*get_source_uri)(GSwatDebuggable *object);
  gint (*get_source_line)(GSwatDebuggable *object);
  void (*cont)(GSwatDebuggable *object);
  void (*finish)(GSwatDebuggable *object);
  void (*next)(GSwatDebuggable *object);
  void (*step)(GSwatDebuggable *object);
  void (*interrupt)(GSwatDebuggable *object);
  void (*restart)(GSwatDebuggable *object);
  guint (*get_state)(GSwatDebuggable *object);
  GQueue *(*get_stack)(GSwatDebuggable *object);
  GList *(*get_breakpoints)(GSwatDebuggable *object);
  GList *(*get_locals_list)(GSwatDebuggable *object);
  guint (*get_frame)(GSwatDebuggable *object);
  void (*set_frame)(GSwatDebuggable *object, guint frame);
};

typedef enum {
    GSWAT_DEBUGGABLE_DISCONNECTED,
    GSWAT_DEBUGGABLE_RUNNING,
    GSWAT_DEBUGGABLE_INTERRUPTED
}GSwatDebuggableState;

typedef struct {
    gchar *name;
    gchar *value;
}GSwatDebuggableFrameArgument;

/* TODO: promote this into a fully fledged g_object */
typedef struct {
    guint level;
    unsigned long address;
    gchar *function;
    GList *arguments;
    gchar *source_uri;
    gint line;
}GSwatDebuggableFrame;

typedef struct {
    gchar   *source_uri;
    gint    line;
}GSwatDebuggableBreakpoint;

GType gswat_debuggable_get_type (void);
GQuark gswat_debuggable_error_quark (void);

gboolean gswat_debuggable_connect (GSwatDebuggable* object, GError **error);
void gswat_debuggable_disconnect (GSwatDebuggable* object);
guint gswat_debuggable_get_state (GSwatDebuggable* object);
void gswat_debuggable_request_line_breakpoint (GSwatDebuggable* object,
					       gchar *uri,
					       guint line);
void gswat_debuggable_request_function_breakpoint (GSwatDebuggable* object,
						   gchar *symbol);
gchar *gswat_debuggable_get_source_uri (GSwatDebuggable* object);
gint gswat_debuggable_get_source_line (GSwatDebuggable* object);
void gswat_debuggable_continue (GSwatDebuggable* object);
void gswat_debuggable_finish (GSwatDebuggable* object);
void gswat_debuggable_next (GSwatDebuggable* object);
void gswat_debuggable_step (GSwatDebuggable* object);
void gswat_debuggable_interrupt (GSwatDebuggable* object);
void gswat_debuggable_restart (GSwatDebuggable* object);
GQueue *gswat_debuggable_get_stack (GSwatDebuggable* object);
void gswat_debuggable_stack_free (GQueue *stack);
void gswat_debuggable_frame_free (GSwatDebuggableFrame *frame);
GList *gswat_debuggable_get_breakpoints (GSwatDebuggable* object);
void gswat_debuggable_free_breakpoints (GList *breakpoints);
GList *gswat_debuggable_get_locals_list (GSwatDebuggable* object);
guint gswat_debuggable_get_frame (GSwatDebuggable* object);
void gswat_debuggable_set_frame (GSwatDebuggable* object, guint frame);

G_END_DECLS

#endif /* GSWAT_DEBUGGABLE_H */

