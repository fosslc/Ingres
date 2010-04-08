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
#define             OPE_ADDEQCLS        TRUE
#include    <bt.h>
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 

/**
**
**  Name: OPEADDEQ.C - add a joinop attribute to an equivalence class
**
**  Description:
**      Routine will add a joinop attribute to an equivalence class
**
**  History:    
**      19-jun-86 (seputis)    
**          initial creation from addeqcls.c
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/


/*{
** Name: ope_addeqcls	- add an attribute to equivalence class
**
** Description:
**      This routine will add a joinop attribute to an already existing 
**      equivalence class.  If there is an attribute in the equivalence
**      class which belongs to the same range variable as the insertion
**      candidate, then the candidate will not be inserted and the 
**      routine will return FALSE.
**
** Inputs:
**      subquery                        ptr to subquery be analyzed
**      eqcnum                          existing equivalence class index
**      attnum                          joinop attribute to add to the
**                                      equivalence class
**      nulljoin                        TRUE if nulls should be kept in
**                                      any join operation based on this
**                                      equivalence class
**
** Outputs:
**	Returns:
**	    TRUE if attribute was added successfully.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-apr-86 (seputis)
**          initial creation
**	21-dec-88 (seputis)
**          refine NULL join indicator to use regular join if one of the
**	    attributes is non-nullable
**	23-dec-04 (inkdo01)
**	    Add support of OPE_MIXEDCOLL flag for collations.
**	12-jan-05 (inkdo01)
**	    Collation ID of -1 or 0 are equivalent background values.
**	16-aug-05 (inkdo01)
**	    Add support for multi-EQC attributes.
**      05-jan-2009 (huazh01)
**          Remove the restriction which prevents two or more different
**          columns from same range variable to be put into same eqcls.
**          bug 117642.
**      25-feb-2009 (huazh01)
**          Remove the fix to b117642.
**	21-May-2009 (kiria01) b122051
**	    Reduce uninitialised db_collID
[@history_line@]...
*/
bool
ope_addeqcls(
	OPS_SUBQUERY       *subquery,
	OPE_IEQCLS	   eqcnum,
	OPZ_IATTS	   attnum,
	bool               nulljoin)
{

    register OPE_EQCLIST *eqp;	    /* ptr to equivalence class element to which
                                    ** the attribute will be added
				    */
    OPZ_AT		*abase;	    /* ptr to base of joinop attribute table
				    */
    OPZ_ATTS		*attrp;	    /* ptr to attribute descriptor to add
				    */

    eqp = subquery->ops_eclass.ope_base->ope_eqclist[eqcnum]; /* get ptr to 
				    ** equivalence class element
				    */
    abase = subquery->ops_attrs.opz_base; /* ptr to base of attribute table */
    {
	OPV_IVARS	    varno;  /* range variable number of joinop attr
				    ** to be added to equivalence class
				    */
	OPZ_IATTS	    attr;   /* attribute number of a joinop attr
				    ** which is already in the equivalence
				    ** class
				    */
	OPZ_IATTS	    maxattr; /* maximum joinop attribute index
				    ** assigned in subquery
				    */
	DB_COLL_ID	    collID; /* to perform mixed collation check */

	maxattr = subquery->ops_attrs.opz_av;
	attrp = abase->opz_attnums[attnum];
	collID = attrp->opz_dataval.db_collID;
	varno = attrp->opz_varnm;   /* range var of attribute to add to
				    ** equivalence class
				    */
	for (attr = -1; 
	     (attr = BTnext((i4)attr,
			    (char *) &eqp->ope_attrmap, 
			    maxattr)
	     ) >= 0;)
	{
	    /* Perform mixed collation check with rest of eqclass. */
	    if (collID != abase->opz_attnums[attr]->opz_dataval.db_collID &&
		(collID > DB_NOCOLLATION || 
		abase->opz_attnums[attr]->opz_dataval.db_collID > DB_NOCOLLATION))
	    {
		collID = attrp->opz_dataval.db_collID  = DB_UNSET_COLL;
		eqp->ope_mask |= OPE_MIXEDCOLL;
	    }

	    /* cannot merge cause then we would have more than one att from a
	    ** single relation in the same eqclass.  Just keep the eqclasses
	    ** seperate and keep the clause in the query 
	    */
	    if (abase->opz_attnums[attr]->opz_varnm == varno)
		return(FALSE);
	}
    }

    /* the attribute range variable is not indirectly referenced in the
    ** equivalence class so it is safe to add the attribute
    */

    BTset((i4)attnum, (char *) &eqp->ope_attrmap);
    if (attrp->opz_dataval.db_datatype < 0)
    {	/* nullable attribute is being added so NULLs may exist */
	eqp->ope_nulljoin = eqp->ope_nulljoin && nulljoin; /* all equivalence
				    ** class elements need to allow nulls
                                    ** in the join otherwise NULLs will be
				    ** removed */ 
    }
    else
    {	/* non-null attribute added, so all NULLs will be removed */
	eqp->ope_nulljoin = FALSE;  /* reset to do simple join */
    }
    attrp->opz_equcls = eqcnum; /* remember that this att
				    ** has been assigned to a eqclass 
				    */
    BTset((i4)eqcnum, (char *)&attrp->opz_eqcmap); /* and set the 
				    ** multi-EQC map, too */
    if (attrp->opz_attnm.db_att_id == DB_IMTID)
	eqp->ope_eqctype = OPE_TID;   /* if the new att is an
				    ** implicit TID then the whole
				    ** eqclass is now a TID eqclass
				    */
    return (TRUE);
}
