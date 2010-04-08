/*
**  Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adfops.h>
#include    <adumoney.h>
#include    <ulf.h>
#include    <adfint.h>

/**
**
**  Name: ADCKEYBLD.C - Build a HASH, ISAM or BTREE key for a data value.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the  external ADF call "adc_keybld()" which
**      builds a HASH, ISAM or BTREE key from the supplied data value.
**
**      This file defines the following externally visible routine:
**
**          adc_keybld() - Build a HASH, ISAM or BTREE key for a data value.
**
**
**  History:
**      25-feb-86 (thurston)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	14-nov-86 (thurston)
**	    Made adc_keybld() work for the LIKE operator.
**	02-dec-86 (thurston)
**	    Added the ad0_keycvt() static routine to avoid having to create
**	    a large buffer on stack for every call to adc_keybld().  Now this
**	    buffer will only be created if actually needed.
**	13-jan-87 (thurston)
**	    Added special case code to adc_keybld() to handle the .db_data PTR
**	    in the input key value being NULL.  This can occurr for user
**	    parameters in repeat queries, and this routine should just fill in
**	    the type of key to be built to what is "probably" going to be built.
**	19-feb-87 (thurston)
**	    Reversed all STRUCT_ASSIGN_MACRO's to conform to CL spec.
**	22-feb-87 (thurston)
**	    Added support for nullable datatypes.  Also, fixed bug in
**	    ad0_keycvt() that could have caused big problem:  coercion was
**	    being placed in the temp's low-key field, not the input value field.
**	    Therefore, when the low level keybld routine was being called, it
**	    would assume that the types were the same, but they wouldn't be.
**	    In effect, this bug was making it appear that the coercion was
**	    never taking place.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	26-mar-87 (thurston)
**	    Added code to prevent the lo and hi keys' data are from being
**	    written if their .db_data ptr is NULL.
**	19-apr-87 (thurston)
**	    Added a FIX_ME jury rig to allow OPF to call adc_keybld() with
**	    the `not like' and `!=' operators; they will always return
**	    ADC_KALLMATCH.
**	18-jun-87 (thurston)
**	    Altered adc_keybld() so that the low and high key values are ALWAYS
**	    set, regardless of the type of key built, unless overridden by a
**	    NULL .db_data pointer.
**	20-jul-87 (thurston)
**	    No longer call ad0_keycvt() if datatypes are different, but are both
**	    string types.  Assume that low level key builder can handle this.
**	20-jul-87 (thurston)
**	    Changed ad0_keycvt() from returning an error when a coercion cannot
**          be done without error, to returning an ALLMATCH.  This covers
**          complex cases such as `i = c' which yields money comparisons.  No
**          real good easy way to detect these situations and do the proper
**          keying, so better to force a full scan. 
**	31-jul-87 (thurston)
**	    Got rid of static routine ad0_keycvt() since new versions of the
**	    low level key building routines can accept input datatypes that are
**	    different from the key to be built.
**	25-aug-87 (thurston)
**	    Fixed a bug that would cause adc_keybld() to access violate when it
**	    attempted to do an ADF_CLR_NULL_MACRO() on a DB_DATA_VALUE whose
**	    .db_data ptr was NULL.  This would only happen if the datatype to
**	    build the key for was nullable, and either the pattern's datatype
**          and length did not match that of the key, OR pattern matching
**          characters could exist for that datatype. 
**	19-oct-87 (thurston)
**	    A bug was found where adc_keybld() would access violate if the type
**	    of key built was ADC_KEXACTKEY, and the low value had a data ptr of
**	    NULL while the high value's data ptr was non-NULL.  This was fixed
**	    by recursively calling the adc_keybld() routine when detected with
**	    exchanging the low and high data ptrs.  (Currently, this will never
**	    occur in the DBMS since keybld is never called in this way but,
**	    "ya neva no."
**	04-feb-88 (thurston)
**	    Had to revert to old special short-cut for a NULL .db_data ptr for
**	    now.  This should eventually be moved down into the lower level key
**	    builders, since only they TRULY know what to return.
**	02-jun-88 (thurston)
**	    Added comments to the input parameters for the new .adc_escape and
**	    .adc_isescape fields in the ADC_KEY_BLK.
**	10-oct-88 (thurston)
**	    Quick patch in adc_keybld() for case where there is no data pointer
**	    for the constant (or pattern, or input, or whatever you want to
**	    call it).  When the constant datatype is C and the column is some
**	    other datatype, then we should return ALLMATCH here.
**	20-mar-89 (jrb)
**	    Made changes for adc_lenchk interface change.
**	10-apr-89 (mikem)
**	    Logical key development.  Added support for DB_LOGKEY_TYPE and
**	    DB_TABKEY_TYPE.  These types do not support pattern matching.
**	28-apr-89 (fred)
**	    Altered to use ADI_DT_MAP_MACRO().
**	05-jun-89 (fred)
**	    Completed above fix.
**	21-may-90 (jrb)
**	    Changed case where no key data were provided and the key type was
**	    abstract and different from the column's type; fixes bug #21291.
**	27-aug-92 (stevet)
**	    Fixed B45057 and B45510.  The return key types for CHARACTER keys
**	    when input type is INTEGER or FLOAT are inconsistence
**	    between optimization phase and execution phase of the QP.
**	    The problem is in these two situations, data values are not
**	    available during optimization, which cause a different key
**	    type being return.  The fix is to change the return key
**	    type to be the same with or without given data value.
**      22-dec-1992 (stevet)
**          Added function prototype.
**      14-jun-1993 (stevet)
**          Introduced the support for NULLABLE key join (B40762).  For
**          nullable join (ADI_NULJN_OP) we look at whether the input
**          value is NULL, if not, then we simply handle ADI_NULJN_OP
**          like ADI_EQ_OP.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	10-jan-94 (rickh)
**	    When building a key for a nulljoin, we weren't filling in a
**	    key type under some circumstances.
**	26-sep-95 (inkdo01)
**	    Changes to support > and < index optimization.
**	11-sep-96 (inkdo01)
**	    Changes to build a key for "<>" (ne) operator, for better
**	    OPF selectivity estimates.
**       7-jul-98 (sarjo01)
**          Bugs 91176, 91265: Disallow key build for "<>" if QUEL.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-Jun-2008 (kiria01) SIR120473
**	    Move adc_isescape into new adc_pat_flags.
**	    Extend the ADE_ESC_CHAR to support an optional second
**	    parameter.
**/

