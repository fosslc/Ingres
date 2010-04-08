/*
**Copyright (c) 2004 Ingres Corporation
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
#define             OPE_NEWEQCLS        TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 

/**
**
**  Name: OPENEWEQ.C - create a new equivalence class
**
**  Description:
**      Routine to create a new equivalence class
**
**  History:    
**      16-jun-86 (seputis)    
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/

/*{
** Name: ope_neweqcls	- create a new equivalence class for joinop attribute
**
** Description:
**      This routine will create a new equivalence class for the joinop 
**      attribute.  It is assumed that the joinop attribute is not already 
**      assigned to an equivalence class.
**
** Inputs:
**      subquery                        ptr to subquery be analyzed
**      attr                          joinop attribute to be placed
**                                      in equivalence class
**
** Outputs:
**	Returns:
**	    equivalence class which was assigned
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-apr-86 (seputis)
**          initial creation
**	24-oct-88 (seputis)
**          init ope_nulljoin field
**	21-dec-88 (seputis)
**          init ope_mask field, check for non-nullable attributes
**	13-may-91 (seputis)
**	    - fix for b37172 - added ope_sargp
**	31-jan-94 (rganski)
**	    Removed ope_sargp, due to change to oph_sarglist.
**	9-aug-05 (inkdo01)
**	    Add support of OJ & EQC bit maps.
**	24-aug-05 (inkdo01)
**	    Drop ope_ojs - problem was solved differently.
**	21-jul-06 (hayke02)
**	    Re-introcude and initialize ope_ojs. This change fixes bug 116406.
[@history_line@]...
*/
OPE_IEQCLS
ope_neweqcls(
	OPS_SUBQUERY	   *subquery,
	OPZ_IATTS          attr)
{
    OPE_IEQCLS          eqcls;	    /* index into equivalence class array */
    OPE_ET              *ebase;	    /* ptr to base of equivalence class array */

    ebase = subquery->ops_eclass.ope_base;
    eqcls = subquery->ops_eclass.ope_ev++; /* find first available unassigned
                                    ** equivalence class, allocate it */
    if (eqcls >= OPE_MAXEQCLS)
	opx_error(E_OP0301_EQCLS_OVERFLOW);

    {	/* eqcls now contains the unassigned equivalence class index */
	OPE_EQCLIST            *eqcls_ptr;

	eqcls_ptr = (OPE_EQCLIST *) opu_memory(subquery->ops_global, 
	    (i4) sizeof(OPE_EQCLIST));
	ebase->ope_eqclist[eqcls] = eqcls_ptr; /* assign memory for the
				    ** equivalence class element */
	MEfill(sizeof(OPZ_BMATTS), (u_char) 0, 
	    (PTR) &eqcls_ptr->ope_attrmap); /* initialize equivalence class 
                                    ** element attribute bit map to  zero */
        eqcls_ptr->ope_bfindex = OPB_NOBF; /* no constant predicates found yet*/
        eqcls_ptr->ope_nbf = OPB_NOBF; /* no sargable predicates found yet */
	eqcls_ptr->ope_nulljoin = TRUE; /* keep nulls in joins unless user 
				    ** explicitly joins two attributes */
	eqcls_ptr->ope_mask = 0;    /* mask of various booleans */
	if (attr != OPZ_NOATTR)
	{   /* update information in attribute to reference equivalence class*/
	    OPZ_ATTS           *attr_ptr;   /* ptr to attribute element to
				    ** be placed in equivalence class
				    */

	    BTset((i4) attr, (char *)&eqcls_ptr->ope_attrmap);  /* set the appropriate
					** bit to indicate attribute */
	    attr_ptr = subquery->ops_attrs.opz_base->opz_attnums[attr];
	    if (attr_ptr->opz_dataval.db_datatype > 0)
	    	/* NULLs will not exist, if this eventually becomes a joining eqcls */
		eqcls_ptr->ope_nulljoin = FALSE;
            eqcls_ptr->ope_eqctype = (attr_ptr->opz_attnm.db_att_id ==DB_IMTID) 
		? OPE_TID : OPE_NONTID; /* set type of this eqclass to
				    ** OPE_TID if the att is an implicit TID */
	    MEfill((u_i2) sizeof(OPL_BMOJ), (u_char) 0,
		(PTR) &eqcls_ptr->ope_ojs); /* init OJ bit map */
	    attr_ptr->opz_equcls = eqcls; /* remember that this att has
				    ** been assigned to an eqclass */
	    BTset((i4)eqcls, (char *)&attr_ptr->opz_eqcmap);
				    /* & set eqc in map (for multi-EQC atts) */
	}
	else
	    eqcls_ptr->ope_eqctype = OPE_NONTID;
    }
    return(eqcls);
}
