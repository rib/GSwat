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

#include <string.h>
#include <libxml/xmlreader.h>

#include "gswat-session.h"
#include "gswat-session-manager.h"

#define GSWAT_SESSION_MANAGER_GET_PRIVATE(object) \
  (G_TYPE_INSTANCE_GET_PRIVATE((object), \
			       GSWAT_TYPE_SESSION_MANAGER, \
			       GSwatSessionManagerPrivate))

#if 0
enum {
    SIGNAL_NAME,
    LAST_SIGNAL
};
#endif

#if 0
enum {
    PROP_0,
    PROP_NAME,
};
#endif

struct _GSwatSessionManagerPrivate
{
  GList *sessions;
  gchar *sessions_filename;
};


static void gswat_session_manager_class_init (GSwatSessionManagerClass *klass);
static void gswat_session_manager_get_property (GObject *object,
						guint id,
						GValue *value,
						GParamSpec *pspec);
static void gswat_session_manager_set_property (GObject *object,
						guint property_id,
						const GValue *value,
						GParamSpec *pspec);
/* static void gswat_session_manager_mydoable_interface_init (gpointer interface,
   gpointer data); */
static void gswat_session_manager_init (GSwatSessionManager *self);
static void gswat_session_manager_finalize (GObject *self);

static GSwatSession *parse_session (xmlDocPtr doc, xmlNodePtr cur);
static void save_session (xmlNodePtr parent,
			  GSwatSession *session);
static void on_gswat_session_name_changed (GObject *object,
					   GParamSpec *property,
					   gpointer data);
#if 0
static void on_gswat_session_target_type_changed (GObject *object,
						  GParamSpec *property,
						  gpointer data);
static void on_gswat_session_target_changed (GObject *object,
					     GParamSpec *property,
					     gpointer data);
static void on_gswat_session_working_dir_changed (GObject *object,
						  GParamSpec *property,
						  gpointer data);
static void on_gswat_session_environment_changed (GObject *object,
						  GParamSpec *property,
						  gpointer data);
#endif

static GObjectClass *parent_class = NULL;
/* static guint gswat_session_manager_signals[LAST_SIGNAL] = { 0 }; */

GType
gswat_session_manager_get_type (void)
{
  static GType self_type = 0;

  if  (!self_type)
    {
      static const GTypeInfo object_info =
	{
	  sizeof (GSwatSessionManagerClass),
	  NULL, /* base class initializer */
	  NULL, /* base class finalizer */
	  (GClassInitFunc)gswat_session_manager_class_init,
	  NULL, /* class finalizer */
	  NULL, /* class data */
	  sizeof (GSwatSessionManager),
	  0, /* preallocated instances */
	  (GInstanceInitFunc)gswat_session_manager_init,
	  NULL /* function table */
	};

      self_type = g_type_register_static (G_TYPE_OBJECT,
					  "GSwatSessionManager",
					  &object_info,
					  0 /* flags */);
    }

  return self_type;
}

static void
gswat_session_manager_class_init (GSwatSessionManagerClass *klass) /* Class Initialization */
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  /* GParamSpec *new_param; */

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = gswat_session_manager_finalize;

  gobject_class->get_property = gswat_session_manager_get_property;
  gobject_class->set_property = gswat_session_manager_set_property;

  g_type_class_add_private (klass, sizeof (GSwatSessionManagerPrivate));
}

static void
gswat_session_manager_get_property (GObject *object,
				    guint id,
				    GValue *value,
				    GParamSpec *pspec)
{
  /* GSwatSessionManager* self = GSWAT_SESSION_MANAGER (object); */

  switch (id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, id, pspec);
      break;
  }
}

static void
gswat_session_manager_set_property (GObject *object,
				    guint property_id,
				    const GValue *value,
				    GParamSpec *pspec)
{
  /* GSwatSessionManager* self = GSWAT_SESSION_MANAGER (object); */

  switch (property_id)
    {
    default:
      g_warning ("gswat_session_manager_set_property on unknown property");
      return;
    }
}

static void
gswat_session_manager_init (GSwatSessionManager *self)
{
  self->priv = GSWAT_SESSION_MANAGER_GET_PRIVATE (self);
  /* populate your object here */
}

GSwatSessionManager*
gswat_session_manager_new (void)
{
  return GSWAT_SESSION_MANAGER (g_object_new (gswat_session_manager_get_type (), NULL));
}

