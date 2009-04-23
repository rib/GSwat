/*
 * <preamble>
 * gswat - A graphical program debugger for Gnome
 * Copyright (C) 2006  Robert Bragg
 * </preamble>
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
 */

#ifndef MY_DOABLE_H
#define MY_DOABLE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MY_TYPE_DOABLE           (my_doable_get_type ())
#define MY_DOABLE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), MY_TYPE_DOABLE, MyDoable))
#define MY_IS_DOABLE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MY_TYPE_DOABLE))
#define MY_DOABLE_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), MY_TYPE_DOABLE, MyDoableIface))

typedef struct _MyDoableIface MyDoableIface;
typedef void MyDoable; /* dummy typedef */

struct _MyDoableIface
{
	GTypeInterface g_iface;

	/* signals: */

    //void (* signal_member)(MyDoable *self);

    /* VTable: */
    //void (*method1)(MyDoable *self);
    //void (*method2)(MyDoable *self);
};

GType my_doable_get_type(void);

/* Interface functions */
//void my_doable_method1(MyDoable *self);
//void my_doable_method2(MyDoable *self);

G_END_DECLS

#endif /* MY_DOABLE_H */

