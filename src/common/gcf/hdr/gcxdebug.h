/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: gcxdebug.h - GCX tracing support.
**
** Description:
**
**	GCX tracing provides symbolic message tracking up and down 
**	the GCC protocol stack using TRdisplay's.  The stack is this:
**		gcc_gca_exit
**		AL
**		PL
**		SL
**		TL
**		gcc_prot_exit
**      
**	It also provides tracing for the protocol bridge.
**
**	The preprocessor define GCXDEBUG enables tracing, and the 
**	environment variable "II_GCC_TRACE" controls the level of
**	trace output, as such:
**		0 only errors
**		1 input events and state transitions
**		2 output events/actions and output calls       
**		3 perform conversion details     		(PL)
**		4 perform conversion dump		        (PL)
**
**	The tracing support consists mostly of TRdisplay's, with constant
**	to symbol mapping provided by gcx_look().
**
** History: $Log-for RCS$
**	22-jun-89 (seiwald)
**	  New header for tracing.
**      24-Aug-89 (cmorris)
**        Spruced up Al tracing, primitive type output.
**      07-Sep-89 (cmorris)
**        Spruced up TL tracing.
**      15-Nov-89 (cmorris)
**        Added tracing of SL actions.
**	12-Feb-90 (cmorris)
**	  Added tracing of protocol driver requests.
**	01-May-90 (cmorris)
**        Added support for network bridge.
**	27-Mar-91 (seiwald)
**	  gcx_init is VOID.
**      09-Jul-91 (cmorris)
**	  changes for protocol bridge.
**	17-aug-91 (leighb) DeskTop Porting Change:
**	  GCXDEBUG is never on for IBM/PC (PMFE).
**	13-Aug-92 (seiwald)
**	  Added conversion ops in the PL.
**	10-oct-92 (leighb) DeskTop Porting Change:
**	  Change PMFE ifdef to PMFEDOS ifdef for GCXDEBUG.
**	25-May-93 (seiwald)
**	    Added tabs() macro and gcx_gcc_eop[].
**	17-Mar-94 (seiwald)
**	    Renamed gcc_tdump() to gcx_tdump().
**      29-Mar-96 (rajus01)
**	  changes for protocol bridge.
**	24-Apr-96 (chech02)
**	  Added gcx_look() function prototypes for windows 3.1 port.
**	 5-Feb-97 (gordy)
**	    Added gcx_gcd_type[] for new internal GCD datatypes.
**	 5-Aug-97 (gordy)
**	    Guard against multiple inclusions.
**	 8-Jan-99 (gordy)
**	    Added gcx_timestamp().
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**      06-07-01 (wansh01) 
**          removed gcxlevel and gcx_init. 
**          trace init is done in gcd_init and gcc_init seperately. 
**	22-Mar-02 (gordy)
**	    Made separate list for GCA atomic types.
**	12-Dec-02 (gordy)
**	    Renamed gcd component to gco.
**      18-Nov-04 (Gordon.Thorpe@ca.com and Ralph.Loen@ca.com)
**          Moved GCC global references from this file into gcc.h, as they are
**          intended for the private use of GCC only.
**/

#ifndef _GCX_INCLUDED_
#define _GCX_INCLUDED_

# ifndef PMFEDOS					 
# define GCXDEBUG	/* always on for now */
# endif

GLOBALREF char tabs8[];

# define tabs( x ) ( tabs8 + 8 - x )

/*
** Name: GCXLIST - map of numbers to symbols
**
** Description:
**	A way of getting from #define'd constants back to their symbolic
**	value.
*/

typedef struct _GCXLIST {
	i4	number;
	char	*msg;
} GCXLIST;


FUNC_EXTERN char 	*gcx_look(GCXLIST *, i4);
FUNC_EXTERN i4		gcx_timestamp( void );
FUNC_EXTERN VOID	gcx_tdump( char *buf, i4 len );
FUNC_EXTERN GCXLIST *   gcx_getGCAMsgMap();

/*
** The various mappings.
*/

GLOBALREF GCXLIST gcx_gca_msg[];	/* gca messages */
GLOBALREF GCXLIST gcx_mde_service[];	/* mde service primitives */
GLOBALREF GCXLIST gcx_mde_primitive[];	/* mde service sub-primitives */
GLOBALREF GCXLIST gcx_atoms[]; 		/* atomic datatypes */
GLOBALREF GCXLIST gcx_datatype[]; 	/* DBMS datatypes */
GLOBALREF GCXLIST gcx_gco_type[]; 	/* GCO datatypes */
GLOBALREF GCXLIST gcx_gca_elestat[];	/* array status for PL */

#endif /* _GCX_INCLUDED_ */

