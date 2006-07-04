#include <locale.h>
#include <argp.h>
#include <pthread.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "global.h"
#include "signals.h"
#include "interface.h"
#include "simulator.h"

/* argp setup */
const char *argp_program_version = PACKAGE_STRING;
const char *argp_program_bug_address = "http://bugzilla.sixbynine.org";

static const char doc[] = "A neat gnome debugger idea";

struct argp _ogo_argp = { 
    NULL, // Specifies what options we directly understand here
    NULL, // Function to handle the parsed options
    NULL, // Usage string
    doc,  // Extra doc to go with --help
    NULL, // Children parsers (as provided by modules)
    NULL  // The domain used to translate argp library strings
};



int
main(int argc, char **argv)
{
    char *loc;
    pthread_t simulator_thread;
    int ret;
    

    /* set the locale */
    loc=setlocale(LC_ALL, "");
    if (loc==NULL ){
        WARN("Was not able to correctly set locale");
    }

    /* parse command line options */
    argp_parse(&_ogo_argp, argc, argv, 0, 0, NULL);


    gtk_init(&argc, &argv);
    
    GladeXML *xml;

    /* load the main window (which is named app1) */
    xml = glade_xml_new("ggdb.glade", NULL, NULL);

    /* in case we can't load the interface, bail */
    if(!xml) {
        g_warning("We could not load the interface!");
        return 1;
    } 

    
    glade_xml_signal_autoconnect(xml);
    
    /* FIXME free xml data */

    
    DEBUG("Initialisation finished");

    DEBUG("Entering main even loop");

    gtk_main();
}




void
gg_deinit(void)
{
    DEBUG("Bye Bye..");
}

