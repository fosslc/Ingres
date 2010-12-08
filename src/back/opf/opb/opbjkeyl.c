/*
**Copyright (c) 2004, 2010 Ingres Corporation
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
#define             OPB_JKEYLIST        TRUE
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 

/**
**
**  Name: OPBJKEYL.C - create key list of joinop attributes
**
**  Description:
**      This file contains routines which will translate a key list of
**      range variable attributes into a key list of joinop attributes
**
**  History:    
**      3-may-86 (seputis)    
**          initial creation from setordatt.c
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opb_jkeylist(
	OPS_SUBQUERY *subquery,
	OPV_IVARS varno,
	OPO_STORAGE storage,
	RDD_KEY_ARRAY *dmfkeylist,
	OPB_MBF *mbfkl);

/*{
** Name: opb_jkeylist	- create joinop attribute key list
**
** Description:
**	The keylist indicates which attributes the relation is ordered on.
**      This routine will translate a keylist of
**      range variable attributes into a keylist of joinop attributes
**      which is more convenient for optimization.  Only keys which
**      are directly referenced in the query, are placed in the list.  Thus,
**      if no key is referenced directly, then the list will be empty.
**
**      Note, that indexes stored as heap can still be useful.  Imagine wide 
**      tuples with a small indexed key. 
**
** Inputs:
**      varno                           joinop range variable associated with
**                                      keylist
**      storage                         storage structure on which keylist is
**                                      built
**      dmfkeylist                      keylist using system catalog attribute
**                                      numbers
**
** Outputs:
**      mbfkl                           matching boolean factor / keylist
**                                      - the keylist will be initialized
**                                      with the joinop attributes which
**                                      correspond to the DMF attributes
**                                      but the matching boolean factors
**                                      will not be found until a later
**                                      phase
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	3-may-86 (seputis)
**          initial creation from setordatt
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not simply != 0.
[@history_line@]...
*/
VOID
opb_jkeylist(
	OPS_SUBQUERY       *subquery,
	OPV_IVARS	   varno,
	OPO_STORAGE        storage,
	RDD_KEY_ARRAY	   *dmfkeylist,
	OPB_MBF            *mbfkl)
{
    i4                  count;		/* number of element entered into the
                                        ** joinop keylist
                                        */
    i4                  dmfcount;       /* number of attributes in key */
    OPZ_AT		*abase;		/* ptr to base of array of attributes*/

    abase = subquery->ops_attrs.opz_base;
    mbfkl->opb_bfcount = 0;
    mbfkl->opb_keywidth = DB_BPTR;	/* initial width is the TID ptr */

    if (!dmfkeylist)
    {	/* check if dmf keylist exists - it will not for a temporary relation 
	** or for a relation with heap storage structure
	*/
	mbfkl->opb_count = 0;
	return;
    }
    mbfkl->opb_mcount =			/* save number of keys in index
                                        ** so that unique secondaries
                                        ** can be detected */
    dmfcount = dmfkeylist->key_count;
    count = 0;				/* joinop keylist may be empty */
    for ( count = 0; count < dmfcount; count++)
    {	/* look for joinop attribute representing the dmf key and save the
        ** joinop attribute number if found
        */
	OPZ_IATTS	attr;		/* joinop attribute in key */

	if ( (mbfkl->opb_kbase->opb_keyorder[count].opb_attno = attr
		    = opz_findatt( subquery, 
				   varno, 
				   (OPZ_DMFATTR) dmfkeylist->key_array[count] 
                                 )
              ) == OPZ_NOATTR )
	    break;			/* exit if joinop attribute is not
                                        ** in query
                                        */
	mbfkl->opb_keywidth += abase->opz_attnums[attr]->opz_dataval.db_length;
    }
    if ( (storage == DB_HASH_STORE) && (count != dmfcount) )
	mbfkl->opb_count = 0;	    /* 0 if the key cannot be used - all
                                    ** attributes must be present for hash
                                    */
    else
	mbfkl->opb_count = count;

    if (subquery->ops_vars.opv_base->opv_rt[varno]->opv_grv->opv_relation->
	    rdr_rel->tbl_id.db_tab_index > 0) /* if the table is an index 
                                    ** then the attributes in the key array
				    ** will reference the base relation */
	mbfkl->opb_keywidth = subquery->ops_vars.opv_base->opv_rt[varno]->
	    opv_grv->opv_relation->rdr_rel->tbl_width; /* the width of the key
				    ** is the width of the entire tuple for
				    ** a secondary index */
    else if (count < dmfcount)
    {	/* for a primary, the width of the key is the sum of the attributes
	** in the key */
	DMT_ATT_ENTRY	**rdr_attr; /* Pointer to array of pointers to the 
				    ** attribute entry. */
	mbfkl->opb_keywidth = DB_BPTR;	/* initial width is the TID ptr */
	rdr_attr = subquery->ops_vars.opv_base->opv_rt[varno]->opv_grv->
	    opv_relation->rdr_attr;
	for ( ; count < dmfcount; count++)
	{   /* add width of attributes not in query but part of key, so that the
	    ** directory search cost calculation will be accurate */
	    mbfkl->opb_keywidth += rdr_attr[dmfkeylist->key_array[count]]
		->att_width;
	}
    }
}
