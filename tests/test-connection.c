#include <stdio.h>
#include <gswat/gswat.h>

int
main(int argc, char **argv)
{
    GSwatSession *session;
    GSwatDebuggable *debuggable;
    GError *error = NULL;
    
    printf("Hello, World!\n");
    
    gswat_init();

    session = gswat_session_new();
    gswat_session_set_name(session, "Test0");
    gswat_session_set_target_type(session, "Run Local");
    gswat_session_set_target(session, "./test");
    
    debuggable = GSWAT_DEBUGGABLE(gswat_gdb_debugger_new(session));
    if (!gswat_debuggable_connect(debuggable, &error))
    {
	g_print ("Failed to connect to target: %s", error->message);
	return 1;
    }

    gswat_debuggable_next(debuggable);
    gswat_debuggable_step(debuggable);

    gswat_debuggable_continue(debuggable);
    //gswat_debuggable_disconnect(debuggable);
    
    return 0;
}

