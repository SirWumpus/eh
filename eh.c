/*
 * Edit Here
 *
 * Copyright 2024, 2025 by Anthony C Howe.  All rights reserved.
 *
 * For TextPad suggest this Unicode font...
 * https://www.unifoundry.com/pub/unifont/unifont-16.0.03/font-builds/unifont-16.0.03.otf
 */

#ifndef IOCCC
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <regex.h>
#include <locale.h>
#include <iso646.h>
#include <curses.h>
#include <ctype.h>
#include <string.h>
#include <wctype.h>
#include <signal.h>
#include <sys/wait.h>
#include <uchar.h>
#else /* IOCCC */
#endif /* IOCCC */

#ifndef BUF
# define BUF		(64*1024)
#endif
#ifndef MODE
# define MODE		0600
#endif

#define CTRL_C		'\003'
#define CTRL_R		'\022'
#define CTRL_V		'\026'
#define CTRL_Z		'\032'
#define ESC		'\033'

#define CHANGED		'*'
#define NOCHANGE	' '
#define MARKS		27
#define MAX_COLS	999
#define TABWIDTH	8
#define TABSTOP(col)	(TABWIDTH - ((col) bitand  (TABWIDTH-1)))

#define TOP_LINE	1
#define ROWS		(LINES-TOP_LINE)

#define ALL_CMDS	99

#ifndef IOCCC
#define MATCHES		10
#define MOTION_CMDS	22

static char chg = NOCHANGE;
static int cur_row, cur_col, count, ere_dollar_only;
static char *filename, *scrap, *replace;
static char *buf, *gap, *egap, *ebuf;
static const char ins[] = "INS", cmd[] = "   ", *mode = cmd;
static off_t here, page, epage, match_length, scrap_length, marks[MARKS], marker = -1;
static regex_t ere;
#else /* IOCCC */
#define MATCHES		1
#define MOTION_CMDS	14

static int cur_row, cur_col, count, ere_dollar_only;
static char *filename, *scrap;
static char buf[BUF], *gap = buf, *egap, *ebuf, *ugap, *uegap;
static off_t here, uhere, page, epage, match_length, scrap_length, marker = -1;
static regex_t ere;
#endif /* IOCCC */

void
getcmd(int);

/*
 * The original prog.c for the IOCCC conformed to the 1536 bytes of
 * source size rule.  Not yet sure if this version will still conform,
 * though it should for more current versions of the size rule (4096,
 * 2503).
 *
 *	0 <= off_t <= file size	eg. here (the cursor)
 *
 *	buf <= char * <= ebuf	eg. gap (start of the hole)
 */

/*
 *	The following assertions must be maintained.
 *
 *	o  buf <= gap <= egap <= ebuf
 *		If gap == egap then the buffer is full.
 *
 *	o  point = ptr(here) and point < gap or egap <= point
 *
 *	o  page <= here < epage
 *
 *	o  0 <= here <= pos(ebuf) <= BUF
 *
 *
 *	Memory representation of the file:
 *
 *		low	buf  -->+----------+
 *				|  front   |
 *				| of file  |
 *			gap  -->+----------+<-- character not in file
 *				|   hole   |
 *			egap -->+----------+<-- character in file
 *				|   back   |
 *				| of file  |
 *		high	ebuf -->+----------+<-- character not in file
 *
 *
 *	point & gap
 *
 *	The Point is the current cursor position while the Gap is the
 *	position where the last edit operation took place. The Gap is
 *	ment to be the cursor but to avoid shuffling characters while
 *	the cursor moves it is easier to just move a pointer and when
 *	something serious has to be done then you move the Gap to the
 *	Point.
 */

/*
 * Translate a buffer offset into a cursor offset,
 * where the gap size has to be factored out.
 */
off_t
pos(const char *s)
{
	assert(buf <= s and s <= ebuf);
	return s-buf - (s < egap ? 0 : egap-gap);
}

/*
 * Translate a cursor offset into a buffer offset,
 * where the gap size has to be factored in.
 */
char *
ptr(off_t cur)
{
	assert(0 <= cur and cur <= pos(ebuf));
	return buf+cur + (buf+cur < gap ? 0 : egap-gap);
}

/**
 * Alternative to mblen() that assumes ch is a the start byte of
 * UTF-8 multibyte character.  Returns the multibyte length; if
 * ch is a continuation byte, it returns 1, so a scanner will
 * move across a partial multibyte character to the start of the
 * next sequence.
 */
int
mblength(int ch)
{
//	return (1+(ch > 193)+(ch > 223)+(ch > 239)) * (ch < 128 or (193 < ch and ch < 245));
	assert(0 <= ch and ch < 245);
	return 1+(ch > 193)+(ch > 223)+(ch > 239);
}

off_t
nextch(off_t cur)
{
	/* Advance to next UTF-8 start byte.  Do not read past eof. */
	return cur + (cur < pos(ebuf) ? mblength(*ptr(cur)) : 0);
}

off_t
prevch(off_t cur)
{
	/* Find UTF-8 start byte skipping continuation bytes. */
	while (0 < cur and (192 bitand *ptr(--cur)) == 128) {
		;
	}
	assert(0 <= cur);
	return cur;
}

#ifndef IOCCC
#else /* IOCCC */
void
undo(void)
{
	char *p = ugap;
	ugap = gap;
	gap = p;
	p = uegap;
	uegap = egap;
	egap = p;
	/* epage gets assigned below, so we can use temporarily. */
	epage = uhere;
	uhere = here;
	here = epage;
	/* Force display() to reframe, ie. 1GdGu fails. */
	epage = here+1;
}
#endif /* IOCCC */

