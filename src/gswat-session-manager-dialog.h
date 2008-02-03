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

#ifndef GSWAT_SESSION_MANAGER_DIALOG_H
#define GSWAT_SESSION_MANAGER_DIALOG_H

#include <glib.h>
#include <glib-object.h>

#include "gswat-session.h"
#include "gswat-session-manager.h"

G_BEGIN_DECLS

#define GSWAT_SESSION_MANAGER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSWAT_TYPE_GSWAT_SESSION_MANAGER_DIALOG, GSwatSessionManagerDialog))
#define GSWAT_TYPE_GSWAT_SESSION_MANAGER_DIALOG            (gswat_session_manager_dialog_get_type())
#define GSWAT_SESSION_MANAGER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSWAT_TYPE_GSWAT_SESSION_MANAGER_DIALOG, GSwatSessionManagerDialogClass))
#define GSWAT_IS_GSWAT_SESSION_MANAGER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSWAT_TYPE_GSWAT_SESSION_MANAGER_DIALOG))
#define GSWAT_IS_GSWAT_SESSION_MANAGER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSWAT_TYPE_GSWAT_SESSION_MANAGER_DIALOG))
#define GSWAT_SESSION_MANAGER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSWAT_TYPE_GSWAT_SESSION_MANAGER_DIALOG, GSwatSessionManagerDialogClass))

typedef struct _GSwatSessionManagerDialog        GSwatSessionManagerDialog;
typedef struct _GSwatSessionManagerDialogClass   GSwatSessionManagerDialogClass;
typedef struct _GSwatSessionManagerDialogPrivate GSwatSessionManagerDialogPrivate;

struct _GSwatSessionManagerDialog
{
    /* add your parent type here */
    GObject parent;

    /* add pointers to new members here */
    
    /*< private > */
    GSwatSessionManagerDialogPrivate *priv;
};

struct _GSwatSessionManagerDialogClass
{
    /* add your parent class here */
    GObjectClass parent_class;

    /* add signals here */
    void (* edit_done)		(GSwatSession *session);
    void (* edit_abort)		(GSwatSession *session);
};

GType gswat_session_manager_dialog_get_type(void);

/* add additional methods here */
GSwatSessionManagerDialog *gswat_session_manager_dialog_new(GSwatSessionManager *manager);

void gswat_session_manager_dialog_edit(GSwatSessionManagerDialog *self);

/* Get the active session */
GSwatSession *gswat_session_manager_dialog_get_current_session(GSwatSessionManagerDialog *self);


G_END_DECLS

#endif /* GSWAT_SESSION_MANAGER_DIALOG_H */

