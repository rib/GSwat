
bin_PROGRAMS = gswat-server gswat-ptrace-stub
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

BUILT_SOURCES = gedit/gedit-marshal.c \
               gedit/gedit-marshal.h


libgswat_@GSWAT_API_VERSION@_la_SOURCES = \
	gswat-init.c \
	$(GSWAT_CORE_SOURCES)

libgswat_@GSWAT_API_VERSION@_la_LIBADD = @LIBGSWAT_DEP_LIBS@
libgswat_@GSWAT_API_VERSION@_la_LDFLAGS = -no-undefined
libgswat_@GSWAT_API_VERSION@_la_CFLAGS = @LIBGSWAT_DEP_CFLAGS@
# TODO once we seperate out the ui code from gswat-session.c then
# lots of these defines can go away.
libgswat_@GSWAT_API_VERSION@_la_CPPFLAGS = \
    -DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" \
    -DSHARE_DIR=\"$(pkgdatadir)\" \
    -DGSWAT_LOCALEDIR=\""$(gswatlocaledir)"\" \
    -DGSWAT_BIN_DIR=\""$(bindir)"\" \
    -DG_LOG_DOMAIN=\"GSwat\" \
    -I$(top_srcdir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/gedit \
    @EXTRA_CPPFLAGS@
libgswat_@GSWAT_API_VERSION@_la_includedir = $(includedir)/gswat-@GSWAT_MAJOR_VERSION@.0
libgswat_@GSWAT_API_VERSION@_la_include_HEADERS = \
    gswat.h \
    gswat-debuggable.h \
    gswat-variable-object.h \
    gswat-gdb-debugger.h \
    gswat-gdb-variable-object.h \
    gswat-gdbmi.h \
    gswat-session.h


gswat_server_SOURCES = gswat-server.c
gswat_server_LDADD = @GSWAT_SERVER_DEP_LIBS@
gswat_server_CFLAGS = @EXTRA_CFLAGS@ @GSWAT_SERVER_DEP_CFLAGS@
gswat_server_CPPFLAGS = @EXTRA_CPPFLAGS@

gswat_ptrace_stub_SOURCES = gswat-ptrace-stub.c
gswat_ptrace_stub_LDADD = @GSWAT_PTRACE_STUB_DEP_LIBS@
gswat_ptrace_stub_CFLAGS = @EXTRA_CFLAGS@ @GSWAT_PTRACE_STUB_DEP_CFLAGS@
gswat_ptrace_stub_CPPFLAGS = @EXTRA_CPPFLAGS@


GSWAT_EXTRA_DIST = \
		gswat-context.h \
		gswat-control-flow-debugging-context.h \
		gswat-debuggable.h \
		gswat-gdb-debugger.h \
		gswat-gdbmi.h \
		gswat-gdb-variable-object.h \
		gswat-notebook.h \
		gswat-session.h \
		gswat-source-browsing-context.h \
		gswat-server.h \
		gswat-ptrace-stub.h \
		gswat-src-view-tab.h \
		gswat-tabable.h \
		gswat-utils.h \
		gswat-variable-object.h \
		gswat-view.h \
		gswat-window.h \
		gswat-in-limbo-context.h \
		sexy-icon-entry.h \
		gswat-variable-object.h \
		templates/g-object-class.c \
		templates/g-object-class.h \
		templates/new_g_object_class.sh \
		templates/g-interface.c \
		templates/g-interface.h \
		templates/new_g_interface.sh

EXTRA_DIST = \
	$(GSWAT_EXTRA_DIST)


@INTLTOOL_DESKTOP_RULE@


#CLEANFILES =


#DISTCLEANFILES = 

