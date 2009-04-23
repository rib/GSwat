
noinst_PROGRAMS = \
	test-connection

GSWAT_LIB=$(top_builddir)/gswat/libgswat-@GSWAT_MAJOR_VERSION@.@GSWAT_MINOR_VERSION@.la
GSWAT_INCLUDES=-I$(top_srcdir)

test_connection_SOURCES = test-connection.c
test_connection_CFLAGS = $(GSWAT_INCLUDES) @LIBGSWAT_DEP_CFLAGS@
test_connection_LDADD = $(GSWAT_LIB) @LIBGSWAT_DEP_LIBS@

