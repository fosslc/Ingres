/*
**  Copyright (c) 2009, 2010 Ingres Corporation. All rights reserved.
** 
**  Name: pm.c -- parameter management functions 
**
**  Description:
**
**	A subset of the functions contained in this file are documented
**	entry points of the PM General Library module.  These functions
**	provide a read-only interface for PM data files, as described in
**	the PM General Library Specification document.
**
**  Externally callable GL functions:
**
**	PMdelete()	remove a (name, value) pair from the implicit
**			context.
**
**	PMmDelete()	remove a (name, value) pair from a specified
**			context.
**
**	PMexpToRegExp()	convert a PM expression to a regular expression 
**			(using the implicit context).
**
**	PMmExpToRegExp() convert a PM expression to a regular expression 
**			(using a specified context).
**
**	PMfree()	free memory associated with (and render unitialized)
**			the implicit context.
**
**	PMmFree()	free memory associated with (and render unitialized)
**			a specified context.
**
**	PMget()		return a value from the implicit context which
**			matches a resource request.
**
**	PMmGet()	return a value from a specified context which
**			matches a resource request.
**
**	PMgetDefault()	return a specified default resource name component
**			for the implicit context.
**
**	PMmGetDefault()	return a specified default resource name component
**			for a specified context.
**
**	PMhost()	return the name of the local host modified to be 
**			a valid resource name component.
**
**	PMmHost()	see PMhost().	
**
**	PMinit()	initialize the implicit PM context before call(s)
**			to PMload().
**
**	PMmInit()	initialize and return a PM context which can be
**			used in subsequent calls to PMmLoad(), etc.
**
**	PMinsert()	insert a (name, value) pair into the implicit
**			context.
**
**	PMmInsert()	insert a (name, value) pair into a specified
**			context.
**
**	PMload()	load the contents of a PM data file into the
**			implicit context. 
**
**	PMmLoad()	load the contents of a PM data file into a specified
**			context. 
**
**	PMscan()	return all resources stored in the implicit context
**			which match an egrep-style regular expression.
**
**	PMmScan()	return all resources stored in a specified context
**			which match an egrep-style regular expression.
**
**	PMsetDefault()	set a specified resource name component default
**			for the implicit context.
**
**	PMmSetDefault()	set a specified resource name component default
**			for a specific context.
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	14-dec-92 (tyler)
**		Fixed several bugs in the initial revision.
**	15-mar-93 (tyler)
**		Fixed public PM calls to use PM context (memory tag) 1
**		instead of 0 since PMfree() was broken for this reason. 
**	26-apr-93 (tyler)
**		Changed alloc_string() to alloc_mem() and added a
**		bunch of code which enables PMmLoad() to use a YACC 
**		generated yyparse() function to load PM files.
**	28-apr-93 (tyler)
**		Removed changes introduced on 26-apr-93.
**	28-apr-93 (tyler)
**		Fixed message ID problem introduced in earlier shuffle.
**	28-apr-93 (tyler)
**		Reintroduced changes removed earlier minus call to YACC
**		generated yyparse().  This file now contains a direct
**		C implementation of yyparse() which employs a recursive
**		descent algorithm.
**	24-may-93 (tyler)
**		Prepended token definitions with YY_ to be consistent
**		with parser symbols.  Added SCRATCH_MEMORY context to
**		eliminate leak of temporary memory allocated during calls
**		to PMget().  Had to move PMmFree() and PMfree() to eliminate
**		symbol redeclaration errors.  Also, PMexpToRegExp() now
**		gets declared in pm.h and is therefore officially part
**		of the GL.
**	27-may-93 (tyler)
**		Fixed bug in PMmInit() which caused a segmentation violation
**		when called without II_CONFIG or II_SYSTEM defined in the
**		environment and modified PMmLoad() to return PM_NO_II_SYSTEM
**		when called with a NULL LOCATION argument in this
**		circumstance.  Also fixed a bug introduced by the previous
**		change which broke PMget()'s ability to find resources
**		specified with the "!." syntax.
**	21-jun-93 (tyler)
**		Added support for proper handling of octal and string
**		constants within delimited value strings.  Changed the
**		interfaces of multiple context functions to use caller
**		allocated control blocks rather than memory tags and static
**		arrays for maintaining multiple contexts.  Fixed bug in
**		PMmExpToRegExp() which caused crashes on certain invalid
**		PM expressions.  Moved some functions to avoid compiler
**		redeclaration errors.
**	23-jun-93 (rmuth)
**		Declare context0 as static.
**	23-jun-93 (tyler)
**		Replaced separate declarations of PM_CONTEXT temporary
**		memory structure within various functions by a single
**		static global declaration.
**	25-jun-93 (tyler)
**		Fixed bug introduced in previous revision which caused
**		a crash during access to mutiple contexts.  Minor 
**		corrections to function headers.  Also fixed checking of
**		PM_MAX_ELEM array limits in PMmSetDefault(), PMmGetDefault(),
**		and PMmGetElem().
**	26-jul-93
**		Switched to PM_MAX_LINE from SI_MAX_TXT_REC for the maximum
**		line length in PM files since SI_MAX_TXT_REC has become 
**		too huge on some systems.  Modified PMmInit() to allocate
**		and return a PM control block rather than have the caller
**		pass it in.  MEgettag() and MEfreetag() now get called to
**		manange memory tags.  Added mechanism which allows trusted
**		clients of config.dat to specify that a second config.dat
**		to be loaded on top of the first.  Requires CONFIG_LOCAL
**		to appear in the value of ii.(host).config.privilege.(user)
**      26-aug-93 (huffman)
**              Ensure that space is allocated before calling a copy
**              function.
**	20-sep-93 (tyler)
**		Minor bug fixes - details below. 
**	 6-oct-93 (donj)
**		Fixed a Free-memory-read error, found by Purify, in PMmFree().
**		This 'could' cause some problems on some platforms. PMmFree()
**		was freeing the CONTEXT structure then accessing the structure
**		to free the ME tag.
**	18-oct-93 (tyler)
**		Added PMhost() and PMmHost() functions.  Updated function
**		headers out of date with respect to current mechanism for
**		designating the default context.	
**	16-dec-93 (tyler)
**		Fixed BUG which prevented II_CONFIG_LOCAL from working
**		when protect.dat didn't exist.  Made STATUS values returned
**		by PM functions ER loggable.  Fixed bug in PMmExpToRegExp()
**		which prevented the default component ("*") from being matched
**		by PMmScan() when a default component was set but not found.
**		Fixed PMhost() to return "*" if GChostname() returns EOS.
**	22-feb-94 (tyler)
**		Switched to new privilege resource format.  Added code to
**		handle of MEreqmem() failures.
**	23-may-95 (emmag)
**		Renamed bsearch to ii_bsearch on NT.
**      12-jun-1995 (canor01)
**          semaphore protect NMloc()'s static buffer (MCT)
**	07-sep-1995 (canor01)
**	    fix MCT semaphore placement for PMhost().
**	19-sep-1995 (shero03)
**	    fix MCT semaphore placement for PMget().
**      24-nov-1995 (abowler)
**              Corrected memory allocation (size and decision when needed) for
**              PM inserts (Bug 72382).
**	26-feb-1996 (canor01)
**		Correctly handle all backslash-escaped special characters
**		('\'', '\n', '\r', '\f', '\t', '\b' and '\\') both on 
**		input and output.
**	22-apr-1996 (lawst01)
**	   Windows 3.1 port changes - added include for cv.h and gc.h
**	   added cast of variable.
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores. Removed ME_stream_sem. Removed NMloc() semaphore.
**	    Replaced PM_misc_sem with static MU semaphores.
**	10-oct-1996 (canor01)
**	    Allow for generic override of system parameters.
**	16-may-1997 (muhpa01)
**	    Added hp8_us5 to platforms which need to rename bsearch() to
**	    ii_besearch()
**	10-aug-1997 (canor01)
**	    Initialize the scratch context and semaphore only once to 
**	    prevent window for corruption when running multi-threaded.
**      25-Sep-97 (fanra01)
**          Bug 85215.
**          Changed the expansion of the list in PMminsert to use MEcopylng
**          since the size can exceed a u_i2 when the config.dat files is huge.
**	23-feb-1998 (fucch01)
**	    Set NO_OPTIM for sgi_us5 to resolve bug where iigenres calls
**	    pm functions to setup config.dat (default values...)
**	18-apr-1998 (canor01)
**	    Add rs4_us5 to platforms which need to rename bsearch() to
**	    ii_bsearch()
**	01-Jun-1998 (hanch04)
**	    Use ii_bsearch for all platforms, not bsearch
**      21-Jul-1998 (hanal04)
**         If II_CONFIG_LOCAL is set use config.dat from that directory
**         rather than II_SYSTEM/ingres/files. Also modify existing use
**         of II_CONFIG_LOCAL to ignore it if set to ON. b91480.
**  04-Aug-1998 (horda03) X_Integration of change 436925
**      29-Jul-1998 (horda03)
**         Corrected the change for bug 91480 which broke the VMS build.
**         The changes for this bug accessed the fields of LOCATION
**         directly. The fields of this variable type is platform
**         specific. This change makes use of the LOdetail function which
**         obtains the LOCATION components for the appropriate platform.
**	21-jan-1999 (canor01)
**	    Remove erglf.h and generated-file dependencies.
** 	16-apr-1999 (wonst02)
** 	    If SIgetrec( ) fails, be sure to call SIclose( ).
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**      06-May-1999 (allmi01)
**         Added rmx_us5 to list of platforms which rename bsearch. This is
**         to avoid conflict with the system defined function bsearch.
**      03-jul-1999 (podni01)
**         Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	22-nov-00 (loera01) Bug 103318
**         For PMinit(), added PM_DUP_INIT error to give greater granularity
** 	   to duplicate attempts to initialize PM.
**	13-oct-1999 (mcgem01)
**	    Allow for component names which begin with an underscore.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	09-feb-2001 (somsa01)
**	    Changed several variable types to SIZE_TYPE to properly
**	    reflect the values that they would be set to.
**	09-apr-2001 (mcgem01)
**	    Allow for a username with an embedded $ in it e.g. li$na01.
**	08-May-2001 (hweho01)
**	    Turned off optimizer to avoid the compiler error "1500-004:   
**          (U) INTERNAL COMPILER ERROR while compiling yylex" for 
**          ris_u64 platform. (IBM C and C++ compiler, version 3.6.6.6) 
**	02-aug-2001 (somsa01)
**	    We now use a context's 'data_loaded' setting to decide whether
**	    or not we've already loaded data into this context or not.
**	28-Jun-2002 (thaju02)
**	   Initialize context0 to (PM_CONTEXT *)NULL. (B108079)
**	06-mar-2003 (devjo01)
**	    Modified to address memory leakage.  Basically by avoiding
**	    PMmGetElem in PMmGet, PMmInsert, PMmDelete and descendants.
**	    PMmGetElem itself still leaks memory, but addressing this
**	    efficiently would require changing the public interface.
**	    A cleaner and quicker means of extracting parameter elements
**	    was written for use within PM, which avoids any externally
**	    visible changes, yet addresses the bug.
**	23-apr-2003 (penga03)
**	    Made following changes:
**	    Added Local_CMcpychar(), and replaced all the references to 
**	    CMcpychar with Local_CMcpychar. 
**	    In PMmInit(), added CMset_attr call to initialize CM attribute 
**	    table. 
**	    If count the string by characters, use STnmbcpy, if count 
**	    the string by bytes, use STncpy.
**	    Added @ and # for YY_RESOURCE_NAME so that a username with @ 
**	    or #, that is supported in Windows, can also be supported in 
**	    Ingres.
**	16-Jun-2004 (schka24)
**	    Fix potential buffer overruns with env variables.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**	26-Jul-2004 (lakvi01)
**	    Backed-out the above change to keep the open-source stable.
**	    Will be revisited and submitted at a later date. 
**	16-Jan-2009 (bonro01)
**	    Replace Local_CMcpychar() with CMcpychar() in yylex() routine
**	    because it was causing an abend with UTF8 character sets.
**	    The Local_CMcpychar() routine was not designed to copy to an
**	    uninitialized destination buffer and it overflowed the buffer and
**	    overwrote other variables on the stack.
**	    The Local_CMcpychar() routine was designed to do an in place
**	    substitution instead of what was needed in this routine which
**	    was to copy one character at a time.
** 	23-Sep-2009 (wonst02) SIR 122098
** 	    Change yyresource_list from recursion to a loop when loading CONFIG.
**	16-Jul-2009 (whiro01)
**	    Some small optimizations involving "CMnext()" calls that could
**	    just be ptr++ and calls to STlength to compare against one
**	    character strings; added "stpcpy" routine to optimize copies
**	    followed by bumping pointer past copied contents; got rid of
**	    the Local_CMcpychar nonsense.
**	    In PMmExpToRegExp added support for "!" at beginning
**	    (hopefully matches PMmSearch now).
**	16-Dec-2009 (whiro01)
**	    The previous change 501845 caused a compile problem on SUSE Linux.
**	    Renamed 'stpcpy' to 'string_copy' since it conflicts with a Linux
**	    library function.
**	17-Dec-2009 (whiro01)
**	    The change to "head_remove" also broke HOQA because the params
**	    to "MEmemmove" were wrong.  Pointers were also wrong for replacing
**	    octal constants with the char value.
**	20-Jan-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
*/

/*
NO_OPTIM=sgi_us5 ris_u64
*/

