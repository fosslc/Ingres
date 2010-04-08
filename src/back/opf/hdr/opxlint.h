/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: OPXLINT.H - FUNC_EXTERN used locally in OPF
**
** Description:
**      This file contains the FUNC_EXTERN definitions for procedures
**	used only within OPF.  It should be included in all OPF files.
**      This allows any procedure which is not static to be defined
**      in only one place, so modifications and additions do not require
**      changes to multiple files for FUNC_EXTERN definitions.
**
**	The ifdef'ed code is used for function prototypes so that
**      parameter passing can be checked when running lint when the
**      OPS_LINT is defined.  This can also be used to determine
**      whether all the externally referenced procedures are FUNC_EXTERN'ed
**      correctly.  The link test is done by removing the comments and
**	converting the function prototype to look like procedures and
**	then linkopf.com is run which compiles and links each module
**	individually, any unresolved reference is a missing function 
**	prototype or a missing CL header file
**
** History:
**      13-dec-86 (seputis)
**          initial creation
**	12-jul-91 (seputis)
**	    added #ifdef to FUNC_EXTERN's for CL, so that they can be #define'ed
**	09-nov-92 (rganski)
**	    Changed parameters in prototype of opn_diff().
**	09-nov-92 (ed)
**	    changed ops_decvar, opa_cunion, opa_gsub
**	14-dec-92 (rganski)
**	    Added datatype parameter to prototype of opn_diff().
**      18-jan-1993 (stevet)
**          Removed function prototypes for ADF since they are now defined
**          in ADF.H
**	2-feb-93 (ed)
**	    - fix bug 47377 - add new procedure opn_uniquecheck
**	8-feb-93 (ed)
**	    - bug 49317, avoid outer join processing for inner join id
**	9-mar-93 (walt)
**	    Commented out MEmove below so that the declarations of MEmove() in
**	    this file won't be used.  They conflict with <me.h> and cause OPF
**	    compiles to fail.
**	31-mar-93 (swm)
**	    Commented out STmove below so that the declarations of STmove() in
**	    this file won't be used.  They conflict with <st.h> and cause OPF
**	    compiles to fail.
**	21-may-93 (ed)
**	    added iftrue parameters to correct outer join function attribute
**	    processing problem
**	11-jun-93 (ed)
**	    removed CL definitions which are now in GLHDR
**	21-jun-93 (rganski)
**	    b50012: added opno parameter to, and removed reptf param from,
**	    oph_interpolate prototype.
**      20-jul-93 (ed)
**          changed name ops_lock for solaris, due to OS conflict
**	10-jul-93 (andre)
**	    removed prototypes for PSF functions since they are now prototyped
**	    in PSFPARSE.H
**	11-aug-93 (ed)
**	    - changed oph_tcompare to match formal declaration
**	    - removed opt_ffat and made static, added opt_catname to fix alpha
**	    stack overwrite problem
**	13-oct-93 (swm)
**	    Bug #56448
**	    Altered uld_prnode calls to conform to new portable interface.
**	    It is no longer a "pseudo-varargs" function. It cannot become a
**	    varargs function as this practice is outlawed in main line code.
**	    Instead it takes a formatted buffer as its only parameter.
**	    Each call which previously had additional arguments has an
**	    STprintf embedded to format the buffer to pass to uld_prnode.
**	    This works because STprintf is a varargs function in the CL.
**	31-jan-94 (rganski)
**	    Added prototype for oph_hvalue, which is now an external function.
**	    b47104, b47733 (key join bug): added new function opn_readahead.
**	11-feb-94 (ed)
**	    remove unneeded opl_histogram parameters
**	15-feb-94 (ed)
**	    bug 49814 - E_OP0397 consistency check
**	7-mar-94 (ed) - changed some normalization routines from static to
**	    allow them to be used for outer join flattening
**      25-mar-94 (ed)
**          bug 59355 - an outer join can change the result of a resdom
**          expression from non-null to null, this change needs to be propagated
**          for union view target lists to all parent query references in var nodes
**          or else OPC/QEF will not allocate the null byte and access violations
**          may occur
**      11-apr-94 (ed)
**          b59937 - added opl_ijcheck, opl_sjij
**	    b59588 - changed from static opb_bfinit, opb_bfget
**	25-apr-94 (rganski)
**	    b59982 - added histdt parameter to prototype for opb_bfkget().
**	13-may-94 (ed)
**	    add parameter to allow function attribute qualifications to
**	    be returned to where clause
**      17-feb-95 (inkdo01)
**          added opx_1perror to display 1 token error messages
**      06-mar-96 (nanpr01)
**          Added parameter for opn_tpblk for multiple page size project. 
**	    Added new function prototypes.
**	8-may-97 (inkdo01)
**	    Add bool parm to opb_bfinit.
**	    Added casts to quite compiler warnings
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**	10-aug-99 (inkdo01)
**	    Added opa_subsel_to_oj for NOT EXISTS to outer join transformation.
**	1-feb-00 (inkdo01)
**	    Added opt_qep_dump to give better QEP display.
**      01-Jan-00 (sarjo01)
**          Bug 99120: Added new parameter tbl so collation sequence
**          can be used by opn_diff where applicable.
**	 5-mar-01 (inkdo01 & hayke02)
**	    Added OPN_RLS ** parms to oph_jselect, oph_relselect. This
**	    change fixes bug 103858.
**	19-nov-01 (inkdo01)
**	    Added numleaves parm to opn_deltrl for proper memory linkage.
**	27-aug-02 (inkdo01)
**	    Added bool parm to opn_gnperm call.
**	3-dec-02 (inkdo01)
**	    Changed OPV_GBMVARS to OPV_GBMVARS * parms in opz_savequal
**	    for range table expansion.
**	16-jan-04 (inkdo01)
**	    Added call to opb_pqbf for partitioned tables.
**	30-mar-04 (hayke02)
**	    Added bool longjump to opn_exit() call.
**	7-apr-04 (inkdo01)
**	    Added opv_pcjnbl for partitioned tables/parallel query.
**	4-nov-04 (inkdo01)
**	    Added parms to opn_cpmemory, opo_copyfragco for greedy enum.
**	24-Feb-2005 (schka24)
**	    EX stuff doesn't belong here, caused compiler warnings.
**	15-Mar-2005 (schka24)
**	    Redo above after belated x-integration from aix port put it back.
**	26-apr-05 (inkdo01)
**	    Added opt_fcodisplay to externalize op161 CO node displays.
**	5-july--5 (inkdo01)
**	    Added opn_modcost() for agg sq row count alteration.
**	10-oct-05 (inkdo01)
**	    Added opn_urn() for modified join estimates.
**	25-Oct-2005 (hanje04)
**	    Move declaration of opt_ffat() to optcotree.c as it's a static
**	    function and GCC 4.0 doesn't like it declared here as otherwise.
**	20-mar-06 (dougi)
**	    Change opo_copyco() to return DB_STATUS (for hints project).
**	30-Jun-2006 (kschendel)
**	    Update call to modcost.
**	 4-oct-05 (hayke02)
**	    Added outerp to opl_ojverify() call as part of fix for bug 114807,
**	    problem INGSRV 3349.
**	31-Jul-2007 (kschendel) SIR 122513
**	    Additions for new partition qualification code.
**	10-Mar-2009 (kibro01) b121740
**	    Add opn_eqtamp_multi to allow a check on multiple use of histogram.
**	2-Jun-2009 (kschendel) b122118
**	    Kill unused oj-filter parameters.
**	24-Aug-2009 (kschendel) 121804
**	    Add some missing prototypes.
**	13-Nov-2009 (kiria01) SIR 121883
**	    Update prototype for opb_gbfbm
**	02-Dec-2009 (whiro01)
**	    Added prototype for "opn_nonsystem_rule" per Karl after it was made
**	    non-static to be called from outside here.
**	26-Feb-10 (smeke01) b123349
**	    Amend opn_prleaf() to return OPN_STATUS in order to make trace point
**	    op255 work with set joinop notimeout.
**	22-mar-10 (smeke01) b123457
**	    Amend function signature for opu_qtprint for trace point op214.
[@history_line@]...
**/

/* # ifndef OPD_COST */
FUNC_EXTERN OPO_COST opd_msite(
		OPS_SUBQUERY       *subquery,
		OPO_CO             *cop,
		OPD_ISITE	   target_site,
		bool		   root_flag,
		OPO_CO		   *marker,
		bool		   remove);

FUNC_EXTERN bool opd_jcost(
		OPS_SUBQUERY       *subquery,
		OPN_RLS		   *main_rlsp,
		OPN_SUBTREE        *main_subtp,
		OPO_CO             *newcop,
		OPS_WIDTH	   width,
		OPO_CPU		   sort_cpu,
		OPO_BLOCKS	   sort_dio);

FUNC_EXTERN VOID opd_idistributed(
		OPS_STATE	*global);

FUNC_EXTERN VOID opd_isub_dist(
		OPS_SUBQUERY       *subquery);

