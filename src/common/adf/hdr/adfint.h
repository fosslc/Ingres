/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: ADFINT.H - Internal definitions for ADF, abstract datatype facility.
**
** Description:
**	This file defines the functions, typedefs, constants, and variables
**  used internally by ADF.
**
**	The following typedefs are defined here:
**	    ADI_OPRATION    - Definition of an operation.
**	    ADI_COLUMN_INFO - Information for column specifications
**	    ADP_COM_VECT    - A vector of pointers to the 24 common datatype
**                            functions (lenchk, compare, keybld, getempty,
**                            etc.) for a specific datatype.  This vector
**                            is used internally by ADF's datatypes table.
**	    ADI_DATATYPE    - Definition of a datatype.
**          ADI_FI_LOOKUP   - Definition of the internal table used to look up
**			      function instances quickly.
**          ADT_ATTS        - Definition of an attribute descriptor
**                            defined by DMF in DMP_ATTS.
**
** History:
**      05-Mar-86 (ericj)
**	    Initial creation.
**      02-apr-86 (thurston)
**	    In the ADI_DATATYPE typedef, I added in AD_xx?_TYPE defines for
**	    the three "internal" datatypes "boolean", "text0", and "textn".
**      10-jun-86 (jennifer)
**          Added a definition for attribute descriptor for ADF so it
**          does not include DMF's definition DMP_ATTS.
**	19-jun-86 (thurston)
**	    Added the ADI_FI_LOOKUP typedef which will be used to build a table
**          of indicies (among other things) into the function instance table.
**          Also, added GLOBALREF's for Adi_fis[] (the function instance table)
**          and Adi_fi_lkup[], the table of ADI_FI_LOOKUPs. 
**	27-jun-86 (thurston)
**	    Added the #defines for the op-id's.  Also, in the ADI_FI_LOOKUP
**	    typedef, I changed the "index" into the f.i. table to be an actual
**	    pointer into the f.i. table.  Also added a group of constants that
**	    tell various things about the f.i. tables.
**	01-jul-86 (thurston)
**	    Updated the AD1_NUM_FIS and AD1_FI_BIGGEST constants to be
**	    consistent with new f.i.'s.
**	02-jul-86 (thurston)
**	    Moved the ADP_COM_VECT typedef from <adf.h> into <adfint.h>) since
**	    the external adp_acommon() call has been removed.
**	27-jul-86 (ericj)
**	    Modified for new ADF error-handling.
**	01-aug-86 (ericj)
**	    Added references to the networking routines in the ADP_COM_VECT,
**	    adp_tonet_addr and adp_tolocal_addr.
**	10-sep-86 (thurston)
**	    Fixed the constants relative to the function instance and f.i.
**	    lookup table to account for the four newly added f.i.'s (for the
**	    frontends' DB_DYC_TYPE datatype concat's).  Also changed the
**	    GLOBALREF of adi_operations to be Adi_operations.
**	01-oct-86 (thurston)
**	    Added the operation ID constants ADI_LIKE_OP, ADI_NLIKE_OP,
**	    ADI_CHAR_OP,  and ADI_VARCH_OP.
**	13-oct-86 (thurston)
**	    Added FUNC_EXTERN for adu_error().
**	14-oct-86 (thurston)
**	    Modified the constants describing the function instance
**	    table (AD1_COMPARISON_FIS, etc.) to reflect the newly added
**	    function instances for CHAR, VARCHAR, and LONGTEXT.  (Note:
**	    ************
**	    **  NOTE  **
**	    ***********************************************************
**	    **  I feel that these should become global variables to  **
**	    **  ADF, and set at server startup time to reflect what  **
**	    **  is really in the fi table.)                          **
**	    ***********************************************************
**	15-oct-86 (thurston)
**	    Modified the constants describing the function instance
**	    table (AD1_COMPARISON_FIS, etc.) to reflect the newly added
**	    function instances for comparisons involving the dynamic
**	    string type, and f.i.'s for the HEXIN, HEXOUT, and IFNULL
**	    functions.
**	16-oct-86 (thurston)
**	    Added the operation ID constants ADI_HXIN_OP, ADI_HXOUT_OP,
**	    and ADI_IFNUL_OP.
**	11-nov-86 (thurston)
**	    Changed ADI_HXOUT_OP to ADI_HEX_OP, and changed ADI_HXIN_OP to
**	    ADI_HEXIN_OP, an operator.  Also tweaked the AD1_*_FIS constants
**	    to be in sync with the fi tab.
**	14-nov-86 (thurston)
**	    Added the group of constants AD_C_SEMANTICS, AD_T_SEMANTICS,
**	    and AD_VC_SEMANTICS; as well as the group AD_1LIKE_ONE and
**	    AD_2LIKE_ANY.
**	19-nov-86 (thurston)
**	    Added the AD_MAX_TEXT constant.
**	24-nov-86 (thurston)
**	    Added references to the terminal monitor routines and the embedded
**	    value routines in the ADP_COM_VECT:  adp_tmlen_addr, adp_tmcvt_addr,
**	    adp_dbtoev_addr, and adp_dbcvtev_addr.
**	06-jan-87 (thurston)
**	    In the ADP_COM_VECT typedef, I changed member names .adp_cveql_addr
**          and .adp_cvdsply_addr to be .adp_1rsvd and .adp_2rsvd, making these
**          slots "< RESERVED >" for now, since the adc_cveql() and
**          adc_cvdsply() routines are no longer with us.
**	07-jan-87 (thurston)
**	    Added the constant ADFI_ENDTAB.
**	    Removed the constant ADI_HEXIN_OP.
**	10-feb-87 (thurston)
**	    Removed the ADF_CHAR_MAX constant, since it is being replaced by the
**	    DB_CHAR_MAX constant in <dbms.h>.
**	17-feb-87 (thurston)
**	    Added the ADF_NVL_BIT constant, and the macros ADI_ISNULL_MACRO(),
**	    ADF_SETNULL_MACRO(), and ADF_CLRNULL_MACRO().
**	22-feb-87 (thurston)
**	    Added the ADT_F_NDEFAULT constant to the ADT_ATTS typedef.
**	09-mar-87 (thurston)
**	    Added the .adi_opqlangs field to the ADI_OPRATION typedef.
**	09-mar-87 (thurston)
**	    Got rid of all of the ADI_...U_OP constants for the ...u() aggregate
**	    functions since they are no longer considered different than the
**	    non-`u' variety by ADF.  Also added ADI_ISNUL_OP, ADI_NONUL_OP, and
**	    ADI_CNTAL_OP ... all of which will be truly defined in <adfops.h>.
**      10-mar-87 (thurston)
**	    Added the following fields to the ADF_SERVER_CB typedef:
**              .Adi_comparison_fis
**              .Adi_operator_fis
**              .Adi_agg_func_fis
**              .Adi_norm_func_fis
**              .Adi_coercion_fis
**              .Adi_num_fis
**              .Adi_fi_biggest
**	12-mar-87 (thurston)
**	    Removed the AD_BOO_TYPE constant ... it is now called DB_BOO_TYPE
**	    and resides in <dbms.h>.
**	18-mar-87 (thurston)
**	    Added some WSCREADONLY to pacify the Whitesmith compiler on MVS.
**	20-mar-87 (thurston)
**	    Added ADF_BASETYPE_MACRO.
**	25-mar-87 (thurston)
**	    Added .adp_valchk_addr member to ADP_COM_VECT in place of one of the
**	    reserved slots.  Also, removed .adp_cvinto_addr, .adp_isnull_addr,
**          .adp_ds_dtln_addr, .adp_eq_dtln_addr, and .adp_hp_dtln_addr (made
**          them < RESERVED > slots).  Also removed the datatype ID from this
**	    structure.
**	25-mar-87 (thurston)
**	    Removed the .adi_dtlength function pointer from the ADI_DATATYPE
**	    structure as it was not needed.
**	03-apr-87 (thurston)
**	    In ADI_DATATYPE, changed .adi_dtintrinsic to be .adi_dtstat_bits,
**	    and added status bits AD_INTR and AD_INDB.
**	05-apr-87 (thurston)
**	    Added ADI_DBMSI_OP and ADI_TYNAME_OP; Removed ADI_USRCD_OP.
**	07-apr-87 (derek)
**	    Added the .key_offset and .extra fields.
**	08-apr-87 (thurston)
**	    Moved ADI_ANY_OP into the <adfops.h> file so OPC can use it.
**	05-may-87 (thurston)
**	    Added a fix_me dummy macro definition for ult_getval() so that I
**	    can use ult_check_macro() without having to drag in the ULF lib.
**	28-may-87 (thurston)
**	    Added ADI_EXIST_OP (commented out, since it really exists in the
**	    the <adfops.h> file so OPF can use it).
**	07-jun-87 (thurston)
**	    Added ADI_CHEXT_OP (for `charextract'), ADI_DGMT_OP(for `date_gmt'),
**	    ADI_CHA12_OP (for `iichar12'), ADI_USRLN_OP (for `iiuserlen'), and
**	    ADI_STRUC_OP (for `iistructure').
**	10-jun-87 (thurston)
**	    Added ADI_NTRMT_OP (for `ii_notrm_txt') and ADI_NTRMV_OP
**	    (for `ii_notrm_vch').
**	18-jun-87 (thurston)
**	    Added ADI_NXIST_OP (commented out, since it really exists in the
**	    the <adfops.h> file so OPF can use it).
**	18-jun-87 (thurston)
**	    Added .adp_minmaxdv_addr member to ADP_COM_VECT in place of one of
**	    the reserved slots.
**	19-jun-87 (thurston)
**	    Commented out the #ifdef for AD_MAX_TEXT for the time being.  (Read:
**	    "Until we decide what the definition of the `maximum legal character
**	    allowed in a text data value' is.")  For now, I will just use 0xFF.
**      24-jun-87 (thurston)
**	    Added the .Adi_copy_coercion_fis field to the ADF_SERVER_CB typedef.
**	24-aug-87 (thurston)
**	    Changed the ADI_SETNULL_MACRO() and ADI_CLRNULL_MACRO() to assure
**	    the value of the NULL byte is either 0 or 1.
**	01-sep-87 (thurston)
**	    Added ADI_HXINT_OP for the iihexint() function.
**	28-sep-87 (thurston)
**	    Added the ADK_MAP typedef, and the following members to the
**	    ADF_SERVER_CB typedef:
**		.Adk_bintim_map
**		.Adk_cpu_ms_map
**		.Adk_et_sec_map
**		.Adk_dio_cnt_map
**		.Adk_bio_cnt_map
**		.Adk_pfault_cnt_map
**	30-sep-87 (thurston)
**	    Added ADI_TB2DI_OP and ADI_DI2TB_OP for the ii_tabid_di() and
**	    ii_di_tabid() functions.
**	26-feb-88 (thurston)
**	    Added ADI_DVDSC_OP for the ii_dv_desc() function.
**	07-jun-88 (thurston)
**	    Added AD_3LIKE_LBRAC and AD_4LIKE_RBRAC.
**	29-jun-88 (jrb)
**	    Added ADF_TRIMBLANKS_MACRO for Kanji support in ADF.
**	07-jul-88 (thurston)
**	    Added the .adi_opuse and .adi_opsystems fields to ADI_OPRATION, and
**	    used two of the reserved slots in the ADP_COM_VECT structure for
**	    the new TITAN related function pointers, .adp_klen_addr and
**	    .adp_kcvt_addr.
**	03-aug-88 (thurston)
**	    Used two of the reserved slots in the ADP_COM_VECT structure in
**	    order to add the .adp_tpls_addr and .adp_decompose_addr members.
**	    Also added the .adi_tpl_num field to the ADI_DATATYPE structure.
**	23-sep-88 (sandyh)
**	    Moved the ADF_NVL_BIT to ADF.H for global access.
**	04-oct-88 (thurston)
**	    Added the ADI_COERC_ENT typedef, and the .Adi_ents_coerc and
**	    .Adi_tab_coerc fields to the ADF_SERVER_CB typedef.
**	06-oct-88 (thurston)
**	    Added the .adi_coerc_ents field to the ADI_DATATYPES typedef.
**	06-nov-88 (thurston)
**	    Temporarily commented out the WSCREADONLY on the filkup table.
**      26-Jan-89 (fred)
**          Added new members to ADF_SERVER_CB to allow user-defined datatypes
**	    to be used.  This involved adding a pointer to the function instance
**	    table, and sizes of all the tables, so that they can be extended
**	    easily.
**	06-mar-89 (jrb)
**	    Changed adi_optabfi in ADI_OPRATION typedef to be called
**	    adi_optabfidx and be an index into the fi table instead of an
**	    adi_fidesc.
**      07-apr-89 (fred)
**          Removed AD_INTR & AD_INDB as they are now in ADF.H.  This changed as
**	    a result of needing to get at these bits in PSF so that we can
**	    figure out when user defined datatypes are sendable to the frontends.
**	20-may-89 (andre)
**	    move ADI_CNT_OP to adfops.h s.t. it would be visible to PSF.
**      22-may-89 (fred)
**	    Added ADI_E{TYPE,LENGTH}_OP for support of standard catalogs and
**	    User defined ADT's
**      05-jun-89 (mikem)
**	    Added ADI_{LOGKEY,TABKEY}_OP for support of "table_key()" and 
**	    "object_key()" functions.
**	15-jun-89 (jrb)
**	    Removed WSCREADONLY from adi_fi declaration in ADI_FI_LOOKUP.
**	14-jul-89 (jrb)
**	    Added macro to convert from precision and scale to display length
**	    for decimal.  Added ADI_COLUMN_INFO for adi_encode_colspec.
**	31-jul-90 (linda)
**	    Added .adp_isminmax_addr mamber to ADP_COM_VECT in place of one of
**	    the reserved slots.
**      05-feb-91 (stec)
**          Added ADC_HDEC typedef. Modified the ADF_SERVER_CB struct.
**	31-jan-92 (bonobo)
**	    Replaced multiple question marks; ANSI compiler thinks they
**	    are trigraph sequences.
**	31-jan-92 (bonobo)
**	    Replaced multiple question marks; ANSI compiler thinks they
**	    are trigraph sequences.
**	09-mar-92 (jrb, merged by stevet)
**	    Added definition of ADI_FEXI for synonyms project.
**      03-aug-92 (stevet)
**          Added definition of ADI_GMTSTAMP_OP for new gmt_timestamp 
**          function.
**	16-oct-92 (rickh)
**	    Removed ADT_ATTS.  Replaced references to it with DB_ATTS.
**	29-sep-1992 (fred)
**	    Added bit string operators (bit, varbit)
**      29-oct-1992 (stevet)
**          Added ADI_ROWCNT_OP for ii_row_count.
**      03-dec-1992 (stevet)
**          Added ADI_SESSUSER_OP, ADI_SYSUSER_OP and ADI_INITUSER_OP for
**          FIPS.
**      03-Dec-1992 (fred)
**          Reduced the adc_partition & adc_join functions to a single
**          adc_lo_xform address.
**      09-dec-1992 (rmuth, merged by stevet)
**          Add ADI_ALLOCPG_OP and ADI_OVERPG_OP.
**      13-jan-1993 (stevet)
**          Added function prototypes.  Also move ADI_VARCH_OP, ADI_CHAR_OP,
**          ADI_TEXT_OP and ADI_ASCII_OP to adfops.h file.
**      05-Apr-1993 (fred)
**          Added function prototypes for new byte routines.
**       9-Apr-1993 (fred)
**          Added byte operations.
**      14-jun-1993 (stevet)
**          Added new operator ADI_NULJN_OP to handle nullable key join.
**      31-aug-1993 (stevet)
**          Added support for INGRES class object, these objects (DT, FI etc)
**          have values from 8192 to ADI_LK_MAP_MACRO.  UDT objects remain 
**          to be from 16384.
**      12-Oct-1993 (fred)
**          Added adp_seglen_addr for large objects.
**	14-dec-1994 (shero03)
**	    Removed iixyzzy()
**	11-jan-1995 (shero03)
**	    Added iixyzzy()
**	16-may-1996 (shero03)
**	    Added ADP_COMPR_ADDR for compression/expansion
**	    Added ADFmo_attach_adg routine
**	07-jun-1996 (ramra01)
**	    Added long_varchar and long_byte ext funt(coerce routines)
**	5-jul-1996 (angusm)
**		Add new operator ii_permit_type()
**	19-feb-1997 (shero03)
**	    Add soundex()
**	06-oct-1998 (shero03)
**	    Add bitwise functions: adu_bit_add, _and, _not, _or, _xor
**	    ii_ipaddr.  Removd xyzzy
**	23-oct-1998 (kitch01)
**		Buf 92735. Add _date4()
**	12-Jan-1999 (shero03)
**	    Add hash(all)
**	19-Jan-1999 (shero03)
**	    Add random, randomf()
**	25-Jan-1999 (shero03)
**	    Add uuid_create(), uuid_to_char(u), uuid_from_char(c), and
**	    uuid_compare(u1,u2)
**	20-Apr-1999 (shero03)
**	    Add substring(c FROM n [FOR l])
**	08-Jun-1999 (shero03)
**	    Add unhex function.
**	21-sep-99 (inkdo01)
**	    Added numerous functions for OLAP aggregate group.
**	6-jul-00 (inkdo01)
**	    Commented RANDOM opid's so they can be exposed in OPF in adfops.h
**	18-dec-00 (inkdo01)
**	    Commented aggregate opid's so they can be exposed in OPF.
**	19-Jan-2001 (jenjo02)
**	    Added ADI_B1SECURE, ADI_C2SECURE, Adi_status flags to
**	    ADF_SERVER_CB.
**	16-Feb-2001 (gupsh01)
**	    Added the new Unicode constant AD_MAX_UNICODE.
**	    This is required to set the limit for UCS2 fields.
**	    also added other related constants for Unicode
**	    implementation.
**	15-March-2001 (gupsh01)
**	    Added new constants for unicode U_BLANK and U_NULL.
**      16-apr-2001 (stial01)
**          Added support for tableinfo function
**      17-apr-2001 (stial01)
**          Deleted iinum_rows, iinum_pages (they will use iitableinfo function)
**      7-nov-01 (inkdo01)
**          Added POS_OP for ANSI position function, ATRIM_OP for ANSI trim function
**          and CHLEN_OP/OCLEN_OPBLEN_OP for ANSI char_length, octet_length and
**          bit_length funcs.
**	29-Mar-02 (gordy)
**	    In the long forgotten past, ADF provided support for Het-Net to
**	    'know' about datatypes.  This support has not been used for a 
**	    long time.  Changed structure entries to 'unused' for future use.
**      29-mar-2002 (gupsh01)
**	    Added Adi_maptbl to the ADF_SERVER_CB to hold coercion map 
**	    table for local characterset mapping to unicode.
**	27-may-02 (inkdo01)
**	    Returned RANDOM opid's, since they no longer need to be visible 
**	    outside ADF.
**	17-jul-2003 (toumi01)
**	    Add to PSF some flattening decisions.
**	    to make ADI_IFNUL_OP visible to PSF move #define from
**	    common/adf/hdr/adfint.h to common/hdr/hdr/adfops.h
**      15-aug-2003 (stial01)
**          Added ADI_UTF8 (b110727)
**	23-jan-2004 (guphs01)
**	    Added comments for nchar, nvarchar, iiutf8_to_utf16, 
**	    iiutf16_to_utf8 functions, these are in adfops.h.
**	4-mar-04 (inkdo01)
**	    Commented out ADI_TRIM_OP after adding to common!hdr!adfops.h
**      12-Apr-2004 (stial01)
**          Define adf_length as SIZE_TYPE.
**	12-apr-04 (inkdo01)
**	    Added I8_OP for int8() coercion (for BIGINT support)
**	3-dec-04 (inkdo01)
**	    Added COWGT_OP for collation_weight().
**	09-feb-2005 (gupsh01)
**	    Moved definitions for U_BLANK and U_NULL to adf.h.
**	29-Aug-2005 (schka24)
**	    Missing type declaration, fix.
**	13-apr-06 (dougi)
**	    Added ADI_ISDST_OP for isdst() function.
**	13-Jul-2006 (kschendel)
**	    protos for new hashpre functions.
**	18-Aug-2006 (gupsh01)
**	    Added ADI_DTTIME_OP and ADO_DTTIME_CNT for ANSI datetime
**	    system constants.
**	05-Sep-2006 (gupsh01)
**	    Added ADI_INYM_OP and ADI_INDS_OP.
**	18-oct-2006 (dougi)
**	    Added entries for year(), quarter(), month(), week(), week_iso(), 
**	    day(), hour(), minute(), second(), microsecond() functions for date 
**	    extraction.
**	20-dec-2006 (dougi)
**	    Removed date/time registers to place in common/hdr.
**	15-jan-2007 (dougi)
**	    Removed ADI_DPART_OP to place in common/hdr.
**	07-Mar-2007 (kiria01) b117768
**	    Moved ADI_CNCAT_OP to adfops.h as it is also accessible
**	    as infix operator:  strA || atrB  ==  CONCAT(strA, strB)
**	21-Mar-2007 (kiria01) b117892
**	    Consolidated operator/function code definitions into adfops.h
**	24-Sep-2007 (lunbr01)  bug 119125
**	    Add #defines for IPv6 address support in ii_ipaddr() scalar 
**	    function.  Increase output byte len from 4 to 16.
**	26-Jun-2008 (kiria01) SIR120473
**	    Added AD_5LIKE_RANGE, AD_6LIKE_CIRCUM, AD_7LIKE_BAR,
**	    AD_8LIKE_LPAR, AD_9LIKE_RPAR, AD_10LIKE_LCURL, AD_11LIKE_RCURL,
**	    AD_12LIKE_STAR, AD_13LIKE_PLUS,AD_14LIKE_QUEST, 
**	    AD_15LIKE_COMMA and AD_16LIKE_COLON for LIKE/SIMILAR TO
**	18-Sep-2008 (jonj)
**	    SIR 120874: Rename adu_error() to aduErrorFcn() invoked by
**	    adu_error macro.
**	29-Oct-2008 (jonj)
**	    SIR 120874: Add non-variadic macros and functions for
**	    compilers that don't support them.
**	aug-2009 (stephenb)
**		Remove aduErrorFcn(), it's already called outside ADF and needs
**		to be prototyped elsewhere
**      01-Aug-2009 (martin bowes) SIR122320
**          Added soundex_dm (Daitch-Mokotoff soundex)
**      05-oct-2009 (joea)
**          Add prototype for adc_bool_compare.
**/


/*
**  Defines of other constants.
*/

/*: ADF Operation IDs
**      These are symbolic constants to use for the ADF operation codes,
**      otherwise known as op-id's. These op-id's are now all defined in
**	the <adfops.h> file which is visible to the parser.
*/
#include <adfops.h>


/*
**      These constants describe various pieces of information about the
**      function instance and function instance lookup tables.
*/

#define         ADFI_ENDTAB    ((ADI_FI_ID) -99)    /* End of fitab marker */


/*
**      These constants define what semantics to use when looking at strings.
*/

#define         AD_C_SEMANTICS	    1   /* Use "c" semantics:                 */
					/* That is, ignore blanks, and        */
					/* shorter strings < longer strings.  */

#define		AD_T_SEMANTICS	    2   /* Use "text" semantics:              */
					/* That is, blanks are important, and */
					/* shorter strings < longer strings.  */

#define         AD_VC_SEMANTICS    3   /* Use "char/varchar" semantics:      */
					/* That is, blanks are important, and */
					/* effectively pad shorter strings    */
					/* with blanks to the length of       */
					/* longer strings.                    */

/*
** This constant is the largest legal text character.
** It is ASCII/EBCDIC dependent.  (IS IT ?)
*/

/**************************************************************************
**                            {@fix_me@}                                 **
**                                                                       **
**  Until we decide exactly what the definition of the largest character **
**  allowed in a text data value is, I am commenting out this #ifdef     **
**  chunk in favor of the one #define below it.                          **
**  ==================================================================== **
**                                                                       **
**  #ifdef EBCDIC                                                        **
**  #   define  AD_MAX_TEXT	((u_char) 0xFF)                          **
**  #else                                                                **
**  #   define  AD_MAX_TEXT	((u_char) 0x7F)                          **
**  #endif EBCDIC                                                        **
                                                                         */
#define  AD_MAX_TEXT	((u_char) 0xFF)                                 /**/
                                                                         /*
**                                                                       **
**************************************************************************/

/*
** for Unicode implementation we need to define new max characters
** this is actually the code point value, UCS2
*/

#define AD_MAX_UNICODE	((UCS2) 0xFFFF)

#define AD_MIN_UNICODE	((UCS2) 0)

/* Security labels are no longer with us, but we can't remove the
** entries from the function instance table without changing FI ID's,
** which is considered a Bad Thing.  Define a couple dummy constants
** for unused/illegal types so that we can have dummy table entries
** that are never picked up for any type usage.
*/
#define DB_SEC_TYPE     (DB_DT_ID) 98
#define DB_SECID_TYPE   (DB_DT_ID) 99



/*
**      This group of constants defines the pattern matching characters for
**      SQL's LIKE and NOTLIKE operators.
*/

#define		AD_1LIKE_ONE    ((u_char)'_')  /* Matches one char  */
#define		AD_2LIKE_ANY    ((u_char)'%')  /* Matches any chars */
#define		AD_3LIKE_LBRAC  ((u_char)'[')  /* If escaped, use for set */
#define		AD_4LIKE_RBRAC  ((u_char)']')  /* If escaped, use for set */
#define		AD_5LIKE_RANGE  ((u_char)'-')  /* If in set, use for range */
#define		AD_6LIKE_CIRCUM ((u_char)'^')  /* If in set, use for complement */
#define		AD_7LIKE_BAR    ((u_char)'|')  /* If escaped, use for alternation */
#define		AD_8LIKE_LPAR   ((u_char)'(')  /* SIMILAR TO set introducer */
#define		AD_9LIKE_RPAR   ((u_char)')')  /* SIMILAR TO set terminator */
#define		AD_10LIKE_LCURL ((u_char)'{')  /* SIMILAR TO repeat introducer */
#define		AD_11LIKE_RCURL ((u_char)'}')  /* SIMILAR TO repeat terminator */
#define		AD_12LIKE_STAR  ((u_char)'*')  /* SIMILAR TO zero or more */
#define		AD_13LIKE_PLUS  ((u_char)'+')  /* SIMILAR TO one or more */
#define		AD_14LIKE_QUEST ((u_char)'?')  /* SIMILAR TO zero or one */
#define		AD_15LIKE_COMMA ((u_char)',')  /* SIMILAR TO repeat punctuation */
#define		AD_16LIKE_COLON ((u_char)':')  /* SIMILAR TO set embellishment */

/*
**	This is the constants section dealing with the Soundex code
*/
#define		AD_LEN_SOUNDEX	4

#define         AD_LEN_SOUNDEX_DM         6
#define         AD_SOUNDEX_DM_INT_BUFFER 64
#define         AD_SOUNDEX_DM_PAD_BUFFER 20
#define         AD_SOUNDEX_DM_MAX_OUTPUT 16
#define         AD_SOUNDEX_DM_MAX_ENCODE 20
#define         AD_SOUNDEX_DM_OUTPUT_LEN     sizeof(i2) + AD_LEN_SOUNDEX_DM  + ((AD_SOUNDEX_DM_MAX_OUTPUT - 1) * (AD_LEN_SOUNDEX_DM + 1))

/*
**	This is the coustants section dealing with the IPADDR code
*/
#define		AD_LEN_IPV4ADDR	 4
#define		AD_LEN_IPV6ADDR	16
#define		AD_LEN_IPADDR	16
/*
**	This is the coustants section dealing with the UUID code
*/
#define		AD_LEN_UUID	16
#define		AD_LEN_CHR_UUID	36


/*
**      This group of constants and macros define usefull things regarding null
**      values represented in DB_DATA_VALUEs.
*/

#define  ADI_ISNULL_MACRO(v)  ((v)->db_datatype > 0 ? FALSE : (*(((u_char *)(v)->db_data) + (v)->db_length - 1) & ADF_NVL_BIT ? TRUE : FALSE))
    /* This macro tells if the data value pointed to by `v' contains the null
    ** value.  If it does, the macro evaluates to TRUE, and if it does not, the
    ** macro evaluates to FALSE.  It assumes that `v' is of a legal datatype,
    ** nullable or not.
    */

#define  ADF_SETNULL_MACRO(v)  (*(((u_char *)(v)->db_data) + (v)->db_length - 1) = ADF_NVL_BIT)
    /* This macro sets a nullable data value to the null value.  It assumes that
    ** the data value pointed to by `v' is of a legal nullable type.
    */

#define  ADF_CLRNULL_MACRO(v)  (*(((u_char *)(v)->db_data) + (v)->db_length - 1) = 0)
    /* This macro sets a nullable data value to the null value.  It assumes that
    ** the data value pointed to by `v' is of a legal nullable type.
    */

#define  ADF_BASETYPE_MACRO(v)  (abs((v)->db_datatype))
    /* This macro evaluates to the base datatype of the data value pointed
    ** to by `v'.
    */


/*}
**  Name: ADF_TRIMBLANKS_MACRO - Strip trailing blanks from a string
**
**  Description:
**	This macro takes a string (pointer to char) variable called "str" and
**  an integer variable "len" which is the length of the string in bytes.  It
**  then adjusts the len variable to reflect the length of the string disre-
**  garding trailing spaces.  Note that it does NOT modify the string.
**
**  History:
**	29-jun-88 (jrb)
**	    First written.  This may end up in the CL eventually.
**	20-aug-88 (jrb)
**	    Bug fix.  Would leave one blank if str was all blanks.
**	19-jul-95 (Nick Fletcher)
**	    DEC C compiler won't allow you to substract (pointer to unsigned
**	    char) from (pointer to char). Therefore, cast added in 
**	    DOUBLEBYTE branch of this macro to force 'str' parameter into
**	    (pointer to char) for final length calculation.
**	27-aug-04 (inkdo01 for stephenb)
**	    Recoded for merged single/double byte server.
*/

# define ADF_TRIMBLANKS_MACRO(str, len) \
    {if (Adf_globs->Adi_status & ADI_DBLBYTE ) \
    {\
	char	*lastxx = NULL;\
	char	*pxx    = (char *)(str);\
	char	*endpxx = (char *)(str) + (len);\
	\
	while (pxx < endpxx)\
	{\
	    if (!CMspace(pxx))\
		lastxx = pxx;\
	    CMnext(pxx);\
	}\
	(len) = (lastxx == NULL ? 0 : lastxx - (char *)(str) + CMbytecnt(lastxx));\
    } \
    else \
    {\
	while ((len)--  &&  *((str) + (len)) == ' ');\
	(len)++;\
    }}

/*
**  This macro converts from precision and scale to the display length of
**  a decimal number with this precision and scale.  This is used for the
**  terminal monitor and for various coercions from decimal to the string
**  types.  The formula is:
**  
**		prec		   - because we need room for this many digits
**		+ 1		   - for possible `-' sign
**		+ (scale > 0)      - for decimal point
**		+ (prec == scale)  - for leading zero
**		
*/

#define		AD_PS_TO_PRN_MACRO(pr, sc)	((pr)+1+((sc)>0)+((pr)==(sc)))

/*
[@group_of_defined_constants@]...
*/


/*}
** Name: ADI_OPRATION - Operation definition.
**
** Description:
**	This structure is used to define an operation.  An operation consists
**	of a name, an id, a type (comparison, operator, normal function, or
**	aggregate function), number of function instances associated with this
**	operation, and an index into the function instance table to the first
**	function instance associated with this operation.  One of these
**	structures will exist for every operation known to ADF, and be used
**	only for information purposes (i.e. READ ONLY).
**
** History:
**	5-mar-86 (ericj)
**	    Initial creation.
**	19-jun-86 (thurston)
**	    Moved the #defines for ADI_COERCION, ADI_OPERATOR, ADI_NORM_FUNC,
**	    and ADI_AGG_FUNC into the <adf.h> file, since they will now be
**	    needed by the ADI_FI_DESC typedef, which is externally visable.
**	09-mar-87 (thurston)
**	    Added the .adi_opqlangs field.
**	07-jul-88 (thurston)
**	    Added the .adi_opuse and .adi_opsystems fields.
**	06-mar-89 (jrb)
**	    Changed adi_optabfi from ptr to adi_fi_desc to an index into the fi
**	    table.
*/

typedef struct _ADI_OPRATION
{
    ADI_OP_NAME	    adi_opname;         /* An operation name */
    ADI_OP_ID	    adi_opid;		/* The operation id */
/*  [@defined_constants@]...  */
    i4		    adi_optype;         /* The operation type.  This is one
                                        ** of ADI_COMPARISON, ADI_OPERATOR,
                                        ** ADI_NORM_FUNC, or ADI_AGG_FUNC.
                                        ** (Note that coercions and copy
                                        ** coercions do not have op names, thus
                                        ** no ADI_OPERATION will be of type
                                        ** ADI_COERCION or ADI_COPY_COERCION.
                                        ** These constants are defined in
                                        ** <adf.h>. 
					*/
    i4		    adi_opuse;		/* How the OP is normally used; one of
					** the following symbolic constants,
					** defined in <adf.h>:
					**	ADI_PREFIX
					**	ADI_POSTFIX
					**	ADI_INFIX
					*/
    DB_LANG	    adi_opqlangs;       /* Bit mask representing the query
					** languages this operation is defined
					** for.  Currently, the only two bits
					** in use are DB_SQL and DB_QUEL.
					*/
    i4		    adi_opsystems;	/* Bit mask representing the DBMS
					** systems this operation is defined
					** for.  Currently, the only three bits
					** in use are ADI_INGRES_6, ADI_ANSI,
					** and ADI_DB2.
					*/
    i4		    adi_opcntfi;        /* The number of function instances
					** associated with the operation.
					*/
    i4		    adi_optabfidx;	/* An index into the function
					** instance table indicating the first
					** function instance which uses
					** this operation.
					*/
#define	    ADZ_NOFIIDX	    ((i4) -1)	/* Signifies that there are no	    */
					/* function instances for this op   */
}   ADI_OPRATION;


/*}
** Name: ADI_COERC_ENT - Used to hold sorted table of coercions for fast lookup.
**
** Description:
**	An array of these structures will be set up in the server CB at server
**	startup time.  There will be one for each unique pair of datatypes that
**	represents a legal coercion or copy coercion in the function instance
**	table.  This table will be sorted by `from datatype', `into datatype',
**	so that a binary search can be performed to find the coercion of
**	interest quickly.
**
** History:
**	04-oct-88 (thurston)
**	    Initial creation.
*/

typedef struct _ADI_COERC_ENT
{
    DB_DT_ID        adi_from_dt;        /* The `from' datatype ID.
					*/
    DB_DT_ID        adi_into_dt;        /* The `into' datatype ID.
					*/
    ADI_FI_ID       adi_fid_coerc;      /* The function instanc ID for the
					** `normal' coercion.
					*/
}   ADI_COERC_ENT;


/*}
** Name: ADI_FEXI   - Table of externally implemented functions
**
** Description:
**      This structure defines one row of the FEXI table (Function EXternally
**      Implemented).  It consists of the operation id and a pointer to the
**      function to call when executing this operation.
**
** History:
**      26-jan-90 (jrb)
**          Created.
*/

typedef struct  _ADI_FEXI
{
    ADI_OP_ID	    adi_op_fexi;                /* The operation id */
    DB_STATUS	    (*adi_fcn_fexi)();          /* Function to call to execute
                                                ** this operation
                                                */
} ADI_FEXI;


/*}
** Name: ADI_COLUMN_INFO  - Information pertaining to columns specs for dts
**
** Description:
**	This structure contains information about how a particular datatype can
**	appear in a column specification.  It contains information like whether
**	numbers can be appended to the datatype, whether parenthesized numbers
**	can be specified after the datatype, and what the default length,
**	precision, and scale is when no numbers are specified.
**
**
** History:
**	03-aug-89 (jrb)
**	    Created.
*/

typedef struct	_ADI_COLUMN_INFO
{
    bool	adi_embed_allowed;	    /* is an embedded number permitted
					    ** to follow the datatype name?
					    */
    bool	adi_paren_allowed;	    /* are parenthesized numbers allowed
					    ** to follow the datatype name?
					    */
    i4		adi_default_len;	    /* default length if no numbers
					    ** specified for a datatype
					    */
    i4		adi_default_prec;	    /* default prec if no numbers
					    ** specified for a datatype
					    */
    i4		adi_default_scale;	    /* default scale if no numbers
					    ** specified for a datatype
					    */
} ADI_COLUMN_INFO;

   
/*
** }
** Name: ADP_COM_VECT - Vector of pointers to the 24 common datatype
**                      functions for a given datatype.
**
** Description:
**      This structure is a vector of pointers to the 24 common datatype
**      functions (lenchk, compare, keybld, getempty, cvinto, hashprep,
**      helem, hmin, hmax, tmlen, tmcvt, etc.) for a specific datatype.
**      This vector is used internally by ADF's datatypes table.
**
** History:
**      20-feb-86 (thurston)
**          Initial creation.
**	30-jun-86 (thurston)
**	    Added the .adp_isnull_addr member to hold a pointer to a routine
**	    that will check to see if a supplied data value is null or not.
**	02-jul-86 (thurston)
**	    Moved the ADP_COM_VECT typedef from <adf.h> into <adfint.h>) since
**	    the external adp_acommon() call has been removed.
**	01-aug-86 (ericj)
**	    Added references to the networking routines, adp_tonet_addr and
**	    adp_tolocal_addr.
**	24-nov-86 (thurston)
**	    Added references to the terminal monitor routines and the embedded
**	    value routines:  adp_tmlen_addr, adp_tmcvt_addr,
**	    adp_dbtoev_addr, and adp_dbcvtev_addr.
**	06-jan-87 (thurston)
**	    Changed member names .adp_cveql_addr and .adp_cvdsply_addr to be
**	    .adp_1rsvd and .adp_2rsvd, making these slots "< RESERVED >" for
**	    now, since the adc_cveql() and adc_cvdsply() routines are no longer
**	    with us.
**	25-mar-87 (thurston)
**	    Added .adp_valchk_addr member in place of one of the reserved slots.
**	    Also, removed .adp_cvinto_addr, .adp_isnull_addr, .adp_ds_dtln_addr,
**	    .adp_eq_dtln_addr, and .adp_hp_dtln_addr (made them < RESERVED >
**	    slots).  Also removed the datatype ID from this structure.
**	18-jun-87 (thurston)
**	    Added .adp_minmaxdv_addr member in place of one of the reserved
**	    slots.
**	07-jul-88 (thurston)
**	    Used two of the reserved slots for the new TITAN related function
**	    pointers, .adp_klen_addr and .adp_kcvt_addr.
**	03-aug-88 (thurston)
**	    Used two of the reserved slots to add .adp_tpls_addr and
**	    .adp_decompose_addr members.
**	01-Feb-1990 (fred)
**	    Scarfed another reserved slot for the adp_join_addr.  This is the
**	    address of a function for use by peripheral datatypes to
**	    reconstruct them from their underlying datatypes.
**	16-may-1990 (fred)
**	    Scarfed yet another reserved slot for the adp_partition_addr.
**	    This routine performs the opposite of the join routine -- it divides
**	    a peripheral datatype into segments of the underlying type.
**	31-jul-90 (linda)
**	    Added .adp_isminmax_addr mamber to ADP_COM_VECT in place of one of
**	    the reserved slots.
**	04-sep-1992 (fred)
**	    Added more peripheral datatype support.  Added ADI_CPNDMP_OP
**	    to support the ii_cpn_dump() operation to dump a coupon.
**	    Changed ADI_DATATYPE definition to hold the underlying datatype
**	    for a peripheral datatype.
**      03-Dec-1992 (fred)
**          Reduced the adc_partition & adc_join functions to a single
**          adc_lo_xform address.
**      12-Oct-1993 (fred)
**          Added adp_seglen_addr for large objects.  This function
**          calculates the underlying datatype format for large objects.
**	29-Mar-02 (gordy)
**	    Changed adp_tpls_addr to unused1 and adp_decompose_addr to unused2.
**	    Intended for Het-Net but not used.
*/
typedef struct _ADP_COM_VECT
{
    ADP_LENCHK_FUNC     *adp_lenchk_addr;   /* Ptr to lenchk function. */ 
    ADP_COMPARE_FUNC    *adp_compare_addr;  /* Ptr to compare function. */
    ADP_KEYBLD_FUNC     *adp_keybld_addr;   /* Ptr to keybld function. */
    ADP_GETEMPTY_FUNC   *adp_getempty_addr; /* Ptr to getempty function. */
    ADP_KLEN_FUNC       *adp_klen_addr;     /* Ptr to klen function. */
    ADP_KCVT_FUNC       *adp_kcvt_addr;     /* Ptr to kcvt function. */
    ADP_VALCHK_FUNC     *adp_valchk_addr;   /* Ptr to valchk function. */
    ADP_HASHPREP_FUNC   *adp_hashprep_addr; /* Ptr to hashprep function. */
    ADP_HELEM_FUNC      *adp_helem_addr;    /* Ptr to helem function. */
    ADP_HMIN_FUNC       *adp_hmin_addr;     /* Ptr to hmin function. */
    ADP_HMAX_FUNC       *adp_hmax_addr;     /* Ptr to hmax function. */
    ADP_DHMIN_FUNC	*adp_dhmin_addr;    /* Ptr to "default" hmin function*/
    ADP_DHMAX_FUNC	*adp_dhmax_addr;    /* Ptr to "default" hmax function*/
    ADP_ISMINMAX_FUNC   *adp_isminmax_addr; /* Ptr to isminmax function. */
    ADP_XFORM_FUNC      *adp_xform_addr;    /* Ptr to xform function (periph)*/
    ADP_HG_DTLN_FUNC    *adp_hg_dtln_addr;  /* Ptr to hg_dtln function. */
    ADP_SEGLEN_FUNC	*adp_seglen_addr;   /* Ptr to seglen function */
    ADP_MINMAXDV_FUNC   *adp_minmaxdv_addr; /* Ptr to minmaxdv function. */
    DB_STATUS	        (*unused1)();	    /* Unused. */
    DB_STATUS	        (*unused2)();	    /* Unused. */
    ADP_TMLEN_FUNC      *adp_tmlen_addr;    /* Ptr to tmlen function. */
    ADP_TMCVT_FUNC      *adp_tmcvt_addr;    /* Ptr to tmcvt function. */
    ADP_DBTOEV_FUNC     *adp_dbtoev_addr;   /* Ptr to dbtoev function. */
    ADP_DBCVTEV_FUNC    *adp_dbcvtev_addr;  /* Ptr to dbcvtev function. */
    ADP_COMPR_FUNC      *adp_compr_addr;    /* Ptr to compr/expand function */
}   ADP_COM_VECT;

/*}
** Name: ADI_DATATYPE - Datatype definition.
**
** Description:
**	    This structure defines a datatype.  All datatypes known to ADF
**      must be defined using one of these structures.  A datatype consists of
**	a name, an id, whether it is intrinsic or abstract, datatypes this
**	datatype can be coerced to, a set of pointers to the common datatype
**	functions used for this datatype, and a pointer to a function that
**	will fill in an ADI_DTLN_VECT given a data value of this datatype.
**
** History:
**     10-mar-86 (ericj)
**	    Initial creation.
**     02-apr-86 (thurston)
**	    Added in AD_xx?_TYPE defines for the three "internal" datatypes
**	    "boolean", "text0", and "textn".
**	12-mar-87 (thurston)
**	    Removed the AD_BOO_TYPE constant ... it is now called DB_BOO_TYPE
**	    and resides in <dbms.h>.
**	25-mar-87 (thurston)
**	    Removed the .adi_dtlength function pointer ... no longer used.
**	03-apr-87 (thurston)
**	    Changed .adi_dtintrinsic to be .adi_dtstat_bits,
**	    and added status bits AD_INTR and AD_INDB.
**	03-aug-88 (thurston)
**	    Added the .adi_tpl_num field.
**	06-oct-88 (thurston)
**	    Added the .adi_coerc_ents field.
**	07-apr-89 (fred)
**	    Removed aforementioned AD_INTR & AD_INDB bits, as they have been
**	    defined in adf.h.  PSF needs to get at them to determine if the
**	    datatype in question is sendable (AD_NOEXPORT) -- this information
**	    is now retrieved (selected?) via adi_dtinfo().
**	03-aug-89 (jrb)
**	    Added adi_column_info member.
**	01-feb-1990 (fred)
**	    Added adi_under_dt datatype for use by peripheral datatypes in
**	    describing how they are represented.
**	29-Mar-02 (gordy)
**	    Changed adi_tpl_num to unused.  Intended for Het-Net but not used.
**	20-apr-06 (dougi)
**	    Added adi_dtfamily to simplify FI tables (part of date/time
**	    enhancements).
**	22-Oct-07 (kiria01) b119354
**	    Split adi_dtcoerce into a quel and SQL version
*/

typedef struct _ADI_DATATYPE
{
    ADI_DT_NAME     adi_dtname;		/* Datatype name */
    DB_DT_ID        adi_dtid;		/* Datatype id, as defined in DBMS.h */
    DB_DT_ID	    adi_dtfamily;	/* type ID of family to which this 
					** type belongs */
    i4		    adi_dtstat_bits;	/* Status bits:  Found in ADF.H */
    i4		    unused;		/* Unused. */
    ADI_DT_BITMASK  adi_dtcoerce_quel;  /* Datatypes this datatype can be
					** coerced to under Quel.
					*/
    ADI_DT_BITMASK  adi_dtcoerce_sql;   /* Datatypes this datatype can be
					** coerced to under SQL.
					*/
    ADI_COERC_ENT   *adi_coerc_ents;	/* Ptr to 1st entry in sorted coercion
					** table for this `from' datatype.
					*/
    ADI_COLUMN_INFO adi_column_info;	/* Information for interpretting
					** column specs for this datatype
					*/
    DB_DT_ID	    adi_under_dt;	/*
					** Set in peripheral datatypes to
					** indicate what non-peripheral datatype
					** the peripheral datatype is broken up
					** into, i.e. the datatypes of the
					** segments
					*/
    ADP_COM_VECT    adi_dt_com_vect;	/* Set of pointers to the common
					** datatype routines used by this
					** datatype.
					*/
}   ADI_DATATYPE;


/*}
** Name: ADI_FI_LOOKUP - Definition of the internal table used to look up
**			 function instances quickly.
**
** Description:
**	This structure defines an element of the ADF internal table used to look
**      up function instances in the function instance table quickly.  Each
**      element of this table will correspond to one function instance, whose ID
**      will be equal to the index into this table.  The information held by
**      each element of this table (i.e. for each f.i.) will include a pointer
**      into the function instance table where the ADI_FI_DESC for the f.i. can
**      be found, along with a pointer to the actual ADF routine that performs
**      the function instance.  For the aggregate f.i.'s, pointers to the
**	adf_agbgn() and adf_agend() routines will also be included.
**
** History:
**      19-jun-86 (thurston)
**	    Initial creation.
**	27-jun-86 (thurston)
**	    Changed the "index" into the f.i. table to be an actual pointer
**	    into the f.i. table.
**	15-jun-89 (jrb)
**	    Removed WSCREADONLY from adi_fi declaration.
*/

typedef struct
{
    ADI_FI_DESC	      *adi_fi;          /* Pointer into the function instance
                                        ** table for the corresponding function
                                        ** instance.
					*/
    DB_STATUS         (*adi_func)();      /* Ptr to the ADF routine that processes
                                        ** the corresponding function instance.
                                        ** If this f.i. is an aggregate, this
                                        ** points to the "adf_agnext()" routine
                                        ** for the f.i.
                                        */
    DB_STATUS         (*adi_agbgn)();     /* If the corresponding f.i. is an
                                        ** aggregate, this will be a pointer to
                                        ** the ADF routine that initializes that
                                        ** aggregate.  If the f.i. is not an
                                        ** aggregate, this will be a NULL
                                        ** pointer.
                                        */
    DB_STATUS         (*adi_agend)();     /* If the corresponding f.i. is an
                                        ** aggregate, this will be a pointer to
                                        ** the ADF routine that finalizes that
                                        ** aggregate.  If the f.i. is not an
                                        ** aggregate, this will be a NULL
                                        ** pointer.
                                        */
}   ADI_FI_LOOKUP;


/*}
** Name: ADK_MAP - Mapping of dbmsinfo request code to ADF constant bit.
**
** Description:
**	This structure is used to hold the mapping of the dbmsinfo requests
**	that correspond to the db query constants, to the ADF query constant
**	bits.  One of these structures exist in the ADF server CB for each of
**	the query constants, and is initialized at server startup time.
**
** History:
**	28-sep-87 (thurston)
**	    Initial creation.
*/

typedef struct _ADK_MAP
{
    ADF_DBMSINFO    *adk_dbi;		/* Pointer to dbmsinfo() request record.
					*/
    i4              adk_kbit;		/* Corresponding ADF query constant
					** bit.  The bit definitions can be
					** found in <adf.h> with names such
					** as ADK_BINTIM, ADK_CPU_MS, etc.
					*/
}   ADK_MAP;


/*}
** Name: ADC_HDEC - floating-point values for adc_hdec().
**
** Description:
**      This structure contains all floating-point values required
**	by adc_hdec().
**
** History:
**     06-feb-91 (stec)
**          Created.
*/
typedef struct _ADC_HDEC
{
    f4		    fp_f4neg;		/* Multiplication factor for f4 to be
					** used in adc_hdec() when f4 value is
					** negative.
					*/
    f4		    fp_f4nil;		/* A floating-point number for f4 to be
					** used in adc_hdec() when f4 value is
					** zero.
					*/
    f4		    fp_f4pos;		/* Multiplication factor for f4 to be
					** used in adc_hdec() when f4 value is
					** positive.
					*/
    f8		    fp_f8neg;		/* Multiplication factor for f8 to be
					** used in adc_hdec() when f8 value is
					** negative.
					*/
    f8		    fp_f8nil;		/* A floating-point number for f8 to be
					** used in adc_hdec() when f8 value is
					** zero.
					*/
    f8		    fp_f8pos;		/* Multiplication factor for f8 to be
					** used in adc_hdec() when f8 value is
					** positive.
					*/
}   ADC_HDEC;


/*}
** Name: ADF_SERVER_CB - Definition of ADF's global server control block.
**
** Description:
**	This structure defines ADF's server control block which will carry all
**	of those items that need to be global (i.e. server wide) and cannot be
**	READONLY.  Such things as the datatypes table, and operations table are
**	good examples, since they must be written once, at server startup.
**
**	The three tables contained in this structure are defined simply as
**	pointers.  They will be used as tables, however, and at server startup
**	time, the data for the tables will be placed in memory right after the
**	ADF_SERVER_CB struct.  The pointers will be set appropriately at that
**	time.
**
** History:
**      24-feb-87 (thurston)
**	    Initial creation.
**      10-mar-87 (thurston)
**	    Added the following fields:
**              .Adi_comparison_fis
**              .Adi_operator_fis
**              .Adi_agg_func_fis
**              .Adi_norm_func_fis
**              .Adi_copy_coercion_fis
**              .Adi_coercion_fis
**              .Adi_num_fis
**              .Adi_fi_biggest
**      24-jun-87 (thurston)
**	    Added the .Adi_copy_coercion_fis field.
**	28-sep-87 (thurston)
**	    Added the ADK_MAPs for the query constants:
**		.Adk_bintim_map
**		.Adk_cpu_ms_map
**		.Adk_et_sec_map
**		.Adk_dio_cnt_map
**		.Adk_bio_cnt_map
**		.Adk_pfault_cnt_map
**	04-oct-88 (thurston)
**	    Added the sorted coercion lookup table:
**		.Adi_ents_coerc
**		.Adi_tab_coerc
**	26-Jan-89 (fred)
**	    Added fields to support user-define ADT's
**	    In order to dynamically move things, all tables are now found via
**	    the scb.  These are pointer back to the readonly table as long as
**	    adg_augment() has not been called.
**		.Adi_fis
**		.Adi_fi_lkup
**		.Adi_fi_rt_biggest
**		.Adi_op_size
**		.Adi_fi_size
**		.Adi_fil_size
**		.Adi_dt_size
**		.Adi_dtp_size
**		.Adi_co_size
**	27-Jan-89 (fred)
**	    Removed references to Adi_fis & Adi_fi_lkup, since these are now
**	    available via similarly name fields in ADF_SERVER_CB.  This was done
**	    to fully support user-defined abstract datatypes.
**      05-feb-91 (stec)
**          Added Adc_hdec to the ADF_SERVER_CB struct.
**	09-mar-92 (jrb, merged by stevet)
**	    Added ADI_FEXI table to ADF_SERVER_CB.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	3-jul-96 (inkdo01)
**	    Added Adi_pred_func_fis to ADF_SERVER_CB.
**      29-mar-2002 (gupsh01)
**	    Added Adi_maptbl to the ADF_SERVER_CB to hold coercion map 
**	    table for local characterset mapping to unicode. 
**	27-aug-04 (inkdo01 for stephenb)
**	    Add ADI_DBLBYTE for merged single/doublebyte server.
**	19-aug-06 (gupsh01)
**	    Added support for system constants CURRENT_TIME, CURRENT_DATE...
*/

typedef struct _ADF_SERVER_CB
{
/********************   START OF STANDARD HEADER   ********************/

    struct _ADF_SERVER_CB *adf_next;    /* Forward pointer.
                                        */
    struct _ADF_SERVER_CB *adf_prev;    /* Backward pointer.
                                        */
    SIZE_TYPE       adf_length;         /* Length of this structure: */
    i2              adf_type;		/* Type of structure.  For now, 3245 */
#define                 ADSRV_TYPE      3245	/* for no particular reason */
    i2              adf_s_reserved;	/* Possibly used by mem mgmt. */
    PTR             adf_l_reserved;	/*    ''     ''  ''  ''  ''   */
    PTR             adf_owner;          /* Owner for internal mem mgmt. */
    i4              adf_ascii_id;       /* So as to see in a dump. */
					/*
					** Note that this is really a char[4]
					** but it is easer to deal with as an i4
					*/
#define                 ADSRV_ASCII_ID  0xADFADF

/*********************   END OF STANDARD HEADER   *********************/

    i4         	    Adi_inited;         /* If set to 1, then ADF has been
                                        ** started successfully.
					*/
    i4		    Adi_dt_size;	/* Size of ... */
    ADI_DATATYPE    *Adi_datatypes;	/* ADF's datatypes table.  This is
					** defined as a pointer, but will be
					** used as an array.
                                        */
    i4		    Adi_dtp_size;
    ADI_DATATYPE    **Adi_dtptrs;	/* ADF's datatype pointers table.  This
					** is defined as a pointer to a pointer,
					** but will be used as an array of
					** pointers.
                                        */
    i4		    Adi_op_size;
    ADI_OPRATION    *Adi_operations;	/* ADF's operations table.  This is
					** defined as a pointer, but will be
					** used as an array.
                                        */
    i4		    Adi_co_size;
    ADI_COERC_ENT   *Adi_tab_coerc;	/* Sorted coercion lookup table.
					*/
    i4		    Adi_ents_coerc;	/* # entries in the sorted coercion
					** lookup table.
					*/
    i4		    Adi_fi_size;
    ADI_FI_DESC	    *Adi_fis;		/* Function instance table */
    i4		    Adi_fil_size;
    ADI_FI_LOOKUP   *Adi_fi_lkup;	/* FI Lookup Table	   */

    i4              Adi_comparison_fis; /* Where in the function instance table
                                        ** the COMPARISON group starts.
                                        */
    i4              Adi_operator_fis;   /* Where in the function instance table
                                        ** the OPERATOR group starts.
                                        */
    i4              Adi_agg_func_fis;   /* Where in the function instance table
                                        ** the AGGREGATE group starts.
                                        */
    i4              Adi_norm_func_fis;  /* Where in the function instance table
                                        ** the NORMAL FUNC group starts.
                                        */
    i4              Adi_pred_func_fis;  /* Where in the function instance table
                                        ** the PREDICATE FUNC group starts.
                                        */
    i4              Adi_copy_coercion_fis;  /* Where in the function instance
                                        ** table the COPY COERCION group starts.
                                        */
    i4              Adi_coercion_fis;   /* Where in the function instance table
                                        ** the COERCION group starts.
                                        */
    i4              Adi_num_fis;        /* # of function instances in the
					** function instance table.
                                        */
    ADI_FI_ID       Adi_fi_biggest;     /* Largest function instance ID in the
					** function instance table.
                                        */
    ADI_FI_ID	    Adi_fi_rt_biggest;	/* Largest FI ID defined by RTI */
    i4              Adi_num_dbis;       /* # of dbmsinfo() requests in the
					** dbmsinfo() request table.
                                        */
    ADF_DBMSINFO    *Adi_dbi;           /* Table of dbmsinfo() requests.
                                        */
    ADK_MAP	    Adk_bintim_map;	/* Mapping of request code to constant
					** bit for the `_bintim' constant.
					*/
    ADK_MAP	    Adk_cpu_ms_map;	/* Mapping of request code to constant
					** bit for the `_cpu_ms' constant.
					*/
    ADK_MAP	    Adk_et_sec_map;	/* Mapping of request code to constant
					** bit for the `_et_sec' constant.
					*/
    ADK_MAP	    Adk_dio_cnt_map;	/* Mapping of request code to constant
					** bit for the `_dio_cnt' constant.
					*/
    ADK_MAP	    Adk_bio_cnt_map;	/* Mapping of request code to constant
					** bit for the `_bio_cnt' constant.
					*/
    ADK_MAP         Adk_pfault_cnt_map; /* Mapping of request code to constant
					** bit for the `_bio_cnt' constant.
					*/
    ADK_MAP	    Adk_curr_date_map;	/* Mapping of request code to constant
					** bit for the `current_date' constant.
					*/
    ADK_MAP	    Adk_curr_time_map;	/* Mapping of request code to constant
					** bit for the `current_time' constant.
					*/
    ADK_MAP	    Adk_curr_tstmp_map;	/* Mapping of request code to constant
					** bit for the `current_timestamp' constant.
					*/
    ADK_MAP	    Adk_local_time_map;	/* Mapping of request code to constant
					** bit for the `local_time' constant.
					*/
    ADK_MAP	    Adk_local_tstmp_map;/* Mapping of request code to constant
					** bit for the `local_timestamp' constant.
					*/
    ADI_FEXI	    *Adi_fexi;          /*
                                        ** FEXI table
                                        */
    ULT_VECTOR_MACRO(128, 8)
                    Adf_trvect;         /* ADF's trace vector struct.
					*/
    ADC_HDEC	    Adc_hdec;		/* Structure containing floating-
					** point values for adc_hdec().
					*/
    i4              Adi_fi_int_biggest; /* 
					** Biggest FI for INGRES and
					** class object (if any)
					*/
    i4		    Adi_status;		/* Misc status flags: */
#define		ADI_C2SECURE	0x0002
#define		ADI_IS_SECURE	(ADI_C2SECURE)
#define		ADI_UTF8	0x0004
#define		ADI_DBLBYTE	0x0008  /* server is running using double
					** byte charater set */
    PTR		    Adu_maptbl;		/* Mapping table for local 
					** to unicode 
					*/
}   ADF_SERVER_CB;


/* {@fix_me@} ...
** Definition of dummy macro ult_getval() to overcome linking error msg.
*/

#define	    ult_getval(v,b,v1,v2)   (0)


/*
** References to globals variables declared in other C files.
*/

GLOBALREF		ADF_SERVER_CB	    *Adf_globs;
					    /* Ptr to ADF's server CB */

/*
**  Added support for INGRES class object, these objects (DT, FI etc)
**  have values from 8192 to ADI_LK_MAP_MACRO.  UDT objects remain 
**  to be from 16384.
*/
#define                 ADI_LK_MAP_MACRO(i) \
		((i) <= Adf_globs->Adi_fi_rt_biggest \
		? (i) : ((i) < 16383   \
		    ? ((i) - 8191 + Adf_globs->Adi_fi_rt_biggest) \
			 : ((i) - 16383 + Adf_globs->Adi_fi_int_biggest)))

/* [@global_variable_reference@]... */


/*
**  Forward and/or External function references.
*/


#ifdef HAS_VARIADIC_MACROS

#define adu_error(adf_scb,adf_errcode,...) \
	aduErrorFcn(adf_scb,adf_errcode,__FILE__,__LINE__,__VA_ARGS__)

#else

/* Variadic macros not supported */
#define adu_error adu_errorNV

FUNC_EXTERN DB_STATUS adu_errorNV(
	ADF_CB	*adf_scb,
	i4	adf_errorcode,
	i4	pcnt,
		...);

#endif /* HAS_VARIADIC_MACROS */

FUNC_EXTERN ADP_COMPARE_FUNC adc_float_compare;
FUNC_EXTERN ADP_COMPARE_FUNC adc_bool_compare;
FUNC_EXTERN ADP_COMPARE_FUNC adc_int_compare;
FUNC_EXTERN ADP_COMPARE_FUNC adc_dec_compare;
FUNC_EXTERN ADP_COMPARE_FUNC adc_bit_compare;
FUNC_EXTERN ADP_COMPARE_FUNC adc_byte_compare;
FUNC_EXTERN ADP_HASHPREP_FUNC adc_1hashprep_rti;
FUNC_EXTERN ADP_HASHPREP_FUNC adc_inplace_hashprep;
FUNC_EXTERN ADP_HASHPREP_FUNC adc_vch_hashprep;
FUNC_EXTERN ADP_GETEMPTY_FUNC adc_1getempty_rti;
FUNC_EXTERN ADP_HELEM_FUNC adc_1helem_rti;
FUNC_EXTERN ADP_HG_DTLN_FUNC adc_1hg_dtln_rti;
FUNC_EXTERN ADP_DHMAX_FUNC adc_1dhmax_rti;
FUNC_EXTERN ADP_HMAX_FUNC adc_1hmax_rti;
FUNC_EXTERN ADP_DHMIN_FUNC adc_2dhmin_rti;
FUNC_EXTERN ADP_HMIN_FUNC adc_1hmin_rti;
FUNC_EXTERN ADP_MINMAXDV_FUNC adc_1minmaxdv_rti;
FUNC_EXTERN ADP_ISMINMAX_FUNC adc_1isminmax_rti;
FUNC_EXTERN ADP_KLEN_FUNC adc_1klen_rti;
FUNC_EXTERN ADP_KCVT_FUNC adc_2kcvt_rti;
FUNC_EXTERN ADP_LENCHK_FUNC adc_1lenchk_rti;
FUNC_EXTERN ADP_LENCHK_FUNC adc_2lenchk_bool;
FUNC_EXTERN ADP_TMLEN_FUNC adc_1tmlen_rti;
FUNC_EXTERN ADP_TMCVT_FUNC adc_2tmcvt_rti;
FUNC_EXTERN ADP_VALCHK_FUNC adc_1valchk_rti;
FUNC_EXTERN ADP_DBTOEV_FUNC adc_1dbtoev_ingres;

FUNC_EXTERN STATUS ADFmo_attach_adg(void);
