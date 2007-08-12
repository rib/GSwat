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

#ifndef GSWAT_SESSION_H
#define GSWAT_SESSION_H

#include <glib.h>
#include <glib-object.h>
/* include your parent object here */

G_BEGIN_DECLS

#define GSWAT_SESSION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSWAT_TYPE_SESSION, GSwatSession))
#define GSWAT_TYPE_SESSION            (gswat_session_get_type())
#define GSWAT_SESSION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSWAT_TYPE_SESSION, GSwatSessionClass))
#define GSWAT_IS_SESSION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSWAT_TYPE_SESSION))
#define GSWAT_IS_SESSION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSWAT_TYPE_SESSION))
#define GSWAT_SESSION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSWAT_TYPE_SESSION, GSwatSessionClass))

typedef struct _GSwatSession        GSwatSession;
typedef struct _GSwatSessionClass   GSwatSessionClass;
typedef struct _GSwatSessionPrivate GSwatSessionPrivate;

struct _GSwatSession
{
    /* add your parent type here */
    GObject parent;

    /* add pointers to new members here */
    
	/*< private > */
	GSwatSessionPrivate *priv;
};

struct _GSwatSessionClass
{
    /* add your parent class here */
    GObjectClass parent_class;

    /* add signals here */
	void (* edit_done)		(GSwatSession *session);
	void (* edit_abort)		(GSwatSession *session);
};

/* Session types */
typedef enum {
    GSWAT_SESSION_TYPE_LOCAL_RUN,
    GSWAT_SESSION_TYPE_LOCAL_PID,
    GSWAT_SESSION_TYPE_REMOTE_RUN,
    GSWAT_SESSION_TYPE_REMOTE_PID,
    GSWAT_SESSION_TYPE_COUNT 
}GSwatSessionType;


GType gswat_session_get_type(void);

/* add additional methods here */
GSwatSession *gswat_session_new(void);

gchar *gswat_session_get_name(GSwatSession *self);
void gswat_session_set_name(GSwatSession* self, const char *name);
gchar *gswat_session_get_target_type(GSwatSession *self);
void gswat_session_set_target_type(GSwatSession *self, const gchar *target_type);
gchar *gswat_session_get_target(GSwatSession *self);
void gswat_session_set_target(GSwatSession *self, const gchar *target);
gchar *gswat_session_get_working_dir(GSwatSession* self);
void gswat_session_set_working_dir(GSwatSession* self, const gchar *working_dir);
void gswat_session_add_environment_variable(GSwatSession *self,
                                            const gchar *name,
                                            const gchar *value);

/* TODO seperate out UI code into a different class */
void gswat_session_edit(GSwatSession *session);

G_END_DECLS

#endif /* GSWAT_SESSION_H */