# include <compat.h>
# include <gl.h>
# include <st.h>
# include <si.h>
# include <lo.h>
# include <er.h>
# include <cm.h>
# include <nm.h>
# include <id.h>
# include <gcccl.h>
# include <me.h>
# include <st.h>
# include <mu.h>
# include <pm.h>
# include "re.h"
# include <cs.h>
# include <cv.h>
# include <gc.h>

# if defined(rmx_us5) || defined(rux_us5)
# define bsearch ii_bsearch
# endif

/*
** A magic constant used to (probably) detect uninitialized control blocks.
** This constant is a sequence of alternating bits - which seems unlikely
** to occur in randomly allocated blocks of memory.  Assuming that bit
** sequences (other than all 0s - i.e. cleared memory) are truly random in
** allocated memory segments, this method has a 1 in 4294967295 chance
** of failing to detect an uninitialzed control block.
*/

# define PM_MAGIC_COOKIE 0xAAAA

/* Maximum line length supported by PM */ 

# define PM_MAX_LINE 1024 

/* Implicit context used by single-context entry points. */

static PM_CONTEXT *context0 = (PM_CONTEXT *)NULL;
static MU_SEMAPHORE context0_sem;

/* Temporary memory context */

static PM_CONTEXT *scratch;
static MU_SEMAPHORE scratch_sem;

/* Global buffer for value returned by PMhost() and PMmHost() */

static char pm_hostname[ GCC_L_NODE + 1 ];

/* Byte increment used for memory allocation */ 

# define MALLOC_SIZE	1024
 

/*
** Forward declarations of the parser functions
*/
static STATUS yyinit();
static bool yywrap( PM_CONTEXT *context );
static bool yyreserved( char *c );
static i4 yylex( PM_CONTEXT *context );
static void yyunlex( i4  token );
static char *yyresource_value( PM_CONTEXT *context );
static void yyresource_list( PM_CONTEXT *context );
static void yyparse( PM_CONTEXT *context );


/*
**  Name: string_copy() -- copy string and return pointer of end of result
**
**  Description:
**	This is a utility function to replace STcopy followed by STlength
**	to update the destination pointer, which traverses the string twice.
**	Just traverse the string once, copying it and return the end pointer
**	of the destination, ready for the next copy.
**
**  Inputs:
**	src		Pointer to source string (EOS-terminated)
**	dest		Pointer to destination buffer
**
**  Outputs:
**	None
**
**  Returns:
**	Pointer to new EOS position of destination (where next copy should begin)
**
**  History:
**	16-Jul-2009 (whiro01)
**		Initial coding.
**		I remember this code from the Lattice C compiler library
**		as far back as 1984.
*/
static char *
string_copy(const char *src, char *dest)
{
	while ( *src != EOS )
		*dest++ = *src++;
	*dest = EOS;
	return dest;
}


/*
**  Name: alloc_mem() -- allocate tagged memory for a string
**
**  Description:
**	Local micro-allocater function which returns a (char *) pointer
**	to a specified number of free bytes allocated from a specified
**	tagged memory pool.
**
**  Inputs:
**	context		Pointer to PM context control block
**	size		bytes of memory required. 
**
**  Outputs:
**	None
**
**  Returns:
**	Pointer to chunk of tagged memory cast to char *.
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	26-apr-93 (tyler)
**		Fixed inaccuracies in header.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
*/
static char * 
alloc_mem( PM_CONTEXT *context, SIZE_TYPE size )
{
	char *mem;

	if( size <= 0 )
		return( NULL );

	if( context->mem_free < size )
	{
		context->mem_free = ( size > MALLOC_SIZE ) ?
			size : MALLOC_SIZE; 
		context->mem_block = (char *) MEreqmem( context->memory_tag,
			context->mem_free, FALSE, NULL );
	}
	if( (mem = context->mem_block) == NULL )
		return( NULL );	
	context->mem_block += size;
	context->mem_free -= size;
	return( mem );
}


/*
**  Name: alloc_copy_string() -- allocate memory for a string and then
**	copy string into that memory.
**
**  Description:
**	This is a utility function to replace STlength, alloc_mem then STcopy
**	to reduce code clutter.
**
**  Inputs:
**	context		Pointer to PM context control block
**	string		source string to be copied
**
**  Outputs:
**	None
**
**  Returns:
**	Pointer to new string allocated from tagged memory or NULL
**	if memory allocation failed.
**
**  History:
**	16-Jul-2009 (whiro01)
**		Initial coding.
**	10-Dec-2009 (whiro01)
**		Added 'len' param for common case where we know the
**		length ahead of time (byte length that is)
*/
static char *
alloc_copy_string( PM_CONTEXT *context, const char *string, int len )
{
	char *mem = NULL;
	if( string != NULL )
	{
		SIZE_TYPE size;
		if (len == -1)
			size = STlength( string );
		else
			size = (SIZE_TYPE)len;
		mem = alloc_mem( context, size + 1 );
		if ( mem != NULL )
		{
			if (len == -1)
				STcopy( string, mem );
			else
			{
				MEcopy( string, len, mem );
				mem[len] = '\0';
			}
		}
	}
	return mem;
}


/*
**  Name: concat_strings() -- concatentate two strings 
**
**  Description:
**	Concatenates two strings using alloc_mem() micro-allocater.
**
**  Inputs:
**	context		Pointer to PM context control block
**	s1		first string	
**	s2		second string	
**
**  Outputs:
**	None
**
**  Returns:
**	Pointer to new string.
**
**  History:
**	26-apr-93 (tyler)
**		Created.
**	28-apr-93 (tyler)
**		Added context argument.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
**	20-sep-93 (tyler)
**		NULL arguments are now handled gracefully (without
**		crashing).
**	15-Dec-2009 (whiro01)
**		Using MEcopy (since we know the string lengths) to make it faster.
*/
static char
*concat_strings( PM_CONTEXT *context, char *s1, char *s2 )
{
	char *s;
	SIZE_TYPE l1, l2;

	if( s2 == NULL )
		return( s1 );
	if( s1 == NULL )
		return( s2 );
	l1 = STlength( s1 );
	l2 = STlength( s2 );
	if( (s = alloc_mem( context, l1 + l2 + 1 )) != NULL )
	{
		MEcopy( s1, l1, s );
		MEcopy( s2, l2, &s[ l1 ] );
		s[ l1 + l2 ] = EOS;
	} 
	return( s );
}


/*
**  Name: head_remove() -- remove a character from a string 
**
**  Description:
**	Removes a character from a string by manipulating the strings
**	head (front) rather than tail.
**
**  Inputs:
**	c		pointer to character to be removed.
**	s		pointer to pointer to string.
**	cnt		number of bytes to remove (size of the char at 'c')
**
**  Outputs:
**	s		pointer to pointer to string. 
**
**  Returns:
**	None	
**
**  History:
**	21-jun-93 (tyler)
**		Created.
**	21-apr-2003 (penga03)
**	    Modified to take it into account that **s has mixed single 
**	    byte and double byte charateres.
**	10-dec-2009 (whiro01)
**	    This code was actually incorrect and MUCH slower than necessary.
**	    Just do a memory move taking into account the byte len
**	    of the char to be deleted.
**	17-dec-2009 (whiro01)
**	    Don't move (len - cnt) bytes, move 'len' bytes.
*/
static void
head_remove( char *c, char **s, int cnt )
{
	SIZE_TYPE len = c - *s;

	/* Move everything from *s up to c by cnt bytes */
	MEmemmove(*s + cnt, *s, len);
	/* Update the head pointer by number of bytes removed */
	*s += cnt;
}


/*
**  Names: yyerror(), yyinit(), yywrap(), yyreserved(), yylex(), yyunlex(),
**	yyresource_value(), yyresource_list(), yyparse()
**
**  Description:
**	These functions implement a recursive descent parser which was
**	orignially handled by a YACC grammar - which had to be removed
**	for build reasons.  The wierd function names are an artifact
**	of this file's short-lived association with a YACC generated
**	parser. 
**
**  History:
**	26-apr-93 (tyler)
**		Created.
**	28-apr-93 (tyler)
**		Replaced call to YACC-generated yyparse() with hand
**		built recursive descent parser.
**	21-jun-93 (tyler)
**		Added code which processes string ('\n', '\r', '\f',
**		'\b', '\t') and octal constants ( '\ddd' - where ddd
**		represents 1-3 octal digits) according to the spec. 
**		Switched to use of control block for maintaining context.
**	26-jul-93 (tyler)
**		yylex() uses PM_MAX_LINE instead of SI_MAX_TXT_REC as 
**		the size of yytext[] (allocated on the stack) since
**		SI_MAX_TXT_REC is ridiculously huge on many machines.
**		Removed loc argument from yyinit(); function assumes 
**		that yyloc, yyloc_buf, and yyloc_path are defined
**		appropriately.  Added error message for failure to
**		open file condition (E_GL6001_PM_OPEN_FAILED).
**		Removed err_func argument from yyinit().  Added support
**		for undocumented II_CONFIG_LOCAL environment setting
**		and CONFIG_LOCAL privilege.
**	20-sep-93 (tyler)
**		Modified yyresource_value() to eliminate multiple reports
**		of certain syntax errors.  Fixed yywrap() bug which
**		broke the II_CONFIG_LOCAL feature.
**	18-oct-93 (tyler)
**		Added PMhost() and PMmHost() functions.
**	16-dec-93 (tyler)
**		Modified yywrap() to support II_CONFIG_LOCAL feature
**		even if protect.dat doesn't exist.  Syntax errors encountered
**		at the end of config.dat are now reported correctly.
**	11-apr-2003 (penga03)
**	    In PMmInit, added CMset_attr call to initialize CM 
**	    attribute table.
**	    In yylex, count the double byte situation when putting null 
**	    character to the copied string. Also, added @ and # to 
**	    YY_RESOURCE_NAME so that a username with @ or #, that is 
**	    supported in Windows, can also be supported in Ingres.
*/

/* PM token definitions */ 

# define YY_RESOURCE_NAME	1	
# define YY_ASSIGNMENT_OP	2	
# define YY_PLAIN_STRING	3	
# define YY_DELIM_STRING	4	
# define YY_FAIL		5	

/* Miscellaneous parser variables */ 

static FILE	*yyin;
static char	yybuf[ PM_MAX_LINE + 1 ];
static char	*yyp;
static char	yynull = EOS;
static i4	yylineno;
static i4	yycol;
static bool	yyloc_default;
static PM_ERR_FUNC *yyerr_func;
static LOCATION	yyloc;
static char	yyloc_buf[ MAX_LOC + 1 ];
static char	*yyloc_path;
static bool	yyfail = FALSE;
static i4	yywraps;
static i4	yycache = -1;
static struct { char *string; } yylval;


static void
yyerror( void )
{
	ER_ARGUMENT er_args[ 2 ];
	i4 lineno = yylineno;

	er_args[ 0 ].er_value = (PTR) &lineno; 
	er_args[ 0 ].er_size = ER_PTR_ARGUMENT;
	er_args[ 1 ].er_value = (PTR) yyloc_path; 
	er_args[ 1 ].er_size = ER_PTR_ARGUMENT;

	if( yyerr_func != (PM_ERR_FUNC *) NULL )
		yyerr_func( PM_SYNTAX_ERROR, 2, er_args ); 

	yyfail = TRUE;
}

static STATUS
yyinit()
{
	STATUS status;

	/* zero the line counter */
	yylineno = 0;

	/* open parameter file */
	status = SIfopen( &yyloc, ERx( "r" ), SI_TXT, PM_MAX_LINE, &yyin );

	if( status != OK && yyerr_func != (PM_ERR_FUNC *) NULL )
	{
		ER_ARGUMENT er_arg;

		er_arg.er_value = (PTR) yyloc_path; 
		er_arg.er_size = ER_PTR_ARGUMENT;

		yyerr_func( PM_OPEN_FAILED, 1, &er_arg ); 
	}

	/* initialize input pointer */
	yyp = &yynull;

	yywraps = 0;

	return( status );
}

