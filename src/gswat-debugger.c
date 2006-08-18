/* This file is part of gnome gdb
 *
 * AUTHORS
 *     Sven Herzberg  <herzi@gnome-de.org>
 *
 * Copyright (C) 2006  Sven Herzberg <herzi@gnome-de.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "gswat-debugger.h"

#include "gswat-session.h"
#include "gswat-gdb-variables.h"

#include "gdbmi-parse-tree.h"
#include "gdbmi-parser.h"


struct GSwatDebuggerPrivate {
	GIOChannel* gdb_in;
	GIOChannel* gdb_out;
	GIOChannel* gdb_err;
	GPid        gdb_pid;
    unsigned long gdb_sequence;

	GList     * stack;
	GSwatDebuggerState state;

    GSwatSession *session;

    gchar   *source_uri;
    gulong   source_line;

    int spawn_read_fifo_fd;
    int spawn_write_fifo_fd;
    GIOChannel *spawn_read_fifo;
    GIOChannel *spawn_write_fifo;
    GPid    spawner_pid;
    
    GPid    target_pid;
};



/* GType */
G_DEFINE_TYPE(GSwatDebugger, gswat_debugger, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_STATE,
	PROP_STACK,
    PROP_SOURCE_URI,
    PROP_SOURCE_LINE
};



static void gswat_debugger_spawn_local_process(GSwatDebugger *self);



static void
gd_send_cli_command(GSwatDebugger* self, gchar const* command)
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


static unsigned long
gd_send_mi_command(GSwatDebugger* self, gchar const* command)
{ // make sync and add an error code?
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


static GSList *
gd_parse_stdout(GIOChannel* io, GSwatDebugger* self)
{
	GSList *lines = NULL;
    GSList *gdb_sequences = NULL;
    GSList *tmp = NULL;
    
    /* TODO gnomify */
    gdbmi_parser_ptr parser_ptr;
    gdbmi_output_ptr output_ptr;
    gboolean status;

    /* read up to the next (gdb) prompt */
	while(TRUE)
    {
        GString *gdb_string = g_string_new("");

		g_io_channel_read_line_string(io, gdb_string, NULL, NULL);
        /* FIXME check io/return status*/
        
        g_message("%s", gdb_string->str);

		if(gdb_string->len >= 7 && (strncmp(gdb_string->str, "(gdb) \n", 7) == 0)) {
            g_string_free(gdb_string, TRUE);
			break;
		}
        
        g_string_append(gdb_string, "(gdb) \n");

        lines = g_slist_prepend(lines, gdb_string);

        
    }
    
    /* parse each line read */
    for(tmp=lines; tmp != NULL; tmp=tmp->next)
    {
        GString *current_line = tmp->data;
        int parse_failed;

        parser_ptr = gdbmi_parser_create();
        
        status = gdbmi_parser_parse_string(parser_ptr,
                                           //"*stopped,reason=\"breakpoint-hit\",bkptno=\"1\",thread-id=\"0\",frame={addr=\"0x0804844f\",func=\"main\",args=[{name=\"argc\",value=\"1\"},{name=\"argv\",value=\"0xbffffcf4\"}],file=\"test.c\",line=\"35\"}\n(gdb)\n",
                                           current_line->str,
                                           &output_ptr,
                                           &parse_failed);
        
        gdbmi_parser_destroy(parser_ptr);
        
        //if(status){
        if(parse_failed == 0){
            gdb_sequences = g_slist_prepend(gdb_sequences, output_ptr);
        }else{
            g_warning("Failed to parse gdb output:\n%s\n", current_line->str);
        }
        
        g_string_free(current_line, TRUE);
    }
    

    g_slist_free(lines);


    gdb_sequences = g_slist_reverse(gdb_sequences);
    
    return gdb_sequences;
    
}



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
gd_handle_gdb_stack_result(GSwatDebugger* self, MIResult *result)
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
gd_handle_gdb_bkpt_result(GSwatDebugger* self, MIResult *result)
{
    MITuple *tuple;
    MIResult *cur;
    MIValue *val;

    g_assert(result->value->value_choice == GDBMI_TUPLE);
    tuple = result->value->option.tuple;

    for(cur = tuple->result; cur!=NULL; cur=cur->next)
    {
        if(strcmp(cur->variable, "file")==0)
        {   
            val = cur->value;
            g_assert(val->value_choice == GDBMI_CSTRING);
/*
            if(self->priv->source_uri != NULL){
                g_free(self->priv->source_uri);
            }
            self->priv->source_uri = uri_from_filename(val->option.cstring);
            g_message("Breakpoint file=%s\n", self->priv->source_uri);
            
            g_object_notify(G_OBJECT(self), "source-uri");
            */
        }

    }

}





static void
gd_handle_gdb_breakpoint_hit(GSwatDebugger* self, MIAsyncRecord *bkpt_hit_record)
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
    gd_send_mi_command(self, "-stack-list-frames");
}


