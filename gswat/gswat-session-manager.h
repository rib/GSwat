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

#ifndef GSWAT_SESSION_MANAGER_H
#define GSWAT_SESSION_MANAGER_H

#include <glib.h>
#include <glib-object.h>

#include "gswat-session.h"

G_BEGIN_DECLS

#define GSWAT_SESSION_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSWAT_TYPE_SESSION_MANAGER, GSwatSessionManager))
#define GSWAT_TYPE_SESSION_MANAGER            (gswat_session_manager_get_type ())
#define GSWAT_SESSION_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSWAT_TYPE_SESSION_MANAGER, GSwatSessionManagerClass))
#define GSWAT_IS_SESSION_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSWAT_TYPE_SESSION_MANAGER))
#define GSWAT_IS_SESSION_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSWAT_TYPE_SESSION_MANAGER))
#define GSWAT_SESSION_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSWAT_TYPE_SESSION_MANAGER, GSwatSessionManagerClass))

typedef struct _GSwatSessionManager        GSwatSessionManager;
typedef struct _GSwatSessionManagerClass   GSwatSessionManagerClass;
typedef struct _GSwatSessionManagerPrivate GSwatSessionManagerPrivate;

struct _GSwatSessionManager
{
  GObject parent;

  /*< private > */
  GSwatSessionManagerPrivate *priv;
};

struct _GSwatSessionManagerClass
{
  GObjectClass parent_class;
};

GType gswat_session_manager_get_type (void);

GSwatSessionManager *gswat_session_manager_new (void);


void gswat_session_manager_set_sessions_file (GSwatSessionManager *self,
					      const gchar *filename);
gboolean gswat_session_manager_read_sessions_file (GSwatSessionManager *self);
void gswat_session_manager_save_sessions (GSwatSessionManager *self);

GList *gswat_session_manager_get_sessions (GSwatSessionManager *self);
void gswat_session_manager_unref_sessions (GList *sessions);

void gswat_session_manager_add_session (GSwatSessionManager *self,
					GSwatSession *session);
void gswat_session_manager_remove_session (GSwatSessionManager *self,
					   GSwatSession *session);

GSwatSession *gswat_session_manager_find_session (GSwatSessionManager *self,
						  const gchar *name);

G_END_DECLS

#endif /* GSWAT_SESSION_MANAGER_H */

