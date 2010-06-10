/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <cm.h>
#include    <cv.h>
#include    <mh.h>
#include    <sl.h>
#include    <clfloat.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adfops.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
#include    <adudate.h>
#include    <adumoney.h>
#include    "adustrcmp.h"

/**
**
**  Name: ADUKEY.C -	This file contains support routines necessary for
**			building keys to be used with Ingres ISAM, HASH,
**			and BTREE tables.
**
**  Description:
**	This file contains those functions necessary to do key building at
**	the lowest level.  Some cases are handled in the externally visible
**	"adc_keybld" routine, and those cases which cannot be handled are
**	handed to the appropriate function in this file.  The interface to
**	these routines are very similar to the interface to adc_keybld.  One
**	notable exception is that for ADI_EQ_OP these routines are required
**	to set only the low key and adc_keybld will copy the computed value
**	up to the high key for ADC_KEXACTKEY keys.
**
**	This file defines the following externally visible routines:
**
**	    adu_cbldkey()   - Build a "c", "text", "char", or "varchar" key.
**	    adu_dbldkey()   - Build a "date" key.
**	    adu_fbldkey()   - Build an "f" key.
**	    adu_ibldkey()   - Build an "i" key.
**	    adu_mbldkey()   - Build a "money" key.
**	    adu_bbldkey()   - Build a logical key (byte key)
**	    adu_decbldkey() - Build a decimal key
**	    adu_bitbldkkey()- Build a bit key
**	    adu_utf8cbldkey() - Build a "c", "text", "char", or "varchar" key for
**				a UTF8 installation.
**
**      This file also defines the following static routines:
**
**	    key_fsm() - Finite state machine for building any type of string key
**			for the `=', or `LIKE' operators.
**	    key_ltgt()- Finite state machine for building any type of string key
**			for the `<', `<=', `>', or `>=' operators.
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:
**      09-jun-86 (ericj)    
**          Created this file for Jupiter.  Merged all these routines into
**	    this one file.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	20-oct-86 (thurston)
**	    Added the adu_vcbldkey() and (static) key_like() routines to handle
**	    char and varchar, and the LIKE operator.
**	03-nov-86 (thurston)
**	    Dummied up static routine key_like() to always return
**	    ADC_KALLMATCH for now.
**	19-nov-86 (thurston)
**	    Wrote the static routine key_like() to function the correct way.
**	19-nov-86 (thurston)
**	    Removed the adu_vcbldkey() routine as it isn't needed; adu_cbldkey()
**	    can handle all string datatypes, all operators, and all query
**	    languages.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	18-may-87 (thurston)
**	    Fixed bug in key_like() that was getting the length of the low and
**	    high keys to be built (for text and varchar) by looking at the
**	    uninitialized count field instead of the .db_length - DB_CNTSIZE.
**	30-jul-87 (thurston)
**          Recoded adu_dbldkey() to accept other than date data values as
**	    input, and to be a bit smarter.
**	31-jul-87 (thurston)
**          Recoded adu_mbldkey() to accept other than money data values as
**	    input, and to be a bit smarter.
**	31-jul-87 (thurston)
**	    Totally rewrote adu_ibldkey() and adu_fbldkey() to be (a) more
**	    efficient and (b) able to handle input datatypes other than
**	    integers.
**	20-oct-87 (thurston)
**	    Added code to adu_cbldkey() to check for patterns that are not
**	    string datatypes (such as i, or f, etc.), and if found return
**	    ALLMATCH.
**	20-oct-87 (thurston)
**	    Added the `rmv_blanks' argument to c_pat_chk(), so caller can
**          specify whether blanks should be stipped out or not.  This was
**          needed for the case where the pattern is `text' or `varchar', but
**          the column we are building a key for is `c'. 
**	02-jun-88 (thurston)
**	    Added comments and code for the new .adc_escape and .adc_isescape
**	    fields in the ADC_KEY_BLK.
**	19-jun-88 (thurston)
**	    Rewrote the STATIC key_like() routine to use a finite state
**	    machine, allow the ESCAPE clause, and in conjunction with it, the
**	    QUEL like range pattern matching capability.
**	05-jul-88 (thurston)
**	    Got rid of the former key building routines for non-LIKE operators,
**	    replacing them with a new routine much like the finite state machine
**	    recently added for the LIKE operator.  The former STATIC routines,
**	    c_pat_chk(), cpy_c_key() have been removed.  The new STATIC routine
**	    key_lteqgt() has been added.
**	11-jul-88 (jrb)
**	    Made changes for Kanji support.
**	27-oct-88 (thurston & jrb)
**	    In key_fsm(), fixed up `MATCH-ONE' to assure we get a RANGEKEY.
**	21-apr-89 (mikem)
**	    Logical key development.  Added adu_bbldkey() for support of 
**	    "byte" datatypes.
**	10-jul-89 (jrb)
**	    Added decimal support to all routines which govern building keys
**	    on types coercible to/from decimal.  Also, added new function for
**	    decimal key building.
**	09-apr-90 (mikem)
**	    bug #20966
**	    Add code to handle the case of non-logical key values being 
**	    comparedto logical key values in the where clause:
**	    	(ie. where table_key = char(8))
**	    Previous code would return an adf error which would cause a 
**	    "severe" error in query processing - the 2nd of which would 
**	    crash the server.
**      26-mar-91 (jrb)
**          Bug 36421.  In adu_mbldkey we were converting a float to a money
**          just by storing it directly into the money value.
**      14-aug-91 (jrb)
**          Added missing "break" stmt in adu_mbldkey; this is part of the
**          previous bug fix (bug 36421) which somehow didn't get integrated.
**	03-oct-1992 (fred)
**	    Added adu_bitbldkey() for building keys from bits.
**      04-jan-1993 (stevet)
**          Added function prototypes.
**      14-jun-1993 (stevet)
**          Fixed uninitialize variable in adu_mbldkey().
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	08-jan-1994 (stevet)
**          B36421 is back due to another problem with decimal->money
**          coercion problem on VMS.  Round the number after decimal->money
**          just like float correct this problem.
**      01-apr-1994 (stevet)
**          Added min/max check for float keybld routine similar to
**          the integer keybld routine.  If input value is above the
**          max/min value for the result data type, then NOMATCH
**          or ALLMATCH will be returned. (B62041)
**	04-apr-1996 (abowler)
**          Fixed bug 74788: embedded MATCH_ONEs (SQL '_' or QUEL '?') 
**          in C datatype don't key correctly. Fix was to generate a low 
**          key using MIN_CHAR+1 to replace MATCH_ONEs instead of 
**          MIN_CHAR. This fix looks to have been mis-integrated in '89.
**	12-sep-1996 (inkdo01)
**	    Added code in all type-specific routines to classify "<>" (ne)
**	    relops as ADC_KNOKEY (to permit improved selectivity estimates
**	    from OPF).
**      30-Nov-1998 (wanfr01)
**          Included st.h for ST cl functions in key_fsm
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	13-apr-1999 (marro11)
**	    Fix paren usage with previous change; see STskipblank().
**      08-jul-1999 (gygro01)
**	    Xintegration of change for bug 42085 and 42341 fixed by stevet
**	    (14-feb-92). Fix changes the return keytype for an empty date
**	    for ADI_LT_OP and ADI_LE_OP from EXACTKEY to RANGEKEY.
**	    Returning EXACTKEY when QEF/ADE expecting RANGEKEY cause QEF/SCF
**	    to parse the query again  until the retry count exceed.
**	    In case of repeat query, QEF will request FE to resend query
**	    text and causing an infinite loop.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**       7-mar-2001 (mosjo01)
**          Had to make MAX_CHAR more unique, IIMAX_CHAR_VALUE, due to conflict
**          between compat.h and system limits.h on (sqs_ptx).
**	16-Mar-2008 (kiria01) b120004
**	    As part of cleaning up timezone handling the internal date
**	    conversion routines now can detect and return errors.
**	3-july-2008 (dougi)
**	    Tidy error handling for bad coercions - it is a user error.
**	15-Jul-2008 (kiria01) SIR120473
**	    Move adc_isescape into new adc_pat_flags.
**	27-Mar-2009 (kiria01) b121841
**	    Ensure key build code can cope with a NULL buffer for
**	    the key so that datatype specific checks can be made even
**	    in the absence of a buffer.
**/


/*
**  Definition of static variables and forward static functions.
*/

static	DB_STATUS	key_fsm(ADF_CB       *adf_scb,
				i4          semantics,
				ADC_KEY_BLK  *adc_kblk);

static	DB_STATUS	key_ltgt(ADF_CB       *adf_scb,
				 i4           semantics,
				 ADC_KEY_BLK  *adc_kblk);


/*
**  The following is the transition/state table representing the finite state
**  machines for building keys for the INGRES string types.
**
**  Each row of the table represents one of the legal states, excluding the
**  EXIT state, which is obviously not needed.  Each column of the table
**  represents one of the legal character classes.  The finite state machines
**  work by scanning the pattern string, handling the escape sequences for LIKE
**  at that time, and then moving from state to state, performing the specified
**  action found in this table.
*/
static	AD_TRAN_STATE	ad_ts_tab[5][7] =
{
    {	/* state:  AD_S0_NOT_IN_RANGE         */
	/* ---------------------------------- */
	    { AD_ACT_A, AD_S0_NOT_IN_RANGE         },	/* cc:  AD_CC0_NORMAL */
	    { AD_ACT_A, AD_S0_NOT_IN_RANGE         },	/* cc:  AD_CC1_DASH   */
	    { AD_ACT_B, AD_S0_NOT_IN_RANGE         },	/* cc:  AD_CC2_ONE    */
	    { AD_ACT_I, AD_SX_EXIT                 },	/* cc:  AD_CC3_ANY    */
	    { AD_ACT_C, AD_S1_IN_RANGE_DASH_NORM   },	/* cc:  AD_CC4_LBRAC  */
	    { AD_ACT_H, AD_SX_EXIT                 },	/* cc:  AD_CC5_RBRAC  */
	    { AD_ACT_J, AD_SX_EXIT                 }	/* cc:  AD_CC6_EOS    */
    },
    {	/* state:  AD_S1_IN_RANGE_DASH_NORM */
	/* -------------------------------- */
	    { AD_ACT_D, AD_S2_IN_RANGE_DASH_IS_OK  },	/* cc:  AD_CC0_NORMAL */
	    { AD_ACT_D, AD_S2_IN_RANGE_DASH_IS_OK  },	/* cc:  AD_CC1_DASH   */
	    { AD_ACT_G, AD_SX_EXIT                 },	/* cc:  AD_CC2_ONE    */
	    { AD_ACT_G, AD_SX_EXIT                 },	/* cc:  AD_CC3_ANY    */
	    { AD_ACT_G, AD_SX_EXIT                 },	/* cc:  AD_CC4_LBRAC  */
	    { AD_ACT_F, AD_S0_NOT_IN_RANGE         },	/* cc:  AD_CC5_RBRAC  */
	    { AD_ACT_H, AD_SX_EXIT                 }	/* cc:  AD_CC6_EOS    */
    },
    {	/* state:  AD_S2_IN_RANGE_DASH_IS_OK  */
	/* ---------------------------------- */
	    { AD_ACT_K, AD_S2_IN_RANGE_DASH_IS_OK  },	/* cc:  AD_CC0_NORMAL */
	    { AD_ACT_Z, AD_S3_IN_RANGE_AFTER_DASH  },	/* cc:  AD_CC1_DASH   */
	    { AD_ACT_G, AD_SX_EXIT                 },	/* cc:  AD_CC2_ONE    */
	    { AD_ACT_G, AD_SX_EXIT                 },	/* cc:  AD_CC3_ANY    */
	    { AD_ACT_G, AD_SX_EXIT                 },	/* cc:  AD_CC4_LBRAC  */
	    { AD_ACT_L, AD_S0_NOT_IN_RANGE         },	/* cc:  AD_CC5_RBRAC  */
	    { AD_ACT_H, AD_SX_EXIT                 }	/* cc:  AD_CC6_EOS    */
    },
    {	/* state:  AD_S3_IN_RANGE_AFTER_DASH  */
	/* ---------------------------------- */
	    { AD_ACT_E, AD_S4_IN_RANGE_DASH_NOT_OK },	/* cc:  AD_CC0_NORMAL */
	    { AD_ACT_H, AD_SX_EXIT                 },	/* cc:  AD_CC1_DASH   */
	    { AD_ACT_G, AD_SX_EXIT                 },	/* cc:  AD_CC2_ONE    */
	    { AD_ACT_G, AD_SX_EXIT                 },	/* cc:  AD_CC3_ANY    */
	    { AD_ACT_G, AD_SX_EXIT                 },	/* cc:  AD_CC4_LBRAC  */
	    { AD_ACT_H, AD_SX_EXIT                 },	/* cc:  AD_CC5_RBRAC  */
	    { AD_ACT_H, AD_SX_EXIT                 }	/* cc:  AD_CC6_EOS    */
    },
    {	/* state:  AD_S4_IN_RANGE_DASH_NOT_OK */
	/* ---------------------------------- */
	    { AD_ACT_D, AD_S2_IN_RANGE_DASH_IS_OK  },	/* cc:  AD_CC0_NORMAL */
	    { AD_ACT_H, AD_SX_EXIT                 },	/* cc:  AD_CC1_DASH   */
	    { AD_ACT_G, AD_SX_EXIT                 },	/* cc:  AD_CC2_ONE    */
	    { AD_ACT_G, AD_SX_EXIT                 },	/* cc:  AD_CC3_ANY    */
	    { AD_ACT_G, AD_SX_EXIT                 },	/* cc:  AD_CC4_LBRAC  */
	    { AD_ACT_F, AD_S0_NOT_IN_RANGE         },	/* cc:  AD_CC5_RBRAC  */
	    { AD_ACT_H, AD_SX_EXIT                 }	/* cc:  AD_CC6_EOS    */
    }
};


