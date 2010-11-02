/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <bt.h>
#include    <cm.h>
#include    <cv.h>
#include    <er.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>	/* needed for psfparse.h */
#include    <dmucb.h>
#include    <adf.h>	/* needed for psfparse.h */
#include    <adfops.h>
#include    <ddb.h>	/* needed for psfparse.h */
#include    <ulf.h>	/* needed for qsf.h, rdf.h and pshparse.h */
#include    <qsf.h>	/* needed for psfparse.h and qefrcb.h */
#include    <qefrcb.h>	/* needed for psfparse.h */
#include    <psfparse.h>
#include    <dmtcb.h>	/* needed for rdf.h and pshparse.h */
#include    <qu.h>	    /* needed for pshparse.h */
#include    <rdf.h>	    /* needed for pshparse.h */
#include    <psfindep.h>    /* needed for pshparse.h */
#include    <pshparse.h>
#include    <psftrmwh.h>
#include    <psldef.h>
#include    <cui.h>
#include    <usererror.h>

/*
** Name: PSLDEFAU.C:	this file contains functions used to handle INGRES and
** 			user-defined defaults (during CREATE TABLE and INSERT).
**
** Description:
**	this file contains functions used by the SQL grammar in
**	creating and using INGRES and user-defined defaults on columns.
**
**	Routines (external):
**	    psl_1col_default        - set up the default ID/tuple for a column
**	    			     during CREATE TABLE
**	    psl_check_defaults     - find all unspecified columns (during an
**	    			     INSERT/APPEND) and add defaults to qtree
**	    psl_make_default_node  - build a default CONST node for a column
**	    psl_2col_ingres_default - set up the default ID for INGRES default
**
**	Routines (internal):
**	    psl_col_user_default   - set up the user-defined default tuple
**	    psl_make_def_const     - create const node, given a default string
**	    psl_make_func_node     - create const-operator for an ADF func
**	    psl_make_canon_default - create node for a canonical default
**	    psl_make_user_default  - create const node for user-defined default
**
**  History:
**      29-jan-93 (rblumer)    
**          created.
**          Moved psl_col_default here (from pslctbl.c) and added
**          psl_check_defaults, psl_make_default_node, and supporting routines.
**      16-mar-93 (rblumer)    
**          Changed DEFAULT_TOO_LONG error to 4A0.
**	24-mar-93 (rblumer)
**	    change psl_col_user_default to fix bug where nullable columns
**	    couldn't have user functions as a default; change
**	    psl_col_ingres_default and psl_make_user_default to call CUI
**	    functions to handle embedded quotes.
**	10-apr-93 (rblumer)
**	    in psl_col_user_default, check for varchar defaults that are
**	    too long; replace bogus error message with new error message.
**	20-apr-93 (rblumer)
**	    made psl_col_ingres_default non-static; it's now called by the QUEL
**	    grammar.  Changed external names to psl_1col_default and
**	    psl_2col_ingres_default, so that they are unique within 7 chars.
**	18-may-93 (rblumer)
**	    in psl_col_user_default, when checking that char field can contain
**	    a user-name, adjust attribute size for null byte and count field.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	08-jul-93 (rblumer)
**	    change psl_check_defaults to correctly handle inserts in Star.
**	10-aug-93 (rblumer)
**	    in psl_2col_ingres_default:
**	    use new error messages for ADF failures during UDT processing.
**	    This should make it easier for users to debug UDTs and defaults.
**	    in psl_col_user_default:
**	    change error processing after adc_defchk call to handle
**	    ADF_EXTERNAL errors, which can be returned by users' UDT functions.
**	13-aug-93 (rblumer)
**	    in order to allow ADF to ignore math exceptions when desired,
**	    we set the PSS_CALL_ADF_EXCEPTON bit to tell the PSF exception
**	    handler to call the ADF exception handler whenever we do math on
**	    behalf of the user. 
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
**	13-sep-93 (rblumer)
**	    in psl_1col_default,
**	    we were overwriting attr_flags_mask; use |= to set bit instead.
**	20-jan-94 (rblumer)
**	    B58651: in psl_1col_default, disallow defaults on long datatypes
**	    (i.e. blob datatypes) because it is not fully supported.
**	20-jan-94 (rblumer)
**	    Fix DEFAULT bugs 58994 and 58995 by adding specials cases for
**	    null-string defaults in UDT default generation
**	    (psl_2col_ingres_default) and in query tree node generation 
**          (psl_make_user_default)
**	06-sep-94 (mikem)
**	    bug #59589
**	    Changed psl_col_user_default() to use the LONGTXT version of the
**	    datatype when calling adf rather than the scanner presented version
**	    to check whether a given default value could be coerced into the 
**	    destination datatype.  This addresses the case where 13.2E3 is 
**	    given as a default value for a money column - this is an illegal
**	    money format but the previous code would not catch this because
**	    the parser's scanner would present the token as a float which does
**	    coerce into a money value.  If anyone adds support to the money
**	    datatype to allow exponential notation to be legal money format this
**	    code will still work (ie. it just asks adf if 13.2E3 is a legal
**	    money format).
**	20-dec-94 (nanpr01)
**	    bug # 66144 & 66146
**          Backed out part of mikem's previous change. The change is only
**          valid for float type where it would display illegal value if
**          one enters the default value in a money field in exponential
**          format.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    psl_rptqry_tblids(), psl_cons_text(), psl_gen_alter_text(),
**	    psq_tmulti_out(), psq_1rptqry_out(), psq_tout().
**      19-Nov-2003 (hanal04) Bug 111323 INGSRV2606
**          Added default handling for unicode columns in 
**          psl_2col_ingres_default()
**	12-Jun-2006 (kschendel)
**	    Allow sequence defaults (next value for <seq>).
**	16-Jun-2009 (thich01)
**	    Treat GEOM type the same as LBYTE.
**	20-Aug-2009 (thich01)
**	    Treat all spatial types the same as LBYTE.
**      04-sep-2009 (joea)
**          Add cases for DB_BOO_TYPE in psl_2col_ingres_default and
**          psl_col_user_default.  Add cases for DB_DEF_ID_FALSE and
**          DB_DEF_ID_TRUE in psl_make_canon_default.
**      23-oct-2009 (joea)
*           Correct case for DB_DEF_ID_FALSE/TRUE in psl_make_canon_default.
**	05-Nov-2009 (kiria01) b122841
**	    Use psl_mk_const_similar to cast default values directly.
**	12-Nov-2009 (kiria01) b122841
**	    Corrected psl_mk_const_similar parameters with explicit
**	    mstream.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
*/