static bool
yywrap( PM_CONTEXT *context )
{
	char *save_yyloc_path = NULL;
	STATUS s;

	(void) SIclose( yyin );

	if( !yyloc_default )
		return( TRUE );
wrap:
	switch( yywraps )
	{
		case 0:
			/* prepare protect.dat LOCATION */ 
			++yywraps;
			save_yyloc_path = STalloc( yyloc_path );
			NMloc( ADMIN, FILENAME, ERx( "protect.dat" ),
				&yyloc );
			LOcopy( &yyloc, yyloc_buf, &yyloc );	
			LOtos( &yyloc, &yyloc_path );
			break;

		case 1:
		{
			char *username, *hostname;
			char request[ PM_MAX_LINE ], *value, *p;
			char *ii_config_local;
			bool trusted = FALSE;
			i4 i;
# define MAX_CONFIG_PRIV 50
			char *privs[ MAX_CONFIG_PRIV ];
			i4 nprivs = MAX_CONFIG_PRIV;

			hostname = PMmHost( context );
			username = (char *) MEreqmem( 0, 100, FALSE, NULL );
			if( username == NULL )
			{
				/* NEED TO LOG OUT OF MEMORY ERROR HERE */ 
				return( TRUE );
			}
			IDname( &username );
			STprintf( request, ERx( "%s.%s.privileges.user.%s" ),
				SystemCfgPrefix, hostname, username );
			MEfree( username );

			if( PMmGet( context, request, &value ) != OK )
				return( TRUE );

			value = STalloc( value );
			for( p = value; *p != EOS; CMnext( p ) )
			{
				if( *p == ',' )
					*p = ' ';
			}
			STgetwords( value, &nprivs, privs ); 

			/* search privilege resource for CONFIG_LOCAL */ 
			for( i = 0; i < nprivs; i++ )
			{
				if( STncmp( privs[ i ],
					ERx( "CONFIG_LOCAL" ), 12 ) == 0 )
				{
					trusted = TRUE;
					break;
				}
			}
			MEfree( value );

			if( !trusted )
				return( TRUE );

			NMgtAt( ERx( "II_CONFIG_LOCAL" ), &ii_config_local );
                        /* b91480 - Prevent corruption of environment   */
                        /* variable and ignore it if its set to ON.     */
			if( ii_config_local == NULL ||
				*ii_config_local == EOS ||
                                (STcompare(ii_config_local, "ON") == 0))
			{
				return( TRUE ); 
			}

			/* prepare local config.dat LOCATION */ 
			STlcopy(ii_config_local, yyloc_buf, sizeof(yyloc_buf)-1);
			save_yyloc_path = STalloc( yyloc_path );
			LOfroms( PATH, yyloc_buf, &yyloc );
			LOfstfile( ERx( "config.dat" ), &yyloc );
			LOtos( &yyloc, &yyloc_path );
			++yywraps;
			break;
		}

		default:
			return( TRUE );
	}

	s = SIfopen( &yyloc, ERx( "r" ), SI_TXT, PM_MAX_LINE, &yyin );
	if (s == OK)
	{
	    s = SIgetrec( yybuf, PM_MAX_LINE + 1, yyin );
	    if (s != OK )
	    {
	    	(void) SIclose( yyin );
	    }
	}
	if( s != OK )
	{
		if( save_yyloc_path != NULL )
		{
			/* restore yyloc_path for correct error report */
			STcopy( save_yyloc_path, yyloc_buf );
			LOfroms( PATH & FILENAME, yyloc_buf, &yyloc );
			LOtos( &yyloc, &yyloc_path );
			MEfree( save_yyloc_path );
			save_yyloc_path = NULL;
		}
		goto wrap;
	}

	/* free temporary storage */
	if( save_yyloc_path != NULL )
            {
		MEfree( save_yyloc_path );
            }

	/* initialize the line counter */
	yylineno = 0;

	/* file opened successfully */
	return( FALSE );
}

static bool
yyreserved( char *c )
{
	if( CMwhite( c ) ||
	    *c == '\''   ||
	    *c == '"'    ||
	    *c == '`'    ||
	    *c == '\\'   ||
	    *c == '#' )
	{
		return( TRUE );
	}
	return( FALSE );
}

static i4
yylex( PM_CONTEXT *context )
{
	bool component;
	char *yystart;
	i4 yyleng;	/* byte count, NOT character count */
	int cnt;	/* count of bytes in current character */

	if( yycache >= 0 )
	{
		i4 token = yycache;

		yycache = -1;
		return( token );
	}

	/* skip whitespace and comments */
	/* anytime we know what character we have we can use yyp++ */
	/* instead of the (very expensive) CMnext() call           */
	while( CMwhite( yyp ) || *yyp == '#' || *yyp == EOS )
	{
		if( *yyp == '#' )
		{
			while( *yyp != EOS )
				yyp++;
			/* It is not necessary to deal with yycol here because the value */
			/* won't be used until we get to the next line anyway.           */
		}
		if( *yyp == EOS )
		{
			if( SIgetrec( yybuf, PM_MAX_LINE + 1, yyin ) != OK &&
				yywrap( context )
			)
				return( 0 );
			else
			{
				yyp = yybuf;
				++yylineno;
				yycol = 1;
			}
		}
		else
		{
			++yycol;
			CMnext( yyp );
		}
	}

	yyleng = 0;

	/* check for a YY_RESOURCE_NAME */
	if( yycol == 1 )
	{
		yystart = yyp;
		while( CMalpha( yyp ) || CMdigit( yyp ) || *yyp == '*' 
			|| *yyp == '_')
		{
			component = FALSE;
	
			/* check for single-character component */
			if( *yyp == '*' ) 
			{
				component = TRUE;
				++yyleng;
				++yycol;
				++yyp;
				if( *yyp == '.' )
				{
					++yyleng;
					++yycol;
					++yyp;
					continue;
				}
				else if( CMalpha( yyp ) )
				{
					yyerror(); 
					return( YY_FAIL );
				}
				continue;
			}
	
			while( CMalpha( yyp ) || CMdigit( yyp ) ||
				*yyp == '_' || *yyp == '-' || *yyp == '$' || *yyp == '@' || *yyp == '#')
			{
				cnt = CMbytecnt( yyp );		/* number of bytes this char occupies */
				yyleng += cnt;
				yyp += cnt;
				++yycol;
				component = TRUE;
			}
			if( component )
			{
				if( *yyp == '.' )
				{
					++yyleng;
					++yycol;
					++yyp;
					continue;
				}
				continue;
			}
		}
	
		/* check for uneaten '.' */
		if( *yyp == '.' )
		{
			yyerror();
			return( YY_FAIL );
		}
	
		if( yyleng > 0 )
		{
			/* YY_RESOURCE_NAME was scanned */
			if( (yylval.string = alloc_copy_string( context,
				yystart, yyleng )) == NULL )
			{ 
				/* NEED TO LOG OUT OF MEMORY ERROR HERE */ 
				return( YY_FAIL );
			}
			return( YY_RESOURCE_NAME );
		}
	}

	if( yycol != 1 )
	{
		bool string_found;

		/* check for YY_DELIM_STRING */
		if( *yyp == '\'' )
		{
			char *head = yyp, *yyprev;
			bool twobackslash = FALSE;
	
			for( yyleng = 0, ++yycol, CMnext( yyp ),
				yyprev = &yynull;
				*yyp != '\'' || *yyprev == '\\';
				yyprev = ( *yyprev == EOS ) ? yyp :
				CMnext( yyprev ), ++yycol,
				CMbyteinc( yyleng, yyp), CMnext( yyp ) )
			{
				if( *yyp == '\'' && *yyprev == '\\' )
				{
					/* remove escape before ' */
					--yyleng;
					head_remove( yyprev, &head, 1 );
				}
				else if( *yyprev == '\\' )
				{
					/* have we processed two '\\' ? */
					if ( twobackslash == TRUE )
					{
						--yyleng;
						head_remove( yyprev, &head, 1 );
						twobackslash = FALSE;
						continue;
					}
					/* process C string literals */
					switch( *yyp )
					{
						case 'n':
						case 't':
						case 'r':
						case 'f':
						case 'b':
							--yyleng;
							head_remove( yyprev, &head, 1 );
							switch( *yyp )
							{
								case 'n':
									*yyp = '\n';
									break;
								case 't':
									*yyp = '\t';
									break;
								case 'r':
									*yyp = '\r';
									break;
								case 'f':
									*yyp = '\f';
									break;
								case 'b':
									*yyp = '\b';
									break;
							}
							break;
						case '\\':
							twobackslash = TRUE;
							break;
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						{
							char *pp = yyp;
							i4 bytes;
							i4 val = (i4)( *pp++ - '0' );
							if( *pp >= '0' && *pp <= '7' )
								val = val * 8 + (i4)( *pp++ - '0' );
							if( *pp >= '0' && *pp <= '7' )
								val = val * 8 + (i4)( *pp++ - '0' );
							if( val == 0 )
							{
								/* disallow octal 0 */
								yyerror();
								return( YY_FAIL );
							}
							/* Remove the leading backslash */
							/* and all but one of the digits */
							--yyleng;
							bytes = (i4)(pp - yyp);
							head_remove( yyprev, &head, bytes );
							bytes--;
							/* Bump past the removed chars */
							yyprev += bytes;
							yyp += bytes;
							*yyp = (char)val;
							break;
						}
					}
				}
	
				if( *yyp == EOS )
				{
					yyerror();
					return( YY_FAIL );
				}
			}
			++yycol;
			CMnext( yyp );
			yystart = head;
			CMnext( yystart );
			if( (yylval.string = alloc_copy_string( context,
				yystart, yyleng )) == NULL )
			{
				/* NEED TO LOG OUT OF MEMORY ERROR HERE */ 
				return( YY_FAIL );
			}
			return( YY_DELIM_STRING );
		}

		/* check for YY_ASSIGNMENT_OP */
		if( *yyp == ':' || *yyp == '=' )
		{
			++yyleng;
			++yycol;
			++yyp;
			return( YY_ASSIGNMENT_OP );
		}

		/* check for YY_PLAIN_STRING */
		yystart = yyp;
		string_found = FALSE;
		while( *yyp != EOS && !yyreserved( yyp ) )
		{
			cnt = CMbytecnt(yyp);
			yyleng += cnt;
			yyp += cnt;
			++yycol;
			string_found = TRUE;
		}
		if( string_found ) 
		{
			if( (yylval.string = alloc_copy_string( context,
				yystart, yyleng )) == NULL )
			{
				/* NEED TO LOG OUT OF MEMORY ERROR HERE */ 
				return( YY_FAIL );
			}
			return( YY_PLAIN_STRING );
		}
	}

	/* anything else is an error */
	yyerror();
	return( YY_FAIL );
}

static void
yyunlex( i4  token )
{
	yycache = token;
}

static char
*yyresource_value( PM_CONTEXT *context )
{
	i4 token = yylex( context );
	char *s = yylval.string, *next;

	if( token == YY_FAIL )
		return( NULL );
	
	if( token != YY_PLAIN_STRING && token != YY_DELIM_STRING )	
	{
		yyunlex( token );
		return( &yynull );
	}

	if( (next = yyresource_value( context )) == NULL )
		return( NULL );

	return( concat_strings( context, s, next ) );
}

static void 
yyresource_list( PM_CONTEXT *context )
{
    while ( TRUE )
    {	
	i4	token = yylex( context );
	char    *name, *value;

	if (token == 0)
	    return;

	if( token != YY_RESOURCE_NAME )
	{
	    yyerror();
	    return;
	}

	name = yylval.string;

	if( yylex( context ) != YY_ASSIGNMENT_OP )
	{
	    yyerror();
	    return;
	}

	if( (value = yyresource_value( context )) == NULL )
	    return;	

	if( context->load_restriction == (RE_EXP *) NULL || 
	    REexec( context->load_restriction, name ) )
	{
	    PMmDelete( context, name );
	    PMmInsert( context, name, value );
	}
    }	
}

static void
yyparse( PM_CONTEXT *context )
{
	yyresource_list( context );
}


/*
**  Name: hash() -- compute a hash index
**
**  Description:
**	Local function to compute a hash index for a string. 
**
**  Inputs:
**	s	pointer to string to be hashed.
**
**  Outputs:
**	None
**
**  Returns:
**	The hash index.
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	16-Jul-09 (whiro01)
**		Optimized without using "CMnext()"
*/
static i4
hash( char *s ) 
{
	if( s == NULL || *s == EOS ) 
		return( 1 );
	else
	{
		i4	sum = (i4)*s++;
		while ( *s != EOS )
			sum += (i4)*s++;
		return( sum % PM_HASH_SIZE );
	}
}



/*
**  Name: PMmHost() -- return PM-modified local host name
**
**  Description:
**	Returns pointer to local host name modified to allow use as
**	a PM name component.  Converts illegal characters (i.e. '.' )
**	to the underbar character.  Since the modified host name is
**	stored in a static global buffer, no memory is allocated by
**	this function.
**
**  Inputs:
**	context		Pointer to PM context control block
**
**  Outputs:
**	None
**
**  Returns:
**	PM-modified local host name.	
**
**  History:
**	18-oct-93 (tyler)
**		Created.
**	16-dec-93 (tyler)
**		Now returns "*" when GChostname() returns EOS.
**	01-apr-96 (tutto01)
**		Modify host names on Windows NT for PM compliance.  Windows NT
**		allows the use of &, ', $, #, !, %, and blanks when defining
**		the host name.  Since PM cannot handle these, they are 
**		translated here into underscores.
**	01-apr-96 (tutto01)
**		Translation was meant to check for single quotes, not double.
*/
char *
PMmHost( PM_CONTEXT *context )
{
	static i4 init = TRUE;

	if( init )
	{
		char *p;

		GChostname( pm_hostname, sizeof( pm_hostname ) ); 
		if( *pm_hostname == EOS )
		{
			pm_hostname[ 0 ] = '*';
			pm_hostname[ 1 ] = EOS;
			return( pm_hostname );
		}
		for( p = pm_hostname; *p != EOS; CMnext( p ) )
		{
			switch( *p )
			{ 
				case '.':
# ifdef NT_GENERIC
				case '&':
				case '\'':
				case '$':
				case '#':
				case '!':
				case ' ':
				case '%':
# endif
					*p = '_';
			}
		}
		init = FALSE;
	}
	return( pm_hostname );
}


/*
** PMhost() -- wrapper for PMmHost() which omits the "context" argument 
** 	and passes the default context to PMmHost().
**
**  History:
**	18-oct-93 (tyler)
**		Created.
*/
char *
PMhost( void )
{
	char	*hostname;

	hostname = PMmHost( context0 );
	return (hostname);
}


/*
**  Name: PMmLowerOn() -- switch to lower (from upper) case conversion
**
**  Description:
**	Causes pm routines to convert all name strings to lower case
**	before testing for equivalence.  Only relevant if returned names
**	are being displayed.	
**
**  Inputs:
**	context		Pointer to PM context control block
**
**  Outputs:
**	None
**
**  Returns:
**	None
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
*/
void
PMmLowerOn( PM_CONTEXT *context )
{
	context->force_lower = TRUE;
}


