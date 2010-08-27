/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/
/**
** Name: ADF.H - All external definitions needed to use the ADF.
**
** Description:
**      This file contains all of the typedefs and defines necessary
**      to use any of the Abstract Datatype Facility routines.  It
**      depends on the DBMS.H header file already being #included.
**
**      In all of the following descriptions, whenever the length of
**      a datatype is talked about, unless otherwise stated, this refers
**      to the internal length.  That is, the number of bytes required
**      to store the data, not the user specified length, as in "text(50)".
**
**      This file defines the following typedefs:
**	ADF_OUTARG	- A struct which defines how output should be formatted
**			    for a particular session.
**	ADF_ERROR	- A struct which will be used for processing ADF errors.
**	ADI_WARN	- A struct that will be used for processing ADF
**			  warnings.
**      ADX_MATHEX_OPT  - Used to hold one of the 3 possible options for
**                        handling math exceptions in the adx_handler()
**                        routine (ignore, warn, error).
**      ADK_CONST_BLK   - ADF query constants block.
**	ADF_CB		- ADF session control block.
**      ADI_OP_ID	- Used to hold an operator id.
**      ADI_OP_NAME	- Used to hold an operator name.
**      ADI_OPINFO	- Used to hold info about an operator.
**      ADI_DT_NAME	- Used to hold a datatype name.  (Note:  There is
**                          no ADI_DT_ID; use DB_DT_ID defined in DBMS.H,
**                          instead.)
**      ADI_DT_BITMASK  - A bit vector of 128 bits that represents some
**                          set of the possible 128 datatypes via the
**                          bits that are set.
**      ADI_LENSPEC	- Method for computing length of result from a
**                          function instance based on the length of
**                          the arguments to the function instance.
**	ADI_FI_ID	- Used to hold an function instance id.
**      ADI_FI_DESC	- Function instance description.  Contains all
**                          all information about each instance of a
**                          function or operator known to ADF.
**      ADI_FI_TAB	- A table of function instance descriptions.
**                          (i.e. A table of ADI_FI_DESCs).
**	ADI_RSLV_BLK	- The function call block for adi_resolve().
**	ADC_KEY_BLK	- The function call block used to call the
**                          adc_keybld() routine.
**	ADF_FN_BLK	- The function call block used to call the
**                          adf_func() routine.
**      ADF_AG_STRUCT	- This structure is used during 3-phase aggregate
**                          processing.  It is initialized via a call to
**                          adf_agbegin(), used during subsequent calls
**                          adf_agnext(), and the final result of the
**                          aggregate is derived from it by adf_agend().
**	ADP_AG_VECT	- A vector of pointers to the 3 aggregate processing
**                          functions (agbegin, agnext, and agend) for a
**                          specific function instance.  (The function
**                          instance must be for an aggregate function.)
**                          This vector is returned by the adp_aagfunc()
**                          routine as a means of improving performance.
**
** History:
**      22-jan-86 (thurston)
**          Initial creation.
**      20-feb-86 (thurston)
**          Completely reworked.
**      21-feb-86 (thurston)
**          Added the ADI_DT_BITMASK typedef for use by the
**          adi_tycoerce() routine, and the ADI_NOCOERCION
**          #define for use by the adi_ficoerce() routine.
**	27-feb-86 (thurston)
**	    Added the .adf_agnx and .adf_agnd members to the
**	    ADF_AG_STRUCT typedef.
**      21-mar-86 (thurston)
**	    Changed ADI_DT_BITMASK bitmask from four u_i4's to be
**	    an array of u_i4's.
**      25-mar-86 (thurston)
**	    Changed all E_ADxxxx #defines so that the xxxx now represents
**	    a hexadecimal integer.  Also added the E_AD9999_INTERNAL_ERROR
**	    which should be returned only if the underlying ADF data structures
**	    become trashed.
**	28-apr-86 (ericj)
**	    Classified error numbers so that the severity of an ADF error can
**	    be determined.
**	13-may-86 (thurston)
**	    Re-numbered the error codes returned by adx_handler() so that they
**	    fall into the OK range.
**	16-may-86 (thurston)
**	    Changed the name of constant ADI_NOCOMP to ADI_NOCMPL because of
**	    name space con-flicts with ADI_NOCOERCION (8 character uniqueness).
**	    Also, removed the _ADI_FI_TAB, _ADI_FI_DESC, _ADI_DT_NAME, and
**	    _ADI_DT_BITMASK from the fronts of their respective typedefs
**	    because they were not unique to 8 chars and were never used.
**	16-may-86 (thurston)
**	    Changed names of the error codes returned by adx_handler() to
**	    be more generic.
**	19-may-86 (thurston)
**	    Changed the constant ADI_NOCMPL to be ADI_NOFI, which is more
**	    generic.
**	19-may-86 (thurston)
**	    In the ADI_FI_TAB typedef, I changed the definition of the
**	    .adi_tab_fi member to be a pointer to an ADI_FI_DESC instead
**	    of an array of ADI_FI_DESC's, which was incorrect.
**	26-may-86 (ericj)
**	    Moved and redefined error numbers associated with date functions
**	    from "dates.c" to this file.
**	30-may-86 (ericj)
**	    Moved the definition of the outarg struct from aux.h to this
**	    file.  A reference to it will be needed in the ADF control block.
**	10-jun-86 (ericj)
**	    Added a pointer to the ADF_CB in the ADC_KEY_BLK and added the
**	    error number E_AD3003_DLS_NOT_SAME.
**	19-jun-86 (ericj)
**	    Added several new functions to ADP_COM_VECT struct.  These are
**	    adp_dhmin_addr, adp_dhmax_addr, adp_eq_dtln, adp_ds_dtln,
**	    adp_hg_dtln, adp_hp_dtln.  The last four have been added
**	    in preparation for eliminating the adi_dtln_info() routine.
**	19-jun-86 (thurston)
**	    Redesigned the ADI_FI_DESC typedef to cover all needs for an
**	    externally visable function instance description.  Specifically, I
**	    have rearranged the members and added two new members: .adi_fitype
**	    and .adi_agwsdv_len.  I also moved the #defines for ADI_COERCION,
**	    ADI_OPERATOR, ADI_NORM_FUNC, and ADI_AGG_FUNC, as well as the
**	    new ADI_COMPARISON, into this typedef from the <adfint.h> internal
**	    header file.
**	23-jun-86 (thurston)
**	    Added three more error codes for use by adi_calclen():
**	    E_AD2020_BAD_LENSPEC, E_AD2021_BAD_DT_FOR_PRINT, E_2022_UNKNOWN_LEN.
**	27-jun-86 (thurston)
**	    In the ADI_FI_TAB typedef I changed .adi_lntab to be a i4
**	    instead of an i2.
**	30-jun-86 (thurston)
**	    Added the .adi_agprime member to the ADI_FI_DESC typedef to describe
**	    whether an aggregate f.i. is "prime" or not.  (See the comment in
**	    the typedef for the meaning of a prime aggregate.)
**	30-jun-86 (thurston)
**	    Added a new error code, E_AD0FFF_NOT_IMPLEMENTED_YET, which I put
**	    in the "WARNING" range of ADF errors.  This is to be used during
**	    development when some functionality has not yet been implemented.
**	    This error code should probably be removed from the production
**	    system, as it "should" never be used by then.
**	30-jun-86 (thurston)
**	    Added the .adp_isnull_addr member to the ADP_COM_VECT typedef to
**	    hold a pointer to a routine that will check to see if a supplied
**	    data value is null or not. 
**	01-jul-86 (thurston)
**	    Added the ADI_RESLEN constant to the ADI_LENSPEC typedef.
**	02-jul-86 (thurston)
**	    Re-numbered the function instance type constants in the ADI_FI_DESC
**	    typedef, so that none of them are zero.
**	02-jul-86 (thurston)
**	    Removed the ADP_COM_VECT typedef (moved down into <adfint.h>) since
**	    the external adp_acommon() call has been removed.
**	08-jul-86 (ericj)
**	    Changed the sequencing of the "type of key" constant definitions
**	    and added ADC_KNOKEY as needed by OPF.
**	10-jul-86 (thurston)
**	    Added several error codes needed by the ADE module.
**	17-jul-86 (ericj)
**	    Added the ADF_ERROR struct definition and placed this structure
**	    in the ADF_CB.
**	21-jul-86 (thurston)
**	    Added more error codes needed by the ADE module.
**	26-jul-86 (ericj)
**	    Removed the pointer to the ADF_CB from the ADC_KEY_BLK struct.
**	28-jul-86 (thurston)
**	    In the ADF_AG_STRUCT typedef, I changed the .adf_agnx and .adf_agend
**	    members to be pointers to routines that return DB_STATUS instead of
**	    i4.  In the ADP_AG_VECT typedef, I did the same to the
**	    .adp_agbegin_addr, .adp_agnext_addr, and .adp_agend_addr members.
**	28-jul-86 (ericj)
**	    Updated for new ADF error handling.
**	31-jul-86 (ericj)
**	    Added error codes, E_AD1040_CV_DVBUF_TOOSHORT and
**	    E_AD1041_CV_NETBUF_TOOSHORT, for the adc_tonet() and
**	    adc_tolocal() routines.
**	07-aug-86 (ericj)
**	    Added ADI_WARN struct.  This will be used to count various
**	    ADF warning state occurrences.  It will be used in the
**	    processing of exceptions by adx_handler() and by adx_chkwarn()
**	    to check for and process ADF warnings.  Also, added the 
**	    ADX_MATHEX_OPT as a member of an ADF_CB.  Deleted the
**	    E_AD0003_EX_ERR_DECL number and renumbered E_AD0002_EX_WRN_CONT
**	    and E_AD0004_EX_UNKNOWN to E_AD0115_EX_WRN_CONT and
**	    E_AD0116_EX_UNKNOWN respectively so that they will fall into
**	    the warning range of numbers.
**	18-sep-86 (thurston)
**	    Added FUNC_EXTERN's for adc_tonet() and adc_tolocal().
**	02-oct-86 (thurston)
**	    Changed the ADI_FI_ID typedef from an i4 to an i2.
**	13-oct-86 (thurston)
**	    Added the ADI_SUMT constant to the ADI_LENSPEC typedef.
**	03-nov-86 (ericj)
**	    Added ADF error code, E_AD1025_BAD_ERROR_NUM for unknown
**	    error codes passed to adu_error()
**	11-nov-86 (thurston)
**	    Added the ADI_P2D2, ADI_2XP2, and ADI_2XM2 constants to the
**	    ADI_LENSPEC typedef.
**	13-nov-86 (thurston)
**	    Added the .adf_qlang member to the ADF_CB typedef.
**	20-nov-86 (thurston)
**	    Added the E_AD2030_LIKE_ONLY_FOR_SQL error.
**	01-dec-86 (thurston)
**	    Added the E_AD1010_BAD_EMBEDDED_TYPE, E_AD1011_BAD_EMBEDDED_LEN, and
**	    E_AD1012_NULL_TO_NONNULL error codes. 
**	05-jan-87 (thurston)
**	    Added the E_AD1013_EMBEDDED_NUMOVF, and E_AD0101_EMBEDDED_CHAR_TRUNC
**	    error codes for use by adc_dbcvteb().
**	06-jan-87 (thurston)
**	    Commented out the ADI_DTLN_VECT typedef, since it is no longer used.
**	    Will keep it in this file for the time being, in case it will ever
**	    be needed again.
**	06-jan-87 (thurston)
**	    Added FUNC_EXTERN's for adc_tmlen(), adc_tmcvt(), adc_dbtoev(),
**	    adc_dbcvtev(), adh_evtodb(), and adh_evcvtdb().  Removed
**	    FUNC_EXTERN's for adc_cvdsply() and adc_cveql().
**     08-jan-87 (thurston)
**          In the ADF_OUTARG typedef, I changed member names ad_linesperpage
**	    and ad_coldelim to be ad_1_rsvd and ad_3_rsvd, since they are not
**	    used by ADF.  Also re-aligned the members, and added an extra i4
**	    reserved, ad_2_rsvd, and an extra char reserved, ad_4_rsvd.
**	08-jan-87 (thurston)
**	    Added the E_AD1006_BAD_OUTARG_VAL, E_AD1007_BAD_QLANG and
**	    E_AD1008_BAD_MATHEX_OPT errors.
**	04-feb-87 (thurston)
**	    Fixed typo's in FUNC_EXTERN's; adg_startup() and adg_shutdown() were
**	    listed as adi_startup() and adi_shutdown.  Also, temporarily removed
**	    FUNC_EXTERN's for adc_dbtoev(), adc_dbcvtev(), adh_evtodb(), and
**	    adh_evcvtdb().
**      13-feb-87 (thurston)
**          Added the .ad_noprint_cnt field, along with some reserved fields,
**	    to the ADI_WARN typedef.
**      18-feb-87 (thurston)
**          Added the ADK_CONST_BLK typedef.  Also added the .adf_constants
**          field to the ADF_CB.
**	20-feb-87 (thurston)
**	    Altered the constant ADI_MXDTS, and its meaning.
**	20-feb-87 (thurston)
**	    Got rid of the #define for `STATIC'.  This is not needed.
**	20-feb-87 (thurstion)
**	    Added the E_AD1050_NULL_HISTOGRAM, and E_AD1060_TM_BUF_TOO_SMALL
**	    error codes.
**	23-feb-87 (thurston)
**	    Added the E_AD550F_BAD_CX_REQUEST error.
**	24-feb-87 (thurston)
**	    Added the .adf_srv_cb member to the ADF_CB typedef.  Also added the
**	    ADG_SZ_SRVCB constant.
**	24-feb-87 (thurston)
**	    Added the E_AD1009_BAD_SRVCB error.
**	25-feb-87 (thurston)
**	    Added FUNC_EXTERN for adg_srv_size(), and set ADG_SZ_SRVCB to
**	    resolve to it.
**	06-mar-87 (thurston)
**	    Added the ADF_STR_NULL typedef, and the E_AD100A_BAD_NULLSTR error.
**	11-mar-87 (thurston)
**	    Two changes to the ADI_FI_DESC typedef:  Removed the .adi_agprime
**	    member, since ADF will no longer be concerned with `PRIME', or
**	    `DISTINCT' aggregates.  In its place, I have added the
**	    .adi_npre_instr field, to handle nullable datatypes.
**	16-mar-87 (thurston)
**	    Added the .adf_slang member to the ADF_CB typedef.
**	25-mar-87 (thurston)
**	    Added FUNC_EXTERN for adc_valchk().
**	03-apr-87 (thurston)
**	    Added ADF_DBMSINFO and ADF_TAB_DBMSINFO typedefs.
**	20-apr-87 (thurston)
**	    Added E_AD1015_BAD_RANGE error code for an illegal QUEL pattern
**	    matching range specification.  Also added FUNC_EXTERN for new
**	    routine, adi_pm_encode().
**	29-apr-87 (thurston)
**	    Added E_AD6000_BAD_MATH_ARG.
**	04-may-87 (thurston)
**	    Made many changes to the lenspec codes in order to avoid creating
**	    intermediate values that are too long.  (Example: concat(c200,c200):
**	    we want to avoid creating a c400, which cannot exist, and causes
**	    errors in the Jupiter code.)
**	07-jun-87 (thurston)
**	    Added E_AD5061_DGMT_ON_INTERVAL, error code generated when user
**	    attempts to use the `date_gmt' function on a date that is a time
**	    interval.
**	24-jun-87 (thurston)
**	    Added E_AD200A_NOCOPYCOERCION error code, FUNC_EXTERN for routine
**	    adi_cpficoerce(), and the constant ADI_COPY_COERCION.
**	01-jul-87 (thurston)
**	    Added E_AD1030_F_COPY_STR_TOOSHORT and E_AD5002_BAD_NUMBER_TYPE.
**      07-jul-87 (jennifer)
**          Added ADT_VALUE, ADT_REPRESENTATION, and ADT_CMP_LIST defintions.
**          for new ADT routines.
**      29-jul-87 (thurston)
**          Added the ad_textnull_cnt field (in ADI_WARN) to count the number of
**          NULL chars converted to blanks for `text'.  Also added the error
**	    code for this error, E_AD0102_NULL_IN_TEXT.
**	14-sep-87 (thurston)
**	    Added the ADI_RSLNO1CT constant to the ADI_LENSPEC typedef.
**	16-sep-87 (thurston)
**	    Changed two error code names:
**	    From E_AD5509_BAD_PMFLAG to E_AD5509_BAD_RANGEKEY_FLAG, and
**	    from E_AD550A_PM_FAILURE to E_AD550A_RANGE_FAILURE.
**	08-oct-87 (thurston)
**	    Added E_AD3050_BAD_CHAR_IN_STRING.
**	09-oct-87 (thurston)
**	    Added option bit constants for the new adi_pm_encode() interface.
**	    Also corrected several #defines that had the comments done
**	    improperly.  Apparently they were never used, or the module that
**	    used them would not have compiled.
**	18-jan-88 (thurston)
**	    Worked on the ADI_FI_DESC typedef:
**	    Added the .adi_fiflags field to this structure, along with the two
**	    constants, ADI_F0_NOFIFLAGS and ADI_F1_VARFI.  Also, changed the
**	    definition of several fields in this structure in the anticipation
**	    that it may someday become part of the database.  (Also, to cut down
**	    on the amount of storage needed to hold the f.i. table.)
**	    Specifically:
**		changed		    from    to
**		-------		    ----    --
**		.adi_fitype	    i4	    i1
**		.adi_agwsdv_len	    i4	    i4
**		.adi_npre_instr	    i4	    i2
**		.adi_numargs	    i4	    i2
**	01-jun-88 (thurston)
**	    Added the .adc_escape and .adc_isescape fields to the ADC_KEY_BLK
**	    typedef.
**	09-jun-88 (thurston)
**	    Added two new error codes for the ESCAPE clause on LIKE or NOT LIKE:
**	    E_AD5510_BAD_DTID_FOR_ESCAPE and E_AD5511_BAD_DTLEN_FOR_ESCAPE.
**	21-jun-88 (thurston)
**	    Added three new error codes for the ESCAPE clause on LIKE or NOT
**	    LIKE:  E_AD1016_PMCHARS_IN_RANGE, E_AD1017_ESC_AT_ENDSTR, and
**	    E_AD1018_BAD_ESC_SEQ.
**	23-jun-88 (thurston)
**	    Added the .adf_isescape and .adf_escape members to the ADF_FN_BLK
**	    typedef.
**	07-jul-88 (thurston)
**	    Added the ADI_OPINFO typedef, and FUNC_EXTERNs for three new
**	    routines:  adi_op_info(), adc_klen(), and adc_kcvt().
**	02-aug-88 (thurston)
**	    Added the ADC_0001_FMT_SQL and ADC_0002_FMT_QUEL flags.
**	04-aug-88 (thurston)
**	    Added FUNC_EXTERN's for adc_gca_decompose(), adc_tpls(), and
**	    adc_decompose().  Also added three new error codes:
**		E_AD200D_BAD_BASE_DTID
**		E_AD2040_INCONSISTENT_TPL_CNT
**		E_AD2041_TPL_ARRAY_TOO_SMALL
**		E_AD2042_MEMALLOC_FAIL
**		E_AD2050_NO_COMVEC_FUNC
**	12-aug-88 (thurston)
**	    Added new error code E_AD2043_ATOMIC_TPL.
**	01-sep-88 (thurston)
**	    Added the new field .adf_maxstring to the ADF_CB structure.
**	01-sep-88 (thurston)
**	    In the ADI_LENSPEC typedef, I commented out the ADI_T0PRN,
**	    ADI_T0CH, and ADI_T0TX constants since it is obsolete, and
**	    re-worded many others to refer to `maxstring' and `DB_CNTSIZE'
**	    instead of 2000 (or 255), or 2, respectively.
**	01-sep-88 (thurston)
**	    Added new error code E_AD100B_BAD_MAXSTRING.
**	23-sep-88 (sandyh)
**	    Added ADF_NVL_BIT for global access.
**	26-sep-88 (thurston)
**	    Added new error code E_AD5005_BAD_DI_FILENAME.
**	31-oct-88 (thurston)
**	    Booo!  I removed the ADI_DTLN_VECT struct from this file ... this
**	    had been commented out for almost two years, and it is very obvious
**	    that it will never be used.
**	09-nov-88 (thurston)
**	    Removed E_AD5057... and E_AD505E... since they are *NEVER* used, and
**	    added the new error code, E_AD8999_FUNC_NOT_IMPLEMENTED, to be used
**	    when a function instance is defined, but not yet implemented.
**	02-dec-88 (jrb)
**	    Added the ad_agabsdate_cnt field to count the number of absolute
**	    dates which were found during an aggregation.  (Only date intervals
**	    are allowed for the avg() and sum() aggregates.)
**	    Added E_AD0500_ABS_DATE_IN_AG define.
**	17-mar-89 (jrb)
**	    Added FUNC_EXTERN for adc_encode_colspec().
**	    Added the .adi_psfixed field to the ADI_LENSPEC typedef.
**	    Added decimal fields to ADI_WARN and added error codes:
**		E_AD0126_DECDIV_WARN	    WARN: DECIMAL division by zero
**		E_AD0127_DECOVF_WARN	    WARN: DECIMAL overflow
**		E_AD1126_DECDIV_ERROR	    ERROR: DECIMAL division by zero
**		E_AD1127_DECOVF_ERROR	    ERROR: DECIMAL overflow
**		E_AD5006_BAD_STR_TO_DEC	    Error converting string to decimal
**	07-apr-89 (fred)
**	    Added bitmask definitions used in the adi_dtstat_bits.  These
**	    represent the status bits for the datatype, and are returned by the
**	    (new) routine adi_dtinfo().  This routine was added to support
**	    differentiation necessary for user defined datatypes.
**	18-Apr-89 (fred)
**	    Added definitions for errors for user defined datatypes.
**	28-apr-89 (fred)
**	    Added MACRO definition for mapping user defined datatype id's into
**	    their correct bit position.  This macro also maps their id into the
**	    datatype pointer array (ADF internal).  The macro is
**	    ADI_DT_MAP_MACRO().
**	22-may-89 (jrb)
**	    Added ad_generic_err to ADF_ERROR struct.
**	03-may-89 (anton)
**	    Local collation support
**	28-may-89 (jrb)
**	    Added ADI_RSLV_BLK, errors 2060-2063 and new ADI_FIND_COMMON #def
**	    for datatype resolution in ADF (the adi_resolve routine).
**	01-jun-89 (fred)
**	    Added 2 new errors.
**	10-jul-89 (jrb)
**	    Added E_AD3013 for decimal project.
**	10-jul-89 (fred)
**	    Added 2 new errors (8017 & 8018) for reporting errors in the
**	    function instance table.  These errors will be reported when
**	    function instances reference datatypes which are not used.
**	07-aug-89 (fred)
**	    Added error (8019) for reporting operation type mismatches betwixt
**	    the operations and function instance tables.
**	15-aug-89 (mikem)
**	    added error (E_AD5080_LOGKEY_BAD_CVT_LEN) for reporting bad length
**	    of strings in string->logical key conversions.
**	28-jul-89 (jrb)
**	    o Added some new decimal errors and added #defs for precision and
**	      scale of decimal values converted from other numeric types.
**	    o Added flags for new adi_encode_colspec() function.
**	    o Added about 10 new lenspecs for decimal support.
**	30-oct-89 (fred)
**	    Added New datatype information constants for (initially) BLOB
**	    support.  These are necessary to constrain the datatype from direct
**	    involvment in sorting, keying, and/or histograms.
**	14-nov-89 (fred)
**	    Added ADI_F2_MULTIPASS and ADI_F4_WORKSPACE definitions.
**	    ...for large object support
**	20-nov-89 (fred)
**	    Made definition for ADI_F4_WORKSPACE useful by adding a value (:-)
**	24-apr-90 (jrb)
**	    Added adf_rms_context to ADF session CB for recording date
**	    conversion context for RMS gateway.
**	06-sep-90 (jrb)
**	    Added adf_timezone to ADF session CB for support of independent
**	    time zones per session.
**	31-jul-90 (linda)
**	    Added error codes E_AD0117_IS_MINIMUM, E_AD0118_IS_MAXIMUM, and
**	    E_AD0119_IS_NULL for new ADF routine adc_isminmax().  This routine
**	    is used by Gateways in interpreting key values for positioning and
**	    range limits.
**      21-jan-1991 (jennifer)
**        Added E_AD1061_bad_security_label ERROR.
**      06-feb-91 (stec)
**          Added FUNC_EXTERN for adc_hdec().
**	18-feb-92 (stevet)
**	    Reserved E_AD1200 - E_AD12FF message numbers for ESL work.
**	03-mar-92 (jrb, merged by stevet)
**	    Added E_AD2080_IFTRUE_NEEDS_INT: the ii_iftrue() function's first
**	      parameter must be an integer.
**	    Added E_AD2085_LOCATE_NEEDS_STR: the locate() function takes only
**	      string arguments.
**	09-mar-92 (jrb, merged by stevet)
**	    Added 3 error messages for RESTAB support, and changed the contents
**	    of one other error (in an upward compatible way, of course).
**      07-aug-92 (stevet)
**          Removed "comment out" FUNC_EXTERN for adc_tonet, adc_tolocal, 
**          adh_evtodb and adh_evcvtdb.  Added FUNC_EXTERN for adc_dbtoev.
**	04-Sep-1992 (fred)
**	    Added peripheral datatype support.  Added errors returned
**	    by supporting routines for these datatypes (E_AD7xxx).
**	    Moved coupon & peripheral structures to adp.h.  Added definition
**	    for fexi function for ADF to use to spool blobs.
**	23-Sep-92 (stevet)
**	    Replaced adf_timezone of adf session control block to 
**	    adf_tzcb to support new timezone adjustment methods.
**          Also added ADI_02ADF_PRINT_FEXI to FEXI to support ADF print.
**	05-oct-1992 (fred)
**	    Added bit string errors (1070-1072) and new lenspec (BIT2CHAR,
**	    CHAR2BIT) mechanisms for handling bit string datatypes.
**	28-oct-92 (rickh)
**	    Moved ADT_CMP_LIST to DBMS.H where it is called DB_CMP_LIST.
**      29-oct-1992 (stevet)
**          Added AD_DECIMAL_PROTO constant to adf_proto_level to indicate
**          decimal support. Also added E_AD101A_DT_NOT_SUPPORTED.
**	9-nov-92 (ed)
**	    Remove dependency on DB_MAXNAME
**	30-November-1992 (rmuth)
**	    File Extend Project: Add iitotal_allocated_pages and 
**	    iitotal_overflow_pages FEXI's plus associated error
**	    messages.
**	09-dec-1992 (stevet)
**	    Added ADI_LEN_INDIRECT and ADI_LEN_EXTERN new length spec.
**	11-dec-92 (ed)
**	    Changed ADF_MAXNAME to equal DB_MAXNAME, for object name length 
**	    consistency.... no catalog changes are necessary but user ADT
**	    must be recompiled
**      21-dec-1992 (stevet)
**          Added function prototype.  Also added E_AD2090_BAD_DT_FOR_STRFUNC,
**          E_AD1080_STR_TO_DECIMAL and E_AD1081_STR_TO_FLOAT error code.
**      03-feb-1993 (stevet)
**          Added E_AD1090_BAD_DTDEF for bad DEFAULT value.  Also removed
**          error code for ESL, which is no longer necessary.
**	22-mar-1993 (robf)
**		Added E_AD1062_SECURITY_LABEL_LUB, 
**		      E_AD1063_SECURITY_LABEL_EXTERN
**                    E_AD9004_SECURITY_LABEL
**       9-Apr-1993 (fred)
**          Added AD_BIT_PROTO & AD_BYTE_PROTO to handle the cases for
**            when the system is up to handling bit and byte stream
**            datatypes.
**      23-apr-1993 (stevet)
**          Added FUNC_EXTERN for adg_vlup_setnull().
**       1-Jul-1993 (fred)
**          Changed ADP_LENCHK_FUNC prototype.  The second argument (adc_is_usr
**          was a bool.  The problem with this is that bool's are
**          system dependent (defined in compat.h) but the prototype
**          needs to be known/shared with user code for OME.
**          Therefore, we make it a i4  (== int for users).
**      12-aug-1993 (stevet)
**          Added support for -string_trunc flag.  Added ADF_STRTRUNC_OPT
**          and adf_strunct_opt to ADF_CB.  Also added E_AD1082_STR_TRUNCATE.
**      20-sep-1993 (stevet)
**          Added symbols for class library datatypes.
**      12-Oct-1993 (fred)
**          Added prototype for ADP_SEGLEN_FUNC.
**      20-jan-1994 (stevet)
**          Added E_AD1083_UNTERM_C_STR.
**      13-Apr-1994 (fred)
**          Added E_AD7010_ADP_USER_INTR to correctly handle user
**          interrupts during peripheral operations.  Also added 7011
**          for certain couponify errors.  Found that error message
**          file had previously been out of date...
**      21-feb-1994 (ajc)
**          hp8_bls compiler (c89) could not cope with ADP_SEGLEN_FUNC 
**	    prototype.
**      22-apr-1994 (stevet/rog)
**          Added E_AD7012_ADP_WORKSPACE_TOOLONG.
**	13-nov-94 (nick)
**	    Added adf_year_cutoff ( nee II_DATE_CENTURY_BOUNDARY ) to ADF_CB.
**	15-may-1995 (shero03)
**	    Added adi_function for Rtree
**	21-mar-96 (nick)
**	    Added some comments to ADF_CB declaration.
**	16-may-1996 (shero03)
**	    Added compress/expand routine
**	15_apr_1997 (shero03)
**	    Added 3D Rtree support
**	25_nov_1998 (nanpr01)
**	    Added E_AD5062_DATEOVFLO define.
**	4-dec-98 (stephenb)
**	    add E_AD5062_BAD_SYSTIME.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	16-Feb-1999 (shero03)
**	    Support 4 operands plus a result
** 	08-apr-1999 (wonst02)
** 	    Add prototype for adt_compute_part_change().
**	26-May-1999 (shero03)
**	    Added E_AD2092_BAD_START_FOR_SUBSTR.
**	09-Jun-1999 (shero03)
**	    Added ADI_X2P2 and E_AD5007_BAD_HEX_CHAR.
**      12-aug-99 (stial01)
**          Changed prototype for adi_per_under.
**      12-Jul-1999 (hanal04)
**          Added E_AD101B_BAD_FLT_VALUE for out of range floating point
**          values. b85041.
**      04-feb-2000 (gupsh01)
**          Added E_AD5008_BAD_IP_ADDRESS for the illegal IP address entries.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-aug-2000 (somsa01)
**	    In ADF_AG_STRUCT, added one more array entry to adf_agirsv.
**	03-nov-2000 (somsa01)
**	    Added E_AD1074_SOP_NOT_ALLOWED and AD_B1_PROTO.
**	19-Jan-2001 (jenjo02)
**	    Added b1secure, c2secure parms to adg_startup() prototype,
**	    deleted AD_B1_PROTO.
**	13-mar-2001 (somsa01)
**	    Changed lengths of ADP_TMLEN_FUNC to i4's.
**      16-apr-2001 (stial01)
**          Added support for tableinfo function
**	27-jun-2001 (gupsh01)
**	    Added E_AD2093_BAD_LEN_FOR_SUBSTR.
**      29-jun-2001 (mcgem01)
**          Address OpenROAD interoperability issues by moving new elements
**	    to the end of structure declarations and replacing removed
**	    elements.
**	18-sept-01 (inkdo01)
**	    Added E_AD5009_BAD_RESULT_STRING_TYPE
**	25-sep-01 (mcgem01)
**	    Move the OpenROAD argument datatypes in the function instance 
**	    table structure to after our array.
**	9-oct-01 (inkdo01)
**	    Remove adi_dt1/2 added above after discussions with Don Criley 
**	    (OR development).
**      11-dec-01 (inkdo01)
**          Added defines for the various "trim" options.
**      21-mar-02 (gupsh01)
**          Added error E_AD5010_SOURCE_STRING_TRUNC. Added new 
**	    reslen types for UTF8 to unicode coercion.
**      29-Mar-02 (gordy)
**          Removed adc_decompose(), adc_gca_decompose(), adc_tpls().
**          Intended for Het-Net but not used.
**	21-may-02 (gupsh01)
**	    Added new errors E_AD5011 and E_AD5012 for coercion from / to
**	    unicode.
**	26-June-2001 (yeema01) Bug# 104440
**	    Porting changes for OR4.1 on Compaq platform (axp_osf).
**	    Explicitly to define pointer size as 64-bits pointer since
**	    OpenROAD is built in 32-bits pointer size context. 
**	    Also added function prototypes for IIAFcehConversionErrorHandler .
**	24-sep-2001 (crido01) Sir 105874 
**	    Added entries for 2.6 in order to promote interoperabilty with
**	    OpenROAD 4.x
**	24-sep-2001 (crido01)
**	    Add xde to the last change
**	02-aug-2002 (gupsh01)
**	    Added new error message E_AD5019_UDECOMP_COL in order to report
**	    error from adu_unorm, when a decomposition of a non decomposing  
**	    character is requested. 
**	30-sep-2002 (drivi01)
**	    Added a new flag ADI_DISABLED. It is used to mark functions with 
**	    with undefined operations, to insure that the recovery server is
**	    able to startup when it is being linked with udts.
**	25-sep-2002 (gupsh01)
**	    Added new flag ADI_F64_ALLSKIPUNI. This flag will disable calling
**	    function instances with DB_ALL_TYPE paramters, if the input 
**	    parameters are of unicode type. 
**	25-nov-2003 (gupsh01)
**	    Added new functions adu_nvchr_toutf8() and adu_nvchr_fromutf8()
**	    which can be called from the front end programs to translate 
**	    between unicode and UTF8 format.
**	23-jan-2004 (gupsh01)
**	    Added errors E_AD5015_INVALID_BYTESEQ and E_AD5016_NOUNIMAP_FOUND.
**	29-jan-2004 (gupsh01)
**	    Added error E_AD5017_NOMAPTBL_FOUND. 
**	4-mar-04 (inkdo01)
**	    Added adi_castid() for new ANSI functions.
**	24-mar-04 (inkdo01)
**	    Added AD_I8_TO_DEC_xxx constants for bigint.
**      12-Apr-2004 (stial01)
**          Define tdbi_length as SIZE_TYPE.
**	23-apr-04 (inkdo01)
**	    Add adt_partbreak_compare().
**      14-Jul-2004 (hanal04) Bug 112283 INGSRV2819
**          Added E_SPATIAL_MASK in order to detect spatial datatype errors
**          reported by adc_valchk(). In these cases the error code will
**          be zero and he user error code will be an error code from
**          sperr.h
**	03-dec-2004 (gupsh01)
**	    Added E_AD0103_UNINORM_IGN_TRUNC to indicate that unicode 
**	    normalization was truncated. This will not return an error message
**	    to the user.
**	09-Feb-2005 (jenjo02)
**	    Added function prototype for adt_compare().
**	09-Feb-2005 (gupsh01)
**	    Moved defines for U_BLANK and U_NULL to here.
**	14-Feb-2005 (gupsh01)
**	    Added definition for adg_init_unimap() and adg_clean_unimap().
**	09-Apr-2005 (lakvi01)
**	    Prevented multiple inclusion of adf.h
**	11-apr-05 (inkdo01)
**	    Added E_AD50A0_BAD_UUID_PARM msg to interpret errors from 
**	    IDuuid_from_string().
**	22-apr-05 (inkdo01)
**	    Added E_AD5081_UNICODE_FUNC_PARM.
**	17-May-2005 (gupsh01)
**	    Added adg_setnfc() and adu_unorm() to this function.
**	21-Jun-2005 (gupsh01)
**	    Added missing adg_init_unimap and adg_clean_unimap declarations.
**	11-july-05 (inkdo01)
**	    Added E_AD5082_NON_UNICODE_DB.
**	04-Oct-05 (toumi01) BUG 115330
**	   Added AD_MONEY_FUZZ to correct (recently modified) money rounding
**	   function.
**	26-Apr-2006 (jenjo02)
**	   Added function prototype for adt_ixlcmp();
**	17-jun-2006 (gupsh01)
**	   Added adu_7from_dtntrnl() and adu_6to_dtntrnl() for ANSI datetime
**	   support.
**	19-aug-2006 (gupsh01)
**	   Added ADK_CURR_TIME, ADK_CURR_DATE... for ANSI datetime system
**	   constants.
**	05-sep-2006 (gupsh01)
**	   Added E_AD5064_DATEANSI.
**	14-sep-2006 (gupsh01)
**	   Added E_AD5065_DATE_ALIAS_NOTSET.
**	18-Oct-2006 (kiria01)
**	   Added definitions for the Unicode whitespace characters for SQUEEZE
**      16-oct-2006 (stial01)
**         Define locator errors.
**      06-nov-2006 (stial01)
**         Added AD_LOCATOR for locator datatypes
**	14-nov-2006 (gupsh01)
**	   Added E_AD5066_DATE_COERCION.
**	22-nov-2006 (gupsh01)
**	   Added E_AD5067_ANSIDATE_4DIGYR.
**	10-dec-2006 (gupsh01)
**	    Added E_AD5068_ANSIDATE_DATESUB.
**	12-dec-2006 (gupsh01)
**	    Added ANSI date/time formatting errors.
**	08-jan-2007 (gupsh01)
**	    Added E_AD5078_ANSITIME_INTVLADD to avoid adding time and 
**	    interval types.
**	09-mar-2007 (gupsh01)
**	    Added defines for Greek Final sigma's.
**	3-apr-2007 (dougi)
**	    Added ADI_F4_CAST to tweak CAST semantics.
**	29-May-2007 (kschendel) SIR 122513
**	    Added adfcb param to partbreak compare.
**	11-sep-2007 (kibro01) b119098
**	    Corrected typo in max number of operands to make stack overflows
**	    in opc_qual much less likely.
**	16-Oct-2007 (kibro01) b119318
**	    Add adu_date_format function
**	7-Nov-2007 (kibro01) b119428
**      20-jul-07 (stial01)
**          Added locator errors.
**	14-Nov-2007 (kibro01) b119374
**	   Add visible definition of ADFI_845_STDDEV_SAMP_FLT for use in
**	   switching off an optimisation in psl.
**	27-Dec-2007 (kiria01) b119374
**	   Move above change into pslsgram.yi to avoid include ordering
**	   build errors when adf.h included before adffiids.h
**	07-Mar-2008 (kiria01) b120043
**	    Added error messages in support of missing date operators
**      12-Mar-2008 (coomi01) b119995
**          Add message for ansi trim len spec error.
**	16-Mar-2008 (kiria01) b120004
**	    As part of cleaning up timezone handling the internal date
**	    conversion routines now can detect and return errors.
**	25-Jun-2008 (kiria01) SIR120473
**	    Added E_AD5512_BAD_DTID2_FOR_ESCAPE,
**	    E_AD5513_BAD_DTLEN2_FOR_ESCAPE,
**	    E_AD101C_BAD_ESC_SEQ, E_AD101D_BAD_ESC_SEQ,
**	    E_AD101E_ESC_SEQ_CONFLICT and E_AD101F_REG_EXPR_SYN
**      10-sep-2008 (gupsh01,stial01)
**          Added lenspec for unicode normalization
**	04-feb-2009 (gupsh01)
**	    Added ADF_UNI_TMPLEN as temporary buffer length. 
**	12-Oct-2008 (kiria01) SIR121012
**	    Brought interface upto date with ADE_ESC_CHAR enhancments
**	    to allow adf_func to be used for any FI function.
**	07-Jan-2009 (kiria01) SIR120473
**	    Added E_AD1026_PAT_TOO_CPLX & E_AD1027_PAT_LEN_LIMIT
**      04-feb-2009 (gupsh01)
**          Added ADF_UNI_TMPLEN as temporary buffer length.
**	10-Feb-2009 (kiria01) SIR120473
**	    Added prototype adu_patcomp_summary()
**	11-Mar-2009 (kiria01) b121781
**	    Added prototype adu_decprec()
**      27-Mar-2009 (hanal04) Bug 121857
**          Correct FUNC_EXTERN for adi_resolve(). Add missing
**          varchar_precedence parameter.
**      22-Apr-2009 (Marty Bowes) SIR 121969
**          Add E_AD2055_CHECK_DIGIT_SCHEME, E_AD2056_CHECK_DIGIT_STRING
**          and E_AD2057_CHECK_DIGIT_EMPTY
**          and E_AD2058_CHECK_DIGIT_STRING_LENGTH
**          for the generation and validation of checksum values.
**	24-Aug-2009 (kschendel/stephenb) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**	    Prototype various functions that are called outside ADF.
**	9-sep-2009 (stephenb)
**	    define E_AD2107_LASTID_FCN_ERR
**      26-oct-2009 (joea)
**          Add E_AD106A_INV_STR_FOR_BOOL_CAST, E_AD106B_INV_INT_FOR_BOOL_CAST.
**	16-Nov-2009 (kschendel) SIR 122882
**	    Move adu_cmptversion prototype here from aduint.
**	8-Dec-2009 (drewsturgiss)
**		Added ADI_F32768_IFNULL operator to show that a function was like
**		ifnull
**       8-Mar-2010 (hanal04) Bug 122974
**          Removed nfc_flag from adu_nvchr_fromutf8() and adu_unorm().
**          A standard interface is expected by fcn lookup / execute
**          operations. Force NFC normalization is now achieved by temporarily
**          updating the adf_uninorm_flag in the ADF_CB.
**	11-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Add E_AD7016_ADP_MVCC_INCOMPAT
**      12-mar-2010 (joea)
**          Add AD_BOOLEAN_PROTO define for adf_proto_level in ADF_CB.
**	18-Mar-2010 (kiria01) b123438
**	    Added SINGLETON aggregate error E_AD1028_NOT_ZEROONE_ROWS
**	30-Mar-2010 (kschendel) SIR 123485
**	    Update adu-valuetomystr prototype.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	14-Apr-2010 (kschendel) SIR 123485
**	    Define ADF_ISNULL_MACRO here instead of adfint.h because it's
**	    very useful (i.e. a replacement for adc_isnull, which does
**	    the same thing, but slowly).
**	21-apr-2010 (toumi01) SIR 122403
**	    Add ADI_O1AES for encryption.
**      08-Jun-2010 (horda03)
**          When xDEBUG defined build fails due to : at end of prototype
**          definition. And adu_unorm interface wrong.
**      24-Jun-2010 (horda03) B123987
**          Add adf_misc_flags to so that DATE->STRING conversion of
**          greater than 25 chars can be signalled (used for Ascii
**          copy into).
**      01-Jul-2010 (horda03) B123234
**          Added new error E_AD5079_INTERVAL_IN_ABS_FUNC.
*/

