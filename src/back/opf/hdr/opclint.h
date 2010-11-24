/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

/**
** Name: OPCLINT.H - FUNC_EXTERN used locally in OPC
**
** Description:
**      This file contains the FUNC_EXTERN definitions for procedures
**	used only within OPC.  It should be include in all OPC files.
**      This allows any procedure which is not static to be defined
**      in only one place, so modifications and additions do not require
**      changes to multiple files for FUNC_EXTERN definitions.
**
**	The ifdef'ed code is used for function prototypes so that
**      parameter passing can be checked when running lint when the
**      OPS_LINT is defined.  This can also be used to determine
**      whether all the externally referenced procedures are FUNC_EXTERN'ed
**      correctly.
**
** History:
**      13-dec-88 (seputis)
**          initial creation
**	25-jul-89 (jrb)
**	    Changed interface for opc_adcalclen.
**	29-jan-91 (stec)
**	    Reflect changes in the interface to opc_materialize().
**	12-feb-91 (kwatts)
**	    Add entries for opc_sd_qual().
**	15-feb-91 (stec)
**	    Added definition for opc_exhandler().
**	11-aug-92 (rickh)
**	    Added definition for opc_tempTable.
**	22-aug-92 (rickh)
**	    Added prototypes for OPCDQP, OPCPRTREE, OPCDQCMP, OPCQTXT,
**	    OPCTRTXT, OPCTRUTL functions.
**	19-nov-92 (rickh)
**	    Added opc_allocateIDrow, opc_createTableAHD, opc_createIntegrityAHD
**	    for FIPS CONSTRAINTS (release 6.5).
**	14-dec-92 (jhahn)
**	    Added definition for opc_lvar_info.
**	23-dec-92 (andre)
**	    added prototype for opc_crt_view_ahd()
**	21-jan-93 (anitap)
**	    Added prototypes for opc_schemaahd_build() and 
**	    opc_execimmahd_build().
**	12-feb-93 (jhahn)
**	    Added parameter to opc_temptable
**	04-mar-93 (anitap)
**	    Changed opc_schemaahd_build() and opc_exeimmahd_build() function
**	    names to opc_shemaahd_build() and opc_eximmahd_build() to conform to
**	    standard.
**	03-mar-93 (barbara)
**	    Added argument to opc_gatname() to support delimited ids in Star.
**	28-apr-93 (rickh)
**	    Removed an argument from opc_cojnull.  Removed opc_sprow and
**	    opc_ojequal_build.  The former no longer exists and the latter
**	    is static.
**	13-oct-93 (swm)
**	    Bug #56448
**	    Made opc_prnode() portable by eliminating variable-sized
**	    arguments a1, a2 .. a9. Instead caller is required to pass
**	    an already-formatted buffer in frmt; this can be achieved
**	    by using a real varargs function from the CL. It is not
**	    possible to implement opc_prnode as a real varargs function
**	    as this practice is outlawed in main line code.
**	    At present, no callers pass variable arguments so no other
**	    changes were necessary.
**	7-nov-95 (inkdo01)
**	    Changed a variety of declarations from QEN_ADF* to QEN_ADF** as
**	    part of replacement of QEN_ADF instances by pointers. Also added
**	    opc_adalloc_begin for same set of changes.
**	20-mar-96 (nanpr01)
**	    Added the fuction prototype opc_pagesize for calculating the 
**	    page_size from the tuple size.
**	26-mar-97 (inkdo01)
**	    Added prototype for opc_adlabres to perform label resolution
**	    as part of dropping conjunctive normal form.
**	28-may-97 (inkdo01)
**	    Added tidatt parm to opc_iqual to fix TID qual stuff in key joins.
**	23-oct-97 (inkdo01)
**	    Added fattmap parm to opc_fatts call for bug 86478.
**	23-oct-98 (inkdo01)
**	    Added opc_forahd_build for row producing procedure feature.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	26-feb-99 (inkdo01)
**	    Added opc_hjoin_build for hash join support.
**	23-mar-99 (sarjo01)
**	    Added OPE_BMEQCLS param to opc_ojnull_build()
**	20-may-99 (hayke02 for inkdo01)
**	    Added prototype for opc_ijkqual() - builds single qualification
**	    expression from all non-outer key join predicates. This helps
**	    non-outer key joins to run much faster. This change fixes bug
**	    96840.
**	25-jun-99 (inkdo01)
**	    Added keymap parm to opc_ijkqual() and opc_kjkeyinfo() to further
**	    improve key join performance.
**	22-dec-00 (inkdo01)
**	    Added parm to opc_rdmove for inline aggregate code generation.
**	23-jan-01 (inkdo01)
**	    Replaced QEn_ADF ptr with QEF_QEP ptr (convention be damned) 
**	    in opc_agtarget call for hash aggregation.
**	17-july-01 (inkdo01)
**	    Added opt_qp_brief for op149 output.
**      03-oct-03  (chash01)
**          Add a boolean arg to opc_ijkqual() to indicate RMS gateway. 
**          This is to fix rms gateway bug 110758
**	27-jan-04 (inkdo01)
**	    Added opc_partdim_copy for partitioned table processing.
**	28-june-05 (inkdo01)
**	    Added opc_expropt() for CX optimization.
**	7-Dec-2005 (kschendel)
**	    Update protos for hash-join key processing.
**	12-Dec-2005 (kschendel)
**	    Update opc qen-adf protos.
**	15-june-06 (dougi)
**	    Added opc_cparmret() for "before" triggers.
**	17-Aug-2006 (kibro01) Bug 116481
**	    If a partition is created using an integer and it is accessed
**	    using a string (which is usually automatically a varchar) then
**	    allow the coercion if we are being called from pqmat
**	1-Aug-2007 (kschendel) SIR 122513
**	    Protos for new partition qualification routines.
**	8-Jan-2008 (kschendel) SIR 122513
**	    Protos added for T/K-join inclusion in new part qual.
**	2-Jun-2009 (kschendel) b122118
**	    Delete unused gtqual and adbase parameters.
**	25-Aug-2009 (kschendel) 121804
**	    Add a couple missing prototypes.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

#include <excl.h>

/* opcadf.c */
FUNC_EXTERN void opc_adalloc_begin(
	OPS_STATE *global,
	OPC_ADF *cadf,
	QEN_ADF **qadf,
	i4 ninstr,
	i4 nops,
	i4 nconst,
	i4 szconst,
	i4 max_base,
	i4 mem_flag);
