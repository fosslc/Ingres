/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adfops.h>
#include    <adffiids.h>
#include    <ade.h>
#include    <adefebe.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adeint.h>

/*
NO_OPTIM = nc4_us5
*/

/*
**
**  Name: ADECOMPILE.C - Compilation phase of ADF's Compiled Expression Module.
**
**  Description:
**      This file contains all of the routines necessary to compile ADF
**      expressions.  Throughout this file, an ADF compiled expression
**      will be abbreviated as, "CX".
**
**	This file contains the following externally visible routines:
**	------------------------------------------------------------
**          ade_cx_space() - Estimate size of the CX to be compiled.
**          ade_bgn_comp() - Begin the compilation of an expression.
**          ade_end_comp() - End the compilation of an expression.
**	    ade_global_bases() - Change all operand bases to reference
**				global buffers.
**          ade_const_gen() - Compile a constant into the CX.
**          ade_instr_gen() - Compile an instruction into the CX.
**	    ade_cxinfo() - Return various pieces of information about a CX.
**          ade_inform_space() - Inform ADE that the CX size has changed.
**
**	This file also contains the following static routines:
**	-----------------------------------------------------
**	    ad0_is_unaligned() - Check to see if operand is unaligned.
**          ad0_dtlenchk() - Check datatype and length validity for an operand.
**	    ad0_setqconst_flag() - Set the query constants flag from an instr.
**	    ad0_base_transform() - Transform operand base numbers.
**
**
**  History:
**      16-jun-86 (thurston)
**          Initial creation.
**	09-jul-86 (thurston)
**	    Coded the ade_bgn_comp(), ade_end_comp(), and ade_const_gen()
**	    routines.
**	10-jul-86 (thurston)
**	    Coded the ade_instr_gen() routine.
**	10-jul-86 (thurston)
**	    Renamed this file ADECOMPILE.C from ADE.C, and split off the
**	    execution phase routines into the ADEEXECUTE.C file.
**	17-jul-86 (thurston)
**	    Added and coded the two new routines ade_cx_space() and
**	    ade_inform_space().
**	21-jul-86 (thurston)
**	    Removed the static routine ade_op2dv() as a result of adding the
**	    static routine ad0_dtlenchk().
**	29-aug-86 (daved)
**	    Make changes to the way types are handled for aggregate processing.
**	    The first param is the ag_struct and doens't need type/len info.
**	    Therefore, don't check it. 
**	    The setup to the calclen call was changed so that dv1 was filled
**	    in if there were at least two args (the first one being the result)
**	    and dv2 was filled in if there were at least 3 args.
**	    The error checking on unaligned data will now cause a break in
**	    program execution. BAD_DTID will be returned if any of the datatypes
**	    don't match what is expected. Formerly, all datatypes had to
**	    mismatch. I assume that the semantics were that all types had to
**	    match.
**	02-sep-86 (thurston)
**	    Removed the restriction that the ADE_KEYBLD instruction be put in
**	    the INIT segment of a CX.
**	11-sep-86 (thurston)
**	    Added code to ade_instr_gen() to allow the compilation of the
**	    ADE_COMPARE instruction.
**	13-oct-86 (thurston)
**	    Fixed bug in ade_instr_gen() that caused the erroneous error
**	    E_AD5503_BAD_DTID_FOR_FIID to be reported when attempting to compile
**          function instances that used less than two input operands.  Also
**	    fixed a bug with compiling the ADE_COMPARE special CX instruction.
**	21-oct-86 (thurston)
**	    Fixed another bug in ade_instr_gen() having to do with compiling
**	    coercions.
**	28-dec-86 (thurston)
**	    Modified ad0_dtlenchk() code to use the constant ADE_LEN_UNKNOWN.
**	15-jan-87 (thurston)
**	    Fixed bug in ade_instr_gen() where ade_unaligned was being set
**	    instead of *ade_unaligned.
**	23-feb-87 (thurston)
**	    Added the ade_cxinfo() routine.
**	24-feb-87 (thurston)
**	    Moved definitions of forward static functions out of the routines
**	    that use them to the top of the file.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	11-mar-87 (thurston)
**	    In ade_instr_gen():  I allowed the ADE_KEYBLD instruction to use
**	    ADI_LIKE_OP.
**	11-mar-87 (thurston)
**	    Added support for nullable datatypes to ade_instr_gen().  This is
**	    done by compiling an optional `nullable pre-instruction' ahead of
**	    the requested function instance if any nullable inputs are found.
**	11-mar-87 (thurston)
**	    In ade_cxsize():  Added fudge to the extimated size of CX to
**	    account for some unaligned operands (requiring extra ADE_MECOPY
**	    instructions) and some function instances with nullable operands
**	    (that may require nullable pre-instructions).
**	11-mar-87 (thurston)
**	    Added support for nullable datatypes to ad0_is_unaligned() and
**	    ad0_dtlenchk().
**	17-mar-87 (thurston)
**	    Added code to ade_instr_gen() to compile the ADE_NAGEND CX
**	    instruction, and the ADE_3CXI_CMP_SET_SKIP instruction.
**	25-mar-87 (thurston)
**	    Changed the ade_result arg for ade_cxinfo() to be a PTR instead of
**	    a i4*.  Also added several other request codes.
**	05-apr-87 (thurston)
**          Added code to ade_instr_gen() to handle a nullable output where no
**          nullable inputs are amidst, and to handle a non-nullable output
**          where nullable inputs do exist and the nullable-pre-instruction is
**          ADE_1CXI_SET_SKIP.  The latter should now return an error from
**          ade_execute_cx(). 
**	21-apr-87 (thurston)
**	    Added the ADE_PMQUEL, ADE_NO_PMQUEL, and ADE_PMFLIPFLOP
**	    instructions to ade_instr_gen().
**	06-may-87 (thurston & wong)
**	    Integrated FE changes.
**	13-may-87 (thurston)
**	    Added request codes ADE_14REQ_LAST_INSTR_OFFSETS and
**	    ADE_15REQ_LAST_INSTR_ADDRS to ade_cxinfo().
**	13-may-87 (thurston)
**	    Changed all end of offset list terminaters to be ADE_END_OFFSET_LIST
**	    instead of zero, since zero is a legal instruction offset for the
**	    FE's.
**	18-jun-87 (thurston)
**	    In ade_instr_gen(), I fixed the check for `datatype ID is not kosher
**	    for the function instance ID' on aggregates.  The code had assumed
**	    that all aggregate function instances would have exactly one input;
**	    count(*) has zero.
**	01-jul-87 (thurston)
**	    Omit result length check for COPY COERCIONs in ade_instr_gen()
**	31-jul-87 (thurston)
**	    Added code to ade_instr_gen() to compile the ADE_PMENCODE
**	    instruction.
**	03-aug-87 (thurston)
**	    Fix made in ade_instr_gen() routine, to code around the
**	    adi_0calclen() call, that was causing OPC to give an `bad
**	    instruction gen' error `unknown len'.
**	22-oct-87 (thurston)
**	    Added the `ade_qconst_map' argument to ade_instr_gen(), and some
**	    dummy code that just sets all of the bits, for now.
**	26-oct-87 (thurston)
**	    Added code to ade_instr_gen() to allow the compilation of the
**	    ADE_BYCOMPARE instruction.
**	27-oct-87 (thurston)
**	    Added the static routine ad0_setqconst_map().
**	10-nov-87 (thurston)
**          Added bug fix to ade_instr_gen() for nullable datatypes with
**          aggregates; Now the output nullability is called "NOT_APPLICABLE".
**          Previously, the datatype of operand #0 was being looked at to
**          determine whether or not the output would be "NULLABLE" or
**          "NON_NULLABLE".  This was a mistake since for aggregates operand #0
**          is the ag-struct, and there is no datatype associated with it, so
**          this field is undefined.  It just so happened to work since a zero
**          was always being placed there by OPC. 
**	04-feb-88 (thurston)
**	    Aparently, VLTs have never been tested.  ade_instr_gen() was not
**	    incrementing the VLT counter in the CX header.  It does now when it
**	    detects a VLT (by seeing a successfull ADE_CALCLEN instruction).
**	05-feb-88 (thurston)
**	    Did some work here to support the MAJOR work done on ADEEXECUTE.C
**	    for fixing VLTs.  Mostly, this involved allowing the ADE_CALCLEN
**	    instruction to accept an additional operand which represents the
**	    output datatype of the instruction corresponding to the ADE_CALCLEN.
**	    All of this work was done in ade_instr_gen().
**	10-feb-88 (thurston)
**	    In ade_instr_gen(), I did away with using the AD_BIT_TYPE for the
**	    NPI's since it didn't work for VLUP's VLT's.  Also, the code no
**	    longer erroneously decrements the operands length if that operand
**	    is a VLUP or VLT.
**	09-jun-88 (thurston)
**	    Added code into ade_instr_gen() to handle the newly created
**	    ADE_ESC_CHAR instruction.
**	06-nov-88 (thurston)
**	    Placed temp estimation max of 16000 until CX estimation algorithm
**	    gets examined in more detail.
**	19-feb-89 (paul)
**	    Added support for new ADE special instructions to support generation
**	    of CALLPROC parameter lists.
**	    ADE_PNAME places the physical address of parameter 1 at the location
**	    described by parameter 0 and the length of parameter 1 in the
**	    location following the parameter 0 pointer.
**	    ADE_DBDV creates a DB_DATA_VALUE for parameter 1 at the location
**	    described by parameter 0.
**	20-mar-89 (jrb)
**	    Made changes for adc_lenchk interface change.
**	28-apr-89 (fred)
**	    Altered to use ADI_DT_MAP_MACRO().
**	05-jun-89 (fred)
**	    Completed above change...
**	11-dec-89 (fred)
**	    Added support for large datatypes.
**      09-mar-92 (jrb, merged by stevet)
**          Handle DB_ALL_TYPE properly for outer-join project.
**      03-nov-1992 (stevet)
**          Added DB_CHA_TYPE, DB_DEC_TYPE, DB_VCH_TYPE and DB_LTXT_TYPE
**          to ad0_is_unaligned.
**      09-dec-1992 (stevet)
**          Cannot call calclen for ADI_LEN_EXTERN and ADI_LEN_INDIRECT 
**          because we don't have proper db_data filled in.
**      11-Dec-1992 (fred)
**          Added alignment restriction to ADE_REDEEM operation.
**          Since the coupons contain integers, the operation must be
**          aligned.
**      29-dec-1992 (stevet)
**          Added function prototype.
**      14-jun-1993 (stevet)
**          Added ADI_NULJN_OP as a vaild operator for KEYBLD.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      15-Jul-1993 (fred)
**          Added DB_VBYTE_TYPE to list of types that need vlup
**          processing, special calclen handling, and/or alignment.
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	25-oct-95 (inkdo01)
**	    Added logic to collapse consecutive MECOPYs of contiguous 
**	    fields.
**	25-oct-95 (inkdo01)
**	    New logic to remove trailing ADE_AND's from expressions.
**	25-jan-96 (toumi01)
**	    In new code for above 2 optimizations change (i4) and (int)
**	    casts to (long) when doing address calculation to avoid
**	    truncation for 64-bit platforms (e.g. axp_osf).
**	3-jul-96 (inkdo01)
**	    Added ADE_4CXI_CMP_SET_SKIP support for predicate function 
**	    operation types.
**	09-oct-96 (mcgem01)
**	    E_AD550C_COMP_NOT_IN_PROG now takes an extra parameter.
**	21-oct-98 (matbe01)
**	    Added NO_OPTIM for NCR (nc4_us5) to fix error "E_LQ003A Cannot
**	    start up 'select' query."
**	02-Mar-1999 (shero03)
**	    support 4 operands for adi_0calclen.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-feb-2001 (somsa01)
**	    Changed usage of other types to SIZE_TYPE for lengths.
**	04-jul-2001 (somsa01)
**	    In ade_instr_gen(), the addition of ADE_UNORM needs to be
**	    wrapped inside the "#ifndef ADE_FRONTEND" block.
**	25-oct-2001 (inkdo01)
**	    In ade_instr_gen(), add CXI_SET_SKIP into AGNEXT.
**	07-nov-2001 (kinte01)
**	    The variable j was not declared
**	29-jul-02 (inkdo01)
**	    Add null instr to ADE_UNORM.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	24-aug-04 (inkdo01)
**	    Add code to transform operand bases to index global base 
**	    array (from backend).
**	20-dec-04 (inkdo01)
**	    Add numerous collID's.
**	17-Feb-2005 (schka24)
**	    Changes for code segment header in cxhead.
**	    Add pmquel removal optimization.
**	25-Mar-2005 (schka24)
**	    I like ifdef better than ifndef, flip 'em around.
**	29-Aug-2005 (schka24)
**	    More minor post-generation optimizations.
*/

/*
**  Definition of static variables and forward static functions.
*/
/* Check datatype & length of operand */
static  DB_STATUS   ad0_dtlenchk(ADF_CB       *adf_scb,
				 ADE_OPERAND  *op);	
/* Set query constant flag */
static  VOID	    ad0_setqconst_flag(i4  icode,
				      i4   *qconst_flag);    
#ifdef ADE_BACKEND
/* Transform operand bases */
static VOID	ad0_base_transform(
	ADE_CXHEAD	*cxhead,
	i4	first_offset,
	i4	last_offset,
	i4	*basemap);

/* Check for unaligned operand */
static  bool        ad0_is_unaligned(ADE_OPERAND  *op,
				     i4           usage); 

static VOID
ade_verlabs(PTR	cxptr,
	i4	start,
	i4	end);

static void ad0_optim_pmquel(
	ADF_CB *adf_scb, ADE_CXHEAD *cxhead, ADE_CXSEG *cxseg);

static void ad0_post_optim(
	ADF_CB *adf_scb, ADE_CXHEAD *cxhead, ADE_CXSEG *cxseg);

#endif /* ADE_BACKEND */


/*{
** Name: ade_cx_space() - Estimate size of the CX to be compiled.
**
** Description:
**      This routine allows the caller to estimate the size of a CX before 
**      beginning the compile.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      ade_ni				Number of instructions to be compiled.
**      ade_nop_tot                     Total # operands needed by all of these
**                                      instructions.
**      ade_nk                          Number of constants to be compiled.
**      ade_ksz_tot                     Total # bytes for all of these
**                                      constants.  That is, the sum of the
**                                      lengths for all of the constants.
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
**      ade_est_cxsize                  Estimate of the number of contiguous
**					bytes needed to compile a CX, given
**                                      the above parameters.
**
**	Returns:
**	    E_DB_OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-jul-86 (thurston)
**          Initial creation and coding.
**	11-mar-87 (thurston)
**	    Added fudge to the extimated size of CX to account for some
**	    unaligned operands (requiring extra ADE_MECOPY instructions)
**	    and some function instances with nullable operands (that may
**	    require nullable pre-instructions).
**	06-nov-88 (thurston)
**	    Placed temp estimation max of 16000 until CX estimation algorithm
**	    gets examined in more detail.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
ade_cx_space(
ADF_CB             *adf_scb,
i4                 ade_ni,
i4                 ade_nop_tot,
i4                 ade_nk,
i4		   ade_ksz_tot,
i4            *ade_est_cxsize)
# else
DB_STATUS
ade_cx_space( adf_scb, ade_ni, ade_nop_tot, ade_nk, ade_ksz_tot, ade_est_cxsize)
ADF_CB             *adf_scb;
i4                 ade_ni;
i4                 ade_nop_tot;
i4                 ade_nk;
i4		   ade_ksz_tot;
i4            *ade_est_cxsize;
# endif
{
    i4			isize_mecopy;
    i4			isize_avg_npi;
    float		unalign_pct;
    float		npi_pct;


    *ade_est_cxsize =
	    (i4) sizeof(ADE_CXHEAD)	    /* CX header              */
	+   (ade_ni * sizeof(ADE_INSTRUCTION))	    /* instruction headers    */
	+   (ade_nop_tot * sizeof(ADE_I_OPERAND))   /* instruction operands   */
	+   (ade_nk * sizeof(ADE_CONSTANT))	    /* constant headers       */
	+   ade_ksz_tot				    /* constant data          */
	+   ((ade_ni + ade_nk)			    /* worst case padding for */
		* (sizeof(ALIGN_RESTRICT) - 1));    /*   each instruction and */
						    /*   for each constant    */
