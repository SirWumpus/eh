#
# Edit Here - Functional Tests
#

.POSIX:
.SUFFIXES:
.SUFFIXES: .txt .out .cdb

.PHONY: all clean clobber distclean test tests

all: test

clean:
	-rm -f *.core *.stackdump a.out a.txt b.txt
	-rm -rf test/terminfo.cdb

distclean clobber: clean
	-rm -f typescript

#######################################################################
# Tests
#
# The basic terminals, like `dumb`, `glasstty`, `vanilla`, `lpr` are
# insufficient to drive NetBSD Curses.  Try `TERM=ansi-mini` for bare
# bones.
#
# For testing and debugging define a terminal where the defined escape
# sequences are readable text.  This avoids differences between terminal
# definitions across platforms.
#
# However, differences in Curses libraries, ie. NetBSD Curses vs Linux
# NCurses can reorder escape sequences to produce the same visual result.
# Similarly library upgrades might reorder escape sequences as seen between
# NetBSD 9.3 and 10.0.
#
#######################################################################

PROG	?= ./prog

# Set ASK=true to prompt to update a failed test.
ASK	?= false

# Set REBUILD=true to update all the failed tests.
REBUILD	?= false

# Curses implementation.
CURSES_IMP != if grep "NCURSES_VERSION" /usr/include/curses.h >/dev/null; then echo "NCurses"; else echo "Curses"; fi

T		= test
TESTTERM	= $T/${CURSES_IMP}/${TERM}
DATA		= $T/${CURSES_IMP}/data
RESET		= rm -f a.txt a.out *.core
TITLE		= printf "%s      %s: " ${PROG} $@ ; ${RESET}
PASS		= printf "\r%s -OK-\r\n" ${PROG}
SKIP		= printf "\r%s SKIP\r\n" ${PROG}
PROMPT		= ( ${ASK} && echo -n "Replace y/N$$PS2" && read yn && test "$$yn" = 'y' )
SAVE_OUT	= ( ${REBUILD} || ${PROMPT} ) && cp a.out ${TESTTERM}/$@.out
SAVE_TXT	= ( ${REBUILD} || ${PROMPT} ) && cp a.txt ${DATA}/$@.txt
A_CORE		= test ! -f *.core
A_OUT		= ( diff -u ${TESTTERM}/$@.out a.out || ${SAVE_OUT} )
A_TXT		= ( diff -u ${DATA}/$@.txt a.txt || ${SAVE_TXT} )
A_OUT_OR_SKIP	= ( ${A_OUT} >/dev/null && ${PASS} ) || ${SKIP}
A_TXT_OR_SKIP	= ( diff -u ${DATA}/$@.txt a.txt ) || ${SKIP}

# Be sure to build the target binary separately before hand.  This allows
# testing an alternative binary by setting PROG, eg. make PROG=./boo test

test:
	@printf '###\n### YOU ARE HERE >>> %s <<<\n###\n' "$$(pwd)"
	SHELL=/bin/sh ${MAKE} -f $T/Makefile PROG=${PROG} TERM=textterm LINES=24 COLUMNS=80 TERMINFO=$${PWD}/$T/terminfo.cdb tests

TESTS:= tests_have_term_data \
	arg0 term0 empty0 empty1 empty3 redraw0 flip0 flip1 \
	ins0 ins1 ins2 ins3 ins4 ins5 \
	open0 open1 open2 open3 \
	del0 del1 del2 del3 del4 del5 del6 \
	scroll0 scroll1 scroll2 \
	goto0 goto1 goto2 goto3 \
	pgdn0 pgdn1 pgdn2 pgdn3 pgdn4 pgup0 pgup1 pgup2 pgup3 \
	ere0 ere1 ere2 ere3 ere4 ere5 ere6 ere7 ere8 ere9 \
	ere10 ere11 ere12 ere13 \
	mark0 mark1 mark2 mark3 mark4 mark5 \
	del7 del8 del9 del10 del11 del12 del13 \
	undo0 undo1 undo2 undo3 undo4 \
	paste0 paste1 paste2 paste3 \
	bang0 bang1 bang2 bang3 bang4 bang5 bang6 bang7 \
	replace0 replace1 replace2 write0 read0 read1 \
	select0 select1 select2 select3 select4 select5 \
	mb0 mb1 mb2 mb3 mb4 imb0 imb1 imb2 imb3 \
	replace3 replace4 \
	cc0 cc1 dd0 dd1 dd2 dd3 \
	indent0 indent1 indent2 indent3 indent4 indent5 indent6 \
	outdent0 outdent1

