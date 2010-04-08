/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
NO_OPTIM =  sos_us5 i64_aix
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>

/* external routine declarations definitions */
#define        OPHHISTOS      TRUE
#define	       OPX_EXROUTINES TRUE
#include    <me.h>
#include    <bt.h>
#include    <mh.h>
#include    <ex.h>
#include    <opxlint.h>
#include    <adulcol.h>

/**
**
**  Name: OPHISTOS.C - routines to read or create histograms
**
**  Description:
**      Contains routines which will read or create histograms for attributes
**      FIXME - change (or add new type) histograms for char and text to be 
**      relative to previous cell value.  Thus, with two successive values of
**        130                     130
**      A     BBBBBBBBB          A    BBCDDDDDD
**
**      the second value would be represented as  (132),"CDDDDDD" where the
**      132 means take that many chars from the previous element.  This 
**      definition is recursive.
**
**  History:    
**      07-may-86 (seputis)    
**          initial creation
**      06-nov-89 (fred)
**          Added support for non-histogrammable datatypes.
**	15-dec-91 (seputis)
**	    use correct data type for histogram modification based on user
**	    qualifications when explicit secondary indexes are used
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	23-aug-93 (rganski)
**	    Moved histogram cell-splitting code out of oph_catalog into
**	    separate function oph_addkeys, which is further split up for
**	    modularity.
**	15-sep-93 (swm)
**	    Moved cs.h include above other header files which need its
**	    definition of CS_SID.
**	31-jan-94 (rganski)
**	    Moved oph_hvalue to this module from opbcreate.c, since it is
**	    now called from oph_sarglist as well as opb_ikey, and it is a
**	    histogram-type function.
**	12-jan-96 (inkdo01)
**	    Added support for per-cell repetition factors.
**      02-jul-97 (allmi01)
**          NO_OPTIM for sos_us5 for sigv during createdb -S iidbdb.
**	06-oct-98 (sarjo01)
**	    Bug 93561: Assure that per-cell repetition factors are non-zero
**	    prior to performing calculations in oph_join().
**	31-oct-1998 (nanpr01)
**	    Reset the infoblk ptr before checking the status.
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	3-Sep-2005 (schka24)
**	    Remove oph_gtsearch, the variable it depends on (opv_ghist)
**	    was never set anywhere!  so it never found a histogram.
**	03-Oct-2008 (kiria01) b120966
**	    Added DB_BYTE_TYPE support to allow for Unicode CE entries
**	    to be treated as raw collation data alongside DB_CHA_TYPE.
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
[@history_line@]...
**/

/* Static routines and variables: */

static OPB_BFARG *
oph_compargalloc(
	OPS_SUBQUERY	*subquery,
	i4		totallen);

static VOID
oph_compargorder(
	OPS_SUBQUERY	*subquery,
	OPB_BFARG	*bfargp,
	OPB_BFARG	**arglistpp,
	i4		keylen);

static char gtt_model[] = {'_', 'g', 't', 't', '_', 'm', 'o', 'd', 'e', 'l'};
				/* default schema qualifier for model
				** histogram tables */


/*{
** Name: oph_battr	- find var number and ingres attribute number of primary
**
** Description:
**      Routine will find the var number and ingres attribute number of the 
**      primary relation 
**
** Inputs:
**      subquery                        subquery being analyzed
**      atno                            joinop attribute number being analyzed
**
** Outputs:
**      pvarno                          var number of primary being analyzed
**      patno                           ingres attribute number of primary
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-aug-87 (seputis)
**          initial creation
**      19-sep-88 (seputis)
**          fix index attribute lookup
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not simply != 0.
[@history_line@]...
[@history_template@]...
*/
static VOID
oph_battr(
	OPS_SUBQUERY       *subquery,
	OPZ_IATTS          atno,
	OPV_IVARS          *pvarno,
	OPZ_IATTS          *patno)
{
    OPZ_ATTS	    *attrp;	    /* ptr to attribute descriptor for atno*/
    OPV_VARS	    *varp;          /* ptr to var descriptor associated with
                                    ** atno */
    OPV_VARS	    *pvarp;         /* ptr to var descriptor associated with
                                    ** primary relation */
    DMT_IDX_ENTRY   *indexp;        /* ptr to index descriptor for attribute */

    attrp = subquery->ops_attrs.opz_base->opz_attnums[atno]; /* base of array 
					** of ptrs to joinop */
    varp = subquery->ops_vars.opv_base->opv_rt[attrp->opz_varnm];
    *pvarno = attrp->opz_varnm;
    *patno = attrp->opz_attnm.db_att_id;
    if (!varp->opv_grv->opv_relation)
    {	/* if RDF info does not exist then this is a virtual relation so
	** return the same attributes */
	return;
    }
    if ((varp->opv_grv->opv_relation->rdr_rel->tbl_id.db_tab_index > 0)
	&&
	(varp->opv_index.opv_poffset >= 0)
	&&
	(pvarp = 
	    subquery->ops_vars.opv_base->opv_rt[varp->opv_index.opv_poffset])
	&&
	(varp->opv_index.opv_iindex >= 0)
	&&
	(indexp = pvarp->opv_grv->opv_relation->
	    rdr_indx[varp->opv_index.opv_iindex])
	)
    {	/* this is an index so get the ingres attribute number from the
	** base relation if it exists */
	*pvarno = varp->opv_index.opv_poffset; /* get offset of the
					** base relation */
	atno = indexp->idx_attr_id[attrp->opz_attnm.db_att_id-1]; /* get the
					** base relation attribute number from
					** the index decriptor tuple */
	if (atno > 0)
	    *patno = atno;
    }
}

/*{
** Name: oph_fhandler	- exception handler for floats
**
** Description:
**	This exception handler used when float exceptions occur when
**	trying to modify a histogram
**
** Inputs:
**      args                            arguments which describe the 
**                                      exception which was generated
**
** Outputs:
**	Returns:
**	    EXRESIGNAL                 - if this was an unexpected exception
**          EXDECLARE                  - if an expected, internally generated
**                                      exception was found
**	Exceptions:
**	    none
**
** Side Effects:
**	    This routine will remove the calling frame to the point at which
**          the exception handler was declared.
**
** History:
**	11-may-91 (seputis)
**          initial creation 
**	09-nov-92 (rganski)
**	    Print message to log and in QEP when exception occurs while
**	    modifying histogram.
**	02-jul-1993 (rog)
**	    Should return STATUS, not i4.  Removed the EX_DECLARE case because
**	    it won't happen.
[@history_line@]...
*/
static STATUS
oph_fhandler(
	EX_ARGS            *args)
{
    i4	ule_err_code;	/* passed back by ule_format */
    
    switch (EXmatch(args->exarg_num, (i4)10, EX_JNP_LJMP, EX_UNWIND, EXFLTUND,
		    EXFLTOVF, EXFLTDIV, EXINTOVF, EXINTDIV, EXHFLTDIV,
		    EXHFLTOVF, EXHFLTUND))
    {
    case 2:
				    /* a lower level exception handler has
				    ** cleared the stack so return EX_RESIGNAL
				    ** to satisfy the protocol */
	return (EXRESIGNAL);
    case 1:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    {
	OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
	OPS_CB              *opscb;	/* ptr to session control block */
	OPS_STATE           *global;    /* ptr to global state variable */
       
	/* Report problem modifying histogram */

	/* First, set flag to print float/integer exception message 
	** in QEP printout if QEP is SET.
	*/
	opscb = ops_getcb();            /* get the session control block
					** from the SCF */
	global = opscb->ops_state;      /* ptr to global state variable */
	subquery = global->ops_trace.opt_subquery; /* get ptr to current
					** subquery which is being traced */
	subquery->ops_mask |= OPS_SFPEXCEPTION;

	/* Second, print warning in error log that histogram modification
	** failed and the original histogram will be used.
	*/
	(VOID) ule_format(E_OP04AA_HISTOMOD, (CL_ERR_DESC *)NULL, ULE_LOG,
			  NULL, (char *)NULL, (i4)0, 
			  (i4 *)NULL, &ule_err_code, (i4)0); 

	/* Third, print session information to log. */
	scs_avformat();

	return(EXDECLARE);	    /* use the old histogram definition since
				    ** there were problems when trying to
				    ** define a modified histogram */
    }
    default:
        return (EXRESIGNAL);	    /* handle unexpected exception at lower
				    ** level exception handler, this exception
                                    ** handler only deals with out of memory
				    ** errors than can be recovered from */
    }
}

/*{
** Name: oph_hvalue	- create a histogram element given a key
**
** Description:
**      This routine will create the histogram element associated with 
**      a key
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      histdt                          ptr to datatype of histogram
**      valuedt                         ptr to datatype of value
**      key                             ptr to key to be used for creation
**                                      of histogram
**
** Outputs:
**	Returns:
**	    ptr to histogram element
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-may-86 (seputis)
**          initial creation
**	20-apr-90 (seputis)
**	    fix histogram problem with <= and >= 
**	which occurs when <= is used on an integer, in which oph_interpolate
**	does not have enough information because of the "high key"
**	indication implies both <= or < resulting in an error one way
**	or the other ( same is true with >= and >
**	06-feb-91 (stec)
**	    Changed code to call adc_hdec() instead of oph_hdec().
**	31-jan-94 (rganski)
**	    Moved this function to this module from opbcreate.c, since it is
**	    now called from oph_sarglist as well as opb_ikey, and it is a
**	    histogram-type function. Made it an external function.
[@history_line@]...
*/
PTR
oph_hvalue(
	OPS_SUBQUERY       *subquery,
	DB_DATA_VALUE      *histdt,
	DB_DATA_VALUE      *valuedt,
	PTR                key,
	ADI_OP_ID	   opid)
{
    bool		hist_process;
    OPG_CB		*opg_cb;

    opg_cb = subquery->ops_global->ops_cb->ops_server;
    /* JRBCMT should we chechk prec == prec too? */
    /* JRBNOINT -- Can't integrate this if we do */
    if ((opid == opg_cb->opg_lt)
	||
	(opid == opg_cb->opg_ge))
	hist_process = TRUE;	    /* make this equivalent
				    ** to <= for histogram processing
				    ** since this is what oph_interpolate
				    ** expects */
    else if ((histdt->db_datatype == valuedt->db_datatype) &&
	(histdt->db_length == valuedt->db_length))
	return(key);		    /* return key if datatypes match */
    else
	hist_process = FALSE;
    {	/* create a new histogram element if the datatypes do not match */
	DB_STATUS	       helemstatus;	/* ADF return status */
	DB_DATA_VALUE          adc_dvhg; /* data value for histogram element*/
	DB_DATA_VALUE	       adc_dvfrom; /* data value for key to convert*/
	i4		       dt_bits;

	/* copy the datatypes in case the db_data is in use */
	STRUCT_ASSIGN_MACRO((*histdt), adc_dvhg);
	adc_dvhg.db_data = opu_memory( subquery->ops_global,
	    (i4) adc_dvhg.db_length);
	STRUCT_ASSIGN_MACRO((*valuedt), adc_dvfrom);
	adc_dvfrom.db_data = key;
	helemstatus = adi_dtinfo(subquery->ops_global->ops_adfcb,
				adc_dvfrom.db_datatype, &dt_bits);
	if ((helemstatus == E_DB_OK) && (dt_bits & AD_NOHISTOGRAM))
	{
	    MEcopy((PTR)OPH_NH_HELEM, adc_dvhg.db_length, (PTR)adc_dvhg.db_data);
	}
	else
	{
	    helemstatus = adc_helem( subquery->ops_global->ops_adfcb,
					&adc_dvfrom, &adc_dvhg );
	}
# ifdef E_OP0785_ADC_HELEM
	if (helemstatus != E_DB_OK)
	    opx_verror( helemstatus, E_OP0785_ADC_HELEM, 
		subquery->ops_global->ops_adfcb->adf_errcb.ad_errcode);
# endif
	{
	    DB_STATUS	    hist_status;
	    if (hist_process)
	    {
		hist_status = adc_hdec(subquery->ops_global->ops_adfcb,
		    &adc_dvhg, &adc_dvhg);

# ifdef E_OP0785_ADC_HELEM
		if (DB_FAILURE_MACRO(hist_status))
		    opx_error( E_OP078B_ADC_HDEC );
# endif
	    }
	}
	return( adc_dvhg.db_data );
    }
}

/*{
** Name: oph_sarglist	- create list of keys for histogram
**
** Description:
**      Create an ordered list of sargable keys that match the equivalence
**      class, type, and length of an attribute's histogram.  This list will be
**      used to modify the histogram by splitting up the cells on these values.
**      This will cause more estimates from histogram integrating to be either
**      0 or 100 percent per cell which will avoid the problems reported in bug
**      37172.
**
**	Called by oph_findkeys
**	
** Inputs:
**      subquery                Ptr to subquery state
**	attrp			Ptr to attribute
**
** Outputs:
**	keylistpp		Ptr to ordered list of keys that apply to attribute
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	Creates list of keys starting at *keylistpp. Allocates memory
**	for this list.
**
** History:
**      7-may-91 (seputis)
**          initial creation
**      23-jul-91 (seputis)
**          -b38728 fix unix access violations in run_all tests
**      20-jun-92 (seputis)
**          - as part of code walk-thru "=" keys were being replaced
**	    by inequality keys if they pointed to the same value and
**	    the reverse should be true
**	31-jan-94 (rganski)
**	    Changed to look at one attribute at a time. Because of Character
**	    Histogram Enhancements changes, we don't know the length of an
**	    attribute's histogram values until the histogram is read from the
**	    catalog; thus we can't create the keylists for all histograms in
**	    advance (which is what this function used to do). Creates only the
**	    key list for the given attribute.
**	    Added call to oph_hvalue in the case where the datatype is a
**	    character type and the attribute length is greater than the
**	    histogram length. This allows the histogram value to be created
**	    with the correct length, as read from the iihistogram catalog (a
**	    histogram value was created in opb_ikey, but without knowledge of
**	    the correct length - it used the attribute length).
**	    Cosmetic changes to comments.
**	20-apr-98 (inkdo01)
**	    Minor fix to tolerate "constant" Boolean factors (such as 
**	    introduced by iihistograms view).
**	22-oct-00 (inkdo01)
**	    Drop earlier changes for composite histograms.
[@history_line@]...
[@history_template@]...
*/
static VOID
oph_sarglist(
	OPS_SUBQUERY    *subquery,
	OPZ_ATTS	*attrp,
	OPB_BFARG	**keylistpp)
{
    OPS_STATE		*global;
    OPE_IEQCLS		atteqc;		/* Attribute's equivalence class */
    OPB_IBF		bfcount;	/* number of boolean factors */
    OPB_BOOLFACT	**bfpp;		/* ptr array of to boolean factor 
					** ptrs being analyzed */

    global	= subquery->ops_global;
    atteqc	= attrp->opz_equcls;

    /* Search through subquery's boolean factor array for attribute's
    ** equivalence class. Then search through the key lists of the relevant
    ** boolean factors
    */
    bfcount	= subquery->ops_bfs.opb_bv;
    for (bfpp = &subquery->ops_bfs.opb_base->opb_boolfact[0]; bfcount--; bfpp++)
    {
	OPE_IEQCLS	bfeqc;	 /* Boolean factor's equivalence class */
	OPB_BFKEYINFO	*bfkeyp; /* Ptr to a key list in the boolean factor */

	/* See if this boolean factor references the attribute's equivalence
	** class */
	bfeqc = (*bfpp)->opb_eqcls;
	if (bfeqc > OPE_NOEQCLS)
	{
	    /* Boolean factor has single equivalence class */
	    if (bfeqc != atteqc)
		continue;	/* Different equivalence class. Go to next
				** boolean factor */
	}
	else if (bfeqc == OPB_BFMULTI)
	{
	    /* Boolean factor has multiple equivalence classes. See if
	    ** attribute's eqc is in the boolean factor's eqc map */
	    if (!BTtest((i4)atteqc, (char *)&(*bfpp)->opb_eqcmap))
		continue;	/* Not in eqc map. Go to next boolean factor */
	}
	else if (bfeqc == OPB_BFNONE)
	{
	    /* Probably a constant BF - not interesting. */
	    continue;
	}
	else
	{
	    opx_error(E_OP0498_NO_EQCLS_FOUND);	/* At this point, boolean
						** factors should have eqcs */
	}
	
	/* Search through boolean factor's key lists */
	for (bfkeyp = (*bfpp)->opb_keys; bfkeyp; bfkeyp = bfkeyp->opb_next)
	{
	    if ((bfkeyp->opb_eqc == atteqc)
		&&
		(bfkeyp->opb_bfhistdt->db_datatype 
		 == 
		 attrp->opz_histogram.oph_dataval.db_datatype)
		&&
		(bfkeyp->opb_bfhistdt->db_length 
		 == 
		 attrp->opz_histogram.oph_dataval.db_length))
	    {
		/* Found boolean factor key list with matching equivalence
		** class, type and length */
		OPB_BFVALLIST	    *keylistp;
		OPB_BFARG	    **sargpp;
		DB_DATA_VALUE       adc_dv1;
		DB_DATA_VALUE       adc_dv2;

		STRUCT_ASSIGN_MACRO(attrp->opz_histogram.oph_dataval, adc_dv1);
		STRUCT_ASSIGN_MACRO(adc_dv1, adc_dv2);
		for (keylistp = bfkeyp->opb_keylist; keylistp; 
		    keylistp = keylistp->opb_next)
		{   /* place the keys into the list of the attribute in sorted
		    ** order, by doing a sort merge of the two lists */
		    i4                     adc_cmp_result;
		    
		    if (keylistp->opb_hvalue)
		    {	/* If character type and attribute length is greater
			** than histogram length, re-create histogram value of
			** the key, based on histogram info */
			if ((adc_dv1.db_datatype == DB_CHA_TYPE ||
			     adc_dv1.db_datatype == DB_BYTE_TYPE) &&
			    attrp->opz_dataval.db_length > adc_dv1.db_length)
			{
			    keylistp->opb_hvalue =
				oph_hvalue(subquery, bfkeyp->opb_bfhistdt,
				   bfkeyp->opb_bfdt, keylistp->opb_keyvalue,
				   keylistp->opb_opno);
			    if (!keylistp->opb_hvalue)
				continue;
			}
		    }
		    else
			continue;	/* No histogram value to insert */

		    adc_dv2.db_data = keylistp->opb_hvalue;
		    
		    adc_cmp_result = 1;	/* set to non zero so that result
					** will be linked in */
		    
		    for ( sargpp = keylistpp;
			*sargpp; sargpp = &(*sargpp)->opb_bfnext)
		    {   /* compare until a greater value is found if equal then
			** throw it away */
			DB_STATUS            comparestatus;

			adc_dv1.db_data = (*sargpp)->opb_valp->opb_hvalue;
			comparestatus = adc_compare(global->ops_adfcb,
			    &adc_dv1, &adc_dv2, &adc_cmp_result);
			if (comparestatus != E_DB_OK)
			    opx_verror( comparestatus, E_OP078C_ADC_COMPARE,
				global->ops_adfcb->adf_errcb.ad_errcode);
			if (!adc_cmp_result
			    &&
			    ((*sargpp)->opb_valp->opb_opno != keylistp->opb_opno)
			    &&
   			    (	keylistp->opb_opno
   				== 
				global->ops_cb->ops_server->opg_eq
			    ))
			{   /* check operator for purposes of ordering, note
			    ** that the histogram values have been "adjusted"
			    ** in oph_hvalue by decrementing values if GT or LE
			    ** is used so that 
			    ** constant < attribute values <= constant can be
			    ** made when using oph_interpolate, however, if an
			    ** equality is defined, then a new cell needs to be
			    ** created just for this value to ensure correct
			    ** histogramming - this can be done by replacing
			    ** any reference to an inequality for a constant,
			    ** by the equality */
			    (*sargpp)->opb_valp = keylistp;
			}
			if (adc_cmp_result >= 0)
			    break;	/* value needs to be placed before this
					** element - or it is equal to this
					** element and must be discarded */
		    }
		    if (adc_cmp_result)
			/* if value is equal skip this this element
			** FIXME - memory lost here */
		    {
			/* value must be inserted into the list at this point
			** so the values are in increasing order */
			OPB_BFARG	*newsargp;

			newsargp = (OPB_BFARG *)opu_memory(global, 
			    (i4)sizeof(*newsargp));
			newsargp->opb_bfnext = *sargpp;
			*sargpp = newsargp;
			newsargp->opb_valp = keylistp;
		    }
		}
            break;	/* Found matching key list in this boolean factor */
	    }
	}
    }
}

