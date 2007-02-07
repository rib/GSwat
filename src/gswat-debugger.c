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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>

#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "gswat-debugger.h"

#include "gswat-session.h"
#include "gswat-gdbmi.h"
#include "gswat-variable-object.h"


typedef void (*GDBMIDoneCallback)(struct _GSwatDebugger *self,
                                  gulong token,
                                  const GDBMIValue *val,
                                  GString *command,
                                  gpointer data);
typedef struct {
    gulong token;
    GDBMIDoneCallback callback;
    gpointer data;
}GDBMIDoneHandler;

#define GDBMI_DONE_CALLBACK(X)  ((GDBMIDoneCallback)(X))

struct GSwatDebuggerPrivate
{
    GSwatSession        *session;

    GSwatDebuggerState  state;
    guint               state_stamp;

    GList               *stack;
    GList               *breakpoints;
    gchar               *source_uri;
    gulong              source_line;

    GList               *locals;
    guint               locals_stamp;

    /* gdb specific */

    GIOChannel      *gdb_in;
    GIOChannel      *gdb_out;
    guint           gdb_out_event;
    GIOChannel      *gdb_err;
    guint           gdb_err_event;
    GPid            gdb_pid;
    unsigned long   gdb_sequence;
    GList           *gdb_pending;

    int             spawn_read_fifo_fd;
    int             spawn_write_fifo_fd;
    GIOChannel      *spawn_read_fifo;
    GIOChannel      *spawn_write_fifo;
    GPid            spawner_pid;

    GPid            target_pid;

    GSList          *pending;
};



/* GType */
G_DEFINE_TYPE(GSwatDebugger, gswat_debugger, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_STATE,
    PROP_STACK,
    PROP_BREAKPOINTS,
    PROP_SOURCE_URI,
    PROP_SOURCE_LINE
};



static void process_gdbmi_command(GSwatDebugger *self,
                                  GString *command);
static void spawn_local_process(GSwatDebugger *self);



static void
gdb_send_cli_command(GSwatDebugger* self, gchar const* command)
{
    gchar *complete_command = NULL;
    gsize written = 0;

    complete_command = g_strdup_printf("%s\n", command);

    if(g_io_channel_write(self->priv->gdb_in,
                          complete_command,
                          strlen(complete_command),
                          &written) != G_IO_ERROR_NONE) {
        g_warning(_("Couldn't send command '%s' to gdb"), command);
    }

    g_message("gdb cli command:%s\n",complete_command);

    g_free(complete_command);

    return;
}


gulong
gdb_send_mi_command(GSwatDebugger* self, gchar const* command)
{
    gchar *complete_command = NULL;
    gsize written = 0;

    complete_command = g_strdup_printf("%ld%s\n", self->priv->gdb_sequence, command);

    if(g_io_channel_write(self->priv->gdb_in,
                          complete_command,
                          strlen(complete_command),
                          &written) != G_IO_ERROR_NONE) {
        g_warning(_("Couldn't send command '%s' to gdb"), command);
    }

    g_message("gdb mi command:%s\n",complete_command);

    g_free(complete_command);

    return self->priv->gdb_sequence++;
}


static gboolean
idle_check_gdb_pending(gpointer data)
{
    GSwatDebugger *self = GSWAT_DEBUGGER(data);
    GList *tmp;
    GString *current_line;

    for(tmp = self->priv->gdb_pending; tmp != NULL; tmp=tmp->next)
    {
        current_line = (GString *)tmp->data;
        process_gdbmi_command(self, current_line);
        g_string_free(current_line, TRUE);
    }

    g_list_free(self->priv->gdb_pending);
    self->priv->gdb_pending = NULL;

    return FALSE;
}



GDBMIValue *
gdb_get_mi_value(GSwatDebugger *self,
                 gulong token,
                 GDBMIValue **mi_error)
{
    GList *tmp;
    gchar *remainder;
    gboolean found = FALSE;
    GDBMIValue *val = NULL;

    g_io_channel_flush(self->priv->gdb_in, NULL);


    for(tmp = self->priv->gdb_pending; tmp != NULL; tmp=tmp->next)
    {
        GString *current_line;
        gulong command_token;
        GDBMIValue *val;

        current_line = (GString *)tmp->data;

        command_token = strtol(current_line->str, &remainder, 10);
        if(command_token == 0 && current_line->str == remainder)
        {
            continue;
        }

        if(command_token == token)
        {
            self->priv->gdb_pending = 
                g_list_remove(self->priv->gdb_pending, current_line);

            val = gdbmi_value_parse(remainder);

            g_string_free(current_line, TRUE);

            return val;
        }
    }


    while(TRUE)
    {
        GString *gdb_string = g_string_new("");
        GError *error = NULL;
        gulong command_token;

        g_io_channel_read_line_string(self->priv->gdb_out,
                                      gdb_string,
                                      NULL,
                                      &error);
        if(error)
        {
            g_error_free(error);
            g_assert_not_reached();
        }

        if(gdb_string->len >= 7 
           && (strncmp(gdb_string->str, "(gdb) \n", 7) == 0)
          ) 
        {
            if(found)
            {
                g_string_free(gdb_string, TRUE);
                break;
            }
            else
            {
                continue;
            }
        }

        /* remove the trailing newline */
        gdb_string = g_string_truncate(gdb_string, gdb_string->len-1);

        /* We should probably cache line tokens... */
        command_token = strtol(gdb_string->str, NULL, 10);

        if(command_token == token)
        {
            char *error_str;
            g_message("found gdb line match - %s", gdb_string->str);

            found = TRUE;

            error_str=strstr(gdb_string->str, "^error,");
            if(error_str && error_str < strchr(gdb_string->str, ','))
            {
                val = NULL;
                if(mi_error)
                {
                    *mi_error = gdbmi_value_parse(gdb_string->str);
                }
            }
            else
            {
                val = gdbmi_value_parse(gdb_string->str);
                if(mi_error)
                {
                    *mi_error = NULL;
                }
            }
            g_string_free(gdb_string, TRUE);
        }
        else
        {
            g_message("adding to pending - %s", gdb_string->str);
            self->priv->gdb_pending = 
                g_list_prepend(self->priv->gdb_pending, gdb_string);
        }
    }

    if(self->priv->gdb_pending != NULL)
    {
        /* install a one off idle handler to process any commands
         * that were added to the gdb_pending queue
         */
        g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
                        idle_check_gdb_pending,
                        self,
                        NULL);
    }

    if(found)
    {
        return val;
    }

    g_assert_not_reached();
    return NULL;
}



GSwatDebugger*
gswat_debugger_new(GSwatSession *session)
{
    GSwatDebugger *debugger;

    g_return_val_if_fail(session != NULL, NULL);

    debugger = g_object_new(GSWAT_TYPE_DEBUGGER, NULL);

    g_object_ref(session);
    debugger->priv->session = session;

    return debugger;
}



static void
parse_stdout(GSwatDebugger* self, GIOChannel* io)
{
    GList *tmp = NULL;
    GList *pending = NULL;


    /* read new gdb output up to the next (gdb) prompt */
    while(TRUE)
    {
        GString *gdb_string = g_string_new("");
        GError *error = NULL;

        g_io_channel_read_line_string(io, gdb_string, NULL, &error);
        if(error)
        {
            g_error_free(error);
            g_assert_not_reached();
        }

        if(gdb_string->len >= 7 && (strncmp(gdb_string->str, "(gdb) \n", 7) == 0)) {
            g_string_free(gdb_string, TRUE);
            break;
        }

        /* remove the trailing newline */
        gdb_string = g_string_truncate(gdb_string, gdb_string->len-1);

        self->priv->gdb_pending = 
            g_list_prepend(self->priv->gdb_pending, gdb_string);
    }


    /* we clear self->priv->gdb_pending and iterate
     * the list in isolation because 
     * process_gdbmi_command may
     * also end up playing with self->priv->gdb_pending
     */
    pending = g_list_reverse(self->priv->gdb_pending);
    self->priv->gdb_pending = NULL;


    /* Flush the pending queue */ 
    for(tmp = pending; tmp != NULL; tmp=tmp->next)
    {
        GString *current_line = (GString *)tmp->data;

        process_gdbmi_command(self, current_line);
        g_string_free(current_line, TRUE);
    }
    g_list_free(pending);


    return;
}


static gchar *
uri_from_filename(const gchar *filename)
{
    GString *local_path;
    gchar *cwd, *uri;

    if(filename == NULL)
    {
        return NULL;
    }

    local_path = g_string_new("");


    if(filename[0] != '/')
    {
        cwd = g_get_current_dir();
        local_path = g_string_append(local_path, cwd);
        g_free(cwd);
        local_path = g_string_append(local_path, "/");
    }
    /* FIXME - if we arn't given an absolute
     * path, then we need to iterate though
     * gdb's $dir to figure out the correct
     * path
     */
    local_path = g_string_append(local_path, filename);


    uri = gnome_vfs_get_uri_from_local_path(local_path->str);

    g_string_free(local_path, TRUE);

    return uri;
}


