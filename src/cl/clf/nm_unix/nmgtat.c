/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<lo.h>
# include	<st.h>
# include       <me.h>
# include	<nm.h>
# include	"nmlocal.h"

/**
** Name: NMGTAT.C - Get Attribute.
**
**
** Description:
**	Return the value of an attribute which may be systemwide
**	or per-user.  The per-user attributes are searched first.
**	A Null is returned if the name is undefined.
**
**      This file contains the following external routines:
**
**	NMgtAt()	   -  get attribute.
**
** History: Log:	nmgtat.c
 * Revision 1.1  88/08/05  13:43:14  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/13  11:34:25  mikem
**      use nmlocal.h rather than "nm.h" to disambiguate the global nm.h
**	header from he local nm.h header.
**      
**      Revision 1.2  87/11/10  14:44:42  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.5  87/07/27  11:06:08  mikem
**      Updated to meet jupiter coding standards.
** 
**	Revision 1.2  86/06/25  16:24:12  perform
**	Need LO.h due to extern in nm.h (for NM cacheing).
** 
**	Revision 30.2  85/09/17  12:00:56  roger
**	Should not be of type VOID (see concomitant change
**	to INbackend() - use return value to detect improper
**	installation - this is implementation-specific, but...)
**		
**      20-jul-87 (mmm)
**          Updated to meet jupiter standards. 
**	19-jan-1989 (greg)
**	    Don't call NMgtIngAt for II_SYSTEM.
**	    A hack but better than infinite looping.
**	16-feb-1990 (greg)
**	    NMgtAt is a VOID not STATUS
**      14-Oct-1994 (nanpr01)
**          Removed #include "nmerr.h".  Bug # 63157
**	16-dec-1996 (reijo01)
**	    Convert TERM_INGRES to generic system terminal. Only for Jasmine
**	    product.
**	07-apr-1997 (canor01)
**	    Add include of <me.h>.
**	20-may-2000 (somsa01)
**	    The previous change did not allow the correct pickup of
**	    SystemTermType if the variable was TERM_INGRES.
**	15-nov-2010 (stephenb)
**	    Include nm.h for prototyping.
**/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */
GLOBALREF II_NM_STATIC	NM_static;



/*{
** Name: NMgtAt - Get attribute.
**
** Description:
**    Return the value of an attribute which may be systemwide
**    or per-user.  The per-user attributes are searched first.
**    II_SYSTEM will only be searched for per-user
**
**    A Null is returned if the name is undefined.
**
** Inputs:
**	name				    attribute to get
**
** Output:
**	value				    On success value of the attribute.
**
**      Returns:
**          VOID
**
** History:
**      20-jul-87 (mmm)
**          Updated to meet jupiter standards.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	10-oct-1996 (canor01)
**	    Allow generic system parameter overrides.
**	16-dec-1996 (reijo01)
**	    Convert TERM_INGRES to generic system terminal. Only for Jasmine
**	    product.
**	12-may-2000 (somsa01)
**	    Use SystemTermType for all products.
**	20-may-2000 (somsa01)
**	    The previous change did not allow the correct pickup of
**	    SystemTermType if the variable was TERM_INGRES.
**	24-Mar-2010 (hanje04)
**	    SIR 123296
**	    For LSB builds return NM_static.Sysbuf when SystemLocationVariable
**	    and not set
*/
VOID
NMgtAt(name, value)
char	*name;
char	**value;
{
    char newname[MAXLINE];

    if ( MEcmp( name, "II", 2 ) == OK )
        STpolycat( 2, SystemVarPrefix, name+2, newname );
    else if ( STcompare(name, "TERM_INGRES") == OK )
	STcopy( SystemTermType, newname );
    else
        STcopy( name, newname );

    (VOID)NMgtUserAt(newname, value);

    if ( *value == (char *)NULL )
    {
	if ( STcompare( SystemLocationVariable, newname ) )
	{
            if (NMgtIngAt(newname, value))
            {
    	        *value = (char *)NULL;
            }
        }
# ifdef conf_LSB_BUILD
        else if ( STcompare( SystemLocationVariable, newname ) == OK )
	    *value = NM_static.Sysbuf ;
# endif
    }
}