static VOID adc_ixopt(		/* local function declaration */	
ADC_KEY_BLK         *adc_kblk,
i4                   ky_bdt,
i4              ky_blen);

/*{
** Name: adc_keybld()	- Build a HASH, ISAM or BTREE key for a data value.
**
** Description:
**      This function is the external ADF call "adc_keybld()" which builds a
**      HASH, ISAM or BTREE key from the supplied data value. 
**
**      This operation will be used by OPF for building keys for traversing
**      HASH, ISAM and BTREE indexes.  It is a complex operation which requires
**      some explanation.  Whenever the INGRES DBMS has a value, and decides to
**      look up that value in a table using an ordered index (either HASH, ISAM
**      or B-TREE), it will use that value to form two other values.  These
**	two values will represent the "key", or more clearly, the lower and
**	upper limits of the search space.  It is *NOT* guaranteed that all
**	values between these two will match, but it *IS* guaranteed that all
**	values in the relation that match will be between the lower and upper
**	limits produced by adc_keybld().
**
**      Along with the value being keyed on, the caller of this routine needs to
**      specify, the comparison operator being used (e.g. `<', `>=', `like',
**	`is null'). 
**
**	The main job of this routine is to build the upper and lower values
**      of the search space (called the `high key' and `low key', respectively).
**      The building of the high and/or low key can be omitted, however, by
**	supplying the .db_data pointer for it/them as NULL.  This might be used
**	if the caller is only interested in finding out what `type of key' gets
**	built in this situation, but is not interested in the search space.
**
**      In addition to building the upper and lower values of the search space,
**      adc_keybld() returns the `type of key' that was formed.  This tells what
**	type of search should be performed. 
**
**                            **********************
**                            **  IMPORTANT NOTE  **
**      *********************************************************************
**      **  There is an implied ordering for the defined key type values.  **
**      **     If this ordering is changed, the optimizer will break.      **
**      *********************************************************************
**
**      The different key types for the Jupiter version of the INGRES DBMS are: 
**
**	Key Type	Meaning					    Ordering
**	-------------	----------------------------------------    --------
**	ADC_KNOMATCH	None of the values in the relation can		<
**                      possibly match, so no scan of the
**                      relation should be done.  In this case,
**                      the low key will be set to the max value
**                      for the datatype/length of the column
**                      being keyed, and the high key will be
**                      set to the min.
**	ADC_KEXACTKEY	Only a single value from the relation		<
**                      will match.  In this case, the low and
**                      high key will be set to the same value.
**                      The execution phase should seek to this
**                      point and scan forward until it is sure
**                      that it has exhausted all possible
**                      matching values.
**	ADC_KRANGEKEY	All values in the relation that will		<
**                      match lay within a range.  The low key
**                      will be set to represent the lowest
**                      value in the relation that can possibly
**                      be matched, while the high key will be
**                      set to represent the highest value that
**                      can be matched.  The execution phase
**                      should seek to the point matching the
**                      low key, then scan forward until it has
**                      exhausted all values that might be less
**                      than or equal to the high key. 
**	ADC_KHIGHKEY	All values in the relation that will		<
**                      match lay at the low end of the
**                      relation; they are less than or equal to
**                      some value.  In this case, the high key
**                      is set to that value (the upper bound),
**                      and the low key is set to the min value
**                      for the datatype/length of the columned
**                      being keyed (unbounded). The execution
**                      phase should start at the beginning of
**                      the relation and seek forward until it
**                      has exhausted all values that might be
**                      less than or equal to the high key. 
**	ADC_KLOWKEY	All values in the relation that will		<
**                      match lay at the high end of the
**                      relation; they are greater than or equal
**                      to some value.  In this case, the low
**                      key is set to that value (the lower
**                      bound), and the high key is set to the
**                      max value for the datatype/length of the
**                      columned being keyed (unbounded).  The
**                      execution phase should seek to the point
**                      of the low key and scan forward from
**                      there. 
**	ADC_KNOKEY	The boolean expression was too complex for    <
**                      the optimizer to use.  NOTE, THIS VALUE IS
**                      NEVER RETURNED BY THIS ROUTINE, BUT IS
**                      USED INTERNALLY BY THE OPTIMIZER. 
**	ADC_KALLMATCH	All values in the relation may match.
**                      In this case, the low key is set to the
**                      min value for the datatype/length of the
**                      column being keyed, and the high key is
**                      set to the max.  A full scan of the
**                      relation should be done. 
**
**
**	*** A SPECIAL CASE ***
**	    --------------
**		When this routine is called with the input value's .db_data
**		pointer being NULL, then adc_keybld() merely returns the "worst
**		case" key type that would be returned, ignoring the possibility
**		of pattern matching.  The optimizer uses this information in
**		figuring the best query plan to build when it does not have the
**		actual data.  This can happen in repeat queries, as well as in
**		function keys (i.e.  where the value used to key into the
**		relation is the result of some function).
**
**
**      Although there is a fairly strong correlation between the key operator
**      and the type of key that gets built, one cannot predict with certainty
**      the type of key that will be built based on the key operator.  As an
**      example, take the case where we are keying on an i2 column with the `<'
**      operator. However, our supplied key value is an i4 whose value is 50000.
**      The key that gets built here is ADC_KALLMATCH, not ADC_KHIGHKEY, as you
**      may have expected.  Therefore, DO NOT RELY ON THE KEY OPERATOR TO TELL
**      YOU WHAT TYPE OF KEY WILL BE BUILT! 
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	adc_kblk			Ptr to an ADC_KEY_BLK.
**	    .adc_kdv			The DB_DATA_VALUE to form a key for.
**		.db_datatype		Datatype of data value.
**		.db_prec		Precision/Scale of data value
**		.db_length		Length of data value.
**		.db_data		Pointer to the actual data.
**					If this is NULL (which can happen
**					for user parameters in repeat queries),
**					then this routine simply fills in the
**					.adc_tykey field with what would
**					"probably" be built.
**	    .adc_opkey			Key operator used to tell this
**					routine what kind of comparison
**					operation the key is to be
**					built for, as described above.
**					NOTE: ADI_OP_IDs should not be used
**					directly.  Rather the caller should
**					use the adi_opid() routine to obtain
**					the value that should be passed in here.
**          .adc_lokey			DB_DATA_VALUE to recieve the low key.
**					Note, its datatype and length should
**					be the same as .adc_hikey's datatype
**					and length.
**		.db_datatype		Datatype for low key.
**		.db_prec		Precision/Scale for low key
**	    	.db_length		Length for low key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the low key.  (If this
**					is NULL, the low key will not be set.)
**	    .adc_hikey			DB_DATA_VALUE to recieve the high key.
**					Note, its datatype and length should
**					be the same as .adc_lokey's datatype 
**					and length.
**		.db_datatype		Datatype for high key.
**		.db_prec		Precision/Scale for high key
**		.db_length		Length for high key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the high key.  (If this
**					is NULL, the low key will not be set.)
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
**      adc_kblk			Ptr to an ADC_KEY_BLK.
**	    .adc_tykey			The type of key which was built,
**                                      as described above.  This will
**					be one of the following:
**						ADC_KNOMATCH
**						ADC_KEXACTKEY
**						ADC_KRANGEKEY
**						ADC_KHIGHKEY
**						ADC_KLOWKEY
**						ADC_KALLMATCH
**	    .adc_lokey
**		.db_data		If this ptr is not NULL, then the
**					actual data for it will be put at
**					this location.
**          .adc_hikey
**		.db_data		If this ptr is not NULL, then the
**					actual data for it will be put at
**					this location.
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
**          E_AD0000_OK			Operation succeeded.
**          E_AD2004_BAD_DTID		Datatype unknown to ADF.
**          E_AD3002_BAD_KEYOP		Key operator unknown to ADF.
**
**	    NOTE:			The following error numbers are internal
**					consistency checks that will only be
**					generated when the system is compiled
**					with the xDEBUG flag.
**
**          E_AD2005_BAD_DTLEN		Internal length is illegal for
**					the given datatype.
**          E_AD3001_DTS_NOT_SAME	The datatypes of the key DB_DATA_VALUEs
**					are not the same.
**          E_AD3003_DLS_NOT_SAME	The length specifications for the key
**					results, adc_hikey and adc_lokey, were
**					not the same.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      25-feb-86 (thurston)
**          Initial creation.
**	06-jun-86 (ericj)
**	    Changed specification.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	14-nov-86 (thurston)
**	    Made work for the LIKE operator.
**	02-dec-86 (thurston)
**	    Added the ad0_keycvt() static routine to avoid having to create
**	    a large buffer on stack for every call to adc_keybld().  Now this
**	    buffer will only be created if actually needed.
**	13-jan-87 (thurston)
**	    Added special case code to handle the .db_data PTR in the input
**	    key value being NULL.  This can occurr for user parameters in
**	    repeat queries, and this routine should just fill in the type of
**	    key to be built to what is "probably" going to be built.
**	22-feb-87 (thurston)
**	    Added support for nullable datatypes.
**	26-mar-87 (thurston)
**	    Added code to prevent the lo and hi keys' data are from being
**	    written if their .db_data ptr is NULL.
**	19-apr-87 (thurston)
**	    Added a FIX_ME jury rig to allow OPF to call this routine with
**	    the `not like' and `!=' operators; they will always return
**	    ADC_KALLMATCH.
**	18-jun-87 (thurston)
**	    Altered this routine so that the low and high key values are ALWAYS
**	    set, regardless of the type of key built, unless overridden by a
**	    NULL .db_data pointer.
**	25-aug-87 (thurston)
**	    Fixed a bug that would cause adc_keybld() to access violate when it
**	    attempted to do an ADF_CLR_NULL_MACRO() on a DB_DATA_VALUE whose
**	    .db_data ptr was NULL.  This would only happen if the datatype to
**	    build the key for was nullable, and either the pattern's datatype
**          and length did not match that of the key, OR pattern matching
**          characters could exist for that datatype. 
**	19-oct-87 (thurston)
**	    A bug was found where access violation would occur if the type of
**	    key built was ADC_KEXACTKEY, and the low value had a data ptr of
**	    NULL while the high value's data ptr was non-NULL.  This was fixed
**	    by recursively calling the adc_keybld() routine when detected with
**	    exchanging the low and high data ptrs.  (Currently, this will never
**	    occur in the DBMS since keybld is never called in this way but,
**	    "ya neva no.")
**	04-feb-88 (thurston)
**	    Had to revert to old special short-cut for a NULL .db_data ptr for
**	    now.  This should eventually be moved down into the lower level key
**	    builders, since only they TRULY know what to return.
**	02-jun-88 (thurston)
**	    Added comments to the input parameters for the new .adc_escape and
**	    .adc_isescape fields in the ADC_KEY_BLK.  No actual source change
**	    needs to happen in this file ... the low level key building routine
**	    for character strings will have to deal with it.
**	10-oct-88 (thurston)
**	    Quick patch for case where there is no data pointer for the constant
**	    (or pattern, or input, or whatever you want to call it).  When the
**	    constant datatype is C and the column is some other datatype, then
**	    we should return ALLMATCH here.
**	10-apr-89 (mikem)
**	    Logical key development.  Added support for DB_LOGKEY_TYPE and
**	    DB_TABKEY_TYPE.  These types do not support pattern matching.
**	07-jul-89 (jrb)
**	    Added decimal, money, and date to the list of datatypes which do
**	    not have pattern matching.  Also added code to make sure short-cuts
**	    are not taken on DECIMAL unless precision matches as well as length.
**	21-may-90 (jrb)
**	    Changed case where no key data were provided and the key type was
**	    abstract and different from the column's type; this case now returns
**	    "all match" for reasons explained in comments below.
**	27-aug-92 (stevet)
**	    Fixed B45057 and B45510.  Changed the return key type to
**	    ADC_KALLMATCH when input datatype is INTEGER or FLOAT and
**	    the output type is CHARACTER and the data value is not given.
**      14-jun-1993 (stevet)
**          Introduced the support for NULLABLE key join (B40762).  For
**          nullable join (ADI_NULJN_OP) we look at whether the input
**          value is NULL, if not, then we simply handle ADI_NULJN_OP
**          like ADI_EQ_OP.
**	10-jan-94 (rickh)
**	    When building a key for a nulljoin, we weren't filling in a
**	    key type under some circumstances.
**	11-sep-96 (inkdo01)
**	    Changes to build a key for "<>" (ne) operator, for better
**	    OPF selectivity estimates.
**	27-jan-2000 (hayke02)
**	    We now do not call adc_ixopt() if we are executing a DBP
**	    (AD_1RSVD_EXEDBP set in the ADI_WARN block). This prevents
**	    incorrect results when a DBP parameter is used in multiple
**	    predicates. This change fixes bug 99239.
**      19-nov-2008 (huazh01)
**          Back out the fix to b99239, which can't be reproduced
**          after 2.x code line. This allows the 'ad_1rsvd_cnt'
**          field to be removed from ADI_WARN structure. (b121246).
**	27-Mar-2009 (kiria01) b121841
**	    Move NULL buffer logic substantially into the datatype
**	    specific code so that datatype specific checks can be
**	    made even in the absence of a buffer.
**	    Preload the adc_tykey so callee need not worry about
**	    default settings.
**	    NOTE: This routine is often called three or more
**	    times for a given qualifier and each need to return
**	    the same decision regarding key type. Thus before when
**	    the first of the calls might not have had a key buffer
**	    the result was short-circuited by generic code regardless
**	    of whether offered datatypes scould yield no key. In the
**	    worst arrangement, a btree lookup might be prepared but a
**	    full scan needed with inappropriate datatypes present.
**	    This decision is now pushed down to the datatype specific
**	    routines.
*/