void
movegap(off_t cur)
{
	assert(0 <= cur and cur <= pos(ebuf));
#ifdef FAST_MOVE
	ssize_t len = gap - buf - cur;
	if (len < 0) {
		/* Shift data down, moving gap up to cursor. */
		(void) memcpy(gap, egap, -len);
		egap -= len;
		gap -= len;
	} else {
		/* Shift data up, moving gap down to cursor. */
		gap -= len;
		egap -= len;
		(void) memmove(egap, gap, len);
	}
#else /* FAST_MOVE */
	char *p = ptr(cur);
	while (p < gap) {
		*--egap = *--gap;
	}
	while (egap < p) {
		*gap++ = *egap++;
	}
#endif /* FAST_MOVE */
	assert(buf <= gap and gap <= egap and egap <= ebuf);
#ifndef IOCCC
#else /* IOCCC */
	ugap = gap;
	uegap = egap;
	uhere = here;
#endif /* IOCCC */
}

#ifndef IOCCC
void
growgap(size_t min)
{
	char *xbuf;
	assert(buf <= gap and gap <= egap and egap <= ebuf);
	/* The gap is used for field input and should be at least as
	 * large as the screen width.
	 */
	if ((size_t)(egap-gap) < min) {
		/* Remember current gap location. */
		off_t xhere = pos(egap);
		ptrdiff_t buflen = ebuf-buf;
		/* Append the new space to the end of the gap. */
		movegap(pos(ebuf));
		off_t gap_off = pos(gap);
		if (NULL == (xbuf = realloc(buf, buflen+BUF))) {
			/* Bugger.  Don't exit, allow user to write file. */
			(void) beep();
			return;
		}
		egap = ebuf = xbuf+buflen+BUF;
		gap = xbuf+gap_off;
		buf = xbuf;
		/* Restore gap's previous location. */
		movegap(xhere);
	}
}

typedef enum { UNDO_DEL, UNDO_INS, UNDO_DEL_A, UNDO_INS_B } UndoOp;

struct ubuf {
	struct ubuf *next;
	UndoOp op;
	bool paired;
	off_t off;
	size_t size;
	char buf[];
};

struct ubuf *undo_list, *redo_list;

void
adjmarks(off_t n)
{
	off_t p = pos(egap);
	for (int i = 0; 0 != n and i < MARKS; i++) {
		if (p < marks[i]) {
			marks[i] += n;
		}
	}
}

void
undo_free(struct ubuf *obj)
{
	struct ubuf *next;
	for ( ; obj not_eq NULL; obj = next) {
		next = obj->next;
		free(obj);
	}
}

void
undo_save(UndoOp op, off_t off, char *loc, size_t size)
{
	struct ubuf *obj;
	undo_free(redo_list);
	redo_list = NULL;
	/* Append new undo.  We cannot skip saving a zero length
	 * string, because of paired undo objects we need both.
	 */
	if ((obj = realloc(NULL, sizeof (*obj) + size)) not_eq NULL) {
		obj->op = op bitand 1;
		obj->paired = UNDO_INS < op;
		obj->off = off;
		obj->size = size;
		(void) memcpy(obj->buf, loc, size);
		obj->next = undo_list;
		undo_list = obj;
	}
}

void
undo_redo(UndoOp op, struct ubuf *obj)
{
	movegap(obj->off);
	if (op) {
		/* Insert. */
		growgap(obj->size);
		egap -= obj->size;
		(void) memcpy(egap, obj->buf, obj->size);
		adjmarks(obj->size);
	} else {
		/* Delete. */
		adjmarks(-obj->size);
		egap += obj->size;
	}
	here = pos(egap);
	epage = here+1;
}

void
undo_move(struct ubuf **from, struct ubuf **to)
{
	struct ubuf *tmp = *from;
	*from = (*from)->next;
	tmp->next = *to;
	*to = tmp;
}

void
undo(void)
{
	if (undo_list not_eq NULL) {
		undo_redo(not undo_list->op, undo_list);
		if (undo_list->next not_eq NULL and undo_list->paired) {
			undo_move(&undo_list, &redo_list);
			undo_redo(not undo_list->op, undo_list);
		}
		undo_move(&undo_list, &redo_list);
		if (undo_list == NULL) {
			chg = NOCHANGE;
		}
	}
}

void
redo(void)
{
	if (redo_list not_eq NULL) {
		undo_redo(redo_list->op, redo_list);
		if (redo_list->next not_eq NULL and redo_list->paired) {
			undo_move(&redo_list, &undo_list);
			undo_redo(redo_list->op, redo_list);
		}
		undo_move(&redo_list, &undo_list);
		if (redo_list == NULL) {
			chg = CHANGED;
		}
	}
}
#else /* IOCCC */
#define growgap(n)
#endif /* IOCCC */

int
charwidth(const char *s, int col)
{
	wint_t wc;
	(void) mbtowc(&wc, s, 4);
	return wc == '\t' ? TABSTOP(col) : (col = wcwidth(wc)) < 1 ? 1 : col;
}

/*
 * Return the physical BOL or BOF containing cur.
 */
off_t
bol(off_t cur)
{
	while (0 < cur and *ptr(--cur) not_eq '\n') {
		;
	}
	assert(-1 <= cur);
	return (cur+1) * (0 < cur);
}

/*
 * Return offset of column position, newline (EOL), or EOF; otherwise
 * if maxcol is way larger than the terminal width, eg. 999 (assumes
 * that you're not using an IMAX theatre screen for your terminal),
 * just EOL or EOF.
 */
off_t
col_or_eol(off_t cur, int col, int maxcol)
{
	char *p;
	while (col < maxcol and (p = ptr(cur)) < ebuf and *p not_eq '\n') {
		col += charwidth(p, col);
		cur = nextch(cur);
	}
	assert(0 <= cur and cur <= pos(ebuf));
	return cur;
}

/*
 * Return offset to start of logical line containing offset.
 */
