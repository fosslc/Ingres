/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: TMLOCAL.H - Internal TM definitions
**
** Description:
**      This files has definitions for objects only referenced by TM
**	routines and need not be exported in the global tm.h file.
**
**
** History:
 * Revision 1.2  88/08/10  14:24:47  jeff
 * added daveb's fixs for non-bsd
 * 
 * Revision 1.1  88/08/05  13:46:52  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/12/21  13:00:10  mikem
**      bunch of changes for 6.0 version.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	07-mar-89 (russ)
**	    Removed includes of various [sys/]time[s].h that are
**	    included within rusage.h.  Also removed the un-used
**	    MILLI_MASK.
**	29-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Remove superfluous "typedef" before structure.
**	26-apr-93 (vijay)
**	    AIX compiler says: Tag _TM_MONTHTAB requires a complete definition
**	    before it is used. So, move the definition up. Compiler bug.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*  Types defined in this file. */

typedef struct _TM_MONTHTAB TM_MONTHTAB;


/*}
** Name: TM_MONTHTAB - entry in a lookup table of months
**
** Description:
**	An entry in the lookup table contains a character representation of
**	a month (currently only jan, feb, mar, ...).  This is the key that
**	is used to search the table, the numeric representation is found
**	in the corresponding tmm_month field (1, 2, 3, ... 12)..
**
** History:
**     07-jul-1987 (mmm)
**          Created new for jupiter
*/

struct _TM_MONTHTAB
{
    char	   *tmm_code;		    /* string used to specify month */
    i4	   	   tmm_month;		    /* numeric designation of month */
};

/*
**  Forward and/or External typedef/struct references.
*/

GLOBALREF TM_MONTHTAB 		TM_Monthtab[];	/* lookup table of months */

GLOBALREF i4			TM_Dmsize[];	/* table of # days in months */

