/*
**Copyright (c) 2004, 2010 Ingres Corporation
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
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* opaaggr.c */
FUNC_EXTERN void opa_aggregate(
	OPS_STATE *global);
/* opabyprj.c */
FUNC_EXTERN void opa_byproject(
	OPS_STATE *global);
/* opacompat.c */
FUNC_EXTERN void opa_agbylist(
	OPS_SUBQUERY *subquery,
	bool agview,
	bool project);
FUNC_EXTERN void opa_gsub(
	OPS_SUBQUERY *subquery);
FUNC_EXTERN void opa_compat(
	OPS_STATE *global);
/* opafinal.c */
FUNC_EXTERN void opa_tnode(
	OPS_STATE *global,
	OPZ_IATTS rsno,
	OPV_IGVARS varno,
	PST_QNODE *root);
FUNC_EXTERN void opa_stid(
	OPS_SUBQUERY *subquery);
FUNC_EXTERN void opa_final(
	OPS_STATE *global);
/* opagen.c */
FUNC_EXTERN bool opa_rsmap(
	OPS_STATE *global,
	PST_QNODE *root,
	OPZ_BMATTS *attrmap);
FUNC_EXTERN OPV_IGVARS opa_cunion(
	OPS_STATE *global,
	PST_QNODE **rootpp,
	bool copytarget);
FUNC_EXTERN void opa_resolve(
	OPS_SUBQUERY *subquery,
	PST_QNODE *nodep);
FUNC_EXTERN void opa_not(
	OPS_STATE *global,
	PST_QNODE *tree);
FUNC_EXTERN void opa_covar(
	OPS_SUBQUERY *father,
	PST_QNODE *varnodep,
	OPV_IGVARS varno);
FUNC_EXTERN void opa_1pass(
	OPS_STATE *global);
/* opainitsq.c */
FUNC_EXTERN void opa_ifagg(
	OPS_SUBQUERY *new_subquery,
	PST_QNODE *byhead);
FUNC_EXTERN OPS_SUBQUERY *opa_initsq(
	OPS_STATE *global,
	PST_QNODE **agg_qnode,
	OPS_SUBQUERY *father);
/* opalink.c */
FUNC_EXTERN void opa_link(
	OPS_STATE *global);
/* opaoptim.c */
FUNC_EXTERN void opa_optimize(
	OPS_STATE *global);
/* opaprime.c */
FUNC_EXTERN bool opa_prime(
	OPS_SUBQUERY *subquery);
/* opasuboj.c */
FUNC_EXTERN void opa_subsel_to_oj(
	OPS_STATE *global,
	bool *gotone);
/* opbbfkget.c */
FUNC_EXTERN OPB_BFKEYINFO *opb_bfkget(
	OPS_SUBQUERY *subquery,
	OPB_BOOLFACT *bp,
	OPE_IEQCLS eqcls,
	DB_DATA_VALUE *datatype,
	DB_DATA_VALUE *histdt,
	bool mustfind);
/* opbcreate.c */
FUNC_EXTERN OPB_BOOLFACT *opb_bfget(
	OPS_SUBQUERY *subquery,
	OPB_BOOLFACT *bfp);
FUNC_EXTERN bool opb_bfinit(
	OPS_SUBQUERY *subquery,
	OPB_BOOLFACT *bfp,
	PST_QNODE *root,
	bool *complex);
FUNC_EXTERN void opb_create(
	OPS_SUBQUERY *subquery);
FUNC_EXTERN void opb_mboolfact(
	OPS_SUBQUERY *subquery,
	PST_QNODE *expr1,
	PST_QNODE *expr2,
	PST_J_ID joinid);
/* opbjkeyl.c */
FUNC_EXTERN void opb_jkeylist(
	OPS_SUBQUERY *subquery,
	OPV_IVARS varno,
	OPO_STORAGE storage,
	RDD_KEY_ARRAY *dmfkeylist,
	OPB_MBF *mbfkl);
/* opbmbf.c */
FUNC_EXTERN bool opb_mbf(
	OPS_SUBQUERY *subquery,
	OPO_STORAGE storage,
	OPB_MBF *mbf);
/* opbpmbf.c */
FUNC_EXTERN void opb_pmbf(
	OPS_SUBQUERY *subquery);
/* opbpqbf.c */
FUNC_EXTERN void opb_pqbf(
	OPS_SUBQUERY *subquery);
FUNC_EXTERN void opb_pqbf_findeqc(
	DB_PART_DEF *partp,
	OPB_PQBF *pqbfp,
	OPE_IEQCLS eqc,
	i4 *dim,
	i4 *col_index);
/* opbsfind.c */
FUNC_EXTERN bool opb_sfind(
	OPB_BOOLFACT *bp,
	OPE_IEQCLS eqcls);
/* opdcost.c */
FUNC_EXTERN OPO_COST opd_msite(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop,
	OPD_ISITE target_site,
	bool root_flag,
	OPO_CO *marker,
	bool remove);
FUNC_EXTERN bool opd_jcost(
	OPS_SUBQUERY *subquery,
	OPN_RLS *main_rlsp,
	OPN_SUBTREE *main_subtp,
	OPO_CO *newcop,
	OPS_WIDTH width,
	OPO_CPU sort_cpu,
	OPO_BLOCKS sort_dio);
FUNC_EXTERN void opd_idistributed(
	OPS_STATE *global);
FUNC_EXTERN void opd_isub_dist(
	OPS_SUBQUERY *subquery);
FUNC_EXTERN bool opd_prleaf(
	OPS_SUBQUERY *subquery,
	OPN_RLS *rlsp,
	OPO_CO *cop,
	OPS_WIDTH tuple_width,
	OPO_CPU sort_cpu,
	OPO_BLOCKS sort_dio);
FUNC_EXTERN void opd_orig(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop);
FUNC_EXTERN bool opd_bflag(
	OPS_STATE *global);
FUNC_EXTERN void opd_addsite(
	OPS_STATE *global,
	OPV_GRV *gvarp);
FUNC_EXTERN bool opd_bestplan(
	OPS_SUBQUERY *subquery,
	OPO_CO *newcop,
	bool sortrequired,
	OPO_CPU sort_cpu,
	OPO_BLOCKS sort_dio,
	OPO_CO **sortcopp);
FUNC_EXTERN void opd_recover(
	OPS_STATE *global);
