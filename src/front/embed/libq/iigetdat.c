/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<generr.h>
# include	<adf.h>
# include	<gca.h>
# include	<iicgca.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<adh.h>
# include	<erlq.h>
# include	<fe.h>

/**
+*  Name: iigetdata.c - Interface routine to IIcgc_get_value.
**
**  Defines:
**	IIgetdata	- Extract data from GCA into embedded data value.
-*
**  History:
**	17-oct-1986	- Written. (neil)
**	26-aug-1987 	- Modified to use GCA. (ncg)
**	12-dec-1988	- Generic error processing. (ncg)
**	19-may-1989	- Changed global vars for multiple connections. (bjb)
**	15-oct-1990 (barbara)
**	    Fixed bug 20010.  Don't drop out of retrieve loops on errors
**	    that are actually only warnings.
** 	08-feb-1996 	- Added flags argument to iigetdata() to support
**               	  compressed varchars. (loera01)
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	03-apr-2001 (abbjo03)
**	    Null terminate DB_NCHR_TYPEs.
**	15-May-2004 (kodse01)
**		BUG# 112321
**		For tid column, its length check is avoided. 
**	aug-2009 (stephenb)
**		Prototyping front-ends
**/


/*{
+*  Name: IIgetdata - Get next data value in a RETRIEVE or FETCH.
**
**  Description:
**	Gets the next column from the data area.  Using the passed 
**	DB value we load the ED variable and execute any conversions needed.
**
**	Note that for string variables, if the data returned is longer
**	than the space allocated to the variable this routine will overwrite
**	the following variables.
**
**  Inputs:
**	thr_cb		Thread-local-storage control block.
**	dbv		- Describes whats about to be read.
**	  .db_datatype	- Data type in communication block.
**	  .db_length	- Length of that data.
**	edv 		- Pointer to place to read into.
**	  .ed_type	- Type to be converted to.
**	  .ed_length	- Length of user's data (if zero and char types then
**			  assume maximum).
**	colnum		- Column number for errors.
**
**  Outputs:
**	dbv		- Describes whats about to be read.
**	  .db_data	- After this routine will point at a volatile address
**			  into which data was read.
**	edv
**	    .ed_data	- New value retrieved.
**	    .ed_null	- Null indicator (if used).
**	Returns:
**	    i4  - 0 - Fail
**		  1 - Success
**	Errors:
**	    Conversion errors.
-*
**  Side Effects:
**	1. Sets dbv->db_data to be whatever it decides to use.  This should
**	   not be used by the caller.
**	
**  History:
**	17-oct-1986 - Written. (ncg)
**	19-dec-1986 - Added support for embedded ADTs. (bjb)
**	01-apr-1987 - Modified text compression for 6.0. No this isn't an
**		      April Fool's Joke, text really is uncompressed. (ncg)
**	30-jun-1988 - Checked that length of data not too big. (ncg)
**	02-jun-1988 - Conversion into embedded VARCHARS where length is
**		      zero now go through adh. (bjb)
**	26-jul-1990 (barbara)
**	     Updated for decimal in Orion/6.5.  Don't try to optimize for
**	     decimal types.  Always go through ADH/ADF.
**	15-oct-1990 (barbara)
**	    Fixed bug 20010.  Now that the looping code is checking for
**	    warnings (see IInextget), I have classified numeric overflow
**	    as a warning to be consistent.  Previously, loop did NOT break
**	    out on numeric overflow; it still doesn't.  Some of the other
**	    conversion errors should also be classified as warnings.
**	27-Jul-1991 (thomasm)
**	    Fixed alignment bug on hp8.  Buffer retbuf needs to be most
**	    restrictively aligned.  On hp8, floats need 8byte alignment
**	    so this code which previously used i4  would fail.
**	09-dec-1992 (teresal)
**	    Modified ADF control block reference to be session specific
**	    instead of global (part of decimal changes).
**	10-jun-1993 (kathryn)
**	    Add call to adh_charend() rather than just null terminating
**	    DB_CHR_TYPE.  When the II_L_CHRPAD flag is set we need to 
**	    blank-pad as well as EOS terminate. Adh_charend will correctly
**	    end a string value that has been retrieved directly into the
**	    callers buffer.
** 	08-feb-1996 (loera01) Added flags argument to support compressed 
**          varchars. 
**	03-apr-2001 (abbjo03)
**	    Null terminate DB_NCHR_TYPEs.
**	15-May-2004 (kodse01)
**		BUG# 112321
**		For tid column, its length check is avoided. 
**	14-Jul-2004 (schka24)
**	    Revert the last change.  The final column is not necessarily
**	    the tid!  Fix 112321 by fixing afe et al to understand i8.
*/

