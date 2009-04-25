/*
 * GSwat
 *
 * An object oriented debugger abstraction library
 *
 * Copyright (C) 2006-2009 Robert Bragg <robert@sixbynine.org>
 *
 * GSwat is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or  (at your option)
 * any later version.
 *
 * GSwat is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with GSwat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GSWAT_DEBUG_H
#define GSWAT_DEBUG_H

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
    GSWAT_DEBUG_LOG	  = 1 << 0,
    GSWAT_DEBUG_MISC	  = 1 << 1,
    GSWAT_DEBUG_GDB_TRACE = 1 << 2,
    GSWAT_DEBUG_GDBMI	  = 1 << 3
} GSwatDebugFlag;

#define GSWAT_DEBUG(TYPE, FORMAT, ARGS...) \
  G_STMT_START { \
      if (gswat_debug_flags & GSWAT_DEBUG_##TYPE) \
        { g_message ("[" #TYPE "] " G_STRLOC ": " FORMAT, ##ARGS); } \
  } G_STMT_END

extern guint gswat_debug_flags;
extern int gswat_log_file;

void gswat_log (const char *message);

#endif /* GSWAT_DEBUG_H */

