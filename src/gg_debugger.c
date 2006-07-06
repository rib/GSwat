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

#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>

#include "gg_gdb_variable.h"
#include "gg_debugger.h"

typedef enum {
	GG_DEBUGGER_READY   = 0,
	GG_DEBUGGER_RUNNING = 1
} GGDebuggerState;

struct GGDebuggerPrivate {
	GIOChannel* gdb_out;
	GIOChannel* gdb_in;
	gint        gdb_pid;

	GList     * stack;
	GGDebuggerState state;
};

static void
gd_send_command(GGDebugger* self, gchar const* command)
{ // make sync and add an error code?
	gsize written = 0;

	// FIXME: add a debugging check for a trainline newline
	if(g_io_channel_write(self->priv->gdb_in,
                          command,
                          strlen(command),
                          &written) != G_IO_ERROR_NONE) {
		g_warning(_("Couldn't send command '%s' to the gdb"), command);
	}
}

GGDebugger*
gg_debugger_new(void)
{
	return g_object_new(GG_TYPE_DEBUGGER, NULL);
}

static GGGdbVariable*
gd_parse_line(GGDebugger* self, gchar const* line)
{
	GGGdbVariable* retval = NULL;
	gchar type_specifier = *line;

	// FIXME: there may be a token before the specifier

	switch(type_specifier) {
		gchar* separator;
	case '^': // result record: '^done,depth="12"\n'
		//g_print("%s\n", line);
		line++;
		separator = strchr(line, ',');
		if(!strncmp("done", line, 4)) {
		} else if(!strncmp("running", line, 7)) {
			self->priv->state = GG_DEBUGGER_RUNNING;
			g_object_notify(G_OBJECT(self), "running");
		} else if(!strncmp("connected", line, 9)) {
		} else if(!strncmp("error", line, 5)) {
		} else if(!strncmp("exit", line, 4)) {
			g_message("gdb has quit");
			// FIXME: shutdown IO here
		} else {
			gchar* result_code = g_strndup(line, separator ? separator - line + 1 : strlen(line));
			g_warning("Unknown result code in result record: %s", result_code);
			g_free(result_code);
		}

		// check for optional output
		line = strchr(line, ',');
		while(line && *line == ',') {
			line++; // skip comma
			retval = gg_gdb_variable_parse(line, &line);
		}
		break;
	case '*': // async reply: '*stopped,reason="end-stepping-range",thread-id="1",frame={addr="0xb78e08c4",func="poll",args=[],from="/lib/tls/i686/cmov/libc.so.6"}'
		line++;
		if(!strncmp("stopped", line, 7)) {
			self->priv->state = GG_DEBUGGER_READY;
			g_object_notify(G_OBJECT(self), "running");
		} else {
			g_message("Unknown async reply: %s", line);
		}
		break;
	default:
		g_print("gdb: out: %s", line);
	}

	return retval;
}

static GList*
gd_parse_stdout(GIOChannel* io, GGDebugger* self)
{
	GList* list = NULL;

	while(TRUE)
    {
		GGGdbVariable* var = NULL;
		gchar* str = NULL;
		g_io_channel_read_line(io, &str, NULL, NULL, NULL);

		if(!str || !strncmp(str, "(gdb) ", 5)) {
			g_free(str);
			break;
		}

		var = gd_parse_line(self, str);
		g_free(str);

		if(var) {
			list = g_list_prepend(list, var);
		}
	}

	return g_list_reverse(list);
}

static gboolean
gd_parse_stdout_idle(GIOChannel* io, GIOCondition cond, GGDebugger* self)
{
	GList* vars;
	vars = gd_parse_stdout(io, self);
	g_list_foreach(vars, (GFunc)gg_gdb_variable_print, NULL);
	g_list_foreach(vars, (GFunc)gg_gdb_variable_free,  NULL);
	g_list_free(vars);

	return TRUE;
}