static void
on_break_insert_done(struct _GSwatDebugger *self,
                     gulong token,
                     const GDBMIValue *val,
                     GString *command,
                     gpointer *data)
{
    GSwatDebuggerBreakpoint *breakpoint;
    const GDBMIValue *bkpt_val, *literal_val;
    const char *literal;

    /*
       {
       bkpt = {
       addr = "0x08048397",
       times = "0",
       func = "function2",
       type = "breakpoint",
       number = "2",
       file = "test.c",
       disp = "keep",
       enabled = "y",
       fullname = "/home/rob/local/gswat/bin/test.c",
       line = "16",
       },
       },
       */

    bkpt_val = gdbmi_value_hash_lookup(val, "bkpt");
    g_assert(bkpt_val);

    breakpoint = g_new0(GSwatDebuggerBreakpoint, 1);

    literal_val = gdbmi_value_hash_lookup(bkpt_val, "fullname");
    literal = gdbmi_value_literal_get(literal_val);
    breakpoint->source_uri = uri_from_filename(literal);

    literal_val = gdbmi_value_hash_lookup(bkpt_val, "line");
    literal = gdbmi_value_literal_get(literal_val);
    breakpoint->line = strtol(literal, NULL, 10);

    self->priv->breakpoints =
        g_list_prepend(self->priv->breakpoints, breakpoint);

    g_object_notify(G_OBJECT(self), "breakpoints");

}


static void
on_stack_list_done(struct _GSwatDebugger *self,
                   gulong token,
                   const GDBMIValue *val,
                   GString *command,
                   gpointer *data)
{
    GList *tmp, *tmp2;
    GSwatDebuggerFrame *frame=NULL, *current_frame=NULL;
    const GDBMIValue *stack_val, *frame_val;
    const GDBMIValue *stackargs_val, *args_val, *arg_val, *literal_val;
    GDBMIValue *top_val;
    GString *gdbmi_command=g_string_new("");
    GSwatDebuggerFrameArgument *arg;
    gulong tok;
    int n;

    gdbmi_value_dump(val, 0);

    /* Free the current stack */
    for(tmp=self->priv->stack; tmp!=NULL; tmp=tmp->next)
    {
        current_frame = (GSwatDebuggerFrame *)tmp->data;
        g_free(current_frame->function);
        for(tmp2 = current_frame->arguments; tmp2!=NULL; tmp2=tmp2->next)
        {
            arg = (GSwatDebuggerFrameArgument *)tmp2->data;
            g_free(arg->name);
            g_free(arg->value);
            g_free(arg);
        }
        g_list_free(current_frame->arguments);
        g_free(current_frame);
    }
    g_list_free(self->priv->stack);
    self->priv->stack = NULL;


    stack_val = gdbmi_value_hash_lookup(val, "stack");

    n=0;
    while(1){
        g_message("gdbmi_value_list_get_nth 0 - %d", n);
        frame_val = gdbmi_value_list_get_nth(stack_val, n);
        if(frame_val)
        {
            const gchar *literal;

            frame = g_new0(GSwatDebuggerFrame,1);
            frame->level = n;

            literal_val = gdbmi_value_hash_lookup(frame_val, "addr");
            literal = gdbmi_value_literal_get(literal_val);
            frame->address = strtol(literal, NULL, 10);

            literal_val = gdbmi_value_hash_lookup(frame_val, "func");
            literal = gdbmi_value_literal_get(literal_val);
            frame->function = g_strdup(literal);

            self->priv->stack = g_list_prepend(self->priv->stack, frame);

        }else{
            break;
        }

        n++;
    }


    g_string_printf(gdbmi_command, "-stack-list-arguments 1 0 %d", n);
    tok = gdb_send_mi_command(self, gdbmi_command->str);
    g_string_free(gdbmi_command, TRUE);


    top_val = gdb_get_mi_value(self, tok, NULL);
    if(!top_val && n>0)
    {
        g_object_notify(G_OBJECT(self), "stack");
        return;
        //g_assert_not_reached();
    }

    gdbmi_value_dump(top_val, 0);

    stackargs_val = gdbmi_value_hash_lookup(top_val, "stack-args");

    n--;
    for(tmp=self->priv->stack; tmp!=NULL; tmp=tmp->next)
    {
        guint a;
        current_frame = (GSwatDebuggerFrame *)tmp->data;

        g_message("gdbmi_value_list_get_nth 1 - %d", n);
        frame_val = gdbmi_value_list_get_nth(stackargs_val, n);
        args_val = gdbmi_value_hash_lookup(frame_val, "args");

        a=0;
        while(TRUE)
        {
            g_message("gdbmi_value_list_get_nth 2");
            arg_val = gdbmi_value_list_get_nth(args_val, a);
            if(!arg_val)
            {
                break;
            }

            arg = g_new0(GSwatDebuggerFrameArgument, 1);

            literal_val = gdbmi_value_hash_lookup(arg_val, "name");
            arg->name = g_strdup(gdbmi_value_literal_get(literal_val));

            literal_val = gdbmi_value_hash_lookup(arg_val, "value");
            arg->value = g_strdup(gdbmi_value_literal_get(literal_val));

            current_frame->arguments = 
                g_list_prepend(current_frame->arguments, arg);

            a++;
        }

        n--;
    }

    gdbmi_value_free(top_val);

    g_object_notify(G_OBJECT(self), "stack");
}


static void
gdb_mi_done_connect(GSwatDebugger *self,
                    gulong token,
                    GDBMIDoneCallback callback,
                    gpointer *data)
{
    GDBMIDoneHandler *handler = g_new0(GDBMIDoneHandler, 1);

    handler->token = token;
    handler->callback = callback;
    handler->data = data;

    self->priv->pending = g_slist_prepend(self->priv->pending, handler);
}



static void
set_location(GSwatDebugger *self,
             const gchar *filename,
             guint line,
             const gchar *address)
{
    gboolean new_uri=FALSE;
    gboolean new_line=FALSE;
    gchar *tmp_uri;

    tmp_uri = uri_from_filename(filename);
    if(tmp_uri == NULL)
    {
        g_critical("Could not create uri for file=\"%s\"", filename);
        return;
    }

    if(self->priv->source_uri == NULL ||
       strcmp(tmp_uri, self->priv->source_uri)!=0)
    {
        new_uri = TRUE;
        if(self->priv->source_uri != NULL)
        {
            g_free(self->priv->source_uri);
        }
        self->priv->source_uri = tmp_uri;
    }
    else
    {
        g_free(tmp_uri);
    }

    if(self->priv->source_line != line){
        new_line=TRUE;
        self->priv->source_line = line;
    }

    if(new_uri)
    {
        g_object_notify(G_OBJECT(self), "source-uri");
    }
    else if(new_line)
    {
        g_object_notify(G_OBJECT(self), "source-line");
    }

    if(new_uri || new_line){
        gulong token;
        token = gdb_send_mi_command(self, "-stack-list-frames");
        gdb_mi_done_connect(self,
                            token,
                            GDBMI_DONE_CALLBACK(
                                                on_stack_list_done
                                               )
                            ,
                            NULL
                           );
    }
}



static void
process_frame(GSwatDebugger *self, GDBMIValue *val)
{
    const GDBMIValue *file, *line, *frame, *addr;
    const gchar *file_str, *line_str, *addr_str;

    if (!val)
        return;

    file_str = line_str = addr_str = NULL;

    g_return_if_fail (val != NULL);

    file = gdbmi_value_hash_lookup (val, "file");
    line = gdbmi_value_hash_lookup (val, "line");
    frame = gdbmi_value_hash_lookup (val, "frame");
    addr = gdbmi_value_hash_lookup (val, "addr");

    if (file && line)
    {
        file_str = gdbmi_value_literal_get (file);
        line_str = gdbmi_value_literal_get (line);

        if (addr)
            addr_str = gdbmi_value_literal_get (addr);

        if (file_str && line_str)
        {
            set_location(self, file_str,
                         (guint)atoi(line_str), addr_str);
        }
    }
    else if (frame)
    {
        file = gdbmi_value_hash_lookup (frame, "file");
        line = gdbmi_value_hash_lookup (frame, "line");

        if (addr)
            addr_str = gdbmi_value_literal_get (addr);

        if (file && line)
        {
            file_str = gdbmi_value_literal_get (file);
            line_str = gdbmi_value_literal_get (line);
            if (file_str && line_str)
            {
                set_location(self, file_str,
                             (guint)atoi(line_str), addr_str);
            }
        }
    } 
}