FUNC_EXTERN bool opd_prleaf(
		OPS_SUBQUERY       *subquery,
		OPN_RLS		   *rlsp,
		OPO_CO             *cop,
		OPS_WIDTH	   tuple_width,
		OPO_CPU		   sort_cpu,
		OPO_BLOCKS	   sort_dio);

FUNC_EXTERN VOID opd_orig(
		OPS_SUBQUERY       *subquery,
		OPO_CO             *cop);

FUNC_EXTERN VOID opd_addsite(
		OPS_STATE	    *global,
		OPV_GRV		   *gvarp);

FUNC_EXTERN VOID opd_recover(
		OPS_STATE	    *global);

FUNC_EXTERN bool opd_bestplan(
		OPS_SUBQUERY       *subquery,
		OPO_CO             *newcop,
		bool		   sortrequired,
		OPO_CPU		   sort_cpu,
		OPO_BLOCKS	   sort_dio,
		OPO_CO		   **sortcopp);

FUNC_EXTERN bool opd_bflag(
		OPS_STATE	   *global);
/* #endif */

/* # ifndef OPA_AGGREGATE */
FUNC_EXTERN VOID opa_aggregate(
		OPS_STATE          *global);
/* #endif */

/* # ifndef OPA_BYPROJECT */
FUNC_EXTERN VOID opa_byproject(
		OPS_STATE          *global);
/* #endif */

/* # ifndef OPA_COMPAT */
FUNC_EXTERN VOID opa_compat(
		OPS_STATE          *global);

FUNC_EXTERN VOID opa_agbylist(
		OPS_SUBQUERY       *subquery,
		bool		    agview,
		bool		    project);

FUNC_EXTERN VOID opa_gsub(
		OPS_SUBQUERY       *subquery);

/* #endif */

/* # ifndef OPA_FINAL */
FUNC_EXTERN VOID opa_final(
		OPS_STATE          *global);

FUNC_EXTERN VOID opa_stid(
		OPS_SUBQUERY	   *subquery);

FUNC_EXTERN VOID opa_tnode(
		OPS_STATE          *global,
		OPZ_IATTS          rsno,
		OPV_IGVARS	   varno,
		PST_QNODE          *root);
/* #endif */

/* # ifndef OPA_GENERATE */
FUNC_EXTERN VOID opa_1pass(
		OPS_STATE          *global);

FUNC_EXTERN VOID opa_subsel_to_oj(
		OPS_STATE          *global,
		bool		   *gotone);

FUNC_EXTERN VOID opa_not(
		OPS_STATE          *global,
		PST_QNODE          *tree);

FUNC_EXTERN bool opa_rsmap(
		OPS_STATE	   *global,
		PST_QNODE	   *root,
		OPZ_BMATTS         *attrmap);

FUNC_EXTERN VOID opa_covar(
		OPS_SUBQUERY       *father,
		PST_QNODE          *varnodep,
		OPV_IGVARS	   varno);

FUNC_EXTERN VOID opa_resolve(
		OPS_SUBQUERY	   *subquery,
		PST_QNODE          *nodep);
/* #endif */

/* # ifndef OPA_INITSQ */
FUNC_EXTERN OPS_SUBQUERY * opa_initsq(
		OPS_STATE          *global,
		PST_QNODE          **agg_qnode,
		OPS_SUBQUERY       *father);

FUNC_EXTERN VOID opa_ifagg(
		OPS_SUBQUERY       *new_subquery,
		PST_QNODE	   *byhead);
/* #endif */

/* # ifndef OPA_LINK */
FUNC_EXTERN VOID opa_link(
		OPS_STATE          *global);
/* #endif */

/* # ifndef OPA_OPTIMIZE */
FUNC_EXTERN VOID opa_optimize(
		OPS_STATE          *global);
/* #endif */

/* # ifndef OPA_PRIME */
FUNC_EXTERN bool opa_prime(
		OPS_SUBQUERY       *subquery);
/* #endif */

/* # ifndef OPB_BFKGET */
FUNC_EXTERN OPB_BFKEYINFO * opb_bfkget(
		OPS_SUBQUERY       *subquery,
		OPB_BOOLFACT       *bp,
		OPE_IEQCLS         eqcls,
		DB_DATA_VALUE      *datatype,
		DB_DATA_VALUE	   *histdt,
		bool               mustfind);
/* #endif */

/* # ifndef OPB_CREATE */
FUNC_EXTERN VOID opb_mboolfact(
		OPS_SUBQUERY       *subquery,
		PST_QNODE	   *expr1,
		PST_QNODE          *expr2,
		PST_J_ID           joinid);

FUNC_EXTERN VOID opb_create(
		OPS_SUBQUERY       *subquery);

FUNC_EXTERN bool opb_bfinit(
		OPS_SUBQUERY       *subquery,
		OPB_BOOLFACT       *bfp,
		PST_QNODE          *root,
		bool		   *complex);

FUNC_EXTERN OPB_BOOLFACT * opb_bfget(
		OPS_SUBQUERY       *subquery,
		OPB_BOOLFACT       *bfp);

/* #endif */

/* # ifndef OPB_JKEYLIST */
FUNC_EXTERN VOID opb_jkeylist(
		OPS_SUBQUERY       *subquery,
		OPV_IVARS          varno,
		OPO_STORAGE        storage,
		RDD_KEY_ARRAY      *keylist,
		OPB_MBF            *mbf);
/* #endif */

/* # ifndef OPB_MBFK */
FUNC_EXTERN bool opb_mbf(
		OPS_SUBQUERY       *subquery,
		OPO_STORAGE        storage,
		OPB_MBF            *mbf);
/* #endif */

/* # ifndef OPB_PMBF */
FUNC_EXTERN VOID opb_pmbf(
		OPS_SUBQUERY       *subquery);
/* #endif */

FUNC_EXTERN VOID opb_pqbf(
		OPS_SUBQUERY       *subquery);

FUNC_EXTERN VOID opb_pqbf_findeqc(
		DB_PART_DEF	*partp,
		OPB_PQBF	*pqbfp,
		OPE_IEQCLS	eqc,
		i4		*dim,
		i4		*col_index);

/* # ifndef OPB_SFIND */
FUNC_EXTERN bool opb_sfind(
		OPB_BOOLFACT       *bp,
		OPE_IEQCLS         eqcls);
/* #endif */

/* # ifndef OPC_QUERYCOMP */
FUNC_EXTERN VOID opc_querycomp(
		OPS_STATE          *global);
/* #define  OPC_QUERYCOMP TRUE */
/* #endif */

/* # ifndef OPE_ADDEQCLS */
FUNC_EXTERN bool ope_addeqcls(
		OPS_SUBQUERY       *subquery,
		OPE_IEQCLS         eqcls,
		OPZ_IATTS          attr,
		bool		   nulljoin);
/* #endif */

/* # ifndef OPE_ASEQCLS */
FUNC_EXTERN VOID ope_aseqcls(
		OPS_SUBQUERY       *subquery,
		OPE_BMEQCLS        *eqclsmap,
		PST_QNODE          *nodep);

FUNC_EXTERN VOID ope_qual(
		OPS_SUBQUERY       *subquery);
/* #endif */

/* # ifndef OPE_CREATE */
FUNC_EXTERN VOID ope_create(
		OPS_SUBQUERY       *subquery);
/* #endif */

/* # ifndef OPE_FINDJOINS */
FUNC_EXTERN VOID ope_findjoins(
		OPS_SUBQUERY       *subquery);
/* #endif */

/* # ifndef OPE_MERGEATTS */
FUNC_EXTERN bool ope_mergeatts(
		OPS_SUBQUERY       *subquery,
		OPZ_IATTS	   firstattr,
		OPZ_IATTS	   secondattr,
		bool		   nulljoin,
		OPL_IOUTER	   ojid);
/* #endif */

/* # ifndef OPE_NEWEQCLS */
FUNC_EXTERN OPE_IEQCLS ope_neweqcls(
		OPS_SUBQUERY       *subquery,
		OPZ_IATTS          attr);
/* #endif */

/* # ifndef OPE_TWIDTH */
FUNC_EXTERN OPS_WIDTH ope_twidth(
		OPS_SUBQUERY       *subquery,
		OPE_BMEQCLS        *emap);
/* #endif */

/* # ifndef OPE_TYPE */
FUNC_EXTERN DB_DATA_VALUE * ope_type(
		OPS_SUBQUERY       *subquery,
		OPE_IEQCLS         eqcls);
/* #endif */

/* # ifndef OPH_CBMEMORY */
FUNC_EXTERN OPH_CELLS  oph_cbmemory(
		OPS_SUBQUERY       *subquery,
		i4	   	   arraysize);
/* #endif */

/* # ifndef OPH_CCMEMORY */
FUNC_EXTERN OPH_COUNTS oph_ccmemory(
		OPS_SUBQUERY       *subquery,
		OPH_INTERVAL	   *intervalp);
/* #endif */

/* # ifndef OPH_CREATE */
FUNC_EXTERN OPH_HISTOGRAM * oph_create(
		OPS_SUBQUERY       *subquery,
		OPH_HISTOGRAM      *hp,
		OPN_PERCENT        initfact,
		OPE_IEQCLS         eqcls);
/* #endif */

