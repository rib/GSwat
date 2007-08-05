SUBDIRS = src po

pcfiles = gswat-@GSWAT_API_VERSION@.pc

pkgconfig_DATA = $(pcfiles)
pkgconfigdir   = $(libdir)/pkgconfig

EXTRA_DIST = \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	config.rpath \
	m4/ChangeLog \
	clutter.pc.in

DISTCLEANFILES = \
        intltool-extract \
        intltool-merge \
        intltool-update