FUNC_EXTERN void opc_adalloct_begin(
	OPS_STATE *global,
	OPC_ADF *cadf,
	QEN_ADF **qadf,
	i4 ninstr,
	i4 nops,
	i4 nconst,
	i4 szconst,
	i4 max_base,
	i4 mem_flag);
FUNC_EXTERN void opc_adconst(
	OPS_STATE *global,
	OPC_ADF *cadf,
	DB_DATA_VALUE *dv,
	ADE_OPERAND *adfop,
	DB_LANG qlang,
	i4 seg);
FUNC_EXTERN void opc_adparm(
	OPS_STATE *global,
	OPC_ADF *cadf,
	i4 parm_no,
	DB_LANG qlang,
	i4 seg);
FUNC_EXTERN void opc_adinstr(
	OPS_STATE *global,
	OPC_ADF *cadf,
	i4 icode,
	i4 seg,
	i4 nops,
	ADE_OPERAND ops[],
	i4 noutops);
FUNC_EXTERN void opc_adlabres(
	OPS_STATE *global,
	OPC_ADF *cadf,
	ADE_OPERAND *labop);
FUNC_EXTERN void opc_adbase(
	OPS_STATE *global,
	OPC_ADF *cadf,
	i4 array,
	i4 index);
FUNC_EXTERN void opc_adtransform(
	OPS_STATE *global,
	OPC_ADF *cadf,
	ADE_OPERAND *srcop,
	ADE_OPERAND *resop,
	i4 seg,
	bool setresop,
	bool pqmat_retry_char);
FUNC_EXTERN void opc_adend(
	OPS_STATE *global,
	OPC_ADF *cadf);
FUNC_EXTERN void opc_adcalclen(
	OPS_STATE *global,
	OPC_ADF *cadf,
	ADI_LENSPEC *lenspec,
	i4 numops,
	ADE_OPERAND *op[],
	ADE_OPERAND *resop,
	PTR data[]);
FUNC_EXTERN void opc_adempty(
	OPS_STATE *global,
	DB_DATA_VALUE *dv,
	DB_DT_ID type,
	i2 prec,
	i4 length);
FUNC_EXTERN void opc_adkeybld(
	OPS_STATE *global,
	i4 *key_type,
	ADI_OP_ID opid,
	DB_DATA_VALUE *kdv,
	DB_DATA_VALUE *lodv,
	DB_DATA_VALUE *hidv,
	PST_QNODE *qop);