/*{
** Name: oph_compkeybld	- assemble concatenated key values for insertion
**	into composite histograms and use in selectivity computation
**
** Description:
**      Recursively analyzes single attribute OPB_VALLIST entries for 
**	Boolean factors which map composite histogram. Builds 2 lists of
**	concatenated keys representing the useful Boolean factors w.r.t.
**	the composite histogram. One list is ordered by key value and is 
**	used by oph_addkeys to augment the histogram value cells. The other
**	is in the order of the relevant Boolean factors and incorporates
**	the Boolean connectives (AND/OR) linking each concatenated key to
**	the others for the chosen factors. The unordered list is used in
**	ophrelse.c to compute the selectivity of the set of Boolean factors
**	mapping the composite histogram.
**
**	Note that only one set of OPB_BFVALLIST structures and accompanying
**	concatenated key values is allocated and built (representing the 
**	unordered list). The ordered list is implemented by a separately 
**	allocated chain of OPB_BFARG structures which address the 
**	OPB_BFVALLISTs in sorted sequence. 
**
**	Called by oph_compsarg and itself - one level of recursion for each
**	attribute of the composite histogram contributing to the 
**	concatenated keys.
**	
** Inputs:
**      subquery                Ptr to subquery state
**	keylistpp		Ptr to ptr to head of ordered OPB_BFARG list
**	curargpp		Ptr to ptr to current OPB_BFARG being built
**	histp			Ptr to composite histogram
**	offset			Offset of chunk of key being built
**	level			Recursion level (or index into attr vector)
**	attrvec			Ptr to i2 array of attributes in histogram
**	attcount		Count of attributes in composite histogram
**	totallen		Length of concatenated key
**
** Outputs:
**	keylistpp		Ptr to ptr to OPB_BFARG last formatted
**	histp
**	    oph_composite.oph_vallistp Ptr to ptr to OPB_BFVALLIST list
**	    oph_composite.oph_bfacts  Map of BFs applicable to histogram
**
**	Returns:
**	    TRUE	if attribute at current level has no useful BFs
**	    FALSE	if attribute at current level has at least one useful BF
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      18-may-00 (inkdo01)
**          initial creation for composite histograms project
**	4-oct-00 (inkdo01)
**	    Add code to treat low order non-equality compares as ADC_KRANGEKEYs.
**	22-oct-00 (inkdo01)
**	    Rewritten for simplicity and to produce both ordered list of
**	    concatenated keys, and unordered list with Boolean connectors.
**	26-apr-05 (inkdo01)
**	    Do null check before copying from opb_hvalue to fix 114408.
**	12-sep-05 (inkdo01)
**	    Handle varchar/varbyte columns inside composite histogram.
**	 4-may-07 (hayke02)
**	    Modify the previous change (fix for bug 115184) so now we simply
**	    decrement attrlen by DB_CNT_SIZE (2 bytes) if the attribute is
**	    varchar or byte varying. This prevents attrlen being decremented
**	    by a further DB_CNT_SIZE after being decremented by the (offset +
**	    attrlen > totallen) test, and also during multiple iterations of
**	    the klistp 'for' loop. This change fixes bug 118236.
**	11-jan-2008 (dougi)
**	    Changes to properly handle "between" on lower order columns (bug
**	    119721).
**	20-Mar-2008 (kibro01) b120121
**	    Only set the 2nd value to NULL in the precise case where two 
**	    BFs are being combined into a BETWEEN.  Leave it alone for the
**	    remainder of the histogram.
*/
static bool
oph_compkeybld(
	OPS_SUBQUERY    *subquery,
	OPB_BFARG	**arglistpp,
	OPB_BFARG	**curargpp,
	OPH_HISTOGRAM	*histp,
	i4		offset,
	i4		level,
	i2		*attrvec,
	i4		attcount,
	i4		totallen)

{
    OPB_BFT	*bfbase = subquery->ops_bfs.opb_base;
    OPB_BOOLFACT **bfpp;
    OPB_IBF	bfcount;
    OPB_BFVALLIST *klistp, *valp;
    char	*keyaddr;
    OPZ_ATTS	*attrp;
    OPG_CB	*opg_cb = subquery->ops_global->ops_cb->ops_server;
    OPE_IEQCLS	attreqc;

    i4		attrlen, alignment;
    ADI_OP_ID	opno;
    bool	allocarg = FALSE, nolower = FALSE, found = FALSE;
    bool	found_lte = FALSE, found_gte = FALSE, found_between = FALSE;
    bool	between_starting_now = FALSE;

    /* Initialize some local variables. */
    attrp = subquery->ops_attrs.opz_base->opz_attnums[attrvec[level]];
				/* attr to add at this level */
    attreqc = attrp->opz_equcls;
    attrlen = attrp->opz_dataval.db_length;
    if ((attrp->opz_dataval.db_datatype == DB_VCH_TYPE)
	||
	(attrp->opz_dataval.db_datatype == DB_VBYTE_TYPE))
	attrlen -= DB_CNTSIZE;
    if (offset + attrlen > totallen) attrlen = totallen - offset;
				/* in case last attr only
				** contributes a bit to the conc key */
    if (attcount <= level + 1) 
	nolower = TRUE;

    /* Outer loop locates BFs referencing current attribute. */

    for (bfpp = &bfbase->opb_boolfact[0], bfcount = 0;
		bfcount < subquery->ops_bfs.opb_bv;
		bfcount++, bfpp++)
    {
	OPB_BFKEYINFO	*bfkeyp;
	bool	dropbfk, firstval, oneval;

	dropbfk = FALSE;
	if (attreqc != (*bfpp)->opb_eqcls) continue;
	if ((bfkeyp = (*bfpp)->opb_keys) == (OPB_BFKEYINFO *) NULL) 
		continue;

	/* Loop over BFKEYINFO chain, looking for one of correct type. */

	for (; bfkeyp; bfkeyp = bfkeyp->opb_next)
	{
	    /* Check equclass, type and length against current attribute. */
	    if (bfkeyp->opb_eqc == attreqc &&
		bfkeyp->opb_bfhistdt->db_datatype ==
			attrp->opz_dataval.db_datatype &&
		bfkeyp->opb_bfhistdt->db_length == 
			attrp->opz_dataval.db_length)
	    /* Then assure all entries in value list are "=" or
	    ** range operators (and we're at the 2nd attr or more). */
	     for (klistp = bfkeyp->opb_keylist, dropbfk = FALSE; 
			klistp && !dropbfk; klistp = klistp->opb_next)
	      if (klistp->opb_opno == (ADI_OP_ID)opg_cb->opg_eq ||
		(level >= 1 && 
		 (klistp->opb_opno == (ADI_OP_ID)opg_cb->opg_lt ||
		  klistp->opb_opno == (ADI_OP_ID)opg_cb->opg_le ||
		  klistp->opb_opno == (ADI_OP_ID)opg_cb->opg_ge ||
		  klistp->opb_opno == (ADI_OP_ID)opg_cb->opg_gt))) continue;
	      else dropbfk = TRUE;
	    else continue;

	    if (!dropbfk) break;	/* got matching keylist */
	}

	if (bfkeyp == (OPB_BFKEYINFO *) NULL) continue;

	/* We have an applicable Boolean factor with one or more sargable 
	** constants. If at top level (left most column in composite
	** histogram), new BFARG/BFVALLIST structures are needed. Otherwise,
	** value(s) are concatenated to existing structure. 
	**
	** Loop over values in current BF (representing ORs). */

	for (klistp = bfkeyp->opb_keylist, firstval = TRUE, oneval = FALSE;
		klistp; klistp = klistp->opb_next, firstval = FALSE)
	{
	    OPB_BFARG	*newargp;
	    OPB_BFVALLIST *newvalp;

	    if (klistp->opb_hvalue == (PTR) NULL)
		continue;		/* must be valued entry (not parm) */

	    BTset((u_i2)bfcount, (PTR)&histp->oph_composite.oph_bfacts);
					/* add BF to list */
	    found = TRUE;		/* show value found at this level */

	    if (firstval && klistp->opb_next == (OPB_BFVALLIST *) NULL)
		oneval = TRUE;		/* only one value in keylist */

	    /* New BFARG/BFVALLIST structure is required if this is high
	    ** order attribute of composite histogram, a range predicate
	    ** or the low order attribute and this is NOT the first value
	    ** in the BF's value list. */

	    if (level == 0 || !firstval ||
			klistp->opb_opno != (ADI_OP_ID)opg_cb->opg_eq)
	    {
		if (klistp->opb_opno == (ADI_OP_ID)opg_cb->opg_lt ||
		    klistp->opb_opno == (ADI_OP_ID)opg_cb->opg_le)
		    found_lte = TRUE;
		else if (klistp->opb_opno == (ADI_OP_ID)opg_cb->opg_gt ||
		    klistp->opb_opno == (ADI_OP_ID)opg_cb->opg_ge)
		    found_gte = TRUE;

		newargp = oph_compargalloc(subquery, totallen);
		newvalp = newargp->opb_valp;
		if (firstval) newvalp->opb_connector = OPB_BFAND;
		else newvalp->opb_connector = OPB_BFOR;
		newvalp->opb_opno = klistp->opb_opno;
		newvalp->opb_operator = klistp->opb_operator;

		if ((*arglistpp) == (OPB_BFARG *) NULL) 
		{
		    /* Must be first BFARG. Stuff both BFARG and BFVALLIST
		    ** addrs in caller. */
		    *curargpp = *arglistpp = newargp;
		    histp->oph_composite.oph_vallistp = newargp->opb_valp;
		}
		else
		{
		    if (level != 0) MEcopy((PTR)(*curargpp)->opb_valp->opb_hvalue, 
					offset, (PTR)newvalp->opb_hvalue);
					/* copy prefix to attr's value */
		    (*curargpp)->opb_valp->opb_next = newvalp;
					/* link to VALLIST chain */
		    if (klistp->opb_opno == (ADI_OP_ID)opg_cb->opg_eq)
			*curargpp = newargp;
		}
		
	    }

	    valp = (*curargpp)->opb_valp;
	    keyaddr = (char *)valp->opb_hvalue + offset;

	    /* For "=" predicate, copy value to this attr's offset in the
	    ** key buffer. If there's another attr in the composite histo,
	    ** recurse to build next chunk. */

	    if (klistp->opb_opno == (ADI_OP_ID)opg_cb->opg_eq)
	    {
		MEcopy((PTR)klistp->opb_hvalue, attrlen, keyaddr);
		if (!nolower)
		    nolower = oph_compkeybld(subquery, arglistpp, 
			curargpp, histp, offset+attrlen, level+1, 
			attrvec, attcount, totallen);
		if (nolower && level + 1 < attcount)
		{
		    /* Not all attrs of composite histogram are 
		    ** covered. This becomes like a "between" 
		    ** predicate, which we treat as a ADC_KRANGEKEY.
		    */
		    MEfill((totallen - (offset + attrlen)),
			(u_char) 0x00, (PTR) keyaddr + attrlen);
					/* extend low bound with 0's */
		    valp->opb_operator = ADC_KRANGEKEY;
		    valp->opb_opno = (ADI_OP_ID)opg_cb->opg_ge;

		    /* Allocate another BFARG/BFVALLIST and load
		    ** as upper bound. */

		    newargp = oph_compargalloc(subquery, totallen);
		    newvalp = newargp->opb_valp;
		    (*curargpp)->opb_valp->opb_next = newvalp;
		    MEcopy((PTR)valp->opb_hvalue, totallen, 
				(PTR)newvalp->opb_hvalue);
		    keyaddr = (PTR)newvalp->opb_hvalue+offset+attrlen;
		    MEfill((totallen - (offset + attrlen)),
			(u_char) 0xff, (PTR) keyaddr);
					/* extend low bound with f's */
		    newvalp->opb_operator = ADC_KRANGEKEY;
		    newvalp->opb_opno = (ADI_OP_ID)opg_cb->opg_le;
		    newvalp->opb_connector = OPB_BFAND;
		}
	    }

	    /* For range predicate, check first if there's another Boolean
	    ** factor that "closes" the "between". */
	    if (oneval && (found_gte || found_lte))
	    {
		i4	bf1count;
		OPB_BOOLFACT	*bf1p;
		OPB_BFKEYINFO	*bf1keyp;

		/* Loop over remaining BFs. */
		for (bf1count = bfcount+1; bf1count < subquery->ops_bfs.
			opb_bv && (!found_between); bf1count++)
		{
		    bf1p = bfbase->opb_boolfact[bf1count];
		    if (attreqc != bf1p->opb_eqcls ||
			(bf1keyp = bf1p->opb_keys) == (OPB_BFKEYINFO *) NULL)
			continue;		/* skip if not relevant */

		    /* Loop over keylist, looking for match. */
		    for ( ;bf1keyp && (!found_between); bf1keyp =
				bf1keyp->opb_next)
		    {
			/* Check for column & type. */
			if (bf1keyp->opb_eqc == attreqc &&
			    bf1keyp->opb_bfhistdt->db_datatype ==
				attrp->opz_dataval.db_datatype &&
			    bf1keyp->opb_bfhistdt->db_length ==
				attrp->opz_dataval.db_length)
			 /* Check for between. */
			 if ((found_gte && (bf1keyp->opb_keylist->opb_opno ==
					(ADI_OP_ID)opg_cb->opg_lt ||
				bf1keyp->opb_keylist->opb_opno ==
					(ADI_OP_ID)opg_cb->opg_le)) ||
			     (found_lte && (bf1keyp->opb_keylist->opb_opno ==
					(ADI_OP_ID)opg_cb->opg_gt ||
				bf1keyp->opb_keylist->opb_opno ==
					(ADI_OP_ID)opg_cb->opg_ge)))
			 {
			    found_between = TRUE;
			    between_starting_now = TRUE;
			 }
		    }
		}
	    }

	    /* For range predicate, copy value to this attr's offset in the
	    ** key buffer. Depending on which operator, fill out the buffer
	    ** with 0's or f's. Then, if next entry in value chain is 
	    ** opposite operator, make 2nd key buffer with reverse operator.
	    ** Otherwise, copy buffer and fill to end with f's or 0's (the
	    ** reverse of the first). Both cases are flagged as ADC_KRANGEKEY.
	    */

	    if (klistp->opb_opno != (ADI_OP_ID)opg_cb->opg_eq)
	    {
		OPB_BFVALLIST	*nextklistp;
		PTR	key1addr;
		i4	fill1len;
		char	fillchar1, fillchar2;
		bool	reverse = FALSE, between = FALSE;

		/* Copy attr's value to end of buffer, then dup for range. */
		MEcopy((PTR)klistp->opb_hvalue, attrlen, keyaddr);
		MEcopy((PTR)valp->opb_hvalue, offset + attrlen,
			(PTR)newvalp->opb_hvalue);
		valp->opb_opno = klistp->opb_opno;
		key1addr = (PTR)newvalp->opb_hvalue + offset;

		reverse = (klistp->opb_opno == (ADI_OP_ID)opg_cb->opg_lt ||
			klistp->opb_opno == (ADI_OP_ID) opg_cb->opg_le);

		/* Establish the fill characters for the current entry 
		** (if this isn't the rightmost column in the histogram)
		** and for the lower/upper bound (if there isn't a natural
		** between in the query). */
		fillchar1 = (reverse) ? 0xff : 0x00;
		fillchar2 = (reverse) ? 0x00 : 0xff;

		if (totallen > (offset + attrlen))
		    MEfill((totallen - (offset + attrlen)),
			(u_char)fillchar1, (PTR)keyaddr + attrlen);
					/* only if there's stuff to copy */

		if (!found_between)
		{
		    /* There's no natural between - make another value
		    ** entry with 0's or f's, depending on whether it is
		    ** lower or upper bound. */
		    if (reverse) 
			newvalp->opb_opno = (ADI_OP_ID)opg_cb->opg_ge;
		    else newvalp->opb_opno = (ADI_OP_ID)opg_cb->opg_le;

		    key1addr = key1addr - attrlen;
		    fill1len = totallen - offset;
		
		    MEfill(fill1len, (u_char)fillchar2, 
			(PTR)key1addr + attrlen);
		
		    if (reverse)
		    {
			OPB_BFVALLIST	*wvalp;

			/* Switch sequence of range predicates. */
			newvalp->opb_next = valp;
			valp->opb_next = (OPB_BFVALLIST *) NULL;
			if (histp->oph_composite.oph_vallistp &&
			    histp->oph_composite.oph_vallistp == valp)
			    histp->oph_composite.oph_vallistp = newvalp;
			 else for (wvalp = histp->oph_composite.oph_vallistp;
					 wvalp; wvalp = wvalp->opb_next)
			 if (wvalp->opb_next == valp)
			 {
			    wvalp->opb_next = newvalp;
			    break;
			 }
		    }

		    valp->opb_operator = ADC_KRANGEKEY;
		    newvalp->opb_operator = ADC_KRANGEKEY;
		}
		else
		{
		    /* There is a natural between (matching </<= and >=/>
		    ** restrictions). See if the "other" one is in the value
		    ** list yet, and assure they're in sequence. 
		    **
		    ** Note that </<= (a.k.a. "reverse") don't require work
		    ** because if matching >=/> is already in the list, new
		    ** one is positioned correctly, and if it isn't we don't
		    ** have to worry until it is added to the list. */

		    valp->opb_operator = ADC_KRANGEKEY;
		    if (between_starting_now)
		        valp->opb_next = NULL;	/* newvalp will be added later */

		    between_starting_now = FALSE;

		    if ((!reverse) && histp->oph_composite.oph_vallistp &&
			histp->oph_composite.oph_vallistp != valp)
		    {
			/* If there are values in list (other than the 
			** current one), look for </<= so >=/> can be 
			** inserted ahead of it. */
			if (histp->oph_composite.oph_vallistp->opb_opno ==
				(ADI_OP_ID)opg_cb->opg_lt ||
			    histp->oph_composite.oph_vallistp->opb_opno ==
				(ADI_OP_ID)opg_cb->opg_le)
			{
			    valp->opb_next = histp->oph_composite.oph_vallistp;
			    histp->oph_composite.oph_vallistp->opb_next = NULL;
			    histp->oph_composite.oph_vallistp = valp;
			}
		    }
		    else if (reverse && histp->oph_composite.oph_vallistp)
			histp->oph_composite.oph_vallistp->opb_next = valp;
		}
	    }

	    if (nolower)
	    {
		/* Place new BFarg(s) in correct sequence. */
		oph_compargorder(subquery, *curargpp, arglistpp, totallen);

		if (klistp->opb_opno != (ADI_OP_ID)opg_cb->opg_eq ||
			attcount > level + 1)
		{
		    /* 2 BFARGs to order. */
		    oph_compargorder(subquery, newargp, arglistpp, totallen);
		    *curargpp = newargp;		/* for next loop */
		}
	    }

	}
    }

    return (!found);

}


/*{
** Name: oph_compsarg	- create list of composite keys for composite
**	histogram
**
** Description:
**      Coordinates creation of 2 lists of concatenated, sargablke keys which
**	match the equivalence classes, types, and lengths of the attributes of a 
**	composite histogram. The keys will be chosen from relevant boolean 
**	factors where they may be useful in conjunction with the composite 
**	histogram. One of the lists is ordered and is used by oph_addkeys to
**	augment the histogram with all values used in selectivity determination.
**	The other list is in the sequence of the originating Boolean factors
**	and incorporates the Boolean connectors (AND/OR) used to combine 
**	them. This is essential to the selectivity computation.
**
**	Called by oph_findkeys
**	
** Inputs:
**      subquery                Ptr to subquery state
**	histp			Ptr to composite histogram
**	intervalp		Ptr to OPH_INTERVAL for histogram
**
** Outputs:
**	keylistpp		Ptr to ordered list of concatenated keys that 
**				apply to histogram
**	histp->oph_composite.oph_vallistp	Ptr to OPB_BFVALLIST chain of
**				concatenated keys (c/w Boolean connectors)
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	Creates list of keys starting at *keylistp. Allocates memory
**	for this list.
**
** History:
**      17-may-00 (inkdo01)
**          initial creation for composite histograms project
**	27-sep-00 (inkdo01)
**	    Add logic to map BFs which are relevant to composite histograms.
**	3-oct-00 (inkdo01)
**	    Chain concatenated key OPB_BFVALLIST instances together.
**	19-oct-00 (inkdo01)
**	    Build 2nd list of BFARGs per attribute unordered from sarglist
**	    to reflect AND/OR relationship of the factors.
**	22-oct-00 (inkdo01)
**	    Rewritten to deal with need for both lists of concatenated keys.
[@history_line@]...
[@history_template@]...
*/
static VOID
oph_compsarg(
	OPS_SUBQUERY    *subquery,
	OPH_HISTOGRAM	*histp,
	OPH_INTERVAL	*intervalp,
	OPB_BFARG	**keylistpp)
{
    OPB_BFARG	*bfargp;
    OPZ_IATTS	attno;
    OPZ_AT	*abase;
    i4		attcount, cumlen, histlen;
    i2		*attrvec;

    /* Count number of attrs in value cells of histogram. Set up various
    ** parameters, then call oph_compkeybld to construct concatenated key
    ** lists. */

    abase = subquery->ops_attrs.opz_base;
    attrvec = &histp->oph_composite.oph_attrvec[0];
    histlen = histp->oph_composite.oph_interval->oph_dataval.db_length;

    for (attcount = 0, cumlen = 0; 
	(attno = attrvec[attcount]) != OPZ_NOATTR && cumlen < histlen; 
	cumlen += abase->opz_attnums[attno]->opz_dataval.db_length, attcount++);
					/* attr counting loop */

    histp->oph_composite.oph_vallistp = (OPB_BFVALLIST *) NULL;
    *keylistpp = (OPB_BFARG *) NULL;

    oph_compkeybld(subquery, keylistpp, &bfargp,
	histp, 0, 0, attrvec, attcount, histlen);
    histp->oph_composite.oph_bfcount = 
	BTcount((PTR)&histp->oph_composite.oph_bfacts, (u_i2)sizeof(OPB_BMBF));
					/* count applicable BFs */

}


