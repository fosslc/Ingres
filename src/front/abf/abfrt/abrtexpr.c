/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<me.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>
#include	<ade.h>
#include	<afe.h>
#include	<frmalign.h>
#include	<abfcnsts.h>
#include	<fdesc.h>
#include	<abfrts.h>

/**
** Name:	abrtexpr.c -	ABF Run-Time System Expression Evaluation Module
**
** Description:
**	Contains routines used to evaluate expressions for the ABF run-time
**	system using ADF, and to set-up DBDVs used in expressions.  Defines:
**
**	IIARevlExprEval()
**	IIARasnDbdvAssign()
**	IIARtocToCstr()		convert DBDV string to C string.
**	IIARrcsResetCstr()
**	IIARhf2HidFldInitV2()	initialize hidden fields.
**	IIARhffHidFldFree()	deallocate hidden fields.
**	IIARnliNullInit()	initialize constant null DB_DATA_VALUE.
**	IIARivlIntVal()		get an int value, or default if null.
**
** History:
**	Revision 6.0  87/07  arthur
**	Initial revision.
**
**	Revision 6.1  88/09  wong
**	Added support for tagged memory allocation when string size exceeds
**	available buffer space to avoid overwriting memory.
**
**	Revision 6.3/03/00
**	11/27/90 (emerson)
**		Added function IIARhftHidFldTagset (for bug 34663).
**
**	Revision 6.3/03/01
**	01/14/91 (emerson)
**		Create new function IIARhf2HidFldInitV2 (replacing
**		IIARhfiHidFldInit) for bug 34840.
**	02/17/91 (emerson)
**		Changed IIARhfiHidFldInit to convert the FDESCV2's
**		from version 2 to version 3 format (bug 35946; see fdesc.h).
**	03/21/91 (emerson)
**		Fixed a bug (36599) in my last change to IIARhfiHidFldInit.
**
**	Revision 6.4/02
**	11/07/91 (emerson)
**		Part of fix for bugs 39581, 41013, and 41014:
**		Removed no-longer-needed function IIARhftHidFldTagset, and
**		removed calls to no-op functions iiarCifInitFields and
**		iiarCffFreeFields in IIARhf2HidFldInitV2 and IIARhffHidFldFree.
**
**	Revision 6.5
**	21-dec-92 (davel)
**		use IIARgglGetGlobal() and IIARcglCopyGlobal() instead of
**		obsolete iiarGetGlobal().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Delete unnecessary declarations that just clog up the aux-info.
*/

STATUS	IIARhf2HidFldInitV2();
FUNC_EXTERN	STATUS	IIARgglGetGlobal(char *name, DB_DATA_VALUE **gdbv,
						i4 *kind);
FUNC_EXTERN	STATUS	IIARcglCopyGlobal(DB_DATA_VALUE *gdbv, 
					  DB_DATA_VALUE *ldbv,
					  char *globname, char *localname );

/*{
** Name:	IIARevlExprEval() -	Evaluate an expression at runtime
**
** Description:
**	Evaluates an expression at runtime.
**
** Inputs:
**	inst		The ADF instructions to be executed.
**	last_offset	The offset (in bytes) of the last ADF instruction.
**	frmalign	Information about alignment restrictions and version.
**	dbdvs		The array of DB_DATA_VALUEs to be operated upon.
**	result		The DB_DATA_VALUE in which to put the logical result
**			of the expression.  Such a DB_DATA_VALUE has a 1-byte
**			integer as its data area.  This argument is NULL if
**			the expression being evaluated is NOT a logical expr.
**
** History:
**	16-mar-1987 (agh)  First written.
**	10-oct-1988 (jhw)  Check for user errors and do not report as
**				internal error.
**	11/89 (jhw) -- Modified to allocate control block once only.  SIR 6366.
*/