/*
** Name: adu_cbldkey() - Build a "c", "text", "char", or "varchar" key.
**
** Description:
**	  adu_cbldkey() tries to build a key value of the type and length
**  required.  It assumes that adc_kblk->adc_lokey describes the type and
**  length of the key desired.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_qlang			The query language in use.  This is
**					necessary to determine whether QUEL
**					pattern matching characters should be
**					looked for.
**      adc_kblk  
**	    .adc_kdv			The DB_DATA_VALUE to form
**					a key for.
**		.db_datatype		Datatype of data value.
**		.db_length		Length of data value.
**		.db_data		Pointer to the actual data.
**	    .adc_opkey			Key operator used to tell this
**					routine what kind of comparison
**					operation the key is to be
**					built for, as described above.
**					NOTE: ADI_OP_IDs should not be used
**					directly.  Rather the caller should
**					use the adi_opid() routine to obtain
**					the value that should be passed in here.
**          .adc_lokey			DB_DATA_VALUE to recieve the
**					low key, if built.  Note, its datatype
**					and length should be the same as
**					.adc_hikey's datatype and length.
**		.db_datatype		Datatype for low key.
**	    	.db_length		Length for low key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the low key.  If this
**					is NULL, no data will be written.
**	    .adc_hikey			DB_DATA_VALUE to recieve the
**					high key, if built.  Note, its datatype
**					and length should be the same as 
**					.adc_lokey's datatype and length.
**		.db_datatype		Datatype for high key.
**		.db_length		Length for high key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the high key.  If this
**					is NULL, no data will be written.
**	    .adc_escape			*** USED ONLY FOR THE `LIKE' familiy
**					of operators, and then only if
**					.adc_pat_flags AD_PAT_HAS_ESCAPE bit set
**					The escape char to use, if there was an
**					ESCAPE clause with the LIKE or NOT LIKE
**					predicate.
**	    .adc_pat_flags		*** USED ONLY FOR THE `LIKE' family
**					of operators ***
**					If there was an ESCAPE clause with the
**					LIKE or NOT LIKE predicate, then supply
**					.adc_pat_flags with AD_PAT_HAS_ESCAPE set,
**					and set .adc_escape to the escape char.
**					If there is not an ESCAPE clause with the
**					LIKE or NOT LIKE predicate, then this bit
**					must cleared. It is a good idea to set the
**					whole of .adc_pat_flags to 0 for other
**					operators that are not of the LIKE family.
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
**      adc_kblk  
**	    .adc_tykey			The type of key which was built,
**                                      as described above.  This will
**					be one of the following:
**						ADC_KALLMATCH
**						ADC_KNOMATCH
**						ADC_KEXACTKEY
**						ADC_KLOWKEY
**						ADC_KHIGHKEY
**						ADC_KRANGEKEY
**	    .adc_lokey
**		.db_data		If the low key was built, the
**					actual data for it will be put
**					at this location, unless .db_data
**					is NULL.
**          .adc_hikey
**		.db_data		If the high key was built, the
**					actual data for it will be put
**					at this location, unless .db_data
**					is NULL.
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed successfully.
**
**  History:
**      02/22/83 (lichtman) -- made to work with both "c" type
**                  and new "text" type
**      26 mar 85 (ejl) -- Changed the min pad char from 0 to use MIN_CHAR
**          in the "c" case, since 0 isn't a legal char for "c".  This was
**          affecting JENUM, see diff.c.
**	09-jun-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	20-oct-86 (thurston)
**	    Added a conditional check for the LIKE operator, and if found call
**	    on the new special routine to process that.
**	20-oct-87 (thurston)
**	    Added code to check for patterns that are not string datatypes
**          (such as i, or f, etc.), and if found return ALLMATCH. 
**	02-jun-88 (thurston)
**	    Added comments to the input parameters for the new .adc_escape and
**	    .adc_isescape fields in the ADC_KEY_BLK.  No actual source change
**	    needs to happen in this file ... the lower level key building
**	    routine used solely for the LIKE operator will have to deal with it.
**	01-mar-2008 (gupsh01)
**	    UTF8 installs should use the unicode key building routine 
**	    adu_nvchbld_key.
**	11-apr-2008 (gupsh01)
**	    Pass the semantics to the UTF8 key building routine.
**	27-Oct-2008 (kiria01) SIR120473
**	    Handoff LIKE work to pattern code.
**	09-dec-2008 (gupsh01)
**	    Add the nchar/nvarchar type to the valid string types for which
**	    ADC_KALLMATCH should not be returned.
**	25-Mar-2009 (kiria01) b121841
**	    Adjust prior change to only work in UTF8 enabled installations.
**	    Otherwise Unicode collation will roun counter to the key
**	    comparisons potentially skipping matches.
*/

DB_STATUS
adu_cbldkey(
ADF_CB          *adf_scb,
ADC_KEY_BLK	*adc_kblk)
{
    DB_DT_ID	    dtp;
    DB_DT_ID	    dtk;
    i4		    semantics;
    ADI_OP_ID	    opid = adc_kblk->adc_opkey;
    DB_STATUS	    db_stat = E_DB_OK;
    i4		    in_bdt  = abs((i4) adc_kblk->adc_kdv.db_datatype);
    i4		    ky_bdt  = abs((i4) adc_kblk->adc_lokey.db_datatype);

    /*
    ** Get the appropriate semantics for the comparison.
    */
    
    dtp = adc_kblk->adc_kdv.db_datatype;
    dtk = adc_kblk->adc_lokey.db_datatype;
    if (dtp == DB_CHR_TYPE  ||  dtk == DB_CHR_TYPE)
	semantics = AD_C_SEMANTICS;
    else if (dtp == DB_TXT_TYPE  ||  dtk == DB_TXT_TYPE)
	semantics = AD_T_SEMANTICS;
    else
	semantics = AD_VC_SEMANTICS;

    /* Check for precompiled pattern. */
    if (dtp == DB_PAT_TYPE)
	return adu_patcomp_kbld(adf_scb, adc_kblk);

    /*
    ** Check for pattern being a non-string type.  If so, return ALLMATCH.
    */
    
    if (dtp != DB_CHR_TYPE &&
	dtp != DB_CHA_TYPE &&
	dtp != DB_TXT_TYPE &&
	dtp != DB_VCH_TYPE &&
	dtp != DB_LTXT_TYPE &&
	dtp != DB_NCHR_TYPE &&
	dtp != DB_NVCHR_TYPE)
    {
	adc_kblk->adc_tykey = ADC_KALLMATCH;
	return (E_DB_OK);
    }

    switch (opid)
    {
    case ADI_LIKE_OP:
	/*adc_kblk->adc_tykey = ADC_KRANGEKEY;*/
	break;

    case ADI_EQ_OP:
	/*adc_kblk->adc_tykey = ADC_KEXACTKEY;*/
	if (in_bdt == DB_CHR_TYPE  &&  ky_bdt != DB_CHR_TYPE)
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	break;

    case ADI_GT_OP:
    case ADI_GE_OP:
	/*adc_kblk->adc_tykey = ADC_KLOWKEY;*/
	break;

    case ADI_LT_OP:
    case ADI_LE_OP:
	/*adc_kblk->adc_tykey = ADC_KHIGHKEY;*/
	break;

    case ADI_NE_OP:
	/*adc_kblk->adc_tykey = ADC_KNOKEY;*/
	break;

    default:
	db_stat = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
    }

    /*
    ** Check for the special case where the data value to build the key from is
    ** an unknown run-time parameter(*) (e.g. repeat queries) and the comparison
    ** operator is `=' or `LIKE'.  In this case, return the worst case key type,
    ** ignoring the possibility of pattern match characters; i.e. ADC_KEXACTKEY
    ** for `=', and ADC_KRANGEKEY for `LIKE'.
    **
    ** (*) Note:  If this routine sees that the data value to build the key 
    **            from has a NULL pointer to its data then it will be assumed
    **            that it represents an unknown run-time parameter.  (i.e. if
    **            (adc_kblk->adc_kdv.db_data == NULL) then the data value to
    **            build the key from is an unknown run-time parameter
    */

    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
    {
	/* On a UTF8 install let the UTF8 key building routine
	** handle the key building unless no key available.
	*/
	if (adc_kblk->adc_kdv.db_data != NULL)
	    db_stat = adu_utf8cbldkey( adf_scb, semantics, adc_kblk);

	return db_stat;
    }

    if (dtp == DB_NCHR_TYPE ||
	dtp == DB_NVCHR_TYPE)
    {
	/* Given a Unicode key to compare against a non-Unicode
	** column (&possibly index) there is no key that can be
	** usefully formed to reduce the scan. This is due to the
	** Unicode comparisons ultimatly using CE lists that support
	** such things as characters that are ignore in compares
	** thus rendering such a comparison with raw keys futile. */
	adc_kblk->adc_tykey = ADC_KALLMATCH;
	return (E_DB_OK);
    }

    if (adc_kblk->adc_kdv.db_data == NULL)
    {
	return (db_stat);
    }

    if (opid == ADI_EQ_OP  ||  opid == ADI_LIKE_OP || opid == ADI_NE_OP)
    {
	/*
	** Run the finite state machine for building string keys for equality.
	*/
	return (key_fsm(adf_scb, semantics, adc_kblk));
    }
    else
    {
	/*
	** Run the finite state machine for comparing strings for <, >, <=, >=.
	*/
	return (key_ltgt(adf_scb, semantics, adc_kblk));
    }
}

/*
** Name: adu_utf8cbldkey() - Build a "c", "text", "char", or "varchar" key.
**			     in a UTF8 installation.
**
** Description:
**	  adu_utf8cbldkey() is a wrapper routine for building a key value
**	  for character types on a UTF8 installation.
**
**	  The unicode data in the datatypes above requires special handling
**	  and collation dependency which is not the case for the other
**	  types. Moreover while building the key data is normalized as
**	  well by the unicode routines.
**	
**	  This routine converts the UTF8 values to UTF16 and then calls
**	  the appropriate unicode routine to build the key. It then
**	  coerces back to UTF8 and places the results in the high key and
**	  the low key area.
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
**	    E_AD0000_OK			Completed successfully.
**
**  History:
**	04-Mar-2008 (gupsh01)
**	    Initial creation.
**	31-Mar-2008 (gupsh01)
**	    Add support for building key with C and text semantics.
**	07-Jan-2009 (gupsh01)
**	    Use global defines for temporary memory space to build
**	    UTF16 keys.  
*/
DB_STATUS
adu_utf8cbldkey(
ADF_CB          *adf_scb,
i4		semantics,
ADC_KEY_BLK	*key_block)
{
    DB_DATA_VALUE   save_pat_dv;
    DB_DATA_VALUE   save_lo_dv; 
    DB_DATA_VALUE   save_hi_dv;
    DB_DATA_VALUE   coerce_dv;
    DB_STATUS       result = E_DB_OK;
    STATUS          mstat = E_DB_OK;
    bool            gotmemory = FALSE;
    i4		    wksize, /* Size of the workspace */
		    klen, lo_pos, lo_len, hi_pos, hi_len;		
    u_i2	    workspace[ADF_UNI_TMPLEN_UCS2] = {0};
    u_i2 	    *wkspace = (u_i2 *)&workspace[0];
    bool	    lowAllocated = FALSE;
    bool	    hiAllocated = FALSE;

    if (!(adf_scb->adf_utf8_flag & AD_UTF8_ENABLED))
    {
	/* Return an internal error if this routine is called in error */
        return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }

    /* Save the keys in the local variable */
    STRUCT_ASSIGN_MACRO(key_block->adc_lokey, save_lo_dv);
    STRUCT_ASSIGN_MACRO(key_block->adc_hikey, save_hi_dv);
    STRUCT_ASSIGN_MACRO(key_block->adc_kdv, save_pat_dv);

    /* Set up offset lengths */
    klen = key_block->adc_kdv.db_length * sizeof(UCS2) + (DB_CNTSIZE);

    lo_pos = key_block->adc_kdv.db_length + 1;
    lo_len = key_block->adc_lokey.db_length * sizeof(UCS2) + (DB_CNTSIZE); 

    hi_pos = lo_pos + key_block->adc_lokey.db_length + 1;
    hi_len= key_block->adc_hikey.db_length * sizeof(UCS2) + (DB_CNTSIZE);

    wksize = hi_pos * sizeof(UCS2) + hi_len;

    /* Allocate memory for UTF16 keys if necessary */
    if (wksize > ADF_UNI_TMPLEN )
    {
      wkspace = (u_i2 *)MEreqmem(0, wksize, FALSE, &mstat);
      if (mstat != OK)
        return (adu_error(adf_scb, E_AD2042_MEMALLOC_FAIL, 2,
                            (i4) sizeof(mstat), (i4 *)&mstat));
      else
        gotmemory = TRUE;

      MEfill(wksize, '\0', wkspace); /* NULL FILL TO AVOID ERRORS */
    }

    if (key_block->adc_kdv.db_data)
    {
      /* if a pattern was supplied coerce the key to unicode type */
      STRUCT_ASSIGN_MACRO(key_block->adc_kdv, coerce_dv);
      coerce_dv.db_data = (PTR)wkspace;
      coerce_dv.db_datatype = DB_NVCHR_TYPE;
      coerce_dv.db_length = klen;
      result = adu_nvchr_coerce(adf_scb, &key_block->adc_kdv, &coerce_dv);
      if (result != E_DB_OK)
          return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));

      STRUCT_ASSIGN_MACRO(coerce_dv, key_block->adc_kdv);
    }
    else /* Something wrong here this routine should receive a 
	 ** pattern. Return an error */
          return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));

    /* set the low key holding area */
    if (key_block->adc_lokey.db_data)
    {
      key_block->adc_lokey.db_data = (PTR)&wkspace[lo_pos];

      /* lokey is not build for < and <> operations */
      if ((key_block->adc_opkey != ADI_NE_OP) &&
          (key_block->adc_opkey != ADI_LE_OP) &&
          (key_block->adc_opkey != ADI_LT_OP)) 
        lowAllocated = TRUE;
    }
    key_block->adc_lokey.db_length= lo_len;
    key_block->adc_lokey.db_datatype = DB_NVCHR_TYPE;

    /* set the high key holding area */
    if (key_block->adc_hikey.db_data)
    {
      key_block->adc_hikey.db_data = (PTR)&wkspace[hi_pos];

      /* hikey is not build for > and <> */
      if ((key_block->adc_opkey != ADI_NE_OP) &&
          (key_block->adc_opkey != ADI_GE_OP) &&
          (key_block->adc_opkey != ADI_GT_OP)) 
      hiAllocated = TRUE;
    }
    key_block->adc_hikey.db_length= hi_len;
    key_block->adc_hikey.db_datatype = DB_NVCHR_TYPE;

    /* Call the key building routine now */
    if (result = adu_nvchr_utf8_bldkey( adf_scb, semantics, 
		key_block) != E_DB_OK)
    {
      /* Failed to build the key replace the original values and
      ** exit 
      */
      STRUCT_ASSIGN_MACRO( save_pat_dv, key_block->adc_kdv);
      STRUCT_ASSIGN_MACRO( save_lo_dv, key_block->adc_lokey);
      STRUCT_ASSIGN_MACRO( save_hi_dv, key_block->adc_hikey);
      return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }

    STRUCT_ASSIGN_MACRO( save_pat_dv, key_block->adc_kdv);

    STRUCT_ASSIGN_MACRO( key_block->adc_lokey, coerce_dv);
    STRUCT_ASSIGN_MACRO( save_lo_dv, key_block->adc_lokey);
    if (lowAllocated)
    { 
      if (result = adu_nvchr_coerce(adf_scb, &coerce_dv, 
			&key_block->adc_lokey) != E_DB_OK)
      {
        /* Could not coerce correctly, replace the hi key with 
	** original value before exiting. 
	*/
        STRUCT_ASSIGN_MACRO( save_hi_dv, key_block->adc_hikey);
        return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
      }
    }

    STRUCT_ASSIGN_MACRO( key_block->adc_hikey, coerce_dv);
    STRUCT_ASSIGN_MACRO( save_hi_dv, key_block->adc_hikey);
    if (hiAllocated)
    {
      if (result = adu_nvchr_coerce(adf_scb, &coerce_dv, 
			&key_block->adc_hikey) != E_DB_OK)
        return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }

    if (gotmemory)
	MEfree ((PTR)wkspace);

    return (E_DB_OK);
}


/*
** Name: adu_dbldkey() - Build a "date" key.
**
**	adu_dbldkey() builds a key value of the type and length required.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_kblk  
**	    .adc_kdv			The DB_DATA_VALUE to form
**					a key for.
**		.db_datatype		Datatype of data value.
**		.db_length		Length of data value.
**		.db_data		Ptr to the actual data.
**	    .adc_opkey			Key operator used to tell this
**					routine what kind of comparison
**					operation the key is to be
**					built for, as described above.
**					NOTE: ADI_OP_IDs should not be used
**					directly.  Rather the caller should
**					use the adi_opid() routine to obtain
**					the value that should be passed in here.
**          .adc_lokey			DB_DATA_VALUE to recieve the
**					low key, if built.  Note, its datatype
**					and length should be the same as
**					.adc_hikey's datatype and length.
**		.db_datatype		Datatype for low key.
**	    	.db_length		Length for low key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the low key.
**	    .adc_hikey			DB_DATA_VALUE to recieve the
**					high key, if built.  Note, its datatype
**					and length should be the same as 
**					.adc_lokey's datatype and length.
**		.db_datatype		Datatype for high key.
**		.db_length		Length for high key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the high key.
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
**      *adc_kblk  
**	    .adc_tykey			The type of key which was built,
**                                      as described above.  This will
**					be one of the following:
**						ADC_KALLMATCH
**						ADC_KNOMATCH
**						ADC_KEXACTKEY
**						ADC_KLOWKEY
**						ADC_KHIGHKEY
**						ADC_KRANGEKEY
**	    .adc_lokey
**		*.db_data		If the low key was built, the
**					actual data for it will be put
**					at this location, unless .db_data
**					is NULL.
**          .adc_hikey
**		*.db_data		If the high key was built, the
**					actual data for it will be put
**					at this location, unless .db_data
**					is NULL.
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
**	    E_AD0000_OK			Completed successfully.
**	    E_AD9999_INTERNAL_ERROR	Input data value was not valid.
**
**  History:
**	30-jul-87 (thurston)
**          Recoded to accept other than date data values as input and to be a
**	    bit smarter.
**      08-jul-1999 (gygro01)
**	    Xintegration of change for bug 42085 and 42341 fixed by stevet
**	    (14-feb-92). Fix changes the return keytype for an empty date
**	    for ADI_LT_OP and ADI_LE_OP from EXACTKEY to RANGEKEY.
**	    Returning EXACTKEY when QEF/ADE expecting RANGEKEY cause QEF/SCF
**	    to parse the query again  until the retry count exceed.
**	    In case of repeat query, QEF will request FE to resend query
**	    text and causing an infinite loop.
**	31-aug-2006 (gupsh01)
**	    Added support for new ANSI date/time types.
**	15-sep-2006 (gupsh01)
**	    Fixed conversion from any other datatype to date/time types 
**	    family member.
*/

