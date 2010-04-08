/*
**    Copyright (c) 1987, Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<systypes.h>
# include	<rusage.h>
# include	<tm.h>			/* hdr file for TM stuff */


/**
** Name: TMYRSIZE.C - Functions used by the TM CL module.
**
** Description:
**      This file contains the following tm routines:
**    
**      TMyrsize()        -  days in year
**
** History:
 * Revision 1.1  88/08/05  13:47:00  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.2  87/11/10  16:04:47  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**/


/*{
** Name: TMyrsize	- days in year
**
** Description:
**	Routine returns the number of days in a normal year (365)
**	if "year" is not a leap year.  366 is returned for a leap
**	year.  This routine does not guarantee that it will work
**	for all years before the 1900's.
**
**	Will signal a BAD_YEAR exception if year is less than zero (0).
**
** Inputs:
**      year                            year (larger than 0)
**
** Outputs:
**      days                            number of days in year
**	Returns:
**	    OK or FAIL if year < 0
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
**	    initial jupiter cl.
*/

STATUS
TMyrsize(year,days)
i4 	year;
i4 	*days;
{
	if (year < 0)
	{
		*days = 0;
		return(FAIL);
	}

	if (((year % 4) == 0)  && ((year % 100) != 0) || ((year % 400) == 0))
	{
		*days = LEAPYEAR;
		return(OK);
	}
	else
	{
		*days = NORMYEAR;
		return(OK);
	}
}
