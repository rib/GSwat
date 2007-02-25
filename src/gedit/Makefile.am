
INCLUDES = 							\
	-I$(top_srcdir)						\
	-I$(top_srcdir)/src

noinst_LTLIBRARIES = libgedit.la

BUILT_SOURCES = gedit-marshal.c \
               gedit-marshal.h	   

libgedit_la_SOURCES = \
	$(BUILT_SOURCES) \
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
	gedit-io-error-message-area.c \
	gedit-message-area.c \
	gedit-print.c \
	gedit-print-job-preview.c \
	gedit-progress-message-area.c \
	gedit-encodings-option-menu.c \
	gedit-tooltips.c \
	gedit-spinner.c \
	gedit-help.c


libgedit_la_LIBADD = 	@DEPS_LIBS@
libgedit_la_LDFLAGS = 	-export-dynamic
libgedit_la_CFLAGS = 	@EXTRA_CFLAGS@ @DEPS_CFLAGS@
libgedit_la_CPPFLAGS = 	@EXTRA_CPPFLAGS@ \
			-DGEDIT_GLADEDIR=\""$(datadir)/gswat/glade/"\" \
			-DGSWAT_GLADEDIR=\""$(datadir)/gswat/glade/"\"

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


gedit-marshal.h: gedit-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=gedit_marshal > $@

gedit-marshal.c: gedit-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gedit-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gedit_marshal >> $@