/* ADE Execution Control Block
**
** Define aligned space for an ADE_EXCB structure,
** including the real size of the 'excb_bases' member.
*/
static ALIGN_RESTRICT	_excb[(sizeof(ADE_EXCB) + ADE_DVBASE * sizeof(PTR) - 1)
				/ sizeof(ALIGN_RESTRICT) + 1] = {0};

static ADE_EXCB	*excb = (ADE_EXCB *)_excb;

DB_STATUS
IIARevlExprEval(inst, last_offset, frmalign, dbdvs, result)
i2		*inst;
i4		last_offset;
FRMALIGN	*frmalign;
DB_DATA_VALUE	*dbdvs;
DB_DATA_VALUE	*result;
{
	DB_STATUS	db_stat;
	ADF_CB		*cb;
	i1		*ip;
	i4		offsets[6];	/* 1st and last offsets into
					   CX's 3 segments */
	cb = FEadfcb();
	if ( excb->excb_cx == NULL )
	{ /* Initialize the ADE_EXCB structure */
		i4	cxhdsize;	/* size of an ADE_CXHEAD structure */

		/*
		** Allocate and initialize an ADE_CXHEAD structure.  Since
		** this is an internal ADE structure, information about it
		** must be requested via calls to 'afc_cxinfo()'.
		*/
		if ( afc_cxinfo( cb, (PTR)NULL, ADE_09REQ_SIZE_OF_CXHEAD,
				(PTR)&cxhdsize ) != OK
			|| (excb->excb_cx = MEreqmem( 0, cxhdsize, TRUE,
							(STATUS *)NULL) )
				== NULL )
		{
			abproerr(ERx("IIARevlExprEval"), OUTOFMEM, (char*)NULL);
		}

		excb->excb_seg = ADE_SMAIN;
		excb->excb_nbases = ADE_DVBASE + 1;
		/*
		** 'excb_value' is an output member of the ADE_EXCB.
		** It is set, when the expression returns a logical
		** value, by 'afc_execute_cx()'.
		*/
		excb->excb_value = ADE_NOT_TRUE;
	}
	/*
	** The offsets array contains the 1st and last offsets into the
	** 3 instruction segments of a CX.  The ABF RTS is, in fact, only
	** concerned with the cx_mi_last_offset for the MAIN segment.
	** This is the offset (in bytes) of the last instruction in the
	** main segment of the compiled expression.
	*/
	offsets[0] = offsets[1] = offsets[2] = 0;
	offsets[3] = (i4) last_offset;
	offsets[4] = offsets[5] = 0;
	afc_cxhead_init(cb, excb->excb_cx, &(frmalign->afc_veraln), offsets);

	excb->excb_bases[ADE_CXBASE] = (PTR) inst;
	excb->excb_bases[ADE_DVBASE] = (PTR) dbdvs;

	/*
	** Set the logical result value to NOT_TRUE in case of error.
	*/
	if (result != NULL)
	{
		ip = (i1 *) result->db_data;
		*ip = (i1) ADE_NOT_TRUE;
	}

	if ((db_stat = afc_execute_cx(cb, excb)) != OK)
	{
		FEafeerr(cb);
		if ( cb->adf_errcb.ad_errcode != E_AD1012_NULL_TO_NONNULL &&
				cb->adf_errcb.ad_errclass != ADF_USER_ERROR )
			abusrerr(ABEXPREVAL, (char *) NULL);
	}
	else if (result != NULL)
	{
		ip = (i1 *) result->db_data;
		*ip = (i1) (excb->excb_value == ADE_TRUE);
	}

	return db_stat;
}

