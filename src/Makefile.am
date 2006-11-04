SUBDIRS = glade_files dialogs

bin_PROGRAMS = gswat gswat-spawner

INCLUDES = -DGEDIT_GLADEDIR=\""$(datadir)/gswat/glade/"\"	\
	   -DGSWAT_GLADEDIR=\""$(datadir)/gswat/glade/"\"

BUILT_SOURCES = gedit-marshal.c \
               gedit-marshal.h	   

gswat_SOURCES = $(BUILT_SOURCES) \
		main.c \
		gswat-main-window.c \
		gswat-session.c \
		gswat-debugger.c \
		gswat-view.c \
		gedit-document.c \
		gedit-document-loader.c \
		gedit-encodings.c \
		gedit-text-region.c \
		gedit-metadata-manager.c \
		gedit-languages-manager.c \
		gedit-prefs-manager.c \
		gedit-utils.c \
		gedit-convert.c \
		gedit-debug.c \
		gedit-view.c \
		gedit-tab.c \
		gedit-io-error-message-area.c \
		gedit-message-area.c \
		gedit-notebook.c \
		gedit-print.c \
		gedit-print-job-preview.c \
		gedit-progress-message-area.c \
		gedit-encodings-option-menu.c \
		gedit-tooltips.c \
		gedit-spinner.c \
		gedit-help.c \
		sexy-icon-entry.c \
		gswat-gdbmi.c

		
#		gdb.c
#		gedit-prefs-manager-app.c
		
gswat_LDADD = @DEPS_LIBS@ dialogs/libdialogs.la
gswat_LDFLAGS = -export-dynamic
gswat_CFLAGS = @EXTRA_CFLAGS@ @DEPS_CFLAGS@
gswat_CPPFLAGS = @EXTRA_CPPFLAGS@
#gswat_LIBADD = dialogs/libdialogs.la

gswat_spawner_SOURCES = gswat-spawn.c
gswat_spawner_LDADD = @DEPS_LIBS@
gswat_spawner_CFLAGS = @EXTRA_CFLAGS@ @DEPS_CFLAGS@
gswat_spawner_CPPFLAGS = @EXTRA_CPPFLAGS@


EXTRA_DIST = signals.h



gedit-marshal.h: gedit-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=gedit_marshal > $@

gedit-marshal.c: gedit-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gedit-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gedit_marshal >> $@


