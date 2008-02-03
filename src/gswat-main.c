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

#include <gnome.h>
#include "rb-file-helpers.h"

#include <config.h>

#include "gedit-prefs-manager.h"
#include "gswat-window.h"
#include "gswat-session.h"


extern void _gswat_gui_set_application_window(GSwatWindow *new_application_window);

static gint pid = -1;

static GSwatSession *session = NULL;
static GSwatWindow *main_window = NULL;

int
main(int argc, char **argv)
{
    GnomeProgram *program = NULL;
    GOptionContext *option_context;
    gchar **remaining_args = NULL;

    GOptionEntry option_entries[] = {

	/* ... your application's command line options go here ... */

	{ "pid",
	    0,
	    G_OPTION_FLAG_OPTIONAL_ARG,
	    G_OPTION_ARG_INT,
	    &pid,
	    "A private mechanism!", NULL },

	/* last but not least a special option that collects filenames */
	{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY,
	    &remaining_args,
	    "Special option that collects any remaining arguments for us" },
	{ NULL }
    };

    option_context = g_option_context_new("gswat");

    /* if you are using any libraries that have command line options
     * of their own and provide a GOptionGroup with them, you can
     * add them here as well using g_option_context_add_group()
     */

    /* now add our own application's command-line options. If you
     * are using gettext for translations, you should be using
     * GETTEXT_PACKAGE here instead of NULL
     */
    g_option_context_add_main_entries(option_context, option_entries, NULL);


    program = gnome_program_init(PACKAGE, VERSION,
				 LIBGNOMEUI_MODULE, argc, argv,
				 GNOME_PARAM_GOPTION_CONTEXT, option_context,
				 GNOME_PARAM_NONE);

    bindtextdomain(GETTEXT_PACKAGE, GSWAT_LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    if(!gedit_prefs_manager_init())
    {
	g_critical("gedit_prefs_manager_init failed\n");
    }

    /* parse remaining command-line arguments that are not
     * options (e.g. filenames or URIs or whatever), if any
     */
#if 0
    if (remaining_args != NULL) {
	gint i, num_args;

	num_args = g_strv_length (remaining_args);
	for (i = 0; i < num_args; ++i) {
	    /* process remaining_args[i] here */
	}
	g_strfreev (remaining_args);
	remaining_args = NULL;
    }
#endif


    /* First we see if the user has described a new session
     * on the command line
     */
    if(pid != -1)
    {
	gchar *target;
	if(!remaining_args)
	{
	    g_message("%s",
		      g_option_context_get_help(option_context, TRUE, NULL));
	    exit(1);
	}
	session = gswat_session_new();
	gswat_session_set_target_type(session, "PID Local");
	target=g_strdup_printf("pid=%d file=%s", pid, remaining_args[0]);
	gswat_session_set_target(session, target);
	g_free(target);
    }
    else if(remaining_args != NULL)
    {
	int i;
	GString *target;
	session = gswat_session_new();
	gswat_session_set_target_type(session, "Run Local");
	target = g_string_new(remaining_args[0]);
	for(i=1; remaining_args[i] != NULL; i++)
	{
	    g_string_append_printf(target, " %s",
				   g_shell_quote(remaining_args[i]));
	}
	gswat_session_set_target(session, target->str);
	g_string_free(target, TRUE);
    }


    rb_file_helpers_init();

    /* Normal case: display GUI */
    main_window = gswat_window_new(session);
    if(session)
    {
	g_object_unref(session);
    }
    _gswat_gui_set_application_window(main_window);

    if (remaining_args != NULL) {
	g_strfreev (remaining_args);
	remaining_args = NULL;
    }
    
    gtk_main();

    g_object_unref(main_window);

    rb_file_helpers_shutdown();

    return 0;
}


