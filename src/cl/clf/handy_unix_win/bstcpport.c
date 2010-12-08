/*
**Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<cm.h>
# include	<st.h>
# include	"handylocal.h"


/*
** Name: BS_tcp_port - turn a tcp port name into a tcp port number
**
** Description:
**	This routines provides mapping a symbolic port ID, derived
**	from II_INSTALLATION, into a unique tcp port number for the 
**	installation.  This scheme was originally developed by the 
**	NCC (an RTI committee) and subsequently enhanced with an
**	extended port range and explicit port rollup indicator.
**
**	If pin is of the form:
**		XY{n}{+}
**	where
**		X is [a-zA-Z]
**		Y is [a-zA-Z0-9]
**		n is [0-9] | [0][0-9] | [1][0-5]
**		+ indicates port rollup permitted.
**
**	then pout is the string representation of the decimal number:
**
**        15  14  13  12  11  10  09  08  07  06  05  04  03  02  01  00
**       +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
**       !   S   !  low 5 bits of X  !   low 6 bits of Y     !     #     !
**       +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
**
**	# is the low 3 bits of the subport and S is 0x01 if the subport
**	is in the range [0,7] and 0x10 if in range [8,15]. 
**
**	The subport is a combination of the optional base subport {n} and
**	the rollup subport parameter.  The default base subport is 0.  A
**	rollup subport is permitted only if a '+' is present or if the
**	symbolic ID has form XY.
**
**	Port rollup is also permitted with numeric ports when the input
**	is in the form n+.  The numeric port value is combined with the
**	rollup subport parameter which must be in the range [0,15].
**
**	If pin is not a recognized form; copy pin to pout.
**
** Inputs:
**	portin - source port name
**	subport - added as offset to port number
**	portout - resulting port name 
**
** History
**	23-jun-89 (seiwald)
**	    Written.
**	28-jun-89 (seiwald)
**	    Use wild CM macros instead of the sane but defunct CH ones.
**	2-feb-90 (seiwald)
**	    New subport parameter, so we can programmatically generate
**	    successive listen ports to use on listen failure.
**	24-May-90 (seiwald)
**	    Renamed from GC_tcp_port.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 2-Apr-08 (gordy)  SIR 120457
**	    Implemented extended symbolic port range mapping algorithm.
**	    Implemented support for explicit port rollup indicator for
**	    symbolic and numeric ports.
**      06-Oct-09 (rajus01) Bug: 120552, SD issue:129268
**	    The portmapper routine takes additional parameter to return the
**	    actual symbolic port.
**	    The actual symbolic port will only be returned on success. The
**	    failure cases simply indicate that the bounds have been exceeded 
**	    and there is nothing valid left to return.  
**      29-Nov-2010 (frima01) SIR 124685
**	    Added include of handylocal.h.
*/

STATUS
BS_tcp_port( pin, subport, pout, pout_symbolic )
char	*pin;
i4	subport;
char	*pout;
char    *pout_symbolic;
{
    u_i2 portid, offset;
    
    /*
    ** Check for symbolic port ID format: aa or an
    **
    ** Input which fails to correctly match the symbolic 
    ** port ID syntax is ignored (by breaking out of the 
    ** for-loop) rather than being treated as an error.
    */
    for( ; CMalpha( &pin[0] ) && (CMalpha( &pin[1] ) | CMdigit( &pin[1] )); )
    {
	bool	rollup = FALSE;
	u_i2	baseport = 0;
	char	p[ 2 ];

	/*
	** A two-character symbolic port permits rollup.
	*/
	offset = 2;
	if ( ! pin[offset] )  rollup = TRUE;

	/*
	** One or two digit base subport may be specified
	*/ 
	if ( CMdigit( &pin[offset] ) )
	{
	    baseport = pin[offset++] - '0';

	    if ( CMdigit( &pin[offset] ) )
	    {
		baseport = (baseport * 10) + (pin[offset++] - '0');

		/*
		** Explicit base subport must be in range [0,15]
		*/
		if ( baseport > 15 )  break;
	    }
	}

	/*
	** An explict '+' indicates rollup.
	*/
	if ( pin[ offset ] == '+' )
	{
	    rollup = TRUE;
	    offset++;
	}

	/*
	** There should be no additional characters.
	** If so, input is ignored.  Otherwise, this
	** is assumed to be a symbolic port and any
	** additional errors are not ignored.
	*/
	if ( pin[ offset ] )  break;

	/*
	** Prepare symbolic port components.
	** Alphabetic components are forced to upper case.
	** Rollup subport increases base subport.
	*/
	CMtoupper( &pin[0], &p[0] );
	CMtoupper( &pin[1], &p[1] );
	baseport += subport;

	/*
	** A rollup subport is only permitted when
	** rollup is specified.  The combined base
	** and rollup subports must be in the range 
	** [0,15].
	*/
	if ( subport  &&  ! rollup )  return( FAIL );
	if( baseport > 15 )  return( FAIL );

	/*
	** Map symbolic port ID to numeric port.
	*/
	portid = (((baseport > 0x07) ? 0x02 : 0x01) << 14)
		    | ((p[0] & 0x1f) << 9)
		    | ((p[1] & 0x3f) << 3)
		    | (baseport & 0x07);

	CVla( (u_i4)portid, pout );
	/* Suppress 0 when displaying the actual symbolic port. 
	** Windows don't display subport value of 0... 
	*/
	if( baseport == 0 )
	    STprintf(pout_symbolic, "%c%c", pin[0], pin[1]);
	else
	    STprintf(pout_symbolic, "%c%c%d", pin[0], pin[1], baseport);
	return( OK );
    } 

    /*
    ** Check for a numeric port and optional port rollup.
    **
    ** If input is not strictly numeric or if port rollup
    ** is not explicitly requested, input is ignored.
    **
    ** Extract the numeric port and check for expected
    ** syntax: n{n}+
    */
    for( offset = portid = 0; CMdigit( &pin[offset] ); offset++ )
	portid = (portid * 10) + (pin[offset] - '0');

    if ( pin[offset] == '+'  &&  ! pin[offset+1] )
    {
	/*
	** Port rollup is restricted to range [0,15].
	*/
	if ( subport > 15 )  return( FAIL );
	CVla( portid + subport, pout );
	STcopy(pout, pout_symbolic);
	return( OK );
    }

    /*
    ** Whatever we have at this point, port rollup is not
    ** allowed.  Input is simply passed through unchanged.
    */
    if( subport )  return( FAIL );
    STcopy( pin, pout );
    STcopy(pin, pout_symbolic);
    return( OK );
}