i4
IIgetdata
( 
    II_THR_CB		*thr_cb, 
    DB_DATA_VALUE	*dbv,
    DB_EMBEDDED_DATA	*edv,
    i4			colnum,
    i4			flags 
)
{
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
			/* +1 for null byte if conversion is needed */
    ALIGN_RESTRICT	retbuf[(DB_GW4_MAXSTRING + DB_CNTSIZE)/sizeof(ALIGN_RESTRICT) + 1];
    register i4	res;
    char		ebuf[20];	/* Error buffer */
    DB_STATUS		stat;
    i4		locerr;
    i4		generr;
    II_RET_DESC		*rd;

    rd = &IIlbqcb->ii_lq_retdesc;

    if (edv->ed_null)		/* Reset null indicator just in case */
	*edv->ed_null = 0;

    /* Check for null result addresses */
    if (!edv->ed_data)
    { 
	CVna(colnum, ebuf);
	IIlocerr((i4)(GE_DATA_EXCEPTION + GESC_ASSGN),
		 E_LQ0011_EMBNULLOUT, II_ERR, 1, ebuf );
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	return 0;
    }

    /*
    ** If the caller is using a DBV and has the exact type match (ie, the
    ** terminal monitor or QBF), then use that argument for results.
    */
    if (   edv->ed_type == DB_DBV_TYPE
	&& ((DB_DATA_VALUE *)edv->ed_data)->db_datatype == dbv->db_datatype
	&& dbv->db_datatype != DB_DEC_TYPE
	&& ((DB_DATA_VALUE *)edv->ed_data)->db_length == dbv->db_length)
    {
	dbv->db_data = ((DB_DATA_VALUE *)edv->ed_data)->db_data;
  	res = IIcgc_get_value(IIlbqcb->ii_lq_gca, GCA_TUPLES, flags, dbv);
	if (res != dbv->db_length)
	{
	    if (res != IICGC_GET_ERR)   /* Error should cause end of qry */
		IIlocerr(GE_COMM_ERROR, E_LQ0040_SHORTREAD, II_ERR,
			 0, (char *)0);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    return 0;
	}
	return 1;
    } /* if DBV and exact type match */


    /* 
    ** See if we can avoid using a temp buffer which will later
    ** require copying:
    **     if (types and lengths have exact match but not decimal) or
    **	      (types are character and user length is enough) or
    **	      (types are varying character and user length is enough) then
    **	       use user's address.
    ** Note that user char-type lengths of zero are assumed to be maximum.
    */
    if (
          (dbv->db_datatype == edv->ed_type		/* Exact type match */
	     && dbv->db_datatype != DB_DEC_TYPE
	     && edv->ed_length == dbv->db_length)
	|| 						/* Compat char types */
          (((dbv->db_datatype == DB_CHR_TYPE || dbv->db_datatype == DB_CHA_TYPE)
    	     && (edv->ed_type == DB_CHR_TYPE)
	     && (edv->ed_length >= dbv->db_length || edv->ed_length == 0)))
	|| 						/* Compat vchar types */
          (((dbv->db_datatype == DB_TXT_TYPE || dbv->db_datatype == DB_VCH_TYPE)
    	     && (edv->ed_type == DB_VCH_TYPE)
	     && (edv->ed_length >= dbv->db_length )))
       )
    {
	dbv->db_data = edv->ed_data;
    }
    else
    {
	dbv->db_data = (PTR)retbuf;
	if (dbv->db_length > sizeof(retbuf))
	{
	    char	lbuf[20];	/* Error buffer */

	    CVla(dbv->db_length, lbuf);
	    CVna(colnum, ebuf);
	    IIlocerr((i4)(GE_DATA_EXCEPTION + GESC_ASSGN),
	    	     E_LQ0012_DBTOOLONG, II_ERR, 2, lbuf, ebuf);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    return 0;
	}
    }

    /*
    ** NOTE: In 6.0 this call assumes that text is uncompressed, and if
    ** compressed it is handled by a later presentation layer.
    ** NOTE: In 1.3 this call passes a flags argument, to indicate whether
    ** or not the tuple contains compressed varchars.
    */
    res = IIcgc_get_value(IIlbqcb->ii_lq_gca, GCA_TUPLES, flags, dbv);
    if (res != dbv->db_length)
    {
	if (res != IICGC_GET_ERR)   /* Error should cause end of qry */
	    IIlocerr(GE_COMM_ERROR, E_LQ0040_SHORTREAD, II_ERR, 0, (char *)0);
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	return 0;
    }

    /*
    ** Data put directly into user's buffer: if the user's type was character,
    ** then null terminate, as the optimization will not force us throug ADH.
    */
    if (dbv->db_data == edv->ed_data)
    {
	if (edv->ed_type == DB_CHR_TYPE || edv->ed_type == DB_NCHR_TYPE)
	    adh_charend(edv->ed_type, edv->ed_data, dbv->db_length,
			edv->ed_length);
	return 1;
    }

    stat = adh_dbcvtev(IIlbqcb->ii_lq_adf, dbv, edv);
    if (stat != E_DB_OK)
    { 
        /* 
	** Report the error - treat overflow like a message. 
	*/
	CVna(colnum, ebuf);

	/* Map ADF error codes to LIBQ errorcodes */
	switch (IIlbqcb->ii_lq_adf->adf_errcb.ad_errcode)
	{
	  case E_AD1013_EMBEDDED_NUMOVF:
	    locerr = E_LQ000B_EMBOVFL;
	    generr = GE_DATA_EXCEPTION + GESC_FIXOVR;
	    IIlbqcb->ii_lq_curqry |= II_Q_WARN;
	    break;
	  case E_AD1012_NULL_TO_NONNULL:
	    locerr = E_LQ000E_EMBIND;
	    generr = GE_DATA_EXCEPTION + GESC_NEED_IND;
	    break;
	  case E_AD2004_BAD_DTID:
	    locerr = E_LQ000D_EMBDTID;
	    generr = GE_DATA_EXCEPTION + GESC_TYPEINV;
	    break;
	  case E_AD2009_NOCOERCION:
	    locerr = E_LQ000C_EMBCNV;
	    generr = GE_DATA_EXCEPTION + GESC_ASSGN;
	    break;
	  default:
	    locerr = E_LQ000A_EMBBAD;
	    generr = GE_DATA_EXCEPTION + GESC_ASSGN;
	    break;
	}
	IIlocerr(generr, locerr, II_ERR, 1, ebuf);
	if (locerr != E_LQ000B_EMBOVFL)
	{
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    return 0;
	}
    }
    /* Character truncation will return stat OK */
    else if (    (IIlbqcb->ii_lq_adf->adf_errcb.ad_errcode 
	          == E_AD0101_EMBEDDED_CHAR_TRUNC)
	     &&  IISQ_SQLCA_MACRO(IIlbqcb)
	    )
    {
	IIsqWarn( thr_cb, 1 );
    }
    return 1;
}