FUNC_EXTERN void opc_adseglang(
	OPS_STATE *global,
	OPC_ADF *cadf,
	DB_LANG qlang,
	i4 seg);
FUNC_EXTERN void opc_resolve(
	OPS_STATE *global,
	DB_DATA_VALUE *idv,
	DB_DATA_VALUE *odv,
	DB_DATA_VALUE *rdv);
/* opcagg.c */
FUNC_EXTERN void opc_agtarget(
	OPS_STATE *global,
	OPC_NODE *cnode,
	PST_QNODE *aghead,
	QEF_QEP *ahd_qepp);
FUNC_EXTERN void opc_agproject(
	OPS_STATE *global,
	OPC_NODE *cnode,
	QEN_ADF **qadf,
	i4 agg_row,
	i4 prjsrc_row);
/* opcahd.c */
FUNC_EXTERN void opc_ahd_build(
	OPS_STATE *global,
	QEF_AHD **pahd,
	QEF_AHD **proj_ahd);
FUNC_EXTERN void opc_dvahd_build(
	OPS_STATE *global,
	PST_DECVAR *decvar);
FUNC_EXTERN void opc_ifahd_build(
	OPS_STATE *global,
	QEF_AHD **pahd);
FUNC_EXTERN void opc_forahd_build(
	OPS_STATE *global,
	QEF_AHD **pahd);
FUNC_EXTERN void opc_rtnahd_build(
	OPS_STATE *global,
	QEF_AHD **pahd);
FUNC_EXTERN void opc_msgahd_build(
	OPS_STATE *global,
	QEF_AHD **pahd);
FUNC_EXTERN void opc_cmtahd_build(
	OPS_STATE *global,
	QEF_AHD **pahd);
FUNC_EXTERN void opc_rollahd_build(
	OPS_STATE *global,
	QEF_AHD **pahd);
FUNC_EXTERN void opc_emsgahd_build(
	OPS_STATE *global,
	QEF_AHD **new_ahd);
FUNC_EXTERN void opc_cpahd_build(
	OPS_STATE *global,
	QEF_AHD **pahd);
FUNC_EXTERN void opc_build_cp_attribute_list(
	OPS_STATE *global,
	DB_TAB_ID *procedureID,
	DMF_ATTR_ENTRY **p_attr_list[],
	i4 *p_offset_list[]);
FUNC_EXTERN void opc_build_proc_insert_ahd(
	OPS_STATE *global,
	QEF_AHD **pahd);
FUNC_EXTERN void opc_attach_ahd_to_trigger(
	OPS_STATE *global,
	QEF_AHD *new_ahd);
FUNC_EXTERN void opc_build_closetemp_ahd(
	OPS_STATE *global,
	QEF_AHD *proc_insert_ahd);
FUNC_EXTERN void opc_deferred_cpahd_build(
	OPS_STATE *global,
	QEF_AHD **pahd);
FUNC_EXTERN void opc_iprocahd_build(
	OPS_STATE *global,
	QEF_AHD **pahd);
FUNC_EXTERN void opc_rprocahd_build(
	OPS_STATE *global,
	QEF_AHD **pahd);
FUNC_EXTERN void opc_eventahd_build(
	OPS_STATE *global,
	QEF_AHD **new_ahd);
FUNC_EXTERN void opc_shemaahd_build(
	OPS_STATE *global,
	QEF_AHD **sahd);
FUNC_EXTERN void opc_eximmahd_build(
	OPS_STATE *global,
	QEF_AHD **new_ahd);
FUNC_EXTERN void opc_createTableAHD(
	OPS_STATE *global,
	QEU_CB *createTableQEUCB,
	i4 IDrow,
	QEF_AHD **new_ahd);
FUNC_EXTERN void opc_renameAHD(
	OPS_STATE *global,
	PST_STATEMENT *pst_statement,
	i4 IDrow,
	QEF_AHD **new_ahd);
FUNC_EXTERN void opc_createIntegrityAHD(
	OPS_STATE *global,
	PST_STATEMENT *pst_statement,
	i4 IDrow,
	QEF_AHD **new_ahd);
FUNC_EXTERN void opc_crt_view_ahd(
	OPS_STATE *global,
	PST_STATEMENT *pst_statement,
	i4 IDrow,
	QEF_AHD **new_ahd);
FUNC_EXTERN void opc_d_integ(
	OPS_STATE *global,
	PST_STATEMENT *pst_statement,
	QEF_AHD **new_ahd);
FUNC_EXTERN i4 opc_pagesize(
	OPG_CB *opg_cb,
	OPS_WIDTH width);
