
lib_LTLIBRARIES = libgswat-@GSWAT_API_VERSION@.la

#TODO I think gswat-session.c should
# probably be moved out of the core
GSWAT_CORE_SOURCES = \
			gswat-debuggable.c \
			gswat-variable-object.c \
			gswat-gdb-debugger.c \
			gswat-gdb-variable-object.c \
			gswat-gdbmi.c \
			gswat-session.c \
			gswat-session-manager.c


libgswat_@GSWAT_API_VERSION@_la_SOURCES = \
	gswat-init.c \
	$(GSWAT_CORE_SOURCES)

libgswat_@GSWAT_API_VERSION@_la_LIBADD = @LIBGSWAT_DEP_LIBS@
libgswat_@GSWAT_API_VERSION@_la_LDFLAGS = -no-undefined
libgswat_@GSWAT_API_VERSION@_la_CFLAGS = @LIBGSWAT_DEP_CFLAGS@
# TODO once we seperate out the ui code from gswat-session.c then
# lots of these defines can go away.
libgswat_@GSWAT_API_VERSION@_la_CPPFLAGS = \
    -DSHARE_DIR=\"$(pkgdatadir)\" \
    -DGSWAT_LOCALEDIR=\""$(gswatlocaledir)"\" \
    -DGSWAT_BIN_DIR=\""$(bindir)"\" \
    -DG_LOG_DOMAIN=\"GSwat\" \
    -I$(top_srcdir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/gedit \
    @EXTRA_CPPFLAGS@
libgswat_@GSWAT_API_VERSION@_la_includedir = $(includedir)/gswat-@GSWAT_MAJOR_VERSION@.@GSWAT_MINOR_VERSION@/gswat
libgswat_@GSWAT_API_VERSION@_la_include_HEADERS = \
    gswat.h \
    gswat-debuggable.h \
    gswat-variable-object.h \
    gswat-gdb-debugger.h \
    gswat-gdb-variable-object.h \
    gswat-gdbmi.h \
    gswat-session.h


GSWAT_EXTRA_DIST = \
		gswat-debuggable.h \
		gswat-gdb-debugger.h \
		gswat-gdbmi.h \
		gswat-gdb-variable-object.h \
		gswat-session.h \
		gswat-utils.h \
		gswat-variable-object.h \
		gswat-variable-object.h

EXTRA_DIST = \
	$(GSWAT_EXTRA_DIST)


@INTLTOOL_DESKTOP_RULE@


#CLEANFILES =


#DISTCLEANFILES = 

