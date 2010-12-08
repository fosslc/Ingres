/*
** Copyright (c) 1986, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
/* [@#include@]... */

/**
**
**  Name: ADGINIT.C - Initialize ADF for a session.
**
**  Description:
**      This file contains all of the routines necessary to perform the 
**      external ADF call "adg_init()" which initializes the ADF for a
**      session within a server.
**
**	This file defines the following externally visible routines:
**
**          adg_init() - Initialize ADF for a session.
**
**	... and the following static routines:
**
**          adg_outargs() - Initialize/validate output formats.
**			    
**
**  History:
**      17-jan-1986 (thurston)    
**          Initial creation.
**	25-mar-86 (thurston)
**          Added some calls to BTset() in the adg_dtinit() routine
**	    for some coercions that were missing.
**	02-apr-86 (thurston)
**	    Added in the definitions of the "boolean", "text0", "textn", and
**	    "binary" datatypes to the datatypes table.  I have commented
**	    "boolean" as not intended to be assigned to columns, but rather
**	    a vehicle for several of ADF's routines to return TRUE or FALSE
**	    data values.  "text0" and "textn" have been commented to say that
**	    they are for use by the copy command only.  "binary" is commented
**	    to say that this is just a place holder for possible future use.
**	09-apr-86 (ericj)
**	    Added the datatype ptrs table.  This is an array of pointers into
**	    the datatypes table to allow direct lookups of datatypes.
**	15-apr-86 (ericj)
**	    Put functions addresses in the datatypes table for the
**	    common functions, adc_lenchk(), adc_getempty(), adc_compare().
**	03-jun-86 (ericj)
**	    Added routine adg_outarg_init() for initializing default output
**	    formats.
**	30-jun-86 (thurston)
**	    Removed the initialization of the operations table.  This is now in
**	    the ADGOPTAB.ROC file.
**	30-jun-86 (thurston)
**	    Upgraded the comments for the adg_init() routine to include
**          adf_scb.adf_outarg as an input.  Actually, at the moment it is an
**          output as this routine sets it to be the default values.  At some
**          point this will need to be changed to take these values as input, or
**          optionally set them to be the default values. 
**	30-jun-86 (thurston)
**	    Made the adg_dtinit() routine a static routine since it should not
**          be seen outside of this file. 
**	30-jun-86 (thurston)
**	    Added a FUNC_EXTERN for the adc__isnull() routine, and upgraded the
**	    datatypes table to use it.
**	03-jul-86 (ericj)
**	    Added BTset() calls to the adg_dtinit() routine to make coercions
**	    from a datatype to itself valid.  Added routine addresses for
**	    adc_1cvinto_rti() and adc_1keybld_rti() to datatypes table.
**	    Made temporary routines, adg_dfmt_init() and adg_mfmt_init(),
**	    to initialize the date and money formats.
**	07-jul-86 (ericj)
**	    Put in the key building routine addresses for "RTI known"
**	    datatypes.
**	23-jul-86 (thurston)
**	    Major overhaul.  Stripped out all of the datatypes table
**	    initialization and put into the ADGSTARTUP.C file.
**	    Also, the static routine adg_dtinit() has been moved into the
**	    ADGSTARTUP.C file.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	03-nov-86 (ericj)
**	    Added code to adg_init() to initialize the ADI_WARN struct for
**	    the session.
**	20-nov-86 (thurston)
**	    Added temp code to adg_init() assign QUEL as the query language.
**	08-jan-87 (thurston)
**	    Did major overhaul.  adg_init() now checks validity of all fields
**	    in the ADF_CB, returning errors if any are not valid.  Also, allows
**	    the caller to request that it set the default values for any members
**	    of the ADF_OUTARG structure contained therein.  Also, adg_init()
**	    set the decimal character to '.' if the decimal spec says there is
**	    no decimal character specified.  (This should be redundant since
**	    all of the ADF code that requires use of the decimal character has
**	    this same default built in.)
**	17-feb-87 (thurston)
**	    Added line to adg_init() to zero the new .ad_noprint_cnt field of
**	    the ADI_WARN struct in the ADF_CB.
**	24-feb-87 (thurston)
**	    Added GLOBALDEF for ADF's global server control block, Adf_globs,
**	    and added code to adg_init() to set Adf_globs from passed in field
**	    in ADF_CB.  Code also checks for NULL pointer.
**	06-mar-87 (thurston)
**	    Added code to adg_init() to verify (or initialize, if requested)
**	    the description of the NULLstring.
**	18-mar-87 (thurston)
**	    Added some WSCREADONLY to pacify the Whitesmith compiler on MVS.
**	13-aug-87 (thurston)
**	    Forgot to zero out the .ad_textnull_cnt field in the WARN_CB of
**	    ADF's session CB on session init ... adg_init().
**	01-sep-88 (thurston)
**	    Now adg_init() checks the .adf_maxstring parameter passed in to
**	    see if it is valid.
**	02-dec-88 (jrb)
**	    Zero out the .ad_agabsdate_cnt field in the WARN_CB of the session
**	    control block in adg_init().
**	17-mar-89 (jrb)
**	    Zero out the .ad_decdiv_cnt and .ad_devovf_cnt fields in warn_cb.
**	22-May-89 (anton)
**	    NULL collation descriptor
**	15-jun-89 (jrb)
**	    Changed to use GLOBALCONSTREF instead of GLOBALREF WSCREADONLY.
**	14-may-90 (jrb)
**	    Initialized adf_rms_context to 0 for RMS gateway use.
**      21-dec-1992 (stevet)
**          Added function prototype.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	13-mar-1996 (angusm)
**		Add support for MULTINATIONAL4 date format (bug 48458)
**	23-sep-1996 (canor01)
**	    Moved global data definitions to adgdata.c.
**	24-Jun-1999 (shero03)
**	    Add ISO4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      05-feb-2009 (joea)
**          Rename CM_ischarsetUTF8 to CMischarset_utf8.
**	19-Aug-2009 (kschendel) 121804
**	    Need cm.h for proper CM declarations (gcc 4.3).
**/


