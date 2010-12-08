/*
** Copyright (c) 1984, 2004, 2008 Ingres Corporation
*/

#include	<compat.h>
# include	<me.h>		 
# include	<cv.h>		 
#include	<si.h>
#include	<st.h>
#include	<er.h>
#include	<cm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<adf.h>
#ifndef ADE_CHK_TYPE
#define ADE_CHK_TYPE(itype, type) ((itype) == (type) || (itype) == -(type))
#endif
#include	<ade.h>
#include	<fe.h>
#include	<afe.h>
#include	<ooclass.h>
#include	<oslconf.h>
#include	<oserr.h>
#include	<osglobs.h>
#include	<oskeyw.h>

/**
** Name:	oschktype.c -	OSL Parser Check Type Module.
**
** Description:
**	Contains routines that interface with the ADF to determine whether
**	types are legal for certain purposes.  Defines:
**
**	osadf_start()	initialize for ADF.
**	oschktypes()	check types for compatibility.
**	oschkstr()	check string type.
**	oschkop()	check operator for type compatibility.
**	oschkcoerce()	return function instance id for coercion.
**	oschkcmplmnt()	return function instance id for complement.
**	oschkdecl()	check user type specification.
**	ostypeout()	return declaration string for ADF type.
**	ostxtconst()	translate string w.r.t. TEXT string semantics.
**
** History:
**	Revision 6.0  87/01/24  wong
**	Initial revision.
**
**	Revision 6.3/03  89/08  wong
**	Modified 'ostxtconst()' to use 'IIAFpmEncode()'.
**
**	Revision 6.5 92/06  davel
**
**	15-jun-92 (davel)
**		modified oschkstr() to special-check decimal types.
**	30-jul-92 (davel)
**		for ANSI compiler, explicitly cast args in a min() macro to
**		be compatible with regard to signed or unsigned.
**      11-jan-1996 (toumi01; from 1.1 axp_osf port)
**              added dkhor's change for axp_osf
**              20-jan-93 (dkhor)
**              On axp_osf, long is 8 bytes.  Declare osAppid to be 
**              consistently long.
**      23-mar-1999 (hweho01)
**              Extended the change for axp_osf to ris_u64 (AIX
**              64-bit platform).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      08-Sep-2000 (hanje04)
**              Extended the change for axp_osf to axp_lnx (Alpha Linux)
**      06-Mar-2003 (hanje04)
**              Extend axp_osf changes to all 64bit platforms (LP64).
**	24-apr-2003 (somsa01)
**		Use i8 instead of long for LP64 case.
**	21-Jan-2008 (kiria01) b119806
**		Extending grammar for postfix operators beyond IS NULL
**		has removed the need of the _IsNull slots in the operator
**		table.
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	24-Aug-2009 (kschendel) 121804
**	    Add bool externs for gcc 4.3.
*/

#if defined(LP64)
GLOBALREF i8 osAppid;
#else /* LP64 */
GLOBALREF OOID osAppid;
#endif /* LP64 */

/* ADF Control Block */

static ADF_CB	*osl_cb;

/* Operator Name/ID Map */
static struct opmap {
	    const char	*opname;    /* Operator name */
	    ADI_OP_ID	opid;	    /* ADF operator ID */
} opmap[] = {
	{_And,		ADE_AND},
	{_Or,		ADE_OR},
	{_Not,		ADE_NOT},
	{_UnaryMinus,	ADI_NOOP},
	{_Plus,		ADI_NOOP},
	{_Minus,	ADI_NOOP},
	{_Mul,		ADI_NOOP},
	{_Div,		ADI_NOOP},
	{_Exp,		ADI_NOOP},
	{_Less,		ADI_NOOP},
	{_Greater,	ADI_NOOP},
	{_Equal,	ADI_NOOP},
	{_NotEqual,	ADI_NOOP},
	{_LessEqual,	ADI_NOOP},
	{_GreatrEqual,  ADI_NOOP},
	{_Like,		ADI_NOOP},
	{_NotLike,	ADI_NOOP},
	{_Escape,	ADE_ESC_CHAR},
	{_PMEncode,	ADE_PMENCODE},
	NULL
};