/*
** Name: psl_2col_ingres_default  - set up the default ID for a column declared
** 				   using WITH DEFAULT
**
** Description:	    
**      Handles WITH DEFAULT column definitions.
**      
**      For numeric or text datatypes, sets the default tuple to NULL and sets
**      the default ID to the appropriate canonical ID.  For logical datatypes
**      or user-defined datatypes, gets the default value from ADF, converts it
**      into long-text, and copies the default text into a default tuple and
**      attaches it to the column descriptor. 
**      
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_ostream	    memory stream to put text from chain into
** Outputs:
**	attr		    attribute descriptor for this column
**	    attr_defaultID	set to canon ID for canonical defaults
**	    attr_defaultTuple	set to NULL for numeric or text datatypes
**	    			set to default string for user-defined datatypes
**	err_blk		    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**
**	22-jan-93 (rblumer)
**	    written
**	08-mar-93 (rblumer)
**	    changed UDT section to add single quotes around the converted
**	    LTXT value;  also added check for DEFAULT too long.
**	15-mar-93 (rblumer)
**	    added return after find error in UDT section.
**	25-mar-93 (rblumer)
**	    replace temporary code for adding surrounding quotes with 
**	    call to cui_idunorm to add surrounding AND embedded quotes.
**	27-may-93 (robf)
**	     Added DB_SEC_TYPE/DB_SECID_TYPE to list of supported datatypes,
**	     default is blank (empty label)
**	10-aug-93 (rblumer)
**	    use new error messages for ADF failures during UDT processing.
**	    This should make it easier for users to debug UDTs and defaults.
**	13-aug-93 (rblumer)
**	    in order to allow ADF to ignore math exceptions when desired,
**	    we set the PSS_CALL_ADF_EXCEPTON bit to tell the PSF exception
**	    handler to call the ADF exception handler whenever we do math on
**	    behalf of the user. 
**	20-jan-94 (rblumer)		B58994
**	    if UDT default value is a 0-length string (e.g. for BYTE or VARBYTE,
**	    since they were not made a canonical value), cui_idunorm will look
**	    at garbage; change this to create a null string-constant instead.
**      19-Nov-2003 (hanal04) Bug 111323 INGSRV2606
**          Added case for unicode types. Using the default causes us to 
**          referenced uninitialised db_data content in adu_lenaddr()
**          and generates spurious E_AD1014 failures when creating a table
**          with unicode columns.
**	15-jul-04 (toumi01)
**	    Add default value support for BYTE and VARBYTE.
**	17-jul-06 (gupsh01)
**	    Added support for ANSI datetime datatype.
**	20-dec-2006 (dougi)
**	    Add case for long byte so defaults are allowed for all LOBs.
**	17-Dec-2008 (kiria01) SIR120473
**	    Initialise the ADF_FN_BLK.adf_pat_flags.
**	23-Sep-2009 (kiria01) b122578
**	    Initialise the ADF_FN_BLK .adf_fi_desc and adf_dv_n members.
*/
DB_STATUS
psl_2col_ingres_default(
		       PSS_SESBLK     *sess_cb,
		       DMF_ATTR_ENTRY *attr,
		       DB_ERROR	      *err_blk)
{
    /* we have an INGRES default,
    ** so set up default id based on datatype;
    ** use absolute value, because we don't care if datatype is nullable or not
    */
    switch (abs(attr->attr_type))
    {
	case DB_INT_TYPE:
	case DB_FLT_TYPE:
	case DB_MNY_TYPE:
	case DB_DEC_TYPE:
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_0);
	    break;

        case DB_BOO_TYPE:
            SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_FALSE);
            break;

	case DB_CHR_TYPE:
	case DB_TXT_TYPE:
	case DB_DTE_TYPE:
	case DB_INYM_TYPE:
	case DB_INDS_TYPE:
	case DB_CHA_TYPE:
	case DB_BYTE_TYPE:
	case DB_VBYTE_TYPE:
	case DB_LBYTE_TYPE:
	case DB_GEOM_TYPE:
        case DB_POINT_TYPE:
        case DB_MPOINT_TYPE:
        case DB_LINE_TYPE:
        case DB_MLINE_TYPE:
        case DB_POLY_TYPE:
        case DB_MPOLY_TYPE:
        case DB_GEOMC_TYPE:
	case DB_VCH_TYPE:
	case DB_LVCH_TYPE:
	case DB_NCHR_TYPE:
	case DB_NVCHR_TYPE:
	case DB_LNVCHR_TYPE:
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_BLANK);
	    break;
	    
	case DB_ADTE_TYPE:
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_CURRENT_DATE);
	    break;
	    
	case DB_TMWO_TYPE:
	case DB_TMW_TYPE:
	case DB_TME_TYPE:
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_CURRENT_TIME);
	    break;
	    
	case DB_TSWO_TYPE:
	case DB_TSW_TYPE:
	case DB_TSTMP_TYPE:
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_CURRENT_TIMESTAMP);
	    break;
	    
	case DB_LOGKEY_TYPE:
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_LOGKEY);
	    break;
	    
	case DB_TABKEY_TYPE:
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_TABKEY);
	    break;
	    
	default:
	{
	    /* attr_type is a UDT,
	    ** and we have no canonical defaults for these types;
	    ** instead we will get the default value via adc_getempty 
	    ** and convert it to a longtext to store in the catalog
	    */
	    ADF_CB	   *adf_scb;
	    ADI_FI_ID	   conv_func_id;
	    ADF_FN_BLK	   conv_func_blk;
	    ADI_DT_NAME	   type_name;
	    DB_DATA_VALUE  db_data;
	    DB_IIDEFAULT   *def_tuple;
	    DB_STATUS	   status;
	    i4	   err_code;
	    DB_TEXT_STRING *def_value;
	    u_i4	   len, unorm_len;
    
	    
	    adf_scb = (ADF_CB*) sess_cb->pss_adfcb;

	    /* use absolute value of datatype to make sure it is non-null,
	    ** as we always want to get a non-null value back
	    */
	    db_data.db_datatype = abs(attr->attr_type);
	    db_data.db_length   = attr->attr_size;
	    db_data.db_prec     = attr->attr_precision;

	    /* if datatype is nullable,
	    ** subtract 1 so that null byte is not counted in the length
	    */
	    if (attr->attr_type < 0)
		db_data.db_length--;

	    /* allocate space for the default_value
	     */
	    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, db_data.db_length, 
				(PTR *) &db_data.db_data, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return (status);

	    /* in order to allow ADF to ignore math exceptions when desired, we
	    ** set a bit to tell the PSF exception handler to call the ADF
	    ** exception handler whenever we do math on behalf of the user.
	    */
	    sess_cb->pss_stmt_flags |= PSS_CALL_ADF_EXCEPTION;

	    status = adc_getempty (adf_scb, 
				   &db_data);

	    /* reset the flag,
	    ** as we aren't doing math on behalf of the user anymore
	    */
	    sess_cb->pss_stmt_flags &= ~PSS_CALL_ADF_EXCEPTION;

	    if (DB_FAILURE_MACRO(status))
	    {
		err_code = adf_scb->adf_errcb.ad_errcode;
		(void) adi_tyname(adf_scb, db_data.db_datatype, &type_name);

		(void) psf_error(E_PS04A7_UDT_NO_GETEMPTY,
				 err_code,
				 PSF_INTERR, &err_code, err_blk, 3,
				 sizeof(ERx("CREATE TABLE")) -1,
				 ERx("CREATE TABLE"),
				 psf_trmwhite(sizeof(ADI_DT_NAME), 
					      type_name.adi_dtname), 
				 type_name.adi_dtname,
				 psf_trmwhite(sizeof(DB_ATT_NAME), 
					      attr->attr_name.db_att_name),
				 attr->attr_name.db_att_name);
		return (status);
	    }

	    /* get function for converting value to long-text
	     */
	    status = adi_ficoerce(adf_scb, db_data.db_datatype, 
				  DB_LTXT_TYPE, &conv_func_id);
	    if (DB_FAILURE_MACRO(status))
	    {
		err_code = adf_scb->adf_errcb.ad_errcode;
		(void) adi_tyname(adf_scb, db_data.db_datatype, &type_name);

		(void) psf_error(E_PS04A6_UDT_NO_LONGTXT,
				 err_code,
				 PSF_INTERR, &err_code, err_blk, 3,
				 sizeof(ERx("CREATE TABLE")) -1,
				 ERx("CREATE TABLE"),
				 psf_trmwhite(sizeof(ADI_DT_NAME), 
					      type_name.adi_dtname), 
				 type_name.adi_dtname,
				 psf_trmwhite(sizeof(DB_ATT_NAME), 
					      attr->attr_name.db_att_name),
				 attr->attr_name.db_att_name);
		return (status);
	    }

	    /*
	    ** set up function block to convert the default value,
	    ** and call ADF to execute it
	    */
	    /* set up the 1st (and only) parameter
	     */
	    conv_func_blk.adf_fi_desc = 0;
	    conv_func_blk.adf_dv_n = 1;
	    STRUCT_ASSIGN_MACRO(db_data, conv_func_blk.adf_1_dv);

	    /* set up the result value;
	    ** reserve enough space for the largest possible default, and
	    ** include space for the 'count' field in a long-text data value
	    **
	    */
	    conv_func_blk.adf_r_dv.db_datatype = DB_LTXT_TYPE;
	    conv_func_blk.adf_r_dv.db_prec     = 0;
	    conv_func_blk.adf_r_dv.db_length   = DB_MAX_COLUMN_DEFAULT_LENGTH
						    + DB_CNTSIZE;
	    
	    /* allocate space for the text data
	     */
	    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
				conv_func_blk.adf_r_dv.db_length,
				(PTR *) &def_value, err_blk);

	    conv_func_blk.adf_r_dv.db_data = (PTR) def_value;
	    
	    /* set up the function id and execute it
	     */
	    conv_func_blk.adf_fi_id = conv_func_id;
	    conv_func_blk.adf_pat_flags = AD_PAT_NO_ESCAPE;

	    /* in order to allow ADF to ignore math exceptions when desired, we
	    ** set a bit to tell the PSF exception handler to call the ADF
	    ** exception handler whenever we do math on behalf of the user.
	    */
	    sess_cb->pss_stmt_flags |= PSS_CALL_ADF_EXCEPTION;

	    status = adf_func(adf_scb, 
			      &conv_func_blk);

	    /* reset the flag,
	    ** as we aren't doing math on behalf of the user anymore
	    */
	    sess_cb->pss_stmt_flags &= ~PSS_CALL_ADF_EXCEPTION;

	    if (DB_FAILURE_MACRO(status))
	    {
		err_code = adf_scb->adf_errcb.ad_errcode;
		(void) adi_tyname(adf_scb, db_data.db_datatype, &type_name);

		(void) psf_error(E_PS04A8_UDT_LTXT_CONVERSION,
				 err_code,
				 PSF_INTERR, &err_code, err_blk, 3,
				 sizeof(ERx("CREATE TABLE")) -1,
				 ERx("CREATE TABLE"),
				 psf_trmwhite(sizeof(ADI_DT_NAME), 
					      type_name.adi_dtname), 
				 type_name.adi_dtname,
				 psf_trmwhite(sizeof(DB_ATT_NAME), 
					      attr->attr_name.db_att_name),
				 attr->attr_name.db_att_name);
		return (status);
	    }

	    /* allocate an IIDEFAULT tuple to store the default in
	    ** and then unnormalize returned string directly into it
	    ** (unnormalize means add surrounding and embedded single-quotes)
	    */
	    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
				sizeof(DB_IIDEFAULT), 
				(PTR *) &def_tuple, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return(status);

	    MEfill(sizeof(DB_IIDEFAULT), NULLCHAR, (PTR) def_tuple);

	    /* Only call cui_idunorm if the default length is NOT zero,
	    ** because cui_idunorm treats a length of zero as a signal that the
	    ** string is null-terminated, and still tries to look at the string.
	    ** This causes it to look at garbage in the string.
	    */
	    if (def_value->db_t_count != 0)
	    {
		len       = def_value->db_t_count;
		unorm_len = DB_MAX_COLUMN_DEFAULT_LENGTH;
	
		status = cui_idunorm(def_value->db_t_text, &len,
				     (u_char *) def_tuple->dbd_def_value, 
				     &unorm_len,
				     CUI_ID_QUO | CUI_ID_NOLIMIT, err_blk);

		def_tuple->dbd_def_length = unorm_len;
	    }
	    else
	    {
		/* since the length is zero, set the value to the null string
		** (i.e. two single quotes)
		*/
		def_tuple->dbd_def_value[0] = '\'';
		def_tuple->dbd_def_value[1] = '\'';
		def_tuple->dbd_def_length = 2;
		status = E_DB_OK;
	    }
	    
	    if (DB_FAILURE_MACRO(status))
	    {
		if (err_blk->err_code == E_US1A25_ID_TOO_LONG)
		{
		    /* default is too long; return error
		     */
		    i4  size = DB_MAX_COLUMN_DEFAULT_LENGTH;

		    (void) psf_error(E_PS04A0_DEFAULT_TOO_LONG, 0L, PSF_USERERR,
				     &err_code, err_blk, 2,
				     psf_trmwhite(sizeof(DB_ATT_NAME), 
						  attr->attr_name.db_att_name),
				     attr->attr_name.db_att_name,
				     sizeof(size), &size);
		}
		else 
		{
		    /* have some other error */
		    (void) psf_error(err_blk->err_code, 0L, PSF_INTERR,
				     &err_code, err_blk, 2,
				     sizeof(ERx("Normalized char default")),
				     ERx("Normalized char default"),
				     def_tuple->dbd_def_length,
				     def_tuple->dbd_def_value);
		}
		return(status);
	    }

	    /* store default value in the attribute descriptor
	     */
	    attr->attr_defaultTuple = def_tuple;

	}  /* end default case (attr_type is a UDT or unknown type) */
	    
    }  /* end switch (attr_type) */

    return(E_DB_OK);

}  /* end psl_2col_ingres_default */


