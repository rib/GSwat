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
#include "gswat-spawn.h"
#include "gswat-session.h"



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
    if(pid != -1 || remaining_args != NULL)
    {
        session = gswat_session_new();
        gswat_session_set_pid(session, pid);
        
        /* fixme support parsing arguments to a program
         * on the command line
         */
        gswat_session_set_command(session, remaining_args[0]);
    }
    
    rb_file_helpers_init();

    /* Normal case: display GUI */
    main_window = gswat_window_new(session);
    
    if (remaining_args != NULL) {
   		g_strfreev (remaining_args);
		remaining_args = NULL;
	}


    gtk_main();

    if(session)
    {
        g_object_unref(session);
    }

    g_object_unref(main_window);

    rb_file_helpers_shutdown();

    return 0;
}