/*{
** Name:	osadf_start() -	Initialize for ADF.
**
** Description:
**	Initialize the ADF control block for the OSL parser.
**
** Input:
**	dml	{DB_LANG}  DML language.
**
** History:
**	02/87 (jhw)  Written.
**
**	04-mar-92 (leighb) DeskTop Porting Change:
**		Moved function declaration outside of calling function.
**		This is so function prototypes will work.
*/

VOID
osadf_start(dml)
DB_LANG	dml;
{
    register struct opmap    *mp;

    osl_cb = FEadfcb();
    osl_cb->adf_qlang = dml;

    /* Set-up operator ID map */
    for (mp = opmap ; mp->opname != NULL ; ++mp)
    {
	if (mp->opid == ADI_NOOP)
	{
	    const char	*opname;

	    if (dml != DB_QUEL 
	      ||
		(!osw_compare(mp->opname, _Like) 
		  && !osw_compare(mp->opname, _NotLike)))
	    {
		opname = mp->opname;
	    }
	    else
	    { /* Special support for LIKE and NOTLIKE in OSL/QUEL */
		opname = ( osw_compare(mp->opname, _Like) ) ? _Equal : _NotEqual;
	    }

	    if (afe_opid(osl_cb, opname, &mp->opid) != OK)
	    {
		FEafeerr(osl_cb);
		osuerr(OSBUG, 1, ERx("osadf_start(operators)"));
	    }
	}
    }
}

/*{
** Name:	oschktypes() -	Check Types for Compatibility.
**
** Description:
**	The routine determines whether the input types are type compatibile.
**	That is, whether the right-hand value is implicitly coercible to the
**	left-hand value.
**
** Input:
**	rtype	{DB_DT_ID}  Rvalue type.
**	ltype	{DB_DT_ID}  Lvalue type.
**
** Returns:
**	{bool}  TRUE if types are compatible.
**
** History:
**	02/87 (jhw) -- Written.
*/

bool
oschktypes ( DB_DT_ID rtype, DB_DT_ID ltype )
{
	ADI_FI_ID	junk;

	if (ltype == DB_DMY_TYPE || rtype == DB_DMY_TYPE
	  || ltype == DB_NODT || rtype == DB_NODT)
	{
		return FALSE;
	}

	if ( ADE_CHK_TYPE(rtype, DB_DYC_TYPE) )
		rtype = DB_VCH_TYPE;
	if ( ADE_CHK_TYPE(ltype, DB_DYC_TYPE) )
		ltype = DB_VCH_TYPE;

	if ( adi_ficoerce(osl_cb, rtype, ltype, &junk) == OK )
		return TRUE;
	else if ( osl_cb->adf_errcb.ad_errcode == E_AD2009_NOCOERCION )
		return FALSE;
	else
	{ /* programming error */
		FEafeerr(osl_cb);
		osuerr(OSBUG, 1, ERx("oschktypes()"));		/* exit! */
		/*NOTREACHED*/
	}
}