/*
** Name: psl_col_user_default  - set up the default tuple for a column
**
** Description:	    
**      Handles DEFAULT <value> column definitions.
**
**      For canonical defaults, sets the default tuple to NULL and sets
**      the default ID to the appropriate canonical ID.  For user-defined
**      defaults, sets default ID to 0 and copies the default text into the
**      default tuple and attaches it to the column descriptor.
**      
**      Sets other bits as necessary (e.g. NDEFAULT).
**
**      Canonical defaults are the INGRES defaults (0 and blank) and the
**      constant functions like USER or NULL (and future ones will be
**      CURRENT_TIME, CURRENT_DATE, ...). 
**
**	If the user default is a sequence operator rather than a literal,
**	we'll rebuild the default text to assure a canonical form
**	(namely, next value for "owner"."seqname").  For user default
**	literals, we'll depend on the text saved up by the text-save
**	mechanism and passed in as def_text.
**      
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_ostream	    memory stream to put text from chain into
**	attr		    attribute descriptor for this column
**			    	filled in with default info
**	def_txt		    text of default value, as user typed it in
**	def_node	    CONST node or function node representing default
**				used to check type/size validity of default
** Outputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_tchain	    reset to NULL 
**	attr		    attribute descriptor for this column
**	    attr_flags_mask	NDEFAULT bit set for NOT DEFAULT columns
**	    attr_defaultTuple	set to default string for user-defined defaults
**	    			set to NULL for no default or canonical default
**	    attr_defaultID	set to canon ID for canonical defaults
**	err_blk		    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**
**	20-nov-92 (rblumer)
**	    written
**	24-mar-93 (rblumer)
**	    use absolute value of attribute type to check for valid datatypes
**	    for user functions--otherwise nullable columns cause an error.
**	10-apr-93 (rblumer)
**	    check for varchar defaults that are too long;
**	    replace bogus error message with new error message.
**	16-apr-93 (rblumer)
**	    reversed order of CMnext and CMbytedec; otherwise are decrementing
**	    based on the wrong char and could decrement the wrong amount
**	18-may-93 (rblumer)
**	    when checking that a char field can contain a username,
**	    adjust attribute size for null byte and count field.
**	    Also correct number of params to psf_error so message prints out.
**	01-jun-93 (rblumer)
**	    add a check to return a syntax error if the def_node is not a
**	    constant or a constant operator node.  This should only happen if
**	    we get a UOP node with a string, and we won't allow those as
**	    defaults, since the user can easily put the  '-' or '+'  into the
**	    string instead.
**	10-aug-93 (rblumer)
**	    change error processing after adc_defchk call to handle
**	    ADF_EXTERNAL errors, which can be returned by users' UDT functions;
**	    also handle other ADF errors slightly differently, so that we only
**	    print out ADF errors if they add information.
**	13-aug-93 (rblumer)
**	    in order to allow ADF to ignore math exceptions when desired,
**	    we set the PSS_CALL_ADF_EXCEPTON bit to tell the PSF exception
**	    handler to call the ADF exception handler whenever we do math on
**	    behalf of the user. 
**	06-sep-94 (mikem)
**	    bug #59589
**	    Changed psl_col_user_default() to use the LONGTXT version of the
**	    datatype when calling adf rather than the scanner presented version
**	    to check whether a given default value could be coerced into the 
**	    destination datatype.  This addresses the case where 13.2E3 is 
**	    given as a default value for a money column - this is an illegal
**	    money format but the previous code would not catch this because
**	    the parser's scanner would present the token as a float which does
**	    coerce into a money value.  If anyone adds support to the money
**	    datatype to allow exponential notation to be legal money format this
**	    code will still work (ie. it just asks adf if 13.2E3 is a legal
**	    money format).
**	20-dec-94 (nanpr01)
**	    bug # 66144 & 66146
**          Backed out part of mikem's previous change. The change is only
**          valid for float type where it would display illegal value if
**          one enters the default value in a money field in exponential
**          format.
**	20-dec-2006 (dougi)
**	    Add support for date/time registers.
*/
static 
DB_STATUS
psl_col_user_default(
		PSS_SESBLK	*sess_cb,
		DMF_ATTR_ENTRY	*attr,
		DB_TEXT_STRING	*def_txt,
		PST_QNODE	*def_node,
		DB_ERROR	*err_blk)
{
    DB_IIDEFAULT   *def_tuple;
    DB_STATUS	   status;
    register u_char  *def_string;
    register u_char  *end_string;
    u_i2  	   def_length;
    ADF_CB	   *adf_scb;
    DB_DATA_VALUE  *dataval;
    DB_DATA_VALUE  resultval;
    f8		   fval;
    DB_TEXT_STRING *txt;
    i4		   size;
    i4	   dtype;
    i4	   err_code;
    bool	datetime = FALSE;


    /* first check if we have a constant-function default,
    ** like USER and other functions that return a username
    */
    if (def_node->pst_sym.pst_type == PST_COP)
    {
	switch (def_node->pst_sym.pst_value.pst_s_op.pst_opno)
	{
	case ADI_USRNM_OP:
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_USERNAME);
	    break;
	    
	case ADI_SESSUSER_OP:
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_SESSION_USER);
	    break;
	    
	case ADI_SYSUSER_OP:
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_SYSTEM_USER);
	    break;
	    
	case ADI_INITUSER_OP:
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_INITIAL_USER);
	    break;
	    
	case ADI_DBA_OP:
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_DBA);
	    break;
	    
	case ADI_CURDATE_OP:
	    datetime = TRUE;
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_CURRENT_DATE);
	    break;
	    
	case ADI_CURTIME_OP:
	    datetime = TRUE;
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_CURRENT_TIME);
	    break;
	    
	case ADI_CURTMSTMP_OP:
	    datetime = TRUE;
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_CURRENT_TIMESTAMP);
	    break;
	    
	case ADI_LCLTIME_OP:
	    datetime = TRUE;
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_LOCAL_TIME);
	    break;
	    
	case ADI_LCLTMSTMP_OP:
	    datetime = TRUE;
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_LOCAL_TIMESTAMP);
	    break;
	    
	default:
	    (void) psf_error(E_PS049B_BAD_DEFAULT_FUNC, 0L, PSF_INTERR, 
			     &err_code, err_blk, 1,
			     sizeof(def_node->pst_sym
				        .pst_value.pst_s_op.pst_opno),
			     &def_node->pst_sym.pst_value.pst_s_op.pst_opno);
	    return(E_DB_ERROR);
	}
	
	/* if datatype cannot hold all possible usernames
	** (i.e. is not a text datatype of at least DB_OWN_MAXNAME bytes),
	** return an error.
	** 
	** Note that we have to adjust for the null byte,
	** and for the count field in varchar and text.
	** Note also that the actual size of long varchar (blob)
	** is not related to attr_size.
	*/
	size = attr->attr_size;
	dtype = abs(attr->attr_type);

	if (attr->attr_type < 0)
	    size--;			/* adjust for null byte */

	if (   (dtype == DB_VCH_TYPE) 
	    || (dtype == DB_TXT_TYPE))
	    size -= DB_CNTSIZE;		/* adjust for count field */

	if (!datetime &&  (((size < DB_OWN_MAXNAME) && (dtype != DB_LVCH_TYPE))
	    || (   (dtype != DB_CHR_TYPE)
		&& (dtype != DB_TXT_TYPE)
		&& (dtype != DB_CHA_TYPE)
		&& (dtype != DB_VCH_TYPE)
		&& (dtype != DB_LVCH_TYPE))))
	{
	    size = DB_OWN_MAXNAME;
	    
	    (void) psf_error(E_PS0498_DEFAULT_USER_SIZE, 0L, PSF_USERERR, 
			     &err_code, err_blk, 2,
			     psf_trmwhite(sizeof(DB_ATT_NAME), 
					  attr->attr_name.db_att_name),
			     attr->attr_name.db_att_name,
			     sizeof(size), &size);
	    return(E_DB_ERROR);
	}
	return(E_DB_OK);

    }  /* end if def_node == PST_COP */
    else if (def_node->pst_sym.pst_type == PST_SEQOP)
    {
	char own[DB_OWN_MAXNAME+1], name[DB_MAXNAME+1];
	i4 abstype;

	/* Allow "next value for <sequence>" as a default.
	** This is very useful for building auto-incrementing columns.
	** We'll ignore the text gathered by the scanner, because we
	** want to avoid variations (like the oracle-style seq.nextval).
	** Instead, build:
	** next value for "owner"."sequencename"
	** with the delimiters in place regardless.
	*/

	abstype = abs(attr->attr_type);
	if (abstype != DB_INT_TYPE && abstype != DB_FLT_TYPE
	  && abstype != DB_DEC_TYPE)
	{
	    (void) psf_error(E_US0805_2053_SEQDEF_TYPE, 0, PSF_USERERR,
			&err_code, err_blk, 1,
			psf_trmwhite(sizeof(DB_ATT_NAME), 
				attr->attr_name.db_att_name),
			attr->attr_name.db_att_name);
	    return (E_DB_ERROR);
	}
	/* Make sure that the default looks like "NEXT VALUE", not
	** "CURRENT VALUE".  (CURRENT VALUE is an Oraclysm, as Doug
	** put it.)  Someone used to using CURRENT might try to use it
	** after a NEXT default on the same sequence.  This is in fact
	** meaningless, so rather than checking, just ignore it.
	** (The proper ANSI semantics are that each sequence used by a
	** query is incremented, once, prior to operating on the row
	** affected by the query.  So multiple NEXT VALUE FOR s defaults
	** for the same sequence s all get the same values.)
	*/
	def_node->pst_sym.pst_value.pst_s_seqop.pst_seqflag &= ~PST_SEQOP_CURR;
	def_node->pst_sym.pst_value.pst_s_seqop.pst_seqflag |= PST_SEQOP_NEXT;

	status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
			sizeof(DB_IIDEFAULT), (PTR *) &def_tuple, err_blk);

	MEfill(sizeof(DB_IIDEFAULT), 0, (PTR) def_tuple);

	/* Generate space-trimmed copies of seqowner and seqname,
	** and then build the default string.  Might as well do it
	** directly into the default tuple value area.
	*/
	STlcopy((char *) &def_node->pst_sym.pst_value.pst_s_seqop.pst_owner,
		&own[0], sizeof(own)-1);
	STlcopy((char *) &def_node->pst_sym.pst_value.pst_s_seqop.pst_seqname,
		&name[0], sizeof(name)-1);
	STtrmwhite(&own[0]);
	STtrmwhite(&name[0]);
	STlpolycat(5, sizeof(def_tuple->dbd_def_value)-1,
		"next value for \"", own, "\".\"", name, "\"",
		&def_tuple->dbd_def_value[0]);
	def_tuple->dbd_def_length = STlength(def_tuple->dbd_def_value);

	attr->attr_defaultTuple = def_tuple;
	
	return(E_DB_OK);
    }

    if (def_node->pst_sym.pst_type != PST_CONST)
    {
	/* only other node we should be able to get here is a PST_UOP node,
	** and then only if the default is -/+ string (like -'99.99').
	** We won't allow that, because it is much easier for the user
	** to specify '-99.99' than it is to try to handle the UOP here.
	** (note that defaults like +5 or -99.99 will get automatically
	** converted into a PST_CONST node by the grammar (in pst_node).)
	*/
	psf_error(2076L, 0L, PSF_USERERR,
		  &err_code, err_blk, 1,
		  def_txt->db_t_count, def_txt->db_t_text);
	return(E_DB_ERROR);
    }
    
    dataval = &def_node->pst_sym.pst_dataval;

    /*
    ** next check if have a special default
    ** like NULL or 0 or blank
    */

    /* check for NULL
     */
    if (   (dataval->db_datatype == -DB_LTXT_TYPE)
	&& (dataval->db_length   == DB_CNTSIZE + 1))
    {
	SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_NULL);
	return(E_DB_OK);
    }	    


    /* Check for 0 or blank.
    ** The scanner/parser only returns constants of 4 different types
    ** (integer, decimal, float, or varchar), so only need to check for
    ** those 4 types below
    */
    switch (dataval->db_datatype)
    {
    case DB_BOO_TYPE:
        if (((DB_ANYTYPE *)dataval->db_data)->db_booltype == DB_FALSE)
        {
            SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_FALSE);
            return E_DB_OK;
        }
        if (((DB_ANYTYPE *)dataval->db_data)->db_booltype == DB_TRUE)
        {
            SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_TRUE);
            return E_DB_OK;
        }
        break;

    case DB_FLT_TYPE:
	if ( *((f8 *)dataval->db_data) == (f8) 0)
	{
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_0);
	    return(E_DB_OK);
	}
	break;
	    
    case DB_INT_TYPE:
	/* only check for i2; scanner doesn't return i1's and only returns
	** i4's if the value doesn't fit in an i2 (and then it can't be 0)
	*/
	if ((dataval->db_length == 2) 
	    && (*((i2 *)dataval->db_data) == (i2) 0))
	{
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_0);
	    return(E_DB_OK);
	}
	break;
	    
    case DB_DEC_TYPE:
	status = CVpkf(dataval->db_data, 
		       (i4)DB_P_DECODE_MACRO(dataval->db_prec),
		       (i4)DB_S_DECODE_MACRO(dataval->db_prec),
		       &fval);
	if (DB_FAILURE_MACRO(status))
	{
	    (void) psf_error(E_PS04A4_INT_ERROR, 0L, PSF_INTERR,
			     &err_code, err_blk, 2,
			     sizeof(ERx("CREATE TABLE"))-1,
			     ERx("CREATE TABLE"),
			     psf_trmwhite(DB_ATT_MAXNAME,
					  attr->attr_name.db_att_name),
			     attr->attr_name.db_att_name);
	    return(status);
	}

	if (fval == (f8) 0)
	{
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_0);
	    return(E_DB_OK);
	}
	break;
	    
    case DB_VCH_TYPE:
	txt = (DB_TEXT_STRING *) dataval->db_data;
	    
	/* don't need to preserve number of blanks for fixed char
	** datatypes, but do need to preserve it for varchar datatypes
	*/
	if ((txt->db_t_count == 1) && (*txt->db_t_text == ' '))
	{
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_BLANK);
	    return(E_DB_OK);
	}
	else if ((   (abs(attr->attr_type) == DB_CHA_TYPE) 
		  || (abs(attr->attr_type) == DB_CHR_TYPE))
		 && (psf_trmwhite(txt->db_t_count, 
				  (char *) txt->db_t_text) == 0))
	{
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_ID_BLANK);
	    return(E_DB_OK);
	}
	break;
	
    default:
	break;
	    
    }  /* switch (db_datatype) */

    /*
    ** if we've made it this far,
    ** we have a user-defined default that is
    ** a non-zero, non-blank, non-null constant,
    ** so
    **   make sure the default is not longer than the max allowed,
    **   call adc_defchk to make sure the value fits data type, size, & prec,
    **   then allocate an IIDEFAULT tuple & store the value in attr descriptor
    */


    /*
    ** first make sure default is not too long
    */
    /* trim any leading white space
    ** (since we are using the TXTEMIT2 flag to get the text value from the
    **  scanner, it may have leading white space before the value.)
    */
    def_length = def_txt->db_t_count;
    def_string = def_txt->db_t_text;
    end_string  = def_string + def_length;
	
    for (; (def_string < end_string) && CMwhite(def_string) ;)
    {
	CMbytedec(def_length, def_string);
	CMnext(def_string);
    }
	
    if (def_length > DB_MAX_COLUMN_DEFAULT_LENGTH)
    {
	i4 size = DB_MAX_COLUMN_DEFAULT_LENGTH;

	(void) psf_error(E_PS04A0_DEFAULT_TOO_LONG, 0L, PSF_USERERR,
			 &err_code, err_blk, 2,
			 psf_trmwhite(sizeof(DB_ATT_NAME), 
				      attr->attr_name.db_att_name),
			 attr->attr_name.db_att_name,
			 sizeof(size), &size);
	return(E_DB_ERROR);
    }

    adf_scb = (ADF_CB*) sess_cb->pss_adfcb;

    resultval.db_datatype = attr->attr_type;
    resultval.db_length   = attr->attr_size;
    resultval.db_prec     = attr->attr_precision;

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			resultval.db_length, &resultval.db_data, err_blk);
    if (DB_FAILURE_MACRO(status))
	return(status);
    
    /* in order to allow ADF to ignore math exceptions when desired, we
    ** set a bit to tell the PSF exception handler to call the ADF
    ** exception handler whenever we do math on behalf of the user.
    */
    sess_cb->pss_stmt_flags |= PSS_CALL_ADF_EXCEPTION;

    /* allocate an IIDEFAULT tuple to store in the attr descriptor */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
			sizeof(DB_IIDEFAULT), (PTR *) &def_tuple, err_blk);
    if (DB_FAILURE_MACRO(status))
	return(status);
    if (dataval->db_datatype == DB_FLT_TYPE)
    {
      /* use def_tuple as raw memory to store LTXT version of data type */

      MEcopy((char *) &def_length, sizeof(u_i2), def_tuple);
      MEcopy((char *) def_string, def_length, 
			(PTR) ((char *)def_tuple + sizeof(u_i2)));

      /* Data for the default will be stored in the catalog as a varchar, not
      ** as whatever type the scanner has found.  So check whether the string
      ** passed in by the user can be converted to the appropriate result type.
      ** One example where this is necessary is if the default is for a money
      ** datatype and the user has provided "13.2E3", the scanner will read
      ** this as a float and adf can convert from float to money - but when
      ** the default is used the code will try to convert from the string to
      ** money and fail since the string is not a legal money format.
      */
      dataval->db_data = (PTR) def_tuple;
      dataval->db_length = def_length;
      dataval->db_datatype = DB_LTXT_TYPE;
      dataval->db_prec = 0;
    }
    status = adc_defchk(adf_scb, dataval, &resultval);

    /* reset the flag,
    ** as we aren't doing math on behalf of the user anymore
    */
    sess_cb->pss_stmt_flags &= ~PSS_CALL_ADF_EXCEPTION;

    if (DB_FAILURE_MACRO(status))
    {
	/* return appropriate error, depending on the type of error,
	** e.g.    	datatype mismatch
	** 		value too large/small for column
	** 		value loses precision
	*/
	switch (adf_scb->adf_errcb.ad_errcode)
	{
	case E_AD2009_NOCOERCION:
	    (void) psf_error(E_PS0499_BAD_DEFAULT_VALUE, 0L, PSF_USERERR, 
			     &err_code, err_blk, 1, 
			     psf_trmwhite(sizeof(DB_ATT_NAME), 
					  attr->attr_name.db_att_name),
			     attr->attr_name.db_att_name);
	    break;

	case E_AD1090_BAD_DTDEF:
	    (void) psf_error(E_PS049A_BAD_DEFAULT_SIZE, 0L, PSF_USERERR, 
			     &err_code, err_blk, 1, 
			     psf_trmwhite(sizeof(DB_ATT_NAME), 
					  attr->attr_name.db_att_name),
			     attr->attr_name.db_att_name);
	    break;

	case E_AD1020_BAD_ERROR_LOOKUP:
	    (void) psf_error(E_PS0103_ERROR_NOT_FOUND, 0L, PSF_INTERR, 
			     &err_code, err_blk, 0);
	    break;

	default:
	    if (adf_scb->adf_errcb.ad_errclass == ADF_INTERNAL_ERROR)
	    {
		(void) psf_error(E_PS0002_INTERNAL_ERROR, 
				 adf_scb->adf_errcb.ad_errcode, PSF_INTERR, 
				 &err_code, err_blk, 0);
	    }
	    else
	    {
		/* user specified bad default value
		** or error occurred in user's UDT routine;
		** report general error, then more specific ADF error
		*/
		(void) psf_error(E_PS0499_BAD_DEFAULT_VALUE, 0L, PSF_USERERR, 
				 &err_code, err_blk, 1, 
				 psf_trmwhite(sizeof(DB_ATT_NAME), 
					      attr->attr_name.db_att_name),
				 attr->attr_name.db_att_name);

		psf_adf_error(&adf_scb->adf_errcb, err_blk, sess_cb);
	    }
	    break;
	}
	return(status);
    }
	
    MEfill(sizeof(DB_IIDEFAULT), NULLCHAR, (PTR) def_tuple);

    /* copy text into tuple,
    ** then attach tuple to attribute
    */
    MEcopy((char *) def_string, def_length, def_tuple->dbd_def_value);
    def_tuple->dbd_def_length = def_length;

    attr->attr_defaultTuple = def_tuple;
	
    return(E_DB_OK);
    
}  /* end psl_col_user_default */