#ifdef ADE_BACKEND
    /*
    ** At this point, we have a CX guaranteed to be large enough if there
    ** are no unaligned operands, and if there are no nullable datatypes
    ** that require an extra `nullable pre-instruction' to be compiled.
    */

    /* Add some fudge */
    /* -------------- */
    isize_mecopy  = sizeof(ADE_INSTRUCTION) + (2 * sizeof(ADE_I_OPERAND));
    isize_avg_npi = sizeof(ADE_INSTRUCTION) + (3 * sizeof(ADE_I_OPERAND));
    unalign_pct   = 0.25;
    npi_pct       = ((adf_scb->adf_qlang == DB_SQL) ? 0.50 : 0.20);

    *ade_est_cxsize +=
	    (unalign_pct * ade_nop_tot) * isize_mecopy
	+   (    npi_pct * ade_ni     ) * isize_avg_npi;

#define	MAX_CX_SIZE (16000)	/* Make sure not bigger than ULM CHUNKs */
    if (*ade_est_cxsize > MAX_CX_SIZE)
	*ade_est_cxsize = MAX_CX_SIZE;

    ADE_ROUNDUP_MACRO(*ade_est_cxsize, DB_ALIGN_SZ);
#endif /* ADE_BACKEND */

    return (E_DB_OK);
}


/*{
** Name: ade_bgn_comp() - Begin the compilation of an expression.
**
** Description:
**      This routine is used to begin the compilation of an expression.
**      It takes a contiguous piece of memory and initializes it for this
**      purpose.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      ade_cx                          Pointer to the piece of memory that
**                                      will become a CX.
**      ade_cxsize                      Number of bytes available in the memory
**                                      piece pointed to by ade_cx.
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
**	    E_AD5506_NO_SPACE		Not enough space for the CX header.
**
**	Exceptions:
**	    none
**
** Side Effects:
**          The memory pointed to by ade_cx will be modified by this routine.
**
** History:
**	16-jun-86 (thurston)
**          Initial creation.
**	09-jul-86 (thurston)
**	    Coded.
**	13-may-87 (thurston)
**	    Changed all end of offset list terminaters to be ADE_END_OFFSET_LIST
**	    instead of zero, since zero is a legal instruction offset for the
**	    FE's.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
ade_bgn_comp(
ADF_CB             *adf_scb,
PTR                ade_cx,
i4            ade_cxsize)
# else
DB_STATUS
ade_bgn_comp( adf_scb, ade_cx, ade_cxsize)
ADF_CB             *adf_scb;
PTR                ade_cx;
i4            ade_cxsize;
# endif
{
    ADE_CXHEAD		*cxhead = (ADE_CXHEAD *)ade_cx;

    if (ade_cxsize < sizeof(ADE_CXHEAD))
	return (adu_error(adf_scb, E_AD5506_NO_SPACE, 0));

    /* Now set up the CX hdr.  Note that the actual version */
    /* version # is not set until ade_end_comp() time.      */
    /* This is so an unfinished compile could be detected.  */
    /* ---------------------------------------------------- */
    cxhead->cx_hi_version = ADE_CXINPROGRESS;
    cxhead->cx_allocated = ade_cxsize;
    cxhead->cx_bytes_used = sizeof(ADE_CXHEAD);
    cxhead->cx_seg[ADE_SVIRGIN].seg_1st_offset = ADE_END_OFFSET_LIST;
    cxhead->cx_seg[ADE_SVIRGIN].seg_last_offset = ADE_END_OFFSET_LIST;
    cxhead->cx_seg[ADE_SVIRGIN].seg_n = 0;
    cxhead->cx_seg[ADE_SVIRGIN].seg_flags = 0;
    cxhead->cx_seg[ADE_SINIT].seg_1st_offset = ADE_END_OFFSET_LIST;
    cxhead->cx_seg[ADE_SINIT].seg_last_offset = ADE_END_OFFSET_LIST;
    cxhead->cx_seg[ADE_SINIT].seg_n = 0;
    cxhead->cx_seg[ADE_SINIT].seg_flags = 0;
    cxhead->cx_seg[ADE_SMAIN].seg_1st_offset = ADE_END_OFFSET_LIST;
    cxhead->cx_seg[ADE_SMAIN].seg_last_offset = ADE_END_OFFSET_LIST;
    cxhead->cx_seg[ADE_SMAIN].seg_n = 0;
    cxhead->cx_seg[ADE_SMAIN].seg_flags = 0;
    cxhead->cx_seg[ADE_SFINIT].seg_1st_offset = ADE_END_OFFSET_LIST;
    cxhead->cx_seg[ADE_SFINIT].seg_last_offset = ADE_END_OFFSET_LIST;
    cxhead->cx_seg[ADE_SFINIT].seg_n = 0;
    cxhead->cx_seg[ADE_SFINIT].seg_flags = 0;
    cxhead->cx_k_last_offset = ADE_END_OFFSET_LIST;
    cxhead->cx_n_k = 0;
    cxhead->cx_n_vlts = 0;
    cxhead->cx_hi_base = 0;
    cxhead->cx_alignment = DB_ALIGN_SZ;

    return (E_DB_OK);
}


/*{
** Name: ade_end_comp() - End the compilation of an expression.
**
** Description:
**      This routine will be used to finish the compilation of a CX. 
**      It will do some modifications to the buffer being used, and return 
**      the actual number of bytes of this buffer that have been used.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      ade_cx                          Pointer to the CX.
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
**      ade_cxrsize                     Number of bytes actually used by the CX.
**
**	Returns:
**	    E_DB_OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    The CX will be modified by this routine.
**
** History:
**	16-jun-86 (thurston)
**          Initial creation.
**	09-jul-86 (thurston)
**	    Coded.
**	25-oct-95 (inkdo01)
**	    New logic to remove trailing ADE_AND's from expressions.
**	19-feb-96 (inkdo01)
**	    Fix to bonehead coding error in ADE_AND changes.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
ade_end_comp(
ADF_CB             *adf_scb,
PTR                ade_cx,
i4            *ade_cxrsize)
# else
DB_STATUS
ade_end_comp( adf_scb, ade_cx, ade_cxrsize)
ADF_CB             *adf_scb;
PTR                ade_cx;
i4            *ade_cxrsize;
# endif
{
    ADE_CXHEAD		*cxhead = (ADE_CXHEAD *)ade_cx;


    cxhead->cx_hi_version = ADE_CXHIVERSION;
    cxhead->cx_lo_version = ADE_CXLOVERSION;
#ifdef ADE_BACKEND
    {
	ADE_INSTRUCTION	*iptr;
	i4	next_instr_offset; 
	i4	prev_instr_offset = cxhead->cx_seg[ADE_SMAIN].seg_1st_offset;
	i4	last_instr_offset = cxhead->cx_seg[ADE_SMAIN].seg_last_offset;

	if (last_instr_offset != ADE_END_OFFSET_LIST &&
	      (iptr = (ADE_INSTRUCTION *)((SIZE_TYPE)cxhead + last_instr_offset))
			->ins_icode == ADE_AND &&
			prev_instr_offset != last_instr_offset)
	{
	    if (last_instr_offset + sizeof(ADE_INSTRUCTION) == 
				cxhead->cx_bytes_used)
			cxhead->cx_bytes_used = last_instr_offset;
			/* if last instr in CX, chop it off */
	    for (next_instr_offset = prev_instr_offset;
		next_instr_offset != last_instr_offset; 
		iptr = (ADE_INSTRUCTION *)((SIZE_TYPE)cxhead+next_instr_offset),
		prev_instr_offset = next_instr_offset,
		next_instr_offset = ADE_GET_OFFSET_MACRO(iptr, next_instr_offset,
			last_instr_offset));
			/* Note: null loop - just finds penultimate instr */
	    cxhead->cx_seg[ADE_SMAIN].seg_last_offset = prev_instr_offset;
	    ADE_SET_OFFSET_MACRO(iptr, ADE_END_OFFSET_LIST);
	}
	/* Do post-generation optimizations */
	ad0_post_optim(adf_scb, cxhead, &cxhead->cx_seg[ADE_SVIRGIN]);
	ad0_post_optim(adf_scb, cxhead, &cxhead->cx_seg[ADE_SINIT]);
	ad0_post_optim(adf_scb, cxhead, &cxhead->cx_seg[ADE_SMAIN]);
	ad0_post_optim(adf_scb, cxhead, &cxhead->cx_seg[ADE_SFINIT]);
    }
#endif
    *ade_cxrsize = cxhead->cx_bytes_used;

    return (E_DB_OK);
}

#ifdef ADE_BACKEND

/*{
** Name: ade_global_bases() - Coordinate reassignment of operand bases
**	to global index values.
**
** Description:
**      This function oversees the transformation of a completed CX to
**	replace its operand bases (which are local to the CX) with indexes
**	to a global base array for the query.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      ade_cx                          Pointer to the CX.
**	basemap				Pointer to array of integers that
**					map operand local base indexes to 
**					global indexes.
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
**      ade_cx				Pointer to updated CX.
**
**	Returns:
**	    E_DB_OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    The CX will be modified by this routine.
**
** History:
**	24-aug-04 (inkdo01)
**	    Written.
*/

VOID
ade_global_bases(
ADF_CB             *adf_scb,
PTR                ade_cx,
i4		   *basemap)

{
    ADE_CXHEAD		*cxhead = (ADE_CXHEAD *)ade_cx;
    ADE_CXSEG		*seg;
    i4			i;

    /* Simply call transform function with each defined code segment. */
    /* Note seg numbers start at 1! */
    for (i = 1; i <= ADE_SMAX; ++i)
    {
	seg = &cxhead->cx_seg[i];
	if (seg->seg_n > 0)
	    ad0_base_transform(cxhead, seg->seg_1st_offset,
			seg->seg_last_offset, basemap);
    }

}
#endif /* ADE_BACKEND */

#ifdef ADE_BACKEND

/*{
** Name: ade_const_gen() - Compile a constant into the CX.
**
** Description:
**      This routine will be used to compile a constant into a CX.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      ade_cx                          Pointer to the CX.
**      ade_dv                          Pointer to the DB_DATA_VALUE that holds
**                                      the constant that is to be compiled.
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
**      ade_op                          When compiling instructions that use
**                                      this constant, use ade_op as the
**                                      appropriate operand.
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
**	    E_AD5506_NO_SPACE		Not enough space for the constant
**                                      in the CX.
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**          E_AD2005_BAD_DTLEN          Invalid length for this datatype.
**          E_AD550C_COMP_NOT_IN_PROG   The CX is not in the progress of being
**					compiled.  You should be between calls
**					to ade_bgn_comp() and ade_end_comp().
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    The buffer pointed to by ade_cx will be modified.
**
** History:
**	17-jun-86 (thurston)
**          Initial creation.
**	09-jul-86 (thurston)
**	    Coded.
**	18-jul-89 (jrb)
**	    Added support for precision field in ade_constant structure.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
ade_const_gen(
ADF_CB             *adf_scb,
PTR                ade_cx,
DB_DATA_VALUE      *ade_dv,
ADE_OPERAND        *ade_op)
# else
DB_STATUS
ade_const_gen( adf_scb, ade_cx, ade_dv, ade_op)
ADF_CB             *adf_scb;
PTR                ade_cx;
DB_DATA_VALUE      *ade_dv;
ADE_OPERAND        *ade_op;
# endif
{
    ADE_CXHEAD		*cxhead = (ADE_CXHEAD *)ade_cx;
    i4             needed;
    DB_STATUS           db_stat;
    ADE_CONSTANT        *kptr;


    /* Make sure CX is in progress of being compiled */
    /* --------------------------------------------- */
    if (cxhead->cx_hi_version != ADE_CXINPROGRESS)
	return (adu_error(adf_scb, E_AD550C_COMP_NOT_IN_PROG, 2,
                                (i4) sizeof(SystemProductName),
                                (i4 *) &SystemProductName));

    /* Check datatype and length of constant for validity */
    /* -------------------------------------------------- */
    if (db_stat = adc_lenchk(adf_scb, FALSE, ade_dv, NULL))
	return (db_stat);

    /* Check to make sure we have enough space for the constant in the CX */
    /* ------------------------------------------------------------------ */
    needed = sizeof(ADE_CONSTANT) + ade_dv->db_length;
    ADE_ROUNDUP_MACRO(needed, DB_ALIGN_SZ);
    if ((cxhead->cx_allocated - cxhead->cx_bytes_used) < needed)
	return (adu_error(adf_scb, E_AD5506_NO_SPACE, 0));

    /* Now set up the constant */
    /* ------------------------*/
    kptr = (ADE_CONSTANT *) ((char *) ade_cx + cxhead->cx_bytes_used);
    kptr->con_offset_next = cxhead->cx_k_last_offset;
    kptr->con_prec = ade_dv->db_prec;
    kptr->con_len = ade_dv->db_length;
    kptr->con_collID = ade_dv->db_collID;
    kptr->con_dt = ade_dv->db_datatype;
    MEcopy(ade_dv->db_data, ade_dv->db_length, (PTR)(++kptr));

    /* Now update the CX hdr */
    /* --------------------- */
    cxhead->cx_k_last_offset = cxhead->cx_bytes_used;
    cxhead->cx_bytes_used += needed;
    cxhead->cx_n_k++;

    /* Now set up the operand for the caller */
    /* ------------------------------------- */
    ade_op->opr_prec = ade_dv->db_prec;
    ade_op->opr_len = ade_dv->db_length;
    ade_op->opr_dt = ade_dv->db_datatype;
    ade_op->opr_collID = ade_dv->db_collID;
    ade_op->opr_base = ADE_CXBASE;
    ade_op->opr_offset = cxhead->cx_k_last_offset + sizeof(ADE_CONSTANT);

    return (E_DB_OK);
}
#endif /* ADE_BACKEND */
#ifdef ADE_BACKEND

/*{
** Name: ade_lab_resolve - Resolve a chain of label operands.
**
** Description:
**      This routine will run a chain of label operands, changing all
**	their respective offsets to address the current location in the CX.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      ade_cx                          Pointer to the CX.
**	ade_labop			Pointer to first label operand in chain.
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
**
**	Returns:
**	    E_DB_OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    The CX will be modified by this routine.
**
** History:
**	26-mar-97 (inkdo01)
**          Initial creation.
*/

VOID
ade_lab_resolve(
ADF_CB             *adf_scb,
PTR                ade_cx,
ADE_OPERAND	   *ade_labop)
{
    ADE_CXHEAD	*cxhead = (ADE_CXHEAD *)ade_cx;
    ADE_OPERAND	*tmpopr = ade_labop;
    i4		nextoff = ade_labop->opr_offset;

    while (nextoff)
    {
	tmpopr = (ADE_OPERAND *)((char *)ade_cx + nextoff);
	nextoff = tmpopr->opr_offset;
	tmpopr->opr_offset = cxhead->cx_bytes_used;
    }

}

/*{
** Name: ade_verify_labs() - Verify branch label offsets in the CX.
**
** Description:
**      This routine locates branch instructions in a CX and confirms
**	that their label operands correctly address instructions in the 
**	same segment of the CX. This post verification is necessary 
**	because label resolution occurs before the following instruction
**	is compiled, and an ADF CX mixes data and instructions from all
**	segments in the CX, and there is no guarantee that an instruction
**	from the same segment will appear at the branch address.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      ade_cx                          Pointer to the CX.
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
**      ade_cx				A verified and possibly updated 
**					expression.that use
**
**	Returns:
**	    nothing
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    The buffer pointed to by ade_cx will be modified.
**
** History:
**	13-sep-99 (inkdo01)
**	    Written as part of case function implementation
*/

# ifdef ADF_BUILD_WITH_PROTOS
VOID
ade_verify_labs(
ADF_CB             *adf_scb,
PTR                ade_cx)
# else
VOID
ade_verify_labs( adf_scb, ade_cx)
ADF_CB             *adf_scb;
PTR                ade_cx;
# endif
{
    ADE_CXHEAD		*cxhead = (ADE_CXHEAD *)ade_cx;
    ADE_CXSEG		*seg;
    i4			i;

    /* Simply call ade_verlabs with each defined code segment. */
    /* Note seg numbers start at 1! */
    for (i = 1; i <= ADE_SMAX; ++i)
    {
	seg = &cxhead->cx_seg[i];
	if (seg->seg_n > 0 && seg->seg_flags & ADE_CXSEG_LABELS)
	    ade_verlabs((PTR)cxhead, seg->seg_1st_offset, seg->seg_last_offset);
    }
}	/* end of ade_verify_labs */


