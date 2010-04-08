/*
**    Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h> 
# include	<gl.h> 
# include	<clconfig.h>
# include	<systypes.h>
# include	<rusage.h>
# include	<tm.h> 		/* header file for TM stuff */

/**
** Name: TMSUBTR.C - Functions used by the TM compatibility library module
**
** Description:
**      This file contains the following tm routines:
**    
**      TMsubstr()        -  difference of the TIMER structs
**
** History:
 * Revision 1.1  88/08/05  13:46:57  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  88/01/06  17:55:36  anton
**      new tm
**      
**      Revision 1.2  87/11/10  16:04:27  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**      08-mar-91 (rudyw)
**          Comment out text following #endif
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**/


/*{
** Name: TMsubstr	- difference of the TIMER structs
**
** Description:
**	TMsubtr - take the difference of two TIMER structs.  Intended for use 
**		with nested (TMstart, TMend) pairs.  It should be used like 
**		this:
**
**			TIMER	outer;
**			TIMER	inner;
**			TIMER	result;
**
**			TMstart(&outer);
**				.
**				.
**			TMstart(&inner);
**				.
**				.
**			TMend(&inner);
**				.
**				.
**			TMend(&outer);
**			TMsubtr(&outer, &inner, &result);
**
**		This will fill result with the statistics from outer minus the 
**		statistics from inner.  The starting statistics in result will 
**		be those from outer.  The ending statistics will be set to the 
**		ending statistics for outer minus the statistics accumulated 
**		between the TMstart(&inner) and the TMend(&inner).  The 
**		difference statistics will be set to the difference statistics 
**		for outer minus the statistics accumulated for inner.
**
** Inputs:
**      outer                           outer statistics
**      inner                           inner statistics
**
** Outputs:
**      result                          TIMER struct for difference
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	07/06/84 (lichtman) - written
**      16-sep-86 (seputis)
**          initial creation
**	06-jul-87 (mmm)
**	    initial jupiter unix cl.
*/

VOID
TMsubstr(outer, inner, result)
TIMER		   *outer;
TIMER		   *inner;
TIMER		   *result;
{
	register TIMERSTAT	*restart	= &result->timer_start,
				*resend		= &result->timer_end,
				*resval		= &result->timer_value,
				*outend		= &outer->timer_end,
				*inval		= &inner->timer_value;

	/* starting stats for result = starting stats for outer */
	STRUCT_ASSIGN_MACRO(outer->timer_start, *restart);
	resend->stat_cpu	= outend->stat_cpu - inval->stat_cpu;
	resend->stat_usr_cpu	= outend->stat_usr_cpu - inval->stat_usr_cpu;
	resend->stat_dio	= outend->stat_dio - inval->stat_dio;
	resend->stat_bio	= outend->stat_bio - inval->stat_bio;
	resend->stat_wc		= outend->stat_wc - inval->stat_wc;
	resend->stat_pgflts	= outend->stat_pgflts - inval->stat_pgflts;
#	ifdef DECVMS
	/* doesn't make any sense to subtract the working set or
	** physical memory size.
	*/
	resend->stat_ws		= outend->stat_ws;		
	resend->stat_phys	= outend->stat_phys;
#	endif /* DECVMS	*/
	resval->stat_cpu	= resend->stat_cpu - restart->stat_cpu;
	resval->stat_usr_cpu	= resend->stat_usr_cpu - restart->stat_usr_cpu;
	resval->stat_dio	= resend->stat_dio - restart->stat_dio;
	resval->stat_bio	= resend->stat_bio - restart->stat_bio;
	resval->stat_wc		= resend->stat_wc - restart->stat_wc;
	resval->stat_pgflts	= resend->stat_pgflts - restart->stat_pgflts;
#	ifdef DECVMS
	resval->stat_ws		= resend->stat_ws;
	resval->stat_phys	= resend->stat_phys;
#	endif /* DECVMS */
}
