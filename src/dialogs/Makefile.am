gladedir = $(datadir)/gswat/glade/

INCLUDES = 							\
	-I$(top_srcdir)						\
	-I$(top_srcdir)/src 					\
	-I$(top_srcdir)/src/gedit 				\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" 		\
	-DGEDIT_GLADEDIR=\""$(datadir)/gswat/glade/"\"		\
	-DGSWAT_GLADEDIR=\""$(datadir)/gswat/glade/"\"

noinst_LTLIBRARIES = libdialogs.la

libdialogs_la_SOURCES = 			\
	gedit-page-setup-dialog.c		\
	gedit-page-setup-dialog.h		\
	gedit-preferences-dialog.h		\
	gedit-preferences-dialog.c		\
	gedit-close-confirmation-dialog.c 	\
	gedit-close-confirmation-dialog.h 	\
	gedit-encodings-dialog.c		\
	gedit-encodings-dialog.h		\
	gedit-open-location-dialog.c		\
	gedit-open-location-dialog.h		\
	gedit-search-dialog.h			\
	gedit-search-dialog.c

libdialogs_la_LIBADD = @DEPS_LIBS@
libdialogs_la_LDFLAGS = -export-dynamic
libdialogs_la_CFLAGS = @EXTRA_CFLAGS@ @DEPS_CFLAGS@
libdialogs_la_CPPFLAGS = @EXTRA_CPPFLAGS@



glade_DATA =					\
	gedit-encodings-dialog.glade		\
	gedit-open-location-dialog.glade	\
	gedit-page-setup-dialog.glade		\
	gedit-preferences-dialog.glade		\
	gedit-search-dialog.glade



EXTRA_DIST = $(glade_DATA) 

