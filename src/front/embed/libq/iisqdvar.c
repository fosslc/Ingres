/*
** Copyright (c) 2009 Ingres Corporation
*/

#include	<compat.h>
#include	<er.h>
#include	<st.h>
#include	<gl.h>
#include	<sl.h>
#include	<iicommon.h>
#include	<eqlang.h>
#include	<iisqlda.h>
#include	<fe.h>
#include	<adf.h>
#include	<afe.h>

/**
**  Name: iisqdvar.c - Utility for the SQLDA shared between LIBQ and the
**		       form system.
**
**  Description:
**
**	This files contains the routines to update the SQLDA.
**
**  Defines:
**		IIsqdSetSqlvar - Add the DESCRIBE contents to the SQLVAR.
**
**  History:
**	25-may-1988	- Created. (ncg)
**	26-jul-1990 (barbara)
**	    Updated for decimal.
**	10-feb-1993 (kathryn)
**	    Updated for long varchar.
**	13-sep-1993 (sandyd)
**	    Added case for DB_VBYTE_TYPE, since it requires a length
**	    adjustment (just like DB_VCH_TYPE) because of the count field.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	27-aug-2009 (crido01) Bug 122532
**	    Added case for DB_LBYTE_TYPE, since it requires a length
**	    adjustment (just like DB_LVCH_TYPE).
**/

/*{
**  Name: IIsqdSetSqlvar - Set an sqlvar for the SQL DESCRIBE statement.
**
**  Description:
**	Based on the type rules defined for DESCRIBE  this routine 
**	fills in the appropriate sqlvar element.
**
**	If the current value of SQLD is greater than SQLN then the
**	value of SQLD is updated but no copying is done into SQLVAR.
**
**	This routine should replace the last part of the LIBQ routine
**	IIsq_RdDesc, which can be modified to call a common routine.  This
**	routine should be deposited into the the file "libq/iisqdvar.c".
**
**	If adf_types is set then the internal ADF DB_DATA_VALUE is returned
**	in the data field together with the external type description - this
**	is to support front-end usage where they must distinguish between
**	all data types.
**
**	The internal ADF type descriptions are mapped to external types and
**	lengths, as specified by the Dynamic SQL DESCRIBE statement:
**
**	1. Length information is converted from the internal representation
**	   to a user representation:
**
**	    Nullable types	   - Subtract 1 byte (length of null byte),
**	    Varying-length strings - Subtract length of count field,
**	    DATE, MONEY		   - Set length to zero.
**	    DECIMAL		   - Assign internal prec to length field.
**	    LONG VARHCAR	   - Set length to zero (unknown).
**
**	2. Some data types are mapped to external SQL data types:
**
**	    Varying Length Char (TXT, LTXT, VCH) - VCH
**	    Fixed Length (CHR, CHA)		 - CHA
**	    Table field				 - TFLD
**
**  Inputs:
**	lang		- Language constant for possible SQLDA details.
**	sqd		- Pointer to caller's SQLDA to fill.
**	    .sqln		Number of "sqlvar" structures within the SQLDA
**				allocated by program.  If sqln < the number 
**				of objects to be described then	this routine
**				fills in sqln column descriptors in the SQLDA.
**	adf_types	- Indicates whether we should return the ADF descriptor
**			  to the caller.
**	objname		- Object name.
**	objdbv		- DB data value with internal description.
**
**  Outputs:
**	sqd		- The SQLDA is filled with the description of the
**			  object.
**	    .sqld		The number of objects described.  This may be
**				zero if there are none.
**	    .sqlvar[sqld]	Sqld elements of the array are filled.
**		.sqldata	If adf_types then pointed at DB_DATA_VALUE.
**		.sqltype	External data type id of column.
**	    	.sqllen		Length of data in column. (Description above).
**	    	.sqlname	Internal name information
**		    .sqlnamel	Length of internal name.
**	    	    .sqlnamec	Internal name (no EOS termination).
**				Name is blank padded.
**
**	Returns:
**	    TRUE - Was able to update SQLVAR.
**	    FALSE - Updated SQLD but not SQLVAR.
**
**	Errors:
**	    None
**
**  Side Effects:
**	None
**	
**  History:
**	25-may-1988	- Created from IIsq_RdDesc. (ncg)
**	30-aug-1988 	- Added adf_types to allow internal use. (ncg)
**	26-jul-1990 (barbara)
**	    Updated for decimal.
**	10-feb-1993 (kathryn)
**	    Updated for Long Varchar.
**	13-sep-1993 (sandyd)
**	    Added case for DB_VBYTE_TYPE, since it requires a length
**	    adjustment (just like DB_VCH_TYPE) because of the count field.
**	17-oct-2003 (gupsh01)
**	    Added cases for DB_NCHR_TYPE and DB_NVCHR_TYPE, for which the
**	    length of the data needs to be adjusted as per wchar_t size.
**	03-aug-2006 (gupsh01)
**	    Added cases for ANSI date/time types. 
**	27-aug-2009 (crido01) Bug 122532
**	    Added case for DB_LBYTE_TYPE, since it requires a length
**	    adjustment (just like DB_LVCH_TYPE).
**      17-Aug-2010 (thich01)
**          Make changes to treat spatial types like LBYTEs.
*/