/*
** Definition of all global variables owned by this file.
*/

GLOBALREF	ADF_SERVER_CB	*Adf_globs;  /* ADF's global server CB.*/
GLOBALREF	i4				adg_startup_instances;

GLOBALCONSTREF	ADF_OUTARG	Adf_outarg_default; /*Default output fmts.*/


/*
**  Definition of static variables and forward static functions.
*/

static  DB_STATUS   adg_outargs(ADF_CB *adf_scb); /* Init/set ADF_OUTARG struct */


/*{
** Name: adg_init() - Initialize ADF for a session.
**
** Description:
**      This function is the external ADF call "adg_init()" which
**      initializes the ADF for a session within a server.  It verifies
**	that all information in the supplied ADF session control block is
**	valid (including a call to adg_outargs() to init/validate the output
**	formats), then does whatever is needed to initialize the ADF for this
**	session.  A call to adg_startup() must precede all calls to this
**	routine.  In the INGRES DBMS server, adg_startup() will be called
**	once for the life of the server, and adg_init() will be called once
**	for each session.  In a non-server process such as an INGRES frontend
**	these two routines will be called once each, probably back-to-back.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_srv_cb			Pointer to ADF's server control block.
**	    .adf_dfmt			The multinational date format in use.
**					One of:
**					    DB_US_DFMT
**					    DB_MLTI_DFMT
**					    DB_MLT4_DFMT
**					    DB_FIN_DFMT
**					    DB_ISO_DFMT
**					    DB_ISO4_DFMT
**					    DB_GERM_DFMT
**						DB_DMY_FMT
**						DB_YMD_FMT
**						DB_MDY_FMT
**	    .adf_mfmt			Money format and precision to
**					use for this session.
**		.db_mny_sym		The money sign in use (null terminated).
**		.db_mny_lort		One of:
**					    DB_LEAD_MONY  - leading mny symbol.
**					    DB_TRAIL_MONY - trailing mny symbol.
**					    DB_NONE_MONY  - no mny symbol.
**		.db_mny_prec		Number of digits after decimal point.
**					Only legal values are 0, 1, and 2.
**	    .adf_decimal		Decimal character in use specification.
**		.db_decspec		Set to TRUE if a decimal marker is
**					specified in .db_decimal.  If FALSE,
**					ADF will assume '.' is the decimal char.
**		.db_decimal		If .db_decspec is TRUE, this is taken
**					to be the decimal character in use.
**					    ***                            ***
**					    *** NOTE THAT THIS IS A "nat", ***
**					    *** NOT A "char".              ***
**					    ***                            ***
**					This must be either a (i4)'.' or
**					a (i4)','.
**	    .adf_outarg 		Output formatting information.  This
**					is an ADF_OUTARG structure, and contains
**                                      many pieces of information on how the
**                                      various intrinsic datatypes are supposed
**                                      to be formatted when converted into
**                                      string representation.  The caller can
**					request that any field(s) be initialized
**					to the system defaults by setting the
**					field(s) to -1 (for integer fields) or
**					NULLCHAR (for character fields).  Those
**					that are not being initialized will be
**					checked for validity.
**		.ad_c0width		Minimum width of "c" or "char" field.
**		.ad_i1width		Width of "i1" field.
**		.ad_i2width		Width of "i2" field.
**		.ad_i4width		Width of "i4" field.
**		.ad_f4width		Width of "f4" field.
**		.ad_f8width		Width of "f8" field.
**		.ad_i8width		Width of "i8" field.
**    		.ad_f4prec     		Number of decimal places on "f4".
**    		.ad_f8prec     		Number of decimal places on "f8".
**    		.ad_f4style    		"f4" output style ('e', 'f', 'g', 'n').
**    		.ad_f8style    		"f8" output style ('e', 'f', 'g', 'n').
**		.ad_t0width		Min width of "text" or "varchar" field.
**	    .adf_exmathex		What to do on math exceptions.  One of:
**					    ADX_IGN_MATHEX
**					    ADX_WRN_MATHEX
**					    ADX_ERR_MATHEX
**	    .adf_qlang			Query language in use.  This can be set
**					to either DB_QUEL, DB_SQL, or any other
**					supported query language that might be
**					added.
**	    .adf_maxstring		Max user size of any INGRES string for
**					this session.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**	    .adf_outarg
**		.ad_c0width		If caller requested default ...
**		.ad_i1width		            ''
**		.ad_i2width		            ''
**		.ad_i4width		            ''
**		.ad_f4width		            ''
**		.ad_f8width		            ''
**		.ad_i8width			    ''
**    		.ad_f4prec     		            ''
**    		.ad_f8prec     		            ''
**    		.ad_f4style    		            ''
**    		.ad_f8style    		            ''
**		.ad_t0width		            ''
**
**      Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD1001_BAD_DATE		Date format is not legal.
**          E_AD1002_BAD_MONY_SIGN	Money sign is not legal.
**          E_AD1003_BAD_MONY_LORT	Money leading or trailing
**					indicator is not legal.
**          E_AD1004_BAD_MONY_PREC	Money precision is not legal.
**          E_AD1005_BAD_DECIMAL	Decimal specification is not legal.
**	    E_AD1006_BAD_OUTARG_VAL	A supplied outarg value is bad.
**	    E_AD1007_BAD_QLANG		Query language supplied is not legal.
**	    E_AD1008_BAD_MATHEX_OPT	Math exception option is not legal.
**	    E_AD1009_BAD_SRVCB		Either the pointer to server CB is NULL,
**					or the structure found there has not
**					been set up correctly.
**	    E_AD100A_BAD_NULLSTR	Description of NULLstring is illegal.
**
**      Exceptions:
**          none
**
** Side Effects:
**	none
**
** History:
**      24-feb-86 (thurston)
**          Initial creation.
**	30-jun-86 (thurston)
**	    Upgraded the comments for this routine to include adf_scb.adf_outarg
**	    as an input.  Actually, at the moment it is an output as this
**	    routine sets it to be the default values.  At some point this will
**	    need to be changed to take these values as input, or optionally set
**	    them to be the default values.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	03-nov-86 (ericj)
**	    Initialize the ADF math exception warning counts in the ADI_WARN
**	    struct to zero.
**	20-nov-86 (thurston)
**	    Added temp code to assign QUEL as the query language.
**	08-jan-86 (thurston)
**	    Did major overhaul.  adg_init() now checks validity of all fields
**	    in the ADF_CB, returning errors if any are not valid.  Also, allows
**	    the caller to request that it set the default values for any members
**	    of the ADF_OUTARG structure contained therein.  Also, adg_init()
**	    set the decimal character to '.' if the decimal spec says there is
**	    no decimal character specified.  (This should be redundant since
**	    all of the ADF code that requires use of the decimal character has
**	    this same default built in.)
**	17-feb-87 (thurston)
**	    Added line to zero the new .ad_noprint_cnt field of the ADI_WARN
**	    struct in the ADF_CB.
**	24-feb-87 (thurston)
**	    Added code to set up the global pointer to ADF's server control
**	    block.
**	06-mar-87 (thurston)
**	    Added code to verify (or initialize, if requested) the description
**	    of the NULLstring.
**	13-aug-87 (thurston)
**	    Forgot to zero out the .ad_textnull_cnt field in the WARN_CB of
**	    ADF's session CB on session init.
**	20-may-88 (thurston)
**	    Allowed the German date format, DB_GERM_DFMT, to be accepted.
**	01-sep-88 (thurston)
**	    Now checks the .adf_maxstring parameter passed in to see if it is
**	    valid.
**	02-dec-88 (jrb)
**	    Zero out the .ad_agabsdate_cnt field in the WARN_CB of the session
**	    control block in adg_init().
**	17-mar-89 (jrb)
**	    Zero out the .ad_decdiv_cnt and .ad_devovf_cnt fields in warn_db.
**	22-May-89 (anton)
**	    NULL collation descriptor
**	14-may-90 (jrb)
**	    Initialized adf_rms_context to 0 for RMS gateway use.
**      08-may-92 (stevet)
**          Added support of DB_YMD_DFMT, DB_MDY_DFMT, DB_DMY_DFMT new
**          date formats.
**	01-Jul-1999 (shero03)
**	    Added support for II_MONEY_FORMAT=NONE for Sir 92541.
**	17-jul-2000 (rucmi01) Part of fix for bug 101768.
**	    Add code to deal with special case where adg_startup is called 
**          more than once.
**	25-mar-04 (inkdo01)
**	    Init. adf_base_array.
**	27-aug-04 (inkdo01)
**	    Drop adf_base_array (no lonmger needed).
**	10-may-2007 (dougi)
**	    Init adf_utf8_flag.
**	10-may-2007 (gupsh01)
**	    Set the value of adf_utf8_flag based on the character set.
**	14-may-2007 (gupsh01)
**	    For UTF8 strings in the adf_maxstring cannot exceed 
**	    DB_UTF8_MAXSTRING.
**	16-Apr-2008 (kschendel)
**	    Init any-warnings too.
**	29-Sep-2008 (gupsh01)
**	    Fix error is setting the db_maxstring on UTF8 installation.
**      19-nov-2008 (huazh01)
**          Remove 'ad_1rsvd_cnt'. (b121246).
**      24-Jun-2010 (horda03) B123987 
**          Initialise adf_misc_flags 
**      09-nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
*/

