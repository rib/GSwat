SUBDIRS = glade_files gedit dialogs

bin_PROGRAMS = gswat gswat-spawner

gswat_SOURCES = main.c \
		gswat-main-window.c \
		gswat-session.c \
		gswat-debugger.c \
		gswat-view.c \
		sexy-icon-entry.c \
		gswat-gdbmi.c \
		gswat-variable-object.c

		
#		gdb.c
#		gedit-prefs-manager-app.c


# libgedit.la and libdialogs.la are dependent on each other so as per
# the GNU info section Multiple dependencies (libtool), the dependency
# is listed twice and we pass --preserve-dup-deps. This seems a
# bit hacky but I'd really like to keep the gedit code organised
# seperatly for neatness.
gswat_LDADD = @DEPS_LIBS@ gedit/libgedit.la dialogs/libdialogs.la gedit/libgedit.la
gswat_LDFLAGS = -export-dynamic --preserve-dup-deps
gswat_CFLAGS = @EXTRA_CFLAGS@ @DEPS_CFLAGS@
gswat_CPPFLAGS = -DGEDIT_GLADEDIR=\""$(datadir)/gswat/glade/"\"	\
	   	 -DGSWAT_GLADEDIR=\""$(datadir)/gswat/glade/"\" \
		 -I$(top_srcdir)/src/gedit \
	   	 @EXTRA_CPPFLAGS@
#gswat_LIBADD = dialogs/libdialogs.la

gswat_spawner_SOURCES = gswat-spawn.c
gswat_spawner_LDADD = @DEPS_LIBS@
gswat_spawner_CFLAGS = @EXTRA_CFLAGS@ @DEPS_CFLAGS@
gswat_spawner_CPPFLAGS = @EXTRA_CPPFLAGS@


EXTRA_DIST =	gswat-debugger.h \
		gswat-gdbmi.h \
		gswat-main-window.h \
		gswat-session.h \
		gswat-spawn.h \
		gswat-view.h \
		sexy-icon-entry.h \
		gswat-variable-object.h
		


