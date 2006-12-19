/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-languages-manager.h
 * This file is part of gedit
 *
 * Copyright (C) 2003-2005 - Paolo Maggi 
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
 * Modified by the gedit Team, 2003-2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id: gedit-languages-manager.h,v 1.6 2006/01/27 11:51:11 pborelli Exp $
 */

#ifndef __GEDIT_LANGUAGES_MANAGER_H__
#define __GEDIT_LANGUAGES_MANAGER_H__

#include <gtksourceview/gtksourcelanguagesmanager.h>

G_BEGIN_DECLS

GtkSourceLanguagesManager *gedit_get_languages_manager    (void);

GtkSourceLanguage	  *gedit_languages_manager_get_language_from_id
							  (GtkSourceLanguagesManager *lm,
							   const gchar               *lang_id);

void			   gedit_language_set_tag_style	  (GtkSourceLanguage         *language,
							   const gchar               *tag_id,
							   const GtkSourceTagStyle   *style);

void 			   gedit_language_init_tag_styles (GtkSourceLanguage         *language);

const GSList		  *gedit_languages_manager_get_available_languages_sorted
							  (GtkSourceLanguagesManager *lm);

G_END_DECLS

#endif /* __GEDIT_LANGUAGES_MANAGER_H__ */