/*{
** Name: oph_compargorder - places OPB_BFARG parameter in sequence 
**	according to the key value in its OPB_BFVALLIST
**
** Description:
**	Concatenated key value in OPB_BFVALLIST addressed by OPB_BFARG 
**	parameter is used to place BFARG in sequence (for oph_addkey logic)
**	
** Inputs:
**      subquery                Ptr to subquery state
**	bfargp			Ptr to OPB_BFARG to add to list
**	arglistpp		Ptr to ptr to current head of OPB_BFARG list
**	keylen			Size of concatenated key buffer
**
** Outputs:
**
**	Returns:
**	    Ordered list of OPB_BFARGs
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-oct-00 (inkdo01)
**	    Written for rewrite of concatenated key build.
[@history_line@]...
[@history_template@]...
*/
static VOID
oph_compargorder(
	OPS_SUBQUERY	*subquery,
	OPB_BFARG	*bfargp,
	OPB_BFARG	**arglistpp,
	i4		keylen)

{
    OPB_BFVALLIST	*valp, *val1p;
    OPB_BFARG		*curargp, *prevargp;
    DB_DATA_VALUE	adc_dv1, adc_dv2;
    DB_STATUS		comparestatus;
    i4			adc_cmp_result;

    /* Setup argument value, then loop over arglist searching for insertion
    ** point. */

    if (*arglistpp == bfargp) return;		/* 1st entry - no ordering */

    adc_dv1.db_datatype = DB_CHA_TYPE;
    adc_dv1.db_length = keylen;
    adc_dv1.db_data = bfargp->opb_valp->opb_hvalue;
    STRUCT_ASSIGN_MACRO(adc_dv1, adc_dv2);
				/* init adc_dv2 */

    for (curargp = *arglistpp, prevargp = (OPB_BFARG *) NULL; curargp; 
			prevargp = curargp, curargp = curargp->opb_bfnext)
    {
	adc_dv2.db_data = curargp->opb_valp->opb_hvalue;
	comparestatus = adc_compare(subquery->ops_global->ops_adfcb, 
				&adc_dv1, &adc_dv2, &adc_cmp_result);
	if (comparestatus != E_DB_OK)
		opx_verror(comparestatus, E_OP078C_ADC_COMPARE,
		    subquery->ops_global->ops_adfcb->adf_errcb.ad_errcode);
	if (adc_cmp_result == 0) return;	/* drop dups from list */
	if (adc_cmp_result < 0) break;		/* found the spot */
    }

    /* Insert here */
    if (prevargp == (OPB_BFARG *) NULL)
    {
	/* Head of list. */
	bfargp->opb_bfnext = *arglistpp;
	*arglistpp = bfargp;
	return;
    }

    /* Middle of list. */
    bfargp->opb_bfnext = prevargp->opb_bfnext;
    prevargp->opb_bfnext = bfargp;
}


/*{
** Name: oph_compargalloc - allocates a BFARG/BFVALLIST/key buffer for 
**	compoosite histogram value processing
**
** Description:
**	
** Inputs:
**      subquery                Ptr to subquery state
**	totallen		Size of concatenated key buffer
**
** Outputs:
**
**	Returns:
**	    Ptr to OPB_BFARG (with attached BFVALLIST/key buffer)
**	Exceptions:
**	    none
**
** Side Effects:
**	Allocates memory for structures.
**
** History:
**	26-oct-00 (inkdo01)
**	    Written for rewrite of concatenated key build.
[@history_line@]...
[@history_template@]...
*/
static OPB_BFARG *
oph_compargalloc(
	OPS_SUBQUERY	*subquery,
	i4		totallen)

{
    OPB_BFARG	*newargp;
    OPB_BFVALLIST *newvalp;
    i4		alignment;


    alignment = totallen % sizeof(ALIGN_RESTRICT);
    alignment = totallen + sizeof(ALIGN_RESTRICT) - alignment;
    newargp = (OPB_BFARG *) opu_memory(subquery->ops_global,
		sizeof(OPB_BFARG) + sizeof(OPB_BFVALLIST) + 
		alignment + totallen);
    newargp->opb_bfnext = (OPB_BFARG *) NULL;
    newargp->opb_valp = (OPB_BFVALLIST *) 
			&((char *)newargp)[sizeof(OPB_BFARG)];
    newvalp = newargp->opb_valp;
    newvalp->opb_next = (OPB_BFVALLIST *) NULL;
    newvalp->opb_hvalue = &((char *)newvalp)[sizeof(OPB_BFVALLIST)];
    newvalp->opb_keyvalue = NULL;
    newvalp->opb_null = 0;

    return(newargp);
}

/*{
** Name: oph_findkeys - Find key values relevant to histogram
**
** Description:
**      This routine finds the boolean factor keys with the same datatype and
**      length as the given histogram. A list of these keys is created, based
**      at attrp->opz_keylistp. It then scans this key list for keys which are
**      greater than the first boundary value of the histogram, computing the
**      maximum number of cells which can result when those keys are added to
**      the histogram boundary values. This maximum number is based on the
**      assumption that an inequality operator can create one new cell, and an
**      equality operator can create two new cells (see description of
**      oph_addkeys for more details).
**
**      It returns a pointer to the first key that is greater than the lowest
**      boundary value of the histogram.
**
**	Called by oph_addkeys.
**
** Inputs:
**      subquery	Ptr to subquery being processed
**      attrp		Ptr to joinop attribute element
**                      associated with histogram
**	newhistp	Ptr to histogram
**	intervalp	Ptr to OPH_INTERVAL for histogram
**
** Outputs:
**	maxcellcountp	Ptr to maximum number of histogram cells after adding
**			key values.
**
**	Returns:
**	    Pointer to first key that is greater than the lowest boundary value
**	    of the histogram, or NULL if there is none.
**	Exceptions:
**	    none
**
** Side Effects:
**	Histogram value length for the relevant key list is updated, based on
**	the histogram value length retrieved from RDF.
**
** History:
**	23-aug-93 (rganski)
**          Initial creation from part of oph_catalog(), for modularity.
**	31-jan-94 (rganski)
**	    Changed to call oph_sarglist() instead of searching through
**	    keylists in the ope_sargp field of the attribute's equivalence
**	    class. oph_sarglist() now only creates a key list for the current
**	    attribute (see oph_sarglist).
**	    Removed update of histogram value length in sargp from attrp, since
**	    that point is never reached unless they are equal to begin with.
**	12-sep-96 (inkdo01)
**	    Added code for handling "<>" (ne) relops, for better selectivities.
**	16-may-00 (inkdo01)
**	    Changes for composite histogram support.
**	2-oct-00 (inkdo01)
**	    Place list of concatenated keys for composite histograms into 
**	    histogram structure (oph_composite.oph_vallistp) for use by relse.
[@history_line@]...
*/
static OPB_BFARG *
oph_findkeys(
	OPS_SUBQUERY	*subquery,
	OPZ_ATTS        *attrp,
	i4		*maxcellcountp,
	OPH_HISTOGRAM	*newhistp,
	OPH_INTERVAL	*intervalp)
{
    OPZ_ATTS	   *sargp;	/* ptr to element which	defines ordered list of
				** histogram constants used in the query */
    OPB_BFARG	   *keylistp = (OPB_BFARG *) NULL;

    /* Build list of keys in same equivalence class as this attribute, and with
    ** same datatype and length as this attribute's histogram. If this is a 
    ** composite histogram, a different function is called to build a list of
    ** concatenated keys from relevant boolean factors. 
    */
    if (newhistp->oph_mask & OPH_COMPOSITE) 
	oph_compsarg(subquery, newhistp, intervalp, &keylistp);
    else oph_sarglist(subquery, attrp, &keylistp);

    if (keylistp == (OPB_BFARG *) NULL)
	return((OPB_BFARG *) NULL);	/* No keys apply to this attribute */
    else
    {	/* Scan all the keys in the key list, find the first one which
	** falls inside the histogram, and compute the maximum number of
	** cells which can be created from this key list. Any constants
	** less than the lower bound are of no interest in selectivity
	** computation, since the 1st cell of the histogram contains 1 less 
	** than the lowest value known to be in the attribute.
	*/
	OPS_STATE	*global = subquery->ops_global;
	OPB_BFARG	*sbfargp;	/* ptr to key elements in attrp */
	OPB_BFARG	*first_sbfargp; /* ptr to first element in list
					** which is larger than the first
					** interval boundary in the histogram */
	DB_DATA_VALUE	adcmiddle; 	/* describes user specified key value
					** i.e. constant used in where clause */
	DB_DATA_VALUE	adclower; 	/* describes lowest boundary value in
					** histogram */
	
	/* Initialize structure for adc_compare with correct type information */
	STRUCT_ASSIGN_MACRO(intervalp->oph_dataval, adcmiddle);
	STRUCT_ASSIGN_MACRO(adcmiddle, adclower);

	/* Point at first boundary value */
	adclower.db_data = intervalp->oph_histmm;

	/* number of cells is at least equal to number in original histogram */
	*maxcellcountp = intervalp->oph_numcells; 
	
	first_sbfargp = NULL;
	
	for (sbfargp = keylistp;
	     sbfargp; sbfargp = sbfargp->opb_bfnext)
	{
	    if (!first_sbfargp)
	    {   /* skip over keys less than all cells in the histogram */
		i4		result1;
		DB_STATUS	status1;
		
		adcmiddle.db_data = sbfargp->opb_valp->opb_hvalue;
		/* next available histogram element for key referenced in where
		** clause */
		
		status1 = adc_compare(global->ops_adfcb, &adclower, &adcmiddle,
				      &result1);
		if (status1 != E_DB_OK)
		    opx_verror( status1, E_OP078C_ADC_COMPARE,
			       global->ops_adfcb->adf_errcb.ad_errcode);
		if (result1 < 0)
		{   /* found key value that is greater than the first histogram
		    ** boundary value */
		    first_sbfargp = sbfargp;
		}
		else
		    continue;	/* do not add cells for out of range keys */
	    }
	    
	    /* Increase maximum cell count */
	    (*maxcellcountp)++; /* each inequality has the potential to divide a
				** cell in half */
	    if (sbfargp->opb_valp->opb_opno ==
		global->ops_cb->ops_server->opg_eq ||
	        sbfargp->opb_valp->opb_opno ==
		global->ops_cb->ops_server->opg_ne); /* ADF ID for "="/"<>"
						     ** operator */
	    {   /* since an equality can potentially produce 2 cells, since the
		** "decrement" function is called to produce an "range" type
		** interval which replaces the equality operator */
		(*maxcellcountp)++;
	    }
	}

	return(first_sbfargp);
    }
}

/*{
** Name: oph_equalcells - Add histogram cells for equality predicate
**
** Description:
**	When oph_splitcell finds an equality predicate that applies to a
**	histogram cell, it calls this routine, which tries to add one
**	or more subcells to the given histogram cell, with boundary values
**	determined by the key value.
**
**	It first decrements the original key value in the equality predicate
**	and tries to create a new subcell with that decremented value as the
**	upper boundary value. If the decremented value, however, is equal to
**	the previous boundary value, then the subcell is not created. Then the
**	original key value is used as the upper boundary value for the next
**	subcell. This effectively converts the equality operator to
**	an inequality operator, by restricting it to the narrow subcell whose
**	boundaries are the original key value and the decremented key value.
**
**	The selectivity for each subcell is temporarily set to the diff between
**	the boundary values of the subcell, except for the subcell with
**	immediately adjacent boundary values, which gets the value OPH_EXACTKEY
**	as a marker. These numbers are used later, in oph_addkeys, to compute
**	the actual subcell selectivities.
**
**	For example, assume an integer histogram cell for col1 with boundaries
**	2 and 8. The original cell looks like this:
**
**				--------- 8
**				|   	|
**				|	|
**				--------- 2
**
**     If oph_splitcell comes across a predicate "col1 = 6" in the WHERE
**     clause, this routine, in conjunction with oph_splitcell, will split
**     the above cell into three cells:
**     
**				--------- 8
**				|   	|
**				--------- 6
**				|	|
**				--------- 5
**				|	|
**				--------- 2
**
**	Thus the key value in the predicate and the key value decremented by 1
**	(the highest value smaller than the key value) are added as boundary
**	values to the histogram. The "selectivities" for each subcell are,
**	going from low to high, 3, OPH_EXACTKEY, and 2.
**
**	If the predicate were "col1 = 3", only one new subcell will be added,
**	since the decremented key value is equal to one of the boundary values:
**     
**				--------- 8
**				|   	|
**				--------- 3
**				|	|
**				--------- 2
**
**	The "selectivities" will be OPH_EXACTKEY and 5.
**
**	Called by oph_splitcell
**
** Inputs:
**	global		Ptr to global OPF state structure
**      attrp		Ptr to joinop attribute element associated with this
**      		histogram
**      adckeyvaluep	Ptr to data value structure containing key value
**      		from equality predicate
**      newlower	Ptr to position at which to make first split (place to
**      		put first new boundary value)
**      celllength	Length of boundary values in this histogram
**      newcount	Pointer into count array into which cell count for
**      		first new subcell should be placed
**      sdifflength	For opn_diff: length to which diff computations should
**      		go in this cell, for character values
**      celltups	For opn_diff: number of tuples covered by this cell
**	intervalp	Ptr to OPH_INTERVAL for histogram
**
** Outputs:
**      newlower	Adds new boundary values starting at this position.
**      newcount	Adds new cell count values starting at this position.
**
**	Returns:
**	    The number of new subcells created.
**	Exceptions:
**	    None.
**
** Side Effects:
**	    None.
**
** History:
**	23-aug-93 (rganski)
**          Initial creation from part of oph_catalog(), for modularity. See
**          change history of oph_catalog() for changes relating to the
**          Character Histogram Enhancements project and others.
**	    Made change to add new boundary value if adc_compare says that the
**	    value is different, if the data type is character, even if opn_diff
**	    returns a zero. This is because opn_diff can return a zero when the
**	    values are different, and oph_interpolate uses adc_compare, not
**	    opn_diff. The selectivity in such a case will be zero, in line with
**	    the diff returned by opn_diff.
**      21-sep-98 (hayke02)
**	    After decrementing the histogram value (call to adc_hdec()), we
**	    now switch off collation before calling adc_compare() (and restore
**	    collation after). This prevents decremented and collated histogram
**	    values turning out to be greater than the key value. This change
**	    fixes bug 91425.
**      01-Jan-00 (sarjo01)
**          Bug 99120: Added new parameter tbl so collation sequence
**          can be used by opn_diff where applicable.
**	16-may-00 (inkdo01)
**	    Changes for composite histogram support.
[@history_line@]...
*/
static i4
oph_equalcells(
    OPS_STATE		*global,
    OPZ_ATTS		*attrp,
    DB_DATA_VALUE	*adckeyvaluep,
    OPH_CELLS		newlower,
    i4			celllength,
    OPN_PERCENT		*newcount,
    i4			sdifflength,
    OPO_TUPLES		celltups,
    OPH_INTERVAL	*intervalp)
{
    DB_STATUS		adc_status;	/* status returned by adc calls */
    i4		    	adc_cmp_result; /* result of adc_compare */
    DB_DATA_VALUE   	adclower; 	/* describes lower bound of current
					** subcell being analyzed */
    DB_DATA_VALUE   	adcdecvalue;	/* describes highest value below
					** adckeyvalue, i.e. the key value
					** decremented by one */
    i4			newsubcells;	/* Number of new subcells created by
					** this routine */
    PTR			adf_collation;
    union _temp
    {
	char	    	tempspace[DB_MAX_HIST_LENGTH];
	ALIGN_RESTRICT	junk;
    }   temp;				/* temporary work area for creating
					** histogram element */
    PTR			tbl = NULL;	/* collation table */

    newsubcells = 0;

    if (global->ops_gmask & OPS_COLLATEDIFF)
	tbl = global->ops_adfcb->adf_collation;

    /* initialize DB_DATA_VALUEs used in ADF calls
    */
    STRUCT_ASSIGN_MACRO(intervalp->oph_dataval, adclower);
    STRUCT_ASSIGN_MACRO(adclower, adcdecvalue);
    adcdecvalue.db_data = (PTR)&temp;	/* Buffer for decremented value */
    if (sizeof(temp) < adcdecvalue.db_length)
	opx_error(E_OP0391_HISTLEN);	/* histogram length is not within the
					** expected limits */

    /* Decrement histogram value by smallest possible increment to isolate
    ** selectivity */
    MEcopy((PTR)adckeyvaluep->db_data, adckeyvaluep->db_length,
	   (PTR)adcdecvalue.db_data);
    adc_status = adc_hdec(global->ops_adfcb, &adcdecvalue, &adcdecvalue);

# ifdef E_OP0785_ADC_HELEM
    if (DB_FAILURE_MACRO(adc_status))
	opx_error( E_OP078B_ADC_HDEC );
# endif

    /* Check if any difference occurred in the decrementation. Hopefully
    ** some change occured, but a boundary condition may exist;
    ** avoid having two histogram cells with identical values */
    /* Bug 91425 - switch off collation when doing compare */
    adf_collation = global->ops_adfcb->adf_collation;
    global->ops_adfcb->adf_collation = (PTR) NULL;
    adc_status = adc_compare(global->ops_adfcb, adckeyvaluep, &adcdecvalue,
			     &adc_cmp_result);
    global->ops_adfcb->adf_collation = adf_collation;
    if (adc_status != E_DB_OK)
	opx_verror(adc_status, E_OP078C_ADC_COMPARE,
		   global->ops_adfcb->adf_errcb.ad_errcode);
    if (adc_cmp_result > 0)
    {
	/* check that the new cell boundary is still greater than the previous
	** cell boundary, because histograms require all intervals to be
	** increasing */
	adclower.db_data = newlower - celllength;
	adc_status = adc_compare(global->ops_adfcb, &adclower, &adcdecvalue,
				 &adc_cmp_result);
	if (adc_status != E_DB_OK)
	    opx_verror(adc_status, E_OP078C_ADC_COMPARE,
		       global->ops_adfcb->adf_errcb.ad_errcode);

	if (adc_cmp_result < 0)
	{
	    /* adc_compare says that there is room for the decremented value.
	    ** Call opn_diff to see if it agrees; if the diff between the
	    ** decremented key value and the previous boundary value is greater
	    ** than zero, the decremented value will be a new boundary value,
	    ** and the cell count for the new subcell will temporarily be the
	    ** diff between the boundary values.
	    ** If the histogram type is CHAR, we create the new boundary value
	    ** even if the computed diff is zero, since this can happen even
	    ** when the two values are different. The cell count will, however,
	    ** be zero. This is necessary because oph_interpolate uses
	    ** adc_compare, not opn_diff.
	    */
	    /* Compute diff */
	    *newcount = opn_diff(adcdecvalue.db_data, newlower-celllength,
				 &sdifflength, celltups, attrp,
				 &intervalp->oph_dataval, tbl);
	    if (*newcount > 0.0 || adclower.db_datatype == DB_CHA_TYPE ||
				adclower.db_datatype == DB_BYTE_TYPE)
	    {   /* Diff is greater than zero, or character type;
		** create new subcell */
		MEcopy((PTR)adcdecvalue.db_data, celllength, 
		       (PTR)newlower);
		newlower += celllength;
		newcount++;
		newsubcells++;
	    }
	}
    }
    else if (adc_cmp_result < 0)
	opx_error( E_OP078B_ADC_HDEC ); /* expecting adc_hdec to produce
					** smaller result */
    
    /* Add  subcell for the key value itself */
    MEcopy((PTR)adckeyvaluep->db_data, celllength, (PTR)newlower);
    *newcount = OPH_EXACTKEY; /* Mark with negative number so that it can be
			      ** replaced with exact selectivity later */
    newsubcells++;

    return(newsubcells);
}