FUNC_EXTERN void opc_partdef_copy(
	OPS_STATE *global,
	DB_PART_DEF *pdefp,
	PTR (*getmem)(OPS_STATE *, i4),
	DB_PART_DEF **outppp);
FUNC_EXTERN void opc_partdim_copy(
	OPS_STATE *global,
	DB_PART_DEF *pdefp,
	u_i2 dimno,
	DB_PART_DIM **outppp);
/* opcdqcmp.c */
FUNC_EXTERN void opc_dldbadd(
	OPS_STATE *global,
	DD_LDB_DESC *ldbptr,
	DD_LDB_DESC **outldb);
FUNC_EXTERN void opc_ssite(
	OPS_STATE *global);
FUNC_EXTERN void opc_dentry(
	OPS_STATE *global);
/* opcdqp.c */
FUNC_EXTERN void opc_dahd(
	OPS_STATE *global,
	QEF_AHD **daction);
FUNC_EXTERN void opc_qrybld(
	OPS_STATE *global,
	DB_LANG *qlang,
	bool retflag,
	QEQ_TXT_SEG *qtext,
	DD_LDB_DESC *qldb,
	QEQ_D1_QRY *qryptr);
FUNC_EXTERN i4 opc_tstsingle(
	OPS_STATE *global);
/* opcentry.c */
FUNC_EXTERN i4 opc_exhandler(
	EX_ARGS *ex_args);
FUNC_EXTERN void opc_querycomp(
	OPS_STATE *global);
/* opcexch.c */
FUNC_EXTERN void opc_exch_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
/* opcjcommon.c */
FUNC_EXTERN void opc_jinouter(
	OPS_STATE *global,
	OPC_NODE *pcnode,
	OPO_CO *co,
	i4 level,
	QEN_NODE **pqen,
	OPC_EQ **pceq,
	OPC_EQ *oceq);
FUNC_EXTERN void opc_jkinit(
	OPS_STATE *global,
	OPE_IEQCLS *jeqcs,
	i4 njeqcs,
	OPC_EQ *ceq,
	OPE_BMEQCLS *keqcmp,
	OPC_EQ **pkceq);
FUNC_EXTERN void opc_jmerge(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPC_EQ *iceq,
	OPC_EQ *oceq,
	OPC_EQ *ceq);
FUNC_EXTERN void opc_jqual(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPE_BMEQCLS *keqcmp,
	OPC_EQ *iceq,
	OPC_EQ *oceq,
	QEN_ADF **qadf,
	i4 *tidatt);
FUNC_EXTERN void opc_kqual(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPC_EQ *iceq,
	OPC_EQ *oceq,
	QEN_ADF **qadf);
FUNC_EXTERN void opc_iqual(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPC_EQ *iceq,
	QEN_ADF **qadf,
	i4 *tidatt);
FUNC_EXTERN void opc_ijkqual(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPC_EQ *iceq,
	OPC_EQ *oceq,
	OPE_BMEQCLS *keymap,
	QEN_ADF **qadf,
	i4 *tidatt,
	bool rmsflag);
FUNC_EXTERN void opc_jnotret(
	OPS_STATE *global,
	OPO_CO *co,
	OPC_EQ *ceq);
FUNC_EXTERN void opc_jqekey(
	OPS_STATE *global,
	OPE_IEQCLS *jeqcs,
	i4 njeqcs,
	OPC_EQ *iceq,
	OPC_EQ *oceq,
	RDR_INFO *rel,
	QEF_KEY **pqekey);
FUNC_EXTERN bool opc_spjoin(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPO_CO *co,
	OPC_EQ *oceq,
	OPC_EQ *ceq);
FUNC_EXTERN void opc_kjkeyinfo(
	OPS_STATE *global,
	OPC_NODE *cnode,
	RDR_INFO *rel,
	PST_QNODE *key_qtree,
	OPC_EQ *oceq,
	OPC_EQ *iceq,
	OPE_BMEQCLS *keymap);
FUNC_EXTERN bool opc_hashatts(
	OPS_STATE *global,
	OPE_BMEQCLS *keqcmp,
	OPC_EQ *oceq,
	OPC_EQ *iceq,
	DB_CMP_LIST **pcmplist,
	i4 *kcount);
FUNC_EXTERN void opc_ojqual(
	OPS_STATE *global,
	OPC_NODE *cnode,
	QEN_OJINFO *oj,
	OPC_EQ *iceq,
	OPC_EQ *oceq);