static void
process_gdbmi_command(GSwatDebugger *self, GString *command)
{
    GString *tmp = NULL;
    gulong command_token;
    gchar *remainder;
    //guint token;
    //guint lineno;
    //gchar *filename;

    //gchar *line = command->str;
    //const size_t len = command->len;

    if (command->len == 0)
    {
        return;
    }


    /* read any gdbmi "token" */
    command_token = strtol(command->str, &remainder, 10);
    if(!(command_token == 0 && command->str == remainder))
    {
        tmp = g_string_new(remainder);
        command = tmp;
    }

#if 0
    token_str = g_string_new("");
    for(i=0; i<command->len; i++)
    {
        if(isdigit(command->str[i])){
            g_string_append_c(token_str, command->str[i]);
        }else{
            break;
        }
    }
    g_assert(i != (command->len-1));
    command_token = strtol(token_str->str, NULL, 10);

    g_string_erase(command, 0, i);
#endif


    g_message("gdbmi_command: token=%ld, command=%s", command_token, command->str);

    if (command->str[0] == '\032' && command->str[1] == '\032')
    {
        g_assert_not_reached();
#if 0
        gdb_util_parse_error_line (&(line[2]), &filename, &lineno);
        if (filename)
        {
            debugger_change_location (debugger, filename, lineno, NULL);
            g_free (filename);
        }
#endif
    }
    else if (strncasecmp (command->str, "^error", 6) == 0)
    {
        g_assert_not_reached();

#if 0
        /* GDB reported error */
        if (debugger->priv->current_cmd.suppress_error == FALSE)
        {
            GDBMIValue *val = gdbmi_value_parse (line);
            if (val)
            {
                const GDBMIValue *message;
                message = gdbmi_value_hash_lookup (val, "msg");

                anjuta_util_dialog_error (debugger->priv->parent_win,
                                          "%s",
                                          gdbmi_value_literal_get (message));
                gdbmi_value_free (val);
            }
        }
#endif
    }
    else if (strncasecmp(command->str, "^running", 8) == 0)
    {
        self->priv->state = GSWAT_DEBUGGER_RUNNING;
        self->priv->state_stamp++;
        g_object_notify(G_OBJECT(self), "state");
#if 0
        /* Program started running */
        debugger->priv->prog_is_running = TRUE;
        debugger->priv->debugger_is_ready = TRUE;
        debugger->priv->skip_next_prompt = TRUE;
        g_signal_emit_by_name (debugger, "program-running");
#endif
    }
    else if(strncasecmp(command->str, "*stopped", 8) == 0)
    {
        GDBMIValue *val = gdbmi_value_parse(command->str);
        if(val)
        {
            const GDBMIValue *reason;
            const gchar *str = NULL;

            gdbmi_value_dump(val, 0);

            process_frame(self, val);

            reason = gdbmi_value_hash_lookup(val, "reason");
            if(reason){
                str = gdbmi_value_literal_get(reason);
            }

            if (str && strcmp (str, "exited-normally") == 0)
            {
                self->priv->state = GSWAT_DEBUGGER_NOT_RUNNING;
            }
            else if (str && strcmp (str, "exited") == 0)
            {
                /* TODO exited with an error code */
                g_assert_not_reached();
            }
            else if (str && strcmp (str, "exited-signalled") == 0)
            {
                /* TODO */
                g_assert_not_reached();
            }
            else if (str && strcmp (str, "signal-received") == 0)
            {
                self->priv->state = GSWAT_DEBUGGER_INTERRUPTED;
            }
            else if (str && strcmp (str, "breakpoint-hit") == 0)
            {
                self->priv->state = GSWAT_DEBUGGER_INTERRUPTED;
            }
            else if (str && strcmp (str, "function-finished") == 0)
            {
                self->priv->state = GSWAT_DEBUGGER_INTERRUPTED;
            }
            else if (str && strcmp (str, "end-stepping-range") == 0)
            {
                self->priv->state = GSWAT_DEBUGGER_INTERRUPTED;
            }
            else if (str && strcmp (str, "location-reached") == 0)
            {
                self->priv->state = GSWAT_DEBUGGER_INTERRUPTED;
            }

            self->priv->state_stamp++;
            g_object_notify(G_OBJECT(self), "state");

            gdbmi_value_free(val);
        }
#if 0
        /* Process has stopped */
        gboolean program_exited = FALSE;

        GDBMIValue *val = gdbmi_value_parse (line);

        debugger->priv->debugger_is_ready = TRUE;

        /* Check if program has exited */
        if (val)
        {
            const GDBMIValue *reason;
            const gchar *str = NULL;

            debugger_process_frame (debugger, val);

            reason = gdbmi_value_hash_lookup (val, "reason");
            if (reason)
                str = gdbmi_value_literal_get (reason);

            if (str && strcmp (str, "exited-normally") == 0)
            {
                program_exited = TRUE;
                debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
                                                 _("Program exited normally\n"),
                                                 debugger->priv->output_user_data);
            }
            else if (str && strcmp (str, "exited") == 0)
            {
                const GDBMIValue *errcode;
                const gchar *errcode_str;
                gchar *msg;

                program_exited = TRUE;
                errcode = gdbmi_value_hash_lookup (val, "exit-code");
                errcode_str = gdbmi_value_literal_get (errcode);
                msg = g_strdup_printf (_("Program exited with error code %s\n"),
                                       errcode_str);
                debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
                                                 msg, debugger->priv->output_user_data);
                g_free (msg);
            }
            else if (str && strcmp (str, "exited-signalled") == 0)
            {
                const GDBMIValue *signal_name, *signal_meaning;
                const gchar *signal_str, *signal_meaning_str;
                gchar *msg;

                program_exited = TRUE;
                signal_name = gdbmi_value_hash_lookup (val, "signal-name");
                signal_str = gdbmi_value_literal_get (signal_name);
                signal_meaning = gdbmi_value_hash_lookup (val,
                                                          "signal-meaning");
                signal_meaning_str = gdbmi_value_literal_get (signal_meaning);
                msg = g_strdup_printf (_("Program received signal %s (%s) and exited\n"),
                                       signal_str, signal_meaning_str);
                debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
                                                 msg, debugger->priv->output_user_data);
                g_free (msg);
            }
            else if (str && strcmp (str, "signal-received") == 0)
            {
                const GDBMIValue *signal_name, *signal_meaning;
                const gchar *signal_str, *signal_meaning_str;
                gchar *msg;

                signal_name = gdbmi_value_hash_lookup (val, "signal-name");
                signal_str = gdbmi_value_literal_get (signal_name);
                signal_meaning = gdbmi_value_hash_lookup (val,
                                                          "signal-meaning");
                signal_meaning_str = gdbmi_value_literal_get (signal_meaning);

                msg = g_strdup_printf (_("Program received signal %s (%s)\n"),
                                       signal_str, signal_meaning_str);
                debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
                                                 msg, debugger->priv->output_user_data);
                g_free (msg);
            }
            else if (str && strcmp (str, "breakpoint-hit") == 0)
            {
                const GDBMIValue *bkptno;
                const gchar *bkptno_str;
                gchar *msg;

                bkptno = gdbmi_value_hash_lookup (val, "bkptno");
                bkptno_str = gdbmi_value_literal_get (bkptno);

                msg = g_strdup_printf (_("Breakpoint number %s hit\n"),
                                       bkptno_str);
                debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
                                                 msg, debugger->priv->output_user_data);
                g_free (msg);
            }
            else if (str && strcmp (str, "function-finished") == 0)
            {
                debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
                                                 _("Function finished\n"),
                                                 debugger->priv->output_user_data);
            }
            else if (str && strcmp (str, "end-stepping-range") == 0)
            {
                debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
                                                 _("Stepping finished\n"),
                                                 debugger->priv->output_user_data);
            }
            else if (str && strcmp (str, "location-reached") == 0)
            {
                debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
                                                 _("Location reached\n"),
                                                 debugger->priv->output_user_data);
            }
        }

        if (program_exited)
        {
            debugger->priv->prog_is_running = FALSE;
            debugger->priv->prog_is_attached = FALSE;
            debugger->priv->debugger_is_ready = TRUE;
            debugger_stop_terminal (debugger);
            g_signal_emit_by_name (debugger, "program-exited", val);
            debugger_handle_post_execution (debugger);
        }
        else
        {
            debugger->priv->debugger_is_ready = TRUE;
            g_signal_emit_by_name (debugger, "program-stopped", val);
        }

        debugger->priv->stdo_lines = g_list_reverse (debugger->priv->stdo_lines);
        if (debugger->priv->current_cmd.cmd[0] != '\0' &&
            debugger->priv->current_cmd.parser != NULL)
        {
            debugger->priv->debugger_is_ready = TRUE;
            debugger->priv->current_cmd.parser (debugger, val,
                                                debugger->priv->stdo_lines,
                                                debugger->priv->current_cmd.user_data);
            debugger->priv->command_output_sent = TRUE;
            DEBUG_PRINT ("In function: Sending output...");
        }

        if (val)
            gdbmi_value_free (val);