/*{
** Name: oph_newcounts - Compute selectivities for new subcells
**
** Description:
**      This routine is called when a histogram cell is split into new
**      subcells, to assign selectivities ("counts") to each new subcell. For
**      subcells created for equality predicates (exact lookups), there will be
**      one unique value in the subcell, so we try to assign a repetition
**      factor number of tuples to each subcell of this type. Whatever is left
**      over from the original cell's selectivity is then divided up among the
**      inequality (inexact) cells in proportion to the diffs between the
**      boundary values.
**
**	For example, assume an integer histogram cell for col1 with boundaries
**	2 and 8. The original cell looks like this:
**
**				--------- 8
**				|   	|
**				|	|
**				--------- 2
**
**	Assume this cell's selectivity is .4, the repetition factor is 5, and
**	the number of tuples in the table is 50.
**
**	If oph_splitcell comes across a predicate "col1 = 6" in the WHERE
**     	clause, the above cell will be split into three cells:
**     
**				--------- 8
**				|   	|
**				--------- 6
**				|	|
**				--------- 5
**				|	|
**				--------- 2
**
**	When this routine is called, the "selectivities" for each subcell are
**	temporarily, going from low to high, 3, OPH_EXACTKEY, and 2. These are
**	the diffs between the boundary values for the inexact cells and a
**	negative marker for the exact cell. This routine will replace
**	OPH_EXACTKEY with .1, which results in 5 tuples (repetition factor) in
**	the middle cell. It then divides the remaining selectivity (.3) from
**	the original cell between the inexact cells, in proportion to the
**	diffs: 3/5 to the lower cell and 2/5 to the higher cell. This gives
**	(3/5)*.3 = .18 for the lower cell, and (2/5)*.3 = .12 for the higher
**	cell. Thus the selectivities of the subcells adds up to the selectivity
**	of the original cell: .18 + .1 + .12 = .4
**
**	Called by oph_splitcell
**
** Inputs:
**      newsubcells	Number of subcells created by partition of the original
**      		cell.
**	newcount	Pointer to the temporary cell counts for each subcell
**			(see description above).
**	celltups	Number of tuples covered by the original cell
**			(original cell selectivity * tuples in relation)
**	newhistp	Pointer to histogram structure with info like
**			repetition factor etc.
**	originalcount	Selectivity of original cell.
**	dmftuples	Number of tuples in relation, currently estimated by DMF
**	origcreptf	Repetition factor for originating cell
**	newcreptf	Pointer to repetition factor entry of next cell in
**			new histogram
**
** Outputs:
**      newcount	Updates subcell counts with estimate of selectivity
**	newcreptf	Copies original repetition factor to new subcells
**
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	23-aug-93 (rganski)
**          Initial creation from part of oph_catalog(), for modularity. See
**          change history of oph_catalog() for changes relating to the
**          Character Histogram Enhancements project and others.
**	12-jan-96 (inkdo01)
**	    Added support for per-cell repetition factors.
**	24-mar-99 (wanfr01)
**	    Added check for zero diffs between split inexact histogram cells
**	    to avoid corrupting histograms via divide by zero.  Bug 95384,
**	    INGSRV 721
[@history_line@]...
*/
static VOID
oph_newcounts(
    i4			newsubcells,
    OPN_PERCENT		*newcount,
    OPO_TUPLES		celltups,
    OPH_HISTOGRAM	*newhistp,
    OPN_PERCENT		originalcount,
    OPO_TUPLES		dmftuples,
    OPO_TUPLES		origcreptf,
    OPO_TUPLES		*newcreptf)
{
    OPN_PERCENT		exactsel;   /* selectivity to be assigned to each new
				    ** cell which represents an exact key
				    ** lookup (equality predicate) */
    OPN_PERCENT		inexactsel; /* selectivity to be divided up
				    ** proportionally amount all cells which
				    ** are not exact key lookups */
    i4			exactcount; /* number of cells which represent exact
				    ** values, i.e. exact lookup, equality
				    ** comparison */
    OPN_PERCENT 	totaldiffs; /* total diffs of all the new subcells
				    ** which are not part of an "exact key
				    ** cell", i.e. do not include subcells
				    ** which contribute to the exactcount value
				    */
    i4			i;	    /* loop counter */
    
    /* Scan the cell counts of the new subcells, which were assigned by
    ** oph_splitcell. At this point newcount points at the count of the first
    ** new subcell. If the count is OPH_EXACTKEY, then the subcell was created
    ** for an equality operator, so increment exactcount. Otherwise, the cell
    ** count is actually the diff between the boundary values of the cell; add
    ** this subcell diff to the total diff count for the whole original cell,
    ** which is used below to compute selectivity factors for each new subcell.
    ** (OPH_EXACTKEY is -1.0, and couldn't appear naturally as a cell count.)
    */
    exactcount = 0;
    totaldiffs = 0.0;
    for (i = 0; i < newsubcells; i++)
    {
	if (newcount[i] == OPH_EXACTKEY)
	    /* This cell was created for an exact lookup */
	    exactcount++;
	else
	    totaldiffs += newcount[i];
    }
    
    /* Partition the original cell's selectivity among new subcells
    ** corresponding to exact lookups and those corresponding to inexact
    ** lookups. Exact lookups take precedence; what is left over is assigned to
    ** inexact lookups. */
    if (exactcount)
    {	/* Some exact lookups were found; use the repetition factor to assign
	** selectivity to the exact cells */
	if ((celltups < (newhistp->oph_reptf * exactcount))
	    ||
	    (exactcount >= newsubcells) /* only exact cells were created so
					** assign selectivity to them */
	    )
	    /* Assume all the selectivity occurs in the cells represented by
	    ** the selected constants equally */
	    exactsel = originalcount / (OPN_PERCENT)exactcount; 
	else
	    /* Enough to support repetition factors in the cell */
	    exactsel = origcreptf / dmftuples;
	
	/* Assign the rest of the selectivity to  the inexact cells */
	inexactsel = originalcount - (exactsel * exactcount);
	if (inexactsel < 0.0)
	    inexactsel = 0.0;
    }
    else
    {	/* No exact lookups in this cell; assign all of original selectivity to
	** inexact cells */
	inexactsel = originalcount;
	exactsel = 0.0;
    }
    
    /* Replace the diffs and exact lookup markers in the count fields of the
    ** new subcells with selectivities and store repetition factors, too. 
    ** Note: not enough information is available to do anything but insert
    ** same repetition factor as in original cell.
    */
    for (i = 0; i < newsubcells; i++)
    {
	newcreptf[i] = origcreptf;
	if (newcount[i] < 0.0)
	    /* Exact lookup marker */
	    newcount[i] = exactsel;
	else 
	{
            if (totaldiffs == 0.0) 
                newcount[i] = 0;
            else
                newcount[i] = (newcount[i]/totaldiffs) * inexactsel;
        }
    }
}

/*{
** Name: oph_splitcell - Insert key values into histogram cell
**
** Description:
**      This routine scans the list of predicate key values relevant to the
**      given histogram, and determines whether any of them fall in the range
**      of the given histogram cell. If a key value does fall in the range of
**      the cell, it attempts to insert the key value into the cell, thus
**      creating one or more new subcells, depending on the operator: for
**      inequality, the cell may be split in two; for equality the cell can be
**      split into three cells.
**
**      For an inequality operator, the histogram cell can be split in two,
**      providing the key does not fall exactly on one of the boundaries: the
**      lower cell will cover the tuples which are less than or equal to the
**      key, and the upper cell will cover the tuples which are greater than
**      the key. If the key falls exactly on an existing cell boundary, no
**      splitting is necessary.
**
**      For an equality operator, the histogram cell can be split into three
**      cells: the middle cell, whose boundaries are the key and the value
**      immediately preceding the key, will cover the tuples which are exactly
**      equal to the key; the lower cell will cover tuples which are less than
**      the key, and the upper cell will cover tuples which are greater than
**      the key. If the key is exactly equal to the original upper boundary
**      value, or if the original lower boundary value immediately precedes the
**      key value, then the cell will be split into only two cells. See
**      oph_equalcells().
**
**	The "cell count" for each new subcell is stored in the newcount array.
**	If the cell was created as a result of an equality operator, the cell
**	count stored is OPH_EXACTKEY, which is a negative marker for this
**	purpose; otherwise, the cell count stored is the diff between the
**	boundary values of the subcell. These are not true cell counts (i.e.
**	selectivity factors), but are converted to selectivity factors later in
**	oph_addkeys by dividing the subcell diff by the total diff for the
**	original cell (in the case of inequalities), or by other fancy footwork
**	for equalities (see oph_addkeys).
**
**	Cell splitting allows a consistent method for determining matches
**	over a histogram cell, regardless of the operator: a key value falls
**	into a particular cell if it is less than or equal to the upper
**	boundary and greater than the lower boundary. In this case, the
**	matching cell contains only one value.
**
**	Called by oph_addkeys.
**
** Inputs:
**	global		Ptr to global OPF state structure
**      attrp		Ptr to joinop attribute element associated with this
**      		histogram
**      sbfargpp	Ptr to ptr to first key value in list that may be
**      		relevant to this cell
**      adcupperp	Ptr to data value structure containing upper boundary
**      		value of this cell
**      cellcount	Cell count for this whole cell
**      newlower	Ptr to position at which to make first split (place to
**      		put first new boundary value)
**      celllength	Length of boundary values in this histogram
**      newcount	Pointer into count array into which cell count for
**      		first new subcell should be placed
**      sdifflength	For opn_diff: length to which diff computations should
**      		go in this cell, for character values
**      celltups	For opn_diff: number of tuples covered by this cell
**	newhistp	Pointer to histogram element read from catalog
**	dmftuples	Number of tuples in relation, currently estimated by DMF
**	origcreptf	Repetition factor for originating cell
**	newcreptf	Pointer to repetition factor entry of next cell in
**			new histogram
**	intervalp	Ptr to OPH_INTERVAL for histogram
**
** Outputs:
**      sbfargpp	Returns pointer to last key value looked at. Will be
**      		first key value looked at when splitting next cell.
**      newlower	Adds new boundary values starting at this position.
**      newcount	Adds new cell count values starting at this position.
**	newcreptf	Adds new repetition factors at this location.
**
**	Returns:
**	    The number of new subcells created.
**	Exceptions:
**	    None.
**
** Side Effects:
**	    None.
**
** History:
**	23-aug-93 (rganski)
**          Initial creation from part of oph_catalog(), for modularity. See
**          change history of oph_catalog() for changes relating to the
**          Character Histogram Enhancements project and others.
**	    Made change to add new boundary value if adc_compare says that the
**	    value is different, if the data type is character, even if opn_diff
**	    returns a zero. This is because opn_diff can return a zero when the
**	    values are different, and oph_interpolate uses adc_compare, not
**	    opn_diff. The selectivity in such a case will be zero, in line with
**	    the diff returned by opn_diff.
**	12-jan-96 (inkdo01)
**	    Added support for per-cell repetition factors.
**	12-sep-96 (inkdo01)
**	    Added code for handling "<>" (ne) relops, for better selectivities.
**      01-Jan-00 (sarjo01)
**          Bug 99120: Added new parameter tbl so collation sequence
**          can be used by opn_diff where applicable.
**	16-may-00 (inkdo01)
**	    Changes for composite histogram support.
[@history_line@]...
*/
static i4
oph_splitcell(
    OPS_STATE		*global,
    OPZ_ATTS		*attrp,
    OPB_BFARG		**sbfargpp,
    DB_DATA_VALUE	*adcupperp,
    OPN_PERCENT		cellcount,
    OPH_CELLS		newlower,
    i4			celllength,
    OPN_PERCENT		*newcount,
    i4			sdifflength,
    OPO_TUPLES		celltups,
    OPH_HISTOGRAM	*newhistp,
    OPO_TUPLES		dmftuples,
    OPO_TUPLES		origcreptf,
    OPO_TUPLES		*newcreptf,
    OPH_INTERVAL	*intervalp)
{
    DB_DATA_VALUE   adckeyvalue;    /* describes user specified key value
				    ** i.e. constant used in where clause */
    i4		    newsubcells;    /* Number of new subcells created by
				    ** splitting this cell */
    OPN_PERCENT	    *savecount;	    /* ptr to count for first subcell within the
				    ** current original cell being analyzed */
    OPB_BFARG	    *sbfargp;	    /* Pointer to current key value */
    DB_STATUS	    comparestatus;  /* status of call to adc_compare */
    i4		    adc_cmp_result; /* result of call to adc_compare */
    PTR		    tbl = NULL;    /* collation table */

    if (global->ops_gmask & OPS_COLLATEDIFF)
        tbl = global->ops_adfcb->adf_collation;

    /* Initialize key value structure for adc_compare with correct type
    ** information */
    STRUCT_ASSIGN_MACRO(intervalp->oph_dataval, adckeyvalue);

    newsubcells = 0;

    savecount = newcount;	/* save position of count array for
				** oph_newcounts */

    for(sbfargp = *sbfargpp; sbfargp; sbfargp = sbfargp->opb_bfnext)
    {
	bool		equals_found;	/* TRUE if this clause is associated
					** with an equals operator */

	/* Get histogram value of current key */	
	adckeyvalue.db_data = sbfargp->opb_valp->opb_hvalue; 
	
	/* Check if value falls into this cell */
	comparestatus = adc_compare(global->ops_adfcb, adcupperp, &adckeyvalue,
				    &adc_cmp_result);
	if (comparestatus != E_DB_OK)
	    opx_verror(comparestatus, E_OP078C_ADC_COMPARE,
		       global->ops_adfcb->adf_errcb.ad_errcode);
	equals_found = (sbfargp->opb_valp->opb_opno ==
		global->ops_cb->ops_server->opg_eq ||
		sbfargp->opb_valp->opb_opno ==
		global->ops_cb->ops_server->opg_ne); /* ADF ID for
						 ** "="/"<>" operator */
	if (adc_cmp_result < 0)
	    /* this key value will be placed in a cell which is further into
	    ** the histogram, so don't look at any more key values for this
	    ** cell */
	    break;
	else if ( ((adc_cmp_result == 0) && (!equals_found))
		 || (cellcount <= 0.0))
	    /* This is an inequality which falls right on a cell boundary, or
	    ** the cell has a selectivity of 0, so no work needs to be done */
	    continue;
	else
	{   /* at least one new cell must be created */

	    if (equals_found)
	    {   /* Add one or more subcells for equality operator */
		i4	equalcells;
		
		equalcells = oph_equalcells(global, attrp, &adckeyvalue,
					    newlower, celllength, newcount,
					    sdifflength, celltups, intervalp);

		newlower += (equalcells * celllength);
		newcount += equalcells;
		newsubcells += equalcells;
	    }
	    else
	    {	/* Inequality operator. Compute diff between key value and
		** previous boundary value and add new cell if it is greater
		** than 0.
		** If the histogram type is CHAR, we create the new boundary
		** value even if the computed diff is zero, since this can
		** happen even when the two values are different. The cell
		** count will, however, be zero. This is necessary because
		** oph_interpolate uses adc_compare, not opn_diff.
		*/
		*newcount = opn_diff(adckeyvalue.db_data, newlower-celllength,
				     &sdifflength, celltups, attrp,
				     &intervalp->oph_dataval, tbl);

		if (*newcount > 0.0 || adcupperp->db_datatype == DB_CHA_TYPE ||
					adcupperp->db_datatype == DB_BYTE_TYPE)
		{   /* create new subcell. The diff serves temporarily as the
		    ** cell count */
		    MEcopy((PTR)adckeyvalue.db_data, celllength,
			   (PTR)newlower);
		    newlower += celllength;
		    newcount++;
		    newsubcells++;
		}
	    }  
	}
    }
    
    if (newsubcells)
    {
	/* Create last subcell, if any, from what is left of the original cell.
	*/

	/* Get diff between original upper boundary and the upper
	** boundary of the last new subcell created */
	*newcount = opn_diff(adcupperp->db_data, newlower-celllength,
			     &sdifflength, celltups, attrp,
			     &intervalp->oph_dataval, tbl);
	if ((*newcount) > 0.0)
	{   /* Create last subcell */
	    MEcopy((PTR)adcupperp->db_data, celllength, (PTR)newlower); 
	    newsubcells++;
	}
	else
	{   /* Diff computed by opn_diff is zero.
	    ** For character types, we must use adc_compare to see if the
	    ** values are really different (the result of opn_diff can be zero
	    ** even when the values are different); if they are, create last
	    ** subcell. This is necessary because oph_interpolate uses
	    ** adc_compare, not opn_diff.
	    ** Otherwise (non-character type), use original cell
	    ** histogram value, which would be useful for float datatypes in
	    ** which the diff could underflow to zero.
	    */
	    if (adcupperp->db_datatype == DB_CHA_TYPE ||
		adcupperp->db_datatype == DB_BYTE_TYPE)
	    {
		/* Compare original upper boundary with last new boundary */
		adckeyvalue.db_data = newlower - celllength;
		comparestatus = adc_compare(global->ops_adfcb, adcupperp,
					    &adckeyvalue, &adc_cmp_result);
		if (comparestatus != E_DB_OK)
		    opx_verror(comparestatus, E_OP078C_ADC_COMPARE,
			       global->ops_adfcb->adf_errcb.ad_errcode);
		if (adc_cmp_result > 0)
		{   /* original boundary is greater than last new boundary
		    ** - create last subcell */
		    MEcopy((PTR)adcupperp->db_data, celllength,
			   (PTR)newlower);
		    newsubcells++;
		}
		/* Otherwise, do nothing - values are same
		** (check for non-increasing was done previously) */
	    }
	    else
		/* use original cell histogram value, which would be useful for
		** float datatypes in which the diff could underflow to zero.
		*/
		MEcopy((PTR)adcupperp->db_data, celllength, 
		       (PTR)(newlower-celllength)); 
	}

	/* Partition the original selectivity among the new subcells */
	(VOID) oph_newcounts(newsubcells, savecount, celltups, newhistp,
			     cellcount, dmftuples, origcreptf, newcreptf);
    }
    
    /* Finished splitting */

    *sbfargpp = sbfargp;	/* Output last key looked at */

    return(newsubcells);
}

