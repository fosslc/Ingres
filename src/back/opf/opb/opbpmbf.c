/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
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
#define             OPB_PMBF            TRUE
#include    <me.h>
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 


/**
**
**  Name: OPBPMBF.C - find matching boolean factors for primary relations
**
**  Description:
**      These routines will find the matching boolean factors for the primary 
**      relations.
**
**  History:    
**      07-may-86 (seputis)    
**          initial creation from chk_index
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opb_pmbf(
	OPS_SUBQUERY *subquery);

/*{
** Name: opb_pmbf	- find matching boolean factors for primary relations
**
** Description:
**      This routine will search the boolean factors and find those which 
**      apply to the attributes ordering the respective base relations
**      (the matching boolean factors).  This information will be used to
**      determine if an index is useful.  
**
**      In other words this routine will initialize the "opv_mbf" structure
**      for each primary relation
**        
**      The primary relations may have matching boolean factors which are 
**      TID equalities.  These clauses will take priority, and will preclude
**      the use of any index on the base relation.  In 4.0, a performance
**      bug is that if a TID equality is used then all the indexes would
**      be selected for evaluation, even if the index did not contain
**      any attributes in the query.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    all primaries will have the opv_mbf component of the range
**          variable initialized.
**
** History:
**	07-may-86 (seputis)
**          initial creation
**	30-apr-04 (inkdo01)
**	    Changed DB_TID_LENGTH to DB_TID8_LENGTH for partitioned tables.
[@history_line@]...
*/
VOID
opb_pmbf(
	OPS_SUBQUERY       *subquery)
{
    OPV_IVARS		primary;	/* joinop range table number of base
                                        ** relation being analyzed
                                        */
    OPV_IVARS           lastprimary;    /* offset of the last primary relation
                                        ** in the joinop range table
                                        */
    OPV_RT              *vbase;         /* ptr to base of array of ptrs to
                                        ** joinop range table elements
                                        */

    vbase = subquery->ops_vars.opv_base;/* get ptr to base of array of ptrs to 
                                        ** joinop range variables
                                        */
    lastprimary = subquery->ops_vars.opv_rv; /* number of primary relations
                                        ** defined in the joinop range table
                                        */
    for ( primary = 0; primary < lastprimary; primary++)
    {	/* traverse the primary relations and find matching boolean factors */
	OPV_VARS               *rvp;	/* ptr to range variable being analyzed
                                        */
	rvp = vbase->opv_rt[primary];
	if (rvp->opv_primary.opv_tid != OPE_NOEQCLS)
	{   /* check if a joinop attribute has been created for the tid of the
            ** relation ... if so then there is a possibility of performing
            ** a keyed lookup on the TID (which should be the most restrictive).
            ** If this is true then do not search for boolean factors on
            ** relation.  Moreover, do not use any indexes on the base relation.
            ** At this point in the anaylsis a joinop TID attribute should only
            ** be created if it is explicitly referenced in the query
            */

	    /* create a keylist with which to search for boolean factors on
            ** the tid
            */
	    OPB_KA	    keyorder;

	    rvp->opv_mbf.opb_kbase = &keyorder;
	    rvp->opv_mbf.opb_kbase->opb_keyorder[0].opb_attno 
		= opz_findatt(subquery, primary, DB_IMTID);
	    rvp->opv_mbf.opb_kbase->opb_keyorder[0].opb_gbf = OPB_NOBF;
	    rvp->opv_mbf.opb_count = 1;	 /* number of attributes in ordering */
            rvp->opv_mbf.opb_bfcount = 0; /* number of attributes in ordering
                                        ** with matching boolean factors */
	    (VOID) opb_mbf( subquery, (OPO_STORAGE) DB_HASH_STORE, 
		&rvp->opv_mbf);         /* the storage structure is not really
                                        ** defined for a TID lookup but
                                        ** use hash since it will for an
                                        ** "equality"
                                        */
	    if (rvp->opv_mbf.opb_bfcount)
	    {
		rvp->opv_mbf.opb_kbase = (OPB_KA *)opu_memory(
		    subquery->ops_global, (i4)sizeof(OPB_ATBFLST));
		STRUCT_ASSIGN_MACRO(keyorder.opb_keyorder[0],
		    rvp->opv_mbf.opb_kbase->opb_keyorder[0]);
		rvp->opv_mbf.opb_keywidth = DB_TID8_LENGTH;
		continue;		/* do not use any indexes and do not
                                        ** check any more clauses since the
                                        ** most restrictive clause has been
                                        ** found
                                        */
	    }
	}

	{   /* get matching boolean factors on the primary storage structure */
	    OPO_STORAGE        storage; /* storage structure which base relation
                                        ** is ordered on
                                        */
            RDD_KEY_ARRAY      *rdr_keys; /* key array for primary relation */
	    OPB_KA	       keyorder; /* temp used to calculate attributes
					** used in ordering */

	    rvp->opv_mbf.opb_kbase = &keyorder;
            if (rvp->opv_grv->opv_relation)
	    {
		rdr_keys = rvp->opv_grv->opv_relation->rdr_keys;
		if (rdr_keys)
		{
		    if (rdr_keys->key_count > DB_MAXKEYS)
			rvp->opv_mbf.opb_kbase 
			    = (OPB_KA *)opu_memory( subquery->ops_global,
				(i4)(rdr_keys->key_count*sizeof(OPB_ATBFLST)));
		    else
			rvp->opv_mbf.opb_kbase = &keyorder;
		}
	    }
	    else
		rdr_keys = NULL;	/* temporary relation */
	    storage = rvp->opv_tcop->opo_storage;
	    opb_jkeylist( subquery, primary, storage, rdr_keys, &rvp->opv_mbf);
					/* obtain a keylist based on the
					** var nodes 
					** which reference attributes in the
					** key for the storage structure
					*/
	    (VOID) opb_mbf( subquery, storage, &rvp->opv_mbf); /* get all the
                                    ** the matching boolean factors which
                                    ** apply to the keylist
                                    */
	    if ((rvp->opv_mbf.opb_kbase == &keyorder)
		&&
		(rvp->opv_mbf.opb_count > 0)
	       )
	    {	/* place stack structure into heap memory only if ordering
		** is useful for this query */
		rvp->opv_mbf.opb_kbase 
		    = (OPB_KA *)opu_memory( subquery->ops_global,
			(i4)(rvp->opv_mbf.opb_count * sizeof(OPB_ATBFLST)));
		MEcopy ((PTR) &keyorder, 
		    rvp->opv_mbf.opb_count * sizeof(OPB_ATBFLST),
		    (PTR)rvp->opv_mbf.opb_kbase);
	    }
	}
    }
}
