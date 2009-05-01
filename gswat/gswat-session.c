/*
 * GSwat
 *
 * An object oriented debugger abstraction library
 *
 * Copyright (C) 2006-2009 Robert Bragg <robert@sixbynine.org>
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

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <glib/gi18n-lib.h>

#include "gswat-utils.h"
#include "gswat-session.h"


/* Macros and defines */
#define GSWAT_SESSION_GET_PRIVATE(object) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object), \
				GSWAT_TYPE_SESSION, \
				GSwatSessionPrivate))

enum {
    PROP_0,
    PROP_NAME,
    PROP_TARGET_TYPE,
    PROP_TARGET,
    PROP_COMMAND,
    PROP_WORKING_DIR,
    PROP_ACCESS_TIME
};

enum {
    SESSION_NAME_COMBO_STRING,
    SESSION_NAME_COMBO_N_COLUMNS
};

struct _GSwatSessionPrivate
{
  gchar *name;
  gchar *target_type;
  gchar *target;

  /* GSWAT_SESSION_TYPE_LOCAL_RUN */
  /* gint argc; */
  /* gchar **argv; */

  gchar *working_dir;
  GList *environment;

  glong access_time;
};

static void gswat_session_class_init (GSwatSessionClass *klass);
static void gswat_session_get_property (GObject *object,
					guint id,
					GValue *value,
					GParamSpec *pspec);
static void gswat_session_set_property (GObject *object,
					guint property_id,
					const GValue *value,
					GParamSpec *pspec);
/* static void gswat_session_mydoable_interface_init (gpointer interface,
 *						      gpointer data); */
static void gswat_session_init (GSwatSession *self);
static void gswat_session_finalize (GObject *self);


static GObjectClass *parent_class = NULL;
/* static guint gswat_session_signals[LAST_SIGNAL] = { 0 }; */


GType
gswat_session_get_type (void)
{
  static GType self_type = 0;

  if  (!self_type)
    {
      static const GTypeInfo object_info =
	{
	  sizeof (GSwatSessionClass),
	  NULL, /* base class initializer */
	  NULL, /* base class finalizer */
	  (GClassInitFunc)gswat_session_class_init,
	  NULL, /* class finalizer */
	  NULL, /* class data */
	  sizeof (GSwatSession),
	  0, /* preallocated instances */
	  (GInstanceInitFunc)gswat_session_init,
	  NULL /* function table */
	};

      self_type = g_type_register_static (G_TYPE_OBJECT,
					  "GSwatSession",
					  &object_info,
					  0 /* flags */
      );
    }

  return self_type;
}

static void
gswat_session_class_init (GSwatSessionClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GParamSpec *new_param;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = gswat_session_finalize;

  gobject_class->get_property = gswat_session_get_property;
  gobject_class->set_property = gswat_session_set_property;

  new_param = g_param_spec_string ("name",
				   _ ("Session Name"),
				   _ ("The name of the debugging session"),
				   "Session0",
				   GSWAT_PARAM_READWRITE);
  g_object_class_install_property (gobject_class,
				   PROP_NAME,
				   new_param);

  new_param = g_param_spec_string ("target-type",
				   "Target Type",
				   "The type  (Run Local, Run Remote, Serial, "
				   "Core) of target you want to debug",
				   "Run Local",
				   GSWAT_PARAM_READWRITE);
  g_object_class_install_property (gobject_class,
				   PROP_TARGET_TYPE,
				   new_param);

  new_param = g_param_spec_string ("target",
				   _ ("Target Address"),
				   _ ("The address of target you want to "
				      "debug on"),
				   "localhost",
				   GSWAT_PARAM_READWRITE);
  g_object_class_install_property (gobject_class,
				   PROP_TARGET,
				   new_param);

  new_param = g_param_spec_string ("working-dir",
				   _ ("Working Dir"),
				   _ ("The working directory for the program "
				      "being debugged"),
				   NULL,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property (gobject_class,
				   PROP_WORKING_DIR,
				   new_param);

  new_param = g_param_spec_long ("access-time",
				 "Access time",
				 "The last time this session was used "
				 "(seconds since the epoch)",
				 0, /* minimum */
				 G_MAXLONG,
				 0, /* default */
				 GSWAT_PARAM_READWRITE);
  g_object_class_install_property (gobject_class,
				   PROP_ACCESS_TIME,
				   new_param);

  g_type_class_add_private (klass, sizeof (GSwatSessionPrivate));
}

static void
gswat_session_get_property (GObject *object,
			    guint id,
			    GValue *value,
			    GParamSpec *pspec)
{
  GSwatSession* self = GSWAT_SESSION (object);
  char *str = NULL;

  switch (id) {
    case PROP_NAME:
      g_value_set_string (value, self->priv->name);
      break;
    case PROP_TARGET_TYPE:
      g_value_set_string (value, self->priv->target_type);
      break;
    case PROP_TARGET:
      g_value_set_string (value, self->priv->target);
      break;
    case PROP_WORKING_DIR:
      g_value_set_string (value, self->priv->working_dir);
      break;
    case PROP_ACCESS_TIME:
      g_value_set_long (value, self->priv->access_time);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, id, pspec);
      break;
  }

  if (str)
    {
      g_free (str);
    }
}