/*{
** Name:	IIARasnDbdvAssign() -	Assign a value to DB_DATA_VALUE
**
** Description:
**	Assigns a value to a DB_DATA_VALUE at runtime by calling the
**	specified ADF conversion routine.  In ADF, an assignment between
**	2 DB_DATA_VALUEs of the same type is also treated as a coercion.
**
** Inputs:
**	result		The DB_DATA_VALUE for the result (left-hand side
**			of the assignment statement).
**	value		The value being assigned (right-hand side).
**	instd		The function instance id for the coercion.
**
** History:
**	24-mar-1987 (agh)  First written.
**	10-oct-1988 (jhw)  Check for user errors and do not report as
**				internal error.
*/
VOID
IIARasnDbdvAssign ( result, value, instd )
register DB_DATA_VALUE	*result;
register DB_DATA_VALUE	*value;
i4			instd;
{
	register ADF_CB		*cb;

	cb = FEadfcb();
	/*
	** If the DB_DATA_VALUE being assigned contains the empty string,
	** its "length" will be 0.
	** If the result DB_DATA_VALUE is nullable, use a temp DB_DATA_VALUE
	** to make its data value the non-NULL "empty" value.
	*/
	if (value->db_length == 0)
	{
		if (AFE_NULLABLE_MACRO(result->db_datatype))
		{
			DB_DATA_VALUE	temp;

			MEcopy((PTR)value, sizeof(DB_DATA_VALUE), (PTR)&temp);
			AFE_UNNULL_MACRO(&temp);
			_VOID_ adc_getempty(cb, &temp);
			_VOID_ afe_cvinto(cb, &temp, result);
		}
		else
		{
			adc_getempty(cb, result);
		}
	}
	else
	{
		AFE_OPERS		ops;
		DB_DATA_VALUE		*op1 = value;

		ops.afe_opcnt = 1;
		ops.afe_ops = &op1;
		if ( afe_clinstd(cb, (ADI_FI_ID)instd, &ops, result) != OK )
		{
			FEafeerr(cb);
			if ( cb->adf_errcb.ad_errcode
					!= E_AD1012_NULL_TO_NONNULL &&
				  cb->adf_errcb.ad_errclass != ADF_USER_ERROR )
				abusrerr(ABEXPREVAL, (char *)NULL);
		}
	}
}

/*
** Static buffer used in converting string values within DB_DATA_VALUES to
** EOS-terminated C strings.  See 'IIARtocToCstr()' and 'IIARrcsResetCstr()'.
*/
static char	str_buf[FE_PROMPTSIZE * 2] ZERO_FILL;
static char	*str_ptr = str_buf;
static u_i4	str_tag = 0;