/*{
** Name:	oschkstr() -	Check String Type.
**
** Description:
**	Checks that a data type is a character type of some sort.  That is,
**	is or is coercible to DB_CHA_TYPE (except for money and date.)
**
** Input:
**	type	{DB_DT_ID}  Type to be checked.
**
** Returns:
**	{bool}	TRUE if type "DB_CHA_TYPE" or coercible to "DB_CHA_TYPE".
**
** History:
**	01/87 (jhw) -- Written.
**	10/88 (jhw) -- Ack!  Special check for date and money.  Watch this
**		when new types are added!
**
**	04-mar-92 (leighb) DeskTop Porting Change:
**		Moved function declaration outside of calling function.
**		This is so function prototypes will work.
**	15-jun-92 (davel)
**		Add DB_DEC_TYPE in above-mentioned special check.
**	15-nov-93 (robf)
**              Add DB_SEC_TYPE in above-mentioned special check.
**	07-sep-06 (gupsh01)
**		Added new datatypes for ANSI date/time types.
**      30-Oct-07 (kiria01) b117790
**          Having introduced more number-string coercions, the assumption
**          below has
**	23-Nov-2010 (gupsh01) SIR 124685
**	    Protype cleanup.
*/
DB_STATUS	adi_ficoerce(ADF_CB *adf_scb,
			     DB_DT_ID	adi_from_did,
			     DB_DT_ID	adi_into_did,
			     ADI_FI_ID  *adi_fid);		 

bool
oschkstr ( DB_DT_ID type )
{
	ADI_FI_ID	junk;

	if ( type == DB_NODT || ADE_CHK_TYPE(type, DB_DMY_TYPE) )
		return FALSE;

	if ( ADE_CHK_TYPE(type, DB_DYC_TYPE) )
		type = DB_VCH_TYPE;

	/* The use of adi_ficoerce to determine whether data is a string or
	** not is nolonger appropriate as many more data types can coerce
	** into strings than just stringes.
	*/
	if ( ADE_CHK_TYPE(type, DB_CHA_TYPE) ||
			adi_ficoerce(osl_cb, type, DB_CHA_TYPE, &junk) == OK )
	{
		/* Disallow money and date and decimal */
		return (bool)(  !ADE_CHK_TYPE(type, DB_MNY_TYPE) &&
				!ADE_CHK_TYPE(type, DB_DTE_TYPE) &&
				!ADE_CHK_TYPE(type, DB_ADTE_TYPE) &&
				!ADE_CHK_TYPE(type, DB_TMWO_TYPE) &&
				!ADE_CHK_TYPE(type, DB_TMW_TYPE) &&
				!ADE_CHK_TYPE(type, DB_TME_TYPE) &&
				!ADE_CHK_TYPE(type, DB_TSWO_TYPE) &&
				!ADE_CHK_TYPE(type, DB_TSW_TYPE) &&
				!ADE_CHK_TYPE(type, DB_TSTMP_TYPE) &&
				!ADE_CHK_TYPE(type, DB_INYM_TYPE) &&
				!ADE_CHK_TYPE(type, DB_INDS_TYPE) &&
				!ADE_CHK_TYPE(type, DB_INT_TYPE) &&
				!ADE_CHK_TYPE(type, DB_FLT_TYPE) &&
				!ADE_CHK_TYPE(type, DB_DEC_TYPE)
			     );
	}
	else if ( osl_cb->adf_errcb.ad_errcode == E_AD2009_NOCOERCION )
	{
		return FALSE;
	}
	else
	{ /* programming error */
		FEafeerr(osl_cb);
		osuerr(OSBUG, 1, ERx("oschkstr()"));		/* exit! */
		/*NOTREACHED*/
	}
}

/*{
** Name:	oschkop() -		Check Operator for Type Compatibility.
**
** Description:
**	Checks whether the operands of the input operator are type
**	compatible (or implicitly coercible to compatible types) by
**	returning a function instance ID for the operator.
**
**	In addition to the function instance ID, this routine fills in
**	an operand description structure with the type information for
**	the operands it expects (which are coercible from the original
**	input operands) and a value description with the type and length
**	for the result.
**
**	This routine calls 'afe_fdesc()' to perform its functionality, except
**	for boolean operators, which it handles directly as a special case.
**
** Input:
**	name	{char *}  The name of the operator.
**	ops	{AFE_OPERS *}  A structure describing the operands.
**
** Output:
**	cops	{AFE_OPERS *}  A structure describing the coerced operands.
**	result	{DB_DATA_VALUE *}  A description of the result type and length.
**
** Returns:
**	{ADI_FI_ID}  The function instance for the operator, operands.
**			ADI_NOFI, if not compatible.
**
** History:
**	02/87 (jhw) -- Written.
**	06/89 (jhw) -- Allow E_AF6008 return from 'afe_fdesc()'.  JupBug #6572.
**	07/23/91 (emerson)
**		Added missing parentheses:
**		else if (... == E_AD2004_BAD_DTID && ... || ...) ==>
**		else if (... == E_AD2004_BAD_DTID && (... || ...))
**		The only effect of the missing parentheses was that
**		internal compiler errors could, in some cases, be erroneously
**		reported as user errors.
*/

