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
    static gchar buf[100];
    guint read;
    GIOStatus status;

    while(condition & G_IO_IN)
    {
        status = g_io_channel_read_chars(source,
                                buf,
                                100,
                                &read,
                                NULL); /* FIXME - should check for errors */

        if(status == G_IO_STATUS_NORMAL)
        {
            fwrite(buf, 1, read, stderr);
        }else{
            /* FIXME - handle other status values gracefully */
            fprintf(stderr, "IO Error\n");
            break;
        }

        condition = g_io_channel_get_buffer_condition(source);
    }

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
ggswat_gdb_connect(void)
{
    gchar *gdb_argv[] = { "gdb", "--interpreter=mi2", NULL };
    GPid gdb_pid;
    gint gdb_stdin_ret, gdb_stdout_ret, gdb_stderr_ret;
    gboolean status;
    const gchar *charset;

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

    /* what is the current locale charset? */
    g_get_charset(&charset);
    
    g_io_channel_set_encoding(gdb_stdin, charset, NULL);
    g_io_channel_set_encoding(gdb_stdout, charset, NULL);
    g_io_channel_set_encoding(gdb_stderr, charset, NULL);
    
    g_io_channel_set_flags(gdb_stdin, G_IO_FLAG_NONBLOCK, NULL);
    g_io_channel_set_flags(gdb_stdout, G_IO_FLAG_NONBLOCK, NULL);
    g_io_channel_set_flags(gdb_stderr, G_IO_FLAG_NONBLOCK, NULL);
    

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
ggswat_gdb_send_command(void)
{
    guint written;
    g_io_channel_write(gdb_stdin,
                       "list\n",
                       strlen("list\n"),
                       &written);
    /* FIXME - test for error */
}