#ifndef ADF_HDR_INCLUDED
#define ADF_HDR_INCLUDED


/*
**  Defines of other constants.
*/

#define                 ADF_MAXNAME     DB_TYPE_MAXLEN
#define			U_BLANK 	((UCS2) 0x0020)	/* unicode blank character */ 
#define			U_NULL  	((UCS2) 0x0000) /* unicode null character */
#define			U_TAB   	((UCS2) 0x0009) /* unicode TAB character */
#define			U_LINEFEED  	((UCS2) 0x000A) /* unicode LF character */
#define			U_FORMFEED  	((UCS2) 0x000C) /* unicode FF character */
#define			U_RETURN  	((UCS2) 0x000D) /* unicode CR character */
#define			U_CAPITAL_SIGMA ((UCS2) 0x03A3) /* unicode Greek capital sigma */
#define			U_SMALL_FINAL_SIGMA  	((UCS2) 0x03C2) /* unicode Greek small final sigma */
#define			U_SMALL_SIGMA  	((UCS2) 0x03C3) /* unicode Greek small sigma */
#define                 ADF_UNI_TMPLEN          4004              /* Length of temp buffer used for */
#define                 ADF_UNI_TMPLEN_UCS2     4004/sizeof(UCS2) /* Length of temp buffer used for 
                                                                   ** Unicode operations within adf. */

#define			ADF_UNI_TMPLEN		4004		  /* Length of temp buffer used for */
#define			ADF_UNI_TMPLEN_UCS2	4004/sizeof(UCS2) /* Length of temp buffer used for 
								  ** Unicode operations within adf. */	


/* This was changed to eliminate the dependency on DB_MAXNAME */

#define		ADF_NVL_BIT	((u_char) 0x01)	/* The null value bit.  If    */
						/* this bit is set in the     */
						/* null value byte, then the  */
						/* value of the piece of data */
						/* is null.  If not set, the  */
						/* data should be interpreted */
						/* as usual based on datatype */
						/* and length.                */

/* This macro tells if the data value pointed to by DB_DATA_VALUE v is NULL.
** If it is, the macro evaluates to TRUE, otherwise FALSE.  NULL is data
** type independent (other than nullable types being negative).
*/

#define  ADF_ISNULL_MACRO(v)  ( ((v)->db_datatype < 0 && (*(((u_char *)(v)->db_data) + (v)->db_length - 1) & ADF_NVL_BIT) ? TRUE : FALSE))


/*: ADF Error Codes
**      These are all of the error codes that can possibly be
**      returned by any of the ADF routines:
*/

#define                 E_AD_MASK  			( DB_ADF_ID << 16 )
#define                 E_AD0000_OK			      E_DB_OK
#define                 E_AD0001_EX_IGN_CONT		(E_AD_MASK + 0x0001L)
#define			E_AD0002_INCOMPLETE		(E_AD_MASK + 0x0002L)
	/*
	**  INFO message stating that only a portion of some peripheral datatype
	**  was processed.  The caller should either dispose of or obtain more
	**  of the required information and call ADF again, with the excb's
	**  excb_continuation field remaining untouched.
	**
	**  The caller should know the correct action (dispose of or obtain)
	**  from its own context.
	*/
    /*  Numbers 100 - 0FFF have an error severity level of WARNING */
#define			E_AD0100_NOPRINT_CHAR		(E_AD_MASK + 0x0100L)
#define                 E_AD0101_EMBEDDED_CHAR_TRUNC	(E_AD_MASK + 0x0101L)
#define                 E_AD0102_NULL_IN_TEXT		(E_AD_MASK + 0x0102L)
#define                 E_AD0103_UNINORM_IGN_TRUNC	(E_AD_MASK + 0x0103L)
#define                 E_AD0115_EX_WRN_CONT		(E_AD_MASK + 0x0115L)
#define                 E_AD0116_EX_UNKNOWN		(E_AD_MASK + 0x0116L)
    /*  The following 3 codes are returned by the adcisminmax() function and
    **  are always returned along with a status of E_DB_WARN.
    */
#define			E_AD0117_IS_MINIMUM		(E_AD_MASK + 0x0117L)
#define			E_AD0118_IS_MAXIMUM		(E_AD_MASK + 0x0118L)
#define			E_AD0119_IS_NULL		(E_AD_MASK + 0x0119L)
    /*  Numbers 120 - 170 are reserved for numbers associated with exception
    **  generated ADF warning states.
    */
#define			E_AD0120_INTDIV_WARN		(E_AD_MASK + 0x0120L)
#define			E_AD0121_INTOVF_WARN		(E_AD_MASK + 0x0121L)
#define			E_AD0122_FLTDIV_WARN		(E_AD_MASK + 0x0122L)
#define			E_AD0123_FLTOVF_WARN		(E_AD_MASK + 0x0123L)
#define			E_AD0124_FLTUND_WARN		(E_AD_MASK + 0x0124L)
#define			E_AD0125_MNYDIV_WARN		(E_AD_MASK + 0x0125L)
#define			E_AD0126_DECDIV_WARN		(E_AD_MASK + 0x0126L)
#define			E_AD0127_DECOVF_WARN		(E_AD_MASK + 0x0127L)

#define			E_AD0500_ABS_DATE_IN_AG		(E_AD_MASK + 0x0500L)
#define			E_AD0FFF_NOT_IMPLEMENTED_YET    (E_AD_MASK + 0x0FFFL)
    /*  Numbers 1000 - 8FFF have an error severity level of ERROR */