/* opeaddeq.c */
FUNC_EXTERN bool ope_addeqcls(
	OPS_SUBQUERY *subquery,
	OPE_IEQCLS eqcnum,
	OPZ_IATTS attnum,
	bool nulljoin);
/* opeaseqcls.c */
FUNC_EXTERN void ope_aseqcls(
	OPS_SUBQUERY *subquery,
	OPE_BMEQCLS *eqcmap,
	PST_QNODE *root);
FUNC_EXTERN void ope_qual(
	OPS_SUBQUERY *subquery);
/* opecreate.c */
FUNC_EXTERN void ope_create(
	OPS_SUBQUERY *subquery);
/* opefindj.c */
FUNC_EXTERN void ope_findjoins(
	OPS_SUBQUERY *subquery);
/* opemerge.c */
FUNC_EXTERN bool ope_mergeatts(
	OPS_SUBQUERY *subquery,
	OPZ_IATTS firstattr,
	OPZ_IATTS secondattr,
	bool nulljoin,
	OPL_IOUTER ojid);
/* openeweq.c */
FUNC_EXTERN OPE_IEQCLS ope_neweqcls(
	OPS_SUBQUERY *subquery,
	OPZ_IATTS attr);
/* opetwidth.c */
FUNC_EXTERN OPS_WIDTH ope_twidth(
	OPS_SUBQUERY *subquery,
	OPE_BMEQCLS *emap);
/* opetype.c */
FUNC_EXTERN DB_DATA_VALUE *ope_type(
	OPS_SUBQUERY *subquery,
	OPE_IEQCLS eqcls);
/* ophcbmem.c */
FUNC_EXTERN OPH_CELLS oph_cbmemory(
	OPS_SUBQUERY *subquery,
	i4 arraysize);
/* ophccmem.c */
FUNC_EXTERN OPH_COUNTS oph_ccmemory(
	OPS_SUBQUERY *subquery,
	OPH_INTERVAL *intervalp);
/* ophcreate.c */
FUNC_EXTERN OPH_HISTOGRAM *oph_create(
	OPS_SUBQUERY *subquery,
	OPH_HISTOGRAM *hp,
	OPN_PERCENT initfact,
	OPE_IEQCLS eqcls);
/* ophdcbme.c */
FUNC_EXTERN void oph_dcbmemory(
	OPS_SUBQUERY *subquery,
	OPH_CELLS cellptr);
/* ophdccme.c */
FUNC_EXTERN void oph_dccmemory(
	OPS_SUBQUERY *subquery,
	OPH_INTERVAL *intervalp,
	OPH_COUNTS counts);
/* ophdmemory.c */
FUNC_EXTERN void oph_dmemory(
	OPS_SUBQUERY *subquery,
	OPH_HISTOGRAM *histp);
/* ophhistos.c */
FUNC_EXTERN PTR oph_hvalue(
	OPS_SUBQUERY *subquery,
	DB_DATA_VALUE *histdt,
	DB_DATA_VALUE *valuedt,
	PTR key,
	ADI_OP_ID opid);
FUNC_EXTERN void oph_histos(
	OPS_SUBQUERY *subquery);
/* ophinter.c */
FUNC_EXTERN OPN_PERCENT oph_interpolate(
	OPS_SUBQUERY *subquery,
	PTR minv,
	PTR maxv,
	PTR constant,
	OPB_SARG operator,
	DB_DATA_VALUE *datatype,
	OPO_TUPLES cellcount,
	ADI_OP_ID opno,
	OPH_HISTOGRAM *hp);
/* ophjselect.c */
FUNC_EXTERN void oph_jselect(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *np,
	OPN_RLS **trlpp,
	OPN_SUBTREE *lsubtp,
	OPN_EQS *leqclp,
	OPN_RLS *lrlmp,
	OPN_SUBTREE *rsubtp,
	OPN_EQS *reqclp,
	OPN_RLS *rrlmp,
	OPO_ISORT jordering,
	OPO_ISORT jxordering,
	OPE_BMEQCLS *jeqh,
	OPE_BMEQCLS *byeqcmap,
	OPL_OJHISTOGRAM *ojhistp);
/* ophmaxptr.c */
FUNC_EXTERN PTR oph_maxptr(
	OPS_STATE *global,
	PTR hist1ptr,
	PTR hist2ptr,
	DB_DATA_VALUE *datatype);
/* ophmemory.c */
FUNC_EXTERN OPH_HISTOGRAM *oph_memory(
	OPS_SUBQUERY *subquery);
/* ophminptr.c */
FUNC_EXTERN PTR oph_minptr(
	OPS_STATE *global,
	PTR hist1ptr,
	PTR hist2ptr,
	DB_DATA_VALUE *datatype);
/* ophpdf.c */
FUNC_EXTERN void oph_pdf(
	OPS_SUBQUERY *subquery,
	OPH_HISTOGRAM *histp);
/* ophrelse.c */
FUNC_EXTERN void oph_relselect(
	OPS_SUBQUERY *subquery,
	OPN_RLS **trlpp,
	OPN_RLS *trl,
	OPB_BMBF *bfm,
	OPE_BMEQCLS *eqh,
	bool *selapplied,
	OPN_PERCENT *selectivity,
	OPE_IEQCLS jeqc,
	OPN_LEAVES nleaves,
	OPN_JEQCOUNT jxcnt,
	OPV_IVARS varno);
/* ophtcomp.c */
FUNC_EXTERN i4 oph_tcompare(
	char *hist1ptr,
	char *hist2ptr,
	DB_DATA_VALUE *datatype);
/* ophupdate.c */
FUNC_EXTERN void oph_normalize(
	OPS_SUBQUERY *subquery,
	OPH_HISTOGRAM *histp,
	OPN_PERCENT newarea);
/* opjjoinop.c */
FUNC_EXTERN void opj_joinop(
	OPS_STATE *global);
/* opjnorm.c */
FUNC_EXTERN void opj_adjust(
	OPS_SUBQUERY *subquery,
	PST_QNODE **qual);
FUNC_EXTERN void opj_mvands(
	OPS_SUBQUERY *subquery,
	PST_QNODE *qual);
FUNC_EXTERN void opj_normalize(
	OPS_SUBQUERY *subquery,
	PST_QNODE **qual);
FUNC_EXTERN void opj_mvors(
	PST_QNODE *qual);
/* opjunion.c */
FUNC_EXTERN void opj_union(
	OPS_SUBQUERY *subquery);
FUNC_EXTERN void opj_uboolfact(
	OPS_SUBQUERY *subquery);
