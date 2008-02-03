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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#include <glib.h>

#include "gswat-server.h"


static void command_loop(gchar *read_fifo_name, gchar *write_fifo_name);
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

    command_loop(rfifo, wfifo);

    return 0;
}


static void
command_loop(gchar *read_fifo_name, gchar *write_fifo_name)
{
    int read_fifo_fd;
    int write_fifo_fd;
    GIOChannel *read_fifo;
    GIOChannel *write_fifo;
    gsize written;
    GString *fifo_data;
    GError *error = NULL;
    GIOStatus status;
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
    status = g_io_channel_write_chars(write_fifo,
				      fifo_data->str,
				      -1,
				      &written,
				      &error);
    if(error)
    {
	fprintf(stderr, "Failed to write PID: %s\n", error->message);
	g_error_free(error);
	error = NULL;
	g_string_free(fifo_data, TRUE);
	return;
    }
    g_assert(status == G_IO_STATUS_NORMAL);
    fprintf(stderr, "writtenn=%d fifo_data->len=%d\n",
	    written,
	    fifo_data->len);
    g_assert(written == fifo_data->len);

    status = g_io_channel_flush(write_fifo, &error);
    if(error)
    {
	fprintf(stderr, "Failed to flush io channel: %s\n", error->message);
	g_error_free(error);
	error = NULL;
	g_string_free(fifo_data, TRUE);
	return;
    }
    g_assert(status == G_IO_STATUS_NORMAL);


    while(1)
    {
	/* FIXME We need a a more general purpose protocol here
	 * Can we re-use an existing protocol (probably yes)
	 * What about HTTP? */
	
	/* acknowledge the connection */
	status = g_io_channel_write_chars(write_fifo,
					  "ACK\n",
					  -1,
					  &written,
					  &error);
	if(error)
	{
	    fprintf(stderr, "Failed to write ACK: %s\n", error->message);
	    g_error_free(error);
	    error = NULL;
	    continue;
	}
	g_assert(status == G_IO_STATUS_NORMAL);

	g_io_channel_flush(write_fifo, &error);
	if(error)
	{
	    fprintf(stderr, "Failed to flush io channel: %s\n", error->message);
	    g_error_free(error);
	    error = NULL;
	    continue;
	}
	g_assert(status == G_IO_STATUS_NORMAL);
	
	
#if 0
	/* TODO: If the user requested the command be run in a terminal... */

	gint argc;
	gchar **argv;
	gboolean retval;
	GError  *error = NULL;
	gchar *terminal_command;

	terminal_command = g_strdup_printf("gnome-terminal -x " 
					   GSWAT_BIN_DIR 
					   "/gswat-ptrace-stub --read-fifo %s --write-fifo %s",
					   fifo1_name,
					   fifo0_name);

	g_shell_parse_argv(terminal_command, &argc, &argv, NULL);
	g_message(terminal_command);
	g_free(terminal_command);


	/* run gnome-terminal -x argv[0] -magic /path/to/fifo */
	retval = g_spawn_async(NULL,
			       argv,
			       NULL,
			       G_SPAWN_SEARCH_PATH,
			       NULL,
			       NULL,
			       NULL,
			       &error);
	g_strfreev(argv);


	if(error) {
	    g_warning("%s: %s", _("Could not start terminal"), error->message);
	    g_error_free(error);
	    error = NULL;

	    g_free(fifo0_name);
	    g_free(fifo1_name);
	    return FALSE;

	} else if(!retval) {
	    g_warning("%s", _("Could not start terminal"));

	    g_free(fifo0_name);
	    g_free(fifo1_name);
	    return FALSE;
	}
#endif

	/* read the command to run */
	status = g_io_channel_read_line_string(read_fifo,
					       fifo_data,
					       NULL,
					       &error);
	if(error)
	{
	    fprintf(stderr, "Failed to read command: %s\n", error->message);
	    g_error_free(error);
	    error = NULL;
	    continue;
	}
	g_assert(status == G_IO_STATUS_NORMAL);
	g_message("command=%s", fifo_data->str);

	g_shell_parse_argv(fifo_data->str, &argc, &argv, NULL);

	/* read the working directory */
	status = g_io_channel_read_line_string(read_fifo,
					       fifo_data,
					       NULL,
					       &error);
	if(error)
	{
	    fprintf(stderr, "Failed to read working dir: %s\n", error->message);
	    g_error_free(error);
	    error = NULL;
	    continue;
	}
	g_assert(status == G_IO_STATUS_NORMAL);
	g_message("dir=%s", fifo_data->str);
	chdir(fifo_data->str);

	/* read the session environment */
	while(1)
	{
	    status = g_io_channel_read_line_string(read_fifo,
						   fifo_data,
						   NULL,
						   &error);
	    if(error)
	    {
		fprintf(stderr, "Failed to read environment: %s\n", error->message);
		g_error_free(error);
		error = NULL;
		break;
	    }
	    g_assert(status == G_IO_STATUS_NORMAL);

	    if(strcmp(fifo_data->str, "*** END ENV ***\n") == 0)
	    {
		break;
	    }
	}
	if(status == G_IO_STATUS_ERROR)
	{
	    continue;
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
	g_strfreev(argv);


	/* make sure the process group for the new app
	 * controls the terminal
	 */
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	tcsetpgrp(fileno(stderr), pid);

	/* wait for the first ptrace signal */
	wait(&wait_val);

	/* stop the process */
	kill(pid, SIGSTOP);


	/* detach from the app */
	ptrace(PTRACE_DETACH, pid, 0, 0);


	/* Send the new PID back to gswat */
	g_string_printf(fifo_data, "%d\n", pid);

	status = g_io_channel_write_chars(write_fifo,
					  fifo_data->str,
					  -1,
					  &written,
					  &error);
	if(error)
	{
	    fprintf(stderr, "Failed to send child PID back: %s\n", error->message);
	    g_error_free(error);
	    error = NULL;
	    kill(pid, SIGKILL);
	    wait(&wait_val);
	    continue;
	}
	g_assert(status == G_IO_STATUS_NORMAL);
	g_assert(written == fifo_data->len);

	status = g_io_channel_flush(write_fifo, NULL);
	if(error)
	{
	    fprintf(stderr, "Failed to flush IO: %s\n", error->message);
	    g_error_free(error);
	    error = NULL;
	    kill(pid, SIGKILL);
	    wait(&wait_val);
	    continue;
	}
	g_assert(status == G_IO_STATUS_NORMAL);

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


