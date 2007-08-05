
bin_PROGRAMS = gswat gswat-spawner
lib_LTLIBRARIES = libgswat-@GSWAT_API_VERSION@.la

#TODO I think gswat-session.c should
# probably be moved out of the core
GSWAT_CORE_SOURCES = \
			gswat-debuggable.c \
			gswat-variable-object.c \
			gswat-gdb-debugger.c \
			gswat-gdb-variable-object.c \
			gswat-gdbmi.c \
			gswat-session.c

GSWAT_UI_SOURCES = main.c \
		gswat-window.c \
		gswat-tabable.c \
		gswat-notebook.c \
		gswat-src-view-tab.c \
		gswat-view.c \
		gswat-context.c \
		gswat-control-flow-debugging-context.c \
		gswat-source-browsing-context.c \
		gswat-in-limbo-context.c \
		sexy-icon-entry.c


BUILT_SOURCES = gedit/gedit-marshal.c \
               gedit/gedit-marshal.h

GEDIT_SOURCES = $(BUILT_SOURCES) \
	gedit/gedit-document.c \
	gedit/gedit-document-loader.c \
	gedit/gedit-encodings.c \
	gedit/gedit-text-region.c \
	gedit/gedit-metadata-manager.c \
	gedit/gedit-languages-manager.c \
	gedit/gedit-prefs-manager.c \
	gedit/gedit-utils.c \
	gedit/gedit-convert.c \
	gedit/gedit-debug.c \
	gedit/gedit-view.c \
	gedit/gedit-io-error-message-area.c \
	gedit/gedit-message-area.c \
	gedit/gedit-print.c \
	gedit/gedit-print-job-preview.c \
	gedit/gedit-progress-message-area.c \
	gedit/gedit-encodings-option-menu.c \
	gedit/gedit-tooltips.c \
	gedit/gedit-spinner.c \
	gedit/gedit-help.c

DIALOG_SOURCES = \
	dialogs/gedit-page-setup-dialog.c		\
	dialogs/gedit-page-setup-dialog.h		\
	dialogs/gedit-preferences-dialog.h		\
	dialogs/gedit-preferences-dialog.c		\
	dialogs/gedit-close-confirmation-dialog.c 	\
	dialogs/gedit-close-confirmation-dialog.h 	\
	dialogs/gedit-encodings-dialog.c		\
	dialogs/gedit-encodings-dialog.h		\
	dialogs/gedit-open-location-dialog.c		\
	dialogs/gedit-open-location-dialog.h		\
	dialogs/gedit-search-dialog.h			\
	dialogs/gedit-search-dialog.c

RHYTHMBOX_SOURCES = \
	rhythmbox/rb-file-helpers.c \
	rhythmbox/rb-file-helpers.h

gswat_SOURCES = \
	$(GSWAT_CORE_SOURCES) \
	$(GSWAT_UI_SOURCES) \
	$(GEDIT_SOURCES) \
	$(DIALOG_SOURCES) \
	$(RHYTHMBOX_SOURCES)

		
gswat_LDADD = @GSWAT_UI_DEP_LIBS@
gswat_LDFLAGS = -export-dynamic
gswat_CFLAGS = @EXTRA_CFLAGS@ @GSWAT_UI_DEP_CFLAGS@
gswat_CPPFLAGS = -DGEDIT_GLADEDIR=\""$(datadir)/gswat/glade/"\"	\
	   	 		 -DGSWAT_GLADEDIR=\""$(datadir)/gswat/glade/"\" \
				 -DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" 		\
				 -DSHARE_DIR=\"$(pkgdatadir)\"                  \
				 -DGSWAT_LOCALEDIR=\""$(gswatlocaledir)"\"		\
				 -DGSWAT_BIN_DIR=\""$(bindir)"\"	\
		 		 -I$(top_srcdir)								\
		 		 -I$(top_srcdir)/src							\
		 		 -I$(top_srcdir)/src/gedit						\
		 		 -I$(top_srcdir)/src/rhythmbox					\
	   	 		 @EXTRA_CPPFLAGS@

libgswat_@GSWAT_API_VERSION@_la_SOURCES = \
	$(GSWAT_CORE_SOURCES)

