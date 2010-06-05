/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <mh.h>
#include    <pc.h>
#include    <tr.h>
#include    <st.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dmppn.h>
#include    <dm1c.h>
#include    <dm1cn.h>
#include    <dmccb.h>
#include    <dmxcb.h>
#include    <dml.h>

/**
**
**  Name: DM1CCMPN.C - Normal tuple-level format compression routines.
**
**  Description:
**      This file contains the routines needed to (de)compress tuples
**      using "standard" compression.  They are usually called via the
**	dispatch vectors in the DMP_ROWACCESS tuple access descriptor.
**	Note that a "tuple" might be a index or leaf key, not an
**	ordinary tuple as such.
**
**	Standard or normal compression only compresses nulls or
**	character strings.  Trailing junk is trimmed and char is
**	stored as varchar, so that decompression works.  Nulls are
**	stored as the null byte alone.
**
**      The normal (dm1cn_) functions defined in this file are:
**
**	compress	- Compress a record
**	uncompress	- Uncompresses a record
**	compexpand	- how much can a tuple expand when compressed
**	cmpcontrol_setup - Set up control array
**	cmpcontrol_size - Estimate size of control array
**
**  History:
**	13-oct-1992 (jnash)
**	    Reduced Logging project.  Created from dm1cn.c to 
**	    isolate compression code from page access.
**	10-nov-1992 (jnash)
**	    Replaced references to DMP_ATTS with DB_ATTS.  
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system typedefs
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	31-jan-1994 (jnash)
**	    In dm1cn_uncompress(), prefill output buffer with zeros to 
**	    eliminate rollforwarddb dmve_replace() "tuple_mismatch" 
**	    warnings after modify operations.
**	16-may-1996 (shero03)
**	    Support variable length UDTs.
**	23-may-1994 (andys)
**	    Add cast(s) to quiet qac/lint
**      17-jul-1996 (ramra01 for bryanp)
**          Alter Table support: Add new row_version argument to
**          dm1cn_uncompress, and use it to convert older versions of rows
**          to their current versions while uncompressing them.
**	13-sep-1996 (canor01)
**	    Add buffer to dmpp_uncompress call to pass buffer for data
**	    uncompression.
**	23-sep-1996 (canor01)
**	    Moved global data definitions to dmpdata.c.
**      29-Jan-97 (fanra01)
**          Pass adf_scb as address.  On NT compiler places whole structure on
**          stack.
**      28-May-97 (fanra01)
**          Bug 82393
**          Changed instances where adf_scb was being passed to functions as
**          a structure to a pointer.
**          NT compiler places whole structure on the stack.
**	08-Oct-1997 (shero03)
**	    Found an instance similar to 82393.  The call to adc_compr
**	    requires the addr(dv) - not dv itself.
**	25-Feb-1998 (shero03)
**	    Account for a null coupon.
**	06-Apr-1998 (hanch04/nanpr01)
**	    Performace improvements.  Added case for common types.
**	22-Apr-1998 (hanch04)
**	    Add missing assignment for compress.
**	18-May-1998 (jenjo02)
**	    Repaired some code broken by 06-Apr performance changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-may-2001 (stial01)
**          Back out some 06-Apr-1998 Performance improvements (B104806)
**      31-may-2001 (stial01)
**          Fixed cross integration of 28-Aug-2000 change which moved
**          the line 'src += DB_CNTSIZE' out of place.
**       2-Sep-2004 (hanal04) Bug 111474 INGSRV2638 
**          dm1cn_uncompress() was incorrectly materialising data for
**          columns added with defaults.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      18-feb-2005 (srisu02)
**          Fixed a call to MEcopy() as the arguments were in the wrong order
**	21-Apr-2005 (thaju02)
**	    In dm1cn_uncompress, increment src to account for dropped 
**	    nchar/nvarchar cols. (B114367)
**	01-Aug-2005 (sheco02)
**	    Since C2secure was removed for open source, remove the related
**	    code here to fix the x-integration 477914 problem.
**      12-sep-2005 (stial01)
**          dm1cn_uncompress() changes for alter table alter column (b115164)
**      17-jan-2006 (stial01)
**          Fix uncompress of dropped nullable BLOB column (b115642,b115643)
**      27-sep-2006 (stial01)
**          Fixed args to dm1cn_uncompress, use caller adf_cb (b116737)
**	13-Feb-2008 (kschendel) SIR 122739
**	    Revise uncompress call, remove record-type and tid.
**	19-Feb-2008 (kschendel) SIR 122739
**	    More whacking, combine info needed into a rowaccess struct.
**	    This is all preparatory to adding instruction-style
**	    (de)compression, and perhaps new row compression types.
**	    Remove "old" uncompression.
**	08-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat.
**/


/*
**  Forward function references (used in accessor definition).
*/