/* # ifndef OPH_DCBMEMORY */
FUNC_EXTERN VOID oph_dcbmemory(
		OPS_SUBQUERY       *subquery,
		OPH_CELLS          cellptr);
/* #endif */

/* # ifndef OPH_DCCMEMORY */
FUNC_EXTERN VOID oph_dccmemory(
		OPS_SUBQUERY       *subquery,
		OPH_INTERVAL       *intervalp,
		OPH_COUNTS         counts);
/* #endif */

/* # ifndef OPH_DMEMORY */
FUNC_EXTERN VOID oph_dmemory(
		OPS_SUBQUERY       *subquery,
		OPH_HISTOGRAM      *histp);
/* #endif */

/* # ifndef OPHHISTOS */
FUNC_EXTERN VOID oph_histos(
		OPS_SUBQUERY       *subquery);

FUNC_EXTERN PTR oph_hvalue(
	        OPS_SUBQUERY       *subquery,
		DB_DATA_VALUE      *histdt,
		DB_DATA_VALUE      *valuedt,
		PTR                key,
		ADI_OP_ID	   opid);
/* #endif */

/* # ifndef OPHINTERPOLATE */
FUNC_EXTERN OPN_PERCENT oph_interpolate(
		OPS_SUBQUERY       *subquery,
		PTR                minv,
		PTR                maxv,
		PTR                val,
		OPB_SARG           operator,
		DB_DATA_VALUE      *datatype,
		f4                 cellcount,
		ADI_OP_ID	   opno,
		OPH_HISTOGRAM	   *hp);
/* #endif */

/* # ifndef OPH_MAXPTR */
FUNC_EXTERN PTR oph_maxptr(
		OPS_STATE          *global,
		PTR                hist1ptr,
		PTR                hist2ptr,
		DB_DATA_VALUE      *datatype);
/* #endif */

/* # ifndef OPH_MEMORY */
FUNC_EXTERN OPH_HISTOGRAM * oph_memory(
		OPS_SUBQUERY       *subquery);
/* #endif */

/* # ifndef OPH_MINPTR */
FUNC_EXTERN PTR oph_minptr(
		OPS_STATE          *global,
		PTR                hist1ptr,
		PTR                hist2ptr,
		DB_DATA_VALUE      *datatype);
/* #endif */

/* # ifndef OPH_PDF */
FUNC_EXTERN VOID oph_pdf(
		OPS_SUBQUERY       *subquery,
		OPH_HISTOGRAM      *histp);
/* #endif */

/* # ifndef OPH_TCOMPARE */
FUNC_EXTERN i4  oph_tcompare(
		char		   *hist1ptr,
		char		   *hist2ptr,
		DB_DATA_VALUE      *datatype);
/* #endif */

/* # ifndef OPH_UPDATE */
FUNC_EXTERN VOID oph_normalize(
		OPS_SUBQUERY       *subquery,
		OPH_HISTOGRAM      *histp,
		OPN_PERCENT        newarea);
/* #endif */

/* # ifndef OPJ_JOINOP */
FUNC_EXTERN VOID opj_joinop(
		OPS_STATE          *global);
/* #endif */

/* # ifndef OPJ_UNION */
FUNC_EXTERN VOID opj_union(
		OPS_SUBQUERY	    *subquery);

FUNC_EXTERN VOID  opj_uboolfact(
		OPS_SUBQUERY	    *subquery);
/* #endif */

/* # ifndef OPJ_NORMALIZE */
FUNC_EXTERN VOID opj_mvors(
		PST_QNODE *qual);

FUNC_EXTERN VOID opj_adjust(
		OPS_SUBQUERY	*subquery,
		PST_QNODE	**qual);

FUNC_EXTERN VOID opj_mvands(
		OPS_SUBQUERY	*subquery,
		PST_QNODE	*qual);

FUNC_EXTERN VOID opj_normalize(
		OPS_SUBQUERY       *subquery,
		PST_QNODE	   **nodepp);
/* #endif */

/* # ifndef OPJ_KEEPDUPS */
FUNC_EXTERN VOID opj_keepdups(
		OPS_SUBQUERY       *subquery);
/* #endif */

/* # ifndef OPN_ARL */
FUNC_EXTERN bool opn_arl(
		OPS_SUBQUERY       *subquery,
		OPN_JTREE          *root,
		OPN_STLEAVES	   permutation,
		OPN_STATUS	   *sigstat);

FUNC_EXTERN bool opn_jintersect(
		OPS_SUBQUERY       *subquery,
		OPN_JTREE	   *nodep,
		OPN_STATUS	   *sigstat);

FUNC_EXTERN bool opl_ojverify(
		OPS_SUBQUERY	    *subquery,
		OPZ_BMATTS	    *attrmap,
		OPV_BMVARS	    *vmap,
		OPL_OUTER	    *outerp);

FUNC_EXTERN bool opn_tjconnected(
		OPS_SUBQUERY	   *subquery,
		OPV_IVARS	   *partition,
		OPV_IVARS	   maxvar,
		OPV_BMVARS	   *tidmap,
		OPN_JTREE	   *childp);

FUNC_EXTERN bool opn_cartprod(
		OPS_SUBQUERY       *subquery,
		OPV_IVARS          *partition,
		OPV_IVARS          maxvar,
		OPV_IVARS	   keyedvar,
		bool		   *notjoinable);
/* #endif */

/* # ifndef OPN_BFKHGET */
FUNC_EXTERN OPB_BFKEYINFO * opn_bfkhget(
		OPS_SUBQUERY       *subquery,
		OPB_BOOLFACT       *bp,
		OPE_IEQCLS         eqc,
		OPH_HISTOGRAM      *hp,
		bool               mustfind,
		OPH_HISTOGRAM      **hpp);
/* #endif */

/* # ifndef OPN_CALCOST */
FUNC_EXTERN OPN_STATUS opn_calcost(
		OPS_SUBQUERY       *subquery,
		OPO_ISORT	   jordering,
		OPO_ISORT	   keyjoin,
		OPN_SUBTREE        *osbtp,
		OPN_SUBTREE        *isbtp,
		OPN_SUBTREE        *jsbtp,
		OPN_RLS            *orlp,
		OPN_RLS            *irlp,
		OPN_RLS            *rlp,
		OPO_TUPLES	   tuples,
		OPN_EQS            *eqsp,
		OPO_BLOCKS	   blocks,
		OPN_EQS            *oeqsp,
		OPN_EQS            *ieqsp,
		OPN_JTREE	   *np,
		OPL_IOUTER         ojid,
		bool		   kjonly);

FUNC_EXTERN bool opn_goodco(
		OPS_SUBQUERY	   *subquery,
		OPO_CO             *cop);

FUNC_EXTERN VOID opn_modcost(
		OPS_SUBQUERY	   *subquery);
/* #endif */

/* # ifndef OPN_CEVAL */
FUNC_EXTERN OPN_STATUS opn_ceval(
		OPS_SUBQUERY       *subquery);

FUNC_EXTERN bool opn_timeout(
		OPS_SUBQUERY       *subquery);

FUNC_EXTERN VOID opn_recover(
		OPS_SUBQUERY       *subquery);
/* #endif */

/* # ifndef OPN_CHECKSORT */
FUNC_EXTERN bool opn_checksort(
		OPS_SUBQUERY       *subquery,
		OPO_CO		   **oldcopp,
		OPO_COST	   oldcost,
		OPO_BLOCKS	   newdio,
		OPO_CPU            newcpu,
		OPN_EQS            *eqsp,
		OPO_COST           *newcostp,
		OPO_ISORT	   available_ordering,
		bool		   *sortrequired,
		OPO_CO		   *newcop);

FUNC_EXTERN VOID opn_createsort(
		OPS_SUBQUERY       *subquery,
		OPO_CO		   *sortcop,
		OPO_CO             *outercop,
		OPN_EQS            *eqsp);

FUNC_EXTERN VOID opn_cleanup(
		OPS_SUBQUERY       *subquery,
		OPO_CO             *cop,
		OPO_BLOCKS         *dio,
		OPO_CPU            *cpu);

FUNC_EXTERN OPO_CO * opn_oldplan(
		OPS_SUBQUERY       *subquery,
		OPO_CO             **oldcopp);
/* #endif */

/* # ifndef OPN_CMEMORY */
FUNC_EXTERN OPO_CO * opn_cmemory(
		OPS_SUBQUERY       *subquery);
/* #endif */

FUNC_EXTERN OPO_PERM * opn_cpmemory(
		OPS_SUBQUERY       *subquery,
		i4		   cocount);

/* # ifndef OPO_INITCO */
FUNC_EXTERN VOID opo_initco(
		OPS_SUBQUERY	   *subquery,
		OPO_CO             *cop);
/* #endif */

/* # ifndef OPN_COINSERT */
FUNC_EXTERN VOID opn_coinsert(
		OPN_SUBTREE        *sbtp,
		OPO_CO             *cop);
/* #endif */

/* # ifndef OPN_COST */
FUNC_EXTERN OPO_COST opn_cost(
		OPS_SUBQUERY       *subquery,
		OPO_BLOCKS         dio,
		OPO_CPU            cpu);