off_t
row_start(off_t cur, off_t offset)
{
	char *p;
	int col = 0;
	off_t mark = cur;
	assert(/* 0 <= cur and cur <= offset and*/ offset <= pos(ebuf));
	while (cur < offset and (p = ptr(cur = nextch(cur))) < ebuf) {
		col += charwidth(p, col);
		if (COLS <= col) {
			mark = cur;
			col = 0;
		}
	}
	assert(0 <= mark and mark <= cur);
	return mark;
}

/*
 * Return the previous logical BOL or BOF.
 */
off_t
prevline(off_t cur)
{
	off_t s = bol(cur);		/* Current physical line. */
	off_t t = row_start(s, cur);	/* Current logical line. */
	if (s < t) {
		/* Within current physical line, find previous logical line. */
		return row_start(s, t-1);
	}
	/* Previous physical line, find last logical line. */
	return row_start(bol(s-1), s-1);
}

/*
 * Return the next logical EOL or EOF.
 */
off_t
nextline(off_t cur)
{
	return nextch(col_or_eol(cur, cur_col, COLS-1));
}

void
clr_to_eol(void)
{
	(void) printw("%*s", COLS-getcurx(stdscr), "");
}

void
display(void)
{
	char *p;
	int i, j;
	off_t from, to, eof = pos(ebuf);
	/* Find the top of the display page. */
	if (here < page) {
		/* Scroll up one logical line or goto physical line. */
		page = row_start(bol(here), here);
	} else if (epage <= here and here < nextline(epage)) {
		/* Scroll down one logical line. */
		page = nextline(page);
	} else if (epage <= here) {
		/* Find top of page from here.  Avoid an unnecessary
		 * screen redraw when the EOF marker is displayed (mid-
		 * screen) by remembering where the previous page frame
		 * started.
		 */
		epage = page;
		for (page = here, i = ROWS - (here == eof); 0 < --i and epage < page; ) {
			page = prevline(page);
		}
		/* When find the top line of the page over shoots the
		 * previous frame, restore the previous frame.  This
		 * avoid an undesired scroll back of one line.
		 */
		if (page < epage) {
			page = epage;
		}
	} /* Else still within page bounds, update cursor. */
	(void) erase();
	(void) standout();
#ifndef IOCCC
	(void) printw(
		"%s %ldB %ld%% %c %s", filename, eof,
		here * 100 / (eof+(eof <= 0)), chg, mode
	);
#else /* IOCCC */
	(void) printw("%s %ldB", filename, here);
#endif /* IOCCC */
	clr_to_eol();
	if (marker < 0) {
		from = to = marker;
	} else if (here < marker) {
		from = here;
		to = marker;
	} else {
		from = marker;
		to = here;
	}
	for (i = TOP_LINE, j = 0, epage = page; (void) standend(), i < LINES; ) {
		if (here == epage) {
			cur_row = i;
			cur_col = j;
		}
		if (ebuf <= (p = ptr(epage))) {
			break;
		}
#ifndef IOCCC
		bool is_ctrl = iscntrl(*p) and *p not_eq '\t' and *p not_eq '\n';
		if ((from <= epage and epage < to) or is_ctrl) {
			standout();
		}
		/* A multibyte character never stradles the gap,
		 * assumes the gap moves by character, not by byte.
		 * See also nextch() and prevch().
		 */
		int mbl = mblength(*p);
		if (is_ctrl) {
			/* Display control characters as a single byte
			 * highlighted upper case letter (instead of two
			 * byte ^X).
			 */
			(void) mvaddch(i, j, *p+'@');
		} else {
#ifdef PLACEHOLDER
			char32_t wc;
			(void) mbrtoc32(&wc, p, mbl, NULL);
			if (0 < wcwidth(wc) or iswspace(wc)) {
				/* Use addnstr() family that already handles UTF8
				 * instead of add_wch() to avoid all the complexity
				 * of using cchar_t.
				 */
				(void) mvaddnstr(i, j, p, mbl);
			} else {
				/* Place holder for non-printable. */
				(void) mvaddch(i, j, A_REVERSE|'~');
			}
#else
			/* Use addnstr() family that already handles UTF8
			 * instead of add_wch() to avoid all the complexity
			 * of using cchar_t.
			 */
			(void) mvaddnstr(i, j, p, mbl);
#endif
		}
		epage += mbl;
#else /* IOCCC */
		if (from <= epage and epage < to) {
			standout();
		}
		int mbl = mblength(*p);
		(void) mvaddnstr(i, j, p, mbl);
		epage += mbl;
#endif /* IOCCC */
		/* Handle tab expansion ourselves.  Historical
		 * Curses addch() would advance to the next
		 * tabstop (a multiple of 8, eg. 0, 8, 16, ...).
		 * See SUS Curses Issue 7 section 3.4.3.
		 */
		j += charwidth(p, j);
		if (*p == '\n' or COLS <= j) {
			j = 0;
			i++;
		}
	}
	assert(page <= here and here <= epage);
	if (i++ < LINES) {
		(void) standout();
		(void) mvaddstr(i, 0, "^D");
		/* A refresh() side effect is to standend(). */
	}
	(void) move(cur_row, cur_col);
	(void) refresh();
}

void
nil(void)
{
	/* Do nothing. */
}

void
redraw(void)
{
	(void) clear();
	count = 0;
}

void
left(void)
{
	here = prevch(here);
}
void
right(void)
{
	here = nextch(here);
}

/*
 * Move up one logical line.
 */
void
up(void)
{
	here = col_or_eol(prevline(here), 0, cur_col);
}

/*
 * Move down one logical line.
 */
void
down(void)
{
	here = col_or_eol(nextline(here), 0, cur_col);
}

#ifndef IOCCC
/*
 * Beginning of physical line.
 */
void
lnbegin(void)
{
	here = bol(here);
	cur_col = 0;
}

/*
 * End of physical line.
 */
void
lnend(void)
{
	here = col_or_eol(here, 0, MAX_COLS);
}

static const char brackets[] = "()[]{}<>";