#endif
    }
    else if (strncasecmp (command->str, "^done", 5) == 0)
    {
        /* GDB command has reported output */
        GDBMIValue *val = gdbmi_value_parse(command->str);
        if (val)
        {
            GSList *tmp;
            GDBMIDoneHandler *current_handler;

            gdbmi_value_dump(val, 0);

            for(tmp=self->priv->pending; tmp!=NULL; tmp=tmp->next)
            {
                current_handler = (GDBMIDoneHandler *)tmp->data;

                if(current_handler->token == command_token){
                    current_handler->callback(self,
                                              command_token,
                                              val,
                                              command,
                                              current_handler->data);

                    self->priv->pending = 
                        g_slist_remove(self->priv->pending,
                                       current_handler);
                    g_free(current_handler);

                    break;                 
                }

            }

            gdbmi_value_free(val);
        }
#if 0

        debugger->priv->debugger_is_ready = TRUE;

        debugger->priv->stdo_lines = g_list_reverse (debugger->priv->stdo_lines);
        if (debugger->priv->current_cmd.cmd[0] != '\0' &&
            debugger->priv->current_cmd.parser != NULL)
        {
            debugger->priv->current_cmd.parser (debugger, val,
                                                debugger->priv->stdo_lines,
                                                debugger->priv->current_cmd.user_data);
            debugger->priv->command_output_sent = TRUE;
            DEBUG_PRINT ("In function: Sending output...");
        }
        else /* if (val) */
        {
            g_signal_emit_by_name (debugger, "results-arrived",
                                   debugger->priv->current_cmd.cmd, val);
        }

        if (val)
        {
            debugger_process_frame (debugger, val);
            gdbmi_value_free (val);
        }
#endif
    }
#if 0
    else if (strncasecmp (line, GDB_PROMPT, strlen (GDB_PROMPT)) == 0)
    {
        if (debugger->priv->skip_next_prompt)
        {
            debugger->priv->skip_next_prompt = FALSE;
        }
        else
        {
            debugger->priv->debugger_is_ready = TRUE;
            if (debugger->priv->starting)
            {
                debugger->priv->starting = FALSE;
                /* Debugger has just started */
                debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
                                                 _("Debugger is ready.\n"),
                                                 debugger->priv->output_user_data);
            }

            if (strcmp (debugger->priv->current_cmd.cmd, "-exec-run") == 0 &&
                debugger->priv->prog_is_running == FALSE)
            {
                /* Program has failed to run */
                debugger_stop_terminal (debugger);
            }
            debugger->priv->debugger_is_ready = TRUE;

            /* If the parser has not yet been called, call it now. */
            if (debugger->priv->command_output_sent == FALSE &&
                debugger->priv->current_cmd.parser)
            {
                debugger->priv->current_cmd.parser (debugger, NULL,
                                                    debugger->priv->stdo_lines,
                                                    debugger->priv->current_cmd.user_data);
                debugger->priv->command_output_sent = TRUE;
            }

            debugger_queue_execute_command (debugger);	/* Next command. Go. */
            return;
        }
    }
#endif

#if 0
    else
    {
        /* FIXME: change message type */
        gchar *proper_msg, *full_msg;

        if (line[1] == '\"' && line [len - 1] == '\"')
        {
            line[len - 1] = '\0';
            proper_msg = g_strcompress (line + 2);
        }
        else
        {
            proper_msg = g_strdup (line);
        }
        if (proper_msg[strlen(proper_msg)-1] != '\n')
        {
            full_msg = g_strconcat (proper_msg, "\n", NULL);
        }
        else
        {
            full_msg = g_strdup (proper_msg);
        }

        if (debugger->priv->current_cmd.parser)
        {
            if (line[0] == '~')
            {
                /* Save GDB CLI output */
                /* Remove trailing newline */
                full_msg[strlen (full_msg) - 1] = '\0';
                debugger->priv->stdo_lines = g_list_prepend (debugger->priv->stdo_lines,
                                                             g_strdup (full_msg));
            }
        }
        else if (line[0] != '&')
        {
            /* Print the CLI output if there is no parser to receive
             * the output and it is not command echo
             */
            debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
                                             full_msg, debugger->priv->output_user_data);
        }
        g_free (proper_msg);
        g_free (full_msg);
    }


    /* Clear the line buffer */
    g_string_assign (debugger->priv->stdo_line, "");
#endif

}




#if 0
static gchar *
uri_from_filename(gchar *filename)
{
    GString *local_path;
    gchar *cwd, *uri;

    if(filename == NULL)
    {
        return NULL;
    }

    local_path = g_string_new("");


    if(filename[0] != '/')
    {
        cwd = g_get_current_dir();
        local_path = g_string_append(local_path, cwd);
        g_free(cwd);
        local_path = g_string_append(local_path, "/");
    }
    else
    {
        local_path = g_string_append(local_path, filename);
    }


    uri = gnome_vfs_get_uri_from_local_path(local_path->str);

    g_string_free(local_path, TRUE);

    return uri;
}

static void
gswat_debugger_handle_gdb_stack_result(GSwatDebugger* self, MIResult *result)
{
    MIList *list;
    MITuple *tuple;
    MIResult *res0, *res1;
    MIValue *val;
    gchar *level, *addr;
    GList *stack=NULL, *tmp=NULL;
    GSwatDebuggerFrame *frame=NULL, *current_frame=NULL;

    g_assert(result->value->value_choice == GDBMI_LIST);
    list = result->value->option.list;

    g_assert(list->list_choice == GDBMI_RESULT);

    /* Free the current stack */
    for(tmp=self->priv->stack; tmp!=NULL; tmp=tmp->next)
    {
        current_frame = (GSwatDebuggerFrame *)tmp->data;
        g_free(current_frame->function);
        g_free(tmp->data);
    }
    g_list_free(self->priv->stack);
    self->priv->stack = NULL;

    g_assert(list->list_choice == GDBMI_RESULT);
    res0 = list->option.result;
    //g_assert(res0->value->value_choice == GDBMI_TUPLE);
    //tuple = res0->value->option.tuple;


    //for(; list != NULL; list=list->next)
    for(; res0!=NULL; res0=res0->next)
    {
        g_assert(res0->value->value_choice == GDBMI_TUPLE);
        tuple = res0->value->option.tuple;


        frame = g_new(GSwatDebuggerFrame, 1);

        for(res1 = tuple->result; res1!=NULL; res1=res1->next)
        {
            g_message("stack res debug\n");

            if(strcmp(res1->variable, "level")==0)
            {   
                val = res1->value;
                g_assert(val->value_choice == GDBMI_CSTRING);
                level = g_shell_unquote(val->option.cstring, NULL);
                frame->level = strtol(val->option.cstring, NULL, 10);
                g_free(level);

                /*
                   if(self->priv->source_uri != NULL){
                   g_free(self->priv->source_uri);
                   }
                   self->priv->source_uri = uri_from_filename(val->option.cstring);
                   g_message("Breakpoint file=%s\n", self->priv->source_uri);

                   g_object_notify(G_OBJECT(self), "source-uri");
                   */
            }
            if(strcmp(res1->variable, "addr")==0)
            {   
                val = res1->value;
                g_assert(val->value_choice == GDBMI_CSTRING);
                addr = g_shell_unquote(val->option.cstring, NULL);
                frame->address = strtol(addr, NULL, 16);
                g_free(addr);
            }
            if(strcmp(res1->variable, "func")==0)
            {   
                val = res1->value;
                g_assert(val->value_choice == GDBMI_CSTRING);
                frame->function = g_shell_unquote(val->option.cstring, NULL);
            }

        } 
        stack=g_list_prepend(stack, frame);
    }

    //stack=g_list_reverse(stack);

    self->priv->stack = stack;

    g_message("Debug Stack..");
    /* Print the stack for debugging */
    for(tmp=stack; tmp!=NULL; tmp=tmp->next)
    {
        current_frame = (GSwatDebuggerFrame *)tmp->data;
        g_message("frame %d: addr=0x%lX, func=%s",
                  current_frame->level,
                  current_frame->address,
                  current_frame->function);
    }

    g_object_notify(G_OBJECT(self), "stack");


#if 0
    for(cur = list->option.result; cur!=NULL; cur=cur->next)
    {
        if(strcmp(cur->variable, "frame")==0)
        {   
            val = cur->value;
            g_assert(val->value_choice == GDBMI_TUPLE);
            /*
               if(self->priv->source_uri != NULL){
               g_free(self->priv->source_uri);
               }
               self->priv->source_uri = uri_from_filename(val->option.cstring);
               g_message("Breakpoint file=%s\n", self->priv->source_uri);

               g_object_notify(G_OBJECT(self), "source-uri");
               */
        }
        else
        {
            g_assert_not_reached();
        }

    }
#endif

}




static void
gswat_debugger_handle_gdb_bkpt_result(GSwatDebugger* self, MIResult *result)
{
    MITuple *tuple;
    MIResult *cur;
    MIValue *val;
    gchar *filename;
    GSwatDebuggerBreakpoint *breakpoint;

    g_assert(result->value->value_choice == GDBMI_TUPLE);
    tuple = result->value->option.tuple;

    breakpoint = g_new(GSwatDebuggerBreakpoint, 1);

    for(cur = tuple->result; cur!=NULL; cur=cur->next)
    {
        if(strcmp(cur->variable, "file")==0)
        {   
            val = cur->value;
            g_assert(val->value_choice == GDBMI_CSTRING);

            filename = g_shell_unquote(val->option.cstring, NULL);
            breakpoint->source_uri = uri_from_filename(filename);
            g_free(filename);
        }
        if(strcmp(cur->variable, "line") == 0)
        {
            gint line = strtol(g_shell_unquote(val->option.cstring, NULL), NULL, 10);
            breakpoint->line = line;
        }

    }

    self->priv->breakpoints =
        g_list_prepend(self->priv->breakpoints, breakpoint);

    g_object_notify(G_OBJECT(self), "breakpoints");

}





