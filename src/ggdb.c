#include <gnome.h>
#include <glade/glade.h>

#include "global.h"
#include "gdb.h"



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

	option_context = g_option_context_new ("ggdb");

	/* if you are using any libraries that have command line options
	 * of their own and provide a GOptionGroup with them, you can
	 * add them here as well using g_option_context_add_group()
	 */

	/* now add our own application's command-line options. If you
	 * are using gettext for translations, you should be using
	 * GETTEXT_PACKAGE here instead of NULL
	 */
	g_option_context_add_main_entries (option_context, option_entries, NULL);

	/* We assume PACKAGE and VERSION are set to the program name and version
	 * number respectively. Also, assume that 'option_entries' is a global
	 * array of GOptionEntry structures.
	 */
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


    
    GladeXML *xml;

    /* load the main window (which is named app1) */
    xml = glade_xml_new("ggdb.glade", "gg_main_window", NULL);

    /* in case we can't load the interface, bail */
    if(!xml) {
        g_warning("We could not load the interface!");
        return 1;
    } 

    
    glade_xml_signal_autoconnect(xml);
    
    g_object_unref(xml);


    DEBUG("Initialisation finished");

    DEBUG("Entering main even loop");

    gg_gdb_connect(); 
    
    gg_gdb_send_command();

    gtk_main();
}




void
gg_deinit(void)
{
    DEBUG("Bye Bye..");
}