/*{
** Name:	IIARtocToCstr() -	Convert a DB_DATA_VALUE to a C String.
**
** Description:
**	Converts a character string value stored in a DB_DATA_VALUE into a
**	EOS-terminated C string.  Uses a static buffer that is known to be
**	large enough to hold either (1) a reasonable message or prompt string,
**	or (2) a series of 4GL names.  Each 4GL name would become a string
**	passed to an FRS routine and would have a maximum length of FE_MAXNAME.
**	Several such strings can be stored in the static buffer.
**
**	Strings that are too large to fit in the static buffer are
**	allocated in seperate tagged memory with concomitant cost.
**
**	The buffer is reset via 'IIARrcsResetCstr()' and any tagged memory
**	freed.
**
** Inputs:
**	val	{DB_DATA_VALUE *}  The DB_DATA_VALUE that is a string type
**			(e.g., DB_CHR_TYPE, DB_TXT_TYPE.)
**
** Returns:
**	{char *}  A reference to the EOS-terminated C string, which is
**		stored within the static buffer or tagged memory.
**
** History:
**	26-mar-1987 (agh)
**		First written.
**	20-sep-1988  (jhw)  Added support for tagged memory allocation when
**		string size exceeds available buffer.
*/
char *
IIARtocToCstr ( val )
DB_DATA_VALUE	*val;
{
	DB_DATA_VALUE		dbdv;
	DB_EMBEDDED_DATA	embed;
	i2			nullind = 0;
	ADF_CB			*cb;

	/*
	** IIftrim() fills in its output DB_DATA_VALUE (which will be non-
	** Nullable) with the trimmed equivalent of the input DB_DATA_VALUE.
	*/
	if ( IIftrim(val, &dbdv) != OK )
		return NULL;

	cb = FEadfcb();
	embed.ed_data = (PTR) str_ptr;
	embed.ed_null = &nullind;
	/*
	** adh_dbtoev() sets the ed_type and ed_length members of
	** the DB_EMBEDDED_DATA struct.
	** adh_dbcvtev() then converts the DB_DATA_VALUE to a DB_EMBEDDED_DATA,
	** whose ed_data string is NULL-terminated.
	*/
	if ( adh_dbtoev(cb, &dbdv, &embed) != OK )
	{
		FEafeerr(cb);
		return NULL;
	}
	if ( embed.ed_length + 1 > sizeof(str_buf) - (str_ptr - str_buf) )
	{ /* allocate space for this particular string */
		if ( str_tag == 0 )
			str_tag = FEgettag();

		if ( (embed.ed_data = FEreqmem(str_tag, embed.ed_length + 1,
					FALSE, (STATUS *)NULL)) == NULL )
			IIUGbmaBadMemoryAllocation( ERx("IIARtocToCstr()") );
	}
	if ( adh_dbcvtev(cb, &dbdv, &embed) != OK )
	{
		FEafeerr(cb);
		return NULL;
	}
	if ( str_ptr == embed.ed_data )
		str_ptr += STlength(embed.ed_data) + 1 /* EOS */;

	if (nullind == 0)
	{
		return (char *)(embed.ed_data);
	}
	else
	{
		/*
		** User error:  Null value used for an 4GL name.
		** Issue an error message here, and send an impossible
		** value back so that the FRS (or whoever is using
		** the string) can issue their own error message.
		*/
		abusrerr(ABNULLNAME, (char *) NULL);
		return ERx("$NULL$ERROR");
	}
}

/*{
** Name:	IIARrcsResetCstr() - Reset the static buffer for C strings
**
** Description:
**	Resets the static buffer used in 'IIARtocToCstr()' to convert a
**	character string value stored in a DB_DATA_VALUE into a EOS-terminated
**	C string.  Also, frees tagged memory for strings too large to fit
**	within the static buffer.
**
** History:
**	26-mar-1987 (agh)
**		First written.
**	20-sep-1988 (jhw)  Free tagged memory.
*/
IIARrcsResetCstr()
{
	str_ptr = str_buf;
	if ( str_tag != 0 )
		FEfree( str_tag );
}

/*{
** Name:	IIARhf2HidFldInitV2() -	Initialize hidden fields
**
** Description:
**	Initializes hidden fields in an OSL frame or procedure
**	by setting each to the (non-NULL) "empty" data value for its
**	particular datatype.
**
** Inputs:
**	fdesc		The FDESC (pseudo-symbol table) for this frame.
**	dbdvarr		The array of DB_DATA_VALUEs for the frame.
**	global		Initialize complex objects as if they were global
**			(give them a permanent storage tag).  Useful
**			for static frames.  Note that this is a i4, not a bool
**			(because this fuction is called by generated code,
**			and oslhdr.h defines i4  but not bool).
**
** History:
**	3-jun-1987 (agh)
**		First written.
**	3-aug-1989 (billc)  Added support for globals (including constants.)
**	01/14/91 (emerson)
**		Create new function IIARhf2HidFldInitV2 (replacing
**		IIARhfiHidFldInit) for bug 34840.
**		This new function is identical to the old except that
**		it takes a third parameter, which is passed to
**		iiarCidInitDbdv.  The old function is retained
**		in support of applications compiled for an image
**		under an old release.
**	02/17/91 (emerson)
**		Changed IIARhfiHidFldInit to convert the FDESCV2's
**		from version 2 to version 3 format (bug 35946; see fdesc.h).
**	03/21/91 (emerson)
**		Fixed a bug (36599) in my last change to IIARhfiHidFldInit:
**		FEreqmem was being called with a tag of 0 (i.e. the current tag)
**		which means the new FDESCV2 will be freed at exit from the
**		calling frame or procedure, but the FDESC stays updated
**		(including the pointer to the freed FDESCV2).
**		The fix is to get our own private memory tag, which should
**		ensure that the FDESCV2 persists for the life of the application
**		Also added call to IIUGbmaBadMemoryAllocation if FEreqmem failed
**		Also added check to skip converting FDESCV2 if we've already
**		done so on an earlier call to IIARhfiHidFldInit.
**	11/07/91 (emerson)
**		Removed call to no-op function iiarCifInitFields.
*/
static	TAGID	permtag = 0;