static void
gswat_debugger_handle_gdb_breakpoint_hit(GSwatDebugger* self, MIAsyncRecord *bkpt_hit_record)
{
    MIResult *cur, *cur2;
    MIValue *val, *val2;
    MITuple *tuple;
    gchar *filename, *tmp_uri, *line;
    gboolean new_uri = FALSE;
    gboolean new_line = FALSE;

    /* TODO neaten this up */
    for(cur = bkpt_hit_record->result; cur!=NULL; cur=cur->next)
    {
        if(strcmp(cur->variable, "frame")==0)
        {
            val = cur->value;
            g_assert(val->value_choice == GDBMI_TUPLE);
            tuple = val->option.tuple;

            for(cur2 = tuple->result; cur2!=NULL; cur2=cur2->next)
            {
                if(strcmp(cur2->variable, "fullname")==0)
                {
                    val2 = cur2->value;
                    g_assert(val2->value_choice == GDBMI_CSTRING);

                    filename = g_shell_unquote(val2->option.cstring, NULL);
                    if(filename == NULL)
                    {
                        g_assert_not_reached();
                        continue;
                    }

                    tmp_uri = uri_from_filename(filename);
                    if(tmp_uri == NULL)
                    {
                        g_warning("Could not create uri for file=\"%s\"", filename);
                        g_free(filename);
                        continue;
                    }
                    g_free(filename);

                    if(self->priv->source_uri == NULL ||
                       strcmp(tmp_uri, self->priv->source_uri)!=0)
                    {
                        new_uri = TRUE;
                        if(self->priv->source_uri != NULL)
                        {
                            g_free(self->priv->source_uri);
                        }
                        self->priv->source_uri = tmp_uri;
                    }
                    else
                    {
                        g_free(tmp_uri);
                    }

                    g_message("Breakpoint file=%s\n", self->priv->source_uri);

                }
                else if(strcmp(cur2->variable, "line")==0)
                {
                    val2 = cur2->value;
                    g_assert(val2->value_choice == GDBMI_CSTRING);

                    line = g_shell_unquote(val2->option.cstring, NULL);
                    self->priv->source_line = strtoul(line, NULL, 10);
                    g_free(line);

                    g_message("Breakpoint line=%lu\n", self->priv->source_line);
                    new_line = TRUE;
                    g_object_notify(G_OBJECT(self), "source-line");
                }
            }

            if(new_uri)
            {
                g_message("signal source-uri");
                g_object_notify(G_OBJECT(self), "source-uri");

            }
            else if(new_line)
            {
                g_message("signal source-line");
                g_object_notify(G_OBJECT(self), "source-line");
            }

        }
    }
    gdb_send_mi_command(self, "-stack-list-frames");
}


static void
gswat_debugger_handle_gdb_end_stepping_range(GSwatDebugger* self, MIAsyncRecord *async_record)
{

    gswat_debugger_handle_gdb_breakpoint_hit(self, async_record);
}


static void
gswat_debugger_handle_gdb_signal_recieved(GSwatDebugger* self, MIAsyncRecord *async_record)
{

    gswat_debugger_handle_gdb_breakpoint_hit(self, async_record);
}
#endif


#if 0
static void
gswat_debugger_handle_gdb_oob_stopped_record(GSwatDebugger* self, MIAsyncRecord *stopped_record)
{
    MIResult *cur;
    MIValue *val;
    gchar *reason;

    for(cur = stopped_record->result; cur!=NULL; cur=cur->next)
    {
        if(strcmp(cur->variable, "reason")==0)
        {
            val = cur->value;
            g_assert(val->value_choice == GDBMI_CSTRING);

            reason = g_shell_unquote(val->option.cstring, NULL);
            if(strcmp(reason, "breakpoint-hit")==0)
            {
                gswat_debugger_handle_gdb_breakpoint_hit(self, stopped_record);
                self->priv->state = GSWAT_DEBUGGER_INTERRUPTED;
            }
            else if(strcmp(reason, "end-stepping-range")==0)
            {
                gswat_debugger_handle_gdb_end_stepping_range(self, stopped_record);
                self->priv->state = GSWAT_DEBUGGER_INTERRUPTED;
            }
            else if(strcmp(reason, "signal-received")==0)
            {
                gswat_debugger_handle_gdb_signal_recieved(self, stopped_record);
                self->priv->state = GSWAT_DEBUGGER_INTERRUPTED;
            }
            else if(strcmp(reason, "exited-normally")==0)
            {
                self->priv->state = GSWAT_DEBUGGER_NOT_RUNNING;
                self->priv->state_stamp++;
            }
            else if(strcmp(reason, "function-finished")==0)
            {
                self->priv->state = GSWAT_DEBUGGER_INTERRUPTED;
                self->priv->state_stamp++;
            }
            else
            {
                g_warning("OOB Stopped reason = %s", val->option.cstring);
                g_assert_not_reached(); /* FIXME - this is a bit too aggressive */
            }

            self->priv->state_stamp++;
            g_object_notify(G_OBJECT(self), "state");
        }
    }
}




static void
gswat_debugger_handle_gdb_async_oob_records(GSwatDebugger* self, MIAsyncRecord *async_record)
{
    switch(async_record->async_record)
    {
        case GDBMI_STATUS:
            break;
        case GDBMI_EXEC:
            switch(async_record->async_class)
            {
                case GDBMI_STOPPED:
                    gswat_debugger_handle_gdb_oob_stopped_record(self, async_record);
                    break;
                default:
                    g_assert_not_reached();
            }
            break;
        case GDBMI_NOTIFY:

            break;
        default:
            g_assert_not_reached();
    }
}




static void
gswat_debugger_handle_gdb_oob_records(GSwatDebugger* self, MIOOBRecord *oob_record)
{

    while(oob_record)
    {
        switch(oob_record->record)
        {
            case GDBMI_ASYNC:
                gswat_debugger_handle_gdb_async_oob_records(self, oob_record->option.async_record);
                break;
            case GDBMI_STREAM:

                break;
            default:
                g_assert_not_reached();
        }

        oob_record = oob_record->next;
    }
}



static void
gswat_debugger_handle_gdb_result_record(GSwatDebugger* self, MIResultRecord *result_record)
{

    switch(result_record->result_class)
    {
        case GDBMI_DONE:
            if(result_record->result == NULL){
                g_message("NULL GDBMI_DONE result\n");
                return;
            }

            if(strcmp(result_record->result->variable, "bkpt") == 0)
            {
                g_message("GDBMI_DONE Breakpoint\n");
                gswat_debugger_handle_gdb_bkpt_result(self, result_record->result);
            }

            if(strcmp(result_record->result->variable, "stack") == 0)
            {
                g_message("GDBMI_DONE Stack\n");
                gswat_debugger_handle_gdb_stack_result(self, result_record->result);
            }

            break;
        case GDBMI_RUNNING:
            self->priv->state = GSWAT_DEBUGGER_RUNNING;
            self->priv->state_stamp++;
            g_object_notify(G_OBJECT(self), "state");
            break;
        case GDBMI_CONNECTED:
            break;
        case GDBMI_ERROR:
            break;
        case GDBMI_EXIT:
            break;
        default:
            g_assert_not_reached();
    }
}
#endif

#if 0
static void
gswat_debugger_handle_gdb_sequence(GSwatDebugger* self, MIOutput *sequence)
{
    MIOutput *tmp = sequence;

    print_gdbmi_output(sequence);

    for(tmp = sequence; tmp != NULL; tmp = tmp->next)
    {
        if(tmp->oob_record != NULL)
        {
            gswat_debugger_handle_gdb_oob_records(self, sequence->oob_record);
        }

        if(tmp->result_record != NULL)
        {
            gswat_debugger_handle_gdb_result_record(self, tmp->result_record);
        }
    }

    return;
}
#endif



static gboolean
parse_stdout_immediate(GSwatDebugger* self, GIOChannel* io)
{
    //GSList* sequence_list = NULL;
    //GSList* tmp = NULL;

    parse_stdout(self, io);
#if 0 
    sequence_list = parse_stdout(io, self);
    for(tmp=sequence_list; tmp!=NULL; tmp=tmp->next)
    {
        MIOutput *current_sequence = tmp->data;

        gswat_debugger_handle_gdb_sequence(self, current_sequence);

        destroy_gdbmi_output(current_sequence);
    }

    g_slist_free(sequence_list);
#endif

    return TRUE;
}



static gboolean
parse_stdout_idle(GIOChannel* io, GIOCondition cond, gpointer *data)
{
    GSwatDebugger *self = GSWAT_DEBUGGER(data);

    parse_stdout_immediate(self, io);

    return TRUE;
}



static gboolean
parse_stderr_idle(GIOChannel* io, GIOCondition cond, GSwatDebugger* self)
{
    GIOStatus status;
    GString *line;

    line = g_string_new("");

    status = g_io_channel_read_line_string(io,
                                           line,
                                           NULL,
                                           NULL);

    g_warning("gdberr: %s\n", line->str);

    return TRUE;
}