/*{
** Name: ade_verlabs() - Local function to perform label verification.
**
** Description:
**      This function does the grunt work of label verification. For a
**	given segment of a CX, it loops in search of branch instructions.
**	For each branch, it loops through the segment until it finds an
**	instruction at or following the branch offset. If an instruction 
**	is found in the same segment at the branch offset, the label is
**	o.k. If one isn't found, a constant or instruction from another
**	segment is at the label offset and the label must be updated to
**	the next instruction in the current segment.
**
** Inputs:
**	cxptr				Pointer to the ADE_CXHEAD of the CX.
**	start				Offset in CX of start of current 
**					code segment.
**	end				Offset in CX of end of current code
**					segment.
**
** Outputs:
**      cxptr				Pointer to possibly updated CX.  
**
**	Returns:
**	    nothing
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-sep-99 (inkdo01)
**	    Written as part of case function implementation
**	31-aug-00 (inkdo01)
**	    Refinements to code for ANDL/ORL branching off end of code
**	    (necessitated by bug fix for 92148 - IF parse trees don't
**	    leave hanging "and/or" at end of expression)
*/

static VOID
ade_verlabs(PTR	cxptr,
	i4	start,
	i4	end)

{
    ADE_I_OPERAND	*opnd;
    ADE_INSTRUCTION	*inst1, *inst2;
    i4			proff1, proff2;
    i2			icode;
    bool		pass2;

    /* Compute offset of end-of-segment (not just addr of last
    ** instruction). Label may address end-of-segment if no more
    ** code is to be executed. */

    inst1 = (ADE_INSTRUCTION *)&cxptr[end];
    end += sizeof(ADE_INSTRUCTION) + 
	inst1->ins_nops * sizeof(ADE_OPERAND);


    /* Start search at "start" offset and loop over instructions 
    ** looking for the "branchers" (ANDL, ORL, BRANCH, BRANCHF). */
    for (inst1 = (ADE_INSTRUCTION *)&cxptr[start], proff1 = start;
	inst1->ins_offset_next != -1;
	proff1 = inst1->ins_offset_next, 
	    inst1 = (ADE_INSTRUCTION *)&cxptr[proff1])
     if ((icode = inst1->ins_icode) == ADE_BRANCH ||
	    icode == ADE_BRANCHF || icode == ADE_ANDL || icode == ADE_ORL)
    {
	/* Branching instruction means we have at least one label to
	** verify. "do while" loop keeps us going (if necessary) for both
	** labels of ANDL/ORL. Inner "for" loop follows remaining 
	** instructions of segment looking for branch offset. If offset
	** is NOT found, branch is updated with offset of instruction
	** immediately following original branch offset. This is the 
	** first instruction added to segment after branch was resolved
	** and is the correct branch address. */

	opnd = (ADE_I_OPERAND *)&cxptr[proff1 + sizeof(ADE_INSTRUCTION)];
	pass2 = FALSE;
	do
	{
	    for (inst2 = inst1, proff2 = proff1; 
		inst2->ins_offset_next != -1 && proff2 < opnd->opr_offset;
		proff2 = inst2->ins_offset_next,
			inst2 = (ADE_INSTRUCTION *)&cxptr[proff2]);
				/* just loop 'til we get to branch offset */

	    /* Perform verify check. If offsets match, branch is fine. 
	    ** Otherwise branch should be to next instruction in 
	    ** segment; namely, the one at proff2. */
	    if (opnd->opr_offset != proff2)
	     if (inst2->ins_offset_next != -1 || opnd->opr_offset < proff2)
				opnd->opr_offset = proff2;
	     else opnd->opr_offset = end;

	    /* Now see if we're on 1st pass and this is ANDL/ORL. */
	    if (pass2 || icode == ADE_BRANCH || icode == ADE_BRANCHF)
							pass2 = FALSE;
	    else if (inst1->ins_nops > 1 && (icode == ADE_ANDL || icode == ADE_ORL))
	    {
		pass2 = TRUE;
		opnd = (ADE_I_OPERAND *)&cxptr[proff1 +
			sizeof(ADE_INSTRUCTION) + sizeof(ADE_I_OPERAND)];
	    }
	} while (pass2);

    }	/* end of inst1 for loop */

}    /* end of ade_verlabs */
#endif /* ADE_BACKEND */

/*{
** Name: ade_instr_gen() - Compile an instruction into the CX.
**
** Description:
**      Use this routine when you wish to compile an instruction into the CX.
**      An instruction is either an ADF function instance or one of a list
**      of CX special instructions.  Refer to the ADE Detailed Design
**      Specification for a complete list of the special instructions.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      ade_cx                          Pointer to the CX.
**      ade_icode                       The instruction to be compiled.
**      ade_seg                         Which segment of the CX to generate
**                                      the instruction in.  This will be
**                                      either ADE_SVIRGIN, ADE_SINIT,
**                                      ADE_SMAIN, or ADE_SFINIT.
**      ade_nops                        The # of operands in the following
**                                      array.
**      ade_ops                         An array of ADE_OPERANDs, one for each
**			                operand the instruction requires.
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
**	ade_uses_qconst			If caller passes a non-null &i4 in,
**					we'll return TRUE if any instruction
**					was generated that potentially uses
**					query constants.  (If no such instr
**					was generated this time, the value
**					in *ade_uses_qconst is untouched.)
**      ade_unaligned                   If the routine returns an error stating
**                                      that there was an operand that needed
**                                      alignment, then this will hold the
**                                      offset into the ade_ops array of the
**                                      operand in question.  Otherwise, this
**                                      will be ignored.
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
**	    E_AD2004_BAD_DTID		Unknown datatype ID.
**	    E_AD2005_BAD_DTLEN		Illegal length for an operand.
**	    E_AD3002_BAD_KEYOP		Unknown key operator for keybld.
**	    E_AD5500_BAD_SEG		Unknown CX segment.
**	    E_AD5501_BAD_SEG_FOR_ICODE	This instruction supplied cannot be
**					compiled into this segment.
**	    E_AD5502_WRONG_NUM_OPRS	Wrong # operands for this instruction.
**	    E_AD5503_BAD_DTID_FOR_FIID	Wrong datatype for this func. instance
**					instruction.
**	    E_AD5504_BAD_RESULT_LEN	Wrong result length supplied.
**	    E_AD5505_UNALIGNED		One of the operands is not properly
**					aligned.  The culprit's index in the
**					operands' array is given by the output
**					parameter ade_unaligned.
**	    E_AD5506_NO_SPACE		Not enough space in the CX.
**	    E_AD5507_BAD_DTID_FOR_KEYBLD    The output operand's datatype for
**					ADE_KEYBLD must be "i".
**	    E_AD5508_BAD_DTLEN_FOR_KEYBLD   The output operand's length for
**					ADE_KEYBLD must be 2.
**	    E_AD5509_BAD_RANGEKEY_FLAG	Unknown range key flag.
**          E_AD550C_COMP_NOT_IN_PROG   The CX is not in the progress of being
**					compiled.  You should be between calls
**					to ade_bgn_comp() and ade_end_comp().
**          E_AD550E_TOO_MANY_VLTS      Max number of VLTs in a CX has been
**                                      exceeded.
**	    E_AD5510_BAD_DTID_FOR_ESCAPE    The 1st operand for ADE_ESC_CHAR
**					must be "char" or "nchar".
**	    E_AD5511_BAD_DTLEN_FOR_ESCAPE   The 1st operand's length for
**					ADE_ESC_CHAR must be 1 character.
**	    E_AD5512_BAD_DTID2_FOR_ESCAPE   Any 2nd operand for ADE_ESC_CHAR
**					must be "int"
**	    E_AD5513_BAD_DTLEN2_FOR_ESCAPE  Any 2nd operand's length to
**					ADE_ESC_CHAR must be 2 bytes.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    The buffer pointed to by ade_cx will be modified.
**
** History:
**	17-jun-86 (thurston)
**          Initial creation.
**	10-jul-86 (thurston)
**	    Coded.
**	29-aug-86 (daved)
**	    Make changes to the way types are handled for aggregate processing.
**	    The first param is the ag_struct and doens't need type/len info.
**	    Therefore, don't check it. 
**	    The setup to the calclen call was changed so that dv1 was filled
**	    in if there were at least two args (the first one being the result)
**	    and dv2 was filled in if there were at least 3 args.
**	    The error checking on unaligned data will now cause a break in
**	    program execution. BAD_DTID will be returned if any of the datatypes
**	    don't match what is expected. Formerly, all datatypes had to
**	    mismatch. I assume that the semantics were that all types had to
**	    match.
**	02-sep-86 (thurston)
**	    Removed the restriction that the ADE_KEYBLD instruction be put in
**	    the INIT segment of a CX.
**	11-sep-86 (thurston)
**	    Added code to allow the compilation of the ADE_COMPARE instruction.
**	13-oct-86 (thurston)
**	    Fixed bug that caused the erroneous error E_AD5503_BAD_DTID_FOR_FIID
**	    to be reported when attempting to compile function instances that
**	    used less than two input operands.  Also fixed a bug with compiling
**	    the ADE_COMPARE special CX instruction.
**	21-oct-86 (thurston)
**	    Fixed another bug that would not allow OPC to compile a coercion
**	    that used the ADI_RESLEN length specification code, and also, would
**	    not allow OPC to "optimize" certain two coercion sequences into one.
**	    Example:  Instead of coercing a "text(8)" into a "c8" then coercing
**	    the "c8" into a "c4", OPC wants to be able to simple coerce the
**	    "text(8)" directly into a "c4" ... sounds reasonable.
**	15-jan-87 (thurston)
**	    Fixed bug where ade_unaligned was being set instead of
**	    *ade_unaligned.
**	11-mar-87 (thurston)
**	    Allowed the ADE_KEYBLD instruction to use ADI_LIKE_OP.
**	11-mar-87 (thurston)
**	    Added support for nullable datatypes.  This is done by compiling an
**	    optional `nullable pre-instruction' ahead of the requested function
**	    instance if any nullable inputs are found.
**	17-mar-87 (thurston)
**	    Added code to compile the ADE_NAGEND CX instruction, and the
**	    ADE_3CXI_CMP_SET_SKIP instruction.
**	05-apr-87 (thurston)
**	    Added code to handle a nullable output where no nullable inputs are
**	    amidst, and to handle a non-nullable output where nullable inputs do
**	    exist and the nullable-pre-instruction is ADE_1CXI_SET_SKIP.  The
**	    latter should now return an error from ade_execute_cx().
**	21-apr-87 (thurston)
**	    Added the ADE_PMQUEL, ADE_NO_PMQUEL, and ADE_PMFLIPFLOP
**	    instructions.
**	13-may-87 (thurston)
**	    Changed all end of offset list terminaters to be ADE_END_OFFSET_LIST
**	    instead of zero, since zero is a legal instruction offset for the
**	    FE's.
**	07-jun-87 (thurston)
**	    Changed call to adi_calclen() to be a call to adi_0calclen(), tem-
**	    porarily.  Eventually, adi_calclen() will have its interface changed
**	    to accept a DB_DATA_VALUE for the result instead of just returning
**	    the calculated length in an i4.
**	18-jun-87 (thurston)
**	    Fixed the check for `datatype ID is not kosher for the function
**	    instance ID' on aggregates.  The code had assumed that all aggregate
**	    function instances would have exactly one input; count(*) has zero.
**	01-jul-87 (thurston)
**	    Omit result length check for COPY COERCIONs.
**	31-jul-87 (thurston)
**	    Added code to compile the ADE_PMENCODE instruction.
**	03-aug-87 (thurston)
**	    Fix made to code around the adi_0calclen() call, that was causing
**	    OPC to give an `bad instruction gen' error `unknown len'.
**	22-oct-87 (thurston)
**	    Added the `ade_qconst_map' argument, and some dummy code that just
**	    sets all of the bits, for now.
**	26-oct-87 (thurston)
**	    Added code to allow the compilation of the ADE_BYCOMPARE
**	    instruction.
**	10-nov-87 (thurston)
**	    Added bug fix for nullable datatypes with aggregates; Now the
**	    output nullability is called "NOT_APPLICABLE".  Previously, the
**	    datatype of operand #0 was being looked at to determine whether or
**	    not the output would be "NULLABLE" or "NON_NULLABLE".  This was a
**	    mistake since for aggregates operand #0 is the ag-struct, and there
**	    is no datatype associated with it, so this field is undefined.  It
**	    just so happened to work since a zero was always being placed there
**	    by OPC.
**	04-feb-88 (thurston)
**	    Aparently, VLTs have never been tested.  This routine was not
**	    incrementing the VLT counter in the CX header.  It does now when it
**	    detects a VLT (by seeing a successfull ADE_CALCLEN instruction).
**	05-feb-88 (thurston)
**	    Did some work here to support the MAJOR work done on ADEEXECUTE.C
**	    for fixing VLTs.  Mostly, this involved allowing the ADE_CALCLEN
**	    instruction to accept an additional operand which represents the
**	    output datatype of the instruction corresponding to the ADE_CALCLEN.
**	10-feb-88 (thurston)
**	    Did away with using the AD_BIT_TYPE for the NPI's since it didn't
**	    work for VLUP's VLT's.  Also, the code no longer erroneously
**	    decrements the operands length if that operand is a VLUP or VLT.
**	10-feb-88 (thurston)
**	    Added in code to deal with the newly created ADE_SVIRGIN segment.
**	09-jan-88 (thurston)
**	    Added in code to handle the newly created ADE_ESC_CHAR instruction.
**	19-feb-89 (paul)
**	    Enhanced to generate new instructions ADE_PNAME and ADE_DBDV.
**      16-may-89 (eric)
**          added support for the new instruction ADE_TMCVT, which is the CX
**          version of ADC_TMCVT.
**	18-jul-89 (jrb)
**	    Set precision fields in db_data_values and ade_operands correctly.
**	    Also, made special case check for ADI_DECFUNC lenspec because we
**	    cannot call calclen for it (because we don't have the db_data field
**	    filled in with the proper info in this routine).
**      5-dec-89 (eric)
**          Removed the check that only calclen instructions can be put into
**          the VIRGIN segment.
**	11-dec-89 (fred)
**	    Added support for large object management.  This primarily entails
**	    the addition of support for the ADE_REDEEM operation/instruction
**	    code.
**      09-mar-92 (jrb, merged by stevet)
**          Changed consistency checking code to allow DB_ALL_TYPE fi's for
**          the outer join project.
**      09-dec-1992 (stevet)
**          Cannot call calclen for ADI_LEN_EXTERN and ADI_LEN_INDIRECT 
**          because we don't have proper db_data filled in.
**      11-Dec-1992 (fred)
**          Added alignment restriction to ADE_REDEEM operation.
**          Since the coupons contain integers, the operation must be
**          aligned.  Also added support for WORKSPACE fiflag -- need
**          another operand.
**	25-oct-95 (inkdo01)
**	    Added logic to collapse consecutive MECOPYs of contiguous 
**	    fields.
**	3-jul-96 (inkdo01)
**	    Added ADE_4CXI_CMP_SET_SKIP support for predicate function 
**	    operation types.
**	26-mar-97 (inkdo01)
**	    Added support for branching AND/OR for non-conjunctive normal
**	    form Boolean expressions.
**	08-Mar-1999 (shero03)
**	    Support functions with up to 4 operands
**	8-sep-99 (inkdo01)
**	    Added support for simple branch operators (for case functions).
**	10-dec-00 (inkdo01)
**	    Added support for ADE_AVG, ADE_NULLCHECK, ADE_MECOPYQA, 
**	    ADE_AVGQ opcodes as part of inline aggregation.
**	9-apr-01 (inkdo01)
**	    Added support for ADE_UNORM (Unicode value normalization).
**	29-june-01 (inkdo01)
**	    Alignment check for Unicode operands.
**	04-jul-2001 (somsa01)
**	    The addition of ADE_UNORM needs to be wrapped inside the
**	    "#ifndef ADE_FRONTEND" block.
**	30-aug-01 (inkdo01)
**	    Fix aggregate operand check to accomodate FRS doing it 
**	    the old way.
**	17-sep-01 (inkdo01)
**	    Minor fix to above for binary aggregates.
**	25-oct-01 (inkdo01)
**	    Add CXI_SET_SKIP into AGNEXT.
**	09-Nov-2004 (jenjo02)
**	    Enhanced Doug's MECOPY optimization.
**	2-Dec-2004 (schka24)
**	    Bugs in above, fix:  forgot to update both offsets, deal with
**	    situation where code from other segments is interspersed,
**	    back off the cx end when scrapping instrs at the end, and
**	    don't coalesce beyond a branch label.
**	9-Dec-2004 (schka24)
**	    Two more fixes to the above, keep # of instrs in sync, and use
**	    the instruction chain to back up dptr.
**	24-Mar-2005 (schka24)
**	    Change qconst mask to true/false (will get rid of it later).
**	28-Mar-2005 (schka24)
**	    Translate 1CXI, 4CXI, and 5CXI into clear-result-null 6CXI
**	    or check-inputs-only 7CXI, when appropriate.  This will aid
**	    later STCODE generation as well.
**	25-Jul-2005 (schka24)
**	    Add ADE_SETTRUE as a zero operand instruction.
**	29-Aug-2005 (schka24)
**	    Turn on labels flag when branches seen;  turn on "fancy" flag
**	    when blob operands, unknown-length operands, or CALCLEN instr
**	    is seen.
**	3-feb-06 (dougi)
**	    Fix bad loop termination processing instruction list.
**	7-Apr-2006 (kschendel)
**	    Allow for nullable FI result datatypes.
**	25-Apr-2006 (kschendel)
**	    Turns out that we need countvec lenspecs to really support
**	    multi-arg UDF's, fix here.
**	1-may-06 (dougi)
**	    Tolerate data type families in FI parameters.
**	4-oct-2006 (dougi)
**	    Fix above change to avoid dtfamily checks on UDTs.
**	11-oct-2006 (gupsh01)
**	    Expand the datatype family checking logic for all cases.
**	22-Feb-2007 (kschendel) SIR 122890
**	    The MOVE-combining phase was taking huge amounts of time
**	    (minutes) for really gigantic CX's, because of the lack of
**	    a backwards pointer.  Remember a starting position so that
**	    backwards moves don't have to skip from the very beginning
**	    of the CX.  (i.e. keep it quadratic in the number of
**	    adjacent moves, not quadratic in the size of the CX.)
**	17-sep-07 (hayke02)
**	    We now break out of the 'while (--j > 0)' loop if ins_offset_next ==
**	    ADE_END_OFFSET_LIST. This prevents a garbage pptr. This change
**	    fixes bug 118962.
**	25-Jun-2008 (kiria01) SIR120473
**	    Extended ADE_ESC_CHAR instruction to include short operand
**	    fro LIKE modifier support.
**	12-Oct-2008 (kiria01) b121011
**	    Add ADE_SETFALSE as a zero operand instruction.
**	22-Jan-2009 (kibro01) b120461
**	    Add ADE_MECOPY_MANY to turn off attempt to optimise adjacent
**	    MECOPY instructions if many will be produced in one go.
**	29-Jun-2009 (kibro01) b122225
**	    For statistical functions, it is necessary to execute the FINIT
**	    code irrespective of the nullness of the operatores, since the
**	    FINIT code in each case is written to calculate what to do with
**	    nothing.  The npi in those cases is for the iterative code.
**	01-Nov-2009 (kiria01) b122822
**	    Added support for ADE_COMPAREN & ADE_NCOMPAREN instructions
**	    for big IN lists.
**	18-Mar-2010 (kiria01) b123438
**	    Added SINGLETON aggregate for scalar sub-query support.
**	14-Jul-2010 (kschendel) b123104
**	    Settrue/false no longer special cases.
*/