/*
** Name: psl_1col_default  - set up the default ID/tuple for a column
**
** Description:	    
**      Handles NOT DEFAULT, WITH DEFAULT, DEFAULT <value>, and
**      <no default specified> column definitions.
**      
**      For canonical defaults, sets the default tuple to NULL and sets
**      the default ID to the appropriate canonical ID.  For user-defined
**      defaults, sets default ID to 0 and copies the default text into the
**      default tuple and attaches it to the column descriptor.
**      
**      Sets other bits as necessary (e.g. NDEFAULT).
**
**      Canonical defaults are NOT DEFAULT, no default specified, the
**      INGRES defaults (0 and blank), and the constant functions like
**      USER or NULL (and future ones will be CURRENT_TIME, CURRENT_DATE).
**      
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_ostream	    memory stream to put text from chain into
**	col_type	    indicator of whether the attribute has a default
**	    PSS_TYPE_NDEFAULT	    not defaultable
**	    PSS_TYPE_DEFAULT	    any default--INGRES or user
**	    PSS_TYPE_USER_DEFAULT   user-defined default
**	attr		    attribute descriptor for this column
**			    	filled in with default info
**	def_txt		    text of default value, as user typed it in
**	def_node	    CONST node or function node representing default
**				used to check type/size validity of default
** Outputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_tchain	    reset to NULL 
**	attr		    attribute descriptor for this column
**	    attr_flags_mask	NDEFAULT bit set for NOT DEFAULT columns
**	    attr_defaultTuple	set to default string for user-defined defaults
**	    			set to NULL for no default or canonical default
**	    attr_defaultID	set to canon ID for canonical defaults
**	err_blk		    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**
**	20-nov-92 (rblumer)
**	    written
**	13-sep-93 (rblumer)
**	    we were overwriting attr_flags_mask; use |= to set bit instead.
**	20-jan-94 (rblumer)
**	    B58651: defaults on long datatypes (i.e. blob datatypes) cannot be
**	    fully supported at this time, so we must explicitly disallow them.
**	20-dec-2006 (dougi)
**	    Change above to only disallow defaults on UDTs. LOBs are fine.
*/
DB_STATUS
psl_1col_default(
		PSS_SESBLK	*sess_cb,
		i4		col_type,
		DMF_ATTR_ENTRY	*attr,
		DB_TEXT_STRING	*def_txt,
		PST_QNODE	*def_node,
		DB_ERROR	*err_blk)
{
    DB_STATUS	   status;
    ADF_CB	   *adf_scb;
    ADI_DT_NAME	   type_name;
    i4		   dtinfo;
    i4 	   err_code;


    /* initialize to "no default specified"
     */
    attr->attr_defaultTuple = (DB_IIDEFAULT *) NULL;
    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_NOT_SPECIFIED);

    if (col_type & PSS_TYPE_NDEFAULT)
    {
	/* set NOT DEFAULT flag and default id 
	 */
	attr->attr_flags_mask |= DMU_F_NDEFAULT;
	SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_NOT_DEFAULT);
    }
    else if (col_type & PSS_TYPE_DEFAULT)
    {
	/* we don't allow defaults on UDTs,
	** so check to make sure this is not one
	*/
	adf_scb = (ADF_CB*) sess_cb->pss_adfcb;
	status  = adi_dtinfo(adf_scb, (DB_DT_ID) attr->attr_type, &dtinfo);
	if (DB_FAILURE_MACRO(status))
	{
	    (void) psf_error(E_PS0002_INTERNAL_ERROR, 
			     adf_scb->adf_errcb.ad_errcode, PSF_INTERR, 
			     &err_code, err_blk, 0);
	    return(status);
	}
	
	if (dtinfo & AD_USER_DEFINED)
	{
	    (void) adi_tyname(adf_scb, (DB_DT_ID) attr->attr_type, &type_name);

	    (void) psf_error(E_PS049F_NO_PERIPH_DEFAULT, 0L, PSF_USERERR,
			     &err_code, err_blk, 2,
			     psf_trmwhite(sizeof(DB_ATT_NAME), 
					  attr->attr_name.db_att_name),
			     attr->attr_name.db_att_name,
			     psf_trmwhite(sizeof(ADI_DT_NAME), 
					  type_name.adi_dtname), 
			     type_name.adi_dtname);
	    return(E_DB_ERROR);
	}	    

	if (col_type & PSS_TYPE_USER_DEFAULT)
	{
	    /* user-defined default 
	     */
	    status = psl_col_user_default(sess_cb, attr, 
					  def_txt, def_node, err_blk);
	}
	else
	{
	    /* have an INGRES default
	     */
	    status = psl_2col_ingres_default(sess_cb, attr, err_blk);
	}
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }

    /* else no default was specified 
     */

    return(E_DB_OK);

}  /* end psl_1col_default */



