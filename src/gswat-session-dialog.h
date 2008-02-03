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

#ifndef GSWAT_GSWAT_SESSION_DIALOG_H
#define GSWAT_GSWAT_SESSION_DIALOG_H

#include <glib.h>
#include <glib-object.h>
/* include your parent object here */

G_BEGIN_DECLS

#define GSWAT_GSWAT_SESSION_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSWAT_TYPE_GSWAT_SESSION_DIALOG, GSwatGSwatSessionDialog))
#define GSWAT_TYPE_GSWAT_SESSION_DIALOG            (gswat_gswat_session_dialog_get_type())
#define GSWAT_GSWAT_SESSION_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSWAT_TYPE_GSWAT_SESSION_DIALOG, GSwatGSwatSessionDialogClass))
#define GSWAT_IS_GSWAT_SESSION_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSWAT_TYPE_GSWAT_SESSION_DIALOG))
#define GSWAT_IS_GSWAT_SESSION_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSWAT_TYPE_GSWAT_SESSION_DIALOG))
#define GSWAT_GSWAT_SESSION_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSWAT_TYPE_GSWAT_SESSION_DIALOG, GSwatGSwatSessionDialogClass))

typedef struct _GSwatGSwatSessionDialog        GSwatGSwatSessionDialog;
typedef struct _GSwatGSwatSessionDialogClass   GSwatGSwatSessionDialogClass;
typedef struct _GSwatGSwatSessionDialogPrivate GSwatGSwatSessionDialogPrivate;

struct _GSwatGSwatSessionDialog
{
    /* add your parent type here */
    GSwatGSwatSessionDialogParent parent;

    /* add pointers to new members here */
    
	/*< private > */
	GSwatGSwatSessionDialogPrivate *priv;
};

struct _GSwatGSwatSessionDialogClass
{
    /* add your parent class here */
    GSwatGSwatSessionDialogParentClass parent_class;

    /* add signals here */
    //void (* signal) (GSwatGSwatSessionDialog *object);
};

GType gswat_gswat_session_dialog_get_type(void);

/* add additional methods here */
GSwatGSwatSessionDialog *gswat_gswat_session_dialog_new(void);

G_END_DECLS

#endif /* GSWAT_GSWAT_SESSION_DIALOG_H */