#if 0
static void
gswat_debugger_update_stack_real(GSwatDebugger* self)
{
    gchar const* command = "-stack-info-depth\n";
    GList* vars = NULL;

    gdb_send_mi_command(self, command);

    vars = parse_stdout(self->priv->gdb_out, self);
    if(!vars || vars->next) {
        // 0 or 2+ results
        g_warning("Unexpected machine interface answer on '%s': got %d variables", command, g_list_length(vars));
    } else {
        gint j;
        GList* stack = NULL;
        GList* args  = NULL;
        GList* full_stack = NULL;
        j = atoi(g_value_get_string(((GSwatGdbVariable*)vars->data)->val));

        if(j <= 11) {
            // get the whole backtrace (up to eleven ones)
            gdb_send_mi_command(self, "-stack-list-frames\n");
            stack = parse_stdout(self->priv->gdb_out, self);
            gdb_send_mi_command(self, "-stack-list-arguments 1\n");
            args  = parse_stdout(self->priv->gdb_out, self);
        } else {
            // get only the ten last items (not eleven, to avoid widow lines - cutting off the last line)
            gdb_send_mi_command(self, "-stack-list-frames 0 10\n");
            stack = parse_stdout(self->priv->gdb_out, self);
            gdb_send_mi_command(self, "-stack-list-arguments 1 0 10\n"); // FROM 1 to 10
            args  = parse_stdout(self->priv->gdb_out, self);
        }

        // be really careful
        if(g_list_length(stack) != 1 || g_list_length(args) != 1) {
            // FIXME: warning
        } else {
            GList* stack_element, *arg_element;

            for(stack_element = g_value_get_pointer(((GSwatGdbVariable*)stack->data)->val),
                arg_element   = g_value_get_pointer(((GSwatGdbVariable*)args->data)->val);
                stack_element && arg_element;
                stack_element = stack_element->next, arg_element = arg_element->next)
            {
                gchar const* func = NULL;
                GList* stack_frame_element = g_value_get_pointer(((GSwatGdbVariable*)stack_element->data)->val);
                for(; stack_frame_element; stack_frame_element = stack_frame_element->next) {
                    GSwatGdbVariable* var = stack_frame_element->data;
                    if(var->name && !strcmp("func", var->name)) {
                        func = g_value_get_string(var->val);
                        if(func && !strcmp("??", func)) {
                            func = NULL;
                        }
                        break;
                    }
                }

                full_stack = g_list_prepend(full_stack,
                                            g_strdup_printf("%s%s",
                                                            func ? func : _("Unknown function"),
                                                            func ? "()" : ""));
            }
            full_stack = g_list_reverse(full_stack);
        }

        if(self->priv->stack) {
            g_list_foreach(self->priv->stack, (GFunc)g_free, NULL);
        }

        self->priv->stack = full_stack;

        g_object_notify(G_OBJECT(self), "stack");
    }
    g_list_foreach(vars, (GFunc)gswat_gdb_variable_free,  NULL);
    g_list_free(vars);
}
#endif


#if 0
static gboolean
gswat_debugger_update_stack_idle(GSwatDebugger* self)
{

    if(self->priv->state == GSWAT_DEBUGGER_RUNNING) {
        //gswat_debugger_update_stack_real(self);
        return TRUE;
    }else{
        return FALSE;
    }

}

static void
gswat_debugger_update_stack(GSwatDebugger* self)
{
    if(self->priv->state == GSWAT_DEBUGGER_RUNNING)
    {
        g_idle_add_full(G_PRIORITY_LOW, (GSourceFunc)gswat_debugger_update_stack_idle, self, NULL);
    } else {
        //gswat_debugger_update_stack_real(self);
    }
}
#endif





void
gswat_debugger_run(GSwatDebugger* self)
{
    gdb_send_mi_command(self, "-exec-run");

}


void
gswat_debugger_request_line_breakpoint(GSwatDebugger* self, gchar *uri, guint line)
{
    gchar *gdb_command;
    gchar *filename;

    if(self->priv->state == GSWAT_DEBUGGER_NOT_RUNNING
       || self->priv->state == GSWAT_DEBUGGER_INTERRUPTED)
    {
        gulong token;

        filename = gnome_vfs_get_local_path_from_uri(uri);
        g_assert(filename);

        gdb_command = g_strdup_printf("-break-insert %s:%d", filename, line);
        token = gdb_send_mi_command(self, gdb_command);
        gdb_mi_done_connect(self,
                            token,
                            GDBMI_DONE_CALLBACK(
                                                on_break_insert_done
                                               )
                            ,
                            NULL
                           );

        g_free(gdb_command);
        g_free(filename);
    }
}


void
gswat_debugger_request_function_breakpoint(GSwatDebugger* self, gchar *function)
{
    GString *gdb_command = g_string_new("");

    if(self->priv->state == GSWAT_DEBUGGER_NOT_RUNNING
       || self->priv->state == GSWAT_DEBUGGER_INTERRUPTED)
    {
        gulong token;

        g_string_printf(gdb_command, "-break-insert %s", function);

        token = gdb_send_mi_command(self, gdb_command->str);
        gdb_mi_done_connect(self,
                            token,
                            GDBMI_DONE_CALLBACK(
                                                on_break_insert_done
                                               )
                            ,
                            NULL
                           );

        g_string_free(gdb_command, TRUE);
    }
}


void
gswat_debugger_request_address_breakpoint(GSwatDebugger* self, unsigned long address)
{
    gchar *gdb_command;

    if(self->priv->state == GSWAT_DEBUGGER_NOT_RUNNING
       || self->priv->state == GSWAT_DEBUGGER_INTERRUPTED)
    {
        gulong token;

        gdb_command = g_strdup_printf("-break-insert 0x%lx", address);
        token = gdb_send_mi_command(self, gdb_command);
        g_free(gdb_command);
        gdb_mi_done_connect(self,
                            token,
                            GDBMI_DONE_CALLBACK(
                                                on_break_insert_done
                                               )
                            ,
                            NULL
                           );

    }
}



#if 0
void
gswat_debugger_attach(GSwatDebugger* self, guint pid)
{

    gchar* command = g_strdup_printf("attach %d\n", pid); // FIXME: "-target-attach" should be available
    gdb_send_mi_command(self, command);
    g_free(command);

    parse_stdout(self->priv->gdb_out, self);
    // FIXME: check the reply and on success, update the stack
    gswat_debugger_update_stack(self);

    // FIXME: update the connection status
}
#endif

void
gswat_debugger_query_source(GSwatDebugger* self)
{
    //GString *gdb_command = g_string_new("");

    //g_string_printf(gdb_command, "-file-list-exec-source-file\n");

    gdb_send_mi_command(self, "-file-list-exec-source-file");

    //g_string_free(gdb_command, TRUE);
}


void
gswat_debugger_continue(GSwatDebugger* self)
{
    //GString *gdb_command = g_string_new("");

    //g_string_printf(gdb_command, "-exec-continue\n", self->priv->gdb_sequence++);

    gdb_send_mi_command(self, "-exec-continue");

    //g_string_free(gdb_command, TRUE);
}


void
gswat_debugger_finish(GSwatDebugger* self)
{
    //GString *gdb_command = g_string_new("");

    //g_string_printf(gdb_command, "-exec-continue\n", self->priv->gdb_sequence++);

    gdb_send_mi_command(self, "-exec-finish");

    //g_string_free(gdb_command, TRUE);
}




void
gswat_debugger_step_into(GSwatDebugger* self)
{
    //GString *gdb_command = g_string_new("");

    //g_string_printf(gdb_command, "-exec-step", self->priv->gdb_sequence++);

    gdb_send_mi_command(self, "-exec-step");

    //g_string_free(gdb_command, TRUE);


    //parse_stdout_immediate(self, self->priv->gdb_out);
    //gswat_debugger_update_stack(self);
}

void
gswat_debugger_next(GSwatDebugger* self)
{
    //GString *gdb_command = g_string_new("");

    //g_string_printf(gdb_command, "%ld -exec-next\n", self->priv->gdb_sequence++);

    gdb_send_mi_command(self, "-exec-next");

    //g_string_free(gdb_command, TRUE);


    //parse_stdout_immediate(self, self->priv->gdb_out);
    //gswat_debugger_update_stack(self);
}


void
gswat_debugger_interrupt(GSwatDebugger* self)
{
    if(self->priv->state == GSWAT_DEBUGGER_RUNNING)
    {
        kill(self->priv->target_pid, SIGINT);
    }

}


void
gswat_debugger_restart(GSwatDebugger* self)
{
    if(self->priv->state == GSWAT_DEBUGGER_RUNNING
       || self->priv->state == GSWAT_DEBUGGER_INTERRUPTED)
    {
        //kill(self->priv->target_pid, SIGKILL);
        gdb_send_mi_command(self, "-target-detach");

        spawn_local_process(self);
    }
}


gchar *
gswat_debugger_get_source_uri(GSwatDebugger* self)
{

    return g_strdup(self->priv->source_uri);
}


