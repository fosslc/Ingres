/*
** Copyright (c) 1985, 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <tr.h>
#include    <cv.h>
#include    <cm.h>
#include    <lo.h>
#include    <si.h>
#include    <st.h>
#include    <pc.h>
#include    <tm.h>
#include    <me.h>
#include    <clconfig.h>
#include    <meprivate.h>
#include    <cs.h>
#include    <ex.h>
#include    <er.h>

#if !defined(__STDARG_H__)
#include    <varargs.h>
#endif
/*
NO_OPTIM = dg8_us5
*/
#include    <errno.h>

/**
**
**  Name: TR.C - Tracing compatibilty library routines.
**
**  Description:
**      This file contains the trace routines defined by the CL. 
**      These routines provide tracing primitives used to facilitate 
**      debugging and testing.
**
**          TRdisplay - Write trace output.
**	    TRflush - Flush buffers to disk.
**          TRgettrace - Test trace flag
**          TRmaketrace - Setup a trace vector.
**          TRrequest - Read trace input.
**          TRset_file - Set the trace input/output.
**          TRsettrace - Set trace flag
**
** History:
**      Revision 1.5  88/07/01  10:53:08  mikem
**      remove newline for TM string.
**      
**      Revision 1.4  88/02/16  14:31:21  anton
**      user specifiable prompt for TRrequest
**      
**      Revision 1.3  87/12/31  13:16:18  anton
**      new tr semantics - closer to VMS
**      
**      Revision 1.2  87/11/30  14:39:57  mikem
**      changed to use 6.0 version of STindex
**      
**      Revision 1.1  87/11/10  16:04:41  mikem
**      Initial revision
**      
**      Revision 1.3  87/07/27  10:42:24  mikem
**      Updated to conform to jupiter coding standards.
**      
**      30-sep-1985 (derek)    
**          Created new for 5.0.
**	06-aug-1986 (Derek)
**	    Fix problem with date format.
**      08-aug-1986 (seputis)
**          Added floating point support
**	21-aug-1986 (Derek)
**	    Added indirect fetch of width and precision.
**	16-jul-1987 (mmm)
**	    ported to UNIX - use ifdef's for now to facilitate diffs with vms
**	    system which still seems to be changing.
**	17-Oct-1988 (daveb)
**	    readonly isn't legal.  Make the initialization readable.
**	18-oct-1988 (seputis)
**	    fix floating point problem, pass fmt char correctly
**	30-Nov-1988 (russ)
**	    Modified TRdisplay, TRformat, and the underlying do_format
**	    function to use varargs.  Also, TRdisplay now formats the
**	    data only if it is actually going to be printed.
**	04-Apr-89 (GordonW)
**	    added MECOPY_{VAR/CONST}_MACRO call
**	13-Apr-1989 (fredv)
**	    Added ming hint not to optimize this file for rtp_us5.
**	26-Apr-89 (GordonW)
**	    TRdisplay returns a STATUS
**	02-May-89 (GordonW)
**	    remove mecopy.h include because it is already included in me.h
**      16-Apr-90 (Kimman)
**          Using MEcopy for ds3_ulx because compiler has bug and gets
**          confused with to many defines in SET_ARG.
**	08-Aug-90 (GordonW)
**	    TERMCL changes, change # 20511.
**	01-Nov-90 (anton)
**	    Reentrancy protection added
**	14-dec-90 (mikem) restore lost history comments:
**    	    03-Aug-1990 (mikem)
**          Mainline currently passes "NULL" pointer as a "fcn" when it
**          doesn't want any function executed.  Changed TRformat to check
**          for a NULL function pointer before using it.
**	11-mar-91 (rudyw)
**	    Comment out text following #endif.
**	25-jun-1991 (bryanp)
**	    Don't suppress zero-length lines.
**	27-may-1992 (jnash)
**	    Add ability to append to end of file in TRset_file via
**	    TR_F_APPEND.
**	22-june-1992 (jnash)
**	    TR_NOOVERRIDE_MASK added to TRset_file flag param.
**	14-Jul-1993 (daveb)
**	    Add '%p' PTR argument formatting using CVptrax().
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      07-sep-93 (smc
**          Made address cast portable SCALARP.
**	13-sep-93 (dianeh)
**	    Removed NO_OPTIM setting for obsolete PC/RT port (rtp_us5).
**      20-nov-1994 (andyw)
**          Added NO_OPTIM due to Solaris 2.4/Compiler 3.00 errors
**	20-apr-95 (emmag)
**	    Desktop porting changes.
**      15-may-1995 (thoro01)
**          Added NO_OPTIM hp8_us5 to file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch PHSS_5504 for c89
**      21-Jun-1995 (walro03/mosjo01)
**          Added NO_OPTIM dg8_us5 to file.
**          Symptom: loop in csphil/cscleanup.
**          Works in DG/UX 3.00 MU05; fails in DG/UX 3.10 MU02.
**	19-may-95 (canor01)
**	    added <errno.h>
**      1-june-1995 (hanch04)
**          Removed NO_OPTIM su4_us5, installed sun patch
**      09-jun-1995 (canor01)
**          semaphore protect static context structure for MCT server
**	03-jun-1996 (canor01)
**	    Remove VMS-specific code.  Replace generic semaphore protection
**	    with thread-local storage for use with operating-system threads.
**	11-Jul-1996 (jenjo02)
**	    Fixed printing of '%'. '%%' produced nothing.
**	25-oct-1996 (canor01)
**	    Remove some MEtls return values that are not being used.
**      22-nov-1996 (canor01)
**          Changed names of CL-specific thread-local storage functions
**          from "MEtls..." to "ME_tls..." to avoid conflict with new
**          functions in GL.  Included <meprivate.h> for function prototypes.
**      07-mar-1997 (canor01)
**          Allow functions to be called from programs not linked with
**          threading libraries.
**      11-apr-1997 (kosma01)
**	    Changed CS_SID TRcontext_sid to CS_THREAD_ID TRcontext_tid, since
**	    it holds a thread_id. Changed thread_id comparisons to use
**	    CS_thread_id_equal, and changed thread_id assignments to use
**	    CS_thread_id_assign.
**	02-sep-1997 (muhpa01)
**	    Changed initialization of TRcontext_tid from (CS_THREAD_ID)0 to
**	    use ZERO_FILL - cannot cast if CS_THREAD_ID is not scalar, as on
**	    the Draft 4 Posix implementation on HP-UX.
**	22-sep-1997 (canor01)
**	    Clean up compile warnings about va_start.
**      26-Oct-1998 (kosma01)
**          Provide stdarg prototypes for sgi_us5, because
**          sgi_us5 has a problem with floating point values
**          on vararg functions that have no prototype.
** 	08-apr-1999 (wonst02)
** 	    Modify do_format() to enable formatting hex fields of 1 or 2 bytes.
**	19-apr-1999 (somsa01)
**	    Removed undefines of va_start and va_end for NT.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	25-jan-2000 (somsa01)
**	    In do_format(), for the '%t' option, make sure we do not exceed
**	    the limit of the buffer used to print out.
**      12-dec-2001 (loera01) Bug 106636
**          Remove references to base_address and precision when repeating 
**          group format is encountered.  They are not required, and cause
**          compilation problems on some platforms.
**     13-dec-2001 (loera01) Bug 106623
**          For do_format(), use MEcopy for assigning *va_list to va_list, as
**          some product build machines disallow a direct assignment.
**	15-may-2002 (somsa01)
**	    In do_format(), corrected argument passing to isprint().
**	04-Feb-2002 (rigka01) bug #106435
**	    Add type 'u' as a valid TRdisplay datatype.  Same as type 'd'
**	    except data is unsigned instead of signed. 
**	26-May-2002 (hanje04)
**	    Replaced longnats brought in by X-Integ with i4's
**	05-sep-2002 (devjo01)
**	    Add TRwrite, as a default output function which may be
**	    given to TRformat.
**	07-Sep-2002 (hanch04)
**	    Replace l_tmp with i4_tmp to be consistent.  longs should not
**	    be used in place of i4,  long on 64 bit is really an i8.
**      30-Oct-2002 (hanal04) Bug 109026 INGSRV 1990
**          Ensure the TRcontext_mutex is released before exiting these
**          routines in order to prevent indefinite mutex waits.
**       4-Dec-2002 (hanal04) Bug 109222 INGSRV 2020
**          Prevent TRcontext_mutex related hangs by preventing
**          threads from seeing stale TRcontet_tid information.
**	06-Jan-2003 (hanje04)
**	    Added LNX to product defns for p as va_list becuase gcc 3.x
**	    complains about default defn
**	09-jun-2003 (devjo01)
**	    Add support for TR_F_SAFEOPEN. b110302.
**	23-Feb-2004 (hanje04)
**	    Replace a64_lnx with LNX to stop gcc 3.0 complaints on all Linuxes.
**	28-Nov-2003 (fanra01)
**	    Add forward reference for TRwrite.
**	02-Apr-2004 (somsa01)
**	    Corrected forward reference to TRwrite().
**	03-Jun-2004 (somsa01)
**	    Removed product defines, as the correct solution to the original
**	    problem was a Linux issue and not a product issue.
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	16-Nov-2004 (gupsh01)
**	    Fixed TRdisplay() to print double byte characters
**	    correctly. 
**	10-Dec-2004 (schka24)
**	    Typo in prev change caused suppressed tabs, junk at EOL.  Fix.
**	22-Dec-2004 (bonro01)
**	    Solaris a64_sol complained about initializing va_list variables.
**	21-Aug-2006 (kschendel) b122041
**	    Implement %lu, %ld, %lx for i8 parameters.  (Only a couple years
**	    after Jon first asked about it...!)
**	    Also clean up comments, sloppy formatting.
**	26-Jun-2007 (jonj)
**	    Add TRformat_to_buffer().
**	17-Sep-2009 (hanje04)
**	    Add OSX to defns for p because of gcc 3.xx issues with va_list
**	    defns.
**       9-Jun-2010 (hanal04) Bug 123886
**          Add %^ format specifier to include pid, tid information.
*/

