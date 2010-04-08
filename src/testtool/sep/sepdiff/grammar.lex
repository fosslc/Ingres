%e 6000
%p 15000
%n 2000
%k 10000
%a 20000
%o 20000
%option yylineno
%{

/*
** grammar.lex - This is the grammar which SEP uses to tokenize test results.
**
** History:
**	##-XXX-89 (eduardo & mca)
**		Written.
**	05-oct-89 (mca)
**		Don't use malloc to acquire token structures. Call get_token,
**		which will also try to find old token strucures to reuse.
**	25-jan-90 (twong)
**		Convex (cvx_u42) optimizer can not handle grammar.
**	16-may-1990 (rog)
**		Handle the input(), output(), and unput() functions correctly
**		for SEPFILE type files.
**	11-jun-1990 (rog)
**		Make sure external variables are declared correctly for VMS.
**	19-jul-1990 (rog)
**		Change include files to use <> syntax because the headers
**		are all in testtool/sep/hdr.
**	01-aug-1990 (rog)
**		VMS date and time stamps weren't allowing for enough space
**		between the date and time.
**	17-feb-1991 (seiwald)
**		Performance mods: replaced input() and unput() routines 
**		with their original lex macros and instead #defined getc 
**		to SEPgetc().  #defined output() to nothing.  Rewrote
**		put_token to use yyleng and MEcopy instead of STlength and
**		STcopy.
**	14-jan-1992 (donj)
**	    Elaborated on A_DATE token type to allow for recognition of addi-
**	    tional date formats. Added Ingres copyright masking. Created a
**	    A_FLOAT token type for exponential and decimal notation. A_NUMBER
**	    becomes integer only.
**	09-Jul-92 (GordonW)
**	    Bump up the %o parameter. On some machines are  we getting table
**	    overflow.
**	10-Jul-92 (GordonW)
**	    Bump up the %a parameter. On some machines are  we getting error
**	    running out of transition table size. (RS/6000 v3.2).
**	21-jul-92 (dkhor)
**	    Added grammar (character classes) to recognise DBL words.
**	05-aug-92 (dkhor)
**	    Back out (rejected) changes made on 21-jul-92 (change # 266845).
**	24-aug-1992 (donj)
**	    Make seconds optional in time string. Add back in the A_DBNAME
**	    types that were here and were lost.
**	02-dec-1992 (donj)
**	    Re-added dkhor's grammar (character classes) to recognise DBL words.
**		> dblhi           [\201-\377]
**		> dbllo           [\100-\377]
**		> dblword         ({dblhi}{dbllo})
**
**		> {dblword}+                    {put_token(A_DBLWORD); };
**	19-jan-1992 (donj)
**	    Bump %k (packed char classes) parameter. su4_u42 was complaining.
**	04-feb-1993 (donj)
**	    Took out some definitions that added nothing to readability. Had
**	    take out dkhor's doublebyte grammar, it was breaking date masking
**	    when the date was in a selected ingres table output. Added new
**	    token types like A_DBNAME these are Ingres objects, A_RULE,
**	    A_PROCEDURE, A_TABLE, A_USER, A_OWNER, A_COLUMN, A_DESTROY,
**	    A_DROP and A_OWNER. Reset the parameters to be roughly twice
**	    the actual usage.
**      13-apr-93 (vijay)
**          Bump up %o and %a: needed for AIX 3.2. Undefine "abs" since system
**          headers inserted by lex may define abs; and abs will then
**          be redefined in compat.h. (aix compiler does not allow redefine
**          without undefing first.)
**      26-apr-1993 (donj)
**          Finally found out why I was getting these "warning, trigrah sequence
**          replaced" messages. The question marks people used for unknown
**          dates. (Thanks DaveB).
**	 4-may-1993 (donj)
**	    NULLed out some char ptrs, Added prototyping of functions.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**	13-may-1993 (donj)
**	    Update for Alpha. Alpha cc complians on redefining NULL.
**	26-aug-1993 (donj)
**	    Change STprintf() to IISTprintf(), STmove() to IISTmove() and
**	    MEcopy() to IIMEcopy() due to changes in CL.
**	10-sep-1993 (donj)
**	    Added UPPERCASE versions of the object token types.
**	14-sep-1993 (dianeh)
**	    Removed NO_OPTIM setting for obsolete Convex port (cvx_u42).
**	15-oct-1993 (donj)
**	    Refine Day, Month, Hour and Minute token patterns. Add a A_GROUP
**	    token type.
**	15-oct-93 (vijay)
**	    Change IIMEcopy back to MEcopy and include me.h. MEcopy could
**	    be defined as memcpy etc on some systems. MEcopy should work
**	    if you include me.h.
**	30-Nov-93 (donj)
**	    65 frontends that aren't FORMS BASED are printing an additional
**	    '\r' when printing the copyright and company name. As a stop gap
**	    measure, I've added the [\r]? to the lexical grammar for
**	    A_COMPNAME token.
**	17-jan-1994 (donj)
**	    Modified A_DBNAME token to accept dbname with a vnode and server
**	    type specified.
**	28-jan-1994 (donj)
**	    Added new A_PRODUCT token to mask for matching "INGRES" and "OpenINGRES"
**	 9-Mar-1994 (donj)
**	    Added a date format used by the stress test customer application, prophecy.
**	    Example: 10JAN94 10:00pm (time optional).
**	22-mar-1994 (donj)
**	    A_BANNER wasn't handling HP-UX's banner correctly. The dash in "HP-UX"
**	    being excluded from the {alnum} tag.
**	20-apr-1994 (donj)
**	    Added support for new token type, A_COMPILE. For those pesky ABF
**	    "Compiling 10104.c" messages that always diff.
**	11-Jan-1996 (somsa01)
**		The version of flex which I have for NT (from the Internet) uses
**		ECHO, unlike the lex which comes from piccolo. Therefore, I
**		added a #define ECHO to nothing if NT_GENERIC and FLEX_SCANNAR 
**		are defined. This will cause nothing to be written to stdout
**		from the flex-generated .c file.
**	28-Jul-1998 (lamia01)
**		Added new version identifiers, Computer Associates as the company name,
**		and NT directory masking. Version identifier also works now with DBL.
**		New tokens - Database ID, Server ID, Alarm ID, Transaction ID and
**		Checkpoint ID introduced to avoid unnecessary diffs.
**	26-feb-1999 (somsa01)
**	    Time is more precise now.
**	22-mar-1999 (walro03)
**		Increase internal tables to prevent segv in lex.
**	25-Mar-1999 (popri01)
**		Although Iain's previous change works, we have since (re-)discovered
**		sep's sed option, which allows the same type of output masking on
**		a test-specific basis.  Due to the high cost of additional tokens
**		here, masking that is specific to a single test suite should be
**		done via sed and not via grammar.lex. Therefore, as a start, I
**		am migrating Iain's checkpoint file masking to sed for LAR tests.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jan-2001 (hanje04)
**              On Linux (ALL) we also (like NT) use Flex. Inlcuding
**              Linux to platforms having ECHO defined as nothing so that it
**              is not re-defined later on. Re-definition was cause thousands
**              of blank lines to be echoed to the .out files when sep is
**              running in batch mode.
**      21-Jun-2001 (horda03)
**              The token A_DIRECTORY is used to replace both directories and
**		file names on Unix and NT, while on VMS only directories are 
**		replaced. Modified the VMS pattern to optionally replace the 
**		file name too.
**	02-Nov-2001 (hanje04)
**		Added second copyright token to to handle new standard for
**		Copyright message.
**	15-May-2002 (hanje04)
**	    Added Itanium Linux (i64_lnx) to definition of ECHO
**	21-May-2003 (bonro01)
**	    Add support for HP Itanium (i64_hpu)
**	14-aug-2003 (devjo01)
**	    Add support for 2.65 related version tokens.
**	10-Mar-2004 (hanje04)
**	    BUG 111930
**	    Add '-' as a valid character in alnum so that sep can deal with
**	    directories with '-'s in.
**	20-Jul-2004 (hanje04)
**	    If using FLEX on Linux don't define yylineno, because 
**	    flex does too.
**	03-Aug-2004 (hanje04)
**	    The correct way to make sure yylineno is properly defined is to
**	    use %option yylineno. Added this and removed problematic 'C' 
**	    declaration.
**
**	2-aug-2004 (stephenb)
**	    update to previous change, fix for flex. This will builkd only
**	    with flex now.
**	08-Sep-2005(bonro01)
**	    On platforms which use the flex lexical compiler, SEP would output
**	    thousands of blank lines to the test output.
**	    The ECHO macro is used by flex to write tokens to stdout, but
**	    since we don't use this in SEP, we can disable it to prevent the
**	    blank lines.
**       06-Jan-2006 (hweho01)
**          Modified the parsing tokens for product name and version 
**          string.
**      01-Feb-2006 (hweho01)
**          Removed "ING" from verstring list.
**      07-Jul-2009 (hweho01)
**          The parsing token is modified, so 2-digit numbers are supported    
**          in the field of major release.
*/

# undef abs

#ifdef IS_ALPHA
#undef NULL
#endif
#include <compat.h>
#include <lo.h>
#include <me.h>

#include <sepdefs.h>
#include <sepfiles.h>
#include <token.h>

FUNC_EXTERN  i4          SEPgetc ( SEPFILE *fileptr ) ;

#undef getc
#define getc(f) SEPgetc(currSEPptr)
#undef output
#define output(a)	/*nada*/

GLOBALREF    FILE_TOKENS  *list_head ;
GLOBALREF    FILE_TOKENS  *list_tail ;
GLOBALREF    i4            no_tokens ;
GLOBALREF    i4            failMem ;
GLOBALREF    i4            diffSpaces ;
GLOBALREF    SEPFILE      *currSEPptr ;

#ifdef FLEX_SCANNER
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
    if ( (buf[0] = getc(yyin) ) == EOF)\
	result = YY_NULL;\
    else {\
	result = 1;\
	if (buf[0] == '\n')\
	    yylineno++;\
    }
/* Define ECHO as null to prevent flex lexical compiler from */
/* displaying blank lines.                                   */
#define ECHO

#endif

%}

