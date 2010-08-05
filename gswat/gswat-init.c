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
#include <config.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <config.h>

#include "gswat-debug.h"

#ifdef GSWAT_ENABLE_DEBUG
static const GDebugKey gswat_debug_keys[] = {
      { "log", GSWAT_DEBUG_LOG },
      { "misc", GSWAT_DEBUG_MISC },
      { "gdb-trace", GSWAT_DEBUG_GDB_TRACE },
      { "gdbmi", GSWAT_DEBUG_GDBMI },
};

static const gint n_gswat_debug_keys = G_N_ELEMENTS (gswat_debug_keys);
#endif /* GSWAT_ENABLE_DEBUG */

guint gswat_debug_flags;
int gswat_log_file;

#ifdef GSWAT_ENABLE_DEBUG
static gboolean
gswat_arg_debug_cb (const char *key,
		    const char *value,
		    gpointer user_data)
{
  gswat_debug_flags |=
    g_parse_debug_string (value,
			  gswat_debug_keys,
			  n_gswat_debug_keys);
  return TRUE;
}

static gboolean
gswat_arg_no_debug_cb (const char *key,
		       const char *value,
		       gpointer user_data)
{
  gswat_debug_flags &=
    ~g_parse_debug_string (value,
			   gswat_debug_keys,
			   n_gswat_debug_keys);
  return TRUE;
}
#endif /* CLUTTER_DEBUG */

static GOptionEntry gswat_args[] = {
#ifdef GSWAT_ENABLE_DEBUG
      { "gswat-debug", 0, 0, G_OPTION_ARG_CALLBACK, gswat_arg_debug_cb,
	N_("GSwat debugging flags to set"), "FLAGS" },
      { "gswat-no-debug", 0, 0, G_OPTION_ARG_CALLBACK, gswat_arg_no_debug_cb,
	N_("GSwat debugging flags to unset"), "FLAGS" },
#endif /* GSWAT_ENABLE_DEBUG */
      { NULL, },
};

static gboolean
pre_parse_hook (GOptionContext *context,
		GOptionGroup *group,
		gpointer data,
		GError **error)
{
#ifdef GSWAT_ENABLE_DEBUG
  const char *env = g_getenv ("GSWAT_DEBUG");
  if (env)
    {
      gswat_debug_flags =
	g_parse_debug_string (env, gswat_debug_keys, n_gswat_debug_keys);
    }
#endif /* GSWAT_ENABLE_DEBUG */

  return TRUE;
}

static void
parse_args (int *argc, char ***argv)
{
  GOptionContext *option_context;
  GOptionGroup *group;
  GError *error = NULL;

  option_context = g_option_context_new (NULL);
  g_option_context_set_ignore_unknown_options (option_context, TRUE);
  g_option_context_set_help_enabled (option_context, FALSE);

  group = g_option_group_new ("gswat",
			      _("GSwat Options"),
			      _("Show GSwat options"),
			      NULL, NULL);

  g_option_group_set_parse_hooks (group, pre_parse_hook, NULL);
  g_option_group_add_entries (group, gswat_args);
  g_option_group_set_translation_domain (group, GETTEXT_PACKAGE);

  g_option_context_set_main_group (option_context, group);

  if (!g_option_context_parse (option_context, argc, argv, &error))
    {
      if (error)
	{
	  g_warning ("%s", error->message);
	  g_error_free (error);
	}
    }

  g_option_context_free (option_context);
}

void
gswat_log (const char *message)
{
  ssize_t remaining = strlen (message);
  const char *p = message;

  if (!(gswat_debug_flags & GSWAT_DEBUG_LOG)
      || gswat_log_file == -1)
    return;

  while (remaining)
    {
      ssize_t count = write (gswat_log_file, p, remaining);
      if (count == -1)
        {
          if (errno == EAGAIN || errno == EINTR)
            continue;
          else
            break;
        }
      remaining -= count;
      p += count;
    }
}

/* This lets us log in a way that can be combined with a gdb trace by
 * logging everything as a comment */
static void
gswat_log_handler (const char *log_domain,
		   GLogLevelFlags log_level,
		   const char *message,
		   gpointer data)
{
  const char *prefix;
  char **lines = g_strsplit (message, "\n", -1);
  int i;

  switch (log_level)
    {
    case G_LOG_LEVEL_ERROR:
      prefix = "# [ERROR] ";
      break;
    case G_LOG_LEVEL_CRITICAL:
      prefix = "# [CRITICAL] ";
      break;
    case G_LOG_LEVEL_WARNING:
      prefix = "# [warning] ";
      break;
    default:
      prefix = "# ";
      break;
    }

  for (i = 0; lines[i]; i++)
    {
      gswat_log (prefix);
      gswat_log (lines[i]);
      gswat_log ("\n");
    }

  g_strfreev (lines);
}

static void
init_logging (void)
{
  gswat_log_file =
    open ("gswat.log", O_CREAT|O_TRUNC|O_WRONLY|O_APPEND, 0644);
  if (gswat_log_file != -1)
    g_log_set_default_handler (gswat_log_handler, NULL);
}

void
gswat_init (int *argc, char ***argv)
{
  static gboolean initialised = FALSE;

  if (initialised)
    return;

  parse_args (argc, argv);

  if (gswat_debug_flags & GSWAT_DEBUG_LOG)
    init_logging ();

  g_type_init ();

  initialised = TRUE;
}