DB_STATUS
adg_init(
ADF_CB  *adf_scb)
{
    DB_STATUS		db_stat;
    i4			i;

    /* Check and set up GLOBAL for server control block. */
    /* ------------------------------------------------- */
	/*
	** If adg_startup has been called more than once don't reset this here.
	** See comments in adg_startup() code for more details.
	*/
	if (adg_startup_instances <= 1)
        Adf_globs = (ADF_SERVER_CB *)adf_scb->adf_srv_cb;

    if (    Adf_globs == NULL
	||  Adf_globs->adf_type != ADSRV_TYPE
	||  Adf_globs->adf_ascii_id != ADSRV_ASCII_ID
	|| !Adf_globs->Adi_inited
	||  Adf_globs->Adi_datatypes == NULL
	||  Adf_globs->Adi_dtptrs == NULL
	||  Adf_globs->Adi_operations == NULL
       )

	return (adu_error(adf_scb, E_AD1009_BAD_SRVCB, 0));

    /* Initialize the math exception (and other) warning counts. */
    /* --------------------------------------------------------- */
    adf_scb->adf_warncb.ad_anywarn_cnt = 0;
    adf_scb->adf_warncb.ad_intdiv_cnt = 0;
    adf_scb->adf_warncb.ad_intovf_cnt = 0;
    adf_scb->adf_warncb.ad_fltdiv_cnt = 0;
    adf_scb->adf_warncb.ad_fltovf_cnt = 0;
    adf_scb->adf_warncb.ad_fltund_cnt = 0;
    adf_scb->adf_warncb.ad_mnydiv_cnt = 0;
    adf_scb->adf_warncb.ad_noprint_cnt = 0;
    adf_scb->adf_warncb.ad_textnull_cnt = 0;
    adf_scb->adf_warncb.ad_agabsdate_cnt = 0;
    adf_scb->adf_warncb.ad_decdiv_cnt = 0;
    adf_scb->adf_warncb.ad_decovf_cnt = 0;

    /* Initialize the collation sequence */
    adf_scb->adf_collation = NULL;

    /* Initialize UTF8 flag */
    if (CMischarset_utf8())
	adf_scb->adf_utf8_flag = AD_UTF8_ENABLED;
    else
        adf_scb->adf_utf8_flag = 0;

    /* Initialize RMS date conversion context */
    adf_scb->adf_rms_context = 0;

    /* Check date format. */
    /* ------------------ */
    if (    adf_scb->adf_dfmt != DB_US_DFMT
	&&  adf_scb->adf_dfmt != DB_MLTI_DFMT
	&&  adf_scb->adf_dfmt != DB_MLT4_DFMT
	&&  adf_scb->adf_dfmt != DB_FIN_DFMT
	&&  adf_scb->adf_dfmt != DB_ISO_DFMT
	&&  adf_scb->adf_dfmt != DB_ISO4_DFMT
	&&  adf_scb->adf_dfmt != DB_GERM_DFMT
	&&  adf_scb->adf_dfmt != DB_YMD_DFMT
	&&  adf_scb->adf_dfmt != DB_MDY_DFMT
	&&  adf_scb->adf_dfmt != DB_DMY_DFMT
       )
	return (adu_error(adf_scb, E_AD1001_BAD_DATE, 2,
			 (i4) sizeof(adf_scb->adf_dfmt),
			 (i4 *) &adf_scb->adf_dfmt));

    /* Check money symbol.  (i.e. Just verify that it is null terminated.) */
    /* ------------------------------------------------------------------- */
    for (i=0; i<=DB_MAXMONY && adf_scb->adf_mfmt.db_mny_sym[i] != NULLCHAR; i++)
	;
    if (i > DB_MAXMONY)
	return (adu_error(adf_scb, E_AD1002_BAD_MONY_SIGN, 2,
			 (i4) 5,	    /* Show 5 chars */
			 (i4 *) adf_scb->adf_mfmt.db_mny_sym));

    /* Check money symbol leading or trailing indicator. */
    /* ------------------------------------------------- */
    if (    adf_scb->adf_mfmt.db_mny_lort != DB_LEAD_MONY
	&&  adf_scb->adf_mfmt.db_mny_lort != DB_TRAIL_MONY
	&&  adf_scb->adf_mfmt.db_mny_lort != DB_NONE_MONY
       )
	return (adu_error(adf_scb, E_AD1003_BAD_MONY_LORT, 2,
			 (i4) sizeof(adf_scb->adf_mfmt.db_mny_lort),
			 (i4 *) &adf_scb->adf_mfmt.db_mny_lort));

    /* Check money precision. */
    /* ---------------------- */
    if (    adf_scb->adf_mfmt.db_mny_prec < 0
	||  adf_scb->adf_mfmt.db_mny_prec > 2
       )
	return (adu_error(adf_scb, E_AD1004_BAD_MONY_PREC, 2,
			 (i4) sizeof(adf_scb->adf_mfmt.db_mny_prec),
			 (i4 *) &adf_scb->adf_mfmt.db_mny_prec));

    /* Check/set decimal character */
    /* --------------------------- */
    if (adf_scb->adf_decimal.db_decspec)
    {
	if (	(char) adf_scb->adf_decimal.db_decimal != '.'
	    &&	(char) adf_scb->adf_decimal.db_decimal != ','
	   )
	    return (adu_error(adf_scb, E_AD1005_BAD_DECIMAL, 2,
			     (i4) sizeof(adf_scb->adf_decimal.db_decimal),
			     (i4 *) &adf_scb->adf_decimal.db_decimal));
    }
    else
    {
	adf_scb->adf_decimal.db_decspec = TRUE;
	adf_scb->adf_decimal.db_decimal = (i4) '.';
    }

    /* Check/set ADF_OUTARG structure. */
    /* ------------------------------- */
    if (db_stat = adg_outargs(adf_scb))
	return (db_stat);

    /* Check query language. */
    /* --------------------- */
    if (adf_scb->adf_qlang != DB_QUEL  &&  adf_scb->adf_qlang != DB_SQL)
	return (adu_error(adf_scb, E_AD1007_BAD_QLANG, 2,
			 (i4) sizeof(adf_scb->adf_qlang),
			 (i4 *) &adf_scb->adf_qlang));

    /* Check math exception option. */
    /* ---------------------------- */
    if (    adf_scb->adf_exmathopt != ADX_IGN_MATHEX
	&&  adf_scb->adf_exmathopt != ADX_WRN_MATHEX
	&&  adf_scb->adf_exmathopt != ADX_ERR_MATHEX
       )
	return (adu_error(adf_scb, E_AD1008_BAD_MATHEX_OPT, 2,
			 (i4) sizeof(adf_scb->adf_exmathopt),
			 (i4 *) &adf_scb->adf_exmathopt));

    /* Check/set description of NULLstring */
    /* ----------------------------------- */
    if (adf_scb->adf_nullstr.nlst_length == -1)
    {
	/* initialize to hard-wired system default */
	adf_scb->adf_nullstr.nlst_length = 0;
    }
    else if (   adf_scb->adf_nullstr.nlst_length < 0
	     || (   adf_scb->adf_nullstr.nlst_length > 0
		 && adf_scb->adf_nullstr.nlst_string == NULL
		)
	    )
    {
	return (adu_error(adf_scb, E_AD100A_BAD_NULLSTR, 0));
    }

    /* {@fix_me@} */
    /* For now, always set the spoken language to English */
    /* -------------------------------------------------- */
    adf_scb->adf_slang = 1;	/* English */

    /* Check max size of INGRES strings */
    /* -------------------------------- */
    if ( ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) && 
	  (adf_scb->adf_maxstring <= 2 ))
	||  (adf_scb->adf_maxstring <= 0 )
	||  adf_scb->adf_maxstring > DB_GW4_MAXSTRING
       )
    {
	i4	max_maxstring = DB_GW4_MAXSTRING;
	return (adu_error(adf_scb, E_AD100B_BAD_MAXSTRING, 4,
			 (i4) sizeof(adf_scb->adf_maxstring),
			 (i4 *) &adf_scb->adf_maxstring,
			 (i4) sizeof(max_maxstring),
			 (i4 *) &max_maxstring));
    }

    /* For UTF8 strings in the adf_maxstring cannot exceed DB_UTF8_MAXSTRING. 
    ** reduce the size to DB_UTF8_MAXSTRING.
    if ((adf_scb->adf_maxstring > DB_UTF8_MAXSTRING ) && 
	(adf_scb->adf_utf8_flag & AD_UTF8_ENABLED))
        adf_scb->adf_maxstring = DB_UTF8_MAXSTRING;
    */

    /* Initialise the adf_misc_flags */
    adf_scb->adf_misc_flags = 0;

    return (E_DB_OK);
}


