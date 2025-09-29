#
# Edit Here
#
# The main development platform is currently NetBSD.
# Other platforms may require `bmake`.
#

.POSIX:

# More portable between bmake and gmake that have different IF-ELSE syntax.
# Unix vs Windows extensions; gmake supports BSD's X != cmdline vs X = $(cmdline)
MAKE_OS	!= if test "${.MAKE.OS}" = ''; then uname|sed 's/^CYGWIN.*/Cygwin/'; else echo ${.MAKE.OS}; fi
O	!= if test ${MAKE_OS} = 'Cygwin'; then echo '.obj'; else echo '.o'; fi
E	!= if test ${MAKE_OS} = 'Cygwin'; then echo '.exe'; fi

.SUFFIXES:
.SUFFIXES: .c .h .i $O $E

#######################################################################
# User configurable.
#######################################################################

BUF	:= 131072
MODE	:= 0600
USER	:= 0
GROUP	:= 0

INSTALL_FLAGS != if test ${MAKE_OS} != 'Cygwin'; then echo "-o ${USER} -g ${GROUP}"; fi

# Override from the command-line, eg. make DBG='-O0 -g'
DBG	:= -DNDEBUG
LDDBG	:=
CCONFIG	:= -DPLACEHOLDER

CC	!= if test ${CC} = 'c99'; then echo gcc; else echo ${CC}; fi
LDFLAGS	!= if test ${CC} = 'gcc'; then echo '-fno-ident -flto'; fi

#######################################################################
#
#######################################################################

PROG	?= ./eh$E

BUILT	!= date -u +'%a, %d %b %Y %H:%M:%SZ'
COMMIT	!= git describe --tags

# Common C compiler warnings to silence
#
# -Wno-char-subscripts			ctypes macros
# -Wno-incompatible-pointer-types	atexit(endwin)
# -Wno-unused-parameter			main(int argc, ...)
#
# -Wno-strict-prototypes		functions no arguments
# -Wno-missing-prototypes		clang functiosn without static
# -Wno-missing-variable-declarations	clang globals without static
#
CSILENCE := -Wno-char-subscripts -Wno-incompatible-pointer-types -Wno-unused-parameter \
	   -Wno-strict-prototypes -Wno-unused-value

# Clang with -Weverything complains worse than Gcc -Wpedantic.
#
# CSILENCE+= -Wno-missing-prototypes \
# 	-Wno-missing-variable-declarations -Wno-extra-semi-stmt \
# 	-Wno-c99-compat -Wno-incompatible-function-pointer-types-strict \
# 	-Wno-unsafe-buffer-usage

# Some of the most commonly used includes like ctype.h, stdio.h,
# stdlib.h, and string.h should be ignored or counted as 2, eg.
#
#   #include <stdlib.h>		// count as 2
#
# Including them here doesn't "feel right", bit of cheat.
#
# https://discord.com/channels/1327029557718417438/1332198374249594920/1379483193358684170
#
CINCLUDE := -include curses.h -include ctype.h -include string.h \
	-include stdlib.h -include iso646.h -include regex.h \
	-include fcntl.h -include locale.h -include unistd.h

CFLAGS	:= -std=gnu17 -Os -funsigned-char -Wall -Wextra -pedantic ${CSILENCE} ${DBG}

# Frack need extra #define to enable SUS standard strdup(), strndup().
CPPFLAGS:= -DBUF=${BUF} -DMODE=${MODE} -DBUILT="\"${BUILT}\"" -DCOMMIT="\"${COMMIT}\"" \
	-D_XOPEN_SOURCE=700 ${CCONFIG}

LDFLAGS	:=

# Linux & Cygwin NCurses with wide character support.
LIBS	!= if expr "${MAKE_OS}" : '^.*BSD' >/dev/null; then echo '-lcurses'; else echo '-lncursesw'; fi

MANDIR	!= dirname "$$(find /usr/local -maxdepth 3 -type d -name man1)"
MANDIR  != if test "${MANDIR}" = '.'; then echo /usr/local/share/man; else echo ${MANDIR}; fi

#######################################################################
# Inference Rules
#######################################################################

.c.i:
	${CC} ${CFLAGS} ${CPPFLAGS} -E $*.c >$*.i

.c$E :
	${CC} ${CFLAGS} ${CPPFLAGS} ${LDFLAGS} -o $@ $< ${LIBS}

#######################################################################
#
#######################################################################

.PHONY: all clean clobber distclean nuke install strip size test tests

all: build

build: eh$E

clean:
	-rm -f build.h *.core *.stackdump *.i a.out a.txt b.txt
	-rm -rf test/terminfo.cdb

distclean clobber: clean
	-rm -f eh$E ioccc28/prog$E prog.ext$E prog.ext.c typescript

nuke: distclean
	-rm ioccc28/prog.c

strip: build
	strip eh$E
	ls -l

size: ioccc28/prog.c
	-iocccsize -v1 $>

install: README.md eh$E
	install ${INSTALL_FLAGS} -m 555 eh$E /usr/local/bin
	install ${INSTALL_FLAGS} -d ${MANDIR}/cat1
	install ${INSTALL_FLAGS} -p -m 444 README.md ${MANDIR}/cat1/eh.0

eh$E : eh.c
	${CC} ${CFLAGS} ${CPPFLAGS} ${LDFLAGS} -o  $@ eh.c ${LIBS}

debug: clean
	${MAKE} DBG='-O0 -g -fsanitize=address -fsanitize=pointer-subtract -fsanitize=pointer-compare -lasan' build
	paxctl +a ${PROG}

test:
	${MAKE} -f test/Makefile PROG=${PROG} $@

predefines:
	gcc ${CPPFLAGS} -dM -E -xc /dev/null

#######################################################################
# Generated files.
#######################################################################

ioccc28/prog.c: eh.c transform.sed transform_ioccc.sed
	sed -E -f transform_ioccc.sed eh.c | \
	sed -E -f transform.sed | \
	sed -e'/) {$$/{ N;N;s/ {\(.[[:blank:]]*[^;]*;.\)[[:blank:]]*}$$/\1/; }' | \
	sed -e'/^[[:blank:]]*$$/d' >$@

ioccc28/prog$E: ioccc28/prog.c
	${CC} ${CFLAGS} ${CINCLUDE} ${CPPFLAGS} ${LDFLAGS} -o $@ ioccc28/prog.c ${LIBS}

prog.ext.c: eh.c transform.sed transform_ext.sed
	sed -E -f transform_ext.sed eh.c | \
	sed -E -f transform.sed | \
	sed -e'/) {$$/{ N;N;s/ {\(.[[:blank:]]*[^;]*;.\)[[:blank:]]*}$$/\1/; }' | \
	sed -e'/^[[:blank:]]*$$/d' >$@

prog.ext$E: prog.ext.c

ioccc28/prog.alt$E: ioccc28/prog.alt.c
	${CC} ${CFLAGS} ${CPPFLAGS} -o $@ ioccc28/prog.alt.c ${LIBS}

ioccc28/prog.alt.i: ioccc28/prog.alt.c
	${CC} ${CFLAGS} ${CPPFLAGS} -E ioccc28/prog.alt.c >$*.i