/* oplinit.c */
FUNC_EXTERN OPL_IOUTER opl_ioutjoin(
	OPS_SUBQUERY *subquery,
	PST_J_ID joinid);
FUNC_EXTERN void opl_ojinit(
	OPS_SUBQUERY *subquery);
/* oplojoin.c */
FUNC_EXTERN void opl_resolve(
	OPS_SUBQUERY *subquery,
	PST_QNODE *qnode);
FUNC_EXTERN void opl_iftrue(
	OPS_SUBQUERY *subquery,
	PST_QNODE **rootpp,
	OPL_IOUTER ojid);
FUNC_EXTERN PST_QNODE *opl_findresdom(
	OPS_SUBQUERY *subquery,
	OPZ_IATTS rsno);
FUNC_EXTERN void opl_nulltype(
	PST_QNODE *qnodep);
FUNC_EXTERN void opl_ojoin(
	OPS_SUBQUERY *subquery);
FUNC_EXTERN void opl_index(
	OPS_SUBQUERY *subquery,
	OPV_IVARS basevar,
	OPV_IVARS indexvar);
FUNC_EXTERN void opl_ojmaps(
	OPS_SUBQUERY *subquery);
FUNC_EXTERN bool opl_fojeqc(
	OPS_SUBQUERY *subquery,
	OPV_GBMVARS *lvarmap,
	OPV_GBMVARS *rvarmap,
	OPL_IOUTER ojid);
FUNC_EXTERN OPL_OJTYPE opl_ojtype(
	OPS_SUBQUERY *subquery,
	OPL_IOUTER ojid,
	OPV_BMVARS *lvarmap,
	OPV_BMVARS *rvarmap);
FUNC_EXTERN bool opl_histogram(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *np,
	OPN_SUBTREE *lsubtp,
	OPN_EQS *leqclp,
	OPN_RLS *lrlmp,
	OPN_SUBTREE *rsubtp,
	OPN_EQS *reqclp,
	OPN_RLS *rrlmp,
	OPO_ISORT jordering,
	OPO_ISORT jxordering,
	OPE_BMEQCLS *jeqh,
	OPE_BMEQCLS *byeqcmap,
	OPB_BMBF *bfm,
	OPE_BMEQCLS *eqh,
	OPN_RLS **rlmp_resultpp,
	OPN_JEQCOUNT jxcnt);
FUNC_EXTERN void opl_relabel(
	OPS_SUBQUERY *subquery,
	PST_QNODE *qnode,
	OPV_IVARS primary,
	OPE_IEQCLS primaryeqc,
	OPV_IVARS indexvno);
FUNC_EXTERN void opl_subselect(
	OPS_SUBQUERY *subquery,
	OPV_IVARS varno,
	OPL_IOUTER ojid);
FUNC_EXTERN void opl_ijcheck(
	OPS_SUBQUERY *subquery,
	OPL_BMOJ *linnermap,
	OPL_BMOJ *rinnermap,
	OPL_BMOJ *ojinnermap,
	OPL_BMOJ *oevalmap,
	OPL_BMOJ *ievalmap,
	OPV_BMVARS *varmap);
FUNC_EXTERN void opl_sjij(
	OPS_SUBQUERY *subquery,
	OPV_IVARS varno,
	OPL_BMOJ *innermap,
	OPL_BMOJ *ojmap);
/* opnarl.c */
FUNC_EXTERN bool opn_cartprod(
	OPS_SUBQUERY *subquery,
	OPV_IVARS *partition,
	OPV_IVARS maxvar,
	OPV_IVARS keyedvar,
	bool *notjoinable);
FUNC_EXTERN bool opn_tjconnected(
	OPS_SUBQUERY *subquery,
	OPV_IVARS *partition,
	OPV_IVARS maxvar,
	OPV_BMVARS *tidmap,
	OPN_JTREE *childp);
FUNC_EXTERN bool opl_ojverify(
	OPS_SUBQUERY *subquery,
	OPZ_BMATTS *attrmap,
	OPV_BMVARS *vmap,
	OPL_OUTER *outerp);
FUNC_EXTERN bool opn_jintersect(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *nodep,
	OPN_STATUS *sigstat);
FUNC_EXTERN bool opn_arl(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *nodep,
	OPN_STLEAVES varset,
	OPN_STATUS *sigstat);
/* opnbfkhget.c */
FUNC_EXTERN OPB_BFKEYINFO *opn_bfkhget(
	OPS_SUBQUERY *subquery,
	OPB_BOOLFACT *bp,
	OPE_IEQCLS eqc,
	OPH_HISTOGRAM *hp,
	bool mustfind,
	OPH_HISTOGRAM **hpp);
/* opncalcost.c */
FUNC_EXTERN void opn_modcost(
	OPS_SUBQUERY *subquery);
FUNC_EXTERN bool opn_goodco(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop);
FUNC_EXTERN OPN_STATUS opn_calcost(
	OPS_SUBQUERY *subquery,
	OPO_ISORT jordering,
	OPO_ISORT keyjoin,
	OPN_SUBTREE *osbtp,
	OPN_SUBTREE *isbtp,
	OPN_SUBTREE *jsbtp,
	OPN_RLS *orlp,
	OPN_RLS *irlp,
	OPN_RLS *rlp,
	OPO_TUPLES tuples,
	OPN_EQS *eqsp,
	OPO_BLOCKS blocks,
	OPN_EQS *oeqsp,
	OPN_EQS *ieqsp,
	OPN_JTREE *np,
	OPL_IOUTER ojid,
	bool kjonly);
/* opnceval.c */
FUNC_EXTERN void opn_recover(
	OPS_SUBQUERY *subquery);
FUNC_EXTERN bool opn_timeout(
	OPS_SUBQUERY *subquery);
FUNC_EXTERN OPN_STATUS opn_ceval(
	OPS_SUBQUERY *subquery);
/* opncksrt.c */
FUNC_EXTERN void opn_createsort(
	OPS_SUBQUERY *subquery,
	OPO_CO *sortcop,
	OPO_CO *outercop,
	OPN_EQS *eqsp);
FUNC_EXTERN bool opn_checksort(
	OPS_SUBQUERY *subquery,
	OPO_CO **oldcopp,
	OPO_COST oldcost,
	OPO_BLOCKS newdio,
	OPO_CPU newcpu,
	OPN_EQS *eqsp,
	OPO_COST *newcostp,
	OPO_ISORT available_ordering,
	bool *sortrequired,
	OPO_CO *newcop);