/*{
** Name: oph_addkeys - Add key values to histogram
**
** Description:
**      This routine will find the boolean factor keys that apply to a
**      histogram and add them to the histogram if they are in the histogram's
**      range. For a given key, the histogram cell into which the key falls
**      can  be split into two or three cells, depending on the operator: for
**      inequality, the cell may be split in two; for equality the cell can be
**      split into three cells.
**
**      For an inequality operator, the histogram cell can be split in two,
**      providing the key does not fall exactly on one of the boundaries: the
**      lower cell will cover the tuples which are less than or equal to the
**      key, and the upper cell will cover the tuples which are greater than
**      the key. If the key falls exactly on an existing cell boundary, no
**      splitting is necessary.
**
**      For an equality operator, the histogram cell can be split into three
**      cells: the middle cell, whose boundaries are the key and the value
**      immediately preceding the key, will cover the tuples which are exactly
**      equal to the key; the lower cell will cover tuples which are less than
**      the key, and the upper cell will cover tuples which are greater than
**      the key. If the key is exactly equal to the original upper boundary
**      value, or if the original lower boundary value immediately precedes the
**      key value, then the cell will be split into only two cells.
**
**      The cell counts of the new subcells are computed by applying the ratio
**      of the diffs between the new boundary values and the diff between the
**      original boundary values, and multiplying by the original cell count
**      (with extra complications for equality operators).
**
**	Adding the keys to the histogram effectively performs the interpolation
**	operations necessary for computing predicate selectivities from a
**	histogram once, right after the histogram is read, instead of possibly
**	multiple times while generating cost trees.
**
**	The addition of the keys is done by creating a whole new histogram,
**	pointed to by newhistmm and newfcnt, and adding new cell at a time,
**	either from the original ("old") histogram, or by creating a new cell
**	using a relevant key value. The new histogram then replaces the old
**	histogram.
**	
**      This functionality was part of the fix for bug 37172.
**      See the "Product Specification for Release 6.4/00 Optimizer Histogram
**      Processing" by Ed Seputis for more details.
**
**	Called by oph_catalog.
**	
**	FIXME - can write a routine which will do a binary search of the proper
**	histogram insertion point to more quickly find the proper location of
**	key value so that a more accurate memory allocation can be used - this
**	routine could be used in other areas like ophinterpolate to reduce
**	histogram lookup costs
**
** Inputs:
**      subquery		Pointer to subquery being processed
**      attrp			Ptr to joinop attribute element associated with
**      			histogram.
**	    opz_histogram	Contains histogram information read from
**				catalogs for this attribute
**		oph_numcells	Number of cells in histogram
**		oph_histmm	Array of boundary values of histogram.
**	attno			Joinop attribute number associated with
**				histogram.
**	varno			Joinop range variable for table associated with
**				histogram.
**	dmftuples		Number of tuples in relation, currently
**				estimated by DMF
**	newhistp		Pointer to histogram structure containing info
**				from catalogs.
**	    oph_reptf		Repetition factor of histogram
**	    oph_fcnt		Array of cell counts of histogram.
**	intervalp		Ptr to OPH_INTERVAL for histogram
**
** Outputs:
**	attrp
**	    opz_histogram	Updates oph_numcells with new number of cells,
**	    			and oph_histmm with new array of boundary
**	    			values.
**	newhistp
**	    oph_fcnt		Updated with new cell selectivities
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	23-aug-93 (rganski)
**          Initial creation from part of oph_catalog(), for modularity. See
**          change history of oph_catalog() for changes relating to the
**          Character Histogram Enhancements project and others.
**	18-oct-93 (rganski)
**	    b48831 - non-monotonically-increasing error with local collation
**	    sequence. When opn_diff is negative, call adc_compare to see if the
**	    histogram is really non-increasing (adc_compare takes collating
**	    sequence into account when comparing strings, opn_diff does not).
**	    Also, use error E_OP03A2_HISTINCREASING instead of
**	    E_OP0392_HISTINCREASING; E_OP03A2 shows table and column name.
**	31-jan-94 (rganski)
**	    Changes to comments, based on changes to oph_sarglist.
**	8-may-94 (ed)
**	    b59537 - added op130 to skip non-critical consistency checks
**	12-jan-96 (inkdo01)
**	    Added support for per-cell repetition factors.
**	29-jan-97 (inkdo01)
**	    Very minor bug fix in per-cell rep factors (found in VMS port).
**      01-Jan-00 (sarjo01)
**          Bug 99120: Added new parameter tbl so collation sequence
**          can be used by opn_diff where applicable.
**	16-may-00 (inkdo01)
**	    Changes for composite histogram support.
**	21-nov-02 (toumi01)
**	    Catch edge case opn_diff case-insensitive comparison that works
**	    with e.g. 'a' and 'B' but fails with e.g. 'a' and 'a'-1 = '`'.
**	    If the collation table comparison fails, retry with a simple
**	    character comparison.
**	    NOTE: this fixes bug 108794, which was triggered by the fix for
**	    bug 99120 (which added collation table support in opn_diff).
**	    However, while the test case for 108794 works with the fix for
**	    99120 backed out, there can still be failures with the collation
**	    in question with other data samples (e.g. the test case for 99120).
**	    See also bug 95384 - it is the trap for that bug which catches
**	    the error condition that was fixed by the change for 99120.
**	    There may be an undetermined root cause for all these problems.
[@history_line@]...
*/
static VOID
oph_addkeys(
    OPS_SUBQUERY	*subquery,
    OPZ_ATTS		*attrp,
    OPZ_IATTS		attno,
    OPV_IVARS		varno,
    OPO_TUPLES		dmftuples,
    OPH_HISTOGRAM	*newhistp,
    OPH_INTERVAL	*intervalp)
{
    OPS_STATE	    *global;
    OPB_BFARG	    *first_sbfargp; /* ptr to first key value which falls in
				** the range of the histogram */
    i4		    maxcellcount; /* maximum number of histogram cells */
    DB_DATA_VALUE   adcupper; 	/* describes upper bound of original histogram
				** cell being divided into subcells */
    OPH_CELLS	    lower;   	/* ptr to lower bound of current old histogram
				** cell being analyzed */
    i4		    celllength; /* length of histogram boundary values */
    OPH_CELLS	    newhistmm;	/* ptr to base of interval info for new
				** histogram */
    OPH_CELLS	    newlower;   /* ptr to lower bound of current new cell */
    OPN_PERCENT	    *newfcnt; 	/* ptr to base of selectivity counts for new
				** histogram */
    OPN_PERCENT	    *newcount; 	/* ptr to cell count (selectivity) of current
				** new cell */
    OPN_PERCENT	    *oldcount; 	/* ptr to selectivity count of current cell
				** being analyzed from old histogram */
    OPO_TUPLES 	    *newcrp;   	/* ptr to base of repetition factors for new
				** histogram */
    OPO_TUPLES 	    *newcreptf;	/* ptr to repetition factor of current
				** new cell */
    OPO_TUPLES 	    *oldcreptf;	/* ptr to repetition factor of current cell
				** being analyzed from old histogram */
    i4		    cellcount;	/* number of cells in the new histogram */
    OPN_PERCENT	    combine;	/* used to try to recover from obscure problem
				** in which no diff exists between subsequent
				** cells of a histogram */
    OPB_BFARG	    *sbfargp; 	/* ptr to current key participating in merge */
    i4		    cell;    	/* number of old cell currently being analyzed
				*/
    PTR		    tbl = NULL;	/* collation table */

    global = subquery->ops_global;
    if (global->ops_gmask & OPS_COLLATEDIFF)
        tbl = global->ops_adfcb->adf_collation;

    /* Traverse all the key lists and find the keys with the correct datatype
    ** and length for this histogram. Create a key list at attrp->opz_keylist
    ** containing all the relevant keys. Then scan the key list for keys which
    ** are greater than the lowest boundary value of the histogram, and compute
    ** the maximum cell count based on that key list. Get pointer to first key
    ** value. All done by oph_findkeys.
    */
    first_sbfargp = oph_findkeys(subquery, attrp, &maxcellcount, newhistp,
		intervalp);
	    
    if (first_sbfargp == (OPB_BFARG *)NULL)
	return;	/* No relevant keys were found; use original histogram */

    /* Initialize structures used for comparing boundary values and keys */
    STRUCT_ASSIGN_MACRO(intervalp->oph_dataval, adcupper);

    /* lower points to first boundary value in histogram */
    lower = intervalp->oph_histmm;

    celllength = intervalp->oph_dataval.db_length; /* boundary value
							     ** length */

    if (intervalp->oph_numcells == 2)
    {	/* if only one cell (i.e. a lower and upper bound) then check for
	** special case boundary condition in which the histogram is equal to
	** the min value and as a result the upper equals the lower bound
	** opn_diff() is used instead of adc_compare(), because opn_diff can
	** detect the cases where the lower boundary value is one lower than
	** first value, but diff is 0 */
	i4	sdifflength = 0;   /* For character type diffs */
	
	if (opn_diff(lower+celllength, lower, &sdifflength,
		     newhistp->oph_fcnt[1] * dmftuples, attrp,
		     &intervalp->oph_dataval, tbl) == 0.0)
	    return;	/* b43687 use original histogram if special boundary
			** condition occurs */
    }

    /* Allocate memory for worst case of cell division for new histogram
    ** intervals, counts and repetition factors. */
    newlower = newhistmm = 
	(OPH_CELLS)opu_memory(global, (i4)(maxcellcount * celllength));
    newcount = newfcnt =
	(OPN_PERCENT *)opu_memory(global, (i4)(maxcellcount *
						sizeof(*newfcnt)));
    newcrp = newcreptf = 
	(OPO_TUPLES *)opu_memory(global, (i4)(maxcellcount *
						sizeof(*newcrp)));

    *newfcnt = *newcrp = 0.0;	   /* init 1st elements, just to be tidy */
    newcount++;  		   /* position cell count noting that first
				   ** cell is always ignored and not updated */
    newcreptf++;		   /* likewise for repetition factors */

    oldcount = newhistp->oph_fcnt; /* ptr to first count of original histogram
				   */
    oldcreptf = newhistp->oph_creptf; /* and to first repetition factor */
    cellcount = 1;  		   /* number of cells in new histogram */

    /* Copy first boundary value, which will be same in new histogram */
    MEcopy((PTR)lower, celllength, (PTR)newlower);
    newlower += celllength;	   /* position for next cell interval */

    combine = 0.0;
    sbfargp = first_sbfargp;	   /* Pointer for traversing keys */

    /* for each cell, create new subcells as needed, and place
    ** the "diff value" temporarily in the new subcell count position
    ** - after the cell has been subdivided, use the total diff
    ** value to partition the selectivity among the new subcells
    ** - if equality keys are involved then a special case is
    ** made so the repitition factor is used in the calculation
    ** ... if the total cell selectivity is greater than the
    ** repetition factor, i.e. the repetition factor is used
    ** and proportioned among all the exact key cells, and if
    ** any selectivity is left over, then it is proportioned
    ** among the inexact cells 
    ** - per-cell repetition factor in newly created cells is 
    ** same as cell from which they are created.
    */
    for (cell = intervalp->oph_numcells; --cell > 0;
	 lower += celllength)
    {
	i4	    newsubcells;   /* number of new subcells associated with
				   ** the original cell */
	OPN_PERCENT originalcount; /* selectivity of current cell being
				   ** subdivided */
	OPO_TUPLES  originalcreptf; /* repetition factor of current cell */
	OPN_DIFF    originaldiff;  /* Diff between boundary values of current
				   ** cell */
	i4	    sdifflength;   /* For character type cells, length to which
				   ** diffs between boundary values should be
				   ** computed. Determined by opn_diff(). */ 
	OPO_TUPLES  celltups; 	   /* number of tuples covered by current cell
				   */

	oldcount++;
	originalcount = *oldcount;
	oldcreptf++;
	originalcreptf = *oldcreptf;
	celltups = originalcount * dmftuples;
	
	/* Compute diff between original boundary values. The value of
	** sdifflength returned by opn_diff will be used for all subsequent
	** diffs within this cell. */
	sdifflength = 0;
	originaldiff = opn_diff(lower+celllength, lower, &sdifflength,
				celltups, attrp,
				&intervalp->oph_dataval, tbl);
	/* If tbl is valued we are using a simple one-to-one collation mapping
	** (see opn_initcollate). This will usually be a "nocase" mapping of
	** the form [a-z]:[A-Z] or [A-Z]:[a-z] or variants that also nocase
	** accented vowels. Caseless comparison works for comparing e.g.
	** 'a' and 'B' correctly, but not for 'a' and 'a'-1 = '`'. Since
	** optimizedb builds such cells with decremented values, retry the
	** call to opn_diff if we failed with tbl, to catch this edge case.
	*/
	if (originaldiff < 0 && tbl != NULL)
	    originaldiff = opn_diff(lower+celllength, lower, &sdifflength,
				celltups, attrp,
				&intervalp->oph_dataval, NULL);
	if (cellcount == 1)
	    originalcount += combine; /* try to correct for original histogram
				      ** with no diffs */

	if ((originalcount< 0.0) || (originalcount> 1.0))
	    opx_error(E_OP0393_HISTOUTOFRANGE); /* selectivity read from
						** statistics catalog is out of
						** range */
	adcupper.db_data = lower + celllength;  /* mark upper bound of original
					        ** cell */

	/* Scan the key values in the key list beginning at sbfargp and insert
	** key values into the current histogram cell if they fall in its
	** range. This splits the current cell into multiple new subcells.
	** Return number of new subcells added.
	*/
	newsubcells = oph_splitcell(global, attrp, &sbfargp, &adcupper,
				    originalcount, newlower, celllength,
				    newcount, sdifflength, celltups, newhistp,
				    dmftuples, originalcreptf, newcreptf,
				    intervalp);

	/* If no new subcells were created by oph_splitcell, copy original
	** histogram cell to new histogram if possible. If there are new
	** subcells, increment cell count, and advance pointers past new cells.
	*/
	if (!newsubcells)
	{
	    /* No new subcells were created. Try to copy original cell info to
	    ** new histogram */
	    *newcreptf = originalcreptf; /* first do cell's repetition factor */
	    newcreptf++;
	    if (originaldiff > 0.0)	/* diff between cell boundaries */
	    {
		/* Original cell can be used in new histogram */ 
		MEcopy((PTR)adcupper.db_data, celllength, (PTR)newlower); 
		newlower += celllength;
		*newcount = originalcount; /* original selectivity */
		newcount++;
		cellcount++;		 /* number of cells in new histogram */
	    }
	    else
	    {   /* originaldiff is 0 or less. This should not happen, but if it
		** does, try to recover by adding the cell selectivity to the
		** previous cell */
		if (originaldiff == 0.0)
		{   /* the diff is so small that it underflows */
		    if (cellcount > 1) 
			*(newcount-1) += *oldcount; /* add to previous cell */
		    else
			combine += *oldcount; /* at first cell, delay until a
					      ** legitimate cell can be found */
		}
		else
		{   /* originaldiff is negative; Call adc_compare to see if we
		    ** have a non-increasing histogram (adc_compare can
		    ** determine that the histogram is increasing even if the
		    ** diff is negative, in the case of a local collation
		    ** sequence) */
		    DB_DATA_VALUE   adclower;	    /* lower cell boundary */
		    DB_STATUS	    comparestatus;  /* status of call to
						    ** adc_compare */
		    i4		    adc_cmp_result; /* result of call to
						    ** adc_compare */

		    /* Initialize structure for lower value with correct type
		    ** and data information */ 
		    STRUCT_ASSIGN_MACRO(intervalp->oph_dataval,
					adclower);
		    adclower.db_data = lower;

		    comparestatus = adc_compare(global->ops_adfcb, &adcupper,
						&adclower, &adc_cmp_result);
		    if (comparestatus != E_DB_OK)
			opx_verror(comparestatus, E_OP078C_ADC_COMPARE,
				   global->ops_adfcb->adf_errcb.ad_errcode);
		    
		    if ((adc_cmp_result < 0) 
			&& 
			!global->ops_cb->ops_skipcheck)
		    {
			OPT_NAME	attrname1;
			OPT_NAME	varname1;
			opt_paname(subquery, attno, &attrname1);
			opt_ptname(subquery, varno, &varname1, FALSE);
			opx_2perror(E_OP03A2_HISTINCREASING, (PTR)&varname1,
				    (PTR)&attrname1);
		    }
		}
	    }
	}
	else
	{   /* New subcells were created by oph_splitcells */
	    newcount += newsubcells;
	    newcreptf += newsubcells;
	    newlower += (newsubcells * celllength);
	    cellcount += newsubcells;
	}
    }

    if ((cellcount <= 1)
	&& 
	!global->ops_cb->ops_skipcheck)

    {
	/* Histogram must have at least 2 cells */
	OPT_NAME	attrname2;
	OPT_NAME	varname2;
	opt_paname(subquery, attno, &attrname2);
	opt_ptname(subquery, varno, &varname2, FALSE);
	/* original cells should be increasing or data from histogram is
	** corrupted, an attempt was made to combine cells, but cannot combine
	** all cells */
	opx_2perror(E_OP03A2_HISTINCREASING, (PTR)&varname2, (PTR)&attrname2);
    }

    /* Replace old histogram with new histogram */
    intervalp->oph_numcells  = cellcount;
    intervalp->oph_histmm = newhistmm;
    newhistp->oph_fcnt	= newfcnt;
    newhistp->oph_creptf = newcrp;
}

/*{
** Name: oph_temphist - searches for model histogram for global temporary
**	table column.
**
** Description:
**	Search RDF for persistent table from which model histograms may be
**	taken, then search for matching column. Model table must have same
**	name as temp table, with schema qualifier matching default user 
**	name for session or "_gtt_model". Column from which histogram is 
**	taken must have same name as that in temp table, as well as matching 
**	type.
**
** Inputs:
**	subquery			ptr to subquery being analyzed
**	attrp				ptr to temp table column descriptor
**	grvp				ptr to range table entry for temp table
**
** Outputs:
**	model_id			attr ID of model column from which 
**					histogram is to be extracted
**	Returns:
**		TRUE if qualifying model column is found, else
**		FALSE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-june-99 (inkdo01)
**	    Initial creation.
**	1-sep-00 (inkdo01)
**	    Minor check for TID column (not modelable).
**	20-Apr-2001 (thaju02)
**	    Use the session temporary owner rather than the session
**	    default user. (B104476/INGSRV1427, B104342/INGSRV1431)
**	8-nov-01 (inkdo01)
**	    Restore code changed by above fix - bugs now fixed elsewhere.
[@history_line@]...
*/
static bool
oph_temphist(
	OPS_SUBQUERY	*subquery,
	OPZ_ATTS	*attrp,
	OPV_GRV		*grvp,
	DB_ATT_ID	*model_id)

{
    OPS_STATE	*global = subquery->ops_global;
    RDF_CB	*rdfcb = &global->ops_rangetab.opv_rdfcb;
    RDR_INFO	*rdrinfo = grvp->opv_ttmodel;
    DB_STATUS	status;
    bool	first = TRUE;


    /* If we've already been here and didn't find a model table, just leave. */
    if (grvp->opv_gmask & OPV_TEMP_NOSTATS) return(FALSE);

    /* If we haven't been here at all, call RDF to see if there is a model table. */
    if (rdrinfo == (RDR_INFO *) NULL)
     while (TRUE)	/* once for default user, maybe once for "_gtt_model" */
    {
	rdfcb->rdf_rb.rdr_types_mask = RDR_RELATION | RDR_ATTRIBUTES | RDR_BY_NAME;
	STRUCT_ASSIGN_MACRO(grvp->opv_relation->rdr_rel->tbl_name, 
						rdfcb->rdf_rb.rdr_name.rdr_tabname);
	if (first) STRUCT_ASSIGN_MACRO(global->ops_cb->ops_default_user,
					rdfcb->rdf_rb.rdr_owner);
	else 
	{
	    /* Blank init and copy "_gtt_model" on top. */
	    MEfill(sizeof(DB_OWN_NAME), (u_char)' ', 
					(char *)&rdfcb->rdf_rb.rdr_owner);
	    MEcopy((char *)&gtt_model, sizeof(gtt_model),
					(char *)&rdfcb->rdf_rb.rdr_owner);
	}
	rdfcb->rdf_info_blk = NULL;
	status = rdf_call(RDF_GETDESC, (PTR)rdfcb);
	if (status != E_DB_OK)
	{
	    if (first)
	    {
		/* Retry with "_gtt_model" as user name. */
		first = FALSE;
		continue;
	    }
	    grvp->opv_gmask |= OPV_TEMP_NOSTATS;
	    return(FALSE);
	}
	grvp->opv_ttmodel = rdrinfo = rdfcb->rdf_info_blk;
	break;			/* exit loop */
    }

    /* Loop over columns from model table looking for matching column name. */
    {
	DB_ATTNUM	i;
	DMT_ATT_ENTRY	*att1, *att2;

	att1 = grvp->opv_relation->rdr_attr[attrp->opz_attnm.db_att_id];
							/* ttab column to match */
	if (att1 == NULL) return(FALSE);		/* if TID, no histogram */

	for (i = 1; i <= rdrinfo->rdr_no_attr; i++)
	{
	    i4	len1, len2;

	    att2 = rdrinfo->rdr_attr[i];
	    if (MEcmp((PTR)&att1->att_name, (PTR)&att2->att_name,
			sizeof(DB_ATT_NAME)) != 0) continue;
						/* look for name match */
	    if (abs(att1->att_type) != abs(att2->att_type)) break;
						/* names match, types don't */
	    len1 = (att1->att_type < 0) ? att1->att_width - 1 :
							att1->att_width;
	    len2 = (att2->att_type < 0) ? att2->att_width - 1 :
							att2->att_width;
	    if (len1 != len2) break;		/* names match, lengths don't */

	    model_id->db_att_id = i;			/* we have potential model */
	    return(TRUE);
	}
    }

    return(FALSE);				/* didn't find match */

}

