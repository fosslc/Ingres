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
#define        OPN_BFKHGET      TRUE
#include    <opxlint.h>


/**
**
**  Name: OPNBFKHGET.C - get keyinfo structure given equivalence class
**
**  Description:
**      Routines to get the keyinfo structure given the equivalence class
**      and available histogram
**
**  History:    
**      22-may-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/

/*{
** Name: opn_bfkhget	- get keyinfo structure given histogram and eqcls
**
** Description:
**	Get the opb_bfkeyinfo struct that corresponds to the given
**	eqclass and its associated type and length as found among the histos 
**      in relations histogram list.  Note, that for any particular relation
**      there will be a set of equivalences classes associated with that
**      relation.  For each equivalence class there will be exactly one
**      or no histograms associated with that equivalence class.
**      Thus, given the equivalence class
**      and the list of histograms, the histogram associated with the
**      equivalence class can be determined.  For every histogram there is
**      an original joinop attribute associated with it.  The type and length
**      associated with the joinop attribute will be used to obtain the keylist
**      from the boolean factor with the same type and length.  
**        
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      bp                              ptr to boolean factor which has the
**                                      keylists
**      eqc                             equivalence class from which a key
**                                      element is required - there may be
**                                      several keylists in bp associated
**                                      with this equivalence class
**      hp                              list of histograms associated with
**                                      relation
**
** Outputs:
**      hpp                             get histogram associated with 
**                                      equivalence class if this ptr is not
**                                      NULL
**	Returns:
**	    OPB_BFKEYINFO ptr of key associated with (equivalence class and
**          histogram pair).  There will only one histogram in this list with
**	    this equivalence class - get that histogram and use the type and 
**          length associated with the histogram and the input equivalence
**          class to obtain the key from the boolean factor.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-may-86 (seputis)
**          initial creation
**	25-apr-94 (rganski)
**	    b59982 - added histdt parameter to call to opb_bfkget(). See
**	    comment in opb_bfkget() for more details.
{@history_line@}
[@history_line@]...
*/
OPB_BFKEYINFO *
opn_bfkhget(
	OPS_SUBQUERY	   *subquery,
	OPB_BOOLFACT       *bp,
	OPE_IEQCLS         eqc,
	OPH_HISTOGRAM      *hp,
	bool               mustfind,
	OPH_HISTOGRAM      **hpp)
{
    OPH_HISTOGRAM       *thp;	    /* histogram associated with equivalence
                                    ** class eqc */
    OPZ_ATTS            *attrp;     /* ptr to joinop attribute associated with
                                    ** histogram thp
                                    */
    OPB_BFKEYINFO       *bfkeyp;    /* ptr to struct with proper equivalence
                                    ** class, type and length characteristics
                                    */
    OPH_HISTOGRAM       **thpp;     /* used in case NULL is returned */
    if (!(thpp = opn_eqtamp(subquery, &hp, eqc, mustfind)) )
	return (NULL);
    thp = *thpp;
    attrp = subquery->ops_attrs.opz_base->opz_attnums[thp->oph_attribute]; /* 
                                ** get attribute pointer associated with
                                ** histogram */
    if ((bfkeyp = opb_bfkget(subquery, bp, eqc, &attrp->opz_dataval,
			     &attrp->opz_histogram.oph_dataval, mustfind))
	== NULL)
	return (NULL);
    if (hpp)			/* return this info if requested */
	*hpp = thp;
    return (bfkeyp);
}
