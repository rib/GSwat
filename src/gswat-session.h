/*
 * <preamble>
 * gswat - A graphical program debugger for Gnome
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


#ifndef GSWAT_SESSION_H
#define GSWAT_SESSION_H

#include <glib-object.h>

G_BEGIN_DECLS


#define GSWAT_TYPE_SESSION         (gswat_session_get_type())
#define GSWAT_SESSION(i)           (G_TYPE_CHECK_INSTANCE_CAST((i), GSWAT_TYPE_SESSION, GSwatSession))
#define GSWAT_SESSION_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST((c), GSWAT_TYPE_SESSION, GSwatSessionClass))
#define GSWAT_IS_SESSION(i)        (G_TYPE_CHECK_INSTANCE_TYPE((i), GSWAT_TYPE_SESSION))
#define GSWAT_IS_SESSION_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE((c), GSWAT_TYPE_SESSION))
#define GSWAT_SESSION_GET_CLASS(i) (G_TYPE_INSTANCE_GET_CLASS((i), GSWAT_TYPE_SESSION, GSwatSessionClass))

GType gswat_session_get_type(void);



/* TODO 

void	    gswat_session_load(GSwatSession* self);
void	    gswat_session_save(GSwatSession* self);
*/



/* Session types */
enum {
    GSWAT_SESSION_TYPE_LOCAL_RUN,
    GSWAT_SESSION_TYPE_LOCAL_PID,
    GSWAT_SESSION_TYPE_REMOTE_RUN,
    GSWAT_SESSION_TYPE_REMOTE_PID
};



typedef struct _GSwatSession   GSwatSession;

struct _GSwatSession {
	GObject base_instance;
	struct GSwatSessionPrivate* priv;
};




typedef struct _GSwatSessionClass           GSwatSessionClass;

struct _GSwatSessionClass
{
	GObjectClass parent_class;

	/* Signals */ 

	void (* edit_done)		(GSwatSession *session);
	void (* edit_abort)		(GSwatSession *session);
};




/* Public methods */

GSwatSession* gswat_session_new(void);

void gswat_session_edit(GSwatSession *session);

gchar *gswat_session_get_command(GSwatSession* self, gint *argc, gchar ***argv);
void gswat_session_set_command(GSwatSession* self, const gchar *command);
gint gswat_session_get_pid(GSwatSession* self);
void gswat_session_set_pid(GSwatSession* self, gint pid);
gchar *gswat_session_get_working_dir(GSwatSession* self);
void gswat_session_set_working_dir(GSwatSession* self, const gchar *working_dir);


G_END_DECLS

#endif /* !GSWAT_SESSION_H */