DB_STATUS
adu_dbldkey(
ADF_CB              *adf_scb,
ADC_KEY_BLK	    *adc_kblk)
{
    AD_NEWDTNTRNL        *dptr;
    char		*lo_date;
    char		*hi_date;
    DB_DATA_VALUE	ddv;
    AD_NEWDTNTRNL       tempdt;
    DB_STATUS		db_stat;
    i4		err;


    if (adc_kblk->adc_kdv.db_data == NULL)
    {
	/* Parameter marker or function obscuring data.
	** Use pre-loaded vale in adc_tykey - without a key
	** we cannot improve estimate.
	** All other datatypes are handled by the coercions
	** below and there are currently none to handle
	** differently */
	return E_DB_OK;
    }
    /* What is the input datatype?  If not date, attempt to convert */
    if ((adc_kblk->adc_kdv.db_datatype == DB_DTE_TYPE) ||
       (adc_kblk->adc_kdv.db_datatype == DB_ADTE_TYPE) ||
       (adc_kblk->adc_kdv.db_datatype == DB_TMWO_TYPE) ||
       (adc_kblk->adc_kdv.db_datatype == DB_TMW_TYPE) ||
       (adc_kblk->adc_kdv.db_datatype == DB_TME_TYPE) ||
       (adc_kblk->adc_kdv.db_datatype == DB_TSWO_TYPE) ||
       (adc_kblk->adc_kdv.db_datatype == DB_TSW_TYPE) ||
       (adc_kblk->adc_kdv.db_datatype == DB_TSTMP_TYPE) ||
       (adc_kblk->adc_kdv.db_datatype == DB_INYM_TYPE) ||
       (adc_kblk->adc_kdv.db_datatype == DB_INDS_TYPE)) 
    {
	if (db_stat = adu_6to_dtntrnl(adf_scb, &(adc_kblk->adc_kdv), &tempdt))
	    return (db_stat);
	dptr = &tempdt;
    }
    else 
    {
	char coerbuf[15]; /* big enough buffer for largest date type */
	MEfill (sizeof(coerbuf), NULLCHAR, (PTR)coerbuf);

	/* Coerce to the intended date family member */
        ddv.db_datatype = adc_kblk->adc_lokey.db_datatype;
        ddv.db_length = adc_kblk->adc_lokey.db_length;
        ddv.db_data = (PTR) &coerbuf;
        db_stat = adc_cvinto(adf_scb, &adc_kblk->adc_kdv, &ddv);
        err = adf_scb->adf_errcb.ad_errcode;
        if (DB_SUCCESS_MACRO(db_stat))
        {
	    if (db_stat = adu_6to_dtntrnl (adf_scb, &ddv, &tempdt))
		return (db_stat);
	    dptr = &tempdt;
        }
        else if (err == E_AD2009_NOCOERCION)
        {
            return (E_DB_ERROR);
        }
        else
        {
            adc_kblk->adc_tykey = ADC_KNOMATCH;
            return (E_DB_OK);
        }
    }
    /* Now set result keys based on operator */

    lo_date = (char *) adc_kblk->adc_lokey.db_data;
    hi_date = (char *) adc_kblk->adc_hikey.db_data;

    switch (adc_kblk->adc_opkey)
    {
      case ADI_EQ_OP:
      case ADI_NE_OP:
	if (lo_date != NULL)
	   if (db_stat = adu_7from_dtntrnl (adf_scb, &adc_kblk->adc_lokey, &tempdt))
	       return (db_stat);
	break;

      case ADI_GT_OP:
      case ADI_GE_OP:
	if (lo_date != NULL)
	   if (db_stat = adu_7from_dtntrnl (adf_scb, &adc_kblk->adc_lokey, &tempdt))
		return (db_stat);
	break;

      case ADI_LE_OP:
      case ADI_LT_OP:
	    if (hi_date != NULL)
	      if (db_stat = adu_7from_dtntrnl (adf_scb, &adc_kblk->adc_hikey, &tempdt))
		  return (db_stat);
	break;

      default:
	return (adu_error(adf_scb, E_AD3002_BAD_KEYOP, 0));
    }

    return (E_DB_OK);
}


/*
** Name: adu_fbldkey() -  Build an "f" key.
**
** Description:
**    adu_fbldkey() tries to build a key value of the type and length required.
**  It assumes that adc_kblk->adc_lokey and adc_kblk->adc_hikey describe the
**  type and length of the key desired.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_kblk  
**	    .adc_kdv			The DB_DATA_VALUE to form
**					a key for.
**		.db_datatype		Datatype of data value.
**		.db_prec		Precision/Scale of data value
**		.db_length		Length of data value.
**		.db_data		Pointer to the actual data.
**	    .adc_opkey			Key operator used to tell this
**					routine what kind of comparison
**					operation the key is to be
**					built for, as described above.
**					NOTE: ADI_OP_IDs should not be used
**					directly.  Rather the caller should
**					use the adi_opid() routine to obtain
**					the value that should be passed in here.
**          .adc_lokey			DB_DATA_VALUE to recieve the
**					low key, if built.  Note, its datatype
**					and length should be the same as
**					.adc_hikey's datatype and length.
**		.db_datatype		Datatype for low key.
**	    	.db_length		Length for low key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the low key.  If this
**					is NULL, no data will be written.
**	    .adc_hikey			DB_DATA_VALUE to recieve the
**					high key, if built.  Note, its datatype
**					and length should be the same as 
**					.adc_lokey's datatype and length.
**		.db_datatype		Datatype for high key.
**		.db_length		Length for high key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the high key.  If this
**					is NULL, no data will be written.
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
**      *adc_kblk  
**	    .adc_tykey			The type of key which was built,
**                                      as described above.  This will
**					be one of the following:
**						ADC_KALLMATCH
**						ADC_KNOMATCH
**						ADC_KEXACTKEY
**						ADC_KLOWKEY
**						ADC_KHIGHKEY
**						ADC_KRANGEKEY
**	    .adc_lokey
**		*.db_data		If the low key was built, the
**					actual data for it will be put
**					at this location, unless .db_data
**					is NULL.
**          .adc_hikey
**		*.db_data		If the high key was built, the
**					actual data for it will be put
**					at this location, unless .db_data
**					is NULL.
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
**	    E_AD0000_OK				Completed successfully.
**
**  History
**      1-jul-85 (dreyfus) -- handle situation where converting from an f8 to
**	    an f4 loses information. In this case, return kNOMATCH
**      20-feb-1986 (dreyfus) -- back out above change because not all machines 
**          can convert f8 to f4 to f8 such that the first f8 = second f8.
**	09-jun-86 (ericj)
**	    Converted for Jupiter.
**	31-jul-87 (thurston)
**	    Totally rewrote this routine to be (a) more efficient and (b) able
**	    to handle input datatypes other than integers.
**	07-jul-89 (jrb)
**	    Added decimal as a datatype which can be used for building floating
**	    keys.
**      01-apr-1994 (stevet)
**          Added min/max check for float keybld routine similar to
**          the integer keybld routine.  If input value is above the
**          max/min value for the result data type, then NOMATCH
**          or ALLMATCH will be returned.
**	14-May-2004 (schka24)
**	    Add i8 to in-line conversion.
*/

DB_STATUS
adu_fbldkey(
ADF_CB              *adf_scb,
ADC_KEY_BLK	    *adc_kblk)
{
    DB_STATUS		db_stat;
    f8			f8temp;
    /* The following money vars are only used if input is money or string */
    AD_MONEYNTRNL	m;
    AD_MONEYNTRNL	*mptr = (AD_MONEYNTRNL *)adc_kblk->adc_kdv.db_data;
    DB_DATA_VALUE	mdv;
    f8   		maxf;
    f8    		minf;
    bool		overmax = FALSE;
    bool		undermin = FALSE;

    if (adc_kblk->adc_kdv.db_data == NULL)
    {
	/* Parameter marker or function obscuring data.
	** Use pre-loaded vale in adc_tykey - without a key
	** we cannot improve estimate.
	** All unhandled datatypes will need a full scan */
	switch (adc_kblk->adc_kdv.db_datatype)
	{
	case DB_INT_TYPE:
	case DB_DEC_TYPE:
	case DB_FLT_TYPE:
	case DB_CHR_TYPE:
	case DB_CHA_TYPE:
	case DB_TXT_TYPE:
	case DB_VCH_TYPE:
	case DB_NVCHR_TYPE:
	case DB_NCHR_TYPE:
	case DB_MNY_TYPE:
	    break;
	default:
	    /* Force a scan */
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	}
	return E_DB_OK;
    }
    switch (adc_kblk->adc_lokey.db_length)
    {
      case 4:
	maxf = FLT_MAX;
	break;
      case 8:
	maxf = DBL_MAX;
	break;
      default:
	return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }
    minf = -maxf;

    /* Convert pattern value to an f8 */
    switch (adc_kblk->adc_kdv.db_datatype)
    {
      case DB_INT_TYPE:
	switch (adc_kblk->adc_kdv.db_length)
	{
	  case 8:
	    f8temp = *(i8 *)adc_kblk->adc_kdv.db_data;
	    break;
	  case 4:
	    f8temp = *(i4 *)adc_kblk->adc_kdv.db_data;
	    break;
	  case 2:
	    f8temp = *(i2 *)adc_kblk->adc_kdv.db_data;
	    break;
	  case 1:
	    f8temp = I1_CHECK_MACRO(*(i1 *)adc_kblk->adc_kdv.db_data);
	    break;
	}
	break;

      case DB_DEC_TYPE:
	CVpkf((PTR)adc_kblk->adc_kdv.db_data,
		(i4)DB_P_DECODE_MACRO(adc_kblk->adc_kdv.db_prec),
		(i4)DB_S_DECODE_MACRO(adc_kblk->adc_kdv.db_prec),
		&f8temp);
	break;
	
      case DB_FLT_TYPE:
	if (adc_kblk->adc_kdv.db_length == 8)
	{
	    f8temp = *(f8 *)adc_kblk->adc_kdv.db_data;
	}
	else
	{
	    /* If column is f8, we really should build a RANGEKEY here for `=',
	    ** since the f8 values will be converted to f4, thus loosing
	    ** precision ... this means that several f8's will map to the same
	    ** f4 value.  But how do you find the proper range????  Also,
	    ** for the LOWKEY and HIGHKEY that will be created for `>' and `<',
	    ** the same loss of precision may miss some values that we should
	    ** catch.  (Gene Thurston)
	    */
	    f8temp = *(f4 *)adc_kblk->adc_kdv.db_data;
	}
	break;

      case DB_CHR_TYPE:
      case DB_CHA_TYPE:
      case DB_TXT_TYPE:
      case DB_VCH_TYPE:
      case DB_NVCHR_TYPE:
      case DB_NCHR_TYPE:
	/* Must convert to money, and then use */
	mdv.db_datatype = DB_MNY_TYPE;
	mdv.db_length = ADF_MNY_LEN;
	mdv.db_data = (PTR) &m;
	db_stat = adc_cvinto(adf_scb, &adc_kblk->adc_kdv, &mdv);
	if (DB_SUCCESS_MACRO(db_stat))
	{
	    mptr = (AD_MONEYNTRNL *) mdv.db_data;
	}
	else
	{
	    adc_kblk->adc_tykey = ADC_KNOMATCH;
	    return (E_DB_OK);
	}
	/* Purposely no `break;' stmt here */

      case DB_MNY_TYPE:
	f8temp = mptr->mny_cents / 100;
	break;

      default:
	/* Force a scan */
	adc_kblk->adc_tykey = ADC_KALLMATCH;
	return (E_DB_OK);
    }

    if (f8temp > maxf)
	overmax = TRUE;
    else if (f8temp < minf)
	undermin = TRUE;

    switch (adc_kblk->adc_opkey)
    {
      case ADI_EQ_OP:
      case ADI_NE_OP:
	if (overmax || undermin)
	{
	    adc_kblk->adc_tykey = ADC_KNOMATCH;
	}
	else
	{
	    if (adc_kblk->adc_opkey == ADI_EQ_OP)
	      adc_kblk->adc_tykey = ADC_KEXACTKEY;
	     else adc_kblk->adc_tykey = ADC_KNOKEY;
	    if (adc_kblk->adc_lokey.db_data != NULL)
	    {
		if (adc_kblk->adc_lokey.db_length == 4)
		    *(f4 *)adc_kblk->adc_lokey.db_data = f8temp;
		else
		    *(f8 *)adc_kblk->adc_lokey.db_data = f8temp;
	    }
	}
	break;

      case ADI_GT_OP:
      case ADI_GE_OP:
	if (overmax)
	{
	    adc_kblk->adc_tykey = ADC_KNOMATCH;
	}
	else if (undermin)
	{
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	}
	else
	{
	    adc_kblk->adc_tykey = ADC_KLOWKEY;
	    if (adc_kblk->adc_lokey.db_data != NULL)
	    {
		if (adc_kblk->adc_lokey.db_length == 4)
		    *(f4 *)adc_kblk->adc_lokey.db_data = f8temp;
		else
		    *(f8 *)adc_kblk->adc_lokey.db_data = f8temp;
	    }
	}
	break;

      case ADI_LT_OP:
      case ADI_LE_OP:
	if (overmax)
	{
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	}
	else if (undermin)
	{
	    adc_kblk->adc_tykey = ADC_KNOMATCH;
	}
	else
	{
	    adc_kblk->adc_tykey = ADC_KHIGHKEY;
	    if (adc_kblk->adc_hikey.db_data != NULL)
	    {
		if (adc_kblk->adc_hikey.db_length == 4)
		    *(f4 *)adc_kblk->adc_hikey.db_data = f8temp;
		else
		    *(f8 *)adc_kblk->adc_hikey.db_data = f8temp;
	    }
	}
	break;
	    
      default:
	return (adu_error(adf_scb, E_AD3002_BAD_KEYOP, 0));
    }

    return (E_DB_OK);
}


