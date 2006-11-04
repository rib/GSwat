/* This file is part of gnome gdb
 *
 * AUTHORS
 *     Sven Herzberg  <herzi@gnome-de.org>
 *
 * Copyright (C) 2006  Sven Herzberg <herzi@gnome-de.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifndef GSWAT_DEBUGGER_H
#define GSWAT_DEBUGGER_H

#include <glib-object.h>

#include "gswat-session.h"

G_BEGIN_DECLS

typedef struct _GSwatDebugger   GSwatDebugger;
typedef GObjectClass            GSwatDebuggerClass;

#define GSWAT_TYPE_DEBUGGER         (gswat_debugger_get_type())
#define GSWAT_DEBUGGER(i)           (G_TYPE_CHECK_INSTANCE_CAST((i), GSWAT_TYPE_DEBUGGER, GSwatDebugger))
#define GSWAT_DEBUGGER_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST((c), GSWAT_TYPE_DEBUGGER, GSwatDebuggerClass))
#define GSWAT_IS_DEBUGGER(i)        (G_TYPE_CHECK_INSTANCE_TYPE((i), GSWAT_TYPE_DEBUGGER))
#define GSWAT_IS_DEBUGGER_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE((c), GSWAT_TYPE_DEBUGGER))
#define GSWAT_DEBUGGER_GET_CLASS(i) (G_TYPE_INSTANCE_GET_CLASS((i), GSWAT_TYPE_DEBUGGER, GSwatDebuggerClass))



typedef enum {
    GSWAT_DEBUGGER_NOT_RUNNING,
	GSWAT_DEBUGGER_RUNNING,
    GSWAT_DEBUGGER_INTERRUPTED
} GSwatDebuggerState;

struct _GSwatDebugger {
	GObject base_instance;
	struct GSwatDebuggerPrivate* priv;
};

typedef struct {
    gchar *name;
    gchar *value;
}GSwatDebuggerFrameArgument;

typedef struct {
    int level;
    unsigned long address;
    gchar *function;
    GList *arguments;
}GSwatDebuggerFrame;

typedef struct {
    gchar   *source_uri;
    gint    line;
}GSwatDebuggerBreakpoint;

/* Public methods */

GType gswat_debugger_get_type(void);

GSwatDebugger* gswat_debugger_new(GSwatSession *session);

void        gswat_debugger_target_connect(GSwatDebugger* self);

void	    gswat_debugger_request_line_breakpoint(GSwatDebugger* self, gchar *uri, guint line);
void	    gswat_debugger_request_function_breakpoint(GSwatDebugger* self, gchar *symbol);
gchar       *gswat_debugger_get_source_uri(GSwatDebugger* self);
gulong      gswat_debugger_get_source_line(GSwatDebugger* self);
void	    gswat_debugger_continue(GSwatDebugger* self);
void        gswat_debugger_finish(GSwatDebugger* self);
void	    gswat_debugger_next(GSwatDebugger* self);
void	    gswat_debugger_step_into(GSwatDebugger* self);
void        gswat_debugger_interrupt(GSwatDebugger* self);
void        gswat_debugger_restart(GSwatDebugger* self);
guint       gswat_debugger_get_state(GSwatDebugger* self);
GList       *gswat_debugger_get_stack(GSwatDebugger* self);
GList       *gswat_debugger_get_breakpoints(GSwatDebugger* self);



G_END_DECLS

#endif /* !GSWAT_DEBUGGER_H */

