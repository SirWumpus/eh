# NetBSD (POSIX?)
# 	BOW = [[:<:]]
# 	EOW = [[:>:]]
#
# Gnu BRE
#	BOW = \<
#	EOW = \>
#
# Portable ERE (requires sed -E option)
#	BOW = (^|[^[:alnum:]_])
#	EOW = ([^[:alnum:]_]|$)

#
#  Keywords & Phrases
#
/^#ifndef NDEBUG/,/#endif.*NDEBUG/d
/(^|[^[:alnum:]_])error\(/d
/(^|[^[:alnum:]_])EPRINTF/d
/(^|[^[:alnum:]_])INFO/d
/(^|[^[:alnum:]_])INFO/d
/(^|[^[:alnum:]_])DEBUG/d
/(^|[^[:alnum:]_])DUMP/d

/^#include.*limits/d
/^#pragma/d

#/\(/s/(\([^[:blank:]][^[:blank:]]*)[[:blank:]]/\1/g
s/, /,/g
#s/ \(/(/g
#s/\) \{/){/

/^#ifdef.*FAST/,/^#else/d
/^#endif.*FAST/d

/^#if.*BUF/,/^#endif/d

/^#if.*MODE/,/^#endif/d
#s/MODE/0600/

/^#if.*RULER/,/^#endif/d

/^#ifdef.*SCREEN_COLUMN/,/^#endif/d

/#define CHANGED/d
s/CHANGED/'*'/

/#define NOCHANGE/d
s/NOCHANGE/' '/

/#define TOP_LINE/d
#/^#if.*TOP_LINE/,/^#endif/d
#s/TOP_LINE/0/
s/TOP_LINE/1/

/^#define ROWS/d
#s/ROWS/LINES/g
s/ROWS/LINES-1/g

/#define ALT/d
/^#ifdef.*ALT/,/^#else/d
/^#endif.*ALT/d

#/standout()/d
#/standend()/d

# Assertions
/^#include.*assert/d
/assert\(/d

# Comment lines
/^[[:blank:]]*\/\//d
/^[[:blank:]]*\/\*.*\*\//d
#s/^[[:blank:]]*\/\*.*\*\///

# Trailing comment.
s/[[:blank:]]*\/\/[[:blank:]]*$//

# Comment blocks
/^[[:blank:]]*\/\*/,/^[[:blank:]]*\*\//d

# Trailing comment block.
s/[[:blank:]]+\/\*.*\*\/[[:blank:]]*$//

# Inline comment.
s,/\*[^*]+\*/,,g

#
# Replace macros.
#

/^#define TAB/d
#s/TABSTOP(([^)]*))/(8-(\1\&7))/g
s/TABSTOP(([^)]*))/(8-(\1 bitand 7))/g
#s/TABWIDTH/8/g
#s/TABSTOP/TS/g

/^#ifndef.*MAX_COLS/,/^#endif$/d
/^#define MAX_COLS/d
s/MAX_COLS/999/g

/^#define MARKS/d
s/MARKS/27/g

/^#define NOCHANGE/d
s/NOCHANGE/' '/g

/^#define CHANGED/d
s/CHANGED/'*'/g

/^#define MATCHES/d
s/MATCHES/10/g

/^#define .*_CMDS/d
s/ALL_CMDS/99/g

s/STDIN_FILENO/0/g
s/STDOUT_FILENO/1/g
s/STDERR_FILENO/2/g

/#define CTRL_/d
/#define ESC/d
s/CTRL_C/3/
s/CTRL_V/22/
s/CTRL_Z/26/
s/ESC/27/
s/'\\b'/8/

/^#define SET_PREVIOUS_MARK/d

#
#  Types
#

s/const //g
s/static //g
s/ssize_t /long /g
s/\(void\) ([[:alpha:]])/\1/
s/^([_[:alpha:]][_[:alnum:]]*)\(void\)(;)*$/\1()\2/

# Since sizeof (int) <= sizeof (off_t) using a smaller type like
# `int` actually results in binary size of prog-alt < prog (just).
# Otherwise long, off_t, and ptrdiff_t have prog < prog-alt
#
s/pid_t/int/g
s/off_t */long /g
s/ptrdiff_t */long /g
#s/off_t */long /g
#s/ptrdiff_t */long /g

#
# Constant expansion
#

s/NULL/0/g
s/'\\0'/0/g

#
#  Functions & Types
#	U+1f47e=👾 💣 🛸 🔥 🎃
#	U+0194=Ɣ U+03A1=Ρ U+03B3=γ U+03C6=φ U+0B67=୧ U+039a=Κ U+03BA=κ
#

s/prevch([^[:alnum:]_]|$)/D\1/g
s/nextch([^[:alnum:]_]|$)/E\1/g
s/(^|[^[:alnum:]_])adjmarks([^[:alnum:]_]|$)/\1j\2/g
s/(^|[^[:alnum:]_])ptr([^[:alnum:]_]|$)/\1Z\2/g
s/(^|[^[:alnum:]_])pos([^[:alnum:]_]|$)/\1P\2/g
s/(^|[^[:alnum:]_])growgap([^[:alnum:]_]|$)/\1Õ\2/g
s/(^|[^[:alnum:]_])movegap([^[:alnum:]_]|$)/\1V\2/g
s/(^|[^[:alnum:]_])prevline([^[:alnum:]_]|$)/\1M\2/g
s/(^|[^[:alnum:]_])nextline([^[:alnum:]_]|$)/\1N\2/g
s/(^|[^[:alnum:]_])bol([^[:alnum:]_]|$)/\1O\2/g
s/(^|[^[:alnum:]_])col_or_eol([^[:alnum:]_]|$)/\1A\2/g
s/(^|[^[:alnum:]_])row_start([^[:alnum:]_]|$)/\1_\2/g
s/(^|[^[:alnum:]_])getcmd([^[:alnum:]_]|$)/\1T\2/g
s/(^|[^[:alnum:]_])wleft([^[:alnum:]_]|$)/\1B\2/g
s/(^|[^[:alnum:]_])wright([^[:alnum:]_]|$)/\1W\2/g
s/(^|[^[:alnum:]_])pgdown([^[:alnum:]_]|$)/\1É\2/g
s/(^|[^[:alnum:]_])pgup([^[:alnum:]_]|$)/\1È\2/g
s/(^|[^[:alnum:]_])pgtop([^[:alnum:]_]|$)/\1Ê\2/g
s/(^|[^[:alnum:]_])pgbottom([^[:alnum:]_]|$)/\1Ë\2/g
s/(^|[^[:alnum:]_])left([^[:alnum:]_]|$)/\1H\2/g
s/(^|[^[:alnum:]_])down([^[:alnum:]_]|$)/\1J\2/g
s/(^|[^[:alnum:]_])up([^[:alnum:]_]|$)/\1K\2/g
s/(^|[^[:alnum:]_])right([^[:alnum:]_]|$)/\1L\2/g
s/(^|[^[:alnum:]_])lnbegin([^[:alnum:]_]|$)/\1ə\2/g
s/(^|[^[:alnum:]_])lnend([^[:alnum:]_]|$)/\1Ə\2/g
s/(^|[^[:alnum:]_])lngoto([^[:alnum:]_]|$)/\1ө\2/g
s/(^|[^[:alnum:]_])insert([^[:alnum:]_]|$)/\1I\2/g
s/(^|[^[:alnum:]_])yanky([^[:alnum:]_]|$)/\1Ô\2/g
s/(^|[^[:alnum:]_])deld([^[:alnum:]_]|$)/\1X\2/g
s/(^|[^[:alnum:]_])delx([^[:alnum:]_]|$)/\1Ó\2/g
s/(^|[^[:alnum:]_])delX([^[:alnum:]_]|$)/\1Ò\2/g
s/(^|[^[:alnum:]_])pastep([^[:alnum:]_]|$)/\1Ì\2/g
s/(^|[^[:alnum:]_])pasteP([^[:alnum:]_]|$)/\1Í\2/g
s/(^|[^[:alnum:]_])flipcase([^[:alnum:]_]|$)/\1Ç\2/g
s/(^|[^[:alnum:]_])readfile([^[:alnum:]_]|$)/\1Ȓ\2/
s/(^|[^[:alnum:]_])redraw([^[:alnum:]_]|$)/\1🔥\2/g
s/quit([^[:alnum:]_]|$)/Ф\1/g
s/(^|[^[:alnum:]_])display([^[:alnum:]_]|$)/\1Y\2/
s/(^|[^[:alnum:]_])search([^[:alnum:]_]|$)/\1Ñ\2/
s/(^|[^[:alnum:]_])search_next([^[:alnum:]_]|$)/\1F\2/
s/(^|[^[:alnum:]_])clr_to_eol([^[:alnum:]_]|$)/\1C\2/
s/(^|[^[:alnum:]_])gomark([^[:alnum:]_]|$)/\1ǵ\2/
s/(^|[^[:alnum:]_])lnmark([^[:alnum:]_]|$)/\1Ǵ\2/
s/(^|[^[:alnum:]_])setmark([^[:alnum:]_]|$)/\1Ș\2/
s/(^|[^[:alnum:]_])setundo([^[:alnum:]_]|$)/\1U\2/
s/(^|[^[:alnum:]_])undo([^[:alnum:]_]|$)/\1Û\2/
s/(^|[^[:alnum:]_])bang([^[:alnum:]_]|$)/\1Ƃ\2/
s/(^|[^[:alnum:]_])cescape([^[:alnum:]_]|$)/\1Ɔ\2/
s/(^|[^[:alnum:]_])anchor([^[:alnum:]_]|$)/\1Â\2/
s/(^|[^[:alnum:]_])append([^[:alnum:]_]|$)/\1Ä\2/
s/(^|[^[:alnum:]_])error([0-9])/\1_\2/
s/(^|[^[:alnum:]_])getsigch([^[:alnum:]_]|$)/\1R\2/
s/(^|[^[:alnum:]_])cleanup([^[:alnum:]_]|$)/\1Ɵ\2/
s/(^|[^[:alnum:]_])prompt([^[:alnum:]_]|$)/\1Ƌ\2/
s/(^|[^[:alnum:]_])fileread([^[:alnum:]_]|$)/\1Ȑ\2/
s/(^|[^[:alnum:]_])filewrite([^[:alnum:]_]|$)/\1ȑ\2/
s/(^|[^[:alnum:]_])charwidth([^[:alnum:]_]|$)/\1G\2/
s/(^|[^[:alnum:]_])version([^[:alnum:]_]|$)/\1Ж\2/
s/(^|[^[:alnum:]_])isword([^[:alnum:]_]|$)/\1ω\2/


s/mblength([^[:alnum:]_]|$)/S\1/g
s/(^|[^[:alnum:]_])func([^[:alnum:]_]|$)/\1Κ\2/g
s/(^|[^[:alnum:]_])writefile([^[:alnum:]_]|$)/\1κ\2/

#
#  Variables
#
#  a, i, j, m, n, p, *s, *t, are used for parameters and local variables.
#

s/argc/i/g
s/argv/j/g
s/(^|[^[:alnum:]_])col(umn)*([^[:alnum:]_]|$)/\1a\3/g
s/(^|[^[:alnum:]_])off(set)*([^[:alnum:]_]|$)/\1n\3/g
s/(^|[^[:alnum:]_])cur_row([^[:alnum:]_]|$)/\1y\2/g
s/(^|[^[:alnum:]_])cur_col([^[:alnum:]_]|$)/\1x\2/g
s/(^|[^[:alnum:]_])cur([^[:alnum:]_]|$)/\1m\2/g
s/(^|[^[:alnum:]_])maxcol([^[:alnum:]_]|$)/\1n\2/g
s/(^|[^[:alnum:]_])key([^[:alnum:]_]|$)/\1Ќ\2/g
s/(^|[^[:alnum:]_])uhere([^[:alnum:]_]|$)/\1ö\2/g
s/(^|[^[:alnum:]_])uegap([^[:alnum:]_]|$)/\1ò\2/g
s/(^|[^[:alnum:]_])ugap([^[:alnum:]_]|$)/\1ó\2/g
s/(^|[^[:alnum:]_])here([^[:alnum:]_]|$)/\1o\2/g
s/(^|[^[:alnum:]_])epage([^[:alnum:]_]|$)/\1v\2/g
s/(^|[^[:alnum:]_])page([^[:alnum:]_]|$)/\1u\2/g
s/(^|[^[:alnum:]_])eof([^[:alnum:]_]|$)/\1n\2/g
s/(^|[^[:alnum:]_])mark([^[:alnum:]_]|$)/\1i\2/g
s/(^|[^[:alnum:]_])count([^[:alnum:]_]|$)/\1z\2/g
s/(^|[^[:alnum:]_])ebuf([^[:alnum:]_]|$)/\1c\2/g
s/(^|[^[:alnum:]_])buf([^[:alnum:]_]|$)/\1b\2/g
s/(^|[^[:alnum:]_])egap([^[:alnum:]_]|$)/\1h\2/g
s/(^|[^[:alnum:]_])gap([^[:alnum:]_]|$)/\1g\2/g
s/(^|[^[:alnum:]_])chg([^[:alnum:]_]|$)/\1Ȉ\2/g
s/(^|[^[:alnum:]_])ins([^[:alnum:]_]|$)/\1ȉ\2/g
s/(^|[^[:alnum:]_])cmd([^[:alnum:]_]|$)/\1Ƹ\2/g
s/(^|[^[:alnum:]_])mode([^[:alnum:]_]|$)/\1ƹ\2/g
s/(^|[^[:alnum:]_])filename([^[:alnum:]_]|$)/\1f\2/g
s/(^|[^[:alnum:]_])ch([^[:alnum:]_]|$)/\1a\2/g
s/(^|[^[:alnum:]_])wc([^[:alnum:]_]|$)/\1t\2/g
s/(^|[^[:alnum:]_])ere_dollar_only([^[:alnum:]_]|$)/\1w\2/g
s/(^|[^[:alnum:]_])ere([^[:alnum:]_]|$)/\1e\2/g
s/(^|[^[:alnum:]_])replace([^[:alnum:]_]|$)/\1r\2/g
s/(^|[^[:alnum:]_])match_length([^[:alnum:]_]|$)/\1l\2/g
s/(^|[^[:alnum:]_])matches([^[:alnum:]_]|$)/\1p\2/g
s/(^|[^[:alnum:]_])marks([^[:alnum:]_]|$)/\1m\2/g
s/(^|[^[:alnum:]_])marker([^[:alnum:]_]|$)/\1d\2/g
s/(^|[^[:alnum:]_])yank_length([^[:alnum:]_]|$)/\1Q\2/g
s/(^|[^[:alnum:]_])yank_text([^[:alnum:]_]|$)/\1q\2/g
s/(^|[^[:alnum:]_])pipein([^[:alnum:]_]|$)/\1x\2/g
s/(^|[^[:alnum:]_])pipeout([^[:alnum:]_]|$)/\1y\2/g
s/(^|[^[:alnum:]_])child_in([^[:alnum:]_]|$)/\1x\2/g
s/(^|[^[:alnum:]_])child_out([^[:alnum:]_]|$)/\1y\2/g
s/(^|[^[:alnum:]_])child([^[:alnum:]_]|$)/\1a\2/g
s/(^|[^[:alnum:]_])ex([^[:alnum:]_]|$)/\1i\2/g
s/(^|[^[:alnum:]_])from([^[:alnum:]_]|$)/\1s\2/g
s/(^|[^[:alnum:]_])to([^[:alnum:]_]|$)/\1t\2/g
s/(^|[^[:alnum:]_])min([^[:alnum:]_]|$)/\1a\2/g
s/(^|[^[:alnum:]_])xbuf([^[:alnum:]_]|$)/\1i\2/g
s/(^|[^[:alnum:]_])xhere([^[:alnum:]_]|$)/\1j\2/g
s/(^|[^[:alnum:]_])buflen([^[:alnum:]_]|$)/\1m\2/g
s/(^|[^[:alnum:]_])gap_off([^[:alnum:]_]|$)/\1n\2/g
s/(^|[^[:alnum:]_])str([^[:alnum:]_]|$)/\1s\2/g
s/(^|[^[:alnum:]_])mbl([^[:alnum:]_]|$)/\1t\2/g
#s/(^|[^[:alnum:]_])is_ctrl([^[:alnum:]_]|$)/\1U\2/g


#
#  Expressions
#

#    (type *) 0		->   0
s/([a-z ]*\*) *0/0/g

#    if (0 == expr)	->  if (!expr)
s/0 == /not /g
s/0 == /!/g

#    if (expr != 0)	->  if (expr)
s/ not_eq 0//g
s/ != 0//g

#    if (0 != expr)	->  if (expr)
s/0 not_eq //g
s/0 != //g

#    (v = func(expr)) == 0	->  !(v = func(expr))
s/(\([a-zA-Z0-9][a-zA-Z0-9]* = [^)]*\)\)) == 0/!\1/g

#    (expr) == 0	->  !(expr)
#s/(\([^(),]*\)) == 0/!\1/g
s/(\([^(),]*\)) == 0/not \1/g

#    expr == 0	->  !expr
#s/([^(),]*) == 0/!\1/g
s/([^(),]*) == 0/not \1/g

#    func(expr) == 0	->  !func(expr)
#s/([a-zA-Z0-9][a-zA-Z0-9]*\([^)]*\)) == 0/!\1/g
s/([a-zA-Z0-9][a-zA-Z0-9]*\([^)]*\)) == 0/not \1/g


#    array[0].mem	->  array->
s/([^ 	]*[^[])\[0\]\./\1->/g


#    array[0]	->  *array
#s/([^ 	]*[^[])\[0\]([^.])/*\1\2/g

#    &array[n]	->  array+n
#N#s/\&([^[]*)\[([^]]*)\]/\1+\2/g

#
# Split `} else`
#
s/^([[:blank:]]*)} else/\1}\
\1else/

#s/\t/ /g