/*
** Name: adu_ibldkey() - Build an "i" key.
**
** Description:
**	  adu_ibldkey() tries to build a key value of the type and length
**	required.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_kblk  
**	    .adc_kdv			The DB_DATA_VALUE to form
**					a key for.
**		.db_datatype		Datatype of data value.
**		.db_prec		Precision/Scale of data value
**		.db_length		Length of data value.
**		.db_data		Pointer to the actual data.
**	    .adc_opkey			Key operator used to tell this
**					routine what kind of comparison
**					operation the key is to be
**					built for, as described above.
**					NOTE: ADI_OP_IDs should not be used
**					directly.  Rather the caller should
**					use the adi_opid() routine to obtain
**					the value that should be passed in here.
**          .adc_lokey			DB_DATA_VALUE to recieve the
**					low key, if built.  Note, its datatype
**					and length should be the same as
**					.adc_hikey's datatype and length.
**		.db_datatype		Datatype for low key.
**	    	.db_length		Length for low key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the low key.  If this
**					is NULL, no data will be written.
**	    .adc_hikey			DB_DATA_VALUE to recieve the
**					high key, if built.  Note, its datatype
**					and length should be the same as 
**					.adc_lokey's datatype and length.
**		.db_datatype		Datatype for high key.
**		.db_length		Length for high key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the high key.  If this
**					is NULL, no data will be written.
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
**      adc_kblk  
**	    .adc_tykey			The type of key which was built,
**                                      as described above.  This will
**					be one of the following:
**						ADC_KALLMATCH
**						ADC_KNOMATCH
**						ADC_KEXACTKEY
**						ADC_KLOWKEY
**						ADC_KHIGHKEY
**						ADC_KRANGEKEY
**	    .adc_lokey
**		.db_data		If the low key was built, the
**					actual data for it will be put
**					at this location, unless .db_data
**					is NULL.
**          .adc_hikey
**		.db_data		If the high key was built, the
**					actual data for it will be put
**					at this location, unless .db_data
**					is NULL.
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed successfully.
**	    E_AD3002_BAD_KEYOP		Key op not known by this routine.
**
**  History:
**      11/08/84 (lichtman) -- used to assume that out-of-range numbers couldn't
**              match.  This is not true, for example, when making a comparison
**              like i1 < 1000.  Put in special case code to allow these types
**              of comparisons.
**      3/13/85 (dreyfus) -- don't use union ANYTYPE for copying the key
**		into the result.  This assumes an ordering of union elements
**		that might not hold on all machines.
**      1-jul-85 (dreyfus) -- check overflow using the input amount.  Also,
**		change algorithm to: Convert input to i4, check for overflow,
**		converto to key value.
**      7/11/85 (roger)
**	    	Use I1_CHECK_MACRO macro to ensure preservation of i1 values
**	    	across assignments on machines w/o signed chars.
**      8/22/85 (dreyfus) -- changes so i1's and i2's don't get into
**          	the result. significant high order bits dropped off.
**	09-jun-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	31-jul-87 (thurston)
**	    Totally rewrote this routine to be (a) more efficient and (b) able
**	    to handle input datatypes other than integers.
**	07-jul-89 (jrb)
**	    Added decimal as a type which can be used to build integer keys.
**	13-feb-04 (inkdo01)
**	    Add support of bigint.
**	14-May-2004 (schka24)
**	    Fix C type quirk with mini4.
**	5-aug-04 (inkdo01)
**	    Fix a typo in bigint support that made "<=" preds not work.
*/

DB_STATUS
adu_ibldkey(
ADF_CB              *adf_scb,
ADC_KEY_BLK	    *adc_kblk)
{
    DB_STATUS		db_stat;
    DB_DATA_VALUE	*kdv = &adc_kblk->adc_kdv;
    f8			f8temp;
    i8		itemp;
    i8		maxi;
    i8		mini;
    i4			keylen;
    bool		overmax = FALSE;
    bool		undermin = FALSE;
    bool		integral = TRUE;
    /* The following money vars are only used if input is money or string */
    AD_MONEYNTRNL	m;
    AD_MONEYNTRNL	*mptr = (AD_MONEYNTRNL *)kdv->db_data;
    DB_DATA_VALUE	mdv;


    if (adc_kblk->adc_kdv.db_data == NULL)
    {
	/* Parameter marker or function obscuring data.
	** Use pre-loaded vale in adc_tykey - without a key
	** we cannot improve estimate.
	** All unhandled datatypes will need a full scan */
	switch (kdv->db_datatype)
	{
	case DB_INT_TYPE:
	case DB_DEC_TYPE:
	case DB_FLT_TYPE:
	case DB_CHR_TYPE:
	case DB_CHA_TYPE:
	case DB_TXT_TYPE:
	case DB_VCH_TYPE:
	case DB_NVCHR_TYPE:
	case DB_NCHR_TYPE:
	case DB_MNY_TYPE:
	    break;
	default:
	    /* Force a scan */
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	}
	return E_DB_OK;
    }
    switch (adc_kblk->adc_lokey.db_length)
    {
      case 1:
	keylen = 1;
	maxi = MAXI1;
	mini = MINI1;
	break;
      case 2:
	keylen = 2;
	maxi = MAXI2;
	mini = MINI2;
	break;
      case 4:
	keylen = 4;
	maxi = MAXI4;
	mini = MINI4LL;		/* Not MINI4, "mini" is an i8 - see compat.h */
	break;
      case 8:
	keylen = 8;
	maxi = MAXI8;
	mini = MINI8;
	break;
      default:
	return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }


    /* Convert pattern value to an i4 */
    switch (kdv->db_datatype)
    {
      case DB_INT_TYPE:
	switch (kdv->db_length)
	{
	  case 8:
	    itemp = *(i8 *)kdv->db_data;
	    break;
	  case 4:
	    itemp = *(i4 *)kdv->db_data;
	    break;
	  case 2:
	    itemp = *(i2 *)kdv->db_data;
	    break;
	  case 1:
	    itemp = I1_CHECK_MACRO(*(i1 *)kdv->db_data);
	    break;
	}
	if (itemp > maxi)
	    overmax = TRUE;
	else if (itemp < mini)
	    undermin = TRUE;
	break;

      case DB_DEC_TYPE:
        {
	    i4	prec  = DB_P_DECODE_MACRO(kdv->db_prec);
	    i4	scale = DB_S_DECODE_MACRO(kdv->db_prec);
	    u_char	*p;
	    u_char	sign;

	    if (CVpkl8((PTR)kdv->db_data, prec, scale, &itemp) == CV_OVERFLOW)
	    {
		itemp = 0;
		sign = (((u_char *)kdv->db_data)[kdv->db_length-1] & 0xf);

		if (sign == MH_PK_MINUS  ||  sign == MH_PK_AMINUS)
		    undermin = TRUE;
		else
		    overmax = TRUE;
	    }

	    if (itemp > maxi)
		overmax = TRUE;
	    else if (itemp < mini)
		undermin = TRUE;

	    /* now search fractional portion of decimal value for non-zero
	    ** digits; if found then the number is not integral
	    */
	    p = (u_char *)(kdv->db_data) + (prec/2) - (scale/2);
	    while (scale > 0)
	    {
		if ((((scale-- & 1)  ?  *p >> 4  :  *p++) & 0xf) != 0)
		{
		    integral = FALSE;
		    break;
		}
	    }
        }
        break;
	
      case DB_FLT_TYPE:
	switch (kdv->db_length)
	{
	  case 8:
	    f8temp = *(f8 *)kdv->db_data;
	    break;
	  case 4:
	    f8temp = *(f4 *)kdv->db_data;
	    break;
	}
	if (f8temp > maxi)
	    overmax = TRUE;
	else if (f8temp < mini)
	    undermin = TRUE;
	if ((itemp = f8temp) != f8temp)
	    integral = FALSE;
	break;

      case DB_CHR_TYPE:
      case DB_CHA_TYPE:
      case DB_TXT_TYPE:
      case DB_VCH_TYPE:
      case DB_NVCHR_TYPE:
      case DB_NCHR_TYPE:
	/* Must convert to money, and then use */
	mdv.db_datatype = DB_MNY_TYPE;
	mdv.db_length = ADF_MNY_LEN;
	mdv.db_data = (PTR) &m;
	db_stat = adc_cvinto(adf_scb, &adc_kblk->adc_kdv, &mdv);
	if (DB_SUCCESS_MACRO(db_stat))
	{
	    mptr = (AD_MONEYNTRNL *) mdv.db_data;
	}
	else
	{
	    adc_kblk->adc_tykey = ADC_KNOMATCH;
	    return (E_DB_OK);
	}
	/* Purposely no `break;' stmt here */

      case DB_MNY_TYPE:
	f8temp = mptr->mny_cents / 100;
	if (f8temp > MAXI8)
	    overmax = TRUE;
	else if (f8temp < MINI8)
	    undermin = TRUE;
	else if ((itemp = f8temp) != f8temp)
	    integral = FALSE;
	break;

      default:
	/* Force a scan */
	adc_kblk->adc_tykey = ADC_KALLMATCH;
	return (E_DB_OK);
    }


    switch (adc_kblk->adc_opkey)
    {
      case ADI_EQ_OP:
      case ADI_NE_OP:
	if (!integral  ||  overmax  ||  undermin)
	{
	    adc_kblk->adc_tykey = ADC_KNOMATCH;
	}
	else
	{
	    if (adc_kblk->adc_opkey == ADI_EQ_OP)
	      adc_kblk->adc_tykey = ADC_KEXACTKEY;
	     else adc_kblk->adc_tykey = ADC_KNOKEY;
	    if (adc_kblk->adc_lokey.db_data != NULL)
	    {
		switch (keylen)
		{
		  case 1:
		    *(i1 *)adc_kblk->adc_lokey.db_data = itemp;
		    break;
		  case 2:
		    *(i2 *)adc_kblk->adc_lokey.db_data = itemp;
		    break;
		  case 4:
		    *(i4 *)adc_kblk->adc_lokey.db_data = itemp;
		    break;
		  case 8:
		    *(i8 *)adc_kblk->adc_lokey.db_data = itemp;
		    break;
		}
	    }
	}
	break;

      case ADI_GT_OP:
      case ADI_GE_OP:
	if (overmax)
	{
	    adc_kblk->adc_tykey = ADC_KNOMATCH;
	}
	else if (undermin)
	{
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	}
	else
	{
	    adc_kblk->adc_tykey = ADC_KLOWKEY;
	    if (adc_kblk->adc_lokey.db_data != NULL)
	    {
		if (!integral)
		    itemp++;

		switch (keylen)
		{
		  case 1:
		    *(i1 *)adc_kblk->adc_lokey.db_data = itemp;
		    break;
		  case 2:
		    *(i2 *)adc_kblk->adc_lokey.db_data = itemp;
		    break;
		  case 4:
		    *(i4 *)adc_kblk->adc_lokey.db_data = itemp;
		    break;
		  case 8:
		    *(i8 *)adc_kblk->adc_lokey.db_data = itemp;
		    break;
		}
	    }
	}
	break;

      case ADI_LT_OP:
      case ADI_LE_OP:
	if (overmax)
	{
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	}
	else if (undermin)
	{
	    adc_kblk->adc_tykey = ADC_KNOMATCH;
	}
	else
	{
	    adc_kblk->adc_tykey = ADC_KHIGHKEY;
	    if (adc_kblk->adc_hikey.db_data != NULL)
	    {
		switch (keylen)
		{
		  case 1:
		    *(i1 *)adc_kblk->adc_hikey.db_data = itemp;
		    break;
		  case 2:
		    *(i2 *)adc_kblk->adc_hikey.db_data = itemp;
		    break;
		  case 4:
		    *(i4 *)adc_kblk->adc_hikey.db_data = itemp;
		    break;
		  case 8:
		    *(i8 *)adc_kblk->adc_hikey.db_data = itemp;
		    break;
		}
	    }
	}
	break;

      default:
	return (adu_error(adf_scb, E_AD3002_BAD_KEYOP, 0));
    }

    return (E_DB_OK);
}


/*
** Name: adu_mbldkey() - Build a "money" key.
**
** Description:
**	  adu_mbldkey() tries to build a key value of the type and
**	length required.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_kblk  
**	    .adc_kdv			The DB_DATA_VALUE to form
**					a key for.
**		.db_datatype		Datatype of data value.
**		.db_prec		Precision/Scale of data value
**		.db_length		Length of data value.
**		.db_data		Pointer to the actual data.
**	    .adc_opkey			Key operator used to tell this
**					routine what kind of comparison
**					operation the key is to be
**					built for, as described above.
**					NOTE: ADI_OP_IDs should not be used
**					directly.  Rather the caller should
**					use the adi_opid() routine to obtain
**					the value that should be passed in here.
**          .adc_lokey			DB_DATA_VALUE to recieve the
**					low key, if built.  Note, its datatype
**					and length should be the same as
**					.adc_hikey's datatype and length.
**		.db_datatype		Datatype for low key.
**	    	.db_length		Length for low key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the low key.  If this
**					is NULL, no data will be written.
**	    .adc_hikey			DB_DATA_VALUE to recieve the
**					high key, if built.  Note, its datatype
**					and length should be the same as 
**					.adc_lokey's datatype and length.
**		.db_datatype		Datatype for high key.
**		.db_length		Length for high key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the high key.  If this
**					is NULL, no data will be written.
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
**	adc_kblk  
**	    .adc_tykey			The type of key which was built,
**                                      as described above.  This will
**					be one of the following:
**						ADC_KALLMATCH
**						ADC_KNOMATCH
**						ADC_KEXACTKEY
**						ADC_KLOWKEY
**						ADC_KHIGHKEY
**						ADC_KRANGEKEY
**	    .adc_lokey
**		.db_data		If the low key was built, the
**					actual data for it will be put
**					at this location, unless .db_data
**					is NULL.
**          .adc_hikey
**		.db_data		If the high key was built, the
**					actual data for it will be put
**					at this location, unless .db_data
**					is NULL.
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
**	    E_AD0000_OK			Completed successfully.
**
** History:
**	09-jun-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	31-jul-87 (thurston)
**          Recoded to accept other than money data values as
**	    input, and to be a bit smarter.
**	07-jul-89 (jrb)
**	    Added decimal as a type which can be used to build money keys.
**      26-mar-91 (jrb)
**          Bug 36421.  We were converting a float to a money just by storing it
**          directly into the money value (since money is represented with an f8
**          internally).  But all other coercions/conversions were being run
**          through adu_11mny_round() first which slightly alters the value.
**          Added a call to this routine for float to money key building.
**      14-aug-91 (jrb)
**          Added missing "break" stmt; this is part of the previous bug fix
**          (bug 36421) which somehow didn't get integrated.
**      14-jun-1993 (stevet)
**          Set 'err' before using it in DB_FLT_TYPE.
**      08-jan-1994 (stevet)
**          B36421 is back due to another problem with decimal->money
**          coercion problem on VMS.  Round the number after decimal->money
**          just like float correct this problem.
**	14-May-2004 (schka24)
**	    Add i8 to in-line conversion.
*/