/*
** Name: psl_make_def_const  - create const node, given a default string
**
** Description:	    
**      Takes the input string value and builds a PST_CONST node constaining a
**      longtext value (DB_LTXT_TYPE).
**      
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_lang		query language 
**	mstream		    memory stream to allocate node from
**	length		    length of passed-in text value
**	defvalue	    default value, in text format (not null-terminated)
**
** Outputs:
**	newnode		    filled in with CONST node representing default value
**	err_blk		    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**
**	22-jan-93 (rblumer)
**	    written
*/
static DB_STATUS
psl_make_def_const(
		   PSS_SESBLK	*sess_cb, 
		   PSF_MSTREAM	*mstream, 
		   u_i2		length,
		   char	 	*defvalue, 
		   PST_QNODE	**newnode,
		   DB_ERROR	*err_blk)
{
    PST_CNST_NODE   cconst;
    DB_STATUS	    status;
    DB_TEXT_STRING  *txt;
    u_i2	    varlength;

    /* copy the defvalue string into a varchar field
     */
    varlength = DB_CNTSIZE + length;
    
    status = psf_malloc(sess_cb, mstream, (i4) varlength, (PTR *) &txt, err_blk);
    if DB_FAILURE_MACRO(status)
	return(status);

    txt->db_t_count = length;
    MEcopy(defvalue, length, (char *) txt->db_t_text);

    /* Create a constant node holding the default value
    ** as a long-text type
    */
    cconst.pst_tparmtype = PST_USER;
    cconst.pst_parm_no   = 0;
    cconst.pst_pmspec    = PST_PMNOTUSED;
    cconst.pst_cqlang    = sess_cb->pss_lang;
    cconst.pst_origtxt   = (char *) NULL;

    status = pst_node(sess_cb, mstream, (PST_QNODE *) NULL, (PST_QNODE *) NULL,
		      PST_CONST, (char *) &cconst, sizeof(cconst), 
		      DB_LTXT_TYPE, (i2) 0, (i4) varlength, (DB_ANYTYPE *) txt,
		      newnode, err_blk, (i4) 0);

    return (status);

}  /* end psl_make_def_const */



/*
** Name: psl_make_def_seq  - create SEQOP node, given a default string
**
** Description:
**
**	This routine regenerates a SEQOP node from a sequence default.
**	The default text is "next value for <sequence>", and we'll parse
**	that into a SEQOP node.
**
**	Ideally we would simply run it thru the grammar fragment that
**	handles sequence-op's, but yacc parsers don't work that way. :(
**	So we'll parse the seqop by hand, it's not hard.  Note that the
**	owner.seqname will always both be present, and will always be
**	delimited, thanks to psl-col-user-default.
**      
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_lang		query language 
**	mstream		    memory stream to allocate node from
**	length		    length of passed-in text value
**	defvalue	    default value, in text format (not null-terminated)
**			    Caller checks that default is more than 16
**			    chars and starts with next value for "
**
** Outputs:
**	newnode		    filled in with SEQOP node representing default value
**	err_blk		    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**
**	13-Jun-2006 (kschendel)
**	    copied from make-def-const, for sequence defaults.
*/
static DB_STATUS
psl_make_def_seq(
		   PSS_SESBLK	*sess_cb, 
		   PSF_MSTREAM	*mstream, 
		   u_i2		length,
		   char	 	*defvalue, 
		   PST_QNODE	**newnode,
		   DB_ERROR	*err_blk)
{
    char defstr[2*DB_MAXNAME + 30];  /* Null-terminated copy of default */
    char *from, *to;
    DB_STATUS status;
    i4 err_code;
    i4 gseq_flags;		/* Returned sequence flags */
    i4 privs;			/* Sequence privileges */
    PSS_SEQINFO seqinfo;	/* Info about the sequence from RDF */
    PST_SEQOP_NODE seqop;	/* The built-op seqop stuff */

    /* dummy loop to break out of for error */
    status = E_DB_ERROR;
    do {
	/* it's easier to work with a null-terminated copy of the default
	** string.  "defstr" is large enough for any properly constructed
	** sequence default, and the caller guaranteed that it's over 16
	** chars long (next value for ").
	*/
	if (length > sizeof(defstr)-1)
	    break;
	STlcopy(defvalue, &defstr[0], length);

	/* Create a sequence-op node with the sequence owner and name
	** from the default string, and the NEXT operator (implied).
	** The default string contains:
	** next value for "owner"."seqname"
	** exactly like that, as constructed when the default was created.
	*/
	from = &defstr[16];		/* Skip next value for " */
	to = &seqop.pst_owner.db_own_name[0];
	while (*from != '"' && *from != '\0')
	{
	    CMcpyinc(from, to);
	}
	while (to <= &seqop.pst_owner.db_own_name[sizeof(DB_OWN_NAME)-1])
	    *to++ = ' ';			/* Blank pad */
	if (*from++ == '\0') break;		/* Move past "." chars */
	if (*from++ == '\0') break;		/* to get to the seqname */
	if (*from++ == '\0') break;
	to = &seqop.pst_seqname.db_name[0];
	while (*from != '"' && *from != '\0')
	{
	    CMcpyinc(from, to);
	}
	while (to <= &seqop.pst_seqname.db_name[sizeof(DB_NAME)-1])
	    *to++ = ' ';			/* Blank pad */
	if (*from == '\0') break;
	status = E_DB_OK;
    } while (0);
    /* If bad status here, we had a malformed sequence default */
    if (status != E_DB_OK)
    {
	psf_error(E_PS04B3_BAD_SEQOP_DEFAULT, 0, PSF_USERERR,
		&err_code, err_blk, 1,
		length, defvalue);
	return (E_DB_ERROR);
    }


    /* Find the sequence to make sure it's there and we have perms */
    /* Note that we don't know the qmode, it really ought to be stashed
    ** in the sess_cb somewhere!  use APPEND since that's probably
    ** right enough;  it's only for error messages.
    */
    privs = DB_NEXT;
    status = psy_gsequence(sess_cb, &seqop.pst_owner, &seqop.pst_seqname,
		PSS_SEQ_BY_OWNER, &seqinfo, NULL,
		&gseq_flags, &privs, PSQ_APPEND, FALSE, err_blk);
    if (status != E_DB_OK || gseq_flags & PSS_MISSING_SEQUENCE
      || gseq_flags & PSS_INSUF_SEQ_PRIVS)
	return (E_DB_ERROR);

    STRUCT_ASSIGN_MACRO(seqinfo.pss_seqid, seqop.pst_seqid);
    seqop.pst_seqflag = PST_SEQOP_NEXT;
    status = pst_node(sess_cb, mstream, (PST_QNODE *) NULL, (PST_QNODE *) NULL,
			PST_SEQOP, (char *) &seqop, sizeof(seqop), 
			seqinfo.pss_dt,
			DB_PS_ENCODE_MACRO(seqinfo.pss_prec, 0),
			seqinfo.pss_length, NULL,
			newnode, err_blk, 0);

    return (status);

}  /* psl_make_def_seq */