#define                 E_AD1001_BAD_DATE		(E_AD_MASK + 0x1001L)
#define                 E_AD1002_BAD_MONY_SIGN		(E_AD_MASK + 0x1002L)
#define                 E_AD1003_BAD_MONY_LORT		(E_AD_MASK + 0x1003L)
#define                 E_AD1004_BAD_MONY_PREC		(E_AD_MASK + 0x1004L)
#define                 E_AD1005_BAD_DECIMAL		(E_AD_MASK + 0x1005L)
#define                 E_AD1006_BAD_OUTARG_VAL		(E_AD_MASK + 0x1006L)
#define                 E_AD1007_BAD_QLANG		(E_AD_MASK + 0x1007L)
#define                 E_AD1008_BAD_MATHEX_OPT		(E_AD_MASK + 0x1008L)
#define                 E_AD1009_BAD_SRVCB		(E_AD_MASK + 0x1009L)
#define                 E_AD100A_BAD_NULLSTR		(E_AD_MASK + 0x100AL)
#define                 E_AD100B_BAD_MAXSTRING		(E_AD_MASK + 0x100BL)
#define                 E_AD1010_BAD_EMBEDDED_TYPE	(E_AD_MASK + 0x1010L)
#define                 E_AD1011_BAD_EMBEDDED_LEN	(E_AD_MASK + 0x1011L)
#define                 E_AD1012_NULL_TO_NONNULL 	(E_AD_MASK + 0x1012L)
#define                 E_AD1013_EMBEDDED_NUMOVF	(E_AD_MASK + 0x1013L)
#define                 E_AD1014_BAD_VALUE_FOR_DT	(E_AD_MASK + 0x1014L)
#define                 E_AD1015_BAD_RANGE       	(E_AD_MASK + 0x1015L)
#define                 E_AD1016_PMCHARS_IN_RANGE	(E_AD_MASK + 0x1016L)
#define                 E_AD1017_ESC_AT_ENDSTR		(E_AD_MASK + 0x1017L)
#define                 E_AD1018_BAD_ESC_SEQ		(E_AD_MASK + 0x1018L)
#define                 E_AD1019_NULL_FEXI_PTR          (E_AD_MASK + 0x1019L)
#define                 E_AD101A_DT_NOT_SUPPORTED       (E_AD_MASK + 0x101AL)
#define                 E_AD101B_BAD_FLT_VALUE		(E_AD_MASK + 0x101BL)
#define                 E_AD101C_BAD_ESC_SEQ		(E_AD_MASK + 0x101CL)
#define                 E_AD101D_BAD_ESC_SEQ		(E_AD_MASK + 0x101DL)
#define                 E_AD101E_ESC_SEQ_CONFLICT	(E_AD_MASK + 0x101EL)
#define                 E_AD101F_REG_EXPR_SYN		(E_AD_MASK + 0x101FL)
#define                 E_AD1020_BAD_ERROR_LOOKUP	(E_AD_MASK + 0x1020L)
#define                 E_AD1021_BAD_ERROR_PARAMS	(E_AD_MASK + 0x1021L)
#define			E_AD1025_BAD_ERROR_NUM		(E_AD_MASK + 0x1025L)
#define			E_AD1026_PAT_TOO_CPLX		(E_AD_MASK + 0x1026L)
#define			E_AD1027_PAT_LEN_LIMIT		(E_AD_MASK + 0x1027L)
#define			E_AD1028_NOT_ZEROONE_ROWS	(E_AD_MASK + 0x1028L)
#define			E_AD1030_F_COPY_STR_TOOSHORT	(E_AD_MASK + 0x1030L)
#define			E_AD1040_CV_DVBUF_TOOSHORT	(E_AD_MASK + 0x1040L)
#define			E_AD1041_CV_NETBUF_TOOSHORT	(E_AD_MASK + 0x1041L)
#define			E_AD1050_NULL_HISTOGRAM    	(E_AD_MASK + 0x1050L)
#define			E_AD1060_TM_BUF_TOO_SMALL  	(E_AD_MASK + 0x1060L)
#define			E_AD1061_BAD_SECURITY_LABEL	(E_AD_MASK + 0x1061L)
#define			E_AD1062_SECURITY_LABEL_LUB	(E_AD_MASK + 0x1062L)
#define			E_AD1063_SECURITY_LABEL_EXTERN	(E_AD_MASK + 0x1063L)
#define			E_AD1064_SECURITY_LABEL_EXTLEN	(E_AD_MASK + 0x1064L)
#define			E_AD1065_SESSION_LABEL  	(E_AD_MASK + 0x1065L)
#define			E_AD1066_SOP_NOT_AVAILABLE  	(E_AD_MASK + 0x1066L)
#define			E_AD1067_SOP_FIND  		(E_AD_MASK + 0x1067L)
#define			E_AD1068_SOP_COMPARE  		(E_AD_MASK + 0x1068L)
#define			E_AD1069_SOP_COLLATE  		(E_AD_MASK + 0x1069L)
#define                 E_AD106A_INV_STR_FOR_BOOL_CAST  (E_AD_MASK + 0x106AL)
#define                 E_AD106B_INV_INT_FOR_BOOL_CAST  (E_AD_MASK + 0x106BL)
#define			E_AD1070_BIT_TO_STR_OVFL	(E_AD_MASK + 0x1070L)
#define			E_AD1071_NOT_BIT		(E_AD_MASK + 0x1071L)
#define			E_AD1072_STR_TO_BIT_OVFL	(E_AD_MASK + 0x1072L)
#define                 E_AD1073_SOP_MAC_READ           (E_AD_MASK + 0x1073L)
#define                 E_AD1074_SOP_NOT_ALLOWED        (E_AD_MASK + 0x1074L)
#define                 E_AD1080_STR_TO_DECIMAL         (E_AD_MASK + 0x1080L)
#define                 E_AD1081_STR_TO_FLOAT           (E_AD_MASK + 0x1081L)
#define                 E_AD1082_STR_TRUNCATE           (E_AD_MASK + 0x1082L)
#define                 E_AD1083_UNTERM_C_STR           (E_AD_MASK + 0x1083L)
#define                 E_AD1090_BAD_DTDEF              (E_AD_MASK + 0x1090L)
#define			E_AD1095_BAD_INT_SECLABEL	(E_AD_MASK + 0x1095L)
#define			E_AD1096_SESSION_PRIV		(E_AD_MASK + 0x1096L)
#define			E_AD1097_BAD_DT_FOR_SESSPRIV	(E_AD_MASK + 0x1097L)
#define			E_AD1098_BAD_LEN_SEC_INTRNL	(E_AD_MASK + 0x1098L)
#define			E_AD1120_INTDIV_ERROR		(E_AD_MASK + 0x1120L)
#define			E_AD1121_INTOVF_ERROR		(E_AD_MASK + 0x1121L)
#define			E_AD1122_FLTDIV_ERROR		(E_AD_MASK + 0x1122L)
#define			E_AD1123_FLTOVF_ERROR		(E_AD_MASK + 0x1123L)
#define			E_AD1124_FLTUND_ERROR		(E_AD_MASK + 0x1124L)
#define			E_AD1125_MNYDIV_ERROR		(E_AD_MASK + 0x1125L)
#define			E_AD1126_DECDIV_ERROR		(E_AD_MASK + 0x1126L)
#define			E_AD1127_DECOVF_ERROR		(E_AD_MASK + 0x1127L)
#define                 E_AD2001_BAD_OPNAME		(E_AD_MASK + 0x2001L)
#define                 E_AD2002_BAD_OPID		(E_AD_MASK + 0x2002L)
#define                 E_AD2003_BAD_DTNAME		(E_AD_MASK + 0x2003L)
#define                 E_AD2004_BAD_DTID		(E_AD_MASK + 0x2004L)
#define                 E_AD2005_BAD_DTLEN		(E_AD_MASK + 0x2005L)
#define                 E_AD2006_BAD_DTUSRLEN		(E_AD_MASK + 0x2006L)
#define                 E_AD2007_DT_IS_FIXLEN		(E_AD_MASK + 0x2007L)
#define                 E_AD2008_DT_IS_VARLEN		(E_AD_MASK + 0x2008L)
#define                 E_AD2009_NOCOERCION		(E_AD_MASK + 0x2009L)
#define                 E_AD200A_NOCOPYCOERCION		(E_AD_MASK + 0x200AL)
#define                 E_AD200B_BAD_PREC		(E_AD_MASK + 0x200BL)
#define                 E_AD200C_BAD_SCALE		(E_AD_MASK + 0x200CL)
#define                 E_AD200D_BAD_BASE_DTID		(E_AD_MASK + 0x200DL)
#define                 E_AD2010_BAD_FIID		(E_AD_MASK + 0x2010L)
#define                 E_AD2020_BAD_LENSPEC		(E_AD_MASK + 0x2020L)
#define                 E_AD2021_BAD_DT_FOR_PRINT	(E_AD_MASK + 0x2021L)
#define                 E_AD2022_UNKNOWN_LEN		(E_AD_MASK + 0x2022L)
#define                 E_AD2030_LIKE_ONLY_FOR_SQL	(E_AD_MASK + 0x2030L)
#define                 E_AD2040_INCONSISTENT_TPL_CNT	(E_AD_MASK + 0x2040L)
#define                 E_AD2041_TPL_ARRAY_TOO_SMALL	(E_AD_MASK + 0x2041L)
#define                 E_AD2042_MEMALLOC_FAIL		(E_AD_MASK + 0x2042L)
#define                 E_AD2043_ATOMIC_TPL		(E_AD_MASK + 0x2043L)
#define                 E_AD2050_NO_COMVEC_FUNC		(E_AD_MASK + 0x2050L)
#define                 E_AD2051_BAD_ANSITRIM_LEN       (E_AD_MASK + 0x2051L)
/* The following are used by generate_digit() and validate_digit() */
#define			E_AD2055_CHECK_DIGIT_SCHEME	(E_AD_MASK + 0x2055L)
#define			E_AD2056_CHECK_DIGIT_STRING	(E_AD_MASK + 0x2056L)
#define			E_AD2057_CHECK_DIGIT_EMPTY	(E_AD_MASK + 0x2057L)
#define			E_AD2058_CHECK_DIGIT_STRING_LENGTH	(E_AD_MASK + 0x2058L)
/* The following are used by adi_resolve() */
#define			E_AD2060_BAD_DT_COUNT		(E_AD_MASK + 0x2060L)
#define			E_AD2061_BAD_OP_COUNT		(E_AD_MASK + 0x2061L)
#define			E_AD2062_NO_FUNC_FOUND		(E_AD_MASK + 0x2062L)
#define			E_AD2063_FUNC_AMBIGUOUS		(E_AD_MASK + 0x2063L)

/* The following are used by adi_encode_colspec() */
#define			E_AD2070_NO_EMBED_ALLOWED	(E_AD_MASK + 0x2070L)
#define			E_AD2071_NO_PAREN_ALLOWED	(E_AD_MASK + 0x2071L)
#define			E_AD2072_NO_PAREN_AND_EMBED	(E_AD_MASK + 0x2072L)
#define			E_AD2073_BAD_USER_PREC  	(E_AD_MASK + 0x2073L)
#define			E_AD2074_BAD_USER_SCALE		(E_AD_MASK + 0x2074L)

#define			E_AD2080_IFTRUE_NEEDS_INT	(E_AD_MASK + 0x2080L)
#define			E_AD2085_LOCATE_NEEDS_STR	(E_AD_MASK + 0x2085L)
#define			E_AD2090_BAD_DT_FOR_STRFUNC     (E_AD_MASK + 0x2090L)
#define			E_AD2092_BAD_START_FOR_SUBSTR	(E_AD_MASK + 0x2092L)
#define			E_AD2093_BAD_LEN_FOR_SUBSTR	(E_AD_MASK + 0x2093L)
#define			E_AD2094_INVALID_LOCATOR	(E_AD_MASK + 0x2094L)

#define			E_AD20A0_BAD_LEN_FOR_PAD	(E_AD_MASK + 0x20A0L)

#define			E_AD20B0_REPLACE_NEEDS_STR	(E_AD_MASK + 0x20B0L)

#define			E_AD2100_NULL_RESTAB_PTR	(E_AD_MASK + 0x2100L)
#define			E_AD2101_RESTAB_FCN_ERROR	(E_AD_MASK + 0x2101L)
#define			E_AD2102_NULL_ALLOCATED_PTR	(E_AD_MASK + 0x2102L)
#define			E_AD2103_ALLOCATED_FCN_ERR	(E_AD_MASK + 0x2103L)
#define			E_AD2104_NULL_OVERFLOW_PTR	(E_AD_MASK + 0x2104L)
#define			E_AD2105_OVERFLOW_FCN_ERR	(E_AD_MASK + 0x2105L)
#define			E_AD2106_TABLEINFO_FCN_ERR	(E_AD_MASK + 0x2106L)
#define			E_AD2107_LASTID_FCN_ERR		(E_AD_MASK + 0x2107L)

#define                 E_AD3001_DTS_NOT_SAME		(E_AD_MASK + 0x3001L)
#define                 E_AD3002_BAD_KEYOP		(E_AD_MASK + 0x3002L)
#define                 E_AD3003_DLS_NOT_SAME		(E_AD_MASK + 0x3003L)
#define                 E_AD3004_ILLEGAL_CONVERSION	(E_AD_MASK + 0x3004L)
#define                 E_AD3005_BAD_EQ_DTID		(E_AD_MASK + 0x3005L)
#define                 E_AD3006_BAD_EQ_DTLEN		(E_AD_MASK + 0x3006L)
#define                 E_AD3007_BAD_DS_DTID		(E_AD_MASK + 0x3007L)
#define                 E_AD3008_BAD_DS_DTLEN		(E_AD_MASK + 0x3008L)
#define                 E_AD3009_BAD_HP_DTID		(E_AD_MASK + 0x3009L)
#define                 E_AD3010_BAD_HP_DTLEN		(E_AD_MASK + 0x3010L)
#define                 E_AD3011_BAD_HG_DTID		(E_AD_MASK + 0x3011L)
#define                 E_AD3012_BAD_HG_DTLEN		(E_AD_MASK + 0x3012L)
#define                 E_AD3013_BAD_HG_DTPREC		(E_AD_MASK + 0x3013L)
#define                 E_AD3050_BAD_CHAR_IN_STRING	(E_AD_MASK + 0x3050L)
#define                 E_AD4001_FIID_IS_AG		(E_AD_MASK + 0x4001L)
#define                 E_AD4002_FIID_IS_NOT_AG		(E_AD_MASK + 0x4002L)
#define                 E_AD4003_AG_WORKSPACE_TOO_SHORT	(E_AD_MASK + 0x4003L)
#define                 E_AD4004_BAD_AG_DTID		(E_AD_MASK + 0x4004L)
#define                 E_AD4005_NEG_AG_COUNT		(E_AD_MASK + 0x4005L)
#define			E_AD5001_BAD_STRING_TYPE	(E_AD_MASK + 0x5001L)
#define			E_AD5002_BAD_NUMBER_TYPE	(E_AD_MASK + 0x5002L)
#define			E_AD5003_BAD_CVTONUM		(E_AD_MASK + 0x5003L)
#define			E_AD5004_OVER_MAXTUP		(E_AD_MASK + 0x5004L)
#define			E_AD5005_BAD_DI_FILENAME	(E_AD_MASK + 0x5005L)
#define			E_AD5006_BAD_STR_TO_DEC		(E_AD_MASK + 0x5006L)
#define			E_AD5007_BAD_HEX_CHAR		(E_AD_MASK + 0x5007L)
#define                 E_AD5008_BAD_IP_ADDRESS         (E_AD_MASK + 0x5008L)
#define                 E_AD5009_BAD_RESULT_STRING_TYPE (E_AD_MASK + 0x5009L)
#define                 E_AD5010_SOURCE_STRING_TRUNC    (E_AD_MASK + 0x5010L)
#define                 E_AD5011_NOCHARMAP_FOUND	(E_AD_MASK + 0x5011L)
#define                 E_AD5012_UCETAB_NOT_EXISTS	(E_AD_MASK + 0x5012L)
#define                 E_AD5015_INVALID_BYTESEQ	(E_AD_MASK + 0x5015L)
#define                 E_AD5016_NOUNIMAP_FOUND		(E_AD_MASK + 0x5016L)
#define                 E_AD5017_NOMAPTBL_FOUND		(E_AD_MASK + 0x5017L)
#define			E_AD5019_UDECOMP_COL		(E_AD_MASK + 0x5019L)
    /* Numbers 5020 thru 503F are associated with money function instances */
#define			E_AD5020_BADCH_MNY		(E_AD_MASK + 0x5020L)
#define			E_AD5021_MNY_SIGN		(E_AD_MASK + 0x5021L)
#define			E_AD5022_DECPT_MNY		(E_AD_MASK + 0x5022L)
#define			E_AD5031_MAXMNY_OVFL		(E_AD_MASK + 0x5031L)
#define			E_AD5032_MINMNY_OVFL		(E_AD_MASK + 0x5032L)
    /* Numbers 5050 thru 507F are associated with date function instances */
#define			E_AD5050_DATEADD		(E_AD_MASK + 0x5050L)
#define			E_AD5051_DATESUB		(E_AD_MASK + 0x5051L)
#define			E_AD5052_DATEVALID		(E_AD_MASK + 0x5052L)
#define			E_AD5053_DATEYEAR		(E_AD_MASK + 0x5053L)
#define			E_AD5054_DATEMONTH		(E_AD_MASK + 0x5054L)
#define			E_AD5055_DATEDAY		(E_AD_MASK + 0x5055L)
#define			E_AD5056_DATETIME		(E_AD_MASK + 0x5056L)
#define			E_AD5058_DATEBADCHAR		(E_AD_MASK + 0x5058L)
#define			E_AD5059_DATEAMPM		(E_AD_MASK + 0x5059L)
#define			E_AD505A_DATEYROVFLO		(E_AD_MASK + 0x505AL)
#define			E_AD505B_DATEYR			(E_AD_MASK + 0x505BL)
#define			E_AD505C_DOWINVALID		(E_AD_MASK + 0x505CL)
    /*  An absolute date was expected */
#define			E_AD505D_DATEABS		(E_AD_MASK + 0x505DL)
    /*  A date interval was expected */
#define			E_AD505E_NOABSDATES		(E_AD_MASK + 0x505EL)
    /*  Interval spec not valid */
#define			E_AD505F_DATEINTERVAL		(E_AD_MASK + 0x505FL)
#define			E_AD5060_DATEFMT		(E_AD_MASK + 0x5060L)
#define			E_AD5061_DGMT_ON_INTERVAL	(E_AD_MASK + 0x5061L)
#define			E_AD5062_DATEOVFLO		(E_AD_MASK + 0x5062L)
#define			E_AD5063_BAD_SYSTIME		(E_AD_MASK + 0x5063L)
#define			E_AD5064_DATEANSI		(E_AD_MASK + 0x5064L)
#define			E_AD5065_DATE_ALIAS_NOTSET	(E_AD_MASK + 0x5065L)
#define			E_AD5066_DATE_COERCION		(E_AD_MASK + 0x5066L)
#define			E_AD5067_ANSIDATE_4DIGYR	(E_AD_MASK + 0x5067L)
#define			E_AD5068_ANSIDATE_DATESUB	(E_AD_MASK + 0x5068L)
#define			E_AD5069_ANSIDATE_DATEADD	(E_AD_MASK + 0x5069L)
#define			E_AD5070_ANSIDATE_BADFMT	(E_AD_MASK + 0x5070L)
#define			E_AD5071_ANSITMWO_BADFMT	(E_AD_MASK + 0x5071L)
#define			E_AD5072_ANSITMW_BADFMT		(E_AD_MASK + 0x5072L)
#define			E_AD5073_ANSITSWO_BADFMT	(E_AD_MASK + 0x5073L)
#define			E_AD5074_ANSITSW_BADFMT		(E_AD_MASK + 0x5074L)
#define			E_AD5075_ANSIINYM_BADFMT	(E_AD_MASK + 0x5075L)
#define			E_AD5076_ANSIINDS_BADFMT	(E_AD_MASK + 0x5076L)
#define			E_AD5077_ANSITMZONE_BADFMT	(E_AD_MASK + 0x5077L)
#define			E_AD5078_ANSITIME_INTVLADD	(E_AD_MASK + 0x5078L)
#define			E_AD5079_INTERVAL_IN_ABS_FUNC   (E_AD_MASK + 0x5079L)
    /* Numbers 5080 thru 508F are associated with log key function instances */
#define			E_AD5080_LOGKEY_BAD_CVT_LEN	(E_AD_MASK + 0x5080L)
#define			E_AD5081_UNICODE_FUNC_PARM 	(E_AD_MASK + 0x5081L)
#define			E_AD5082_NON_UNICODE_DB 	(E_AD_MASK + 0x5082L)
    /* Numbers 5090 thru 509F are further date related errors */
#define			E_AD5090_DATE_MUL		(E_AD_MASK + 0x5090L)
#define			E_AD5091_DATE_DIV		(E_AD_MASK + 0x5091L)
#define			E_AD5092_DATE_DIV_BY_ZERO	(E_AD_MASK + 0x5092L)
#define			E_AD5093_DATE_DIV_ANSI_INTV	(E_AD_MASK + 0x5093L)
#define			E_AD5094_DATE_ARITH_NOABS	(E_AD_MASK + 0x5094L)
    /* Numbers 50A0 thru 50AF are associated with UUID f.i.'s */
#define			E_AD50A0_BAD_UUID_PARM		(E_AD_MASK + 0x50A0L)
    /* The 55xx series is reserved for the ADE module */
#define			E_AD5500_BAD_SEG                (E_AD_MASK + 0x5500L)
#define			E_AD5501_BAD_SEG_FOR_ICODE      (E_AD_MASK + 0x5501L)
#define			E_AD5502_WRONG_NUM_OPRS         (E_AD_MASK + 0x5502L)
#define			E_AD5503_BAD_DTID_FOR_FIID      (E_AD_MASK + 0x5503L)
#define			E_AD5504_BAD_RESULT_LEN         (E_AD_MASK + 0x5504L)
#define			E_AD5505_UNALIGNED              (E_AD_MASK + 0x5505L)
#define			E_AD5506_NO_SPACE               (E_AD_MASK + 0x5506L)
#define			E_AD5507_BAD_DTID_FOR_KEYBLD    (E_AD_MASK + 0x5507L)
#define			E_AD5508_BAD_DTLEN_FOR_KEYBLD   (E_AD_MASK + 0x5508L)
#define			E_AD5509_BAD_RANGEKEY_FLAG      (E_AD_MASK + 0x5509L)
#define			E_AD550A_RANGE_FAILURE          (E_AD_MASK + 0x550AL)
#define			E_AD550B_TOO_FEW_BASES          (E_AD_MASK + 0x550BL)
#define			E_AD550C_COMP_NOT_IN_PROG       (E_AD_MASK + 0x550CL)
#define			E_AD550D_WRONG_CX_VERSION       (E_AD_MASK + 0x550DL)
#define			E_AD550E_TOO_MANY_VLTS          (E_AD_MASK + 0x550EL)
#define			E_AD550F_BAD_CX_REQUEST         (E_AD_MASK + 0x550FL)
#define                 E_AD5510_BAD_DTID_FOR_ESCAPE	(E_AD_MASK + 0x5510L)
#define                 E_AD5511_BAD_DTLEN_FOR_ESCAPE	(E_AD_MASK + 0x5511L)
#define                 E_AD5512_BAD_DTID2_FOR_ESCAPE	(E_AD_MASK + 0x5512L)
#define                 E_AD5513_BAD_DTLEN2_FOR_ESCAPE	(E_AD_MASK + 0x5513L)

#define			E_AD6000_BAD_MATH_ARG		(E_AD_MASK + 0x6000L)
#define			E_AD6001_BAD_MATHOPT		(E_AD_MASK + 0x6001L)

/*
**  Errors in the 7xxx range refer to peripheral datatypes
*/
#define			E_AD7000_ADP_BAD_INFO		(E_AD_MASK + 0x7000L)
#define			E_AD7001_ADP_NONEXT		(E_AD_MASK + 0x7001L)
#define			E_AD7002_ADP_BAD_GET		(E_AD_MASK + 0x7002L)
#define			E_AD7003_ADP_BAD_RECON		(E_AD_MASK + 0x7003L)
#define			E_AD7004_ADP_BAD_BLOB		(E_AD_MASK + 0x7004L)
#define			E_AD7005_ADP_BAD_POP		(E_AD_MASK + 0x7005L)
#define			E_AD7006_ADP_PUT_ERROR		(E_AD_MASK + 0x7006L)
#define			E_AD7007_ADP_GET_ERROR		(E_AD_MASK + 0x7007L)
#define			E_AD7008_ADP_DELETE_ERROR	(E_AD_MASK + 0x7008L)
#define			E_AD7009_ADP_MOVE_ERROR		(E_AD_MASK + 0x7009L)
#define			E_AD700A_ADP_REPLACE_ERROR	(E_AD_MASK + 0x700AL)
#define			E_AD700B_ADP_BAD_POP_CB		(E_AD_MASK + 0x700BL)
#define			E_AD700C_ADP_BAD_POP_UA		(E_AD_MASK + 0x700CL)
#define			E_AD700D_ADP_NO_COUPON		(E_AD_MASK + 0x700DL)
#define			E_AD700E_ADP_BAD_COUPON		(E_AD_MASK + 0x700EL)
#define                 E_AD700F_ADP_BAD_CPNIFY		(E_AD_MASK + 0x700FL)
#define                 E_AD7010_ADP_USER_INTR          (E_AD_MASK + 0x7010L)
#define                 E_AD7011_ADP_BAD_CPNIFY_OVER    (E_AD_MASK + 0x7011L)
#define                 E_AD7012_ADP_WORKSPACE_TOOLONG  (E_AD_MASK + 0x7012L)
#define			E_AD7013_ADP_CPN_TO_LOCATOR	(E_AD_MASK + 0x7013L)
#define			E_AD7014_ADP_LOCATOR_TO_CPN	(E_AD_MASK + 0x7014L)
#define			E_AD7015_ADP_FREE_LOCATOR	(E_AD_MASK + 0x7015L)
#define			E_AD7016_ADP_MVCC_INCOMPAT	(E_AD_MASK + 0x7016L)

/* Errors in the 8000 - 8400 reserved for user defined error processing	    */
#define			E_AD8000_OP_COUNT_SPEC		(E_AD_MASK + 0x8000L)
#define			E_AD8001_OBJECT_TYPE		(E_AD_MASK + 0x8001L)
#define			E_AD8002_OP_OPID		(E_AD_MASK + 0x8002L)
#define			E_AD8003_OP_OPTYPE		(E_AD_MASK + 0x8003L)
#define			E_AD8004_OP_COUNT_WRONG		(E_AD_MASK + 0x8004L)
#define			E_AD8005_DT_COUNT_SPEC		(E_AD_MASK + 0x8005L)
#define			E_AD8006_DT_DATATYPE_ID		(E_AD_MASK + 0x8006L)
#define			E_AD8007_DT_FUNCTION_ADDRESS	(E_AD_MASK + 0x8007L)
#define			E_AD8008_DT_COUNT_WRONG		(E_AD_MASK + 0x8008L)
#define			E_AD8009_FI_SORT_ORDER		(E_AD_MASK + 0x8009L)
#define			E_AD800A_FI_COUNT_SPEC		(E_AD_MASK + 0x800AL)
#define			E_AD800B_FI_RLTYPE		(E_AD_MASK + 0x800BL)
#define			E_AD800C_FI_INSTANCE_ID		(E_AD_MASK + 0x800CL)
#define			E_AD800D_FI_OPTYPE		(E_AD_MASK + 0x800DL)
#define			E_AD800E_FI_NUMARGS		(E_AD_MASK + 0x800EL)
#define			E_AD800F_FI_NO_COMPLEMENT	(E_AD_MASK + 0x800FL)
#define			E_AD8010_FI_ARG_SPEC		(E_AD_MASK + 0x8010L)
#define			E_AD8011_FI_DUPLICATE		(E_AD_MASK + 0x8011L)
#define			E_AD8012_UD_ADDRESS		(E_AD_MASK + 0x8012L)
#define			E_AD8013_FI_ROUTINE		(E_AD_MASK + 0x8013L)
#define			E_AD8014_DT_NAME		(E_AD_MASK + 0x8014L)
#define			E_AD8015_OP_NAME		(E_AD_MASK + 0x8015L)
#define			E_AD8016_OP_DUPLICATE		(E_AD_MASK + 0x8016L)
#define                 E_AD8017_INVALID_RESULT_TYPE    (E_AD_MASK + 0x8017L)
#define			E_AD8018_INVALID_PARAM_TYPE	(E_AD_MASK + 0x8018L)
#define                 E_AD8019_OPTYPE_MISMATCH        (E_AD_MASK + 0x8019L)
    /*  The function is not implemented */
#define			E_AD8999_FUNC_NOT_IMPLEMENTED   (E_AD_MASK + 0x8999L)

    /*  Numbers 9000 - FFFF have an error severity level of FATAL */
#define                 E_AD9000_BAD_PARAM		(E_AD_MASK + 0x9000L)
#define			E_AD9001_BAD_CALL_SEQUENCE	(E_AD_MASK + 0x9001L)
#define			E_AD9002_AUGMENT_SIZE		(E_AD_MASK + 0x9002L)
#define			E_AD9003_TABLE_INIT		(E_AD_MASK + 0x9003L)
#define			E_AD9004_SECURITY_LABEL		(E_AD_MASK + 0x9004L)
#define                 E_AD9998_INTERNAL_ERROR		(E_AD_MASK + 0x9998L)
#define                 E_AD9999_INTERNAL_ERROR		(E_AD_MASK + 0x9999L)

    /*  Mask that is true for all spatial errors in adf/ads/sperr.h */
#define			E_SPATIAL_MASK			(0x300000L)


/*
**	These defines show certain inherent maximums in the ADF structures.
*/

#define                 ADG_SZ_SRVCB	(adg_srv_size())
    /*	The above constant represents the size of ADF's server control block, in
    **  bytes. A piece of memory this big needs to be allocated by the ADF user
    **  before the call to adg_startup(), and a PTR to that memory is then
    **  passed to adg_startup().  That routine will fill this structure in
    **  appropriately; in effect, this contains all ADF global variables that
    **  are not READONLY.  Then, for each session, the ADF user must place the
    **  PTR to this memory into the ADF_CB for the call to adg_init().
    */

#define                 ADI_MXDTS       383	/* Largest legal base      */
						/*    datatype ID.         */

#define                 ADI_LNBM         12	/* # u_i4's needed for a   */
						/* datatype bitmask.  This */
						/* should always contain   */
						/* at least ADI_MXDTS + 1  */
						/* bits.                   */

#ifndef conf_ADI_MAX_OPERANDS
#    define	ADI_MAX_OPERANDS	5	/* Default if no build value */
#else
#    define	ADI_MAX_OPERANDS	conf_ADI_MAX_OPERANDS
#endif
	/* Notes on ADI_MAX_OPERANDS:
	** It is the maximum supported number of parameters to a CX operator.
	** It should include +1 for a result, as the result is passed to
	** a function as one of the operands.
	** It must be at least 5 to accomodate the operator ADE_KEYBLD.
	*/


/*
**	These defines show certain inherent limits associated with output and
**	input formatting.
*/

# define		ADI_OUTMXFIELD	255	/* The maximum width any      */
						/* field in the ADF_OUTARG    */
						/* struct can have.           */


/*
**	Any text line read from a file (for example, .../files/users) can be at
**	most MAXLINE bytes long.  buffers designed for holding such info should
**	be declared as char buf[MAXLINE + 1] to allow for the null terminator.
*/