/*
** PMlowerOn() -- wrapper for PMmLowerOn() which omits the "context" argument 
** 	and passes the default context to PMmLowerOn().
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
*/
void
PMlowerOn( void )
{
	if (MUp_semaphore( &context0_sem ))
	    return;
	PMmLowerOn( context0 );
	MUv_semaphore( &context0_sem );
}


/*
**  Name: string_to_upper() -- force a string to upper case 
**
**  Description:
**	Local function to convert lower case characters in a string to
**	upper case.
**
**  Inputs:
**	s	string to be upper-cased.	
**
**  Outputs:
**	None
**
**  Returns:
**	None
**
**  History:
**	9-nov-92 (tyler)
**		Created.
*/
static void
string_to_upper( char *s )
{
	while( *s != EOS )
	{
		CMtoupper( s, s ); 
		CMnext( s );
	}
}


/*
**  Name: string_to_lower() -- force a string to lower case 
**
**  Description:
**	Local function to convert upper case characters in a string to
**	lower case.
**
**  Inputs:
**	s	string to be lower-cased.	
**
**  Outputs:
**	None
**
**  Returns:
**	None
**
**  History:
**	9-nov-92 (tyler)
**		Created.
*/
static void
string_to_lower( char *s )
{
	while( *s != EOS )
	{
		CMtolower( s, s ); 
		CMnext( s );
	}
}


/*
**  Name: PMmSetDefault() -- set a default name component 
**
**  Description:
**	Set the default resource name component correspoding to the given
**	index.  Default resource name components are stored in def_elem[] 
**	and used by PM routines to simplify look-up requests.
**
**  Inputs:
**	context		Pointer to PM context control block
**	idx		index of default resource name component	
**	s		string to be made the default 
**
**  Outputs:
**	None
**
**  Returns:
**	OK		default name component set successfully.
**	status
**	PM_NO_INIT	PM was not initialized.	
**	PM_BAD_INDEX	specified index less than 0 or greater than
**			or equal to PM_MAX_ELEM.
**	PM_NO_MEMORY	Unable to allocate memory.
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	18-jan-93 (tyler)
**		Now returns PM_NO_INIT if PM was not initialized.	
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
**	25-jun-93 (tyler)
**		PM_BAD_REQUEST is now returned if the specified resource name
**		component contains '.' or whitespace.  Fixed broken checking
**		of PM_MAX_ELEM array limit.
**	26-jul-93 (tyler)
**		Fixed test for PM_NO_INIT condition to avoid dumping
**		core on NULL control block pointer.
*/
STATUS
PMmSetDefault( PM_CONTEXT *context, i4  idx, char *s )
{
	char *p;

	if( context == (PM_CONTEXT *) NULL ||
		context->magic_cookie != PM_MAGIC_COOKIE ) 
	{
		return( PM_NO_INIT );
	}

	/* check for embedded whitespace or '.' */
	for( p = s; p != NULL && *p != EOS; CMnext( p ) )
	{
		if( CMwhite( p ) || *p == '.' )
			return( PM_BAD_REQUEST );
	}

	if( idx < 0 || idx >= PM_MAX_ELEM )
		return( PM_BAD_INDEX );

	if( s == NULL )
	{
		context->def_elem[ idx ] = NULL;
		/* find next highest default index */
		for( --idx; idx > -1 && context->def_elem[ idx ] == NULL;
			idx-- );
		context->high_idx = idx;
		return( OK );
	}

	if( (context->def_elem[ idx ] = alloc_copy_string( context,
		s, -1 )) == NULL )
	{
		return( PM_NO_MEMORY );
	}

	if( context->force_lower )
		string_to_lower( context->def_elem[ idx ] );
	else
		string_to_upper( context->def_elem[ idx ] );

	if( idx > context->high_idx )
		context->high_idx = idx;

	return( OK );
}


/*
**  Name: PMsetDefault() -- wrapper for PMmSetDefault() which omits the
** 	"context" argument and passes the default context to PMmSetDefault().
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
*/
STATUS
PMsetDefault( i4  idx, char *s )
{
	STATUS	status;

	status = MUp_semaphore( &context0_sem );
	if ( status )
	    return status;
	status = PMmSetDefault( context0, idx, s );
	MUv_semaphore( &context0_sem );
	
	return status;
}


/*
**  Name: PMmGetDefault() -- return a default name component 
**
**  Description:
**	Return the default resource name component (stored in def_elem[])
**	correspoding to the given index.
**
**  Inputs:
**	context		Pointer to PM context control block
**	idx		index of default resource name component	
**
**  Outputs:
**	None
**
**  Returns:
**	default resource name component or NULL if none is set.
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
**	25-jun-93 (tyler)
**		Fixed broken checking of PM_MAX_ELEM array limit.
*/
char *
PMmGetDefault( PM_CONTEXT *context, i4  idx )
{
	if( idx < 0 || idx >= PM_MAX_ELEM )
		return( NULL );
	return( context->def_elem[ idx ] );
}


/*
**  Name: PMgetDefault() -- wrapper for PMmGetDefault() which omits the
** 	"context" argument and passes the default context to PMmGetDefault().
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
*/
char *
PMgetDefault( i4  idx )
{
	char *	value;

	if (MUp_semaphore( &context0_sem ))
	    return NULL;
	value = PMmGetDefault( context0, idx );
	MUv_semaphore( &context0_sem );

	return value;
}


/*
**  Name: PMmNumElem() -- return the number of elements in a resource name
**
**  Description:
**	Counts the number of strings of non-whitespace characters seperated
**	by '.' in the argument.
**
**  Inputs:
**	context		Pointer to PM context control block
**	name		pointer to the name of the resource whose
**			elements are to be counted.
**
**  Outputs:
**	None.
**
**  Returns:
**	The number of name elements counted.
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
*/
i4
PMmNumElem( PM_CONTEXT *context, char *name )
{
	i4 len;
	char *c;

	if( *name == EOS )
		return( 1 );
	for( c = name, len = 0; *c != EOS; len++ )
		for( CMnext( c ) ; *c != EOS && *c != '.';
			CMnext( c ) );
	return( len );
}


/*
**  PMnumElem() -- wrapper for PMmNumElem() which omits the "context"
** 	argument and passes the default context to PMmNumElem().
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
*/
i4
PMnumElem( char *name )
{
	i4	Num;

	if (MUp_semaphore( &context0_sem ))
	    return 0;
	Num = PMmNumElem( context0, name );
	MUv_semaphore( &context0_sem );

	return Num;
}


/*
**  Name: PMmFree() -- free all PM data structures. 
**
**  Description:
**	Sets all counters to 0 and frees allocated memory structures 
**	corresponding to the specified PM context.
**
**  Inputs:
**	context		Pointer to PM context control block
**
**  Outputs:
**	None.
**
**  Returns:
**	None.
**
**  History:
**	9-nov-92 (tyler)
**		Created.	
**	14-dec-92 (tyler)
**		Added check to make sure context is in use before trying
**		to free it.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
**	26-jul-93 (tyler)
**		MEfreetag() now gets called to free the memory tag
**		associated with each PM control block.
**	23-aug-93 (mikes)
**		New calling sequence for MEfreetag
**	20-sep-93 (tyler)
**		Removed redundant assignment to magic_cookie field
**		which might cause a crash on the PC.
*/
void
PMmFree( PM_CONTEXT *context )
{
	u_i2 t;

	if( context == (PM_CONTEXT *) NULL ||
		context->magic_cookie != PM_MAGIC_COOKIE )
	{
		return;
	}
	context->magic_cookie = 0;
	t = context->memory_tag;
	(void) MEtfree(t);
	(void) MEfreetag(t);
}


/*
** PMfree() -- wrapper for PMmFree() which omits the "context" argument 
** 	and passes the default context to PMmFree().
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
**	26-jul-93 (tyler)
**		context0 now gets reset to NULL.
*/
void
PMfree( void )
{
	if (MUp_semaphore( &context0_sem ))
	    return;
	PMmFree( context0 );
	context0 = (PM_CONTEXT *) NULL;
	MUv_semaphore( &context0_sem );
	MUr_semaphore( &context0_sem );
	PMmFree( scratch );
	scratch = (PM_CONTEXT *) NULL;
	MUv_semaphore( &scratch_sem );
	MUr_semaphore( &scratch_sem );
}


/*
**  Name: PMmGetElem() -- return the specified element of a resource name. 
**
**  Description:
**	Returns an element of a resource name corresponding to an index.
**
**  Inputs:
**	context		Pointer to PM context control block
**	idx		the index of the elem to be returned.  0
**			indicates the first element.
**	name		pointer to the name of the resource from
**			which an element is to be extracted.
**
**  Outputs:
**	None.
**
**  Returns:
**	NULL		An invalid name component element index was
**			specified or unable to allocated memory.
**	Otherwise	A pointer to a string containing the specified
**			resource name element.  The string is allocated
**			internally by the PMmGetElem() and should not
**			be freed by the caller.	
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
**	25-jun-93 (tyler)
**		NULL is now returned if the specified index is an invalid
**		PM array index.
*/
char *
PMmGetElem( PM_CONTEXT *context, i4  idx, char *name )
{
	char *c, *begin, *end;
	i4 len;

	if( name == NULL || idx < 0 || idx >= PM_MAX_ELEM )
		return( NULL );

	/* find name element sub-string */
	for( len = 0, end = NULL; len <= idx; len++ )
	{
		if( end == NULL )
			{
			end = name;
			}
		else
			CMnext( end );
		begin = end; 
		if( *begin == EOS )
			return( NULL );
		for( ; *end != EOS && *end != '.'; CMnext( end ) );
	}

	/* count bytes in sub-string */
	for( len = 0, c = begin; c != end; CMbyteinc( len, c ), CMnext( c ) ); 
	if( len == 0 )
		return( NULL );

        /* duplicate sub-string */
        if( (c = alloc_mem( context, len + 1 )) == NULL )
		return( NULL);
        STncpy( c, begin, len );
	c[ len ] = EOS;

        return( c );
}


/*
**  PMgetElem() -- wrapper for PMmGetElem() which omits the "context" argument
** 	and passes the default context to PMmGetElem().
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
*/
char *
PMgetElem( i4  idx, char *name )
{
	char *	value;

	if (MUp_semaphore( &context0_sem ))
	    return NULL;
	value = PMmGetElem( context0, idx, name );
	MUv_semaphore( &context0_sem );

	return value;
}


/*
**  Name: parse_request -- Copy & correct case of passed string &
**			   populate elements array.
**
*/
static	i4
parse_request( PM_CONTEXT *context, char *request,
	       char *(*elements)[PM_MAX_ELEM] )
{
    char	*piece;
    i4		 i, count;

    piece = request;

    if( context->force_lower )
	    string_to_lower( piece );
    else
	    string_to_upper( piece );

    (*elements)[0] = piece;
    count = 0;

    if ( EOS != *piece )
    {
	while ( EOS != *piece )
	{
	    if ( '.' == *piece )
	    {
		char *elem = (*elements)[count];

		*piece = EOS;
	
		/* Close off previous element */

		if ( piece == elem )
		{
		    /* Bad request, zero length element. */
		    return 0;
		}

		if ( 0 == count &&
		     *elem == '!' && *(elem+1) == EOS )
		{
		    /* "!" as 1st element means use default prefixs */
		    for( i = 0; i <= context->high_idx; i++ )
		    {
			if( context->def_elem[ i ] == NULL )
			    (*elements)[i] = ERx( "*" );
			else
			    (*elements)[i] = context->def_elem[ i ];
		    }
		    count = context->high_idx + 1;
		}
		else
		{
		    if ( *elem == '$' && *(elem+1) == EOS )
		    {
			/* "$" as an element means use default */
			if( context->def_elem[ count ] == NULL )
			    (*elements)[ count ] = ERx( "*" );
			else
			    (*elements)[ count ] = context->def_elem[ count ];
		    }
		    count++;
		}
		++piece;
		if ( EOS == *piece || PM_MAX_ELEM == count )
		{
		    /* Bad request.  Trailing ".", or too many elements.  */
		    return 0;
		}

		(*elements)[ count ] = piece;
	    }
	    else
	    {
		CMnext(piece);
	    }
	}
	count++;
    }
    return (count);
}

        


/*
**  Name: alloc_rec() -- allocate a data structure list entry.
**
**  Description:
**	Local micro-allocater function which allocates a PM_LIST_REC structure
**	in the specified tagged memory pool.  More memory is allocated as
**	necessary. 
**
**  Inputs:
**	context		Pointer to PM context control block
**
**  Outputs:
**	None
**
**  Returns:
**	NULL		Unable to allocate memory.
**	Otherwise	Pointer to newly created PM_LIST_REC struct.
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
*/
static PM_LIST_REC * 
alloc_rec( PM_CONTEXT *context )
{
	PM_LIST_REC *rec;
	SIZE_TYPE size;
	char *next_block;

	if( context->mem_free < sizeof( PM_LIST_REC ) + sizeof( ALIGN_RESTRICT ) )
	{
		context->mem_free = MALLOC_SIZE; 
		context->mem_block = (char *) MEreqmem( context->memory_tag,
			context->mem_free, FALSE, NULL );
		if( context->mem_block == NULL )
			return( (PM_LIST_REC *) NULL ); 
	}
	rec = (PM_LIST_REC *) ME_ALIGN_MACRO( (PTR) context->mem_block,
		sizeof( PM_LIST_REC * ) );
	next_block = (char *) rec + sizeof( PM_LIST_REC );

	/* set size to aligned structure size */
	size = next_block - context->mem_block;
	context->mem_block += size;
	context->mem_free -= size;

	/* initialize structure */
	rec->name = NULL;
	rec->elem = NULL;
	rec->value = NULL;
	rec->next = NULL;
	rec->owner = NULL;
	return( rec );
}