.PHONY : ${TESTS}
.SILENT: ${TESTS}

tests: $T/terminfo.cdb ${TESTS} clean

tests_skipped: empty2 write1

# While the terminal definition will probably contain definitions for
# the number of hardware `lines` and `columns`, we explicitly override
# them to be sure to ensure terminal emulator dimensions are ignored.

$T/terminfo.cdb : $T/terminfo
	tic -x -o $@ $T/terminfo

tests_have_term_data:
	@test -d ${TESTTERM}
	@echo TERM=${TERM}

# Allow missing filename argument.
arg0:
	-${TITLE}
	-printf "\022Q" | ${PROG} >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Dumb terminal.
term0:
	-${TITLE}
	TERM=BOGUS ${PROG} a.txt 2>/dev/null || test $$? -eq 1
	${A_CORE}
	-${PASS}

# An unknown command character redraws the screen.
redraw0:
	-${TITLE}
	printf "\022Q" | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Start and stop.
empty0:
	-${TITLE}
	printf "Q" | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Start, write an empty buffer, and stop.  Creates a.txt.
empty1:
	-${TITLE}
	printf "W\nQ" | ${PROG} a.txt >a.out
	diff -u ${DATA}/empty.txt a.txt
	${A_CORE}
	${A_OUT}
	-${PASS}

# This test does not work since backspace in redirected input is
# treated differently from a tty.  See NetBSD 10.0 lib/58151
empty2:
	-${TITLE}
	printf 'W\b\b\b\b\bb.txt\nQ' | ${PROG} a.txt >a.out
	test ! -f a.txt && diff -u ${DATA}/empty.txt b.txt
	${A_CORE}
	${A_OUT}
	-${PASS}

# Start and stop.
empty3:
	-${TITLE}
	printf "\003" | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

flip0:
	-${TITLE}
	printf "iHELLO world!\e|~~~~~~~~~~~~W\nQ" | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

flip1:
	-${TITLE}
	printf "iHello World!\e|12~W\nQ" | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Start, insert nothing, and stop.
ins0:
	-${TITLE}
	printf "i\eQ" | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Insert newline terminated, write, and stop.
ins1:
	-${TITLE}
	printf "iHello\n\tworld!\n\eW\nQ" | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Insert not newline terminated, write, and stop.
#
# + Insert `i` and type some text, newline, more text, `ESC`.  TOP should
#   remain unchanged, newline pushes `^D` down a row.
ins2:
	-${TITLE}
	printf "iHello\n\twoot!\eW\nQ" | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# + Word right `L`, insert `i`, type some text, then `ESC`.  Any text
#   right of cursor should shift as new text inserted.
ins3:
	-${TITLE}
	printf "iHello world!\e|wiWOOT! \eW\nQ" | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# + `G`, insert `i` and type in a word, newline, next word, `ESC`.  TOP
#    should remain unchanged, newline pushes `^D` down a row.
ins4:
	-${TITLE}
	printf "Hello\n\tworld!\n" >a.txt
	printf "Giinsert at EOF\npush EOF down\neach newline\n\eW\nQ" | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

ins5:
	-${TITLE}
	printf "Hello\n\tworld!\n" >a.txt
	printf 'j99|i WOOT!\eW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

open0:
	-${TITLE}
	printf 'o123\eW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

open1:
	-${TITLE}
	printf 'O123\eW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

open2:
	-${TITLE}
	printf 'oHello\eoworld!\eW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

open3:
	-${TITLE}
	printf 'Oworld!\eOHello\eW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Delete from empty buffer.
del0:
	-${TITLE}
	printf "xQy" | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Insert nothing, delete from empty buffer.
del1:
	-${TITLE}
	printf "i\exQy" | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Insert single character, delete from buffer.
