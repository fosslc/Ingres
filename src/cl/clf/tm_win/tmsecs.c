/*
** Copyright (c) 1987, 2001 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<systypes.h>
# include	<rusage.h>
# include	<tm.h>			/* hdr file for TM stuff */

/*
** Name: TMSECS.C - Return time in seconds.
**
** Description:
**      This file contains the following tm routines:
**    
**      TMsecs()        -  Current time is seconds
**
** History:
** Revision 1.1  88/08/05  13:46:55  roger
** UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
** 
**      Revision 1.2  87/11/10  16:04:10  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	15-dec-92 (smc)
**	    Declaration of time() changed for axp_osf as it differs from
**	    one in time.h.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	14-apr-95 (emmag)
**	    NT porting changes.
**	11-oct-95 (tutto01)
**	    NT porting changes for Alpha.
**	07-feb-2001 (somsa01)
**	    Added porting changes for i64_win.
*/


/*{
** Name: TMsecs	- Current time in seconds
**
** Description:
**	Return the number of seconds since Jan. 1, 1970.
**
** Inputs:
**
** Outputs:
**	Returns:
**          Return the number of seconds since Jan. 1, 1970.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-sep-86 (seputis)
**          initial creation
**	06-jul-87 (mmm)
**	    initial jupiter unix CL.
*/
i4
TMsecs()
{
# if defined (axp_osf) || defined (NT_GENERIC)

    return((i4) time((time_t *)NULL));

# else

    long	time();
    return((i4) time((long *)NULL));

# endif /* axp_osf || NT_GENERIC */

}