/* #endif */

/* # ifndef OPN_DCMEMORY */
FUNC_EXTERN VOID opn_dcmemory(
		OPS_SUBQUERY       *subquery,
		OPO_CO             *cop);
/* #endif */

/* # ifndef OPN_DELTRL */
FUNC_EXTERN VOID opn_deltrl(
		OPS_SUBQUERY       *subquery,
		OPN_RLS            *trl,
		OPE_IEQCLS         jeqc,
		OPN_LEAVES	   numleaves);
/* #endif */

/* # ifndef OPN_DEMEMORY */
FUNC_EXTERN VOID opn_dememory(
		OPS_SUBQUERY       *subquery,
		OPN_EQS            *eqclp);
/* #endif */

/* # ifndef OPN_RDIFF */
FUNC_EXTERN OPN_DIFF opn_diff(
		PTR                value1,
		PTR                value2,
		i4		   *charlength,
		OPO_TUPLES	   celltups,
		OPZ_ATTS	   *attrp,
		DB_DATA_VALUE	   *datatype,
		PTR		   tbl);

FUNC_EXTERN void opn_initcollate(
		OPS_STATE	*global);

/* #endif */

/* # ifndef OPN_DRMEMORY */
FUNC_EXTERN VOID opn_drmemory(
		OPS_SUBQUERY       *subquery,
		OPN_RLS            *trl);
/* #endif */

/* # ifndef OPN_DSMEMORY */
FUNC_EXTERN VOID opn_dsmemory(
		OPS_SUBQUERY       *subquery,
		OPN_SUBTREE	   *subtp);
/* #endif */

/* # ifndef OPN_EMEMORY */
FUNC_EXTERN OPN_EQS * opn_ememory(
		OPS_SUBQUERY       *subquery);
/* #endif */

/* # ifndef OPN_ENUM */
FUNC_EXTERN VOID opn_enum(
		OPS_SUBQUERY       *subquery);

FUNC_EXTERN bool opn_nonsystem_rule(
                OPS_SUBQUERY    *subquery);

FUNC_EXTERN bool opn_uniquecheck(
		OPS_SUBQUERY	*subquery,
		OPV_BMVARS	*vmap,
		OPE_BMEQCLS	*jeqcmap,
		OPE_IEQCLS	eqcls);
/* #endif */

/* # ifndef OPN_EPRIME */
FUNC_EXTERN f4 opn_eprime(
		OPO_TUPLES         rballs,
		OPO_TUPLES	   mballspercell,
		OPH_DOMAIN	   ncells,
		f8		   underflow);
/* #endif */

/* # ifndef OPN_EQTAMP */
FUNC_EXTERN OPH_HISTOGRAM ** opn_eqtamp(
		OPS_SUBQUERY       *subquery,
		OPH_HISTOGRAM      **hp,
		OPE_IEQCLS         eqcls,
		bool               mustfind);

FUNC_EXTERN OPH_HISTOGRAM ** opn_eqtamp_multi(
		OPS_SUBQUERY       *subquery,
		OPH_HISTOGRAM      **hp,
		OPE_IEQCLS         eqcls,
		bool               mustfind,
		bool		   *multiple_eqcls);
/* #endif */

/* # ifndef OPN_EXIT */
FUNC_EXTERN VOID opn_exit(
		OPS_SUBQUERY	*subquery,
		bool		longjump);
/* #endif */

/* # ifndef OPN_GNPERM */
FUNC_EXTERN bool opn_gnperm(
		OPS_SUBQUERY       *subquery,
		OPN_STLEAVES	   permutation,
		OPN_LEAVES         numleaves,
		OPN_PARTSZ         partvar,
		OPV_BMVARS         *pr_n_included,
		bool		   firstcomb);
/* #endif */

/* # ifndef OPN_HINT */
FUNC_EXTERN bool opn_index_hint(
		OPS_SUBQUERY       *subquery,
		OPN_STLEAVES	   combination,
		OPN_LEAVES         numleaves);
 
FUNC_EXTERN bool opn_nonkj_hint(
		OPS_SUBQUERY	   *subquery,
		OPN_JTREE	   *nodep);

FUNC_EXTERN bool opn_fsmCO_hint(
		OPS_SUBQUERY	   *subquery,
		OPO_CO		   *nodep);

FUNC_EXTERN bool opn_kj_hint(
		OPS_SUBQUERY	   *subquery,
		OPN_JTREE	   *nodep,
		bool		   *lkjonly,
		bool		   *rkjonly);

FUNC_EXTERN bool opn_order_hint(
		OPS_SUBQUERY	   *subquery,
		OPN_JTREE	   *nodep);
/* #endif */

/* # ifndef OPH_RELSELECT */
FUNC_EXTERN VOID oph_relselect(
		OPS_SUBQUERY	   *subquery,
		OPN_RLS		   **trlpp,
		OPN_RLS            *trl,
		OPB_BMBF           *bfm,
		OPE_BMEQCLS        *eqh,
		bool		   *selapplied,
		OPN_PERCENT        *selectivity,
		OPE_IEQCLS         jeqc,
		OPN_LEAVES         nleaves,
		OPN_JEQCOUNT       jxcnt,
		OPV_IVARS	   varno);
/* #endif */

/* # ifndef OPH_JSELECT */
FUNC_EXTERN VOID oph_jselect(
		OPS_SUBQUERY       *subquery,
		OPN_JTREE          *np,
		OPN_RLS		   **trlpp,
		OPN_SUBTREE        *lsubtp,
		OPN_EQS            *leqclp,
		OPN_RLS            *lrlmp,
		OPN_SUBTREE        *rsubtp,
		OPN_EQS            *reqclp,
		OPN_RLS            *rrlmp,
		OPO_ISORT          jordering,
		OPO_ISORT          jxordering,
		OPE_BMEQCLS        *jeqh,
		OPE_BMEQCLS        *byeqcmap,
		OPL_OJHISTOGRAM    *ojhistp);
/* #endif */

/* # ifndef OPN_IMTIDJOIN */
FUNC_EXTERN bool opn_imtidjoin(
		OPS_SUBQUERY       *subquery,
		OPN_JTREE          *lp,
		OPN_JTREE          *rp,
		OPE_IEQCLS         eqcls,
		bool               *rfflag);
/* #endif */

/* # ifndef OPN_IOMAKE */
FUNC_EXTERN VOID opn_iomake(
		OPS_SUBQUERY       *subquery,
		OPV_BMVARS         *rlmap,
		OPE_BMEQCLS        *eqm,
		OPE_BMEQCLS        *iom);
/* #endif */

/* # ifndef OPN_JMAPS */
FUNC_EXTERN bool opn_jmaps(
		OPS_SUBQUERY       *subquery,
		OPN_JTREE          *nodep,
		OPL_BMOJ	   *ojmap);
/* #endif */

/* # ifndef OPN_JPRR */
FUNC_EXTERN VOID opn_jprr(
		OPS_SUBQUERY       *subquery,
		OPN_SUBTREE        *jsbtp,
		OPN_EQS            *eqsp,
		OPN_RLS            *rlsp,
		OPO_BLOCKS         blocks);
/* #endif */

/* # ifndef OPN_JMEMORY */
FUNC_EXTERN PTR opn_jmemory(
		OPS_STATE          *global,
		i4                size);
/* #endif */

/* # ifndef OPN_FMEMORY */
FUNC_EXTERN VOID opn_fmemory(
		OPS_SUBQUERY       *subquery,
		PTR                *targetptr);
/* #endif */

/* # ifndef OPN_MEMORY */
FUNC_EXTERN PTR opn_memory(
		OPS_STATE          *global,
		i4                size);
/* #endif */

/* # ifndef OPN_NCOMMON */
FUNC_EXTERN VOID opn_ncommon(
		OPS_SUBQUERY       *subquery,
		OPN_JTREE          *nodep,
		OPN_RLS            **rlmp,
		OPN_EQS            **eqclp,
		OPN_SUBTREE        **subtp,
		OPE_BMEQCLS        *eqr,
		OPN_SRESULT        search,
		OPN_LEAVES         nleaves,
		OPO_BLOCKS         *blocks,
		bool               selapplied,
		OPN_PERCENT        selectivity,
		OPO_TUPLES         *tuples);
/* #endif */

/* # ifndef OPN_NODECOST */
FUNC_EXTERN bool opn_nodecost(
		OPS_SUBQUERY 	   *subquery,
		OPN_JTREE          *np,
		OPE_BMEQCLS        *eqr,
		OPN_SUBTREE        **subtp,
		OPN_EQS            **eqclp,
		OPN_RLS            **rlmp,
		OPN_STATUS	   *sigstat);
/* #endif */

/* # ifndef OPNPARTITION */
FUNC_EXTERN bool opn_partition(
		OPS_SUBQUERY       *subquery,
		OPN_STLEAVES       partbase,
		OPN_LEAVES         numleaves,
		OPN_PARTSZ         partsz,
		OPN_CHILD          numpartitions,
		bool               parttype);
/* #endif */

