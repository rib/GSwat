
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <signal.h>

#include <gnome.h>

#include "gswat-spawn.h"


static void spawn_loop(gchar *read_fifo_name, gchar *write_fifo_name);
static int assign_terminal(int ctty, pid_t pgrp);


static gchar *rfifo;
static gchar *wfifo;

int
main(int argc, char **argv)
{
    GError *error = NULL;
    gchar **remaining_args = NULL;

	GOptionEntry option_entries[] = {
    
		/* ... your application's command line options go here ... */
        { "read-fifo",
          0,
          0,
          G_OPTION_ARG_FILENAME,
          &rfifo,
          "gswat IPC read fifo", NULL },

        { "write-fifo",
          0,
          0,
          G_OPTION_ARG_FILENAME,
          &wfifo,
          "gswat IPC write fifo", NULL },

		/* last but not least a special option that collects filenames */
		{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY,
		  &remaining_args,
		  "Special option that collects any remaining arguments for us" },
		{ NULL }
	};

	GOptionContext *option_context;

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


    g_option_context_parse(option_context, &argc, &argv, &error);
    
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

    spawn_loop(rfifo, wfifo);
    

    return 0;
}








static void
spawn_loop(gchar *read_fifo_name, gchar *write_fifo_name)
{
    int read_fifo_fd;
    int write_fifo_fd;
    GIOChannel *read_fifo;
    GIOChannel *write_fifo;
    gsize written;
    GString *fifo_data;
    gint argc;
    gchar **argv;
    pid_t pid;
    int wait_val;
    pid_t curpgrp;
    pid_t pgrp;
    int ctty;
    
    
    g_message("read_fifo_name=%s", read_fifo_name);
    g_message("write_fifo_name=%s", write_fifo_name);


    write_fifo_fd = open(write_fifo_name, O_WRONLY);
    read_fifo_fd = open(read_fifo_name, O_RDONLY);

    /* attach g_io_channels to the fifos */
    write_fifo = g_io_channel_unix_new(write_fifo_fd);
    read_fifo = g_io_channel_unix_new(read_fifo_fd);

    /* look up the pgid controlling the tty 
     * (assuming we are currently forground)
     */
    if ((curpgrp = tcgetpgrp(ctty = 2)) < 0
        && (curpgrp = tcgetpgrp(ctty = 0)) < 0
        && (curpgrp = tcgetpgrp(ctty = 1)) < 0)
    {
        g_assert_not_reached();
    }
    
    
    g_assert(curpgrp == getpgrp());

    /* Do a one off write to tell gswat our pid */

    fifo_data = g_string_new("");

    g_string_printf(fifo_data, "%d\n", getpid());
    g_io_channel_write_chars(write_fifo,
                                 fifo_data->str,
                                 fifo_data->len,
                                 &written,
                                 NULL);
    g_io_channel_flush(write_fifo, NULL);


    
    while(1)
    {
        /* acknowledge the connection */
        g_io_channel_write_chars(write_fifo,
                                 "ACK\n",
                                 strlen("ACK\n"),
                                 &written,
                                 NULL);
        g_io_channel_flush(write_fifo, NULL);

        /* read the command to run */
        g_io_channel_read_line_string(read_fifo,
                                      fifo_data,
                                      NULL,
                                      NULL);
        g_message("command=%s", fifo_data->str);

        g_shell_parse_argv(fifo_data->str, &argc, &argv, NULL);

        /* read the working directory */
        g_io_channel_read_line_string(read_fifo,
                                      fifo_data,
                                      NULL,
                                      NULL);
        g_message("dir=%s", fifo_data->str);
        chdir(fifo_data->str);

        /* read the session environment */
        while(1)
        {
            g_io_channel_read_line_string(read_fifo,
                                          fifo_data,
                                          NULL,
                                          NULL);

            if(strcmp(fifo_data->str, "*** END ENV ***\n") == 0)
            {
                break;
            }
        }
        g_message("env done");
        
        
    
        switch(pid=fork())
        {
            case -1:
                g_critical("Failed to spawn sub-process");
                break;
            case 0: /* child */
                pgrp = getpid();
                
                g_assert(setpgid(0,pgrp) == 0);
                
                assign_terminal(ctty, pgrp);
                
                ptrace(PTRACE_TRACEME, 0, 0, 0);

                execvp(argv[0], argv);

                break;
            default: /* parent */
                
                pgrp = pid;

                setpgid(pid, pgrp);
                
                break;
        }

        
        /* make sure the process group for the new app
         * controls the terminal
         */
        signal (SIGTTOU, SIG_IGN);
        signal (SIGTTIN, SIG_IGN);
        tcsetpgrp(fileno(stderr), pid);

        /* wait for the first ptrace signal */
        wait(&wait_val);

        /* stop the process */
        kill(pid, SIGSTOP);


        /* detach from the app */
        ptrace (PTRACE_DETACH, pid, 0, 0);


        /* Send the new PID back to gswat */
        g_string_printf(fifo_data, "%d\n", pid);

        g_io_channel_write_chars(write_fifo,
                                 fifo_data->str,
                                 fifo_data->len,
                                 &written,
                                 NULL);
        g_io_channel_flush(write_fifo, NULL);
        
        g_strfreev(argv);

        /* wait for the process to quit */
        wait(&wait_val);
    }

    g_string_free(fifo_data, TRUE);

}






/* assign the terminal (open on ctty) to a specific pgrp. This wrapper
 * around tcsetpgrp() is needed only because of extreme bogosity on the
 * part of POSIX; conforming systems deliver STGTTOU if tcsetpgrp is
 * called from a non-foreground process (which it almost invariably is).
 * A triumph of spurious consistency over common sense.
 */

static int
assign_terminal(int ctty, pid_t pgrp)
{
    sigset_t sigs;
    sigset_t oldsigs;
    int rc;

    sigemptyset(&sigs);
    sigaddset(&sigs,SIGTTOU);
    sigprocmask(SIG_BLOCK, &sigs, &oldsigs);

    rc = tcsetpgrp(ctty, pgrp);

    sigprocmask(SIG_SETMASK, &oldsigs, NULL);

    return rc;
}

