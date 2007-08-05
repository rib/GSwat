#!/bin/sh

exuberant-ctags -R
exuberant-ctags -a -R --exclude=gtkalias.h --exclude=*.html /home/rob/src/gtk+-2.10.14
exuberant-ctags -a -R --exclude=galias.h --exclude=gobjectalias.h --exclude=*.html /home/rob/src/glib-2.12.13
exuberant-ctags -a -R /home/rob/src/gtksourceview-1.6.0