ADI_FI_ID
oschkop (char *name, AFE_OPERS *ops, AFE_OPERS *cops, DB_DATA_VALUE *result)
{
    register struct opmap  *mp;
    ADI_OP_ID		    opid;
    ADI_FI_DESC	 	    fdesc;

    /* Lookup operator ID */
    for (mp = opmap ; mp->opname != NULL ; ++mp)
    {
	if (osw_compare(mp->opname, name))
	{
	    opid = mp->opid;
	    break;
	}
    }

    if (mp->opname == NULL && afe_opid(osl_cb, name, &opid) != OK)
    { /* a built-in function, e.g., 'concat()', etc. */
	FEafeerr(osl_cb);
	osuerr(OSBUG, 1, ERx("oschkop(op)"));   /* exit! */
    }

    /* Check operator and return function instance ID */

    if ( opid == ADE_AND  ||  opid == ADE_OR  ||  opid == ADE_NOT )
    { /* Boolean special case */
	if ( (opid == ADE_NOT && ops->afe_opcnt != 1)  ||
		(opid != ADE_NOT && ops->afe_opcnt != 2) )
	{
	    (VOID)afe_error(osl_cb, E_AF6001_WRONG_NUMBER, 0);
	    FEafeerr(osl_cb);
	    osuerr(OSBUG, 1, ERx("oschkop(bool)"));	/* exit! */
	}

	if (!ADE_CHK_TYPE(ops->afe_ops[0]->db_datatype, DB_BOO_TYPE)
	  || (opid != ADE_NOT 
	    && !ADE_CHK_TYPE(ops->afe_ops[1]->db_datatype, DB_BOO_TYPE) ))
	{
	    return ADI_NOFI;
	}

	if (cops != NULL)
	{
	    MEcopy((PTR) ops->afe_ops[0], sizeof(*ops->afe_ops[0]), 
			(PTR) cops->afe_ops[0]
		);
	    if (cops->afe_opcnt > 1)
	    {
		MEcopy((PTR) ops->afe_ops[1], sizeof(*ops->afe_ops[1]),
				(PTR) cops->afe_ops[1]
			);
	    }
	}

	result->db_datatype = DB_BOO_TYPE;
	result->db_length = 1;

	return opid;
    }
    else if ( opid == ADE_ESC_CHAR )
    {
	if ( ops->afe_opcnt != 2 )
	{
	    (VOID)afe_error(osl_cb, E_AF6001_WRONG_NUMBER, 0);
	    FEafeerr(osl_cb);
	    osuerr(OSBUG, 1, ERx("oschkop(escape)"));	/* exit! */
	}

	if ( !ADE_CHK_TYPE(ops->afe_ops[0]->db_datatype, DB_BOO_TYPE)  ||
		!oschkstr(ops->afe_ops[1]->db_datatype) )
	    return ADI_NOFI;

	if ( cops != NULL )
	{
	    MEcopy((PTR) ops->afe_ops[0], 
			sizeof(*ops->afe_ops[0]), (PTR) cops->afe_ops[0]
		);
	    if (cops->afe_opcnt > 1)
		MEcopy((PTR) ops->afe_ops[1], sizeof(*ops->afe_ops[1]),
			(PTR) cops->afe_ops[1]
		);
	}

	result->db_datatype = DB_BOO_TYPE;
	result->db_length = 1;

	return opid;
    }
    else if ( opid == ADE_PMENCODE )
    {
	if ( ops->afe_opcnt != 1 )
	{
	    (VOID)afe_error(osl_cb, E_AF6001_WRONG_NUMBER, 0);
	    FEafeerr(osl_cb);
	    osuerr(OSBUG, 1, ERx("oschkop(pmencode)"));	/* exit! */
	}

	if ( !oschkstr(ops->afe_ops[0]->db_datatype) )
	    return ADI_NOFI;

	if ( cops != NULL )
	{
	    MEcopy((PTR) ops->afe_ops[0], sizeof(*ops->afe_ops[0]), 
			(PTR) cops->afe_ops[0]
		);
	}

	result->db_datatype = ops->afe_ops[0]->db_datatype;
	result->db_length = ops->afe_ops[0]->db_length;

	return opid;
    }
    else if (afe_fdesc(osl_cb, opid, ops, cops, result, &fdesc) != OK)
    {
	if (osl_cb->adf_errcb.ad_errcode == E_AD2022_UNKNOWN_LEN &&
	      ( (ops->afe_opcnt > 0 &&
			ops->afe_ops[0]->db_length == ADE_LEN_UNKNOWN)
	      ||
		(ops->afe_opcnt > 1 &&
			ops->afe_ops[1]->db_length == ADE_LEN_UNKNOWN) )
	   )
	{
	    result->db_length = ADE_LEN_UNKNOWN;
	}
	else if (osl_cb->adf_errcb.ad_errcode == E_AF6006_OPTYPE_MISMATCH  ||
		 osl_cb->adf_errcb.ad_errcode == E_AF6008_AMBIGUOUS_OPID  ||
		    (mp->opname == NULL /* a function */ &&
			osl_cb->adf_errcb.ad_errcode == E_AF6019_BAD_OPCOUNT) )
	{
	    return ADI_NOFI;
	}
	else if (osl_cb->adf_errcb.ad_errcode == E_AD2004_BAD_DTID &&
	      ( (ops->afe_opcnt > 0 &&
			ops->afe_ops[0]->db_datatype == DB_DMY_TYPE)
	      ||
	        (ops->afe_opcnt > 1 &&
			ops->afe_ops[1]->db_datatype == DB_DMY_TYPE) )
	   )
	{
	    return ADI_NOFI;
	}
	else
	{
	    FEafeerr(osl_cb);
	    osuerr(OSBUG, 1, ERx("oschkop()"));  /* exit! */
	}
    }

    return fdesc.adi_finstid;
}

