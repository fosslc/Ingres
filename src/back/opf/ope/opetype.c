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
#define             OPE_TYPE            TRUE
#include    <bt.h>
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 
/**
**
**  Name: OPETYPE.C - get datatype of equivalence class
**
**  Description:
**      routine to return datatype of the equivalence class 
**
**  History:    
**      13-jun-86 (seputis)    
**          initial creation from newatt.c
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/

/*{
** Name: opn_numeric	- test for numeric datatype
**
** Description:
**      Returns TRUE if a numeric datatype is discovered (float or integer)
**
** Inputs:
**      type                            type to be tested for numeric
**
** Outputs:
**	Returns:
**	    TRUE if type is float or integer
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-jun-86 (seputis)
**          initial creation from numtype
[@history_line@]...
*/
static opn_numeric(
	    DB_DT_ID		type)
{
    return(type == DB_INT_TYPE || type == DB_FLT_TYPE);
}

/*{
** Name: ope_type	- get datatype of equivalence class
**
** Description:
**	Get attribute from the eqclass with the greatest length if
**	the types are the same or if FLOATs and INTs, pick the 
**	FLOAT.
**
**	this only works for interior nodes, not original nodes
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      eqcls                           equivalence class
**
** Outputs:
**	Returns:
**	    ptr to DB_DATA_VALUE datatype of the equivalence class
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-jun-86 (seputis)
**          initial creation from newatt
**	4-nov-97 (inkdo01)
**	    Don't get mad at the dopey OPE_SPATJ eqclass (it can be 
**	    heterogeneous).
[@history_line@]...
*/
DB_DATA_VALUE *
ope_type(
	OPS_SUBQUERY       *subquery,
	OPE_IEQCLS         eqcls)
{
    OPZ_BMATTS          *attrmap;	/* ptr to bit map of attributes in the
                                        ** equilvalence class */
    OPZ_IATTS           attr;           /* current attribute of equivalence
                                        ** class being analyzed */
    OPZ_AT              *abase;		/* ptr to base of array of ptrs to
                                        ** joinop attributes */
    DB_DATA_VALUE       *bestdtp;	/* ptr to data type of attribute with
                                        ** largest width in equivalence class
                                        ** map */
    DB_DT_ID            besttype;	/* best type found so far */
    OPZ_IATTS           maxattr;        /* number of attributes defined in
                                        ** subquery */
    OPE_EQCLIST		*eqcp;		/* ptr to eqcls structure */

    bestdtp = NULL;
    eqcp = subquery->ops_eclass.ope_base->ope_eqclist[eqcls];
    attrmap = &eqcp->ope_attrmap;
    maxattr = subquery->ops_attrs.opz_av; /* number of joinop attributes defined
					*/
    abase = subquery->ops_attrs.opz_base; /* ptr to base of array of ptrs to
                                        ** joinop attributes */
    for (attr = -1; (attr = BTnext(attr, (char *)attrmap, (i4)maxattr)) >= 0;)
    {
	DB_DATA_VALUE          *datatypep; /* ptr to datatype of current
                                        ** attribute element being tested */
	DB_DT_ID               type;	/* type of next attribute being tested
                                        */
	    
	datatypep = &abase->opz_attnums[attr]->opz_dataval; /* datatype of
                                        ** this attribute */
	type = abs(datatypep->db_datatype); /* save type of new attribute */
	if (!bestdtp)
	{   /* if this is the first then use it */
	    bestdtp = datatypep;
	    besttype = type;
	}
	else
	{
	    if ((   (type == DB_FLT_TYPE)   
		    && 
		    (besttype == DB_INT_TYPE)
		)
		||	
		(   (type == besttype || eqcp->ope_mask & OPE_SPATJ)
		    && 
		    (datatypep->db_length > bestdtp->db_length)
		))
	    {	/* new attribute has higher priority type */
		bestdtp = datatypep;
		besttype = type;
	    }
#ifdef    E_OP0383_TYPEMISMATCH
	    if ((abs(datatypep->db_datatype) != abs(bestdtp->db_datatype) )
		&& !(eqcp->ope_mask & OPE_SPATJ) &&
		(!opn_numeric(type) || !opn_numeric(besttype)))
		/* newatt: bad types:%d, %d */
		opx_error(E_OP0383_TYPEMISMATCH);
#endif
	}
    }
#ifdef    E_OP0384_NOATTS
    if (!bestdtp)
	/* newatt: nothing in att map, %d */
	opx_error(E_OP0384_NOATTS);
#endif
    return(bestdtp);
}