FUNC_EXTERN OPO_CO *opn_oldplan(
	OPS_SUBQUERY *subquery,
	OPO_CO **oldcopp);
FUNC_EXTERN void opn_cleanup(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop,
	OPO_BLOCKS *dio,
	OPO_CPU *cpu);
/* opncmemory.c */
FUNC_EXTERN void opo_initco(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop);
FUNC_EXTERN OPO_PERM *opn_cpmemory(
	OPS_SUBQUERY *subquery,
	i4 cocount);
FUNC_EXTERN OPO_CO *opn_cmemory(
	OPS_SUBQUERY *subquery);
/* opncoins.c */
FUNC_EXTERN void opn_coinsert(
	OPN_SUBTREE *sbtp,
	OPO_CO *cop);
/* opncost.c */
FUNC_EXTERN OPO_COST opn_cost(
	OPS_SUBQUERY *subquery,
	OPO_BLOCKS dio,
	OPO_CPU cpu);
/* opndcmem.c */
FUNC_EXTERN void opn_dcmemory(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop);
/* opndeltrl.c */
FUNC_EXTERN void opn_deltrl(
	OPS_SUBQUERY *subquery,
	OPN_RLS *trl,
	OPE_IEQCLS jeqc,
	OPN_LEAVES nleaves);
/* opndemem.c */
FUNC_EXTERN void opn_dememory(
	OPS_SUBQUERY *subquery,
	OPN_EQS *eqsp);
/* opndiff.c */
FUNC_EXTERN void opn_initcollate(
	OPS_STATE *global);
FUNC_EXTERN OPN_DIFF opn_diff(
	PTR high,
	PTR low,
	i4 *charlength,
	OPO_TUPLES celltups,
	OPZ_ATTS *attrp,
	DB_DATA_VALUE *datatype,
	PTR tbl);
/* opndrmem.c */
FUNC_EXTERN void opn_drmemory(
	OPS_SUBQUERY *subquery,
	OPN_RLS *rlsp);
/* opndsmem.c */
FUNC_EXTERN void opn_dsmemory(
	OPS_SUBQUERY *subquery,
	OPN_SUBTREE *stp);
/* opnememory.c */
FUNC_EXTERN OPN_EQS *opn_ememory(
	OPS_SUBQUERY *subquery);
/* opnenum.c */
FUNC_EXTERN bool opn_uniquecheck(
	OPS_SUBQUERY *subquery,
	OPV_BMVARS *vmap,
	OPE_BMEQCLS *jeqcmap,
	OPE_IEQCLS eqcls);
FUNC_EXTERN void opn_enum(
	OPS_SUBQUERY *subquery);
FUNC_EXTERN bool opn_nonsystem_rule(
	OPS_SUBQUERY *subquery);
/* opneprime.c */
FUNC_EXTERN OPH_DOMAIN opn_eprime(
	OPO_TUPLES rballs,
	OPO_TUPLES mballspercell,
	OPH_DOMAIN ncells,
	f8 underflow);
/* opneqtamp.c */
FUNC_EXTERN OPH_HISTOGRAM **opn_eqtamp(
	OPS_SUBQUERY *subquery,
	OPH_HISTOGRAM **hp,
	OPE_IEQCLS eqcls,
	bool mustfind);
FUNC_EXTERN OPH_HISTOGRAM **opn_eqtamp_multi(
	OPS_SUBQUERY *subquery,
	OPH_HISTOGRAM **hp,
	OPE_IEQCLS eqcls,
	bool mustfind,
	bool *multiple_eqcls);
/* opnexit.c */
FUNC_EXTERN void opn_exit(
	OPS_SUBQUERY *subquery,
	bool longjump);
/* opngnperm.c */
FUNC_EXTERN bool opn_gnperm(
	OPS_SUBQUERY *subquery,
	OPN_STLEAVES permutation,
	OPN_LEAVES numleaves,
	OPN_PARTSZ partsz,
	OPV_BMVARS *pr_n_included,
	bool firstcomb);
/* opnhint.c */
FUNC_EXTERN bool opn_index_hint(
	OPS_SUBQUERY *subquery,
	OPN_STLEAVES combination,
	OPN_LEAVES numleaves);
FUNC_EXTERN bool opn_nonkj_hint(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *nodep);
FUNC_EXTERN bool opn_fsmCO_hint(
	OPS_SUBQUERY *subquery,
	OPO_CO *nodep);
FUNC_EXTERN bool opn_kj_hint(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *nodep,
	bool *lkjonly,
	bool *rkjonly);
FUNC_EXTERN bool opn_order_hint(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *nodep);
/* opnimtid.c */
FUNC_EXTERN bool opn_imtidjoin(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *lp,
	OPN_JTREE *rp,
	OPE_IEQCLS eqcls,
	bool *lfflag);
/* opniomake.c */
FUNC_EXTERN void opn_iomake(
	OPS_SUBQUERY *subquery,
	OPV_BMVARS *relmap,
	OPE_BMEQCLS *eqmap,
	OPE_BMEQCLS *iomap);
/* opnjmaps.c */
FUNC_EXTERN bool opn_jmaps(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *nodep,
	OPL_BMOJ *ojmap);
/* opnmemory.c */
FUNC_EXTERN PTR opn_jmemory(
	OPS_STATE *global,
	i4 size);
FUNC_EXTERN void opn_fmemory(
	OPS_SUBQUERY *subquery,
	PTR *target);
FUNC_EXTERN PTR opn_memory(
	OPS_STATE *global,
	i4 size);
/* opnncommon.c */
FUNC_EXTERN void opn_ncommon(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *nodep,
	OPN_RLS **rlmp,
	OPN_EQS **eqclp,
	OPN_SUBTREE **subtp,
	OPE_BMEQCLS *eqr,
	OPN_SRESULT search,
	OPN_LEAVES nleaves,
	OPO_BLOCKS *blocks,
	bool selapplied,
	OPN_PERCENT selectivity,
	OPO_TUPLES *tuples);
FUNC_EXTERN i4 opn_pagesize(
	OPG_CB *opg_cb,
	OPS_WIDTH width);
/* opnncost.c */
FUNC_EXTERN bool opn_nodecost(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *np,
	OPE_BMEQCLS *eqr,
	OPN_SUBTREE **subtp,
	OPN_EQS **eqclp,
	OPN_RLS **rlmp,
	OPN_STATUS *sigstat);
