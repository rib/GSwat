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

#ifndef GG_DEBUGGER_H
#define GG_DEBUGGER_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _GGDebugger GGDebugger;
typedef GObjectClass       GGDebuggerClass;

#define GG_TYPE_DEBUGGER         (gg_debugger_get_type())
#define GG_DEBUGGER(i)           (G_TYPE_CHECK_INSTANCE_CAST((i), GG_TYPE_DEBUGGER, GGDebugger))
#define GG_DEBUGGER_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST((c), GG_TYPE_DEBUGGER, GGDebuggerClass))
#define GG_IS_DEBUGGER(i)        (G_TYPE_CHECK_INSTANCE_TYPE((i), GG_TYPE_DEBUGGER))
#define GG_IS_DEBUGGER_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE((c), GG_TYPE_DEBUGGER))
#define GG_DEBUGGER_GET_CLASS(i) (G_TYPE_INSTANCE_GET_CLASS((i), GG_TYPE_DEBUGGER, GGDebuggerClass))

GType gg_debugger_get_type(void);

GGDebugger* gg_debugger_new   (void);
void        gg_debugger_attach(GGDebugger* self,
			       guint       pid);
void	    gg_debugger_continue(GGDebugger* self);
void	    gg_debugger_step_into(GGDebugger* self);
void	    gg_debugger_step_over(GGDebugger* self);

struct _GGDebugger {
	GObject base_instance;
	struct GGDebuggerPrivate* priv;
};

G_END_DECLS

#endif /* !GG_DEBUGGER_H */

