#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME=gswat

(test -f $srcdir/configure.in \
  && test -f $srcdir/autogen.sh \
  && test -d $srcdir/gswat \
  && test -f $srcdir/gswat/gswat.h) || {
    echo -n "**Error**: Directory \"$srcdir\" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

which gnome-autogen.sh || {
    echo "You need to install gnome-common from the GNOME CVS"
    exit 1
}


REQUIRED_AUTOMAKE_VERSION=1.9 
GNOME_DATADIR="$gnome_datadir"
USE_GNOME2_MACROS=1
USE_COMMON_DOC_BUILD=yes

. gnome-autogen.sh