/* opnnewenum.c */
FUNC_EXTERN void opn_bestfrag(
	OPS_SUBQUERY *subquery);
FUNC_EXTERN OPN_STATUS opn_newenum(
	OPS_SUBQUERY *subquery,
	i4 enumcode);
/* opnpart.c */
FUNC_EXTERN bool opn_partition(
	OPS_SUBQUERY *subquery,
	OPN_STLEAVES partbase,
	OPN_LEAVES numleaves,
	OPN_PARTSZ partsz,
	OPN_CHILD numpartitions,
	bool parttype);
/* opnprcost.c */
FUNC_EXTERN OPO_BLOCKS opn_readahead(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop);
FUNC_EXTERN void opn_prcost(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop,
	OPB_SARG *sargtype,
	OPO_BLOCKS *pblk,
	OPH_HISTOGRAM **hpp,
	bool *imtid,
	bool order_only,
	OPO_ISORT *ordering,
	bool *pt_valid,
	OPO_BLOCKS *dio,
	OPO_CPU *cpu);
/* opnprleaf.c */
FUNC_EXTERN OPN_STATUS opn_prleaf(
	OPS_SUBQUERY *subquery,
	OPN_SUBTREE *jsbtp,
	OPN_EQS *eqsp,
	OPN_RLS *rlsp,
	OPO_BLOCKS blocks,
	bool selapplied,
	OPN_PERCENT selectivity,
	OPV_VARS *varp,
	bool keyed_only);
/* opnprocess.c */
FUNC_EXTERN void opn_cjtree(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *jtree);
FUNC_EXTERN OPN_STATUS opn_process(
	OPS_SUBQUERY *subquery,
	i4 numleaves);
/* opnrefmt.c */
FUNC_EXTERN void opn_reformat(
	OPS_SUBQUERY *subquery,
	OPO_TUPLES tuples,
	OPS_WIDTH width,
	OPO_BLOCKS *diop,
	OPO_CPU *cpup);
/* opnrmemory.c */
FUNC_EXTERN OPN_RLS *opn_rmemory(
	OPS_SUBQUERY *subquery);
/* opnro3.c */
FUNC_EXTERN bool opn_ro3(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *nodep);
/* opnro4.c */
FUNC_EXTERN OPV_IVARS opn_ro4(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *nodep,
	OPV_BMVARS *rmap);
/* opnsavest.c */
FUNC_EXTERN void opn_savest(
	OPS_SUBQUERY *subquery,
	i4 nleaves,
	OPN_SUBTREE *subtp,
	OPN_EQS *eqclp,
	OPN_RLS *rlmp);
/* opnsm1.c */
FUNC_EXTERN void opn_sm1(
	OPS_SUBQUERY *subquery,
	OPE_BMEQCLS *eqm,
	OPE_BMEQCLS *eqh,
	OPB_BMBF *bfm,
	OPV_BMVARS *vmap);
/* opnsm2.c */
FUNC_EXTERN void opb_gbfbm(
	OPS_SUBQUERY *subquery,
	OPE_BMEQCLS *avail,
	OPE_BMEQCLS *lavail,
	OPE_BMEQCLS *ravail,
	OPL_BMOJ *lojevalmap,
	OPL_BMOJ *rojevalmap,
	OPL_BMOJ *ojevalmap,
	OPE_IEQCLS maxeqcls,
	OPV_VARS *lvarp,
	OPV_VARS *rvarp,
	OPB_BMBF *jxbfm,
	OPE_BMEQCLS *jeqh,
	OPE_BMEQCLS *jxeqh,
	OPE_BMEQCLS *leqr,
	OPE_BMEQCLS *reqr,
	OPV_BMVARS *lvmap,
	OPV_BMVARS *rvmap,
	OPV_BMVARS *bvmap,
	OPL_IOUTER ojid,
	OPL_BMOJ *ojinnermap,
	OPO_CO *cop);
FUNC_EXTERN void opn_sm2(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *np,
	OPE_BMEQCLS *jxeqh,
	OPB_BMBF *jxbfm,
	OPE_BMEQCLS *jeqh,
	OPE_BMEQCLS *byeqcmap,
	OPE_BMEQCLS *leqr,
	OPE_BMEQCLS *reqr,
	OPE_BMEQCLS *eqr,
	OPN_JEQCOUNT *jxcnt,
	OPO_ISORT *jordering,
	OPO_ISORT *jxordering,
	OPO_ISORT *lkeyjoin,
	OPO_ISORT *rkeyjoin);
/* opnsmemory.c */
FUNC_EXTERN OPN_SUBTREE *opn_smemory(
	OPS_SUBQUERY *subquery);
/* opnsrchst.c */
FUNC_EXTERN OPN_SRESULT opn_srchst(
	OPS_SUBQUERY *subquery,
	OPN_LEAVES nleaves,
	OPE_BMEQCLS *eqr,
	OPV_BMVARS *rmap,
	OPN_STLEAVES rlasg,
	OPN_TDSC sbstruct,
	OPN_SUBTREE **subtp,
	OPN_EQS **eqclp,
	OPN_RLS **rlmp,
	bool *icheck);
/* opntpblk.c */
FUNC_EXTERN OPO_TUPLES opn_tpblk(
	OPG_CB *opg_cb,
	i4 pagesize,
	OPS_WIDTH tuplewidth);
/* opntree.c */
FUNC_EXTERN void opn_jtalloc(
	OPS_SUBQUERY *subquery,
	OPN_JTREE **jtreep);
FUNC_EXTERN OPN_STATUS opn_tree(
	OPS_SUBQUERY *subquery);
/* opnukey.c */
FUNC_EXTERN bool opn_ukey(
	OPS_SUBQUERY *subquery,
	OPE_BMEQCLS *oeqcmp,
	OPO_ISORT jeqc,
	OPV_IVARS keyedvar,
	bool primkey);
/* opnurn.c */
FUNC_EXTERN OPH_DOMAIN opn_urn(
	OPO_TUPLES rballs,
	OPH_DOMAIN ncells);
/* opocombine.c */
FUNC_EXTERN OPO_ISORT opo_combine(
	OPS_SUBQUERY *subquery,
	OPO_ISORT jordering,
	OPO_ISORT outer,
	OPO_ISORT inner,
	OPE_BMEQCLS *eqcmap,
	OPO_ISORT *jorderp);