/*{
** Name: oph_catalog	- read histogram information from the catalogs
**
** Description:
**      This routine will read histogram information from the catalogs 
**      and initialize the histogram element if it is successfully found.
**
** Inputs:
**      subquery                        ptr to subquery being processed
**      attrp                              ptr to joinop attribute element
**                                      associated with histogram
**      var_ptr                         ptr to joinop range variable element
**                                      into which the histogram element
**                                      will be placed
**	intervalp			ptr to OPH_INTERVAL for histogram
**
** Outputs:
**      newhistp                        ptr to histogram element being created
**                                      and will be initialized if histogram
**                                      is contained in catalog
**	nocells				ptr to bool set to TRUE if the 
**					histogram has no cells (as in
**					an all null column), else FALSE
**	Returns:
**	    TRUE if histogram was successfully initialized from catalog
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	7-may-86 (seputis)
**          initial creation
**	26-feb-91 (seputis)
**	    - improved diagnostics for b35862
**	19-mar-91 (seputis)
**	    - self joins on tables on explicitly referenced index should
**	    not have the relationships overridden, also fix b36597
**	11-apr-91 (seputis)
**	    - correct problems with 19-mar-91 fix
**	11-may-91 (seputis)
**	    - modify histograms based on constants used in sargable
**	    clauses in histogram, fix for b37172
**	16-aug-91 (seputis)
**	    - correct alignment problem causing AV's on unix
**	15-apr-92 (seputis)
**	    - b43587 - special boundary condition case causes E_OP0392
**	    error
**	18-may-92 (seputis)
**	    - improved diagnostics for error e_op0392
**	15-jun-92 (seputis)
**	    - on 18-may-92 fix cannot add parameters to existing error
**	    message, instead create a new one e_op03A2
**	15-jun-92 (seputis)
**	    - fix bug 44611 - concurrency problem calling RDF interface
**	    incorrectly, need to assume input parameter can change.
**	22-jun-92 (seputis)
**	    - make some minor fixes from a code walk-thru looking for
**	    the monotonically increasing histogram problem
**	09-nov-92 (rganski)
**	    Changed parameters to calls to opn_diff(), to support string diff
**	    extensions for Character Histogram Enhancements project (see spec
**	    thereof). Added new variables celltups and sdifflength to serve as
**	    2 of these new parameters. Added variable originaldiff, which is
**	    the diff between the original boundary values of a cell, before
**	    splitting. The opn_diff() call which sets this variable also sets
**	    the value of sdifflength (the string length to which diffs for
**	    this cell should be done) for subsequent calls in the same cell.
**	    This makes the last call to opn_diff(), for the remainder of the
**	    cell, unnecessary when there are no new subcells.
**	    Added initialization of intervalp->>oph_charnunique and
**	    oph_chardensity, which are new fields of OPH_INTERVAL for
**	    supporting string diff enhancements. 
**	14-dec-92 (rganski)
**	    Added datatype parameter to calls to opn_diff().
**	18-jan-93 (rganski)
**	    Character Histogram Enhancements project, continued.
**	    After call to RDF, set intervalp->oph_dataval.db_length
**	    to rdfhistp->shistlength; RDF may have read a new value from
**	    iistatistics for boundary value length. Also update hist value
**	    length in sargp.
**	26-apr-93 (rganski)
**	    Changed special case where a single-cell histogram has same upper
**	    and lower boundaries: now uses opn_diff() instead of adc_compare(),
**	    because opn_diff() catches the case where the lower boundary value
**	    is outside of the range of the data values; in this case the diff
**	    returned is zero, since this is the maximum diff for the data, and
**	    the attempt to combine cells will fail.
**	    Moved declaration of dmftuples so that more code could use it.
**	    Cosmetic changes.
**	24-may-93 (rganski)
**	    Character Histogram Enhancements project:
**	    Changed size of temp buffer to DB_MAX_HIST_LENGTH, since character
**	    histogram values can now be this size (1950 bytes).
**	    Removed old_hist_length, which is now obsolete, since boolean
**	    factor keys are no longer limited to 8 chars. Added limitation to
**	    rdf_cb->rdf_rb.rdr_cellsize, which is also necessitated by the
**	    removal of the limit on boolean factor keys; RDF uses this cellsize
**	    when old-style statistics are found.
**	23-aug-93 (rganski)
**	   Moved cell-splitting code to new function oph_addkeys and subsidiary
**	   functions, for modularity.
**	15-sep-93 (ed)
**	    Fix bug 52546 in which an explicitly referenced secondary index
**	    was specified, and the tid attribute of this relation was referenced,
**	    an out of range array reference was made, which looked at
**	    uninitialized memory
**	20-sep-93 (rganski)
**	   Added initialization of oph_version, which is a new field of
**	   OPH_INTERVAL.
**	31-jan-94 (rganski)
**	   Removed call to oph_sarglist: this is now called from oph_findkeys,
**	   because we don't know a histogram's length until we read the
**	   histogram, and we can't build a key list for a histogram without
**	   knowing its length. Thus, instead of building key lists for all
**	   attributes at one time, we build them one at a time after reading
**	   the histogram (see comments in oph_sarglist also).
**	   Removed sarglist parameter, which is now obsolete.
**	25-apr-94 (rganski)
**	   Added initialization of rdf_cb->rdf_rb.rd_histo_type as part of fix
**	   for b62184.
**	12-jan-96 (inkdo01)
**	    Added support for per-cell repetition factors.
**	24-june-99 (inkdo01)
**	    Changes to support model histograms for global temp tables.
**	17-aug-01 (hayke02)
**	    We now only call oph_temphist() (to create model histograms for
**	    session temp tables) if we are dealing with a session temp as
**	    oppposed to an internal temp (db_tab_name of "$<Temporary
**	    Table>"). This change fixes bug 105556.
**	16-may-00 (inkdo01)
**	    Changes for composite histogram support.
**	8-nov-01 (inkdo01)
**	    Modified fix for bugs 104342/104476 (original disabled model 
**	    histogram for temp table feature).
**      09-Nov-2001 (hanal04) Bug 106213 INGSRV 1581.
**          Fix RDF memory leak. After calling RDF for table descriptor
**          information we were setting opv_relation to the
**          rdf_info_block even if the RDF call was for opv_ttmodel.
**          This meant that opv_relation would incorrectly generate an 
**          RDF_UNFIX in ops_deallocate() on the rdf_info_block used for
**          the opv_ttmodel and leak the rdf_info_block block to which
**          opv_relation originally pointed to.
**          The original comment states that the assignment after the
**          call to rdf_call() is required in case the rdf_info_block has
**          changed. Added a TRdisplay() for instances where this happens
**          as we may need to unfix the old value to prevent a further
**          RDF leak.
**	14-aug-2002 (toumi01) i1198334/b104342 variant
**	    To avoid E_OP0082 don't do temp model histogram processing for
**	    composite keys (attrp will be NULL calling oph_catalog). 
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not simply != 0.
**	15-apr-05 (inkdo01)
**	    Tolerate 0-cell histograms (i.e. columns with stats but no
**	    histogram - as in an all-null column).
**	26-oct-05 (inkdo01)
**	    Add support for exact cell counts.
**	30-May-2006 (jenjo02)
**	    DMT_IDX_ENTRY idx_attr_id array now has DB_MAXIXATTS instead
**	    of DB_MAXKEYS elements.
**	30-Nov-2006 (kschendel) b122118
**	    Remove trdisplay from above, not useful and clogs the dbms log.
**	5-dec-2006 (dougi)
**	    Apparently bool's aren't always 1 byte - change memory allocation
**	    to be based on sizeof(OPH_BOOL).
**	29-may-07 (hayke02)
**	    Modify the change for SIR 115366 so that we now do not do exact
**	    cell processing for composite histograms (comphist), as they are
**	    not used for joins (yet). This change fixes bug 118411.
**	18-Oct-2007 (kibro01) b119319
**	    For a histogram where all values are null, don't try to allocate
**	    array of excells based on the number of cells.
**	 5-dec-08 (hayke02)
**	    Add trace point op213 to report all non-select list columns missing
**	    stats. This change addresses SIR 121322.
*/
static bool
oph_catalog(
	OPS_SUBQUERY       *subquery,
	OPH_HISTOGRAM      *newhistp,
	OPZ_IATTS	   attno,
	OPZ_ATTS           *attrp,
	OPV_IVARS	   varno,
	OPV_VARS	   *var_ptr,
	OPH_INTERVAL	   *intervalp,
	bool		   *nocells)
{
    OPV_GRV	    *grv_ptr;	    /* ptr to global range variable */
    DB_ATT_ID	    dmfattr;	    /* ingres attribute number of the base
				    ** relation */
    DB_TAB_ID	    base;	    /* table ID of base relation */
    OPS_STATE	    *global;
    OPO_TUPLES	    dmftuples;	    /* number of tuples in entire
				    ** relations, currently estimated
				    ** by DMF */
    bool	    ttab_hist = FALSE;
    bool	    comphist = (newhistp->oph_mask & OPH_COMPOSITE);

    grv_ptr = var_ptr->opv_grv;
    if (!grv_ptr->opv_relation	    /* does RDF information exist ? */
	||
	(attrp && attrp->opz_func_att != OPZ_NOFUNCATT)) /* function attributes
				    ** will not have histograms in
				    ** catalogs
				    */
	return(FALSE);

    global = subquery->ops_global;
    dmftuples = var_ptr->opv_tcop->opo_cost.opo_tups;
    
    base.db_tab_base = grv_ptr->opv_relation->rdr_rel->tbl_id.db_tab_base;
    if (grv_ptr->opv_relation->rdr_rel->tbl_id.db_tab_index > 0 && !comphist)
    {	/* if this is an explicitly referenced index by the user then
	** get the RDF histogram information from the primary relation
	** if it exists, since histogram information is only stored with
	** the primary, FIXME the relationship of the index to the base
	** relation should be obtained in opv_relatts */
	OPV_IVARS	    primary;
	OPV_GRV		    *grvp;

	grvp = NULL;
	for (primary = 0; primary <= subquery->ops_vars.opv_prv; primary++)
	{
	    if (primary == subquery->ops_vars.opv_prv)
	    {   /* if the index is referenced explicitly but the base
		** table is not available in the query, the call RDF to open
		** the base table to get statistics
		*/
		OPV_IGVARS	newgvar;
		OPV_GBMVARS	*rdfmap;    /* ptr to map of global range
                                            ** variables which have RDF info
                                            ** fixed */
		OPV_GRT		*gbase;     /* ptr to base of array of ptrs
                                            ** to global range table elements
                                            */
		OPV_IGVARS      maxgvar;    /* number of global range table
                                            ** elements allocated
                                            */
		gbase = global->ops_rangetab.opv_base;
		maxgvar = global->ops_rangetab.opv_gv;
		rdfmap = &global->ops_rangetab.opv_mrdf;
		for (newgvar = -1; (newgvar = BTnext((i4)newgvar, (char *)rdfmap, (i4)maxgvar))>=0;)
		{
		    RDR_INFO	    *rdfinfop;
		    rdfinfop = gbase->opv_grv[newgvar]->opv_relation;
		    if (rdfinfop 
			&& 
			(   rdfinfop->rdr_rel->tbl_id.db_tab_base
			    ==
			    base.db_tab_base
			)
			&&
			rdfinfop->rdr_rel->tbl_id.db_tab_index <= 0
			)
			break;		    /* found an instance of an
					    ** available RDF descriptor for
					    ** the underlying base table */
		}
		if (newgvar < 0)
		{   /* create a new descriptor for the base table since one
		    ** cannot be found */
		    base.db_tab_index = 0;
		    newgvar = opv_agrv(global, (DB_TAB_NAME *)NULL, 
			(DB_OWN_NAME *)NULL, &base, OPS_MAIN, FALSE,  &subquery->ops_vars.opv_gbmap,
			OPV_NOGVAR);
		    if (newgvar < 0)
			return (FALSE);
		}
		grvp = global->ops_rangetab.opv_base->opv_grv[newgvar];
	    }
	    else
		grvp = subquery->ops_vars.opv_base->opv_rt[primary]->opv_grv;

	    if (grvp
		&&
		grvp->opv_relation
		&&
		(   grvp->opv_relation->rdr_rel->tbl_id.db_tab_base 
		    == 
		    base.db_tab_base
		)
		&&
		grvp->opv_relation->rdr_rel->tbl_id.db_tab_index <= 0
	       )
	    {
#if 0
do not over-ride explicit index selection, since there may be self-joins
involved
		var_ptr->opv_index.opv_poffset = primary; /* save the
					** relationship to the primary */
#endif
		if (grvp->opv_relation->rdr_indx)
		{
		    OPV_IINDEX         ituple; /* index into the array of index tuples
					    ** associated with global range table
					    */

		    for( ituple = grvp->opv_relation->rdr_rel->
				    tbl_index_count;
			ituple-- ;)
		    {   /* see if the index can be found */
			if (!MEcmp((PTR)&grvp->opv_relation->
				    rdr_indx[ituple]->idx_name,
				(PTR)&grv_ptr->opv_relation->rdr_rel->
				    tbl_name,
				sizeof(DB_TAB_NAME)))
			{	/* index tuple is found so update info */

#if 0
do not over-ride explicit index selection, since there may be self-joins
involved
			    var_ptr->opv_index.opv_iindex = ituple; /* save
					** the relationship of the primary
					** and index */
#endif
			    if ((attrp->opz_attnm.db_att_id < 1)
				||
				(attrp->opz_attnm.db_att_id > DB_MAXIXATTS))
				dmfattr.db_att_id = DB_IMTID;	/* use tid
					** histogram if attribute is out
					** of range */
			    else
				dmfattr.db_att_id = grvp->opv_relation->
				    rdr_indx[ituple]->idx_attr_id
				    [attrp->opz_attnm.db_att_id-1]; /* get the
					** base relation attribute number
					** which will contain the histogram
					** information */
			    grv_ptr = grvp;	/* use global range var of 
					** base relation to get histogram 
					** info for index */
			    break;
			}
		    }
		}
		break;
	    }
	    else
		grvp = NULL;
	}
	if (!grvp)
	{   /* this should never be called */
	    return (FALSE);
	}
    }
    else if ( !comphist &&
	    (grv_ptr->opv_relation->rdr_rel->tbl_temporary != 0) &&
	    (MEcmp((PTR)&grv_ptr->opv_relation->rdr_rel->tbl_name.db_tab_name,
	    (PTR)DB_TEMP_TABLE, (sizeof(DB_TEMP_TABLE)-1)) !=0))
    {
	/* Global temp table - see if there's a model histogram. */
	ttab_hist = oph_temphist(subquery, attrp, grv_ptr, &dmfattr);
    }
    else
    {	/* assign attribute info for the histogram lookup */
	dmfattr.db_att_id = (comphist) ? -1 : attrp->opz_attnm.db_att_id;	
    }

    if (grv_ptr->opv_relation->rdr_rel->tbl_status_mask & DMT_ZOPTSTATS ||
					ttab_hist) 
        			    /* check if table has any histogram
				    ** statistics associated with it
				    */
    {	/* there is a possibility of some histogram info existing
	** in the system catalogs so ask RDF for it
	*/
	RDF_CB                *rdf_cb; /* control block used to obtain
				    ** information from RDF
				    */
	RDR_INFO              *rdf_info; /* ptr to RDF info block
				    ** for histograms
				    */
	i4               status; /* RDF return status */

	rdf_cb = &global->ops_rangetab.opv_rdfcb;
	rdf_cb->rdf_info_blk = 
        rdf_info = (ttab_hist) ? grv_ptr->opv_ttmodel : grv_ptr->opv_relation; 
				    /* get current info state associated
				    ** with the rdf (or for model table, if
				    ** this is global temptab) */
	STRUCT_ASSIGN_MACRO(rdf_info->rdr_rel->tbl_id, rdf_cb->rdf_rb.rdr_tabid);
				    /* get the table id from the parser range
                                    ** range table */
	rdf_cb->rdf_rb.rdr_cellsize =
	    intervalp->oph_dataval.db_length; /* provide length
				    ** of histogram cell to RDF. RDF may change
				    ** this depending on sversion in
				    ** iistatistics.
				    */
	rdf_cb->rdf_rb.rdr_histo_type =
	    intervalp->oph_dataval.db_datatype;
	if (intervalp->oph_dataval.db_datatype == DB_CHA_TYPE ||
	    intervalp->oph_dataval.db_datatype == DB_BYTE_TYPE)
	    /* Limit cell size to OPH_NH_LENGTH for char type. This is because
	    ** RDF uses this cell size when old-style statistics, which have
	    ** limited cell size, are found; and because cell size is assumed
	    ** to be column size until statistics tell us otherwise */
	    if (rdf_cb->rdf_rb.rdr_cellsize > OPH_NH_LENGTH)
		rdf_cb->rdf_rb.rdr_cellsize = OPH_NH_LENGTH;
	    
	rdf_cb->rdf_rb.rdr_hattr_no = dmfattr.db_att_id; /*provide dmf 
				    ** attr number for which histogram info
				    ** is required
				    */
	rdf_cb->rdf_rb.rdr_no_of_attr = rdf_info->rdr_rel->tbl_attr_count;
	rdf_cb->rdf_rb.rdr_types_mask = RDR_STATISTICS;
	rdf_cb->rdf_rb.rdr_2types_mask = (comphist) ? RDR2_COMPHIST : 0;
	status = rdf_call( RDF_GETINFO, (PTR)rdf_cb);

	/* Don't overwrite grv_ptr->opv_relation if we were using opv_ttmodel */
	if(ttab_hist)
	{
            grv_ptr->opv_ttmodel = rdf_cb->rdf_info_blk;
        }	
	else
	{
            if (grv_ptr->opv_relation->rdr_rel->tbl_temporary == 0)
	        grv_ptr->opv_relation = rdf_cb->rdf_info_blk; 
					/* possible that info block ptr
					** has changed (though not for temp
					** tables), so use current info
					** block */
        }
	if (status == E_RD0000_OK) 
	    /* continue if successful and use uniform histogram if error
	    ** - perhaps log an error to indicate a problem with the
	    ** histogram?
	    */
	{
	    RDD_HISTO	*rdfhistp;	/* ptr to RDF histogram
					** element */
	    EX_CONTEXT	excontext;	/* context block for exception 
					** handler*/
	    i4		i;

	    rdf_info = rdf_cb->rdf_info_blk; /* possible that info block ptr
					** has changed, so use current info
					** block */
	    rdfhistp = (rdf_info->rdr_histogram)[dmfattr.db_att_id];
	    /* the following elements are obtained directly from RDF
	    ** - this is considered to be the "old" or orignal 
	    ** histogram and will be modified if any sargable 
	    ** (i.e. keyed clauses) exist in the range of any 
	    ** histogram cell */
	    newhistp->oph_reptf	    = rdfhistp->sreptfact;
	    newhistp->oph_nunique   = rdfhistp->snunique;
            newhistp->oph_null      = rdfhistp->snull;
	    newhistp->oph_pcnt	    = NULL;
            newhistp->oph_pnull     = 0.0;
	    newhistp->oph_prodarea  = 0.0;
	    newhistp->oph_unique    = (rdfhistp->sunique != FALSE);
	    newhistp->oph_complete  = (rdfhistp->scomplete != FALSE);
            newhistp->oph_default   = FALSE;
	    intervalp->oph_domain	 = rdfhistp->sdomain;
	    intervalp->oph_numcells  	 = rdfhistp->snumcells;
	    intervalp->oph_histmm    	 = rdfhistp->datavalue;
	    intervalp->oph_charnunique = rdfhistp->charnunique;
	    intervalp->oph_chardensity = rdfhistp->chardensity;
	    intervalp->oph_dataval.db_length
		                                 = rdfhistp->shistlength;
	    MEcopy(rdfhistp->sversion, sizeof(DB_STAT_VERSION),
		   intervalp->oph_version.db_stat_version);

	    newhistp->oph_fcnt	    = rdfhistp->f4count;
	    if (rdfhistp->snumcells)
	    {
		newhistp->oph_creptf = (OPO_TUPLES *)opu_memory(global, 
		    (i4)(rdfhistp->snumcells) * sizeof (OPO_TUPLES));
		MEcopy(rdfhistp->f4repf, (i4)(rdfhistp->snumcells *
		    sizeof (OPO_TUPLES)), newhistp->oph_creptf);
	    }
	    else
	    {
		newhistp->oph_creptf = NULL;
		*nocells = TRUE;
	    }
	    for (i = 0; i < rdfhistp->snumcells; i++)
	    {
		if (newhistp->oph_creptf[i] == 0.0)
		    newhistp->oph_creptf[i] = 1.0;
	    }

	    if ( EXdeclare(oph_fhandler, &excontext) == EXDECLARE ) /* set
					** up exception handler to 
					** skip histogram calculations
					** and use original if problems
					** occur */
	    {
		(VOID)EXdelete();	/* cancel exception handler prior
					** to exiting routine */
		return(TRUE);		/* use original histogram definition
					** since float exceptions occurred */
	    }
	    {	/* adjust repetition factor and number of unique values
		** to reflect the current tuple count of the relation,
		** since (unique values * repetition) should equal the
		** relation tuple count
		** - keep the smaller of the two values fixed, since
		** it is more likely that domain will not change
		** that drastically */
		OPO_TUPLES	estimated_tuples;
		OPO_TUPLES	oldreptf;
		i4		i;

		oldreptf = newhistp->oph_reptf;
		if (newhistp->oph_reptf < 1.0)
		    newhistp->oph_reptf = 1.0;
		if (newhistp->oph_nunique < 1.0)
		    newhistp->oph_nunique = 1.0;
		estimated_tuples = newhistp->oph_reptf * newhistp->oph_nunique;
		if (dmftuples >= estimated_tuples)
		{
		    if (newhistp->oph_reptf > newhistp->oph_nunique)
			newhistp->oph_reptf = dmftuples/newhistp->oph_nunique;
		    else
			newhistp->oph_nunique = dmftuples/newhistp->oph_reptf;
		}
		else
		{   /* reduce proportionally */
		    OPN_PERCENT	    factor; /* factor to reduce stats */
		    OPN_PERCENT	    *uniquep; /* ptr to base of count array */
		    i4		    uniq_cell;	/* number of cells left to be
					** processed */
		    i4		    nunique; /* minimum number of unique
					** values */
		    factor = MHsqrt(dmftuples / estimated_tuples);
		    newhistp->oph_reptf *= factor;
		    newhistp->oph_nunique *= factor;
		    /* the number of unique values should be greater or equal to
		    ** the number of non zero cells in the histogram */
		    uniquep = newhistp->oph_fcnt;
		    nunique = 0;
		    for (uniq_cell = intervalp->oph_numcells;
			--uniq_cell > 0;)
		    {
			if (*(++uniquep) > 0.0)
			    nunique++;
		    }
		    if (newhistp->oph_nunique < (OPO_TUPLES)nunique)
		    {	/* assuming the same number of unique values exist
			** as when optimizedb was run, there should be a lower
			** bound given by the number of non-zero cells in the
			** histogram */
			newhistp->oph_nunique = (OPO_TUPLES)nunique;
			newhistp->oph_reptf = dmftuples / newhistp->oph_nunique;
		    }
		    if (newhistp->oph_reptf < 1.0)
			newhistp->oph_reptf = 1.0;
		    if (newhistp->oph_nunique < 1.0)
			newhistp->oph_nunique = 1.0;
		}
		if (oldreptf != newhistp->oph_reptf)
		 for (i = 0; i < intervalp->oph_numcells; i++)
		  if (newhistp->oph_creptf[i] != 0.0)
		       newhistp->oph_creptf[i] *= newhistp->oph_reptf/oldreptf;
				/* adjust per-cell repetition factors */
	    }
	    /* Add relevant key values to the histogram, splitting cells and
	    ** adjusting selectivities accordingly. Note, this is done in
	    ** oph_composites for composite histograms.
	    */
	    if (!comphist && !(*nocells)) 
		oph_addkeys(subquery, attrp, attno, varno, dmftuples, 
			newhistp, intervalp);

	    /* Loop over values looking for exact cells. Sum counts and
	    ** number of exact cells for the histogram. This can help
	    ** later in join processing. */
	    if (!(newhistp->oph_unique || comphist))
	    {
		i4	i, celllength, charlength;
		PTR	low, high;
		PTR	tbl = NULL;

		if (!*nocells)	/* b119319 (kibro01) */
		{
		    newhistp->oph_excells = (OPH_BOOL *) opu_memory(global,
			sizeof(OPH_BOOL)*intervalp->oph_numcells);
		}
		for (i = 0; i < intervalp->oph_numcells; i++)
		    newhistp->oph_excells->oph_bool[i] = FALSE;

		if (global->ops_gmask & OPS_COLLATEDIFF)
		    tbl = global->ops_adfcb->adf_collation;
		celllength = intervalp->oph_dataval.db_length;
		low = intervalp->oph_histmm;
		high = low + celllength;
		for (i = 1; i < intervalp->oph_numcells; i++)
		{
		    charlength = celllength;
		    if (opn_diff(high, low, &charlength, dmftuples, attrp,
				&intervalp->oph_dataval, tbl) == 1.0)
		    {
			newhistp->oph_numexcells++;
			newhistp->oph_exactcount += newhistp->oph_fcnt[i];
			newhistp->oph_excells->oph_bool[i] = TRUE;
		    }
		    low += celllength;
		    high += celllength;
		}
		if (newhistp->oph_exactcount >= 0.95)
		    newhistp->oph_mask |= OPH_ALLEXACT;
				/* effectively all cells are exact */
	    }
	    EXdelete();		/* cancel exception handler prior
				** to exiting routine */
	    return(TRUE);	/* process next joinop attribute */
	}
    }

    /* Trace point op213 - report all non-select list columns without stats */
    if (subquery->ops_global->ops_cb->ops_check
	&&
	opt_strace(subquery->ops_global->ops_cb, OPT_F085_MISSINGSTATS)
	&&
	attno >=0
	&&
	varno >= 0
	&&
	subquery->ops_vars.opv_base->opv_rt[varno])
    {
	OPE_IEQCLS	    eqcls;
	OPT_NAME	    attname;

	for (eqcls = -1; (eqcls = BTnext((i4)eqcls,
	    (char *)&subquery->ops_attrs.opz_base->opz_attnums[attno]
			    ->opz_eqcmap, subquery->ops_eclass.ope_ev)) >=0;)
	{
	    if (BTcount((char *)&subquery->ops_eclass.ope_base->
		ope_eqclist[eqcls]->ope_attrmap,
		(i4)subquery->ops_attrs.opz_av) > 1
		||
		subquery->ops_eclass.ope_base->ope_eqclist[eqcls]->ope_bfindex
								    != OPB_NOBF)
	    {
		opt_paname(subquery, attno, (OPT_NAME *)&attname);
		TRformat(opt_scc, (i4 *)global, 
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "column name=%s ", &attname);
		opt_tbl(subquery->ops_vars.opv_base->opv_rt[varno]->
							opv_grv->opv_relation);
	    }
	}
    }
    return(FALSE);		/* histogram was not processed
                                ** successfully */
}

