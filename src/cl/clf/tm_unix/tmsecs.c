/*
**    Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<systypes.h>
# include	<rusage.h>
# include	<tm.h>			/* hdr file for TM stuff */

/**
** Name: TMSECS.C - Return time in seconds.
**
** Description:
**      This file contains the following tm routines:
**    
**      TMsecs()        -  Current time is seconds
**
** History:
 * Revision 1.1  88/08/05  13:46:55  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
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
**	03-Dec-1998 (muhpa01)
**	    Remove local declaration of time() for hp8_us5.
**	12-May-1999 (hweho01)
**	    Remove local declaration of time() for ris_u64.
**	03-May-2000 (ahaal01)
**	    Remove local declaration of time() for rs4_us5.
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**/


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
**	26-may-1998 (fucch01)
**	    Added sgi_us5 to axp to make it use 2nd call to time
**	    w/ time_t instead of long.
**	05-oct-1999 (hweho01/mosjo01) SIR 97564
**	    exclude rs4_us5 from using long time().
**	08-aug-2000 (hayke02)
**	    Exclude hp8_us5 from using long time().
*/
i4
TMsecs()
{
# if !defined(axp_osf) && !defined(sgi_us5) && !defined(any_hpux) && \
     !defined(any_aix)
    long	time();

    return((i4) time((long *)NULL));
# endif /* axp_osf sgi_us5 aix hpux */

    return((i4) time((time_t *)NULL));
}
