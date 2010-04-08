/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/


# include <compat.h>
# include <gl.h>
# include <cs.h>
# include <cv.h>
# include <st.h>
# include <lo.h>
# include <er.h>

# include <sp.h>
# include <pm.h>
# include <mo.h>
# include <mu.h>
# include "moint.h"

/**
** Name: moutil.c	- utilities for MO, including init.
**
** Description:
**	This file defines:
**
**   Data:
**	MO_disabled	bool
**	MO_classes	SPTREE
**	MO_instances	SPTREE
**	MO_strings	SPTREE
**	MO_monitors	SPTREE
**
**   Functions:
**
** 	MO_instance_compare	- comparison for MOobjects tree.
** 	MO_mon_compare		- comparison for monitors tree.
** 	MO_once			- one time initialization for MO.
**  	MOstrout		- output utility for MO methods to put strings
** 	MOlstrout		- string output with bounded input
**  	MOlongout		- output utility for MO methods to
**				  put signed i8s
**  	MOulongout		- output utility for MO methods for
**				  unsigned i8s
**	MOptrout		- output utility to put pointers.
**	MOptrxout		- output utility to put pointers in hex.
** 	MO_str_to_uns		- string to unsigned conversion.
**
** History:
**	24-sep-92 (daveb)
**	    documented.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	4-Aug-1993 (daveb) 58937
**	    cast PTRs before use.
**	8-Mar-1994 (daveb) 60415
**	    make MOlongout work right with negative numbers.  It
**	    was copying the unpadded buffer out!
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.  
**	22-apr-1996 (lawst01)
**	    Windows 3.1 porting changes - Set BOOL variable correctly
**      24-sep-96 (mcgem01)
**          Global data moved to modata.c
**      21-jan-1999 (canor01)
**          Remove erglf.h. Rename "class" to "mo_class".
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-aug-2001 (somsa01)
**	    Cleaned up compiler warnings.
**      09-Feb-2010 (smeke01) b123226, b113797 
**          Added handling of 8-byte integers.
**/

/* external variables */
 
GLOBALREF MU_SEMAPHORE MO_sem;
GLOBALREF i4  MO_semcnt;
GLOBALREF bool MO_disabled;

/*
** internal: Initialize symbol table when needed.
*/
GLOBALREF SPTREE *MO_instances;
GLOBALREF SPTREE *MO_classes;
GLOBALREF SPTREE *MO_strings;
GLOBALREF SPTREE *MO_monitors;

/* above pointers attached to below trees */

static SPTREE MOclass_tree ZERO_FILL;
static SPTREE MOinstance_tree ZERO_FILL;
static SPTREE MOstring_tree ZERO_FILL;
static SPTREE MOmonitor_tree ZERO_FILL;

/* MO statistics */

GLOBALREF i4 MO_nattach;
GLOBALREF i4 MO_nclassdef;
GLOBALREF i4 MO_ndetach;
GLOBALREF i4 MO_nget;
GLOBALREF i4 MO_ngetnext;
GLOBALREF i4 MO_nset;

GLOBALREF i4 MO_ndel_monitor;
GLOBALREF i4 MO_nset_monitor;
GLOBALREF i4 MO_ntell_monitor;

GLOBALREF i4 MO_nmutex;
GLOBALREF i4 MO_nunmutex;



GLOBALREF MO_CLASS_DEF MO_cdefs[];


/*{
** Name: MO_instance_compare	- comparison for MOobjects tree.
**
** Description:
**
**	comparison function for the MOobjects tree.
**
** 	Must arrange for first real instance to be found
** 	if a.instance == NULL.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	aptr	first object
**	bptr	first object
**
** Outputs:
**	none.
**
** Returns:
**	< 0, 0, > 0 based on ordering.
**
** History:
**	23-sep-92 (daveb)
**	    documented.
**	4-Aug-1993 (daveb)
**	    cast PTRs before use.
*/