/*
**  Name: get_rec() -- return a pointer to an existing PM_LIST_REC
**
**  Description:
**	Local function which returns a pointer to a specified node in the
**	search tree corresponding to a PM context id.
**
**  Inputs:
**	context		Pointer to PM context control block
**	idx		idx of lookup element
**	elements	Address of an array of element pointers
**	owner		Pointer to parent PM_LIST_REC
**
**  Outputs:
**	None
**
**  Returns:
**	Pointer to the retrieved PM_LIST_REC.
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
**	06-mar-2003 (devjo01)
**		3rd parameter changed to an array of element pointers
**		from a pointer to one element.
*/
static PM_LIST_REC *
get_rec( PM_CONTEXT *context, i4  idx, char *(*elements)[PM_MAX_ELEM],
	 PM_LIST_REC *owner )
{
	PM_LIST_REC	*rec;
	char		*elem;

	if ( NULL == elements )
	    elem = ERx("*");
	else
	    elem = (*elements)[idx];

	rec = context->htab[ idx ][ hash( elem ) ];

	while( rec != NULL )
	{
		if( STcompare( elem, rec->elem ) == 0 && rec->owner == owner )
			break;
		rec = rec->next;
	}
	return( rec );
}


/*
**  Name: add_rec() -- add a PM_LIST_REC to a local search tree.
**
**  Description:
**	Local function which adds a node to the search tree corresponding
**	to the specified PM context id.
**
**  Inputs:
**	context		Pointer to PM context control block
**	idx		idx of lookup element
**	elements	Address of an array of element pointers. Element
**			strings must either be in static storage, or
**			allocated from the context memory pool.
**	owner		Pointer to parent PM_LIST_REC
**
**  Outputs:
**	None
**
**  Returns:
**	NULL		Unable to allocate memory.
**	Othewise	Pointer to the new PM_LIST_REC.
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	14-dec-92 (tyler)
**		Fixed bad call to alloc_rec() which was incorrectly
**		ignoring context argument and passing a memory tag of 1.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
**	06-mar-2003 (devjo01)
**		3rd parameter changed to an array of element pointers
**		from a pointer to one element.
*/
static PM_LIST_REC *
add_rec( PM_CONTEXT *context, i4  idx, char *(*elements)[PM_MAX_ELEM],
	 PM_LIST_REC *owner )
{
	PM_LIST_REC *rec;
	i4 bucket;
	char		*elem;

	if( (rec = get_rec( context, idx, elements, owner )) != NULL )
		return( rec );
	if( (rec = alloc_rec( context )) == (PM_LIST_REC *) NULL )
		return( (PM_LIST_REC *) NULL );
	
	elem = (*elements)[idx];

	rec->elem = elem;
	rec->owner = owner;
	bucket = hash( elem );
	rec->next = context->htab[ idx ][ bucket ];
	context->htab[ idx ][ bucket ] = rec;
	return( rec );
}


/*
**  Name: ii_bsearch() -- somewhat generic binary search function 
**
**  Description:
**	Local function which finds the insertion point for a record in a
**	sorted array using a binary search.
**
**  Inputs:
**	key		pointer to the key of the record to be matched.	
**	base		pointer to first byte in the array to be searched.
**	n		number of records in the array to be searched.
**	size		number of bytes in each record.
**	compare		pointer to comparison function.
**
**  Outputs:
**	None	
**
**  Returns:
**	Index into the array where the record should be inserted.
**
**  History:
**	9-nov-92 (tyler)
**		Created.
*/
static i4
ii_bsearch( char *key, char *base, u_i4 n, u_i4 size, i4  (* compare) (char *, char *) )
{
	i4 current, offset, direction;
	i4 low = -1, high = n;

	offset = (high - low) / 2; 
	if( offset == 0 )
		return( -1 );
	current = ( offset == 1 ) ? 0 : offset;
	for( ;; )
	{
		direction = compare( key, base + current * size );
		if( direction == 0 )
			return( current );
		direction = ( direction > 0 ) ? 1 : -1;
		if( direction < 0 )
			high = current;
		else
			low = current;
		if( low + 1 == high )
			return( high );
		offset = (high - low) / 2;
			current += direction * offset;
	}
}


/*
**  Name: PM_LIST_REC_compare() -- PM_LIST_REC comparison function
**
**  Description:
**	Local function which compares a string to the name field of a
**	PM_LIST_REC structure.
**
**  Inputs:
**	s		string to be compared PM_LIST_REC name field.
**	rec		pointer to PM_LIST_REC structure.
**
**  Outputs:
**	None	
**
**  Returns:
**	< 0		s is lexically less than rec name field.	
**	> 0		s is lexically greater than rec name field.
**	0		s is lexically equal to rec name field.
**
**  History:
**	9-nov-92 (tyler)
**		Created.
*/
static i4
PM_LIST_REC_compare( char *s, char *rec )
{
	return( STcompare( s, ((PM_LIST_REC *) *((PM_LIST_REC **) rec))->name ) );
}


/*
**  Name: PMmExpandRequest() -- expand a PM request 
**
**  Description:
**	Returns	a pointer to an expanded PM request.  The returned string
**	is stored in a buffer from the specified PM memory pool.
**
**  Inputs:
**	context		Pointer to PM context control block
**	request		Pointer to a PM request.  See the GL PM
**			specification document for a definition of
**			"PM request".
**
**  Outputs:
**	None
**
**  Returns:
**	NULL		Unable to allocate memory.
**	Otherwise	Pointer to the expanded request.
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	14-dec-92 (tyler)
**		Added code to cache one request and the expanded result 
**		since it ends up being called twice (indirectly) with the
**		same request by PMmLoad(). 
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
**	23-jun-93 (tyler)
**		Fixed bug introduced in last revision in which the address
**		of a PM control block was being passed to STtalloc in lieu
**		of a memory tag.
**	16-Jul-2009 (whiro01)
**		Optimized a little bit using regular ptr ++ instead of
**		(expensive) CMnext() calls; using "string_copy" instead of
**		STcopy followed by increment by STlength.
*/
char *
PMmExpandRequest( PM_CONTEXT *context, char *request )
{
	i4 len, i;
	SIZE_TYPE size;
	char *elem[ PM_MAX_ELEM ], *c, *result; 

	/* return matching cached request */
	if( context->request_cache != NULL &&
		STcompare( context->request_cache, request ) == 0
	)
		return( context->request_cache );

	/* cache request */
	context->request_cache = STtalloc( context->memory_tag, request );

	len = PMmNumElem( context, request );
	size = STlength( request ) + 1;

	/* figure out length of full resource request */
	for( i = 0; i < len; i++ )
	{
		elem[ i ] = PMmGetElem( context, i, request );
		if( elem[ i ] == NULL )
			continue;
		if( STcompare( ERx( "$" ), elem[ i ] ) == 0 &&
			context->def_elem[ i ] != NULL
		)
			size += STlength( context->def_elem[ i ] ) - 1;
	}
	if( (c = result = alloc_mem( context, size + 1 )) == NULL )
		return( NULL );

	/* insert default name elements to generate full request */
	for( i = 0; i < len; i++ )
	{
		char *el;
		if( i > 0 )
		{
			*c++ = '.';
			*c = EOS;
		}
		if( ( el = elem[ i ] ) == NULL )
			continue;
		if( el[ 0 ] == '$' && el[ 1 ] == EOS )
		{
			char * def_elem = context->def_elem[ i ];
			if( def_elem != NULL )
			{
				c = string_copy( def_elem, c );
			}
			else
			{
				*c++ = '*';
				*c = EOS;
			}
		}
		else
		{
			c = string_copy( el, c );
		}
	}
	/* cache expanded request */
	context->request_cache = result;

	return( result );
}


/*
**  Name: PMexpandRequest() -- wrapper for PMmExpandRequest() which omits
**	the "context" argument and passes the default context to
**	PMmExpandRequest().
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
*/
char *
PMexpandRequest( char *request )
{
	char *	value;

	if (MUp_semaphore( &context0_sem ))
	    return NULL;
	value = PMmExpandRequest( context0, request );
	MUv_semaphore( &context0_sem );

	return value;
}


/*
**  Name: PMmInsert() -- insert a PM resource
**
**  Description:
**	Add a (name, value) pair to the specified set of in-memory resources.
**
**  Inputs:
**	context		Pointer to PM context control block
**	request		Pointer to request for the resource to be added.
**	value		Pointer to the (string) value of the resource to
**			be added. 
**  Outputs:
**	None
**
**  Returns:
**	OK		The resource was added successfully.
**	PM_NO_INIT	The PM context has not been initialized.	
**	PM_FOUND	The resource already exists.
**	PM_BAD_REQUEST	The specified request is invalid.	
**	PM_NO_MEMORY	Unable to allocate memory.
**
**  History:
**	9-nov-92 (tyler)
**		Created.		
**	14-dec-92 (tyler)
**		Fixed bug which truncated values at the first whitespace
**		character.  Removed code which attempted to strip leading
**		or trailing whitespace from values.
**	18-jan-93 (tyler)
**		Now returns PM_NO_INIT if PM was not initialized.	
**	26-apr-93 (tyler)
**		Cast parameter to MEfree() to PTR to match prototype.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
**	25-jun-93 (tyler)
**		PM_BAD_REQUEST is now returned when the request contains
**		whitespace.
**	26-jul-93 (tyler)
**		Fixed test for PM_NO_INIT condition to avoid dumping
**		core on NULL control block pointer.
**	20-sep-93 (tyler)
**		Cast MEcopy() arguments to PTR.
**      25-Sep-97 (fanra01)
**              Bug 85215.
**              Changed the expansion of the list to use MEcopylng since the
**              size can exceed a u_i2 when the config.dat files is huge.
**	06-mar-2003 (devjo01)
**		Modified to address memory leakage.
*/
STATUS
PMmInsert( PM_CONTEXT *context, char *request, char *value )
{
	i4		 i, j, len, count;
	char  		*elembuf, *name, *elements[ PM_MAX_ELEM ], *c;
	PM_LIST_REC *rec, *owner = NULL;

	if( context == (PM_CONTEXT *) NULL ||
		context->magic_cookie != PM_MAGIC_COOKIE ) 
	{
		return( PM_NO_INIT );
	}

	/* reject requests containing whitespace */
	for( c = request; *c != EOS; CMnext( c ) )
	{
		if( CMwhite( c ) )
			return( PM_BAD_REQUEST );
	}

	/* Copy request string into context memory */
	if( (elembuf = alloc_copy_string( context,
			request, -1 )) == NULL )
	{
	    return( PM_NO_MEMORY );
	}

	/* break request into parts and force its case */
	count = parse_request( context, elembuf, &elements );

	if ( 0 == count )
	{
	    return PM_BAD_REQUEST;
	}

	/* initialize "value" if necessary */
	if( value == NULL )
	    value = ERx(""); 

	/* add necessary record(s) to search tree */
	for( i = 0; i < count; i++ )
	{
		rec = get_rec( context, i, &elements, owner );	
		if( i == (count - 1) )
		{
			/* add record for last element */
			if( rec != NULL && rec->value != NULL )
			{
				return PM_FOUND;
			}
			if( (rec = add_rec( context, i, &elements, owner ))
				== (PM_LIST_REC *) NULL )
			{
				return PM_NO_MEMORY;
			}

			/*
			** Reconstruct full name and copy value
			** into "context" memory.
			*/
			for ( len = j = 0; j < count; j++ )
			{
			    len += STlength( elements[ j ] );
			}

			if( (name = alloc_mem( context,
				len + count + STlength( value ) + 2 )) == NULL )
			{
				return( PM_NO_MEMORY );
			}
			rec->name = name;
			rec->value = name + len + count + 1;

			for ( j = 0; j < count; j++ )
			{
			    if ( j > 0 )
			    {
				*name++ = '.';
			    }
			    name = string_copy( elements[ j ], name );
			}
			*name = EOS;

			STcopy( value, rec->value );
		}	
		else if( rec == NULL )
		{
			/* add record for intermediate element */
			if( (rec = add_rec( context, i, &elements, owner ))
				== (PM_LIST_REC *) NULL )
			{
				return( PM_NO_MEMORY );
			}
		}
		owner = rec;
	}

	/* (re)allocate list if necessary */
	if( (context->num_name * sizeof (PM_LIST_REC*) + 1) 
	  >= context->list_len)
	{
		PM_LIST_REC **tmp;

		tmp = (PM_LIST_REC **) MEreqmem( context->memory_tag,
			context->list_len + (MALLOC_SIZE *
			sizeof( PM_LIST_REC *)), FALSE, NULL );
		if( tmp == NULL )
			return( PM_NO_MEMORY );
		if( context->list != (PM_LIST_REC **) NULL )
		{
			/* free old list */
			MEcopy( (PTR) context->list, context->list_len,
				   (PTR) tmp );
			MEfree( (PTR) context->list );
		}
		context->list_len += MALLOC_SIZE * sizeof( PM_LIST_REC ** );
		context->list = tmp;
	}

	/* insert record into list */
	if( context->num_name == 0 )
		context->list[ context->num_name++ ] = rec;
	else
	{
		/* find insertion point */
		i = ii_bsearch( rec->name, (char *) context->list,
			context->num_name, sizeof( PM_LIST_REC * ),
			PM_LIST_REC_compare );

		/* shift array contents */
		for( j = context->num_name; j > i; j-- )	
			context->list[ j ] = context->list[ j - 1 ];

		/* insert record */
		context->list[ i ] = rec;
		++context->num_name;
	}
	return( OK );
}