void
gswat_session_manager_finalize (GObject *object)
{
  GSwatSessionManager *self = GSWAT_SESSION_MANAGER (object);
  GList *tmp;

  for (tmp=self->priv->sessions; tmp!=NULL; tmp=tmp->next)
    {
      g_object_unref (tmp->data);
    }
  g_list_free (self->priv->sessions);

  g_free (self->priv->sessions_filename);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
gswat_session_manager_set_sessions_file (GSwatSessionManager *self,
					 const gchar *filename)
{
  g_free (self->priv->sessions_filename);

  self->priv->sessions_filename = g_strdup (filename);
}

/* Read in all the users saved sessions */
gboolean
gswat_session_manager_read_sessions_file (GSwatSessionManager *self)
{
  xmlDocPtr doc;
  xmlNodePtr cur;

  /* FIXME: file locking */
  if  (!g_file_test  (self->priv->sessions_filename, G_FILE_TEST_EXISTS))
    {
      g_warning ("Failed to open saved sessions file: %s\n", self->priv->sessions_filename);
      return FALSE;
    }

  doc = xmlParseFile (self->priv->sessions_filename);
  if (doc == NULL)
    {
      g_warning ("Failed to parse saved sessions file: %s\n", self->priv->sessions_filename);
      return FALSE;
    }

  cur = xmlDocGetRootElement (doc);
  if (cur == NULL)
    {
      g_message  ("The sessions file '%s' is empty", self->priv->sessions_filename);
      xmlFreeDoc  (doc);

      return FALSE;
    }

  if (xmlStrcmp  (cur->name,  (const xmlChar *) "sessions"))
    {
      g_message  ("File '%s' is of the wrong type", self->priv->sessions_filename);
      xmlFreeDoc  (doc);

      return FALSE;
    }

  cur = xmlDocGetRootElement (doc);
  cur = cur->xmlChildrenNode;

  g_message ("read_sessions");

  /* FIXME take sessions mutex */

  while  (cur != NULL)
    {
      GSwatSession *next_session = parse_session (doc, cur);
      if (next_session)
	{
	  gswat_session_manager_add_session (self, next_session);
	}

      cur = cur->next;
    }

  /* FIXME release sessions mutex */

  xmlFreeDoc (doc);

  return TRUE;
}

GSwatSession *
gswat_session_manager_find_session (GSwatSessionManager *self, const gchar *session_name)
{
  GList *tmp;

  if (!session_name)
    {
      return NULL;
    }

  for (tmp=self->priv->sessions; tmp!=NULL; tmp=tmp->next)
    {
      GSwatSession *current_session =  (GSwatSession *)tmp->data;
      if (strcmp (gswat_session_get_name (current_session), session_name) == 0)
	{
	  return current_session;
	}
    }
  return NULL;
}

void
gswat_session_manager_add_session (GSwatSessionManager *self,
				   GSwatSession *session)
{
  GSwatSession *clashing_session;
  const gchar *new_session_name;

  g_object_ref (session);

  new_session_name = gswat_session_get_name (session);

  clashing_session =
    gswat_session_manager_find_session (self, new_session_name);
  if (clashing_session)
    {
      gswat_session_manager_remove_session (self, clashing_session);
    }
  self->priv->sessions = g_list_prepend (self->priv->sessions,
					 session);

  g_signal_connect (session,
		    "notify::name",
		    G_CALLBACK (on_gswat_session_name_changed),
		    self);
#if 0
  g_signal_connect (session,
		    "notify::target-type",
		    G_CALLBACK (on_gswat_session_target_type_changed),
		    self);
  g_signal_connect (session,
		    "notify::target",
		    G_CALLBACK (on_gswat_session_target_changed),
		    self);
  g_signal_connect (session,
		    "notify::working-dir",
		    G_CALLBACK (on_gswat_session_working_dir_changed),
		    self);
  g_signal_connect (session,
		    "notify::environment",
		    G_CALLBACK (on_gswat_session_environment_changed),
		    self);
#endif
}

void
gswat_session_manager_remove_session (GSwatSessionManager *self,
				      GSwatSession *session)
{
  self->priv->sessions =
    g_list_remove (self->priv->sessions, session);
  g_object_unref (session);
}

GList *
gswat_session_manager_get_sessions (GSwatSessionManager *self)
{
  GList *sessions;
  GList *tmp;

  sessions = g_list_copy (self->priv->sessions);
  for (tmp=sessions; tmp!=NULL; tmp=tmp->next)
    {
      g_object_ref (tmp->data);
    }
  return sessions;
}

void
gswat_session_manager_unref_sessions (GList *sessions)
{
  GList *tmp;

  for (tmp=sessions; tmp!=NULL; tmp=tmp->next)
    {
      g_object_unref (tmp->data);
    }
  g_list_free (sessions);
}

static GSwatSession *
parse_session (xmlDocPtr doc, xmlNodePtr cur)
{
  GSwatSession *session;
  xmlChar *name, *target_type, *target, *working_dir;
  xmlChar *access_time;
  gboolean name_set=FALSE, target_type_set=FALSE;
  gboolean target_set=FALSE, working_dir_set=FALSE;
  gboolean access_time_set=FALSE;

  g_message ("parse_session");

  /* ignore blank space */
  if (xmlIsBlankNode (cur))
    {
      return NULL;
    }

  if (xmlStrcmp (cur->name,  (const xmlChar *)"session") != 0)
    {
      g_warning ("missing \"session\" tag");
      return NULL;
    }

  session = gswat_session_new ();

  for (cur=cur->xmlChildrenNode; cur!=NULL; cur = cur->next)
    {
      if (xmlIsBlankNode (cur))
	{
	  continue;
	}

      if (xmlStrcmp (cur->name,  (const xmlChar *)"name") == 0)
	{
	  name = xmlGetProp (cur,  (const xmlChar *)"value");
	  if (!name){
	      goto parse_error;
	  }

	  gswat_session_set_name (session,  (gchar *)name);
	  xmlFree (name);
	  name_set = TRUE;
	}
      if (xmlStrcmp (cur->name,  (const xmlChar *)"target_type") == 0)
	{
	  target_type = xmlGetProp (cur,  (const xmlChar *)"value");
	  if (!target_type){
	      goto parse_error;
	  }

	  gswat_session_set_target_type (session,  (gchar *)target_type);
	  xmlFree (target_type);
	  target_type_set = TRUE;
	}
      if (xmlStrcmp (cur->name,  (const xmlChar *)"target") == 0)
	{
	  target = xmlGetProp (cur,  (const xmlChar *)"value");
	  if (!target){
	      goto parse_error;
	  }

	  gswat_session_set_target (session,  (gchar *)target);
	  xmlFree (target);
	  target_set = TRUE;
	}
      if (xmlStrcmp (cur->name,  (const xmlChar *)"working_dir") == 0)
	{
	  working_dir = xmlGetProp (cur,  (const xmlChar *)"value");
	  if (!working_dir){
	      goto parse_error;
	  }

	  gswat_session_set_working_dir (session,  (gchar *)working_dir);
	  xmlFree (working_dir);
	  working_dir_set = TRUE;
	}
      if (xmlStrcmp (cur->name,  (const xmlChar *)"access_time") == 0)
	{
	  access_time = xmlGetProp (cur,  (const xmlChar *)"value");
	  if (!access_time)
	    {
	      goto parse_error;
	    }

	  gswat_session_set_access_time (session,
					 strtol((char *)access_time, NULL, 10));
	  xmlFree (access_time);
	  access_time_set = TRUE;
	}
    }

  if (!name_set || !target_type_set || !target_set
      || !working_dir_set || !access_time_set)
    {
      goto parse_error;
    }

  g_message ("Read in session with name=%s", gswat_session_get_name (session));

  return session;

parse_error:

  g_warning ("session parse error");

  g_object_unref (session);
  return NULL;
}

static void
save_session (xmlNodePtr parent, GSwatSession *session)
{
  xmlNodePtr session_node;
  xmlNodePtr name_node;
  xmlNodePtr target_type_node;
  xmlNodePtr target_node;
  xmlNodePtr working_dir_node;
  xmlNodePtr access_time_node;
  xmlNodePtr environment_node;
  xmlNodePtr variable_node;
  GList *tmp;
  GList *environment;
  gchar *access_time_str;

  session_node = xmlNewChild (parent, NULL,  (const xmlChar *)"session", NULL);


  name_node = xmlNewChild (session_node, NULL, (const xmlChar *)"name", NULL);
  xmlSetProp (name_node,
	      (const xmlChar *)"value",
	      (const xmlChar *)gswat_session_get_name (session));

  target_type_node = xmlNewChild (session_node, NULL,
				  (const xmlChar *)"target_type", NULL);
  xmlSetProp (target_type_node,
	      (const xmlChar *)"value",
	      (const xmlChar *)gswat_session_get_target_type (session));

  target_node = xmlNewChild (session_node, NULL,
			     (const xmlChar *)"target", NULL);
  xmlSetProp (target_node,
	      (const xmlChar *)"value",
	      (const xmlChar *)gswat_session_get_target (session));

  working_dir_node = xmlNewChild (session_node, NULL,
				  (const xmlChar *)"working_dir", NULL);
  xmlSetProp (working_dir_node,
	      (const xmlChar *)"value",
	      (const xmlChar *)gswat_session_get_working_dir (session));

  access_time_str =
    g_strdup_printf ("%ld", gswat_session_get_access_time (session));
  access_time_node = xmlNewChild (session_node, NULL,
				  (const xmlChar *)"access_time", NULL);
  xmlSetProp (access_time_node,
	      (const xmlChar *)"value",
	      (const xmlChar *)access_time_str);
  g_free (access_time_str);

  environment = gswat_session_get_environment (session);
  if (environment)
    {
      environment_node = xmlNewChild (session_node, NULL,
				      (const xmlChar *)"environment",
				      NULL);

      for (tmp=environment; tmp!=NULL; tmp=tmp->next)
	{
	  GSwatSessionEnvironmentVariable *current_variable=tmp->data;
	  variable_node = xmlNewChild (environment_node, NULL,
				       (const xmlChar *)"evironment_variable",
				       NULL);
	  xmlSetProp (variable_node,
		      (const xmlChar *)"name",
		      (const xmlChar *)current_variable->name);
	  xmlSetProp (variable_node,
		      (const xmlChar *)"value",
		      (const xmlChar *)current_variable->value);
	}
    }
  gswat_session_free_environment (environment);
}

static void
on_gswat_session_name_changed (GObject *object,
			       GParamSpec *property,
			       gpointer data)
{
  GSwatSessionManager *self = data;
  GList *tmp;
  GSwatSession *session = GSWAT_SESSION (object);


  /* If there is already a session being managed with the same name then
   * remove it */
  for (tmp=self->priv->sessions; tmp!=NULL; tmp=tmp->next)
    {
      GSwatSession *current_session = tmp->data;
      if (current_session != session &&
	  strcmp (gswat_session_get_name (current_session),
		  gswat_session_get_name (session)) == 0
      )
	{
	  gswat_session_manager_remove_session (self, current_session);
	  break;
	}
    }
}

#if 0
static void
on_gswat_session_target_type_changed (GObject *object,
				      GParamSpec *property,
				      gpointer data)
{
  GSwatSessionManager *self = data;
  GList *tmp;
  GSwatSession *session = GSWAT_SESSION (object);


}

static void
on_gswat_session_target_changed (GObject *object,
				 GParamSpec *property,
				 gpointer data)
{
  GSwatSessionManager *self = data;
  GList *tmp;
  GSwatSession *session = GSWAT_SESSION (object);


}

static void
on_gswat_session_working_dir_changed (GObject *object,
				      GParamSpec *property,
				      gpointer data)
{
  GSwatSessionManager *self = data;
  GList *tmp;
  GSwatSession *session = GSWAT_SESSION (object);


}

static void
on_gswat_session_environment_changed (GObject *object,
				      GParamSpec *property,
				      gpointer data)
{
  GSwatSessionManager *self = data;
  GList *tmp;
  GSwatSession *session = GSWAT_SESSION (object);


}
#endif

void
gswat_session_manager_save_sessions (GSwatSessionManager *self)
{
  xmlDocPtr doc;
  xmlNodePtr root;
  GList *tmp;
  gchar *file_name;
  gchar *tmp_file;

  doc = xmlNewDoc ( (const xmlChar *)"1.0");
  if  (doc == NULL)
    {
      return;
    }

  /* Create metadata root */
  root = xmlNewDocNode (doc, NULL,  (const xmlChar *)"sessions", NULL);
  xmlDocSetRootElement (doc, root);

  for (tmp=self->priv->sessions; tmp!=NULL; tmp=tmp->next)
    {
      GSwatSession *current_session = tmp->data;
      save_session (root, current_session);
    }

  file_name = self->priv->sessions_filename;
  tmp_file = g_strdup_printf ("%sXXXXXX", file_name);

  xmlSaveFormatFile (tmp_file, doc, 1);

  if (rename (tmp_file, file_name) == -1)
    {
      g_warning ("Failed to save session state to %s", file_name);
    }
  g_free (tmp_file);

  xmlFreeDoc (doc);
}