/*}
** Name: TRCONTEXT - Trace context
**
** Description:
**      This structure contains information about trace files.  This context 
**      is allocated by the caller but interpreted by these routines.
**
** History:
**     30-sep-1985 (derek)
**          Created new for 5.0.
*/

typedef struct _TRCONTEXT
{
    int		    context_id;		/*  Constant to verify argument. */
#define			CONTEXT_ID	'TRIO'
    char	    stdioflag;		/* flags indicating stdio use in the following array */
    FILE	    *fp_array[3];
#define                 FP_FILE		0
#define			FP_TERMINAL	1
#define			FP_INPUT	2
}   TRCONTEXT;


/*
**  Definition of static variables and forward static functions.
*/

static	TRCONTEXT	    tr_default;

# ifdef OS_THREADS_USED
static CS_SYNCH		TRcontext_mutex;
static bool		TRcontext_init = FALSE;
static i4		TRcontext_level = 0;
static CS_THREAD_ID	TRcontext_tid ZERO_FILL;
static ME_TLS_KEY	TRtracevectkey = 0;
static ME_TLS_KEY	TRbufferkey = 0;
# endif /* OS_THREADS_USED */
char	    tr_buffer[ER_MAX_LEN];/* TRdisplay output buffer. */
i4	    tr_offset;		/* Current length of buffer. */

static	i2	    *trace_vector;	/* Current trace vector. */	    
static	STATUS	    write_line();
static  VOID	    do_format();

struct argmng {
    char    loc_buf[1024];
    PTR	    dataptr;		/* area to hold actual value of args */
    PTR	    argptr[256];	/* pointers to args within dataptr */
    i4	    next_arg;		/* next available slot in argptr */
};
/*
** Add forward reference
*/
STATUS TRwrite(PTR, i4, char *);

STATUS tr_handler(EX_ARGS *exargs);


/*{
** Name: TRdisplay	- Write trace output.
**
** Description:
**	Characters from the format string are passed to the trace
**	terminal and/or the trace file.  If a format specifier is
**	recognized parameters are read from the argument list and
**	are displayed in the form described by the format specifier.
**	All output is formatted into 132 characters lines and long
**	lines are broken into multiple lines.
**
**	A format specifier is of the following form:
**
**	% [~] [ width ] [. [ size ] ] directive
**
**	Where width and size are decimal number or '#' or '*'.  The '#'
**	means get the value from the parameter list.  The '*' means
**	get the value indirectly from the parameter list, or get the
**	value from a offset within a structure or array if currently in
**	a structure or array.  The optional leading ~ is only used with
**	the t directive, and trims any trailing blanks (and shrinks the
**	field width accordingly).
**
**	A formatting directive is one of the following:
**
**	b  Print base address of structure.  Width means how wide a field
**	   to use in displaying the string, if none is given as many
**	   characters as needed to display the pointer are used.  Size is not
**	   used.  The parameter is a pointer value, and it's printed in hex.
**
**	c  Format a text character.  If the character is not printable a 
**	   '.' is substituted.  Width and size are ignored.  One parameter
**	   is used, this is the character to format.  If in a structure,
**	   the parameter is used as an offset from the base address of the
**	   structure.
**
**	d  Format an integer in decimal.  Width means how wide of a field to
**	ld use in displaying the number, if none is given as many characters
**	   as needed to display the number are used.  Size means how large
**	   an integer to get when displaying values in structures or arrays.
**	   One parameter is used, this is the number to format.  If this is in
**	   a structure then the parameter is used as an offset from the base
**	   address of the structure.
**	   The number-to-format is an i4 for d, i8 for ld.
**
**	e  The double arg is converted in the style [-]d.dddeSdd, where
**	   there is one digit before the decimal point and the number
**	   of digits after it is equal to the precision;  when the 
**	   precision is missing, a default of 3 is used; if the precision
**	   is zero, no decimal point appears.  The exponent always contains
**	   at least two digits.
**
**	f  The double arg is converted to decimal notation in the style
**	   [-]ddd.ddd, where the number of digits after the decimal
**	   point is equal to the precision specification.  If the precision
**	   is missing 3 decimal digits are output; if the precision is
**	   explicitly 0, no decimal point appears.
**
**	g  The double arg is converted to decimal notation in the
**	   style [-]ddd.ddd, where the number of digits after the decimal
**	   point is equal to the precision specification.  If the precision
**	   is missing, 3 decimal digits are output; if the precision is
**	   explicitly 0, no decimal point appears.  If the number
**	   is a display of zero, the number is displayed in 'e' format.
**
**	n  Same as g for formatting a double, except that g aligns decimal
**	   points in some manner (???) that n doesn't.
**
**	s  Format a C character string.  Width means how wide a field to use
**	   in displaying the string,  if none is given as many characters as
**	   needed to display the string are used.  If size is given it is
**	   assumed to be the length of the string, otherwise a null character
**	   terminates a string.  One parameter is used, this is a pointer to
**	   the string to format.  If this is in a structure then the parameter
**	   is used as an offset from the base address of the structure to get
**	   the pointer.
**
**	t  Format a text string.  Width means how wide a field to use in
**	   displaying the string, if none is given the actual width is
**	   used.  Size is ignored.  The first parameter is the length of
**	   the string, the second parameter is a pointer to the
**	   string.  There is no special meaning when inside a structure.
**	   The ~ flag character can be used (%~t) and requests trailing
**	   blank trimming.
**
**	u  Format an integer in decimal. Width means how wide of a field to
**	lu use in displaying the number,if none is given as many characters
**	   as needed to display the number are used.  Size means how large
**	   an integer to get when displaying values in structures or arrays
**	   One parameter is used,this is the number to format.If this is in
**	   a structure then the parameter is used as an offset from the 
**	   base address of the structure. Same as type d except this is for
**	   unsigned data.
**	   The number parameter is u_i4 for %u and u_i8 for %lu.
**
**	v  Format a bit field into words. Width means how wide a field to use
**	   in displaying the string, if none is given the actual word display
**	   width is used.  Size is used inside a structure to determine the
**	   size of the integer to fetch.  The first parameter is a character
**	   string containing words separated by ','.  The words from left
**	   to right correspond to the bits from least significant to most
**	   significant.  If the bit is on,  the word is copied to the output.
**	   The second parameter is the value to use, or the offset from the
**	   base address of the structure.  If bits are found to be set
**	   beyond the supplied list, the whole value will be displayed as a
**	   hex number in braces, i.e. {0x10000038}
**
**	w  Format an integer into a word.  Width means how wide a field to use
**	   in displaying the string, if none is given the actual word display
**	   width is used.  Size is used inside a structure to determine the
**	   size of the integer to fetch.  The first parameter is a character
**	   string containing words separated by ','.  The words from left to
**	   right correspond to the numbers 0 through whatever.  The second
**	   parameter is the value to use, or the offset from the base address
**	   of the structure.  If a value is passed that is outside the
**	   supplied list, the rogue value will be formatted in decimal
**	   surrounded by braces.
**
**	x  Format an integer in hexidecimal.  Width means how wide of a field
**	lx to use in displaying the number,  if none is given, the actual
**	   display width of the number is used.  Size means how large of an 
**	   integer to get when displaying values in structures or arrays.
**	   One parameter is used, this is the number to format.  If in
**	   a structure, the parameter is used as an offset from the base
**	   address of the structure.
**	   The number is an i4 for %x, an i8 for %lx.
**
**	@  Format the current time and date.  Width means how wide of field
**	   to use in displaying the time and date.  This takes no parameters.
**
**	(  Start repeating group.  Width means how many times to repeat the 
**	   format items following this until the %).  A repeating group can
**	   not be used inside an array or structure.
**
**	)  End repeating group.  Terminates a repeated format group.
**
**	<  Backup parameter.  If width is set this backs up the corresponding
**	   number of parameters otherwise it backs up one parameter.
**
**	>  Skip parameter.  If width is set this skips the corresponding
**	   number of parameters, otherwise it skips one parameter.
**
**	*? Repeat character. Width determines how may times the characters
**	   following the * is copied to the output.
**
**	%  Print a '%'.
**
**      ^  Print the Pid/Tid. Tid not printed if OS_THREADS_USED not defined.
**
**	[  Start array of pointers.  Width is used to tell how many pointers
**	   to follow.  The first parameter is the address of the array of
**	   pointers. If this is in a structure,  this is an offset to the
**	   pointer to the array of pointers.  The format and the parameter are
**	   rescanned with the next pointer until all pointers are exhausted.
**	   This can be nested within an array, structure or repeat group.
**
**	]  End array of pointers.  Delimits an array of pointers.
**
**	{  Start array of structures.  Width is used to tell how many
**	   structures to display. Size is the size of the structure item so
**	   that it can increment to the next item.  The first parameter is the
**	   address of the structure.  If this is in a structure this is an
**	   offset to the structure to be displayed.  The format and the
**	   parameters are rescanned with the next structure address until all
**	   structures have been displayed.  This can be nested within an
**	   array, structure or repeat group.
**
**	}  End array of structures.  Delimits an array of structures.
**
** Inputs:
**      format				String describing the formatting actions
**					to perform.
**      parameter			Variable number of arguments to
**					substitute into the output string.
** Outputs:
**	Returns:
**	    OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-sep-1985 (derek)
**          Created new for 5.0.
**	06-aug-1986 (Derek)
**	    Fix problem with date format that caused garbage to be
**	    printed.
**      08-aug-1986 (seputis)
**          Added floating point support
**      29-sep-1988 (mikem)
**	    Changed length of "format_item" to 128 bytes from 64 bytes.  When
**	    using the %v format from dmfrcp we overflowed this buffer resulting
**	    in an access violation in do_format().  This should really be 
**	    changed to check for lengths, and return errors when overflow 
**	    happens.  Also the unix version of this file needs to be changed
**	    to use a portable "varargs" method of dealing with variable number
**	    of arguments to TRdisplay().
**	17-oct-96 (mcgem01)
**	    Changed the length of the "format_item" buffer from 128 to 256
**	    to facilitate RAAT trace messages.
**	04-Apr-1999 (bonro01)
**	    Change the length of the "format_item" buffer from 256 to 512.
**	    Some messages are now over 256 bytes long.
**      05-Aug-1999 (bonro01)
**	    Change the length of the "format_item" buffer from 512 to 2048.
**          I found a message over 720 bytes long E_DM9854_ACP_JNL_ACCESS_EXIT
**          Modified %t text processing code to replace non-printable
**          characters with spaces.
**      30-Oct-20002 (hanal04) Bug 109026 INGSRV 1990
**          Added exception handler to ensure we release the TRcontext_mutex
**          if an exception occurs.
**       4-Dec-2002 (hanal04) Bug 109222 INGSRV 2020
**          Initialise TRmutex_tid before releasing the TRcontext_mutex
**          to ensure we do not see stale owner information.
**	24-Jun-2003 (devjo01) bug 110458 INGSRV2391.
**	    Processing "%[" spec would SEGV since only half the array address
**	    was getting passed.
**	16-Nov-2004 (gupsh01)
**	    Fixed TRdisplay ~t format to print the double byte strings 
**	    correctly.
**	16-Jun-2008 (kiria01) b120533
**	    Fixup %w & %v so that range errors don't just end in an empty
**	    string.
*/
#if defined(__STDARG_H__)
STATUS
TRdisplay(char *f, ... )
#else
STATUS
TRdisplay(va_alist)
va_dcl
#endif  /* __STDARG_H__ */
{
    TRCONTEXT         *context = &tr_default;
    EX_CONTEXT        ex_context;

#if !defined(__STDARG_H__)
    char	    *f;
#endif
    va_list	    p;
    i4	    cur_arg = 0;
    struct argmng   marg;
    STATUS	    status;
    char	    *buffer = NULL;
    i4	    *offset;
# ifdef OS_THREADS_USED
    char	    *tlsp = NULL;

    if ( TRbufferkey == 0 )
    {
        ME_tls_createkey( &TRbufferkey, &status );
        ME_tls_set( TRbufferkey, NULL, &status );
    }
    if ( TRbufferkey == 0 )
    {
        /* not linked with threading library */
        TRbufferkey = -1;
    }
    if ( TRbufferkey == -1 )
    {
	offset = &tr_offset;
	buffer = tr_buffer;
    }
    else
    {
        ME_tls_get( TRbufferkey, (PTR *)&tlsp, &status );
        if ( tlsp == NULL )
        {
	    /*
	    ** allocate a thread-local buffer for both:
	    **	char    tr_buffer[131];
	    ** and  i4 tr_offset;
	    */
            tlsp = MEreqmem(0, sizeof(tr_buffer) + sizeof(tr_offset), 
			    TRUE, NULL);
            ME_tls_set( TRbufferkey, (PTR)tlsp, &status );
        }
        offset = (i4 *) tlsp;
        buffer = tlsp + sizeof(tr_offset);
    }
# else /* OS_THREADS_USED */
    offset = &tr_offset;
    buffer = tr_buffer;
# endif /* OS_THREADS_USED */

#if defined(__STDARG_H__)
    va_start(p, f);
#else
    va_start(p);

    f = va_arg(p,char *);
#endif

# ifdef OS_THREADS_USED
    if ( TRcontext_init == FALSE )
    {
	TRcontext_init = TRUE;
	CS_synch_init( &TRcontext_mutex );
    }
    if ( TRcontext_level == 0 || !CS_thread_id_equal(TRcontext_tid, CS_get_thread_id()) )
    {
        CS_synch_lock( &TRcontext_mutex );
        if (EXdeclare(tr_handler, &ex_context))
	{
            if(TRcontext_level != 0 && 
		     CS_thread_id_equal(TRcontext_tid, CS_get_thread_id()) )
            {
                TRcontext_level = 0;
		CS_thread_id_assign(TRcontext_tid, 0);
                CS_synch_unlock( &TRcontext_mutex );
		EXdelete();
                return(OK);
	    }
	}
    }
    CS_thread_id_assign(TRcontext_tid, CS_get_thread_id());
    TRcontext_level++;
# endif /* OS_THREADS_USED */

    if ( context->fp_array[FP_TERMINAL] != NULL || 
	 context->fp_array[FP_FILE] != NULL )
    {
	do_format( TRwrite, (i4 *) 0, &f, &p, &cur_arg, buffer, 
		   sizeof(tr_buffer), offset, 0, 0, &marg );

    }
    va_end(p);
# ifdef OS_THREADS_USED
    TRcontext_level--;
    if ( TRcontext_level == 0 )
    {
	CS_thread_id_assign(TRcontext_tid, 0);
        CS_synch_unlock( &TRcontext_mutex );
	EXdelete();
    }
# endif /* OS_THREADS_USED */
    return(OK);
}