/*
**  Name: PMinsert() -- wrapper for PMmInsert() which omits the "context"
** 	argument and passes the default context to PMmInsert().
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
*/
STATUS
PMinsert( char *request, char *value )
{
	STATUS	status;

	status = MUp_semaphore( &context0_sem );
	if (status)
	    return status;
	status = PMmInsert( context0, request, value );
	MUv_semaphore( &context0_sem );

	return status;
}


/*
**  Name: PMmInit() -- initialize a PM context 
**
**  Description:
**	Returns a pointer to an initialized a PM "context" (a.k.a. control
**	block).
**
**  Inputs:
**	None	
**
**  Outputs:
**	context		Pointer to an initialized PM control block.
**
**  Returns:
**	OK		PM context initialized successfully.	
**	PM_NO_MEMORY	Unable to allocate memory.
**
**  History:
**	9-nov-92 (tyler)
**		Created.	
**	14-dec-92 (tyler)
**		Fixed bugs in context re-allocation scheme and stopped 
**		using memory tag 0.  The externally documented GL entry 
**		points now rely on memory tag 1 exclusively.
**	24-may-93 (tyler)
**		Added context argument which allows PM functions to
**		use a scratch context for temporary memory allocation.
**	27-may-93 (tyler)
**		Fixed bug which caused segmentation violation whenever
**		PMmInit() was called without II_CONFIG or II_SYSTEM
**		defined in the environment.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
**		Changed return type from i4  to void.
**	26-jul-93 (tyler)
**		PMmInit() now allocates the control block returns a
**		pointer via an argument to it rather than having the
**		caller pass one in. 
**	02-aug-2001 (somsa01)
**		Added setting of data_loaded boolean in PM_CONTEXT.
**	11-apr-2003 (penga03)
**	    Added CMset_attr call to initialize CM attribute table.
**	17-Sep-2008 (smeke01) b120901
**	    Replaced unbounded STprintf/STcopy calls with STlpolycat/STlcopy.
*/
STATUS
PMmInit( PM_CONTEXT **context )
{
	i4 i, j;
	u_i2 tag;
    char *env = 0;
    char name[MAX_LOC];
    CL_ERR_DESC cl_err;

    NMgtAt(ERx("II_INSTALLATION"), &env);
    if (env && *env)
    {
	STlpolycat(2, MAX_LOC - 1, "II_CHARSET", env, name);
	NMgtAt(name, &env);
    }
    else
    {
	NMgtAt(ERx("II_CHARSET"), &env);
    }

    if(env && *env)
    {
	STlcopy(env, name, MAX_LOC - 1);
	CMset_attr(name, &cl_err);
    }

	if( (tag = MEgettag()) == 0 )
		return( PM_NO_MEMORY );

	*context = (PM_CONTEXT *) MEreqmem( tag, sizeof( PM_CONTEXT ),
		FALSE, NULL );

	if( *context == (PM_CONTEXT *) NULL )
		return( PM_NO_MEMORY );

	(*context)->memory_tag = tag;
	(*context)->force_lower = FALSE;	
	(*context)->mem_block = NULL;
	(*context)->mem_free = 0;
	(*context)->total_mem = 0L;
	(*context)->num_name = 0;
	(*context)->list = (PM_LIST_REC **) NULL;
	(*context)->list_len = 0;
	for( i = 0; i < PM_MAX_ELEM; i++) 
	{
		(*context)->def_elem[ i ] = NULL;
		for( j = 0; j < PM_HASH_SIZE; j++) 
			(*context)->htab[ i ][ j ] = NULL;
	}
	(*context)->high_idx = -1;
	(*context)->request_cache = NULL;
	(*context)->load_restriction = (RE_EXP *) NULL;
	(*context)->magic_cookie = PM_MAGIC_COOKIE;
	(*context)->data_loaded = FALSE;

	return( OK );
}


/*
**  Name: PMinit() -- wrapper for PMmInit().
**
**  Description:
**	Initializes the PM "context" which is accessed by the single-context
**	PM functions.
**
**  Inputs:
**	None	
**
**  Outputs:
**	None
**
**  Returns:
**	OK		The default PM context was initialized successfully.	
**	PM_NO_INIT	The default PM context was already initialized.	
**	PM_NO_MEMORY	Unable to allocate memory.
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
**	26-jul-93 (tyler)
**		Changed return type from void to STATUS.
**	10-aug-1997 (canor01)
**		Initialize scratch semaphore only once.
**      22-nov-00 (loera01) bug 103318
**	        Replaced PM_NO_INIT with PM_DUP_INIT error.
*/
STATUS
PMinit( void )
{
	if( context0 != (PM_CONTEXT *) NULL )
		return( PM_DUP_INIT );

	if( PMmInit( &context0 ) != OK )
        {
		return( PM_NO_MEMORY );
        }
	MUw_semaphore( &context0_sem, "PM context0 sem" );

	if( PMmInit( &scratch ) != OK )
        {
		return( PM_NO_MEMORY );
        }
	MUw_semaphore( &scratch_sem, "PM scratch sem" );

	return( OK );
}


/*
**  Name: PMmSearch() -- search for a PM resource 
**
**  Description:
**	Recursively descends the search tree corresponding to the specified
**	context in order to locate the "best match" for the named resource
**	according the wildcard mechanism described in the PM GL specification.
**
**  Inputs:
**	context		Pointer to PM context control block
**	name		Pointer to name (substring) for the resource
**			to be found.
**	value		Pointer to the (string) value to be added. 
**
**  Outputs:
**	None
**
**  Returns:
**	OK		The requested resource was found.
**	PM_NOT_FOUND	The resource was not found.
**
**  History:
**	9-nov-92 (tyler)
**		Created.		
**	14-dec-92 (tyler)
**		Fixed bug which caused partial resource names to be "found"
**		and treated as complete resources (with NULL values) during
**		lookup. 
**	18-jan-93 (tyler)
**		Fixed bug which caused segmentation violation whenever
**		'name' argument contained a NULL component.  PM_NOT_FOUND
**		is returned instead. 
**	24-may-93 (tyler)
**		Memory which is needed only for the duration of one call
**		to PMmGet() now gets allocated from a temporary memory
**		pool (SCRATCH_MEMORY) and is freed before PMmGet() returns.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
**		Temporary memory control block is now allocated locally.
**	23-jun-93 (tyler)
**		Removed local declaration and initialization of scratch
**		memory context.
**	25-jun-93 (tyler)
**		Removed calls to PMmFree() for scratch memory context
**		since they caused programs which access multiple contexts
**		to crash. 
**	06-mar-2003 (devjo01)
**		Modified to address memory leakage. 'name' parameter
**		replaced by 'elements'.  Code for special case substitutions
**		removed, since elements already points to final
**		string values.
*/
static STATUS
PMmSearch( PM_CONTEXT *context, char *(*elements)[PM_MAX_ELEM], char **value,
	   i4  idx, i4  len, PM_LIST_REC *owner )
{
	PM_LIST_REC *rec;

	/* if last element */
	if( idx == len - 1 )
	{
		/* check for exact or wildcard match and return */
		if( ((rec = get_rec( context, idx, elements, owner )) == NULL &&
		     (rec = get_rec( context, idx, NULL, owner )) == NULL ) ||
		    rec->value == NULL )
		{
			return( PM_NOT_FOUND );
		}
		/* need to set value here */
		*value = rec->value;
		return( OK );
	}

	/*
	** if here, proceed with depth-first search - search the
	** exact-match subtree first.
	*/
	if( (rec = get_rec( context, idx, elements, owner )) != NULL &&
		PMmSearch( context, elements, value, idx + 1, 
		len, rec ) == OK )
	{
		return( OK );
	}

	/* search the wildcard-match subtree last */
	if( (rec = get_rec( context, idx, NULL, owner )) != NULL &&
		PMmSearch( context, elements, value, idx + 1, 
		len, rec ) == OK )
	{
		return( OK );
	}

	/* if here, subtree search failed */
	return( PM_NOT_FOUND );
}


/*
**  Name: PMmGet() -- get a value matching the named resource. 
**
**  Description:
**	Returns the value of the resource which matches the resource request.
**	For an explanation of the rules used to match resources to resource
**	requests, see the PM GL specification.
**
**  Inputs:
**	context		Pointer to PM context control block
**	request		Pointer to the PM resource request.
**	request		Pointer to request for the resource to be added.
**
**  Outputs:
**	value		Pointer used to return the value.
**
**  Returns:
**	OK		A value was returned for a matching resource.
**	PM_NO_INIT	PM was not initialized.	
**	PM_NOT_FOUND	No matching resource was found.	
**	PM_NO_MEMORY	Memory cound not be allocated.
**
**  History:
**	9-nov-92 (tyler)
**		Created.	
**	18-jan-93 (tyler)
**		Now returns PM_NO_INIT if PM was not initialized.	
**	24-may-93 (tyler)
**		Memory which is needed only for the duration of one call
**		to PMmGet() - the only caller of PMmSearch() - now gets
**		allocated from a temporary memory pool (SCRATCH_MEMORY)
**		and is freed before PMmGet() returns.
**	27-may-93 (tyler)
**		Fixed bug introduced by previous change which broke
**		PMget()'s ability to find resources specified with the
**		"!." syntax.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
**		Temporary memory control block is now allocated locally.
**	23-jun-93 (tyler)
**		Removed local declaration of scratch memory context.
**	26-jul-93 (tyler)
**		Fixed test for PM_NO_INIT condition to avoid dumping
**		core on NULL control block pointer.
**      16-dec-94 (lawst01)
**              Test return code from function 'PMmInit' and return
**              if not OK
**	10-aug-1997 (canor01)
**		Move scratch initialization code to PMinit so this
**		code is only executed once.
**	06-mar-2003 (devjo01)
**		Modified to address memory leakage.  parse_request now
**		takes care of "!" expansion.  Memory is now not allocated
**		at all, except for oversized requests.
*/
STATUS
PMmGet( PM_CONTEXT *context, char *request, char **value )
{
	char *elements[PM_MAX_ELEM];
	char  reqbuf[128];
	STATUS status;
	i4 count;

	if( context == (PM_CONTEXT *) NULL ||
		context->magic_cookie != PM_MAGIC_COOKIE ) 
	{
		return( PM_NO_INIT );
	}

	/* make sure scratch area is initialized */
	if ( scratch == NULL )
	{
	    status = PMinit();
	    if( status != OK )
		return ( status );
	}

	if ( STlcopy(request, reqbuf, sizeof(reqbuf) - 1) <
	     (sizeof(reqbuf) - 1) )
	    request = reqbuf;
	else
	{
	    request = STalloc(request);
	    if ( NULL == request )
	    {
		return( PM_NO_MEMORY );
	    }
	}

	/* hold semaphore for entire duration of use of scratch area */
	MUp_semaphore( &scratch_sem );

	count = parse_request( context, request, &elements );
	if ( 0 == count )
	{
	    status = PM_NOT_FOUND;
	}
	else
	{
	    status = PMmSearch( context, &elements, value, 0, count, NULL );
	}

	MUv_semaphore( &scratch_sem );
	if ( request != reqbuf )
	    MEfree(request);

	return( status );
}


/*
** PMget() -- wrapper for PMmGet() which omits the "context" argument by
** 	always passing context0.	
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
**	20-sep-1995 (shero03)
**	    Removed PM misc sem because PMmGet uses it.
*/
STATUS
PMget( char *request, char **value )
{
	return PMmGet( context0, request, value );

}