DB_STATUS
adc_keybld(
ADF_CB              *adf_scb,
ADC_KEY_BLK	    *adc_kblk)
{
    DB_STATUS           db_stat = E_DB_OK;
    i4		*err_code = &adf_scb->adf_errcb.ad_errcode;
    ADC_KEY_BLK		*kb_ptr = adc_kblk;
    i4			in_bdt;
    i4			in_bprec;
    i4		in_blen;
    i4			ky_bdt   = abs((i4) adc_kblk->adc_lokey.db_datatype);
    i4			ky_bprec = adc_kblk->adc_lokey.db_prec;
    i4		ky_blen  = adc_kblk->adc_lokey.db_length -
				    (adc_kblk->adc_lokey.db_datatype < 0
					? 1 : 0);
    i4			using_local = FALSE;
    i4			pat_match = TRUE;	    /* Can the input data 
						    ** possibly have pattern
						    ** matching characters.
						    */
    i4			tmp_in;
    i4			tmp_out;
    ADC_KEY_BLK		local_kblk;
    i4			m_in_bdt;	/* Mapped version of in_bdt */
    i4			m_ky_bdt;


    /*
    **	Check for `is null' and `is not null', which can all be handled here:
    */
    if (adc_kblk->adc_opkey == ADI_ISNUL_OP)
    {
	if (adc_kblk->adc_lokey.db_datatype > 0)
	{
	    /*
	    **	Op is `is null', and the datatype is non-nullable.  Call it
	    **	a NOMATCH since no null values can possibly exist.
	    */
	    adc_kblk->adc_tykey = ADC_KNOMATCH;

	    /* Set low key to MAX, high key to MIN */
	    *err_code = E_AD0000_OK;
	    return (adc_minmaxdv(adf_scb,
				 &adc_kblk->adc_hikey,
				 &adc_kblk->adc_lokey));
	}
	else
	{
	    /*
	    **	Op is `is null', but the datatype is nullable.  Call it
	    **	an EXACTKEY where the only match will be the null value.
	    */
	    adc_kblk->adc_tykey = ADC_KEXACTKEY;

	    /* Set low key to NULL, high key to NULL */
	    if (adc_kblk->adc_lokey.db_data != NULL)
		ADF_SETNULL_MACRO(&adc_kblk->adc_lokey);
	    if (adc_kblk->adc_hikey.db_data != NULL)
		ADF_SETNULL_MACRO(&adc_kblk->adc_hikey);
	    *err_code = E_AD0000_OK;
	    return (E_DB_OK);
	}
    }
    else if (adc_kblk->adc_opkey == ADI_NONUL_OP)
    {
	/*
	**  Op is `is not null'.  If the datatype is non-nullable, call it
	**  an ALLMATCH since all values will be non-null.  If the datatype
	**  is nullable, call it a HIGHKEY with the upperbound being the
	**  highest possible non-null value for the datatype.
	*/
	if (adc_kblk->adc_lokey.db_datatype > 0)
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	else
	    adc_kblk->adc_tykey = ADC_KHIGHKEY;

	/* Set low key to MIN, high key to MAX */
	*err_code = E_AD0000_OK;
	return (adc_minmaxdv(adf_scb,
			     &adc_kblk->adc_lokey,
			     &adc_kblk->adc_hikey));
    }

    /* Check for datatype and parameter consistency */
    
    in_bdt   = abs((i4) adc_kblk->adc_kdv.db_datatype);
    in_bprec = adc_kblk->adc_kdv.db_prec;
    in_blen  = adc_kblk->adc_kdv.db_length - (adc_kblk->adc_kdv.db_datatype < 0
							? 1 : 0);
    m_in_bdt = ADI_DT_MAP_MACRO(in_bdt);
    m_ky_bdt = ADI_DT_MAP_MACRO(ky_bdt);
    /* Check if the input and output datatypes are valid */
    if  (   m_in_bdt <= 0 || m_in_bdt > ADI_MXDTS
	 || m_ky_bdt <= 0 || m_ky_bdt > ADI_MXDTS
	 || Adf_globs->Adi_dtptrs[m_in_bdt] == NULL
	 || Adf_globs->Adi_dtptrs[m_ky_bdt] == NULL
	)
	return (adu_error(adf_scb, E_AD2004_BAD_DTID, 0));

    /*
    **	Check for `not like' and `!=', which can all be handled right here:
    */
    if	( (adc_kblk->adc_opkey == ADI_NLIKE_OP) ||
          (adc_kblk->adc_opkey == ADI_NE_OP && adf_scb->adf_qlang == DB_QUEL)
        )
    {
	if (adc_kblk->adc_lokey.db_datatype > 0)
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	else
	    adc_kblk->adc_tykey = ADC_KHIGHKEY;

	/* Set low key to MIN, high key to MAX */
	*err_code = E_AD0000_OK;
	return (adc_minmaxdv(adf_scb,
			     &adc_kblk->adc_lokey,
			     &adc_kblk->adc_hikey));
    }

#ifdef    xDEBUG_OBSOLETE
    /* Check the consistency of the internal datalengths of the DB_DATA_VALUE
    ** structs passed in the adc_kblk parameter.
    */
    if (db_stat = adc_lenchk(adf_scb, FALSE, &adc_kblk->adc_lokey, NULL))
	return (db_stat);
    if (db_stat = adc_lenchk(adf_scb, FALSE, &adc_kblk->adc_kdv, NULL))
	return (db_stat);
	
    /* Check for output datatype consistency */
    if (adc_kblk->adc_lokey.db_datatype != adc_kblk->adc_hikey.db_datatype)
	return (adu_error(adf_scb, E_AD3001_DTS_NOT_SAME, 0));

    /* Check for output length consistency */
    if (adc_kblk->adc_lokey.db_length != adc_kblk->adc_hikey.db_length)
	return (adu_error(adf_scb, E_AD3003_DLS_NOT_SAME, 0));
#endif /* xDEBUG_OBSOLETE */


    /* Preset adc_tykey to the usual value based on OP */
    switch (adc_kblk->adc_opkey)
    {
    case ADI_LIKE_OP:
	adc_kblk->adc_tykey = ADC_KRANGEKEY;
	break;

    case ADI_EQ_OP:
    case ADI_NULJN_OP:
	adc_kblk->adc_tykey = ADC_KEXACTKEY;
	break;

    case ADI_GT_OP:
    case ADI_GE_OP:
	adc_kblk->adc_tykey = ADC_KLOWKEY;
	break;

    case ADI_LT_OP:
    case ADI_LE_OP:
	adc_kblk->adc_tykey = ADC_KHIGHKEY;
	break;

    case ADI_NE_OP:
	adc_kblk->adc_tykey = ADC_KNOKEY;
	break;

    default:
	/* Check if the operator id value passed in is valid */
	return (adu_error(adf_scb, E_AD3002_BAD_KEYOP, 0));

    }

    if (adc_kblk->adc_kdv.db_data == NULL)
    {
	/* If the input datatype (the key value) is an abstract type whose type
	** is different from the column's type, we are in a bind.  In many cases
	** we have a one to many relationship between an intrinsic and an
	** abstract value (for instance, many different string values map to the
	** same date value) so there is no way to build keys for an instrinsic
	** column using an abstract key.  For example, you can't build a key
	** into a char column using a date key because the actual comparison
	** will be done as a date comparison (because dates have higher
	** precedence than strings) and a date value can be specified by many
	** different char values.
	**
	** So we return all-match in this case.
	*/
	if ((Adf_globs->Adi_dtptrs[m_in_bdt]->adi_dtstat_bits & AD_INTR) == 0)
	{
	    if (in_bdt != ky_bdt)
		adc_kblk->adc_tykey = ADC_KALLMATCH;

	    /* As adc_kdv has a NULL buffer - don't pass to it's specific
	    ** keybuild routine - accept the pre-loaded adc_tykey */
	    return(E_DB_OK);
	}
    }
    else
    {
	/* 
	** Now we are really building a key.  First do some special processing
	** to handle NULLABLE KEY JOIN and input key value that is NULL.
	*/
	if (  adc_kblk->adc_opkey == ADI_NULJN_OP)
	{
	    if( ADI_ISNULL_MACRO(&adc_kblk->adc_kdv))
	    {
		if( adc_kblk->adc_lokey.db_datatype > 0 || 
		    adc_kblk->adc_hikey.db_datatype > 0)
		{
		    /* Cannot do NULL join when output data is not nullable */
		    adc_kblk->adc_tykey = ADC_KNOMATCH; 
		    *err_code = E_AD0000_OK;
		    return (adc_minmaxdv(adf_scb,
				 &adc_kblk->adc_hikey,
				 &adc_kblk->adc_lokey));
		}
		/* Set output values to NULL */
		adc_kblk->adc_tykey = ADC_KEXACTKEY;
		if (adc_kblk->adc_lokey.db_data != NULL)
		    ADF_SETNULL_MACRO(&adc_kblk->adc_lokey);
		if (adc_kblk->adc_hikey.db_data != NULL)
		    ADF_SETNULL_MACRO(&adc_kblk->adc_hikey);
		*err_code = E_AD0000_OK;
		return (E_DB_OK);
	    }
	    else
	    {
		/* 
		** The input value is not null so we want to handle the
		** KEYBLD like ADI_EQ_OP but first we have to strip the
		** null bytes.
		*/
		using_local = TRUE;
		kb_ptr = &local_kblk;

		STRUCT_ASSIGN_MACRO(*adc_kblk, local_kblk);
		if (local_kblk.adc_kdv.db_datatype < 0)
		{
		    local_kblk.adc_kdv.db_datatype = in_bdt;
		    local_kblk.adc_kdv.db_length--;
		}
		if (local_kblk.adc_lokey.db_datatype < 0)
		{
		    if (local_kblk.adc_lokey.db_data != NULL)
			ADF_CLRNULL_MACRO(&local_kblk.adc_lokey);
		    local_kblk.adc_lokey.db_datatype = ky_bdt;
		    local_kblk.adc_lokey.db_length--;

		    if (local_kblk.adc_hikey.db_data != NULL)
			ADF_CLRNULL_MACRO(&local_kblk.adc_hikey);
		    local_kblk.adc_hikey.db_datatype = ky_bdt;
		    local_kblk.adc_hikey.db_length--;
		}
		local_kblk.adc_opkey = ADI_EQ_OP;
	    }
	}
	else if (ADI_ISNULL_MACRO(&adc_kblk->adc_kdv))  /* input is NULL value 
							** and we are not doing
							** NULL join
							*/
	{
	    adc_kblk->adc_tykey = ADC_KNOMATCH; /* This is because a NULL value
						** is never equal to, less than,
						** greater than, or `like' any
						** value, including another NULL
						** value.
						*/
	    /* Set low key to MAX, high key to MIN */
	    *err_code = E_AD0000_OK;
	    return (adc_minmaxdv(adf_scb,
				 &adc_kblk->adc_hikey,
				 &adc_kblk->adc_lokey));
	}


	/*
	** At this point we know we do not have a NULL value as input.  The
	** datatype of the input and/or the output could, however, be nullable.
	** --------------------------------------------------------------------
	*/

	/*
	** If there's no possibility of pattern matching and the input value's
	** base datatype/precision/length are the same as the output keys'
	** base datatype/precision/length, just copy input to output (doing
	** appropriate nullable stuff), and set the return value according to the
	** comparison op type.  (The precisions must match only if the types are
	** DECIMAL.)
	*/
	if (in_bdt == ky_bdt &&
	    (in_bdt != DB_DEC_TYPE || in_bprec == ky_bprec) &&
	    in_blen == ky_blen &&
	    /* and no possibility of pattern matching ...*/
	    (in_bdt == DB_FLT_TYPE ||  
	     in_bdt == DB_INT_TYPE ||
	     in_bdt == DB_DEC_TYPE ||
	     in_bdt == DB_MNY_TYPE ||
	     in_bdt == DB_DTE_TYPE ||
	     in_bdt == DB_LOGKEY_TYPE ||
	     in_bdt == DB_TABKEY_TYPE))
	{
	    switch(adc_kblk->adc_opkey)
	    {
	      case ADI_EQ_OP:
	      case ADI_NE_OP:
	      case ADI_NULJN_OP:
		if (adc_kblk->adc_opkey == ADI_NE_OP)
		    adc_kblk->adc_tykey = ADC_KNOKEY;
		 else adc_kblk->adc_tykey = ADC_KEXACTKEY;
		/* Set both low and high key to the value */
		if (adc_kblk->adc_lokey.db_data != NULL)
		{
		    MEcopy( (PTR) adc_kblk->adc_kdv.db_data,
			    ky_blen,
			    (PTR) adc_kblk->adc_lokey.db_data);
		    if (adc_kblk->adc_lokey.db_datatype < 0)
			ADF_CLRNULL_MACRO(&adc_kblk->adc_lokey);
		}
		if (adc_kblk->adc_hikey.db_data != NULL)
		{
		    MEcopy( (PTR) adc_kblk->adc_kdv.db_data,
			    ky_blen,
			    (PTR) adc_kblk->adc_hikey.db_data);
		    if (adc_kblk->adc_hikey.db_datatype < 0)
			ADF_CLRNULL_MACRO(&adc_kblk->adc_hikey);
		}
		break;

	      case ADI_GT_OP:
	      case ADI_GE_OP:
		adc_kblk->adc_tykey = ADC_KLOWKEY;
		/* Set low key to the value, and set high key to the MAX */
		if (adc_kblk->adc_lokey.db_data != NULL)
		{
		    MEcopy( (PTR) adc_kblk->adc_kdv.db_data,
			    ky_blen,
			    (PTR) adc_kblk->adc_lokey.db_data);
		    if (adc_kblk->adc_lokey.db_datatype < 0)
			ADF_CLRNULL_MACRO(&adc_kblk->adc_lokey);
		}
		db_stat = adc_minmaxdv(adf_scb,
				       (DB_DATA_VALUE *) NULL,
				       &adc_kblk->adc_hikey);
		/* Check for index positioning optimization */
		if (adc_kblk->adc_opkey == ADI_GT_OP &&
		     adc_kblk->adc_lokey.db_data != NULL)
		    adc_ixopt(adc_kblk, ky_bdt, ky_blen);
		break;

	      case ADI_LT_OP:
	      case ADI_LE_OP:
		adc_kblk->adc_tykey = ADC_KHIGHKEY;
		/* Set high key to the value, and set low key to the MIN */
		if (adc_kblk->adc_hikey.db_data != NULL)
		{
		    MEcopy( (PTR) adc_kblk->adc_kdv.db_data,
			    ky_blen,
			    (PTR) adc_kblk->adc_hikey.db_data);
		    if (adc_kblk->adc_hikey.db_datatype < 0)
			ADF_CLRNULL_MACRO(&adc_kblk->adc_hikey);
		}
		db_stat = adc_minmaxdv(adf_scb,
				       &adc_kblk->adc_lokey,
				       (DB_DATA_VALUE *) NULL);
		/* Check for index positioning optimization */
		if (adc_kblk->adc_opkey == ADI_LT_OP &&
		     adc_kblk->adc_hikey.db_data != NULL)
		    adc_ixopt(adc_kblk, ky_bdt, ky_blen);
		break;
	    }

	    *err_code = E_AD0000_OK;
	    return (E_DB_OK);
	}
    }
    /* Look for nullable types, and set up local key block if there are any */
    if ( adc_kblk->adc_opkey != ADI_NULJN_OP 
	&& (adc_kblk->adc_kdv.db_datatype < 0
	    ||  adc_kblk->adc_lokey.db_datatype < 0)
       )
    {
	using_local = TRUE;
	kb_ptr = &local_kblk;

	local_kblk = *adc_kblk;
	if (local_kblk.adc_kdv.db_datatype < 0)
	{
	    local_kblk.adc_kdv.db_datatype = in_bdt;
	    local_kblk.adc_kdv.db_length--;
	}
	if (local_kblk.adc_lokey.db_datatype < 0)
	{
	    if (local_kblk.adc_lokey.db_data != NULL)
		ADF_CLRNULL_MACRO(&local_kblk.adc_lokey);
	    local_kblk.adc_lokey.db_datatype = ky_bdt;
	    local_kblk.adc_lokey.db_length--;

	    if (local_kblk.adc_hikey.db_data != NULL)
		ADF_CLRNULL_MACRO(&local_kblk.adc_hikey);
	    local_kblk.adc_hikey.db_datatype = ky_bdt;
	    local_kblk.adc_hikey.db_length--;
	}
    }

#ifdef    xDEBUG
    if (Adf_globs->Adi_dtptrs[m_ky_bdt]->
			    adi_dt_com_vect.adp_keybld_addr == NULL)
    {
	/* No function listed in datatypes table. */
	return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }
#endif /* xDEBUG */
    /****** MWC adc_kblk->adc_kdv.db_data is NULL  *****/
    db_stat = (*Adf_globs->Adi_dtptrs[m_ky_bdt]->
		    adi_dt_com_vect.adp_keybld_addr)
			(adf_scb, kb_ptr);

    /* Pick up type incase adjusted */
    if (using_local)
	adc_kblk->adc_tykey = kb_ptr->adc_tykey;

    if (DB_SUCCESS_MACRO(db_stat))
    {
	/*
	** Depending on the type of key that has been built by the lower lever
	** routines, we may have to set the low and/or high key values to the
	** appropriate MIN/MAX.
	*/
	switch (adc_kblk->adc_tykey)
	{
	  case ADC_KNOMATCH:
	    /* Set low key to MAX, high key to MIN */
	    db_stat = adc_minmaxdv(adf_scb,
				   &adc_kblk->adc_hikey,
				   &adc_kblk->adc_lokey);
	    break;

	  case ADC_KEXACTKEY:
	    /*
	    ** Must set high key to same as low key, or
	    ** recalculate if low value's data ptr was NULL.
	    */
	    if (adc_kblk->adc_hikey.db_data != NULL)
	    {
		if (adc_kblk->adc_lokey.db_data != NULL)
		{
		    /* Just copy the data */
		    MEcopy( (PTR) adc_kblk->adc_lokey.db_data,
			    ky_blen,
			    (PTR) adc_kblk->adc_hikey.db_data);
		    if (adc_kblk->adc_hikey.db_datatype < 0)
			ADF_CLRNULL_MACRO(&adc_kblk->adc_hikey);
		}
		else
		{
		    /* Must re-calculate */
		    STRUCT_ASSIGN_MACRO(*adc_kblk, local_kblk);

		    /*
		    ** we know that high and low keys have same
		    ** datatype/length, so we only have to move
		    ** the .db_data ptr from high to low.
		    */
		    local_kblk.adc_lokey.db_data = local_kblk.adc_hikey.db_data;
		    local_kblk.adc_hikey.db_data = NULL;
		    db_stat = adc_keybld(adf_scb, &local_kblk);
		}
	    }
	    break;

	  case ADC_KHIGHKEY:
	    /* Must set low key to MIN */
	    db_stat = adc_minmaxdv(adf_scb,
				   &adc_kblk->adc_lokey,
				   (DB_DATA_VALUE *) NULL);
            /* Check for index positioning optimization */
            if (adc_kblk->adc_opkey == ADI_LT_OP &&
		 adc_kblk->adc_hikey.db_data != NULL)
		adc_ixopt(adc_kblk, ky_bdt, ky_blen);
	    break;

	  case ADC_KLOWKEY:
	    /* Must set high key to MAX */
	    db_stat = adc_minmaxdv(adf_scb,
				   (DB_DATA_VALUE *) NULL,
				   &adc_kblk->adc_hikey);
            /* Check for index positioning optimization */
            if (adc_kblk->adc_opkey == ADI_GT_OP &&
		 adc_kblk->adc_lokey.db_data != NULL)
		adc_ixopt(adc_kblk, ky_bdt, ky_blen);
	    break;

	  case ADC_KALLMATCH:
	    /* Set low key to MIN, high key to MAX */
	    db_stat = adc_minmaxdv(adf_scb,
				   &adc_kblk->adc_lokey,
				   &adc_kblk->adc_hikey);
	    break;
	}
    }

    if (db_stat == E_DB_OK)
    	*err_code = E_AD0000_OK;

    return (db_stat);
}