DB_STATUS
adu_mbldkey(
ADF_CB              *adf_scb,
ADC_KEY_BLK	    *adc_kblk)
{
    AD_MONEYNTRNL	m;
    AD_MONEYNTRNL	*lo_mny;
    AD_MONEYNTRNL	*hi_mny;
    DB_DATA_VALUE	mdv;
    f8			cents;
    DB_STATUS		db_stat;
    i4		err;


    if (adc_kblk->adc_kdv.db_data == NULL)
    {
	/* Parameter marker or function obscuring data.
	** Use pre-loaded vale in adc_tykey - without a key
	** we cannot improve estimate.
	** All unhandled datatypes will need a full scan */
	switch (adc_kblk->adc_kdv.db_datatype)
	{
      /* Doesn't belong here, needs its own function in this file */
      case DB_PT_TYPE:
        TRdisplay("Error: adu_mbldkey for DB_PT_TYPE, not coded\n");
        break;

	case DB_MNY_TYPE:
	case DB_FLT_TYPE:
	case DB_DEC_TYPE:
	case DB_INT_TYPE:
	case DB_CHR_TYPE:
	case DB_CHA_TYPE:
	case DB_TXT_TYPE:
	case DB_VCH_TYPE:
	case DB_NVCHR_TYPE:
	case DB_NCHR_TYPE:
	    break;
	default:
	    /* Force a scan */
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	}
	return E_DB_OK;
    }
    /* What is the input datatype?  If not money, attempt to convert */
    switch (adc_kblk->adc_kdv.db_datatype)
    {
      case DB_MNY_TYPE:
	cents = ((AD_MONEYNTRNL *) adc_kblk->adc_kdv.db_data)->mny_cents;
	break;

      case DB_FLT_TYPE:
	if (adc_kblk->adc_kdv.db_length == 4)
	    m.mny_cents = *(f4 *)adc_kblk->adc_kdv.db_data * 100;
	else
	    m.mny_cents = *(f8 *)adc_kblk->adc_kdv.db_data * 100;

	db_stat = adu_11mny_round(adf_scb, &m);
	err = adf_scb->adf_errcb.ad_errcode;
	if (DB_SUCCESS_MACRO(db_stat))
	{
	    cents = m.mny_cents;
	}
	else if (err == E_AD5031_MAXMNY_OVFL)
	{
	    cents = AD_1MNY_MAX_NTRNL + 1;
	}
	else if (err == E_AD5032_MINMNY_OVFL)
	{
	    cents = AD_3MNY_MIN_NTRNL - 1;
	}
	else
	{
	    adc_kblk->adc_tykey = ADC_KNOMATCH;
	    return (E_DB_OK);
	}
	break;

      case DB_DEC_TYPE:
	CVpkf((PTR)adc_kblk->adc_kdv.db_data,
		(i4)DB_P_DECODE_MACRO(adc_kblk->adc_kdv.db_prec),
		(i4)DB_S_DECODE_MACRO(adc_kblk->adc_kdv.db_prec),
		&m.mny_cents);
	m.mny_cents *= 100;
        db_stat = adu_11mny_round(adf_scb, &m);
        err = adf_scb->adf_errcb.ad_errcode;
        if (DB_SUCCESS_MACRO(db_stat))
        {
            cents = m.mny_cents;
        }
        else if (err == E_AD5031_MAXMNY_OVFL)
        {
            cents = AD_1MNY_MAX_NTRNL + 1;
        }
        else if (err == E_AD5032_MINMNY_OVFL)
        {
            cents = AD_3MNY_MIN_NTRNL - 1;
        }
        else
        {
            adc_kblk->adc_tykey = ADC_KNOMATCH;
            return (E_DB_OK);
        }
        break;	

      case DB_INT_TYPE:
	switch (adc_kblk->adc_kdv.db_length)
	{
	  case 1:
	    cents = I1_CHECK_MACRO(*(i1 *)adc_kblk->adc_kdv.db_data);
	    break;
	  case 2:
	    cents = *(i2 *)adc_kblk->adc_kdv.db_data;
	    break;
	  case 4:
	    cents = *(i4 *)adc_kblk->adc_kdv.db_data;
	    break;
	  case 8:
	    cents = *(i8 *)adc_kblk->adc_kdv.db_data;
	    break;
	}
	cents *= 100;
	break;

      case DB_CHR_TYPE:
      case DB_CHA_TYPE:
      case DB_TXT_TYPE:
      case DB_VCH_TYPE:
      case DB_NVCHR_TYPE:
      case DB_NCHR_TYPE:
	mdv.db_datatype = DB_MNY_TYPE;
	mdv.db_length = ADF_MNY_LEN;
	mdv.db_data = (PTR) &m;
	db_stat = adc_cvinto(adf_scb, &adc_kblk->adc_kdv, &mdv);
	err = adf_scb->adf_errcb.ad_errcode;
	if (DB_SUCCESS_MACRO(db_stat))
	{
	    cents = ((AD_MONEYNTRNL *) mdv.db_data)->mny_cents;
	}
	else if (err == E_AD5031_MAXMNY_OVFL)
	{
	    cents = AD_1MNY_MAX_NTRNL + 1;
	}
	else if (err == E_AD5032_MINMNY_OVFL)
	{
	    cents = AD_3MNY_MIN_NTRNL - 1;
	}
	else if (err == E_AD2009_NOCOERCION)
	{
            return (E_DB_ERROR);
	}
	else
	{
	    adc_kblk->adc_tykey = ADC_KNOMATCH;
	    return (E_DB_OK);
	}
	break;

      default:
	/* Force a scan */
	adc_kblk->adc_tykey = ADC_KALLMATCH;
	return (E_DB_OK);
    }


    /* Now set result keys based on operator */

    lo_mny = (AD_MONEYNTRNL *)adc_kblk->adc_lokey.db_data;
    hi_mny = (AD_MONEYNTRNL *)adc_kblk->adc_hikey.db_data;

    switch (adc_kblk->adc_opkey)
    {
      case ADI_EQ_OP:
      case ADI_NE_OP:
	if (cents > AD_1MNY_MAX_NTRNL  ||  cents < AD_3MNY_MIN_NTRNL)
	{
	    adc_kblk->adc_tykey = ADC_KNOMATCH;
	}
	else
	{
	    if (adc_kblk->adc_opkey == ADI_EQ_OP)
	      adc_kblk->adc_tykey = ADC_KEXACTKEY;
	     else adc_kblk->adc_tykey = ADC_KNOKEY;
	    if (lo_mny != NULL)
		lo_mny->mny_cents = cents;
	}
        break;

      case ADI_GT_OP:
      case ADI_GE_OP:
	if (cents > AD_1MNY_MAX_NTRNL)
	{
	    adc_kblk->adc_tykey = ADC_KNOMATCH;
	}
	else if (cents < AD_3MNY_MIN_NTRNL)
	{
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	}
	else
	{
	    adc_kblk->adc_tykey = ADC_KLOWKEY;
	    if (lo_mny != NULL)
		lo_mny->mny_cents = cents;
	}
        break;

      case ADI_LT_OP:
      case ADI_LE_OP:
	if (cents > AD_1MNY_MAX_NTRNL)
	{
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	}
	else if (cents < AD_3MNY_MIN_NTRNL)
	{
	    adc_kblk->adc_tykey = ADC_KNOMATCH;
	}
	else
	{
	    adc_kblk->adc_tykey = ADC_KHIGHKEY;
	    if (hi_mny != NULL)
		hi_mny->mny_cents = cents;
	}
        break;

      default:
	return (adu_error(adf_scb, E_AD3002_BAD_KEYOP, 0));
    }

    return (E_DB_OK);
}


/*
** Name: key_fsm() -	Builds a string key for the LIKE or = operator, using
**			LIKE or QUEL pattern matching characters (if qlang is
**			QUEL).
** 
** Description:
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_qlang			The query language in use.  This is
**					necessary to determine whether QUEL
**					pattern matching characters should be
**					looked for.
**      semantics			One of the following:
**					    AD_C_SEMANTICS - compare as C.
**					    AD_T_SEMANTICS - comapre as TEXT.
**					    AD_VC_SEMANTICS - comapre as CHAR or
**							      VARCHAR.
**      adc_kblk
**	    .adc_kdv			The DB_DATA_VALUE to form
**					a key for.
**		.db_datatype		Datatype of data value.
**		.db_length		Length of data value.
**		.db_data		Pointer to the actual data.
**	    .adc_opkey			Key operator used to tell this
**					routine what kind of comparison
**					operation the key is to be
**					built for, as described above.
**					NOTE: ADI_OP_IDs should not be used
**					directly.  Rather the caller should
**					use the adi_opid() routine to obtain
**					the value that should be passed in here.
**          .adc_lokey			DB_DATA_VALUE to recieve the
**					low key, if built.  Note, its datatype
**					and length should be the same as
**					.adc_hikey's datatype and length.
**		.db_datatype		Datatype for low key.
**	    	.db_length		Length for low key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the low key.  If this
**					is NULL, no data will be written.
**	    .adc_hikey			DB_DATA_VALUE to recieve the
**					high key, if built.  Note, its datatype
**					and length should be the same as 
**					.adc_lokey's datatype and length.
**		.db_datatype		Datatype for high key.
**		.db_length		Length for high key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the high key.  If this
**					is NULL, no data will be written.
**	    .adc_escape			*** USED ONLY FOR THE `LIKE' familiy
**					of operators, and then only if
**					.adc_pat_flags AD_PAT_HAS_ESCAPE bit set
**					The escape char to use, if there was an
**					ESCAPE clause with the LIKE or NOT LIKE
**					predicate.
**	    .adc_pat_flags		*** USED ONLY FOR THE `LIKE' family
**					of operators ***
**					If there was an ESCAPE clause with the
**					LIKE or NOT LIKE predicate, then supply
**					.adc_pat_flags with AD_PAT_HAS_ESCAPE set,
**					and set .adc_escape to the escape char.
**					If there is not an ESCAPE clause with the
**					LIKE or NOT LIKE predicate, then this bit
**					must cleared. It is a good idea to set the
**					whole of .adc_pat_flags to 0 for other
**					operators that are not of the LIKE family.
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
**      adc_kblk  
**	    .adc_tykey			The type of key which was built,
**                                      as described above.  This will
**					be one of the following:
**						ADC_KALLMATCH
**						ADC_KNOMATCH
**						ADC_KEXACTKEY
**						ADC_KLOWKEY
**						ADC_KHIGHKEY
**						ADC_KRANGEKEY
**	    .adc_lokey
**		.db_data		If the low key was built, the
**					actual data for it will be put
**					at this location, unless .db_data
**					is NULL.
**          .adc_hikey
**		.db_data		If the high key was built, the
**					actual data for it will be put
**					at this location, unless .db_data
**					is NULL.
**	Returns:
**
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**  History:
**	20-oct-86 (thurston)
**	    Initial creation.
**	03-nov-86 (thurston)
**	    Dummied up to always return ADC_KALLMATCH for now.
**	19-nov-86 (thurston)
**	    Wrote to function the correct way.
**	18-may-87 (thurston)
**	    Fixed bug that was getting the length of the low and high keys to be
**	    built (for text and varchar) by looking at the uninitialized count
**	    field instead of the .db_length - DB_CNTSIZE.
**	02-jun-88 (thurston)
**	    Added comments to the input parameters for the new .adc_escape and
**	    .adc_isescape fields in the ADC_KEY_BLK.  No actual source change
**	    needs to happen in this file ... the low level key building routine
**	    for character strings will have to deal with it.
**	19-jun-88 (thurston)
**	    Rewrote this routine to use a finite state machine, allow the ESCAPE
**	    clause, and in conjunction with it, the QUEL like range pattern
**	    matching capability.
**	11-jul-88 (jrb)
**	    Made changes for Kanji support.
**	27-oct-88 (thurston & jrb)
**	    Fixed up `MATCH-ONE' to assure we get a RANGEKEY.
**      05-oct-89 (jrb)
**          Fixed bug #8227: embedded MATCH_ONEs (SQL '_' or QUEL '?') in C
**          datatype don't key correctly.  Fix was to generate a low key using
**          MIN_CHAR+1 to replace MATCH_ONEs instead of MIN_CHAR.
**      26-mar-91 (jrb)
**          Bug 35749.  Need to strip trailing blanks on pattern key before
**          building key values because if we're building a TEXT key (where
**          trailing blanks are significant) and we build key values with
**          trailing blanks where they should have been stripped (like for the
**          CHAR datatype) then the keys will be incorrect.
**	6-nov-1996 (inkdo01)
**	    Added code in all type-specific routines to classify "<>" (ne)
**	    relops as ADC_KNOKEY (to permit improved selectivity estimates
**	    from OPF).
**      30-nov-1998 (wanfr01)
**          Bug 94216, Problem INGSRV 602 - need to confirm that the 'extra'
**          characters are blank when comparing two varchars of different
**          length.  Comments for this check were there, but not the code
**	14-mar-1999 (hayke02)
**	    Back out the fix for bug 94582, which was causing incorrect
**	    results from queries against char and varchar key columns with
**	    equals predicate strings containing trailing blanks. Bug 94582
**	    has been fixed correctly in opc_orig_build() by the fix for
**	    bug 94859. This change fixes bug 96353.
**	 1-oct-2004 (hayke02)
**	    For AD_T_SEMANTICS, pad out the remaining loptr with NULLCHAR's
**	    ('\0') to prevent previous contents of the adc_lokey being used.
**	    This change fixes problem INGSRV 2979, bug 11313.
**    17-Mar-2006 (bolke01) Bug 115791
**        Identified a regression in change for 113123 by hayke02.
**        When using a TEXT column as an indexed column in BTREE or
**        ISAM tables/indexes and the WHERE clause contains
**        col1 LIKE 'xxx%' exact matches of 'xxx' are ommited from the
**        returned tuple set.
**	  Pad the buffer with MIN_CHAR for AD_T_SEMANTICS.
**	21-mar-06 (hayke02)
**	    Further modification of the fixes for 113123 and 115791. We now
**	    pad out the remaining loptr with lowestchar if low string datatype
**	    is char or C, regardless of the semantics type. This ensures that
**	    the previous contents of loptr are not used for these two datatypes
**	    - they are without length specifiers and so db_t_count will not be
**	    set (see good_return section at the bottom of this function).
**	31-may-2007 (gupsh01)
**	    Set valid maximum values of UT8 characters, as 0xFF is not valid
**	    in Unicode UTF8 representation.
**	02-aug-07 (toumi01)
**	    Correct if/else flaw that was causing incorrect query plan
**	    literals to be stored for key comparisons (bad opc_khi trailing
**	    character). This could cause bad query results, e.g. running
**	    a sep test rfb007 query against a btree table.
**	17-aug-2007 (gupsh01)
**	    Modified the max values for a UTF8 key.
**	14-sep-2007 (gupsh01)
**	    Reassign the max keys for the UTF8 installation.
**	    The new keys are based on the maximum weight in the
**	    collation table, rather than just the maximum code
**	    point. 
**	06-mar-2008 (gupsh01)
**	    Remove handling of key building for UTF8 installs from here.
**	    This is now handled in the routine adu_utf8cbldkey.
**	26-Jun-2008 (kiria01) SIR120473
**	    Drop out with KALLMATCH if an alternation character is seen as
**	    the alternatives will overly complicate things at present.
**	27-Oct-2008 (kiria01) SIR120473
**	    Handoff LIKE work to pattern code.
*/