/*
**  Name: PMmDelete() -- delete a (name, value) pair. 
**
**  Description:
**	Deletes the resource described by the request from the specified
**	PM context. 
**
**  Inputs:
**	context		Pointer to PM context control block
**	request		pointer to the resource request.
**
**  Outputs:
**	None
**
**  Returns:
**	OK		The specified resource was deleted successfully.
**	PM_NO_INIT	PM was not initialized.	
**	PM_NOT_FOUND	The specified resource was not found. 
**
**  History:
**	9-nov-92 (tyler)
**		Created.	
**	14-dec-92 (tyler)
**		Fixed a bug which caused search tree records to be
**		orphaned when resources were deleted which were leading 
**		substrings of other resources. 
**	18-jan-93 (tyler)
**		Now returns PM_NO_INIT if PM was not initialized.	
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
**		A binary search is now used (instead of a linear scan) to
**		locate the resource to be deleted within the sorted array. 
**	26-jul-93 (tyler)
**		Fixed test for PM_NO_INIT condition to avoid dumping
**		core on NULL control block pointer.
**	06-mar-2003 (devjo01)
**		Modified to address memory leakage.
*/
STATUS
PMmDelete( PM_CONTEXT *context, char *request )
{
	i4 count, bucket, i, j;
	char *elem;
	PM_LIST_REC *rec, *prev, *owner;
	bool is_owner = FALSE;
	char *elements[PM_MAX_ELEM];
	char  reqbuf[128];

	if( context == (PM_CONTEXT *) NULL ||
		context->magic_cookie != PM_MAGIC_COOKIE ) 
	{
		return( PM_NO_INIT );
	}

	/* duplicate request and force its case */
	if ( STlcopy(request, reqbuf, sizeof(reqbuf) - 1) <
	     (sizeof(reqbuf) - 1) )
	    request = reqbuf;
	else
	{
	    request = STalloc(request);
	    if ( NULL == request )
	    {
		return( PM_NO_MEMORY );
	    }
	}

	count = parse_request( context, request, &elements );

	/* find element to be deleted */
	for( owner = rec = NULL, i = 0; i < count; i++  )
	{
		elem = elements[ i ];
		bucket = hash( elem );
		rec = context->htab[ i ][ bucket ];
		prev = NULL;
		while( rec != NULL )
		{
			if( STcompare( elem, rec->elem ) == 0 &&
				rec->owner == owner
			)
				break;
			prev = rec;
			rec = rec->next;
		}
		if( rec == NULL )
			break;
		owner = rec;
	}

	/* No longer referencing 'request', free mem if not on stack. */
	if ( request != reqbuf )
	    MEfree(request);

	if( rec == NULL || rec->value == NULL )
	{
		return( PM_NOT_FOUND );
	}

	/* find out if rec is an 'owner' of other record(s) */
	if ( count < PM_MAX_ELEM )
	{
	    for( i = 0; i < PM_HASH_SIZE; i++ ) 
	    {
		PM_LIST_REC *r;

		if( (r = context->htab[ count ][ i ]) == NULL )
			continue;
		while( r != NULL )
		{
			if( r->owner == rec )
			{
				is_owner = TRUE;
				break;
			}
			r = r->next;
		}
		if( is_owner )
			break;
	    }
	}

	if( is_owner )
	{
		/* If so, set the 'value' field of rec to NULL */
		rec->value = NULL;
	}
	else
	{
		/* otherwise, delete the entire record */
		if( prev == NULL )
			context->htab[ count - 1 ][ bucket ] = rec->next;
		else
			prev->next = rec->next;
	}

	i = ii_bsearch( rec->name, (char *) context->list, context->num_name,
		sizeof( PM_LIST_REC * ), PM_LIST_REC_compare );

	/* It's got to be there or something is seriously wrong */
	--context->num_name;
	for( j = i; j < context->num_name; j++ )
		context->list[ j ] = context->list[ j + 1 ];
	return( OK );
}


/*
** PMdelete() -- wrapper for PMmDelete() which omits the "context" argument 
** 	and passes the default context to PMmDelete().
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	14-dec-92 (tyler)
**		Now returns STATUS from PMmDelete().
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
*/
STATUS
PMdelete( char *request )
{
	STATUS	status;
	
	status = MUp_semaphore( &context0_sem );
	if ( status )
	    return status;
	status = PMmDelete( context0, request );
	MUv_semaphore( &context0_sem );

	return status;
}


/*
**  Name: PMmRestrict() -- restrict resources loaded by subsequent PMmLoad().
**
**  Description:
**	This routine modifies the behavior of the subsequent call to
**	PMmLoad() (for the specified context) in order restrict the set
**	of resources loaded to those which match a specified regular
**	expression.  Calling this function is recommended whenever the
**	resource file being loaded may contain a large number of resources
**	relative to the number required for look-up.  In this way,
**	PMmRestrict() can be used to minimize the amount of memory which
**	is allocated by PMmLoad().
**
**  Inputs:
**	context		Optional Pointer to PM context control block
**	regexp		An egrep-style regular expression which, if
**			specified (not NULL), restricts the subsequent
**			load initiated with PMmLoad() to only those
**			resources which are matched by the regular
**			expression.
**
**  Outputs:
**	None
**
**  Returns:
**	OK		Restriction expression registered. 
**	PM_NO_INIT	PM was not initialized.	
**	PM_BAD_REGEXP	Regular expression compilation failed.
**
**  History:
**	24-may-93 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
**	26-jul-93 (tyler)
**		Fixed bug which caused NULL restriction not to be registered.
**		Also fixed test for PM_NO_INIT condition to avoid dumping
**		core on NULL control block pointer.
*/
STATUS
PMmRestrict( PM_CONTEXT *context, char *regexp )
{
	if( context == (PM_CONTEXT *) NULL ||
		context->magic_cookie != PM_MAGIC_COOKIE ) 
	{
		return( PM_NO_INIT );
	}

	if( regexp == NULL ) 
	{
		context->load_restriction = NULL;
		return( OK );
	}

	if( REcompile( regexp, &context->load_restriction,
		context->memory_tag ) != OK )
	{
		return( PM_BAD_REGEXP );
	}

	return( OK );
}


/*
** PMrestrict() -- wrapper for PMmRestrict() which omits the "context" argument 
** 	and passes the default context to PMmRestrict().
**
**  History:
**	24-may-93 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
*/
STATUS
PMrestrict( char *regexp )
{
	STATUS	status;

	status = MUp_semaphore( &context0_sem );
	if ( status )
	    return status;
	status = PMmRestrict( context0, regexp );
	MUv_semaphore( &context0_sem );

	return status;
}


/*
**  Name: PMmLoad() -- load a PM data file. 
**
**  Description:
**	This routine must be called before a program can access entries
**	stored in a PM data file with other PM functions. 
**
**  Inputs:
**	context		Pointer to PM context control block
**	loc		Pointer to the LOCATION of the PM file.  If the
**			pointer is NULL, the default configuration files
**			are loaded ($II_CONFIG/config.dat and
**			$II_CONFIG/protect.dat).
**	err_func	Pointer to a caller-declared function to be called 
**			when an error is encountered loading the file.		
**  Outputs:
**	None
**
**  Returns:
**	OK		File was read successfully.
**	PM_NO_INIT	PMinit() has not been initialized.
**	PM_FILE_BAD	The specified file contained a syntax error.
**	PM_NO_II_SYSTEM	II_SYSTEM is required but not defined.
**	Otherwise	A system specific error status.
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	14-dec-92 (tyler)
**	 	Fixed various bugs in first revision.  Most important: 
**		PMmLoad() now accepts single-quotes (instead of
**		double-quotes) as the delimiter for strings. 
**	18-jan-93 (tyler)
**		Now returns PM_NO_INIT if PM was not initialized.	
**	26-apr-93 (tyler)
**		Replaced inadequate home-brew parser with correct
**		YACC-generated one.
**	27-may-93 (tyler)
**		Added code to check whether the default resource file
**		location is defined (by II_SYSTEM or II_CONFIG) before
**		trying to open it.  Added PM_NO_II_SYSTEM return status.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
**		Improved handling of PM_NO_II_SYSTEM error condition.  Fixed
**		broken default input/output file handling.
**	26-jul-93 (tyler)
**		Assign err_func to yyerr_func before calling yyinit().
**		Remove argument from yyinit() call.  Also fixed test
**		for PM_NO_INIT condition to avoid dumping core on NULL
**		control block pointer.
**	16-dec-93 (tyle)
**		Reset yyfail to FALSE before returning after failure.
**	21-Jul-1998 (hanal04)
**	    If II_CONFIG_LOCAL is set use config.dat from that directory
**	    rather than II_SYSTEM/ingres/files. b91480.
**	29-Jul-1998 (horda03)
**	    Corrected the change for bug 91480. The changes for this bug
**	    accessed the fields of LOCATION directly. The fields of this
**	    variable type is platform specific. This change makes use of
**	    the LOdetail function which obtains the LOCATION components
**	    for the appropriate platform.
**	02-aug-2001 (somsa01)
**	    If we've already loaded data into this context, don't do
**	    anything.
*/
STATUS
PMmLoad( PM_CONTEXT *context, LOCATION *loc, PM_ERR_FUNC *err_func )
{
	STATUS status;
        LOCATION ii_loc;
        char     *ii_config_local = NULL;
        char     loc_buf[MAX_LOC+1];
	i4       config_file_specified = FALSE;

	if( context == (PM_CONTEXT *) NULL ||
		context->magic_cookie != PM_MAGIC_COOKIE ) 
	{
		return( PM_NO_INIT );
	}

	if (context->data_loaded)
	    return (OK);

        /* b91480 - A consequence of the NFS client workaround is */
        /* that we will not load the protect.dat file.            */

        /* The components of LOCATION are platform specific, so use
        ** LOdetail to obtain the required details.
        */
        if (loc)
        {
           char device[MAX_LOC+1];
           char path[MAX_LOC+1];
           char fprefix[MAX_LOC+1];
           char fsuffix[MAX_LOC+1];
           char version[MAX_LOC+1];

           LOdetail( loc, device, path, fprefix, fsuffix, version );

           config_file_specified = ( (STcompare(fprefix, "config") == 0) &&
                                     (STcompare(fsuffix, "dat")    == 0) );
        }

        if ( (loc == NULL) || config_file_specified )
          {
           /*
           ** If II_CONFIG_LOCAL then we should load
           ** II_CONFIG_LOCAL/config.dat else load default.
	   ** ON indicates server generation of NFS clients
           ** so don't use II_CONFIG_LOCAL/config.dat
           */
           NMgtAt( ERx( "II_CONFIG_LOCAL" ), &ii_config_local );
           if (ii_config_local != NULL && *ii_config_local != EOS
	      && STcompare(ii_config_local, "ON") != 0 )
           {
              STlcopy(ii_config_local, loc_buf, sizeof(loc_buf)-1);
              /* prepare local config.dat LOCATION */
              LOfroms( PATH, loc_buf, &ii_loc);
              LOfstfile( ERx( "config.dat" ), &ii_loc);
              loc = &ii_loc;
           }
          }

	if( loc == NULL )
	{
		char *env;

		/* check for PM_NO_II_SYSTEM error condition */
		NMgtAt( SystemLocationVariable, &env );
		if( env == NULL || *env == EOS )
		{
			NMgtAt( ERx( "II_CONFIG" ), &env );
			if( env == NULL || *env == EOS )
                           {
				return( PM_NO_II_SYSTEM ); 
                           }
		}

		/* prepare default input LOCATION */
		NMloc( ADMIN, FILENAME, ERx( "config.dat" ), &yyloc );
		LOcopy( &yyloc, yyloc_buf, &yyloc );
		LOtos( &yyloc, &yyloc_path );

		/* instruct the parser to load "protect.dat" */ 
		yyloc_default = TRUE;
	}
	else
	{
		/* set up yyloc, yylog_buf, and yyloc_path * */
		LOcopy( loc, yyloc_buf, &yyloc );	
		LOtos( &yyloc, &yyloc_path );

/*
                if(nfs_client)
                   yyloc_default = TRUE;
                else
*/
		/* instruct parser not to load protect.dat */ 
		yyloc_default = FALSE;
	}

	yyerr_func = err_func;

	if( (status = yyinit()) != OK )
	{
		return( status );
	}

	yyparse( context );

	context->load_restriction = (RE_EXP *) NULL;

	if( yyfail )
	{
		yyfail = FALSE;
		return( PM_FILE_BAD ); 
	}

	context->data_loaded = TRUE;

	return( OK );
}


/*
** PMload() -- wrapper for PMmLoad() which omits the "context" argument 
** 	and passes the default context to PMmLoad().
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
*/
STATUS
PMload( LOCATION *loc, PM_ERR_FUNC *err_func )
{
	STATUS	status;

	status = MUp_semaphore( &context0_sem );
	if ( status )
	    return status;
	status = PMmLoad( context0, loc, err_func );
	MUv_semaphore( &context0_sem );

	return status;
}


/*
**  Name: PMmExpToRegExp() -- convert PM expression to regular expression 
**
**  Description:
**	Returns a pointer to an internally allocated buffer containing
**	an egrep-style regular expression describing a PM expression.
**	Memory is allocated from the tagged memory pool indicated by
**	the "context" argument.
**
**  Inputs:
**	context		Pointer to PM context control block
**	s		The PM expression to be converted.
**
**  Outputs:
**	None
**
**  Returns:
**	NULL		The specified PM expression is invalid or unable
**			to allocate memory.
**	Otherwise	An egrep-style regular expression.
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	26-apr-93 (tyler)
**		Removed (char *) cast from call to alloc_mem().
**	21-jun-93 (tyler)
**		Added check which avoids crash which occurred on certain
**		invalid PM expressions.  Switched to use of control
**		block for maintaining context.
**	16-dec-93 (tyler)
**		Scans which are specified with "$" will match "*" 
**		when the default component is not found.
**	16-Jul-2009 (whiro01)
**		Optimized using regular character ops instead of STcopy
**		where possible.
**		Added support for "!" at beginning (hopefully matches
**		PMmSearch now).
*/
char *
PMmExpToRegExp( PM_CONTEXT *context, char *s )
{
	char *elem[ PM_MAX_ELEM ], *c;
	i4 i, len;
	SIZE_TYPE size;
	static char *result = NULL;

	/* duplicate name and force its case */
	s = STalloc( s );
	if( context->force_lower )
		string_to_lower( s );
	else
		string_to_upper( s );

	/* determine size required for regular expression */
	len = PMmNumElem( context, s );
	if( len == 0 )
		return( NULL );

	/* expression begins with "^" - count 1 */
	size = 1;
	
	/* convert each "." to "\." - count 2 for each */
	size +=  2 * (len - 1);

	for( i = 0; i < len; i++ )
	{
		elem[ i ] = PMmGetElem( context, i, s );

		if( elem[ i ] == NULL )
			return( NULL );

		/* convert "*" to "\*" - count 2 */
		if( STcompare( ERx( "*" ), elem[ i ] ) == 0 )
			size += 2;

		else if( STcompare( ERx( "$" ), elem[ i ] ) == 0 )
		{
			if( context->def_elem[ i ] != NULL &&
				STcompare( context->def_elem[ i ],
				ERx( "*" ) ) != 0 )
			{
				/* count characters for (host|\*) */
				size += STlength( context->def_elem[ i ] )
					+ 5;
			}
			else
			{
				/* convert "$" or "*" to "\*" - count 2 */
				size += 2;
			}
		}
		else if( STcompare( ERx( "!" ), elem[ i ] ) == 0 )
		{
			/* insert wildcard expression: ".+" */
			size += 2;
		}
		else if( STcompare( ERx( "%" ), elem[ i ] ) == 0 )
		{
			/* insert wildcard segment "[^.]*" - count 5 */
			size += 5;
		}
		else
		{
			/* count original component length */
			size += STlength( elem[ i ] );
		}
	}

	/* expression ends with "$" - count 1 */
	++size;

	/* allocate buffer */
	if( (c = result = alloc_mem( context, size + 1 )) == NULL )
		return( NULL );

	/* construct regular expression */
	*c++ = '^';
	for( i = 0; i < len; i++ )
	{
		if( i > 0 )
		{
			*c++ = '\\';
			*c++ = '.';
		}
		if( STcompare( ERx( "*" ), elem[ i ] ) == 0 )
		{
			*c++ = '\\';
			*c++ = '*';
		}
		else if( STcompare( ERx( "$" ), elem[ i ] ) == 0 )
		{
			if( context->def_elem[ i ] != NULL &&
				STcompare( context->def_elem[ i ],
				ERx( "*" ) ) != 0 )
			{
				STprintf( c, ERx( "(%s|\\*)" ),
					context->def_elem[ i ] );
				c += STlength( context->def_elem[ i ] ) + 5;
			}
			else
			{
				*c++ = '\\';
				*c++ = '*';
			}
		}
		else if( STcompare( ERx( "!" ), elem[ i ] ) == 0 )
		{
			c = string_copy( ".+", c );
		}
		else if( STcompare( ERx( "%" ), elem[ i ] ) == 0 )
		{
			c = string_copy( "[^.]*", c );
		}
		else
		{
			c = string_copy( elem[ i ], c );
		}
	}
	*c++ = '$';
	*c = EOS;

	MEfree( s );

	return( result );
}