FUNC_EXTERN void opo_mexact(
	OPS_SUBQUERY *subquery,
	OPO_ISORT *ordering,
	OPO_ISORT psjeqc,
	bool valid_sjeqc,
	OPO_ISORT pordeqc,
	bool valid_ordeqc,
	bool truncate);
/* opocopyco.c */
FUNC_EXTERN void opo_copyfragco(
	OPS_SUBQUERY *subquery,
	OPO_CO **copp,
	bool top);
FUNC_EXTERN i4 opo_copyco(
	OPS_SUBQUERY *subquery,
	OPO_CO **copp,
	bool remove);
/* opocordr.c */
FUNC_EXTERN OPO_ISORT opo_gnumber(
	OPS_SUBQUERY *subquery);
FUNC_EXTERN void opo_iobase(
	OPS_SUBQUERY *subquery);
FUNC_EXTERN OPO_ISORT opo_cordering(
	OPS_SUBQUERY *subquery,
	OPO_EQLIST *keyp,
	bool copykey);
FUNC_EXTERN void opo_makey(
	OPS_SUBQUERY *subquery,
	OPB_MBF *mbfp,
	i4 count,
	OPO_EQLIST **keylistp,
	bool tempmem,
	bool noconst);
FUNC_EXTERN void opo_pr(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop);
FUNC_EXTERN void opo_orig(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop);
FUNC_EXTERN void opo_rsort(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop);
FUNC_EXTERN OPO_ISORT opo_kordering(
	OPS_SUBQUERY *subquery,
	OPV_VARS *varp,
	OPE_BMEQCLS *eqcmap);
/* opofordr.c */
FUNC_EXTERN OPO_ISORT opo_fordering(
	OPS_SUBQUERY *subquery,
	OPE_BMEQCLS *eqcmap);
/* opominc.c */
FUNC_EXTERN OPO_CO *opo_mincocost(
	OPS_SUBQUERY *subquery,
	OPN_SUBTREE *subtp,
	OPO_COST *mcost);
/* opsalter.c */
FUNC_EXTERN i4 ops_exlock(
	OPF_CB *opf_cb,
	SCF_SEMAPHORE *semaphore);
FUNC_EXTERN i4 ops_unlock(
	OPF_CB *opf_cb,
	SCF_SEMAPHORE *semaphore);
FUNC_EXTERN i4 ops_alter(
	OPF_CB *opf_cb);
/* opsbgnsess.c */
FUNC_EXTERN i4 ops_bgn_session(
	OPF_CB *opf_cb);
/* opsdealloc.c */
FUNC_EXTERN void ops_deallocate(
	OPS_STATE *global,
	bool report,
	bool partial_dbp);
/* opsendsess.c */
FUNC_EXTERN i4 ops_end_session(
	OPF_CB *opf_cb);
/* opsgetcb.c */
FUNC_EXTERN OPS_CB *ops_getcb(void);
/* opsinit.c */
FUNC_EXTERN i4 ops_gqtree(
	OPS_STATE *global);
FUNC_EXTERN void ops_qinit(
	OPS_STATE *global,
	PST_STATEMENT *statementp);
FUNC_EXTERN void ops_init(
	OPF_CB *opf_cb,
	OPS_STATE *global);
/* opsseq.c */
FUNC_EXTERN void ops_decvar(
	OPS_STATE *global,
	PST_QNODE *constp);
FUNC_EXTERN i4 ops_sequencer(
	OPF_CB *opf_cb);
/* opsshut.c */
FUNC_EXTERN i4 ops_shutdown(
	OPF_CB *opf_cb);
/* opsstartup.c */
FUNC_EXTERN i4 ops_startup(
	OPF_CB *opf_cb);
/* optcall.c */
FUNC_EXTERN i4 opt_call(
	DB_DEBUG_CB *debug_cb);
/* optcotree.c */
FUNC_EXTERN i4 opt_scc(
	PTR global,
	i4 length,
	char *msg_buffer);
FUNC_EXTERN i4 opt_noblanks(
	i4 strsize,
	char *charp);
FUNC_EXTERN void opt_catname(
	OPS_SUBQUERY *subquery,
	PST_QNODE *qnode,
	OPT_NAME *attrname);
FUNC_EXTERN void opt_paname(
	OPS_SUBQUERY *subquery,
	OPZ_IATTS attno,
	OPT_NAME *attrname);
FUNC_EXTERN bool opt_ptname(
	OPS_SUBQUERY *subquery,
	OPV_IVARS varno,
	OPT_NAME *namep,
	bool corelation);
FUNC_EXTERN void opt_coprint(
	OPO_CO *cop,
	PTR uld_control);
FUNC_EXTERN void opt_tbl(
	RDR_INFO *rdrinfop);
FUNC_EXTERN void opt_prattr(
	RDR_INFO *rdrinfop,
	i4 dmfattr);
FUNC_EXTERN void opt_rattr(
	RDR_INFO *rdrinfop);
FUNC_EXTERN void opt_index(
	RDR_INFO *rdrinfop);
FUNC_EXTERN void opa_pconcurrent(
	OPS_SUBQUERY *header);
#ifdef xDEBUG
FUNC_EXTERN void opt_agall(void);
#endif
FUNC_EXTERN void opt_ehistmap(
	OPE_BMEQCLS *eqh);
FUNC_EXTERN void opt_enum(void);
FUNC_EXTERN void opt_evp(
	OPN_EVAR *evp,
	i4 vcount);
FUNC_EXTERN void opt_rv(
	OPV_IVARS rv,
	OPS_SUBQUERY *sq,
	bool histdump);
FUNC_EXTERN void opt_ojstuff(
	OPS_SUBQUERY *sq1);
FUNC_EXTERN void opt_rall(
	bool histdump);
FUNC_EXTERN void opt_grange(
	OPV_IGVARS grv);
FUNC_EXTERN void opt_jnode(
	OPN_JTREE *nodep);
FUNC_EXTERN void opt_jtree(void);
FUNC_EXTERN void opt_eclass(
	OPE_IEQCLS eqcls);
FUNC_EXTERN void opt_eall(void);
FUNC_EXTERN void opt_jall(
	OPS_SUBQUERY *subquery);
FUNC_EXTERN PTR opt_coleft(
	OPO_CO *cop);
FUNC_EXTERN PTR opt_coright(
	OPO_CO *cop);
FUNC_EXTERN void opt_printCostTree(
	OPO_CO *cop,
	bool withStats);
FUNC_EXTERN void opt_cotree_without_stats(
	OPS_STATE *global);