/* # ifndef OPN_PRCOST */
FUNC_EXTERN VOID opn_prcost(
		OPS_SUBQUERY       *subquery,
		OPO_CO             *cop,
		OPB_SARG           *sargtype,
		OPO_BLOCKS	   *pblk,
		OPH_HISTOGRAM      **hpp,
		bool		   *imtid,
		bool               order_only,
		OPO_ISORT          *ordering,
		bool               *pt_valid,
		OPO_BLOCKS	   *dio,
		OPO_CPU		   *cpu);
/* #endif */

/* # ifndef OPN_PROCESS */
FUNC_EXTERN OPN_STATUS opn_process(
		OPS_SUBQUERY       *subquery,
		i4                numleaves);

FUNC_EXTERN VOID opn_cjtree(
		OPS_SUBQUERY       *subquery,
		OPN_JTREE	   *jtree);

/* #endif */

/* # ifndef OPN_PRRLEAF */
FUNC_EXTERN OPN_STATUS opn_prleaf(
		OPS_SUBQUERY       *subquery,
		OPN_SUBTREE        *jsbtp,
		OPN_EQS            *eqsp,
		OPN_RLS            *rlsp,
		OPO_BLOCKS	   blocks,
		bool		   selapplied,
		OPN_PERCENT        selectivity,
		OPV_VARS           *varp,
		bool               keyed_only);
/* #endif */

FUNC_EXTERN OPO_BLOCKS opn_readahead(
		OPS_SUBQUERY        *subquery,
		OPO_CO		    *cop);

/* # ifndef OPN_REFORMAT */
FUNC_EXTERN VOID opn_reformat(
		OPS_SUBQUERY       *subquery,
		OPO_TUPLES         tuples,
		OPS_WIDTH          width,
		OPO_BLOCKS         *diop,
		OPO_CPU            *cpup);
/* #endif */

/* # ifndef OPN_RMEMORY */
FUNC_EXTERN OPN_RLS * opn_rmemory(
		OPS_SUBQUERY       *subquery);
/* #endif */

/* # ifndef OPN_RO3 */
FUNC_EXTERN bool opn_ro3(
		OPS_SUBQUERY       *subquery,
		OPN_JTREE          *nodep);
/* #endif */

/* # ifndef OPN_RO4 */
FUNC_EXTERN OPV_IVARS opn_ro4(
		OPS_SUBQUERY       *subquery,
		OPN_JTREE          *nodep,
		OPV_BMVARS         *rmap);
/* #endif */
    
/* # ifndef OPN_SAVEST */
FUNC_EXTERN VOID opn_savest(
		OPS_SUBQUERY       *subquery,
		i4                leaves,
		OPN_SUBTREE        *subtp,
		OPN_EQS            *eqclp,
		OPN_RLS            *rlmp);
/* #endif */

/* # ifndef OPN_SM1 */
FUNC_EXTERN VOID opn_sm1(
		OPS_SUBQUERY       *subquery,
		OPE_BMEQCLS        *eqm,
		OPE_BMEQCLS        *eqh,
		OPB_BMBF           *bfm,
		OPV_BMVARS         *vmap);
/* #endif */

/* # ifndef OPN_SM2 */
FUNC_EXTERN VOID opb_gbfbm(
		OPS_SUBQUERY       *subquery,
		OPE_BMEQCLS        *avail,
		OPE_BMEQCLS        *lavail,
		OPE_BMEQCLS        *ravail,
		OPL_BMOJ	   *lojevalmap,
		OPL_BMOJ	   *rojevalmap,
		OPL_BMOJ	   *ojevalmap,
		OPE_IEQCLS         maxeqcls,
		OPV_VARS           *lvarp,
		OPV_VARS           *rvarp,
		OPB_BMBF           *jxbfm,
		OPE_BMEQCLS        *jeqh,
		OPE_BMEQCLS        *jxeqh,
		OPE_BMEQCLS        *leqr,
		OPE_BMEQCLS        *reqr,
		OPV_BMVARS	   *lvmap,
		OPV_BMVARS         *rvmap,
		OPV_BMVARS	   *bvmap,
		OPL_IOUTER	   ojid,
		OPL_BMOJ	   *ojinnermap,
		OPO_CO		   *cop);

FUNC_EXTERN VOID opn_sm2(
		OPS_SUBQUERY       *subquery,
		OPN_JTREE          *np,
		OPE_BMEQCLS        *jxeqh,
		OPB_BMBF           *jxbfm,
		OPE_BMEQCLS        *jeqh,
		OPE_BMEQCLS        *byeqcmap,
		OPE_BMEQCLS        *leqr,
		OPE_BMEQCLS        *reqr,
		OPE_BMEQCLS        *eqr,
		OPN_JEQCOUNT       *jxcnt,
		OPO_ISORT          *jordering,
		OPO_ISORT          *jxordering,
		OPO_ISORT          *lkeyjoin,
		OPO_ISORT          *rkeyjoin);
/* #endif */

/* # ifndef OPN_SMEMORY */
FUNC_EXTERN OPN_SUBTREE * opn_smemory(
		OPS_SUBQUERY       *subquery);
/* #endif */

/* # ifndef OPN_SRCHST */
FUNC_EXTERN OPN_SRESULT opn_srchst(
		OPS_SUBQUERY       *subquery,
		OPN_LEAVES	   nleaves,
		OPE_BMEQCLS        *eqr,
		OPV_BMVARS         *rmap,
		OPN_STLEAVES       rlasg,
		OPN_TDSC           sbstruct,
		OPN_SUBTREE        **subtp,
		OPN_EQS            **eqclp,
		OPN_RLS            **rlmp,
		bool		   *icheck);
/* #endif */

/* # ifndef OPN_TPBLK */
FUNC_EXTERN OPO_TUPLES opn_tpblk(
		OPG_CB		   *opg_cb,
		i4  	   pagesize,
		OPS_WIDTH          tuplewidth);
/* #endif */

FUNC_EXTERN VOID opn_jtalloc(
		OPS_SUBQUERY	   *subquery,
		OPN_JTREE	   **jtreep);

FUNC_EXTERN VOID opn_bestfrag(
		OPS_SUBQUERY       *subquery);

FUNC_EXTERN OPN_STATUS opn_newenum(
		OPS_SUBQUERY       *subquery,
		i4		   enumcode);

/* # ifndef OPN_URN */
FUNC_EXTERN OPH_DOMAIN opn_urn(
		OPO_TUPLES         rballs,
		OPH_DOMAIN	   ncells);
/* #endif */

FUNC_EXTERN VOID opt_fcodisplay(
		OPS_STATE	*global,
		OPS_SUBQUERY	*subquery,
		OPO_CO		*cop);

/* # ifndef OPN_TREE */
FUNC_EXTERN OPN_STATUS opn_tree(
		OPS_SUBQUERY       *subquery);
/* #endif */

/* # ifndef OPN_UKEY */
FUNC_EXTERN bool opn_ukey(
		OPS_SUBQUERY       *subquery,
		OPE_BMEQCLS        *oeqcmp,
		OPE_IEQCLS         jeqc,
		OPV_IVARS          keyedvar,
		bool               primkey);
/* #endif */

/* # ifndef OPN_PAGESIZE */
FUNC_EXTERN i4 opn_pagesize(
	OPG_CB	      *opg_cb,
        OPS_WIDTH     width);
/* #endif */

/* # ifndef OPN_MAXRECLEN */
FUNC_EXTERN i4 opn_maxreclen(
	OPG_CB		*opg_cb,
        i4		page_size);
/* #endif */

/* # ifndef OPO_COMBINE */
FUNC_EXTERN OPO_ISORT opo_combine(
		OPS_SUBQUERY       *subquery,
		OPO_ISORT          jordering,
		OPO_ISORT          outer,
		OPO_ISORT          inner,
		OPE_BMEQCLS        *eqcmap,
		OPO_ISORT	   *jorderp);

FUNC_EXTERN VOID opo_mexact(
		OPS_SUBQUERY       *subquery,
		OPO_ISORT          *ordering,
		OPO_ISORT          psjeqc,
		bool		   valid_sjeqc,
		OPO_ISORT          pordeqc,
		bool               valid_ordeqc,
		bool		   truncate);
/* #endif */

/* # ifndef OPO_FORDERING */
FUNC_EXTERN OPO_ISORT opo_fordering(
		OPS_SUBQUERY       *subquery,
		OPE_BMEQCLS        *eqcmap);
/* #endif */

/* # ifndef OPO_CORDERING */
FUNC_EXTERN OPO_ISORT opo_gnumber(
		OPS_SUBQUERY       *subquery);

FUNC_EXTERN VOID opo_iobase(
		OPS_SUBQUERY	*subquery);

FUNC_EXTERN OPO_ISORT opo_cordering(
		OPS_SUBQUERY       *subquery,
		OPO_EQLIST	   *keyp,
		bool		    copykey);

FUNC_EXTERN VOID opo_makey(
		OPS_SUBQUERY       *subquery,
		OPB_MBF            *mbfp,
		i4                count,
		OPO_EQLIST         **keylistp,
		bool		   enummemory,
		bool		   noconst);