STATUS
IIARhfiHidFldInit (fdesc, dbdvarr)
FDESC		*fdesc;
DB_DATA_VALUE	*dbdvarr;
{
	/*
	** IIARhfiHidFldInit is called from code generated by
	** the release 6.0 thru 6.3/03/00 OSL compiler.
	** If the code was generated by the release 6.0 thru 6.3/02
	** OSL compiler, the FDESCV2's are in version 2 format (FDESCV2_V2).
	** If the code was generated by the release 6.3/03/00
	** OSL compiler, the FDESCV2's are in version 3 format (FDESCV2),
	** but fdsc_version in the FDESC is unfortunately still 2.
	** If the FDESCV2's are in version 2 format, we need to convert
	** them to version 3 format and set the FDSC_60_6302 bit in the FDESC
	** (to help fix bug 35798), but we don't want to execute this
	** conversion logic if they are already in version 3 format
	** (stepping through the array of FDESCV2's would yield garbage,
	** except in the case where the only FDESCV2 is the dummy entry).
	**
	** To distinguish between a version 2 and a version 3 array of FDESCV2's
	** the logic below depends on the following observations:
	** (1) For all 6.0 thru 6.3/03/00 OSL compilers, fdsc_cnt in the FDESC
	**     contains the number of entries in the FDESCV2 array,
	**     including the dummy entry at the end.
	** (2) For all 6.0 thru 6.3/03/00 OSL compilers, the dummy entry
	**     at the end of the FDESCV2 array contains zero values,
	**     i.e. NULL pointers, '\0' characters, and 0 integers.
	** (3) NULL pointers, '\0' chars, and 0 integers are all represented
	**     by binary zeroes.  (Actually, I'm not certain that NULL
	**     pointers and 0 integers are binary zeroes on all conceivable
	**     platforms, but our code seems to assume it, e.g. when we
	**     ask FEreqmem to allocate a structure initialized to zeroes
	**     and then assume that any pointers within the structure are NULL).
	** (4) For the 6.3/03/00 OSL compiler, the fields
	**     fdv2_name, fdv2_tblname, fdv2_visible, and fdv2_typename
	**     are guaranteed to be non-zero in all FDESCV2 entries
	**     except the last one (the dummy entry).
	** (5) For any plausible layout of the FDESCV2_V2 and FDESCV2
	**     structures in storage, (2), (3), and (4) imply that if
	**     we look at where the last FDESCV2_V2 (the dummy entry) should be,
	**     we'll see zero values if and only if the FDESCV2's
	**     are in fact in version 2 format.  The only exception
	**     is the case where the only FDESCV2 is the dummy one:
	**     we'll see zero values even if the FDESCV2's are in version 3
	**     format.  In that case, it won't hurt to do the conversion logic,
	**     except that we'll erroneusly set the FDSC_60_6302 bit.
	** (6) The code below assumes that no exception will occur no matter
	**     what garbage bit patterns are being compared.  While this
	**     assumption may not be true for all platforms, it does seem
	**     to be true for VAX/VMS, which is the only platform to which
	**     6.3/03/00 was released.
	*/
	if ( fdesc->fdsc_version < FD_VERSION )
	{
		FDESCV2    *nfp;
		FDESCV2_V2 *ofp;
		i4 fcount = fdesc->fdsc_cnt;	/* # of FDSECV2_V2's, including
						** the dummy entry at the end */

		fdesc->fdsc_version = FD_VERSION;
		ofp = (FDESCV2_V2 *) fdesc->fdsc_ofdesc + fcount - 1;

		if (    ofp->fdv2_name    == NULL
		     && ofp->fdv2_tblname == NULL
		     && ofp->fdv2_visible == '\0'
		     && ofp->fdv2_order   == 0
		     && ofp->fdv2_dbdvoff == 0 )
		{
			ofp = (FDESCV2_V2 *) fdesc->fdsc_ofdesc;
		
			if (permtag == 0)
			{
				permtag = FEgettag();
			}
			nfp = (FDESCV2 *) FEreqmem(permtag,
						(sizeof(FDESCV2) * fcount),
						TRUE, (STATUS*)NULL);
			if (nfp == (FDESCV2 *)NULL)
			{
				IIUGbmaBadMemoryAllocation(
						ERx("IIARhfiHidFldInit") );
			}
			fdesc->fdsc_ofdesc = (OFDESC *) nfp;
		
			while (fcount > 0)
			{
				nfp->fdv2_name     = ofp->fdv2_name;
				nfp->fdv2_tblname  = ofp->fdv2_tblname;
				nfp->fdv2_visible  = ofp->fdv2_visible;
				nfp->fdv2_order    = ofp->fdv2_order;
				nfp->fdv2_dbdvoff  = ofp->fdv2_dbdvoff;
				nfp->fdv2_flags    = 0;
				nfp->fdv2_typename = ERx("");
				nfp++;
				ofp++;
				fcount--;
			}
			(nfp-1)->fdv2_typename = NULL;
			fdesc->fdsc_version = FD_VERSION | FDSC_60_6302;
		}
	}

	return IIARhf2HidFldInitV2 (fdesc, dbdvarr, (i4)FALSE);
}