gulong
gswat_debugger_get_source_line(GSwatDebugger* self)
{
    return self->priv->source_line;
}


guint
gswat_debugger_get_state(GSwatDebugger* self)
{
    return self->priv->state;
}


guint
gswat_debugger_get_state_stamp(GSwatDebugger* self)
{
    return self->priv->state_stamp;
}



GList *
gswat_debugger_get_stack(GSwatDebugger* self)
{ 
    GSwatDebuggerFrame *current_frame, *new_frame;
    GList *tmp, *tmp2, *new_stack=NULL;
    GSwatDebuggerFrameArgument *current_arg, *new_arg;

    /* copy the stack list */
    for(tmp=self->priv->stack; tmp!=NULL; tmp=tmp->next)
    {
        current_frame = (GSwatDebuggerFrame *)tmp->data;

        new_frame = g_new(GSwatDebuggerFrame, 1);
        new_frame->level = current_frame->level;
        new_frame->address = current_frame->address;
        new_frame->function = g_strdup(current_frame->function);

        /* deep copy the arguments */
        new_frame->arguments = NULL;
        for(tmp2=current_frame->arguments; tmp2!=NULL; tmp2=tmp2->next)
        {
            current_arg = (GSwatDebuggerFrameArgument *)tmp2->data;

            new_arg = g_new0(GSwatDebuggerFrameArgument, 1);
            new_arg->name = g_strdup(current_arg->name);
            new_arg->value = g_strdup(current_arg->value);
            new_frame->arguments = 
                g_list_prepend(new_frame->arguments, new_arg);
        }

        new_stack = g_list_prepend(new_stack, new_frame);
    }

    return new_stack;
}



GList *
gswat_debugger_get_breakpoints(GSwatDebugger* self)
{ 
    GSwatDebuggerBreakpoint *current_breakpoint, *new_breakpoint;
    GList *tmp, *new_breakpoints=NULL;

    /* copy the stack list */
    for(tmp=self->priv->breakpoints; tmp!=NULL; tmp=tmp->next)
    {
        current_breakpoint = (GSwatDebuggerBreakpoint *)tmp->data;

        new_breakpoint = g_new(GSwatDebuggerBreakpoint, 1);
        memcpy(new_breakpoint, current_breakpoint, sizeof(GSwatDebuggerBreakpoint));

        new_breakpoints = g_list_prepend(new_breakpoints, new_breakpoint);
    }

    return new_breakpoints;
}



GList *
gswat_debugger_get_locals_list(GSwatDebugger* self)
{
    GList *new_locals_list=NULL;
    gulong token;
    GDBMIValue *top_val;
    const GDBMIValue *locals_val, *local_val;
    int n;
    GList *tmp;


    if(self->priv->state & GSWAT_DEBUGGER_RUNNING)
    {
        return g_list_copy(self->priv->locals);
    }

    if(self->priv->locals_stamp == self->priv->state_stamp)
    {
        return g_list_copy(self->priv->locals);
    }

    self->priv->locals_stamp = self->priv->state_stamp;

    g_list_free(self->priv->locals);


    token = gdb_send_mi_command(self, "-stack-list-locals --simple-values");
    top_val = gdb_get_mi_value(self, token, NULL);
    if(!top_val)
    {
        g_warning("Failed to list stack local variables");
        return NULL;
    }
    locals_val = gdbmi_value_hash_lookup(top_val, "locals");


    n=0;
    while((local_val = gdbmi_value_list_get_nth(locals_val, n)))
    {
        const GDBMIValue *value_val, *name_val;
        gchar *value = NULL;
        gchar *expression;
        GSwatVariableObject *variable_object;

        name_val = gdbmi_value_hash_lookup(local_val, "name");
        expression = g_strdup(gdbmi_value_literal_get(name_val));

        value_val = gdbmi_value_hash_lookup(local_val, "value");

        /* arrays, unions structs
         * wont have a value
         */
        if(value_val)
        {
            value = g_strdup(gdbmi_value_literal_get(value_val));
        }

        /* create a new list of gswat variable objects for each variable
         * (Note that doesn't mean gdb "variable objects"
         * are immediatly created for simple types)
         */
        variable_object = gswat_variable_object_new(self,
                                                    expression,
                                                    value,
                                                    -1);

        new_locals_list = g_list_prepend(new_locals_list, variable_object);
        n++;
    }
    gdbmi_value_free(top_val);


    for(tmp=self->priv->locals; tmp!=NULL; tmp=tmp->next)
    {
        g_object_unref(G_OBJECT(tmp->data));
    }
    self->priv->locals = new_locals_list;

    return self->priv->locals;
}



static gboolean
start_spawner_process(GSwatDebugger *self)
{
    gchar *fifo0_name;
    gchar *fifo1_name;
    int read_fifo_fd;
    int write_fifo_fd;
    GIOChannel *read_fifo;
    GIOChannel *write_fifo;
    gchar *terminal_command;
    gint argc;
    gchar **argv;
    gboolean retval;
    GError  *error = NULL;
    GString *fifo_data;

    fifo0_name=g_strdup_printf("/tmp/gswat%d0", getpid());
    fifo1_name=g_strdup_printf("/tmp/gswat%d1", getpid());

    /* create named fifos */
    mkfifo(fifo0_name, S_IRUSR|S_IWUSR);
    mkfifo(fifo1_name, S_IRUSR|S_IWUSR);


    terminal_command = g_strdup_printf("gnome-terminal -x gswat-spawner --read-fifo %s --write-fifo %s",
                                       fifo1_name,
                                       fifo0_name);

    g_shell_parse_argv(terminal_command, &argc, &argv, NULL);


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

    read_fifo_fd = open(fifo0_name, O_RDONLY);
    write_fifo_fd = open(fifo1_name, O_WRONLY);

    g_free(fifo0_name);
    g_free(fifo1_name);

    /* attach g_io_channels to the fifos */
    read_fifo = g_io_channel_unix_new(read_fifo_fd);
    write_fifo = g_io_channel_unix_new(write_fifo_fd);


    self->priv->spawn_read_fifo_fd = read_fifo_fd;
    self->priv->spawn_write_fifo_fd = write_fifo_fd;

    self->priv->spawn_read_fifo = read_fifo;
    self->priv->spawn_write_fifo = write_fifo;


    fifo_data = g_string_new("");

    g_io_channel_read_line_string(self->priv->spawn_read_fifo,
                                  fifo_data,
                                  NULL,
                                  NULL);

    self->priv->spawner_pid = strtoul(fifo_data->str, NULL, 10);


    return TRUE;
}


static void
stop_spawner_process(GSwatDebugger *self)
{
    kill(self->priv->spawner_pid, SIGKILL);

    g_io_channel_shutdown(self->priv->spawn_read_fifo, FALSE, NULL);
    g_io_channel_shutdown(self->priv->spawn_write_fifo, FALSE, NULL);
    g_io_channel_unref(self->priv->spawn_read_fifo);
    g_io_channel_unref(self->priv->spawn_write_fifo);

    close(self->priv->spawn_read_fifo_fd);
    close(self->priv->spawn_write_fifo_fd);
}




static void
spawn_local_process(GSwatDebugger *self)
{
    gchar **argv;
    GString *fifo_data;

    gsize written;
    gchar *cwd;
    gchar *command;
    gchar *gdb_command;



    fifo_data = g_string_new("");

    /* wait for ACK */
    g_io_channel_read_line_string(self->priv->spawn_read_fifo,
                                  fifo_data,
                                  NULL,
                                  NULL);

    if(strcmp(fifo_data->str, "ACK\n") != 0)
    {
        g_critical("Didn't get an expected \"ACK\" from target fifo\n");
    }


    command = gswat_session_get_command(self->priv->session, NULL, &argv);
    g_string_printf(fifo_data, "%s\n", command);
    g_free(command);

    /* Send the command to run */
    g_io_channel_write_chars(self->priv->spawn_write_fifo,
                             fifo_data->str,
                             fifo_data->len,
                             &written,
                             NULL);
    g_io_channel_flush(self->priv->spawn_write_fifo, NULL);

    /* Send working directory */
    cwd = gswat_session_get_working_dir(self->priv->session);
    g_string_printf(fifo_data, "%s\n", cwd);
    g_free(cwd);
    g_io_channel_write_chars(self->priv->spawn_write_fifo,
                             fifo_data->str,
                             fifo_data->len,
                             &written,
                             NULL);
    g_io_channel_flush(self->priv->spawn_write_fifo, NULL);

    /* Send the session environment */
    /* TODO */

    /* Terminate environment data */
    g_io_channel_write_chars(self->priv->spawn_write_fifo,
                             "*** END ENV ***\n",
                             strlen("*** END ENV ***\n"),
                             &written,
                             NULL);
    g_io_channel_flush(self->priv->spawn_write_fifo, NULL);


    /* The command we sent should now get started in the
     * terminal using ptrace so SIGSTOP can be sent
     * before main() is reached.
     * a ptrace detach is then done and we get sent
     * the PID of the new process
     *
     * Note: The reason for all this hassle is
     * so we can have our debug app run in a nice
     * terminal (so you can e.g. debug ncurses apps)
     *
     * If there is a neater way, please tell me.
     */


    g_io_channel_read_line_string(self->priv->spawn_read_fifo,
                                  fifo_data,
                                  NULL,
                                  NULL);
    self->priv->target_pid = strtoul(fifo_data->str, NULL, 10);
    g_message("process PID=%d",self->priv->target_pid);

    g_string_free(fifo_data, TRUE);


    /* load the symbols into gdb and attach to the suspended
     * process
     */


    gdb_command=g_strdup_printf("-file-exec-and-symbols %s", argv[0]);
    gdb_send_mi_command(self, gdb_command);
    g_free(gdb_command);
    g_strfreev(argv);


    /* AARRRgh GDB/MI is a pain in the arse! */
    //gdb_command=g_strdup_printf("-target-attach %d", self->priv->target_pid);
    gdb_command=g_strdup_printf("attach %d", self->priv->target_pid);
    g_message("SENDING = \"%s\"", gdb_command);
    //gdb_send_mi_command(self, gdb_command);
    gdb_send_cli_command(self, gdb_command);
    g_free(gdb_command);


    gswat_debugger_request_function_breakpoint(self, "main");


    /* easier than sending a SIGCONT... */
    //gswat_debugger_run(self);
    gdb_send_cli_command(self, "signal SIGCONT");
    gdb_send_mi_command(self, "-exec-continue");
    gdb_send_mi_command(self, "-exec-continue");


}