FUNC_EXTERN void opc_speqcmp(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPO_CO *child,
	OPE_BMEQCLS *eqcmp0,
	OPE_BMEQCLS *eqcmp1);
FUNC_EXTERN void opc_ojequal_build(
	OPS_STATE *global,
	OPC_NODE *cnode,
	QEN_OJINFO *oj,
	OPC_EQ sceq[],
	OPE_BMEQCLS *returnedEQCbitMap);
FUNC_EXTERN void opc_emptymap(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPO_CO *cco,
	OPE_BMEQCLS *eqcmp);
FUNC_EXTERN void opc_ojnull_build(
	OPS_STATE *global,
	OPC_NODE *cnode,
	QEN_OJINFO *oj,
	OPC_EQ *iceq,
	OPC_EQ *oceq,
	OPE_BMEQCLS *returnedEQCbitMap);
FUNC_EXTERN void opc_ojinfo_build(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPC_EQ *iceq,
	OPC_EQ *oceq);
FUNC_EXTERN void opc_tisnull(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPC_EQ *oceq,
	OPV_IVARS innerRelation,
	QEN_ADF **tisnull);
/* opcjoins.c */
FUNC_EXTERN void opc_tjoin_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
FUNC_EXTERN void opc_kjoin_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
FUNC_EXTERN void opc_hjoin_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
FUNC_EXTERN void opc_fsmjoin_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
FUNC_EXTERN void opc_cpjoin_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
FUNC_EXTERN void opc_isjoin_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
/* opckey.c */
FUNC_EXTERN void opc_srange(
	OPS_STATE *global,
	OPC_QUAL *cqual,
	OPC_KOR *dnf_or);
FUNC_EXTERN void opc_meqrange(
	OPS_STATE *global,
	OPC_QUAL *cqual,
	OPC_KOR *dnf_or);
FUNC_EXTERN void opc_range(
	OPS_STATE *global,
	OPC_QUAL *cqual,
	OPC_KOR *dnf_or);
/* opcorig.c */
FUNC_EXTERN void opc_orig_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
FUNC_EXTERN void opc_tproc_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
FUNC_EXTERN void opc_ratt(
	OPS_STATE *global,
	OPC_EQ *ceq,
	OPO_CO *co,
	i4 rowno,
	i4 rowsz);
FUNC_EXTERN QEF_VALID *opc_valid(
	OPS_STATE *global,
	OPV_GRV *grv,
	i4 lock_mode,
	i4 npage_est,
	bool size_sensitive);
FUNC_EXTERN QEF_RESOURCE *opc_get_resource(
	OPS_STATE *global,
	OPV_GRV *grv,
	i4 *ptbl_type,
	PTR *ptemp_id,
	QEF_MEM_CONSTTAB **cnsttab_p);
FUNC_EXTERN void opc_bgmkey(
	OPS_STATE *global,
	QEF_AHD *ahd,
	OPC_ADF *cadf);
FUNC_EXTERN QEN_PART_QUAL *opc_pqmat(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPV_VARS *varp,
	OPB_BMBF *bfactbm);
FUNC_EXTERN void opc_pqe_new(
	OPS_STATE *global,
	QEN_PQ_EVAL **pqe_base,
	QEN_PQ_EVAL **pqe_end,
	QEN_PQ_EVAL **pqe,
	i4 eval_op);
/* opcprtree.c */
FUNC_EXTERN void opc_prtree(
	PTR root,
	void (*printnode)(PTR, PTR),
	PTR (*leftson)(PTR),
	PTR (*rightson)(PTR),
	i4 indent,
	i4 lbl,
	i4 facility);
FUNC_EXTERN void opc_prnode(
	PTR control,
	char *frmt);
FUNC_EXTERN void opc_treedump(
	PST_QNODE *tree);
/* opcqen.c */
FUNC_EXTERN void opc_qentree_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
FUNC_EXTERN void opc_qnatts(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPO_CO *tco,
	QEN_NODE *qn);
FUNC_EXTERN void opc_fatts(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPO_CO *co,
	QEN_NODE *qn,
	QEN_ADF **pqadf,
	OPE_BMEQCLS *fattmap);
FUNC_EXTERN void opc_getFattDataType(
	OPS_STATE *global,
	i4 attno,
	ADE_OPERAND *adeOp);
FUNC_EXTERN void opc_ojspfatts(
	OPS_STATE *global,
	OPC_NODE *cnode,
	i4 *ojspecialEQCrow);
/* opcqp.c */
FUNC_EXTERN void opc_iqp_init(
	OPS_STATE *global);