bool
IIsqdSetSqlvar(lang, sqd, adf_types, objname, objdbv)
i4		lang;
IISQLDA		*sqd;
i4		adf_types;
char		*objname;
DB_DATA_VALUE	*objdbv;
{
    i4			col;			/* Index into sqlvar */
    struct sqvar	*sqv;			/* User sqlvar */
    i4			sqlen, sqtype;		/* Mapping to local */

    col = sqd->sqld++;				/* Always increment sqld */

    if (sqd->sqld > sqd->sqln)			/* Data will not fit */
	return FALSE;

    sqv = &sqd->sqlvar[col];			/* Data will fit */
    sqtype = objdbv->db_datatype;
    sqlen = objdbv->db_length;

    /* If using ADF types, return the original descriptor to the caller */
    if (adf_types)
	sqv->sqldata = (PTR)objdbv;

    /* Now process external type information */
    if (sqtype < 0)
    {
	sqlen--;
	sqtype = -sqtype;			/* Locally positive */
    }
    if (    sqtype == DB_VCH_TYPE 
	||  sqtype == DB_TXT_TYPE 
	||  sqtype == DB_LTXT_TYPE
       )
    {
	sqlen -= DB_CNTSIZE;
	sqtype = DB_VCH_TYPE;		/* External type is VARCHAR */
    }
    else if ( sqtype == DB_NCHR_TYPE)
    { 
	sqlen = sqlen/sizeof (UCS2) * sizeof(wchar_t); 
    }
    else if ( sqtype == DB_NVCHR_TYPE)
    {
	sqlen = (sqlen - DB_CNTSIZE)/(sizeof (UCS2)) * sizeof(wchar_t); 
    }
    else if (sqtype == DB_VBYTE_TYPE)
    {
	/* Adjust for count field, but don't map to a different type. */
	sqlen -= DB_CNTSIZE;
    }
    else if (sqtype == DB_CHR_TYPE)
    {
	sqtype = DB_CHA_TYPE;
    }
    else if (   sqtype == DB_DTE_TYPE 
	     || sqtype == DB_ADTE_TYPE
	     || sqtype == DB_TMWO_TYPE
	     || sqtype == DB_TMW_TYPE
	     || sqtype == DB_TME_TYPE
	     || sqtype == DB_TSWO_TYPE
	     || sqtype == DB_TSW_TYPE
	     || sqtype == DB_TSTMP_TYPE
	     || sqtype == DB_INYM_TYPE
	     || sqtype == DB_INDS_TYPE
	     || sqtype == DB_MNY_TYPE
	     || sqtype == DB_LBYTE_TYPE
	     || sqtype == DB_GEOM_TYPE
	     || sqtype == DB_POINT_TYPE
	     || sqtype == DB_MPOINT_TYPE
	     || sqtype == DB_LINE_TYPE
	     || sqtype == DB_MLINE_TYPE
	     || sqtype == DB_POLY_TYPE
	     || sqtype == DB_MPOLY_TYPE
	     || sqtype == DB_NBR_TYPE
	     || sqtype == DB_GEOMC_TYPE
	     || sqtype == DB_LVCH_TYPE
	     || sqtype == DB_LNVCHR_TYPE)
    {
	sqlen = 0;
    }
    else if (sqtype == DB_DEC_TYPE)
    {
	sqlen = objdbv->db_prec;
    }
    sqv->sqllen = sqlen;
    if (objdbv->db_datatype < 0)
	sqv->sqltype = -sqtype;		/* Preserve nullability */
    else
	sqv->sqltype = sqtype;

    /* Assign object name */
    sqv->sqlname.sqlnamel = min(IISQD_NAMELEN, STlength(objname));
    STmove(objname, ' ', IISQD_NAMELEN, sqv->sqlname.sqlnamec);

    return TRUE;

} /* IIsqdSetSqlvar */