static void
gd_handle_gdb_end_stepping_range(GSwatDebugger* self, MIAsyncRecord *async_record)
{

    gd_handle_gdb_breakpoint_hit(self, async_record);
}


static void
gd_handle_gdb_signal_recieved(GSwatDebugger* self, MIAsyncRecord *async_record)
{

    gd_handle_gdb_breakpoint_hit(self, async_record);
}




static void
gd_handle_gdb_oob_stopped_record(GSwatDebugger* self, MIAsyncRecord *stopped_record)
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
                gd_handle_gdb_breakpoint_hit(self, stopped_record);
                self->priv->state = GSWAT_DEBUGGER_INTERRUPTED;
            }
            else if(strcmp(reason, "end-stepping-range")==0)
            {
                gd_handle_gdb_end_stepping_range(self, stopped_record);
                self->priv->state = GSWAT_DEBUGGER_INTERRUPTED;
            }
            else if(strcmp(reason, "signal-received")==0)
            {
                gd_handle_gdb_signal_recieved(self, stopped_record);
                self->priv->state = GSWAT_DEBUGGER_INTERRUPTED;
            }
            else if(strcmp(reason, "exited-normally")==0)
            {
                self->priv->state = GSWAT_DEBUGGER_NOT_RUNNING;
            }
            else if(strcmp(reason, "function-finished")==0)
            {
                self->priv->state = GSWAT_DEBUGGER_INTERRUPTED;
            }
            else
            {
                g_warning("OOB Stopped reason = %s", val->option.cstring);
                g_assert_not_reached(); /* FIXME - this is a bit too aggressive */
            }

            g_object_notify(G_OBJECT(self), "state");
        }
    }
}