/*{
** Name: TRformat	- Format text into memory.
**
** Description:
**      This function will format text into a memory buffer and then call a 
**	caller provided function to dispose of the buffer contents.
**
**	The use function is called like:
**	    (*fcn)(arg, length, buffer)
**
** Inputs:
**      fcn                             Function to call back.
**      arg                             Argument to pass to fcn.
**      buffer                          Pointer to buffer.
**      l_buffer                        Length of buffer.
**      format                          Format string.
**      parameter                       Parameter list.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      02-jun-1987 (Derek)
**          Created.
**	14-dec-90 (mikem) restore lost history comments:
**    	    03-Aug-1990 (mikem)
**          Mainline currently passes "NULL" pointer as a "fcn" when it
**          doesn't want any function executed.  Changed TRformat to check
**          for a NULL function pointer before using it.
**	04-Feb-2002 (rigka01) bug #106435
**	    Add type 'u' as a valid TRdisplay datatype.  Same as type 'd'
**	    except data is unsigned instead of signed.  Change is in do_format.
**	06-Nov-2006 (kiria01) b117042
**	    Conform CM macro calls to relaxed bool form
*/
VOID
#if defined(__STDARG_H__)
TRformat(i4 (*fcn)(PTR, i4, char *), ...)
# else
TRformat(va_alist)
va_dcl
# endif
{
#if !defined(__STDARG_H__)
    i4		    (*fcn)();
# endif
    PTR		    arg;
    char	    *buffer;
    i4		    l_buffer;
    char	    *f;
    va_list	    p;
    i4		    offset = 0;
    i4		    cur_arg = 0;
    struct argmng   marg;

#if defined(__STDARG_H__)
    va_start(p, fcn);
# else
    va_start(p);
    fcn =	(i4 (*)()) va_arg(p,PTR);
# endif

    arg =	va_arg(p,PTR);
    buffer =	va_arg(p,char *);
    l_buffer =	va_arg(p,i4);
    f =		va_arg(p,char *);

    do_format(fcn, arg, &f, &p, &cur_arg, buffer, l_buffer, &offset, 0, 0,
		&marg);
    if (fcn && offset)
	(*fcn)(arg, offset, buffer);

    va_end(p);
}

/*{
** Name: TRformat_to_buffer	- Format text into memory.
**
** Description:
**      This function will format text into a memory buffer and then call a 
**	caller provided function to dispose of the buffer contents.
**	
**	It differs from TRformat() only in that the parameter list is passed
**	as a va_list.
**
**	The use function is called like:
**	    (*fcn)(arg, length, buffer)
**
** Inputs:
**      fcn                             Function to call back.
**      arg                             Argument to pass to fcn.
**      buffer                          Pointer to buffer.
**      l_buffer                        Length of buffer.
**      fmt                          	Format string.
**      ap                       	Parameter list.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-Jun-2007 (jonj)
**	    Created for circular cached deadlock tracing by LK
**	    for VMS clusters.
*/
VOID
TRformat_to_buffer(
VOID		    (*fcn)(),
i4		    *arg,
char		    *buffer,
i4		    l_buffer,
char		    *fmt,
va_list		    ap )
{
    i4		    offset = 0;
    i4		    cur_arg = 0;
    struct argmng   marg;

    do_format(fcn, arg, &fmt, &ap, &cur_arg, buffer, l_buffer, &offset, 0, 0,
		&marg);

    if (fcn && offset)
	(*fcn)(arg, offset, buffer);
}

static VOID
do_format(fcn, arg, format, parameter, pcur_arg, buffer, buffer_length, buffer_offset, base, type, marg)
i4		    (*fcn)();
PTR		    arg;
char		    **format;
va_list		    *parameter;
i4		    *pcur_arg;
char		    *buffer;
i4		    buffer_length;
i4		    *buffer_offset;
PTR		    base;
i4		    type;
#define			IN_REPEAT   1
#define			IN_ARRAY    2
#define			IN_STRUCT   3
struct argmng	    *marg;
{
    char	    *f = *format;
#if defined(EDBC) || defined(LNX) || defined(a64_sol) || defined(OSX)
    va_list	    p;
# else
    va_list	    p = *parameter;
# endif
    i4	    cur_arg = *pcur_arg;
    char	    format_item[2048];
    i4	    offset = *buffer_offset;
    i4	    length = buffer_length;
    i4	    format_length;
    char	    *format_buffer;
    char    fmt[20];
    PTR		    base_address;
    i4	    width;
    bool	    noprecision;	    /* TRUE if no precision specified */
    bool	long_flag;		/* TRUE for i8 forms */
    i4	    precision;
    char	    directive;
    i4	    trim;
    i4	    move_amount;
    i4	    save_cur_arg;
    i4	    save_offset;
    char	    *save_f;
    i4	    save_length;
    i4	    number;
    char	    *string;