static void
gswat_session_set_property (GObject *object,
			    guint property_id,
			    const GValue *value,
			    GParamSpec *pspec)
{
  GSwatSession *self = GSWAT_SESSION (object);

  switch (property_id)
    {
    case PROP_NAME:
      gswat_session_set_name (self, g_value_get_string (value));
      break;
    case PROP_TARGET_TYPE:
      gswat_session_set_target_type (self, g_value_get_string (value));
      break;
    case PROP_TARGET:
      gswat_session_set_target (self, g_value_get_string (value));
      break;
    case PROP_WORKING_DIR:
      gswat_session_set_working_dir (self, g_value_get_string (value));
      break;
    case PROP_ACCESS_TIME:
      gswat_session_set_access_time (self, g_value_get_long (value));
      break;
    default:
      g_warning ("gswat_session_set_property on unknown property");
      return;
    }

}

static void
gswat_session_init (GSwatSession *self)
{
  GTimeVal time;

  self->priv = GSWAT_SESSION_GET_PRIVATE (self);

  self->priv->name = g_strdup ("Session0");

  self->priv->target_type = g_strdup ("Run Local");

  self->priv->working_dir = g_get_current_dir ();

  g_get_current_time (&time);
  self->priv->access_time = time.tv_sec;
}

GSwatSession*
gswat_session_new (void)
{
  return GSWAT_SESSION (g_object_new (gswat_session_get_type (), NULL));
}