void
pairs(void)
{
	int ch, level = 0;
	off_t mark = here;
	ptrdiff_t b = strchr(brackets, *ptr(mark))-brackets;
	if (b < 0) {
		return;
	}
	if (b bitand 1 == 1) {
		/* Find previous. */
		for (mark--; 0 < mark; mark--) {
			ch = *ptr(mark);
			if (ch == brackets[b]) {
				level++;
			} else if (ch == brackets[b-1]) {
				if (level == 0) {
					here = mark;
					break;
				}
				level--;
			}
		}
	} else {
		/* Find next. */
		off_t eof = pos(ebuf);
		for (mark++; mark < eof; mark++) {
			ch = *ptr(mark);
			if (ch == brackets[b]) {
				level++;
			} else if (ch == brackets[b+1]) {
				if (level == 0) {
					here = mark;
					break;
				}
				level--;
			}
		}
	}
}
#else /* IOCCC */
#endif /* IOCCC */

/*
 * Goto column of physical line.
 */
void
column(void)
{
	here = col_or_eol(bol(here), 0, count-1);
	count = 0;
}

void
pgtop(void)
{
#ifndef IOCCC
	marks[0] = here;
#else /* IOCCC */
#endif /* IOCCC */
	here = page;
}

void
pgbottom(void)
{
#ifndef IOCCC
	marks[0] = here;
#else /* IOCCC */
#endif /* IOCCC */
	here = row_start(bol(epage-1), epage-1);
}

/*
 * Page down logical lines.
 */
void
pgdown(void)
{
	int i;
	/* Maintain cursor row within the next page frame. */
	for (here = epage, i = TOP_LINE; i < cur_row; i++) {
		here = nextline(here);
	}
	/* Maintain cursor column on logical line, if possible. */
	here = col_or_eol(here, 0, cur_col);
	/* Page down advances to the next page or remains as-is because
	 * of a short page at EOF, ie. using short.txt J should not redraw
	 * the screen, just move the cursor to EOF.
	 */
	page = here < pos(ebuf) ? epage : page;
	/* Maintain cursor row within the page by adjusting the frame. */
	for (epage = here; i < LINES; i++) {
		epage = nextline(epage);
	}
}

/*
 * Page up logical lines.
 */
void
pgup(void)
{
	/* Page up N logical lines. */
	for (int i = ROWS; 0 < i--; ) {
		here = prevline(here);
	}
	/* Maintain cursor row within the page by adjusting the frame. */
	for (page = here; TOP_LINE < cur_row--; ) {
		page = prevline(page);
	}
	/* Maintain cursor column on logical line. */
	here = col_or_eol(here, 0, cur_col);
}

int
isword(int ch)
{
	return isalnum(ch) or ch == '_';
}

void
wleft(void)
{
	while (0 < here and isspace(*ptr(here-1))) {
		--here;
	}
	if (0 < here and isword(*ptr(here-1))) {
		/* Move backwards to start of previous word. */
		while (0 < here and isword(*ptr(here-1))) {
			--here;
		}
	} else {
		/* Move backwards to end of previous word. */
		while (0 < here and ispunct(*ptr(here-1))) {
			--here;
		}
	}
}

/*
 * See https://pubs.opengroup.org/onlinepubs/9799919799/utilities/vi.html
 * definition of "word" (not "bigword").
 */
void
wright(void)
{
	off_t eof = pos(ebuf);
	if (here < eof and isword(*ptr(here))) {
		/* Move forwards to end of current word. */
		while (here < eof and isword(*ptr(here))) {
			++here;
		}
		/* Skip blanks, but not newlines. */
		while (here < eof and isblank(*ptr(here))) {
			++here;
		}
	} else if (here < eof and ispunct(*ptr(here))) {
		/* Move forwards to start of next word. */
		while (here < eof and ispunct(*ptr(here))) {
			++here;
		}
		/* Skip blanks, but not newlines. */
		while (here < eof and isblank(*ptr(here))) {
			++here;
		}
	} else {
		/* Skip over whitespace. */
		while (here < eof and isspace(*ptr(here))) {
			++here;
		}
	}
}

void
lngoto(void)
{
#ifndef IOCCC
	marks[0] = here;
#else /* IOCCC */
#endif /* IOCCC */
	/* Count physcical lines, just as ed, grep, or wc would. */
	off_t eof = pos(ebuf);
	for (here = eof * (count == 0); here < eof and 1 < count; count--) {
		/* Next physical line. */
		here = col_or_eol(here, 0, MAX_COLS);
		here += here < eof;
	}
	/* Set page to eof if beyond page end to force display() to
	 * reframe the page with target line at top.
	 */
	page = eof;
}

#ifndef IOCCC
int
getsigch(void)
{
	int ch;
	while ((ch = getch()) == CTRL_Z) {
		(void) raise(SIGTSTP);
	}
	return ch;
}
#else /* IOCCC */
#define getsigch	getch
#endif /* IOCCC */

void
insert(void)
{
	int ch, mbl;
	off_t eof = pos(ebuf);
#ifndef IOCCC
	mode = ins;
	display();
#else /* IOCCC */
#endif /* IOCCC */
	movegap(here);
	while ((ch = getsigch()) not_eq CTRL_C and ch not_eq ESC) {
		mbl = mblength(ch);
		if (ch == '\b') {
			/* Move to previous (multibyte) character. */
			while (eof < pos(ebuf) and (192 bitand *--gap) == 128) {
				;
			}
		} else if (gap+mbl < egap) {
#ifndef IOCCC
			/* Using cbreak() then ^V is handled
			 * so ESC ^[ is inserted with ^V^V^[.
			 * With raw() we handle ^V so ESC ^[
			 * is inserted with ^V^[ and we'd have
			 * to handle ^C ourselves.
			 */
			if (ch == CTRL_V) {
				(void) nonl();	/* CR as-is */
				ch = getch();
				(void) nl();	/* CR -> LF */
			}
#else /* IOCCC */
#endif /* IOCCC */
			/* Read the remainder of a multibyte
			 * character BEFORE updating the display.
			 */
			growgap(mbl);
			do {
				*gap++ = ch;
				epage++;
			} while (0 < --mbl and (ch = getch()));
		}
		here = pos(egap);
#ifndef IOCCC
		chg = CHANGED;
#else /* IOCCC */
#endif /* IOCCC */
		display();
	}
#ifndef IOCCC
	mode = cmd;
	off_t len = pos(ebuf)-eof;
	undo_save(UNDO_INS, here-len, gap-len, len);
	adjmarks(len);
#else /* IOCCC */
#endif /* IOCCC */
	/* Not repeatable yet. */
	count = 0;
}