/*
** Name: adg_outargs() - Initialize/validate output formats.
**
** Description:
**      This routine initializes all of the default output formats for INGRES
**      datatypes that the caller has requested.  To request a field in the
**      ADF_OUTARG structure to be set to the default value, the caller fills it
**      with -1 (for the integer members) or NULLCHAR (for the character
**      members) before calling this routine.  For those fields that are not
**      being set to the default, this routine will check for valid entries, and
**      return an appropriate error if it finds an invalid one. 
**
**      This routine is called directly from adg_init() during ADF session
**      initialization. The caller of adg_init() may wish to set all of the
**      fields to the default values before processing the appropriate INGRES
**      command line flags, and then set whatever fields need be set after.  Or,
**      the caller may wish to set those fields affected by the command line
**      flags prior to adg_init(), and ask for the rest to be set to default
**      values, thus taking advantage of this routines validity checking.  Note
**      that this routine may also be called (through a call to adg_init()) to
**	check validity after a "set command" that would have affected the
**	ADF_OUTARG structure. 
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_outarg 		Output formatting information.  This
**					is an ADF_OUTARG structure, and contains
**                                      many pieces of information on how the
**                                      various intrinsic datatypes are supposed
**                                      to be formatted when converted into
**                                      string representation.  The caller can
**					request that any field(s) be initialized
**					to the system defaults by setting the
**					field(s) to -1 (for integer fields) or
**					NULLCHAR (for character fields).  Those
**					that are not being initialized will be
**					checked for validity.
**		.ad_c0width		Minimum width of "c" or "char" field.
**		.ad_i1width		Width of "i1" field.
**		.ad_i2width		Width of "i2" field.
**		.ad_i4width		Width of "i4" field.
**		.ad_f4width		Width of "f4" field.
**		.ad_f8width		Width of "f8" field.
**		.ad_i8width		Width of "i8" field.
**    		.ad_f4prec     		Number of decimal places on "f4".
**    		.ad_f8prec     		Number of decimal places on "f8".
**    		.ad_f4style    		"f4" output style ('e', 'f', 'g', 'n').
**    		.ad_f8style    		"f8" output style ('e', 'f', 'g', 'n').
**		.ad_t0width		Min width of "text" or "varchar" field.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**	    .adf_outarg
**		.ad_c0width		If caller requested default ...
**		.ad_i1width		            ''
**		.ad_i2width		            ''
**		.ad_i4width		            ''
**		.ad_f4width		            ''
**		.ad_f8width		            ''
**		.ad_i8width			    ''
**    		.ad_f4prec     		            ''
**    		.ad_f8prec     		            ''
**    		.ad_f4style    		            ''
**    		.ad_f8style    		            ''
**		.ad_t0width		            ''
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD1006_BAD_OUTARG_VAL	A supplied outarg value is bad.
**
**      Exceptions:
**          none
**
** Side Effects:
**	none
**
** History:
**	03-jun-86 (ericj)
**          Created.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	08-jan-86 (thurston)
**	    Did major overhaul.  Now allows caller to specify which if any
**          fields are to be set to their default values.  Those that are not
**          are checked for validity and an error returned.  Routine now returns
**          a DB_STATUS instead of VOID. 
**	12-apr-04 (inkdo01)
**	    Added ad_i8width for support of BIGINT.
*/