i4
MO_instance_compare( const char *aptr, const char *bptr )
{
    MO_INSTANCE *a = (MO_INSTANCE *)aptr;
    MO_INSTANCE *b = (MO_INSTANCE *)bptr;

    register i4  res;

    res = *(char *)a->classdef->node.key - *(char *)b->classdef->node.key;
    if( res == 0 )
    {
	{
	    res = STcompare( a->classdef->node.key, b->classdef->node.key );
	    if( res == 0 )
	    {
		if( a->instance == NULL )
		{
		    res = b->instance == NULL? 0 : 1;
		}
		else if( b->instance == NULL )
		{
		    res = 1;
		}
		else
		{
		    res = a->instance[0] - b->instance[0];
		    if( res == 0 )
			res = STcompare( a->instance, b->instance );
		}
	    }
	}
    }

# ifdef xDEBUG
    SIprintf("compare %s:%s %s:%s -> %d\n",
	 a->classdef->node.key,
	 a->instance ? a->instance : "<nil>",
	 b->classdef->node.key,
	 b->instance ? b->instance : "<nil>",
	 res );
# endif

    return( res );
}


/*{
** Name: MO_mon_compare	- comparison for monitors tree.
**
** Description:
**	compare two monitor blocks for ordering purposes.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	aptr	the first one.
**	bptr	the second one.
**
** Outputs:
**	none
**
** Returns:
**	< 0, 0, > 0, based on ordering.
**
** History:
**	24-sep-92 (daveb)
**	    documented.
**	4-Aug-1993 (daveb)
**	    cast PTRs before use.
*/

i4
MO_mon_compare( const char *aptr, const char *bptr )
{
    MO_MON_BLOCK *a = (MO_MON_BLOCK *)aptr;
    MO_MON_BLOCK *b = (MO_MON_BLOCK *)bptr;

    register SIZE_TYPE  res;

    res = a->mo_class  - b->mo_class;
    if( res == 0 )
	res = (char *)a->mon_data - (char *)b->mon_data;

    return( res < 0 ? -1 : res == 0 ? 0 : 1 );
}


/*{
** Name: MO_once	- one time initialization for MO.
**
** Description:
**
**	Do all one time initialization for MO, in a way that is
**	safe to call repeatedly.  Sets up all the trees, the
**	MO mutex semaphore, etc.
**
** Re-entrancy:
**	yes, sort of.  Initial entry is a problem, afterwhich is
**	should be OK.  In a real multi-threaded world, this will
**	take some modification.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	24-sep-92 (daveb)
**	    Documented, move to GWM_globals for some things.
*/

VOID
MO_once(void)
{
    i4	state;
 
    if( MO_classes == NULL )
    {
	/* tricky recursion problem here -- i_sem
	   wants to do an attach, but we aren't ready.
	   This means no monitoring of MO_sem, sigh.  FIXME. */
	state = MO_disabled;
	MO_disabled = MO_DISABLE;
	(VOID) MUi_semaphore( &MO_sem );
	MUn_semaphore( &MO_sem, "MO" );
	MO_semcnt = 0;
	MO_disabled = (state) ? TRUE : FALSE;

	(VOID) MO_mutex();
	MUn_semaphore( &MO_sem, "MO" );
	MO_classes = SPinit( &MOclass_tree, STcompare );
	MO_instances = SPinit( &MOinstance_tree, MO_instance_compare );
	MO_strings = SPinit( &MOstring_tree, STcompare );
	MO_monitors = SPinit( &MOmonitor_tree, MO_mon_compare );
	(VOID) MO_unmutex();

	/* MO classes */

	(void) MOclassdef( MAXI2, MO_cdefs );
	(void) MOclassdef( MAXI2, MO_mem_classes );
	(void) MOclassdef( MAXI2, MO_meta_classes ); 
	(void) MOclassdef( MAXI2, MO_mon_classes ); 
	(void) MOclassdef( MAXI2, MO_str_classes );
	(void) MOclassdef( MAXI4, MO_tree_classes );

	/* tables */

	MO_classes->name = "mo_classes";
	MO_instances->name = "mo_instances";
	MO_strings->name = "mo_strings";
	MO_monitors->name = "mo_monitors";

	(void) MOsptree_attach( MO_classes );
	(void) MOsptree_attach( MO_instances );
	(void) MOsptree_attach( MO_strings );
	(void) MOsptree_attach( MO_monitors );
    }
}


