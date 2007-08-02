
bin_PROGRAMS = gswat gswat-spawner


GSWAT_SOURCES = main.c \
		gswat-window.c \
		gswat-session.c \
		gswat-debuggable.c \
		gswat-variable-object.c \
		gswat-gdb-debugger.c \
		gswat-gdb-variable-object.c \
		gswat-tabable.c \
		gswat-notebook.c \
		gswat-src-view-tab.c \
		gswat-view.c \
		sexy-icon-entry.c \
		gswat-gdbmi.c

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
	$(GSWAT_SOURCES) \
	$(GEDIT_SOURCES) \
	$(DIALOG_SOURCES) \
	$(RHYTHMBOX_SOURCES)

		
gswat_LDADD = @DEPS_LIBS@
gswat_LDFLAGS = -export-dynamic
gswat_CFLAGS = @EXTRA_CFLAGS@ @DEPS_CFLAGS@
gswat_CPPFLAGS = -DGEDIT_GLADEDIR=\""$(datadir)/gswat/glade/"\"	\
	   	 		 -DGSWAT_GLADEDIR=\""$(datadir)/gswat/glade/"\" \
				 -DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" 		\
				 -DSHARE_DIR=\"$(pkgdatadir)\"                  \
				 -DGSWAT_BIN_DIR=\""$(bindir)"\"					\
		 		 -I$(top_srcdir)								\
		 		 -I$(top_srcdir)/src							\
		 		 -I$(top_srcdir)/src/gedit						\
		 		 -I$(top_srcdir)/src/rhythmbox					\
	   	 		 @EXTRA_CPPFLAGS@

gswat_spawner_SOURCES = gswat-spawn.c
gswat_spawner_LDADD = @DEPS_LIBS@
gswat_spawner_CFLAGS = @EXTRA_CFLAGS@ @DEPS_CFLAGS@
gswat_spawner_CPPFLAGS = @EXTRA_CPPFLAGS@


GSWAT_EXTRA_DIST = \
		gswat-debugger.h \
		gswat-gdbmi.h \
		gswat-main-window.h \
		gswat-session.h \
		gswat-spawn.h \
		gswat-view.h \
		sexy-icon-entry.h \
		gswat-variable-object.h \
		templates/g-object-class.c \
		templates/g-object-class.h \
		templates/new_g_object_class.sh \
		templates/g-interface.c \
		templates/g-interface.h \
		templates/new_interface.sh

GEDIT_EXTRA_DIST = \
		gedit-marshal.list \
		gedit-convert.h \
		gedit-debug.h \
		gedit-document.h \
		gedit-document-loader.h \
		gedit-encodings.h \
		gedit-encodings-option-menu.h \
		gedit-help.h \
		gedit-io-error-message-area.h \
		gedit-languages-manager.h \
		gedit-marshal.h \
		gedit-message-area.h \
		gedit-metadata-manager.h \
		gedit-panel.h \
		gedit-prefs-manager-app.h \
		gedit-prefs-manager.h \
		gedit-prefs-manager-private.h \
		gedit-print.h \
		gedit-print-job-preview.h \
		gedit-progress-message-area.h \
		gedit-spinner.h \
		gedit-text-region.h \
		gedit-tooltips.h \
		gedit-utils.h \
		gedit-view.h \
		gedit-window.h

gladedir = $(datadir)/gswat/glade/

glade_DATA = \
	data/glade/gswat.glade						\
	dialogs/gedit-encodings-dialog.glade		\
	dialogs/gedit-open-location-dialog.glade	\
	dialogs/gedit-page-setup-dialog.glade		\
	dialogs/gedit-preferences-dialog.glade		\
	dialogs/gedit-search-dialog.glade

uidir = $(datadir)/gswat/ui/

ui_DATA = \
	data/ui/toolbar-menu-ui.xml

EXTRA_DIST = \
	$(GSWAT_EXTRA_DIST) \
	$(GEDIT_EXTRA_DIST) \
	$(glade_DATA) \
	$(ui_DATA)


gedit/gedit-marshal.h: gedit/gedit-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=gedit_marshal > $@

gedit/gedit-marshal.c: gedit/gedit-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gedit-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gedit_marshal >> $@