FUNC_EXTERN void opc_cqp_continue(
	OPS_STATE *global);
FUNC_EXTERN void opc_fqp_finish(
	OPS_STATE *global);
FUNC_EXTERN PTR opu_qsfmem(
	OPS_STATE *global,
	i4 size);
FUNC_EXTERN void opc_ptrow(
	OPS_STATE *global,
	i4 *rowno,
	i4 rowsz);
FUNC_EXTERN void opc_allocateIDrow(
	OPS_STATE *global,
	i4 *rowno);
FUNC_EXTERN void opc_ptcparm(
	OPS_STATE *global,
	i4 *rowno,
	i4 rowsz);
FUNC_EXTERN void opc_ptcb(
	OPS_STATE *global,
	i4 *cbno,
	i4 cbsz);
FUNC_EXTERN void opc_pthold(
	OPS_STATE *global,
	i4 *holdno);
FUNC_EXTERN void opc_pthash(
	OPS_STATE *global,
	i4 *hashno);
FUNC_EXTERN void opc_tempTable(
	OPS_STATE *global,
	QEN_TEMP_TABLE **tempTable,
	i4 tupleSize,
	OPC_OBJECT_ID **tupleRow);
FUNC_EXTERN void opc_ptsort(
	OPS_STATE *global,
	i4 *sortno);
FUNC_EXTERN void opc_ptparm(
	OPS_STATE *global,
	PST_QNODE *qconst,
	i4 rindex,
	i4 roffset);
/* opcqtree.c */
FUNC_EXTERN OPC_QUAL *opc_keyqual(
	OPS_STATE *global,
	OPC_NODE *cnode,
	PST_QNODE *root_qual,
	OPV_IVARS varno,
	i4 tid_key);
FUNC_EXTERN i4 opc_onlyatts(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPC_QUAL *cqual,
	PST_QNODE *root,
	OPV_IVARS varno,
	RDR_INFO *rel,
	OPC_QSTATS *qstats,
	i4 *ncompares);
FUNC_EXTERN i4 opc_cnst_expr(
	OPS_STATE *global,
	PST_QNODE *root,
	i4 *pm_flag,
	DB_LANG *qlang);
FUNC_EXTERN PST_QNODE *opc_qconvert(
	OPS_STATE *global,
	DB_DATA_VALUE *dval,
	PST_QNODE *qconst);
FUNC_EXTERN i4 opc_compare(
	OPS_STATE *global,
	PST_QNODE *d1,
	PST_QNODE *d2);
FUNC_EXTERN void opc_gtqual(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPB_BMBF *bfactbm,
	PST_QNODE **pqual,
	OPE_BMEQCLS *qual_eqcmp);
FUNC_EXTERN void opc_gtiqual(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPB_BMBF *bfactbm,
	PST_QNODE **pqual,
	OPE_BMEQCLS *qual_eqcmp);
FUNC_EXTERN void opc_ogtqual(
	OPS_STATE *global,
	OPC_NODE *cnode,
	PST_QNODE **pqual,
	OPE_BMEQCLS *qual_eqcmp);
FUNC_EXTERN i4 opc_metaops(
	OPS_STATE *global,
	PST_QNODE *root);
/* opcqtxt.c */
FUNC_EXTERN void opc_pltext(
	OPS_STATE *global);
/* opcqual.c */
FUNC_EXTERN void opc_cxest(
	OPS_STATE *global,
	OPC_NODE *cnode,
	PST_QNODE *root,
	i4 *ninstr,
	i4 *nops,
	i4 *nconst,
	i4 *szconst);
FUNC_EXTERN i4 opc_cqual(
	OPS_STATE *global,
	OPC_NODE *cnode,
	PST_QNODE *root,
	OPC_ADF *cadf,
	i4 segment,
	ADE_OPERAND *resop,
	i4 *tidatt);
FUNC_EXTERN void opc_target(
	OPS_STATE *global,
	OPC_NODE *cnode,
	PST_QNODE *root,
	QEN_ADF **qadf);
FUNC_EXTERN i1 *opc_estdefault(
	OPS_STATE *global,
	PST_QNODE *root,
	i4 *ninstr,
	i4 *nops,
	i4 *nconst,
	i4 *szconst);
FUNC_EXTERN void opc_cdefault(
	OPS_STATE *global,
	i1 *dmap,
	i4 outbase,
	OPC_ADF *cadf);
