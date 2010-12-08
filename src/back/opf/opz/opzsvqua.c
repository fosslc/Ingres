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
#define        OPZ_SAVEQUAL      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>


/**
**
**  Name: OPZSVQUA.C - save qualification of possible function attribute
**
**  Description:
**      Save the qualification of a possible function attribute
**
**  History:    
**      19-jun-86 (seputis)    
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	21-may-93 (ed)
**	    - fix outer join problem, when function attributes use the
**	    result of an outer join
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	10-Mar-1999 (shero03)
**	    support SQL functions with 4 operands
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
static OPZ_FACLASS opz_fclass(
	i4 nodetype,
	OPV_GBMVARS *varmap,
	bool conversion,
	bool iftrue);
void opz_savequal(
	OPS_SUBQUERY *subquery,
	PST_QNODE *andnode,
	OPV_GBMVARS *lvarmap,
	OPV_GBMVARS *rvarmap,
	PST_QNODE *oldnodep,
	PST_QNODE *inodep,
	bool left_iftrue,
	bool right_iftrue);

/*{
** Name: opz_fclass	- predict type of function attribute class
**
** Description:
**      This routine will return a function attribute class which will be
**      used to order the "creation" of function attributes.
**
** Inputs:
**      nodetype                        query tree nodetype of operand
**      varmap                          varmap of function attribute
**      conversion                      TRUE - if conversion is required
**
** Outputs:
**	Returns:
**	    class of function attribute
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-apr-86 (seputis)
**          initial creation
**	5-dec-02 (inkdo01)
**	    Changes for range table expansion.
[@history_line@]...
*/
static OPZ_FACLASS
opz_fclass(
	i4                nodetype,
	OPV_GBMVARS	  *varmap,
	bool               conversion,
	bool		   iftrue)
{
    if (iftrue)
	return(OPZ_MVAR);
    if (nodetype != PST_VAR || conversion)
    {
	if( nodetype == PST_VAR )
	    return(OPZ_TCONV);
	else if ( BTcount((char *) varmap, BITS_IN(OPV_GBMVARS)) == 1)
	    return(OPZ_SVAR);
	else
	    return(OPZ_MVAR);
    }
    else
	return(OPZ_VAR);
}

