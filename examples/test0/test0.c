#include <stdio.h>
#include <gswat.h>

int
main(int argc, char **argv)
{
    GSwatSession *session;
    GSwatDebuggable *debuggable;
    
    printf("Hello, World!\n");
    
    gswat_init();

    session = gswat_session_new();
    gswat_session_set_name(session, "Test0");
    gswat_session_set_target_type(session, "Run Local");
    gswat_session_set_target(session, "./test");
    
    debuggable = GSWAT_DEBUGGABLE(gswat_gdb_debugger_new(session));
    gswat_debuggable_connect(debuggable, NULL);

    gswat_debuggable_next(debuggable);
    gswat_debuggable_step(debuggable);

    gswat_debuggable_continue(debuggable);
    //gswat_debuggable_disconnect(debuggable);
    
    return 0;
}