FUNC_EXTERN void opc_rdmove(
	OPS_STATE *global,
	PST_QNODE *resdom,
	ADE_OPERAND *ops,
	OPC_ADF *cadf,
	i4 segment,
	RDR_INFO *rel,
	bool leavebase);
FUNC_EXTERN void opc_proc_insert_target_list(
	OPS_STATE *global,
	PST_QNODE *root,
	OPC_ADF *cadf,
	RDR_INFO *rel,
	i4 num_params_passed,
	i4 num_params_declared,
	DMF_ATTR_ENTRY *attr_list[],
	i4 temp_table_base,
	i4 offset_list[]);
FUNC_EXTERN void opc_crupcurs(
	OPS_STATE *global,
	PST_QNODE *root,
	OPC_ADF *cadf,
	ADE_OPERAND *resop,
	RDR_INFO *rel,
	OPC_NODE *cnode);
FUNC_EXTERN bool opc_cparmret(
	OPS_STATE *global,
	PST_QNODE *root,
	OPC_ADF *cadf,
	ADE_OPERAND *resop,
	RDR_INFO *rel);
FUNC_EXTERN void opc_lvar_info(
	OPS_STATE *global,
	i4 lvarno,
	i4 *rowno,
	i4 *offset,
	DB_DATA_VALUE **row_info);
FUNC_EXTERN void opc_lvar_row(
	OPS_STATE *global,
	i4 lvarno,
	OPC_ADF *cadf,
	ADE_OPERAND *ops);
FUNC_EXTERN void opc_expropt(
	OPS_SUBQUERY *subquery,
	OPC_NODE *cnode,
	PST_QNODE **rootp,
	PST_QNODE **subexlistp);
/* opcran.c */
FUNC_EXTERN void opc_ran_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
FUNC_EXTERN void opc_lsahd_build(
	OPS_STATE *global,
	OPS_SUBQUERY *subqry_list,
	QEF_AHD **pahd_list,
	i4 is_top_ahd);
/* opcsejoin.c */
FUNC_EXTERN void opc_rsequal(
	PST_QNODE *qual);
FUNC_EXTERN void opc_sejoin_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
FUNC_EXTERN void opc_ccqtree(
	OPS_STATE *global,
	OPC_NODE *cnode,
	PST_QNODE *root,
	QEN_ADF **qadf,
	i4 dmf_alloc_row);
FUNC_EXTERN i4 opc_eqqtree(
	OPS_STATE *global,
	OPE_BMEQCLS *eqcmap,
	PST_QNODE **inroot,
	PST_QNODE **outroot);
FUNC_EXTERN void opc_sekeys(
	OPS_STATE *global,
	OPC_NODE *cnode,
	PST_QNODE *seop,
	OPC_EQ *iceq,
	OPC_EQ *oceq);
/* opcsorts.c */
FUNC_EXTERN void opc_sort_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
FUNC_EXTERN void opc_tsort_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
FUNC_EXTERN void opc_tloffsets(
	OPS_STATE *global,
	PST_QNODE *root,
	OPC_TGINFO *tginfo,
	i4 topsort);
/* opctrtxt.c */
FUNC_EXTERN void opc_addseg(
	OPS_STATE *global,
	char *stringval,
	i4 stringlen,
	i4 temp_num,
	QEQ_TXT_SEG **seglist);
FUNC_EXTERN void opc_atname(
	OPC_TTCB *textcb,
	OPS_SUBQUERY *subquery,
	OPZ_IATTS attno,
	bool forcetemp,
	OPT_NAME *attrname);
FUNC_EXTERN void opc_rdfname(
	OPC_TTCB *textcb,
	OPV_GRV *grv_ptr,
	OPCUDNAME *name,
	PSQ_MODE qmode);
FUNC_EXTERN void opc_nat(
	OPC_TTCB *textcb,
	OPS_STATE *statevar,
	i4 varno,
	OPCUDNAME *name,
	OPT_NAME *alias,
	i4 *tempno,
	bool *is_result,
	PSQ_MODE qmode,
	bool *e_att_flag);
FUNC_EXTERN void opc_one_etoa(
	OPC_TTCB *textcb,
	OPE_IEQCLS eqcls);
FUNC_EXTERN bool opc_etoa(
	OPC_TTCB *textcb,
	OPE_IEQCLS eqcls,
	bool *nulljoin);
FUNC_EXTERN void opc_funcatt(
	OPC_TTCB *textcbp,
	OPZ_IATTS attno,
	OPO_CO *cop,
	ULM_SMARK *markp);
FUNC_EXTERN bool opc_fchk(
	OPC_TTCB *textcbp,
	OPZ_IATTS attno,
	OPO_CO *cop);