FUNC_EXTERN VOID opo_pr(
		OPS_SUBQUERY       *subquery,
		OPO_CO             *cop);

FUNC_EXTERN VOID opo_orig(
		OPS_SUBQUERY       *subquery,
		OPO_CO             *cop);

FUNC_EXTERN VOID opo_rsort(
		OPS_SUBQUERY       *subquery,
		OPO_CO             *cop);

FUNC_EXTERN OPO_ISORT opo_kordering(
		OPS_SUBQUERY       *subquery,
		OPV_VARS           *varp,
		OPE_BMEQCLS        *eqcmap);
/* #endif */

/* # ifndef OPO_COPYCO */
FUNC_EXTERN DB_STATUS opo_copyco(
		OPS_SUBQUERY       *subquery,
		OPO_CO             **copp,
		bool               remove);
/* #endif */

FUNC_EXTERN VOID opo_copyfragco(
		OPS_SUBQUERY       *subquery,
		OPO_CO             **copp,
		bool		   top);

/* # ifndef OPO_MINCOCOST */
FUNC_EXTERN OPO_CO * opo_mincocost(
		OPS_SUBQUERY       *subquery,
		OPN_SUBTREE        *subtp,
		OPO_COST           *mcost);
/* #endif */

/* # ifndef OPS_RALTER */
FUNC_EXTERN DB_STATUS ops_alter(
		OPF_CB             *opf_cb);
	
FUNC_EXTERN DB_STATUS ops_exlock(
		OPF_CB             *opf_cb,
		SCF_SEMAPHORE      *semaphore);

FUNC_EXTERN DB_STATUS ops_unlock(
		OPF_CB             *opf_cb,
		SCF_SEMAPHORE      *semaphore);
/* #endif */

/* # ifndef OPS_BGN_SESSION */
FUNC_EXTERN DB_STATUS ops_bgn_session(
		OPF_CB             *opf_cb);
/* #endif */

/* # ifndef OPS_DEALLOCATE */
FUNC_EXTERN VOID ops_deallocate(
		OPS_STATE          *global,
		bool               report,
		bool               partial_dbp);
/* #endif */

/* # ifndef OPS_END_SESSION */
FUNC_EXTERN DB_STATUS ops_end_session(
		OPF_CB             *opf_cb);
/* #endif */

/* # ifndef OPS_GETCB */
FUNC_EXTERN OPS_CB * ops_getcb( );
/* #endif */

/* # ifndef OPS_INIT */
FUNC_EXTERN VOID ops_init(
		OPF_CB             *opf_cb,
		OPS_STATE          *global);

FUNC_EXTERN VOID ops_qinit(
		OPS_STATE          *global,
		PST_STATEMENT      *statementp);

FUNC_EXTERN DB_STATUS ops_gqtree(
		OPS_STATE          *global);
/* #endif */

/* # ifndef OPS_SEQUENCER */
FUNC_EXTERN DB_STATUS ops_sequencer(
		OPF_CB             *opf_cb);
FUNC_EXTERN VOID ops_decvar(
		OPS_STATE	    *global,
		PST_QNODE	    *constp);

FUNC_EXTERN OPV_IGVARS opa_cunion(
		OPS_STATE          *global,
		PST_QNODE          **rootpp,
		bool		   copytarget);

/* #endif */

/* # ifndef OPS_SHUTDOWN */
FUNC_EXTERN DB_STATUS ops_shutdown(
		OPF_CB             *opf_cb);
/* #endif */

/* # ifndef OPS_STARTUP */
FUNC_EXTERN DB_STATUS ops_startup(
		OPF_CB             *opf_cb);
/* #endif */

/* # ifndef OPT_TREETEXT */
FUNC_EXTERN VOID opt_treetext(
		OPS_SUBQUERY	   *subquery,
		OPO_CO             *cop);
/* #endif */

/* # ifndef OPT_COTREE */
FUNC_EXTERN VOID opt_cotree(
		OPO_CO             *cop);

FUNC_EXTERN VOID opt_cotree_without_stats( 
		OPS_STATE	    *global);

FUNC_EXTERN VOID opt_coprint(
		OPO_CO              *cop,
		PTR                 uld_control);

FUNC_EXTERN PTR opt_coleft(
		OPO_CO             *cop);

FUNC_EXTERN PTR opt_coright(
		OPO_CO             *cop);
    
FUNC_EXTERN i4 opt_scc(
		PTR		   global,
		i4                 length,
		char               *msg_buffer);

FUNC_EXTERN VOID opt_jall(
		OPS_SUBQUERY	    *subquery);

FUNC_EXTERN VOID opt_enum();

FUNC_EXTERN VOID opt_evp(
		OPN_EVAR	   *evp,
		i4		   vcount);

FUNC_EXTERN VOID opt_ehistmap(
		OPE_BMEQCLS        *eqh);

FUNC_EXTERN VOID opt_paname(
		OPS_SUBQUERY       *subquery,
		OPZ_IATTS          attno,
		OPT_NAME           *attrname);

FUNC_EXTERN bool opt_ptname(
		OPS_SUBQUERY	    *subquery,
		OPV_IVARS	    varno,
		OPT_NAME	    *namep,
		bool		    corelation);

FUNC_EXTERN i4  opt_noblanks(
		i4                strsize,
		char               *charp);

FUNC_EXTERN VOID opt_jtree();
 
FUNC_EXTERN VOID opt_jnode(
                OPN_JTREE          *nodep);
		 
FUNC_EXTERN VOID opt_rall(
		bool		   histdump);
 
FUNC_EXTERN VOID opt_rv(
                OPV_IVARS          rv,
		OPS_SUBQUERY       *subquery,
		bool		   histdump);
		  
FUNC_EXTERN VOID opt_agall();

FUNC_EXTERN VOID opt_ojstuff(
                OPS_SUBQUERY       *subquery);

FUNC_EXTERN VOID opt_pst_list(
                OPS_STATE          *global,
		PST_STATEMENT	   *stmtp);

FUNC_EXTERN VOID opt_pt_entry(
                OPS_STATE          *global,
                PST_QTREE          *header, 
                PST_QNODE          *qnode,
                char               *origin);
		 
FUNC_EXTERN VOID opt_qep_dump(
		OPS_STATE	   *global,
		OPS_SUBQUERY	   *subquery);

FUNC_EXTERN VOID opt_tbl(
		RDR_INFO           *rdrinfop);

FUNC_EXTERN VOID opt_prattr(
		RDR_INFO           *rdrinfop,
		i4                 dmfattr);

FUNC_EXTERN VOID opt_rattr(
		RDR_INFO           *rdrinfop);

FUNC_EXTERN VOID opt_index(
		RDR_INFO           *rdrinfop);

FUNC_EXTERN VOID opa_pconcurrent(
		OPS_SUBQUERY       *header);

FUNC_EXTERN VOID opt_grange(
		OPV_IGVARS         grv);

FUNC_EXTERN VOID opt_eclass(
		OPE_IEQCLS         eqcls);

FUNC_EXTERN VOID opt_eall();

FUNC_EXTERN VOID opt_printCostTree(
		OPO_CO		*cop,
		bool		withStats);
/* #endif */

/* # ifndef OPT_PNODE */
FUNC_EXTERN VOID opt_pnode(
		PST_QNODE          *qnode);
/* #endif */

/* # ifndef OPT_STRACE */
FUNC_EXTERN bool opt_strace(
		OPS_CB             *ops_cb,
		i4                flag);

FUNC_EXTERN bool opt_svtrace(
		OPS_CB		*ops_cb,
		i4             traceflag,
		i4		*firstvalue,
		i4         *secondvalue);

FUNC_EXTERN bool opt_gtrace(
		OPG_CB		   *opg_cb,
		i4                traceflag);
/* #endif */

/* # ifndef OPU_ALLOCATE */
FUNC_EXTERN PTR opu_allocate(
		OPS_STATE          *global);
/* #endif */

/* # ifndef OPU_COMPARE */
FUNC_EXTERN i4  opu_compare(
		OPS_STATE          *global,
		PTR                value1,
		PTR                value2,
		DB_DATA_VALUE      *datatype);
    
FUNC_EXTERN i4  opu_dcompare(
		OPS_STATE          *global,
		DB_DATA_VALUE      *value1,
		DB_DATA_VALUE      *value2);
/* #endif */

/* # ifndef OPU_DEALLOCATE */
FUNC_EXTERN VOID opu_deallocate(
		OPS_STATE          *global,
		PTR                *streamid);
/* #endif */

/* # ifndef OPU_MEMORY */
FUNC_EXTERN PTR opu_memory(
		OPS_STATE          *global,
		i4                size);

FUNC_EXTERN VOID opu_mark(
		OPS_STATE   *global,
		ULM_RCB     *ulmrcb,
		ULM_SMARK   *mark);

FUNC_EXTERN VOID opu_release(
		OPS_STATE   *global,
		ULM_RCB     *ulmrcb,
		ULM_SMARK   *mark);
/* #endif */

/* # ifndef OPU_SMEMORY */
FUNC_EXTERN VOID opu_Osmemory_open(
		OPS_STATE   *global);