/*
** PMexpToRegExp() -- wrapper for PMmExpToRegExp() which omits the
** 	"context" argument and passes the default context to PMmExpToRegExp().
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
*/
char *
PMexpToRegExp( char *pm_exp )
{
	char *	value;

	if (MUp_semaphore( &context0_sem ))
	    return NULL;
	value = PMmExpToRegExp( context0, pm_exp );
	MUv_semaphore( &context0_sem );
	
	return value;
}


/*
**  Name: PMmScan() -- scan an arbitrary set of PM resources.
**
**  Description:
**	With multiple calls, returns all PM resources which match an egrep 
**	style regular expression, sorted in lexical order.
**
**  Inputs:
**	context		Pointer to PM context control block
**	regexp		An egrep-style regular expression.  A new scan is 
**			initiated whenever regexp is not NULL, therefore
**			NULL should be passed to continue a previously
**			initiated scan.
**	state		Pointer to a caller allocated PM_SCAN_REC used 
**			to store state information for the scan.
**	last		A string which can be used to arbitrarily "jump"
**			around in a scan.  If last is not NULL, the next
**			resource returned will be the (lexically) smallest
**			which is (lexically) greater than the string
**			pointed to by "last". 
**	
**  Outputs
**	state		pointer to a caller allocated PM_SCAN_REC used 
**			to store state information for the scan.
**	name		Pointer used to return the name of the returned
**			resource.
**	value		Pointer used to return the value of the returned
**			resource.
**
**  Returns:
**	OK 		A matching resource was returned.
**	PM_NO_INIT	PM was not initialized.	
**	PM_NOT_FOUND	No (additional) matching resource was found.
**	PM_BAD_REGEXP	Regular expression compilation failed.
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	14-dec-92 (tyler)
**		Added checks to ignore "name" and "value" if they are
**		NULL - in which case no strings are returned.
**	18-jan-93 (tyler)
**		Now returns PM_NO_INIT if PM was not initialized.	
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
**	26-jul-93 (tyler)
**		Fixed test for PM_NO_INIT condition to avoid dumping
**		core on NULL control block pointer.
*/
STATUS
PMmScan( PM_CONTEXT *context, char *regexp, PM_SCAN_REC *state, char *last,
	char **name, char **value ) 
{
	i4 idx;

	if( context == (PM_CONTEXT *) NULL ||
		context->magic_cookie != PM_MAGIC_COOKIE ) 
	{
		return( PM_NO_INIT );
	}

	if( regexp != NULL ) 
	{
		if( regexp == NULL || REcompile( regexp, &state->expbuf,
			context->memory_tag ) != OK )
		{
			return( PM_BAD_REGEXP );
		}
		state->last = -1;
	}

	/* if last scan item was specified, ignore state->last */ 
	if( last != NULL )
	{
		idx = ii_bsearch( last, (char *) context->list,
			context->num_name, sizeof( PM_LIST_REC * ),
			PM_LIST_REC_compare );

		/* this is to handles deletion mid-scan */
		if( STcompare( context->list[ idx ]->name,
			last ) != 0
		)
			--idx;
	}
	else
		idx = state->last;

	if( idx == context->num_name )
		return( PM_NOT_FOUND );

	while( ++idx < context->num_name &&
		!REexec( state->expbuf, context->list[ idx ]->name ) );

	state->last = idx;

	if( idx == context->num_name )
		return( PM_NOT_FOUND );

	/* return name if 'name' not NULL */
	if( name != NULL )
		*name = context->list[ idx ]->name;

	/* return value if 'value' not NULL */
	if( value != NULL )
		*value = context->list[ idx ]->value;

	return( OK );
}


/*
** PMscan() -- wrapper for PMmScan() which omits the "context" argument 
** 	and passes the default context to PMmScan().
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
*/
STATUS
PMscan( char *regexp, PM_SCAN_REC *state, char *last, char **name,
	char **value ) 
{
	STATUS	status;

	status = MUp_semaphore( &context0_sem );
	if ( status )
	    return status;
	status = PMmScan( context0, regexp, state, last, name, value );
	MUv_semaphore( &context0_sem );

	return status;
}


/*
**  Name: PMmWrite() -- write PM resources to a data file
**
**  Description:
**	This function outputs the set of PM resources corresponding to
**	the specified context id to a PM data file. 
**
**  Inputs:
**	context		Pointer to PM context control block
**	loc		Pointer to the LOCATION of the file to write.  If
**			the pointer is NULL, output goes to the default
**			configuration resource file ($II_CONFIG/config.dat).
**
**  Outputs:
**	None
**
**  Returns:
**	OK		Data file written successfully.
**	PM_NO_INIT	PM was not initialized.	
**	Otherwise	a system specific error status.
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	14-dec-92 (tyler)
**		Removed code which grouped related resources by inserting
**		a blank line between groups which had the same first three
**		name components since it broke for shorter resources and
**		generally slowed things down.
**	18-jan-93 (tyler)
**		Now returns PM_NO_INIT if PM was not initialized.	
**	26-apr-93 (tyler)
**		Resource values which contain reserved characters - see 
**		yyreserved() - must be delimited by single quotes.  Also,
**		single quotes embedded in delimited strings must be
**		escaped by a backslash.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
**	26-jul-93 (tyler)
**		Fixed performance problem revealed when SI_MAX_TXT_REC
**		became 32000 on certain platforms.  Replaced SI_MAX_TXT_REC
**		with more reasonable PM_MAX_LINE.  Fixed broken output 
**		file handling.  Also fixed test for PM_NO_INIT condition
**		to avoid dumping core on NULL control block pointer.
**      21-Jul-1998 (hanal04)
**         If II_CONFIG_LOCAL is set use config.dat from that directory
**         rather than II_SYSTEM/ingres/files. b91480.
**  29-Jul-1998 (horda03)
**      Corrected the change for bug 91480. The changes for this bug
**      accessed the fields of LOCATION directly. The fields of this
**      variable type is platform specific. This change makes use of
**      the LOdetail function which obtains the LOCATION components
**      for the appropriate platform.
*/
STATUS
PMmWrite( PM_CONTEXT *context, LOCATION *loc )
{
	FILE *fp;
	char text[ PM_MAX_LINE + 1 ], *out, *p, *q;
	i4 i;
	SIZE_TYPE len;
	STATUS status;
        LOCATION ii_loc;
        char   *ii_config_local = NULL;
        char     loc_buf[MAX_LOC+1];
	i4       config_file_specified = FALSE;

	if( context == (PM_CONTEXT *) NULL ||
		context->magic_cookie != PM_MAGIC_COOKIE ) 
	{
		return( PM_NO_INIT );
	}

        /* b91480 */
        /* The components of LOCATION are platform specific, so use
        ** LOdetail to obtain the required details.
        */
        if (loc)
        {
           char device[MAX_LOC+1];
           char path[MAX_LOC+1];
           char fprefix[MAX_LOC+1];
           char fsuffix[MAX_LOC+1];
           char version[MAX_LOC+1];

           LOdetail( loc, device, path, fprefix, fsuffix, version );

           config_file_specified = ( (STcompare(fprefix, "config") == 0) &&
                                     (STcompare(fsuffix, "dat")    == 0) );
        }

        if ( (loc == NULL) || config_file_specified )
          {
           /*
           ** If II_CONFIG_LOCAL then we should load
           ** II_CONFIG_LOCAL/config.dat else load default.
           ** ON indicates server generation of NFS clients
           ** so don't use II_CONFIG_LOCAL/config.dat
           */
           NMgtAt( ERx( "II_CONFIG_LOCAL" ), &ii_config_local );
           if (ii_config_local != NULL && *ii_config_local != EOS
	      && STcompare(ii_config_local, "ON") != 0)
             {
              STlcopy(ii_config_local, loc_buf, sizeof(loc_buf)-1);
              /* prepare local config.dat LOCATION */
              LOfroms( PATH, loc_buf, &ii_loc);
              LOfstfile( ERx( "config.dat" ), &ii_loc);
              loc = &ii_loc;
             }
          }

	if( loc == (LOCATION *) NULL ) 
	{
		NMloc( ADMIN, FILENAME, ERx( "config.dat" ), &yyloc );
		LOcopy( &yyloc, yyloc_buf, &yyloc );
		loc = &yyloc;
	}

	status = SIfopen( loc, ERx( "w" ), SI_TXT, PM_MAX_LINE, &fp );
	if( status != OK ) 
		return( status );

	for( i = 0; i < context->num_name; i++ )
	{
		bool delimit = FALSE;
		char *value = context->list[ i ]->value;

		/*
		** write resource name to output buffer and pad with
		** whitespace.
		*/
		out = string_copy( context->list[ i ]->name, text );
		*out++ = ':';
		len = out - text;
		/* Note: as far as I can tell, 39 is an empirical value */
		/* chosen to fit *most* keys and not to make the output */
		/* lines *too* long for text editors.                   */
		/* For example:
		    ii.*.setup.middle-east:                 'Middle Eastern time zones'
		    ii.*.setup.middle-east.egypt:           'Egyptian Time Zone'
		    ii.*.setup.middle-east.iran:            'Iran Time Zone'
		    ii.*.setup.middle-east.israel:          'Israel Time Zone'
		    ii.*.setup.middle-east.kuwait:          'Kuwait Time Zone'
		    ii.*.setup.middle-east.saudi-arabia:    'Saudi Arabian Time Zone'
		*/
		if ( len < 39 )
		{
			MEfill( (40 - len), ' ', out );
			out = &text[40];
		}
		else
		{
			*out++ = ' ';
		}

		/* determine whether to delimit resource value string */ 
		for( p = value; *p != EOS; CMnext( p ) )
		{
			if( yyreserved( p ) )
			{
				delimit = TRUE;
				break;
			}
		}

		/* write resource value to output buffer */
		if( delimit )
		{
			int cnt;

			*out++ = '\'';
			/* embedded single-quotes must be escaped */
			for( q = value; *q != EOS; q += cnt, out += cnt )
			{
				char special = *q;

				switch ( special )
				{
				    case '\'':
				    case '\\':
					*out++ = '\\';
					*out = special;
					cnt = 1;
					break;
				    case '\n':
				    case '\t':
				    case '\r':
				    case '\f':
				    case '\b':
					*out++ = '\\';
					cnt = 1;
					switch (special)
					{
					    case '\n':
						*out = 'n';
						break;
					    case '\t':
						*out = 't';
						break;
					    case '\r':
						*out = 'r';
						break;
					    case '\f':
						*out = 'f';
						break;
					    case '\b':
						*out = 'b';
						break;
					}
					break;
				    default:
					cnt = CMbytecnt( q );
					CMcpychar( q, out );
					break;
				}
			}
			*out++ = '\'';
			*out = EOS;
		}
		else
			STcopy( value, out );

		/* write output buffer to file */
		SIfprintf( fp, "%s\n", text );
	}

	SIclose( fp );
	return( OK );
}


/*
** PMwrite() -- wrapper for PMmWrite() which omits the "context" argument 
** 	and passes the default context to PMmWrite().
**
**  History:
**	9-nov-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to use of control block for maintaining context.
*/
STATUS
PMwrite( LOCATION *loc )
{
	STATUS	status;

	status = MUp_semaphore( &context0_sem );
	if ( status )
	    return status;
	status = PMmWrite( context0, loc );
	MUv_semaphore( &context0_sem );

	return status;
}