/*{
** Name:    oschkcoerce() -	Return Function Instance ID for Coercion.
**
** Description:
**	This routine returns the function instance for a coercion of the
**	input types from the first to the last.  It assumes that the types
**	have already been checked for coercibility.
**
** Input:
**	from	{DB_DT_ID}  Type for value to be coerced.
**	to	{DB_DT_ID}  Type for resultant value.
**
** Return:
**	{ADI_FI_ID}  Function instance ID for coercion.
**
** History:
**	02/87 (jhw) -- Written.
*/

ADI_FI_ID
oschkcoerce (DB_DT_ID from, DB_DT_ID to)
{
    ADI_FI_ID	fid;

    if (ADE_CHK_TYPE(from, DB_DYC_TYPE))
	from = DB_VCH_TYPE;
    if (adi_ficoerce(osl_cb, from, to, &fid) != OK)
    {
	FEafeerr(osl_cb);
	osuerr(OSBUG, 1, ERx("oschkcoerce()"));  /* exit! */
    }

    return fid;
}

/*{
** Name:    oschkcmplmnt() -	Return Function Instance ID for Complement.
**
** Description:
**	Returns the function instance ID of the complement for
**	the input function instance ID.
**
** Input:
**	fiid	{ADI_FI_ID}  The function instance ID.
**
** Returns:
**	{ADI_FI_ID}  The function instance ID of the complement.
**		     (ADI_NOFI if no complement exists.)
**
** History:
**	02/87 (jhw) -- Written.
*/