FUNC_EXTERN void opt_cotree(
	OPO_CO *cop);
FUNC_EXTERN void opt_pt_entry(
	OPS_STATE *global,
	PST_QTREE *header,
	PST_QNODE *root,
	char *origin);
FUNC_EXTERN void opt_qep_dump(
	OPS_STATE *global,
	OPS_SUBQUERY *subquery);
/* optdata.c */
/* optstrace.c */
FUNC_EXTERN bool opt_strace(
	OPS_CB *ops_cb,
	i4 traceflag);
FUNC_EXTERN bool opt_svtrace(
	OPS_CB *ops_cb,
	i4 traceflag,
	i4 *firstvalue,
	i4 *secondvalue);
FUNC_EXTERN bool opt_gtrace(
	OPG_CB *opg_cb,
	i4 traceflag);
/* opttretx.c */
FUNC_EXTERN void opt_treetext(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop);
/* opttxttree.c */
FUNC_EXTERN void opt_seg_dump(
	OPS_STATE *global,
	QEQ_TXT_SEG *qseg);
FUNC_EXTERN void opt_ldb_dump(
	OPS_STATE *global,
	QEQ_LDB_DESC *ldbdesc,
	bool justone);
FUNC_EXTERN void opt_qrydump(
	OPS_STATE *global,
	QEQ_D1_QRY *qryptr);
FUNC_EXTERN void opt_qtdump(
	OPS_STATE *global,
	QEF_AHD *dda);
FUNC_EXTERN void opt_qpdump(
	OPS_STATE *global,
	QEF_QP_CB *qplan);
FUNC_EXTERN bool opt_chkprint(
	char *buf_to_check,
	i4 buflen);
FUNC_EXTERN void opt_seg_chk(
	OPS_STATE *global,
	QEQ_D1_QRY *qseg);
FUNC_EXTERN void opt_qtchk(
	OPS_STATE *global,
	QEF_QP_CB *qplan);
/* opualloc.c */
FUNC_EXTERN PTR opu_allocate(
	OPS_STATE *global);
/* opucompare.c */
FUNC_EXTERN i4 opu_dcompare(
	OPS_STATE *global,
	DB_DATA_VALUE *vp1,
	DB_DATA_VALUE *vp2);
FUNC_EXTERN i4 opu_compare(
	OPS_STATE *global,
	PTR vp1,
	PTR vp2,
	DB_DATA_VALUE *datatype);
/* opudealloc.c */
FUNC_EXTERN void opu_deallocate(
	OPS_STATE *global,
	PTR *streamid);
/* opumemory.c */
FUNC_EXTERN PTR opu_memory(
	OPS_STATE *global,
	i4 size);
FUNC_EXTERN void opu_mark(
	OPS_STATE *global,
	ULM_RCB *ulmrcb,
	ULM_SMARK *mark);
FUNC_EXTERN void opu_release(
	OPS_STATE *global,
	ULM_RCB *ulmrcb,
	ULM_SMARK *mark);
/* opuqtprint.c */
FUNC_EXTERN void opu_qtprint(
	OPS_STATE *global,
	PST_QNODE *root,
	char *origin);
/* opusmemory.c */
FUNC_EXTERN void opu_Osmemory_open(
	OPS_STATE *global);
FUNC_EXTERN void opu_Csmemory_close(
	OPS_STATE *global);
FUNC_EXTERN PTR opu_Gsmemory_get(
	OPS_STATE *global,
	i4 size);
FUNC_EXTERN void opu_Msmemory_mark(
	OPS_STATE *global,
	ULM_SMARK *mark);
FUNC_EXTERN void opu_Rsmemory_reclaim(
	OPS_STATE *global,
	ULM_SMARK *mark);
/* oputmemory.c */
FUNC_EXTERN PTR opu_tmemory(
	OPS_STATE *global,
	i4 size);
/* opvconst.c */
FUNC_EXTERN void opv_rqinsert(
	OPS_STATE *global,
	OPS_PNUM parmno,
	PST_QNODE *qnode);
FUNC_EXTERN PST_QNODE *opv_constnode(
	OPS_STATE *global,
	DB_DATA_VALUE *datatype,
	i4 parmno);
FUNC_EXTERN PST_QNODE *opv_intconst(
	OPS_STATE *global,
	i4 intvalue);
FUNC_EXTERN PST_QNODE *opv_i1const(
	OPS_STATE *global,
	i4 intvalue);
FUNC_EXTERN void opv_uparameter(
	OPS_STATE *global,
	PST_QNODE *qnode);
/* opvcpytr.c */
FUNC_EXTERN void opv_copynode(
	OPS_STATE *global,
	PST_QNODE **nodepp);
FUNC_EXTERN void opv_copytree(
	OPS_STATE *global,
	PST_QNODE **nodepp);
FUNC_EXTERN void opv_push_ptr(
	OPV_STK *base,
	PTR nodep);
FUNC_EXTERN PTR opv_pop_ptr(
	OPV_STK *base);
FUNC_EXTERN void opv_pop_all(
	OPV_STK *base);
FUNC_EXTERN PST_QNODE *opv_parent_node(
	OPV_STK *base,
	PST_QNODE *child);
FUNC_EXTERN PST_QNODE *opv_antecedant_by_1type(
	OPV_STK *base,
	PST_QNODE *child,
	PST_TYPE t1);
FUNC_EXTERN PST_QNODE *opv_antecedant_by_2types(
	OPV_STK *base,
	PST_QNODE *child,
	PST_TYPE t1,
	PST_TYPE t2);
FUNC_EXTERN PST_QNODE *opv_antecedant_by_3types(
	OPV_STK *base,
	PST_QNODE *child,
	PST_TYPE t1,
	PST_TYPE t2,
	PST_TYPE t3);
FUNC_EXTERN PST_QNODE *opv_antecedant_by_4types(
	OPV_STK *base,
	PST_QNODE *child,
	PST_TYPE t1,
	PST_TYPE t2,
	PST_TYPE t3,
	PST_TYPE t4);
/* opvcreate.c */
FUNC_EXTERN void opv_submap(
	OPS_SUBQUERY *subquery,
	OPV_GBMVARS *globalmap);
FUNC_EXTERN void opv_create(
	OPS_SUBQUERY *subquery);
/* opvctrees.c */
FUNC_EXTERN bool opv_ctrees(
	OPS_STATE *global,
	PST_QNODE *node1,
	PST_QNODE *node2);
