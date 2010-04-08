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
#define             OPB_BFKGET          TRUE
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 

/**
**
**  Name: OPBBFKGET.C - get key info given equivalence class and datatype
**
**  Description:
**      routines to get proper boolean factor keyinfo ptr given the
**      equivalence class and the datatype
**
**  History:
**      23-may-86 (seputis)    
**          initial creation
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
[@history_line@]...
**/

/*{
** Name: opb_bfkget	- get key info ptr from boolean factor
**
** Description:
**      Each boolean factor has a key list for each (equivalence class, type,
**      length) triple.  This routine finds the key list for the requested
**      triple.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      bp                              boolean factor which contains the
**                                      keylist
**      eqcls                           equivalence class of requested key
**      datatype                        type and length of requested key
**	histdt				type and length of histogram for
**					datatype
**      mustfind                        consistency check parameter - TRUE
**                                      if the key is expected to be in list
**
** Outputs:
**	Returns:
**	    OPB_BFKEYINFO ptr to key list header with proper equivalence class
**          datatype and length
**	Exceptions:
**	    none
**
** Side Effects:
**	    If mustfind is TRUE then an internal exception is generated if
**          the keyinfo ptr cannot be found
**
** History:
**	23-may-86 (seputis)
**          initial creation
**	25-apr-94 (rganski)
**	    Added histdt parameter, which represents the type and length of the
**	    histogram for the given datatype. If this is not null, and the
**	    histogram type is DB_CHA_TYPE, this function now checks for a match
**	    against the histdt length in addition to the previous checks.  This
**	    is necessary because of the fix for b59982, which builds an
**	    OPB_BFKEYINFO structure for each character column of over 8
**	    characters; this was done because two long character columns can
**	    have the same type and length but have different histogram lengths,
**	    as a result of the character histogram enhancements.
**	03-Oct-2008 (kiria01) b120966
**	    Added DB_BYTE_TYPE support to allow for Unicode CE entries
**	    to be treated as raw collation data alongside DB_CHA_TYPE.
[@history_line@]...
*/
OPB_BFKEYINFO *
opb_bfkget(
    OPS_SUBQUERY       *subquery,
    OPB_BOOLFACT       *bp,
    OPE_IEQCLS         eqcls,
    DB_DATA_VALUE      *datatype,
    DB_DATA_VALUE      *histdt,
    bool               mustfind)
{
    OPB_BFKEYINFO       *bfkeyp;      /* used to traverse list of OPB_BFKEYINFO
                                      ** ptrs associated with bp */
    DB_DT_ID            type;         /* type of key to look for */
    i4                  length;       /* length of key to look for */

/* JRBCMT -- Should probably look for prec here too.			    */
/* JRBNOINT -- Can't integrate this change.				    */
    type = datatype->db_datatype;
    length = datatype->db_length;
    /* traverse the list of keylist header structures until the one with
    ** the values (eqcls, type, length) is found */
    for (bfkeyp = bp->opb_keys; bfkeyp; bfkeyp = bfkeyp->opb_next)
	if ((bfkeyp->opb_eqc == eqcls) && 
            (bfkeyp->opb_bfdt->db_datatype == type) && 
            (bfkeyp->opb_bfdt->db_length == length))
	{
	    if (histdt &&
		(histdt->db_datatype == DB_CHA_TYPE ||
		 histdt->db_datatype == DB_BYTE_TYPE) &&
		histdt->db_length != bfkeyp->opb_bfhistdt->db_length)
		/* Character type histogram, but it doesn't match in length.
		** Continue searching through the keylist headers */
		continue;
	    else
		return(bfkeyp);
	}

#ifdef    E_OP0481_KEYINFO
    if (mustfind)   /* cannot find boolean factor keyinfo ptr */
	opx_error(E_OP0481_KEYINFO);
#endif
    return(NULL);
}