/*
** Name: psl_make_func_node     - create const-operator node for an ADF func
**
** Description:	    
**      Builds a PST_OP_NODE for the ADF operator passed in.
**      
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	mstream		    memory stream to allocate node from
**	opid		    id of ADF operator
**
** Outputs:
**	newnode		    filled in with CONST node representing default value
**	err_blk		    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**
**	22-jan-93 (rblumer)
**	    written
**	25-Jun-2008 (kiria01) SIR120473
**	    Move pst_isescape into new u_i2 pst_pat_flags.
**	3-Aug-2010 (kschendel) b124170
**	    Probably don't need joinid here, but let pst-node decide.
*/
static DB_STATUS
psl_make_func_node(
		   PSS_SESBLK	*sess_cb, 
		   PSF_MSTREAM	*mstream, 
		   ADI_OP_ID	opid,
		   PST_QNODE	**newnode,
		   DB_ERROR	*err_blk)
{
    PST_OP_NODE	opnode;
    DB_STATUS	status;

    opnode.pst_opno     = opid;
    opnode.pst_opmeta   = PST_NOMETA;
    opnode.pst_pat_flags = AD_PAT_DOESNT_APPLY;

    status = pst_node(sess_cb, mstream, (PST_QNODE *) NULL, (PST_QNODE *) NULL, 
		      PST_COP, (char *) &opnode, sizeof(opnode),
		      DB_NODT, (i2) 0, (i4) 0, (DB_ANYTYPE *) NULL, 
		      newnode, err_blk, PSS_JOINID_STD);
    return(status);

}  /* end psl_make_func_node */



/*
** Name: psl_make_canon_default  - creates a CONST node for a canonical default
**
** Description:	    
**      Handles uses of columns with defaults declared as NOT DEFAULT, WITH
**      DEFAULT, DEFAULT <user-func>, DEFAULT NULL, and <no default specified>.
**      
**      Since the canonical (pre-defined) default values are known,
**      this function builds the string representing the value and create a
**      PST_CONST node holding that string.
**      For the user-functions, a constant-operator node (PST_OP_NODE) is built
**      instead of a PST_CONST node.
**
**      Canonical defaults are NOT DEFAULT, no default specified, the
**      INGRES defaults (0 and blank), and the constant functions like
**      USER or NULL (and future ones will be CURRENT_TIME, CURRENT_DATE).
**      
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	mstream		    memory stream to allocate node from
**	defaultID	    id of canonical default that is needed
**	
** Outputs:
**	newnode		    filled in with CONST node representing default value
**	err_blk		    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**
**	22-jan-93 (rblumer)
**	    written
**	26-mar-93 (rblumer)
**	    change blank defid to pass 0-length string instead of 1-space
**	    string; otherwise get a blank in varchar instead of 0-length string
**	20-dec-2006 (dougi)
**	    Add support for date/time registers.
*/
static DB_STATUS
psl_make_canon_default(
		       PSS_SESBLK   *sess_cb, 
		       PSF_MSTREAM  *mstream, 
		       DB_DEF_ID    *defaultID, 
		       PST_QNODE    **newnode, 
		       DB_ERROR	    *err_blk)
{
    i4 	  err_code;
    DB_STATUS	  status;
    PST_CNST_NODE cconst;
    DB_DATA_VALUE db_data;
    char bool_buf[6];
    char	  buf[DB_LEN_OBJ_LOGKEY];  /* the length of this buffer must
					   ** the max of  NULL_LTXT_LEN,
					   **             DB_LEN_TAB_LOGKEY,
					   **         and DB_LEN_OBJ_LOGKEY
					   */
    
    switch (defaultID->db_tab_base)
    {
    case DB_DEF_ID_0:
	/* build a const node holding a '0' (zero) value
	 */
	status = psl_make_def_const(sess_cb, mstream, 
				    sizeof(ERx("0")) -1, ERx("0"), 
				    newnode, err_blk);
	break;

    case DB_DEF_ID_BLANK:
	/* build a const node holding a null string,
	** as this is what adc_getempty returns for char datatypes.
	** (Note that if you pass it a string with a single blank,
	**  you get a 1-byte value entered for varchar instead of
	**  the 0-length value we desired)
	*/
	status = psl_make_def_const(sess_cb, mstream, 
				    sizeof(ERx("")) -1, ERx(""), 
				    newnode, err_blk);
	break;

    case DB_DEF_ID_FALSE:
    case DB_DEF_ID_TRUE:
        /* build a const node holding the FALSE or TRUE literals */
        STcopy(defaultID->db_tab_base == DB_DEF_ID_FALSE ? "FALSE" : "TRUE",
               bool_buf);
        status = psl_make_def_const(sess_cb, mstream, STlength(bool_buf),
                                    bool_buf, newnode, err_blk);
        break;

    case DB_DEF_NOT_SPECIFIED:
    case DB_DEF_ID_NULL:
	/* build a const node holding the NULL value
	 */
#define NULL_LTXT_LEN	(DB_CNTSIZE+1)

	db_data.db_datatype = -DB_LTXT_TYPE;
	db_data.db_prec     = 0;
	db_data.db_length   = NULL_LTXT_LEN;
	db_data.db_data	    = (PTR) buf;

	status = adc_getempty ((ADF_CB *) sess_cb->pss_adfcb, &db_data);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	cconst.pst_tparmtype = PST_USER;
	cconst.pst_parm_no = 0;
	cconst.pst_pmspec  = PST_PMNOTUSED;
	cconst.pst_cqlang = DB_SQL;
	cconst.pst_origtxt = (char *) NULL;
	status = pst_node(sess_cb, mstream, 
			  (PST_QNODE *) NULL, (PST_QNODE *) NULL, 
			  PST_CONST, (char *) &cconst, sizeof(cconst),
			  -DB_LTXT_TYPE, (i2) 0, (i4) NULL_LTXT_LEN,
			  (DB_ANYTYPE *) db_data.db_data, 
			  newnode, err_blk, (i4) 0);
	break;

    case DB_DEF_ID_TABKEY:
	/* build a const node holding a string of null bytes
	 */
	MEfill(DB_LEN_TAB_LOGKEY, NULLCHAR, buf);
	
	status = psl_make_def_const(sess_cb, mstream, 
				    DB_LEN_TAB_LOGKEY, buf,
				    newnode, err_blk);
	break;

    case DB_DEF_ID_LOGKEY:
	/* build a const node holding a string of null bytes
	 */
	MEfill(DB_LEN_OBJ_LOGKEY, NULLCHAR, buf);
	
	status = psl_make_def_const(sess_cb, mstream, 
				    DB_LEN_OBJ_LOGKEY, buf,
				    newnode, err_blk);
	break;

    case DB_DEF_ID_USERNAME:
	/* build a const-operator node holding the user function
	 */
	status = psl_make_func_node(sess_cb, mstream, 
				    ADI_USRNM_OP, newnode, err_blk);
	break;

    case DB_DEF_ID_SESSION_USER:
	/* build a const-operator node holding the user function
	 */
	status = psl_make_func_node(sess_cb, mstream, 
				    ADI_SESSUSER_OP, newnode, err_blk);
	break;

    case DB_DEF_ID_SYSTEM_USER:
	/* build a const-operator node holding the user function
	 */
	status = psl_make_func_node(sess_cb, mstream, 
				    ADI_SYSUSER_OP, newnode, err_blk);
	break;

    case DB_DEF_ID_INITIAL_USER:
	/* build a const-operator node holding the user function
	 */
	status = psl_make_func_node(sess_cb, mstream, 
				    ADI_INITUSER_OP, newnode, err_blk);
	break;

    case DB_DEF_ID_DBA:
	/* build a const-operator node holding the user function
	 */
	status = psl_make_func_node(sess_cb, mstream, 
				    ADI_DBA_OP, newnode, err_blk);
	break;

    case DB_DEF_ID_CURRENT_DATE:
	/* build a const-operator node holding the user function
	 */
	status = psl_make_func_node(sess_cb, mstream, 
				    ADI_CURDATE_OP, newnode, err_blk);
	break;

    case DB_DEF_ID_CURRENT_TIMESTAMP:
	/* build a const-operator node holding the user function
	 */
	status = psl_make_func_node(sess_cb, mstream, 
				    ADI_CURTMSTMP_OP, newnode, err_blk);
	break;

    case DB_DEF_ID_CURRENT_TIME:
	/* build a const-operator node holding the user function
	 */
	status = psl_make_func_node(sess_cb, mstream, 
				    ADI_CURTIME_OP, newnode, err_blk);
	break;

    case DB_DEF_ID_LOCAL_TIMESTAMP:
	/* build a const-operator node holding the user function
	 */
	status = psl_make_func_node(sess_cb, mstream, 
				    ADI_LCLTMSTMP_OP, newnode, err_blk);
	break;

    case DB_DEF_ID_LOCAL_TIME:
	/* build a const-operator node holding the user function
	 */
	status = psl_make_func_node(sess_cb, mstream, 
				    ADI_LCLTIME_OP, newnode, err_blk);
	break;


	/* the following cases should never happen
	 */
    case DB_DEF_NOT_DEFAULT:
    case DB_DEF_UNKNOWN:
    default:
	(void) psf_error(E_PS0002_INTERNAL_ERROR, 0L, PSF_INTERR, 
			 &err_code, err_blk, 0);
	return (E_DB_ERROR);
    }
    
    return(status);

}  /* end psl_make_canon_default */