/*{
**  Name: MOstrout - output utility for MO methods to put strings
**
**  Description:
**
**	Copy the input string to the output buffer up to a maximum
**	length.  If the length is exceeded, return the specified error
**	status.
**
**  Inputs:
**	 errstat
**		the status to return if the output buffer isn't big enough.
**	 src
**		the string to write out
**	 destlen
**		the length of the user buffer.
**	 dest
**		the buffer to use for output.
**  Outputs:
**	 dest
**		contains a copy of the the string value.
**  Returns:
**	OK or input errstat value.
**
**  History:
**	23-sep-92 (daveb)
**	    documented
*/

STATUS
MOstrout( STATUS errstat, char *src, i4  destlen, char *dest )
{
    STATUS ret_val = OK;

    while( --destlen && (*dest++ = *src++) )
	continue;
    if( destlen <= 0 )
	ret_val = errstat;

    return( ret_val );
}


/*{
** Name: MOlstrout	- string output with bounded input
**
** Description:
**	Copy the input string to the output, knowing the length of both the
**	input and output.  If input won't fit, return given error status.
**	Still terminated on EOS character in the input.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	errstat		value to return on error.
**	srclen		length of source string.
**	src		the source string.
**	destlen		length of dest buffer
**	dest		the dest buffer.
**
** Outputs:
**	dest		gets a copy of src.
**
** Returns:
**	OK or input errstat value.
**
** History:
**	24-sep-92 (daveb)
**	    documented.
*/
STATUS
MOlstrout( STATUS errstat, i4  srclen, char *src, i4  destlen, char *dest )
{
    STATUS ret_val = OK;

    while( --srclen && --destlen && (*dest++ = *src++) )
	continue;
    if( destlen <= 0 )
	ret_val = errstat;
    else
	*dest = EOS;

    return( ret_val );
}


/*{
**  Name: MOlongout - output utility for MO methods to put i8s
**
**  Description:
**
**	Turn the input i8 into a fixed width, 0 padded string and
**	copy it to the output buffer up to a maximum length.  Will write
**	a leading minus sign.  The fixed width is so string comparisons
**	will yield numeric sort ordering.
**
**  Inputs:
**	 errstat
**		the status to return if the output buffer isn't big enough.
**	 val
**		the value to write as a string.
**	 destlen
**		the length of the user buffer.
**	 dest
**		the buffer to use for output.
**  Outputs:
**	 dest
**		contains a copy of the the string value.
**  Returns:
**	 OK
**		if the operation succeeded
**	 MO_VALUE_TRUNCATED
**		if the output buffer was too small and the value was
**		chopped off to fit.
**  History:
**	23-sep-92 (daveb)
**	    documented
**	5-May-1993 (daveb)
**	    wrote it correctly, without STprintf, and putting
**	    the possible sign at the front.
**	8-Mar-1994 (daveb) 60415
**	    make MOlongout work right with negative numbers.  It
**	    was copying the unpadded buffer out!  Also fix width it
**  	    was putting out to be consistant with actual width needed
**  	    rather than the arbitrary value used for MO_MAX_NUM_BUF.
**      09-Feb-2010 (smeke01) b123226, b113797 
**          Added handling of 8-byte integers.
*/

STATUS
MOlongout( STATUS errstat, i8 val, i4  destlen, char *dest )
{
    char bufa[ MO_MAX_NUM_BUF ];
    char bufb[ MO_MAX_NUM_BUF ];
    char *p;
    char *q;
    i4  padlen;
    
    bufa[0] = EOS;

    /* Ignore errors in CVla8 -- force to 0 */
    (void)CVla8( val, bufa );
    padlen = MAX_I8_DIGITS_AND_SIGN - (i4)STlength( bufa );

    p = bufb;
    q = bufa;
    /* put sign first, before pad */
    if( bufa[0] == '-' )
	*p++ = *q++;
    while( padlen-- > 0 )
	*p++ = '0';
    STcopy( q, p );
	
    return( MOstrout( errstat, bufb, destlen, dest ) );
}