del2:
	-${TITLE}
	printf "iZ\ehxW\nQy" | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	diff -u ${DATA}/empty.txt a.txt
	-${PASS}

# Insert some characters, delete last from buffer.
del3:
	-${TITLE}
	printf "i123Z\ehxW\nQ" | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Insert some characters with newline, delete newline from buffer.
del4:
	-${TITLE}
	printf "i123Z\n\ehxW\nQy" | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# + Delete `x` a character.  Text to the right of cursor shifts left one.
del5:
	-${TITLE}
	printf "Hello\n\tworld!\n" >a.txt
	printf "GkxhxW\nQy" | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

del6:
	-${TITLE}
	printf "Hello\n\tworld!\n" >a.txt
	printf "Gk6xW\nQy" | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# + Cursor down `j` from BOF to EOF.  Only cursor moves.
# + Cursor up `k` from EOF to BOF.  Only cursor moves.
scroll0:
	-${TITLE}
	printf "jjjjjjjjjjjkkkkkkkkkkkQ" | ${PROG} $T/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# + Cursor down `j` from BOF to BOP.  Only cursor moves.
# + Cursor down `j` twice from BOP.  Screen scrolls up two logical lines.
# + Cursor up `k` to TOP.  Only cursor moves.
# + Cursor up `k` twice from TOP.  Screen scrolls down two logical lines.
scroll1:
	-${TITLE}
	printf "jjjjjjjjjjjjjjjjjjjjjjjjkkkkkkkkkkkkkkkkkkkkkkkkQ" | ${PROG} test/usage.md >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

scroll2:
	-${TITLE}
	printf "24j24kQ" | ${PROG} test/usage.md >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# The goto line `G` always repositions the target line at the top of
# the page by design.  This can look a little odd on short less than
# a screen of text, since it does not simply reposition the cursor
# within the screen.  Or jumping to the EOF, since there is no context
# displayed (just cursor up and down a line).
#
# One can debate the merits of this design choice when compared to
# other editors that try to position the target line in the middle
# of the screen or highlight the cursor row or both.  While pretty
# and probably less shocking, positioning the target line at top
# makes for consistency display and easier testing.

# Goto line `<num>G` where `<num>` is on the screen.  Screen redraws
# with target line at TOP.
goto0:
	-${TITLE}
	printf "7G15G2G1GQ" | ${PROG} test/usage.md >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Cursor at BOF, cursor down `j` a few lines on the screen, `1G`.
# Cursor moves to BOF.
goto1:
	-${TITLE}
	printf "jjjjjjjjjjjjjjj1GQ" | ${PROG} test/usage.md >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Goto EOF `G`.  No terminating newline.
goto2:
	-${TITLE}
	printf "GhkkQ" | ${PROG} test/usage-nonl.md >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Goto EOF `G`.
goto3:
	-${TITLE}
	printf "GhkkQ" | ${PROG} test/usage.md >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Page down `J`.  Short file, no terminating newline.
pgdn0:
	-${TITLE}
	printf "JQ" | ${PROG} test/short-nonl.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Page down `J`.  Short file, terminating newline.
pgdn1:
	-${TITLE}
	printf "JQ" | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Page down to EOF.
pgdn2:
	-${TITLE}
	printf "JJJJQ" | ${PROG} test/usage.md >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

pgdn3:
	-${TITLE}
	printf "3JQ" | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

pgdn4:
	-${TITLE}
	printf "3JJQ" | ${PROG} test/usage.md >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# + Page up `K` once.  Screen remains unchanged with cursor at TOP and BOF.
# + Page down `J` once.  Cursor moves to EOF.
# + Page up `K`.  Screen redraws its starting point with cursor at TOP and BOF.
pgup0:
	-${TITLE}
	printf "KJKQ" | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Page down and up a couple of pages.
pgup1:
	-${TITLE}
	printf "JJKKQ" | ${PROG} test/usage.md >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Goto EOF and page up to BOF.
pgup2:
	-${TITLE}
	printf "GKKKKQ" | ${PROG} test/usage.md >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