DB_STATUS
ade_instr_gen(
ADF_CB             *adf_scb,
PTR                ade_cx,
i4                 ade_icode,
i4                 ade_seg,
i4                 ade_nops,
ADE_OPERAND        ade_ops[],
i4                 *ade_uses_qconst,
i4                 *ade_unaligned)
{
    ADE_CXHEAD		*cxhead = (ADE_CXHEAD *)ade_cx;
    ADE_CXSEG		*cxseg;
    DB_STATUS           db_stat = E_DB_OK;
    i4             status = E_AD0000_OK;
    i4             needed;
    i4                  npi = ADE_0CXI_IGNORE;
    i4			npi_input_ops;
    i4			npi_size;
    i4                  i,j;
    i4                  exp_nops;
    i4                  rangekey_flag;
    i4                  rlen;
    DB_DATA_VALUE	dvr;
    DB_DATA_VALUE	dv[ADI_MAX_OPERANDS];
    ADI_OP_ID           keyop;
    ADI_FI_DESC         *fdesc = NULL;
    ADE_INSTRUCTION     *iptr;
    ADE_I_OPERAND       *optr;
    ADE_OPERAND		tmp_opr;
    i4                  new_instr_offset;
    i4                  prev_instr_offset;
    i4			out_nullability;
    bool		optimise_mecopy = TRUE;
    bool		add_npi;

#define	    AD_USE_ALIGN_RESTRICT 0 /* use DB_ALIGN_SZ for check */
#define     AD_USE_DTLEN          1 /* use datatype and length for check */

#define	    AD_NOT_APPLICABLE   (-1) /* output nullability is not applicable */
#define	    AD_NON_NULLABLE       0  /* output is non-nullabe */
#define	    AD_NULLABLE           1  /* output is nullabe */

    if (ade_icode == ADE_MECOPY_MANY)
    {
	ade_icode = ADE_MECOPY;
	optimise_mecopy = FALSE;
    }

    /* Make sure CX is in progress of being compiled */
    /* --------------------------------------------- */
    if (cxhead->cx_hi_version != ADE_CXINPROGRESS)
	return (adu_error(adf_scb, E_AD550C_COMP_NOT_IN_PROG, 2,
                                (i4) sizeof(SystemProductName),
                                (i4 *) &SystemProductName));

    /* Check for invalid segment */
    /* ------------------------- */
    if (    ade_seg != ADE_SVIRGIN
	&&  ade_seg != ADE_SINIT
	&&  ade_seg != ADE_SMAIN
	&&  ade_seg != ADE_SFINIT
       )
	return (adu_error(adf_scb, E_AD5500_BAD_SEG, 0));

    cxseg = &cxhead->cx_seg[ade_seg];

    /* Validity checks for instruction code, # of     */
    /* operands, datatypes and lengths, and alignment */
    /* ---------------------------------------------- */
    status = E_AD0000_OK;
    switch (ade_icode)
    {
	case ADE_AND:
	case ADE_OR:
	case ADE_NOT:
	case ADE_PMQUEL:
	case ADE_NO_PMQUEL:
	case ADE_PMFLIPFLOP:
	    if (ade_nops != 0)
		status = E_AD5502_WRONG_NUM_OPRS;
	    break;

	case ADE_ANDL:
	case ADE_ORL:
	case ADE_BRANCH:
	case ADE_BRANCHF:
	    if (ade_nops != 1)
		status = E_AD5502_WRONG_NUM_OPRS;
	    /* These are the branching operators, signal that we have
	    ** labels in this segment
	    */
	    cxseg->seg_flags |= ADE_CXSEG_LABELS;
	    break;

	case ADE_ESC_CHAR:
	    switch (ade_nops)
	    {
	    default:
		status = E_AD5502_WRONG_NUM_OPRS;
		break;
	    case 2:
		if (ade_ops[1].opr_dt != DB_INT_TYPE)
		    status = E_AD5512_BAD_DTID2_FOR_ESCAPE;
		else if (ade_ops[1].opr_len != 2)
		    status = E_AD5513_BAD_DTLEN2_FOR_ESCAPE;
		/*FALLTHROUGH*/
	    case 1:
		if (ade_ops[0].opr_dt != DB_CHA_TYPE &&
		    ade_ops[0].opr_dt != DB_NCHR_TYPE)
		    status = E_AD5510_BAD_DTID_FOR_ESCAPE;
		else if (ade_ops[0].opr_len > 2)
		    status = E_AD5511_BAD_DTLEN_FOR_ESCAPE;
	    }
	    break;

	case ADE_PMENCODE:
	    if (ade_nops != 1)
		status = E_AD5502_WRONG_NUM_OPRS;
	    else if (db_stat = ad0_dtlenchk(adf_scb, &ade_ops[0]))
		return (db_stat);	/* Checks for datatype and length */
#ifdef ADE_BACKEND
	    else if (ad0_is_unaligned(&ade_ops[0], AD_USE_DTLEN))
	    {
		status = E_AD5505_UNALIGNED;
		*ade_unaligned = 0;
		break;
	    }
#endif /* ADE_BACKEND */
	    break;

#ifdef ADE_BACKEND
	case ADE_AGBGN:
	case ADE_OLAGBGN:
	    if (ade_nops != 1)
		status = E_AD5502_WRONG_NUM_OPRS;
	    else if (ade_seg != ADE_SINIT)
		status = E_AD5501_BAD_SEG_FOR_ICODE;
	    else if (ad0_is_unaligned(&ade_ops[0], AD_USE_ALIGN_RESTRICT))
	    {
		status = E_AD5505_UNALIGNED;
		*ade_unaligned = 0;
	    }
	    else
	    {
		ade_ops[0].opr_len = 0;	/* unused, must not be ADE_LEN_UNKNOWN*/
	    }
	    break;

	case ADE_AGNEXT:
	    /* With new agg code scheme for backend, this replaces all agg f.i.'s
	    ** for FRS (which still does it the old fashioned way). 
	    ** NOTE: must also do a null detect first. */
	    npi = ADE_2CXI_SKIP;
	    if (ade_nops != 2)
		status = E_AD5502_WRONG_NUM_OPRS;
	    else if (ade_seg != ADE_SMAIN)
		status = E_AD5501_BAD_SEG_FOR_ICODE;
	    else if (db_stat = ad0_dtlenchk(adf_scb, &ade_ops[1]))
		return (db_stat);	/* Checks for datatype and length */
	    else if (ad0_is_unaligned(&ade_ops[1], AD_USE_DTLEN))
	    {
		status = E_AD5505_UNALIGNED;
		*ade_unaligned = 1;
	    }
	    else if (ad0_is_unaligned(&ade_ops[0], AD_USE_ALIGN_RESTRICT))
	    {
		status = E_AD5505_UNALIGNED;
		*ade_unaligned = 0;
	    }
	    else
	    {
		ade_ops[0].opr_len = 0;	/* unused, must not be ADE_LEN_UNKNOWN*/
	    }
	    break;
	
	case ADE_AGEND:
	case ADE_OLAGEND:
	case ADE_NAGEND:
	    if (ade_nops != 2 && ade_icode != ADE_OLAGEND)
		status = E_AD5502_WRONG_NUM_OPRS;
	    else if (ade_icode == ADE_OLAGEND && ade_nops != 3)
		status = E_AD5502_WRONG_NUM_OPRS;
	    else if (ade_seg != ADE_SFINIT)
		status = E_AD5501_BAD_SEG_FOR_ICODE;
	    else if (db_stat = ad0_dtlenchk(adf_scb, &ade_ops[0]))
		return (db_stat);	/* Checks for datatype and length */
	    else if (ad0_is_unaligned(&ade_ops[0], AD_USE_DTLEN))
	    {
		status = E_AD5505_UNALIGNED;
		*ade_unaligned = 0;
	    }
	    else if (ad0_is_unaligned(&ade_ops[1], AD_USE_ALIGN_RESTRICT))
	    {
		status = E_AD5505_UNALIGNED;
		*ade_unaligned = 1;
	    }
	    else
	    {
		ade_ops[1].opr_len = 0;	/* unused, must not be ADE_LEN_UNKNOWN*/
	    }
	    break;
	
	case ADE_AVG:
	case ADE_AVGQ:
	case ADE_SINGLETON:
	    if (ad0_is_unaligned(&ade_ops[0], AD_USE_ALIGN_RESTRICT))
	    {
		status = E_AD5505_UNALIGNED;
		*ade_unaligned = 0;
	    }
	    if (ade_nops != 3)
		status = E_AD5502_WRONG_NUM_OPRS;
	    break;

	case ADE_UNORM:
	    npi = ADE_1CXI_SET_SKIP;
	    if (ad0_is_unaligned(&ade_ops[0], AD_USE_DTLEN))
	    {
		status = E_AD5505_UNALIGNED;
		*ade_unaligned = 0;
		break;
	    }
	    if (ad0_is_unaligned(&ade_ops[1], AD_USE_DTLEN))
	    {
		status = E_AD5505_UNALIGNED;
		*ade_unaligned = 1;
		break;
	    }
#endif /* ADE_BACKEND */

	case ADE_MECOPY:
	case ADE_MECOPYN:
	case ADE_MECOPYQA:
	case ADE_PNAME:
	case ADE_DBDV:
	case ADE_NULLCHECK:
	    if (ade_icode == ADE_MECOPYN) npi = ADE_1CXI_SET_SKIP;
	    else if (ade_icode == ADE_MECOPYQA) npi = ADE_2CXI_SKIP;
	    if (ade_nops != 2)
		status = E_AD5502_WRONG_NUM_OPRS;
	    break;

	case ADE_REDEEM:
	    if (ade_nops != 3)
	    {
		status = E_AD5502_WRONG_NUM_OPRS;
	    }
	    else if (ade_ops[0].opr_len != ADE_LEN_LONG)
	    {
		status = E_AD5504_BAD_RESULT_LEN;
	    }
#ifdef ADE_BACKEND
	    else if (ad0_is_unaligned(&ade_ops[1], AD_USE_ALIGN_RESTRICT))
	    {
		status = E_AD5505_UNALIGNED;
		*ade_unaligned = 1;
		break;
	    }
#endif /* ADE_BACKEND */
	    break;

#ifdef ADE_BACKEND
	case ADE_KEYBLD:
	    if (ade_nops != 5)
		status = E_AD5502_WRONG_NUM_OPRS;
	    else if (ade_ops[0].opr_dt != DB_INT_TYPE)
		status = E_AD5507_BAD_DTID_FOR_KEYBLD;
	    else if (ade_ops[0].opr_len != 2)
		status = E_AD5508_BAD_DTLEN_FOR_KEYBLD;
	    else if (ad0_is_unaligned(&ade_ops[0], AD_USE_DTLEN))
	    {
		status = E_AD5505_UNALIGNED;
		*ade_unaligned = 0;
	    }
	    else if (db_stat = ad0_dtlenchk(adf_scb, &ade_ops[1]))
		return (db_stat);	/* Checks for datatype and length */
	    else if (ad0_is_unaligned(&ade_ops[1], AD_USE_DTLEN))
	    {
		status = E_AD5505_UNALIGNED;
		*ade_unaligned = 1;
	    }
	    else if (db_stat = ad0_dtlenchk(adf_scb, &ade_ops[2]))
		return (db_stat);	/* Checks for datatype and length */
	    else if (ad0_is_unaligned(&ade_ops[2], AD_USE_DTLEN))
	    {
		status = E_AD5505_UNALIGNED;
		*ade_unaligned = 2;
	    }
	    else if ((keyop=(ADI_OP_ID)ade_ops[4].opr_dt) != ADI_LT_OP
						&&  keyop != ADI_LE_OP
						&&  keyop != ADI_EQ_OP
						&&  keyop != ADI_GE_OP
						&&  keyop != ADI_GT_OP
						&&  keyop != ADI_NE_OP
						&&  keyop != ADI_NLIKE_OP
						&&  keyop != ADI_LIKE_OP
						&&  keyop != ADI_ISNUL_OP
						&&  keyop != ADI_NONUL_OP
						&&  keyop != ADI_NULJN_OP
		    )
		status = E_AD3002_BAD_KEYOP;
	    else if (	(keyop != ADI_ISNUL_OP && keyop != ADI_NONUL_OP)
		     &&	(db_stat = ad0_dtlenchk(adf_scb, &ade_ops[3]))
		    )
		return (db_stat);	/* Checks for datatype and length */
	    else if (	(keyop != ADI_ISNUL_OP && keyop != ADI_NONUL_OP)
		     &&	(ad0_is_unaligned(&ade_ops[3], AD_USE_DTLEN))
		    )
	    {
		status = E_AD5505_UNALIGNED;
		*ade_unaligned = 3;
	    }
	    else if ((rangekey_flag=ade_ops[4].opr_len) != ADE_2RANGE_YES
				      &&  rangekey_flag != ADE_3RANGE_NO
				      &&  rangekey_flag != ADE_1RANGE_DONTCARE
		    )
		status = E_AD5509_BAD_RANGEKEY_FLAG;
	    break;

	case ADE_CALCLEN:
	    if (cxhead->cx_n_vlts >= ADE_MXVLTS)
		status = E_AD550E_TOO_MANY_VLTS;
	    else if (ade_nops < 2)
		status = E_AD5502_WRONG_NUM_OPRS;
	    else if (ade_seg != ADE_SVIRGIN)
		status = E_AD5501_BAD_SEG_FOR_ICODE;
	    else
		for (i=1; i<ade_nops; i++)
		{
		    if (db_stat = ad0_dtlenchk(adf_scb, &ade_ops[i]))
			return (db_stat);    /* Checks for datatype and length */
		    else if (ad0_is_unaligned(&ade_ops[i], AD_USE_DTLEN))
		    {
			status = E_AD5505_UNALIGNED;
			*ade_unaligned = i;
		    }
		}
	    cxseg->seg_flags |= ADE_CXSEG_FANCY;
	    break;

	case ADE_COMPAREN:
	case ADE_NCOMPAREN:
	    if (ade_nops < 2)
		status = E_AD5502_WRONG_NUM_OPRS;
	    else if (db_stat = ad0_dtlenchk(adf_scb, &ade_ops[0]))
		return (db_stat);	 /* Checks for datatype and length */
	    else
	    {
		for (i=0; i<ade_nops; i++)
		{
		    if (db_stat = ad0_dtlenchk(adf_scb, &ade_ops[i]))
			return (db_stat);	/* Checks for datatype and length */
		    else if (ad0_is_unaligned(&ade_ops[i], AD_USE_DTLEN))
		    {
			status = E_AD5505_UNALIGNED;
			*ade_unaligned = i;
		    }
		}
	    }
	    break;
	case ADE_COMPARE:
	case ADE_BYCOMPARE:
	    if (ade_nops != 2)
		status = E_AD5502_WRONG_NUM_OPRS;
	    else if (db_stat = ad0_dtlenchk(adf_scb, &ade_ops[0]))
		return (db_stat);    /* Checks for datatype and length */
	    else if (ad0_is_unaligned(&ade_ops[0], AD_USE_DTLEN))
	    {
		status = E_AD5505_UNALIGNED;
		*ade_unaligned = 0;
	    }
	    else if (db_stat = ad0_dtlenchk(adf_scb, &ade_ops[1]))
		return (db_stat);    /* Checks for datatype and length */
	    else if (ad0_is_unaligned(&ade_ops[1], AD_USE_DTLEN))
	    {
		status = E_AD5505_UNALIGNED;
		*ade_unaligned = 1;
	    }
	    break;
#endif /* ADE_BACKEND */

	case ADE_TMCVT:
	    if (ade_nops != 2)
	    {
		status = E_AD5502_WRONG_NUM_OPRS;
	    }
	    else if (db_stat = ad0_dtlenchk(adf_scb, &ade_ops[1]))
	    {
		return(db_stat);
	    }
#ifdef ADE_BACKEND
	    else if (ad0_is_unaligned(&ade_ops[1], AD_USE_ALIGN_RESTRICT))
	    {
		status = E_AD5505_UNALIGNED;
		*ade_unaligned = 1;
		break;
	    }
#endif /* ADE_BACKEND */
	    break;


	default:    /* Not a CX special instruction ... is it a f.i. ID? */
	    if (db_stat = adi_fidesc(adf_scb, (ADI_FI_ID)ade_icode, &fdesc))
		return (db_stat);    /* If not a legal instruction code,
				    ** this will catch it, and return.
				    */
	    npi = fdesc->adi_npre_instr;

	    /* to break out of on error */
	    for (;;)
	    {
		exp_nops = fdesc->adi_numargs;
		if (fdesc->adi_fitype != ADI_COMPARISON)
		    exp_nops++;
		if (fdesc->adi_fiflags & ADI_F4_WORKSPACE)
		    exp_nops++;

		if (fdesc->adi_fiflags & ADI_F128_VARARGS)
		{
		    if (ade_nops < exp_nops)
		    {
			status = E_AD5502_WRONG_NUM_OPRS;
			break;
		    }
		}
		else
		{
		    if (ade_nops != exp_nops && fdesc->adi_fitype != ADI_AGG_FUNC)
		    {
			status = E_AD5502_WRONG_NUM_OPRS;
			break;
		    }
		}

		/* All aggregates used to pass ag-struct as 1st operand
		** and the datatype/length check was skipped. Now, backend
		** compiled aggs don't use the ag-struct and do check 1st
		** operand. */

		i = 0;

		for (; i < ade_nops; i++)
		{
		    if (   (fdesc->adi_fiflags & ADI_F4_WORKSPACE)
			&& (i == (ade_nops - 1)))
		    {
			continue;
		    }
			
		    if (db_stat = ad0_dtlenchk(adf_scb, &ade_ops[i]))
			return (db_stat);   /* Checks for datatype and length */
#ifdef ADE_BACKEND
		    else if (ad0_is_unaligned(&ade_ops[i], AD_USE_DTLEN))
		    {
			status = E_AD5505_UNALIGNED;
			*ade_unaligned = i;
			break;
		    }
#endif /* ADE_BACKEND */
		}
		/* if we had an unaligned data value ... break */
		if (status != E_DB_OK)
		    break;

		if (fdesc->adi_fitype == ADI_COMPARISON)
		{
		    if (    (ade_nops > 0
				&&  abs(ade_ops[0].opr_dt) != fdesc->adi_dt[0]
				&&  fdesc->adi_dt[0] != DB_ALL_TYPE
				&&  Adf_globs->Adi_dtptrs[abs(ade_ops[0].opr_dt)]->adi_dtfamily 
				    != fdesc->adi_dt[0]
				&& (abs(ade_ops[0].opr_dt) >= ADI_DT_CLSOBJ_MIN))
			||
			    (ade_nops > 1
				&&  abs(ade_ops[1].opr_dt) != fdesc->adi_dt[1]
				&&  fdesc->adi_dt[1] != DB_ALL_TYPE
				&&  Adf_globs->Adi_dtptrs[abs(ade_ops[1].opr_dt)]->adi_dtfamily 
				    != fdesc->adi_dt[1]
				&& (abs(ade_ops[1].opr_dt) < ADI_DT_CLSOBJ_MIN))
		       )
		    {
			status = E_AD5503_BAD_DTID_FOR_FIID;
			break;
		    }
		    /* Skip over all the result length calculation
		    ** stuff on comparison.
		    */
		    break;
		}
#ifdef ADE_BACKEND
		else if (fdesc->adi_fitype == ADI_AGG_FUNC)
		{
		    /* Aggs are screwy, because OPC compiles in extra
		    ** operands.  The operand count in the FI descriptor
		    ** is for parsing, not code generating.
		    ** Don't type-check operands past the actual func
		    ** parameter operand.
		    */
		    if (ade_nops <= fdesc->adi_numargs+1
		      && ( (ade_nops > 1
				&&  abs(ade_ops[1].opr_dt) != fdesc->adi_dt[0]
				&&  fdesc->adi_dt[0] != DB_ALL_TYPE
				&& (abs(ade_ops[1].opr_dt) >= ADI_DT_CLSOBJ_MIN)
				&&  Adf_globs->Adi_dtptrs[abs(ade_ops[1].opr_dt)]->adi_dtfamily 
					!= fdesc->adi_dt[0])
			||
			    (ade_nops > 2
				&&  abs(ade_ops[2].opr_dt) != fdesc->adi_dt[1]
				&&  fdesc->adi_dt[1] != DB_ALL_TYPE
				&& (abs(ade_ops[2].opr_dt) >= ADI_DT_CLSOBJ_MIN)
				&&  Adf_globs->Adi_dtptrs[abs(ade_ops[2].opr_dt)]->adi_dtfamily 
					!= fdesc->adi_dt[1])
		       ) )
		    {
			status = E_AD5503_BAD_DTID_FOR_FIID;
			break;
		    }
		    /* Skip over all the result length calculation
		    ** stuff on aggregates.
		    */
		    break;
		}
#endif /* ADE_BACKEND */
		else
		{
		    /* While this is wrong for many-param UDF's, this check
		    ** ought to be just a consistency check, so I'm leaving
		    ** it as-is.
		    */
		    if (    (abs(ade_ops[0].opr_dt) != abs(fdesc->adi_dtresult)
				&&  fdesc->adi_dtresult != DB_ALL_TYPE
				&& (abs(ade_ops[0].opr_dt) >= ADI_DT_CLSOBJ_MIN)
				&&  Adf_globs->Adi_dtptrs[abs(ade_ops[0].opr_dt)]->adi_dtfamily 
					!= abs(fdesc->adi_dtresult))
			|| (ade_nops > 1 && fdesc->adi_numargs > 0
			        &&  abs(ade_ops[1].opr_dt) != fdesc->adi_dt[0]
				&&  fdesc->adi_dt[0] != DB_ALL_TYPE
				&& (abs(ade_ops[1].opr_dt) >= ADI_DT_CLSOBJ_MIN)
				&&  Adf_globs->Adi_dtptrs[abs(ade_ops[1].opr_dt)]->adi_dtfamily 
					!= fdesc->adi_dt[0])
			|| (ade_nops > 2 && fdesc->adi_numargs > 1
			        &&  abs(ade_ops[2].opr_dt) != fdesc->adi_dt[1]
				&&  fdesc->adi_dt[1] != DB_ALL_TYPE
				&& (abs(ade_ops[2].opr_dt) >= ADI_DT_CLSOBJ_MIN)
				&&  Adf_globs->Adi_dtptrs[abs(ade_ops[2].opr_dt)]->adi_dtfamily 
					!= fdesc->adi_dt[1])
			|| (ade_nops > 3 && fdesc->adi_numargs > 2
			        &&  abs(ade_ops[3].opr_dt) != fdesc->adi_dt[2]
				&&  fdesc->adi_dt[2] != DB_ALL_TYPE
				&& (abs(ade_ops[3].opr_dt) >= ADI_DT_CLSOBJ_MIN)
				&&  Adf_globs->Adi_dtptrs[abs(ade_ops[3].opr_dt)]->adi_dtfamily 
					!= fdesc->adi_dt[2])
			|| (ade_nops > 4 && fdesc->adi_numargs > 3
			        &&  abs(ade_ops[4].opr_dt) != fdesc->adi_dt[3]
				&&  fdesc->adi_dt[3] != DB_ALL_TYPE
				&& (abs(ade_ops[4].opr_dt) >= ADI_DT_CLSOBJ_MIN)
				&&  Adf_globs->Adi_dtptrs[abs(ade_ops[4].opr_dt)]->adi_dtfamily 
					!= fdesc->adi_dt[3])
		       )
		    {
			status = E_AD5503_BAD_DTID_FOR_FIID;
			break;
		    }
		}

		status = E_AD0000_OK;
#ifdef xDEBUG
		/*
		** Let's now do a consistency check to see if the result type
		** we're given matches the one we expect.  We do this for all
		** cases except for certain lenspecs which need to dereference
		** the db_data pointer in order to calculate the result length.
		** Since the caller of this routine will not supply the db_data
		** pointer, we will not call calclen for these lenspecs.
		*/
		if (fdesc->adi_lenspec.adi_lncompute == ADI_DECFUNC
		    || fdesc->adi_lenspec.adi_lncompute == ADI_LEN_INDIRECT
		    || fdesc->adi_lenspec.adi_lncompute == ADI_LEN_EXTERN
		    || fdesc->adi_lenspec.adi_lncompute == ADI_LEN_EXTERN_COUNTVEC)
		    break;

		/*
		** Only check result length for non-coercions, and even
		** then, only if compiled with "xDEBUG".  The former is
		** because of expression evaluation efficiency:  OPC can
		** avoid compiling an extra coercion at a resdom in cases
		** like this: the final attribute is "c4" and the calculated
		** result is "text(8)".  If we allowed this check to occur,
		** OPC would have to first compile a coercion from text(8)
		** into c8 (because of the ADI_O1TC length spec used by that
		** coercion), and then compile a coercion from c8 into c4.
		** (This second is OK, because it uses the ADI_RESLEN length
		** spec.)
		*/

		    
		/* setup to call calclen */
		/* --------------------- */

		/* Operand #0 is always the result for those routines
		** coming through here.
		*/
		if (ade_nops > 0)
		{
		    dvr.db_prec = ade_ops[0].opr_prec;
		    dvr.db_collID = ade_ops[0].opr_collID;
		    dvr.db_length = ade_ops[0].opr_len;
		    dvr.db_datatype = ade_ops[0].opr_dt;
		}
		for (i = 1, j = 0; i < ade_nops; i++, j++)
		{	
		    dv[j].db_prec = ade_ops[i].opr_prec;
		    dv[j].db_collID = ade_ops[i].opr_collID;
		    dv[j].db_length = ade_ops[i].opr_len;
		    dv[j].db_datatype = ade_ops[i].opr_dt;
		}

		/* {@fix_me@} ... the following call to adi_0calclen() should
		**		  changed back to adi_calclen() when that
		**		  routine has its interface upgraded to take a
		**		  DB_DATA_VALUE for the result.
		*/
		if (db_stat = adi_0calclen(adf_scb, &fdesc->adi_lenspec, 
					   ade_nops, dv, &dvr)
		   )
		    status = adf_scb->adf_errcb.ad_errcode;

		rlen = dvr.db_length;	/* returned from calclen */
		if (	status == E_AD2022_UNKNOWN_LEN
		    &&	ade_ops[0].opr_len == ADE_LEN_UNKNOWN
		   )
		{
		    status = E_AD0000_OK;
		    adf_scb->adf_errcb.ad_errcode = E_AD0000_OK;
		}

		if (	DB_SUCCESS_MACRO(status)
		    &&	(   fdesc->adi_fitype != ADI_COERCION
			 && fdesc->adi_fitype != ADI_COPY_COERCION
			 && rlen != ade_ops[0].opr_len
			)
		   )
		    status = E_AD5504_BAD_RESULT_LEN;
		else
		    status = E_AD0000_OK;
#endif /* xDEBUG */
		    
		/* get out of the 'for' loop */
		break;
	    }
	    break;
    }		    /* END OF SWITCH STATEMENT */

    if (status)
	return (adu_error(adf_scb, status, 0));

    /* Now do space check */
    /* ------------------ */
    needed = npi_input_ops = npi_size = 0;
    if (npi != ADE_0CXI_IGNORE)
    {
	if (ade_icode == ADE_MECOPYN || ade_icode == ADE_MECOPYQA ||
		ade_icode == ADE_AGNEXT || ade_icode == ADE_UNORM)
	{
	    if (ade_ops[0].opr_dt < 0)
	    {
		i = 1;
		out_nullability = AD_NULLABLE;
	    }
	    else
	    {
		i = 1;
		out_nullability = AD_NON_NULLABLE;
	    }
	}
	else if (fdesc->adi_fitype == ADI_COMPARISON)
	{
	    i = 0;
	    out_nullability = AD_NOT_APPLICABLE;
	}
	else if (fdesc->adi_fitype == ADI_AGG_FUNC)
	{
	    if (npi == ADE_5CXI_CLR_SKIP)
	    {
		/* Max/Min require null checking. */
		if (ade_ops[0].opr_dt < 0)
		{
		    i = 1;
		    out_nullability = AD_NULLABLE;
		}
		else
		{
		    i = 1;
		    out_nullability = AD_NON_NULLABLE;
		}
	    }
	    else
	    {
	  	i = 1;
		out_nullability = AD_NOT_APPLICABLE;
	    }
	}
	else if (ade_ops[0].opr_dt < 0)
	{
	    i = 1;
	    out_nullability = AD_NULLABLE;
	}
	else
	{
	    i = 1;
	    out_nullability = AD_NON_NULLABLE;
	}

	while (i < ade_nops)
	{
	    if (ade_ops[i++].opr_dt < 0)	    /* is type nullable? */
	    {
		npi_input_ops++;
		needed += sizeof(ADE_I_OPERAND);
	    }
	}

	/*
	** If any nullable inputs found, or if output
	** is nullable, add room for instr and result.
	*/
	add_npi = FALSE;
	if (npi_input_ops  ||  out_nullability == AD_NULLABLE)
	{
	    add_npi = TRUE;
	    /* ...unless this is the FINIT segment and this is a statistical
	    ** aggregate, in which case we always have to process the final
	    ** result even if nothing has been provided so far.
	    ** (kibro01) b122225
	    */
	    if ((ade_seg == ADE_SFINIT) &&
		(ade_icode == ADFI_844_STDDEV_POP_FLT ||
		 ade_icode == ADFI_845_STDDEV_SAMP_FLT ||
		 ade_icode == ADFI_846_VAR_POP_FLT ||
		 ade_icode == ADFI_847_VAR_SAMP_FLT))
	    {
		add_npi = FALSE;
	    }
	}
	if (add_npi)
	{
	    if (npi == ADE_1CXI_SET_SKIP || npi == ADE_4CXI_CMP_SET_SKIP
	      || npi == ADE_5CXI_CLR_SKIP)
	    {
		if (out_nullability == AD_NON_NULLABLE)
		{
		    /* Must have nullable input ops, just check them.
		    ** Don't generate operand slot for result.
		    */
		    npi = ADE_7CXI_CKONLY;
		    out_nullability = AD_NOT_APPLICABLE;
		}
		else if (npi_input_ops == 0)
		{
		    /* Must have just nullable output, just clear it */
		    npi = ADE_6CXI_CLR_OP0NULL;
		}
	    }
	    needed += sizeof(ADE_INSTRUCTION);
	    if (out_nullability != AD_NOT_APPLICABLE)
		needed += sizeof(ADE_I_OPERAND);
	    ADE_ROUNDUP_MACRO(needed, DB_ALIGN_SZ);
	    npi_size = needed;
	}
	else
	{
	    npi = ADE_0CXI_IGNORE;	/* no nullables found, so no NPI */
	}
    }

    needed += sizeof(ADE_INSTRUCTION) + (ade_nops * sizeof(ADE_I_OPERAND));
    ADE_ROUNDUP_MACRO(needed, DB_ALIGN_SZ);
    if ((cxhead->cx_allocated - cxhead->cx_bytes_used) < needed)
	return (adu_error(adf_scb, E_AD5506_NO_SPACE, 0));


    /*======================================================*/
    /*                                                      */
    /* Finally, we can go ahead and compile the instruction */
    /*                                                      */
    /*======================================================*/


    /* Set up the appropriate "may-be-needed" query constant flag */
    if (ade_uses_qconst != NULL)
	ad0_setqconst_flag(ade_icode, ade_uses_qconst);


    new_instr_offset = cxhead->cx_bytes_used;

    /* Find the previous instruction in the requested segment, and     */
    /* update some of the CX header information regarding that segment */
    /* --------------------------------------------------------------- */
    prev_instr_offset = cxseg->seg_last_offset;
    if (prev_instr_offset == ADE_END_OFFSET_LIST)
	cxseg->seg_1st_offset = new_instr_offset;

    /* Now update previous instruction if there was one */
    /* ------------------------------------------------ */
    if (prev_instr_offset != ADE_END_OFFSET_LIST)
    {
	iptr = (ADE_INSTRUCTION *) ((char *) ade_cx + prev_instr_offset);
	ADE_SET_OFFSET_MACRO(iptr, new_instr_offset);
    }


    /* ---------------------------------------------------------- */
    /* This section compiles an optional nullable pre-instruction */
    /* ---------------------------------------------------------- */
    if (npi != ADE_0CXI_IGNORE)
    {
	cxseg->seg_last_offset = new_instr_offset;
	++ cxseg->seg_n;
	iptr = (ADE_INSTRUCTION *) ((char *) ade_cx + new_instr_offset);
	ADE_SET_OFFSET_MACRO(iptr, ADE_END_OFFSET_LIST);
	iptr->ins_icode = npi;
	switch (npi)
	{
	  case ADE_1CXI_SET_SKIP:
	  case ADE_4CXI_CMP_SET_SKIP:
	  case ADE_5CXI_CLR_SKIP:
	  case ADE_6CXI_CLR_OP0NULL:
	    iptr->ins_nops = npi_input_ops + 1;

	    /* Set up the operands */
	    /* ------------------- */
	    optr = (ADE_I_OPERAND *) ((char *) iptr + sizeof(ADE_INSTRUCTION));

	    /* Output operand first */
	    /* -------------------- */
	    ADE_SET_OPERAND_MACRO(&ade_ops[0], optr);
	    optr++;

	    /* Now the nullable input operands, if any */
	    /* --------------------------------------- */
	    for (i=1; i<ade_nops; i++)
	    {
		if (ade_ops[i].opr_dt < 0)
		{
		    ADE_SET_OPERAND_MACRO(&ade_ops[i], optr);
		    optr++;
		}
	    }
	    break;

	  case ADE_3CXI_CMP_SET_SKIP:
	    iptr->ins_nops = npi_input_ops;

	    /* Set up the operands */
	    /* ------------------- */
	    optr = (ADE_I_OPERAND *) ((char *) iptr + sizeof(ADE_INSTRUCTION));
	    for (i=0; i<ade_nops; i++)
	    {
		if (ade_ops[i].opr_dt < 0)
		{
		    ADE_SET_OPERAND_MACRO(&ade_ops[i], optr);
		    optr++;
		}
	    }
	    break;

	  case ADE_2CXI_SKIP:
	  case ADE_7CXI_CKONLY:
	    iptr->ins_nops = npi_input_ops;

	    /* Set up the operands */
	    /* ------------------- */
	    optr = (ADE_I_OPERAND *) ((char *) iptr + sizeof(ADE_INSTRUCTION));
	    for (i=1; i<ade_nops; i++)	/* Start at opr #1; don't need result */
	    {
		if (ade_ops[i].opr_dt < 0)
		{
		    ADE_SET_OPERAND_MACRO(&ade_ops[i], optr);
		    optr++;
		}
	    }
	    break;

	}
	new_instr_offset += npi_size;
	ADE_SET_OFFSET_MACRO(iptr, new_instr_offset);
    }
    /* ----------------------------------------------- */
    /* Finished with optional nullable pre-instruction */
    /* ----------------------------------------------- */

#ifdef ADE_BACKEND
    /* ------------------------------------------- */
    /* Optimization to find and coalesce contiguous MECOPY's.
    ** Bits of a contiguous area being moved might arrive out of order,
    ** so we rescan the mecopy block until nothing more happens.
    ** For a nice canonical example, try this:
    ** given a table tmp identical to iidbdb's iidatabase, do
    ** insert into tmp (columns) select columns from iidatabase
    ** where the columns are listed in *alphabetical* order.
    */
    /* ------------------------------------------- */
    /* If current and prev are MECOPY... (don't need to test for noop,
    ** we never leave noop's at the end).
    */
    if (ade_icode == ADE_MECOPY && prev_instr_offset != ADE_END_OFFSET_LIST &&
	npi == ADE_0CXI_IGNORE && iptr->ins_icode == ADE_MECOPY &&
	optimise_mecopy)
    {
	ADE_INSTRUCTION	*pptr, *dptr = NULL;
	ADE_INSTRUCTION	*start_insn;
	i4		opr_len = ade_ops[0].opr_len;
	i4		next_offset, combine;
	i4		bbstart_offset;
	i4		start_n;
	i4		mecopy_size = sizeof(ADE_INSTRUCTION) + 
				    (2 * sizeof(ADE_I_OPERAND));
	SIZE_TYPE	opr0_offset = ade_ops[0].opr_offset;
	SIZE_TYPE	opr1_offset = ade_ops[1].opr_offset;
	ADE_I_OPERAND   *pop0, *pop1;

	i = cxseg->seg_n;

	/* Before we get going, we need to make sure that we don't try
	** to coalesce beyond the start of a "basic block", ie a branch
	** label.  It would be bad to coalesce instructions if some of
	** them are conditionally executed!  The way code-gen works at
	** present (Dec '04), I don't think it's actually possible to get
	** into trouble;  but better safe than sorry.
	** Scan the segment looking for branching-things, and simply
	** remember the highest label offset seen.  We must not consider
	** any MECOPY prior to that highest label offset.
	**
	** While we're at it, record some convenient position that we
	** know we won't be coalescing beyond ... i.e. the position of
	** the last non-MOVE, non-NOOP instruction.  We need this as an
	** optimization:  there is no "previous instruction offset", and
	** to move backwards in the CX we have to start at the front
	** and skip forwards.  This is clearly quadratic, and for large
	** CX's (10000's of instructions), it can take massive amounts
	** of time just repositioning.  (E.g. a full minute of compile
	** time doing a 35000 instruction CX with a bunch of moves at
	** the end.)  By saving a useful starting position we don't have
	** to count from the very beginning when moving backwards.
	*/

	pptr = (ADE_INSTRUCTION *) (ade_cx + cxseg->seg_1st_offset);
	bbstart_offset = -1;
	start_n = 0;
	start_insn = pptr;
	j = 1;
	while (pptr != iptr)
	{
	    if (pptr->ins_icode == ADE_BRANCH
	      || pptr->ins_icode == ADE_BRANCHF
	      || pptr->ins_icode == ADE_ANDL
	      || pptr->ins_icode == ADE_ORL)
	    {
		/* A branching instruction, remember highest label */
		pop0 = (ADE_I_OPERAND *)((PTR)pptr + sizeof(ADE_INSTRUCTION));
		if (pop0->opr_offset > bbstart_offset)
		    bbstart_offset = pop0->opr_offset;
		/* Both operands if two-label instruction */
		if (pptr->ins_nops > 1)
		{
		    pop1 = (ADE_I_OPERAND *) ((PTR)pop0 + sizeof(ADE_I_OPERAND));
		    if (pop1->opr_offset > bbstart_offset)
			bbstart_offset = pop1->opr_offset;
		}
	    }
	    if (pptr->ins_icode != ADE_MECOPY && pptr->ins_icode != ADE_NOOP)
	    {
		start_n = j;
		start_insn = pptr;
	    }
	    ++j;
	    if (pptr->ins_offset_next == -1)
		pptr = iptr;
	    else pptr = (ADE_INSTRUCTION *) (ade_cx + pptr->ins_offset_next);
	}
	/* Note we end with pptr == iptr, ie last generated MECOPY.
	** We also end with start_n, start_insn set to a useful
	** base position for locating arbitrary instructions.
	*/

	/* While there are adjacent previous MECOPY instructions... */
	while ( (--i >= 0) && (pptr->ins_icode == ADE_MECOPY || 
			pptr->ins_icode == ADE_NOOP) )
	{
	    /* Skip over NOOPs, can skip the instr we just combined into */
	    if ( pptr->ins_icode == ADE_MECOPY && pptr != dptr)
	    {
		/* pptr is previous ADE_MECOPY instruction */

		/* Extract previous instruction operands */
		pop0 = (ADE_I_OPERAND *)((PTR)pptr + sizeof(ADE_INSTRUCTION));
		pop1 = (ADE_I_OPERAND *)((PTR)pop0 + sizeof(ADE_I_OPERAND));

		/* from/to bases must be the same */
		/* new opnd's better start where prev. 
		** left off. Note: pop1->opr_len isn't
		** always set by caller, so pop0->opr_len
		** is used for both offset checks */

		if ( pop1->opr_base == ade_ops[1].opr_base &&
		     pop0->opr_base == ade_ops[0].opr_base )
		{
		    if ( pop1->opr_offset+pop0->opr_len == opr1_offset &&
			 pop0->opr_offset+pop0->opr_len == opr0_offset )
		    {
			/* (b) */
			/* New mecopy is just a continuation */
			combine = TRUE;
		    }
		    else if ( opr1_offset+opr_len == pop1->opr_offset &&
			      opr0_offset+opr_len == pop0->opr_offset )
		    {
			/* (a) */
			/* Adjust target offsets to include these bytes */
			pop0->opr_offset = opr0_offset;
			pop1->opr_offset = opr1_offset;

			combine = TRUE;
		    }
		    else
			combine = FALSE;

		    if ( combine )
		    {
			/* Extend lengths to include these bytes */
			pop0->opr_len += opr_len;
			pop1->opr_len = pop0->opr_len;

			/* These are our next set of candidates */
			opr_len = pop0->opr_len;
			opr0_offset = pop0->opr_offset;
			opr1_offset = pop1->opr_offset;

			/*
			** If we combined from a compiled instruction,
			** (dptr != NULL) rather than from the
			** input ade_ops parameters,
			** mark the instruction as a NOOP, unlink it,
			** and decrease the number of instructions.
			** If it was the "last" compiled instruction,
			** set the previous as the new last.
			** dptr can't be the first mecopy since we work
			** from the back.
			*/
			if ( dptr )
			{
			    ADE_INSTRUCTION *tptr, *tptr1;

			    next_offset = dptr->ins_offset_next;
			    dptr->ins_icode = ADE_NOOP;
			    /* Tediously find the predecessor instruction,
			    ** by chasing instructions from the start and
			    ** remembering the predecessor, until we get to
			    ** dptr.
			    */
			    j = cxseg->seg_n - start_n;
			    tptr = NULL;
			    tptr1 = start_insn;
			    while (tptr1 != dptr && --j > 0)
			    {
				tptr = tptr1;
				tptr1 = (ADE_INSTRUCTION *) (ade_cx + tptr->ins_offset_next);
			    }
			    dptr = tptr;
			    ADE_SET_OFFSET_MACRO(dptr, next_offset);
			    -- cxseg->seg_n;

			    if ( next_offset == ADE_END_OFFSET_LIST )
			    {
				/* if we snipped at the end of the cx, the
				** seg's last offset + mecopy-size would be the
				** end of the CX -- if this is the case, trim
				** the CX back as well.
				** Note: any noop's that we left lying about
				** are still in there, clogging up the works,
				** but this is better than nothing.
				*/
				if (cxseg->seg_last_offset + mecopy_size == cxhead->cx_bytes_used)
				    cxhead->cx_bytes_used -= mecopy_size;
				cxseg->seg_last_offset = (char*)dptr - ade_cx;
				iptr = dptr;
			    }
			}
			else
			    /* Reset next offset in last instr */
			    ADE_SET_OFFSET_MACRO(iptr, ADE_END_OFFSET_LIST);

			/* Where the next set of comparands came from */
			dptr = pptr;

			/* Rescan from the back */
			i = cxseg->seg_n;
			continue;
		    }
		}
	    }
	    /* Unwind to next previous instruction.  We can't just subtract
	    ** mecopy_size because other segments might have generated stuff
	    ** into the CX.  We need a wee loop to skip to the i'th instr.
	    ** (which is outrageously stupid, but IMHO the whole way cx's
	    ** are physically allocated is clumsy.)
	    */
	    j = i - start_n;
	    pptr = start_insn;
	    while (--j > 0)
	    {
		if (pptr->ins_offset_next == ADE_END_OFFSET_LIST)
		    break;
		pptr = (ADE_INSTRUCTION *) (ade_cx + pptr->ins_offset_next);
	    }
	    /* Drop out if we moved out of the basic block */
	    if ( ((PTR) pptr - ade_cx) < bbstart_offset)
		break;
		
	}

	if ( opr_len != ade_ops[0].opr_len )
	{
	    /* ...then we were successful, discard this instruction */
	    return(E_DB_OK);
	}
    }

#endif  /* ADE_BACKEND */


    /* -------------------------- */
    /* Set up the new instruction */
    /* -------------------------- */
    cxseg->seg_last_offset = new_instr_offset;
    ++ cxseg->seg_n;
    iptr = (ADE_INSTRUCTION *) ((char *) ade_cx + new_instr_offset);
    ADE_SET_OFFSET_MACRO(iptr, ADE_END_OFFSET_LIST);
    iptr->ins_icode = ade_icode;
    iptr->ins_nops = ade_nops;

    /* Set up the new instruction's operands, if any */
    /* --------------------------------------------- */
    optr = (ADE_I_OPERAND *) ((char *) iptr + sizeof(ADE_INSTRUCTION));
    for (i=0; i<ade_nops; i++)
    {
	/* Update CX's high base, if necessary */
	cxhead->cx_hi_base = max(cxhead->cx_hi_base, ade_ops[i].opr_base);

	tmp_opr.opr_prec   = ade_ops[i].opr_prec;
	tmp_opr.opr_collID = ade_ops[i].opr_collID;
	tmp_opr.opr_len    = ade_ops[i].opr_len;
	tmp_opr.opr_dt     = ade_ops[i].opr_dt;
	tmp_opr.opr_base   = ade_ops[i].opr_base;
	tmp_opr.opr_offset = ade_ops[i].opr_offset;
	if (    tmp_opr.opr_dt < 0			/* if nullable type...*/
	    &&  npi != ADE_0CXI_IGNORE			/* & need pre-instr...*/
	    &&  (    i != 0				/* & not opr #0 of ...*/
		 ||  ade_icode == ADE_MECOPY		/* ADE_MECOPY	      */
		 ||  ade_icode == ADE_MECOPYN		/* ADE_MECOPYN	      */
		 ||  ade_icode == ADE_MECOPYQA		/* ADE_MECOPYQA	      */
		 ||  ade_icode == ADE_UNORM		/* ADE_UNORM	      */
		 ||  fdesc->adi_fitype != ADI_AGG_FUNC	/* aggregate f.i.     */
		)
	   )
	{
	    if (tmp_opr.opr_len != ADE_LEN_UNKNOWN)
		tmp_opr.opr_len--;
	    tmp_opr.opr_dt = (- tmp_opr.opr_dt);
	}
	if (tmp_opr.opr_base == ADE_LABBASE)
	    ade_ops[i].opr_offset = (i4)(new_instr_offset + 
		sizeof(ADE_INSTRUCTION) + i*sizeof(ADE_I_OPERAND));	
						/* it's a label - chain label
						** operand to previous */
	/* Check operand for blob-ness, variable len - set "fancy" flag.
	** keybld instruction is goofy, has faked-up operand 4, ignore it.
	** Note we already turned on "fancy" for CALCLEN instructions.
	*/
	if (ade_icode != ADE_KEYBLD || i != 4)
	{
	    j = abs(tmp_opr.opr_dt);
	    if (j == DB_LVCH_TYPE || j == DB_LBYTE_TYPE || j == DB_LNVCHR_TYPE
	      || tmp_opr.opr_len == ADE_LEN_UNKNOWN)
	    cxseg->seg_flags |= ADE_CXSEG_FANCY;
	}
	ADE_SET_OPERAND_MACRO(&tmp_opr, optr);
	optr++;
    }

    /* Was a VLT added? */
    if (ade_icode == ADE_CALCLEN)
	cxhead->cx_n_vlts++;

    cxhead->cx_bytes_used += needed;

    return (E_DB_OK);
}


