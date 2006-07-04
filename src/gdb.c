#include <stdio.h>
#include <gnome.h>

#include "global.h"


static GIOChannel *gdb_stdin;
static GIOChannel *gdb_stdout;
static GIOChannel *gdb_stderr;


/* FIXME create a g_object to represent a gdb_session */


/* FIXME - rehome */
static gboolean 
gdb_stdout_io_watcher(GIOChannel *source, GIOCondition condition, gpointer data)
{
   fprintf(stderr, " gdb_stdout_io_watcher %s\n", (gchar *)data);
   return TRUE;
}

static gboolean 
gdb_stderr_io_watcher(GIOChannel *source, GIOCondition condition, gpointer data)
{
    
   fprintf(stderr, " gdb_stderr_io_watcher %s\n", (gchar *)data);
   return TRUE;
}

/* TODO return a GDBSession object */
void
gg_gdb_connect(void)
{
    gchar *gdb_argv[] = { "gdb", "--interpreter=mi2", NULL };
    GPid gdb_pid;
    gint gdb_stdin_ret, gdb_stdout_ret, gdb_stderr_ret;
    gboolean status;
    status = g_spawn_async_with_pipes(NULL, /* inherit current working dir */
                                     gdb_argv,
                                     NULL, /* inherit environment */
                                     G_SPAWN_SEARCH_PATH,
                                     NULL, /* no setup function */
                                     NULL, /* no user data */
                                     &gdb_pid,
                                     &gdb_stdin_ret,
                                     &gdb_stdout_ret,
                                     &gdb_stderr_ret,
                                     NULL); /* FIXME - we should feedback errors to the user! */
    if(!status)
    {
        /* FIXME [ not very good error handling */
        fprintf(stderr, "Failed to spawn gdb subprocess\n");
        exit(1);
    }

    /* connect the pipes to iochannels */
    gdb_stdin = g_io_channel_unix_new(gdb_stdin_ret);
    gdb_stdout = g_io_channel_unix_new(gdb_stdout_ret);
    gdb_stderr = g_io_channel_unix_new(gdb_stderr_ret);

    /* Have the main loop watch stdout and stderr */
    g_io_add_watch(gdb_stdout,
                   G_IO_IN|G_IO_PRI, /* FIXME we should also be watching for broken pipes */
                   gdb_stdout_io_watcher,
                   NULL);
    g_io_add_watch(gdb_stderr,
                   G_IO_IN|G_IO_PRI, /* FIXME we should also be watching for broken pipes */
                   gdb_stderr_io_watcher,
                   NULL);
   
}


void
gg_gdb_send_command(void)
{
    guint written;
    g_io_channel_write(gdb_stdin,
                       "list\n",
                       strlen("list\n"),
                       &written);
    /* FIXME - test for error */
}