FUNC_EXTERN VOID opu_Csmemory_close(
		OPS_STATE   *global);

FUNC_EXTERN PTR opu_Gsmemory_get(
		OPS_STATE   *global,
		i4	    size);

FUNC_EXTERN VOID opu_Msmemory_mark(
		OPS_STATE   *global,
		ULM_SMARK   *mark);

FUNC_EXTERN VOID opu_Rsmemory_reclaim(
		OPS_STATE   *global,
		ULM_SMARK   *mark);
/* #endif */

/* # ifndef OPU_QTPRINT */
FUNC_EXTERN VOID opu_qtprint(
		OPS_STATE          *global,
		PST_QNODE          *root,
		char               *origin);
/* #endif */
FUNC_EXTERN VOID opu_dtprint(
                OPS_STATE          *global,
		PST_QTREE          *header,
		PST_QNODE          *root, 
                char               *origin);

/* # ifndef OPU_TMEMORY */
FUNC_EXTERN PTR opu_tmemory(
		OPS_STATE          *global,
		i4                size);
/* #endif */

/* # ifndef OPV_CONSTNODE */
FUNC_EXTERN VOID opv_rqinsert(
		OPS_STATE          *global,	
		i4		   parmno,
		PST_QNODE          *qnode);

FUNC_EXTERN VOID opv_uparameter(
		OPS_STATE          *global,	
		PST_QNODE          *qnode);

FUNC_EXTERN PST_QNODE * opv_constnode(
		OPS_STATE          *global,
		DB_DATA_VALUE      *datatype,
		i4                parmno);

FUNC_EXTERN PST_QNODE * opv_intconst(
		OPS_STATE          *global,
		i4                intvalue);

FUNC_EXTERN PST_QNODE * opv_i1const(
		OPS_STATE          *global,
		i4                intvalue);
/* #endif */
    
/* # ifndef OPV_COPYTREE */
FUNC_EXTERN VOID opv_copynode(
		OPS_STATE	    *global, 
		PST_QNODE	    **nodepp);
FUNC_EXTERN VOID opv_copytree(
		OPS_STATE          *global,
		PST_QNODE          **nodepp);
/* #endif */

/* # ifndef OPV_CREATE */
FUNC_EXTERN VOID opv_submap(
		OPS_SUBQUERY       *subquery,
		OPV_GBMVARS        *globalmap);

FUNC_EXTERN VOID opv_create(
		OPS_SUBQUERY       *subquery);
/* #endif */

/* # ifndef OPV_CTREES */
FUNC_EXTERN bool opv_ctrees(
		OPS_STATE          *global,
		PST_QNODE          *node1p,
		PST_QNODE          *node2p);
/* #endif */

/* # ifndef OPV_DEASSIGN */
FUNC_EXTERN VOID opv_deassign(
		OPS_STATE          *global,
		OPV_IGVARS         varno);
/* #endif */

/* # ifndef OPV_GRTINIT */
FUNC_EXTERN VOID opv_grtinit(
		OPS_STATE          *global,
		bool		   rangetable);
/* #endif */

/* # ifndef OPV_GRTINIT */
FUNC_EXTERN OPV_IGVARS opv_agrv(
		OPS_STATE          *global,
		DB_TAB_NAME        *name,
		DB_OWN_NAME        *owner,
		DB_TAB_ID	   *tableid,
		OPS_SQTYPE         sqtype,
		bool               abort,
		OPV_GBMVARS        (*gbmap),
		OPV_IGVARS	   gvar);
/* #endif */

/* # ifndef OPV_IJNBL */
FUNC_EXTERN VOID opv_ijnbl(
		OPS_SUBQUERY       *subquery);
/* #endif */

/* # ifndef OPV_PCJNBL */
FUNC_EXTERN VOID opv_pcjnbl(
		OPS_SUBQUERY       *subquery);
/* #endif */

/* # ifndef OPV_INDEX */
FUNC_EXTERN VOID opv_index(
		OPS_SUBQUERY       *subquery);
/* #endif */

/* # ifndef OPV_MAPVAR */
FUNC_EXTERN VOID opv_mapvar(
		PST_QNODE          *nodep,
		OPV_GBMVARS        *map);
/* #endif */

/* # ifndef OPV_OPNODE */
FUNC_EXTERN PST_QNODE * opv_opnode(
		OPS_STATE          *global,
		i4                type,
		ADI_OP_ID          operator,
		OPL_IOUTER	   ojid);
/* #endif */

/* # ifndef OPV_PARSER */
FUNC_EXTERN bool opv_parser(
		OPS_STATE          *global,
		OPV_IGVARS	   gvar,
		OPS_SQTYPE         sqtype,
		bool               rdfinfo,
		bool               parser_table,
		bool               abort);
/* #endif */
    
/* # ifndef OPV_RELATTS */
FUNC_EXTERN VOID opv_tstats(
		OPS_SUBQUERY	*subquery);

FUNC_EXTERN OPV_IVARS opv_relatts(
		OPS_SUBQUERY       *subquery,
		OPV_IGVARS         grv,
		OPE_IEQCLS         eqcls,
		OPV_IVARS          primary);

FUNC_EXTERN VOID opv_virtual_table(
		OPS_SUBQUERY       *subquery,
		OPV_VARS           *lrvp,
		OPV_IGVARS         grv);
/* #endif */

/* # ifndef OPV_RESDOM */
FUNC_EXTERN PST_QNODE * opv_resdom(
		OPS_STATE          *global,
		PST_QNODE          *function,
		DB_DATA_VALUE      *datatype);
/* #endif */

/* # ifndef OPV_SMAP */
FUNC_EXTERN VOID opv_smap(
		OPS_SUBQUERY       *subquery);
/* #endif */

/* # ifndef OPV_TUPLESIZE */
FUNC_EXTERN i4 opv_tuplesize(
		OPS_STATE          *global,
		PST_QNODE          *root);
/* #endif */

/* # ifndef OPV_VARNODE */
FUNC_EXTERN PST_QNODE * opv_varnode(
		OPS_STATE          *global,
		DB_DATA_VALUE      *datatype,
		OPV_IGVARS         variable,
		DB_ATT_ID          *dmfattr);
    
FUNC_EXTERN PST_QNODE * opv_qlend(
		OPS_STATE	*global);
/* #endif */
    
/* # ifndef OPX_CINTER */
FUNC_EXTERN OPN_STATUS opx_cinter(
		OPS_SUBQUERY		*subquery);
/* #endif */

/* # ifndef OPX_PERROR */
FUNC_EXTERN VOID opx_error(
		OPX_ERROR          error);

FUNC_EXTERN STATUS opx_float(
		OPX_ERROR          error,
		bool		   hard);
    
FUNC_EXTERN VOID opx_vrecover(
		DB_STATUS          status,
		OPX_ERROR          error,
		OPX_FACILITY       facility);

FUNC_EXTERN VOID opx_rerror(
		OPF_CB             *opfcb,
		OPX_ERROR          status);

FUNC_EXTERN VOID opx_verror(
		DB_STATUS          status,
		OPX_ERROR          error,
		OPX_FACILITY       facility);

FUNC_EXTERN VOID opx_rverror(
		OPF_CB             *opfcb,
		DB_STATUS          status,
		OPX_ERROR          error,
		OPX_FACILITY       facility);

FUNC_EXTERN VOID opx_1perror(
		OPX_ERROR           error,
		PTR		    p1);

FUNC_EXTERN VOID opx_2perror(
		OPX_ERROR           error,
		PTR		    p1,
		PTR		    p2);

FUNC_EXTERN VOID opx_lerror(
# ifdef OPS_LINT
		OPX_ERROR	    error,
		i4		    num_parms,
		...
#endif
);

#ifdef EX_JNP_LJMP
FUNC_EXTERN STATUS opx_exception(
		EX_ARGS            *args);
#endif

/* #endif */

/* # ifndef OPX_STATUS */
FUNC_EXTERN DB_STATUS opx_status(
		OPF_CB             *opf_cb,
		OPS_CB             *ops_cb);
/* #endif */

/* # ifndef OPT_CALL */
FUNC_EXTERN DB_STATUS opt_call(
		DB_DEBUG_CB        *debug_cb);
/* #endif */

/* # ifndef OPX_CALL */
FUNC_EXTERN DB_STATUS opx_init(
		OPF_CB	    *opf_cb);
/* #endif */

/* # ifndef OPZ_ADDATTS */
FUNC_EXTERN OPZ_IATTS opz_addatts(
		OPS_SUBQUERY       *subquery,
		OPV_IVARS          varno,
		OPZ_DMFATTR        dmfattr,
		DB_DATA_VALUE      *datatype);
/* #endif */

/* # ifndef OPZ_CREATE */
FUNC_EXTERN VOID opz_create(
		OPS_SUBQUERY       *subquery);
/* #endif */


/* # ifndef OPZFATTR */
FUNC_EXTERN VOID opz_fattr(
		OPS_SUBQUERY       *subquery);

FUNC_EXTERN OPZ_IATTS opz_ftoa(
		OPS_SUBQUERY       *subquery,
		OPZ_FATTS          *fatt,
		OPV_IVARS	   fvarno,
		PST_QNODE          *function,
		DB_DATA_VALUE      *datatype,
		OPZ_FACLASS	   class);

