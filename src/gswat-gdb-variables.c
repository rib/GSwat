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


#include <glib/gmem.h>
#include <glib/gmessages.h>
#include <glib/gstrfuncs.h>
#include <glib/gstring.h>

#include "gswat-gdb-variables.h"



GSwatGdbVariable*
gswat_gdb_variable_parse(gchar const* string, gchar const** endptr)
{
	GSwatGdbVariable* var = g_new0(GSwatGdbVariable, 1);
	GString* tempdata = g_string_new("");

	// FIXME: return NULL if we get NULL

	for(;string && *string && *string != '='; string++) {
		g_string_append_c(tempdata, *string);
	}
	var->name = g_strdup(tempdata->str);

	string++; // skip '='
	g_string_set_size(tempdata, 0);

	switch(*string) {
		gboolean last_was_slash;
	case '"': // read c-string
		last_was_slash = FALSE;
		for(string++; string && *string; string++) {
			gboolean stop = FALSE;
			switch(*string) {
			case '\\':
				if(last_was_slash) {
					last_was_slash = FALSE;
				} else {
					last_was_slash = TRUE;
				}
				g_string_append_c(tempdata, *string);
				break;
			case '"':
				if(last_was_slash) {
					g_string_append_c(tempdata, *string);
				} else {
					stop = TRUE;
				}
				break;
			default:
				g_string_append_c(tempdata, *string);
				break;
			}

			if(stop) {
				break;
			}
		}
		string++; // skip closing '"'

		{
			gchar* compressed = g_strcompress(tempdata->str);
			var->val = g_new0(GValue, 1);
			g_value_init(var->val, G_TYPE_STRING);
			g_value_take_string(var->val, compressed);
		}
		break;
	case '[':
		{
			GList* list = NULL;
			string++; // skip '['
			while(string && *string && *string != ']') {
				list = g_list_prepend(list,
						      gswat_gdb_variable_parse(string, &string));
				if(*string != ']') {
					string++;
				}
			}
			list = g_list_reverse(list);
			var->val = g_new0(GValue, 1);
			g_value_init(var->val, G_TYPE_POINTER);
			g_value_set_pointer(var->val, list);
		}
		break;
	case '{':
		{
			GList* list = NULL;
			string++; // skip '{'
			while(string && *string && *string != '}') {
				list = g_list_prepend(list,
						      gswat_gdb_variable_parse(string, &string));
				if(*string != '}') {
					string++;
				}
			}
			list = g_list_reverse(list);
			var->val = g_new0(GValue, 1);
			g_value_init(var->val, G_TYPE_POINTER);
			g_value_set_pointer(var->val, list);
		}
		break;
	default:
		g_message("Unparsed variable data: %s", (string && *string) ? string+1 : NULL);
		break;
	}

	if(endptr) {
		*endptr = (string && *string) ? string : NULL;
	}

	g_string_free(tempdata, TRUE);

	return var;
}

static void gswat_gdb_variable_print_depth(GSwatGdbVariable const* self, guint depth);

static void
gswat_gdb_variable_print_depth_p(GSwatGdbVariable const* self, gpointer user_data)
{
	gswat_gdb_variable_print_depth(self, GPOINTER_TO_INT(user_data));
}

static void
gswat_gdb_variable_print_depth(GSwatGdbVariable const* self, guint depth)
{
	guint done;
	for(done = 0; done < depth; done++) { g_print("    "); }
	g_print("(%s)%s=", G_VALUE_TYPE_NAME(self->val), self->name);

	switch(G_VALUE_TYPE(self->val))
    {
        case G_TYPE_STRING:
            g_print("\"%s\"\n", g_value_get_string(self->val));
            break;
        case G_TYPE_POINTER:
            g_print("[");
            if(g_value_get_pointer(self->val) && ((GList*)g_value_get_pointer(self->val))->next)
            {
                g_print("\n");
                g_list_foreach(g_value_get_pointer(self->val),
                                   (GFunc)gswat_gdb_variable_print_depth_p, GINT_TO_POINTER(depth+1));
                for(done = 0; done < depth; done++) { g_print("    "); }
            }
            g_print("]\n");
            break;
        default:
            g_print("unknown data type: %s\n", G_VALUE_TYPE_NAME(self->val));
	}
}

void
gswat_gdb_variable_print(GSwatGdbVariable const* self)
{
	gswat_gdb_variable_print_depth(self, 0);
}

void
gswat_gdb_variable_free(GSwatGdbVariable* self)
{
	g_free(self->name);
	if(G_VALUE_TYPE(self->val) == G_TYPE_POINTER)
    {
		g_list_foreach(g_value_get_pointer(self->val), (GFunc)gswat_gdb_variable_free, NULL);
    }
	g_value_unset(self->val);
	g_free(self->val);
	g_free(self);
}