STATUS
IIARhf2HidFldInitV2 (fdesc, dbdvarr, global)
FDESC		*fdesc;
DB_DATA_VALUE	*dbdvarr;
i4		global;
{
	register FDESCV2	*fp;
	ADF_CB			*adf_cb;

	adf_cb = FEadfcb();
	for ( fp = (FDESCV2*)fdesc->fdsc_ofdesc ; fp->fdv2_name != NULL ; ++fp )
	{
		register DB_DATA_VALUE	*dbdv;

		if (fp->fdv2_visible == 'v')
			continue; 

		/* An 'h' (hidden), a 'g' (global), or a 'c' (constant) */

		dbdv = dbdvarr + fp->fdv2_dbdvoff;

		if ( fp->fdv2_visible == 'h' )
		{ /* hidden field */
			if (dbdv->db_datatype == DB_DMY_TYPE)
			{
				if (iiarCidInitDbdv(fp->fdv2_typename, 
						dbdv, (bool)global) != OK)
				{
					return FAIL;
				}
			}
			else
			{
				iiarInitDbv(adf_cb, dbdv);
			}
		}
		else if ( fp->fdv2_visible == 'g' || fp->fdv2_visible == 'c' )
		{ /* global variable or constant */

			DB_DATA_VALUE	*gdbv;

			/* get the global, which is ready to run. */
			if ( IIARgglGetGlobal( fp->fdv2_name, &gdbv, 
						(i4 *)NULL) != OK
			 ||  IIARcglCopyGlobal( gdbv, dbdv, fp->fdv2_name,
						fp->fdv2_typename) != OK
			   )
			{
				return FAIL;
			}
		}
		else
		{ /* unknown type */
			return FAIL;
		}
	}
	return OK;
}

