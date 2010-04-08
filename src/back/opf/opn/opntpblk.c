/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>

/* external routine declarations definitions */
#define             OPN_TPBLK           TRUE
#include    <opxlint.h>
/**
**
**  Name: OPNTPBLK.C - number of tuples per block
**
**  Description:
**      Routine to get the number of tuples per block
**
**  History:    
**      15-jun-86 (seputis)    
**          initial creation from tuperbuf.c
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      06-mar-96 (nanpr01)
**          Changed the parameter for multiple page size project. 
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      28-apr-2004 (stial01)
**          Optimizer changes for 256K rows, rows spanning pages
[@history_line@]...
**/

/*{
** Name: opn_tpblk	- estimate number of tuples per block
**
** Description:
**      This routine will estimate the number of tuples per block
**
** Inputs:
**	pagesize			size of the page
**      tuplewidth                      width of tuple in bytes
**
** Outputs:
**	Returns:
**	    number of tuples in a disk block
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-jun-86 (seputis)
**          initial creation from tuperbuf
**      06-mar-96 (nanpr01)
**          Changed the parameter for multiple page size project. 
**	06-may-96 (nanpr01)
**	    Add the tuple header overhead for calculating the number of blocks.
**      12-oct-2000 (stial01)
**          opn_tpblk() fixed tuples per block calculation
[@history_line@]...
*/
OPO_TUPLES
opn_tpblk(
	OPG_CB		   *opg_cb,
	i4		   pagesize,
	OPS_WIDTH          tuplewidth)
{
    OPO_TUPLES		tpblk;
    i4			page_type;

    if (pagesize == DB_COMPAT_PGSIZE)
	page_type = DB_PG_V1;
    else
	page_type = DB_PG_V2;

    tpblk = (OPO_TUPLES)(pagesize - DB_VPT_PAGE_OVERHEAD_MACRO(page_type))/
		(OPO_TUPLES)((tuplewidth + DB_VPT_SIZEOF_LINEID(page_type) 
		+ DB_VPT_SIZEOF_TUPLE_HDR(page_type)));

    /* tpblk should only be fraction if < 1 */
    if (tpblk > 1)
	tpblk = (OPO_TUPLES)(i4)tpblk;

    /* if rows span pages tpblk may be < 1 */
    return ((OPO_TUPLES)tpblk);
}