letter		[$a-zA-Z]
alnum		[-#$.%&@!_a-zA-Z0-9]
hexa		[0-9a-fA-F]
day             (([0-2 ]?[0-9])|([3][01]))
mon             (([0 ][0-9])|([1][0-2]))
yr		([0-9]?[0-9]?[0-9][0-9])
hora            (([01 ]?[0-9])|([2][0-4]))
min             (([0-5][0-9])|([6][0]))
nsec            ([0-9][0-9])
time		([ ]{hora}:{min}(:{min}(.{nsec})?)?)
word            {letter}{alnum}*
vmsfile		([$_a-zA-Z0-9][$_\-a-zA-Z0-9]*)?"."([$_\-a-zA-Z0-9]*)?(";"[0-9]*)?

release		([0-9]?[0-9]"."[0-9]("5"?))
ingres          (("Open")?("INGRES"|"Ingres"|"Ingres {yr}"))

n_mon		(JAN|FEB|MAR|APR|MAY|JUN|JUL|AUG|SEP|OCT|NOV|DEC|[Jj]an|[Ff]eb|[Mm]ar|[Aa]pr|[Mm]ay|[Jj]un|[Jj]ul|[Aa]ug|[Ss]ep|[Oo]ct|[Nn]ov|[Dd]ec)
l_mon1		((JAN|FEBR)UARY|MARCH|APRIL|MAY|JUNE|JULY|AUGUST|OCTOBER|(SEPT|NOV|DEC)EMBER)
l_mon2		(([Jj]an|[Ff]ebr)uary|[Mm]arch|[Aa]pril|[Mm]ay|[Jj]une|[Jj]uly|[Aa]ugust|[Oo]ctober|([Ss]ept|[Nn]ov|[Dd]ec)ember)

n_dow           (SUN|MON|TUE|WED|THU|FRI|SAT|[Ss]un|[Mm]on|[Tt]ue|[Ww]ed|[Tt]hu|[Ff]ri|[Ss]at)

l_dow		((SUN|MON|TUES|WEDNES|THURS|FRI)DAY|([Ss]un|[Mm]on|[Tt]ues|[Ww]ednes|[Tt]hurs|[Ff]ri)day)

vers		({release}"/"[0-9][0-9]" ("{alnum}{alnum}{alnum}"."{alnum}{alnum}{alnum}"/"[0-9][0-9]")"("DBL"?))

n_vers 	({release}"/"[0-9][0-9][0-9][0-9]" ("{alnum}{alnum}{alnum}"."{alnum}{alnum}{alnum}"/"[0-9][0-9]")"("DBL"?))

verstring	(("II ")|("II")|("OI ")|("OI")|("OpING ")|("OPING"))

d_day		(("-"{day}"-")|("/"{day}"/")|("_"{day}"_"))
d_mon		(("-"({mon}|{n_mon})"-")|("/"({mon}|{n_mon})"/")|("_"({mon}|{n_mon})"_"))

dblhi           [\201-\377]
dbllo           [\100-\377]
dblword         ({dblhi}{dbllo})

vnode           ({word}"::")
server_type     ("/"(STAR|[St]ar|ALB|[Aa]lb|INGRES|[Ii]ngres|ORACLE|[Oo]racle|RDB|[Rr]db|RMS|[Rr]ms))

dbspec          {vnode}?{word}{server_type}?

obj             [,:]?[ \t]+((["']{alnum}+["'])|{alnum}+)[,]?
dbobj           [,:]?[ \t]+((["']{dbspec}["'])|{dbspec})[,]?

%%

[ \t]+				{if (diffSpaces) put_token(A_SPACE);};

(("Ingres Corporation")|("Computer Associates"(" Intl, Inc.")?))[\r]?	{put_token(A_COMPNAME);};

(([Dd]atabase)|(DATABASE)){dbobj}               {put_token(A_DBNAME);};

(([Rr]ule)|(RULE)){obj}                         {put_token(A_RULE);};

(([Cc]ompiling)|(COMPILING)){obj}              {put_token(A_COMPILE);};

(([Pp]rocedure)|(PROCEDURE)){obj}               {put_token(A_PROCEDURE);};

(([Tt]able)|(TABLE)){obj}                       {put_token(A_TABLE);};

(([Gg]roup)|(GROUP)){obj}                       {put_token(A_GROUP);};

(([Uu]ser)|(USER)){obj}                         {put_token(A_USER);};

(([Oo]wn(er|ed[ ]by))|(OWN(ER|ED[ ]BY))){obj}   {put_token(A_OWNER);};

(([Cc]olumn)|(COLUMN)){obj}                     {put_token(A_COLUMN);};

E_US1452[ ]DESTROY{obj}                         {put_token(A_DESTROY);};

E_US0AC1[ ]DROP{obj}                            {put_token(A_DROP);};

(("(owner ")|("(OWNER ")){word}")"              {put_token(A_OWNER);};

Copyright" (c) "{yr}[,][ \t]?{yr}		{put_token(A_COPYRIGHT);};
Copyright" "{yr}				{put_token(A_COPYRIGHT);};

[+]([-]+[+])+    				{put_token(A_RETCOLUMN); };

{verstring}?({vers}|{n_vers})				{put_token(A_VERSION); };

"svr_id_server"((" =")|([:]))[ \t]+[0-9]+		{put_token(A_SERVERID); };

"Tran-id: "[\<]({hexa}{16})[\>]			{put_token(A_TRANID); };

"ID "[:]" 0x"({hexa}{8})				{put_token(A_DATABSID); };

[\$]"a    _t_"({hexa}{16})				{put_token(A_ALARMID); };

[`]?{word}[:][[]{word}([.]{word})*[\]]({vmsfile})?[']?	|
[`]?[\/:]{alnum}+([\/:]{alnum}+)*[\/:]?[']?	|
[`]?({letter}[:])[\\|\/:]{alnum}+([\\|\/:]{alnum}+)*[\\|\/:]?[']?	{put_token(A_DIRECTORY); };

^{ingres}[ ]{alnum}+("-"{alnum}+)?[ ]Version[ ]{verstring}?({vers}|{n_vers})" login"	{put_token(A_BANNER); };

^{ingres}[ ]{word}[ ]{word}[ ]Version[ ]{verstring}?({vers}|{n_vers})" login"	{put_token(A_BANNER); };

^{ingres}[ ]{word}[ ]{word}[ ]{word}[ ]Version[ ]{verstring}?({vers}|{n_vers})" login"	{put_token(A_BANNER); };

^{ingres}[\/][^ ]+[ ]Version[ ]{release}	{put_token(A_BANNER); };

"Version"[ ]?[:][ \t]*{verstring}{release}	{put_token(A_BANNER); };

"("?{yr}{d_mon}{day}{time}?([ ]{letter}{letter}{letter})?")"?	{put_token(A_DATE);};

"("?{day}{d_mon}{yr}{time}?")"?		{put_token(A_DATE); };

"("?({mon}|{n_mon}){d_day}{yr}{time}?")"?	{put_token(A_DATE); };

"("?({n_dow}|{l_dow})[,]?[ ]({n_mon}|{l_mon1}|{l_mon2})[ ][ ]?{day}{time}[ ]{yr}")"?	{put_token(A_DATE); };

"("?({n_mon}|{l_mon1}|{l_mon2})[ ][ ]?{day}","?[ ]?{yr}{time}?")"?	{put_token(A_DATE); };

"("?{day}{n_mon}{yr}{time}?")"?                 {put_token(A_DATE); };

{hora}:{min}(:{min})?				{put_token(A_TIME); };

[1-9][0-9]*					{put_token(A_NUMBER); };

([0-9]+|[0-9]+\.[0-9]*|[0-9]*\.[0-9]+)([eE][-+]?[0-9]+)?[fF]? {put_token(A_FLOAT); };

(\\[0-7]([0-7][0-7]?)?)|(0[0-7]+)		{put_token(A_OCTAL); };

(\\x{hexa}({hexa}{hexa}?)?)|(0x{hexa}+)		{put_token(A_HEXA); };

{ingres}                                        {put_token(A_PRODUCT); };

{word}						{put_token(A_WORD); };

.						{put_token(A_UNKNOWN); };

%%

FUNC_EXTERN  char         *get_token ( i4  size ) ;

i4
put_token(i4 tokenid)
{
    FILE_TOKENS           *token = NULL ;

    if( !*yytext )
	return;

    token = (FILE_TOKENS *)get_token( sizeof( *token ) + yyleng + 1 );

    if( !token )
	return;

    token->lineno = yylineno;
    token->prev = token->next = NULL;
    token->token = (char *)(token + 1);
    token->tokenID = tokenid;
    token->token_no = ++no_tokens;

    MEcopy( yytext, yyleng + 1, token->token );

    if (!list_head)
    {
	list_head = list_tail = token;
    }
    else
    {
	token->prev = list_tail;
	list_tail->next = token;
	list_tail = token;
    }
}

i4
yywrap()
{
    yylineno = 1;
    return(1);
}