# define		ADI_INMXLINE     256

/*
**      These defines enumerate the flavours if the ANSI/ISO "trim"
**      function. The Ingres function only supports trailing.
*/

#define                 ADF_TRIM_LEADING 1
#define                 ADF_TRIM_TRAILING 2
#define                 ADF_TRIM_BOTH    3



/*: PM_ENCODE flags
**	These defines represent the option bits that can be used to drive the
**	adi_pm_encode() routine in several different ways.  The options they
**	represent are turned on by setting the appropriate bit in the
**	`adi_pm_reqs' argument to adi_pm_encode().
*/

#define     ADI_DO_PM           0x00000001  /* Encode QUEL pattern match      */
					    /* characters.  This would be     */
					    /* used for strings found in a    */
					    /* QUEL qualification, only.      */

#define     ADI_DO_BACKSLASH	0x00000002  /* Encode backslash sequences.    */
					    /* Sequences such as "\\n" will   */
					    /* be encoded into a single char; */
					    /* a new-line.  This would be     */
					    /* used for strings found in QUEL */
					    /* target lists AND qualifica-    */
					    /* tions, and would never be used */
					    /* for SQL strings.               */
					    
#define     ADI_DO_MOD_LEN	0x00000004  /* Modify the .db_length field of */
					    /* the DB_DATA_VALUE if the number*/
					    /* of characters in the string is */
					    /* reduced due to ADI_DO_PM or    */
					    /* ADI_DO_BACKSLASH.  If this bit */
					    /* is not set, and the number of  */
					    /* characters are reduced, `char' */
					    /* and `c' types will be blank    */
					    /* padded, while `longtext',      */
					    /* `varchar', `text' will just    */
					    /* have their count field reduced */
					    /* appropriately.                 */


/*: KLEN and KCVT flags
**	These defines represent the option bits that can be used to drive the
**	adc_klen() and adc_kcvt() routines for formatting data values into
**	their character string constant formats.  The options they represent
**	are turned on by setting the appropriate bit in the `adc_flags'
**	argument to adc_klen() and adc_kcvt().  Note that some of the flags
**	cannot be used together.
*/

#define     ADC_0001_FMT_SQL    0x00000001  /* Format the data value for SQL. */
					    /* This flag cannot be used in    */
					    /* conjunction with the           */
					    /* ADC_0002_FMT_QUEL flag.        */

#define     ADC_0002_FMT_QUEL   0x00000002  /* Format the data value for QUEL.*/
					    /* This flag cannot be used in    */
					    /* conjunction with the           */
					    /* ADC_0001_FMT_SQL flag.         */
/*:
**   COLUMN SPEC flags
**	These flags are used when calling adi_encode_colspec().
*/

#define	    ADI_F1_WITH_NULL	0x01	    /* Nullability was requested by
					    ** user.
					    */
#define	    ADI_F2_COPY_SPEC	0x02	    /* Use slightly different semantics
					    ** required by COPY command rather
					    ** than normal table creation
					    ** semantics.
					    */
#define	    ADI_F4_CAST		0x04	    /* Set db_length to 0 for length
					    ** qualified types with no 
					    ** explicit length (for CAST)
					    */

/*:  DATATYPE description bits
**      These form a bitmask describing usage of the datatype.
**	Note that these fit in (at present) a nat.
*/
#define                 AD_INTR         0x1 /* Datatype is intrinsic (else  */
					    /* abstract)		    */
#define			AD_INDB		0x2 /* DT is allowed in database    */
#define			AD_USER_DEFINED	0x4 /* DT is user defined	    */
#define			AD_NOEXPORT	0x8 /* DT cannot be exported (sent  */
					    /* to FE).			    */
#define			AD_CONDEXPORT	0x10
					    /* DT can be exported to	    */
					    /* cooperative FE (== one which */
					    /* has extended datatype	    */
					    /* support)			    */
    /*
    ** The following four constants are added for initial support of large
    ** objects.  These datatypes do not have any inherent sort order,
    ** keyability, or histogram support.  For many cases, these three attributes
    ** overlap.  A datatype which is not sortable cannot be keyed on (as DMF
    ** will attempt a sort as part of a modify operation).  However, they are
    ** provided as separate attributes for user defined datatype support.
    ** Furthermore, the converse of the aforementioned implication is not true:
    ** a datatype which is not keyable may very well be sortable.
    **
    ** Very large attributes are also designated as peripheral datatypes.  The
    ** peripheral refers to the fact that they are actually stored outside of
    ** the base table/tuple.  In fact, they will be stored in a set of separate
    ** files which are associated with the base table in magical DMFish ways.
    **
    **	    30-oct-89 (fred) -- Added.
    */
#define			AD_NOKEY	0x20
					    /*
					    ** Datatype cannot be specified as a
					    ** key column in a modify or index
					    ** statement.
					    */
#define			AD_NOSORT	0x40
					    /* Datatype cannot be specified as
					    ** the target of a SORT BY or ORDER
					    ** BY clause in a query.  Also, OPF
					    ** is not permitted to require a
					    ** sort on this datatype for correct
					    ** execution of a query plan.
					    ** Datatypes tagged with this bit
					    ** may have no inherent sort order
					    ** -- they will be simply marked as
					    ** "different" by the sort
					    ** comparison routine.
					    */
#define			AD_NOHISTOGRAM	0x80
					    /* Histograms are not available for
					    ** this datatype.  Furthermore, they
					    ** cannot (and should not) be
					    ** constructed.  OPF is expected to
					    ** "make do" without such data.  (At
					    ** present, OPF will choose to make
					    ** such datatypes "like" some other
					    ** datatype -- and use that
					    ** datatypes histograms.
					    */
#define                 AD_PERIPHERAL   0x100
					    /*
					    ** Datatype is stored outside of
					    ** the basic tuple format.  This
					    ** means that the tuple itself may
					    ** contain either the full data
					    ** element or it may contain a
					    ** `coupon' which may be redeemed
					    ** later to obtain the actual
					    ** datatype.  These coupons will be
					    ** in the form of the ADP_COUPON
					    ** structure described in the file
					    ** adp.h.
					    **
					    ** The data representing a
					    ** peripheral datatype is always
					    ** represented by an ADP_PERIPHERAL
					    ** structure.  This structure
					    ** represents the union of the
					    ** ADP_COUPON structure and a byte
					    ** stream (an array of 1
					    ** byte/char).  This data structure
					    ** also contains a flag indicating
					    ** whether this is the real thing
					    ** or the coupon.
					    */

#define                 AD_VARIABLE_LEN   0x200
					    /*
					    ** Datatype is variable length.
					    ** Data can be written in 
					    ** compressed form requiring
					    ** less space than the full
					    ** value.
					    */

#define			AD_LOCATOR	  0x400 /* Locator datatype */


/*
** These constants define the default precision of decimal numbers which are
** coerced or converted from other numeric types.  `FP' means any "floating
** point" type (float, decimal, or money).
*/
#define		AD_I1_TO_DEC_PREC		    5
#define		AD_I1_TO_DEC_SCALE		    0

#define		AD_I2_TO_DEC_PREC		    5
#define		AD_I2_TO_DEC_SCALE		    0

#define		AD_I4_TO_DEC_PREC		    11
#define		AD_I4_TO_DEC_SCALE		    0

#define		AD_I8_TO_DEC_PREC		    19
#define		AD_I8_TO_DEC_SCALE		    0

#define		AD_FP_TO_DEC_PREC		    15
#define		AD_FP_TO_DEC_SCALE		    0

/*
** Define a floating point fuzz factor to be added to or subtracted from
** money values that are to be rounded to adjust for float imprecision.
*/
#define		AD_MONEY_FUZZ			    1.0e-6

/* */
/* [@group_of_defined_constants@]... */
/* [@global_variable_references@] */


/*
** These constants define slots in the FEXI table which will contain passed in
** addresses to functions needed to implement operations which ADF cannot do
** itself (e.g. resolve_table() needs to access catalogs, so ADF cannot execute
** this function).
**
** The ADI_##XXXX... symbols define slots in the table, and the ADI_SZ_FEXI
** defines the size of the table.  Note the size must be incremented when adding
** a new slot definition (there are no compatibility issues in increasing the
** size of this table, so there is no need for reserved space in it).
*/
#define		ADI_00RESTAB_FEXI		    0
#define		ADI_01PERIPH_FEXI		    1
#define		ADI_02ADF_PRINTF_FEXI		    2
#define		ADI_03ALLOCATED_FEXI		    3
#define		ADI_04OVERFLOW_FEXI		    4
#define		ADI_06TABLEINFO_FEXI		    6
#define		ADI_07LASTID_FEXI		    7

#define		ADI_SZ_FEXI			    8


/*}
** Name: ADF_ERROR - struct to be used in all ADF error processing.
**
** Description:
**	ADF_ERROR defines the structure to be used in all ADF control
**	blocks for returning errors and formatted messages to the caller.
**	ad_errcode will contain the ADF code which identifies the error to the
**	calling facility.  ad_errclass will specify whether this was a user
**	error or some other kind of error.  If an ADF error is a user error,
**	the user error code will be placed into the ad_usererr field.  Always
**	a generic error code will be placed into the ad_generic_error field.
**	User errors are indicated when the ad_errclass is set to ADF_USER_ERROR.
**	ad_ebuflen is sent by the caller to specify how large the buffer
**	pointed to by ad_errmsgp is.  ad_emsglen is sent back to the caller to
**	tell him the length of the resulting formatted error message which was
**	placed in the buffer pointed to by ad_errmsgp.  ad_errmsgp contains a
**	pointer to a buffer where a formatted message can be placed.  If this
**	pointer is NULL, then no message will be formatted.
**
**
** History:
**	17-jun-86 (ericj)
**          Created.
**	11-jul-86 (ericj)
**	    Updated.
**	17-jul-86 (ericj)
**	    Added the ad_usererr field.
**	22-may-89 (jrb)
**	    Added the ad_generic_err field.
**	14-jun-89 (fred)
**	    Added ADF_EXTERNAL_ERROR classification which corresponds to errors
**	    generated by user code called for user defined adt's.  This
**	    classification is used in the calling facilities to set the
**	    appropriate flags in the error message sent to the frontend to avoid
**	    formatting problems with non-standard errors.
**	20-oct-92 (andre)
**	    replaced ad_generic_err with ad_sqlstate
**	18-Sep-2008 (jonj)
**	    SIR 120874: Add ad_dberror, here and in iiadd.h, ADF_CB static
**	    feadfcb.c
*/

/* ** DANGER DANGER the exact format and layout of this structure is
** exposed to object management extension users, as II_ERR_STRUCT.
** Don't change it!
*/

typedef struct _ADF_ERROR
{
    i4	ad_errcode;	    /* The error being returned.  When an
				    ** internal error occurs, this will contain
				    ** an error code that maps to ADF.
				    ** When a user error occurs this should
				    ** contain the appropriate user error
				    ** code which may or may not map to
				    ** ADF.  NOTE, user error codes that
				    ** that existed before Jupiter should
				    ** remain the same for upward compatibility
				    ** of user applications.
				    */
	/* # defines are listed above. */
    i4		ad_errclass;	    /* This field is used to classify what
				    ** kind of ADF error occurred according
				    ** to the following definitions:
				    */
#define                 ADF_INTERNAL_ERROR	1
#define                 ADF_USER_ERROR		2
#define			ADF_EXTERNAL_ERROR	3
    i4	ad_usererr;	    /* The pre-Jupiter user error code that
				    ** corresponds to ad_errcode.
				    */
    DB_SQLSTATE	ad_sqlstate;	    /* The SQLSTATE status code that corresponds
				    ** to ad_errcode.
				    */
    i4		ad_ebuflen;	    /* Length of buffer, in bytes, pointed
				    ** to by ad_errmsgp.  If this is equal
				    ** to zero, then no error message will be
				    ** formatted and returned to the user.
				    */
    i4		ad_emsglen;	    /* Length of a formatted message pointed
				    ** to by ad_errmsgp.
				    */
    char	*ad_errmsgp;	    /* A ptr to a buffer where the error mess-
				    ** age corresponding to ad_errcode can be
				    ** formatted and placed.  If this pointer
				    ** is NULL, then no error message will
				    ** be formatted and returned to user.
				    */
    DB_ERROR	ad_dberror;	    /* Error source info about ad_errcode */
}   ADF_ERROR;


/*}
** Name: ADI_WARN - used in the processing of ADF warnings.
**
** Description:
**        This struct is used in the processing of ADF warnings.  Examples
**	of these warnings include math exceptions like EXFLTOVF, floating
**	point overflow, EXINTDIV, integer division by zero, and EXMNYDIV,
**	money division by zero.  There is one field defined in this struct
**	for each ADF state that can generate a warning message.  Some of
**	these states will be countable over some period.  For example,
**	integer division by zero may occur N times during the execution of
**	a query.  The count N will be kept in the ad_intdiv_cnt field.
**	It will be updated by the adx_handler() routine when the math
**	exception handling option is set to ADX_WRN_MATHEX.  It will be
**	reset at the end of a period (often a query) by adx_chkwarn().
**
** History:
**	07-aug-86 (ericj)
**	    Added ADI_WARN struct.  This will be used to count various
**	    ADF warning state occurences.  It will be used in the
**	    processing of exceptions by adx_handler() and by adx_chkwarn()
**	    to check for and process ADF warnings.
**      13-feb-87 (thurston)
**          Added the ad_noprint_cnt field, along with a couple reserved.
**      29-jul-87 (thurston)
**          Added the ad_textnull_cnt field to count the number of NULL chars
**          converted to blanks for `text'.
**	02-dec-88 (jrb)
**	    Added the ad_agabsdate_cnt field to count the number of absolute
**	    dates which were found during an aggregation.  (Only date intervals
**	    are allowed for the avg() and sum() aggregates.)
**	17-mar-89 (jrb)
**	    Added new decimal-related fields.
**	27-jan-2000 (hayke02)
**	    Added AD_1RSVD_EXEDBP as a flag for ad_1rsvd_cnt. This
**	    new flag is used to indicate to adc_keybld() that we
**	    are executing a DBP and should not call adc_ixopt().
**	    This prevents incorrect results when a DBP parameter
**	    is used in multiple predicates. This change fixes bug
**	    99239.
**	11-Apr-2008 (kschendel)
**	    Add ad_anywarn_cnt so that one can quickly test for any warning
**	    things at all.
**      19-nov-2008 (huazh01)
**          Remove 'ad_1rsvd_cnt' from ADI_WARN structure. (b121246).
[@history_template@]...
*/

typedef struct _ADI_WARN
{
    i4              ad_intdiv_cnt;      /* Counts integer divisions by zero */
    i4		    ad_intovf_cnt;	/* Counts integer overflows */
    i4		    ad_fltdiv_cnt;	/* Counts float divisions by zero */
    i4		    ad_fltovf_cnt;	/* Counts float overflows */
    i4		    ad_fltund_cnt;	/* Counts float underflows */
    i4		    ad_mnydiv_cnt;	/* Counts money division by zero */
    i4		    ad_noprint_cnt;	/* Counts non-print cvt'ed to blank */
    i4		    ad_textnull_cnt;	/* Counts NULLs cvt'ed to blank in txt*/
    i4		    ad_agabsdate_cnt;	/* Counts # of abs dates aggregated */
    i4		    ad_decdiv_cnt;	/* Counts decimal divisions by zero */
    i4		    ad_decovf_cnt;	/* Counts decimal overflows */
    i4		    ad_anywarn_cnt;	/* Counted up if ANY warnings at all */
}   ADI_WARN;



/*}
** Name: ADF_OUTARG - describes printed output formats available to user.
**
** Description:
**      This structure contains information that is session specific, and
**      needed by ADF to properly format various datatypes.  It is set up
**      by the caller of adg_init() prior to that call.  Any members that
**      are set to -1 (for the i4's) or NULLCHAR (for the char's) will be
**      reset by adg_init() to be the system defaults.
**
** History:
**	30-may-86 (ericj)
**          Converted for Jupiter.
**	08-jan-87 (thurston)
**          Changed member names ad_linesperpage and ad_coldelim to be
**	    ad_1_rsvd and ad_2_rsvd, since they are not used by ADF.  Also
**	    re-aligned the members, and added an extra i4 reserved, ad_2_rsvd,
**	    and an extra char reserved, ad_4_rsvd.
**	12-apr-04 (inkdo01)
**	    Added ad_i8width for BIGINT support (replaces ad_1_rsvd).
*/

/* *** DANGER DANGER do not change the size of this structure, as it
** precedes adf_error in the ADF_CB, and hence its size is exposed to
** object management extension users.  (The contents may change, but
** not the total size.
*/

typedef struct _ADF_OUTARG
{
    i4	    ad_c0width;     /* Minimum output width of "c" or "char" field.
			    ** At adg_init() time, if -1 is supplied
			    ** here, this means:  Use the default value.
			    */
    i4	    ad_t0width;     /* Minimum output width of "text" or "varchar" field
			    ** At adg_init() time, if -1 is supplied
			    ** here, this means:  Use the default value.
			    */
    i4	    ad_i1width;     /* Output width of "i1" field.
			    ** At adg_init() time, if -1 is supplied
			    ** here, this means:  Use the default value.
			    */
    i4	    ad_i2width;     /* Output width of "i2" field.
			    ** At adg_init() time, if -1 is supplied
			    ** here, this means:  Use the default value.
			    */
    i4	    ad_i4width;     /* Output width of "i4" field.
			    ** At adg_init() time, if -1 is supplied
			    ** here, this means:  Use the default value.
			    */
    i4	    ad_f4width;     /* Output width of "f4" field.
			    ** At adg_init() time, if -1 is supplied
			    ** here, this means:  Use the default value.
			    */
    i4	    ad_f8width;     /* Output width of "f8" field.
			    ** At adg_init() time, if -1 is supplied
			    ** here, this means:  Use the default value.
			    */
    i4	    ad_f4prec;      /* Number of decimal places on "f4" output.
			    ** At adg_init() time, if -1 is supplied
			    ** here, this means:  Use the default value.
			    */
    i4	    ad_f8prec;      /* Number of decimal places on "f8" output.
			    ** At adg_init() time, if -1 is supplied
			    ** here, this means:  Use the default value.
			    */
    i4	    ad_i8width;     /* Output width of "i8" field.
			    ** At adg_init() time, if -1 is supplied
			    ** here, this means:  Use the default value.
			    */
    i4	    ad_2_rsvd;      /* <RESERVED>
			    */
    char    ad_f4style;     /* "f4" output style:  'e', 'f', 'g', or 'n'.
			    ** At adg_init() time, if NULLCHAR is supplied
			    ** here, this means:  Use the default value.
			    */
    char    ad_f8style;     /* "f8" output style:  'e', 'f', 'g', or 'n'.
			    ** At adg_init() time, if NULLCHAR is supplied
			    ** here, this means:  Use the default value.
			    */
    char    ad_3_rsvd;      /* <RESERVED>
			    */
    char    ad_4_rsvd;      /* <RESERVED>
			    */
}   ADF_OUTARG;


/*}
** Name: ADX_MATHEX_OPT - Option for handling a math exception.
**
** Description:
**      This is used to hold one of the 4 possible options for handling
**      a math exception in the adx_handler() routine.  These options
**      are:  Ignore, Warn, or Error. The fourth IgnoreAll is to allow
**	all exceptions to to be handled and converted to returns and
**	is intended to allow ringfencing of adf_func() calls.
**
** History:
**      19-feb-86 (thurston)
**          Initial creation.
**      03-mar-86 (thurston)
**          Put the (ADX_MATHEX_OPT) cast on #defines to please lint.
**	27-Nov-2009 (kiria01) b122966
**	    Allow an ignore ALL form for ringfencing exceptions. Intended
**	    for constant folding where an exception may be raised prematurely
**	    and will be re-tried in proper context later.
*/

typedef	    i2	    ADX_MATHEX_OPT;

#define  ADX_IGN_MATHEX  (ADX_MATHEX_OPT) 1 /* Ignore the exception,         */
                                            /* and tell calling routine      */
                                            /* to continue.                  */

#define  ADX_WRN_MATHEX  (ADX_MATHEX_OPT) 2 /* Format a warning message      */
                                            /* for the user, and tell        */
                                            /* calling routine to continue.  */

#define  ADX_ERR_MATHEX  (ADX_MATHEX_OPT) 3 /* Format an error message       */
                                            /* for the user, and tell        */
                                            /* calling routine to abort      */
                                            /* the query.                    */

#define  ADX_IGNALL_MATHEX  (ADX_MATHEX_OPT) 4 /* Ignore all exceptions,     */
                                            /* and tell calling routine      */
                                            /* to continue. Includes those   */
					    /* exceptions like MH_BADARG etc */


/*}
** Name: ADF_STRTRUNC_OPT - Option for handling string truncation.
**
** Description:
**      This is used to hold one of the 3 possible options for handling
**      a string truncation.  These 3 options are:  Ignore, Warn, or Error.
**
** History:
**      12-aug-1993 (stevet)
*/

typedef	    i2	    ADF_STRTRUNC_OPT;

#define  ADF_IGN_STRTRUNC  (ADF_STRTRUNC_OPT) 1 /* Ignore the string         */
                                                /* truncation                */
#define  ADF_WRN_STRTRUNC  (ADF_STRTRUNC_OPT) 2 /* Format a warning message  */
                                                /* for the user but continue */
                                                /* processing.               */
#define  ADF_ERR_STRTRUNC  (ADF_STRTRUNC_OPT) 3 /* Format an error message   */
                                                /* and tell calling routine  */
                                                /* to abort the query.       */


/*}
** Name: ADF_STR_NULL - String to use when displaying a null value.
**
** Description:
**	One of these structures will be contained in the ADF session control
**	block.  At session initialization (i.e. the adg_init() call) ADF
**	may be asked to initialize this with the hard-wired system default
**	NULLstring, or it may be passed in an already initialized one.  (See
**	below for details.)  The NULLstring is that string which will be
**	displayed to the user when the value is null.  This will be used by
**	the TM routine adc_tmcvt() among others.
**
** History:
**      06-mar-87 (thurston)
**          Initial creation.
*/

typedef struct _ADF_STR_NULL
{
    i4	    	    nlst_length;	/* Length of the NULLstring, not
					** including any null terminator,
					** even if there is one.  If this is
					** set to -1 on call to adg_init(),
					** that routine will fill in the buffer
					** pointed to by .nlst_string with the
					** hard-wired system default NULLstring,
					** and reset .nlst_length.
					*/
#define			ADF_MXDEF_STR_NULL   1	/* Max length of hard-wired   */
						/* system default NULLstring, */
						/* plus 1 (to avoid zero).    */
    char	    *nlst_string;	/* The string to use for null value.
					** If .nlst_length is zero, this may be
					** a NULL pointer.
					*/
}   ADF_STR_NULL;


/*}
** Name: ADK_CONST_BLK - ADF query constants block.
**
** Description:
**      One of these structures is allocated and a pointer to it is placed in
**      the ADF_CB for each query.  There may be several queries active for a
**	particular session at any given time (e.g. cursors).  Before a query is
**	to be executed, the ADF caller should set the .adf_constants pointer in
**	the ADF_CB to point to the correct ADK_CONST_BLK.  This is important
**	because certain values are to remain constant for the life of the query;
**	not the server, and not the session.  ADF will fill in this structure
**	when it needs to, but not before.  This is because certain values held
**	in this block may be expensive to calculate, and may not even be needed
**	for a particular query.
**
**	If a query is known to not use query constants, the adf_constants
**	pointer in the ADF_CB can be left null.  A constants-block is
**	only needed if the query might reference a query constant.
**
** History:
**     18-feb-87 (thurston)
**          Initial creation.
**	24-Mar-2005 (schka24)
**	    Get rid of "need mask", not used.  Also, get rid of all the
**	    reserved stuff, this is not used or known by any user program
**	    so we're not locked in to anything.
**	10-Sep-2006 (gupsh01)
**	    Fixed the lengths of ANSI date/time system constants.
*/

typedef struct _ADK_CONST_BLK
{
    i4              adk_set_mask;       /* Mask of items that have already been
					** calculated and stored in this struct.
					** This should be zero'ed when the query
					** is initialized for execution.
					*/
#define			ADK_BINTIM	((i4) 0x00000001) /* _bintim bit */
#define			ADK_CPU_MS	((i4) 0x00000002) /* _cpu_ms bit */
#define			ADK_ET_SEC	((i4) 0x00000004) /* _et_sec bit */
#define			ADK_DIO_CNT	((i4) 0x00000008) /* _dio_cnt bit */
#define			ADK_BIO_CNT	((i4) 0x00000010) /* _bio_cnt bit */
#define			ADK_PFAULT_CNT	((i4) 0x00000020) /* _pfault_cnt bit */
#define			ADK_CURR_DATE	((i4) 0x00000040) /* _curr_date bit */
#define			ADK_CURR_TIME	((i4) 0x00000080) /* _curr_time bit */
#define			ADK_CURR_TSTMP	((i4) 0x00000100) /* _curr_tmstmp bit */
#define			ADK_LOCAL_TIME	((i4) 0x00000200) /* _local_time bit */
#define			ADK_LOCAL_TSTMP	((i4) 0x00000400) /* _local_timestmp bit */
    i4              adk_bintim;         /* */
    i4              adk_cpu_ms;         /* */
    i4              adk_et_sec;         /* */
    i4              adk_dio_cnt;        /* */
    i4              adk_bio_cnt;        /* */
    i4              adk_pfault_cnt;     /* */
    char	    adk_curr_date[4];      /* Store the current date */
    char	    adk_curr_time[10];      /* Store the current time */
    char	    adk_curr_tstmp[14];     /* Store the current timestamp */
    char	    adk_local_time[10];     /* Store the local time */
    char	    adk_local_tstmp[14];    /* Store the local timestamp */
    /*  Can add up to 26 more things before the i4 bit-mask runs out. */
}   ADK_CONST_BLK;

#define AD_MAX_SUBCHAR  4


