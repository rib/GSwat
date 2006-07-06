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

#ifndef GG_GDB_VARIABLE_H
#define GG_GDB_VARIABLE_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _GGGdbVariable GGGdbVariable;

struct _GGGdbVariable {
	gchar* name;
	GValue* val;
};

GGGdbVariable* gg_gdb_variable_parse(gchar const* string,
				     gchar const**endptr);
void           gg_gdb_variable_print(GGGdbVariable const* self);
void           gg_gdb_variable_free (GGGdbVariable* self);

G_END_DECLS

#endif /* !GG_GDB_VARIABLE_H */