    i4	    	    i4_tmp;
    u_i4            ui4_tmp;
    i8		i8_tmp;
    i4	   	    *i4p_tmp;
    f8		    f_tmp;
    PTR		    p_tmp;
    char 	    *cptr;
	char 	    *fptr;
    i4 		    bcnt = 0;
    
#if defined(LNX) || defined(a64_sol) || defined(OSX)
        MEcopy( parameter, sizeof(*parameter), &p );
#endif
	/*
	** If type is 0, we are being called from TR{format|display}, not
	** recursively.  So, we need to set marg->next_arg to 0 to begin a new
	** argument list.
	*/
	if (type == 0)
	{
		marg->next_arg = 0;
		marg->dataptr  = marg->loc_buf;
	}

	/*
	** Macro to set a variable to the value of an arg.
	** If the arg has already been seen, get it from the
	** data space pointed to by marg->dataptr via cur_arg and
	** the marg->argptr array.  Otherwise, use varargs to get the
	** value, stash the data in the data space pointed to by
	** marg->dataptr, set the next pointer in marg->argptr to the address
	** of this space, and then increment the counters cur_arg
	** and marg->next_arg.
	*/

# ifdef ds3_ulx
# undef MECOPY_CONST_MACRO
# define MECOPY_CONST_MACRO MEcopy
# endif

# define SET_ARG(VAR,VALIST,TYPE) \
	if (cur_arg < marg->next_arg) \
	{ \
		VAR = *((TYPE *)marg->argptr[cur_arg++]); \
	} \
	else \
	{ \
		marg->dataptr = ME_ALIGN_MACRO(marg->dataptr,sizeof(TYPE)); \
		VAR = va_arg(VALIST,TYPE); \
		MECOPY_CONST_MACRO((PTR)&VAR,sizeof(TYPE),marg->dataptr); \
		marg->argptr[marg->next_arg++] = marg->dataptr; \
		marg->dataptr += sizeof(TYPE); \
		cur_arg++; \
	}

