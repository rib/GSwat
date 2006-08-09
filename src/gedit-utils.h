/*
 * gedit-utils.h
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002 - 2005 Paolo Maggi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA.
 */
 
/*
 * Modified by the gedit Team, 1998-2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id: gedit-utils.h,v 1.37.2.1 2006/04/20 16:53:41 pborelli Exp $
 */

#ifndef __GEDIT_UTILS_H__
#define __GEDIT_UTILS_H__

#include <glib.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkaboutdialog.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtktextiter.h>
#include <atk/atk.h>

#include "gedit-encodings.h"

G_BEGIN_DECLS

/* useful macro */
#define GBOOLEAN_TO_POINTER(i) ((gpointer) ((i) ? 2 : 1))
#define GPOINTER_TO_BOOLEAN(i) ((gboolean) ((((gint)(i)) == 2) ? TRUE : FALSE))

#define IS_VALID_BOOLEAN(v) (((v == TRUE) || (v == FALSE)) ? TRUE : FALSE)

enum { GEDIT_ALL_WORKSPACES = 0xffffffff };

gboolean	 gedit_utils_uri_has_writable_scheme	(const gchar *uri);
gboolean	 gedit_utils_uri_has_file_scheme	(const gchar *uri);

void		 gedit_utils_menu_position_under_widget (GtkMenu  *menu,
							 gint     *x,
							 gint     *y,
							 gboolean *push_in,
							 gpointer  user_data);

GtkWidget	*gedit_gtk_button_new_with_stock_icon	(const gchar *label,
							 const gchar *stock_id);

GtkWidget	*gedit_dialog_add_button		(GtkDialog   *dialog,
							 const gchar *text,
							 const gchar *stock_id, 
							 gint         response_id);

gchar		*gedit_utils_escape_underscores		(const gchar *text,
							 gssize       length);

gchar		*gedit_utils_escape_slashes		(const gchar *text,
							 gssize       length);

gchar		*gedit_utils_str_middle_truncate	(const gchar *string, 
							 guint        truncate_length);

gboolean	 g_utf8_caselessnmatch			(const char *s1,
							 const char *s2,
							 gssize n1,
							 gssize n2);

void		 gedit_utils_set_atk_name_description	(GtkWidget  *widget,
							 const gchar *name,
							 const gchar *description);

void		 gedit_utils_set_atk_relation		(GtkWidget       *obj1,
							 GtkWidget       *obj2,
							 AtkRelationType  rel_type);

gboolean	 gedit_utils_uri_exists			(const gchar* text_uri);

gchar		*gedit_utils_escape_search_text		(const gchar *text);

gchar		*gedit_utils_unescape_search_text	(const gchar *text);

gchar		*gedit_utils_get_stdin			(void);

void		gedit_warning				(GtkWindow  *parent,
							 const gchar *format,
							 ...) G_GNUC_PRINTF(2, 3);

gchar		*gedit_utils_str_middle_truncate	(const char *string,
							 guint truncate_length);

gchar		*gedit_utils_make_valid_utf8		(const char *name);

/* Note that this function replace home dir with ~ */
gchar		*gedit_utils_uri_get_dirname		(const char *uri);

gchar		*gedit_utils_replace_home_dir_with_tilde (const gchar *uri);

guint		 gedit_utils_get_current_workspace	(GdkScreen *screen);

guint		 gedit_utils_get_window_workspace	(GtkWindow *gtkwindow);

void		 gedit_utils_activate_url		(GtkAboutDialog *about,
							 const gchar    *url,
							 gpointer        data);

gboolean	 gedit_utils_is_valid_uri		(const gchar *uri);

gboolean	 gedit_utils_get_glade_widgets		(const gchar  *filename,
							 const gchar  *root_node,
							 GtkWidget   **error_widget,
							 const gchar  *widget_name,
							 ...) G_GNUC_NULL_TERMINATED;

/* Return NULL if str is not a valid URI and/or filename */
gchar		*gedit_utils_make_canonical_uri_from_shell_arg
							(const gchar *str);
		
/* Like gnome_vfs_format_uri_for_display but removes the password from the 
 * resulting string */
gchar		*gedit_utils_format_uri_for_display 	(const gchar *uri);

G_END_DECLS

#endif /* __GEDIT_UTILS_H__ */


