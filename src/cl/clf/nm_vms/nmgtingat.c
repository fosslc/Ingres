/*
**	Copyright (c) 1995,2000 - Ingres Corporation
*/
/*
**	NMgtIngAt - Get Ingres Attribute.
**
**	Return the value of an installation specific attribute.
**	The argument is combined with the installation identifier 
**	to form an installation specific logical name, which is
**	then (attempted to be) translated.
**
**	History:
**	   12-jul-1995 (duursma) created
**	   20-jul-1995 (wolf) 
**		Remove superfluous definition of NMgtAt().
**	   16-aug-1995 (duursma)
**		Changed return type from VOID to STATUS for compatibility
**		with platforms where the CL defines this routine
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	08-aug-2001 (kinte01)
**	    Update prototype for STlength
**
*/

# include	<compat.h>
# include	<ercl.h>  
# include       <gl.h>
# include	<nm.h>
# include	<me.h>
# include	<st.h>

FUNC_EXTERN	char *NMgetenv();
FUNC_EXTERN	size_t STlength();

STATUS
NMgtIngAt(name, value)
char	*name;
char	**value;
{
    i2 namelen = STlength( name );

    /* allocate a new string to hold the old string plus the installation id */
    char *buffer = MEreqmem( 0, namelen + 3, FALSE, NULL );

    i2 underscores = 0;
    i2 bufinx      = 0;
    i2 nameinx     = 0;
    
    char *ii_installation;

    /* Get the Installation ID */
    NMgtAt( ERx( "II_INSTALLATION" ), &ii_installation );

    while (EOS != name[nameinx]) {
	if ('_'  == name[nameinx] &&
	    2    == ++underscores &&
	    NULL != ii_installation) {
	    /* Insert the installation ID just before the second underscore */
	    /* for example, this would turn II_DUAL_LOG into II_DUALXX_LOG  */
	    /* if XX is the installation ID                                 */
	    buffer[bufinx++] = ii_installation[0];
	    buffer[bufinx++] = ii_installation[1];
	}
	buffer[bufinx++] = name[nameinx++];
    }

    if (NULL != ii_installation && 2 > underscores) {
	/* We found fewer than 2 underscores, just append installation id */
	buffer[bufinx++] = ii_installation[0];
	buffer[bufinx++] = ii_installation[1];
    }

    buffer[bufinx] = EOS;
    *value = NMgetenv( buffer );
    (void) MEfree( buffer );

    return (OK);
}