/*}
** Name: ADF_CB - ADF session control block.
**
** Description:
**	One of these structures is allocated and a pointer to it is passed to
**	the adg_init() routine in order to initialize ADF for a particular
**	session.  In a typical front end process this is done only once.  In
**	the INGRES DBMS server SCF's sequencer module, SCS, will do this for
**	each session, during session initialization.  Most ADF routines will
**	require a pointer to this block as a parameter.
**
**	Note that additions to this structure must be cloned to the file
**	feadfcb.c in front!utils!ug.  Failure to do so means that the 
**	additional functionality you are adding won't work on some front-ends.
**
** History:
**      19-feb-86 (thurston)
**          Initial creation.
**	30-may-86 (ericj)
**	    Added the ADF_OUTARG struct.
**	20-jul-86 (ericj)
**	    Added ADF_ERROR struct.
**	07-aug-86 (ericj)
**	    Added an ADI_WARN struct.  This will be used in the processing
**	    of ADF warnings.
**	08-aug-86 (ericj)
**	    Added the ADX_MATHEX_OPT struct.
**	13-nov-86 (thurston)
**	    Added the .adf_qlang member.
**	18-feb-87 (thurston)
**	    Added the .adf_constants member.
**	24-feb-87 (thurston)
**	    Added the .adf_srv_cb member.
**	16-mar-87 (thurston)
**	    Added the .adf_slang member.
**	01-sep-88 (thurston)
**	    Added the .adf_maxstring member, along with some extra reserved
**	    space, just in case.
**	03-may-89 (anton)
**	    Added adf_collation to specify a collation sequence for string
**	    comparison
**	24-apr-90 (jrb)
**	    Added adf_rms_context for recording date conversion context for RMS
**	    gateway.
**	06-sep-90 (jrb)
**	    Added adf_timezone to support independent time zones per session.
**	06-oct-1992 (fred)
**	    Added adf_lo_context for storing large object processing context
**	    in the control block.  This had previously been hackily stored
**	    in adf_4rsvd, which someone [legally] used.
**      12-aug-1993 (stevet)
**          Added adf_strtrunc_opt for detecting string truncation.
**	1-nov-93 (robf)
**          Added E_AD1098_BAD_LEN_SEC_INTRNL
**	13-nov-94 (nick)
**	    Added adf_year_cutoff ( nee II_DATE_CENTURY_BOUNDARY ).
**	21-mar-96 (nick)
**	    Added some comments to ensure changes made here are made elsewhere
**	    as well.
**	24-mar-99 (stephenb)
**	    Add adf_lo_status field.
**	24-mar-2001 (stephenb)
**	    Add adf_ucollation field.
**      29-jun-2001 (mcgem01)
**          Moved adf_ucollation to the end of the control block.
**	18-nov-03 (inkdo01)
**	    Added adf_base_array for ade_execute_cx() to use in place of
**	    excb_bases to resolve operand addrs.
**	19-feb-2004 (gupsh01)
**	    Added adf_unisub_status and adf_unisub_char to support
**	    set [no]unicode_substitution. 
**	7-May-2004 (schka24)
**	    adf_lo_status unused, remove.
**	18-may-2004 (gupsh01)
**	    Added ADI_NPRINT and ADI_NTPRINT for nchar and nvarchar output
**	    length calculation. This is used for nchar() or nvarchar()
**	    function calls for integer/float/decimal inputs.
**	27-aug-04 (inkdo01)
**	    Removed adf_base_array - no longer needed for global base arrays.
**	15-feb-2005 (gupsh01)
**	    Added adf_uninorm_flag for database level NFC support.
**	04-Apr-2005 (jenjo02)
**	    Added adf_autohash to resolve MODIFY with AUTOMATIC
**	    partition anomalies.
**	05-sep-2006 (gupsh01)
**	    Added E_AD5064_DATEANSI.
**	06-sep-2006 (gupsh01)
**	    Added adi_dtfamily_resolve() and adi_need_dtfamily_resolve()
**	    to resolve datatype family.
**	27-apr-2007 (dougi)
**	    Added adf_utf8_flag for UTF8-enabled servers.
**	10-Jul-2007 (kibro01) b118702
**	    Remove AD_DATE_INGRESDATE which is never referred to.
**	23-Jul-2007 (smeke01) b118798.
**	    Added adf_proto_level flag value AD_I8_PROTO. adf_proto_level 
**	    has AD_I8_PROTO set iff the client session can cope with i8s.
**	    Also added new length spec constants for integer arithmetic 
**	    operators to allow these to be treated separately by 
**	    adi_0calclen. 
**	21-Mar-2008 (kiria01) b120144
**	    Added adi_dtfamily_resolve_union.
**	8-Jul-2008 (kibro01) b120584
**	    Add AD_ANSIDATE_PROTO
**	26-Oct-2008 (kiria01) SIR121012
**	    Added .adf_const_const for constant folding support.
**	12-Dec-2008 (kiria01) b121297
**	    Added  prototype for adu_0parsedate
**      26-Nov-2009 (hanal04) Bug 122938
**          Added adf_max_decprec to hold the maximum decimal precision
**          supported by the client.
**      24-Jun-2010 (horda03) B123987
**          Add adf_misc_flags, and ADF_LONG_DATE_STRINGS to signal a
**          String of AD_11_MAX_STRING_LEN length should be returned
**          for a DATE->CHAR conversion.
*/

typedef struct _ADF_CB
{
/* *** DANGER DANGER Change nothing from here to adf_errcb, because the
** start of the ADF_CB is exposed to OME users as "II_SCB".
*/
    PTR		    adf_srv_cb;		/* Ptr to ADF's server control block */
    DB_DATE_FMT     adf_dfmt;           /* The date format to use */
    DB_MONY_FMT     adf_mfmt;           /* The money format to use */
    DB_DECIMAL      adf_decimal;        /* The decimal char to use */
    ADF_OUTARG	    adf_outarg;		/* Output formatting info */
    ADF_ERROR	    adf_errcb;		/* ADF error control block */
/* *** OK to change stuff beyond here */
    ADI_WARN	    adf_warncb;		/* ADF warning control block */
    ADX_MATHEX_OPT  adf_exmathopt;	/* The math exception handling option */
    DB_LANG	    adf_qlang;		/* Query language in use */
    ADF_STR_NULL    adf_nullstr;	/* Descrip. of the NULLstring in use */
    ADK_CONST_BLK   *adf_constants;	/* Ptr to block of values that should
					** remain constant for the life of a
					** query; not server, not session.
					** These are things like _cpu_ms,
					** _bintim, _et_sec, etc.
					*/
    i4		    adf_slang;		/* Which spoken language is in use */
    i4		    adf_maxstring;	/* This is passed into the adg_init()
					** routine at session init time.  It
					** represents the largest INGRES string
					** supported for that session.  It must
					** be in the range 1 - DB_GW4_MAXSTRING.
					** Typically, this will be set to
					** DB_MAXSTRING by the INGRES DBMS,
					** while a Frontend program, connected
					** to a Gateway using a foreign DBMS
					** may set this number higher.
					*/
		/*	     ---------------------------------
		**	     SPECIAL NOTE ABOUT .adf_maxstring
		**	     ---------------------------------
		** This value may actually be changed during the life of the
		** session; but it should NOT BE DONE LIGHTLY!!!!  It will only
		** be checked for validity during the adg_init() call to
		** initialize the ADF session.  Changing it to an illegal value
		** subsequently could prove disasterous.  The reason for
		** allowing it to change is that a Frontend process must start
		** an ADF session *BEFORE* it can connect to the DBMS/Gateway
		** to find out how large the DBMS's strings can be.  This will
		** probably be done by initially setting .adf_maxstring to be
		** DB_MAXSTRING, connecting to the DBMS, retrieving the
		** information from the `capabilities' catalog, and then
		** possibly re-adjusting  it afterwards.  An alternative
		** approach (and this is probably cleaner) would be:  after
		** retrieving the capabilities information, the Frontend could
		** release the ADF session by calling adg_release(), and then
		** re-initialize it again with another call to adg_init().
		*/
    PTR		    adf_collation;	/* collation descriptor */
    i4 		    adf_rms_context;	/* date conversion context for rms gw */
    PTR             adf_tzcb;	        /* Timezone pointer */
    i4 		    adf_proto_level;	/*
                                        ** ADF datatypes support at current 
					** protocol level
					*/
# define                AD_DECIMAL_PROTO     0x00000001 /* Support decimal */
# define    	    	AD_BIT_PROTO	     0x00000002 /* ... bit & */
# define                AD_BYTE_PROTO        0x00000004 /* byte types */
# define                AD_LARGE_PROTO       0x00000008 /* Large objs */
# define                AD_I8_PROTO          0x00000010 /* Supports 8-byte integers */
# define                AD_ANSIDATE_PROTO    0x00000020 /* Supports "ansidate" data type */
#define                 AD_BOOLEAN_PROTO     0x00000040 /* Supports boolean */
    i4              adf_max_decprec;    /* Max precision for decimal */
    i4		    adf_lo_context;	/* Context area for large objects */
    ADF_STRTRUNC_OPT adf_strtrunc_opt;
    i4		    adf_year_cutoff;	/* II_DATE_CENTURY_BOUNDARY */
    PTR		    adf_ucollation;	/* Unicode collation descriptor */
    i2              adf_unisub_status;  /* status of unicode substitution */
# define                AD_UNISUB_OFF   0x0000 /*Substitution off */
# define                AD_UNISUB_ON    0x0001 /*Substitution on */
    char            adf_unisub_char [AD_MAX_SUBCHAR];   /* Preferred substitution
                                                        ** character */
    i2              adf_uninorm_flag;  /* status of unicode normalization */
# define                AD_UNINORM_NFD   0x0000 /* Unicode normalization NFD used*/
# define                AD_UNINORM_NFC   0x0001 /* Unicode normalization NFC used*/
    u_i8	    *adf_autohash;     /* Optional pointer to u_i8 to be hashed
				       ** for AUTOMATIC partition dimension in lieu 
				       ** of randon number dimension assignment.
				       */
    i2		    adf_date_type_alias;    /* data type referred by 'date' type */
# define		AD_DATE_TYPE_ALIAS_NONE   0x0000 /* date may not be used */
# define		AD_DATE_TYPE_ALIAS_INGRES 0x0001 /* date refers to ANSI date */
# define		AD_DATE_TYPE_ALIAS_ANSI   0x0002 /* date refers to ANSI date */
    i1		    adf_utf8_flag;	/* UTF8edness of server */
# define		AD_UTF8_ENABLED		0x01	/* server is UTF8 enabled */
    u_i1	    adf_const_const;
# define		AD_CNST_DSUPLMT	0x01	/* Will be set each time an otherwise
						** constant date string is converted
						** into a date needing to date or
						** time parts supplemented from 'now'
						** See ADI_F16384_CHKVARFI */
# define		AD_CNST_USUPLMT	0x02	/* Will be set each time an otherwise
						** constant Unicode string is converted
						** into atring needing a substitution
						** char supplemented -> not constant.
						** See ADI_F16384_CHKVARFI */
   u_i4             adf_misc_flags;
#define ADF_LONG_DATE_STRINGS 0x00000001    /* Date strings of AD_11_MAX_STRING_LEN required */
}   ADF_CB;


/*}
** Name: ADI_OP_ID - Operation id.
**
** Description:
**      Used to hold an operation id, which uniquely identifies an operation,
**      whether it is a function (such as "concat"), operator (such as "+"),
**      conversion function (such as "text"), aggregate function (such as
**      "sum"), or whatever.  An operation id is retrieved from ADF via the
**      adi_opid() routine by using the operator name as a handle.
**
** History:
**      19-feb-86 (thurston)
**          Initial creation.
**      03-mar-86 (thurston)
**          Put the (ADI_OP_ID) cast on #define for ADI_NOOP to please lint.
**	30-may-89 (jrb)
**	    Added ADI_FIND_COMMON to support the relocation of datatype
**	    resolution into ADF.
*/

typedef	    i2	    ADI_OP_ID;

#define		ADI_NOOP	    (ADI_OP_ID) -1  /* The null operator id */
#define		ADI_FIND_COMMON	    (ADI_OP_ID) -2  /* Tells adi_resolve to */
						    /* find a common data-  */
						    /* type.		    */
#define		ADI_DISABLED	    (ADI_OP_ID) -4  /* Flags undefined ope- */
						    /* rator of a function. */

/*}
** Name: ADI_OP_NAME - Operator name.
**
** Description:
**      This structure defines an ADF operator name.  All functions and
**      operators known to the ADF have an associated operator name
**      which is used as a handle to retrieve the operator id via a call
**      to adi_opid().  The function adi_opname() will return the operator
**      name given the operator id.
**
** History:
**     20-feb-86 (thurston)
**          Initial creation.
*/

typedef struct _ADI_OP_NAME
{
    char            adi_opname[ADF_MAXNAME]; /* Operator name */
}   ADI_OP_NAME;


/*}
** Name: ADI_OPINFO - Structure to hold information about an INGRES operation.
**
** Description:
**      This structure is used for returning externally visible pieces of
**	information about any INGRES operator known to ADF.  This is done
**	through the adi_op_info() routine.
**
** History:
**     07-jul-88 (thurston)
**          Initial creation.
*/

typedef struct _ADI_OPINFO
{
    ADI_OP_ID	    adi_opid;		/* The OP id ... just for fun.
					*/
    ADI_OP_NAME	    adi_opname;		/* The OP name associated with the
					** OP id.
					*/
    i4		    adi_use;		/* How the OP is used; one of the
					** following symbolic constants:
					*/
#define                 ADI_PREFIX	    1
#define                 ADI_POSTFIX	    2
#define                 ADI_INFIX	    3

    i4		    adi_optype;		/* The "type" of operation.  One of
					** the constants:
                                        **	ADI_COMPARISON
                                        **	ADI_OPERATOR
                                        **	ADI_AGG_FUNC
                                        **	ADI_NORM_FUNC
                                        **	ADI_PRED_FUNC  
					** The actual symbolic constants are
					** defined later in this file under the
					** ADI_FI_DESC typedef, where they are
					** also used.
					*/
    DB_LANG	    adi_langs;		/* Bit mask representing the query
					** languages this operation is defined
					** for.  Currently, the only two bits
					** in use are DB_SQL and DB_QUEL.
					*/
    i4		    adi_systems;	/* What systems this OP is valid
					** for.  This is a bit mask with the
					** following bits currently defined:
					*/
#define                 ADI_INGRES_6        0x0001  /* Valid for all INGRES */
						    /* installations of	    */
						    /* release 6.0 or above.*/

#define                 ADI_ANSI            0x0002  /* Valid for any ANSI   */
						    /* standard SQL	    */
						    /* installation.	    */

#define                 ADI_DB2             0x0004  /* Valid for all DB2    */
						    /* installations	    */

    i4		    adi_4_rsvd;		/* <RESERVED> */
    i4		    adi_3_rsvd;		/* <RESERVED> */
    i4		    adi_2_rsvd;		/* <RESERVED> */
    i4		    adi_1_rsvd;		/* <RESERVED> */
}   ADI_OPINFO;


/*}
** Name: ADI_DT_NAME - Datatype name.
**
** Description:
**      This structure defines an ADF datatype name.  All datatypes,
**      whether intrinsic, user defined, or whatever, will have an
**      associated datatype name.  This name is used as a handle to
**      retrieve the datatype id via a call to adi_tyid().  The function
**      adi_tyname() will return the datatype name given the id.
**
** History:
**     20-feb-86 (thurston)
**          Initial creation.
**	16-may-86 (thurston)
**	    Removed the _ADI_DT_NAME from the front of the typedef because it
**	    was not unique to 8 chars (_ADI_DT_BITMASK) and was never used.
*/

typedef struct
{
    char            adi_dtname[ADF_MAXNAME]; /* Datatype name */
}   ADI_DT_NAME;


/*}
** Name: ADI_DT_BITMASK - Bit mask representing some set of datatypes.
**
** Description:
**	This structure is a bit vector of 256 bits which represents some set of
**	the 256 possible datatypes.  If bit number 'i' is set, that means that
**	the datatype whose id is number 'i' is included in this set.  Bits are
**	numbered from 0 to 255, in accordance with the BT routines in the CL.
**	(i.e. Always use BTset, BTclear, BTtest, etc. to manipulate these bit
**	vectors.)  This implies that datatype ids must fall within the range
**	0-255, and obviously there can be no more than 255 total datatypes for
**	any installation.  (Note that, for other reasons, a datatype ID of 0 is
**	not legal.)
**
**	User defined datatypes begin at number 16384.  For this reason, there is
**	a mapping macro (ADI_DT_MAP_MACRO) which is used to determine the
**	correct bit position into which to place the set bit.  Note that this
**	macro is also used to map datatype id's into their array index for
**	obtaining information from the datatype pointer array (ADF internal).
**
**      One of these structures is filled in by the adi_tycoerce() routine
**      to show which datatypes a supplied datatype can be automatically
**      converted (coerced) into.
**
** History:
**      21-feb-86 (thurston)
**          Initial creation.
**      21-mar-86 (thurston)
**	    Changed bitmask from four u_i4's to be an array of u_i4's.
**	16-may-86 (thurston)
**	    Removed the _ADI_DT_BITMASK from the front of the typedef because it
**	    was not unique to 8 chars (_ADI_DT_NAME) and was never used.
**	28-apr-89 (fred)
**	    Increased length to 256 bits.  This is to [begin] support for user
**	    defined datatypes.  Added ADI_DT_MAP_MACRO() to map user defined
**	    datatypes into this structure.
**      26-agu-1993 (stevet)
**          Added class library support, which have DT id from 8192 - 8319.
**          The valid datatypes ID are:
**                0-127   INGRES native types
**             8192-8319  INGRES class objects
**            16384-16511 UDT
**          The adi_dtbits now defines as:
**               0-127    INGRES native datatypes
**             128-255    INGRES class object
**             256-383    UDT
*/

typedef struct
{
    u_i4            adi_dtbits[ADI_LNBM];        /* bits  0 - 383 */
}   ADI_DT_BITMASK;

#define			ADI_DT_RTI_LIMIT	128
#define                 ADI_DT_CLSOBJ_MIN       8192
#define                 ADI_DT_UDT_MIN          16384
#define			ADI_DT_MAP_MACRO(i) \
		((i) < ADI_DT_RTI_LIMIT	    \
		? (i) : ((i) < ADI_DT_CLSOBJ_MIN + ADI_DT_RTI_LIMIT   \
		    ? (i + ADI_DT_RTI_LIMIT - ADI_DT_CLSOBJ_MIN) \
			 : ((i) + 2*ADI_DT_RTI_LIMIT - ADI_DT_UDT_MIN)))


/*}
** Name: ADI_LENSPEC - Method for computing result length.
**
** Description:
**      These structures contain a pair of integers.  The first
**      integer, an i2, tells how to compute the result length,
**      possibly based on the length of the arguments to the function
**      instance.  The second integer, an i4, is only used if the first
**      stated the result length was to be fixed length.  If so, the
**      second will be the length of the result.  All lengths are in
**      bytes.  The use of the term `maxstring' in the description below
**	refers to the .adf_maxstring field in ADF's session control block.
**	This is the maximum allowable size for an INGRES string in this
**	session.
**
** History:
**      22-jan-86 (thurston)
**          Initial creation.
**	01-jul-86 (thurston)
**	    Added the ADI_RESLEN constant.
**	13-oct-86 (thurston)
**	    Added the ADI_SUMT constant.
**	11-nov-86 (thurston)
**	    Added the ADI_P2D2, ADI_2XP2, and ADI_2XM2 constants.
**	04-may-87 (thurston)
**	    Made many changes to the lenspec codes in order to avoid creating
**	    intermediate values that are too long.  (Example: concat(c200,c200):
**	    we want to avoid creating a c400, which cannot exist, and causes
**	    errors in the Jupiter code.)
**	14-sep-87 (thurston)
**	    Added the ADI_RSLNO1CT constant.
**	01-sep-88 (thurston)
**	    Commented out the ADI_T0PRN, ADI_T0CH, and ADI_T0TX constants since
**	    it is obsolete, and re-worded many others to refer to `maxstring'
**	    and `DB_CNTSIZE' instead of 2000 (or 255), or 2, respectively.
**	28-jul-89 (jrb)
**	    Added a dozen new lenspecs for the DECIMAL project.
**	05-oct-1992 (fred)
**	    Added new lenspec (BIT2CHAR, CHAR2BIT) mechanisms for handling bit
**	    string datatypes.
**	31-jul-1998 (somsa01)
**	    Added new lenspec ADI_COUPON for handling long varchar and
**	    long varbyte input datatypes.  (Bug #74162)
**	12-apr-04 (inkdo01)
**	    Added ADI_4ORLONGER for BIGINT support.
**	20-dec-04 (inkdo01)
**	    Added ADI_CWTOVB for collation_weight() result value.
**	25-Apr-2006 (kschendel)
**	    Turns out that to make multi-param UDF's useful, we need countvec
**	    style lenspec calls.  Add definition here.
**	3-july-06 (dougi)
**	    Add ADI_CTO48 to handle string to int/float coercions.
**	13-sep-2006 (dougi)
**	    Drop ADI_CTO48 along with string/numeric coercions.
**	25-oct-2006 (gupsh01)
**	    Added ADI_DATE2DATE. 
**	14-may-2007 (gupsh01)
**	    Add NFC flag to adu_nvchr_fromutf8. Added adu_utf8_unorm.
**	13-jul-2007 (gupsh01)
**	    Added ADI_O1UNIDBL for length calculation in a case operation.
**	12-Apr-2008 (kiria0) b119410
**	    Added ADI_MINADD1 (At least 'fixed bytes' and then add 1 extra.
**	27-Oct-2008 (kiria01) SIR120473
**	    Added ADI_PATCOMP & ADI_PATCOMPU for pattern compiler.
**	4-jun-2008 (stephenb)
**	    Add ADI_LONGER23RES for nvl2 support. (Bug 123880)
[@history_template@]...
*/