/*{
** Name: ade_cxinfo() - Return various pieces of information about a CX.
**
** Description:
**	This routine enables one to obtain various pieces of interesting
**      information about a CX.  Such things as the amount of memory allocated
**      to the CX, or the amount of that memory that has already been used by
**      the CX are good examples. 
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      ade_cx                          Pointer to the CX.
**      ade_request			Request code for the piece of info the
**					caller desires.  The following table
**					shows the recognized request codes
**					(defined in <ade.h>), what they return,
**					and what type they are returned as:
**
**
**	*Note* the only ones used are requests 1, and 9 thru 14.
**	10 thru 14 are only used by front-ends.
**
**		ADE_01REQ_BYTES_ALLOCATED   Mem alloc'ed to CX.      i4
**		ADE_02REQ_BYTES_USED        Mem used by CX.	     i4
**		ADE_03REQ_NUM_INSTRS	    # instrs in each seg.    i4[3]
**								       [0]=INIT
**								       [1]=MAIN
**								       [2]=FINIT
**		ADE_04REQ_NUM_CONSTANTS     # constants.	     i4
**		ADE_05REQ_NUM_VLTS          # variable length tmps.  i4
**		ADE_06REQ_HIGH_BASE         Highest #'ed base used.  i4
**		ADE_07REQ_ALIGNMENT_IN_USE  Alignment used by CX.    i4
**		ADE_08REQ_VERSION	    Version # of CX.	     i2[2]
**		ADE_09REQ_SIZE_OF_CXHEAD    # bytes in a CX header.  i4
**		    *** NOTE: ptr to CX can be NULL **
**		ADE_10REQ_1ST_INSTR_OFFSETS Offsets to 1st instrs.   i4[3]
**								       [0]=INIT
**								       [1]=MAIN
**								       [2]=FINIT
**		ADE_11REQ_1ST_INSTR_ADDRS   Addrs of instr lists.    PTR[3]
**		    *** FRONTENDS TAKE NOTE!!! ***		       [0]=INIT
**			This request will only be valid as long as     [1]=MAIN
**			the CX header is immediately followed by       [2]=FINIT
**			the instructions/constants. (i.e. They are
**			contiguous.)
**		ADE_12REQ_FE_INSTR_LIST_I2S # i2s in FE instr lists. nat[3]
**		    *** FRONTENDS TAKE NOTE!!! ***		       [0]=INIT
**			This request will only be valid as long as     [1]=MAIN
**			the CX header is immediately followed by       [2]=FINIT
**			the instructions/constants. (i.e. They are
**			contiguous.)
**		    *** FURTHER ASSUMPTION!!! ***
**			This will only be used by FE's, and all
**			instructions in each list (i.e. segment)
**			are contiguous.
**		ADE_13REQ_ADE_VERSION	    Version # of ADE.	     i2[2]
**		    *** NOTE: ptr to CX can be NULL **
**		ADE_14REQ_LAST_INSTR_OFFSETS Offsets to last instrs. i4[3]
**								       [0]=INIT
**								       [1]=MAIN
**								       [2]=FINIT
**		ADE_15REQ_LAST_INSTR_ADDRS   Addrs end of inst lists.PTR[3]
**		    *** FRONTENDS TAKE NOTE!!! ***		       [0]=INIT
**			This request will only be valid as long as     [1]=MAIN
**			the CX header is immediately followed by       [2]=FINIT
**			the instructions/constants. (i.e. They are
**			contiguous.)
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
**	ade_result			Pointer to location to place info.
**					This must be passed in as a PTR, but
**					will be cast to the appropriate type
**					based on the request code.  Example:
**					If request code is ADE_10REQ_VERSION,
**					then this routine will assume that
**					ade_result points to an array of 2 i2s.
**					If, however, the request code is
**					ADE_02REQ_BYTES_USED, it will be assumed
**					that ade_result points to a i4.
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
**	    E_AD550F_BAD_CX_REQUEST	Unknown request code.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      23-feb-87 (thurston)
**          Initial creation and coding.
**	25-mar-87 (thurston)
**	    Changed the ade_result arg to be a PTR instead of a i4*.  Also
**	    added several other request codes.
**	13-may-87 (thurston)
**	    Added request codes ADE_14REQ_LAST_INSTR_OFFSETS and
**	    ADE_15REQ_LAST_INSTR_ADDRS.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
ade_cxinfo(
ADF_CB              *adf_scb,
PTR                 ade_cx,
i4		    ade_request,
PTR                 ade_result)
# else
DB_STATUS
ade_cxinfo( adf_scb, ade_cx, ade_request, ade_result)
ADF_CB              *adf_scb;
PTR                 ade_cx;
i4		    ade_request;
PTR                 ade_result;
# endif
{
    ADE_CXHEAD	        *cxhead = (ADE_CXHEAD *)ade_cx;
    DB_STATUS		db_stat = E_DB_OK;
    ADE_INSTRUCTION	*instr;
    i4			instr_sz;
    i4			num_i2s;
    i4		i;


    switch (ade_request)
    {
      case ADE_01REQ_BYTES_ALLOCATED:
	*(i4 *)ade_result = cxhead->cx_allocated;
	break;

      case ADE_02REQ_BYTES_USED:
	*(i4 *)ade_result = cxhead->cx_bytes_used;
	break;

      case ADE_03REQ_NUM_INSTRS:
	((i4 *)ade_result)[0] = cxhead->cx_seg[ADE_SINIT].seg_n;
	((i4 *)ade_result)[1] = cxhead->cx_seg[ADE_SMAIN].seg_n;
	((i4 *)ade_result)[2] = cxhead->cx_seg[ADE_SFINIT].seg_n;
	break;

      case ADE_04REQ_NUM_CONSTANTS:
	*(i4 *)ade_result = cxhead->cx_n_k;
	break;

      case ADE_05REQ_NUM_VLTS:
	*(i4 *)ade_result = cxhead->cx_n_vlts;
	break;

      case ADE_06REQ_HIGH_BASE:
	*(i4 *)ade_result = cxhead->cx_hi_base;
	break;

      case ADE_07REQ_ALIGNMENT_IN_USE:
	*(i4 *)ade_result = cxhead->cx_alignment;
	break;

      case ADE_08REQ_VERSION:
	((i2 *)ade_result)[0] = cxhead->cx_hi_version;
	((i2 *)ade_result)[1] = cxhead->cx_lo_version;
	break;

      case ADE_09REQ_SIZE_OF_CXHEAD:
	*(i4 *)ade_result = sizeof(ADE_CXHEAD);
	break;

      case ADE_10REQ_1ST_INSTR_OFFSETS:
	((i4 *)ade_result)[0] = cxhead->cx_seg[ADE_SINIT].seg_1st_offset;
	((i4 *)ade_result)[1] = cxhead->cx_seg[ADE_SMAIN].seg_1st_offset;
	((i4 *)ade_result)[2] = cxhead->cx_seg[ADE_SFINIT].seg_1st_offset;
	break;

      case ADE_11REQ_1ST_INSTR_ADDRS:
	/*
	** NOTE:  This request will only be valid as long as the CX header
	**	  is immediately followed by the instructions/constants.
	**	  (i.e.  They are contiguous.)
	*/
	((PTR *)ade_result)[0] = (PTR)((char *)cxhead + cxhead->cx_seg[ADE_SINIT].seg_1st_offset);
	((PTR *)ade_result)[1] = (PTR)((char *)cxhead + cxhead->cx_seg[ADE_SMAIN].seg_1st_offset);
	((PTR *)ade_result)[2] = (PTR)((char *)cxhead + cxhead->cx_seg[ADE_SFINIT].seg_1st_offset);
	break;

      case ADE_12REQ_FE_INSTR_LIST_I2S:
	/*
	** NOTE:  This request will only be valid as long as the CX header
	**	  is immediately followed by the instructions/constants.
	**	  (i.e.  They are contiguous.)
	**
	** FURTHER ASSUMPTION:  This will only be used by FE's, and all
	**			instructions in a list (i.e. segment)
	**			are contiguous.
	*/
	num_i2s = 0;
	instr = (ADE_INSTRUCTION *)((char *)cxhead + cxhead->cx_seg[ADE_SINIT].seg_1st_offset);
	for (i=cxhead->cx_seg[ADE_SINIT].seg_n; i; i--)
	{
	    instr_sz = sizeof(ADE_INSTRUCTION)
		     + (instr->ins_nops * sizeof(ADE_I_OPERAND));
	    num_i2s += instr_sz / sizeof(i2);
	    instr = (ADE_INSTRUCTION *)((char *)instr + instr_sz);
	}
	((i4 *)ade_result)[0] = num_i2s;

	num_i2s = 0;
	instr = (ADE_INSTRUCTION *)((char *)cxhead + cxhead->cx_seg[ADE_SMAIN].seg_1st_offset);
	for (i=cxhead->cx_seg[ADE_SMAIN].seg_n; i; i--)
	{
	    instr_sz = sizeof(ADE_INSTRUCTION)
		     + (instr->ins_nops * sizeof(ADE_I_OPERAND));
	    num_i2s += instr_sz / sizeof(i2);
	    instr = (ADE_INSTRUCTION *)((char *)instr + instr_sz);
	}
	((i4 *)ade_result)[1] = num_i2s;

	num_i2s = 0;
	instr = (ADE_INSTRUCTION *)((char *)cxhead + cxhead->cx_seg[ADE_SFINIT].seg_1st_offset);
	for (i=cxhead->cx_seg[ADE_SFINIT].seg_n; i; i--)
	{
	    instr_sz = sizeof(ADE_INSTRUCTION)
		     + (instr->ins_nops * sizeof(ADE_I_OPERAND));
	    num_i2s += instr_sz / sizeof(i2);
	    instr = (ADE_INSTRUCTION *)((char *)instr + instr_sz);
	}
	((i4 *)ade_result)[2] = num_i2s;

	break;

      case ADE_13REQ_ADE_VERSION:
	((i2 *)ade_result)[0] = ADE_CXHIVERSION;
	((i2 *)ade_result)[1] = ADE_CXLOVERSION;
	break;

      case ADE_14REQ_LAST_INSTR_OFFSETS:
	((i4 *)ade_result)[0] = cxhead->cx_seg[ADE_SINIT].seg_last_offset;
	((i4 *)ade_result)[1] = cxhead->cx_seg[ADE_SMAIN].seg_last_offset;
	((i4 *)ade_result)[2] = cxhead->cx_seg[ADE_SFINIT].seg_last_offset;
	break;

      case ADE_15REQ_LAST_INSTR_ADDRS:
	/*
	** NOTE:  This request will only be valid as long as the CX header
	**	  is immediately followed by the instructions/constants.
	**	  (i.e.  They are contiguous.)
	*/
	((PTR *)ade_result)[0] = (PTR)((char *)cxhead + cxhead->cx_seg[ADE_SINIT].seg_last_offset);
	((PTR *)ade_result)[1] = (PTR)((char *)cxhead + cxhead->cx_seg[ADE_SMAIN].seg_last_offset);
	((PTR *)ade_result)[2] = (PTR)((char *)cxhead + cxhead->cx_seg[ADE_SFINIT].seg_last_offset);
	break;

      default:
	db_stat = adu_error(adf_scb, E_AD550F_BAD_CX_REQUEST, 2,
			    (i4) sizeof(ade_request),
			    (i4 *) &ade_request);
	break;
    }

    return (db_stat);
}