/*
** {
** Name: oph_uniform	- create a default uniform distribution for histogram
**
** Description:
**      This routine will initialize a histogram element to a default
**      uniform distribution.  Special consideration is given to the
**      tid attribute of the relation
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      attrp                           ptr to joinop attribute associated
**                                      with histogram element
**      varp                            ptr to joinop range variable element
**                                      into which the histogram element
**                                      will be placed
**	nocells				TRUE if there was iistatistics row on 
**					catalog with no cells (all null column)
**					FALSE otherwise
**
** Outputs:
**      newhistp                        ptr to histogram element which will
**                                      be initialized with a uniform 
**                                      distribution
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	7-may-86 (seputis)
**          initial creation
**	27-dec-90 (seputis)
**	    change the complete flag default to be off so that
**	    histogram estimates are used more frequently
**	09-nov-92 (rganski)
**	    Added allocation and initialization of new OPH_INTERVAL fields
**	    oph_charnunique and oph_chardensity. The memory is only allocated
**	    if the attribute is of a character type.
**	    For now, these fields are being initialized to constants that will
**	    retain OPF's current behavior. 
**	15-apr-93 (ed)
**	    - bug 49864 - use same value OPH_DDOMAIN as optimizedb so that the
**	    complete behaviour is as expected
**	20-sep-93 (rganski)
**	    Added initialization of oph_version, which is a new field of
**	    OPH_INTERVAL. Substituted symbolic definitions
**	    DB_DEFAULT_CHARNUNIQUE and DB_DEFAULT_CHARDENSITY for numeric
**	    literals.
**      18-Oct-1993 (fred)
**          Removed duplicate adc_dhmin() call.  The duplicate was in
**          the wrong place (outside of a check for lack of
**          histograms), and was messing up (only in STAR) when
**          processing a large object.
**	12-jan-96 (inkdo01)
**	    Added support for per-cell repetition factors.
**	16-may-00 (inkdo01)
**	    Changes for composite histogram support.
**	15-apr-05 (inkdo01)
**	    Support for cell-less histogram (column is all null).
[@history_line@]...
*/
static VOID
oph_uniform(
	OPS_SUBQUERY       *subquery,
	OPH_HISTOGRAM      *newhistp,
	OPZ_ATTS           *attrp,
	OPV_VARS           *varp,
	OPH_INTERVAL	   *intervalp,
	bool		   nocells)
{
    /* create a uniform distribution for the attribute, since the
    ** histogram information is not available from the catalogs
    */
    OPS_DTLENGTH      histlength; /* length of histogram 
				** element for domain of attribute */
    ADF_CB            *adf_scb;	/* ADF session control block */
    OPS_STATE	      *global;
    
    global = subquery->ops_global;
    adf_scb = global->ops_adfcb;
    newhistp->oph_prodarea = OPH_UNIFORM;
    newhistp->oph_default = TRUE; /* default value used for lower bound */
    newhistp->oph_pcnt = NULL;
    newhistp->oph_pnull = 0.0;
    if (!nocells)
    {
	newhistp->oph_unique = OPH_DUNIQUE; /* values are not unique within
				** attribute */
	newhistp->oph_complete = 
	    (   (global->ops_cb->ops_alter.ops_amask & OPS_ACOMPLETE)
	    != 0
	    )
	    ||
	    (   global->ops_cb->ops_check
            &&
	    opt_strace(global->ops_cb, OPT_F050_COMPLETE)
	    )
	    ;			/* TRUE if all values for domain exist
				** within attribute */
	intervalp->oph_domain = OPH_DDOMAIN;
	newhistp->oph_reptf = OPH_NODUPS; /* no duplicates within attr*/
	/* FIXME - opv_relation will not be defined for aggregate temporaries */
    
	newhistp->oph_nunique = varp->opv_trl->opn_reltups; /* define
				** number of unique tuples in
				** relation to be equal to number
				** of tuples
				*/
    }
    intervalp->oph_numcells = OPH_DNUMCELLS; /* default number of
				** histogram cells
				*/
    histlength = intervalp->oph_dataval.db_length;
    newhistp->oph_fcnt = (OPH_COUNTS) opu_memory(global,
	(i4) (OPH_DNUMCELLS * (2 * sizeof(OPH_COUNTS) + histlength))); 
				/* get memory for cell boundaries and
				** histogram elements
				** - assign space for oph_fcnt[0]
				** and oph_fcnt[1]
				*/
    newhistp->oph_creptf = (OPO_TUPLES *)&(newhistp->oph_fcnt[2]);
				/* Two cells for repetition factors */
    intervalp->oph_histmm = (OPH_CELLS) (&newhistp->oph_fcnt[4]);/* 
				** assign two cells for histogram 
				** where the min and max values 
				** for entire domain will be placed
				** - oph_fcnt[2] is the address of
				** the first unused count element
				*/
    MEcopy(DB_NO_STAT_VERSION, sizeof(DB_STAT_VERSION),
	   intervalp->oph_version.db_stat_version);	/* Initialize
				** version field to blanks, indicating no
				** verion.
				*/

    /* If histogram is a character type, allocate character
    ** statistics arrays and initialize to default values.
    ** All character types are reduced to DB_CHA_TYPE for histograms
    ** unless Unicode is involved in which case we're using binary
    ** collation date stored as DB_BYTE_TYPE.
    */
    if (intervalp->oph_dataval.db_datatype == DB_CHA_TYPE ||
	intervalp->oph_dataval.db_datatype == DB_BYTE_TYPE)
    {
	i4	i;	/* loop counter */
	
	intervalp->oph_charnunique = (i4 *) opu_memory(global,
					       sizeof(i4) * histlength);
	intervalp->oph_chardensity  = (OPH_COUNTS) opu_memory(global,
					       sizeof(OPN_PERCENT) * histlength);
	for (i = 0; i < histlength; i++)
	{
	    intervalp->oph_charnunique[i] = 
		                          (i4) DB_DEFAULT_CHARNUNIQUE;
	    intervalp->oph_chardensity[i] =
		                          (OPN_PERCENT) DB_DEFAULT_CHARDENSITY;
	}
    }
    else
    {
	intervalp->oph_charnunique = (i4 *) NULL;
	intervalp->oph_chardensity = (OPH_COUNTS) NULL;
    }
    
    /* Initialize histogram boundary values to default values for the type
    */
    {
	DB_STATUS		dhstatus; /* ADF return status for min/max
				** histogram value functions
				*/
        i4                      dt_bits;

        intervalp->oph_dataval.db_data
            = intervalp->oph_histmm; /* save ptr to min value */
        dhstatus = adi_dtinfo(adf_scb,
                                attrp->opz_dataval.db_datatype,
                                &dt_bits);
        if ((dhstatus == E_DB_OK) && (dt_bits & AD_NOHISTOGRAM))
        {
            MEcopy(OPH_NH_DHMIN,
                    OPH_NH_LENGTH,
                    intervalp->oph_dataval.db_data);
        }
        else
        {
            dhstatus = adc_dhmin( adf_scb, &attrp->opz_dataval,
                &intervalp->oph_dataval);
            /*get minimum histogram value */
        }
# ifdef E_OP0787_ADC_DHMIN
	if (dhstatus != E_DB_OK)
	    opx_verror( dhstatus, E_OP0787_ADC_DHMIN, 
		adf_scb->adf_errcb.ad_errcode);
# endif
	intervalp->oph_dataval.db_data 
	    = &intervalp->oph_histmm[histlength]; /* save ptr to max
				** value */
        if (dt_bits & AD_NOHISTOGRAM)
        {
            MEcopy(OPH_NH_DHMAX,
                    OPH_NH_LENGTH,
                    intervalp->oph_dataval.db_data);
        }
        else
        {
            dhstatus = adc_dhmax( adf_scb, &attrp->opz_dataval,
                &intervalp->oph_dataval); /* get maximum histogram
				** value*/
        }
	intervalp->oph_dataval.db_data = NULL;
# ifdef E_OP0786_ADC_DHMAX
	if (dhstatus != E_DB_OK)
	    opx_verror( dhstatus, E_OP0786_ADC_DHMAX,
		adf_scb->adf_errcb.ad_errcode);
# endif
    }

    /* Initialize cell counts
    */
    newhistp->oph_fcnt[0] = OPH_D1CELL; /* default for first cell
				** is 0% since cell boundary chosen
				** to be less than entire domain
				*/
    newhistp->oph_fcnt[1] = OPH_D2CELL; /* default for second cell
				** is 100% since cell boundary
				** chosen to be greater than entire
				** domain
				*/
    newhistp->oph_creptf[0] = 0.0;
    newhistp->oph_creptf[1] = OPH_NODUPS;  /* one cell reptf - same as for
				** whole histogram */

    if (!nocells)
    {
	if (attrp->opz_dataval.db_datatype < 0)
	    newhistp->oph_null = global->ops_cb->ops_alter.ops_exact;
				/* if nullable, treat like "=" */
	else newhistp->oph_null = 0.0;   /* else, assume no NULLs */
    }

    if (attrp->opz_attnm.db_att_id == DB_IMTID)
    {   /* - if the attribute is the implicit TID for the relation
	** then choose some more reasonable defaults for the
	** histogram
	** - histograms are built on the page number of the tid
	*/
	DB_TID             maxtid;  /* temporary used to build max
				** tid (until DMF externalize
				** tid format)
				*/

        /* FIXME - should probably initialize mintid also since the defaults
        ** for integer are assumed.  tid_page are assigned in increasing order
        ** so the largest value would be the number of pages in the relation
        */
        maxtid.tid_i4 = 0;
	maxtid.tid_tid.tid_page = varp->opv_tcop->opo_cost.opo_reltotb;
				/* use CO node since it will exist for function
                                ** aggregate temporaries also */
	newhistp->oph_unique = TRUE; /* tids are unique per tuple */
	MEcopy( (PTR) &maxtid, histlength, 
	    (PTR) (intervalp->oph_histmm + histlength)); /* copy 
				** largest current page number for table */
    }
}

/*{
** Name: oph_index	- create histogram for index
**
** Description:
**      The routine creates a histogram for an attribute associated with an
**      index.  Since all primary attributes are processed before indexes
**      and an index attribute will only be in an equivalence class which
**      contains the corresponding primary attribute, the histogram must
**      already exist and belong to the list of histograms associated with
**      the primary.  Make a copy of the histogram element for this attribute.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      newhistp                        ptr to allocated histogram element
**                                      to be initialized for the index.
**      attrp                              ptr to joinop attribute element which
**                                      is an attribute of an index and needs
**                                      a histogram created
**      attr                            joinop attribute number of attrp
**
** Outputs:
**      newhistp                        histogram element will be initialized
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	8-may-86 (seputis)
**          initial creation
[@history_line@]...
*/
static VOID
oph_index(
	OPS_SUBQUERY       *subquery,
	OPH_HISTOGRAM      *newhistp,
	OPZ_IATTS	   attr,
	OPZ_ATTS           *attrp)
{
    OPH_HISTOGRAM         *primhisto;   /* corresponding histogram
			       ** element from primary relation
			       */
    OPH_HISTOGRAM         *savep; /* temporary used to save and
			       ** restore link ptr
			       */
    OPV_RT                *vbase; /* ptr to base of array of ptrs to joinop
			       ** range variables */

    vbase = subquery->ops_vars.opv_base;
    primhisto = *opn_eqtamp( subquery, &vbase->opv_rt
	[vbase->opv_rt[attrp->opz_varnm]->opv_index.opv_poffset]->opv_trl
	->opn_histogram,
	attrp->opz_equcls, TRUE);
			       /* search the histogram list of the primary
			       ** relation associated with this the index 
			       ** for a histogram for this
			       ** equivalence class
			       ** - each range var can only have one
			       ** attribute within any one
			       ** equivalence class.
			       ** - guaranteed that a histogram
			       ** will be found or an error will
			       ** be signalled by the routine
			       */
    savep = newhistp->oph_next; /* save ptr which will be trashed*/
    STRUCT_ASSIGN_MACRO((*primhisto), (*newhistp));
    newhistp->oph_next = savep; /* restore link */
    newhistp->oph_attribute = attr; /* save joinop attribute associated
			       ** with the histogram which was trashed
			       */
    {	/* make a copy of the histogram components found in the joinop attribute
        */
	OPZ_ATTS	    *primptr;/* ptr to joinop attribute element
			       ** associated with histogram in primary relation 
			       */

	primptr = subquery->ops_attrs.opz_base
	    ->opz_attnums[primhisto->oph_attribute];
        STRUCT_ASSIGN_MACRO((primptr->opz_histogram), (attrp->opz_histogram));
    }
}

/*{
** Name: collatable_type - Returns whether this data type uses collation
**
** Description:
**      Return TRUE if this data type uses collation, FALSE otherwise.
**
** Inputs:
**      datatype - data type to check for collation
**
** Outputs:
**	Returns:
**	    TRUE if collatable data type
**	    FALSE if non-collatable data type
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	5-Jan-2009 (kibro01) b121385
**	    Written.
*/

static bool
collatable_type(i4    datatype)
{
        switch (abs(datatype))
        {
        case DB_CHA_TYPE:
        case DB_VCH_TYPE:
        case DB_VBYTE_TYPE:
        case DB_VBIT_TYPE:
        case DB_TXT_TYPE:
        case DB_NVCHR_TYPE:
                return(TRUE);
        default:
                return(FALSE);
        }
}


/*{
** Name: oph_composites	- search for and build relevant composite histograms
**
** Description:
**      The routine searches through the range table for variables (currently
**	those with multi-attribute key structures) which might have composite
**	histograms defined. An attempt is then made to build the corresponding
**	histogram structure.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-may-00 (inkdo01)
**          initial creation
**	26-sep-00 (inkdo01)
**	    Replicate histogram and attach to primary range var.
**	 1-jul-04 (hayke02)
**	    Replace opb_mcount with opb_count for the opb_keyorder[i] 'for'
**	    loop. Although opb_mcount/key_count is used to assign the values in
**	    opb_keyorder[] (with OPZ_NOATTR at opb_keyorder[opb_count]) in
**	    opb_jkeylist(), opb_count is used to size the MEcopy() of keyorder
**	    to rvp->opv_mbf.opb_kbase in opb_pmbf(). This change fixes problem
**	    INGSRV 2885, bug 112585.
**	24-jan-2008 (dougi)
**	    Slight adjustment to previous change to use opb_bfcount, because
**	    this is only used to build new cells and the attrvec should match
**	    the cells we're building.
**	5-Jan-2009 (kibro01) b121385
**	    Don't apply collation unless all the composite histogram is
**	    collatable (similar to bug 121063 during generation of statistics).
*/
static VOID
oph_composites(
	OPS_SUBQUERY       *subquery)