static void
gd_handle_gdb_async_oob_records(GSwatDebugger* self, MIAsyncRecord *async_record)
{
    switch(async_record->async_record)
    {
        case GDBMI_STATUS:
            break;
        case GDBMI_EXEC:
            switch(async_record->async_class)
            {
                case GDBMI_STOPPED:
                    gd_handle_gdb_oob_stopped_record(self, async_record);
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
gd_handle_gdb_oob_records(GSwatDebugger* self, MIOOBRecord *oob_record)
{
    
    while(oob_record)
    {
        switch(oob_record->record)
        {
            case GDBMI_ASYNC:
                gd_handle_gdb_async_oob_records(self, oob_record->option.async_record);
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
gd_handle_gdb_result_record(GSwatDebugger* self, MIResultRecord *result_record)
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
                gd_handle_gdb_bkpt_result(self, result_record->result);
            }
            
            if(strcmp(result_record->result->variable, "stack") == 0)
            {
                g_message("GDBMI_DONE Stack\n");
                gd_handle_gdb_stack_result(self, result_record->result);
            }

            break;
	    case GDBMI_RUNNING:
            self->priv->state = GSWAT_DEBUGGER_RUNNING;
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



static void
gd_handle_gdb_sequence(GSwatDebugger* self, MIOutput *sequence)
{
    MIOutput *tmp = sequence;

    print_gdbmi_output(sequence);
    
    for(tmp = sequence; tmp != NULL; tmp = tmp->next)
    {
        if(tmp->oob_record != NULL)
        {
            gd_handle_gdb_oob_records(self, sequence->oob_record);
        }

        if(tmp->result_record != NULL)
        {
            gd_handle_gdb_result_record(self, tmp->result_record);
        }
    }

    return;
}




static gboolean
gd_parse_stdout_immediate(GIOChannel* io, GSwatDebugger* self)
{
	GSList* sequence_list = NULL;
	GSList* tmp = NULL;

	sequence_list = gd_parse_stdout(io, self);
    
    for(tmp=sequence_list; tmp!=NULL; tmp=tmp->next)
    {
        MIOutput *current_sequence = tmp->data;

        gd_handle_gdb_sequence(self, current_sequence);
        
        destroy_gdbmi_output(current_sequence);
    }
    
	g_slist_free(sequence_list);

	return TRUE;
}



static gboolean
gd_parse_stdout_idle(GIOChannel* io, GIOCondition cond, GSwatDebugger* self)
{
    gd_parse_stdout_immediate(io, self);

	return TRUE;
}



static gboolean
gd_parse_stderr_idle(GIOChannel* io, GIOCondition cond, GSwatDebugger* self)
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
gd_update_stack_real(GSwatDebugger* self)
{
    gchar const* command = "-stack-info-depth\n";
    GList* vars = NULL;

    gd_send_mi_command(self, command);

    vars = gd_parse_stdout(self->priv->gdb_out, self);
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
            gd_send_mi_command(self, "-stack-list-frames\n");
            stack = gd_parse_stdout(self->priv->gdb_out, self);
            gd_send_mi_command(self, "-stack-list-arguments 1\n");
            args  = gd_parse_stdout(self->priv->gdb_out, self);
        } else {
            // get only the ten last items (not eleven, to avoid widow lines - cutting off the last line)
            gd_send_mi_command(self, "-stack-list-frames 0 10\n");
            stack = gd_parse_stdout(self->priv->gdb_out, self);
            gd_send_mi_command(self, "-stack-list-arguments 1 0 10\n"); // FROM 1 to 10
            args  = gd_parse_stdout(self->priv->gdb_out, self);
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


static gboolean
gd_update_stack_idle(GSwatDebugger* self)
{

    if(self->priv->state == GSWAT_DEBUGGER_RUNNING) {
        //gd_update_stack_real(self);
        return TRUE;
    }else{
        return FALSE;
    }

}

static void
gd_update_stack(GSwatDebugger* self)
{
    if(self->priv->state == GSWAT_DEBUGGER_RUNNING)
    {
        g_idle_add_full(G_PRIORITY_LOW, (GSourceFunc)gd_update_stack_idle, self, NULL);
    } else {
        //gd_update_stack_real(self);
    }
}





void
gswat_debugger_run(GSwatDebugger* self)
{
    gd_send_mi_command(self, "-exec-run");

}


void
gswat_debugger_request_line_breakpoint(GSwatDebugger* self, gchar *file, guint line)
{
    gchar *gdb_command;
    gdb_command = g_strdup_printf("-break-insert %s %d", file, line);
    gd_send_mi_command(self, gdb_command);
    g_free(gdb_command);
}


void
gswat_debugger_request_function_breakpoint(GSwatDebugger* self, gchar *function)
{
    GString *gdb_command = g_string_new("");

    g_string_printf(gdb_command, "-break-insert %s", function);

    gd_send_mi_command(self, gdb_command->str);

    g_string_free(gdb_command, TRUE);
}


void
gswat_debugger_request_address_breakpoint(GSwatDebugger* self, unsigned long address)
{
    gchar *gdb_command;
    gdb_command = g_strdup_printf("-break-insert 0x%lx", address);
    gd_send_mi_command(self, gdb_command);
    g_free(gdb_command);
}



#if 0
void
gswat_debugger_attach(GSwatDebugger* self, guint pid)
{

    gchar* command = g_strdup_printf("attach %d\n", pid); // FIXME: "-target-attach" should be available
    gd_send_mi_command(self, command);
    g_free(command);

    gd_parse_stdout(self->priv->gdb_out, self);
    // FIXME: check the reply and on success, update the stack
    gd_update_stack(self);

    // FIXME: update the connection status
}
#endif

void
gswat_debugger_query_source(GSwatDebugger* self)
{
    //GString *gdb_command = g_string_new("");

    //g_string_printf(gdb_command, "-file-list-exec-source-file\n");

    gd_send_mi_command(self, "-file-list-exec-source-file");

    //g_string_free(gdb_command, TRUE);
}


void
gswat_debugger_continue(GSwatDebugger* self)
{
    //GString *gdb_command = g_string_new("");

    //g_string_printf(gdb_command, "-exec-continue\n", self->priv->gdb_sequence++);

    gd_send_mi_command(self, "-exec-continue");

    //g_string_free(gdb_command, TRUE);
}


void
gswat_debugger_finish(GSwatDebugger* self)
{
    //GString *gdb_command = g_string_new("");

    //g_string_printf(gdb_command, "-exec-continue\n", self->priv->gdb_sequence++);

    gd_send_mi_command(self, "-exec-finish");

    //g_string_free(gdb_command, TRUE);
}




void
gswat_debugger_step_into(GSwatDebugger* self)
{
    //GString *gdb_command = g_string_new("");

    //g_string_printf(gdb_command, "-exec-step", self->priv->gdb_sequence++);

    gd_send_mi_command(self, "-exec-step");

    //g_string_free(gdb_command, TRUE);


    gd_parse_stdout_immediate(self->priv->gdb_out, self);
    gd_update_stack(self);
}

void
gswat_debugger_next(GSwatDebugger* self)
{
    //GString *gdb_command = g_string_new("");

    //g_string_printf(gdb_command, "%ld -exec-next\n", self->priv->gdb_sequence++);

    gd_send_mi_command(self, "-exec-next");

    //g_string_free(gdb_command, TRUE);


    gd_parse_stdout_immediate(self->priv->gdb_out, self);
    gd_update_stack(self);
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
        gd_send_mi_command(self, "-target-detach");

        gswat_debugger_spawn_local_process(self);
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



GList *
gswat_debugger_get_stack(GSwatDebugger* self)
{ 
    GSwatDebuggerFrame *current_frame, *new_frame;
    GList *tmp, *new_stack=NULL;

    /* copy the stack list */
    for(tmp=self->priv->stack; tmp!=NULL; tmp=tmp->next)
    {
        current_frame = (GSwatDebuggerFrame *)tmp->data;

        new_frame = g_new(GSwatDebuggerFrame, 1);
        memcpy(new_frame, current_frame, sizeof(GSwatDebuggerFrame));

        new_stack = g_list_prepend(new_stack, new_frame);
    }

    return new_stack;
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
gswat_debugger_spawn_local_process(GSwatDebugger *self)
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
    gd_send_mi_command(self, gdb_command);
    g_free(gdb_command);
    g_strfreev(argv);
    
    
    /* AARRRgh GDB/MI is a pain in the arse! */
    //gdb_command=g_strdup_printf("-target-attach %d", self->priv->target_pid);
    gdb_command=g_strdup_printf("attach %d", self->priv->target_pid);
    g_message("SENDING = \"%s\"", gdb_command);
    //gd_send_mi_command(self, gdb_command);
    gd_send_cli_command(self, gdb_command);
    g_free(gdb_command);
    
    
    gswat_debugger_request_function_breakpoint(self, "main");
    

    /* easier than sending a SIGCONT... */
    //gswat_debugger_run(self);
    gd_send_cli_command(self, "signal SIGCONT");
    gd_send_mi_command(self, "-exec-continue");
    gd_send_mi_command(self, "-exec-continue");
    

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

        g_io_add_watch(self->priv->gdb_out, G_IO_IN, (GIOFunc)gd_parse_stdout_idle, self);
        g_io_add_watch(self->priv->gdb_err, G_IO_IN, (GIOFunc)gd_parse_stderr_idle, self);
    }


    const gchar *user_command;
    gint user_argc;
    gchar **user_argv;

    user_command = gswat_session_get_command(GSWAT_SESSION(self->priv->session),
                                             &user_argc,
                                             &user_argv);


    /* assume this session is for debugging a local file */
    gswat_debugger_spawn_local_process(self);

#if 0
    gchar *gdb_command;
    gdb_command = g_strdup_printf("-file-exec-and-symbols %s\n", user_command);
    gd_send_mi_command(self, gdb_command);
    g_free(gdb_command);

    gswat_debugger_request_function_breakpoint(self, "main");

    //gd_parse_stdout_immediate(self->priv->gdb_out, self);

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
gd_finalize(GObject* object)
{
    GSwatDebugger* self = GSWAT_DEBUGGER(object);

    stop_spawner_process(self);

    if(self->priv->gdb_in) {
        gd_send_mi_command(self, "-gdb-exit\n");
        //gd_parse_stdout_idle(self->priv->gdb_out, G_IO_OUT, self);
        g_io_channel_unref(self->priv->gdb_in);
        self->priv->gdb_in = NULL;
    }

    if(self->priv->gdb_out) {
        g_io_channel_unref(self->priv->gdb_out);
        self->priv->gdb_out = NULL;
    }

    if(self->priv->gdb_err) {
        g_io_channel_unref(self->priv->gdb_err);
        self->priv->gdb_err = NULL;
    }

    kill(self->priv->gdb_pid, SIGTERM);

    if(self->priv->stack) {
        g_list_foreach(self->priv->stack, (GFunc)g_free, NULL);
        g_list_free(self->priv->stack);
        self->priv->stack = NULL;
    }

    G_OBJECT_CLASS(gswat_debugger_parent_class)->finalize(object);
}

static void
gd_get_property(GObject* object, guint id, GValue* value, GParamSpec* pspec)
{
    GSwatDebugger* self = GSWAT_DEBUGGER(object);

    switch(id) {
        case PROP_STATE:
            g_value_set_uint(value, self->priv->state);
            break;
        case PROP_STACK:
            g_value_set_pointer(value, self->priv->stack);
            break;
        case PROP_SOURCE_URI:
            g_value_set_string(value, self->priv->source_uri);
            break;
        case PROP_SOURCE_LINE:
            g_value_set_ulong(value, self->priv->source_line);
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

    go_class->finalize     = gd_finalize;
    go_class->get_property = gd_get_property;



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