/*{
** Name: ade_inform_space() - Inform ADE that the CX size has changed.
**
** Description:
**	This routine enables one to change the size of a CX.  It will typically
**      be used by someone who is compiling a CX and runs out of space in the
**      CX buffer (i.e. receives the E_AD5506_NO_SPACE error from one of ADE's
**	compilation routines).  Even if the compiler had estimated the size of
**	the CX using ade_cx_space(), this can occur, since instructions that
**	were not counted on may need to be compiled (such as, additional
**	ADE_MECOPY instructions to conquer alignment problems reported by
**	ade_instr_gen()).  When this happens, the compiler would need to
**	allocate a bigger CX buffer and copy the contents of the original 
**	buffer into it.  This can be done because a CX is relocatable.  Then,
**	the compiler must inform ADE of the size change by calling this routine,
**	after which compilation can continue as if nothing happened.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      ade_cx                          Pointer to the CX.
**      ade_new_cxsize                  New size of the CX in bytes.
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
**          E_AD550C_COMP_NOT_IN_PROG   The CX is not in the process of being
**					compiled.  You should be between calls
**					to ade_bgn_comp() and ade_end_comp().
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-jul-86 (thurston)
**          Initial creation and coding.
**	7-nov-95 (inkdo01)
**	    Loosen error test to allow OPC to reset shorter cx_allocated size.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
ade_inform_space(
ADF_CB             *adf_scb,
PTR                ade_cx,
i4            ade_new_cxsize)
# else
DB_STATUS
ade_inform_space( adf_scb, ade_cx, ade_new_cxsize)
ADF_CB             *adf_scb;
PTR                ade_cx;
i4            ade_new_cxsize;
# endif
{
    ADE_CXHEAD	       *cxhead = (ADE_CXHEAD *)ade_cx;


    /* Make sure CX is in progress of being compiled */
    /* --------------------------------------------- */
    if (cxhead->cx_hi_version != ADE_CXINPROGRESS && 
		cxhead->cx_bytes_used != ade_new_cxsize)
	return (adu_error(adf_scb, E_AD550C_COMP_NOT_IN_PROG, 2,
                                (i4) sizeof(SystemProductName),
                                (i4 *) &SystemProductName));

    cxhead->cx_allocated = ade_new_cxsize;
    return (E_DB_OK);
}