    for (;;)
    {
	/*  Check for end of format string. */

	if (*f == 0)
	    break;

	/* Check for embedded new-line. */

	if (*f == '\n')
	{
	    if (fcn)
	    	(*fcn)(arg, offset, buffer);
	    offset = 0;
	    f++;
	    continue;
	}

	/*  If non-specifier then put in buffer. */

	if (*f != '%')
	{
	    if (offset >= length)
	    {
		/*  Write the line and signal a continuation. */

		if (fcn)
		    (*fcn)(arg, length, buffer);
		offset = 0;
	    }
	    buffer[offset++] = *f++;
	    continue;
	}

	/*  Parse a format specifier. */

	trim = width = precision = 0;
        noprecision = TRUE;
	long_flag = FALSE;
	if (*++f == '~')
	{
	    trim = TRUE;
	    f++;
	}
	if (*f == '#')
	{
	    SET_ARG(width,p,i4);
	    f++;
	}
	else if (*f == '*')
	{
	    if (type > IN_REPEAT)
	    {
		SET_ARG(i4_tmp,p,i4);
		width = *((i4 *)((char *)base + i4_tmp));
	    }
	    else
	    {
		SET_ARG(i4p_tmp,p,i4 *);
		width = *i4p_tmp;
	    }
	    f++;
	}
	else
	    while (*f >= '0' && *f <= '9')
		width = width * 10 + *f++ - '0';
	if (*f == '.')
	{	
            noprecision = FALSE;	    /* precision for floating pt */
	    if (*++f == '#')
	    {
		SET_ARG(precision,p,i4);
		f++;
	    }
	    else if (*f == '*')
	    {
		if (type > IN_REPEAT)
		{
		    SET_ARG(i4_tmp,p,i4);
		    precision = *((i4 *)((char *)base + i4_tmp));
		}
		else
		{
		    SET_ARG(i4p_tmp,p,i4 *);
		    precision = *i4p_tmp;
		}
		f++;
	    }
	    else
		while (*f >= '0' && *f <= '9')
		    precision = precision * 10 + *f++ - '0';
	}
	if (*f == 'l')
	{
	    /* Don't bother checking that the l modifier is meaningful */
	    long_flag = TRUE;
	    ++f;
	}
	directive = *f++;
		
	/*  Now execute the directive. */

	switch (directive)
	{
	case '%':
	    buffer[offset] = directive;
	    offset++;
	    continue;

	case '[':
	    format_length = 0;
	    SET_ARG(base_address,p,PTR);
	    if (type > IN_REPEAT)
		/* base_address is a structure offset */
		base_address = (PTR)((SCALARP)base + (SCALARP)base_address);
	    save_offset = offset;
	    do
	    {
	        save_f = f;
	        save_cur_arg = cur_arg;
	        do_format(fcn, arg, &save_f, &p, &save_cur_arg, buffer, length,
		    &save_offset, *(PTR *)base_address, IN_ARRAY, marg);
		base_address += sizeof (i4 *);
	    } while (--width > 0);
	    f = save_f;
	    cur_arg = save_cur_arg;
	    offset = save_offset;
	    continue;

	case ']':
	case '}':
	case ')':
	    if ((type == IN_REPEAT && directive == ')') ||
		(type == IN_STRUCT && directive == '}') ||
		(type == IN_ARRAY && directive == ']'))
	    {
		*format = f;
#if defined(LNX) || defined(a64_sol) || defined(OSX)
                MEcopy( &p, sizeof(p), parameter );
# else
		*parameter = p;
# endif
		*pcur_arg = cur_arg;
		*buffer_offset = offset;
		return;
	    }
	    continue;

	case '<':
	    if (width)
	        cur_arg -= width;
	    else
	        cur_arg--;
	    if (cur_arg < 0)
		cur_arg = 0;
	    continue;

	case '>':
	    /*
	    ** This "feature" exhibits a high degree of
	    ** bogosity, and should be removed.
	    */
	    if (width)
	    {
		int i;

		for (i=0; i < width; i++)
		{
		    SET_ARG(i4_tmp,p,i4);
		}
	    }
	    else
	    {
		SET_ARG(i4_tmp,p,i4);
	    }
	    continue;

	case '{':
	    SET_ARG(base_address,p,PTR);
	    if (type > IN_REPEAT)
		/* base_address is a structure offset */
		base_address = (PTR)(SCALARP)base + (SCALARP)base_address;
	    save_offset = offset;
	    do
	    {
	        save_f = f;
	        save_cur_arg = cur_arg;
	        do_format(fcn, arg, &save_f, &p, &save_cur_arg, buffer, length,
		    &save_offset, base_address, IN_STRUCT, marg);
		base_address += precision;
	    } while (--width > 0);
	    f = save_f;
	    cur_arg = save_cur_arg;
	    offset = save_offset;
	    continue;

	case '(':
	    save_cur_arg = cur_arg;
	    save_offset = offset;
	    do
	    {
	        save_f = f;
	        do_format(fcn, arg, &save_f, &p, &save_cur_arg, buffer, length,
		    &save_offset, (PTR)0, IN_REPEAT, marg);
	    } while (--width > 0);
	    f = save_f;
	    cur_arg = save_cur_arg;
	    offset = save_offset;
	    continue;

	case '*':
	    if (width > length - offset)
	        width = length - offset;
	    MEfill(width, *f++, &buffer[offset]);
	    offset += width;
	    continue;

	case '?':
	    {
		SYSTIME		*time;

		if (type > IN_REPEAT)
		{
		    SET_ARG(i4_tmp,p,i4);
		    time = (SYSTIME *)((char *)base + i4_tmp);
		}
		else
		{
		    SET_ARG(time,p,SYSTIME *);
		}
		format_length = 0;
		TMstr(time, format_item);
		format_length = STlength(format_item);

		/* get rid of newline if tm puts it in */

		if (format_item[format_length - 1] == '\n')
		{
		    format_item[format_length - 1] = '\0';
		    format_length--;
		}

		format_buffer = format_item;
	    }

	    break;

	case '@':
	    {   
		SYSTIME		time;

		TMnow(&time);
		format_length = 0;
		TMstr(&time, format_item);
		format_length = STlength(format_item);
		format_buffer = format_item;
	    }
	    break;

        case '^':
            {
                PID	pid;

                PCpid(&pid);
# ifdef OS_THREADS_USED
                STprintf(format_item, "[%d,%d]", pid, TRcontext_tid);
# else
                STprintf(format_item, "[%d]", pid);
# endif
                format_length = STlength(format_item);
                format_buffer = format_item;
            }
            break;

	case 'c':
	    format_length = 1;
	    if (type > IN_REPEAT)
	    {
		SET_ARG(i4_tmp,p,i4);
	        format_item[0] = *((char *)base + i4_tmp);
	    }
	    else
	    {
		SET_ARG(format_item[0],p,i4);
	    }
	    if (format_item[0] < 32 || format_item[0] > 126)
	        format_item[0] = '.';
	    format_buffer = format_item;
	    break;

	case 'd':
	    format_buffer = format_item;
	    if (type > IN_REPEAT)
	    {
	        if (precision == 1)
		{
		    SET_ARG(i4_tmp,p,i4);
		    CVla(*((char *)((char *)base + i4_tmp)), format_buffer);
		}
		else if (precision == 2)
		{
		    SET_ARG(i4_tmp,p,i4);
		    CVla(*((short *)((char *)base + i4_tmp)), format_buffer);
		}
		else if (precision == 8)
		{
		    SET_ARG(i4_tmp,p,i4);
		    CVla8(*((i8 *)((char *)base + i4_tmp)), format_buffer);
		}
		else
		{
		    SET_ARG(i4_tmp,p,i4);
		    CVla(*((i4 *)((char *)base + i4_tmp)), format_buffer);
		}
	    }
	    else if (long_flag)
	    {
		SET_ARG(i8_tmp,p,i8);
	        CVla8(i8_tmp, format_buffer);
	    }
	    else
	    {
		SET_ARG(i4_tmp,p,i4);
	        CVla(i4_tmp, format_buffer);
	    }
	    for (format_length = 0; format_buffer[format_length]; format_length++)
	        ;
	    break;

	case 'u':
	    format_buffer = format_item;
	    if (type > IN_REPEAT)
	    {
	        if (precision == 1)
		{
		    SET_ARG(i4_tmp,p,i4);
		    CVula(*((char *)((char *)base + i4_tmp)), format_buffer);
		}
		else if (precision == 2)
		{
		    SET_ARG(i4_tmp,p,i4);
		    CVula(*((short *)((char *)base + i4_tmp)), format_buffer);
		}
		else if (precision == 8)
		{
		    SET_ARG(i4_tmp,p,i4);
		    CVula8(*((u_i8 *)((char *)base + i4_tmp)), format_buffer);
		}
		else
		{
		    SET_ARG(i4_tmp,p,i4);
		    CVula(*((i4 *)((char *)base + i4_tmp)), format_buffer);
		}
	    }
	    else if (long_flag)
	    {
		SET_ARG(i8_tmp,p,i8);
	        CVula8((u_i8)i8_tmp, format_buffer);
	    }
	    else
	    {
		SET_ARG(ui4_tmp,p,u_i4);
	        CVula(ui4_tmp, format_buffer);
	    }
	    for (format_length = 0; format_buffer[format_length]; format_length++)
	        ;
	    break;

 	case 'e':
	case 'f':
	case 'g':
	case 'n':
	    {
		i4                   wid;          /* float width field */
		i4                   prec;	    /* decimal field */
		i2                    real_width;   /* width of result */

	        format_buffer = format_item;
                if (noprecision)
		    prec = 3;
		else
		    prec = precision;
                if (!(wid = width))
		    wid = 20;
	        if (type > IN_REPEAT)
		{
		    SET_ARG(i4_tmp,p,i4);
	            CVfa(*((f8 *)((char *)base + i4_tmp)), wid, prec, *(f-1), 
		         '.', format_buffer, &real_width);
		}
                else
		{
		    SET_ARG(f_tmp,p,f8);
	            CVfa(f_tmp, wid, prec, *(f-1), '.', format_buffer, 
			&real_width);
		}
                format_length = real_width;
	        break;
	    }

	case 'x':
	case 'b':

	    format_buffer = format_item;
	    if (directive == 'b')
	        CVptrax(base, format_buffer);
	    else if (type > IN_REPEAT)
	    {
	        if (precision == 1)
		{
		    SET_ARG(i4_tmp,p,i4);
		    CVlx(*((unsigned char *)((char *)base + i4_tmp)), format_buffer);
		    format_buffer += 3 * 2; /* skip 3 unneeded hex bytes */
		}
		else if (precision == 2)
		{
		    SET_ARG(i4_tmp,p,i4);
		    CVlx(*((unsigned short *)((char *)base + i4_tmp)), format_buffer);
		    format_buffer += 2 * 2; /* skip 2 unneeded hex bytes */
		}
		else if (precision == 8)
		{
		    SET_ARG(i4_tmp,p,i4);
		    CVlx8(*((u_i8 *)((char *)base + i4_tmp)), format_buffer);
		}
		else
		{
		    SET_ARG(i4_tmp,p,i4);
		    CVlx(*((u_i4 *)((char *)base + i4_tmp)), format_buffer);
		}
	    }
	    else if (long_flag)
	    {
		SET_ARG(i8_tmp,p,i8);
	        CVlx8(i8_tmp, format_buffer);
	    }
	    else
	    {
		SET_ARG(i4_tmp,p,i4);
	        CVlx(i4_tmp, format_buffer);
	    }
	    for (format_length = 0; format_buffer[format_length];
		 format_length++)
		    ;
	    break;

	case 'p':

	    format_buffer = format_item;
	    if (type > IN_REPEAT)	    
	    {
		SET_ARG(i4_tmp,p,i4);
		CVptrax(*((PTR *)((char *)base + i4_tmp)), format_buffer);
	    }
	    else
	    {
		SET_ARG(p_tmp, p, PTR);
		CVptrax(p_tmp, format_buffer);
	    }
	    for (format_length = 0; format_buffer[format_length];
		 format_length++)
		    ;
	    break;

	case 's':
	    width = -width;
	    if (type > IN_REPEAT)
	    {
		SET_ARG(i4_tmp,p,i4);
	        format_buffer = ((char *)base + i4_tmp);
	    }
	    else
	    {
		SET_ARG(format_buffer,p,char *);
	    }
	    format_length = precision;
	    if (precision == 0)
	        for (format_length = 0; format_buffer[format_length]; format_length++)
	            ;
	    break;
	    
	case 't':
	    width = -width;
	    SET_ARG(format_length,p,i4);
	    format_length = min(format_length, 2048);
	    SET_ARG(string,p,char *);
	    format_buffer = format_item;
	    i4_tmp = format_length;
	    fptr = format_buffer;
	    cptr = string;
	    bcnt = 0;

	    while ((cptr - string) < format_length)
	    {
	      /* Allow tab, newline, printable through.  Replace other
	      ** stuff with spaces.
	      */
	      if (*cptr != '\t' && *cptr != '\n'
		&& !CMprint(cptr) || CMcntrl(cptr))
	      {
		CMnext(cptr);
		*fptr++ = ' ';
	      }
	      else 
		CMcpyinc(cptr, fptr);	        
	    }
	    if (trim)
		while (format_length)
		    if (format_buffer[format_length - 1] != ' ')
			break;
		    else
			format_length--;
	    break;

	case 'v':
	    width = -width;
	    SET_ARG(string,p,char *);
	    if (type > IN_REPEAT)
	    {
	        if (precision == 1)
		{
		    SET_ARG(i4_tmp,p,i4);
	            number = *(u_i1 *)((char *)base + i4_tmp);
		}
	        else if (precision == 2)
		{
		    SET_ARG(i4_tmp,p,i4);
	            number = *(u_i2 *)((char *)base + i4_tmp);
		}
	        else
		{
		    SET_ARG(i4_tmp,p,i4);
	            number = *(i4 *)((char *)base + i4_tmp);
		}
	    }
	    else
	    {
		SET_ARG(number,p,i4);
	    }
	    ui4_tmp = (u_i4)number;
	    format_buffer = format_item;
	    format_length = 0;
	    save_length = 0;
	    do
	    {
	        format_buffer[format_length++] = *string;
	        if (*string == 0 || *string == ',')
	        {
		    if ((ui4_tmp & 1) == 0)
		        format_length = save_length;
		    else
		        save_length = format_length;
		    if ((ui4_tmp >>= 1) == 0)
			break;
		}
	    } while (*string++);
	    if (ui4_tmp)
	    {
		/*
		** If string exhausted and we still have bits set force
		** output of the offending number surrounded with { and }
		*/
		format_buffer[format_length++] = '{';
		format_buffer[format_length++] = '0';
		format_buffer[format_length++] = 'x';
	        CVlx(number, format_buffer+format_length);
		while(format_buffer[++format_length])
		    /*SKIP*/;
		format_buffer[format_length++] = '}';
		format_buffer[format_length] = EOS;
		/* Treat the presence of extra bits as an error and make
		** sure these diags are not truncated */
		if (-width < format_length)
		    width = -format_length;
	    }
	    else if (format_length &&
		(format_buffer[format_length - 1] == ',' ||
		    format_buffer[format_length - 1] == 0))
	    {
		    format_length--;
	    }
	    break;

	case 'w':
	    width = -width;
	    SET_ARG(string,p,char *);
	    if (type > IN_REPEAT)
	    {
	        if (precision == 1)
		{
		    SET_ARG(i4_tmp,p,i4);
	            number = *(char *)((char *)base + i4_tmp);
		}
	        else if (precision == 2)
		{
		    SET_ARG(i4_tmp,p,i4);
	            number = *(short *)((char *)base + i4_tmp);
		}
	        else
		{
		    SET_ARG(i4_tmp,p,i4);
	            number = *(i4 *)((char *)base + i4_tmp);
		}
	    }
	    else
	    {
		SET_ARG(number,p,i4);
	    }
	    i4_tmp = number;
	    format_buffer = string;
	    format_length = 0;
	    do
	    {
	        if (*string == 0 || *string == ',')
	        {
		    if (number <= 0)
		        break;
		    number--;
		    format_buffer = string + 1;
		    format_length = 0;
		    continue;
		}
		format_length++;
	    } while (*string++);
	    if (number)
	    {
		/*
		** If number was negative to start with or string exhausted
		** force output of the offending number surrounded with { and }
		*/
		format_buffer = format_item;
		format_length = 0;
		format_buffer[format_length++] = '{';
	        CVla(i4_tmp, format_buffer+format_length);
		while (format_buffer[++format_length])
		    /*SKIP*/;
		format_buffer[format_length++] = '}';
		format_buffer[format_length] = EOS;
		/* Treat the presence of out of range value as an error and
		** make sure these diags are not truncated */
		if (-width < format_length)
		    width = -format_length;
	    }
	    break;

	default:
	    continue;
	}

	/* Copy result to output buffer. */

	if (width >= 0)
	{
	    save_length = width;
	    if (width == 0)
		save_length = format_length;
	    if (save_length < format_length)
		format_length = save_length;
	    save_length -= format_length;
	    
	    /*  Move leading pad characters to buffer. */

	    while (save_length)
	    {
		move_amount = save_length;
		if (move_amount > length - offset)
		    move_amount = length - offset;
		MEfill(move_amount, ' ', &buffer[offset]);
		save_length -= move_amount;
		offset += move_amount;
		if (offset >= length)
		{
		    if (fcn)
		    	(*fcn)(arg, length, buffer);
		    offset = 0;
		}
	    }
	}
	else
	{
	    save_length = -width;
	    if (save_length < format_length)
		format_length = save_length;
	    save_length -= format_length;
	}

	/*  Move formatted data to buffer. */

	while (format_length)
	{
	    move_amount = format_length;
	    if (move_amount > length - offset)
	        move_amount = length - offset;
	    MECOPY_VAR_MACRO(format_buffer, move_amount, &buffer[offset]);
	    format_length -= move_amount;
	    format_buffer += move_amount;
	    offset += move_amount;
	    if (offset >= length)
	    {
		if (fcn)
		    (*fcn)(arg, length, buffer);
	        offset = 0;
	    }
	}

	/*  Move trailing pad characters to buffer. */

	while (save_length)
	{
	    move_amount = save_length;
	    if (move_amount > length - offset)
	        move_amount = length - offset;
	    MEfill(move_amount, ' ', &buffer[offset]);
	    save_length -= move_amount;
	    offset += move_amount;
	    if (offset >= length)
	    {
		if (fcn)
		    (*fcn)(arg, length, buffer);
	        offset = 0;
	    }
	}

	/* Continue with next character from format. */
    }