/*
** Name: dmpp_compress - Compress a record, given pointers to attributes
**
** Description:
**      Compress the record into work area provided, using
**	standard compression (nulls and trailing blanks).
**
**	Compression Algorithms:
**	    Nullable datatypes - nullable datatypes are compressed by writing
**			one byte null descriptor (indicating whether the value
**			is NULL) to the front of the compressed value.  If the
**			value is NULL, then no further data is written.
**			(Note: an exception is null LOB data, those write
**			the coupon even if null.  Since the coupon is inserted
**			into the row after it's first built, this means that
**			the row size won't change for null coupons.)  If it's
**			not NULL, then the data value itself is compressed
**			and written following the null indicator.
**
**	    C		- copied up to the last non-blank character and then
**			an EOS byte ('\0') is written.
**	    CHAR	- number of bytes to the last non-blank character is
**			calculated.  Two-byte length desciptor is written
**			followed by the string up to this length.
**	    VARCHAR, TEXT - Two byte length descriptor is written giving the
**			length indicated in the VARCHAR or TEXT datatype,
**			followed by this many bytes from the data portion of
**			the datatype.
**	    INT, FLOAT, DATE, other - copied as is: no compression.
**
** Inputs:
**      rac				Pointer to DMP_ROWACCESS descriptor
**					that contains access, att info.
**      
**      rec                             Pointer to record to compress.
**      rec_size                        Uncompressed record size. 
**      crec                            Pointer to an area to return compressed
**                                      record.
**
** Outputs:
**      crec_size		        Pointer to an area where compressed
**                                      record size can be returned.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			Tuple could not be compressed.
**
**	7-mar-1991 (bryanp)
**	    Created for the Btree index compression project.
**	13-oct-1992 (jnash)
**	    Make att_ln i2 to quiet compiler.
**	16-may-1996 (shero03)
**	    Support variable length UDTs.
**      29-Jan-97 (fanra01)
**          Pass adf_scb as address.
**	08-Oct-1997 (shero03)
**	    Found an instance similar to 82393.  The call to adc_compr
**	    requires the addr(dv) - not dv itself.
**	25-Feb-1998 (shero03)
**	    Account for a null coupon.
**	03-Oct-1999 (wanfr01)
** 	    Partial cross-integration of change 435281 - performance improvement
**	    to dm1cn_compress and dm1cn_umcompress
**	30-Aug-2000 (thaju02)
**	    If columns have been previous dropped via alter table, then
**	    avoid attempting to compress for dropped column. (b102455)
**	06-Apr-1998 (hanch04/nanpr01)
**	    Performace improvements.  Added case for common types.
**	18-May-1998 (jenjo02)
**	    Repaired some code broken by 06-Apr performance changes.
**	09-Feb-2005 (gupsh01)
**	    Added support for nchar/nvarchar.
**	26-Apr-2005 (gupsh01)
**	    Fix the nchar case.
**	21-Jun-2005 (gupsh01)
**	    Fix processing for nchar case. NCHAR lengths are number of
**	    unicode characters.
**	03-Aug-2006 (gupsh01)
**	    Added ANSI date/time types.
**	3-Apr-2008 (kschendel) SIR 122739
**	    Revise to use the compression-control array instead of att by att.
**	    This should speed things up a bit for runs of non-nullable
**	    non-compressing columns, plus reduce jumping about slightly.
*/
DB_STATUS	
dm1cn_compress(
	DMP_ROWACCESS *rac,
	char	*rec,
	i4	rec_size,
	char	*crec,
	i4	*crec_size)
{
    register char	*ptr;
    register char       *src;
    register char	*dst;
    DB_ATTS		**attpp;
    DM1CN_CMPCONTROL	*op;
    DM1CN_CMPCONTROL	*end_op;	/* Stop when we reach this */
    char		*new_src;
    i4			src_len, dst_len;
    i4	 		control_count;
    i4			error;
    u_i2		i2_len;
    u_i2		u_ch;

    src = rec;
    dst = crec;
    attpp = rac->att_ptrs;
    op = (DM1CN_CMPCONTROL *) rac->cmp_control;
    end_op = &op[rac->control_count];
    do
    {
	dst_len = op->length;
	/* Careful here ... if nullable, dest will be <nullind> <value>
	** but source is <value> <nullind>, so after we've done with the
	** source we have to skip non-nullable-length + 1.
	** Note that group copyn's are by definition not nullable.
	*/
	src_len = dst_len + (op->flags & DM1CN_NULLABLE);
	switch (op->opcode)
	{
	case DM1CN_OP_VERSCHK:
	    /* for compression, version check is simply proceed normally
	    ** vs ignore this column entirely.
	    */
	    if (op->flags & (DM1CN_DROPPED | DM1CN_ALTERED) )
	    {
		/* Column doesn't exist any more, just skip control op
		** for this column.  Skip any null-check also.
		*/
		++op;
		if (op->opcode == DM1CN_OP_NULLCHK)
		    ++op;
	    }
	    break;

	case DM1CN_OP_NULLCHK:
	    /* Write the null indicator at the front, rather than at the
	    ** end where it's stored for the data value.  If NULL, write
	    ** nothing more, unless we're writing a LOB coupon.
	    */
	    *dst = src[dst_len];	/* dst_len is nullable_len - 1 */
	    if (*dst++ & ADF_NVL_BIT)
	    {
		if (op->flags & DM1CN_PERIPHERAL)
		{
		    /* Make room for a coupon.  Value is irrelevant, might
		    ** as well zero it.
		    */
		    MEfill(dst_len, 0, dst);
		    dst += dst_len;
		}
		src += src_len;
		/* Skip column's control op since column is NULL */
		++op;
	    }
	    break;

	case DM1CN_OP_COPYN:
	    /* Just copy N bytes */
	    MEcopy(src, dst_len, dst);
	    src += src_len;
	    dst += dst_len;
	    break;

	case DM1CN_OP_CHR:
	    /* C type - snip off trailing blanks, then write null-terminated.
	    ** Be careful to convert any embedded nulls to blanks;  they
	    ** shouldn't be there in the first place.
	    */
	    ptr = src + dst_len - 1;
	    new_src = src + src_len;
	    do
	    {
		if (*ptr != ' ' && *ptr != '\0')
		    break;
	    } while (--ptr >= src);
	    while (src <= ptr)
	    {
		if ( (*dst++ = *src++) == '\0')
		    *(dst-1) = ' ';
	    }
	    *dst++ = '\0';
	    src = new_src;
	    break;

	case DM1CN_OP_CHA:
	    /* CHAR type - snip trailing blanks, then store as if varchar */
	    ptr = src + dst_len - 1;
	    do
	    {
		if (*ptr != ' ')
		    break;
	    } while (--ptr >= src);
	    if (ptr < src)
	    {
		i2_len = 0;
	    }
	    else
	    {
		i2_len = ptr - src + 1;		/* Number of nonblanks */
		MEcopy(src, i2_len, dst+sizeof(u_i2));
	    }
	    I2ASSIGN_MACRO(i2_len, *(u_i2 *)dst);
	    dst += sizeof(u_i2) + i2_len;
	    src += src_len;
	    break;

	case DM1CN_OP_NCHR:
	    /* Very much like char, except double-byte characters */
	    ptr = src + dst_len - sizeof(UCS2);
	    do
	    {
		/* have to assume that UCS2 is an i2 here.  Do an
		** unaligned double-byte char access
		*/
		I2ASSIGN_MACRO(*(u_i2 *)ptr, u_ch);
		if (u_ch != U_BLANK)
		    break;
		ptr -= sizeof(UCS2);
	    } while (ptr >= src);
	    if (ptr < src)
	    {
		i2_len = 0;
	    }
	    else
	    {
		i2_len = (UCS2 *)ptr - (UCS2 *)src + 1;
		MEcopy(src, i2_len * sizeof(UCS2), dst+sizeof(u_i2));
	    }
	    I2ASSIGN_MACRO(i2_len, *(u_i2 *)dst);
	    dst += sizeof(u_i2) + i2_len*sizeof(UCS2);
	    src += src_len;
	    break;

	case DM1CN_OP_VCH:
	    /* Varchar or text, store <count> bytes */
	    I2ASSIGN_MACRO(((DB_TEXT_STRING *)src)->db_t_count, i2_len);
	    i2_len += DB_CNTSIZE;

	    /*
	    ** Verify that length descriptor is valid before writing it out.
	    ** Ideally, this check is not needed if we guaranteed all data
	    ** values were checked before being passed to the low levels of the
	    ** system.
	    */
	    if (i2_len <= dst_len)
	    {
		MEcopy(src, i2_len, dst);
		dst += i2_len;
		src += src_len;
		break;
	    }

	    /*
	    ** If we get here then we have a bad data value - the length
	    ** descriptor has an impossible value.
	    */
	    uleFormat(NULL, E_DM9388_BAD_DATA_LENGTH, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		0, (i4)i2_len);
	    uleFormat(NULL, E_DM9389_BAD_DATA_VALUE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    return (E_DB_ERROR);

        case DM1CN_OP_NVCHR:
	    /* Like varchar, except adjust for double-byte UCS2 */
	    I2ASSIGN_MACRO(((DB_TEXT_STRING *)src)->db_t_count, i2_len);
            i2_len = i2_len * sizeof(UCS2) + DB_CNTSIZE;

            /* Check that length descriptor is valid */
	    if (i2_len <= dst_len)
	    {
		MEcopy(src, i2_len, dst);
		dst += i2_len;
		src += src_len;
		break;
	    }

	    /*
	    ** If we get here then we have a bad value - the length
	    ** descriptor has an impossible value.
	    */
	    uleFormat(NULL, E_DM9388_BAD_DATA_LENGTH, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		0, (i4)i2_len);
	    uleFormat(NULL, E_DM9389_BAD_DATA_VALUE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    return (E_DB_ERROR);

	case DM1CN_OP_VLUDT:
	    /* Call the UDTs compress routine.  We've already handled the
	    ** null indicator, so tell adf it's not nullable.
	    ** Toss any error, maybe not such a hot idea?
	    */
	    {
		ADF_CB adf_scb;
		DB_DATA_VALUE tmp_dv;
		i4 compr_len;

		MEfill(sizeof(ADF_CB), 0, (PTR) &adf_scb);
		adf_scb.adf_maxstring = DB_MAXSTRING;
		tmp_dv.db_datatype = abs(attpp[op->attno]->type);
		tmp_dv.db_data = src;
		tmp_dv.db_length = dst_len;
		tmp_dv.db_prec = attpp[op->attno]->precision;

		(void) adc_compr(&adf_scb, &tmp_dv, TRUE, dst, &compr_len);
		dst += compr_len;
		src += src_len;
            }
	    break;

        } /* switch */
	++op;
    } while (op < end_op);
    *crec_size = dst - crec;	/* calculate resulting tuple length */
    return (E_DB_OK);
}

/*
** Name: dmpp_uncompress - Uncompresses a record, given pointers to attributes
**
** Description:
**    This routine uncompresses a record and returns the record
**    and size in bytes.  Refer to dmpp_compress for the algorithm.
**
**	NOTE! This routine is the BTREE version used all the time now.
**
** Inputs:
**      rac				Pointer to DMP_ROWACCESS info struct
**                                      containing attribute info.
**      src                             Pointer to an area containing record
**                                      to uncompress.
**	dst_maxlen			Size of the destination area. If, during
**					the uncompress operation, we detect
**					that we are exceeding the destination
**					area size, we abort the uncompress
**					operation and return an error.
**
** Outputs:
**      dst                             Pointer to an area to return
**                                      uncompressed record.
**      dst_size                        Size of uncompressed record.
**      
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			Bad tuple encountered.
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	7-mar-1991 (bryanp)
**	    Created for the Btree index compression project.
**	13-oct-1992 (jnash)
**	    Reduced logging project.  As part of separating
**	    compression from the page accessor code, added record_type
**	    and tid params, calling dm1cn_old_uncomp_rec if old style
**	    compression.   
**	31-jan-1994 (jnash)
**	    Prefill output buffer with zeros to eliminate rollforwarddb 
**	    dmve_replace() "tuple_mismatch" warnings after modify
**	    operations.
**	23-may-1994 (andys)
**	    Add cast(s) to quiet qac/lint
**      17-jul-1996 (ramra01 for bryanp)
**          Alter Table support: Add new row_version argument to
**          dm1cn_uncompress, and use it to convert older versions of rows
**          to their current versions while uncompressing them.
**	25-Feb-1998 (shero03)
**	    Account for a null coupon.
**	03-Oct-1999 (wanfr01)
** 	    Partial cross-integration of change 435281 - performance improvement
**	    to dm1cn_compress and dm1cn_umcompress
**	18-Aug-2000 (thaju02)
**	    If DB_CHA_TYPE, DB_TXT_TYPE, DB_VCH_TYPE column has been dropped,
**	    increment src pointer by DB_CNTSIZE to account for two-byte 
**	    length identifier. (b101362)
**	28-Aug-2000 (thaju02)
**	    If nullable column has been dropped, increment src pointer 
**	    to account for null indicator. Also modified code to account
**	    for UDT dropped column. (b102446)
**	06-Apr-1998 (hanch04/nanpr01)
**	    Performace improvements.  Added case for common types.
**	18-May-1998 (jenjo02)
**	    Fix code broken by 06-Apr change. 
**	    Selectively zero-fill otherwise unfilled destination fields
**	    instead of zero-filling the entire destination beforehand.
**	12-aug-99 (stephenb)
**	    Alter parameters, buffer is now char** since it may be 
**	    freed by called function.
**       2-Sep-2004 (hanal04) Bug 111474 INGSRV2638 
**          Added code to insert default value for not null with default
**          added columns. 
**	09-Feb-2005 (gupsh01)
**	    Added support for nchar/nvarchar.
**	21-Apr-2005 (thaju02)
**	    Increment src to account for dropped nchar/nvarchar cols. 
**	    (B114367)
**	21-Jun-2005 (gupsh01)
**	    Fix processing for nchar case. NCHAR lengths are number of
**	    unicode characters.
**	03-aug-2006 (gupsh01)
**	    Added support for new ANSI date/time types.
**	29-Sep-2006 (wanfr01)
**	    Bug 116727
**	    cv_att and save_dst need to be nulled for each att's
**	    processing to avoid misuse of previous attribute info.
**	    also restore saved dst prior to processing next row.
**	3-Apr-2008 (kschendel) SIR 122739
**	    Redo, drive from control array rather than att by att.
**	    Should be faster for runs of non-nullable non-character atts,
**	    and reduce jumping around for nullable and version checking.
**	29-Aug-2008 (kschendel)
**	    Set exception ADF cb pointer around cvinto calls to handle
**	    arithmetic exceptions (e.g. overflow) during conversion.
*/

/* For alter column conversions, keep small held values on the stack,
** larger ones will have to come from heap memory.  This choice of
** "small" (in bytes) is pretty arbitrary.
*/
#define DM1CN_STACKLOCAL	80

DB_STATUS
dm1cn_uncompress(
	DMP_ROWACCESS *rac,
	register char	    *src,
	register char	    *dst,
	i4	dst_maxlen,
	i4	*dst_size,
	char	**buffer,
	i4	row_version,	
	ADF_CB	*adf_scb)
{
    ALIGN_RESTRICT	local_value[DM1CN_STACKLOCAL / sizeof(ALIGN_RESTRICT)];
    bool		got_sid = FALSE;
    char		*old_value;	/* points to local or heap memory */
    char		*orig_dst;
    char		*p1, *p2;
    char		*save_dst;
    CS_SID		sid;
    DB_ATTS		*att;
    DB_ATTS		*att_from;	/* Convert-from att if ALTERED */
    DB_ATTS		**attpp;
    DB_DATA_VALUE	fromvalue;
    DB_DATA_VALUE	tovalue;
    DB_STATUS		stat;
    DM1CN_CMPCONTROL	*op;
    DM1CN_CMPCONTROL	*end_op;
    DML_SCB		*scb;
    i4  		err_code;
    i4			dst_len;	/* Result length including null ind */
    i4			nnull_len;	/* Length of non-null value part */
    u_i2		i2_len;
    u_i2		u_ch;
    u_i2		*up1, *up2;

    orig_dst = dst;
    att_from = NULL;
    attpp = rac->att_ptrs;
    op = (DM1CN_CMPCONTROL *) rac->cmp_control;
    end_op = &op[rac->control_count];	/* Stop when we reach this */
    do
    {
	nnull_len = op->length;
	dst_len = nnull_len + (op->flags & DM1CN_NULLABLE);

	switch (op->opcode)
	{
	case DM1CN_OP_VERSCHK:
	    /* Massive jumping around, depending on the row version
	    ** vs the attribute versions.
	    ** We can assume that some version number for the related att
	    ** is nonzero.
	    ** The possibilities are:
	    ** att: ver_added > 0, not dropped.
	    ** --> if rowvers >= ver_added, current flavor of value is in
	    **     the source row, just use it;
	    **     else if no ALTER pending, generate a default value;
	    **     else convert the held value to the current column data type.
	    **	   In either of the last two cases, skip over the control
	    **	   ops for this attribute.
	    ** att: 0 <= ver_added < ver_dropped, ver_altcol zero.
	    ** --> if ver_added <= rowvers < ver_dropped, skip the data
	    **     in the row with no output, else ignore completely.
	    ** att: 0 <= ver_added < ver_dropped, ver_altcol nonzero.
	    ** (ver_altcol should be equal to ver_dropped, it's really just
	    ** a flag rather than a version.)
	    ** --> if rowvers < ver_added OR rowvers >= ver_dropped, just
	    **     ignore completely;  else, if DROPPED flag, this column was
	    **     eventually dropped, so skip over it in the source row;
	    **     else (ALTERED flag) save the current state and set the
	    **     destination to a local temp, then uncompress out of the
	    **     source row (into the temp).  Eventually (possibly skipping
	    **     intermediate type alterations), a VERSCHK that is added
	    **     and not dropped will be reached, which will do the final
	    **     type conversion.
	    */

	    att = attpp[op->attno];
	    if (att->ver_dropped == 0)
	    {
		/* This is a "current version" att, so we need something
		** in the output.  The only question is where to get it from!
		*/
		if (row_version >= att->ver_added)
		    break;		/* Att is in source row per normal */
		if (att_from != NULL)
		{
		    /* Second stage of ALTERED att, old-type value out of
		    ** the row is in old_value.  Convert it into the dest row.
		    */

		    if (! got_sid)
		    {
			CSget_sid(&sid);
			scb = GET_DML_SCB(sid);
			got_sid = TRUE;
		    }
		    if (scb != NULL)
			scb->scb_qfun_adfcb = adf_scb;
		    adf_scb->adf_errcb.ad_errclass = 0;
		    adf_scb->adf_errcb.ad_errcode = 0;
		    fromvalue.db_datatype = att_from->type;
		    fromvalue.db_length = att_from->length;
		    fromvalue.db_prec = att_from->precision;
		    fromvalue.db_data = old_value;
		    fromvalue.db_collID = att_from->collID;

		    tovalue.db_datatype = att->type;
		    tovalue.db_length = att->length;
		    tovalue.db_prec = att->precision;
		    tovalue.db_data = save_dst;
		    tovalue.db_collID = att->collID;

		    stat = adc_cvinto (adf_scb, &fromvalue, &tovalue);
		    if (scb != NULL)
			scb->scb_qfun_adfcb = NULL;
		    if (stat != E_DB_OK)
		    {
			/* Could call dmf-adf-error here, not sure if we
			** want to, just let it show row conversion failed.
			*/
			uleFormat(NULL, E_DM019D_ROW_CONVERSION_FAILED, NULL,
				ULE_LOG, NULL, (char *)NULL,
				(i4)0, (i4 *)0, &err_code, 0);
			if (old_value != (char *)&local_value[0])
			    MEfree(old_value);
			return (E_DB_ERROR);
		    }
		    /* Finish up by adjusting dst stuff */
		    dst = save_dst + att->length;
		    att_from = NULL;
		    if (old_value != (char *)&local_value[0])
			MEfree(old_value);
		    old_value = NULL;
		}
		else
		{
		    /* Only other possibility is that the column was added
		    ** after this row was inserted, so fake up a default value.
		    ** Generate zeros and null-indicator set if nullable;
		    ** blanks for C, CHAR, NCHAR;  zeros for everything else.
		    ** FIXME someday we need to be able to do value defaults,
		    ** if some higher level can discover what those are and
		    ** put them in some accessible place.
		    */
		    if (att->type < 0)
		    {
			MEfill(att->length-1, 0, dst);
			dst[att->length-1] = ADF_NVL_BIT;
		    }
		    else
		    {
			switch (att->type)
			{
			case DB_CHA_TYPE:
			case DB_CHR_TYPE:
			    MEfill(att->length, ' ', dst);
			    break;

			case DB_NCHR_TYPE:
			    {
				i4 char_len = att->length / sizeof(UCS2);
				u_i2 *p = (u_i2 *) dst;
				u_i2 u_ch = U_BLANK;

				while (--char_len >= 0)
				{
				    I2ASSIGN_MACRO(u_ch, *(i2 *)p);
				    ++p;
				}
			    }
			    break;

			case DB_DEC_TYPE:
			    /* Need to include decimal plus-sign */
			    MEfill(att->length-1, 0, dst);
			    dst[att->length-1] = MH_PK_PLUS;
			    break;

			default:
			    MEfill(att->length, 0, dst);
			    break;
			}
		    }
		    dst += att->length;
		}
	    }
	    else if (op->flags & DM1CN_DROPPED)
	    {
		/* Looks like a straight drop.  If the column is in the
		** row, skip it, otherwise just simply ignore.
		*/
		if (row_version >= att->ver_added && row_version < att->ver_dropped)
		{
		    /* Skipping input data requires interpreting the column
		    ** control for the column, but sending the data to
		    ** nowhere.  This is best done by hand to avoid slowing
		    ** down the normal case.
		    */
		    ++op;
		    if (op->opcode == DM1CN_OP_NULLCHK)
		    {
			/* Test input stream for null */
			if (*src++ & ADF_NVL_BIT)
			{
			    /* If null, that's it, unless coupon */
			    if (op->flags & DM1CN_PERIPHERAL)
				src += op->length;
			    ++op;
			    break;	/* switch */
			}
			++op;
		    }
		    /* This opcode can't be a nullchk or verschk */
		    switch (op->opcode)
		    {
		    case DM1CN_OP_COPYN:
			src += op->length;
			break;
		    case DM1CN_OP_CHR:
			while (*src++ != '\0')
			    ;
			break;
		    case DM1CN_OP_CHA:
		    case DM1CN_OP_VCH:
			I2ASSIGN_MACRO( *(u_i2 *)src, i2_len);
			src += sizeof(u_i2) + i2_len;
			break;
		    case DM1CN_OP_NVCHR:
		    case DM1CN_OP_NCHR:
			/* stored count is ucs2-chars, not octets */
			I2ASSIGN_MACRO( *(u_i2 *)src, i2_len);
			src += sizeof(u_i2) + i2_len * sizeof(UCS2);
			break;
		    case DM1CN_OP_VLUDT:
			/* Call UDT uncompress and pitch the result, but
			** uncompress into what?  traditionally it's the
			** dest, which we then don't move and thus overwrite,
			** but I am not 100% convinced...
			*/
			{
			    DB_DATA_VALUE temp_dv;
			    i4 compr_len;

			    temp_dv.db_datatype = abs(att->type);
			    temp_dv.db_data = src;
			    temp_dv.db_length = op->length;
			    temp_dv.db_prec = att->precision;
			    (void) adc_compr(adf_scb, &temp_dv, FALSE,
							dst, &compr_len);
			    src += compr_len;
			}
			break;
		    }
		    break;  /* main switch */
		}
		/* else irrelevant control entry, fall thru to skip it */
	    }
	    else
	    {
		/* It's alter column.  If the original flavor of the column
		** is in this row, fetch it, but fiddle the destination so
		** that the value ends up in a holding area instead of the
		** target row.  When we later reach the control entry for
		** the current flavor of the column, it will get converted
		** out of the holding area into the destination.
		** If the row version doesn't match, just ignore this op.
		*/

		if (row_version >= att->ver_added && row_version < att->ver_dropped)
		{
		    save_dst = dst;
		    old_value = (char *) &local_value[0];
		    if (att->length > DM1CN_STACKLOCAL)
			old_value = (char *) MEreqmem(0, att->length, TRUE, NULL);
		    dst = old_value;
		    att_from = att;
		    break;	/* switch */
		}
	    }
	    /* Don't need this column's control entry. */
	    ++op;
	    if (op->opcode == DM1CN_OP_NULLCHK)
		++op;
	    break;	/* switch */

	case DM1CN_OP_NULLCHK:
	    /* Check for null.  An incoming null is just one byte, otherwise
	    ** ensure that the destination null indicator is zero.
	    ** Note that we clear the value part of a null, this may help
	    ** with any outside-world compression that uses this value.
	    */
	    if (*src++ & ADF_NVL_BIT)
	    {
		MEfill(nnull_len, 0, dst);
		dst[nnull_len] = ADF_NVL_BIT;
		dst += dst_len;
		/* If source was a coupon, move past it, otherwise it was
		** just the null indicator and no value.
		*/
		if (op->flags & DM1CN_PERIPHERAL)
		    src += nnull_len;
		/* Skip column's control op since column is NULL */
		++op;
	    }
	    else
		dst[nnull_len] = 0;
	    break;

	case DM1CN_OP_COPYN:
	    /* Just copy N bytes from source to result */
	    MEcopy(src, nnull_len, dst);
	    src += nnull_len;
	    dst += dst_len;	/* Skipping any possible null indicator */
	    break;

	case DM1CN_OP_CHR:
	    /* Copy until null, then space-fill tail */
	    p1 = dst;
	    p2 = dst + nnull_len - 1;	/* End of result value */
	    while (*src != '\0')
		*p1++ = *src++;
	    while (p1 <= p2)
		*p1++ = ' ';
	    dst += dst_len;
	    ++src;			/* eat the trailing null */
	    break;

	case DM1CN_OP_CHA:
	    /* Stored as if varchar, copy to result, space-fill */
	    p1 = dst;
	    p2 = dst + nnull_len - 1;	/* End of result value */
	    I2ASSIGN_MACRO(*(u_i2 *)src, i2_len);
	    src += sizeof(u_i2);
	    if (i2_len > nnull_len)
		goto overrun;
	    MEcopy(src, i2_len, p1);
	    p1 += i2_len;
	    while (p1 <= p2)
		*p1++ = ' ';
	    src += i2_len;
	    dst += dst_len;	/* Skipping any possible null indicator */
	    break;

	case DM1CN_OP_NCHR:
	    /* Like CHA except double-byte. */
	    /* Stored as if varchar, copy to result, space-fill */
	    p1 = dst;
	    p2 = dst + nnull_len - sizeof(UCS2);  /* End of result value */
	    I2ASSIGN_MACRO(*(u_i2 *)src, i2_len);
	    src += sizeof(u_i2);
	    i2_len *= sizeof(UCS2);	/* Convert char count to octet count */
	    if (i2_len > nnull_len)
		goto overrun;
	    MEcopy(src, i2_len, p1);
	    p1 += i2_len;
	    u_ch = U_BLANK;
	    while (p1 <= p2)
	    {
		I2ASSIGN_MACRO(u_ch, *(u_i2 *)p1);
		p1 += sizeof(UCS2);
	    }
	    src += i2_len;
	    dst += dst_len;	/* Skipping any possible null indicator */
	    break;

	case DM1CN_OP_VCH:
	    /* varchar and text stored exact-length */
	    I2ASSIGN_MACRO(((DB_TEXT_STRING *)src)->db_t_count, i2_len);
	    i2_len += sizeof(u_i2);
	    if (i2_len > nnull_len)
		goto overrun;
	    MEcopy(src, i2_len, dst);
	    src += i2_len;
	    /* Zero-fill the rest, keeps external compression happy */
	    if (i2_len < nnull_len)
		MEfill(nnull_len - i2_len, 0, dst + i2_len);
	    dst += dst_len;
	    break;

	case DM1CN_OP_NVCHR:
	    /* Same as varchar with double-byte adjustment */
	    I2ASSIGN_MACRO(((DB_TEXT_STRING *)src)->db_t_count, i2_len);
	    i2_len *= sizeof(UCS2);
	    i2_len += sizeof(u_i2);
	    if (i2_len > nnull_len)
		goto overrun;
	    MEcopy(src, i2_len, dst);
	    src += i2_len;
	    /* Zero-fill the rest, keeps external compression happy */
	    if (i2_len < nnull_len)
		MEfill(nnull_len - i2_len, 0, dst + i2_len);
	    dst += dst_len;
	    break;

	case DM1CN_OP_VLUDT:
	    /* UDT with its own compressing routine, call it */
	    att = attpp[op->attno];
	    tovalue.db_datatype = abs(att->type);
	    tovalue.db_data = src;
	    tovalue.db_prec = att->precision;
	    tovalue.db_length = nnull_len;
	    (void) adc_compr(adf_scb, &tovalue, FALSE, dst, &nnull_len);
	    src += nnull_len;
	    dst += dst_len;
	    break;

	} /* switch */
	++op;
    } while (op < end_op);
    /* Now fill that part of the destination buffer that wasn't used */
    dst_len = dst - orig_dst;
    if (dst_len < dst_maxlen)
	MEfill(dst_maxlen - dst_len, '\0', (char *)dst);

    *dst_size = dst_len;
    return (E_DB_OK);


overrun:
    uleFormat(NULL, E_DM9388_BAD_DATA_LENGTH, (CL_ERR_DESC *)NULL, ULE_LOG,
	NULL, (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 1,
	0, (i4)i2_len);
    uleFormat(NULL, E_DM9389_BAD_DATA_VALUE, (CL_ERR_DESC *)NULL, ULE_LOG,
	NULL, (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
    return (E_DB_ERROR);
} /* dm1cn_uncompress */

/*{
** Name: dmpp_compexpand -- how much can a tuple expand when compressed?
**
** Description:
**	Though the obvious goal of compression is to shrink actual tuple
**	size, in all our compression algorithms there is potential growth
**	in the worst case.  Given the list of attributes for a relation,
**	this accessor returns the amount by which a tuple may expand in the
**	worst case.
**
**	C columns can grow by one byte, since they are stored compressed
**	with a trailing '\0'.  CHAR and NCHAR can grow by two bytes,
**	since they are converted to VARCHAR / NVARCHAR which adds a
**	length count to the compressed value.  In all cases the worst-case
**	occurs when the value completely fills the declared column width.
**
**	The worst-case expansion is computed for a current version row.
**
** Inputs:
**      atts_array                      Pointer to an array of attribute 
**                                      descriptors.
**      atts_count                      Number of entries in atts_array.
**
** Outputs:
**	Returns worst-case expansion in bytes.
**
** Side Effects:
**          none.
**
** History:
**      08-nov-85 (jennifer)
**          Converted for Jupiter.
**	28-sep-88 (sandyh)
**	    Added support for CHAR, VARCHAR, and nullable datatypes.
**	08-mar-89 (arana)
**	  Changed MEcopy of u_i2 to I2ASSIGN_MACRO.
**	29-may-89 (rogerk)
**	    Added checks for bad data values so we don't write bad data
**	    to the database that cannot be uncompressed.
**	    Changed from being VOID routine to being DB_STATUS.
**	    Took out IFDEF's for byte alignment.
**	10-jul-89 (jas)
**	    Changed to multiple routines for multiple page format support.
**	03-aug-89 (kimman)
**	    Used MECOPY_VAR_MACRO instead of MEcopy.
**	20-Feb-2008 (kschendel)
**	    NCHAR works like CHAR and can add two bytes.
*/

i4	
dm1cn_compexpand(
			DB_ATTS		**attpp,
			i4		att_cnt)
{
    register i4  expansion = 0;
    register i4  abs_type;
    register DB_ATTS	*att;

    while (--att_cnt >= 0)
    {
	att = *attpp++;

	if (att->ver_dropped)
	   continue;

	abs_type = (att->type < 0) ? -att->type : att->type;
	if (abs_type == DB_CHR_TYPE)
	    ++expansion;
	else if (abs_type == DB_CHA_TYPE || abs_type == DB_NCHR_TYPE)
	    expansion += DB_CNTSIZE;
    }
    return expansion;
}

/*
** Name: dm1cn_cmpcontrol_setup - Set up compression control array
**
** Description:
**
**	This routine "sets up" the compression control array for standard
**	compression.  Standard compression doesn't touch non-character,
**	non-null columns, so we can just copy them.  Likewise, we can
**	arrange to ask for null testing and row-version testing only for
**	those attributes that actually need it.
**
**	The caller will have allocated space to put the control array,
**	and the pointer to that space (and its size in bytes) will be
**	in the ROWACCESS structure that we're setting up.  It's OK to
**	not use all of the allocated space, but it's obviously not OK
**	to try to use more.
**
** Inputs:
**	rac			Pointer to DMP_ROWACCESS with att_ptrs,
**				att_count, cmp_control set
**	    .control_count	Available size of control array in bytes
**
** Outputs:
**	rac			Control array set up
**	    .control_count	Set to number of entries in the array
**	Returns E_DB_OK or error status
**
** History:
**	2-Apr-2008 (kschendel) SIR 122739
**	    Written.
**	3-Nov-2009 (kschendel) SIR 122739
**	    Remove C99 initializer because MS's C compiler is an oozing,
**	    pustulent excrescence.
**	7-May-2010 (kschendel)
**	    Fix to never compress encrypted columns, not even null-compression.
*/

/* Define a helper table indexed by (abs) type.
** This is an ORDER SENSITIVE table.
** If it weren't for MSVC we could use C99 initializers here, e.g.
** [DB_CHA_TYPE] = DM1CN_OP_CHA, etc.  But noooooo....
*/
static const i4 dm1cn_opcodes[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* Types 0..9 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* Types 10..19 */
	DM1CN_OP_CHA, DM1CN_OP_VCH, 0, 0, 0,	/* 20..24 */
	0, DM1CN_OP_NCHR, DM1CN_OP_NVCHR, 0, 0,	/* 25..29 */
	0, 0, DM1CN_OP_CHR, 0, 0, 0, 0, DM1CN_OP_VCH 	/* 30..37 */
};
#define DM1CN_OPTAB_MAX (sizeof(dm1cn_opcodes)/sizeof(i4))


DB_STATUS
dm1cn_cmpcontrol_setup(DMP_ROWACCESS *rac)
{
    ADF_CB dummy;		/* Stub to hand to ADI function */
    bool cumok;			/* Not checked column, group COPY ok */
    DB_ATTS *att;		/* Current attribute for inspection */
    DB_ATTS **attpp;		/* Ptr to att ptrs array */
    DM1CN_CMPCONTROL *guard;	/* End-of-space checker */
    DM1CN_CMPCONTROL *op;	/* Control entry being built */
    i4 abstype;
    i4 attno, attcount;		/* Attribute index and count */
    i4 att_type, att_len;	/* Attribute type and length (adjusted) */
    i4 control_count;		/* Number of entries so far */
    i4 cum_length;		/* Accumulated length for COPYN */
    i4 dt_bits;			/* Data type flags */
    i4 opcode;
    i4 toss;

    MEfill(sizeof(dummy), 0, (PTR) &dummy);
    dummy.adf_maxstring = DB_MAXSTRING;
    attpp = rac->att_ptrs;
    attcount = rac->att_count;
    op = (DM1CN_CMPCONTROL *) rac->cmp_control;
    guard = (DM1CN_CMPCONTROL *) ((char *) op + rac->control_count);
    cum_length = 0;
    /* Operate via increment-then-use */
    --op;
    control_count = 0;
    attno = -1;
    /* Assume at least one att! (do-while instead of while-do) */
    do
    {
	++attno;
	att = *attpp++;
	att_type = att->type;
	att_len = att->length;
	abstype = abs(att_type);
	cumok = TRUE;
	if (att->encflags & ATT_ENCRYPT)
	{
	    /* Encrypted columns are never compressed, and since the null
	    ** is encrypted into the data, they aren't null-compressed either.
	    */
	    att_type = abstype;
	    att_len = att->encwid;
	}
	if (att_type < 0 || att->ver_added != 0 || att->ver_dropped != 0)
	{
	    /* Needs a ver-check or null-check or both.  Tie off any
	    ** COPYN that might be in progress, and make sure that this
	    ** column is handled by itself (even if non-compressed type).
	    */
	    cumok = FALSE;
	    if (cum_length > 0)
	    {
		op->length = cum_length;
		cum_length = 0;
	    }
	    if (att->ver_added != 0 || att->ver_dropped != 0)
	    {
		++op;
		if (op >= guard)
		    goto space_error;	/* common error jump */
		++control_count;
		op->opcode = DM1CN_OP_VERSCHK;
		op->flags = 0;
		if (att->ver_dropped > 0)
		{
		    if (att->ver_altcol == 0)
		    {
			op->flags |= DM1CN_DROPPED;
		    }
		    else
		    {
			/* Things get tricky with ALTER.  This column may have
			** gone thru a series of changes, and then been dropped
			** entirely.
			** Fortunately all the att entries for the column
			** variants are together, with the same ordinal_id
			** and successively increasing intl_id's.  If we come
			** to an att with this ordinal that is dropped and not
			** also altered, the column was ultimately dropped.
			** Otherwise we must necessarily come to an att that
			** was added and NOT dropped, and we mark this column
			** as ALTERED but not DROPPED.
			** Danger: normally, ordinal's aren't ever reused,
			** but due to a glitch in add column they can be,
			** at the end.  e.g. alter / drop / add will have 3
			** atts, one altered, one a pure drop, and one an add.
			** All with the same ordinal ID.
			** Fortunately it's easy enough to catch the pure drop.
			** (A lot easier to figure this crap out once, rather
			** than over and over for every row!)
			*/

			DB_ATTS **searchpp = attpp;
			i4 flag;
			i4 search_count = attcount;

			flag = DM1CN_DROPPED;
			while (--search_count > 0)
			{
			    if ((*searchpp)->ordinal_id != att->ordinal_id
			      || ( (*searchpp)->ver_dropped > 0
				   && (*searchpp)->ver_altcol == 0) )
				break;	/* Off the end, or definite drop */
			    if ((*searchpp)->ver_dropped == 0)
			    {
				flag = DM1CN_ALTERED;
				break;
			    }
			    ++ searchpp;
			}
			op->flags |= flag;
		    }
		}
		op->length = att_len;
		if (att_type < 0)
		{
		    --op->length;
		    op->flags |= DM1CN_NULLABLE;
		}
		op->attno = attno;
	    }
	    if (att_type < 0)
	    {
		++op;
		if (op >= guard)
		    goto space_error;	/* common error jump */
		++control_count;
		op->opcode = DM1CN_OP_NULLCHK;
		op->flags = DM1CN_NULLABLE;
		op->length = att_len - 1;
		op->attno = attno;
		/* Light flag if this is a coupon type.
		** NOTE:  the only reason dtinfo needs an ADF CB is for
		** error construction, and we don't really care about that.
		** If this changes, consider using get-sid and get-dml-scb
		** to locate the session ADF_CB rather than passing it.
		*/
		(void) adi_dtinfo(&dummy, abstype, &dt_bits);
		if (dt_bits & AD_PERIPHERAL)
		    op->flags |= DM1CN_PERIPHERAL;
	    }
	}

	/* Now emit an opcode to handle this column, or accumulate
	** a COPYN if non-compressed and non-checked column.
	*/
	if (att->encflags & ATT_ENCRYPT)
	{
	    opcode = DM1CN_OP_COPYN;	/* Never compress if encrypted */
	}
	else if (abstype < DM1CN_OPTAB_MAX)
	{
	    opcode = dm1cn_opcodes[abstype];
	    if (opcode == 0)
		opcode = DM1CN_OP_COPYN;
	}
	else
	{
	    /* Check for UDT defined with its own compressor */
	    opcode = DM1CN_OP_COPYN;
            if (abstype >= ADI_DT_CLSOBJ_MIN)
	    {
		dt_bits = 0;
		(void) adi_dtinfo(&dummy, abstype, &dt_bits);
	        if (dt_bits & AD_VARIABLE_LEN)
		    opcode = DM1CN_OP_VLUDT;
	    }
	}
	if (opcode == DM1CN_OP_COPYN && cum_length > 0)
	{
	    /* Just tag this column onto a group being copied.
	    ** Column can't be nullable or cum would have been zeroed.
	    */
	    cum_length += att_len;
	}
	else
	{
	    /* Starting a new entry, tie off any accumulated copy */
	    if (cum_length > 0)
	    {
		op->length = cum_length;
		cum_length = 0;
	    }
	    ++op;
	    if (op >= guard)
		goto space_error;	/* common error jump */
	    ++control_count;
	    op->opcode = opcode;
	    op->attno = attno;
	    op->flags = 0;
	    op->length = att_len;
	    if (opcode == DM1CN_OP_COPYN && cumok)
		cum_length = op->length;
	    if (att_type < 0)
	    {
		--op->length;
		op->flags |= DM1CN_NULLABLE;
	    }
	}
    } while (--attcount > 0);
    if (cum_length > 0)
	op->length = cum_length;
    rac->control_count = control_count;
    return (E_DB_OK);

space_error:
    uleFormat(NULL, E_DM9398_CMPCONTROL_SPACE, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, &toss, 2,
		0, (i4) rac->control_count,
		0, (i4) rac->att_count);
    return (E_DB_ERROR);
} /* dm1cn_cmpcontrol_setup */

/*
** Name: dm1cn_cmpcontrol_size - Estimate control array size
**
** Description:
**
**	This routine estimates the size needed for the compression control
**	array used by standard compression.
**
**	In a perfect world, we would be passed an attribute array.
**	A quick scan of the attribute array for nullables and added or
**	dropped attributes would produce a reasonable upper estimate of
**	the size needed.
**
**	Unfortunately, most if not all callers want to include the control
**	array as part of a larger structure (a TCB or MCT).  The attribute
**	details may not even be known at that stage.  Deferring the control
**	size estimate until later would work, but then the array would
**	have to be separately allocated, a nuisance.
**
**	For the time being, we'll estimate the array size based on a worst
**	case estimate of one null-check and one compression operation,
**	per attribute -- i.e. 2x the attribute count times the size of
**	a control array entry.  If the table relversion is nonzero,
**	which implies that some attributes have been added / dropped,
**	we need to include possible version check ops, making the
**	worst-case estimate 3x the attribute count.
**
**	The hope is that the extra memory wastage is irrelevant in the
**	big scheme of things.  If this turns out to be false, we'll have
**	to reshuffle things and allocate the control array separately.
**
** Inputs:
**	att_count		Number of attributes in the row
**	relversion		Current maximum row version (only care
**				about zero vs nonzero)
**
** Outputs:
**	Returns an upper limit on the control array size, in bytes.
**
** History:
**	20-Feb-2008 (kschendel) SIR 122739
**	    Part of the row-accessing overhaul.
*/

i4
dm1cn_cmpcontrol_size(i4 att_count, i4 relversion)
{

    if (relversion == 0)
	return (2 * att_count * sizeof(DM1CN_CMPCONTROL));
    else
	return (3 * att_count * sizeof(DM1CN_CMPCONTROL));
}