static	DB_STATUS
key_fsm(
ADF_CB		*adf_scb,
i4		semantics,
ADC_KEY_BLK	*adc_kblk)
{
    DB_DATA_VALUE   *pat_dv  = &adc_kblk->adc_kdv;
    DB_DATA_VALUE   *lo_dv   = &adc_kblk->adc_lokey;
    DB_DATA_VALUE   *hi_dv   = &adc_kblk->adc_hikey;
    i4		    *tykey   = &adc_kblk->adc_tykey;
    bool	    isescape = (adc_kblk->adc_pat_flags & AD_PAT_HAS_ESCAPE)!=0;
    i4		    eschar   = adc_kblk->adc_escape;
    DB_STATUS	    db_stat;
    i4		    patlen;
    i4		    lolen;
    i4		    hilen;
    i4		    save_lohilen;
    char	    *tmpptr;	    /* Needed to call by address */
    register char   *patptr;
    register char *loptr;
    register char *hiptr;
    char	    *endpat;
    char	    *endlo;
    char	    *endhi;
    char	    patchar;
    char	    *saveptr;
    char	    lowestchar;
    char	    highestchar;
    char	    lorangechar;
    char	    *lorangeptr;
    char	    hirangechar;
    char	    *hirangeptr;
    bool	    op_is_like = (adc_kblk->adc_opkey == ADI_LIKE_OP);
    bool	    pm_found = op_is_like;  /* NEVER blank pad for LIKE */
    bool	    in_bpad_mode = FALSE;
    i4		    cc;
    i4		    cur_state = AD_S0_NOT_IN_RANGE;

    if (op_is_like)
    {
	if (abs(adc_kblk->adc_kdv.db_datatype) == DB_PAT_TYPE)
	    return adu_patcomp_kbld(adf_scb, adc_kblk);

	*tykey = ADC_KRANGEKEY;
    }
    else if (adc_kblk->adc_opkey == ADI_NE_OP) 
	*tykey = ADC_KNOKEY;	/* for "<>" op histogram support */
    else
	*tykey = ADC_KEXACTKEY;

    /* Get string length and string address of the pattern string */
    if (db_stat = adu_3straddr(adf_scb, pat_dv, (char **) &tmpptr))
	return (db_stat);
    if (db_stat = adu_size(adf_scb, pat_dv, &patlen))
	return (db_stat);
    patptr = tmpptr;	/* Needed because patptr is 'register' */
    endpat = patptr + patlen;

    /* Now get string length of low and high keys (assumed to be the same) */
    save_lohilen = lo_dv->db_length;
    if (    abs(lo_dv->db_datatype) == DB_VCH_TYPE
	||  abs(lo_dv->db_datatype) == DB_TXT_TYPE
	||  abs(lo_dv->db_datatype) == DB_LTXT_TYPE
       )
	save_lohilen -= DB_CNTSIZE;
    lolen = save_lohilen;
    hilen = save_lohilen;

    /* Now get string address of low key */
    if (lo_dv->db_data == NULL)
    {
	loptr = NULL;
    }
    else
    {
	if (db_stat = adu_3straddr(adf_scb, lo_dv, (char **) &tmpptr))
	    return (db_stat);
	loptr = tmpptr;	    /* Needed because loptr is 'register' */
    }
    endlo = loptr + save_lohilen;

    /* Now get string address of high key */
    if (hi_dv->db_data == NULL)
    {
	hiptr = NULL;
    }
    else
    {
	if (db_stat = adu_3straddr(adf_scb, hi_dv, (char **) &tmpptr))
	    return (db_stat);
	hiptr = tmpptr;	    /* Needed because hiptr is 'register' */
    }
    endhi = hiptr + save_lohilen;

    /* Set up lowest and highest characters based on semantics in use */
    switch (semantics)
    {
      case AD_C_SEMANTICS:
	lowestchar  = MIN_CHAR;
	highestchar = IIMAX_CHAR_VALUE;
	break;

      case AD_T_SEMANTICS:
	lowestchar  = (u_char) 0x01;
	highestchar = (u_char) AD_MAX_TEXT;
	break;

      case AD_VC_SEMANTICS:
	lowestchar  = (u_char) 0x00;
	highestchar = (u_char) 0xFF;
	break;

      default:
	/* should *NEVER* get here */
	return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }

    if (adc_kblk->adc_pat_flags & (AD_PAT_WO_CASE|AD_PAT_BIGNORE))
    {
	/* Text modification is implied that would
	** considerably complicate this routine
	** which would be better done using the pattern
	** keybld code if it were accessible.
	** For the moment, return ALLMATCH */
	adc_kblk->adc_tykey = ADC_KALLMATCH;
	return E_DB_OK;
    }

    for (;;)
    {
	/* Get next character in pattern string, and assign character class */
	/* ---------------------------------------------------------------- */

	if (patptr >= endpat)
	{
	    /* end of pattern string */
	    cc = AD_CC6_EOS;
	}
	else
	{
	    patchar = *patptr;
	    if (op_is_like)
	    {
		if (isescape  &&  patchar == eschar)
		{
		    /* we have an escape sequence */
		    if (patptr >= endpat)
		    {
			/* ERROR: escape at end of pattern string not allowed */
			return (adu_error(adf_scb, E_AD1017_ESC_AT_ENDSTR, 0));
		    }
		    patchar = *++patptr;    /* OK since eschar is single-byte */
		    switch (patchar)
		    {
		      case AD_1LIKE_ONE:
		      case AD_2LIKE_ANY:
		      case AD_5LIKE_RANGE:
			cc = AD_CC0_NORMAL;
			break;

		      case AD_3LIKE_LBRAC:
			cc = AD_CC4_LBRAC;
			pm_found = TRUE;
			break;

		      case AD_4LIKE_RBRAC:
			cc = AD_CC5_RBRAC;
			break;

		      default:
			if (patchar == eschar)
			    cc = AD_CC0_NORMAL;
			else
			{
			    if (patchar == AD_7LIKE_BAR)
			    {
				/* If alternation is found, the likelihood is that
				** the min-max range will be of dubious value short
				** of evaluating it for every sub-pattern. To do
				** so will considerably complicate this routine
				** which would be better done using the pattern
				** keybld code if it were accessible.
				** For the moment, return ALLMATCH */
				*tykey = ADC_KALLMATCH;
				goto good_return;
			    }
			    /* ERROR:  illegal escape sequence */
			    return (adu_error(adf_scb, E_AD1018_BAD_ESC_SEQ,0));
			}
			break;
		    }
		}
		else
		{
		    /* not an escape character */
		    switch (patchar)
		    {
		      case AD_1LIKE_ONE:
			cc = AD_CC2_ONE;
			pm_found = TRUE;
			break;

		      case AD_2LIKE_ANY:
			cc = AD_CC3_ANY;
			pm_found = TRUE;
			break;

		      case AD_5LIKE_RANGE:
			cc = AD_CC1_DASH;
			break;

		      case AD_3LIKE_LBRAC:
		      case AD_4LIKE_RBRAC:
		      default:
			cc = AD_CC0_NORMAL;
			break;
		    }
		}
	    }
	    else if (adf_scb->adf_qlang == DB_QUEL)
	    {
	        /* op is not LIKE, qlang is QUEL:  use QUEL pm characters */
		switch (patchar)
		{
		  case DB_PAT_ONE:
		    cc = AD_CC2_ONE;
		    pm_found = TRUE;
		    break;

		  case DB_PAT_ANY:
		    cc = AD_CC3_ANY;
		    pm_found = TRUE;
		    break;

		  case DB_PAT_LBRAC:
		    cc = AD_CC4_LBRAC;
		    pm_found = TRUE;
		    break;

		  case DB_PAT_RBRAC:
		    cc = AD_CC5_RBRAC;
		    break;

		  case '-':
		    cc = AD_CC1_DASH;
		    break;

		  default:
		    cc = AD_CC0_NORMAL;
		    break;
		}
	    }
	    else
	    {
	        /* op is not LIKE, qlang is not QUEL:  do *NOT* use QUEL pm */
		cc = AD_CC0_NORMAL;
	    }
	}

	if (	semantics == AD_C_SEMANTICS
	    &&	cc == AD_CC0_NORMAL
	    &&	(patchar == ' ' || patchar == NULLCHAR)
	   )
	{
	    CMnext(patptr);
	    continue;	/* ignore blanks for the C datatype */
	}

	if (in_bpad_mode  &&  cc != AD_CC6_EOS)
	{
	    if (cc == AD_CC0_NORMAL  &&  patchar == ' ')
	    {
		CMnext(patptr);
		continue;
	    }
	    else
	    {
		*tykey = ADC_KNOMATCH;
		break;
	    }
	}


	/* Now we have the character class, use it to find the action */
	/* ---------------------------------------------------------- */

	switch (ad_ts_tab[cur_state][cc].ad_action)
	{
	  case AD_ACT_Z:
	    /*
	    ** do nothing.
	    */
	    break;

	  case AD_ACT_A:
	    /*
	    ** we got a `normal' character; just move
	    ** it into the low and high value strings.
	    */
	    if (lolen <= 0  ||  hilen <= 0)
	    {
		/*
		** No more available chars in low and/or high values.  If
		** current patchar is non-blank, or the semantics are not
		** CHAR/VARCHAR, or we have already found pm chars,
		** then just return NOMATCH.  Otherwise, scan the rest of the
		** pattern for a non-blank.  If found, return NOMATCH.
		*/
		if (semantics == AD_VC_SEMANTICS  &&  !pm_found)
		{
		    in_bpad_mode = FALSE;   
		}
		else
		{
		    *tykey = ADC_KNOMATCH;
		    goto good_return;
		}
	    }
	    else
	    {
		/* Still available chars in low and high values */
		if (loptr != NULL)
		{
		    CMcpychar(patptr, loptr);
		    CMnext(loptr);
		}
		CMbytedec(lolen, patptr);
		
		if (hiptr != NULL)
		{
		    CMcpychar(patptr, hiptr);
		    CMnext(hiptr);
		}
		CMbytedec(hilen, patptr);
	    }
	    break;

	  case AD_ACT_B:
	    /*
	    ** we got a `MATCH-ONE' pattern match character; use lowest
	    ** and highest characters allowed for the output datatype.
	    */
	    if (lolen-- <= 0  ||  hilen-- <= 0)
	    {
		*tykey = ADC_KNOMATCH;
		goto good_return;
	    }
	    *tykey = ADC_KRANGEKEY;

	    if (loptr != NULL)
	    {
		if (semantics == AD_C_SEMANTICS)
		  *loptr++ = MIN_CHAR+1;
		else
		  *loptr++ = lowestchar;
	    }

	    if (hiptr != NULL)
	    {
		*hiptr++ = highestchar;
	    }
	    break;

	  case AD_ACT_C:
	    /*
	    ** beginning a range sequence, init low
	    ** and high characters for the range.
	    */
	    lorangechar = '\377';
	    lorangeptr  = &lorangechar;
	    hirangechar = '\000';
	    hirangeptr  = &hirangechar;
	    break;

	  case AD_ACT_D:
	    /*
	    ** we are in a range sequence, so the next character from the
	    ** pattern could be a dash, so just remember this one, for now.
	    */
	    saveptr = patptr;
	    break;

	  case AD_ACT_E:
	    /*
	    ** finishing a `dash' sequence inside a range sequence;
	    ** see that the character preceeding the dash was lower
	    ** than the character following the dash.
	    */
	    if (CMcmpcase(patptr, saveptr) > 0)
	    {
		if (CMcmpcase(saveptr, lorangeptr) < 0)
		    lorangeptr = saveptr;
		if (CMcmpcase(patptr, hirangeptr) > 0)
		    hirangeptr = patptr;
	    }
	    break;

	  case AD_ACT_F:
	    /*
	    ** Ending a range spec without having a saved char; see that the
	    ** lowest specified character in the range is less than (set type
	    ** of key to `RANGE-KEY') or equal to the highest specified
	    ** character in the range.
	    */
	    if (CMcmpcase(lorangeptr, hirangeptr) <= 0)
	    {
		if (lolen <= 0  ||  hilen <= 0)
		{
		    /*
		    ** Since pm chars were in pattern, we *NEVER* do blank
		    ** padding, so just return a NOMATCH.
		    */
		    *tykey = ADC_KNOMATCH;
		    goto good_return;
		}

		if (loptr != NULL)
		{
		    CMcpychar(lorangeptr, loptr);
		    CMnext(loptr);
		}
		CMbytedec(lolen, lorangeptr);
		
		if (hiptr != NULL)
		{
		    CMcpychar(hirangeptr, hiptr);
		    CMnext(hiptr);
		}
		CMbytedec(hilen, hirangeptr);
		
		if (CMcmpcase(lorangeptr, hirangeptr) < 0)
		    *tykey = ADC_KRANGEKEY;
	    }
	    /* ... else, empty range; do nothing */
	    break;

	  case AD_ACT_G:
	    /*
	    ** ERROR:  pattern match characters not allowed in range spec.
	    */
	    db_stat = adu_error(adf_scb, E_AD1016_PMCHARS_IN_RANGE, 0);
	    break;

	  case AD_ACT_H:
	    /*
	    ** ERROR:  bad range specification.
	    */
	    db_stat = adu_error(adf_scb, E_AD1015_BAD_RANGE, 0);
	    break;

	  case AD_ACT_I:
	    /*
	    ** We got a `MATCH-ANY' character; do not process the pattern string
	    ** any further.  If no output chars have been used (i.e. this is the
	    ** first relevant character in the pattern) then call it an
	    ** ALLMATCH.  Otherwise, do appropriate low-padding for low value,
	    ** and high-padding for high value, depending on semantics in use.
	    */
	    if (lolen == save_lohilen  &&  hilen == save_lohilen)
	    {
		/* no output characters used yet */
		*tykey = ADC_KALLMATCH;
		goto good_return;
	    }
	    else
	    {
		*tykey = ADC_KRANGEKEY;

		/* take care of the low value */
		if (loptr != NULL)
		{
		    /* do not pad low value strings for TEXT/VCH */
		    if (lo_dv->db_datatype != DB_VCH_TYPE
			&&
			lo_dv->db_datatype != DB_TXT_TYPE
			&&
			lo_dv->db_datatype != DB_LTXT_TYPE)
		    {
			while (lolen--)
			    *loptr++ = lowestchar;
		    }
		}

		/* take care of high value */
		if (hiptr != NULL)
		{
		    while (hilen--)
		    {
 		       *hiptr++ = highestchar;
		    }
		}
	    }
	    break;

	  case AD_ACT_J:
	    if (lolen > 0  ||  hilen > 0)
	    {
		/*
		** More chars available in low and/or high values;
		** may need to do something special before returning.
		*/
		switch (lo_dv->db_datatype)
		{
		  case DB_CHR_TYPE:
		    if (semantics != AD_C_SEMANTICS)
		    {
			/* should *NEVER* get here */
			return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
		    }
		    /* blank pad the low and high values */
		    if (loptr != NULL)
		    {
			while (lolen--)
			    *loptr++ = ' ';
		    }

		    if (hiptr != NULL)
		    {
			while (hilen--)
			    *hiptr++ = ' ';
		    }
		    break;

		  case DB_TXT_TYPE:
		    switch (semantics)
		    {
		      case AD_T_SEMANTICS:
			/* do nothing special */
			break;

		      case AD_C_SEMANTICS:
			/* return ALLMATCH */
			*tykey = ADC_KALLMATCH;
			goto good_return;

		      case AD_VC_SEMANTICS:
		      default:
			/* should *NEVER* get here */
			return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
		    }
		    break;

		  case DB_CHA_TYPE:
		    switch (semantics)
		    {
		      case AD_T_SEMANTICS:
			/* blank pad the low and high values */
			if (loptr != NULL)
			{
			    while (lolen--)
				*loptr++ = ' ';
			}

			if (hiptr != NULL)
			{
			    while (hilen--)
				*hiptr++ = ' ';
			}
			break;

		      case AD_C_SEMANTICS:
			/* return ALLMATCH */
			*tykey = ADC_KALLMATCH;
			goto good_return;

		      case AD_VC_SEMANTICS:
			/*
			** If pm chars found, then we *NEVER* do blank padding,
			** so a NOMATCH should be returned.  If no pm involved,
			** then blank pad.
			*/
			if (pm_found)
			{
			    /* return NOMATCH */
			    *tykey = ADC_KNOMATCH;
			    goto good_return;
			}
			else
			{
			    /* blank pad the low and high values */
			    if (loptr != NULL)
			    {
				while (lolen--)
				    *loptr++ = ' ';
			    }

			    if (hiptr != NULL)
			    {
				while (hilen--)
				    *hiptr++ = ' ';
			    }
			}
			break;

		      default:
			/* should *NEVER* get here */
			return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
		    }
		    break;

		  case DB_VCH_TYPE:
		    switch (semantics)
		    {
		      case AD_T_SEMANTICS:
			/* do nothing special */
			break;

		      case AD_C_SEMANTICS:
			/* return ALLMATCH */
			*tykey = ADC_KALLMATCH;
			goto good_return;

		      case AD_VC_SEMANTICS:
			/*
			** If pm chars found, then we *NEVER* do blank padding,
			** so a NOMATCH should be returned if the pattern type
			** was CHAR (fixed length).  If no pm involved, or
			** pattern type was VARCHAR, then no need to do anything
			** special.
			*/
			if (pat_dv->db_datatype == DB_CHA_TYPE  &&  pm_found)
			{
			    /* return NOMATCH */
			    *tykey = ADC_KNOMATCH;
			    goto good_return;
			}
			/* else, do nothing special */
			break;

		      default:
			/* should *NEVER* get here */
			return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
		    }
		    break;

		  default:
		    /* should *NEVER* get here */
		    return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
		}
	    }
	    break;

	  case AD_ACT_K:
	    /*
	    ** Previous character was not the start of a dash sequence, so
	    ** handle the previous `saved char', and then save the current char.
	    */
	    if (CMcmpcase(saveptr, lorangeptr) < 0)
		lorangeptr = saveptr;
	    if (CMcmpcase(saveptr, hirangeptr) > 0)
		hirangeptr = saveptr;
	    saveptr = patptr;
	    break;

	  case AD_ACT_L:
	    /*
	    ** Ending a range spec with a saved char; first process the saved
	    ** char like action `K', then do what action `F' does:  See that
	    ** the lowest specified character in the range is less than (set
	    ** type of key to `RANGE-KEY') or equal to the highest specified
	    ** character in the range.
	    */
	    if (CMcmpcase(saveptr, lorangeptr) < 0)
		lorangeptr = saveptr;
	    if (CMcmpcase(saveptr, hirangeptr) > 0)
		hirangeptr = saveptr;

	    if (CMcmpcase(lorangeptr, hirangeptr) <= 0)
	    {
		if (lolen <= 0  ||  hilen <= 0)
		{
		    /*
		    ** Since pm chars were in pattern, we *NEVER* do blank
		    ** padding, so just return a NOMATCH.
		    */
		    *tykey = ADC_KNOMATCH;
		    goto good_return;
		}
		if (loptr != NULL)
		{
		    CMcpychar(lorangeptr, loptr);
		    CMnext(loptr);
		}
		CMbytedec(lolen, lorangeptr);
		
		if (hiptr != NULL)
		{
		    CMcpychar(hirangeptr, hiptr);
		    CMnext(hiptr);
		}
		CMbytedec(hilen, hirangeptr);
		
		if (CMcmpcase(lorangeptr, hirangeptr) < 0)
		    *tykey = ADC_KRANGEKEY;
	    }
	    /* ... else, empty range; do nothing */
	    break;

	  default:
	    /* should *NEVER* get here */
	    return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	}

	CMnext(patptr);

	/* Now find the next state, and make it the current state */
	/* ------------------------------------------------------ */
	
	cur_state = (ad_ts_tab[cur_state][cc]).ad_next_state;

	if (cur_state == AD_SX_EXIT)
	    break;
    }


good_return:
    if (DB_SUCCESS_MACRO(db_stat))
    {
	if (	lo_dv->db_datatype == DB_VCH_TYPE
	    ||	lo_dv->db_datatype == DB_TXT_TYPE
	    ||	lo_dv->db_datatype == DB_LTXT_TYPE
	   )
	{
	    if (loptr != NULL)
		((DB_TEXT_STRING *) lo_dv->db_data)->db_t_count =
				loptr - (char *)lo_dv->db_data - DB_CNTSIZE;
	    if (hiptr != NULL)
		((DB_TEXT_STRING *) hi_dv->db_data)->db_t_count =
				hiptr - (char *)hi_dv->db_data - DB_CNTSIZE;
	}
    }

    return (db_stat);
}


