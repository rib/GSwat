#include <gnome.h>

#include <config.h>

#include "gswat-main-window.h"
#include "gswat-spawn.h"
#include "gswat-session.h"



static gint pid = -1;

static GSwatSession *session = NULL;
static GSwatWindow *main_window = NULL;

int
main(int argc, char **argv)
{
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

	GOptionContext *option_context;
	GnomeProgram *my_app;

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

	my_app = gnome_program_init(PACKAGE, VERSION,
	                            LIBGNOMEUI_MODULE, argc, argv,
	                            GNOME_PARAM_GOPTION_CONTEXT, option_context,
	                            GNOME_PARAM_NONE);
    
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


    return 0;
}


