SUBDIRS = glade_files

bin_PROGRAMS = gswat gswat-spawner

INCLUDES = -DGEDIT_GLADEDIR=\""$(datadir)/gswat/glade/"\"	\
	   -DGSWAT_GLADEDIR=\""$(datadir)/gswat/glade/"\"
	   

BUILT_SOURCES = gedit-marshal.c \
		gedit-marshal.h \
		lex.yy.c \
		gdbmi-grammar.tab.c \
		gdbmi-grammar.tab.h

gswat_SOURCES = 	$(BUILT_SOURCES) \
		main.c \
		gswat-main-window.c \
		gswat-session.c \
		gswat-debugger.c \
		gswat-gdb-variables.c \
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
		sexy-icon-entry.c \
		gdbmi-parse-tree.c \
		gdbmi-parser.c

		
#		gdb.c
#		gedit-prefs-manager-app.c
		
gswat_LDADD = @DEPS_LIBS@
gswat_LDFLAGS = -export-dynamic
gswat_CFLAGS = @EXTRA_CFLAGS@ @DEPS_CFLAGS@
gswat_CPPFLAGS = @EXTRA_CPPFLAGS@

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

lex.yy.c: gdbmi-lexer.l
	flex gdbmi-lexer.l

gdbmi-grammar.tab.h: gdbmi-grammar.y
	bison gdbmi-grammar.y

gdbmi-grammar.tab.c: gdbmi-grammar.y
	bison gdbmi-grammar.y