pgup3:
	-${TITLE}
	printf "G3KKQ" | ${PROG} test/usage.md >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Invalid ERE pattern.
ere0:
	-${TITLE}
	printf "/)(\nQ" | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Start of lines.
ere1:
	-${TITLE}
	printf "/^\nnnnnnnnnnnQ" | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Start of lines (no NL at EOL).
ere2:
	-${TITLE}
	printf "/^\nnnnnnnnnnQ" | ${PROG} test/short-nonl.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# End of lines.
ere3:
	-${TITLE}
	printf "/$$\nnnnnnnnnnnQ" | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# End of lines (no NL at EOL)
ere4:
	-${TITLE}
	printf "/$$\nnnnnnnnnnnQ" | ${PROG} test/short-nonl.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Last character of line.
ere5:
	-${TITLE}
	printf "/.$$\nnnnnnnnnQ" | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Last character of line (no NL at EOL).
ere6:
	-${TITLE}
	printf "/.$$\nnnnnnnnnQ" | ${PROG} test/short-nonl.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Blank lines.
ere7:
	-${TITLE}
	printf "/^$$\nnnQ" | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Blank lines (no NL at EOL).
ere8:
	-${TITLE}
	printf "/^$$\nnnQ" | ${PROG} test/short-nonl.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Plain text
ere9:
	-${TITLE}
	printf "/line\nnnnQ" | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Blank lines.
ere10:
	-${TITLE}
	printf "/line\nnnnQ" | ${PROG} test/short-nonl.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# No match found.
ere11:
	-${TITLE}
	printf "6w/WOOT\nQ" | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Match \t.
