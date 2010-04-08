/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
NO_OPTIM = nc4_us5
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
#define        OPZ_ADDATTS      TRUE
#include    <bt.h>
#include    <me.h>
#include    <opxlint.h>


/**
**
**  Name: OPZADDATTS.C - create or find joinop attribute
**
**  Description:
**      Routines to create or find a joinop attribute
**
**  History:    
**      16-jun-86 (seputis)    
**          initial creation from addatts.c
**      06-nov-89 (fred)
**          Added support for non-histogramable attributes.  Initially, this is
**	    for support of large objects, although it might be nice to extend
**	    this support to user defined datatypes as well.
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/

/*{
** Name: opz_addatts	- add joinop attributes to the array
**
** Description:
**      This routine will add or find the appropriate joinop attribute 
**      to the joinop attributes array given the 
**      (range variable, range variable attribute number) pair.
**
**	The exception to this rule occurs with function attributes which
**      always results in a new joinop  attribute being allocated.  A
**      function attribute uses "OPZ_FANUM" as the input attribute number.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      joinopvar                       index into joinop range table
**      dmfattr			        dmf attribute number
**      datatype                        ptr to ADT datatype
**
** Outputs:
**	Returns:
**	    joinop attribute number
**	Exceptions:
**	    none
**
** Side Effects:
**	    An entry may be allocated in the joinop attributes array 
**	    (OPS_SUBQUERY->ops_attrs)
**
** History:
**	20-apr-86 (seputis)
**          initial creation addatts
**      06-nov-89 (fred)
**          Added support for non-histogrammable datatypes.  This support
**	    entails providing a simple, coordinated replacement for this
**	    histogram.  In our case, the datatype & length will be varchar,
**	    minimum & maximum being those for varchar, and adc_helem()
**	    replacement will be the histogram for "nohistos".
**           
**          This code is done in OPF as opposed to ADF because I think it
**	    desirable that OPF manage the histogram replacement.  ADF will not
**	    know the appropriate values.  Furthermore, it is more desirable to
**	    teach OPF to manage w/out histograms;  however, that change is
**	    beyond the scope of this project.
**	26-dec-90 (seputis)
**	    init mask of booleans associated with attribute descriptor
**	18-jan-93 (rganski)
**	    Character Histogram Enhancements project:
**	    Initialize attr_ptr->opz_histogram.oph_dataval.db_length to 8
**	    before call to adc_hg_dtln(), which has been changed; this gives
**	    old behavior (limit for char types was 8). This length may be
**	    changed later when RDF gets info from iistatistics.
**	24-may-93 (rganski)
**	    Character Histograms Enhancements project:
**	    Initialize attr_ptr->opz_histogram.oph_dataval.db_length to 0
**	    before call to adc_hg_dtln(), which causes histogram value to be
**	    same length as attribute. This is necessary because otherwise the
**	    histogram value, which is used to determine selectivity of boolean
**	    factors, is truncated; this removes the benefits of having long
**	    histogram values. The histogram value length is adjusted in
**	    oph_catalog(), which reads the actual length from iistatistics (or
**	    truncates to 8, for old histograms).
**	6-dec-93 (ed)
**	    - bug 56139 - project union view before it is instantiated
**	09-oct-98 (matbe01)
**	    Added NO_OPTIM for NCR to eliminate runtime error that produces the
**	    message:  E_OP0889 Eqc is not available at a CO node.
**	 6-sep-04 (hayke02)
**	    Add OPZ_COLLATT attribute if OPS_COLLATION is on (resdom list,
**	    collation sequence). This change fixes problem INGSRV 2940, bug
**	    112873.
**	 8-nov-05 (hayke02)
**	    Add OPZ_VAREQVAR if OPS_VAREQVAR is on. Return if OPZ_VAREQVAR is
**	    not set and the attribute is found. This ensures that the fix for
**	    110675 is limited to OPZ_VAREQVAR PST_BOP nodes. This change
**	    fixes bug 115420 problem INGSRV 3465.
**       15-aug-2007 (huazh01)
**          ensure the datatype/length info of a created attribute matches 
**          the corresponding column type/length info on the user table. 
**          bug 117316.
**
[@history_line@]...
*/
OPZ_IATTS
opz_addatts(
	OPS_SUBQUERY       *subquery,
	OPV_IVARS	   joinopvar,
	OPZ_DMFATTR        dmfattr,
	DB_DATA_VALUE      *datatype)
{
	OPZ_IATTS              attribute;   /* used to save joinop attribute
					    ** number if found by opz_findatt
                                            */
        RDR_INFO		*rel;
   
        /* FIXME - need to deal with unsigned compare problem here */
	if ( (dmfattr != OPZ_FANUM) 
	     &&
             (dmfattr != OPZ_SNUM)
	     && 
             (dmfattr != OPZ_CORRELATED)
	     && 
	     (attribute = opz_findatt(subquery, joinopvar, dmfattr)) >= 0
	     &&
	     !(subquery->ops_attrs.opz_base->opz_attnums[attribute]->opz_mask &
								OPZ_VAREQVAR
	     &&
	     subquery->ops_mask2 & OPS_COLLATION
	     &&
	     (abs(datatype->db_datatype) == DB_CHA_TYPE
	     ||
	     abs(datatype->db_datatype) == DB_VCH_TYPE
	     ||
	     abs(datatype->db_datatype) == DB_CHR_TYPE
	     ||
	     abs(datatype->db_datatype) == DB_TXT_TYPE)) )
	    return( attribute );	    /* if this (joinopvar,dmfattr) pair 
					    ** exists and is not a function 
					    ** attribute, or subselect attribute
                                            ** then return
					    */

    {	/* create new joinop attribute */

	OPZ_IATTS		    nextattr; /* next available joinop attribute
                                            ** number
                                            */
	OPS_STATE                   *global; /* ptr to global state variable */

        global = subquery->ops_global;	    /* get ptr to global state variable 
                                            */
	if ( (nextattr = subquery->ops_attrs.opz_av++) >= OPZ_MAXATT)
	    opx_error(E_OP0300_ATTRIBUTE_OVERFLOW); /* exit with error
					    ** if no more room in attributes
                                            ** array
                                            */
	{
	    OPZ_ATTS               *attr_ptr;   /* ptr to newly created joinop 
					    ** attribute element */
	    OPV_VARS               *var_ptr; /* ptr to respective joinop
                                            ** variable containing attribute */

            var_ptr = subquery->ops_vars.opv_base->opv_rt[joinopvar];

	    /* allocate new joinop attribute structure and initialize */
	    subquery->ops_attrs.opz_base->opz_attnums[nextattr] = attr_ptr = 
		(OPZ_ATTS *) opu_memory( global, (i4) sizeof(OPZ_ATTS));
	    MEfill(sizeof(*attr_ptr), (u_char)0, (PTR)attr_ptr);


            /* b117316:
            **
            ** the fix to b109879 set PST_VAR node under the RESDOM to 
            ** to nullable and increase the db_length by one, though
            ** the column corresponds to PST_VAR is defined as not null.
            ** Need to reset them to not null in order to prevent
            ** wrong db_type and db_length being used during query
            ** execution. e.g., wrong db_type and db_length could cause OPC
            ** to generate a set of ADF code which materialize tuples
            ** using incorrect offset and cause wrong result. 
            **
            */
            rel = var_ptr->opv_grv->opv_relation;
            if (datatype->db_datatype < 0 && 
                dmfattr > 0 && 
                rel && 
                rel->rdr_attr[dmfattr]->att_type * -1 == datatype->db_datatype &&
                rel->rdr_attr[dmfattr]->att_width + 1 == datatype->db_length)
            {
                datatype->db_datatype = rel->rdr_attr[dmfattr]->att_type; 
                datatype->db_length = rel->rdr_attr[dmfattr]->att_width;
            }

	    attr_ptr->opz_varnm = joinopvar; /* index into local range table */
            BTset( (i4)nextattr, (char *)&var_ptr->opv_attrmap); /* indicate 
					    ** that this joinop
                                            ** attribute belongs to this range
                                            ** variable */
	    attr_ptr->opz_gvar = var_ptr->opv_gvar; /* move global range
                                            ** variable number to attribute */
	    attr_ptr->opz_attnm.db_att_id = dmfattr;/* dmf attribute of 
                                            ** range variable  */
	    STRUCT_ASSIGN_MACRO((* datatype), attr_ptr->opz_dataval);
	    attr_ptr->opz_equcls = OPE_NOEQCLS; /* this attribute has not been
                                            ** assigned an equivalence class yet
                                            */
	    attr_ptr->opz_func_att = OPZ_NOFUNCATT; /* this indicates that a
                                            ** function attribute is not
                                            ** associated with this joinop
                                            ** attribute element
                                            */
	    if ((dmfattr != OPZ_FANUM) 
		&&
		(dmfattr != OPZ_SNUM)
		&& 
		(dmfattr != OPZ_CORRELATED)
		&&
		(attribute >= 0))
		attr_ptr->opz_mask |= OPZ_COLLATT;
	    if (subquery->ops_mask2 & OPS_VAREQVAR)
		attr_ptr->opz_mask |= OPZ_VAREQVAR;
	    if ((var_ptr->opv_grv->opv_gmask & OPV_UVPROJECT)
		&&
		(dmfattr >= 0))
	    {	/* if union view exists in which a projection is possible 
		** (i.e. a UNION ALL view) then
		** mark the attribute which are referenced in the union view*/
		BTset((i4)dmfattr, (char *)var_ptr->opv_grv->opv_attrmap);
	    }
	    {
		/* initialize the histogram information associated with this
		** attribute
		*/
		DB_STATUS	hgstatus;	/* ADT return status */
		i4		dt_bits;

		hgstatus = adi_dtinfo(global->ops_adfcb,
					    datatype->db_datatype, &dt_bits);

		if ((hgstatus == E_AD0000_OK) && (dt_bits & AD_NOHISTOGRAM))
		{
		    attr_ptr->opz_histogram.oph_dataval.db_datatype =
							    OPH_NH_TYPE;
		    attr_ptr->opz_histogram.oph_dataval.db_prec =
							    OPH_NH_PREC;
		    attr_ptr->opz_histogram.oph_dataval.db_length =
							    OPH_NH_LENGTH;
		}
		else
		{
		    attr_ptr->opz_histogram.oph_dataval.db_length = 0;
		    hgstatus = adc_hg_dtln(global->ops_adfcb, datatype, 
			&attr_ptr->opz_histogram.oph_dataval );
		}
# ifdef E_OP0780_ADF_HISTOGRAM
		if ((hgstatus != E_AD0000_OK)
		    ||
		    (attr_ptr->opz_histogram.oph_dataval.db_datatype < 0)
					    /* do not expect a nullable
                                            ** histogram datatype */
		   )
		    opx_verror( hgstatus, E_OP0780_ADF_HISTOGRAM, 
			global->ops_adfcb->adf_errcb.ad_errcode); /*
                                            ** abort if unexpected error occurs
                                            */
# endif
		attr_ptr->opz_histogram.oph_dataval.db_data = NULL; /* init the
                                            ** data value ptr */
		attr_ptr->opz_histogram.oph_numcells = OPH_NOCELLS; /* histogram
                                            ** has not been fetched so no cells
                                            ** exist at this point
                                            */
		attr_ptr->opz_histogram.oph_histmm = NULL; /* no histogram
                                            ** exists at this point
					    */
                attr_ptr->opz_histogram.oph_mindomain = NULL; /* no minimum
                                            ** value for this histogram yet */
                attr_ptr->opz_histogram.oph_maxdomain = NULL; /* no maximum
					    ** value for this histogram yet */
	    }
	    if (dmfattr == DB_IMTID)
	    {
		/* implicit TID found so an equivalence class needs to be
                ** created ... so we know the equivalence class of the
                ** implicit TID for this (since the implicit TID is referenced
                ** by an index or explicitly in the qualification)
		*/
		var_ptr->opv_primary.opv_tid 
		    = ope_neweqcls( subquery, nextattr );
	    }
	}
    return( nextattr );			/* return joinop attribute which has
                                        ** been assigned
                                        */
    }
}