/*
** Name: key_ltgt() -	Builds a string key for the <, <=, >, >= operators,
**			using QUEL pattern matching characters if qlang is QUEL.
** 
** Description:
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_qlang			The query language in use.  This is
**					necessary to determine whether QUEL
**					pattern matching characters should be
**					looked for.
**      semantics			One of the following:
**					    AD_C_SEMANTICS - compare as C.
**					    AD_T_SEMANTICS - comapre as TEXT.
**					    AD_VC_SEMANTICS - comapre as CHAR or
**							      VARCHAR.
**      adc_kblk
**	    .adc_kdv			The DB_DATA_VALUE to form
**					a key for.
**		.db_datatype		Datatype of data value.
**		.db_length		Length of data value.
**		.db_data		Pointer to the actual data.
**	    .adc_opkey			Key operator used to tell this
**					routine what kind of comparison
**					operation the key is to be
**					built for, as described above.
**					NOTE: ADI_OP_IDs should not be used
**					directly.  Rather the caller should
**					use the adi_opid() routine to obtain
**					the value that should be passed in here.
**          .adc_lokey			DB_DATA_VALUE to recieve the
**					low key, if built.  Note, its datatype
**					and length should be the same as
**					.adc_hikey's datatype and length.
**		.db_datatype		Datatype for low key.
**	    	.db_length		Length for low key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the low key.  If this
**					is NULL, no data will be written.
**	    .adc_hikey			DB_DATA_VALUE to recieve the
**					high key, if built.  Note, its datatype
**					and length should be the same as 
**					.adc_lokey's datatype and length.
**		.db_datatype		Datatype for high key.
**		.db_length		Length for high key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the high key.  If this
**					is NULL, no data will be written.
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
**      adc_kblk  
**	    .adc_tykey			The type of key which was built,
**                                      as described above.  This will
**					be one of the following:
**						ADC_KALLMATCH
**						ADC_KNOMATCH
**						ADC_KEXACTKEY
**						ADC_KLOWKEY
**						ADC_KHIGHKEY
**						ADC_KRANGEKEY
**	    .adc_lokey
**		.db_data		If the low key was built, the
**					actual data for it will be put
**					at this location, unless .db_data
**					is NULL.
**          .adc_hikey
**		.db_data		If the high key was built, the
**					actual data for it will be put
**					at this location, unless .db_data
**					is NULL.
**	Returns:
**
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**  History:
**	11-jul-88 (thurston)
**	    Initial creation and coding.
**	11-jul-88 (jrb)
**	    Changes for Kanji support.
**      26-mar-91 (jrb)
**          Bug 35749.  Need to strip trailing blanks on pattern key before
**          building key values because if we're building a TEXT key (where
**          trailing blanks are significant) and we build key values with
**          trailing blanks where they should have been stripped (like for the
**          CHAR datatype) then the keys will be incorrect.
*/

static	DB_STATUS
key_ltgt(
ADF_CB		*adf_scb,
i4		semantics,
ADC_KEY_BLK	*adc_kblk)
{
    DB_DATA_VALUE   *pat_dv  = &adc_kblk->adc_kdv;
    DB_DATA_VALUE   *col_dv;
    i4		    *tykey   = &adc_kblk->adc_tykey;
    DB_STATUS	    db_stat;
    i4		    patlen;
    i4		    collen;
    u_char	    *tmpptr;	    /* Needed to call by address */
    register u_char *patptr;
    register u_char *colptr;
    u_char	    *endpat;
    u_char	    *endcol;
    u_char	    patchar;
    bool	    bpad;
    ADI_OP_ID	    opid;


    switch (opid = adc_kblk->adc_opkey)
    {
      case ADI_LT_OP:
      case ADI_LE_OP:
	*tykey = ADC_KHIGHKEY;
	col_dv = &adc_kblk->adc_hikey;
	opid = ADI_LT_OP;	/* for simplicity, just use `less than' */
	break;

      case ADI_GT_OP:
      case ADI_GE_OP:
	*tykey = ADC_KLOWKEY;
	col_dv = &adc_kblk->adc_lokey;
	opid = ADI_GT_OP;	/* for simplicity, just use `greater than' */
	break;

      default:
	/* should *NEVER* get here */
	return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }
    
    /* Get string length and string address of the pattern string */
    if (db_stat = adu_3straddr(adf_scb, pat_dv, (char **) &tmpptr))
	return (db_stat);
    if (db_stat = adu_size(adf_scb, pat_dv, &patlen))
	return (db_stat);
    patptr = tmpptr;	/* Needed because patptr is 'register' */
    endpat = patptr + patlen;

    /* Now get string length of the column key (i.e. output value) */
    collen = col_dv->db_length;
    if (    abs(col_dv->db_datatype) == DB_VCH_TYPE
	||  abs(col_dv->db_datatype) == DB_TXT_TYPE
	||  abs(col_dv->db_datatype) == DB_LTXT_TYPE
       )
	collen -= DB_CNTSIZE;

    /* Now get string address of the column key */
    if (col_dv->db_data == NULL)
    {
	colptr = NULL;
    }
    else
    {
	if (db_stat = adu_3straddr(adf_scb, col_dv, (char **) &tmpptr))
	    return (db_stat);
	colptr = tmpptr;    /* Needed because colptr is 'register' */
    }
    endcol = colptr + collen;


    /* Do we need to blank pad the column key if shorter than pattern? */
    if (    col_dv->db_datatype == DB_CHR_TYPE
	||  col_dv->db_datatype == DB_CHA_TYPE
	||  semantics == AD_VC_SEMANTICS
       )
	bpad = TRUE;
    else
	bpad = FALSE;


    while (patptr < endpat)
    {
	switch (patchar = *patptr)
	{
	  case DB_PAT_ANY:
	  case DB_PAT_ONE:
	  case DB_PAT_LBRAC:
	    if (adf_scb->adf_qlang == DB_QUEL)
	    {
		*tykey = ADC_KALLMATCH;
		return (E_DB_OK);
	    }
	    break;

	  case DB_PAT_RBRAC:
	    if (adf_scb->adf_qlang == DB_QUEL)
	    {
		return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));
	    }
	    break;

	  default:
	    /* do nothing special */
	    break;
	}

	/* Just place the character in the column key */
	if (colptr < endcol)
	{
	    if (colptr != NULL)
	    {
		CMcpychar(patptr, colptr);
		CMnext(colptr);
	    }
	}
	CMnext(patptr);
    }


    if (colptr != NULL)
    {
	/*
	** Done with pattern.  If blank padding, see if column has more
	** available chars.
	*/
	if (bpad)
	{
	    while (colptr < endcol)
		*colptr++ = (u_char) ' ';
	}

	if (	col_dv->db_datatype == DB_VCH_TYPE
	    ||	col_dv->db_datatype == DB_TXT_TYPE
	    ||	col_dv->db_datatype == DB_LTXT_TYPE
	   )
	{
	    if (colptr != NULL)
		((DB_TEXT_STRING *) col_dv->db_data)->db_t_count =
			    colptr - (u_char *)col_dv->db_data - DB_CNTSIZE;
	}
    }
    
    return (E_DB_OK);
}

/*
** Name: adu_bbldkey() - Build a "byte" key.
**
**	adu_bbldkey() builds a key value of the type and length required.
**	Currently only used for logical keys, in the future may be used for
**	datatypes that are "byte" oriented.
**
**	This code now supports keybuilding from any of the character types,
**	and the logical key types to either of the logical key datatypes.
**
**	The code here mimics the key building code done in the case of integer
**	key building.  This means that it includes some "knowledge" about 
**	type resolution of logical keys.  It will accept all "character"
**	datatypes and, using adf internal knowledge, "convert" these datatypes
**	into logical key datatypes (without calling the normal conversion
**	routines).
**
**	The routine will return ADC_KNOMATCH for any datatype lengths which 
**      don't exactly match the logical key length that they are being compared
**	against.  This means, for instance, that given a char(6) or a table_key
**	datatype to be built into an object_key "key", the routine will return
**	ADC_KNOMATCH for both since the lengths and therefore the datatypes
**	are not compatible.
**	
**	If any other datatype is input to this routine then ADC_KALLMATCH
**	will be returned which will force the optimizer to execute a scan
**	(thus no key need be built).  I'm not sure if this routine will ever
**	be called with any other datatype, but this functionality will work
**	if it is.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_kblk  
**	    .adc_kdv			The DB_DATA_VALUE to form
**					a key for.
**		.db_datatype		Datatype of data value.
**		.db_length		Length of data value.
**		.db_data		Ptr to the actual data.
**	    .adc_opkey			Key operator used to tell this
**					routine what kind of comparison
**					operation the key is to be
**					built for, as described above.
**					NOTE: ADI_OP_IDs should not be used
**					directly.  Rather the caller should
**					use the adi_opid() routine to obtain
**					the value that should be passed in here.
**          .adc_lokey			DB_DATA_VALUE to recieve the
**					low key, if built.  Note, its datatype
**					and length should be the same as
**					.adc_hikey's datatype and length.
**		.db_datatype		Datatype for low key.
**	    	.db_length		Length for low key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the low key.
**	    .adc_hikey			DB_DATA_VALUE to recieve the
**					high key, if built.  Note, its datatype
**					and length should be the same as 
**					.adc_lokey's datatype and length.
**		.db_datatype		Datatype for high key.
**		.db_length		Length for high key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the high key.
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
**      *adc_kblk  
**	    .adc_tykey			The type of key which was built,
**                                      as described above.  This will
**					be one of the following:
**						ADC_KALLMATCH
**						ADC_KNOMATCH
**						ADC_KEXACTKEY
**						ADC_KLOWKEY
**						ADC_KHIGHKEY
**						ADC_KRANGEKEY
**	    .adc_lokey
**		*.db_data		If the low key was built, the
**					actual data for it will be put
**					at this location, unless .db_data
**					is NULL.
**          .adc_hikey
**		*.db_data		If the high key was built, the
**					actual data for it will be put
**					at this location, unless .db_data
**					is NULL.
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
**	    E_AD0000_OK			Completed successfully.
**	    E_AD9999_INTERNAL_ERROR	Input data value was not valid.
**
**  History:
**	03-mar-89 (mikem)
**	    created.
**	09-apr-90 (mikem)
**	    bug #20966
**	    Add code to handle the case of non-logical key values being 
**	    comparedto logical key values in the where clause:
**	    	(ie. where table_key = char(8))
**	    Previous code would return an adf error which would cause a 
**	    "severe" error in query processing - the 2nd of which would 
**	    crash the server.
**	    
*/

DB_STATUS
adu_bbldkey(
ADF_CB              *adf_scb,
ADC_KEY_BLK	    *adc_kblk)
{
    PTR				dptr;
    PTR				lo;
    PTR				high;
    i4				src_dt, dst_dt;
    i4				src_len, dst_len;
    bool			compatible_dt;
    
    src_dt  = adc_kblk->adc_kdv.db_datatype;
    src_len = adc_kblk->adc_kdv.db_length;
    dst_dt    = adc_kblk->adc_lokey.db_datatype;
    dst_len   = adc_kblk->adc_lokey.db_length;
    compatible_dt = TRUE;
    
    /* based on datatype and length, set the data pointer to point to the
    ** "real" data part of the source key (ie. db_data in most cases, and
    ** db_data + 2 in the length encoded character datatypes).  
    */
    
    switch (src_dt)
    {
    	case DB_LOGKEY_TYPE:
    	case DB_TABKEY_TYPE:
    	case DB_CHA_TYPE:
    	case DB_CHR_TYPE:
    
    	    if (((dst_dt == DB_LOGKEY_TYPE) && (src_len == DB_LEN_OBJ_LOGKEY))
    		||
    	        ((dst_dt == DB_TABKEY_TYPE) && (src_len == DB_LEN_TAB_LOGKEY))
    	       )
    	    {
    		dptr = (PTR) adc_kblk->adc_kdv.db_data;
    	    }
    	    else
    	    {
    		compatible_dt = FALSE;
    	    }
    	    break;
    
    	case DB_VCH_TYPE:
    	case DB_TXT_TYPE:
    	case DB_LTXT_TYPE:
    
    	    /* Since we are ADF here, do some magic below the sheets.  We could
    	    ** go through adf here to convert these source keys into the 
    	    ** datatype in question, but this is harder then it looks.  We 
    	    ** can't just convert straight to the logical key datatypes as
    	    ** some of those conversions don't exist.  The datatype resolution 
    	    ** algorithm handles this by automatically converting any datatype
    	    ** (except for LOGKEY,TABKEY, and CHA) to CHA and then converting
    	    ** the LOGKEY or TABKEY to CHA to do the comparison.  Here we
    	    ** will take advantage of "knowing" the layout of the datatypes
    	    ** and copying the data straight from the base dt's rather than
    	    ** going through the conversion.
    	    */
    	    src_len = src_len - DB_CNTSIZE;
    	    if (((dst_dt == DB_LOGKEY_TYPE) && (src_len == DB_LEN_OBJ_LOGKEY))
    		||
    	        ((dst_dt == DB_TABKEY_TYPE) && (src_len == DB_LEN_TAB_LOGKEY))
    	       )
    	    {
    		dptr = (PTR)
    		   (((DB_TEXT_STRING *) adc_kblk->adc_kdv.db_data)->db_t_text);
    	    }
    	    else
    	    {
    		compatible_dt = FALSE;
    	    }
    	    break;
    
    	default:
    	    /* Force a scan */
    	    adc_kblk->adc_tykey = ADC_KALLMATCH;
    	    return(E_DB_OK);
    }
    
    if (adc_kblk->adc_kdv.db_data == NULL)
    {
	/* Parameter marker or function obscuring data.
	** Use pre-loaded vale in adc_tykey - without a key
	** we cannot improve estimate.
	** All other datatypes are handled by the coercions
	** below and there are currently none to handle
	** differently */
	if (!compatible_dt)
    	    adc_kblk->adc_tykey = ADC_KNOMATCH;

	return E_DB_OK;
    }

    if (!compatible_dt)
    {
    	/* since comparisons between logical keys and character datatype of
    	** differing lengths is "undefined", always return nomatch in this
    	** case.
	*/
    	adc_kblk->adc_tykey = ADC_KNOMATCH;
    }
    else
    {
    	/* Now set result keys based on operator */
    
    	lo = (PTR) adc_kblk->adc_lokey.db_data;
    	high = (PTR) adc_kblk->adc_hikey.db_data;
    
    	switch (adc_kblk->adc_opkey)
	{
    	  case ADI_EQ_OP:
    	  case ADI_NE_OP:
    
	    if (adc_kblk->adc_opkey == ADI_EQ_OP)
	      adc_kblk->adc_tykey = ADC_KEXACTKEY;
	     else adc_kblk->adc_tykey = ADC_KNOKEY;
    	    if (lo != NULL)
    		MEcopy(dptr, adc_kblk->adc_lokey.db_length, lo);
    	    break;
    
    	  case ADI_GT_OP:
    	  case ADI_GE_OP:
    	    adc_kblk->adc_tykey = ADC_KLOWKEY;
    	    if (lo != NULL)
    		MEcopy(dptr, adc_kblk->adc_lokey.db_length, lo);
    	    break;
    
    	  case ADI_LE_OP:
    	  case ADI_LT_OP:
    	    adc_kblk->adc_tykey = ADC_KHIGHKEY;
    	    if (high != NULL)
    		MEcopy(dptr, adc_kblk->adc_hikey.db_length, high);
    	    break;
    
    	  default:
    	    return (adu_error(adf_scb, E_AD3002_BAD_KEYOP, 0));
	}
    }

    return (E_DB_OK);
}