ere12:
	-${TITLE}
	printf '/\\t\nnQ' | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Match \( and \{ different from meta ( and {.
ere13:
	-${TITLE}
	printf "Hello(int arg) {\n\tworld();\n\txyzzy(arg);\n\tcave(lost());\n}\n" >a.txt
	printf '/\\)\nnnnnn/\\}\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Unset set mark moves cursor to BOF.
mark0:
	-${TITLE}
	printf '4w``Q' | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	printf '4w`aQ' | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	printf '4w`zQ' | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Set a mark, move, goto mark, goto previous.
mark1:
	-${TITLE}
	printf 'wm`Jb````Q' | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	printf 'wmaJb`a``Q' | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	printf 'wmzJb`z``Q' | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Set some marks, edit before the marks, marks remain unchanged.
mark2:
	-${TITLE}
	printf '/butt\nmajmb/rude\nifuzz\e`a`bu`a`bQ' | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

mark3:
	-${TITLE}
	printf '/butt\nmajmb/rude\nd2w`a`bu`a`bQ' | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

mark4:
	-${TITLE}
	printf '/butt\nmajmb/rude\ny2wPP`a`bu`a`bQy' | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# BUG: The last undo failed to adjmarks correctly.  Must adjustment
# marks above the `gap`, _not_ `here`.
mark5:
	-${TITLE}
	printf '/butt\nma/rude\nifuzz\e`au`aU`aQy' | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}


# `dd` now supported
del7:
	-${TITLE}
	cp test/short.txt a.txt
	printf '4wddW\nQy' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Delete from cursor to same column of next line.
del8:
	-${TITLE}
	cp test/short.txt a.txt
	printf '4wdjW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Delete whole line.
del9:
	-${TITLE}
	cp test/short.txt a.txt
	printf '4w|djW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Delete to pattern and then next.
del10:
	-${TITLE}
	cp test/short.txt a.txt
	printf 'wd/line\ndnW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Delete entire buffer.
del11:
	-${TITLE}
	cp test/short.txt a.txt
	printf 'dGW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Delete back to mark.
del12:
	-${TITLE}
	cp test/short.txt a.txt
	printf 'wma2jd`aW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Delete six words, ie. [count]d[count]motion
del13:
	-${TITLE}
	cp test/short.txt a.txt
	printf '4w1d7wW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Undo read file.
undo0:
	-${TITLE}
	printf 'uUQ' | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Undo insert.
undo1:
	-${TITLE}
	cp test/short.txt a.txt
	printf '3liWOOT!\euUW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Undo delete.
undo2:
	-${TITLE}
	cp test/short.txt a.txt
	printf '4wd3wuUW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Delete and undo entire buffer.
undo3:
	-${TITLE}
	printf 'dGuQ' | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Fix bug with paired undo operations.
undo4:
	-${TITLE}
	printf 'itest undo redo\ebbcwfoo \euuUUQy' | ${PROG} >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Paste empty scrap.
paste0:
	-${TITLE}
	printf 'PQ' | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Delete buffer and paste back.
paste1:
	-${TITLE}
	printf 'dGPQy' | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Delete word, paste 3 times.
paste2:
	-${TITLE}
	cp test/short.txt a.txt
	printf '4wdwj2w3PW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Paste at EOF.
paste3:
	-${TITLE}
	cp test/short.txt a.txt
	printf '4wdwuGPkW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Command not found.
bang0:
	-${TITLE}
	cp test/short.txt a.txt
	printf 'j!!WOOT\nQy' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Command fail.
bang1:
	-${TITLE}
	cp test/short.txt a.txt
	printf 'j!!false\nQy' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Command success.
bang2:
	-${TITLE}
	cp test/short.txt a.txt
	printf 'j!!true\nQy' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Command read-only.
bang3:
	-${TITLE}
	cp test/short.txt a.txt
	printf 'j!!ls -d test/*Curses\nW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Command save and delete region.
bang4:
	-${TITLE}
	cp test/short.txt a.txt
	printf 'j!2jcat >b.txt\nW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	test $$(wc -c <b.txt) -eq 43
	-${PASS}

# Command save region.
bang5:
	-${TITLE}
	cp test/short.txt a.txt
	printf 'j!2jtee b.txt\nW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	test $$(wc -c <b.txt) -eq 43
	-${PASS}

# Command format region fails.
bang6:
	-${TITLE}
	cp test/short.txt a.txt
	printf 'j!2jfmt -g68 -w40\nW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Command format region succeeds.
bang7:
	-${TITLE}
	cp test/short.txt a.txt
	printf 'j!2jfmt -g32 -w40\nW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

replace0:
	-${TITLE}
	cp test/short.txt a.txt
	printf '/butt/xyzzy\nW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

replace1:
	-${TITLE}
	cp test/short.txt a.txt
	printf '/butt/$0hole\nW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

replace2:
	-${TITLE}
	cp test/short.txt a.txt
	printf '/line/plugh!\nnnnW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# CB-1 /^/XYZZY/a loops forever.
replace3:
	-${TITLE}
	cp test/short.txt a.txt
	printf 'jjw/^/plugh!/a\nW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# CB-4 /$/XYZZY/a will crash.
replace4:
	-${TITLE}
	cp test/short.txt a.txt
	printf 'jjw/$/plugh!/a\nW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

write0:
	-${TITLE}
	printf 'iThe quick brown fox jumps over the lazy dog.\n\eWa.txt\nQ' | ${PROG} >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# This test does not work since line erase in redirected input is
# treated differently from a tty.  See NetBSD 10.0 lib/58151.
# NetBSD 10 kill character visual behaviour in noraw()/nocbreak()
# (line displine on) does appear correctly, though the string buffer
# appears correct.
write1:
	-${TITLE}
	printf 'iThe quick brown fox jumps over the lazy dog.\n\eWa.txt\nW\025b.txt\nQ' | ${PROG} >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Read file into an empty buffer.
read0:
	-${TITLE}
	printf 'Rtest/short.txtWa.txt\nQy' | ${PROG} >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Add text, then read a file into the middle, and save.
read1:
	-${TITLE}
	printf 'iThe quick brown fox jumps over the lazy dog.\n\e/fox\nRtest/short.txt\nW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Toggle select on/off without selecting anything.
select0:
	-${TITLE}
	printf '\\\\Q' | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Select some words right.
select1:
	-${TITLE}
	printf '/rude\n\\4w\\Q' | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Select some words left.
select2:
	-${TITLE}
	printf '/rude\n\\4b\\Q' | ${PROG} test/short.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Select some words right, yank, paste. Selction is extended.
select3:
	-${TITLE}
	cp test/short.txt a.txt
	printf '/rude\n\\4wy/but\nPW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Select some words left, delete, paste.
select4:
	-${TITLE}
	cp test/short.txt a.txt
	printf '/rude\n\\4bd/but\nPW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Select format selection.
select5:
	-${TITLE}
	cp test/short.txt a.txt
	printf 'j\\3j!fmt -g25 -w30\nW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Insert multibyte character.
mb0:
	-${TITLE}
	printf 'i🔥\n\eW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_TXT}
	${A_OUT_OR_SKIP}

# Move over 2 column wide multibyte character to newline, next line.
#
# There is a visual/redraw bug here with NetBSD Curses that messes
# with the % position and cursor position moving over the wide
# characters and newline; force a redraw fixes it.  NCurses is fine.
#
mb1:
	-${TITLE}
	cp ${DATA}/$@.txt a.txt
	printf 'llQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT_OR_SKIP}

# Move goto end of line containing multibyte character, then next line.
mb2:
	-${TITLE}
	cp ${DATA}/$@.txt a.txt
	printf '$$lQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT_OR_SKIP}

# Cursor back from eof.
mb3:
	-${TITLE}
	cp ${DATA}/$@.txt a.txt
	printf '2jhhhhhhhhQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT_OR_SKIP}

# Insert and erase multibyte character.
mb4:
	-${TITLE}
	printf 'i🔥\b\n\eW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_TXT}
	${A_OUT_OR_SKIP}

# Test display and movement over invalid multibyte sequence.
imb0:
	-${TITLE}
	printf '[\342\202]' >a.txt
	printf 'llhhQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Test display and movement over multibyte continuation.
imb1:
	-${TITLE}
	printf '[\202]' >a.txt
	printf 'lhQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	-${PASS}

# Test delete of invalid multibyte sequence.
imb2:
	-${TITLE}
	printf '[\342\202]' >a.txt
	printf 'xulXW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Test delete of multibyte continuation.
imb3:
	-${TITLE}
	printf '[\202]' >a.txt
	printf 'xulXW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Single line
cc0:
	-${TITLE}
	cp test/short.txt a.txt
	printf '4wccwhisky tango foxtrot\n\eW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Count lines `count cc`
cc1:
	-${TITLE}
	cp test/short.txt a.txt
	printf '4w2ccwhisky\ntango\nfoxtrot\n\eW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Single line
dd0:
	-${TITLE}
	cp test/short.txt a.txt
	printf '4wddW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Count lines `count dd`
dd1:
	-${TITLE}
	cp test/short.txt a.txt
	printf '4w2ddW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Count lines `d count d`
dd2:
	-${TITLE}
	cp test/short.txt a.txt
	printf '4wd2dW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Count lines `count0 d count1 d`
dd3:
	-${TITLE}
	cp test/short.txt a.txt
	printf '4w2d3dW\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Single line
indent0:
	-${TITLE}
	cp test/short.txt a.txt
	printf 'w>>W\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# BOL to BOL region
indent1:
	-${TITLE}
	cp test/short.txt a.txt
	printf '3>>W\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# MOL to MOL region
indent2:
	-${TITLE}
	cp test/short.txt a.txt
	printf 'w3>>W\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# BOL to BOL selection
indent3:
	-${TITLE}
	cp test/short.txt a.txt
	printf '\\2j>W\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# MOL to MOL selection
indent4:
	-${TITLE}
	cp test/short.txt a.txt
	printf 'w\\2j>W\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# BOL to MOL selection
indent5:
	-${TITLE}
	cp test/short.txt a.txt
	printf '\\2jww>W\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# MOL to BOL selection
indent6:
	-${TITLE}
	cp test/short.txt a.txt
	printf 'w\\2j^>W\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Single line
outdent0:
	-${TITLE}
	cp test/short.txt a.txt
	printf 'ji\t\e<<W\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}

# Count lines `count <<`
outdent1:
	-${TITLE}
	cp test/short.txt a.txt
	printf 'ji\t\ej^i\t\ek<<W\nQ' | ${PROG} a.txt >a.out
	${A_CORE}
	${A_OUT}
	${A_TXT}
	-${PASS}