static void
gd_update_stack_real(GGDebugger* self)
{
	gchar const* command = "-stack-info-depth\n";
	GList* vars = NULL;

	gd_send_command(self, command);

	vars = gd_parse_stdout(self->priv->gdb_out, self);
	if(!vars || vars->next) {
		// 0 or 2+ results
		g_warning("Unexpected machine interface answer on '%s': got %d variables", command, g_list_length(vars));
	} else {
		gint j;
		GList* stack = NULL;
		GList* args  = NULL;
		GList* full_stack = NULL;
		j = atoi(g_value_get_string(((GGGdbVariable*)vars->data)->val));

		if(j <= 11) {
			// get the whole backtrace (up to eleven ones)
			gd_send_command(self, "-stack-list-frames\n");
			stack = gd_parse_stdout(self->priv->gdb_out, self);
			gd_send_command(self, "-stack-list-arguments 1\n");
			args  = gd_parse_stdout(self->priv->gdb_out, self);
		} else {
			// get only the ten last items (not eleven, to avoid widow lines - cutting off the last line)
			gd_send_command(self, "-stack-list-frames 0 10\n");
			stack = gd_parse_stdout(self->priv->gdb_out, self);
			gd_send_command(self, "-stack-list-arguments 1 0 10\n"); // FROM 1 to 10
			args  = gd_parse_stdout(self->priv->gdb_out, self);
		}

		// be really careful
		if(g_list_length(stack) != 1 || g_list_length(args) != 1) {
			// FIXME: warning
		} else {
			GList* stack_element, *arg_element;

			for(stack_element = g_value_get_pointer(((GGGdbVariable*)stack->data)->val),
			    arg_element   = g_value_get_pointer(((GGGdbVariable*)args->data)->val);
			    stack_element && arg_element;
			    stack_element = stack_element->next, arg_element = arg_element->next)
			{
				gchar const* func = NULL;
				GList* stack_frame_element = g_value_get_pointer(((GGGdbVariable*)stack_element->data)->val);
				for(; stack_frame_element; stack_frame_element = stack_frame_element->next) {
					GGGdbVariable* var = stack_frame_element->data;
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
	g_list_foreach(vars, (GFunc)gg_gdb_variable_free,  NULL);
	g_list_free(vars);
}

static gboolean
gd_update_stack_idle(GGDebugger* self)
{
	gboolean running = (self->priv->state & GG_DEBUGGER_RUNNING) != 0;

	if(!running) {
		gd_update_stack_real(self);
	}

	return running;
}

static void
gd_update_stack(GGDebugger* self)
{
	if(self->priv->state & GG_DEBUGGER_RUNNING)
    {
		g_idle_add_full(G_PRIORITY_LOW, (GSourceFunc)gd_update_stack_idle, self, NULL);
	} else {
		gd_update_stack_real(self);
	}
}

void
gg_debugger_attach(GGDebugger* self, guint pid)
{
	gchar* command = g_strdup_printf("attach %d\n", pid); // FIXME: "-target-attach" should be available
	gd_send_command(self, command);
	g_free(command);

	gd_parse_stdout(self->priv->gdb_out, self);
	// FIXME: check the reply and on success, update the stack
	gd_update_stack(self);

	// FIXME: update the connection status
}

void
gg_debugger_continue(GGDebugger* self)
{
	gd_send_command(self, "-exec-continue\n");
}

void
gg_debugger_step_into(GGDebugger* self)
{
	gd_send_command(self, "-exec-step\n");
	gd_parse_stdout(self->priv->gdb_out, self);
	gd_update_stack(self);
}

void
gg_debugger_step_over(GGDebugger* self)
{
	gd_send_command(self, "-exec-next\n");
	gd_parse_stdout(self->priv->gdb_out, self);
	gd_update_stack(self);
}

/* GType */
G_DEFINE_TYPE(GGDebugger, gg_debugger, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_RUNNING,
	PROP_STACK
};

static void
gg_debugger_init(GGDebugger* self)
{
	GError  * error = NULL;
	gchar* argv[] = {
		"gdb", "--interpreter=mi", NULL
	};
	gint fd_in, fd_out, fd_err;
	gboolean  retval;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, GG_TYPE_DEBUGGER, struct GGDebuggerPrivate);

	retval = g_spawn_async_with_pipes(NULL, argv, NULL,
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
		self->priv->gdb_out = g_io_channel_unix_new(fd_out);
		g_io_add_watch(self->priv->gdb_out, G_IO_IN, (GIOFunc)gd_parse_stdout_idle, self);
		self->priv->gdb_in  = g_io_channel_unix_new(fd_in);
	}
}

static void
gd_finalize(GObject* object)
{
	GGDebugger* self = GG_DEBUGGER(object);

	if(self->priv->gdb_in) {
		gd_send_command(self, "-gdb-exit\n");
		//gd_parse_stdout_idle(self->priv->gdb_out, G_IO_OUT, self);
		g_io_channel_unref(self->priv->gdb_in);
		self->priv->gdb_in = NULL;
	}

	if(self->priv->gdb_out) {
		g_io_channel_unref(self->priv->gdb_out);
		self->priv->gdb_out = NULL;
	}

	if(self->priv->stack) {
		g_list_foreach(self->priv->stack, (GFunc)g_free, NULL);
		g_list_free(self->priv->stack);
		self->priv->stack = NULL;
	}

	G_OBJECT_CLASS(gg_debugger_parent_class)->finalize(object);
}

static void
gd_get_property(GObject* object, guint id, GValue* value, GParamSpec* pspec)
{
	GGDebugger* self = GG_DEBUGGER(object);

	switch(id) {
	case PROP_RUNNING:
		g_value_set_boolean(value, (self->priv->state & GG_DEBUGGER_RUNNING) != 0);
		break;
	case PROP_STACK:
		g_value_set_pointer(value, self->priv->stack);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
		break;
	}
}

static void
gg_debugger_class_init(GGDebuggerClass* self_class)
{
	GObjectClass* go_class = G_OBJECT_CLASS(self_class);

	go_class->finalize     = gd_finalize;
	go_class->get_property = gd_get_property;

	g_object_class_install_property(go_class,
					PROP_RUNNING,
					g_param_spec_boolean("running",
							     _("Running State"),
							     _("The State of the debugger (whether or not it is currently running"),
							     FALSE,
							     G_PARAM_READABLE));
	g_object_class_install_property(go_class,
					PROP_STACK,
					g_param_spec_pointer("stack",
							     _("Stack"),
							     _("The Stack of called functions"),
							     G_PARAM_READABLE));

	g_type_class_add_private(self_class, sizeof(struct GGDebuggerPrivate));
}