/*{
**  Name: MOulongout - output utility for MO methods to put unsigned i8s
**
**  Description:
**
**	Turn the input unsigned i8 into a fixed width, 0 padded
**	string and copy it to the output buffer up to a maximum length.
**	No sign will be printed.  The fixed width is so string
**	comparisons will yield numeric sort ordering.
**
**  Inputs:
**	 errstat
**		the status to return if the output buffer isn't big enough.
**	 val
**		the value to write as a string.
**	 destlen
**		the length of the user buffer.
**	 dest
**		the buffer to use for output.
**  Outputs:
**	 dest
**		contains a copy of the the string value.
**  Returns:
**	 OK
**		if the operation succeeded
**	 MO_VALUE_TRUNCATED
**		if the output buffer was too small and the value was
**		chopped off to fit.
**  History:
**	23-sep-92 (daveb)
**	    documented
**       8-Mar-1994 (daveb)
**          simplify pad length; can use CV_DEV_PTR_SIZE instead of
**  	    ugly if based on sizeof(u_i4).
**	 15-jan-1996 (toumi01; from 1.1 axp_osf port)
**	    ON axp_osf, CV_DEC_PTR_SIZE=22 which caused excessive looping
**	    when destlen was smaller (kinpa04).
**      09-Feb-2010 (smeke01) b123226, b113797 
**          Added handling of 8-byte integers.
*/

STATUS
MOulongout( STATUS errstat, u_i8 val, i4  destlen, char *dest )
{
    char buf[ MO_MAX_NUM_BUF ];	
    char *p;
    
    /* put p after last possible converted decimal, + leading 0  */

    /* comparison is plus 1 to account for space for EOS in the length */
    if( destlen < MAX_UI8_DIGITS + 1 )
    	 p = buf + destlen - 1; /* minus 1 because destlen is a length and includes '\0' */
    else 
    	 p = buf + MAX_UI8_DIGITS; /* Length constant used as 0-based offset to 1 beyond digits */
    *p-- = EOS;
    
    while( val != 0 && p >= buf )
    {
	*p-- = (char)((val % 10) + '0');
	val /= 10;
    }
    while( p >= buf )
	*p-- = '0';

    return( MOstrout( errstat, buf, destlen, dest ) );
}


/*{
**  Name:	MOptrout - output routine for methods to put a pointer value
**
**  Description:
**
**
**	Turn the input PTR into a fixed width, 0 padded string and copy
**	it to the output buffer up to a maximum length.  The fixed width
**	is so string comparisons will yield numeric sort ordering.
**
**  Inputs:
**	 errstat
**		the status to return if the output buffeer isn't big enough.
**	 ptr
**		the pointer value to write as a string.
**	 destlen
**		the length of the user buffer.
**	 dest
**		the buffer to use for output
**  Outputs:
**	 dest
**		contains a copy of the string value.
**  Returns:
**	 OK
**		if the operation succeeded.
**	 errstat
**		if the output buffer was too small and the value was chopped off
**		at the end to fit.
**
**  History:
**	5-May-1993 (daveb)
**	    created.
*/

STATUS
MOptrout( STATUS errstat, PTR ptr, i4  destlen, char *dest )
{
    char buf[ CV_DEC_PTR_SIZE + 1 ];

    CVptra( ptr, buf );

    return( MOstrout( errstat, buf, destlen, dest ) );
}


/*{
**  Name:	MOptrxout - output routine for methods to put a pointer value
**
**  Description:
**
**
**	Turn the input PTR into a fixed width, 0 padded string using
**	hexadecimal digits, and copy it to the output buffer up to a 
**	maximum length.  The fixed width is so string comparisons will
**	yield numeric sort ordering.
**
**  Inputs:
**	 errstat
**		the status to return if the output buffeer isn't big enough.
**	 ptr
**		the pointer value to write as a string.
**	 destlen
**		the length of the user buffer.
**	 dest
**		the buffer to use for output
**  Outputs:
**	 dest
**		contains a copy of the string value.
**  Returns:
**	 OK
**		if the operation succeeded.
**	 errstat
**		if the output buffer was too small and the value was chopped off
**		at the end to fit.
**
**  History:
**	7-jul-2003 (devjo01)
**	    created from MOptrout.
*/