{
    OPH_HISTOGRAM	lochist;
    OPH_HISTOGRAM	*newhistp;
    OPZ_ATTS		locattr;
    OPV_IVARS		varno;
    OPV_VARS		*varptr;
    OPV_GRV		*gvarptr;
    OPV_RT		*vbase = subquery->ops_vars.opv_base;
    i2			i;
    bool		nocells;
    bool		all_collatable;
    PTR			save_collation;

    save_collation = subquery->ops_global->ops_adfcb->adf_collation;

    MEfill(sizeof(OPZ_ATTS), (u_char)0, (PTR)&locattr);
    locattr.opz_mask = OPZ_COMPATT;	/* init dummy attr desc */

    /* Loop over range table, looking for variables with multi-attr
    ** key structures, at least 2 columns of which are covered by
    ** Boolean factors. Look for composite histograms for such vars. */

    for (varno = 0; varno < subquery->ops_vars.opv_rv; varno++)
    {
	all_collatable = TRUE;
	varptr = vbase->opv_rt[varno];		/* get var ptr */
	if (varptr->opv_mbf.opb_bfcount <= 1) continue;
					/* gotta have more than one attr 
					** covered by BFs */
	lochist.oph_mask = OPH_COMPOSITE;
	lochist.oph_attribute = -1;
	locattr.opz_histogram.oph_dataval.db_data = NULL;
	locattr.opz_histogram.oph_dataval.db_datatype = DB_CHA_TYPE;
	locattr.opz_histogram.oph_dataval.db_length = 8;
	locattr.opz_histogram.oph_dataval.db_prec = 0;
	nocells = FALSE;
	if (!oph_catalog( subquery, &lochist, (OPZ_IATTS) -1, 
		(OPZ_ATTS *) NULL, varno, varptr, &locattr.opz_histogram, &nocells)) 
	    continue;

	/* It found a histogram - allocate space for it, the attr bit map
	** and the OPH_INTERVAL, and copy 'em all over. */
	newhistp = (OPH_HISTOGRAM *)opu_memory( subquery->ops_global,
		(i4) (sizeof(OPH_HISTOGRAM) + sizeof(OPH_INTERVAL) + 
		sizeof(OPE_BMEQCLS) + (varptr->opv_mbf.opb_mcount+1) * 
							sizeof(OPZ_IATTS)));
	STRUCT_ASSIGN_MACRO(lochist, (*newhistp));
	newhistp->oph_composite.oph_interval = 
		((OPH_INTERVAL *)((char *)newhistp + sizeof(OPH_HISTOGRAM)));
	STRUCT_ASSIGN_MACRO(locattr.opz_histogram, 
		(*newhistp->oph_composite.oph_interval));
	newhistp->oph_composite.oph_eqclist =
	    ((OPE_BMEQCLS *)((char *)newhistp->oph_composite.oph_interval + 
						sizeof(OPH_INTERVAL)));
	newhistp->oph_composite.oph_attrvec = 
	    ((OPZ_IATTS *)((char *)newhistp->oph_composite.oph_eqclist + 
						sizeof(OPE_BMEQCLS)));
	newhistp->oph_composite.oph_bfcount = 0;
	MEfill(sizeof(OPB_BMBF), (u_char)0,
				(PTR)&newhistp->oph_composite.oph_bfacts);

	/* Init, then assign the attribute bit map using joinop attno's 
	** mapped by key columns. */
	for (i = 0; i < varptr->opv_mbf.opb_bfcount; i++)
	{
	    i4	j;
	    j = varptr->opv_mbf.opb_kbase->opb_keyorder[i].opb_attno;
	    newhistp->oph_composite.oph_attrvec[i] = j;
	    BTset((u_i2)subquery->ops_attrs.opz_base->opz_attnums[j]->
			opz_equcls, (PTR)newhistp->oph_composite.oph_eqclist);
				/* and set corresponding eqc in map */
	    if (save_collation &&
		all_collatable &&
		!collatable_type(subquery->ops_attrs.opz_base->opz_attnums[j]->
			opz_dataval.db_datatype))
		all_collatable = FALSE;
	}
	newhistp->oph_composite.oph_attrvec[i] = OPZ_NOATTR;	
				/* terminate attno array */

	if (!all_collatable)
	    subquery->ops_global->ops_adfcb->adf_collation = NULL;
	    
	/* Add cells for predicate restriction constants. */
	oph_addkeys(subquery, &locattr, (OPZ_IATTS) 0, varno, 
		varptr->opv_tcop->opo_cost.opo_tups, newhistp, 
		newhistp->oph_composite.oph_interval);

	if (!all_collatable)
	    subquery->ops_global->ops_adfcb->adf_collation = save_collation;

	/* Attach new histogram to range variable chain. */
	newhistp->oph_next = varptr->opv_trl->opn_histogram;
	varptr->opv_trl->opn_histogram = newhistp;
	varptr->opv_mask |= OPV_COMPHIST;

	/* If this is secondary index (and all will be in initial version),
	** make copy of histogram for corresponding base table and diddle
	** the attribute vector, as needed. */
	if (varno >= subquery->ops_vars.opv_prv)
	{
	    OPH_HISTOGRAM	*primhistp;
	    OPV_IVARS		primvarno;
	    OPV_VARS		*primvarp;

	    primhistp = (OPH_HISTOGRAM *)opu_memory(subquery->ops_global,
		(i4)(sizeof(OPH_HISTOGRAM) + 
		(varptr->opv_mbf.opb_mcount+1)*sizeof(OPZ_IATTS)));
	    STRUCT_ASSIGN_MACRO((*newhistp), (*primhistp));
	    primhistp->oph_composite.oph_attrvec = 
		(OPZ_IATTS *)((char *)primhistp + sizeof(OPH_HISTOGRAM));
	    primvarno = varptr->opv_index.opv_poffset;
	    primvarp = vbase->opv_rt[primvarno];
	    primhistp->oph_next = primvarp->opv_trl->opn_histogram;
	    primvarp->opv_trl->opn_histogram = primhistp;
	    primvarp->opv_mask |= OPV_COMPHIST;
	
	    /* To re-assign the joinop attrs, we must loop over attrvec,
	    ** get the eqclass for the indexed columns, then find the
	    ** base table column with the same eqclass. */
	    for (i = 0; i < varptr->opv_mbf.opb_mcount && 
		newhistp->oph_composite.oph_attrvec[i] != OPZ_NOATTR; i++)
	    {
		OPE_IEQCLS	eqc;
		OPZ_IATTS	j;
		OPZ_ATTS	*primattrp;

		eqc = subquery->ops_attrs.opz_base->opz_attnums[newhistp->
			oph_composite.oph_attrvec[i]]->opz_equcls;
		for (j = 0; j < subquery->ops_attrs.opz_av; j++)
		 if ((primattrp = subquery->ops_attrs.opz_base->opz_attnums[j])->
			opz_varnm == primvarno &&
		    primattrp->opz_equcls == eqc)
		{
		    primhistp->oph_composite.oph_attrvec[i] = j;
		    break;
		}
	    }
	    primhistp->oph_composite.oph_attrvec[i] = OPZ_NOATTR;
					/* terminate attribute vector */

	}

    }	/* end of range table loop */

}


/*{
** Name: oph_histos	- read or create histograms for attributes in query
**
** Description:
**      This routine will read or create histogram elements for the attributes
**      used in the qualification of the query or used in a joining clause
**
**	for attributes which have histo info in the zopt1stat and
**	zopt2stat system catalogs read that info in
**
**	for attributes which do not have entries in these catalogs
**	create a "uniform" distribution
**
**	attributes which belong to INDEX relations must necessarily
**	be distributed the same as the relation being indexed and so
**	their histo info is set to be the same as the corresponding
**	attribute in the PRIMARY
**
**      read or create histograms for those attributes which are either in
**         a) in the qualification 
**         b) in a joining equivalence class (i.e. equivalence class with
**            more than two attributes)
**      At this point, this set is precisely those attributes which have been
**      assigned to an equivalence class
**
** Inputs:
**      subquery                        ptr to subquery to be optimized
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	07-may-86 (seputis)
**          initial creation
**	15-nov-90 (seputis)
**	    fix multi-attribute keying estimates
**      28-jan-92 (seputis)
**          fix 41809 - check if btree exists and is possibly useful for
**          top sort node removal for single variable query
**      7-dec-93 (ed)
**          b56139 - check OPE_HIST_NEEDED for attributes which
**          need histograms,... needed since target list is traversed
**          earlier than before
**	31-jan-94 (rganski)
**	    Removed sarglist variable, which is no longer passed to oph_catalog
**	    (see comment in oph_catalog).
**	7-apr-94 (ed)
**	    b60191 - E_OP04B3 consistency check caused by uninitialized variable
**	16-may-00 (inkdo01)
**	    Changes for composite histogram support.
**	15-apr-05 (inkdo01)
**	    Support for cell-less histogram (column is all null).
**	26-oct-05 (inkdo01)
**	    Init exact count fields.
**	17-Nov-2006 (kschendel) b122118
**	    Fix precedence typo with OPZ_SPATJ, no symptoms known.
[@history_line@]...
*/
VOID
oph_histos(
	OPS_SUBQUERY       *subquery)
{
    OPZ_IATTS		attr;		/* joinop attribute currently being
                                        ** analyzed
                                        */
    OPZ_IATTS           maxattr;        /* maximum joinop attribute which is
                                        ** defined
                                        */
    OPZ_AT              *abase;         /* ptr to base of array of ptrs to
                                        ** joinop elements
                                        */
    OPV_RT              *vbase;         /* ptr to base of array of ptrs to
                                        ** joinop range table elements
                                        */
    OPV_IVARS           primary;        /* last primary joinop range table
                                        ** element which is defined
                                        */
    bool		unique_found;   /* TRUE if there exists a "unique"
					** storage structure */
    OPZ_BMATTS		uattrmap;       /* map of attributes which have
					** unique secondary indices */
    OPS_CB		*ops_cb;	/* ptr to session control block */
    OPE_ET		*ebase;		/* ptr to array of ptrs to eqcls 
					** descriptors */
    bool		nocells;

    if ((subquery->ops_eclass.ope_ev == 0) /* if no equivalence classes exist
                                        ** then return since histograms
                                        ** will not be useful
                                        */
#if 0
/* remove condition for b41809 since histograms required, ophhistos
** has been moved to enumeration from joinop phase so check is no
** longer necessary */
	||
	( (subquery->ops_vars.opv_rv <= 1)/* if only one relation exists
                                        ** in the query then histograms will
                                        ** not be needed */
	  &&
          (!subquery->ops_enum)         /* unless required for a temporary */
        )
#endif
       )                                
	return;

    ops_cb = subquery->ops_global->ops_cb; /* get session control block for
					** thread */
    maxattr = subquery->ops_attrs.opz_av; /* maximum joinop attribute defined */
    abase = subquery->ops_attrs.opz_base; /* base of array of ptrs to joinop
                                        ** elements
                                        */
    ebase = subquery->ops_eclass.ope_base; /* base of array of ptrs to eqcls
					** elements */
    vbase = subquery->ops_vars.opv_base; /* base of array of ptrs to joinop
                                        ** range variables
                                        */
    primary = subquery->ops_vars.opv_prv; /* max primary relation */
    unique_found = FALSE;		/* set TRUE if one unique index is
					** found */

    for ( attr = 0; attr < maxattr; attr++ )
    {	/* look at each attribute and create a histogram for it (if it will
        ** be useful i.e. if an equivalence class exists)
        */
	OPZ_ATTS               *attrp;	/* ptr to joinop attribute being
                                        ** analyzed
                                        */
        OPV_VARS               *var_ptr; /* ptr to joinop range variable
                                        ** element associated with the joinop
                                        ** attribute being analyzed
                                        */
	attrp = abase->opz_attnums[attr];
	if ((attrp->opz_equcls >= subquery->ops_eclass.ope_maxhist)
	    &&
	    !(ebase->ope_eqclist[attrp->opz_equcls]->ope_mask & OPE_HIST_NEEDED)
	    &&
	    (attrp->opz_mask & OPZ_SPATJ) == 0)
	    continue;			/* continue if no equivalence class
                                        ** has been defined since histogram
                                        ** will not be useful for this
                                        ** attribute
                                        */
	var_ptr = vbase->opv_rt[attrp->opz_varnm]; /* get ptr to
                                        ** joinop range table element
                                        ** associated with range variable
                                        */

	{   /* - a new histogram element needs to be created 
	    ** - the histogram does not exist in the global list so it must be
	    ** read or a uniform histogram created
	    */

	    OPH_HISTOGRAM      *newhistp;   /* used to create new histogram
					    ** element for attribute
					    */

	    /* get memory and copy info into OPF histogram structure */
	    newhistp = (OPH_HISTOGRAM *) opu_memory( subquery->ops_global,
		(i4) sizeof(OPH_HISTOGRAM));

	    /* link into list associated with joinop range variable */
	    MEfill(sizeof(OPH_HISTOGRAM), 0, newhistp);
	    newhistp->oph_next = var_ptr->opv_trl->opn_histogram;
	    var_ptr->opv_trl->opn_histogram = newhistp;  /* link into 
                                            ** local list ... if this is not
                                            ** the main query then
					    ** the local list will be returned
					    ** to the global list once the
					    ** processing of the subquery has
					    ** been completed
					    */
	    /* Init fields only used for composite histograms. */
	    newhistp->oph_composite.oph_attrvec = (OPZ_IATTS *) NULL;
	    newhistp->oph_composite.oph_interval = (OPH_INTERVAL *) NULL;
	    newhistp->oph_composite.oph_vallistp = (OPB_BFVALLIST *) NULL;
	    newhistp->oph_composite.oph_eqclist = (OPE_BMEQCLS *) NULL;
	    /* Init exact cell fields. */
	    newhistp->oph_excells = NULL;
	    newhistp->oph_exactcount = 0.0;
	    newhistp->oph_numexcells = 0;
    	    if ( attrp->opz_varnm < primary ) /* check for primary relation */
	    {
		newhistp->oph_attribute = attr; /* save the joinop attribute
					    ** associated with the histogram
					    */
		nocells = FALSE;
		if (
#ifdef    OPT_F037_STATISTICS
					    /* if trace flag is set then 
					    ** ignore histograms */
		    (	ops_cb->ops_check
			&&
			opt_strace(ops_cb, OPT_F037_STATISTICS)
		    )
		    ||
#endif
		    !oph_catalog( subquery, newhistp, attr, attrp, 
			attrp->opz_varnm, var_ptr, &attrp->opz_histogram,
			&nocells)
		    ||
		    nocells   /* hist was found but is all null */
		    )
                                            /* if histogram was not
                                            ** found in the catalogs then
                                            */
		    oph_uniform( subquery, newhistp, attrp, var_ptr,
					&attrp->opz_histogram, nocells); 
					    /* create a default 
                                            ** uniform distribution
                                            ** for the attribute
                                            */
	    }
	    else
		oph_index( subquery, newhistp, attr, attrp); /* create
                                            ** histogram for an attribute in
                                            ** an index by copying the
                                            ** corresponding histogram from the
                                            ** attribute of the base relation
                                            */

	    if ((!newhistp->oph_unique)	    /* if attribute is already unique
					    ** then no need to check for
					    ** secondary indices */
		&&
		(var_ptr->opv_mbf.opb_count == 1) /* if exactly one attribute is
					    ** referenced in the ordering 
					    ** in the qualification */
		&&
		(var_ptr->opv_mbf.opb_kbase->opb_keyorder[0].opb_attno == attr) /*
					    ** and that attribute is the one
					    ** that is being currently analyzed
					    */
		&&
		var_ptr->opv_grv->opv_relation
		&&
		var_ptr->opv_grv->opv_relation->rdr_keys /* if there are no other
					    ** attributes in this key */
		&&
		(var_ptr->opv_mbf.opb_mcount == 1) /* number of attributes in
                                            ** in the key must be 1 ... this
                                            ** is all that can be handled
                                            ** in the cost model */
		&&
		var_ptr->opv_grv->opv_relation->rdr_rel /* and this is a unique
					    ** storage structure */
		&&
		(   var_ptr->opv_grv->opv_relation->rdr_rel->tbl_status_mask 
		    & 
		    DMT_UNIQUEKEYS
		)
	       )
	    {
		newhistp->oph_unique = TRUE; /* set uniqueness if storage
					    ** structure will enforce it */
		if (!unique_found)
		{
		    unique_found = TRUE;    /* at least one unique key has been
					    ** found which altered the histogram
					    ** so make another pass to
					    ** update all join attribute 
					    ** descriptors
					    ** based on this attribute */
		    MEfill(sizeof(uattrmap), (u_char)0, (PTR)&uattrmap);
		}
		BTset((i4)attr, (char *)&uattrmap); /* set attribute to be
					    ** checked for uniqueness */
	    }
	    newhistp->oph_odefined = FALSE;
	}
	{
	    /* update some info in the cost ordering node associated with
            ** this histogram - this code should probably be somewhere else
            ** FIXME
            */
	    if (var_ptr->opv_mbf.opb_count > 0 /* is there an ordering attribute
                                               */
                &&
                var_ptr->opv_mbf.opb_kbase->opb_keyorder[0].opb_attno == attr 
						/* is this histogram on the 
						** ordering attribute */
                )
	    {
		/* make some better estimates of the adjacent duplicate
                ** factor and sortedness factor 
                */
		OPO_CO                *cop;	/* cost ordering associated
                                                ** with this base relation */

		cop = var_ptr->opv_tcop;	/* get respective cost
						** ordering node */
                cop->opo_ordeqc = abase->opz_attnums[attr]->opz_equcls; /*
                                                ** save the ordering equivalence
                                                ** class here */
		switch (cop->opo_storage)
		{

		case DB_ISAM_STORE:
		case DB_BTRE_STORE:
		{
		    cop->opo_cost.opo_sortfact = var_ptr->opv_trl->opn_reltups;
                    cop->opo_cost.opo_adfactor 
			= var_ptr->opv_trl->opn_histogram->oph_reptf;
		    break;
		}
		case DB_HASH_STORE:
		{
		    cop->opo_cost.opo_sortfact = 2.0 * (
			cop->opo_cost.opo_adfactor 
			    = var_ptr->opv_trl->opn_histogram->oph_reptf);
		    break;
		}
		}
	    }
	    if ((var_ptr->opv_mbf.opb_count > 2)
		&&
		!(attrp->opz_mask & OPZ_MAINDEX))
	    {	/* check if attribute is part of key, and mark attribute
		** so that optimization will not be baised towards FSM
		** - FIXME - need to use multi-attribute distribution */
		OPZ_IATTS	keyattr;
		for (keyattr = 0; keyattr < var_ptr->opv_mbf.opb_count; keyattr++)
		if (var_ptr->opv_mbf.opb_kbase->opb_keyorder[keyattr].opb_attno == attr)
		{   /* mark this attribute as well as all other attributes other
		    ** indexes and the base relation for this var */
		    OPE_IEQCLS	    eqcls;
		    OPZ_BMATTS	    *attrmap;
		    OPZ_IATTS	    attr1;
		    OPV_IVARS	    primary;

		    if (var_ptr->opv_index.opv_poffset >=0)
			primary = var_ptr->opv_index.opv_poffset;
		    else
			primary = attrp->opz_varnm;
		    eqcls = abase->opz_attnums[attr]->opz_equcls;
		    attrmap = &ebase->ope_eqclist[eqcls]->ope_attrmap;
		    for (attr1 = -1; (attr1 = BTnext((i4)attr1, (char *)attrmap, (i4)maxattr))>=0;)
		    {
			OPZ_ATTS	*attrp1;
			attrp1 = abase->opz_attnums[attr1];
			if ((attrp1->opz_varnm == primary)
			    ||
			    (	(attrp1->opz_varnm >= 0)
				&&
				(vbase->opv_rt[attrp1->opz_varnm]->opv_index.opv_poffset == primary)
			    ))
			    attrp1->opz_mask |= OPZ_MAINDEX;
		    }
		}
	    }
	}
    }
    if (unique_found)
    {   /* all attribute info has histogram information initialized, with
	** default histogram info, or optimizedb info, so update the
	** attribute descriptor with uniqueness information that is
	** available when unique secondary indexes are involved */
	OPZ_IATTS	attr2;

	for ( attr2 = 0; attr2 < maxattr; attr2++ )
	{   /* look at each attribute and check if it is based on an attribute
	    ** in the unique map */
	    OPZ_ATTS	*attrp2;		    /* ptr to joinop attribute being
					    ** analyzed
					    */
	    OPZ_IATTS	uattr;		    /* attribute which is a unique
					    ** secondary index */
	    attrp2 = abase->opz_attnums[attr2];
	    if (attrp2->opz_equcls == OPE_NOEQCLS)
		continue;		    /* continue if no equivalence class
					    ** has been defined since histogram
					    ** will not be useful for this
					    ** attribute
					    */
	    for (uattr = -1; (uattr = BTnext((i4)uattr, 
					     (char *)&uattrmap, 
					     (i4)maxattr
					    )
			     ) >= 0;)
	    {
		if ((attrp2->opz_equcls == abase->opz_attnums[uattr]->opz_equcls)
		    &&
		    (uattr != attr2)	    /* uniqueness is already set */
		   )
		{
		    OPV_IVARS	    varno;  /* primary relation of attribute
					    ** being analyzed */
		    OPZ_IATTS	    atno;   /* ingres attribute number of
					    ** relation being analyzed */
		    OPV_IVARS	    uvarno; /* primary relation of attribute
					    ** with unique index */
		    OPZ_IATTS	    uatno;  /* ingres attribute number in
					    ** primary of attribute with
					    ** unique secondary index */
		    oph_battr(subquery, uattr, &uvarno, &uatno); /* find the
					    ** var number and attribute number
					    ** from the base relation 
					    ** corresponding to the unique
					    ** secondary index */
		    oph_battr(subquery, attr2, &varno, &atno); /* find the var
					    ** number and attribute number from
					    ** the base relation corresponding
					    ** to the attribute being analyzed
					    */
		    if ((uvarno == varno) && (atno == uatno))
		    {	/* find the histogram for the attribute and mark
			** it as unique */
			OPH_HISTOGRAM *histop; /* corresponding histogram
					    ** element from primary relation */
			histop = *opn_eqtamp( subquery, &vbase->opv_rt
			    [attrp2->opz_varnm]->opv_trl->opn_histogram,
			    attrp2->opz_equcls, TRUE);
			histop->oph_unique = TRUE;
		    }
		}
	    }
	}
    }

    /* Before we leave - call oph_composites to look for potentially useful
    ** composite histograms. */
    oph_composites(subquery);
}