typedef struct _ADI_LENSPEC
{
    i2              adi_lncompute;      /* How to compute result length,
                                        ** as follows:
                                        */
#define                 ADI_O1          1   /* Result length/prec is same as */
					    /* first operand.                */

#define                 ADI_O2          2   /* Result length/prec is same as */
					    /* second operand.               */

#define                 ADI_LONGER      3   /* Result length is the longer  */
					    /* of the two operands.	    */
					    /* Precision is inherited from  */
					    /* the longer.		    */

#define                 ADI_SHORTER     4   /* Result length is the shorter */
					    /* of the two operands.	    */
					    /* Precision is inherited from  */
					    /* the shorter.		    */

#define                 ADI_DIFFERENCE  5   /* Result length is length      */
					    /* of first operand - length    */
					    /* of second operand.           */
					    /* Precision is set to 0.	    */

#define                 ADI_CSUM        6   /* Result length is sum of      */
					    /* the lengths of the two       */
					    /* operands, but <= maxstring,  */
					    /* and > 0.                     */
					    /* Precision is set to 0.	    */

#define                 ADI_FIXED       7   /* Result length and precision  */
					    /* are constant as specified in */
					    /* adi_fixedsize and adi_psfixed*/

#define                 ADI_PRINT       8   /* Result length is default     */
					    /* print size of first          */
					    /* operand (numerics only).     */
					    /* Precision is set to 0.	    */

#define                 ADI_O1CT        9   /* Result length is length      */
					    /* of first operand + DB_CNTSIZE*/
					    /* Precision is set to 0.	    */

#define                 ADI_O1TC        10  /* Result length is length of   */
					    /* first operand - DB_CNTSIZE,  */
					    /* but <= maxstring, and > 0.   */
					    /* Precision is set to 0.	    */

#define                 ADI_TSUM        12  /* Result length is sum of the  */
					    /* lengths of the two           */
					    /* operands - DB_CNTSIZE, but   */
					    /* <= maxstring + DB_CNTSIZE.   */
					    /* Precision is set to 0.	    */

#define                 ADI_ADD1        13  /* Result length is length      */
					    /* of the first operand + 1.    */
					    /* Precision is set to 0.	    */

#define                 ADI_SUB1        14  /* Result length is length      */
					    /* of the first operand - 1.    */
					    /* Precision is set to 0.	    */

#define                 ADI_TPRINT      15  /* Result length is default     */
					    /* print size of first          */
					    /* operand (numerics only)      */
					    /* for text/varchar.            */
					    /* Precision is set to 0.	    */

#define			ADI_4ORLONGER	16  /* Result length is 4 or the    */
					    /* longest operand length. For  */
					    /* integer ops possibly with    */
					    /* a BIGINT operand             */

#define                 ADI_RESLEN      18  /* Result length and precision  */
					    /* should already be known.	    */
					    /* (i.e. It is what- ever the   */
					    /* length and precision of the  */
					    /* result DB_DATA_VALUE is.)    */
					    /* This is used by the	    */
					    /* coercions of a datatype into */
					    /* itself in order to adjust    */
					    /* its length and precision	    */
					    /* before being output into a   */
					    /* tuple.			    */

#define                 ADI_SUMT        19  /* Result length is sum of      */
					    /* the lengths of the two       */
					    /* operands + DB_CNTSIZE.       */
					    /* Precision is set to 0.	    */

#define                 ADI_P2D2        20  /* Result length is (length of  */
					    /* 1st operand + DB_CNTSIZE) / 2*/
					    /* Precision is set to 0.	    */

#define                 ADI_2XP2        21  /* Result length is (length of  */
					    /* 1st operand * 2) + DB_CNTSIZE*/
					    /* Precision is set to 0.	    */

#define                 ADI_2XM2        22  /* Result length is (length of   */
                                            /* 1st operand * 2) - DB_CNTSIZE,*/
                                            /* but <= maxstring + DB_CNTSIZE.*/
					    /* Precision is set to 0.	    */

#define                 ADI_CTSUM       23  /* Result length is sum of the   */
                                            /* lengths of the two operands,  */
                                            /* - DB_CNTSIZE, but <=          */
					    /* maxstring, and > 0.           */
					    /* Precision is set to 0.	    */


#define                 ADI_SUMV        24  /* Result length is sum of the   */
                                            /* lengths of the two operands,  */
                                            /* but <= maxstring + DB_CNTSIZE.*/
					    /* Precision is set to 0.	    */

#define                 ADI_RSLNO1CT    25  /* Result length should already */
					    /* be known.  (i.e. It is what- */
					    /* ever the length of the	    */
					    /* result DB_DATA_VALUE is.)    */
					    /* However, a check will be	    */
					    /* made to assure that the	    */
					    /* length is valid if it is	    */
					    /* not, the result length will  */
					    /* be set to the length of the  */
					    /* of the input, plus	    */
					    /* DB_CNTSIZE.  This is only    */
					    /* used by the ii_notrm_txt and */
					    /* ii_notrm_vch functions that  */
					    /* are needed by OSL.	    */
					    /* Precision is set to 0.	    */
/*
** The next group of length specs are for the DECIMAL datatype.  These are all
** based on the input precision(s).  To obtain the output length we first
** calculate the output precision and then convert to length.  A list of the
** formulas follows:
**
** Legend:  p1 --> precision of 1st input	s1 --> scale of 1st input
**	    p2 --> precision of 2nd input	s2 --> scale of 2nd input
**	    rp --> precision of result  	rs --> scale of result
**
** Note: there are four "mixed type" lenspecs (DECIMUL, IDECMUL, DECIDIV, and
** IDECDIV) and their formulas are obtained by first using the DECINT lenspec to
** get the precision and scale of the decimal value obtained from coercing the
** integer, and then applying the normal decimal formula.
**
**  For decimal addition and subtraction (lenspec ADI_DECADD):
**	    rp = min(DB_MAX_DECPREC, max(p1-s1, p2-s2) + max(s1, s2) + 1)
**	    rs = max(s1, s2)
**
**  For decimal multiplication (lenspec ADI_DECMUL):
**	    rp = min(DB_MAX_DECPREC, p1+p2)
**	    rs = min(DB_MAX_DECPREC, s1+s2)
**
**  For decimal division (lenspec ADI_DECDIV):
**	    rp = DB_MAX_DECPREC
**	    rs = DB_MAX_DECPREC - p1 + s1 - s2
**
**  For decimal avg() (lenspec ADI_DECAVG):
**	    rp = DB_MAX_DECPREC
**	    rs = max(DB_MAX_DECPREC - p1 + s1, 0)
**
**  For decimal sum() (lenspec ADI_DECSUM):
**	    rp = DB_MAX_DECPREC
**	    rs = s1
**
**  For coercion from integer to decimal (lenspec ADI_DECINT):
**	    rp = 11 (if i4) or 5 (if i1 or i2)
**	    rs = 0
**
**  For decimal function (lenspec ADI_DECFUNC) we take the VALUE of
**  the 2nd parameter.
*/
#define			ADI_DECADD		26
#define			ADI_DECMUL		27
#define			ADI_DECIMUL		28
#define			ADI_IDECMUL		29
#define			ADI_DECDIV		30
#define			ADI_DECIDIV		31
#define			ADI_IDECDIV		32
#define			ADI_DECAVG		33
#define			ADI_DECSUM		34
#define			ADI_DECINT		35
#define			ADI_DECFUNC		36
#define			ADI_DECBLEND		37

/*
** The following are used for bit types to/from character translation.
** Basically, the lengths are the same -- the units changes, since bits
** are represented by 1 character, but are stored BITSPERBYTE to a byte.
** Thus, the units in lengths must change (multiply/divide) by BITSPERBYTE,
** dealing with leftovers.  There are two types -- differences between fixed
** and varying character and bit strings are handled independently.
*/
#define			ADI_BIT2CHAR		38
#define			ADI_CHAR2BIT		39

/*
** This is used in the case of long varchar or long varbyte input datatypes.
** When we try to get the spec of this type of column in the base table,
** we will get back 32 as the size of the column (which is right, because
** that is the length of the coupon). However, this size does not reflect
** the size of the actual underlying datatype, which can be up to 2G! Thus,
** the result length will be the maximum length of a string for the session,
** plus DB_CNTSIZE depending upon the result type. Presicion is set to zero.
*/
#define			ADI_COUPON		40

/* Handle unhex situation		*/
#define                 ADI_X2P2        41  /* Result length is (length of  */
					    /* 1st operand / 2) + DB_CNTSIZE*/
					    /* Precision is set to 0.	    */

/* These coercion functions are added to support the coercion from
** unicode data types nchar and nvarchar to the regular datatypes
** char and varchar.
*/

#define                 ADI_CTOU        42  /* Convert char to nchar or    */
                                            /* varchar to nvarchar         */
                                            /* Precision is set to 0.      */
#define                 ADI_UTOC        43  /* Convert nchar to char or    */
                                            /* nvarchar to varchar         */
                                            /* result length is 1.5 times  */
                                            /* the length of first operand */
                                            /* Precision is set to 0.      */
#define                 ADI_CTOVARU     44  /* convert char to nvarchar    */
                                            /* Precision is set to 0.      */
#define                 ADI_VARCTOU     45  /* convert varchar to nchar    */
                                            /* Precision is set to 0.      */
#define                 ADI_UTOVARC     46  /* convert nchar to varchar    */
                                            /* Precision is set to 0.      */
#define                 ADI_VARUTOC     47  /* convert nvarchar to char    */
#define                 ADI_NPRINT      48  /* Result length is default     */
                                            /* print size of first          */
                                            /* operand (numerics only)      */ 
                                            /* multiplied by sizeof(UCS2)   */
                                            /* for nchar output.            */
                                            /* Precision is set to 0.       */

#define                 ADI_NTPRINT     49  /* Result length is default     */
                                            /* print size of first          */
                                            /* operand (numerics only)      */
                                            /* multiplied by sizeof(UCS2)   */
                                            /* and fixed for nvarchar       */
                                            /* Precision is set to 0.       */

#define			ADI_CWTOVB	50  /* Computes result length of    */
					    /* collation_weight() function  */
					    /* stored in varbyte.	    */

#define			ADI_DATE2DATE	51  /* Computes result length of    */
					    /* date family coercions 	    */
#define			ADI_DECROUND	52  /* result length of round(),    */
					    /* trunc()                      */
#define			ADI_DECCEILFL	53  /* result length of ceil(),     */
					    /* floor()                      */
#define                 ADI_O1UNIDBL    54  /* same as ADI_01 but for unicode */
					    /* characters where the result  */	
					    /* doubles in size.		    */
/* 
** The following group together integer multiply, add, subtract & divide. 
** This is because we want to set the result length for these to the widest 
** supported by the session. From 9.0.1 this is 8 bytes. Prior to 9.0.1 it
** is 4 bytes. 
*/
#define			ADI_IMULI	55  
#define			ADI_IADDI	56 
#define			ADI_ISUBI	57
#define			ADI_IDIVI	58
#define			ADI_MINADD1	59	/* At least 'fixed' bytes +1 */
#define                 ADI_O1UNORM     60	/* lenspec to unorm 1st op */
#define			ADI_PATCOMP	61	/* Est pat len */
#define			ADI_PATCOMPU	62	/* Est unicode pat len */
#define			ADI_O1AES	63	/* padded for block crypto */

/*
** Special for 3 operand functions where we need the length
** of the longest of parameters 2 and 3
*/
#define			ADI_LONGER23RES	64	/* longer of parms 2 and 3
						** taking the result into 
						** account. 
						*/

/*
** ADI_LEN_INDIRECT is introduced to support the DB_ALL_TYPE and
** ADI_LEN_EXTERN is used for UDT functions.
** ADI_LEN_EXTERN_COUNTVEC is used for UDT's as well, when they have
** multiple params.  (The LEN_EXTERN call is only defined for two.)
*/
#define                 ADI_LEN_INDIRECT        998
#define                 ADI_LEN_EXTERN          999
#define			ADI_LEN_EXTERN_COUNTVEC	1000

    i4              adi_fixedsize;      /* If adi_lncompute = ADI_FIXED,
                                        ** this will be the size of the
                                        ** result.
                                        */

    i2		    adi_psfixed;	/* If adi_lncompute == ADI_FIXED,
					** this will be the prec/scale of
					** the result.  Normally zero,
					** except for DECIMAL results.
					*/
}   ADI_LENSPEC;


/*}
** Name: ADI_FI_ID - Function instance id.
**
** Description:
**	Used to hold a function instance id, which is a specific operation on
**	specific datatype(s), yielding a specific datatype.  Let me attempt to
**	clarify this.  ADF knows about certain functions and operators, such as
**	"concat", or "+" for instance.  Each operator and function known to ADF
**	is assigned a unique operator id.  Now, each of these operator ids is
**	capable of being used in several different ways.  For example, the
**	operator id corresponding to the "char" function can be used to convert
**	a float into a character, an integer into a character, a date into a
**	character, etc.  Or, operator id corresponding to the "+" operator can
**	be used to add 2 integers, concatenate 2 texts, etc.  Each one of these
**	uses is called a "function instance," and has a unique function
**	instance id assigned to it.  What uniquely identifies a function
**	instance is the operator id, the number of operands, their datatypes,
**	and the datatype of the result.
**
** History:
**      19-feb-86 (thurston)
**          Initial creation.
**      03-mar-86 (thurston)
**          Put the (ADI_FI_ID) cast on #defines to please lint.
**	16-may-86 (thurston)
**	    Changed the name of constant ADI_NOCOMP to ADI_NOCMPL because it
**	    conflicted with ADI_NOCOERCION (8 character uniqueness).
**	19-may-86 (thurston)
**	    Changed the constant ADI_NOCMPL to be ADI_NOFI, which is more
**	    generic.
**	02-oct-86 (thurston)
**	    Changed this typedef from an i4 to an i2.
**	20-Oct-2008 (kiria01) b121098
**	    Added ADI_NILCOERCE
*/

typedef	    i2	    ADI_FI_ID;

#define     ADI_NOFI       ((ADI_FI_ID) -1) /* This value will signify the  */
					    /* absence of an f.i. id.  For  */
					    /* instance, it will appear in  */
					    /* an ADI_FI_DESC to signify    */
					    /* that there is no		    */
					    /* "complement" f.i. for the    */
					    /* f.i. being described.	    */

#define     ADI_NOCOERCION ((ADI_FI_ID) -2) /* This value will be returned  */
					    /* by the adi_ficoerce()	    */
					    /* routine to signify that	    */
					    /* there is no automatic	    */
					    /* conversion (thus no f.i.)    */
					    /* between the two supplied	    */
					    /* datatypes.  Also returned by */
					    /* adi_cpficoerce() to signify  */
					    /* no copy-coercion.	    */
#define     ADI_NILCOERCE ((ADI_FI_ID) -3)  /* This value will be used to   */
					    /* indicate that a stored FI_ID */
					    /* in a coercion context means  */
					    /* that no coercion is needed.  */
					    /* Usually, a mismatch in types */
					    /* will require a coercion but  */
					    /* if dtfamily processing has   */
					    /* been performed then this     */
					    /* value means that the types   */
					    /* are really compatible.       */


/*}
** Name: ADI_FI_DESC - Structure to hold full description of a function
**                     instance.
**
** Description:
**	Each function instance known by ADF has an ADI_FI_DESC structure which
**	gives a full description of the function instance.  This contains the
**	function instance id,  the function instance id of the complement
**	function instance (for the comparisons), the type of function instance,
**	whether the function's result can vary or not, the operator id of the
**	function, the method for calculating the length of the result, the
**	length to make the DB_DATA_VALUE used for work space by the aggregates,
**	the nullable pre-instruction, the number of arguments (or input
**	operands), and the datatype ids for the result and all arguments.
**	Internally, ADF will maintain a table of these function instance
**	descriptions.  This table will be kept grouped by function instance
**	type, and by operator id.
**
**	Note: An argument may be DB_ALL_TYPE.  This means that any type is a
**	valid input type for this function instance and the output type may be
**	determined by calling adi_res_type().  The rest of ADF doesn't know
**	about this type, so don't store it in (for example) a DB_DATA_VALUE.
**
** History:
**      22-jan-86 (thurston)
**          Initial creation.
**	16-may-86 (thurston)
**	    Removed the _ADI_FI_DESC from the front of the typedef because it
**	    was not unique to 8 chars (_ADI_FI_TAB) and was never used.
**	19-jun-86 (thurston)
**	    Redesigned this typedef to cover all needs for an externally
**	    visable function instance description.  Specifically, I have
**	    rearranged the members and added two new members: .adi_fitype
**	    and .adi_agwsdv_len.  I also moved the #defines for ADI_COERCION,
**	    ADI_OPERATOR, ADI_NORM_FUNC, and ADI_AGG_FUNC, as well as the
**	    new ADI_COMPARISON, into this typedef from the <adfint.h> internal
**	    header file.
**	30-jun-86 (thurston)
**	    Added the .adi_agprime member to describe whether an aggregate f.i.
**	    is "prime" or not.  (See the comment in the typedef for the meaning
**	    of a prime aggregate.)
**	02-jul-86 (thurston)
**	    Re-numbered the function instance type constants so that none of
**	    them are zero.
**	11-mar-87 (thurston)
**	    Removed the .adi_agprime member, since ADF will no longer be
**	    concerned with `PRIME', or `DISTINCT' aggregates.  In its place, I
**	    have added the .adi_npre_instr field, to handle nullable datatypes.
**	24-jun-87 (thurston)
**	    Added ADI_COPY_COERCION.
**	18-jan-88 (thurston)
**	    Added the .adi_fiflags field to this structure, along with the two
**	    constants, ADI_F0_NOFIFLAGS and ADI_F1_VARFI.  Also, changed the
**	    definition of several fields in this structure in the anticipation
**	    that it may someday become part of the database.  (Also, to cut down
**	    on the amount of storage needed to hold the f.i. table.)
**	    Specifically:
**		changed		    from    to
**		-------		    ----    --
**		.adi_fitype	    i4	    i1
**		.adi_agwsdv_len	    i4	    i4
**		.adi_npre_instr	    i4	    i2
**		.adi_numargs	    i4	    i2
**	14-nov-89 (fred)
**	    Added ADI_F2_MULTIPASS and ADI_F4_WORKSPACE definitions.
**	    ...for large object support
**	16-jan-01 (inkdo01)
**	    Added ADI_F16_OLAPAGG flag.
**	9-oct-01 (inkdo01)
**	    After discussion with Don Criley (Openroad development), it was agreed to
**	    drop adi_dt1/1 added by mcgem01.
**	27-may-02 (inkdo01)
**	    Added ADI_F32_NOTVIRGIN flag.
**	24-jul-02 (inkdo01)
**	    Once more for emphasis - adi_dt2 is dropped after it had been readded by 
**	    change 457225, an integration of a Don Criley change that pre-dated the 
**	    decision to remove adi_dt1/2.
**	22-Mar-2005 (schka24)
**	    "Indirect" flag is meaningless, change comment here.
**	12-Mar-2006 (kschendel)
**	    Playing with UDF's, need a way to set null IGNORE for user FI's.
**	    Also need a couple flags for counted-vector call and varargs.
**	10-Aug-2007 (kiria01) b118254
**	    Define ADI_F4096_LEGACY for legacy function support (A legacy function
**	    is one where there may still be CX data that would dispatch to it but
**	    we have an updated version of the function, or rather its FI, that is
**	    to be used in preference.)
**	21-Oct-2007 (kiria01) b119354
**	    Extend the legacy function support by splitting into language
**	    support renaming ADI_F4096_LEGACY to ADI_F4096_SQL_CLOAKED and
**	    adding ADI_F8192_QUEL_CLOAKED. This allows us to add or 'remove'
**	    FI_DEFNs from without destablising Quel in the process.
**      13-May-2010 (horda03) B123704
**          Added ADI_F65536_PAR1MATCH and ADI_F131072_PAR2MATCH. This required
**          changing adi_fiflags to an i4.
*/

typedef struct
{
    ADI_FI_ID       adi_finstid;        /* Function instance id.
					*/
    ADI_FI_ID       adi_cmplmnt;        /* Function instance id for complement
                                        ** f.i., or ADI_NOFI if one does not
					** exist.  *** NOTE: Only the comparison
                                        ** operators ("=", "!=", "<", "<=", ">",
                                        ** ">=", "like", "not like", "is null",
					** and "is not null") will have a
                                        ** complement f.i.  (That is, if
                                        ** .adi_fitype == ADI_COMPARISON.)
					*/
    i1		    adi_fitype;		/* Function instance type.  One of
					** the following symbolic constants:
					*/
#define                 ADI_COMPARISON	    1
#define                 ADI_OPERATOR	    2
#define                 ADI_AGG_FUNC	    3
#define                 ADI_NORM_FUNC	    4
#define                 ADI_PRED_FUNC  	    5
#define                 ADI_COERCION	    6
#define                 ADI_COPY_COERCION   7

    u_i4	    adi_fiflags;	/* Function instance flags.  The
					** following bits have meaning:
					*/
#define                 ADI_F0_NOFIFLAGS    0	/* No flags set. */

#define                 ADI_F1_VARFI	    1	/* Variable function instance.
					** This means that this f.i.
					** may return different
					** results when evaluated
					** multiple times with the
					** same set of inputs.
					*/
#define			ADI_F2_COUNTVEC	    2	/*
					** Call FI with (cb,count,vect)
					** where count is # of args in vect,
					** and vect is DB_DATA_VALUE *[] ptrs
					** to params.  [0] is the
					** result ptr.  Note that FI's with
					** more than 5 args including the
					** result MUST be countvec FI's.
					** (See adeexecute.c)
					*/

#define			ADI_F4_WORKSPACE    4	/*
					** Indicates that the f.i. needs
					** some workspace.  This is not
					** necessarily set for aggregate
					** operations, although it is
					** true there, as well.  It is
					** primarily for non-aggregate
					** fi's which require a
					** workspace to operate, which
					** is ``unusual.''
					*/
#define		        ADI_F8_INDIRECT	    8	/*
					** No longer used; ignored if
					** set.  (set by spatial??)
					*/
#define			ADI_F16_OLAPAGG	   16	/*
					** Indicates that the f.i. is a 
					** statistical aggregate (from
					** the OLAP enhancement) and 
					** must execute the ADE_OLAP
					** opcode in the FINIT code.
					*/
#define			ADI_F32_NOTVIRGIN  32	/*
					** Even though this is a parm-less
					** f.i., it should NOT be added
					** to the VIRGIN segment. Functions
					** such as random() should produce
					** distinct values for each row
					** processed by the containing CX.
					*/

#define			ADI_F64_ALLSKIPUNI  64	/* If ADI_F64_ALLSKIPUNI is
					** specified, a *toall function
					** instance will not get called for
					** nchar/nvchar parameters
					*/
#define			ADI_F128_VARARGS 128	/* Set if numargs is the
					** MINIMUM number of args that
					** the fun can have, it can have
					** indefinitely more up to
					** ADI_MAX_OPERANDS.  The args
					** beyond the minimum are "ANY" type,
					** and are passed as-is with no
					** type coercion.
					*/

#define			ADI_F256_NONULLSKIP 256	/* For user FI definition only!
					** Doesn't fit in adi_fiflags, but
					** since IIADD_FI_DFN forgot some way
					** to indicate null preinst we need a
					** flag.  Set if user wants IGNORE
					** null handling instead of setskip.
					*/

#define			ADI_F512_RES1STARG 512	/* For dtfamily result is 1st arg */
#define			ADI_F1024_RES2NDARG 1024	/* For dtfamily result is 2nd arg */
#define			ADI_COMPAT_IDT	    2048 /* 'compatible' input dt ok */

#define			ADI_F4096_SQL_CLOAKED 4096 /* Not for SQL FI
					** This FI will not be resolved to
					** when in SQL mode. The resolver
					** will skip this when in SQL but
					** there could be 'legacy' instances
					** in CX data that will map to it.
					*/
#define			ADI_F8192_QUEL_CLOAKED 8192 /* Not for Quel FI
					** This FI will not be resolved to
					** when in Quel mode. The resolver
					** will skip this when in Quel but
					** there could be 'legacy' instances
					** in CX data that will map to it.
					*/
#define			ADI_F16384_CHKVARFI 16384 /* A VARFI that can give
					** an indication of whether actually
					** constant or not
					*/
#define			ADI_F32768_IFNULL 32768/* Specifies that a function is ifnull
					**Or an ifnull equivelant.
					*/
#define                 ADI_F65536_PAR1MATCH 65536 /* Parameter 1 must match
                                        ** function's 1st parameter type.
                                        */
#define                 ADI_F131072_PAR2MATCH 131072 /* Parameter 2 must match
                                        ** function's 2nd parameter type.
                                        */
    ADI_OP_ID       adi_fiopid;         /* Operator id for this f.i.
					*/
    ADI_LENSPEC     adi_lenspec;        /* Method for computing result length.
					*/
    i4              adi_agwsdv_len;     /* If this function instance is an
                                        ** aggregate (i.e. if .adi_fitype ==
                                        ** ADI_AGG_FUNC) then this is the length
                                        ** for the DB_DATA_VALUE to be used as
                                        ** work space for the aggregate.  If the
                                        ** function instance is not an
                                        ** aggregate, this is unused.
                                        */
    i2              adi_npre_instr;     /* "Nullable PRE-INSTRuction."  If one
					** of the input operands to this f.i. is
					** of a nullable datatype, this field
					** instructs ADF which `pre-instruction'
					** (if any) to use prior to the actual
					** f.i.  The ade_instr_gen() routine
					** will actually compile this CX special
					** instruction into the CX prior to the
					** instruction for the f.i.  The
					** adf_func() routine will also look at
					** this field and take the appropriate
					** action.  By using this field properly
					** in ADF, the low-level f.i. processing
					** code can remain mostly unaware of
					** nulls.  This has 3 main advantages:
					** (1) Keeps code size down.  (2) Keeps
					** performance high for non-nullable
					** types.  (3) Cheaper to develop.  All
					** of these advantages are much desired.
					**
					** Currently, the possible
					** pre-instructions are:
					**
					**   (These CX special instructions are
					**    defined in <ade.h> with the other
					**    CX special instructions.)
					**
					** ADE_0CXI_IGNORE:  Use no pre-
					**   instruction.  F.i. knows how to
					**   handle null values.  This is used
					**   by functions such as `ifnull()',
					**   and operators like `is null'.
					**
					** ADE_1CXI_SET_SKIP:  This pre-
					**   instruction says, if a null value
					**   is found, set the result to null,
					**   and do not execute the f.i.  If no
					**   null value is found, clear the
					**   null value bit in the result then
					**   execute the f.i.  This is used by
					**   most f.i.'s, since most of them
					**   are defined as returning a null
					**   value if any of the inputs are
					**   null.
					**
					** ADE_2CXI_SKIP:  This pre-instruction
					**   says, if a null value is found, do
					**   not execute the f.i.  This gives a
					**   way for a f.i. to skip any null
					**   values.  This is used primarily by
					**   aggregate funcs that do not wish
					**   to `count' or `sum', etc. null
					**   values.
					*/
    i2              adi_numargs;        /* Number of args (or input operands).
					*/
    DB_DT_ID        adi_dtresult;       /* Datatype for result.  Always the
					** positive (not-null) type for builtin
					** FI's, nullability is calculated.
					** For UDF's defined with NONULLSKIP,
					** user can specify the negative
					** nullable type here.
					*/
    DB_DT_ID        adi_dt[ADI_MAX_OPERANDS];/* Datatype for each argument 
					*/
}   ADI_FI_DESC;


/*}
** Name: ADI_FI_TAB - Table of function instance descriptions.
**
** Description:
**      This structure defines a table of function instance descriptions.
**      It is returned by the adi_fitab() routine to give the caller all
**      of the function instances associated with a specific operator id.
**      Each element in this table is an ADI_FI_DESC, and contains the
**      function instance id, the operator id of the function, the number
**      of arguments or operands, the datatype ids for all arguments and
**      the result, the method for calculating the length of the result,
**      and in some cases, the function instance id of the complement
**      function instance.
**
** History:
**     20-feb-86 (thurston)
**          Initial creation.
**	16-may-86 (thurston)
**	    Removed the _ADI_FI_TAB from the front of the typedef because it
**	    was not unique to 8 chars (_ADI_FI_DESC) and was never used.
**	19-may-86 (thurston)
**	    Changed the definition of the .adi_tab_fi member to be a pointer
**	    to an ADI_FI_DESC instead of an array of ADI_FI_DESC's, which was
**	    incorrect.
**	27-jun-86 (thurston)
**	    Changed .adi_lntab to be a i4  instead of an i2.
*/

typedef struct
{
    i4              adi_lntab;          /* # elements in table */
    ADI_FI_DESC     *adi_tab_fi;        /* Pointer to table of function instance
                                        ** descriptions (ADI_FI_DESC's).
                                        */
}   ADI_FI_TAB;



/*}
** Name: ADI_RSLV_BLK - Structure for calling adi_resolve
**
** Description:
**	A pointer to one of these will be supplied to the "adi_resolve()"
**	routine which does datatype resolution.
**
** History:
**      28-may-89 (jrb)
**	    Initial creation.
*/
typedef struct _ADI_RSLV_BLK
{
    ADI_OP_ID		adi_op_id;	    /* The operator being resolved.
					    */
    ADI_FI_DESC		*adi_fidesc;	    /* A place to store a ptr to the
					    ** resulting fi descriptor.
					    */
    i2			adi_num_dts;	    /* The number of datatypes being
					    ** passed in.
					    */
    DB_DT_ID		adi_dt[ADI_MAX_OPERANDS];/* The datatypes, if any.
					    */
} ADI_RSLV_BLK;


/*}
** Name: ADC_KEY_BLK - The function call block used to call the
**                     adc_keybld() routine.
**
** Description:
**      A pointer to one of these structures will be supplied to the
**      "adc_keybld()" routine.  This structure will contain the type
**      of key that is being built, and three DB_DATA_VALUES.  One of
**      these will be the value to build the key from, and the other
**      two will be where the key (low, high, or both) will be placed.
**      The type of key that was built will also be returned via this
**      block.
**
** History:
**     18-feb-86 (thurston)
**          Initial creation.
**	09-jun-86 (ericj)
**	    Changed field names to conform to adc_keybld() specification.
**	    Also changed adc_opkey type to an ADI_OP_ID as requested by
**	    seputis for OPF interface.
**	10-jun-86 (ericj)
**	    Added a pointer to the ADF_CB in the ADF_KEY_BLK and added the
**	26-jul-86 (ericj)
**	    Removed the pointer to the ADF_CB, because a pointer to this
**	    structure is now passed to all routines.
**	01-jun-88 (thurston)
**	    Added the .adc_escape and .adc_isescape fields.
**	25-Jun-2008 (kiria01) SIR120473
**	    Move adc_isescape into new adc_pat_flags.
*/