/**
 * Yank current selection or by motion.
 */
void
yanky(void)
{
	off_t mark = marker;
	if (marker < 0) {
		mark = here;
		getcmd(MOTION_CMDS);
	}
	if (mark < here) {
		mark xor_eq here;
		here xor_eq mark;
		mark xor_eq here;
	}
	/* SUS 2018 vi(1) `y` yank sets the cursor on the last column
	 * of the first character of the yanked region; results in yank
	 * forward leaving the cursor as-is, while yank back moves the
	 * cursor back.
	 *
	 * The alternatives are to either always update cursor according
	 * to the motion (better reflects GUI editors, select then copy
	 * or cut, and better visual feedback) or not update cursor at
	 * all (functional but lacks visual feedback ie. did it work).
	 */
	free(scrap);
	movegap(here);
	scrap = strndup(egap, scrap_length = mark-here);
	marker = -1;
}

/**
 * Delete current selection or by motion.
 */
void
deld(void)
{
	yanky();
#ifndef IOCCC
	chg = CHANGED;
	undo_save(UNDO_DEL, here, scrap, scrap_length);
#else /* IOCCC */
#endif /* IOCCC */
	egap += scrap_length;
	here = pos(egap);
	adjmarks(-scrap_length);
}

#ifndef IOCCC
void
ungetstr(const char *str)
{
	ssize_t n = strlen(str);
	assert(n < COLS and COLS <= egap-gap);
	while (0 < n and 0 == ungetch(str[--n])) {
		;
	}
}

/**
 * Yank current line.
 */
void
yankY(void)
{
	marker = -1;
	ungetstr("^yj");
}

/**
 * Delete character right (under) the cursor; same as `dl`.
 */
void
delx(void)
{
	if (marker < 0) {
		(void) ungetch('l');
	}
	deld();
}

/**
 * Delete character left the cursor; same as `dh`.
 */
void
delX(void)
{
	if (marker < 0) {
		(void) ungetch('h');
	}
	deld();
}

/**
 * Delete from cursor to end of line, same as `d$`.
 */
void
delD(void)
{
	if (marker < 0) {
		(void) ungetch('$');
	}
	deld();
}

/**
 * Change current selection or by motion.
 */
void
chgc(void)
{
	deld();
	/* Convert delete to paired delete-insert. */
	undo_list->paired = true;
	insert();
	/* Convert insert to paired delete-insert. */
	undo_list->paired = true;
}

/**
 * Change current selection or from cursor to end of line, same as `c$`.
 */
void
chgC(void)
{
	if (marker < 0) {
		(void) ungetch('$');
	}
	chgc();
}

void
insertI(void)
{
	ungetstr("^i");
}

/**
 * Insert right of the cursor.
 */
void
append(void)
{
	ungetstr("li");
}

/**
 * Insert at the end of the line, same as `$i`.
 */
void
appendA(void)
{
	ungetstr("$i");
}

/**
 * Open a new empty line below the current cursor line.
 */
void
openo(void)
{
	ungetstr("$a\n\033hi");
}

/**
 * Open a new empty line above the current cursor line.
 */
void
openO(void)
{
	ungetstr("^i\n\033ki");
}

/**
 * Find end of word.
 */
void
wend(void)
{
	off_t eof = pos(ebuf);
	while (here < eof and isspace(*ptr(++here))) {
		;
	}
	if (here < eof and isword(*ptr(here))) {
		/* Move forwards to end of current word. */
		while (here < eof and isword(*ptr(here))) {
			here++;
		}
	} else {
		/* Move forwards to start of next word. */
		while (here < eof and ispunct(*ptr(here))) {
			here++;
		}
	}
	here--;
}
#else /* IOCCC */
/**
 * Delete character right (under) the cursor; same as `dl`.
 */
void
delx(void)
{
	/* Do not delete selection, use `d` instead. */
	(void) ungetch('l');
	deld();
}

#endif /* IOCCC */

void
pasteP(void)
{
	if (0 < scrap_length) {
#ifndef IOCCC
		movegap(here);
		growgap(COLS+(count+(count == 0))*scrap_length);
		undo_save(UNDO_INS, here, scrap, scrap_length);
		adjmarks(scrap_length);
		chg = CHANGED;
#else /* IOCCC */
		movegap(here);
#endif /* IOCCC */
		(void) memcpy(gap, scrap, scrap_length);
		gap += scrap_length;
		/* SUS 2018 vi(1) `P` paste-before unnamed buffer leaves
		 * the cursor on the last column of the last character.
		 * Instead keep the cursor on the same character before
		 * the paste, which allows for clean repeated pasting,
		 * eg. `PP` or `2P` XX_YY => XXpastepaste_YY, where '_'
		 * is the cursor.
		 */
		here = pos(egap);
		/* Force display() to reframe, ie. 1GdGP fails. */
		epage = here+1;
	}
}

#ifndef IOCCC
void
pastep(void)
{
	/* SUS 2018 vi(1) `p` paste-after unnamed buffer leaves the
	 * cursor on the last column of the last character.  Allows
	 * for common transpose combo `xp`, eg. te_h => th_e.  Also
	 * `pp` or `2p` eg. 1_23 => 12pastepaste_3, where '_' is the
	 * cursor.
	 */
	ungetstr("lPh");
}