void
gswat_debugger_target_disconnect(GSwatDebugger* self)
{
    stop_spawner_process(self);
    
    g_source_remove(self->priv->gdb_out_event);
    g_source_remove(self->priv->gdb_err_event);
    
    
    if(self->priv->gdb_out) {
        g_io_channel_unref(self->priv->gdb_out);
        self->priv->gdb_out = NULL;
    }

    if(self->priv->gdb_err) {
        g_io_channel_unref(self->priv->gdb_err);
        self->priv->gdb_err = NULL;
    }

    if(self->priv->gdb_in) {
        gdb_send_mi_command(self, "-gdb-exit\n");
        //parse_stdout_idle(self->priv->gdb_out, G_IO_OUT, self);
        g_io_channel_unref(self->priv->gdb_in);
        self->priv->gdb_in = NULL;
    }

    //kill(self->priv->gdb_pid, SIGTERM);
}


/* Connect to the target as appropriate for the session */
void
gswat_debugger_target_connect(GSwatDebugger* self)
{
    GError  * error = NULL;
    gchar *gdb_argv[] = {
        "gdb", "--interpreter=mi", NULL
    };
    gint fd_in, fd_out, fd_err;
    const gchar *charset;
    gboolean  retval;


    /* Currently we assume a local gdb session */

    retval = g_spawn_async_with_pipes(NULL, gdb_argv, NULL,
                                      G_SPAWN_SEARCH_PATH,
                                      NULL, NULL,
                                      &self->priv->gdb_pid, &fd_in,
                                      &fd_out, &fd_err,
                                      &error);

    if(error) {
        g_warning("%s: %s", _("Could not start debugger"), error->message);
        g_error_free(error);
        error = NULL;
    } else if(!retval) {
        g_warning("%s", _("Could not start debugger"));
    } else {
        // everything went fine
        self->priv->gdb_in  = g_io_channel_unix_new(fd_in);
        self->priv->gdb_out = g_io_channel_unix_new(fd_out);
        self->priv->gdb_err = g_io_channel_unix_new(fd_err);

        /* what is the current locale charset? */
        g_get_charset(&charset);

        g_io_channel_set_encoding(self->priv->gdb_in, charset, NULL);
        g_io_channel_set_encoding(self->priv->gdb_out, charset, NULL);
        g_io_channel_set_encoding(self->priv->gdb_err, charset, NULL);

        //g_io_channel_set_flags(self->priv->gdb_in, G_IO_FLAG_NONBLOCK, NULL);
        //g_io_channel_set_flags(self->priv->gdb_out, G_IO_FLAG_NONBLOCK, NULL);
        //g_io_channel_set_flags(self->priv->gdb_err, G_IO_FLAG_NONBLOCK, NULL);

        self->priv->gdb_out_event = 
                g_io_add_watch(self->priv->gdb_out,
                               G_IO_IN,
                               (GIOFunc)parse_stdout_idle,
                               self);
        self->priv->gdb_err_event =
                g_io_add_watch(self->priv->gdb_err,
                               G_IO_IN,
                               (GIOFunc)parse_stderr_idle,
                               self);
    }


    const gchar *user_command;
    gint user_argc;
    gchar **user_argv;

    user_command = gswat_session_get_command(GSWAT_SESSION(self->priv->session),
                                             &user_argc,
                                             &user_argv);


    /* assume this session is for debugging a local file */
    spawn_local_process(self);

#if 0
    gchar *gdb_command;
    gdb_command = g_strdup_printf("-file-exec-and-symbols %s\n", user_command);
    gdb_send_mi_command(self, gdb_command);
    g_free(gdb_command);

    gswat_debugger_request_function_breakpoint(self, "main");

    //parse_stdout_immediate(self->priv->gdb_out, self);

    //gswat_debugger_query_source(self);

    g_message("GGGGGGGGG\n");
    gswat_debugger_run(self);


    g_object_notify(G_OBJECT(self), "source_uri");
#endif

}




static void
gswat_debugger_init(GSwatDebugger* self)
{

    self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, GSWAT_TYPE_DEBUGGER, struct GSwatDebuggerPrivate);

    if(!start_spawner_process(self))
    {
        g_critical("Failed to start spawner process");
    }

}

static void
gswat_debugger_finalize(GObject* object)
{
    GSwatDebugger* self = GSWAT_DEBUGGER(object);


    gswat_debugger_target_disconnect(self);

    /* FIXME - free arguments */
    if(self->priv->stack) {
        g_list_foreach(self->priv->stack, (GFunc)g_free, NULL);
        g_list_free(self->priv->stack);
        self->priv->stack = NULL;
    }

    g_object_unref(self->priv->session);
    self->priv->session=NULL;

    G_OBJECT_CLASS(gswat_debugger_parent_class)->finalize(object);
}

static void
gswat_debugger_get_property(GObject* object, guint id, GValue* value, GParamSpec* pspec)
{
    GSwatDebugger* self = GSWAT_DEBUGGER(object);
    guint state;
    GList *breakpoints, *stack;
    gchar *source_uri;
    gulong source_line;


    switch(id) {
        case PROP_STATE:
            state = gswat_debugger_get_state(self);
            g_value_set_uint(value, state);
            break;
        case PROP_STACK:
            stack = gswat_debugger_get_stack(self);
            g_value_set_pointer(value, stack);
            break;
        case PROP_BREAKPOINTS:
            breakpoints = gswat_debugger_get_breakpoints(self);
            g_value_set_pointer(value, breakpoints);
            break;
        case PROP_SOURCE_URI:
            source_uri = gswat_debugger_get_source_uri(self);
            g_value_set_string(value, source_uri);
            break;
        case PROP_SOURCE_LINE:
            source_line = gswat_debugger_get_source_line(self);
            g_value_set_ulong(value, source_line);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
            break;
    }
}




static void
gswat_debugger_class_init(GSwatDebuggerClass* self_class)
{
    GObjectClass* go_class = G_OBJECT_CLASS(self_class);

    go_class->finalize     = gswat_debugger_finalize;
    go_class->get_property = gswat_debugger_get_property;



    g_object_class_install_property(go_class,
                                    PROP_STATE,
                                    g_param_spec_uint("state",
                                                      _("The Running State"),
                                                      _("The State of the debugger (whether or not it is currently running)"),
                                                      0,
                                                      G_MAXUINT,
                                                      GSWAT_DEBUGGER_NOT_RUNNING,
                                                      G_PARAM_READABLE));

    g_object_class_install_property(go_class,
                                    PROP_STACK,
                                    g_param_spec_pointer("stack",
                                                         _("Stack"),
                                                         _("The Stack of called functions"),
                                                         G_PARAM_READABLE));

    g_object_class_install_property(go_class,
                                    PROP_STACK,
                                    g_param_spec_pointer("breakpoints",
                                                         _("Breakpoints"),
                                                         _("The list of user defined breakpoints"),
                                                         G_PARAM_READABLE));

    g_object_class_install_property(go_class,
                                    PROP_SOURCE_URI,
                                    g_param_spec_string("source-uri",
                                                        _("Source URI"),
                                                        _("The location for the source file currently being debuggged"),
                                                        NULL,//"file:///home/rob/src/ggdb/src/gswat-debugger.c",
                                                        G_PARAM_READABLE));

    g_object_class_install_property(go_class,
                                    PROP_SOURCE_LINE,
                                    g_param_spec_ulong("source-line",
                                                       _("Source Line"),
                                                       _("The line of your source file currently being debuggged"),
                                                       0,
                                                       G_MAXULONG,
                                                       0,
                                                       G_PARAM_READABLE));

    g_type_class_add_private(self_class, sizeof(struct GSwatDebuggerPrivate));
}


