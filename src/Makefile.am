bin_PROGRAMS = ggdb
ggdb_SOURCES = ggdb.c gg_main_window.c gdb.c gg_debugger.c gg_gdb_variable.c
ggdb_LDADD = @DEPS_LIBS@
ggdb_LDFLAGS = -export-dynamic
ggdb_CFLAGS = @EXTRA_CFLAGS@ @DEPS_CFLAGS@
ggdb_CPPFLAGS = @EXTRA_CPPFLAGS@

EXTRA_DIST = signals.h