/* opvdeass.c */
FUNC_EXTERN void opv_deassign(
	OPS_STATE *global,
	OPV_IGVARS varno);
/* opvgrtinit.c */
FUNC_EXTERN void opv_grtinit(
	OPS_STATE *global,
	bool rangetable);
/* opvgrv.c */
FUNC_EXTERN OPV_IGVARS opv_agrv(
	OPS_STATE *global,
	DB_TAB_NAME *name,
	DB_OWN_NAME *owner,
	DB_TAB_ID *table_id,
	OPS_SQTYPE sqtype,
	bool abort,
 	OPV_GBMVARS *gbmap,
	OPV_IGVARS gvarno);
/* opvijnbl.c */
FUNC_EXTERN void opv_pcjnbl(
	OPS_SUBQUERY *subquery);
FUNC_EXTERN void opv_ijnbl(
	OPS_SUBQUERY *subquery);
/* opvindex.c */
FUNC_EXTERN void opv_index(
	OPS_SUBQUERY *subquery);
/* opvmapvar.c */
FUNC_EXTERN void opv_mapvar(
	PST_QNODE *nodep,
	OPV_GBMVARS *map);
/* opvopnode.c */
FUNC_EXTERN PST_QNODE *opv_opnode(
	OPS_STATE *global,
	i4 type,
	ADI_OP_ID operator,
	OPL_IOUTER ojid);
/* opvparser.c */
FUNC_EXTERN bool opv_parser(
	OPS_STATE *global,
	OPV_IGVARS gvar,
	OPS_SQTYPE sqtype,
	bool rdfinfo,
	bool psf_table,
	bool abort);
/* opvrelatts.c */
FUNC_EXTERN void opv_virtual_table(
	OPS_SUBQUERY *subquery,
	OPV_VARS *lrvp,
	OPV_IGVARS grv);
FUNC_EXTERN OPV_IVARS opv_relatts(
	OPS_SUBQUERY *subquery,
	OPV_IGVARS grv,
	OPE_IEQCLS eqcls,
	OPV_IVARS primary);
FUNC_EXTERN void opv_tstats(
	OPS_SUBQUERY *subquery);
/* opvresdom.c */
FUNC_EXTERN PST_QNODE *opv_resdom(
	OPS_STATE *global,
	PST_QNODE *function,
	DB_DATA_VALUE *datatype);
/* opvsmap.c */
FUNC_EXTERN void opv_smap(
	OPS_SUBQUERY *subquery);
/* opvtsize.c */
FUNC_EXTERN i4 opv_tuplesize(
	OPS_STATE *global,
	PST_QNODE *qnode);
/* opvvarnode.c */
FUNC_EXTERN PST_QNODE *opv_varnode(
	OPS_STATE *global,
	DB_DATA_VALUE *datatype,
	OPV_IGVARS variable,
	DB_ATT_ID *dmfattr);
FUNC_EXTERN PST_QNODE *opv_qlend(
	OPS_STATE *global);
/* opxcall.c */
FUNC_EXTERN i4 opx_call(
	i4 eventid,
	PTR opscb);
FUNC_EXTERN i4 opx_init(
	OPF_CB *opf_cb);
/* opxcinter.c */
FUNC_EXTERN OPN_STATUS opx_cinter(
	OPS_SUBQUERY *subquery);
/* opxerror.c */
FUNC_EXTERN i4 opx_status(
	OPF_CB *opf_cb,
	OPS_CB *ops_cb);
FUNC_EXTERN void opx_rverror(
	OPF_CB *opfcb,
	i4 status,
	OPX_ERROR error,
	OPX_FACILITY facility);
FUNC_EXTERN void opx_verror(
	i4 status,
	OPX_ERROR error,
	OPX_FACILITY facility);
FUNC_EXTERN void opx_lerror(
	OPX_ERROR error,
	i4 num_parms,
	PTR p1,
	PTR p2,
	PTR p3,
	PTR p4);
FUNC_EXTERN void opx_1perror(
	OPX_ERROR error,
	PTR p1);
FUNC_EXTERN void opx_2perror(
	OPX_ERROR error,
	PTR p1,
	PTR p2);
FUNC_EXTERN void opx_vrecover(
	i4 status,
	OPX_ERROR error,
	OPX_FACILITY facility);
FUNC_EXTERN void opx_rerror(
	OPF_CB *opfcb,
	OPX_ERROR error);
FUNC_EXTERN void opx_error(
	OPX_ERROR error);
FUNC_EXTERN i4 opx_float(
	OPX_ERROR error,
	bool hard);
#ifdef EX_JNP_LJMP
FUNC_EXTERN i4 opx_exception(
	EX_ARGS *args);
#endif
/* opzaddatts.c */
FUNC_EXTERN OPZ_IATTS opz_addatts(
	OPS_SUBQUERY *subquery,
	OPV_IVARS joinopvar,
	OPZ_DMFATTR dmfattr,
	DB_DATA_VALUE *datatype);
/* opzcreate.c */
FUNC_EXTERN void opz_create(
	OPS_SUBQUERY *subquery);
/* opzfattr.c */
FUNC_EXTERN OPZ_IATTS opz_ftoa(
	OPS_SUBQUERY *subquery,
	OPZ_FATTS *fatt,
	OPV_IVARS fvarno,
	PST_QNODE *function,
	DB_DATA_VALUE *datatype,
	OPZ_FACLASS class);
FUNC_EXTERN void opz_fattr(
	OPS_SUBQUERY *subquery);
FUNC_EXTERN void opz_faeqcmap(
	OPS_SUBQUERY *subquery,
	OPE_BMEQCLS *targetp,
	OPV_BMVARS *varmap);
/* opzfindatt.c */
FUNC_EXTERN OPZ_IATTS opz_findatt(
	OPS_SUBQUERY *subquery,
	OPV_IVARS joinopvar,
	OPZ_DMFATTR dmfattr);
/* opzfmake.c */
FUNC_EXTERN OPZ_FATTS *opz_fmake(
	OPS_SUBQUERY *subquery,
	PST_QNODE *qnode,
	OPZ_FACLASS class);
/* opzsvqua.c */
FUNC_EXTERN void opz_savequal(
	OPS_SUBQUERY *subquery,
	PST_QNODE *andnode,
	OPV_GBMVARS *lvarmap,
	OPV_GBMVARS *rvarmap,
	PST_QNODE *oldnodep,
	PST_QNODE *inodep,
	bool left_iftrue,
	bool right_iftrue);