typedef struct _ADC_KEY_BLK
{
    DB_DATA_VALUE   adc_kdv;            /* This is the data value to
                                        ** build a key for.
                                        */
    ADI_OP_ID	    adc_opkey;		/* Key operator used to tell
                                        ** adc_keybld() what kind of
                                        ** comparison operation this key
					** is being built for.
                                        */
    i4              adc_tykey;          /* Type of key created by
                                        ** adc_keybld().  Note, that the
					** following ordering is required
					** by the OPF module for it to
					** work correctly.
                                        */

#define                 ADC_KNOMATCH    1   /* The value given can't possibly*/
                                            /* match any value of the type   */
                                            /* and length being keyed, so no */
                                            /* scan of the relation should   */
                                            /* be done.  (No low or high key */
					    /* was built.)                   */

#define                 ADC_KEXACTKEY   2   /* The key formed can match      */
                                            /* exactly one value from the    */
                                            /* relation.  The execution      */
                                            /* phase should seek to this     */
                                            /* point and scan forward until  */
                                            /* it is sure that it has        */
                                            /* exhausted all possible        */
                                            /* matching values.  (The key    */
					    /* formed was placed in the low  */
					    /* key slot.)                    */

#define                 ADC_KRANGEKEY   3   /* Two keys were formed:  One    */
					    /* representing the lowest value */
					    /* in the relation that can be   */
					    /* matched, and the other which  */
					    /* represents the highest value  */
					    /* that can be matched.  The     */
					    /* execution phase should seek   */
					    /* to the point matching the low */
					    /* key, then scan forward until  */
					    /* it has exhausted all values   */
					    /* that might be less than or    */
					    /* equal to the high key.  (Both */
					    /* low and high keys were        */
					    /* formed.)                      */

#define                 ADC_KHIGHKEY    4   /* The key formed can match all  */
					    /* values in the relation that   */
					    /* are less than or equal to the */
					    /* key; that is, the key         */
					    /* represents the highest value  */
					    /* that can be matched.  The     */
					    /* execution phase should start  */
					    /* at the beginning of the       */
					    /* relation and seek forward     */
					    /* until it has exhausted all    */
					    /* values that might be less     */
					    /* than or equal to the key.     */
					    /* (Only the high key was        */
					    /* formed.)                      */

#define                 ADC_KLOWKEY     5   /* The key formed can match all  */
                                            /* values in the relation that   */
                                            /* are greater than or equal to  */
					    /* the key; that is, the key     */
					    /* represents the lowest value   */
					    /* that can be matched.  The     */
					    /* execution phase should seek   */
					    /* to this point and scan forward*/
					    /* from there.  (Only the low key*/
					    /* was formed.)                  */

#define			ADC_KNOKEY	6   /* The boolean expression was too*/
					    /* complex for the optimizer to  */
					    /* use.  Note, this value is     */
					    /* never returned by adc_keybld()*/
					    /* routine.  It is used only by  */
					    /* the optimizer.		     */

#define                 ADC_KALLMATCH   7   /* The value given will match    */
                                            /* any value of the type and     */
                                            /* length being keyed, so a full */
                                            /* scan of the relation should   */
                                            /* be done.  (No low or high key */
					    /* was built.)                   */

    DB_DATA_VALUE    adc_lokey;	        /* If adc_tykey = ADC_KEXACTKEY or
                                        ** ADC_KLOWKEY, this will be the key
                                        ** built.  If adc_tykey = ADC_KRANGEKEY,
                                        ** this will be the low key built.
                                        ** In all other cases, this will be
                                        ** undefined.
                                        */
    DB_DATA_VALUE    adc_hikey;		/* If adc_tykey = ADC_KHIGHKEY, this
                                        ** will be the key built.  If
                                        ** adc_tykey = ADC_KRANGEKEY, this
                                        ** will be the high key built.
                                        ** In all other cases, this will be
                                        ** undefined.
                                        */
    UCS2	    adc_escape;		/* adc_escape is the escape char that
					** the user has specified for the LIKE
					** family of operators.  adc_escape
					** has valid info in it *ONLY* if bit
					** AD_PAT_HAS_ESCAPE set in adc_pat_flags.
                                        */
    u_i2	    adc_pat_flags;	/* Pattern flags == like_modifiers
					** This will be seen in the parse tree
					** too in operator nodes called be
					** pst_pat_flags. Although these are 16
					** values, they are widened in the 
					** pattern code so the flags here are
					** represented in the full width and
					** note should be taken that the upper
					** 16 bits are reserved */
#define AD_PAT_ISESCAPE_MASK    0x00000003 /* Mask for checking escape bits */
#define	    AD_PAT_NO_ESCAPE	0   /* LIKE op, no escape char given    */
#define	    AD_PAT_HAS_ESCAPE	1   /* valid escape char in pst_escape  */
#define	    AD_PAT_DOESNT_APPLY	2   /* not a LIKE operator		    */
#define	    AD_PAT_HAS_UESCAPE	3   /* valid Unicode escape char in pst_escape  */
                                        /* If HAS_ESCAPE, then adc_escape holds the escape
					** char that the user specified for the LIKE
					** operator or one of its variants. If this is
					** not set, then the user either has not given an
					** escape clause, or this is not a LIKE op. */
#define AD_PAT_WO_CASE		0x00000004
					/* If set the LIKE is to be forced to be
					** WITHOUT CASE. */
#define AD_PAT_WITH_CASE	0x00000008
					/* If set the LIKE is to be forced to be
					** WITH CASE. If neither AD_PAT_WO_CASE or
					** AD_PAT_WITH_CASE set, defer to collation
					** setting. If both set, AD_PAT_WO_CASE taken. */
#define AD_PAT_DIACRIT_OFF	0x00000010
					/* If set, ignore diacriticals in unicode. */
#define AD_PAT_BIGNORE		0x00000020
					/* If set ignore blanks ala Quel. Usually,
					** derived from the C datatype being present. */
#define AD_PAT_UNUSED1		0x00000040
#define AD_PAT_UNUSED2		0x00000080
#define AD_PAT_FORM		0x0000FF00 /* Controls the FORM of pattern. Presently
					** this will be LIKE and the positional key
					** words */
#define AD_PAT_FORM_NEGATED	0x00008000	/* NOT xxxx */
#define AD_PAT_FORM_MASK   (AD_PAT_FORM^AD_PAT_FORM_NEGATED)
#define AD_PAT_FORMS \
_DEFINE(LIKE,		0x0000, "LIKE")\
_DEFINE(BEGINNING,	0x0100, "BEGINNING")\
_DEFINE(ENDING,		0x0200, "ENDING")\
_DEFINE(CONTAINING,	0x0300,	"CONTAINING")\
_DEFINE(SIMILARTO,	0x0400,	"SIMILAR TO")\
_DEFINEEND
#define AD_PAT_RESERVED		0xFFFF0000
					
}   ADC_KEY_BLK;

#define _DEFINE(n,v,s) AD_PAT_FORM_##n=v,
#define _DEFINEEND
enum AD_PAT_FORMS_enum {AD_PAT_FORMS AD_PAT_FORM_MAX,AD_PAT_FORM_MAXNEG=AD_PAT_FORM_MAX|AD_PAT_FORM_NEGATED};
#undef _DEFINEEND
#undef _DEFINE

/*}
** Name: ADF_FN_BLK - The function call block used to call the
**                    adf_func() routine.
**
** Description:
**	A pointer to one of these structures will be supplied to the
**	"adf_func()" routine.  This structure will contain a pointer to the ADF
**	session control block.  (This will be used only if the information
**	therein is required to do the particular operation adf_func() is being
**	asked to do, otherwise it will be ignored; so a NULL pointer will be
**	fine if the caller knows he is not doing anything with date or money
**	formatting, string manipulation, or anything which requires the decimal
**	character.)  The structure will also contain the function instance id
**	to perform, and DB_DATA_VALUEs for the operands and result.
**
** History:
**	20-feb-86 (thurston)
**          Initial creation.
**	23-jun-88 (thurston)
**	    Added the .adf_isescape and .adf_escape members.
**	15-Oct-2008 (kiria01) b121061
**	    Rearranges the structure of the result and paramters so that
**	    the can be handled in a loop. The old member names have been
**	    switched to macros for legacy support.
**	23-Sep-2009 (kiria01) b122578
**	    Added adf_fi_desc to streamline lookups and allow the passing of
**	    modified fi_desc as from dtfamily processing. Added adf_dv_n
**	    so that arg count can be stated and not just inferred.
**	    Moved the block to the end to allow for varargs more easily
*/

typedef struct _ADF_FN_BLK
{
    ADI_FI_DESC	    *adf_fi_desc;	/* Optional FI_DESC. Passed when this
					** is available to avoid re-lookup.
					** Pass NULL if not used.
					*/
    ADI_FI_ID       adf_fi_id;          /* Function instance id for the
                                        ** operation being requested.
                                        */
    UCS2	    adf_escape;		/* adf_escape is the escape char that
					** the user has specified for the LIKE
					** family of operators.  adf_escape
					** has valid info in it *ONLY* if bit
					** AD_PAT_HAS_ESCAPE set in adf_pat_flags.
					*/
    u_i2	    adf_pat_flags;	/* Pattern flags == like_modifiers
					** This will be seen in the parse tree
					** too in operator nodes called be
					** pst_pat_flags. Although these are 16
					** values, they are widened in the 
					** pattern code so the flags here are
					** represented in the full width and
					** note should be taken that the upper
					** 16 bits are reserved
					** SEE ADC_KEY_BLK declaration for
					** constant definitions */
    u_i4	    adf_dv_n;		/* Arg count */
    DB_DATA_VALUE   adf_dv[ADI_MAX_OPERANDS]; /* Result and parameters. [0] is
					** the result slot and the rest are
					** the params. Note that the value of
					** ADI_MAX_OPERANDS is one MORE than
					** the max number of parameters. This
					** is because the symbol refers to
					** CX data operators and not to ADI
					** function calls. */

#define adf_r_dv adf_dv[0]		/* Legacy support - Result data value. */
#define adf_1_dv adf_dv[1]		/* Legacy support - 1st param */
#define adf_2_dv adf_dv[2]		/* Legacy support - 2nd param */
#define adf_3_dv adf_dv[3]		/* Legacy support - 3rd param */
#define adf_4_dv adf_dv[4]		/* Legacy support - 4th param */
#define adf_isescape adf_pat_flags	/* Legacy support - previously boolean i1 */


}   ADF_FN_BLK;


/*}
** Name: ADI_DT_RTREE - Table of function instance descriptions.
**
** Description:
**      This structure defines data types and functions used to deal with
**      an R-tree index.  Since a user can define an R-tree on any UDT,
**      this routine, in returning the information, verifies that the user's
**      datatype can be used as an R-tree index.
**      There must be a function instance: nbr(obj_dt, range_dt)->nbr_type. 
**      This gives the dimension of obj_dt, the nbr_type, the hilbertsize, 
**      and the range_dt.  Given those values, then the other nbr functions
**      are located: hilbert(nbr), nbr overlaps nbr, nbr inside nbr, and
**      nbr union nbr.
**
** History:
**     15-apr-1997 (shero03)
**          Initial creation.
*/

typedef struct
{
    u_i2		adi_hilbertsize;	/* hilbert size */
    u_i2		adi_dimension;		/* dimension of base dt */
    DB_DT_ID		adi_obj_dtid;		/* datatype of base object */
    DB_DT_ID		adi_nbr_dtid;		/* datatype of nbr object */
    DB_DT_ID		adi_range_dtid;		/* datatype of range object */
    ADI_FI_ID		adi_nbr_fiid;		/* function id of adi_nbr */
    ADI_FI_ID		adi_hilbert_fiid;	/* function id of adi_hilbert */
    char		adi_range_type;		/* I for integer, F for float */
    
    DB_STATUS (*adi_nbr) (          		/* nbr(obj, range)	*/
    	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*dv_object,
	DB_DATA_VALUE	*dv_range,
	DB_DATA_VALUE	*dv_result);
    
    DB_STATUS (*adi_hilbert) (         		/* hilbert(nbr)	*/
    	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*dv_object,
	DB_DATA_VALUE	*dv_result);
    
    DB_STATUS (*adi_nbr_overlaps) (    		/* overlaps(nbr, nbr)	*/
    	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*dv_nbr1,
	DB_DATA_VALUE	*dv_nbr2,
	DB_DATA_VALUE	*dv_result);		/* i2 0=false, 1=true	*/
    
    DB_STATUS (*adi_nbr_inside) (    		/* inside(nbr, nbr)	*/
    	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*dv_nbr1,
	DB_DATA_VALUE	*dv_nbr2,
	DB_DATA_VALUE	*dv_result);		/* i2 0=false, 1=true	*/
    
    DB_STATUS (*adi_nbr_union) (    		/* union(nbr, nbr)	*/
    	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*dv_nbr1,
	DB_DATA_VALUE	*dv_nbr2,
	DB_DATA_VALUE	*dv_result);		/* smallest nbr that contains	*/
						/* nbr1 and nbr2	*/
    
    DB_STATUS (*adi_nbr_perimeter) (      	/* perimeter(nbr)	*/
    	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*dv_object,
	DB_DATA_VALUE	*dv_result);		/* f8 perimeter		*/

}   ADI_DT_RTREE;


/*}
** Name: ADF_AG_STRUCT - Aggregate processing work space.
**
** Description:
**      This structure is used during the processing of aggregate
**      functions.  A pointer to one of these structures is passed
**      in to the adf_agbegin() routine, which initializes it for
**      the aggregation which is about to begin.  Then, for each
**      data value to be aggregated, the adf_agnext() routine is
**      called with a pointer to this structure (among other things).
**      That routine will use the space in the ADF_AG_STRUCT to
**      "work on" the aggregation.  Finally, when all data values
**      have been aggregated, a call to adf_agend() (again, with a
**      pointer to this structure, among other things) will finalize
**      the aggregation and supply the caller with the resulting
**      data value.
**
** History:
**     20-feb-86 (thurston)
**          Initial creation.
**     27-feb-86 (thurston)
**          Added the adf_agnx and adf_agnd members to help improve
**	    improve performance of the aggregate processing routines.
**	28-jul-86 (thurston)
**	    Changed the .adf_agnx and .adf_agend members to be pointers to
**	    routines that return DB_STATUS instead of i4.
**	24-sep-99 (inkdo01)
**	    Swiped 2 elements from int vector and added 1 to f8 to allow
**	    workspace needed by a few of the binary statistical aggregates.
**	14-aug-2000 (somsa01)
**	    Added one more array entry to adf_agirsv.
**	7-Nov-2005 (schka24)
**	    Add an i8 into the reserves.
*/

typedef struct _ADF_AG_STRUCT
{
    ADI_FI_ID       adf_agfi;           /* Function instance to be
                                        ** used for this aggregation.
                                        */
    DB_STATUS       (*adf_agnx)();	/* Ptr to specific function that
					** will compile another data
					** value into the aggregation
					** for the function instance id
					** in .adf_agfi.  This will get
					** filled in by adf_agbegin(),
					** then used by adf_agnext().
					*/
    DB_STATUS       (*adf_agnd)();	/* Ptr to specific function that
					** will calculate the result of
					** the aggregation for the
					** function instance id
					** in .adf_agfi.  This will get
					** filled in by adf_agbegin(),
					** then used by adf_agend().
					*/
    i4              adf_agcnt;          /* # data values currently
                                        ** aggregated.
                                        */
    DB_DATA_VALUE   adf_agwork;         /* Caller supplied DB_DATA_VALUE
                                        ** that might be used for temporary
                                        ** work space during the aggregation.
                                        */
    i4              adf_agirsv[3];      /* Reserved work space. */
    i8		    adf_ag8rsv;		/* Reserved work space */
    f8              adf_agfrsv[5];      /* Reserved work space. */
}   ADF_AG_STRUCT;


/*}
** Name: ADF_AG_OLAP - Workspace for statistical (OLAP) aggregates.
**
** Description:
**      This structure is used for the statistical aggregates added
**	with the OLAP enhancements. It is one of the parameters to 
**	the ADE_SMAIN function instances and contains enough fields
**	to compute the values needed for the final computation 
**	(performed by the ADE_OLAP opcode). 
**
** History:
**	9-jan-01 (inkdo01)
**	    Written.
*/

typedef struct _ADF_AG_OLAP
{
    f8		adf_agfrsv[5];		/* vector of 5 f8's */
    i4		adf_agcnt;		/* count of inputs */
} ADF_AG_OLAP;


/*}
** Name: ADF_DBMSINFO - A dbmsinfo() request.
**
** Description:
**      An array of these structures will be handed into adg_startup() at server
**	startup time.  Each one of these structures represents a recognized
**	dbmsinfo() request.  Along with each request is a ptr to a function to
**	call to process this request:  .dbi_func().  .dbi_func() can take either
**	0 or 1 input DB_DATA_VALUEs (whose datatype must be one of the intrinsic
**	datatypes:  DB_INT_TYPE, DB_FLT_TYPE, DB_CHR_TYPE, DB_CHA_TYPE,
**	DB_TXT_TYPE, or DB_VCH_TYPE).  Its output will also be a DB_DATA_VALUE
**	with the same datatype restriction.  Note that the actual dbmsinfo()
**	routine in ADF will take care of converting the result into VARCHAR(32),
**	which by definition is what dbmsinfo() returns.  Also contained in each
**	ADF_DBMSINFO structure will be a `length specification' for calculating
**	the length of the result of .dbi_func().
**
**	Here is the interface specification for every .dbi_func():
**
**	DB_STATUS .dbi_func(dbi, dv1, dvr, err)
**		  ADF_DBMSINFO	    *dbi;	Ptr to dbmsinfo() request entry.
**		  DB_DATA_VALUE	    *dv1;	Ptr to input data value, if
**						expecting one.  Otherwise, this
**						can be supplied as NULL.
**		  DB_DATA_VALUE	    *dvr;	Ptr to result data value. Length
**						and type should be properly
**						supplied, and the .db_data ptr
**						will point to location to put
**						the result.
**		  DB_ERROR	    *err;	Ptr to error block, which this
**						routine should fill in if return
**						status is not E_DB_OK.
**
** History:
**	02-apr-87 (thurston)
**          Initial creation.
*/

typedef struct _ADF_DBMSINFO
{
    DB_STATUS       (*dbi_func)();          /* Func to call to process this
					    ** dbmsinfo() request.
                                            */
    char            dbi_reqname[ADF_MAXNAME];/* Request name, NULL terminated if
					    ** shorter than DB_MAXNAME.
                                            */
    i4              dbi_reqcode;            /* Request code, which may be used
					    ** internally instead of request
					    ** name.
					    */
    i4              dbi_num_inputs;         /* # of DB_DATA_VALUE inputs to this
                                            ** request.  At present, this can
                                            ** only be zero or one, since more
                                            ** than that would mean a dbmsinfo()
                                            ** function with more than two
                                            ** arguments.
					    */
    ADI_LENSPEC	    dbi_lenspec;	    /* Length specification for result
					    ** of .dbi_func().
					    */
    DB_DT_ID	    dbi_dtr;		    /* Datatype of .dbi_func() result.
					    ** This may be any intrinsic type,
					    ** and the dbmsinfo() routine will
					    ** take care of converting it into
					    ** a VARCHAR(32), which by defini-
					    ** tion is what dbmsinfo() always
					    ** returns.
					    */
    DB_DT_ID	    dbi_dt1;		    /* If .dbi_func() has a
                                            ** DB_DATA_VALUE input, this is the
                                            ** datatype it is expecting.
					    */
}   ADF_DBMSINFO;


/*}
** Name: ADF_ERRMSG_OFF - MACRO to temporary turn OFF ADF error
**                        message formatting option.
**
** Description:
**      ADF_ERRMSG_OFF accepts ADF session control block as the argument and
**      convert the adf_errcb->ad_ebuflen to its negative value so that
**      ADF error message formatting can be turned off.  The ADF
**      error message formatting can be turned back on by calling
**      ADF_ERRMSG_ON macro.  Since error message look up requires
**      accessing slow message files, callers who only needs to know the
**      return code of an ADF routine can temporary turn off ADF error
**      message formatting to improve performance.  When ADF_ERRMSG_OFF
**      is used, the SQLSTATE error code is always set to SS5000B_INTERNAL_ERROR,
**      this is consistence with the current behavior when error message
**      formatting option is permenently turned off.
**
** Input:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.
**              .ad_ebuflen             The length, in bytes, of the buffer
**                                      pointed to by ad_errmsgp.
** Output:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.
**              .ad_ebuflen             The length, in bytes, of the buffer
**                                      pointed to by ad_errmsgp.  If > 0 will
**                                      be converted to its negative value.
** History:
**	08-mar-1993 (stevet)
**          Initial creation.
*/
#define ADF_ERRMSG_OFF(adf_scb)  \
    ((ADF_CB *)adf_scb)->adf_errcb.ad_ebuflen > 0 \
    ? ((ADF_CB *)adf_scb)->adf_errcb.ad_ebuflen = \
          -((ADF_CB *)adf_scb)->adf_errcb.ad_ebuflen \
    : FALSE


/*}
** Name: ADF_ERRMSG_ON - MACRO to turn ON ADF error message formatting option
**                       after its been temporary turned off.
**
** Description:
**      ADF_ERRMSG_ON accepts ADF session control block as the argument and
**      convert the adf_errcb->ad_ebuflen from a negative value its absolute
**      value so that ADF error message formatting can be turned back on.
**
** Input:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.
**              .ad_ebuflen             The length, in bytes, of the buffer
**                                      pointed to by ad_errmsgp.  If > 0 will
**                                      be converted to its negative value.
** Output:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.
**              .ad_ebuflen             The length, in bytes, of the buffer
**                                      pointed to by ad_errmsgp.  If < 0 will
**                                      be converted to its absolute value.
** History:
**	08-mar-1993 (stevet)
**          Initial creation.
*/
#define ADF_ERRMSG_ON(adf_scb)  \
    ((ADF_CB *)adf_scb)->adf_errcb.ad_ebuflen < 0 \
    ? ((ADF_CB *)adf_scb)->adf_errcb.ad_ebuflen = \
          -((ADF_CB *)adf_scb)->adf_errcb.ad_ebuflen \
    : FALSE

/*}
** Name: ADF_TAB_DBMSINFO - Table of dbmsinfo() requests.
**
** Description:
**      Table of dbmsinfo() requests that gets handed to adg_startup() at server
**	startup time.
**
** History:
**	02-apr-87 (thurston)
**          Initial creation.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
*/

typedef struct _ADF_TAB_DBMSINFO
{
/********************   START OF STANDARD HEADER   ********************/

    struct _ADF_TAB_DBMSINFO *tdbi_next;/* Forward pointer.
                                        */
    struct _ADF_TAB_DBMSINFO *tdbi_prev;/* Backward pointer.
                                        */
    SIZE_TYPE       tdbi_length;        /* Length of this structure: */
    i2              tdbi_type;		/* Type of structure.  For now, 10101 */
#define                 ADTDBI_TYPE     10101	/* for no particular reason */
    i2              tdbi_s_reserved;	/* Possibly used by mem mgmt. */
    PTR             tdbi_l_reserved;	/*    ''     ''  ''  ''  ''   */
    PTR             tdbi_owner;         /* Owner for internal mem mgmt. */
    i4              tdbi_ascii_id;      /* So as to see in a dump. */
					/*
					** Note that this is really a char[4]
					** but it is easer to deal with as an i4
					*/
#define                 ADTDBI_ASCII_ID  0xADFDB1

/*********************   END OF STANDARD HEADER   *********************/

    i4              tdbi_numreqs;           /* # of dbmsinfo() request entries
					    ** in the following array.
					    */
    ADF_DBMSINFO    tdbi_reqs[1];           /* Array of dbmsinfo() requests.
					    ** The [1] here is bogus ... there
					    ** may be many.
					    */
}   ADF_TAB_DBMSINFO;


/*
**  Defintions used in ADT routines.
**
*/
#define              ADT_NOCHANGE         00  /* Value has not changed.       */

#define              ADT_REPRESENTATION   01  /* The representation of an     */
                                              /* attribute changed.           */

#define              ADT_VALUE            02  /* The value of an attribute    */
                                              /* changed.                     */


/*
**  Forward and/or External function references.
*/

/*
** First typedef functions definitions that is used more than once
*/
typedef DB_STATUS ADP_LENCHK_FUNC (ADF_CB         *adf_scb,
				   i4             adc_is_usr,
				   DB_DATA_VALUE  *adc_dv,
				   DB_DATA_VALUE  *adc_rdv);
typedef DB_STATUS ADP_SEGLEN_FUNC(ADF_CB   	 *adf_scb,
				  DB_DT_ID       adc_l_dt,
				  DB_DATA_VALUE  *adc_rdv);
typedef DB_STATUS ADP_COMPARE_FUNC (ADF_CB         *adf_scb,
				    DB_DATA_VALUE  *adc_dv1,
				    DB_DATA_VALUE  *adc_dv2,
				    i4	           *adc_cmp_result);
typedef DB_STATUS ADP_KEYBLD_FUNC  (ADF_CB       *adf_scb,
				    ADC_KEY_BLK  *adc_kblk);
typedef DB_STATUS ADP_GETEMPTY_FUNC  (ADF_CB         *adf_scb,
				      DB_DATA_VALUE  *adc_emptydv);
typedef DB_STATUS ADP_KLEN_FUNC  (ADF_CB         *adf_scb,
				  i4             adc_flags,
				  DB_DATA_VALUE  *adc_dv,
				  i4        *adc_width);
typedef DB_STATUS ADP_KCVT_FUNC  (ADF_CB         *adf_scb,
				  i4             adc_flags,
				  DB_DATA_VALUE  *adc_dv,
				  char           *adc_buf,
				  i4        *adc_buflen);
typedef DB_STATUS ADP_VALCHK_FUNC  (ADF_CB         *adf_scb,
				    DB_DATA_VALUE  *adc_dv);
typedef DB_STATUS ADP_HASHPREP_FUNC  (ADF_CB         *adf_scb,
				      DB_DATA_VALUE  *adc_dvfrom,
				      DB_DATA_VALUE  *adc_dvhp);
typedef DB_STATUS ADP_HELEM_FUNC  (ADF_CB         *adf_scb,
				   DB_DATA_VALUE  *adc_dvfrom,
				   DB_DATA_VALUE  *adc_dvhg);
typedef DB_STATUS ADP_HMIN_FUNC  (ADF_CB         *adf_scb,
				  DB_DATA_VALUE  *adc_fromdv,
				  DB_DATA_VALUE  *adc_min_dvdhg);
typedef DB_STATUS ADP_HMAX_FUNC  (ADF_CB         *adf_scb,
				  DB_DATA_VALUE  *adc_fromdv,
				  DB_DATA_VALUE  *adc_max_dvhg);
typedef DB_STATUS ADP_DHMIN_FUNC  (ADF_CB         *adf_scb,
				   DB_DATA_VALUE  *adc_fromdv,
				   DB_DATA_VALUE  *adc_min_dvdhg);
typedef DB_STATUS ADP_DHMAX_FUNC  (ADF_CB         *adf_scb,
				   DB_DATA_VALUE  *adc_fromdv,
				   DB_DATA_VALUE  *adc_min_dvdhg);
typedef DB_STATUS ADP_ISMINMAX_FUNC  (ADF_CB         *adf_scb,
				      DB_DATA_VALUE  *adc_dv);
typedef DB_STATUS ADP_HG_DTLN_FUNC  (ADF_CB         *adf_scb,
				     DB_DATA_VALUE  *adc_fromdv,
				     DB_DATA_VALUE  *adc_hgdv);
typedef DB_STATUS ADP_COMPR_FUNC (ADF_CB         *adf_scb,
				  DB_DATA_VALUE  *adc_dv,
				  i4		 adc_flag,
				  char		 *adc_buf,
				  i4	 *adc_outlen);