    *format = f;
#if defined(LNX) || defined(a64_sol) || defined(OSX)
    MEcopy( &p, sizeof(p), parameter );
# else
    *parameter = p;
# endif
    *pcur_arg = cur_arg;
    *buffer_offset = offset;
    return;
}

/*{
** Name: TRflush	- Flush buffers to disk.
**
** Description:
**      This routine will flush any trace output buffers to disk.
**
** Inputs:
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      18-nov-1986 (Derek)
**          Created.
**      30-Oct-2002 (hanal04) Bug 109026 INGSRV 1990
**          Added exception handler to ensure we release the TRcontext_mutex
**          if an exception occurs.
**      25-Nov-2002 (hanal04) Bug 109026 INGSRV 1990
**          Correct above change to resolve build errors on Solaris.
**          return(OK); should be return; for a VOID function.
**       4-Dec-2002 (hanal04) Bug 109222 INGSRV 2020
**          Initialise TRmutex_tid before releasing the TRcontext_mutex
**          to ensure we do not see stale owner information.
[@history_template@]...
*/
VOID
TRflush()
{
    FILE	*fp;
    TRCONTEXT	*context = &tr_default;
    EX_CONTEXT        ex_context;

# ifdef OS_THREADS_USED
    if ( TRcontext_init == FALSE )
    {
	TRcontext_init = TRUE;
	CS_synch_init( &TRcontext_mutex );
    }
    if ( TRcontext_level == 0 || !CS_thread_id_equal(TRcontext_tid, CS_get_thread_id()) )
    {
        CS_synch_lock( &TRcontext_mutex );
        if (EXdeclare(tr_handler, &ex_context))
	{
            if(TRcontext_level != 0 && 
		     CS_thread_id_equal(TRcontext_tid, CS_get_thread_id()) )
            {
                TRcontext_level = 0;
		CS_thread_id_assign(TRcontext_tid, 0);
                CS_synch_unlock( &TRcontext_mutex );
		EXdelete();
                return;
	    }
	}
    }

    CS_thread_id_assign(TRcontext_tid, CS_get_thread_id());
    TRcontext_level++;
# endif /* OS_THREADS_USED */

    if ((fp = context->fp_array[FP_TERMINAL]) != NULL)
	SIflush(fp);
    if ((fp = context->fp_array[FP_FILE]) != NULL)
	SIflush(fp);

# ifdef OS_THREADS_USED
    TRcontext_level--;
    if ( TRcontext_level == 0 )
    {
	CS_thread_id_assign(TRcontext_tid, 0);
        CS_synch_unlock( &TRcontext_mutex );
	EXdelete();
    }
# endif /* OS_THREADS_USED */
}

/*{
** Name: TRgettrace	- Test a trace flag.
**
** Description:
**      Test a specific word and/or bit of a word that was previously 
**      initialized by calling TRmaketrace.  TRgettrace uses the vector
**	that was used in the last call to TRmaketrace.  If no calls have
**	been made to TRmaketrace, then it always returns FALSE.
**
** Inputs:
**      word                            The index of the word to set.
**      bit                             The bit within the wword to set or
**					and bit if bit == -1.
**
** Outputs:
**	Returns:
**	    bool			TRUE if bit was set
**					False if bit wasn't set.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-sep-1985 (derek)
**          Created new for 5.0.
*/
bool
TRgettrace(word,bit)
i4	word;
i4	bit;
{
	bool   ret;
	i2	**trace_vect = &trace_vector;
	STATUS status;

# ifdef OS_THREADS_USED
	if ( TRtracevectkey == 0 )
	{
            ME_tls_createkey( &TRtracevectkey, &status );
            ME_tls_set( TRtracevectkey, NULL, &status );
	}
        if ( TRtracevectkey == 0 )
        {
            /* not linked with threading library */
            TRtracevectkey = -1;
        }
        if ( TRtracevectkey != -1 )
        {
	    ME_tls_get( TRtracevectkey, (PTR *)&trace_vect, &status );
	    if ( trace_vect == NULL )
	    {
                trace_vect = (i2 **) MEreqmem(0, sizeof(i2 **), TRUE, NULL);
                ME_tls_set( TRtracevectkey, (PTR)trace_vect, &status );
	    }
	}
# endif /* OS_THREADS_USED */

	if (*trace_vect == 0)
	    ret = FALSE;
	else if (bit < 0)
	    ret = ((*trace_vect)[word] != 0);
	else
	    ret = ((((*trace_vect)[word] >> bit) & 1) != 0);
	return (ret);
}

/*{
** Name: TRmaketrace	- Setup trace vector
**
** Description:
**      Initilize a trace vector based on the flags that are passed to 
**	this routine.  The address of the trace vector is remembered for
**	future calls to TRgetrace or TRsettrace. 
**
**	Trace flags have syntax:
**	[-t | -tn1 | -tn2.n3 | -tn4/n5 ]
**	where t is whatever single character was set by argument tflag
**	of tTrace.  The four options mean set all flags in the trace vector,
**	set word n1, set bit n3 in word n2, and set words n4 through n5.
**
** Inputs:
**      argv                            An array of characters to scan for
**					trace arguments.
**      tflag                           The characters that must follow the
**					'-' in trace arguements.
**      tsize                           The size of the trace vector in i2's.
**	tonoff				Wheter to turn the bits on or off.
**
** Outputs:
**      tvect                           Sets bits in tvect determined by the 
**					arguments passed.  The name of tvect
**					if remembered as the default.
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-sep-1985 (derek)
**          Created new for 5.0.
**	14-nov-1991 (bryanp)
**	    Return STATUS to match CL spec.
*/
STATUS
TRmaketrace(
	char	**argv,
	char	tflag,
	i4	tsize,
	i4	*tvect,
	bool	tonoff)
{
    char		**ap;
    char		*p;
    i4			low_word;
    i4			high_word;
    i4			low_bit;
    i4			high_bit;
    i4			i;
    i2			**trace_vect = &trace_vector;
    STATUS		status;

# ifdef OS_THREADS_USED
    if ( TRtracevectkey == 0 )
    {
        ME_tls_createkey( &TRtracevectkey, &status );
        ME_tls_set( TRtracevectkey, NULL, &status );
    }
    if ( TRtracevectkey == 0 )
    {
        /* not linked with threading library */
        TRtracevectkey = -1;
    }
    if ( TRtracevectkey != -1 )
    {
        ME_tls_get( TRtracevectkey, (PTR *)&trace_vect, &status );
        if ( trace_vect == NULL )
        {
            trace_vect = (i2 **) MEreqmem(0, sizeof(i2 **), TRUE, NULL);
            ME_tls_set( TRtracevectkey, (PTR)trace_vect, &status );
        }
    }
# endif /* OS_THREADS_USED */

    for (ap = argv; *ap != NULL; ap++)
    {
        if ((*ap)[0] != '-' || (*ap)[1] != tflag)
	    continue;

	*trace_vect = (i2 *) tvect;
	p = *ap;

	while (*p)
	{
	    /* Search for n. */

	    for (low_word = 0; *p >= '0' && *p <= '9'; p++)
		low_word = low_word * 10 + *p - '0';

	    /* If '.' then search for 'm'. */
	    
	    if (*p == '.')
		for (low_bit = 0, p++; *p >= '0' && *p <= '9'; p++)
		    low_bit = low_bit * 10 + *p - '0';
	    else
		low_bit = -1;
	    high_word = low_word;
	    high_bit = low_bit;

	    /* If '/' then search for high n.m. */

	    if (*p == '/')
	    {
		for (high_word = 0; *p >= '0' && *p <= '9'; p++)
		    high_word = high_word * 10 + *p - '0';
		if (*p == '.')
		    for (high_bit = 0, p++; *p >= '0' && *p <= '9'; p++)
			high_bit = high_bit * 10 + *p - '0';
		else
		    high_bit = -1;
	    }

	    /* Set bits low_word.low_bit through high_word.high_bit. */

	    for (i = low_word; i <= high_word; i++)
	    {
		if (i == low_word)
		    tvect[i] |= tonoff ? (-1 << low_bit) : 
			~ (-1 << low_bit);
		else if (i == high_word)
		    tvect[i] |= tonoff ? ~(-1 << high_bit) :
			(-1 << high_bit);
		else if (tonoff)
		    tvect[i] = -1;
		else tvect[i] = 0;
	    }
	}
    }
    return (OK);
}

