/*
 * gedit-notebook.h
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi 
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
 * Modified by the gedit Team, 2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

/* This file is a modified version of the epiphany file ephy-notebook.h
 * Here the relevant copyright:
 *
 *  Copyright (C) 2002 Christophe Fergeau
 *  Copyright (C) 2003 Marco Pesenti Gritti
 *  Copyright (C) 2003, 2004 Christian Persch
 *
 */
 
#ifndef GSWAT_NOTEBOOK_H
#define GSWAT_NOTEBOOK_H

#include "gswat-tab.h"

#include <glib.h>
#include <gtk/gtknotebook.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define GSWAT_TYPE_NOTEBOOK		(gswat_notebook_get_type ())
#define GSWAT_NOTEBOOK(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GSWAT_TYPE_NOTEBOOK, GSwatNotebook))
#define GSWAT_NOTEBOOK_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GSWAT_TYPE_NOTEBOOK, GSwatNotebookClass))
#define GEDIT_IS_NOTEBOOK(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GSWAT_TYPE_NOTEBOOK))
#define GSWAT_IS_NOTEBOOK_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GSWAT_TYPE_NOTEBOOK))
#define GSWAT_NOTEBOOK_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GSWAT_TYPE_NOTEBOOK, GSwatNotebookClass))

/* Private structure type */
typedef struct _GSwatNotebookPrivate	GSwatNotebookPrivate;

/*
 * Main object structure
 */
typedef struct _GSwatNotebook		GSwatNotebook;
 
struct _GSwatNotebook
{
	GtkNotebook notebook;

	/*< private >*/
    GSwatNotebookPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _GSwatNotebookClass	GSwatNotebookClass;

struct _GSwatNotebookClass
{
        GtkNotebookClass parent_class;

	/* Signals */
	void	 (* tab_added)      (GSwatNotebook *notebook,
				     GSwatTab      *tab);
	void	 (* tab_removed)    (GSwatNotebook *notebook,
				     GSwatTab      *tab);
	void	 (* tab_detached)   (GSwatNotebook *notebook,
				     GSwatTab      *tab);
	void	 (* tabs_reordered) (GSwatNotebook *notebook);
	void	 (* tab_close_request)
				    (GSwatNotebook *notebook,
				     GSwatTab      *tab);
};

/*
 * Public methods
 */
GType		gswat_notebook_get_type		(void) G_GNUC_CONST;

GtkWidget      *gswat_notebook_new		(void);

void		gswat_notebook_add_tab		(GSwatNotebook *nb,
						 GSwatTab      *tab,
						 gint           position,
						 gboolean       jump_to);

void		gswat_notebook_remove_tab	(GSwatNotebook *nb,
						 GSwatTab      *tab);

void		gswat_notebook_remove_all_tabs 	(GSwatNotebook *nb);

void		gswat_notebook_reorder_tab	(GSwatNotebook *src,
			    			 GSwatTab      *tab,
			    			 gint           dest_position);
			    			 
void            gswat_notebook_move_tab		(GSwatNotebook *src,
						 GSwatNotebook *dest,
						 GSwatTab      *tab,
						 gint           dest_position);

/* FIXME: do we really need this function ? */
void		gswat_notebook_set_always_show_tabs	
						(GSwatNotebook *nb,
						 gboolean       show_tabs);

void		gswat_notebook_set_close_buttons_sensitive
						(GSwatNotebook *nb,
						 gboolean       sensitive);

gboolean	gswat_notebook_get_close_buttons_sensitive
						(GSwatNotebook *nb);

void		gswat_notebook_set_tab_drag_and_drop_enabled
						(GSwatNotebook *nb,
						 gboolean       enable);

gboolean	gswat_notebook_get_tab_drag_and_drop_enabled
						(GSwatNotebook *nb);

G_END_DECLS

#endif /* GSWAT_NOTEBOOK_H */