/*{
** Name:	IIARhffHidFldFree() -	De-allocate hidden fields
**
** Description:
**	deallocates hidden fields in an OSL frame or procedure.
**	Only (currently) applies to complex types, whose descriptors and
**	data areas are deallocated.
**
** Inputs:
**	fdesc		The FDESC (pseudo-symbol table) for this frame.
**	dbdvarr		The array of DB_DATA_VALUEs for the frame.
**
** History:
**	10-1989 (billc) written.
**	11/07/91 (emerson)
**		Removed call to no-op function iiarCffFreeFields.
*/
STATUS
IIARhffHidFldFree(fdesc, dbdvarr)
FDESC		*fdesc;
DB_DATA_VALUE	*dbdvarr;
{
	return OK;
}

/*{
** Name:	iiarInitDbv() -	Initialize a DB_DATA_VALUE
**
** Description:
**	Initializes a DB_DATA_VALUE by setting it to the (non-NULL) "empty" 
**	data value for its particular datatype.
**
** Inputs:
**	adf_cb		The ADF_CB.
**	dbv		The DB_DATA_VALUE to initialize.
**
** History:
**	7-24-1989 (billc)
**		Abstracted from IIARhfiHidFldInit.
*/
iiarInitDbv (adf_cb, dbv)
ADF_CB *adf_cb;
DB_DATA_VALUE	*dbv;
{
	/*
	** If dbv is nullable, use a temp DB_DATA_VALUE
	** to make its data value the non-NULL "empty" value.
	*/
	if (AFE_NULLABLE_MACRO(dbv->db_datatype))
	{
		DB_DATA_VALUE		temp;

		MEcopy((PTR)dbv, sizeof(DB_DATA_VALUE), (PTR)&temp);
		AFE_UNNULL_MACRO(&temp);
		_VOID_ adc_getempty(adf_cb, &temp);
		_VOID_ afe_cvinto(adf_cb, &temp, dbv);
	}
	else
	{
		adc_getempty(adf_cb, dbv);
	}
}

/*{
** Name:	IIARnliNullInit() -	Initialize Constant Null DB_DATA_VALUE.
**
** Description:
**	Initializes the passed in DB_DATA_VALUE, which should be the constant
**	Null DB_DATA_VALUE (i.e., a nullable DB_LTXT_TYPE) to Null by setting
**	the data area and calling 'adc_getempty()'.
**
** Inputs:
**	dbdv	{DB_DATA_VALUE *}  The constant Null DB_DATA_VALUE.
**
** History:
**	5-jun-1987 (agh)
**		First written.
*/
VOID
IIARnliNullInit (dbdv)
DB_DATA_VALUE	*dbdv;
{
	/*
	** Note:  Even though the data area will get initialized
	** by the 'adc_getempty()' call, its value is constant.
	*/
	static DB_TEXT_STRING	data = {0, 0};

	dbdv->db_data = (PTR)&data;
	adc_getempty(FEadfcb(), dbdv);
}

/*{
** Name:	IIARivlIntVal() -	Get integer value from DB_DATA_VALUE
**
** Description:
**	Gets an integer value from the dbv, unless the dbv is null.  If the
**	dbv is null we print an error message and return the default.
**
** Inputs:
**	dbvr		The DB_DATA_VALUE for the integer.
**	default_val	{i4} the default value;
**	msg		{char *} a context-describing string to pass to the
**				error msg if a null if found.
**
** History:
**	11-1989 (billc)
**		First written.
*/
i4
IIARivlIntVal (dbv, default_val, msg)
DB_DATA_VALUE	*dbv;
i4	default_val;
char 	*msg;
{
	i4		i4var;
	DB_DATA_VALUE	ldbv;

	if (AFE_NULLABLE(dbv->db_datatype))
	{
		i4 itsnull;

		adc_isnull(FEadfcb(), dbv, &itsnull);
		if (itsnull != 0)
		{
			IIUGerr(E_AR0061_nullint, 0, 1, msg);
			return default_val;
		}
	}
	ldbv.db_data = (PTR) &i4var;
	ldbv.db_length = sizeof(i4);
	ldbv.db_datatype = DB_INT_TYPE;
	afe_cvinto(FEadfcb(), dbv, &ldbv);
	return (i4) i4var;
}