FUNC_EXTERN void opc_makename(
	DD_OWN_NAME *ownername,
	DD_TAB_NAME *tabname,
	char *bufptr,
	i2 *length,
	DD_CAPS *cap_ptr,
	PSQ_MODE qmode,
	OPS_STATE *global);
FUNC_EXTERN void opc_treetext(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop,
	bool qualonly,
	bool remdup,
	QEQ_TXT_SEG **wheretxt,
	QEQ_TXT_SEG **qrytxt,
	OPCQHD *qthdr);
/* opctrutl.c */
FUNC_EXTERN void opc_gatname(
	OPS_STATE *global,
	OPV_IGVARS gvarno,
	DB_ATT_ID *ingres_atno,
	OPT_NAME *attrname,
	DD_CAPS *cap_ptr);
FUNC_EXTERN void opcu_joinseg(
	QEQ_TXT_SEG **main_seg,
	QEQ_TXT_SEG *seg_to_add);
FUNC_EXTERN void opcu_getrange(
	OPCQHD *qhdptr,
	i4 tno,
	char *rangename);
FUNC_EXTERN void opcu_arraybld(
	PST_QNODE *root,
	PST_TYPE nodetype,
	PST_QNODE *stop_at,
	PST_QNODE *nodearray[],
	i4 *nnodes);
FUNC_EXTERN i4 opcu_tree_to_text(
	PTR handle,
	PSQ_MODE qmode,
	ULD_TSTRING *rnamep,
	ADF_CB *adfcb,
	ULM_RCB *ulmcb,
	ULD_LENGTH linelength,
	bool pretty,
	PST_QNODE *qroot,
	i4 conjflag,
	bool resdomflg,
	ULD_TSTRING **tstring,
	DB_ERROR *error,
	DB_LANG language,
	bool qualonly,
	bool finish,
	bool notqual);
/* opctuple.c */
FUNC_EXTERN void opc_materialize(
	OPS_STATE *global,
	OPC_NODE *cnode,
	QEN_ADF **qadf,
	OPE_BMEQCLS *eqcmp,
	OPC_EQ *ceq,
	i4 *prowno,
	i4 *prowsz,
	i4 align_tup,
	bool alloc_drow,
	bool projone);
FUNC_EXTERN void opc_hash_materialize(
	OPS_STATE *global,
	OPC_NODE *cnode,
	QEN_ADF **qadf,
	OPE_BMEQCLS *eqcmp,
	OPE_BMEQCLS *keqcmp,
	OPC_EQ *ceq,
	DB_CMP_LIST *cmplist,
	i4 *prowno,
	i4 *prowsz,
	bool oj);
FUNC_EXTERN void opc_kcompare(
	OPS_STATE *global,
	OPC_NODE *cnode,
	QEN_ADF **qadf,
	OPO_ISORT eqcno,
	OPC_EQ *ceq1,
	OPC_EQ *ceq2);
FUNC_EXTERN void opc_ecsize(
	OPS_STATE *global,
	OPO_ISORT eqcno,
	i4 *ninstr,
	i4 *nops);
FUNC_EXTERN void opc_emsize(
	OPS_STATE *global,
	OPE_BMEQCLS *eqcmp,
	i4 *ninstr,
	i4 *nops);
FUNC_EXTERN void opc_ecinstrs(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPC_ADF *cadf,
	OPO_ISORT ineqcno,
	OPC_EQ *ceq1,
	OPC_EQ *ceq2);
FUNC_EXTERN void opc_eminstrs(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPC_ADF *cadf,
	OPE_BMEQCLS *eqcmp,
	OPC_EQ *ceq1,
	OPC_EQ *ceq2);
/* optcomp.c */
FUNC_EXTERN void opt_qp(
	OPS_STATE *global,
	QEF_QP_CB *qp);
FUNC_EXTERN void opt_qp_brief(
	OPS_STATE *global,
	QEF_QP_CB *qp);
FUNC_EXTERN void opt_opkeys(
	OPS_STATE *global,
	OPC_QUAL *cqual);
FUNC_EXTERN void opt_opkor(
	OPS_STATE *global,
	OPC_QUAL *cqual,
	OPC_KOR *kor);
/* optftoc.c */
FUNC_EXTERN void opt_state(
	OPS_STATE *global);
FUNC_EXTERN void opt_fcodisplay(
	OPS_STATE *global,
	OPS_SUBQUERY *subqry,
	OPO_CO *co);