void
setmark(void)
{
	/* ASCII characters ` a..z are the allowed marks. */
	int i = getsigch() - '`';
	if (0 <= i and i < MARKS) {
		marks[i] = here;
	}
}

void
gomark(void)
{
	/* ASCII characters ` a..z are the allowed marks. */
	int i = getsigch() - '`';
	if (0 <= i and i < MARKS) {
		off_t j = marks[0];
		marks[0] = here;
		here = 0 < i ? marks[i] : j;
	}
	count = 0;
}

void
lnmark(void)
{
	int ch = getsigch();
	if (ch == '\'') {
		ch = '`';
	}
	(void) ungetch(ch);
	gomark();
	lnbegin();
}

void
prompt(int ch, const char *str)
{
	(void) echo();
	(void) noraw();
	(void) standend();
	(void) attron(A_UNDERLINE);
	(void) mvaddch(0, 0, ch);
	clr_to_eol();
	/* Limit the input to a phrase no wider than COLS, not lines. */
	if (str == NULL or strchr(str, '\n') != NULL) {
		str = "";
	}
	/* Prime the input with initial input. */
	ungetstr(str);
	/* NetBSD 9.3 erase ^H works fine, but not the kill ^U character. */
	(void) mvgetnstr(0, 1, gap, COLS);
	(void) attroff(A_UNDERLINE);
	(void) standout();
	(void) noecho();
	(void) raw();
}

int
filewrite(const char *fn)
{
	int fd;
	movegap(0);
	ssize_t n = 0;
	if (0 < (fd = creat(fn, MODE))) {
		n = write(fd, egap, ebuf-egap);
		(void) close(fd);
	}
	return fd < 0 or n < 0;
}

void
writefile(void)
{
	prompt('>', filename);
	if (*gap not_eq '\0') {
		free(filename);
		filename = strdup(gap);
		if (not filewrite(filename)) {
			chg = NOCHANGE;
		}
	} else {
		(void) beep();
	}
	count = 0;
}

int
fileread(const char *fn)
{
	int fd;
	ssize_t n = 0;
	if (0 < (fd = open(fn, 0))) {
		movegap(here);
		while (0 < (n = read(fd, gap, egap-gap))) {
			gap += n;
			growgap(BUF/2);
		}
		(void) close(fd);
	}
	return (int) n;
}

void
readfile(void)
{
	prompt('<', "");
	off_t eof = pos(ebuf);
	if (fileread(gap)) {
		/* ed(1) ? */
		(void) beep();
	} else {
		off_t len = pos(ebuf)-eof;
		undo_save(UNDO_INS, here, gap-len, len);
		here = pos(egap);
		epage = here+1;
		chg = CHANGED;
		adjmarks(len);
	}
	count = 0;
}

/*
 * [countA]![countB]motion command
 *
 * Example format one or more lines of text:
 *
 *	^				Start of line.
 *	!/^$<newline> fmt -w68		Reformat here to next empty line.
 *
 * Can also save a text region from `a to here, eg. !`a tee save.txt
 * If motion is `!` then read-only from command, eg. !! ls
 */
void
bang(void)
{
	ssize_t n;
	pid_t child;
	int child_in[2], child_out[2], ex = 74;
	deld();
	prompt('!', "");
	if (0 == pipe(child_in)) {
		if (0 == pipe(child_out)) {
			if ((child = fork()) >= 0) {
				if (child == 0) {
					/* Redirect standard I/O for the child. */
					if (STDIN_FILENO == dup2(child_in[0], STDIN_FILENO)
					and STDOUT_FILENO == dup2(child_out[1], STDOUT_FILENO)
					and STDERR_FILENO == dup2(child_out[1], STDERR_FILENO)) {
						const char *sh, *shell = getenv("SHELL");
						if (NULL == (sh = strrchr(shell, '/'))) {
							sh = shell;
						} else {
							sh++;
						}
						/* Close duplicates and unused. */
						(void) close(child_out[0]);
						(void) close(child_out[1]);
						(void) close(child_in[0]);
						(void) close(child_in[1]);
						/* Pass command(s) through a shell. */
						(void) execl(shell, sh, "-c", gap, NULL);
					}
					/* Use instead of exit() to avoid flushing standard I/O. */
					_exit(127);
				}
				/* Finally write text region to filter and read result. */
				if (scrap not_eq NULL and (scrap_length == 0 or (n = write(child_in[1], scrap, scrap_length)) == scrap_length)) {
					/* Signal EOF write to child. */
					(void) close(child_in[1]);
					/* Wait for the child _before_ reading input. */
					(void) waitpid(child, &ex, 0);
					ex = WIFEXITED(ex) ? WEXITSTATUS(ex) : 127;
					/* Avoid blocking on read() and allow for long or no output. */
					(void) fcntl(child_out[0], F_SETFL, O_NONBLOCK);
					off_t eof = pos(ebuf);
					while (0 < (n = read(child_out[0], gap, egap-gap))) {
						gap += n;
						chg = CHANGED;
						growgap(BUF/2);
					}
					/* Convert delete to paired delete-insert. */
					undo_list->paired = true;
					off_t len = pos(ebuf)-eof;
					undo_save(UNDO_INS_B, here, gap-len, len);
					adjmarks(len-undo_list->next->size);
					here = pos(egap);
					epage = here+1;
				}
			}
			(void) close(child_out[0]);
			(void) close(child_out[1]);
		}
		(void) close(child_in[0]);
		(void) close(child_in[1]);
	}
	if (ex not_eq 0) {
		(void) beep();
	}
}