/*{
** Name: adc_ixopt      - fiddles low/high values of GT/LT keys to improve index 
**			  access
**
** Description:
**	Simply checks for alterable data type (currently int's, char, varchar
**      and money) and changes GT comparison to GE on the next value for that
**      data type (e.g. > 25 is set to >= 26) and analogously for LT
**	comparisons. This avoids a DMF scan of potentially large numbers of
**	rows at the > or < constant, even though they'll be filtered out
**	in QEF.
**
** Inputs:
**	adc_kblk		key control block
**	ky_bdt			normalized (un-nulled) datatype
**	ky_blen			normalized constant length
**
** Outputs:
**	adc_kblk		constant changed, as well as adc_opkey (if 
**				optimization was possible)
**
** History:
**	26-sep-95 (inkdo01)
**		Written.
**	30-oct-97 (inkdo01)
**	    Add TEXT as synonymous with VARCHAR.
**	26-apr-00 (inkdo01)
**	    Change GE to LE (undiscovered bug from earlier range predicate
**	    optimization).
**	12-may-04 (inkdo01)
**	    Added support for bigint.
*/

static VOID adc_ixopt(
ADC_KEY_BLK         *adc_kblk,
i4                   ky_bdt,
i4              ky_blen)
{

/* Separate logic for GT optimization and LT optimization. */

if (adc_kblk->adc_opkey == ADI_GT_OP)  
    {
    /* Simply scan for supported data types, pick up the value
     * to be altered (or byte, in the case of CHA, VCH), see if
     * it's already the max or min (in which case we do nothing),
     * then increment (or decrement) to the ">=" (or "<=") value.
    */
    if (ky_bdt == DB_INT_TYPE)
       switch (ky_blen) {
        case 1:
        {
        i1  *i;
        i = (i1*)adc_kblk->adc_lokey.db_data;
        if (*i == MAXI1) return;
	(*i)++; 
        break;
        }
       case 2:
        {
        i2  *i;
        i = (i2*)adc_kblk->adc_lokey.db_data;
        if (*i == MAXI2) return;
	(*i)++; 
        break;
        }
       case 4:
        {
        i4  *i;
        i = (i4*)adc_kblk->adc_lokey.db_data;
        if (*i == MAXI4) return;
	(*i)++; 
        break;
        }
       case 8:
        {
        i8  *i;
        i = (i8*)adc_kblk->adc_lokey.db_data;
        if (*i == MAXI8) return;
	(*i)++; 
        break;
        }
        }     /* end of integer length switch */
       else if (ky_bdt == DB_CHA_TYPE)
        {
        u_i1   *i;
	/* Last character in string gets diddled */
        i = &((u_i1*)adc_kblk->adc_lokey.db_data)[ky_blen-1];
        if (*i == 255) return;
	(*i)++; 
        }
       else if (ky_bdt == DB_VCH_TYPE || ky_bdt == DB_TXT_TYPE)
        {
        u_i2   *i;
        u_i1   *j;
        i = (u_i2*)adc_kblk->adc_lokey.db_data;  /* get length */
	/* Last character in string gets diddled */
        j = &((u_i1*)adc_kblk->adc_lokey.db_data)[*i+1];
        if (*j == 255) return;
	(*j)++; 
        }
       else if (ky_bdt == DB_MNY_TYPE)
        {
        AD_MONEYNTRNL *i;
        i = (AD_MONEYNTRNL *)adc_kblk->adc_lokey.db_data;
	/* MONEY values are stored in FLOATs as the number of cents
	 * (e.g. $210.50 is stored as 21050.0) - so just add 1.0
	*/
        i->mny_cents += 1.0;
        }
    adc_kblk->adc_opkey = ADI_GE_OP;
    return;
    }
 else if (adc_kblk->adc_opkey == ADI_LT_OP)  
    {
    if (ky_bdt == DB_INT_TYPE)
       switch (ky_blen) {
        case 1:
        {
        i1  *i;
        i = (i1*)adc_kblk->adc_hikey.db_data;
        if (*i == MINI1) return;
	(*i)--; 
        break;
        }
       case 2:
        {
        i2  *i;
        i = (i2*)adc_kblk->adc_hikey.db_data;
        if (*i == MINI2) return;
	(*i)--; 
        break;
        }
       case 4:
        {
        i4  *i;
        i = (i4*)adc_kblk->adc_hikey.db_data;
        if (*i == MINI4) return;
	(*i)--; 
        break;
        }
       case 8:
        {
        i8  *i;
        i = (i8*)adc_kblk->adc_hikey.db_data;
        if (*i == MINI8) return;
	(*i)--; 
        break;
        }
        }     /* end of integer length switch */
      else if (ky_bdt == DB_CHA_TYPE)
        {
        u_i1   *i;
        i = &((u_i1*)adc_kblk->adc_hikey.db_data)[ky_blen-1];
        if (*i == 0) return;
	(*i)--; 
        }
       else if (ky_bdt == DB_VCH_TYPE || ky_bdt == DB_TXT_TYPE)
        {
        u_i2   *i;
        u_i1   *j;
        i = (u_i2*)adc_kblk->adc_hikey.db_data;
        j = &((u_i1*)adc_kblk->adc_hikey.db_data)[*i+1];
        if (*j == 0) return;
	(*j)--; 
        }
       else if (ky_bdt == DB_MNY_TYPE)
        {
        AD_MONEYNTRNL *i;
        i = (AD_MONEYNTRNL *)adc_kblk->adc_hikey.db_data;
        i->mny_cents -= 1.0;
        }
    adc_kblk->adc_opkey = ADI_LE_OP;
    return;
    }

}   	/* end of adc_ixopt */