void
gswat_session_finalize (GObject *object)
{
  GSwatSession* self = GSWAT_SESSION (object);

  if (self->priv->name){
      g_free (self->priv->name);
      self->priv->name = NULL;
  }

  if (self->priv->target_type){
      g_free (self->priv->target_type);
      self->priv->target_type = NULL;
  }

  if (self->priv->target){
      g_free (self->priv->target);
      self->priv->target = NULL;
  }

  if (self->priv->working_dir){
      g_free (self->priv->working_dir);
      self->priv->working_dir = NULL;
  }

  if (self->priv->environment)
    {
      GList *tmp;
      for (tmp=self->priv->environment; tmp!=NULL; tmp=tmp->next)
	{
	  g_free (tmp->data);
	}
      g_list_free (self->priv->environment);
    }
  self->priv->environment = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * gswat_session_get_name:
 * @self:  A GSwatSession.
 *
 * Fetches the name of the GSwatSession.
 *
 * Return value: A newly allocated string containing the
 *               sessions name.
 */
gchar *
gswat_session_get_name (GSwatSession *self)
{
  g_return_val_if_fail (GSWAT_IS_SESSION (self), NULL);

  return g_strdup (self->priv->name);
}

/**
 * gswat_session_set_name:
 * @self:  A GSwatSession.
 * @property:  The session name to set.
 *
 * Sets the name of the session.
 *
 * This will also clear the sessions previous name.
 */
void
gswat_session_set_name (GSwatSession* self, const gchar *name)
{
  g_return_if_fail (GSWAT_IS_SESSION (self));
  g_return_if_fail (name != NULL);

  if (self->priv->name == NULL
      || strcmp (self->priv->name, name) != 0)
    {
      g_free (self->priv->name);
      self->priv->name = g_strdup (name);
      g_object_notify (G_OBJECT (self), "name");
    }
}

gchar *
gswat_session_get_target_type (GSwatSession *self)
{
  g_return_val_if_fail (GSWAT_IS_SESSION (self), NULL);

  return g_strdup (self->priv->target_type);
}

void
gswat_session_set_target_type (GSwatSession *self, const gchar *target_type)
{
  g_return_if_fail (target_type != NULL);

  if (self->priv->target_type == NULL
      || strcmp (self->priv->target_type, target_type) != 0)
    {
      g_free (self->priv->target_type);
      self->priv->target_type = g_strdup (target_type);
      g_object_notify (G_OBJECT (self), "target-type");
    }
}

#if 0
static GSwatSessionType
session_type_strtoui (const gchar *session_type)
{
  if (strcmp (session_type, "GSWAT_SESSION_TYPE_LOCAL_RUN")==0)
    return 0;
  if (strcmp (session_type, "GSWAT_SESSION_TYPE_LOCAL_PID")==0)
    return 1;
  if (strcmp (session_type, "GSWAT_SESSION_TYPE_REMOTE_RUN")==0)
    return 2;
  if (strcmp (session_type, "GSWAT_SESSION_TYPE_REMOTE_PID")==0)
    return 3;
  g_warning ("Unknown session type  (%s)", session_type);
  return 0;
}

static const gchar *
session_type_uitostr (GSwatSessionType session_type)
{
  const gchar *type[] = {
      "GSWAT_SESSION_TYPE_LOCAL_RUN",
      "GSWAT_SESSION_TYPE_LOCAL_PID",
      "GSWAT_SESSION_TYPE_REMOTE_RUN",
      "GSWAT_SESSION_TYPE_REMOTE_PID"
  };
  return type[session_type];
}
#endif

/**
 * gswat_session_get_target:
 * @self:  A MyObject.
 *
 * Fetches the target of the MyObject.
 *
 * Returns: The sessions target name.
 */
gchar *
gswat_session_get_target (GSwatSession *self)
{
  g_return_val_if_fail (GSWAT_IS_SESSION (self), NULL);

  return g_strdup (self->priv->target);
}

/**
 * gswat_session_set_target:
 * @self:  A GSwatSession.
 * @property:  The target name to set.
 *
 * Sets the sessions target name.
 *
 * This will also clear the properties previous value.
 */
void
gswat_session_set_target (GSwatSession *self, const gchar *target)
{
  g_return_if_fail (GSWAT_IS_SESSION (self));

  if (self->priv->target == NULL
      || strcmp (self->priv->target, target) != 0)
    {
      g_free (self->priv->target);
      self->priv->target = g_strdup (target);
      g_object_notify (G_OBJECT (self), "target");
    }
}

gchar *
gswat_session_get_working_dir (GSwatSession* self)
{
  g_return_val_if_fail (GSWAT_IS_SESSION (self), NULL);

  return g_strdup (self->priv->working_dir);
}

void
gswat_session_set_working_dir (GSwatSession* self, const gchar *working_dir)
{
  g_assert (working_dir != NULL);

  if (self->priv->working_dir == NULL ||
      strcmp (working_dir, self->priv->working_dir) != 0)
    {
      g_free (self->priv->working_dir);
      self->priv->working_dir = g_strdup (working_dir);
      g_object_notify (G_OBJECT (self), "working-dir");
    }
}

void
gswat_session_add_environment_variable (GSwatSession *self,
					const gchar *name,
					const gchar *value)
{
  GSwatSessionEnvironmentVariable *variable;

  variable = g_new (GSwatSessionEnvironmentVariable, 1);
  variable->name = g_strdup (name);
  variable->value = g_strdup (value);

  self->priv->environment =
    g_list_prepend (self->priv->environment, variable);
}

void
gswat_session_delete_environment_variable (GSwatSession *self,
					   const gchar *name)
{
  GList *tmp;

  for (tmp=self->priv->environment; tmp!=NULL; tmp=tmp->next)
    {
      GSwatSessionEnvironmentVariable *current_variable = tmp->data;

      if (strcmp (current_variable->name, name) == 0)
	{
	  self->priv->environment =
	    g_list_remove (self->priv->environment, current_variable);
	  g_free (current_variable);
	  return;
	}
    }
}

GList *
gswat_session_get_environment (GSwatSession *self)
{
  GList *tmp;
  GList *environment_copy=NULL;

  for (tmp=self->priv->environment; tmp!=NULL; tmp=tmp->next)
    {
      GSwatSessionEnvironmentVariable *new_variable;

      new_variable = g_new (GSwatSessionEnvironmentVariable, 1);
      memcpy (new_variable,
	      tmp->data,
	      sizeof (GSwatSessionEnvironmentVariable));
      environment_copy =
	g_list_prepend (environment_copy, new_variable);
    }

  return environment_copy;
}

void
gswat_session_free_environment (GList *environment)
{
  GList *tmp;
  for (tmp=environment; tmp!=NULL; tmp=tmp->next)
    {
      g_free (tmp->data);
    }
  g_list_free (environment);
}

void
gswat_session_clear_environment (GSwatSession *self)
{
  gswat_session_free_environment (self->priv->environment);
  self->priv->environment = NULL;
}

glong
gswat_session_get_access_time (GSwatSession *self)
{
  return self->priv->access_time;
}

void
gswat_session_set_access_time (GSwatSession *self, glong access_time)
{
  self->priv->access_time = access_time;
  g_object_notify (G_OBJECT (self), "access-time");
}