ADI_FI_ID
oschkcmplmnt (ADI_FI_ID fiid)
{
    ADI_FI_DESC	*fp;	/* function instance description */

    if (adi_fidesc(osl_cb, fiid, &fp) != OK)
    {
	FEafeerr(osl_cb);
	osuerr(OSBUG, 1, ERx("oschkcmplmnt()"));	/* exit! */
    }

    return fp->adi_cmplmnt;
}

/*{
** Name:    oschkdecl() -	Check User Type Specification.
**
** Description:
**	The OSL parser interface to 'afe_ctychk()', which verifies that the
**	components of a user-entered type specification are correct.  A type
**	specification consists of a type name, length (or precision and scale)
**	and a null indicator.  A referenced DB_DATA_VALUE has its datatype ID
**	and length (or precision and scale) set with internal values
**	corresponding to the input type specification.
**
** Input:
**	name	{char *}  The name of the field or column
**			  (in case there is an error.)
**	type	{char *}  The user-specified type name.
**	len	{char *}  The user-specified length or precision.
**	scale	{char *}  The user-specified scale.
**	null	{char *}  Null indicator (tri-valued logic, can't be bool).
**
** Output:
**	dbv	{DB_DATA_VALUE *}  DB_DATA_VALUE passed by reference.  Sets
**	    .db_datatype    {DB_DT_ID}  The internal datatype ID.
**	    .db_length	    {i4}  The internal data length.
**	    .db_scale	    {nat}  The internal scale when the length
**					specifies a precision.
**
** History:
**	01/87 (jhw) -- Written.
**	12/1991 (jhw) - Removed check for E_AF6011, which hasn't been returned
**		by AFE for a long time.
**	03-oct-2006 (gupsh01)
**	    Added call to IIAMdateAlias for handling the date data type
**	    handling.
**	10-Jul-2007 (kibro01) b118702
**	    Removed IIAMdateAlias since its functionality is in adi_tyid
*/

VOID
oschkdecl (name, type, len, scale, null, dbv)
char		*name;
char		*type;
char		*len;
char		*scale;
char		*null;
DB_DATA_VALUE	*dbv;
{
    DB_USER_TYPE    spec;
    char	    dtemp[11];
    char	    *dp;
    
    STATUS osreclook();

    dbv->db_data = NULL;

    /* Don't look for complex types in QUEL */
    if (len == NULL && scale == NULL && !QUEL && osreclook(type) == OK)
    {
	/* Nullability clause disallowed on complex types. */
	if (null != NULL)
		oswarn(OSNULLDECL, 2, type, null);

	dbv->db_datatype = DB_DMY_TYPE;
	dbv->db_length = dbv->db_prec = 0;

	dbv->db_data = STalloc(type);
	CVlower(dbv->db_data);

	return;
    }

    /* OK, not a record type, so check for an ADF type */

    if (type[ STlcopy(type, spec.dbut_name, DB_TYPE_SIZE - 1) ] != EOS)
	oscerr(OSHIDTYPE, 1, name);

    if (null == NULL)
    {
	/* No nullability declaration.  Use the default. */
    	spec.dbut_kind = (SQL && osCompat > 5) ? DB_UT_NULL : DB_UT_NOTNULL;
    }
    else if (STequal(null, ERx("not null")))
    {
    	spec.dbut_kind = DB_UT_NOTNULL;
    }
    else if (STequal(null, ERx("with null")))
    {
    	spec.dbut_kind = DB_UT_NULL;
    }

    if (afe_ctychk(osl_cb, &spec, len, scale, dbv) != OK)
    {
	DB_STATUS	err = osl_cb->adf_errcb.ad_errcode;

	if (err == E_AD2003_BAD_DTNAME )
	    oscerr(OSHIDTYPE, 1, name);
	else if (err == E_AD2006_BAD_DTUSRLEN || err == E_AD2007_DT_IS_FIXLEN ||
		err == E_AD2008_DT_IS_VARLEN || err == E_AF6013_BAD_NUMBER)
	{
	    oscerr(OSHIDSIZE, 1, name);
	    dbv->db_length = ADE_LEN_UNKNOWN;
	}
	else
	{
	    FEafeerr(osl_cb);
	    osuerr(OSBUG, 1, ERx("oschkdecl()"));		/* exit! */
	}
    }
}