libgswat_@GSWAT_API_VERSION@_la_LIBADD = @LIBGSWAT_DEP_LIBS@
libgswat_@GSWAT_API_VERSION@_la_LDFLAGS = -no-undefined
libgswat_@GSWAT_API_VERSION@_la_CFLAGS = @LIBGSWAT_DEP_CFLAGS@
# TODO once we seperate out the ui code from gswat-session.c then
# lots of these defines can go away.
libgswat_@GSWAT_API_VERSION@_la_CPPFLAGS = \
				 -DGEDIT_GLADEDIR=\""$(datadir)/gswat/glade/"\"	\
	   	 		 -DGSWAT_GLADEDIR=\""$(datadir)/gswat/glade/"\" \
				 -DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" 		\
				 -DSHARE_DIR=\"$(pkgdatadir)\"                  \
				 -DGSWAT_LOCALEDIR=\""$(gswatlocaledir)"\"		\
				 -DGSWAT_BIN_DIR=\""$(bindir)"\"	\
		 		 -I$(top_srcdir)								\
		 		 -I$(top_srcdir)/src							\
		 		 -I$(top_srcdir)/src/gedit						\
		 		 -I$(top_srcdir)/src/rhythmbox					\
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


gswat_spawner_SOURCES = gswat-spawn.c
gswat_spawner_LDADD = @GSWAT_SPAWNER_DEP_LIBS@
gswat_spawner_CFLAGS = @EXTRA_CFLAGS@ @GSWAT_SPAWNER_DEP_CFLAGS@
gswat_spawner_CPPFLAGS = @EXTRA_CPPFLAGS@


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
		gswat-spawn.h \
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

GEDIT_EXTRA_DIST = \
		gedit/gedit-marshal.list \
		gedit/gedit-convert.h \
		gedit/gedit-debug.h \
		gedit/gedit-document.h \
		gedit/gedit-document-loader.h \
		gedit/gedit-encodings.h \
		gedit/gedit-encodings-option-menu.h \
		gedit/gedit-help.h \
		gedit/gedit-io-error-message-area.h \
		gedit/gedit-languages-manager.h \
		gedit/gedit-marshal.h \
		gedit/gedit-message-area.h \
		gedit/gedit-metadata-manager.h \
		gedit/gedit-panel.h \
		gedit/gedit-prefs-manager-app.h \
		gedit/gedit-prefs-manager.h \
		gedit/gedit-prefs-manager-private.h \
		gedit/gedit-print.h \
		gedit/gedit-print-job-preview.h \
		gedit/gedit-progress-message-area.h \
		gedit/gedit-spinner.h \
		gedit/gedit-tab.h  \
		gedit/gedit-text-region.h \
		gedit/gedit-tooltips.h \
		gedit/gedit-utils.h \
		gedit/gedit-view.h \
		gedit/gedit-window.h

gladedir = $(datadir)/gswat/glade/

glade_DATA = \
	data/glade/gswat.glade						\
	dialogs/gedit-encodings-dialog.glade		\
	dialogs/gedit-open-location-dialog.glade	\
	dialogs/gedit-page-setup-dialog.glade		\
	dialogs/gedit-preferences-dialog.glade		\
	dialogs/gedit-search-dialog.glade

uidir = $(datadir)/gswat/ui/

ui_DATA = data/ui/toolbar-menu-ui.xml

gnomemenudir = $(datadir)/applications
gnomemenu_in_files = data/gswat.desktop.in
gnomemenu_DATA = $(gnomemenu_premerge_files:.desktop.in=.desktop)

EXTRA_DIST = \
	$(GSWAT_EXTRA_DIST) \
	$(GEDIT_EXTRA_DIST) \
	$(glade_DATA) \
	$(ui_DATA) \
	$(gnomemenu_DATA)


gedit/gedit-marshal.h: gedit/gedit-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=gedit_marshal > $@

gedit/gedit-marshal.c: gedit/gedit-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gedit-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gedit_marshal >> $@

@INTLTOOL_DESKTOP_RULE@


CLEANFILES = \
	$(gnomemenu_DATA)

DISTCLEANFILES = \
	$(DISTCLEANFILES)