/*
** Name: adu_decbldkey() -  Build a "decimal" key.
**
** Description:
**    adu_decbldkey() tries to build a key value of the type/prec/length
**  required.  It assumes that adc_kblk->adc_lokey and adc_kblk->adc_hikey
**  describe the type/prec/length of the key desired.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_kblk  
**	    .adc_kdv			The DB_DATA_VALUE to form
**					a key for.
**		.db_datatype		Datatype of data value.
**		.db_prec		Precision/Scale of data value.
**		.db_length		Length of data value.
**		.db_data		Pointer to the actual data.
**	    .adc_opkey			Key operator used to tell this
**					routine what kind of comparison
**					operation the key is to be
**					built for, as described above.
**					NOTE: ADI_OP_IDs should not be used
**					directly.  Rather the caller should
**					use the adi_opid() routine to obtain
**					the value that should be passed in here.
**          .adc_lokey			DB_DATA_VALUE to recieve the
**					low key, if built.  Note, its datatype
**					and length should be the same as
**					.adc_hikey's datatype and length.
**		.db_datatype		Datatype for low key.
**		.db_prec		Precision/Scale of low key.
**	    	.db_length		Length for low key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the low key.  If this
**					is NULL, no data will be written.
**	    .adc_hikey			DB_DATA_VALUE to recieve the
**					high key, if built.  Note, its datatype
**					and length should be the same as 
**					.adc_lokey's datatype and length.
**		.db_datatype		Datatype for high key.
**		.db_prec		Precision/Scale of high key.
**		.db_length		Length for high key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the high key.  If this
**					is NULL, no data will be written.
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
**      adc_kblk  
**	    .adc_tykey			The type of key which was built,
**                                      as described above.  This will
**					be one of the following:
**						ADC_KALLMATCH
**						ADC_KNOMATCH
**						ADC_KEXACTKEY
**						ADC_KLOWKEY
**						ADC_KHIGHKEY
**						ADC_KRANGEKEY
**	    .adc_lokey
**		.db_data		If the low key was built, the
**					actual data for it will be put
**					at this location, unless .db_data
**					is NULL.
**          .adc_hikey
**		.db_data		If the high key was built, the
**					actual data for it will be put
**					at this location, unless .db_data
**					is NULL.
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
**	    E_AD0000_OK				Completed successfully.
**
**  History:
**	06-jul-89 (jrb)
**	    Created.
**      01-apr-1994 (stevet)
**          Removed the used of un-initialize f8_tmp for MONEY DT.
**	14-May-2004 (schka24)
**	    Add i8 to inline conversion.
*/

DB_STATUS
adu_decbldkey(
ADF_CB              *adf_scb,
ADC_KEY_BLK	    *adc_kblk)
{
    DB_STATUS		db_stat;
    DB_DATA_VALUE	*kdv = &adc_kblk->adc_kdv;
    DB_DATA_VALUE	*ddv;
    i8			i8_tmp;
    f8			f8_tmp;
    bool		undermin = FALSE;
    bool		overmax = FALSE;
    STATUS		status;
    AD_MONEYNTRNL	m;
    AD_MONEYNTRNL	*mptr = (AD_MONEYNTRNL *)kdv->db_data;
    DB_DATA_VALUE	mdv;


    if (adc_kblk->adc_kdv.db_data == NULL)
    {
	/* Parameter marker or function obscuring data.
	** Use pre-loaded vale in adc_tykey - without a key
	** we cannot improve estimate.
	** All unhandled datatypes will need a full scan */
	switch (kdv->db_datatype)
	{
	case DB_INT_TYPE:
	case DB_DEC_TYPE:
	case DB_FLT_TYPE:
	case DB_CHR_TYPE:
	case DB_CHA_TYPE:
	case DB_TXT_TYPE:
	case DB_VCH_TYPE:
	case DB_NVCHR_TYPE:
	case DB_NCHR_TYPE:
	case DB_MNY_TYPE:
	    break;
	default:
	    /* Force a scan */
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	}
	return E_DB_OK;
    }
    /* first we set the key type to what it probably will end up being as long
    ** as no overflows occur
    */
    switch (adc_kblk->adc_opkey)
    {
      case ADI_EQ_OP:
      case ADI_NE_OP:
	ddv = &adc_kblk->adc_lokey;
	break;

      case ADI_GT_OP:
      case ADI_GE_OP:
	ddv = &adc_kblk->adc_lokey;
	break;

      case ADI_LT_OP:
      case ADI_LE_OP:
	ddv = &adc_kblk->adc_hikey;
	break;

      default:
	return (adu_error(adf_scb, E_AD3002_BAD_KEYOP, 0));
	break;
    }

    /* Convert pattern value to decimal */
    switch (kdv->db_datatype)
    {
      case DB_INT_TYPE:
	switch (kdv->db_length)
	{
	  case 8:
	    i8_tmp = *(i8 *)kdv->db_data;
	    break;
	  case 4:
	    i8_tmp = *(i4 *)kdv->db_data;
	    break;
	  case 2:
	    i8_tmp = *(i2 *)kdv->db_data;
	    break;
	  case 1:
	    i8_tmp = I1_CHECK_MACRO(*(i1 *)kdv->db_data);
	    break;
	}

	if (ddv->db_data != NULL)
	{
	    if(CVl8pk(i8_tmp,
		    (i4)DB_P_DECODE_MACRO(ddv->db_prec),
		    (i4)DB_S_DECODE_MACRO(ddv->db_prec),
		    (PTR)ddv->db_data)
		== CV_OVERFLOW)
	    {
		if (i8_tmp < 0)
		    undermin = TRUE;
		else
		    overmax = TRUE;
	    }
	}
	break;
	
      case DB_DEC_TYPE:
	if (ddv->db_data != NULL)
	{
	    status = CVpkpk((PTR)kdv->db_data,
				(i4)DB_P_DECODE_MACRO(kdv->db_prec),
				(i4)DB_S_DECODE_MACRO(kdv->db_prec),
				(i4)DB_P_DECODE_MACRO(ddv->db_prec),
				(i4)DB_S_DECODE_MACRO(ddv->db_prec),
				(PTR)ddv->db_data);

	    /* test for overflow; note: we don't have to check for truncation
	    ** because truncating never causes us to lose values that could
	    ** possibly match a value in the column we keying on
	    */
	    if (status == CV_OVERFLOW)
	    {
		u_char		sign;

		sign = (((u_char *)kdv->db_data)[kdv->db_length-1] & 0xf);

		if (sign == MH_PK_MINUS  ||  sign == MH_PK_AMINUS)
		    undermin = TRUE;
		else
		    overmax = TRUE;
	    }
	}
	break;
	
      case DB_FLT_TYPE:
	if (kdv->db_length == 8)
	    f8_tmp = *(f8 *)kdv->db_data;
	else
	    f8_tmp = (f8)(*(f4 *)kdv->db_data);

	if (ddv->db_data != NULL)
	{
	    if (CVfpk(f8_tmp,
		    (i4)DB_P_DECODE_MACRO(ddv->db_prec),
		    (i4)DB_S_DECODE_MACRO(ddv->db_prec),
		    (PTR)ddv->db_data)
		== CV_OVERFLOW)
	    {
		if (f8_tmp < 0.0)
		    undermin = TRUE;
		else
		    overmax = TRUE;
	    }
	}
	break;

      case DB_CHR_TYPE:
      case DB_CHA_TYPE:
      case DB_TXT_TYPE:
      case DB_VCH_TYPE:
      case DB_NVCHR_TYPE:
      case DB_NCHR_TYPE:
	/* Must convert to money, and then use */
	mdv.db_datatype = DB_MNY_TYPE;
	mdv.db_length = ADF_MNY_LEN;
	mdv.db_data = (PTR) &m;
	db_stat = adc_cvinto(adf_scb, kdv, &mdv);
	if (DB_SUCCESS_MACRO(db_stat))
	{
	    mptr = (AD_MONEYNTRNL *) mdv.db_data;
	}
	else
	{
	    adc_kblk->adc_tykey = ADC_KNOMATCH;
	    return (E_DB_OK);
	}
	/* Purposely no `break;' stmt here */

      case DB_MNY_TYPE:
	if (ddv->db_data != NULL)
	{
	    if (CVfpk((f8)(mptr->mny_cents / 100),
		    (i4)DB_P_DECODE_MACRO(ddv->db_prec),
		    (i4)DB_S_DECODE_MACRO(ddv->db_prec),
		    (PTR)ddv->db_data)
		== CV_OVERFLOW)
	    {
		if ((f8)(mptr->mny_cents) < 0.0)
		    undermin = TRUE;
		else
		    overmax = TRUE;
	    }
	}
	break;

      default:
	/* Force a scan */
	adc_kblk->adc_tykey = ADC_KALLMATCH;
	return (E_DB_OK);
    }

    /* look for any abnormal conditions which will change the key type */
    if (undermin  ||  overmax)
    {
	switch (adc_kblk->adc_opkey)
	{
	  case ADI_EQ_OP:
	  case ADI_NE_OP:
	    adc_kblk->adc_tykey = ADC_KNOMATCH;
	    break;
	  case ADI_GT_OP:
	  case ADI_GE_OP:
	    if (overmax)
	    {
		adc_kblk->adc_tykey = ADC_KNOMATCH;
	    }
	    else if (undermin)
	    {
		adc_kblk->adc_tykey = ADC_KALLMATCH;
	    }
	    break;
	  case ADI_LT_OP:
	  case ADI_LE_OP:
	    if (overmax)
	    {
		adc_kblk->adc_tykey = ADC_KALLMATCH;
	    }
	    else if (undermin)
	    {
		adc_kblk->adc_tykey = ADC_KNOMATCH;
	    }
	    break;
	  default:
	    return (adu_error(adf_scb, E_AD3002_BAD_KEYOP, 0));
	}
    }

    return (E_DB_OK);
}


/*
** Name: adu_bitbldkey() - Build a "bit" key.
**
**	adu_bitbldkey() builds a key value of the type and length required.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_kblk  
**	    .adc_kdv			The DB_DATA_VALUE to form
**					a key for.
**		.db_datatype		Datatype of data value.
**		.db_length		Length of data value.
**		.db_data		Ptr to the actual data.
**	    .adc_opkey			Key operator used to tell this
**					routine what kind of comparison
**					operation the key is to be
**					built for, as described above.
**					NOTE: ADI_OP_IDs should not be used
**					directly.  Rather the caller should
**					use the adi_opid() routine to obtain
**					the value that should be passed in here.
**          .adc_lokey			DB_DATA_VALUE to recieve the
**					low key, if built.  Note, its datatype
**					and length should be the same as
**					.adc_hikey's datatype and length.
**		.db_datatype		Datatype for low key.
**	    	.db_length		Length for low key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the low key.
**	    .adc_hikey			DB_DATA_VALUE to recieve the
**					high key, if built.  Note, its datatype
**					and length should be the same as 
**					.adc_lokey's datatype and length.
**		.db_datatype		Datatype for high key.
**		.db_length		Length for high key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the high key.
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
**      *adc_kblk  
**	    .adc_tykey			The type of key which was built,
**                                      as described above.  This will
**					be one of the following:
**						ADC_KALLMATCH
**						ADC_KNOMATCH
**						ADC_KEXACTKEY
**						ADC_KLOWKEY
**						ADC_KHIGHKEY
**						ADC_KRANGEKEY
**	    .adc_lokey
**		*.db_data		If the low key was built, the
**					actual data for it will be put
**					at this location, unless .db_data
**					is NULL.
**          .adc_hikey
**		*.db_data		If the high key was built, the
**					actual data for it will be put
**					at this location, unless .db_data
**					is NULL.
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
**	    E_AD0000_OK			Completed successfully.
**	    E_AD9999_INTERNAL_ERROR	Input data value was not valid.
**
**  History:
**	03-oct-1992 (fred)
**          Created.
*/

DB_STATUS
adu_bitbldkey(
ADF_CB              *adf_scb,
ADC_KEY_BLK	    *adc_kblk)
{
    DB_STATUS		db_stat;
    DB_DATA_VALUE	*key_dv;
    i4		err;


    if (adc_kblk->adc_kdv.db_data == NULL)
    {
	/* Parameter marker or function obscuring data.
	** Use pre-loaded vale in adc_tykey - without a key
	** we cannot improve estimate.
	** All other datatypes are handled by the coercions
	** below and there are currently none to handle
	** differently */
	return E_DB_OK;
    }
    switch (adc_kblk->adc_opkey)
    {
      case ADI_EQ_OP:
      case ADI_NE_OP:
	key_dv = &adc_kblk->adc_lokey;
	break;

      case ADI_GT_OP:
      case ADI_GE_OP:
	key_dv = &adc_kblk->adc_lokey;
	break;

      case ADI_LE_OP:
      case ADI_LT_OP:
	key_dv = &adc_kblk->adc_hikey;
	break;

      default:
	return (adu_error(adf_scb, E_AD3002_BAD_KEYOP, 0));
    }

    /*
    ** What is the input datatype the same as the output?
    ** If not, try to convert.
    **
    ** If conversion not possible, then
    **    if there are no possible comparison semantics (no conversion)
    **		return an error;
    **	  Otherwise, return no matches.
    */

    if ((adc_kblk->adc_kdv.db_data == NULL) || (key_dv->db_data == NULL))
    {
	/* If the request was just for key type, then just return */
	adf_scb->adf_errcb.ad_errcode = E_DB_OK;
	return(E_DB_OK);
    }
    if (    (adc_kblk->adc_kdv.db_datatype != adc_kblk->adc_lokey.db_datatype)
	||  (adc_kblk->adc_kdv.db_length != adc_kblk->adc_lokey.db_length)
	||  (adc_kblk->adc_kdv.db_prec != adc_kblk->adc_lokey.db_prec))
    {
	db_stat = adc_cvinto(adf_scb,
		    &adc_kblk->adc_kdv,
		    key_dv);
	err = adf_scb->adf_errcb.ad_errcode;
	if (DB_FAILURE_MACRO(db_stat))
	{
	    if (err == E_AD2009_NOCOERCION)
	    {
        	return (E_DB_ERROR);
	    }
	    else
	    {
		adc_kblk->adc_tykey = ADC_KNOMATCH;
		return (E_DB_OK);
	    }
	}
    }
    else
    {
	MEcopy((PTR) adc_kblk->adc_kdv.db_data,
		    adc_kblk->adc_kdv.db_length,
		    (PTR) key_dv->db_data);
	adf_scb->adf_errcb.ad_errcode = E_DB_OK;
    }
    return (E_DB_OK);
}