#ifdef ADE_BACKEND

/*
** Name: ad0_is_unaligned() - Check to see if operand is unaligned.
**
** Description:
**      This routine returns FALSE if the alignment of the supplied operand 
**      is OK, and TRUE if it is not OK.  The input parameter "usage" tells 
**      the routine whether to use the datatype and length to check for 
**      alignment, or whether to check for alignment by using DB_ALIGN_SZ.
**
** Inputs:
**      op                              Ptr to the operand to check.
**      usage                           If zero, just use DB_ALIGN_SZ to
**					check for alignment.  Otherwise, be
**					smart and look at the datatype and
**					length to figure out alignment.
**
** Outputs:
**      none
**
**	Returns:
**	    TRUE	Operand is not properly aligned.
**	    FALSE	Operand is properly aligned.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      09-jul-86 (thurston)
**          Initial creation and coding.
**	11-mar-87 (thurston)
**	    Added support for nullable datatypes.
**      03-nov-1992 (stevet)
**          Added DB_CHA_TYPE and DB_DEC_TYPE, which require no alignment.
**          Also added DB_VCH_TYPE and DB_LTXT_TYPE, which require to be
**          2 byte aligned.
**      15-Jul-1993 (fred)
**          Also DB_VBYTE_TYPE, DB_BYTE_TYPE...
**	29-june-01 (inkdo01)
**	    Alignment check for Unicode operands.
**	16-june-2006 (gupsh01)
**	    Alignment check for new ANSI datetime operands.
*/

