/*
** Copyright (c) 1987, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>

#include    <gcccl.h>
#include    <cv.h>		 
#include    <cvgcc.h>
#include    <me.h>
#include    <qu.h>
#include    <tm.h>
#include    <tr.h>

#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <gca.h>
#include    <gcocomp.h>
#include    <gcc.h>
#include    <gccpci.h>
#include    <gccpl.h>
#include    <gccgref.h>

/*
**
**  Name: gccalcv.c - perform data conversion for the AL
**
**  These routines exist to do the work of perform_conversion in
**  special cases by the GCC AL.  These special cases should be
**  removed.
**
**  External entry points:
**	GCn{get,add}{nat,long,s} - get and put data in NTS
**
**  History:
**      12-Feb-90 (seiwald)
**	    Collected from gccpl.c and gccutil.c.
**	14-Mar-94 (seiwald)
**	    Extracted from the defunct gccpdc.c.
**	24-mar-95 (peeje01)
**	    Integrate doublebyte changes from 6500db_su4_us42
**	20-Nov-95 (gordy)
**	    Added prototypes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-Dec-02 (gordy)
**	    Renamed gcd component to gco.
**	12-May-03 (gordy)
**	    Changed frag parameter in gco_do_16src() to return fragment
**	    type using DOC status.
**	20-Apr-04 (gordy)
**	    CV header files cleaned up.  NAT/LONGNAT replaced by I4.
**	22-Jan-10 (gordy)
**	    DOUBLEBYTE code generalized for multi-byte charset processing.
*/


/*
** Name: GCn{get,add}{nat,long,s} - get and put data in NTS
**
** Description:
**
**	GCngetnat - get a i4  from NTS stream
**	GCnaddnat - put a i4  into NTS stream
**	GCngetlong - get a i4 from NTS stream
**	GCnaddlong - put a i4 into NTS stream
**	GCngets - get a string from NTS stream
**	GCnadds - put a string into NTS stream
**
** 	These routines convert between C datatypes and NTS (network 
**	transfer syntax). 
**
** Inputs:
**	src	pointer to source C datatype (add)
**		pointer to NTS stream (get)
**	dst	pointer to destination C datatype (get)
**		pointer to NTS stream (add)
**	len	length of source string (add)
**		length of destination string (get)
** Output:
**	status	failure status - set only upon failure!
**
** Returns:
**	number of source bytes used (get)
**	number of destination bytes used (add)
**
** History:
**	26-Oct-89 (seiwald)
**	    Written.
**	15-Feb-90 (seiwald)
**	    Rebuild upon CV macros.
**	6-Apr-92 (seiwald)
**	    Added support for doublebyte mappings, using macros for
**	    speed.
**	24-mar-95 (peeje01)
**	    Integrate doublebyte changes from 6500db_su4_us42
**	20-Nov-95 (gordy)
**	    Added prototypes.
**	12-May-03 (gordy)
**	    Changed frag parameter in gco_do_16src() to return fragment
**	    type using DOC status.
**	22-Jan-10 (gordy)
**	    New multi-byte charset processing routine replaces double-byte.
**	    Character set translation handling moved to gco.  Added IDENTITY 
**	    mappings, single-byte charset processing routine.
*/

i4
GCngetnat( char *src, i4  *dst, STATUS *status )
{
	CV2L_I4_MACRO( src, dst, status );
	return CV_N_I4_SZ;
}

i4
GCnaddnat( i4  *src, char *dst, STATUS *status )
{
	CV2N_I4_MACRO( src, dst, status );
	return CV_N_I4_SZ;
}

i4
GCngetlong( char *src, i4 *dst, STATUS *status )
{
	CV2L_I4_MACRO( src, dst, status );
	return CV_N_I4_SZ;
}

i4
GCnaddlong( i4 *src, char *dst, STATUS *status )
{
	CV2N_I4_MACRO( src, dst, status );
	return CV_N_I4_SZ;
}

i4
GCnadds( char *src, i4 len, char *dst )
{
    u_i1		*usrc = (u_i1 *)src;
    u_i1		*udst = (u_i1 *)dst;
    u_i1		*odst = (u_i1 *)dst;
    i4			delta = 0;
    GCC_CSET_XLT	*xlt = (GCC_CSET_XLT *)IIGCc_global->gcc_cset_xlt;
	
    while( len > 0 )  (*xlt->xlt.l2n.xlt)( xlt->xlt.l2n.map, TRUE,
					   &usrc, len, &udst, 32767,
					   &len, NULL, &delta );

    return( udst - odst );
}

i4
GCngets( char *src, i4  len, char *dst )
{
    u_i1		*osrc = (u_i1 *)src;
    u_i1		*usrc = (u_i1 *)src;
    u_i1		*udst = (u_i1 *)dst;
    i4			delta = 0;
    GCC_CSET_XLT	*xlt = (GCC_CSET_XLT *)IIGCc_global->gcc_cset_xlt;
	
    while( len ) (*xlt->xlt.n2l.xlt)( xlt->xlt.n2l.map, TRUE,
				      &usrc, len, &udst, 32767, 
				      &len, NULL, &delta );

    return( usrc - osrc );
}