/*{
** Name:    ostypeout() -	Return Declaration String for ADF Type.
**
** Description:
**	Get declaration string for type.
**
** Input:
**	dbv	{DB_DATA_VALUE *}  DB data value describing type.
**
** Output:
**	buf	{DB_USER_TYPE *}
**		dbut_name	{char *}  Declaration string for type.
**
** History:
**	04/87 (jhw) -- Written.
*/

VOID
ostypeout (dbv, buf)
DB_DATA_VALUE	*dbv;
DB_USER_TYPE	*buf;
{
    if (afe_tyoutput(osl_cb, dbv, buf) != OK) 
    {
	FEafeerr(osl_cb);
	osuerr(OSBUG, 1, ERx("ostypeout()"));	/* exit! */
    }
    if (buf->dbut_kind == DB_UT_NULL)
	STcat(buf->dbut_name, ERx(" with null"));
}

/*{
** Name:	ostxtconst() -	Translate String with respect to
**					TEXT String Semantics.
** Description:
**	Applies the semantic rules defined for TEXT strings by the DBMS to an
**	OSL parser string, translating escaped characters and possibly QUEL
**	pattern-matching characters.  Calls 'IIAFpmEncode()' to perform the
**	translation and NULL-terminates the result.
**
** Input:
**	match	{bool}  Whether to translate QUEL pattern-matching characters.
**	str	{char *}  The OSL parser string.
**	output	{char []}  The address of the output buffer.
**	bsize	{nat}  The size of the output buffer.
**
** Output:
**	output	{char []}  The translated string.
**
** History:
**	06/87 (jhw) -- Written to use 'IIAFtxTrans()'.
**	08/89 (jhw) -- Modified to use 'IIAFpmEncode()'.
*/
VOID
ostxtconst ( match, str, output, bsize )
bool	match;
char	*str;
char	output[];
i4	bsize;
{
	register DB_TEXT_STRING		*buffer;
	i4				pm;
	DB_DATA_VALUE			dbvbuf;
	AFE_DCL_TXT_MACRO(DB_MAXSTRING)	_buf;

	buffer = (DB_TEXT_STRING *)&_buf;
	buffer->db_t_count =
		STlcopy( (PTR)str, (PTR)buffer->db_t_text,
			sizeof(_buf) - sizeof(buffer->db_t_count)
		);

	/* Use the buffer as a DB_TXT_TYPE DB_DATA_VALUE. */

	dbvbuf.db_datatype = DB_LTXT_TYPE;
	dbvbuf.db_length = sizeof(_buf);
	dbvbuf.db_data = (PTR)&_buf;

	if ( IIAFpmEncode(&dbvbuf, match, &pm) != OK )
	{
		if (osl_cb->adf_errcb.ad_errcode == E_AD3050_BAD_CHAR_IN_STRING)
			oscerr(OSILLCHAR, 1, ERx("\\0"));
		else
		{
			FEafeerr(osl_cb);
			osuerr(OSBUG, 1, ERx("ostxtconst"));
		}
	}

	MEcopy( (PTR)buffer->db_t_text, 
		(u_i2)min((u_i2)bsize, buffer->db_t_count), (PTR)output
	);
	output[min((u_i2)bsize, buffer->db_t_count)] = EOS;
}