void
altx(void)
{
	int i;
	char *p;
	char32_t wc;
	movegap(here);
	assert(gap < egap);
	/* Scan backwards at most 5 hex digits. */
	for (i = 0, *gap = '\0'; i < 6 and buf < gap and isxdigit(gap[-1]); gap--, i++) {
		;
	}
	wc = (char32_t) strtoul(gap, &p, 16);
	if (gap == p) {
		/* Erase UTF-8 character, insert code point. */
		gap -= here-prevch(here);
		i = mbrtoc32(&wc, gap, 4, NULL);
		if (0 < i) {
			gap += snprintf(gap, 9, "%06X", wc);
		}
	} else if (wc <= 0x10FFFF) {
		/* Erase code point, insert printable UTF-8 character. */
		gap += c32rtomb(gap, wc, NULL);
	} else {
		/* Restore state, nothing converted. */
		gap += i;
		beep();
	}
	here = pos(egap);
	epage = here+1;
}

int
cescape(int ch)
{
	for (const char *s = "a\ab\bf\fn\nr\rt\tv\ve\033?\177"; *s not_eq '\0'; s += 2) {
		if (ch == *s) {
			return s[1];
		}
	}
	return ch;
}

void
version(void)
{
	mode = BUILT " " COMMIT;
}

void
quit(void)
{
	mode = NULL;
}

void
flipcase(void)
{
	char *p = ptr(here);
	if (p < ebuf) {
		undo_save(UNDO_DEL_A, here, p, mblength(*p));
		/* Skip moving the gap and modify in place.
		 * Does NOT support (yet) non-ASCII alphabetics.
		 */
		*p = islower(*p) ? toupper(*p) : tolower(*p);
		undo_save(UNDO_INS_B, here, p, mblength(*p));
		chg = CHANGED;
		right();
	}
}

void
scrollup(off_t here, size_t n)
{
	for (page = here; 0 < n--; ) {
		page = prevline(page);
	}
}
#else /* IOCCC */
void
writefile(void)
{
	int i;
	movegap(0);
	(void) write(i = creat(filename, MODE), egap, ebuf-egap);
	(void) close(i);
}

void
quit(void)
{
	filename = NULL;
}
#endif /* IOCCC */

/**
 * In case we're sitting on a previous match, we need to search starting
 * from the end of that match.  Note some special cases:
 *
 * Pattern /^/
 * -----------
 *      BOFtext\n\nterminated\nEOF
 *         ^     ^ ^           ^
 *      BOFtext\n\nnot terminatedEOF
 *         ^     ^ ^
 *
 * Pattern /$/
 * -----------
 *	BOFtext\n\nterminated\nEOF
 *             ^ ^           ^ ^
 *	BOFtext\n\nnot terminatedEOF
 *             ^ ^               ^
 *
 * Pattern /.$/
 * ------------
 *	BOFtext\n\nterminated\nEOF
 *            ^             ^
 *	BOFtext\n\nnot terminatedEOF
 *            ^                 ^
 *
 * Pattern /^$/
 * ------------
 *      BOFtext\n\nterminated\n\nEOF
 *               ^             ^
 *      BOFtext\n\nnot terminatedEOF
 *	         ^               ^
 */
void
search_next(void)
{
	regmatch_t matches[MATCHES];
	/* Move the gap out of the way in case it sits in the middle
	 * of a potential match and NUL terminate the buffer.
	 */
	assert(gap < egap);
	movegap(pos(ebuf));
#ifndef IOCCC
	marks[0] = here;
#else /* IOCCC */
#endif /* IOCCC */
	*gap = '\0';
	/* REG_NOTBOL allows /^/ to advance to start of next line. */
	if (here+match_length < pos(ebuf) and 0 == regexec(&ere, ptr(here+match_length), MATCHES, matches, REG_NOTBOL)) {
		here += match_length + matches[0].rm_so;
		/* Position match in the centre of the screen.*/
		scrollup(here, LINES/2-TOP_LINE);
	}
	/* Wrap-around search. */
	else if (0 == regexec(&ere, buf, MATCHES, matches, 0)) {
		here = matches[0].rm_so;
		/* Position match in the centre of the screen.*/
		scrollup(here, LINES/2-TOP_LINE);
	}
	/* No match after wrap-around. */
	else {
		match_length = 0;
#ifndef IOCCC
		free(replace);
		replace = NULL;
#else /* IOCCC */
#endif /* IOCCC */
		return;
	}
	match_length = matches[0].rm_eo - matches[0].rm_so + ere_dollar_only;
#ifndef IOCCC
	if (NULL not_eq replace) {
		movegap(here);
		char *xgap = gap;
		for (const char *s = replace; *s not_eq '\0'; s++) {
			growgap(COLS);
			if (*s == '$' and isdigit(s[1])) {
				/* Subexpression $0..$9 */
				int i = *++s-'0';
				off_t n = matches[i].rm_eo - matches[i].rm_so;
				(void) memcpy(gap, egap + (matches[i].rm_so - matches[0].rm_so), n);
				gap += n;
				continue;
			} else if (*s == '\\') {
				*gap++ = cescape(*++s);
				continue;
			} else if (*s == '/') {
				/* End replacement string. */
				if (*++s == 'a') {
					/* a = replace all (do it again) */
					(void) ungetch('n');
				}
				break;
			}
			*gap++ = *s;
		}
		/* Delete the match. */
		undo_save(UNDO_DEL_A, here, egap, match_length);
		egap += match_length;
		/* Replacement string length. */
		match_length = gap-xgap;
		undo_save(UNDO_INS_B, here, xgap, match_length);
		adjmarks(match_length-undo_list->next->size);
		/* Advance the next serach by one, not the match length.
		 * Consider initial match, undo, then match next where
		 * we want to ignore the last match/undo and continue.
		 * See regexec(&ere, ptr(here+match_length)... above.
		 */
		match_length = 1;
	}
#else /* IOCCC */
#endif /* IOCCC */
}

/**
 * Search and replace.
 *
 *	/regex			find only
 *	/regex/			find only
 *	/regex//		replace with empty string
 *	/regex/pattern		replace with pattern
 *	/regex/pattern/		replace with pattern
 *	/regex/pattern/flags 	replace with pattern subject to flags
 *
 * Current flags:
 *
 *	a			replace all, no prompt
 */