/*{
** Name: opz_fatts	- make function keys
**
** Description:
**	Take the join claus and replace the left and right subtrees with the
**	varnode representing the result of the datatype conversion or function 
**	computation.
**
**	In the first implementation of joins on functions, I will not try to 
**	find similar functions that will derive non-user specified joins. 
**      That is, if the user specified f(a) = f(b) and f(b) = f(c),
**	I will not derive f(a) = f(c). This can be done at a later date.
**
**      It is assumed that the left and right varmaps of the conditional
**      are not equal (should have been previously checked).  It is also
**      assumed that a straightforward join of two vars with identical
**      types is not possible (this was also previously checked).
**
** Inputs:
**      subquery                        ptr to subquery being processed
**      eqnode                          ptr to conditional query tree node
**                                      i.e. PST_BOP "=" node
**      lvarmap                         left varmap of conditional
**      rvarmap                         right varmap of conditional
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-apr-86 (seputis)
**          initial creation
**	26-mar-90 (seputis)
**	    fix function attribute bug with nullable types
**	22-mar-92 (seputis)
**	    fix b43226 - function attribute bug with nullable types
**	13-apr-92 (seputis)
**	    fix b43569 - save original null join qual if available
**	7-sep-93 (robf)
**	    Add support for security label/id joins.
**	6-may-94 (ed)
**	    - null joins were not reconstructing the qualifications completely
**	    if the equivalence class insertions failed
**	    - this will be more common for nested outer joins in which
**	    null joins need to be used for relation placement
**	5-dec-02 (inkdo01)
**	    Changes for range table expansion.
**	12-july-06 (dougi)
**	    Remove consistency check that caused OP0389 error. This runs
**	    afoul of new implicit integer coercion support.
**	28-Aug-08 (kibro01) b120849
**	    Allow combinations of TSxx and TMxx to be directly compared, since
**	    adf allows this without explicit coercion, and uses the same data
**	    structure within these sets of data types.
**	19-Nov-2010 (kiria01) SIR 124690
**	    Add propagation of collID for UCS_BASIC support.
**
*/
VOID
opz_savequal(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          *andnode,
	OPV_GBMVARS        *lvarmap,
	OPV_GBMVARS        *rvarmap,
	PST_QNODE	   *oldnodep,
	PST_QNODE	   *inodep,
	bool		   left_iftrue,
	bool		   right_iftrue
	/* bool            savequal; */)
{
    /* extract qualification for potential function attributes, the
    ** qualification may be restored if it is determined that creating a
    ** function attribute would not be useful.  This approach will allow
    ** "obvious join" qualifications to be removed first, and function
    ** attributes to be created on a second pass if necessary.  Information
    ** is placed in the function attribute element to describe the left and
    ** right sides of the qualification.
    */
    OPZ_FATTS                *funcattr;	    /* ptr to function attribute element
                                            ** to being created */
    funcattr = opz_fmake(subquery, andnode, OPZ_QUAL);   /* find an empty
					    ** function attribute slot */
    funcattr->opz_left.opz_nulljoin = oldnodep;	/* save ptr to OR part of null
					    ** join if applicable */
    funcattr->opz_right.opz_nulljoin = inodep; /* save ptr to other OR part
					    ** of null join */
    MEcopy((char *)rvarmap, sizeof(OPV_GBMVARS),
		(char *)&funcattr->opz_right.opz_varmap);
    if (right_iftrue)
	funcattr->opz_right.opz_opmask |= OPZ_OPIFTRUE;
    MEcopy((char *)lvarmap, sizeof(OPV_GBMVARS),
		(char *)&funcattr->opz_left.opz_varmap);
    if (left_iftrue)
	funcattr->opz_left.opz_opmask |= OPZ_OPIFTRUE;
#if 0
/*  not needed since strength reduction solves this problem */
    if (savequal)
	funcattr->opz_mask |= OPZ_SOJQUAL;  /* mark qualification as being
					    ** required for outer join semantics */
#endif

    {
	/* find the correct datatype for the comparison */
        PST_QNODE              *eqnode; /* "=" node for conjunct */
	OPS_DTLENGTH	       llen;    /* length of left operand before
                                        ** conversion
                                        */
        OPS_DTLENGTH           rlen;    /* length of right operand before
                                        ** conversion
                                        */
	DB_DT_ID	       ltype;	/* ADT of left operand */
	DB_DT_ID               rtype;	/* ADT of right operand */
	ADI_FI_DESC	       *adi_fdesc;  /* ptr to function instance
					** description of conversion
					*/
	DB_DT_ID               result;  /* target type for function attr */
	DB_COLL_ID		collID;
        eqnode = andnode->pst_left;
	ltype = abs(eqnode->pst_left->pst_sym.pst_dataval.db_datatype);
	rtype = abs(eqnode->pst_right->pst_sym.pst_dataval.db_datatype); /* the
                                        ** datatype is negative if it is
                                        ** a nullable attribute, but this
                                        ** needs to be ignored for these
                                        ** calculations, however the
                                        ** nullability must be retained
                                        ** if a function attribute is
                                        ** eventually created, to avoid
                                        ** coercing a nullable type into
                                        ** a non-nullable type */

	llen = eqnode->pst_left->pst_sym.pst_dataval.db_length;
	rlen = eqnode->pst_right->pst_sym.pst_dataval.db_length;
        adi_fdesc = eqnode->pst_sym.pst_value.pst_s_op.pst_fdesc; /* get
					** function instance description of
                                        ** equality operation */
        if ((ltype != adi_fdesc->adi_dt[0]) || (rtype != adi_fdesc->adi_dt[1]) )
	{   /* There used to be a consistency check here to assure that 
	    ** the adi_dt[] are equal. This was an artifact of that fact
	    ** that char/int compares were done in money (?!?). Char/int
	    ** are now mutually coercible, so the check has been eliminated.
            */
	    result = adi_fdesc->adi_dt[0];
	}
	else
	{   /* pick the prefered type amongst the intrinsics
            ** FIXME - should make sure that the semantics of converting
            ** and comparing are the same as ADF comparing directly, or
            ** there is a possibility of queries returning non-deterministic
            ** results e.g. i4 compare to f4 will cause i4 to be converted
            ** to f4, but then some significant could be lost in the conversion
            ** - another thought here is the transitive property assumed by
            ** joinop does not necessarily hold if the string types are
            ** different, so care must be taken if equivalence classes
            ** will contain differing types e.g.
            **     r.text = s.text AND r.text=t.c
            ** will not be the same as
            **     t.c = r.text AND t.c = s.text
            **
            ** in SQL char and varchar, this would not be a problem since the
            ** comparison semantics are the same, for now convert type of the
            ** shorter attribute
	    */
	    switch (ltype)
	    {
	    case DB_CHA_TYPE:
	    case DB_VCH_TYPE:
	    {	/* conversion to SQL string types have preference */
		switch (rtype)
		{
		case DB_VCH_TYPE:
		case DB_CHA_TYPE:	/* choose type with shorter 
                                        ** length to convert*/
                    if (rlen < llen)
			result = ltype;
		    else
			result = rtype;
		    break;
		case DB_CHR_TYPE:
		case DB_TXT_TYPE:	/* convert to SQL type */
		    result = ltype;
		    break;
		default:
# ifdef E_OP0381_FUNCATTR
		    opx_error(E_OP0381_FUNCATTR); /* unexpected type */
		    /* float or integer should convert to money ? */
# endif
		}
		break;
	    }
	    case DB_CHR_TYPE:
	    {	/* left side is a quel string type */
		switch (rtype)
		{
		case DB_VCH_TYPE:
		case DB_CHA_TYPE:	/* SQL types have preference */
		case DB_CHR_TYPE:    /* ltype == rtype */
		    result = rtype;
		    break;
		case DB_TXT_TYPE:
		    result = DB_CHR_TYPE;
		    break;
		default:
# ifdef E_OP0381_FUNCATTR
		    opx_error(E_OP0381_FUNCATTR); /* unexpected type */
# endif
		}
		break;
	    }
	    case DB_TXT_TYPE:
	    {	/* left side is a quel string type */
		switch (rtype)
		{
		case DB_VCH_TYPE:   /* SQL types have preference */
		case DB_CHA_TYPE:   /* SQL types have preference */
		case DB_CHR_TYPE:   /* CHR has preference over TXT */
		case DB_TXT_TYPE:   /* ltype == rtype */
		    result = rtype;
		    break;
		default:
# ifdef E_OP0381_FUNCATTR
		    opx_error(E_OP0381_FUNCATTR); /* unexpected type */
# endif
		}
		break;
	    }
	    case DB_FLT_TYPE:
	    {
		switch (rtype)
		{
		case DB_FLT_TYPE:
		case DB_INT_TYPE:
		    result = DB_FLT_TYPE;
		    break;
		default:
# ifdef E_OP0381_FUNCATTR
		    opx_error(E_OP0381_FUNCATTR); /* unexpected type */
# endif
		}
		break;
	    }
	    case DB_INT_TYPE:
	    {
		switch (rtype)
		{
		case DB_INT_TYPE:
		    result = DB_INT_TYPE;
		    break;
		case DB_FLT_TYPE:
		    result = DB_FLT_TYPE;
		    break;
		default:
# ifdef E_OP0381_FUNCATTR
		    opx_error(E_OP0381_FUNCATTR); /* unexpected type */
# endif
		}
		break;
	    }
	    default:
	    {	
/* Add in the possibility that the types are not exactly the same, but are
** directly equivalent, and therefore not being coerced explicitly, such as
** the different timestamp types.  (kibro01) b120849
*/
#define TS_TYPE(a) ((a == DB_TSWO_TYPE) || (a == DB_TSW_TYPE) || (a == DB_TSTMP_TYPE))
#define TM_TYPE(a) ((a == DB_TMWO_TYPE) || (a == DB_TMW_TYPE) || (a == DB_TME_TYPE))
		if (ltype == rtype) /* this should handle intrinsic types */
		    result = rtype;
		else if (TS_TYPE(ltype) && TS_TYPE(rtype))
		    result = DB_TSW_TYPE;
		else if (TM_TYPE(ltype) && TM_TYPE(rtype))
		    result = DB_TMW_TYPE;
# ifdef E_OP0381_FUNCATTR
		else
		    opx_error(E_OP0381_FUNCATTR);
# endif
		break;
	    }
	    }
	}
	if ((	eqnode->pst_left->pst_sym.pst_dataval.db_datatype < 0
		||
		eqnode->pst_right->pst_sym.pst_dataval.db_datatype < 0
	    )
	    &&
	    (result >= 0)
	    )
	    result = -result;		    /* since one of the
					    ** operands is a nullable type,
                                            ** the nullability needs to be
                                            ** retained for any function
                                            ** attribute which is created */
	funcattr->opz_dataval.db_datatype = result; /* save the result type,
					    ** - the length is not calculated
                                            ** until it can be determined that
                                            ** this function attribute is
                                            ** useful (see opz_rdatetype) */
	/* Any L and/or R collIDs will have been combined and set in place
	** on the eqnode. It just needs propagating. */
	funcattr->opz_dataval.db_collID = eqnode->pst_sym.pst_dataval.db_collID;
	/* classify the function attribute operands */
	funcattr->opz_left.opz_class = opz_fclass( 
	    eqnode->pst_left->pst_sym.pst_type, 
	    lvarmap,
	    ltype != abs(result),
	    left_iftrue);
	funcattr->opz_right.opz_class = opz_fclass(
	    eqnode->pst_right->pst_sym.pst_type,
	    rvarmap,
	    rtype != abs(result),
	    right_iftrue);
    }
}    