static bool
ad0_is_unaligned(
ADE_OPERAND        *op,
i4                 usage)
{
    bool                ret_val = FALSE;
    DB_DT_ID            bdt;
    i4                  blen;
    i4                  offset;


    bdt = abs(op->opr_dt);
    blen = op->opr_len - ((op->opr_dt < 0) ? 1 : 0);
    offset = op->opr_offset;

    if (!usage)
	bdt = DB_NODT;

    switch (bdt)
    {
	case DB_CHR_TYPE:
        case DB_CHA_TYPE:
        case DB_DEC_TYPE:
        case DB_BYTE_TYPE:
	    break;

	case DB_TXT_TYPE:
        case DB_VCH_TYPE:
	case DB_VBYTE_TYPE:
	case DB_LTXT_TYPE:
	case DB_NCHR_TYPE:
	case DB_NVCHR_TYPE:
	    if (offset % 2)
		ret_val = TRUE;
	    break;

	case DB_INT_TYPE:
	case DB_FLT_TYPE:
	    if (offset % min(blen, DB_ALIGN_SZ))
		ret_val = TRUE;
	    break;

	case DB_DTE_TYPE:
	case DB_ADTE_TYPE:
	case DB_TMWO_TYPE:
	case DB_TMW_TYPE:
	case DB_TME_TYPE:
	case DB_TSWO_TYPE:
	case DB_TSW_TYPE:
	case DB_TSTMP_TYPE:
	case DB_INYM_TYPE:
	case DB_INDS_TYPE:
	    if (offset % min(4, DB_ALIGN_SZ))
		ret_val = TRUE;
	    break;

	case DB_MNY_TYPE:
	    if (offset % min(8, DB_ALIGN_SZ))
		ret_val = TRUE;
	    break;

	default:
	    if (offset % DB_ALIGN_SZ)
		ret_val = TRUE;
	    break;
    }

    return (ret_val);
}
#endif /* ADE_BACKEND */


/*
** Name: ad0_dtlenchk() - Check datatype and length validity for an operand.
**
** Description:
**      This routine checks the datatype and length validity of an operand, 
**      calling a length of zero OK since this might be compiled in for a VLUP 
**      or VLT.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      op                              Ptr to the operand.
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
**	    E_AD2004_BAD_DTID
**	    E_AD2005_BAD_DTLEN
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-jul-86 (thurston)
**          Initial creation and coding.
**	28-dec-86 (thurston)
**	    Modified code to use the constant ADE_LEN_UNKNOWN.
**	11-mar-87 (thurston)
**	    Added support for nullable datatypes.
**	18-jul-89 (jrb)
**	    Set precision field in db_data_value before calling lenchk.
*/

static DB_STATUS
ad0_dtlenchk(
ADF_CB             *adf_scb,
ADE_OPERAND        *op)
{
    DB_DATA_VALUE	dv;		/* Temp used for adc_lenchk() */
    DB_DT_ID            bdt;
    i4                  len = op->opr_len;

    bdt = ADI_DT_MAP_MACRO(abs(op->opr_dt));


    /* Check for illegal datatype ID */
    /* ----------------------------- */

    if (bdt <= 0  ||  bdt > ADI_MXDTS
	    ||  Adf_globs->Adi_dtptrs[bdt] == NULL)
	return (adu_error(adf_scb, E_AD2004_BAD_DTID, 0));

    /* Datatype is good, so look at length (call length of zero OK) */
    /* ------------------------------------------------------------ */

    else if (len == ADE_LEN_UNKNOWN)
	return (E_DB_OK);
    else if (len == ADE_LEN_LONG)
	return(OK);
    else
    {
	/* Now we must call adc_lenchk() */
	dv.db_datatype = op->opr_dt;
	dv.db_prec = op->opr_prec;
	dv.db_collID = op->opr_collID;
	dv.db_length = len;
	return (adc_lenchk(adf_scb, FALSE, &dv, NULL));
    }
}


/*
** Name: ad0_setqconst_flag() - Set the query constants flag for a given instr.
**
** Description:
**	This routine sets the might-be-using-qconst flag if the supplied
**	instruction might possibly be referencing a query constant.
**
** Inputs:
**      icode                           The CX instruction code.
**	qconst_flag			Ptr to an i4 which represents the flag
**					for query constants, so far.
**
** Outputs:
**	qconst_flag			Flag set if icode might require a
**					query constant, untouched otherwise
**
**	Returns:
**	    none -- VOID function.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-oct-87 (thurston)
**	    Initial creation.
**	24-Mar-2005 (schka24)
**	    Rename, and all we need is yes/no, not a mask.
**	29-Aug-2006 (gupsh01)
**	    Added CURRENT_DATE, CURRENT_TIME, CURRENT_TIMESTAMP
**	    LOCAL_TIME, LOCAL_TIMESTAMP as valid ANSI datetime 
**	    system constants.
*/

static VOID
ad0_setqconst_flag(
i4                 icode,
i4                 *qconst_flag)
{
    switch (icode)
    {
      case ADFI_442__BINTIM:             /* _bintim() */
      case ADFI_194__BINTIM_I:           /* _bintim(i) */
      case ADFI_125_DATE_C:              /* date(c) */
      case ADFI_126_DATE_TEXT:           /* date(text) */
      case ADFI_280_DATE_CHAR:           /* date(char) */
      case ADFI_281_DATE_VARCHAR:        /* date(varchar) */
      case ADFI_215_C_TO_DATE:           /* c -> date */
      case ADFI_216_TEXT_TO_DATE:        /* text -> date */
      case ADFI_336_CHAR_TO_DATE:        /* char -> date */
      case ADFI_348_LONGTEXT_TO_DATE:    /* longtext -> date */
      case ADFI_363_VARCHAR_TO_DATE:     /* varchar -> date */
      case ADFI_197__CPU_MS:             /* _cpu_ms() */
      case ADFI_198__ET_SEC:             /* _et_sec() */
      case ADFI_199__DIO_CNT:            /* _dio_cnt() */
      case ADFI_200__BIO_CNT:            /* _bio_cnt() */
      case ADFI_201__PFAULT_CNT:         /* _pfault_cnt() */
      case ADFI_397_DBMSINFO_VARCHAR:    /* dbmsinfo(varchar) */
      case ADFI_1233_CURRENT_DATE:    	 /* CURRENT_DATE */
      case ADFI_1234_CURRENT_TIME:    	 /* CURRENT_TIME */
      case ADFI_1235_CURRENT_TMSTMP:   	 /* CURRENT_TIMESTAMP */
      case ADFI_1236_LOCAL_TIME:   	 /* LOCAL_TIME */
      case ADFI_1237_LOCAL_TMSTMP:   	 /* LOCAL_TIMESTAMP */
	*qconst_flag = 1;
	break;

      default:
	/*
	** no possible need for any query constants
	*/
	break;
    }
}

#ifdef ADE_BACKEND 

/*
** Name: ad0_base_transform() - Replace operand base indexes.
**
** Description:
**      This function uses an integer array to map all operand base
**	values for a code segment of a CX.
**
** Inputs:
**      cxhead				Pointer to the CX.
**	first_offset			Integer offset to first instruction
**					of code segment.
**	last_offset			Integer offset to last instruction
**					of code segment.
**	basemap				Pointer to integer array defining 
**					base value mapping.
**
** Outputs:
**
**	Returns:
**	    none -- VOID function.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-aug-04 (inkdo01)
**	    Written.
*/

static VOID
ad0_base_transform(
	ADE_CXHEAD	*cxhead,
	i4		first_offset,
	i4		last_offset,
	i4		*basemap)

{
    ADE_INSTRUCTION	*instrp;
    ADE_I_OPERAND	*oprp;
    i4			i, next_offset = first_offset;

    /* Loop over all instructions in segment, then for each operand
    ** in the instruction, replace base value as necessary. Note that
    * "special bases" (<= ADE_ZBASE) are not replaced. */

    while (next_offset != ADE_END_OFFSET_LIST &&
		next_offset <= last_offset)
    {
	instrp = (ADE_INSTRUCTION *) ((char *)cxhead + next_offset);
					/* instruction addr */
	next_offset = instrp->ins_offset_next;

	for (i = 0; i < instrp->ins_nops; i++)
	{
	    oprp = (ADE_I_OPERAND *)((char *)instrp +
		sizeof(ADE_INSTRUCTION) + i*sizeof(ADE_I_OPERAND));
					/* operand addr */
	    if (oprp->opr_base >= ADE_ZBASE && 
			basemap[oprp->opr_base-ADE_ZBASE] >= 0)
		oprp->opr_base = basemap[oprp->opr_base-ADE_ZBASE];
	}	/* end of operand loop */
    }	/* end of instruction loop */

}


/* Name: ad0_post_optim - Post-generation CX optimizations
**
** Description:
**	This routine is called at end-compile time to do minor optimizations
**	on a generated CX code segment.  The optimizations we do are
**	akin to peephole optimizations, in that they do not rely on any
**	sort of global knowledge of the generated code or context.
**
**	The optimizations we currently do are:
**	- PMQUEL removal (see ad0_optim_pmquel)
**	- NOOP removal.  MOVE optimization may introduce NOOP operations,
**	  which are a nuisance to remove at generation time.  We'll remove
**	  them now so that execution can pretend that there's no such thing.
**
**	To come, maybe:  ADE_MECOPY motion and recombination.  maybe move
**	all the mecopy combining here.
**
** Inputs:
**	adf_scb		ADF_CB session control block
**	cxhead		CX header 
**	cxseg		Segment header to process
**
** Outputs:
**	None
**
** History:
**	29-Aug-2005 (schka24)
**	    Written.
*/

static void
ad0_post_optim(ADF_CB *adf_scb, ADE_CXHEAD *cxhead, ADE_CXSEG *cxseg)
{
    ADE_INSTRUCTION *instr;		/* A generated instruction */
    i4 *prev_offsetp;			/* For snipping out instructions */

    /* If the code segment is empty, return */
    prev_offsetp = &cxseg->seg_1st_offset;
    if (cxseg->seg_n == 0 || *prev_offsetp == ADE_END_OFFSET_LIST)
	return;

    /* Loop through and snip out NOOP's.  It's possible that a branch might
    ** branch to a NOOP, but fortunately we haven't done label verification
    ** yet.  That will fix up any dangling labels properly.
    */
    instr = (ADE_INSTRUCTION *)((char *)cxhead + *prev_offsetp);
    for (;;)
    {
	if (instr->ins_icode == ADE_NOOP)
	{
	    *prev_offsetp = instr->ins_offset_next;
	    --cxseg->seg_n;
	}
	else
	{
	    prev_offsetp = &instr->ins_offset_next;
	}
	if (instr->ins_offset_next == ADE_END_OFFSET_LIST)
	    break;
	instr = (ADE_INSTRUCTION *) ((char *)cxhead + instr->ins_offset_next);
    }

    ad0_optim_pmquel(adf_scb, cxhead, cxseg);

    /* Future optimizations go here */

} /* ad0_post_optim */

/*
** Name: ad0_optim_pmquel - Post-process CX to remove PMQUEL handling
**
** Description:
**	The original QUEL semantics for string comparisons involved
**	special characters in the constant string, rather than a
**	separate operator such as LIKE.  (One of the places where QUEL
**	went astray, IMHO.)  Since most queries run in SQL, the code
**	generators have to emit ADE_PMQUEL and ADE_NOPMQUEL instructions
**	to control quel-ness.  This is a pain and a nuisance, albeit a
**	minor one, and it would be nice to get rid of the PMQUEL
**	stuff entirely.
**
**	This routine is called after a segment is compiled.  It looks
**	through the generated code to see if any testing operator was
**	emitted with a character-style operand, under the control of PMQUEL.
**	(We'll assume that QUEL mode is initially ON.)  If the answer
**	is NO, we'll remove all PMQUEL-controlling instructions from
**	the CX and turn off the segment flag that says that execution
**	needs to start with PMQUEL turned on.  (Lack of said flag
**	tells the runtime that it can assume that nothing in the CX does
**	QUEL style testing operators.)
**
**	One note:  there's no need to be concerned with branching and
**	basic blocks and such.  PMQUEL-controlling instructions are always
**	emitted right before the testing operation that uses them, so
**	there is no need to carry quelness information along CX branching.
**
** Inputs:
**	adf_scb		ADF_CB session control block
**	cxhead		CX header 
**	cxseg		Segment header to process
**
** Outputs:
**	None
**
** History:
**	5-Mar-2005 (schka24)
**	    Written.
*/

static void
ad0_optim_pmquel(ADF_CB *adf_scb, ADE_CXHEAD *cxhead, ADE_CXSEG *cxseg)
{
    ADE_INSTRUCTION *instr;		/* A generated instruction */
    ADE_OPERAND *op;			/* Instruction operand */
    ADI_FI_DESC *fdesc;			/* FI descriptor */
    bool any_quel;			/* TRUE if any PMQUEL-ish instrs */
    bool is_quel;			/* TRUE if quel-ness holds */
    i4 dt;				/* (Positive) data type */
    i4 offset;				/* Offset to next instruction */
    i4 *prev_offsetp;			/* For snipping out instructions */

    cxseg->seg_flags |= ADE_CXSEG_PMQUEL;	/* Assume it's needed */
    offset = cxseg->seg_1st_offset;
    is_quel = TRUE;
    any_quel = FALSE;
    while (offset != ADE_END_OFFSET_LIST)
    {
	instr = (ADE_INSTRUCTION *) ((char *)cxhead + offset);
	if (instr->ins_icode == ADE_NO_PMQUEL)
	{
	    any_quel = TRUE;
	    is_quel = FALSE;
	}
	else if (instr->ins_icode == ADE_PMQUEL)
	{
	    any_quel = TRUE;
	    is_quel = TRUE;
	}
	else if (instr->ins_icode == ADE_PMFLIPFLOP)
	{
	    any_quel = TRUE;
	    is_quel = ! is_quel;
	}
	else if (instr->ins_icode >= 0)
	{
	    /* A regular FI opcode, not an ADE_xxx instruction.
	    ** Get its description and see if it's a testing operator,
	    ** one of < <= = != > >=.
	    */
	    (void) adi_fidesc(adf_scb, (ADI_FI_ID)instr->ins_icode, &fdesc);
	    if (fdesc->adi_fitype == ADI_COMPARISON
	      && (fdesc->adi_fiopid == ADI_EQ_OP
		  || fdesc->adi_fiopid == ADI_NE_OP
		  || fdesc->adi_fiopid == ADI_LT_OP
		  || fdesc->adi_fiopid == ADI_LE_OP
		  || fdesc->adi_fiopid == ADI_GT_OP
		  || fdesc->adi_fiopid == ADI_GE_OP) )
	    {
		/* It's a comparison, see if the operands are string-ish */
		op = (ADE_OPERAND *) ((char *)instr + sizeof(ADE_INSTRUCTION));
		dt = abs(op->opr_dt);
		/* It would be nice if there was a "I'm a string-y type"
		** flag in the data-types table.  New character-like types
		** (that need QUEL semantics) have to be added here too.
		*/
		if ((dt == DB_CHR_TYPE
		  || dt == DB_TXT_TYPE
		  || dt == DB_CHA_TYPE
		  || dt == DB_VCH_TYPE
		  || dt == DB_LVCH_TYPE
		  || dt == DB_NCHR_TYPE
		  || dt == DB_NVCHR_TYPE
		  || dt == DB_LNVCHR_TYPE) && is_quel)
		{
		    /* Found a string testing op needing QUEL mode... */
		    return;
		}
	    }
	}
	/* Skip to the next instruction */
	offset = instr->ins_offset_next;
    }
    /* If we got all the way through the CX, there's no need to run this
    ** segment with PMQUEL control.  Turn off the flag that says we need
    ** it.  And, if we saw any PMQUEL-controlling instructions, snip them
    ** out.
    */
    cxseg->seg_flags &= ~ADE_CXSEG_PMQUEL;
    if (! any_quel)
	return;
    /* Another pass through the segment (which is necessarily nonempty).
    ** Maintain a pointer to the previous offset so that we can link around
    ** any PMQUEL-ish instructions that we encounter.
    */
    prev_offsetp = &cxseg->seg_1st_offset;
    instr = (ADE_INSTRUCTION *)((char *)cxhead + *prev_offsetp);
    for (;;)
    {
	if (instr->ins_icode == ADE_NO_PMQUEL
	  || instr->ins_icode == ADE_PMQUEL
	  || instr->ins_icode == ADE_PMFLIPFLOP)
	{
	    *prev_offsetp = instr->ins_offset_next;
	    --cxseg->seg_n;
	}
	else
	{
	    prev_offsetp = &instr->ins_offset_next;
	}
	if (instr->ins_offset_next == ADE_END_OFFSET_LIST)
	    return;
	instr = (ADE_INSTRUCTION *) ((char *)cxhead + instr->ins_offset_next);
    }
    /* notreached */

} /* ad0_optim_pmquel */
#endif /* ADE_BACKEND */