/*
** Name: psl_make_user_default  - creates tree node for a user-defined default
**
** Description:	    
**      Handles uses of columns with defaults declared as DEFAULT <value>.
**      
**      If default value is not already cached, calls RDF to fetch default value
**      from the IIDEFAULT catalog.  Once it has the default value, it strips
**      off surrounding and embedded single-quotes and calls either
**      psl_make_def_const for literal constants, or psl_make_def_seq
**	for sequence defaults. 
**      
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	mstream		    memory stream to allocate node from
**	rngvar		    range table entry of the table that 
**				the column belongs to;
**				ASSUMED TO already contain table and attribute
**				info from RDF.
**	attno		    number of column default is needed for
**	
** Outputs:
**	newnode		    filled in with tree node representing default value
**	err_blk		    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**
**	22-jan-93 (rblumer)
**	    written
**	25-mar-93 (rblumer)
**	    replace temporary code for removing surrounding quotes with 
**	    call to cui_idxlate to remove surrounding AND embedded quotes.
**	10-aug-93 (andre)
**	    removed cause of a compiler warning
**	20-jan-94 (rblumer)
**	    B58995: add code to handle null-string case of just 2 single quotes
**	17-aug-2001 (fruco01)
**	    Added logic to re-interpret the decimal mark before creating a
**	    default node.  The default float is stored with the set mark at
**	    the time of creation which may differ from the one when retrieved.
**          Bug #102757
*/
static DB_STATUS
psl_make_user_default(
		      PSS_SESBLK    *sess_cb, 
		      PSF_MSTREAM   *mstream, 
		      PSS_RNGTAB    *rngvar, 
		      i2	    attno,
		      PST_QNODE	    **newnode, 
		      DB_ERROR 	    *err_blk)
{
    DB_STATUS	  status;
    DMT_ATT_ENTRY *att;
    RDD_DEFAULT   *def_desc;
    RDF_CB	  rdf_cb;
    i4	  err_code;
    char	  *ch;
    
    att = rngvar->pss_attdesc[attno];

    /* check if default tuple has already been fetched from RDF
     */
    if (att->att_default == NULL)
    {
	/* call RDF to get default tuples from IIDEFAULT catalog.
	**
	** First set up constant part of rdf control block
	*/
	pst_rdfcb_init(&rdf_cb, sess_cb);

	rdf_cb.rdf_rb.rdr_types_mask  = RDR_ATTRIBUTES;
	rdf_cb.rdf_rb.rdr_2types_mask = RDR2_DEFAULT;
	rdf_cb.rdf_info_blk = rngvar->pss_rdrinfo;

	status = rdf_call(RDF_GETDESC, (PTR) &rdf_cb);
	if (DB_FAILURE_MACRO(status))
	{
	    switch (rdf_cb.rdf_error.err_code)
	    {
	    case E_RD001D_NO_SUCH_DEFAULT:
		(void) psf_error(E_PS049C_NO_DEFAULT_FROM_RDF, 0L, PSF_INTERR,
				 &err_code, err_blk, 1, 
				 psf_trmwhite(sizeof(DB_TAB_NAME), 
					      rngvar->pss_tabname.db_tab_name),
				 rngvar->pss_tabname.db_tab_name);
		break;
	    case E_RD001C_CANT_READ_COL_DEF:
		(void) psf_error(E_PS049D_RDF_CANT_READ_DEF, 0L, PSF_INTERR,
				 &err_code, err_blk, 1, 
				 psf_trmwhite(sizeof(DB_TAB_NAME), 
					      rngvar->pss_tabname.db_tab_name),
				 rngvar->pss_tabname.db_tab_name);
	    default:
		(void) psf_rdf_error(RDF_GETDESC, &rdf_cb.rdf_error, err_blk);
	    }
	    return(status);
	}

	/* RDF call succeeded, but could have reset the info block.
	** If it has changed, we have to reassign pointers to it.
	*/
	if (rngvar->pss_rdrinfo != rdf_cb.rdf_info_blk)
	{
	    rngvar->pss_rdrinfo = rdf_cb.rdf_info_blk;
	    rngvar->pss_tabdesc = rdf_cb.rdf_info_blk->rdr_rel;
	    rngvar->pss_attdesc = rdf_cb.rdf_info_blk->rdr_attr;
	    rngvar->pss_colhsh  = rdf_cb.rdf_info_blk->rdr_atthsh;

	    att = rngvar->pss_attdesc[attno];
	}
	

    }  /* end if tuple == NULL */

    def_desc = (RDD_DEFAULT *) att->att_default;

    /* Need to ensure that the default decimal
    ** mark is translated to whichever one is
    ** current for this session
    */

	if (DB_FLT_TYPE == abs(att->att_type) || 
	    DB_MNY_TYPE == abs(att->att_type) || 
	    DB_DEC_TYPE == abs(att->att_type) )
	{
	  char * defptr = def_desc->rdf_default;
	  while ( *defptr)
	  {
		if ( *defptr == '.' || *defptr == ',') 
	 	{	
			*defptr = sess_cb->pss_decimal;
			break;
		}

		CMnext(defptr);

	  }
	}

    ch =  def_desc->rdf_default;

    /* Check for: next value for ".  The default-maker generates this
    ** string for sequence defaults so that we don't have to worry about
    ** variants (case differences, oracle-style, etc).  There's no
    ** confusion with text-string defaults, as they are always quoted.
    */
    if (def_desc->rdf_deflen > 16
      && MEcmp((PTR) ch, (PTR) "next value for \"", 16) == 0)
    {
	return (psl_make_def_seq(sess_cb, mstream,
			def_desc->rdf_deflen, ch,
			newnode, err_blk));
    }

    /* make a constant node with the default value obtained from RDF.
    ** 
    ** Since character defaults are stored in the catalog with surrounding
    ** and embedded single quotes (due to semantics of ANSI catalogs),
    ** we need to strip out the single quotes before making the node.
    **
    ** NOTE: it is not possible for a default value from the iidefault catalog
    **       to start with a single quote unless it is a character default.
    */
    if (CMcmpcase(ch, ERx("'")) != 0)
    {
	status = psl_make_def_const(sess_cb, mstream, 
				    def_desc->rdf_deflen,
				    def_desc->rdf_default,
				    newnode, err_blk);
    }
    else if (   (def_desc->rdf_deflen == 2)
	     && (CMnext(ch), CMcmpcase(ch, ERx("'")) == 0))
    {
	/* have a null string (just two single quotes);
	** cui_idxlate complains about this, so have a special case here
	*/
	status = psl_make_def_const(sess_cb, mstream, 
				    sizeof(ERx("")) -1, ERx(""), 
				    newnode, err_blk);
    }
    else
    {
	u_i4	  len, norm_len;
	u_i4 mode;
	u_char	  normalized_value[DB_MAX_COLUMN_DEFAULT_LENGTH];

	len      = def_desc->rdf_deflen;
	norm_len = DB_MAX_COLUMN_DEFAULT_LENGTH;
	mode     = CUI_ID_QUO | CUI_ID_NOLIMIT;
    
	/* remove surrounding and embedded single-quotes
	 */
	status = cui_idxlate((u_char *) def_desc->rdf_default, &len,
			     normalized_value, &norm_len, 
			     mode, &mode, err_blk);
	if (DB_FAILURE_MACRO(status))
	{
	    (void) psf_error(err_blk->err_code, 0L, PSF_INTERR,
			     &err_code, err_blk, 2,
			     sizeof(ERx("Char default")),
			     ERx("Char default"),
			     def_desc->rdf_deflen, def_desc->rdf_default);
	    return(status);
	}

	status = psl_make_def_const(sess_cb, mstream, 
				    norm_len, (char *) normalized_value,
				    newnode, err_blk);
    }
    
    return(status);

}  /* end psl_make_user_default */


/*
** Name: psl_make_default_node  - creates a tree node for a default,
** 				  which can be used in INSERT statements or
** 				  WHERE clauses
**
** Description:	    
**      Handles uses of columns with defaults declared as NOT DEFAULT, WITH
**      DEFAULT, DEFAULT <value>, and <no default specified>.
**      
**      For canonical defaults, calls psl_make_canon_default; for all other
**      defaults, calls psl_make_user_default.  A tree node is created
**	to hold the default.  This is usually a PST_CONST node containing
**	the default value (in LTXT form);  but it could be a COP function
**	node (for things like CURRENT_USER), or a SEQOP node (for
**	sequence defaults).
**
**      Canonical defaults are NOT DEFAULT, no default specified, the
**      INGRES defaults (0 and blank), and the constant functions like
**      USER or NULL (and future ones will be CURRENT_TIME, CURRENT_DATE).
**      
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	mstream		    memory stream to allocate node from
**	rngvar		    range table entry of the table that 
**				the column belongs to;
**				ASSUMED TO already contain table and attribute
**				info from RDF.
**	attno		    number of column default is needed for
** Outputs:
**	newnode		    filled in with tree node representing default value
**	err_blk		    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**
**	22-jan-93 (rblumer)
**	    written
*/
DB_STATUS
psl_make_default_node(
		      PSS_SESBLK    *sess_cb,
		      PSF_MSTREAM   *mstream,
		      PSS_RNGTAB    *rngvar,
		      i2	    attno,
		      PST_QNODE	    **newnode,
		      DB_ERROR	    *err_blk)
{   
    DMT_ATT_ENTRY *att;
    DB_STATUS	  status;
    
    att = rngvar->pss_attdesc[attno];

    if (IS_CANON_DEF_ID(att->att_defaultID))
    {
	status = psl_make_canon_default(sess_cb, mstream, 
					&att->att_defaultID, newnode, err_blk);
    }
    else
    {
	status = psl_make_user_default(sess_cb, mstream, rngvar, 
				       attno, newnode, err_blk);
    }
    return(status);

}  /* end psl_make_default_node */





