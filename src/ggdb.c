#include <gnome.h>

#include "global.h"
#include "gg_main_window.h"



int
main(int argc, char **argv)
{
    gchar **remaining_args = NULL;

	GOptionEntry option_entries[] = {
		/* ... your application's command line options go here ... */
		/* last but not least a special option that collects filenames */
		{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY,
		  &remaining_args,
		  "Special option that collects any remaining arguments for us" },
		{ NULL }
	};

	GOptionContext *option_context;
	GnomeProgram *my_app;

	option_context = g_option_context_new("ggdb");

	/* if you are using any libraries that have command line options
	 * of their own and provide a GOptionGroup with them, you can
	 * add them here as well using g_option_context_add_group()
	 */

	/* now add our own application's command-line options. If you
	 * are using gettext for translations, you should be using
	 * GETTEXT_PACKAGE here instead of NULL
	 */
	g_option_context_add_main_entries (option_context, option_entries, NULL);

	my_app = gnome_program_init(PACKAGE, VERSION,
	                            LIBGNOMEUI_MODULE, argc, argv,
	                            GNOME_PARAM_GOPTION_CONTEXT, option_context,
	                            GNOME_PARAM_NONE);
    
	/* parse remaining command-line arguments that are not
	 * options (e.g. filenames or URIs or whatever), if any
	 */
	if (remaining_args != NULL) {
	    gint i, num_args;

		num_args = g_strv_length (remaining_args);
		for (i = 0; i < num_args; ++i) {
			/* process remaining_args[i] here */
		}
		g_strfreev (remaining_args);
		remaining_args = NULL;
	}


    gg_main_window_init("ggdb.glade");

    gtk_main();
}