/*{
** Name: TRset_file	- Set trace input/output.
**
** Description:
**      Changes the trace system log file and terminal output file. 
**      The log file contains a log of all inputs and output of the 
**      trace system.  The terminal output contains only data written 
**      to the output.
**
** Inputs:
**      flag                            Flag to determine the action type.
**					Either: TR_F_OPEN, TR_F_CLOSE,
**					TR_T_OPEN, TR_T_CLOSE, TR_I_OPEN,
**					TR_I_CLOSE.
**      filename                        Pointer to the filename.
**      name_length                     The length of the filename.
**
** Outputs:
**	tr_context			Updates context if it was passed.
**      err_code                        Pointer to location to store error.
**	Returns:
**	    STATUS			CL error status.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-sep-1985 (derek)
**          Created new for 5.0.
**	27-may-1992 (jnash)
**	    Add ability to append to end of file via TR_F_APPEND.
**	22-june-1992 (jnash)
**	    TR_NOOVERRIDE_MASK added.
**      30-Oct-2002 (hanal04) Bug 109026 INGSRV 1990
**          Added exception handler to ensure we release the TRcontext_mutex
**          if an exception occurs.
**          Also modified return of TR_NOT_OPENED to ensure we adjust
**          TRcontext_level and release the TRcontext_mutex if
**          necessary.
**       4-Dec-2002 (hanal04) Bug 109222 INGSRV 2020
**          Initialise TRmutex_tid before releasing the TRcontext_mutex
**          to ensure we do not see stale owner information.
**	09-jun-2003 (devjo01)
**	    Add support for TR_F_SAFEOPEN. b110302.
*/
STATUS
TRset_file(flag, filename, name_length, err_code)
i4                 flag;
char               *filename;
i4                 name_length;
CL_ERR_DESC        *err_code;
{
    TRCONTEXT	    *context = &tr_default;
    i4		    file_index;
    int		    create = FALSE;
    int		    status;
    char	    *mode;
    LOCATION	    loc;
    int		    nooverride = 0;
    char	    locbuf[MAX_LOC];
    char	    signaturebuf[TR_SAFE_STRING_LEN+2];
    i4		    count;
    i4		    ret_val = 0;
    EX_CONTEXT        ex_context;

    /*	 Check for bad parameters. */

    if (err_code == 0)
	return (TR_BADPARAM);
    CL_CLEAR_ERR( err_code );

# ifdef OS_THREADS_USED
    if ( TRcontext_init == FALSE )
    {
	TRcontext_init = TRUE;
	CS_synch_init( &TRcontext_mutex );
    }
    if ( TRcontext_level == 0 || !CS_thread_id_equal(TRcontext_tid, CS_get_thread_id()) )
    {
        CS_synch_lock( &TRcontext_mutex );
        if (EXdeclare(tr_handler, &ex_context))
	{
            if(TRcontext_level != 0 && 
		     CS_thread_id_equal(TRcontext_tid, CS_get_thread_id()) )
            {
                TRcontext_level = 0;
		CS_thread_id_assign(TRcontext_tid, 0);
                CS_synch_unlock( &TRcontext_mutex );
		EXdelete();
                return(OK);
	    }
	}
    }
    CS_thread_id_assign(TRcontext_tid, CS_get_thread_id());
    TRcontext_level++;
# endif /* OS_THREADS_USED */

    nooverride = flag & TR_NOOVERRIDE_MASK;
    flag &= ~TR_NOOVERRIDE_MASK;

    /* Determine the file to work with. */

    status = OK;

    for (;;)
    {
    switch (flag)
    {
    case TR_T_CLOSE:
    case TR_T_OPEN:
	file_index = FP_TERMINAL;
	break;

    case TR_F_OPEN:
    case TR_F_SAFEOPEN:
    case TR_F_CLOSE:
    case TR_F_APPEND:
	file_index = FP_FILE;
	break;

    case TR_I_OPEN:
    case TR_I_CLOSE:
	file_index = FP_INPUT;
	break;

    default:
	status = TR_BADPARAM;
	break;
    }
    if ( status )
	break;

    /* Determine the action to perform. */

    switch (flag)
    {
    case TR_T_OPEN:
    case TR_F_OPEN:
    case TR_I_OPEN:
    case TR_F_SAFEOPEN:
	/*
	**  If this is an open and a file in already open, fall through
	**  to the close code.
	*/

	if (context->fp_array[file_index] == NULL)
	    break;
	else if (nooverride)
	{
	    status = TR_NOT_OPENED;
	    break;
	}

	create = TRUE;

    case TR_I_CLOSE:
    case TR_F_CLOSE:
    case TR_T_CLOSE:
	/*  Close the file */

	if (context->fp_array[file_index] == NULL)
	{
	    status = TR_BADCLOSE;
	    break;
	}
	if (BTtest(file_index, &context->stdioflag))
	{
	    BTclear(file_index, &context->stdioflag);
	    SIflush(context->fp_array[file_index]);
	}
	else
	{
	    status = SIclose(context->fp_array[file_index]);
	    if (status != OK)
	    {
		status = TR_BADCLOSE;
		break;
	    }
	}
	context->fp_array[file_index] = NULL;
	if (create == FALSE)
	{
	    status = OK;
	    ret_val = 1;
	    break;
	}
	break;

    case TR_F_APPEND:
	/* Append creates a new file unless nooverride requested */
	if ( (context->fp_array[file_index] != NULL) && nooverride )
	{
	    status = TR_NOT_OPENED;
	    break;
	}
    }
    if ( status != OK || ret_val != 0 )
	break;

    /* Must create the file now */

    /* Either an open, create or append depending on usage. */

    if (STscompare(filename, name_length, "stdio", 0) == 0)
    {
	switch (flag)
	{
	case TR_T_OPEN:
	case TR_F_OPEN:
	case TR_F_SAFEOPEN:
	    context->fp_array[file_index] = stdout;
	    break;
	case TR_I_OPEN:
	    context->fp_array[file_index] = stdin;
	    break;
	}
	BTset(file_index, &context->stdioflag);
    }
    else
    {
	STncpy( locbuf, filename, name_length);
	locbuf[ name_length ] = EOS;
	LOfroms(FILENAME&PATH, locbuf, &loc);
	switch (flag)
	{
	case TR_I_OPEN:
	case TR_F_SAFEOPEN:
	    mode = "r";
	    break;
	case TR_F_APPEND:
	    mode = "a";
	    break;
	default:
	    mode = "w";
	}
	status = SIopen(&loc, mode, &(context->fp_array[file_index]));

	if (TR_F_SAFEOPEN == flag)
	{
	    if (OK == status)
	    {
		/* File exists, check if it is a trace log */
		signaturebuf[TR_SAFE_STRING_LEN+1] = '\0';
		if ( OK != SIread(context->fp_array[file_index],
				  TR_SAFE_STRING_LEN+1, &count, signaturebuf) ||
				  (TR_SAFE_STRING_LEN+1) != count ||
				  0 != STcmp(signaturebuf + 1, TR_SAFE_STRING) )
		{
		    status = TR_BADOPEN;
		}
		SIclose(context->fp_array[file_index]);
		context->fp_array[file_index] = NULL;
	    }
	    else
	    {
		/* It's OK for file not to exist. */
		status = OK;
	    }
	}

	if (status != OK)
	{
	    if ( 'r' == *mode )
		status = TR_BADOPEN;
	    else
		status = TR_BADCREATE;
	    break;
	}

	if (TR_F_SAFEOPEN == flag)
	{
	    status = SIopen(&loc, "w", &(context->fp_array[file_index]));
	    if ( status )
	    {
		status = TR_BADCREATE;
		break;
	    }
	    /* Put signature line into file. */
	    TRdisplay( "%s %@\n", TR_SAFE_STRING );
	}
    }
    break;
    }

# ifdef OS_THREADS_USED
    TRcontext_level--;
    if ( TRcontext_level == 0 )
    {
	CS_thread_id_assign(TRcontext_tid, 0);
        CS_synch_unlock( &TRcontext_mutex );
	EXdelete();
    }
# endif /* OS_THREADS_USED */
    return (status);

}   