/*
** Name: psl_check_defaults    - find all unspecified columns in a qtree
** 				 (during an INSERT/APPEND) and add their
** 				 default values to the qtree 
** Description:
** 	Given a query tree for an insert/append statement,
** 	this function will find all columns for which the user has not
** 	specified a value.
** 	It will first make sure that all mandatory columns have been specified;
** 	after that it will build default tree nodes for any columns that
** 	haven't been specified, stick them into resdom nodes and add them to
** 	the tree. 
**      
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_ostream		memory stream used to allocate nodes from
**	    pss_lineno		line number of query (for error messages)
**	    pss_rsdmno		total number of resdoms in query tree
**	rngvar		    range table entry of the table for this query
**	root_node	    root node of query tree
**	insert_into_view    TRUE  if inserting into a view,
**			    FALSE if inserting into a table (for error messages)
**	view_name	    name of view (used in error message)
**	
** Outputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_rsdmno		incremented for each resdom added to tree
**	root_node	    updated with new resdoms for any columns
**				requiring default values
**	err_blk		    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**
**	22-jan-93 (rblumer)
**	    written
**	08-jul-93 (rblumer)
**	    change psl_check_defaults to correctly handle inserts in Star:
**	    distributed sessions don't have default-ID info,
**	    so only check default-IDs for local sessions.
**	    Also, don't add default nodes for distributed sessions;
**	    that will be done by the LDB server.
**	28-feb-94 (robf)
**          Initialize pst_dmuflags in resdom.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	    Set PST_RS_DEFAULTED in resdom to mark as not supplied by user.
**	    This is to enable INSERT DISTINCT to exclude these from the
**	    uniqueness checks.
**	14-Mar-2008 (kiria01) b119899
**	    Arrange for the insert from select resdoms to be sorted so that
**	    tuple buffers of the select(s) can match that of the table.
**	    This implies the default tuples need adjusting too.
**	10-Oct-2008 (kibro01) b121034
**	    Ensure we don't run off the end of the RESDOM list if the rsno is
**	    lower than any of the columns actually specified.
**	18-May-2010 (kiria01) b123442
**	    Force psl_mk_const_similar to generate coercion to cleanly 
**	    enable correct datatype to be represented when substituting
**	    default values.
**	11-Jun-2010 (kiria01) b123908
**	    Switch the order of b121034 check to ensure that the block is
**	    a resdom prior to checking its content.
*/
DB_STATUS
psl_check_defaults(
		   PSS_SESBLK	*sess_cb,
		   PSS_RNGTAB	*resrng,
		   PST_QNODE 	*root_node,
		   i4		insert_into_view,
		   DB_TAB_NAME	*view_name,
		   DB_ERROR	*err_blk)
{
    DB_COLUMN_BITMAP	attmap;
    i4			attcount;
    i4			attno;
    PST_QNODE		*node;
    PST_QNODE		**pnode;
    PST_QNODE		*defnode;
    PST_QNODE		*resdom_node;
    PST_RSDM_NODE	resdom;
    DMT_ATT_ENTRY	*att;
    DB_STATUS		status;
    i4		err_code;
    i4			mandatory;
    

    /* first make a bitmap of all attributes that have not been specified
    ** (by clearing the bits for atts that ARE in the target list).
    */
    MEfill(sizeof(DB_COLUMN_BITMAP), NULLCHAR, (PTR) &attmap);

    attcount = resrng->pss_rdrinfo->rdr_rel->tbl_attr_count;

    /* attribute numbers start at 1,
    ** so set an extra bit and clear bit 0.
    ** (Set exact number bits using BTnot because certain BT functions 
    **  assume any extra bits are 0.)
    */
    BTnot((i4)(attcount + 1), (char *) attmap.db_domset);
    BTclear((i4)0, (char *) &attmap.db_domset);    

    /* clear bits for atts that ARE in the target list
     */
    for (node = root_node->pst_left;
	 node != (PST_QNODE *) NULL && node->pst_sym.pst_type == PST_RESDOM;
	 node = node->pst_left)
    {
	if (node->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT)
	    BTclear((i4) node->pst_sym.pst_value.pst_s_rsdm.pst_rsno, 
		    (char *) &attmap.db_domset);
    }

    /* for each unspecified attribute, check if it's mandatory;
    ** if it's not, build a tree node holding its default value
    ** and add it to the query tree
    **
    ** This is being done here in PSF instead of in OPF because it is
    ** more efficient for dynamic SQL inserts, and no slower for other inserts.
    */
    for (attno = BTnext(0, (char *) &attmap.db_domset, attcount+1);
	 attno != -1 && attno <= attcount;
	 attno = BTnext(attno, (char *) &attmap.db_domset, attcount+1))
    {
	att = resrng->pss_attdesc[attno];

	/* if column is system-maintained, don't insert default value;
	** a unique value will be generated by DMF
	*/
	if (att->att_flags & DMU_F_SYS_MAINTAINED)
	    continue;

	/* if a column contains the row security label don't insert default
	** value, this is generated by DMF
	*/
	if (att->att_flags & DMT_F_SEC_LBL)
	    continue;

	/* decide if the column is mandatory.
	**
	** Note: we only have default-id info if this is a local session,
	**       so don't try to use that info for distributed sessions
	*/
	if (sess_cb->pss_distrib & DB_3_DDB_SESS)
	{
	    mandatory = (att->att_flags & DMU_F_NDEFAULT) ? TRUE : FALSE;
	}
	else
	{
	    /* if default for column is UNKNOWN,
	    ** then it's a calculated column in a view,
	    ** so don't check if it's mandatory or build a default node for it
	    */
	    if (EQUAL_CANON_DEF_ID(att->att_defaultID, DB_DEF_UNKNOWN))
		continue;

	    /* if the attribute is declared NOT DEFAULT
	    ** or NOT NULL with no default, it is a mandatory column,
	    */
	    if ((att->att_flags & DMU_F_NDEFAULT)
		|| ((att->att_type > 0) 
		    && (EQUAL_CANON_DEF_ID(att->att_defaultID, 
		                           DB_DEF_NOT_SPECIFIED))))
		mandatory = TRUE;
	    else
		mandatory = FALSE;
	}

	/* if this attribute is mandatory,
	** return an error.
	*/
	if (mandatory)
	{
	    if (insert_into_view)
	    {
		/* Base table mandatory column value not present */
		(void) psf_error(2781L, 0L, PSF_USERERR, &err_code,
				 err_blk, 3,
				 att->att_nmlen, att->att_nmstr,
				 psf_trmwhite(sizeof(DB_TAB_NAME), 
					      resrng->pss_tabname.db_tab_name),
				 &resrng->pss_tabname.db_tab_name,
				 psf_trmwhite(sizeof(DB_TAB_NAME), 
					      (char *) view_name->db_tab_name),
				 view_name->db_tab_name);
	    }
	    else
	    {
		/* Mandatory column value not present */
		(void) psf_error(2779L, 0L, PSF_USERERR, &err_code,
				 err_blk, 1, att->att_nmlen, att->att_nmstr);
	    }		
	    return (E_DB_ERROR);
	}

	/* for distributed queries, skip default node processing,
	** as that will be done by the local server
	*/
	if (sess_cb->pss_distrib & DB_3_DDB_SESS)
	    break;

	status = psl_make_default_node(sess_cb, &sess_cb->pss_ostream, 
				       resrng, attno, 
				       &defnode, err_blk);
	if DB_FAILURE_MACRO(status)
	    return(status);
	
	{
	    DB_DATA_VALUE dbv;
	    /* Try to cast to column type */
	    dbv.db_data = NULL;
	    dbv.db_length = att->att_width;
	    dbv.db_datatype = att->att_type;
	    dbv.db_prec = att->att_prec;
	    dbv.db_collID = att->att_collID;
	    status = psl_mk_const_similar(sess_cb, &sess_cb->pss_ostream,
			&dbv, &defnode, err_blk, NULL);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	}

	/*
	** make resdom node and put default node into it
	*/

	/* Fill out resdom with attribute info
	 */
	cui_move(att->att_nmlen, att->att_nmstr, ' ',
	       DB_ATT_MAXNAME, (char *) resdom.pst_rsname);
	resdom.pst_rsno      = att->att_number;
	resdom.pst_ntargno   = att->att_number;
	resdom.pst_ttargtype = PST_ATTNO;
	resdom.pst_rsupdt    = FALSE;
	resdom.pst_rsflags   = PST_RS_PRINT | PST_RS_DEFAULTED; /* Mark not user supplied */
	resdom.pst_dmuflags  = 0;

	/* Now allocate the resdom node 
	 */
	status = pst_node(sess_cb, &sess_cb->pss_ostream, (PST_QNODE *) NULL, defnode,
			  PST_RESDOM,  (char *) &resdom, sizeof(PST_RSDM_NODE),
			  (DB_DT_ID) att->att_type, (i2) att->att_prec, 
			  (i4) att->att_width, (DB_ANYTYPE *) NULL, 
			  &resdom_node, err_blk, (i4) 0);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/* attach resdom node to query tree at the correct place. */
	/* But not after the end of the RESDOM list (kibro01) b121034 */
	pnode = &root_node->pst_left;
	while ((node = *pnode) &&
		node->pst_sym.pst_type == PST_RESDOM &&
		node->pst_sym.pst_value.pst_s_rsdm.pst_rsno > resdom.pst_rsno)
	    pnode = &node->pst_left;
	resdom_node->pst_left = node;
	*pnode = resdom_node;

	/* for completeness, increment resdom counter
	 */
	sess_cb->pss_rsdmno++;
	if (sess_cb->pss_rsdmno > (DB_MAX_COLS + 1))
	{
	    /* should never happen */
	    psf_error(2130L, 0L, PSF_INTERR, &err_code,
		err_blk, 1, (i4) sizeof(sess_cb->pss_lineno),
		&sess_cb->pss_lineno);
	    return (E_DB_ERROR);
	}
	
    }  /* end for (attno  != -1) */

    return(E_DB_OK);

}  /* end psl_check_defaults */