STATUS
MOptrxout( STATUS errstat, PTR ptr, i4  destlen, char *dest )
{
    char buf[ CV_DEC_PTR_SIZE + 1 ];

    CVptrax( ptr, buf );

    return( MOstrout( errstat, buf, destlen, dest ) );
}


/*{
**  Name:	MOsidout - output routine for methods to put a session id value
**
**  Description:
**
**	Turn the input PTR into a session ID, which depending on the
**	users configuration will be in either hexadecimal or decimal
**	notation.   Current default is decimal, but this is intended
**	to change to hex at the next point release.
**
**  Inputs:
**	 errstat
**		the status to return if the output buffeer isn't big enough.
**	 ptr
**		the pointer value to write as a string.
**	 destlen
**		the length of the user buffer.
**	 dest
**		the buffer to use for output
**  Outputs:
**	 dest
**		contains a copy of the session ID value in the desired radix.
**  Returns:
**	 OK
**		if the operation succeeded.
**	 errstat
**		if the output buffer was too small and the value was chopped off
**		at the end to fit.
**
**  History:
**	7-jul-2003 (devjo01)
**	    Initial version.
*/

STATUS
MOsidout( STATUS errstat, PTR ptr, i4  destlen, char *dest )
{
    static	i4	usehex = -1;

    STATUS 	(*outfunc)( STATUS, PTR, i4, char * );
    char	*pmvalue;

    if ( usehex < 0 )
    {
	/* Set static config value if not already set.  We don't need to worry
	** about the unlikely event of two threads getting here simultaneously
	** since even if both try to init this, it gets set to the correct value
	** regardless and PM protects itself as needed.  Note also we assume PM
	** has already be initialized and loaded, which is a safe assumption as
	** this routines is not called during server initialization. 
	*/
	if ( OK == PMget("!.hex_session_ids", &pmvalue) &&
	     NULL != pmvalue &&
	     0 == STcasecmp(pmvalue, "on") )
	    usehex = 1;
	else
	    usehex = 0;
    }

    if ( usehex )
	outfunc = MOptrxout;
    else
	outfunc = MOptrout;
    return (*outfunc)( errstat, ptr, destlen, dest );
}


/*{
**  Name:	MOsidget - get session ID at a pointer method
**
**  Description:
**
**	Convert the session ID at the object location to a character
**	string.  The offset is treated as a byte offset to the input
**	object .  They are added together to get the object location.
**	
**	The output userbuf , has a maximum length of luserbuf that will
**	not be exceeded.  If the output is bigger than the buffer, it
**	will be chopped off and MO_VALUE_TRUNCATED returned.
**
**	The objsize is ignored.
**
**  Inputs:
**	 offset
**		value from the class definition, taken as byte offset to the
**		object pointer where the data is located.
**	 objsize
**		ignored
**	 object
**		pointer to the base of the structure holding the session ID to convert.
**	 luserbuf
**		the length of the user buffer.
**	 userbuf
**		the buffer to use for output.
**  Outputs:
**	 userbuf
**		contains the string of the value of the instance.
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		the output buffer was too small, and the string was truncated.
**	 other
**		conversion-specific failure status as appropriate.
**
**  History:
**	07-Jul-2003 (devjo01)
**	    Created from MOptrget().
*/

STATUS 
MOsidget( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    PTR *pobj;

    pobj = (PTR *)((char *)object + offset);

    return MOsidout( MO_VALUE_TRUNCATED, *pobj, lsbuf, sbuf );
}


/*{
** Name: MO_str_to_uns	- string to unsigned conversion.
**
** Description:
**	Convert a string to an unsigned long.  Curiously, we don't
**	have one of these in the CL.  No overflow checking is done.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	a	string to convert
**
** Outputs:
**	u	pointer to an unsigned i4 to stuff.
**
** Returns:
**	none
**
** History:
**	24-sep-92 (daveb)
**	    documented.
*/

void
MO_str_to_uns(char *a, u_i4 *u)
{
    register u_i4	x; /* holds the integer being formed */
    
    /* skip leading blanks */

    while ( *a == ' ' )
	a++;
    
    /* at this point everything had better be numeric */

    for (x = 0; *a <= '9' && *a >= '0'; a++)
	x = x * 10 + (*a - '0');
    
    *u = x;
}