FUNC_EXTERN VOID opz_faeqcmap(
		OPS_SUBQUERY	*subquery,
		OPE_BMEQCLS	*targetp,
		OPV_BMVARS	*varmap);

/* #endif */

/* # ifndef OPZ_FINDATT */
FUNC_EXTERN OPZ_IATTS opz_findatt(
		OPS_SUBQUERY       *subquery,
		OPV_IVARS          joinopvar,
		OPZ_DMFATTR	   dmfattr);
/* #endif */

/* # ifndef OPZ_FMAKE */
FUNC_EXTERN OPZ_FATTS * opz_fmake(
		OPS_SUBQUERY       *subquery,
		PST_QNODE          *qnode,
		OPZ_FACLASS        class);
/* #endif */

/* # ifndef OPZ_SAVEQUAL */
FUNC_EXTERN VOID opz_savequal(
		OPS_SUBQUERY       *subquery,
		PST_QNODE          *andnode,
		OPV_GBMVARS        *lvarmap,
		OPV_GBMVARS        *rvarmap,
		PST_QNODE	   *oldnodep,
		PST_QNODE	   *inodep,
		bool		   left_iftrue,
		bool		   right_iftrue);
/* #endif */

/* # ifndef OPLINIT */
FUNC_EXTERN VOID opl_ojinit(
		OPS_SUBQUERY       *subquery);

FUNC_EXTERN OPL_IOUTER opl_ioutjoin(
		OPS_SUBQUERY       *subquery,
		PST_J_ID	   joinid);

/* #endif */

/* # ifndef OPLOJOIN */
FUNC_EXTERN VOID opl_seqc(
		OPS_SUBQUERY       *subquery,
		OPV_IVARS          varno,
		OPL_IOUTER         ojid);

FUNC_EXTERN VOID opl_ojoin(
		OPS_SUBQUERY       *subquery);

FUNC_EXTERN VOID opl_iojmap(
		OPS_SUBQUERY       *subquery,
		OPV_BMVARS	   *validmap);

FUNC_EXTERN VOID opl_index(
		OPS_SUBQUERY       *subquery,
		OPV_IVARS          basevar,
		OPV_IVARS          indexvar);

FUNC_EXTERN VOID opl_ojmaps(
		OPS_SUBQUERY       *subquery);

FUNC_EXTERN VOID opl_iftrue(
		OPS_SUBQUERY       *subquery,
		PST_QNODE          **rootpp,
		OPL_IOUTER	   ojid);

FUNC_EXTERN bool opl_fojeqc(
		OPS_SUBQUERY       *subquery,
		OPV_GBMVARS        *lvarmap,
		OPV_GBMVARS        *rvarmap,
		OPL_IOUTER	   ojid);

FUNC_EXTERN VOID opl_nulltype(
		PST_QNODE	   *qnodep);

FUNC_EXTERN PST_QNODE * opl_findresdom(
		OPS_SUBQUERY       *subquery,
		OPZ_IATTS	   rsno);

FUNC_EXTERN VOID opl_remove(
		OPS_SUBQUERY       *subquery,
		OPO_CO             *cop,
		OPE_BMEQCLS        *special);

FUNC_EXTERN OPL_OJTYPE opl_ojtype(
		OPS_SUBQUERY       *subquery,
		OPL_IOUTER         ojid,
		OPV_BMVARS         *lvarmap,
		OPV_BMVARS	   *rvarmap);

FUNC_EXTERN bool opl_histogram(
		OPS_SUBQUERY        *subquery,
		OPN_JTREE	    *np,
		OPN_SUBTREE	    *lsubtp,
		OPN_EQS		    *leqclp,
		OPN_RLS		    *lrlmp,
		OPN_SUBTREE	    *rsubtp,
		OPN_EQS		    *reqclp,
		OPN_RLS		    *rrlmp,
		OPO_ISORT	    jordering,
		OPO_ISORT	    jxordering,
		OPE_BMEQCLS	    *jeqh,
		OPE_BMEQCLS	    *byeqcmap,
		OPB_BMBF	    *bfm,
		OPE_BMEQCLS	    *eqh,
		OPN_RLS		    **rlmp_resultpp,
		OPN_JEQCOUNT        jxcnt);
    
FUNC_EXTERN VOID opl_relabel(
		OPS_SUBQUERY       *subquery,
		PST_QNODE          *qnode,
		OPV_IVARS	   primary,
		OPE_IEQCLS         primaryeqc,
		OPV_IVARS	   indexvno);

FUNC_EXTERN VOID opl_resolve(
		OPS_SUBQUERY       *subquery,
		PST_QNODE	   *qnodep);

FUNC_EXTERN VOID opl_subselect(
		OPS_SUBQUERY	    *subquery,
		OPV_IVARS	    varno,
		OPL_IOUTER	    ojid);

FUNC_EXTERN VOID opl_ijcheck(
		OPS_SUBQUERY	    *subquery,
		OPL_BMOJ            *linnermap,
		OPL_BMOJ            *rinnermap,
		OPL_BMOJ            *ojinnermap,
		OPL_BMOJ            *oevalmap,
		OPL_BMOJ            *ievalmap,
		OPV_BMVARS          *varmap);

FUNC_EXTERN VOID opl_sjij(
		OPS_SUBQUERY	     *subquery,
		OPV_IVARS	     varno,
		OPL_BMOJ	     *innermap,
		OPL_BMOJ	     *ojmap);

/* #endif */


# ifndef ule_format
FUNC_EXTERN DB_STATUS ule_format(
# ifdef OPS_LINT
		i4             error_code,
		CL_ERR_DESC         *clerror,
		i4             flag,
		i4             *generic_error,
		char                *msg_buffer,
		i4             msg_buf_length,
		i4             *msg_length,
		i4             *err_code,
		i4                 num_parms,
		...
#endif
);
#endif

# ifndef ulm_closestream
FUNC_EXTERN DB_STATUS ulm_closestream(
# ifdef OPS_LINT
		ULM_RCB            *ulmrcb
#endif
);
#endif

# ifndef ulm_startup
FUNC_EXTERN DB_STATUS ulm_startup(
# ifdef OPS_LINT
		ULM_RCB            *ulmrcb
#endif
);
#endif

# ifndef ulm_mark
FUNC_EXTERN DB_STATUS ulm_mark(
# ifdef OPS_LINT
		ULM_RCB            *ulmrcb
#endif
);
#endif

# ifndef ulm_reclaim
FUNC_EXTERN DB_STATUS ulm_reclaim(
# ifdef OPS_LINT
		ULM_RCB            *ulmrcb
#endif
);
#endif

# ifndef ulm_openstream
FUNC_EXTERN DB_STATUS ulm_openstream(
# ifdef OPS_LINT
		ULM_RCB            *ulmrcb
#endif
);
#endif
    
# ifndef ulm_palloc
FUNC_EXTERN DB_STATUS ulm_palloc(
# ifdef OPS_LINT
		ULM_RCB            *ulm_rcb
#endif
);
#endif
    
# ifndef ulm_shutdown
FUNC_EXTERN DB_STATUS ulm_shutdown(
# ifdef OPS_LINT
		ULM_RCB            *ulmrcb
#endif
);
#endif

# ifndef qsf_call
FUNC_EXTERN DB_STATUS qsf_call(
# ifdef OPS_LINT
		i4                code,
		QSF_RCB            *qsf_rcb
#endif
);
#endif

# ifndef uld_prnode
FUNC_EXTERN VOID uld_prnode(
# ifdef OPS_LINT
		PTR                control,
		char               *format
#endif
);
#endif

# ifndef dmf_call
FUNC_EXTERN DB_STATUS dmf_call(
# ifdef OPS_LINT
		i4		op_code,
		DMT_CB		*dmt_cb
#endif
);
#endif

# ifndef scf_call
FUNC_EXTERN DB_STATUS scf_call(
# ifdef OPS_LINT
		i4		op_code,
		SCF_CB		*scf_cb
#endif
);
#endif

# ifndef ult_clrval
FUNC_EXTERN DB_STATUS ult_clrval(
# ifdef OPS_LINT
		ULT_TVECT	*vector,
		i4		flag
#endif
);
#endif

# ifndef ult_putval
FUNC_EXTERN DB_STATUS ult_putval(
# ifdef OPS_LINT
		ULT_TVECT          *vector,
		i4		    flag,
		i4		    v1, 
		i4		    v2
#endif
);
#endif

# ifndef ult_getval
FUNC_EXTERN bool ult_getval(
# ifdef OPS_LINT
		ULT_TVECT	   *vector,
		i4                bit,
		i4		   *v1,
		i4		   *v2
#endif
);
#endif

# ifndef ulc_bld_descriptor
FUNC_EXTERN DB_STATUS ulc_bld_descriptor(
# ifdef OPS_LINT
		PST_QNODE          *target_list,
		i4                names,
		QSF_RCB            *qsf_rb,
		GCA_TD_DATA        **descrip
#endif
);
#endif