void
search(void)
{
#ifndef IOCCC
	char *s, *t;
	prompt('/', "");
	free(replace);
	replace = NULL;
	/* Find end of pattern. */
	for (s = t = gap; *t not_eq '\0'; s++, t++) {
		/* Check for C escape, but ignore ERE meta. */
		if (*t == '\\' and (isalpha(t[1]) or t[1] == '/')) {
			/* Escape next character. */
			*s = cescape(*++t);
		}
		/* Is end of pattern, start of replacement. */
		else if (*t == '/') {
			*t++ = '\0';
			/* Is there a pattern or empty string to follow? */
			if (*t not_eq '\0') {
				replace = strdup(t);
			}
			break;
		}
		/* Copy/shift string. */
		else {
			*s = *t;
		}
	}
	*s = '\0';
#else /* IOCCC */
	(void) echo();
	(void) standout();
	(void) mvaddch(0, 0, '/');
	clr_to_eol();
	assert(COLS <= egap - gap);
	/* NetBSD 9.3 erase ^H works fine, but not the kill ^U character. */
	(void) mvgetnstr(0, 1, gap, egap-gap);
#endif /* IOCCC */
	regfree(&ere);
	if (regcomp(&ere, gap, REG_EXTENDED bitor REG_NEWLINE) not_eq 0) {
		/* Something about the pattern is fubar. */
		(void) beep();
	} else {
		/* Kludge to handle repeated /$/ matching. */
		ere_dollar_only = gap[0] == '$' and '\0' == gap[1];
		match_length = 0;
		search_next();
	}
	count = 0;
}

void
anchor(void)
{
	marker = marker < 0 ? here : -1;
}

#ifndef IOCCC
/* NOTE A, C, and D are questionable as "good parts", more like muscle
 * memory concessions, since they are composites. X and x are supported
 * since they are so frequently used, while dh and dl are functional,
 * they're less common.
 */

/*                         |--------MOTION_CMDS-------|-------edit--------|------misc-----it dif---| */
static const char key[] = "hjklbewHJKL^$|G/n`'%\006\002~iIaAxXyYdDcCoOPpuU!\030\\mRWQ\003V\022";

static void (*func[])(void) = {
	/* Motion */
	left, down, up, right, wleft, wend, wright,
	pgtop, pgdown, pgup, pgbottom,
	lnbegin, lnend, column, lngoto,
	search, search_next, gomark, lnmark,
	pairs, pgdown, pgup,
	/* Modify */
	flipcase, insert, insertI, append, appendA, delx, delX,
	yanky, yankY, deld, delD, chgc, chgC, openo, openO,
	pasteP, pastep, undo, redo, bang, altx,
	/* Other */
	anchor, setmark, readfile, writefile, quit, quit,
	version, redraw, nil
};
#else /* IOCCC */
/*                        |-MOTION_CMDS-|-edit-|--misc-| */
static const char key[] = "hjklbwHJKL|G/nixydPu\\WQ\003";

static void (*func[])(void) = {
	/* Motion */
	left, down, up, right, wleft, wright,
	pgtop, pgdown, pgup, pgbottom,
	column, lngoto,
	search, search_next,
	/* Modify */
	insert, delx,
	yanky, deld, pasteP, undo,
	/* Other */
	anchor, writefile, quit, quit,
	redraw
};
#endif /* IOCCC */

void
getcmd(int m)
{
	int j = count, ch;
	for (count = 0; isdigit(ch = getsigch()); ) {
		count = count * 10 + ch - '0';
	}
	/* 2dw = d2w and 2d3w = d6w */
	count = j not_eq 0 and count not_eq 0 ? j*count : count not_eq 0 ? count : j;
	for (j = 0; key[j] not_eq '\0' and ch not_eq key[j]; j++) {
		;
	}
	if (j < m) {
		/* Count always defaults to 1. */
		do (*func[j])(); while (1 < count--);
	}
	count = 0;
}

#ifndef IOCCC
void
cleanup(void)
{
	/* Most of these are to satisfy Valgrind or saniisers.  The OS
	 * reclaims memory when the program exits, making the need to
	 * free() theoritcally unnecessary.
	 */
	(void) delwin(stdscr);
	(void) endwin();
	undo_free(undo_list);
	undo_free(redo_list);
	free(filename);
	regfree(&ere);
	free(replace);
	free(scrap);
	free(buf);
}
#else /* IOCCC */
#endif /* IOCCC */

int
main(int argc, char **argv)
{
	(void) setlocale(LC_ALL, "");
	if (NULL == initscr()) {
		/* Try TERM=ansi-mini which works. */
		return 1;
	}
	(void) raw();
#ifndef IOCCC
	(void) noecho();
	(void) atexit(cleanup);
	/* Switching between cbreak() and raw() impacts terminal output
	 * which can alter the expected test output files.
	 */
	growgap(BUF);
	filename = strdup(argv[1] == NULL ? "" : *++argv);
	if (fileread(filename)) {
		/* Good grief Charlie Brown! */
		return 2;
	}
	/* Force display() to frame the initial screen. */
	epage = 1;
	while (mode not_eq NULL) {
		display();
		getcmd(ALL_CMDS);
	}
#else /* IOCCC */
	egap = ebuf = buf + BUF;
	if (0 < (argc = open(filename = *++argv, 0))) {
		gap += read(argc, buf, ebuf-buf);
		(void) close(argc);
		if (gap < buf or ebuf <= gap) {
			/* Good grief Charlie Brown! */
			return 2;
		}
	}
	/* Force display() to frame the initial screen. */
	epage = 1;
	while (filename not_eq NULL) {
		(void) noecho();
		display();
		getcmd(ALL_CMDS);
	}
	(void) endwin();
#endif /* IOCCC */
	return 0;
}