typedef DB_STATUS ADP_MINMAXDV_FUNC  (ADF_CB         *adf_scb,
				      DB_DATA_VALUE  *adc_mindv,
				      DB_DATA_VALUE  *adc_maxdv);
typedef DB_STATUS ADP_TMLEN_FUNC  (ADF_CB         *adf_scb,
				   DB_DATA_VALUE  *adc_dv,
				   i4             *adc_defwid,
				   i4             *adc_worstwid);
typedef DB_STATUS ADP_TMCVT_FUNC  (ADF_CB         *adf_scb,
				   DB_DATA_VALUE  *adc_dv,
				   DB_DATA_VALUE  *adc_tmdv,
				   i4             *adc_outlen);
typedef DB_STATUS ADP_DBTOEV_FUNC  (ADF_CB         *adf_scb,
				    DB_DATA_VALUE  *db_value,
				    DB_DATA_VALUE  *ev_value);
typedef DB_STATUS ADP_DBCVTEV_FUNC  (ADF_CB         *adf_scb,
				     DB_DATA_VALUE  *db_value,
				     DB_DATA_VALUE  *ev_value);
typedef DB_STATUS ADP_XFORM_FUNC  (ADF_CB  *adf_cb,
				   PTR     ws);

typedef DB_STATUS ADT_WHICHPART_FUNC (ADF_CB *adfcb, DB_PART_DIM *dim_ptr,
				PTR row_ptr, u_i2 *ppartseq);
/* Typedef a pointer to an adt which-part function */
typedef ADT_WHICHPART_FUNC *ADT_WHICHPART_FUNCP;
/*
** Begining of declaring function visible from other facilities
*/
FUNC_EXTERN DB_STATUS adc_join(ADF_CB         *adf_scb,
			       i4	      for_gca,
			       i4             continuation,
			       ADF_AG_STRUCT  *workspace,
			       DB_DATA_VALUE  *result_dv,
			       i4	      *result_length);

FUNC_EXTERN DB_STATUS adc_defchk(ADF_CB         *adf_scb,
			         DB_DATA_VALUE  *adc_dv,
			         DB_DATA_VALUE  *adc_rdv);

FUNC_EXTERN VOID adg_vlup_setnull(DB_DATA_VALUE  *dv);

FUNC_EXTERN DB_STATUS adt_keytupcmp(ADF_CB   *adf_scb,
				    i4       adt_nkatts,
				    char     *adt_key,
       				    DB_ATTS  **adt_katts,
				    char     *adt_tup,
				    i4       *adt_cmp_result);

FUNC_EXTERN DB_STATUS adt_kkcmp(ADF_CB   *adf_scb,
			      i4       adt_nkatts,
       			      DB_ATTS  **adt_katts,
			      char     *adt_key1,
			      char     *adt_key2,
			      i4       *adt_cmp_result);

FUNC_EXTERN DB_STATUS adt_ixlcmp(ADF_CB   *adf_scb,
				    i4       adt_nkatts,
       				    DB_ATTS  **adt_ixkatts,
				    char     *adt_index,
       				    DB_ATTS  **adt_lkatts,
				    char     *adt_leaf,
				    i4       *adt_cmp_result);

FUNC_EXTERN DB_STATUS adt_ktktcmp(ADF_CB   *adf_scb,
				  i4       adt_nkatts,
				  DB_ATTS  **adt_katts,
				  char     *adt_tup1,
				  char     *adt_tup2,
				  i4       *adt_cmp_result);

FUNC_EXTERN DB_STATUS adt_tupcmp(ADF_CB   *adf_scb,
				 i4       adt_natts,
				 DB_ATTS  *adt_atts,
				 char     *adt_tup1,
				 char     *adt_tup2,
				 i4       *adt_cmp_result);
FUNC_EXTERN i4  adt_sortcmp(ADF_CB       *adf_scb,
			    DB_CMP_LIST  *adt_atts,
			    i4           adt_natts,
			    char         *adt_tup1,
			    char         *adt_tup2,
			    i4           dif_ret_value);

FUNC_EXTERN i4 adt_compare(ADF_CB  *adf_scb,
			   DB_ATTS  *att,
			   char     *d1,
			   char     *d2,
			   DB_STATUS *status);

/* Compare a row with a partition value */
FUNC_EXTERN DB_STATUS adt_tupvcmp(
	ADF_CB *adfcb,
	i4 ncols,
	DB_PART_LIST *part_list,
	PTR row_ptr,
	PTR pv2,
	i4 *adt_cmp_result);
	
/* Compare two partition values */
FUNC_EXTERN DB_STATUS adt_vvcmp(
	ADF_CB *adfcb,
	i4 ncols,
	DB_PART_LIST *part_list,
	PTR pv1,
	PTR pv2,
	i4 *adt_cmp_result);
	
/* Compute row's partition sequence for AUTOMATIC distributions */
FUNC_EXTERN ADT_WHICHPART_FUNC adt_whichpart_auto;

/* Compute row's partition sequence for HASH distributions */
FUNC_EXTERN ADT_WHICHPART_FUNC adt_whichpart_hash;

/* Compute row's partition sequence for LIST distributions */
FUNC_EXTERN ADT_WHICHPART_FUNC adt_whichpart_list;

/* Compute a row's physical partition number (all dimensions) */
FUNC_EXTERN DB_STATUS adt_whichpart_no(
	ADF_CB *adfcb,
	DB_PART_DEF *part_def,
	PTR row_ptr,
	u_i2 *ppartno);

/* Compute row's partition sequence for RANGE distributions */
FUNC_EXTERN ADT_WHICHPART_FUNC adt_whichpart_range;

/* Compare part breaks of 2 dimension definitions */
FUNC_EXTERN bool adt_partbreak_compare(
	ADF_CB		*adfcb,
	DB_PART_DIM	*pdim1p,
	DB_PART_DIM	*pdim2p);

FUNC_EXTERN DB_STATUS adt_compute_change(ADF_CB   *adf_scb,
					 i4       adt_natts,
					 DB_ATTS  *adt_atts,
					 char     *adt_tup1,
					 char     *adt_tup2,
					 i4  adt_change_set[(DB_MAX_COLS + 31) / 32],
					 i4  adt_value_set[(DB_MAX_COLS + 31) / 32],
					 i4  *adt_key_change,
					 i4  *adt_nkey_change);
FUNC_EXTERN DB_STATUS adt_compute_part_change(
	ADF_CB		    *adf_scb,
	i4		    adt_natts,		/* # of attrs in each tuple. */
	DB_ATTS	    	    *adt_atts,		/* Ptr to vector of DB_ATTS,
						** 1 for each attr in tuple, and
						** in attr order.
						*/
	char		    *adt_tup1,		/* Ptr to 1st tuple. */
	char		    *adt_tup2,		/* Ptr to 2nd tuple. */
	i4		    adt_change_set[(DB_MAX_COLS + 31) / 32],
	i4		    adt_value_set[(DB_MAX_COLS + 31) / 32],
	i4		    adt_compare_set[(DB_MAX_COLS + 31) / 32],
	i4		    *adt_key_change,	/* Place to put cmp result. */
	i4		    *adt_nkey_change,	/* Place to put cmp result. */
	i4		    *adt_del_start,	/* where delta starts */
	i4		    *adt_del_end);	/* where detta ends */
# ifdef hp8_bls
/*
**	21-Feb-1994 (ajc)
** 	    hp8_bls compiler has to have it simply prototyped otherwise it gets
**          confused.
*/
FUNC_EXTERN DB_STATUS adc_seglen (ADF_CB         *adf_scb,
                                  DB_DT_ID       adc_l_dt,
                                  DB_DATA_VALUE  *adc_rdv);
# else
FUNC_EXTERN ADP_SEGLEN_FUNC adc_seglen;
# endif /* hp8_bls */

# ifdef xDEBUG
# define ADF_BUILD_WITH_PROTOS TRUE
# endif /* xDEBUG */

# ifdef ADF_BUILD_WITH_PROTOS
/* Forward declare structures */
struct _AD_NEWDTNTRNL;

FUNC_EXTERN DB_STATUS adb_trace(DB_DEBUG_CB  *adb_debug_cb);
FUNC_EXTERN VOID adb_optab_print();
FUNC_EXTERN VOID adb_breakpoint();
FUNC_EXTERN DB_STATUS adg_init(ADF_CB  *adf_scb);
FUNC_EXTERN DB_STATUS adg_add_fexi(ADF_CB	*adf_scb,
				   ADI_OP_ID	adg_index_fexi,
				   DB_STATUS	(*adg_func_addr)());
FUNC_EXTERN DB_STATUS adg_release(ADF_CB  *adf_scb);
FUNC_EXTERN i4   adg_srv_size();
FUNC_EXTERN DB_STATUS adg_startup(PTR              adf_srv_cb,
				  i4          adf_size,
				  ADF_TAB_DBMSINFO *adf_dbi_tab,
				  i4		   c2secure);
FUNC_EXTERN DB_STATUS adg_shutdown();
FUNC_EXTERN DB_STATUS adi_0calclen(ADF_CB             *adf_scb,
				   ADI_LENSPEC        *adi_lenspec,
				   i4		      adi_numops,
				   DB_DATA_VALUE      *adi_dv[],
				   DB_DATA_VALUE      *adi_dvr);
FUNC_EXTERN DB_STATUS adi_encode_colspec(ADF_CB	        *adf_scb,
					 char	        *name,
					 i4	        numnums,
					 i4		nums[],
					 i4		flags,
					 DB_DATA_VALUE  *dv);
FUNC_EXTERN DB_STATUS adi_dtinfo(ADF_CB	   *adf_scb,
				 DB_DT_ID  dtid,
				 i4	   *dtinfo);
FUNC_EXTERN DB_STATUS adi_dt_rtree(ADF_CB        *adf_scb,
				   DB_DT_ID      adi_obj_did,
				   ADI_DT_RTREE  *adi_dt_rtee);
FUNC_EXTERN DB_STATUS adi_per_under(ADF_CB	   *adf_scb,
				    DB_DT_ID	   dtid,
				    DB_DATA_VALUE  *under_dv);
FUNC_EXTERN DB_STATUS adi_cpficoerce(ADF_CB     *adf_scb,
				     DB_DT_ID   adi_from_did,
				     DB_DT_ID   adi_into_did,
				     ADI_FI_ID  *adi_fid);
FUNC_EXTERN DB_STATUS adi_ficoerce(ADF_CB     *adf_scb,
				   DB_DT_ID   adi_from_did,
				   DB_DT_ID   adi_into_did,
				   ADI_FI_ID  *adi_fid);
FUNC_EXTERN DB_STATUS adi_fidesc(ADF_CB	      *adf_scb,
				 ADI_FI_ID    adi_fid,
				 ADI_FI_DESC  **adi_fdesc);
FUNC_EXTERN DB_STATUS adi_fitab(ADF_CB      *adf_scb,
				ADI_OP_ID   adi_oid,
				ADI_FI_TAB  *adi_ftab);
FUNC_EXTERN DB_STATUS adi_function(ADF_CB      *adf_scb,
				   ADI_FI_DESC *adi_fidesc,
                                   DB_STATUS   (**adi_func)() );  
FUNC_EXTERN DB_STATUS adi_opid(ADF_CB       *adf_scb,
			       ADI_OP_NAME  *adi_oname,
			       ADI_OP_ID    *adi_oid);
FUNC_EXTERN DB_STATUS adi_op_info(ADF_CB      *adf_scb,
				  ADI_OP_ID   adi_opid,
				  ADI_OPINFO  *adi_info);
FUNC_EXTERN DB_STATUS adi_castid(ADF_CB	      *adf_scb,
				ADI_OP_ID     *adi_opid,
				DB_DATA_VALUE *cast_dv);
FUNC_EXTERN DB_STATUS adi_opname(ADF_CB       *adf_scb,
				 ADI_OP_ID    adi_oid,
				 ADI_OP_NAME  *adi_oname);
FUNC_EXTERN DB_STATUS adi_pm_encode(ADF_CB	   *adf_scb,
				    i4		   adi_pm_reqs,
				    DB_DATA_VALUE  *adi_patdv,
				    i4		   *adi_line_num,
				    bool	   *adi_pmchars);
FUNC_EXTERN DB_STATUS adi_tycoerce(ADF_CB          *adf_scb,
				   DB_DT_ID        adi_did,
				   ADI_DT_BITMASK  *adi_dmsk);
FUNC_EXTERN DB_STATUS adi_tyid(ADF_CB       *adf_scb,
			       ADI_DT_NAME  *adi_dname,
			       DB_DT_ID     *adi_did);
FUNC_EXTERN DB_STATUS adi_tyname(ADF_CB       *adf_scb,
				 DB_DT_ID     adi_did,
				 ADI_DT_NAME  *adi_dname);
FUNC_EXTERN DB_STATUS adi_res_type(ADF_CB	*adf_scb,
				   ADI_FI_DESC  *adi_fidesc,
				   i4		adi_numdts,
				   DB_DT_ID     adi_dv[],
				   DB_DT_ID	*adi_res_dt);
FUNC_EXTERN DB_STATUS adi_date_chk(ADF_CB	*adf_scb,
				   DB_DT_ID	adi_dt1,
				   i4		*res);

FUNC_EXTERN DB_STATUS adi_date_add(ADF_CB	*adf_scb,
				   DB_DT_ID	adi_dt1,
				   DB_DT_ID	adi_dt2,
				   DB_DT_ID	*adi_res_dt);

FUNC_EXTERN DB_STATUS adi_resolve(ADF_CB	*adf_scb,
				  ADI_RSLV_BLK  *adi_rslv_blk,
                                  bool          varchar_precedence);
FUNC_EXTERN DB_STATUS adi_dtfamily_resolve(
				  ADF_CB              *adf_scb,
				  ADI_FI_DESC         *oldfidesc,
				  ADI_FI_DESC         *newfidesc,
				  ADI_RSLV_BLK        *adi_rslv_blk);
FUNC_EXTERN DB_STATUS adi_dtfamily_resolve_union(
				  ADF_CB              *adf_scb,
				  ADI_FI_DESC         *oldfidesc,
				  ADI_FI_DESC         *newfidesc,
				  ADI_RSLV_BLK        *adi_rslv_blk);
FUNC_EXTERN DB_STATUS adi_need_dtfamily_resolve(
				  ADF_CB              *adf_scb,
				  ADI_FI_DESC         *infidesc,
				  ADI_RSLV_BLK        *adi_rslv_blk,
				  i2		      *need);
FUNC_EXTERN ADP_COMPARE_FUNC adc_compare;
FUNC_EXTERN ADP_DBTOEV_FUNC adc_dbtoev;
FUNC_EXTERN ADP_GETEMPTY_FUNC adc_getempty;
FUNC_EXTERN ADP_HASHPREP_FUNC adc_hashprep;
FUNC_EXTERN ADP_HELEM_FUNC adc_helem;
FUNC_EXTERN ADP_HG_DTLN_FUNC adc_hg_dtln;
FUNC_EXTERN ADP_DHMAX_FUNC adc_dhmax;
FUNC_EXTERN ADP_HMAX_FUNC adc_hmax;
FUNC_EXTERN ADP_DHMIN_FUNC adc_dhmin;
FUNC_EXTERN ADP_HMIN_FUNC adc_hmin;
FUNC_EXTERN ADP_MINMAXDV_FUNC adc_minmaxdv;
FUNC_EXTERN ADP_KEYBLD_FUNC adc_keybld;
FUNC_EXTERN ADP_ISMINMAX_FUNC adc_isminmax;
FUNC_EXTERN ADP_KLEN_FUNC adc_klen;
FUNC_EXTERN ADP_KCVT_FUNC adc_kcvt;
FUNC_EXTERN ADP_LENCHK_FUNC adc_lenchk;
FUNC_EXTERN ADP_TMLEN_FUNC adc_tmlen;
FUNC_EXTERN ADP_TMCVT_FUNC adc_tmcvt;
FUNC_EXTERN ADP_VALCHK_FUNC adc_valchk;
FUNC_EXTERN ADP_COMPR_FUNC adc_compr;
FUNC_EXTERN DB_STATUS adc_hdec(ADF_CB         *adf_scb,
			       DB_DATA_VALUE  *adc_dvfhg,
			       DB_DATA_VALUE  *adc_dvthg);
FUNC_EXTERN DB_STATUS adc_cvinto(ADF_CB         *adf_scb,
				 DB_DATA_VALUE  *adc_dvfrom,
				 DB_DATA_VALUE  *adc_dvinto);
FUNC_EXTERN DB_STATUS adc_isnull(ADF_CB         *adf_scb,
				 DB_DATA_VALUE  *adc_dv,
				 i4             *adc_null);
FUNC_EXTERN DB_STATUS adf_agbegin(ADF_CB         *adf_scb,
				  ADF_AG_STRUCT  *adf_agstruct);
FUNC_EXTERN DB_STATUS adf_agend(ADF_CB         *adf_scb,
				ADF_AG_STRUCT  *adf_agstruct,
				DB_DATA_VALUE  *adf_dvresult);
FUNC_EXTERN DB_STATUS adf_agnext(ADF_CB         *adf_scb,
				 DB_DATA_VALUE  *adf_dvnext,
				 ADF_AG_STRUCT  *adf_agstruct);
FUNC_EXTERN DB_STATUS adf_func(ADF_CB      *adf_scb,
			       ADF_FN_BLK  *adf_fnblk);
FUNC_EXTERN DB_STATUS adf_exec_quick(ADF_CB	    *adf_scb,
				     ADI_FI_ID	    fid,
				     DB_DATA_VALUE  *dvarr);
FUNC_EXTERN DB_STATUS adu_nvchr_toutf8(ADF_CB	*adf_scb, 
					DB_DATA_VALUE *dv,
					DB_DATA_VALUE *rdv);  
FUNC_EXTERN DB_STATUS adu_nvchr_fromutf8(ADF_CB	*adf_scb, 
					DB_DATA_VALUE *dv,
					DB_DATA_VALUE *rdv);
FUNC_EXTERN DB_STATUS adu_unorm(ADF_CB	*adf_scb,
				DB_DATA_VALUE  *dst_dv,
				DB_DATA_VALUE  *src_dv,
				i4              nfc_flag);
FUNC_EXTERN DB_STATUS adu_setnfc(ADF_CB	*adf_scb, 
					DB_DATA_VALUE *dv,
					DB_DATA_VALUE *rdv);  
FUNC_EXTERN DB_STATUS adu_checksum(ADF_CB	*adf_scb, 
					DB_DATA_VALUE *dv,
					DB_DATA_VALUE *rdv);  
FUNC_EXTERN DB_STATUS adg_init_unimap();

FUNC_EXTERN DB_STATUS adg_clean_unimap();

FUNC_EXTERN DB_STATUS adu_dtntrnl_pend_date(ADF_CB *adf_scb,
        		      struct _AD_NEWDTNTRNL *inval);

FUNC_EXTERN DB_STATUS adu_7from_dtntrnl(ADF_CB	*adf_scb,
		              DB_DATA_VALUE   *outdv,
        		      struct _AD_NEWDTNTRNL *inval);

FUNC_EXTERN DB_STATUS adu_6to_dtntrnl(ADF_CB	*adf_scb,
        		      DB_DATA_VALUE   *indv,
        		      struct _AD_NEWDTNTRNL *outval);

FUNC_EXTERN DB_STATUS adu_utf8_unorm(ADF_CB	*adf_scb,
				DB_DATA_VALUE  *dst_dv,
				DB_DATA_VALUE  *src_dv);
# ifdef EX_included
FUNC_EXTERN DB_STATUS adx_chkwarn(ADF_CB  *adf_scb);
FUNC_EXTERN DB_STATUS adx_handler(ADF_CB   *adf_scb,
				  EX_ARGS  *adx_exstruct);
# endif /* EX_included */
FUNC_EXTERN DB_STATUS adu_cmptversion(ADF_CB *adf_scb,
				DB_DATA_VALUE *idv,
				DB_DATA_VALUE *rdv);
# else  /* ADF_BUILD_WITH_PROTOS */
FUNC_EXTERN DB_STATUS adb_trace();
FUNC_EXTERN VOID adb_optab_print();
FUNC_EXTERN VOID adb_breakpoint();
FUNC_EXTERN DB_STATUS adg_init();
FUNC_EXTERN DB_STATUS adg_add_fexi();
FUNC_EXTERN DB_STATUS adg_release();
FUNC_EXTERN i4   adg_srv_size();
FUNC_EXTERN DB_STATUS adg_startup();
FUNC_EXTERN DB_STATUS adg_shutdown();
FUNC_EXTERN DB_STATUS adi_0calclen();
FUNC_EXTERN DB_STATUS adi_encode_colspec();
FUNC_EXTERN DB_STATUS adi_dtinfo();
FUNC_EXTERN DB_STATUS adi_per_under();
FUNC_EXTERN DB_STATUS adi_cpficoerce();
FUNC_EXTERN DB_STATUS adi_ficoerce();
FUNC_EXTERN DB_STATUS adi_fidesc();
FUNC_EXTERN DB_STATUS adi_fitab();
FUNC_EXTERN DB_STATUS adi_opid();
FUNC_EXTERN DB_STATUS adi_op_info();
FUNC_EXTERN DB_STATUS adi_opname();
FUNC_EXTERN DB_STATUS adi_pm_encode();
FUNC_EXTERN DB_STATUS adi_tycoerce();
FUNC_EXTERN DB_STATUS adi_tyid();
FUNC_EXTERN DB_STATUS adi_tyname();
FUNC_EXTERN DB_STATUS adi_res_type();
FUNC_EXTERN DB_STATUS adi_resolve();
FUNC_EXTERN DB_STATUS adi_dtfamily_resolve();
FUNC_EXTERN DB_STATUS adi_dtfamily_resolve_union();
FUNC_EXTERN DB_STATUS adi_need_dtfamily_resolve();
FUNC_EXTERN DB_STATUS adc_compare();
FUNC_EXTERN DB_STATUS adc_dbtoev();
FUNC_EXTERN DB_STATUS adc_getempty();
FUNC_EXTERN DB_STATUS adc_hashprep();
FUNC_EXTERN DB_STATUS adc_helem();
FUNC_EXTERN DB_STATUS adc_hg_dtln();
FUNC_EXTERN DB_STATUS adc_dhmax();
FUNC_EXTERN DB_STATUS adc_hmax();
FUNC_EXTERN DB_STATUS adc_dhmin();
FUNC_EXTERN DB_STATUS adc_hmin();
FUNC_EXTERN DB_STATUS adc_minmaxdv();
FUNC_EXTERN DB_STATUS adc_keybld();
FUNC_EXTERN DB_STATUS adc_isminmax();
FUNC_EXTERN DB_STATUS adc_klen();
FUNC_EXTERN DB_STATUS adc_kcvt();
FUNC_EXTERN DB_STATUS adc_lenchk();
FUNC_EXTERN DB_STATUS adc_tmlen();
FUNC_EXTERN DB_STATUS adc_tmcvt();
FUNC_EXTERN DB_STATUS adc_valchk();
FUNC_EXTERN DB_STATUS adc_hdec();
FUNC_EXTERN DB_STATUS adc_cvinto();
FUNC_EXTERN DB_STATUS adc_isnull();
FUNC_EXTERN DB_STATUS adf_agbegin();
FUNC_EXTERN DB_STATUS adf_agend();
FUNC_EXTERN DB_STATUS adf_agnext();
FUNC_EXTERN DB_STATUS adf_func();
FUNC_EXTERN DB_STATUS adf_exec_quick();
FUNC_EXTERN DB_STATUS adx_chkwarn();
FUNC_EXTERN DB_STATUS adx_handler();
FUNC_EXTERN DB_STATUS adc_date_chk();
FUNC_EXTERN DB_STATUS adc_date_add();
FUNC_EXTERN DB_STATUS adu_nvchr_toutf8();
FUNC_EXTERN DB_STATUS adu_nvchr_fromutf8();
FUNC_EXTERN DB_STATUS adg_init_unimap();
FUNC_EXTERN DB_STATUS adg_clean_unimap();
FUNC_EXTERN DB_STATUS adu_unorm();
FUNC_EXTERN DB_STATUS adg_setnfc();
FUNC_EXTERN DB_STATUS adu_checksum();
FUNC_EXTERN DB_STATUS adu_7from_dtntrnl();
FUNC_EXTERN DB_STATUS adu_dtntrnl_pend_date();
FUNC_EXTERN DB_STATUS adu_6to_dtntrnl();
FUNC_EXTERN i4 adu_date_format();
FUNC_EXTERN char *adu_date_string();
FUNC_EXTERN DB_STATUS adu_utf8_unorm();
FUNC_EXTERN DB_STATUS adu_cmptversion();
# endif  /* ADF_BUILD_WITH_PROTOS */
FUNC_EXTERN DB_STATUS adu_0parsedate(ADF_CB	*adf_scb,
				DB_DATA_VALUE	*str_dv,
				DB_DATA_VALUE	*date_dv,
				bool		strict,
				bool		update_dv);

/* Return statii for adu_patcomp_summary() */
typedef enum _ADU_PAT_IS {
    ADU_PAT_IS_NORMAL,
    ADU_PAT_IS_MATCH,
    ADU_PAT_IS_PURE,
    ADU_PAT_IS_LITERAL,
    ADU_PAT_IS_NOMATCH,
    ADU_PAT_IS_FAIL
} ADU_PAT_IS;
FUNC_EXTERN ADU_PAT_IS adu_patcomp_summary (
				DB_DATA_VALUE	*pat_dv,
				DB_DATA_VALUE	*eqv_dv);

FUNC_EXTERN DB_STATUS adu_decprec (ADF_CB	*adf_scb,
				DB_DATA_VALUE	*dv1,
				i2		*ps);
FUNC_EXTERN DB_STATUS adu_2strtomny();   /* This routine converts a string
                                            ** data value into a money data
                                            ** value.
                                            */
FUNC_EXTERN DB_STATUS adu_9mnytostr();   /* Routine to coerce a money data
                                            ** value into a string data value.
                                            */
/*
** Routine used internally by ADF error handling,
** including message formatting.
** This routine takes a variable number of parameters.
** (stephenb) seems it's not so internal and is called by the front-end,
** 		moved to adf.h.
*/
FUNC_EXTERN DB_STATUS aduErrorFcn(
	ADF_CB	*adf_scb,
	i4	adf_errorcode,
	PTR	FileName,
	i4	LineNumber,
	i4	pcnt,
		...);

FUNC_EXTERN char *adu_valuetomystr(char *, DB_DATA_VALUE *, ADF_CB *);
#endif /* ADF_HDR_INCLUDED */
