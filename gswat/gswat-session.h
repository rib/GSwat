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

#ifndef GSWAT_SESSION_H
#define GSWAT_SESSION_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

/**
 * SECTION:gswat-session
 * @short_description: An API for describing debug targets and setting up the
 *		       environment
 *
 * A session represents a target you want to connect to and debug as well as
 * the environment you want while debugging, including the current working
 * directory and environment variables.
 */

#define GSWAT_SESSION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSWAT_TYPE_SESSION, GSwatSession))
#define GSWAT_TYPE_SESSION            (gswat_session_get_type ())
#define GSWAT_SESSION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSWAT_TYPE_SESSION, GSwatSessionClass))
#define GSWAT_IS_SESSION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSWAT_TYPE_SESSION))
#define GSWAT_IS_SESSION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSWAT_TYPE_SESSION))
#define GSWAT_SESSION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSWAT_TYPE_SESSION, GSwatSessionClass))

typedef struct _GSwatSession        GSwatSession;
typedef struct _GSwatSessionClass   GSwatSessionClass;
typedef struct _GSwatSessionPrivate GSwatSessionPrivate;

struct _GSwatSession
{
  GObject parent;

  /*< private > */
  GSwatSessionPrivate *priv;
};

struct _GSwatSessionClass
{
  GObjectClass parent_class;
};

typedef struct _GSwatSessionEnvironmentVariable
{
  gchar *name;
  gchar *value;
}GSwatSessionEnvironmentVariable;

/* Session types */
typedef enum {
    GSWAT_SESSION_TYPE_LOCAL_RUN,
    GSWAT_SESSION_TYPE_LOCAL_PID,
    GSWAT_SESSION_TYPE_REMOTE_RUN,
    GSWAT_SESSION_TYPE_REMOTE_PID,
    GSWAT_SESSION_TYPE_COUNT
}GSwatSessionType;

GType gswat_session_get_type (void);

/**
 * gswat_session_new:
 *
 * Creates a new session object. A session represents a target that you
 * want to debug, the current working directory and the environment variables
 * you want set while debugging the target.
 *
 * Returns: A new session object
 */
GSwatSession *gswat_session_new (void);

gchar *gswat_session_get_name (GSwatSession *self);
void gswat_session_set_name (GSwatSession* self, const char *name);
gchar *gswat_session_get_target_type (GSwatSession *self);
void gswat_session_set_target_type (GSwatSession *self, const gchar *target_type);
gchar *gswat_session_get_target (GSwatSession *self);
void gswat_session_set_target (GSwatSession *self, const gchar *target);
gchar *gswat_session_get_working_dir (GSwatSession* self);
void gswat_session_set_working_dir (GSwatSession* self, const gchar *working_dir);
GList *gswat_session_get_environment (GSwatSession *self);
void gswat_session_free_environment (GList *environment);
void gswat_session_clear_environment (GSwatSession *self);
void gswat_session_add_environment_variable (GSwatSession *self,
					    const gchar *name,
					    const gchar *value);
void gswat_session_delete_environment_variable (GSwatSession *self,
					       const gchar *name);
glong gswat_session_get_access_time (GSwatSession *self);
void gswat_session_set_access_time (GSwatSession *self, glong atime);

G_END_DECLS

#endif /* GSWAT_SESSION_H */