static DB_STATUS
adg_outargs(ADF_CB  *adf_scb)
{
    ADF_OUTARG	    *outarg = &adf_scb->adf_outarg;
    DB_STATUS	    db_stat;


    for (;;)	/* Something to break out of if we detect an error. */
    {
	/* Check/set minimum c/char output width */
	/* ------------------------------------- */
	if (outarg->ad_c0width == -1)
	    outarg->ad_c0width = Adf_outarg_default.ad_c0width;
	else
	    if (outarg->ad_c0width <= 0  ||  outarg->ad_c0width > 255)
		break;

	/* Check/set minimum text/varchar output width */
	/* ------------------------------------------- */
	if (outarg->ad_t0width == -1)
	    outarg->ad_t0width = Adf_outarg_default.ad_t0width;
	else
	    if (outarg->ad_t0width <= 0  ||  outarg->ad_t0width > 255)
		break;

	/* Check/set i1 output width */
	/* ------------------------- */
	if (outarg->ad_i1width == -1)
	    outarg->ad_i1width = Adf_outarg_default.ad_i1width;
	else
	    if (outarg->ad_i1width <= 0  ||  outarg->ad_i1width > 255)
		break;

	/* Check/set i2 output width */
	/* ------------------------- */
	if (outarg->ad_i2width == -1)
	    outarg->ad_i2width = Adf_outarg_default.ad_i2width;
	else
	    if (outarg->ad_i2width <= 0  ||  outarg->ad_i2width > 255)
		break;

	/* Check/set i4 output width */
	/* ------------------------- */
	if (outarg->ad_i4width == -1)
	    outarg->ad_i4width = Adf_outarg_default.ad_i4width;
	else
	    if (outarg->ad_i4width <= 0  ||  outarg->ad_i4width > 255)
		break;

	/* Check/set i8 output width */
	/* ------------------------- */
	if (outarg->ad_i8width == -1)
	    outarg->ad_i8width = Adf_outarg_default.ad_i8width;
	else
	    if (outarg->ad_i8width <= 0  ||  outarg->ad_i8width > 255)
		break;

	/* Check/set f4 output width */
	/* ------------------------- */
	if (outarg->ad_f4width == -1)
	    outarg->ad_f4width = Adf_outarg_default.ad_f4width;
	else
	    if (outarg->ad_f4width <= 0  ||  outarg->ad_f4width > 255)
		break;

	/* Check/set f8 output width */
	/* ------------------------- */
	if (outarg->ad_f8width == -1)
	    outarg->ad_f8width = Adf_outarg_default.ad_f8width;
	else
	    if (outarg->ad_f8width <= 0  ||  outarg->ad_f8width > 255)
		break;

	/* Check/set f4 precision */
	/* ---------------------- */
	if (outarg->ad_f4prec == -1)
	    outarg->ad_f4prec = Adf_outarg_default.ad_f4prec;
	else
	    if (    outarg->ad_f4prec < 0
		||  outarg->ad_f4prec >= outarg->ad_f4width
	       )
		break;

	/* Check/set f8 precision */
	/* ---------------------- */
	if (outarg->ad_f8prec == -1)
	    outarg->ad_f8prec = Adf_outarg_default.ad_f8prec;
	else
	    if (    outarg->ad_f8prec < 0
		||  outarg->ad_f8prec >= outarg->ad_f8width
	       )
		break;

	/* Check/set f4 style */
	/* ------------------ */
	if (outarg->ad_f4style == NULLCHAR)
	    outarg->ad_f4style = Adf_outarg_default.ad_f4style;
	else
	    if (    outarg->ad_f4style != 'e'
		&&  outarg->ad_f4style != 'f'
		&&  outarg->ad_f4style != 'g'
		&&  outarg->ad_f4style != 'n'
	       )
		break;

	/* Check/set f8 style */
	/* ------------------ */
	if (outarg->ad_f8style == NULLCHAR)
	    outarg->ad_f8style = Adf_outarg_default.ad_f8style;
	else
	    if (    outarg->ad_f8style != 'e'
		&&  outarg->ad_f8style != 'f'
		&&  outarg->ad_f8style != 'g'
		&&  outarg->ad_f8style != 'n'
	       )
		break;

	/* All outarg values checked or set OK. */
	return (E_DB_OK);
    }

    /* If we got here, we have a bad outarg value */
    return (adu_error(adf_scb, E_AD1006_BAD_OUTARG_VAL, 0));
}
