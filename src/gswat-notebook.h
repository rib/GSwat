/*
 * <preamble>
 * gswat - A graphical program debugger for Gnome
 * Copyright (C) 2006  Robert Bragg
 * </preamble>
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
 */

#ifndef GSWAT_NOTEBOOK_H
#define GSWAT_NOTEBOOK_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtknotebook.h>

#include "gswat-tabable.h"

G_BEGIN_DECLS

#define GSWAT_NOTEBOOK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSWAT_TYPE_NOTEBOOK, GSwatNotebook))
#define GSWAT_TYPE_NOTEBOOK            (gswat_notebook_get_type())
#define GSWAT_NOTEBOOK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSWAT_TYPE_NOTEBOOK, GSwatNotebookClass))
#define GSWAT_IS_NOTEBOOK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSWAT_TYPE_NOTEBOOK))
#define GSWAT_IS_NOTEBOOK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSWAT_TYPE_NOTEBOOK))
#define GSWAT_NOTEBOOK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSWAT_TYPE_NOTEBOOK, GSwatNotebookClass))

typedef struct _GSwatNotebook        GSwatNotebook;
typedef struct _GSwatNotebookClass   GSwatNotebookClass;
typedef struct _GSwatNotebookPrivate GSwatNotebookPrivate;

struct _GSwatNotebook
{
    /* add your parent type here */
    GtkNotebook parent;

    /* add pointers to new members here */
    
	/*< private > */
	GSwatNotebookPrivate *priv;
};

struct _GSwatNotebookClass
{
    /* add your parent class here */
    GtkNotebookClass parent_class;

    /* add signals here */
    void (* tab_added)(GSwatNotebook *notebook,
                       GSwatTabable *tabable);
    void (* tab_close_request)(GSwatNotebook *notebook,
                               GSwatTabable *tabable);
};

GType gswat_notebook_get_type(void);

/* add additional methods here */
GSwatNotebook *gswat_notebook_new(void);
void gswat_notebook_insert_page(GSwatNotebook *self,
                                GSwatTabable *tabable,
                                gint position,
                                gboolean jump_to);
G_END_DECLS

#endif /* GSWAT_NOTEBOOK_H */