/*{
** Name: TRrequest	- Read from trace input file.
**
** Description:
**      Read a line of input from the trace input file.  If there
**	if a log file, the input line is logged to the log file
**	preceded by a comment designator.  Input records that have
**	comment designators are skipped.
**
** Inputs:
**      record                          Record buffer address.
**      record_size                     Size of record buffer.
**      prompt                          Prompt to use, 0 gets default.
**
** Outputs:
**      read_count                      The number of bytes read.
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-sep-1985 (derek)
**          Created new for 5.0.
**	17-Oct-1988 (daveb)
**	    readonly isn't legal.  Make the initialization readable.
**	24-jan-1989 (roger)
**	    Added CL_ERR_DESC usage.  Also, SIgetrec only fails w/ ENDFILE.
**      30-Oct-2002 (hanal04) Bug 109026 INGSRV 1990
**          Added exception handler to ensure we release the TRcontext_mutex
**          if an exception occurs.
**       4-Dec-2002 (hanal04) Bug 109222 INGSRV 2020
**          Initialise TRmutex_tid before releasing the TRcontext_mutex
**          to ensure we do not see stale owner information.
*/
STATUS
TRrequest(record, record_size, read_count, err_code, prompt)
char		   *record;
i4                 record_size;
i4                 *read_count;
CL_ERR_DESC        *err_code;
char               *prompt;
{
    FILE	    *infp;
    FILE	    *outfp;
    TRCONTEXT	    *context = &tr_default;
    char	    log_buffer[256];
    int		    status;
    i4	    count;
    char	    *cp;
    char	    newline = '\n';
    static 	    char *prompt_buffer = "TRrequest> ";
    EX_CONTEXT        ex_context;

    /*	Check for bad paramaters. */

    if (err_code == 0)
	return(TR_BADPARAM);
    CL_CLEAR_ERR( err_code );

# ifdef OS_THREADS_USED
    if ( TRcontext_init == FALSE )
    {
	TRcontext_init = TRUE;
	CS_synch_init( &TRcontext_mutex );
    }
    if ( TRcontext_level == 0 || !CS_thread_id_equal(TRcontext_tid, CS_get_thread_id()) )
    {
        CS_synch_lock( &TRcontext_mutex );
        if (EXdeclare(tr_handler, &ex_context))
	{
            if(TRcontext_level != 0 && 
		     CS_thread_id_equal(TRcontext_tid, CS_get_thread_id()) )
            {
                TRcontext_level = 0;
		CS_thread_id_assign(TRcontext_tid, 0);
                CS_synch_unlock( &TRcontext_mutex );
		EXdelete();
                return(OK);
	    }
	}
    }
    CS_thread_id_assign(TRcontext_tid, CS_get_thread_id());
    TRcontext_level++;
# endif /* OS_THREADS_USED */

    status = OK;

    for (;;) /* something to break out of */
    {
    if (record == 0 || record_size == 0 || read_count == 0)
    {
	status = TR_BADPARAM;
	break;
    }

    /*	Normalize the record size.  */

    if (record_size > sizeof(log_buffer) - 1)
	record_size = sizeof(log_buffer) - 1;

    infp = context->fp_array[FP_INPUT];

    if (infp == NULL)
    {
	status = TR_BADREAD;
	break;
    }

    if (prompt == 0)
    {
	prompt = prompt_buffer;
    }

    /*	Read until a record which is not a comment is found. */

    do
    {
	if ((outfp = context->fp_array[FP_TERMINAL]) != NULL)
	{
	    SIwrite(STlength(prompt), prompt, &count, outfp);
	    SIflush(outfp);
	}
	status = SIgetrec(record, record_size, infp);
	if (status == ENDFILE)
	{
	    status = TR_ENDINPUT;
	    break;
	}
	if ((cp = STchr(record, newline)) != NULL)
		*cp = '\0';
    }	while (record[0] == '!');
    if ( status )
	break;

    /*	If there is a log file, write the input to the log file. */

    *read_count = STlength(record);
    outfp = context->fp_array[FP_FILE];
    if (outfp != NULL)
    {
	SIwrite(*read_count, record, &count, outfp);
    }
    break;
    }
# ifdef OS_THREADS_USED
    TRcontext_level--;
    if ( TRcontext_level == 0 )
    {
	CS_thread_id_assign(TRcontext_tid, 0);
        CS_synch_unlock( &TRcontext_mutex );
	EXdelete();
    }
# endif /* OS_THREADS_USED */
    return (status);

}

/*{
** Name: TRsettrace	- Set a trace flag.
**
** Description:
**      Sets or clears trace flags set in the default trace vector 
**      setup by TRmaketrace.  If no trace vector has been setup the
**	the call is ignored.
**
** Inputs:
**      word                            The index of the word to set.
**      bit                             The bit within the word to set, or
**					all bits if bit == -1.
**      set                             The value to set either 0 or 1.
**
** Outputs:
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-sep-1985 (derek)
**          Created for new 5.0.
**	14-nov-1991 (bryanp)
**	    Return status to match CL spec.
**	14-Jul-1993 (daveb)
**	    Return status when trace_vector is 0.  Was sending back junk.
*/
STATUS
TRsettrace(
	i4	word,
	i4	bit,
	bool	set)
{
    i2		**trace_vect = &trace_vector;
    STATUS	status;

# ifdef OS_THREADS_USED
    if ( TRtracevectkey == 0 )
    {
        ME_tls_createkey( &TRtracevectkey, &status );
        ME_tls_set( TRtracevectkey, NULL, &status );
    }
    if ( TRtracevectkey == 0 )
    {
        /* not linked with threading library */
        TRtracevectkey = -1;
    }
    if ( TRtracevectkey != -1 )
    {
        ME_tls_get( TRtracevectkey, (PTR *)&trace_vect, &status );
        if ( trace_vect == NULL )
        {
            trace_vect = (i2 **) MEreqmem(0, sizeof(i2 **), TRUE, NULL);
            ME_tls_set( TRtracevectkey, (PTR)trace_vect, &status );
        }
    }
# endif /* OS_THREADS_USED */

    if (*trace_vect == 0)
	return(OK);
    if (bit < 0)
    {
	(*trace_vect)[word] = -1;
	if (set == FALSE)
	    (*trace_vect)[word] = 0;
    }
    else
        (*trace_vect)[word] &=  (~(1 << bit)) | ((set & 1) << bit);

    return (OK);
}

/*
** Name: TRwrite	- Write line to trace file.
**
** Description:
**      Internal routine to write the line to the trace output file and/or
**	the trace log file or neither.
** 
** Inputs:
**      buffer                          Address of line to print.
**	length				Length of line to print.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-nov-1985 (derek)
**          Created new for jupiter.
**	25-jun-1991 (bryanp)
**	    Don't suppress zero-length lines.
**	14-Jul-1993 (daveb)
**	    return the status we so nicely maintained and threw away,
**	    returning a random number.  Lots of people must have
**	    been checking...
**	05-sep-2002 (devjo01)
**	    Rename 'write_line' to TRwrite, and make a public function.
**      30-Oct-2002 (hanal04) Bug 109026 INGSRV 1990
**          Added exception handler to ensure we release the TRcontext_mutex
**          if an exception occurs.
**       4-Dec-2002 (hanal04) Bug 109222 INGSRV 2020
**          Initialise TRmutex_tid before releasing the TRcontext_mutex
**          to ensure we do not see stale owner information.
*/
STATUS
TRwrite(arg, length, buffer)
PTR		    arg;
i4		    length;
char                *buffer;
{
    i4		count;
    TRCONTEXT		*context = &tr_default;
    FILE		*fp;
    STATUS		status;
    EX_CONTEXT        ex_context;

# ifdef OS_THREADS_USED
    if ( TRcontext_init == FALSE )
    {
	TRcontext_init = TRUE;
	CS_synch_init( &TRcontext_mutex );
    }
    if ( TRcontext_level == 0 || !CS_thread_id_equal(TRcontext_tid, CS_get_thread_id()) )
    {
        CS_synch_lock( &TRcontext_mutex );
        if (EXdeclare(tr_handler, &ex_context))
	{
            if(TRcontext_level != 0 && 
		     CS_thread_id_equal(TRcontext_tid, CS_get_thread_id()) )
            {
                TRcontext_level = 0;
		CS_thread_id_assign(TRcontext_tid, 0);
                CS_synch_unlock( &TRcontext_mutex );
		EXdelete();
                return(OK);
	    }
	}
    }
    CS_thread_id_assign(TRcontext_tid, CS_get_thread_id());
    TRcontext_level++;
# endif /* OS_THREADS_USED */

    status = FAIL;

    if ((fp = context->fp_array[FP_TERMINAL]) != NULL)
    {
	if (length > 0)
	    SIwrite(length, buffer, &count, fp);
	SIwrite(1, "\n", &count, fp);
	SIflush(fp);
	status = OK;
    }
    if ((fp = context->fp_array[FP_FILE]) != NULL)
    {
	SIwrite(1, "!", &count, fp);
	if (length > 0)
	    SIwrite(length, buffer, &count, fp);
	SIwrite(1, "\n", &count, fp);
	SIflush(fp);
	status = OK;
    }

# ifdef OS_THREADS_USED
    TRcontext_level--;
    if ( TRcontext_level == 0 )
    {
	CS_thread_id_assign(TRcontext_tid, 0);
        CS_synch_unlock( &TRcontext_mutex );
	EXdelete();
    }
# endif /* OS_THREADS_USED */
    return( status );
}

/*
** Name: tr_handler     - Top level TR handler to ensure TRcontext_mutex
**                        released if we pop the stack on an exception.
**
** Description:
**      This handler deals with exceptions which escape up out of the
**      thread that generated them.  Such events are thread fatal.
**      Log what we know about the exception, and return EXDECLARE
**      so the thread will try to cleanup.
**
** Inputs:
**      exargs          EX arguments (this is an EX handler)
** Outputs:
**      Returns:
**          EXDECLARE
**
** Side Effects:
**          Logs exception.
**
**  History:
**      01-Oct-2002 (hanal04) Bug 109026 INGSRV 1990
**          Created.
**      29-May-2007 (horda03) Bug 118412
**          Potential memory corruption when handling
**          an exception as the length of buf is determined
**          before buf is initialised.
*/
STATUS
tr_handler(exargs)
EX_ARGS     *exargs;
{
 
    char        buf[ER_MAX_LEN];
    char        exbuf[ER_MAX_LEN];
    i4          msg_language = 0;
    char        msg_text[ER_MAX_LEN];
    i4          msg_length;
    i4          length;
    i4          i;
    CL_ERR_DESC error;
 
    buf[ 0 ] = '\0';
    if (!EXsys_report(exargs, buf ))
    {
        STprintf(buf, "Exception is %x", exargs->exarg_num);
        for (i = 0; i < exargs->exarg_count; i++)
            STprintf(buf+STlength(buf), ",a[%d]=%x",
                     i, exargs->exarg_array[i]);
    }
    ERsend(ER_ERROR_MSG, buf, STlength(buf), &error);
    TRdisplay("tr_handler() stack dumped to errlog.log \n");
 
    return(EXDECLARE);
}

