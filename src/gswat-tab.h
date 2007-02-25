/*
 * gedit-tab.h
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
 *
 * $Id: gedit-tab.h,v 1.4 2006/02/19 18:01:09 pborelli Exp $
 */

#ifndef __GSWAT_TAB_H__
#define __GSWAT_TAB_H__

#include <gtk/gtk.h>

#include "gedit-document.h"
#include "gedit-print.h"

#include "gswat-view.h"

G_BEGIN_DECLS

typedef enum
{
	GSWAT_TAB_STATE_NORMAL = 0,
	GSWAT_TAB_STATE_LOADING,
	GSWAT_TAB_STATE_REVERTING,
	GSWAT_TAB_STATE_SAVING,	
	GSWAT_TAB_STATE_PRINTING,
	GSWAT_TAB_STATE_PRINT_PREVIEWING,
	GSWAT_TAB_STATE_SHOWING_PRINT_PREVIEW,
	GSWAT_TAB_STATE_GENERIC_NOT_EDITABLE,
	GSWAT_TAB_STATE_LOADING_ERROR,
	GSWAT_TAB_STATE_REVERTING_ERROR,	
	GSWAT_TAB_STATE_SAVING_ERROR,
	GSWAT_TAB_STATE_GENERIC_ERROR,
	GSWAT_TAB_STATE_CLOSING,
	GSWAT_TAB_NUM_OF_STATES /* This is not a valid state */
} GSwatTabState;

/*
 * Type checking and casting macros
 */
#define GSWAT_TYPE_TAB              (gswat_tab_get_type())
#define GSWAT_TAB(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), GSWAT_TYPE_TAB, GSwatTab))
#define GSWAT_TAB_CONST(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), GSWAT_TYPE_TAB, GSwatTab const))
#define GSWAT_TAB_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), GSWAT_TYPE_TAB, GSwatTabClass))
#define GSWAT_IS_TAB(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), GSWAT_TYPE_TAB))
#define GSWAT_IS_TAB_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GSWAT_TYPE_TAB))
#define GSWAT_TAB_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GSWAT_TYPE_TAB, GSwatTabClass))

/* Private structure type */
typedef struct _GSwatTabPrivate GSwatTabPrivate;

/*
 * Main object structure
 */
typedef struct _GSwatTab GSwatTab;

struct _GSwatTab 
{
	GtkVBox vbox;

	/*< private > */
	GSwatTabPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _GSwatTabClass GSwatTabClass;

struct _GSwatTabClass 
{
	GtkVBoxClass parent_class;
};

/*
 * Public methods
 */
GType 		 gswat_tab_get_type 		(void) G_GNUC_CONST;

GSwatView	*gswat_tab_get_view		(GSwatTab            *tab);

/* This is only an helper function */
GeditDocument	*gswat_tab_get_document		(GSwatTab            *tab);

GSwatTab	*gswat_tab_get_from_document	(GeditDocument       *doc);

GSwatTabState	 gswat_tab_get_state		(GSwatTab	     *tab);

gboolean	 gswat_tab_get_auto_save_enabled	
						(GSwatTab            *tab); 
#if 0
void		 gswat_tab_set_auto_save_enabled	
						(GSwatTab            *tab, 
						 gboolean            enable);

gint		 gswat_tab_get_auto_save_interval 
						(GSwatTab            *tab);

void		 gswat_tab_set_auto_save_interval 
						(GSwatTab            *tab, 
						 gint                interval);
#endif

/*
 * Non exported methods
 */
GtkWidget 	*_gswat_tab_new 		(void);

/* Whether create is TRUE, creates a new empty document if location does 
   not refer to an existing file */
GtkWidget	*_gswat_tab_new_from_uri	(const gchar         *uri,
						 const GeditEncoding *encoding,
						 gint                 line_pos,
						 gboolean             create);
gchar 		*_gswat_tab_get_name		(GSwatTab            *tab);
gchar 		*_gswat_tab_get_tooltips	(GSwatTab            *tab);
GdkPixbuf 	*_gswat_tab_get_icon		(GSwatTab            *tab);
void		 _gswat_tab_load		(GSwatTab            *tab,
						 const gchar         *uri,
						 const GeditEncoding *encoding,
						 gint                 line_pos,
						 gboolean             create);
void		 _gswat_tab_revert		(GSwatTab            *tab);
void		 _gswat_tab_save		(GSwatTab            *tab);
void		 _gswat_tab_save_as		(GSwatTab            *tab,
						 const gchar         *uri,
						 const GeditEncoding *encoding);
void		 _gswat_tab_print		(GSwatTab            *tab,
						 GeditPrintJob       *pjob,
						 GtkTextIter         *start, 
			  			 GtkTextIter         *end);
void		 _gswat_tab_print_preview	(GSwatTab            *tab,
						 GeditPrintJob       *pjob,
						 GtkTextIter         *start, 
			  			 GtkTextIter         *end);
void		 _gswat_tab_mark_for_closing	(GSwatTab	     *tab);

gboolean	 _gswat_tab_can_close		(GSwatTab	     *tab);

G_END_DECLS

#endif  /* __GSWAT_TAB_H__  */

