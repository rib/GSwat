SUBDIRS = glade_files gedit dialogs

bin_PROGRAMS = gswat gswat-spawner

gswat_SOURCES = main.c \
		gswat-main-window.c \
		gswat-session.c \
		gswat-debugger.c \
		gswat-view.c \
		sexy-icon-entry.c \
		gswat-gdbmi.c

		
#		gdb.c
#		gedit-prefs-manager-app.c
		
gswat_LDADD = @DEPS_LIBS@ gedit/libgedit.la dialogs/libdialogs.la
gswat_LDFLAGS = -export-dynamic
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


EXTRA_DIST =	gedit-marshal.list \
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
		gedit-notebook.h \
		gedit-panel.h \
		gedit-prefs-manager-app.h \
		gedit-prefs-manager.h \
		gedit-prefs-manager-private.h \
		gedit-print.h \
		gedit-print-job-preview.h \
		gedit-progress-message-area.h \
		gedit-spinner.h \
		gedit-tab.h \
		gedit-text-region.h \
		gedit-tooltips.h \
		gedit-utils.h \
		gedit-view.h \
		gedit-window.h \
		gswat-debugger.h \
		gswat-gdbmi.h \
		gswat-main-window.h \
		gswat-session.h \
		gswat-spawn.h \
		gswat-view.h \
		sexy-icon-entry.h




gedit-marshal.h: gedit-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=gedit_marshal > $@

gedit-marshal.c: gedit-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gedit-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gedit_marshal >> $@


