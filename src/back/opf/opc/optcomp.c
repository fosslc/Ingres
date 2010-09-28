/*
**Copyright (c) 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <ade.h>
#include    <adfops.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <scf.h>
#include    <dmucb.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <uld.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefqeu.h>
#include    <qefmain.h>
#include    <gca.h>
#include    <me.h>
#include    <tr.h>
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
#include    <opckey.h>
#include    <opcd.h>
#define        OPT_COMP		TRUE
#include    <opxlint.h>
#include    <opclint.h>

/**
**
**  Name: OPTCOMP.C - Tracing routines for OPC.
**
**  Description:
**	    Note: put this into the opt directory.
{@comment_line@}...
**
{@func_list@}...
**
**
**  History:
**      5-sept-86 (eric)
**          written
**	02-sep-88 (puree)
**	    Remove qp_cb_cnt.  Rename the follwing QEF fields:
**		    qp_cb_num	to  qp_cb_cnt.
**		    qen_adf	to  qen_ade_cx
**		    qen_param	to  qen_pcx_idx
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**	31-oct-89 (stec)
**	    Implement new QEN_NODE types and outer joins.
**	04-feb-91 (neil)
**	    Alerters: Added simple tracing of EVENT nodes.
**	29-jun-90 (stec)
**	    Modified opt_ahd() to fix proj_ahd display problem.
**	13-jul-90 (stec)
**	    Replaced calls to opt_printf(), which was non-portable,
**	    with calls to TRformat().  All format strings with an '\n' 
**	    at the end of the format string had an additional '\n' added,
**	    because TRformat() strips off the first '\n' (at the end of
**	    the format string).
**	26-nov-90 (stec)
**	    Removed globalref opt_prbuf since this violated coding standards.
**	    We will use ops_prbuf field in OPS_STATE struct instead.
**	10-dec-90 (neil)
**	    Alerters: Added simple tracing of EVENT nodes.
**	28-dec-90 (stec)
**	    Changed code to reflect fact that print buffer is in OPC_STATE
**	    struct.
**	09-jan-91 (stec)
**	    Added code to print out new action types in opt_prahdqen().
**	    Modified code to avoid printing QP nodes of plans already printed.
**      04-feb-91 (neil)
**          Alerters: Added simple tracing of EVENT nodes.
**	26-mar-92 (rickh)
**	    Removed references to sjn_hget and sjn_hcreate, which weren't
**	    being used in QEF.  Also removed references to oj_create and
**	    oj_load, which weren't being used either.  Replaced references
**	    to oj_shd with oj_tidHoldFile.  Added oj_ijFlagsFile.
**	11-jun-92 (anitap)
**	    Fixed su4 acc warning in opt_qenode().	
**      16-jul-92 (anitap)
**          Cast naked NULL being passed into ade_cxbrief_print() in opt_adf().
**	17-may-93 (jhahn)
**	    Added code to print out rule action lists.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-jun-93 (rickh)
**	    Added code to print out outer join info for tid joins.
**	30-jun-93 (rickh)
**	    Added TR.H.
**      7-sep-93 (ed)
**          added uld.h for uld_prtree
**	15-sep-93 (swm)
**	    Moved cs.h include above other header files which need its
**	    definition of CS_SID.
**      5-oct-93 (eric)
**          Added support for resource lists.
**	11-oct-93 (johnst)
**	    Bug #56449
**	    Changed TRdisplay, etc, format args from %x to %p where necessary to
**	    display pointer types. We need to distinguish this on, eg 64-bit
**	    platforms, where sizeof(PTR) > sizeof(int).
**	13-oct-93 (swm)
**	    Bug #56448
**	    Altered uld_prnode calls to conform to new portable interface.
**	    It is no longer a "pseudo-varargs" function. It cannot become a
**	    varargs function as this practice is outlawed in main line code.
**	    Instead it takes a formatted buffer as its only parameter.
**	    Each call which previously had additional arguments has an
**	    STprintf embedded to format the buffer to pass to uld_prnode.
**	    This works because STprintf is a varargs function in the CL.
**	03-feb-94 (rickh)
**	    Replaced 9999 diagnostics with real error messages.
**	15-feb-94 (swm)
**	    Bug #59611
**	    Replace STprintf calls with calls to TRformat which checks
**	    for buffer overrun and use global format buffer to reduce
**	    likelihood of stack overflow.
**	10-nov-95 (inkdo01)
**	    Changes to support reorganization of QEF_AHD, QEN_NODE structs.
**      10-nov-95 (inkdo01)
**	    Changes to support replacement of QEN_ADF instances by pointers.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	19-jan-01 (inkdo01)
**	    Added QEA_HAGGF to print same as QEA_RAGGF (for hash grouping).
**	10-may-01 (inkdo01)
**	    Added all the Star AHD nodes (QEA_D0 through QEA_D10) for op150
**	    display of CDB plan.
**	21-Dec-01 (gordy)
**	    GCA_COL_ATT no longer defined with DB_DATA_VALUE.
**	13-nov-03 (inkdo01)
**	    Added support for EXCHANGE node (for || query processing).
**	10-feb-2004 (schka24)
**	    More partitioning dumps.
**	28-Jun-2004 (schka24)
**	    Dump qen-part-info for tjoin, kjoin as well as orig.
**	13-Dec-2005 (kschendel)
**	    Remaining QEN_ADF inline entries changed to pointers, fix here.
**	4-Aug-2007 (kschendel) SIR 122513
**	    Update partition-qual printer to reflect new stuff.
**	20-may-2008 (dougi)
**	    Made a variety of changes to support display of table procedure
**	    query plan nodes.
**	3-Jun-2009 (kschendel) b122118
**	    Remove obsolete adf-ag stuff, use table lookup for action code,
**	    streamline a couple places, use %p for addresses.
**	27-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-selects - add dump of cardianlity infor with QEN_NODE
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**	18-Jun-2010 (kschendel) b123775
**	    Table procs don't have valid list entries any more, remove code.
**/

/*}
** Name: OPT_AHDQEN - Data struct to hold either an action or a QEN node
**
** Description:
**      This structure points to either an action or a QEN node. It is used 
**      by the tree printing routines to print out the structure of an 
**      entire QP tree. 
[@comment_line@]...
**
** History:
**      22-Feb-88 (eric)
**          created
**	03-jan-90 (stec)
**	    Removed TYPEDEF keyword preceding struct definition.
[@history_template@]...
*/

typedef struct _OPT_QP OPT_QP;
struct _OPT_QP
{
    OPT_QP	*opt_nqp;
    i4		opt_qennum;
};

typedef struct _OPT_AHDQEN OPT_AHDQEN;
struct _OPT_AHDQEN
{
    OPS_STATE	*opt_global;
    i4		opt_aq_type;
#define		    OPT_AQ_AHD  1
#define		    OPT_AQ_QEN  2

    OPT_AHDQEN	*opt_parent;
    OPT_AHDQEN	*opt_next;	/* chain of ALL ahd's so act hdrs
				** aren't repeated under if statements */
    OPT_QP	*opt_qps;
    OPT_QP	**opt_qpsp;

    union
    {
	QEF_AHD	    *opt_pahd;
	QEN_NODE    *opt_pqen;
    } opt_aq_data;
};

/*
**  Forward and/or External function references.
*/
static VOID
opt_ahd(
	OPS_STATE	*global,
	OPT_AHDQEN	*aq,
	OPT_AHDQEN	*rootaq,
	char		*label,
	i4		ahd_no );

static VOID
opt_adf(
	OPS_STATE   *global,
	QEN_ADF	    *adf,
	char	    *title );

static VOID opt_part_def(
	OPS_STATE *global,
	DB_PART_DEF *pdefp);

static VOID opt_part_dim(
	OPS_STATE *global,
	DB_PART_DIM *pdimp,
	char *title);

static VOID opt_qenpart(
	OPS_STATE *global,
	QEN_PART_INFO *partp,
	char *title);

static VOID opt_qenpqual(
	OPS_STATE *global,
	QEN_PART_QUAL *pqual,
	char *title);

static VOID opt_dmucb(
	OPS_STATE *global,
	DMU_CB *dmucb);

static PTR
opt_lahdqen(
	PTR	p );

static PTR
opt_rahdqen(
	PTR	p );

static VOID
opt_prahdqen(
	PTR	p,
	PTR	control );

static PTR
opt_lqep(
	PTR	p );

static PTR
opt_rqep(
	PTR	p );

static VOID
opt_prqep(
	PTR	p,
	PTR	control );

static VOID
opt_qenode(
	OPS_STATE   *global,
	QEN_NODE    *qn,
	OPT_AHDQEN  *paq );

static void opt_lpchar_list(
	OPS_STATE	*global,
	QEU_LOGPART_CHAR *qeu_lpl_ptr);

static VOID
opt_keys(
	OPS_STATE   *global,
	QEF_KEY	    *key );

static VOID
opt_satts(
	OPS_STATE	*global,
	i4		acount,
	DMF_ATTR_ENTRY	**atts,
	i4		kcount,
	DMT_KEY_ENTRY	**keys,
	DB_CMP_LIST	*cmplist );

static RDR_INFO *
opt_rdrdesc(
	OPS_STATE   *global,
	QEF_AHD	    *ahd,
	i4	    dmrcb );


/*{
** Name: OPT_QP	- Print a QEF_QP_CB except for the actions.
**
** Description:
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      5-sept-86 (eric)
**          written
**	11-feb-97 (inkdo01)
**	    changes for MS Access OR transformation
**	27-oct-98 (inkdo01)
**	    Display "sqlda" contents (for queries & row producing procs).
**	21-Dec-01 (gordy)
**	    GCA_COL_ATT no longer defined with DB_DATA_VALUE, use DB_COPY macro.
**	19-Feb-2004 (schka24)
**	    Att names aren't null terminated, avoid garbage in display.
**	1-Dec-2005 (kschendel)
**	    Updates for replacement selection.
**	19-Apr-2005 (kschendel)
**	    Remove replacement selection thing, not used after all.
**	20-june-06 (dougi)
**	    Added support for BEFORE triggers and procedure parameter modes.
**	5-june-2008 (dougi)
**	    Add extra fields from qr_proc.
**	5-Aug-2009 (kschendel) SIR 122512
**	    Print the FOR-loop nesting depth.
**	05-Nov-2009 (kiria01) b122822
**	    Fix op150 segv in Star
**	1-Jul-2010 (kschendel) b124004
**	    Delete qp_upd_cb, fix here.
*/
VOID
opt_qp(
		OPS_STATE	*global,
		QEF_QP_CB	*qp)
{
    i4			i;
    i4			col1, col2, col3, col4, col5;
    i4			col2_start, col3_start, col4_start, col5_start;
    OPT_AHDQEN		*aq;
    i4			div5, mod5;
    i4			dbpno;
    QEF_DBP_PARAM	*dbpp;
    QEF_RESOURCE	*resource;
    QEF_VALID		*vl;
    RDR_INFO		*rel;
    

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"QEF_QP_CB:\n\n");
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"qp_status: %x %v\n\n", 
	qp->qp_status,
	"UPD,RPT,DEL,SQL,SHAREABLE,SINGLETON,LOB,DEFERRED,DBP_COMBRK,ROWPROC,\
NEED_TX,EXEIMM,CALLPR,PARALLEL,GLOBALBASE,NEED_QCONST,ISDBP,SCROLL,REPDYN,\
CALLS_TPROC,LOCATORS", qp->qp_status);
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"qp_fetch_ahd: %p \tqp_id.db_cur_name: %s\n\n", 
	qp->qp_fetch_ahd, qp->qp_id.db_cur_name);
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"qp_stat_cnt: %5d \tqp_cb_cnt: %5d \tqp_pcx_cnt: %5d\n\n",
	qp->qp_stat_cnt, qp->qp_cb_cnt, qp->qp_pcx_cnt);
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"qp_sort_cnt: %5d \tqp_hld_cnt: %5d \tqp_tempTableCount: %5d\n\n",
	qp->qp_sort_cnt, qp->qp_hld_cnt, qp->qp_tempTableCount);
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"qp_hash_cnt: %5d \tqp_pqual_cnt: %5d \tqp_for_depth: %d\n\n",
	qp->qp_hash_cnt, qp->qp_pqual_cnt, qp->qp_for_depth);

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"qp_ahd_cnt: %5d \t\tqp_qmode: %5d \tqp_pqhead_cnt: %d\n\n", 
	qp->qp_ahd_cnt, qp->qp_qmode, qp->qp_pqhead_cnt);
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"qp_rrowcnt_row: %5d \tqp_orowcnt_offset: %5d\n\n",
	qp->qp_rrowcnt_row, qp->qp_orowcnt_offset);
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"qp_rerrorno_row: %5d \tqp_oerrorno_offset: %5d\n\n",
	qp->qp_rerrorno_row, qp->qp_oerrorno_offset);

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"qp_res_row_sz: %5d \tqp_rssplix: %5d \tqp_sqlda: %p\n\n", 
	qp->qp_res_row_sz, qp->qp_rssplix, qp->qp_sqlda);
    /* If there's an sqlda, format its elements. */
    if (qp->qp_sqlda != NULL)
    {
	GCA_COL_ATT	*coldesc;
	i4		i, offset = 0;

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\nqp_sqlda: \n\n");
	for (i = 0, coldesc = &(((GCA_TD_DATA *)qp->qp_sqlda)->gca_col_desc[0]);
		i < ((GCA_TD_DATA *)qp->qp_sqlda)->gca_l_col_desc; i++)
	{	/* loop over column descriptors */
	    DB_DATA_VALUE	dataval;
	    i4			l_attname;
	    DB_COPY_ATT_TO_DV( coldesc, &dataval );
	    MECOPY_CONST_MACRO((PTR)&coldesc->gca_l_attname, 
		sizeof(coldesc->gca_l_attname), (PTR)&l_attname);
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "[%3d]  offset: %d, type: %d, length: %d",
		i, offset, dataval.db_datatype, dataval.db_length);
	    offset += dataval.db_length;
	    if (l_attname > 0)
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, ", name: %.#s\n\n", l_attname, coldesc->gca_attname);
	    else
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\n\n");
	    coldesc = (GCA_COL_ATT *)(((char *) coldesc) +
			sizeof(coldesc->gca_attdbv) +
			sizeof(coldesc->gca_l_attname) +
			l_attname);	/* next column entry */
	}
    }
	    
    /* lets print out the row numbers and lengths in columns for readability.
    */
    if (qp->qp_row_cnt != 0)
    {
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n\n");
    }
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"qp_row_cnt %5d \tqp->qp_res_row_sz: %5d \tqp_key_row: %5d\tqp_key_sz: %5d\n\n", 
	qp->qp_row_cnt, qp->qp_res_row_sz, qp->qp_key_row, qp->qp_key_sz);
    if (qp->qp_row_cnt > 0 && qp->qp_row_len) /* iistar doesn't always have qp_row_len set */
    {
	div5 = (qp->qp_row_cnt / 5);
	mod5 = qp->qp_row_cnt % 5;

	col1 = 0;
	col2 = div5;
	if (mod5 >= 1)
	{
	    col2 += 1;
	}
	col3 = col2 + div5;
	if (mod5 >= 2)
	{
	    col3 += 1;
	}
	col4 = col3 + div5;
	if (mod5 >= 3)
	{
	    col4 += 1;
	}
	col5 = col4 + div5;
	if (mod5 >= 4)
	{
	    col5 += 1;
	}

	col2_start = col2;
	col3_start = col3;
	col4_start = col4;
	col5_start = col5;

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "qp_row_len:\n\n");
	for ( ; col1 < col2_start; 
		col1 += 1, col2 += 1, col3 += 1, col4 += 1, col5 += 1
	    )
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "    [%2d] %5d", col1, qp->qp_row_len[col1]);

	    if (col2 < col3_start)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t[%2d] %5d", col2, qp->qp_row_len[col2]);
	    }

	    if (col3 < col4_start)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t[%2d] %5d", col3, qp->qp_row_len[col3]);
	    }

	    if (col4 < col5_start)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t[%2d] %5d", col4, qp->qp_row_len[col4]);
	    }

	    if (col5 < qp->qp_row_cnt)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t[%2d] %5d", col5, qp->qp_row_len[col5]);
	    }
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\n\n");
	}
    }

    if (qp->qp_ndbp_params > 0)
    {
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "qp_dbp_params:\n\n");

	for (dbpno = 0; dbpno < qp->qp_ndbp_params; dbpno += 1)
	{
	    dbpp = &qp->qp_dbp_params[dbpno];
	    if (qp->qp_dbp_params == NULL)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "    [%2d] ERROR: No dbp_params were found\n\n");
	    }
	    else
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "    [%2d] dbp_name: %24s\tdbp_mode: %d\n\n",
		    dbpno, dbpp->dbp_name, dbpp->dbp_mode);
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\trowno: %5d, offset: %5d\n\n", 
		    dbpp->dbp_rowno, dbpp->dbp_offset);
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\ttype: %5d, prec: %5d, length: %5d\n\n",
		    dbpp->dbp_dbv.db_datatype,
		    dbpp->dbp_dbv.db_prec,
		    dbpp->dbp_dbv.db_length);
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\tbyref_rowno: %5d, byref_offset: %5d\n\n", 
		    dbpp->dbp_byref_value_rowno, dbpp->dbp_byref_value_offset);
	    }
	}
    }

    if (qp->qp_nparams > 0)
    {
	DB_DATA_VALUE	*dvp;

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "qp_params:\n\n");

	for (dbpno = 0; dbpno < qp->qp_nparams; dbpno++)
	{
	    dvp = qp->qp_params[dbpno];
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "parm[%2d]:\ttype: %5d, prec: %5d, length: %5d\n\n",
		    dbpno+1,
		    dvp->db_datatype,
		    dvp->db_prec,
		    dvp->db_length);
	}
    }

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "qp_resources: %5d\n\n", qp->qp_cnt_resources);
    for (resource = qp->qp_resources; 
	    resource != NULL; 
	    resource = resource->qr_next
	)
    {
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\tid: %5d,", resource->qr_id_resource);
	switch (resource->qr_type)
	{
	 case QEQR_PROCEDURE:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\ttype: PROCEDURE\n\n");
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\tprocname: %24s  .owner: %24s\n\n",
		resource->qr_resource.qr_proc.qr_dbpalias.qso_n_id.db_cur_name,
		resource->qr_resource.qr_proc.qr_dbpalias.qso_n_own);
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\tproc_id1: %9d, proc_id2: %9d\n\n",
		    resource->qr_resource.qr_proc.qr_procedure_id1,
		    resource->qr_resource.qr_proc.qr_procedure_id2);
	    break;

	 case QEQR_TABLE:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\ttype: TABLE\n\n");
	    switch (resource->qr_resource.qr_tbl.qr_tbl_type)
	    {
	     case QEQR_REGTBL:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\ttbl_type: REGTBL");
		break;

	     case QEQR_TMPTBL:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\ttbl_type: TMPTBL");
		break;

	     case QEQR_SETINTBL:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\ttbl_type: SETINTBL");
		break;

	     case QEQR_MCNSTTBL:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\ttbl_type: MCNSTTBL");
	    	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\trow count: %5d \t row size: %5d\n\n",
		    resource->qr_resource.qr_tbl.qr_cnsttab_p->qr_rowcount,
		    resource->qr_resource.qr_tbl.qr_cnsttab_p->qr_rowsize);
		break;
	    }
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\ttemp_id: %5d\n\n", 
		    resource->qr_resource.qr_tbl.qr_temp_id);
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN,"\tqr_timestamp.high: %d \t qr_timestamp.low: %d\n\n",
		resource->qr_resource.qr_tbl.qr_timestamp.db_tab_high_time,
		resource->qr_resource.qr_tbl.qr_timestamp.db_tab_low_time);

	    for (vl = resource->qr_resource.qr_tbl.qr_valid;
		    vl != NULL;
		    vl = vl->vl_alt
		)
	    {
		rel = (RDR_INFO *) vl->vl_debug;

		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t\t.relname: %24s  .owner: %24s\n\n",
		    rel->rdr_rel->tbl_name.db_tab_name,
		    rel->rdr_rel->tbl_owner.db_own_name);
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t\t.base: %5d \t .index: %5d\n\n",
		    vl->vl_tab_id.db_tab_base, vl->vl_tab_id.db_tab_index);
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t\tdmr_cb: %5d \t dmf_cb: %5d\n\n",
		    vl->vl_dmr_cb, vl->vl_dmf_cb);
	    }
	    break;
	}
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\n");

    }

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"\nThe full QP tree in brief:\n\n");
    aq = (OPT_AHDQEN *) opu_Gsmemory_get(global, sizeof (OPT_AHDQEN));
    aq->opt_global = global;
    aq->opt_aq_type = OPT_AQ_AHD;
    aq->opt_parent = NULL;
    aq->opt_next = NULL;
    aq->opt_aq_data.opt_pahd = qp->qp_ahd;
    aq->opt_qps = NULL;
    aq->opt_qpsp = &aq->opt_qps;
    uld_prtree((PTR) aq, opt_prahdqen, 
				opt_lahdqen, opt_rahdqen, 4, 2, DB_OPF_ID);

    /* EJLFIX: print out the SQLDA; */

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	OPT_PBLEN, "");
    opt_ahd(global, aq, aq, "Main", 1);
}

/*{
** Name: OPT_QP_BRIEF	- Print the little tree version of QP.
**
** Description:
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      17-july-01 (inkdo01)
**          Cloned from opt_qp for tp op149.
[@history_template@]...
*/
VOID
opt_qp_brief(
		OPS_STATE	*global,
		QEF_QP_CB	*qp)
{
    OPT_AHDQEN		*aq;

    /* Set up call to tree builder & do it. */
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"\nThe full QP tree in brief:\n\n");
    aq = (OPT_AHDQEN *) opu_Gsmemory_get(global, sizeof (OPT_AHDQEN));
    aq->opt_global = global;
    aq->opt_aq_type = OPT_AQ_AHD;
    aq->opt_parent = NULL;
    aq->opt_next = NULL;
    aq->opt_aq_data.opt_pahd = qp->qp_ahd;
    aq->opt_qps = NULL;
    aq->opt_qpsp = &aq->opt_qps;
    uld_prtree((PTR) aq, opt_prahdqen, 
				opt_lahdqen, opt_rahdqen, 4, 2, DB_OPF_ID);

}

/*{
** Name: OPT_PARENT_SEEN	- Has a parent of this node been printed?
**
** Description:
**      Has the parent, or one of its parents, been seen already by the 
**      tree printing routines? This was created because of if and while
**	statements.
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      5-july-88 (eric)
**          created
[@history_template@]...
*/
static i4
opt_parent_seen(
	OPS_STATE	*global,
	OPT_AHDQEN	*aq )
{
    OPT_AHDQEN	    *parent;

    for (parent = aq->opt_parent; parent != NULL; parent = parent->opt_parent)
    {
	if (parent->opt_aq_type == aq->opt_aq_type &&
		( (parent->opt_aq_data.opt_pahd == aq->opt_aq_data.opt_pahd 
			&& aq->opt_aq_type == OPT_AQ_AHD
		   ) ||
		  (parent->opt_aq_data.opt_pqen == aq->opt_aq_data.opt_pqen
			&& aq->opt_aq_type == OPT_AQ_QEN
		   )
		)
	    )
	{
	    return (TRUE);
	}
    }

    return (FALSE);
}

/*{
** Name: OPT_AHD_SEEN	- Has this action header been printed?
**
** Description:
**      Has this action header already been seen by the tree printing 
**	routines? This prevents endless noodling with IF statements.
**
** Inputs:
**
** Outputs:
**	Returns:
**	    TRUE	- if header has already been printed
**	    FALSE	- otherwise
**
** Side Effects:
**
** History:
**      12-july-05 (inkdo01)
**	    Written to untangle IF display.
*/
static bool
opt_ahd_seen(
	OPT_AHDQEN	*rootaq,
	QEF_AHD		*ahd)
{
    OPT_AHDQEN	    *curraq;

    if (ahd == NULL)
	return(FALSE);

    for (curraq = rootaq; curraq != NULL; curraq = curraq->opt_next)
     if (ahd == curraq->opt_aq_data.opt_pahd)
	return(TRUE);

    return(FALSE);
}

/*{
** Name: OPT_LASTAQ - locate last OPT_AHDQEN in chain.
**
** Description:
**      Run opt_next chain from rootaq to locate end of chain (to append
**	next action header).
**
** Inputs:
**
** Outputs:
**	Returns:
**	    Ptr to last OPT_AHDQEN in chain.
**
** Side Effects:
**
** History:
**      12-july-05 (inkdo01)
**	    Written to untangle IF display.
*/
static OPT_AHDQEN *
opt_lastaq(
	OPT_AHDQEN	*rootaq)
{
    OPT_AHDQEN	*curraq;

    for (curraq = rootaq; curraq->opt_next != NULL; 
					curraq = curraq->opt_next);

    return(curraq);

}

/*{
** Name: OPT_LAHDQEN	- Return the left of an AHDQEN tree.
**
** Description:
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      28-oct-86 (eric)
**          written
[@history_template@]...
*/
static PTR
opt_lahdqen(
	PTR	p )
{
    OPT_AHDQEN	*aq = (OPT_AHDQEN *) p;
    OPT_AHDQEN	*ret_aq = NULL;
    OPS_STATE	*global = aq->opt_global;
    QEN_NODE	*qen;
    QEF_AHD	*ahd;

    ret_aq = (OPT_AHDQEN *) opu_Gsmemory_get(global, sizeof (OPT_AHDQEN));
    ret_aq->opt_global = global;
    ret_aq->opt_parent = aq;
    ret_aq->opt_next = NULL;

    switch (aq->opt_aq_type)
    {
     case OPT_AQ_AHD:
	ahd = aq->opt_aq_data.opt_pahd;
	switch (ahd->ahd_atype)
	{
	 case QEA_DMU:
	 case QEA_UTL:
	 case QEA_RUP:
	 case QEA_RDEL:
	 case QEA_COMMIT:
	 case QEA_ROLLBACK:
	 case QEA_RETURN:
	 case QEA_MESSAGE:
	 case QEA_EMESSAGE:
	 case QEA_D0_NIL:
	 case QEA_D1_QRY:
	 case QEA_D2_GET:
	 case QEA_D3_XFR:
	 case QEA_D4_LNK:
	 case QEA_D5_DEF:
	 case QEA_D6_AGG:
	 case QEA_D7_OPN:
	 case QEA_D8_CRE:
	 case QEA_D9_UPD:
	 case QEA_D10_REGPROC:
	 default:
	    ret_aq = NULL;
	    break;

	 case QEA_APP:
	 case QEA_SAGG:
	 case QEA_PROJ:
	 case QEA_AGGF:
	 case QEA_UPD:
	 case QEA_DEL:
	 case QEA_GET:
	 case QEA_FOR:
	 case QEA_RETROW:
	 case QEA_RAGGF:
	 case QEA_HAGGF:
	 case QEA_LOAD:
	 case QEA_PUPD:
	 case QEA_PDEL:
	    ret_aq->opt_aq_type = OPT_AQ_QEN;
	    ret_aq->opt_aq_data.opt_pqen = ahd->qhd_obj.qhd_qep.ahd_qep;

	    if (ret_aq->opt_aq_data.opt_pqen == NULL)
	    {
		ret_aq = NULL;
	    }
	    break;

	 case QEA_IF:
	    ret_aq->opt_aq_type = OPT_AQ_AHD;
	    ret_aq->opt_aq_data.opt_pahd = ahd->qhd_obj.qhd_if.ahd_true;

	    if (ret_aq->opt_aq_data.opt_pahd == NULL)
	    {
		ret_aq = NULL;
	    }
	    break;
	}
	break;

     case OPT_AQ_QEN:
	qen = aq->opt_aq_data.opt_pqen;
	ret_aq->opt_aq_type = OPT_AQ_QEN;
	switch (qen->qen_type)
	{
	 case QE_TJOIN:
	    ret_aq->opt_aq_data.opt_pqen = qen->node_qen.qen_tjoin.tjoin_out;
	    break;

	 case QE_KJOIN:
	    ret_aq->opt_aq_data.opt_pqen = qen->node_qen.qen_kjoin.kjoin_out;
	    break;

	 case QE_HJOIN:
	    ret_aq->opt_aq_data.opt_pqen = qen->node_qen.qen_hjoin.hjn_out;
	    break;

	 case QE_EXCHANGE:
	    ret_aq->opt_aq_data.opt_pqen = qen->node_qen.qen_exch.exch_out;
	    break;

	 case QE_SEJOIN:
	    ret_aq->opt_aq_data.opt_pqen = qen->node_qen.qen_sejoin.sejn_out;
	    break;

	 case QE_FSMJOIN:
	 case QE_ISJOIN:
	 case QE_CPJOIN:
	    ret_aq->opt_aq_data.opt_pqen = qen->node_qen.qen_sjoin.sjn_out;
	    break;

	 case QE_SORT:
	    ret_aq->opt_aq_data.opt_pqen = qen->node_qen.qen_sort.sort_out;
	    break;

	 case QE_TSORT:
	    ret_aq->opt_aq_data.opt_pqen = qen->node_qen.qen_tsort.tsort_out;
	    break;

	 case QE_QP:
	    ret_aq->opt_aq_type = OPT_AQ_AHD;
	    ret_aq->opt_aq_data.opt_pahd = qen->node_qen.qen_qp.qp_act;
	    break;

	 default:
	    ret_aq = NULL;
	    break;
	}
	break;
    }

    if (ret_aq != NULL && opt_parent_seen(global, ret_aq) == TRUE)
    {
	ret_aq = NULL;
    }

    return((PTR) ret_aq);
}

/*{
** Name: OPT_RAHDQEN	- Return the right of an AHDQEN tree.
**
** Description:
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      28-oct-86 (eric)
**          written
**	11-may-99 (inkdo01)
**	    Support for hash joins.
[@history_template@]...
*/
static PTR
opt_rahdqen(
	PTR	p )
{
    OPT_AHDQEN	*aq = (OPT_AHDQEN *) p;
    OPT_AHDQEN	*ret_aq = NULL;
    OPS_STATE	*global = aq->opt_global;
    QEN_NODE	*qen;
    QEF_AHD	*ahd;

    ret_aq = (OPT_AHDQEN *) opu_Gsmemory_get(global, sizeof (OPT_AHDQEN));
    ret_aq->opt_global = global;
    ret_aq->opt_parent = aq;
    ret_aq->opt_next = NULL;

    switch (aq->opt_aq_type)
    {
     case OPT_AQ_AHD:
	ahd = aq->opt_aq_data.opt_pahd;
	ret_aq->opt_aq_type = OPT_AQ_AHD;

	if (ahd->ahd_atype == QEA_IF)
	{
	    ret_aq->opt_aq_data.opt_pahd = ahd->qhd_obj.qhd_if.ahd_false;
	}
	else
	{
	    ret_aq->opt_aq_data.opt_pahd = ahd->ahd_next;
	}

	if (ret_aq->opt_aq_data.opt_pahd == NULL)
	{
	    ret_aq = NULL;
	}
	break;

     case OPT_AQ_QEN:
	qen = aq->opt_aq_data.opt_pqen;
	ret_aq->opt_aq_type = OPT_AQ_QEN;
	switch (qen->qen_type)
	{
	 case QE_SEJOIN:
	    ret_aq->opt_aq_data.opt_pqen = qen->node_qen.qen_sejoin.sejn_inner;
	    break;

	 case QE_HJOIN:
	    ret_aq->opt_aq_data.opt_pqen = qen->node_qen.qen_hjoin.hjn_inner;
	    break;

	 case QE_FSMJOIN:
	 case QE_ISJOIN:
	 case QE_CPJOIN:
	    ret_aq->opt_aq_data.opt_pqen = qen->node_qen.qen_sjoin.sjn_inner;
	    break;

	 default:
	    ret_aq = NULL;
	    break;
	}
	break;
    }

    if (ret_aq != NULL && opt_parent_seen(global, ret_aq) == TRUE)
    {
	ret_aq = NULL;
    }

    return((PTR) ret_aq);
}

/*{
** Name: OPT_PRAHDQEN	- Print an AHDQEN tree node.
**
** Description:
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      28-oct-86 (eric)
**          written
**	09-jan-91 (stec)
**	    Added code to print out new action types.
**	10-Feb-2004 (schka24)
**	    Table drive qef action header lookup.
**	20-Sep-2004 (jenjo02)
**	    Added parenthetical child count to "EXCH".
[@history_template@]...
*/
static VOID
opt_prahdqen(
	PTR	p,
	PTR	control )
{
    char	*actstr;
    OPT_AHDQEN	*aq = (OPT_AHDQEN *) p;
    OPS_STATE	*global = aq->opt_global;
    QEN_NODE	*qen;
    QEF_AHD	*ahd;

    switch (aq->opt_aq_type)
    {
     case OPT_AQ_AHD:
	ahd = aq->opt_aq_data.opt_pahd;
	actstr = uld_qef_actstr(ahd->ahd_atype);
	uld_prnode(control, actstr);
	if (ahd->ahd_atype == QEA_DMU)
	{
	    switch (ahd->qhd_obj.qhd_dmu.ahd_func)
	    {
	     case DMU_CREATE_TABLE:
		uld_prnode(control, "UCRT");
		break;

	     case DMU_MODIFY_TABLE:
		uld_prnode(control, "UMOD");
		break;

	     case DMT_CREATE_TEMP:
		uld_prnode(control, "TCRT");
		break;

	     default:
		uld_prnode(control, "????");
		break;
	    }
	}	    
	break;

     case OPT_AQ_QEN:
	qen = aq->opt_aq_data.opt_pqen;
	TRformat(uld_tr_callback, (i4 *)0,
	    (char *)&global->ops_trace.opt_trformat[0],
	    (i4)sizeof(global->ops_trace.opt_trformat),
	    "|%2d|", qen->qen_num);
        uld_prnode(control, &global->ops_trace.opt_trformat[0]);
	switch (qen->qen_type)
	{
	 case QE_TJOIN:
	    uld_prnode(control, "TJN ");
	    break;

	 case QE_KJOIN:
	    uld_prnode(control, "KJN ");
	    break;

	 case QE_HJOIN:
	    uld_prnode(control, "HJN ");
	    break;

	 case QE_EXCHANGE:
	    TRformat(uld_tr_callback, (i4 *)0,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		"EXCH(%d)", qen->node_qen.qen_exch.exch_ccount);
	    uld_prnode(control, &global->ops_trace.opt_trformat[0]);
	    break;

	 case QE_SEJOIN:
	    uld_prnode(control, "SEJOIN");
	    break;

	 case QE_FSMJOIN:
	    uld_prnode(control, "FSMJOIN");
	    break;

	 case QE_ISJOIN:
	    uld_prnode(control, "ISJOIN");
	    break;

	 case QE_CPJOIN:
	    uld_prnode(control, "CPJOIN");
	    break;

	 case QE_ORIG:
	    uld_prnode(control, "ORIG");
	    break;

	 case QE_TPROC:
	    uld_prnode(control, "TPROC");
	    break;

	 case QE_SORT:
	    uld_prnode(control, "SORT");
	    break;

	 case QE_TSORT:
	    uld_prnode(control, "TSRT");
	    break;

	 case QE_QP:
	    uld_prnode(control, "QP  ");
	    break;

	 default:
	    uld_prnode(control, "BAD ");
	    break;
	}
	break;
    }
}

/*{
** Name: OPT_PART_DEF	- print a DB_PART_DEF struct
**
** Description: format the contents of a DB_PART_DEF structure to
**	describe the partitioning scheme for a table
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      20-jan-04 (inkdo01)
**	    Written for table partitioning.
*/
static VOID
opt_part_def(
	OPS_STATE	*global,
	DB_PART_DEF	*partdp)
{
    char dim_title[20];
    i4	    i, j;

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	OPT_PBLEN, "(ahd/dmu)_part_def: 0x%p\n\n", partdp);

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"nphys_parts: %5d\tndims: %5d\tpart_flags: 0x%4x\n\n",
	partdp->nphys_parts, partdp->ndims, partdp->part_flags);

    for (i = 0; i < partdp->ndims; i++)
    {
	DB_PART_DIM	*dimp = &partdp->dimension[i];

	STprintf(dim_title, "dimension[%2d]", i);
	opt_part_dim(global, dimp, dim_title);
    }

}


/*{
** Name: OPT_PART_DIM	- print a DB_PART_DIM struct
**
** Description: format the contents of a DB_PART_DIM structure to
**	describe one dimension of the partitioning scheme for a table
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      21-jan-04 (inkdo01)
**	    Written for table partitioning.
*/
static VOID
opt_part_dim(
	OPS_STATE	*global,
	DB_PART_DIM	*dimp,
	char		*title)
{
    DB_PART_BREAKS *pbreakp;
    DB_PART_LIST *plistp;
    i4	    i, j;


    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"%s: distrule: %2d\tstride: %5d\tnparts: %5d\n\n",
	title, dimp->distrule, dimp->stride, dimp->nparts);
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"\tncols: %3d\tnbreaks: %3d\tvalue_size: %5d\n\n",
	dimp->ncols, dimp->nbreaks, dimp->value_size);

    plistp = dimp->part_list;
    if (plistp == NULL)
	return;

    for (i = 0; i < dimp->ncols; i++)
    {
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	    "    part[%2d]: type: %5d\tlength: %5d\tprecision: %5d\n\n",
	    i, plistp->type, plistp->length, plistp->precision);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	    "\tatt_number: %5d\tval_offset: %5d\trow_offset: %5d\n\n",
	    plistp->att_number, plistp->val_offset, plistp->row_offset);
	plistp++;
    }

    pbreakp = dimp->part_breaks;
    if (pbreakp == NULL)
	return;
    for (i = 0; i < dimp->nbreaks; i++)
    {
	TRformat(opt_scc, NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
		"    break[%3d]: oper: %5w  partseq: %5d  val_base: 0x%p\n\n",
		i, "LT,LTEQ,EQ,xx,GTEQ,GT,DFLT",pbreakp->oper,
		pbreakp->partseq, pbreakp->val_base);
	if (pbreakp->val_text != NULL)
	{
	    DB_TEXT_STRING **textp = pbreakp->val_text;

	    for (j = 0; j < dimp->ncols; j++)
	    {
		TRformat(opt_scc, NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
			"\t text[%d]: %t\n\n",
			j, (*textp)->db_t_count, (*textp)->db_t_text);
		++textp;
	    }
	}
	++pbreakp;
    }
}


/*{
** Name: OPT_AHD	- Print out a QEF_AHD except for any QEN_NODE's.
**
** Description:
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      5-sept-86 (eric)
**          written
**	04-feb-91 (neil)
**	    Alerters: Trace some event actions.
**	29-jun-90 (stec)
**	    Modified code to fix ahd_proj display problem.
**	10-dec-90 (neil)
**	    Alerters: Trace some event actions.
**      04-feb-91 (neil)
**          Alerters: Trace some event actions.
**	17-may-93 (jhahn)
**	    Added code to print out rule action lists.
**      19-jun-95 (inkdo01)
**          Replaced ahd_any by ahd_qepflag
**	01-oct-98 (inkdo01)
**	    upd_colmap print out.
**	3-mar-00 (inkdo01)
**	    Print ahd_firstn ("select first n ..." support).
**	6-feb-01 (inkdo01)
**	    Print new hash aggregate stuff.
**	15-mar-02 (inkdo01)
**	    Print sequence descriptions.
**	1-Dec-2004 (schka24)
**	    Fix callproc and iproc to use %t not %s, the proc names aren't
**	    null terminated and op150 segv's.  I thought I fixed this, but
**	    maybe it was somewhere else.
**	12-july-05 (inkdo01)
**	    Look for already printed action hdrs (to untangle IFs).
**	20-june-06 (dougi)
**	    Added support for BEFORE triggers and procedure parameter modes.
**	24-Oct-2006 (kschendel) b122118
**	    action display out of date, use uld lookup instead.
**	18-july-2007 (dougi)
**	    Display ahd_offsetn (result offset clause).
**      14-feb-08 (rapma01)
**          SIR 119650
**          Removed qeq_g1_firstn since first n is not currently
**          supported in star
**	5-Aug-2009 (kschendel) SIR 122512
**	    Update qep flag printing, print for-depth.
*/
static VOID
opt_ahd(
	OPS_STATE	*global,
	OPT_AHDQEN	*aq,
	OPT_AHDQEN	*root_aq,
	char		*label,
	i4		ahd_no )
{
    char		*actstr;
    QEF_AHD		*ahd;
    QEF_VALID		*vl;
    i4			i;
    RDR_INFO		*rel;
    QEN_NODE		*qn;
    OPT_AHDQEN		*next_aq;
    OPT_AHDQEN		*prev_aq;

    if (aq == NULL || aq->opt_aq_type != OPT_AQ_AHD || 
	    aq->opt_aq_data.opt_pahd == NULL || 
	    opt_parent_seen(global, aq) == TRUE
	)
    {
	return;
    }

    ahd = aq->opt_aq_data.opt_pahd;

    if (ahd_no > 1)
    {
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "");
    }
    actstr = uld_qef_actstr(ahd->ahd_atype);
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"%s QEF_AHD: #%3d   addr: %p,  ahd_atype: %s (%d)\n\n",
	label, ahd_no, ahd, actstr, ahd->ahd_atype);

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"\tahd_flags: 0x%x, ahd_length: %d, ahd_next: %p\n\n", ahd->ahd_flags, 
	ahd->ahd_length, ahd->ahd_next);

    if (ahd->ahd_valid == NULL) TRformat(opt_scc, (i4*)NULL, 
        global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"ahd_valid list: no entries\n\n");
      else TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, 
        OPT_PBLEN, "ahd_valid list: 0x%p\n\n", ahd->ahd_valid);

    for (i = 0, vl = ahd->ahd_valid; vl != NULL; vl = vl->vl_next, i += 1)
    {
	rel = (RDR_INFO *) vl->vl_debug;

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "[%2d] vl_tab_id.relname: %24s  .owner: %24s\n\n",
	    i, rel->rdr_rel->tbl_name.db_tab_name,
	    rel->rdr_rel->tbl_owner.db_own_name);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\tdmr_cb: %5d \t dmf_cb: %5d \t rw: ",
	    vl->vl_dmr_cb, vl->vl_dmf_cb);
	switch (vl->vl_rw)
	{
	 case DMT_A_READ:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "DMT_A_READ\n\n");
	    break;

	 case DMT_A_WRITE:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "DMT_A_WRITE\n\n");
	    break;

	 case DMT_A_RONLY:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "DMT_A_RONLY\n\n");
	    break;

	 default:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "Unknown read-write mode\n\n");
	    break;
	}
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN,
	    "\test_pages: %5d \t total_pages %5d \t size_sensitive %5d\n\n",
	    vl->vl_est_pages, vl->vl_total_pages, 
	    vl->vl_size_sensitive);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\t.base: %5d \t .index: %5d\n\n",
	    vl->vl_tab_id.db_tab_base, vl->vl_tab_id.db_tab_index);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\ttab_id_index: %5d\tpartition count: %5d\n\n\n",
	    vl->vl_tab_id_index, vl->vl_partition_cnt);
    }

    opt_adf(global, ahd->ahd_mkey, "ahd_mkey");

    switch (ahd->ahd_atype)
    {
    
     case QEA_APP:
     case QEA_UPD:
     case QEA_DEL:
     case QEA_PUPD:
     case QEA_PDEL:
     case QEA_RUP:
     case QEA_RDEL:
     case QEA_GET:
     case QEA_FOR:
     case QEA_RETROW:
     case QEA_SAGG:
     case QEA_AGGF:
     case QEA_PROJ:
     case QEA_RAGGF:
     case QEA_HAGGF:
     case QEA_LOAD: 
        TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "ahd_qep_cnt: %5d \t ahd_odmr_cb: %5d\t ahd_dmtix: %5d\n\n", 
	    ahd->qhd_obj.qhd_qep.ahd_qep_cnt, 
	    ahd->qhd_obj.qhd_qep.ahd_odmr_cb,
	    ahd->qhd_obj.qhd_qep.ahd_dmtix);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "ahd_tidrow: %5d \t ahd_tidoffset: %5d\n\n", 
	    ahd->qhd_obj.qhd_qep.ahd_tidrow,
	    ahd->qhd_obj.qhd_qep.ahd_tidoffset);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "ahd_reprow: %5d \t ahd_repsize: %5d\n\n", 
	    ahd->qhd_obj.qhd_qep.ahd_reprow,
	    ahd->qhd_obj.qhd_qep.ahd_repsize);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
  	    OPT_PBLEN, "ahd_for_depth: %5d \t ahd_proj: %5d \t ahd_duphandle: %w\n\n",
	    ahd->qhd_obj.qhd_qep.ahd_for_depth,
	    ahd->qhd_obj.qhd_qep.ahd_proj,
	    "SKIP_DUP,ERR_DUP",
	    ahd->qhd_obj.qhd_qep.ahd_duphandle);
	TRformat(opt_scc, (i4 *)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "ahd_qepflag: 0x%x: %v\n\n",
	    ahd->qhd_obj.qhd_qep.ahd_qepflag,
	    "ANY,CSTAROPT,NOROWS,FOR-GET,0x10,PCOLS_UPDATE,4BYTE_TIDP,\
SCROLL,KEYSET,PARM_FIRSTN,PARM_OFFSETN,LOAD_CTAS,PART_SEPARATE,HAS-HASHOP,MAIN",
	    ahd->qhd_obj.qhd_qep.ahd_qepflag);

	if (ahd->qhd_obj.qhd_qep.ahd_part_def != (DB_PART_DEF *) NULL)
	    opt_part_def(global, ahd->qhd_obj.qhd_qep.ahd_part_def);

	if (ahd->ahd_atype == QEA_GET && 
		(ahd->qhd_obj.qhd_qep.ahd_qepflag & AHD_SCROLL))
	{
	    /* Scrollable cursor - display the ahd_scroll->stuff. */
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	      OPT_PBLEN, "ahd_rsdmtix:%5d\tahd_rswdmrix:%5d\tahd_rsrdmrix:%5d\n\n",
	      ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsdmtix,
	      ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rswdmrix,
	      ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsrdmrix);

	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	      OPT_PBLEN, "ahd_rsrsize:%5d\tahd_rspsize:%5d\n\n",
	      ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsrsize,
	      ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rspsize);

	    if (ahd->qhd_obj.qhd_qep.ahd_qepflag & AHD_KEYSET)
	    {
		/* KEYSET stuff, too. */
		DMF_ATTR_ENTRY	*attp;
		i4		*off1, *off2;

		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "Keyset - %2d entries:\n\n",
		    ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rskscnt);

		for (i = 0, attp = ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rskattr,
			off1 = ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rskoff1,
			off2 = ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rskoff2;
		    i < ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rskscnt; 
		    i++, attp++, off1++, off2++)
		{
		    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		     OPT_PBLEN,
		     "[%2d] type: %d, size: %d, prec: %d, flags: %d, off1:%d, off2:%d\n\n",
			i, attp->attr_type, attp->attr_size, 
			attp->attr_precision, attp->attr_flags_mask, *off1, *off2);
		}

		opt_adf(global, ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rskcurr,
								"ahd_rskcurr");
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	     	  OPT_PBLEN, "ahd_rsbtrow:%5d\tahd_rsbtdmr:%5d\tahd_rsrrow:%5d\n\n",
		    ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsbtrow,
		    ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsbtdmrix,
		    ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsrrow);
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	     	  OPT_PBLEN, "ahd_tidrow:%5d\tahd_tidoffset:%5d\n\n",
		    ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_tidrow,
		    ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_tidoffset);
	    }
	}

	if (ahd->ahd_atype == QEA_HAGGF)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	      OPT_PBLEN, "ahd_agwbuf: %5d \t ahd_aghbuf: %5d\n\n",
	      ahd->qhd_obj.qhd_qep.u1.s2.ahd_agwbuf,
	      ahd->qhd_obj.qhd_qep.u1.s2.ahd_aghbuf);

	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	      OPT_PBLEN, "ahd_agobuf: %5d \t ahd_agowbuf: %5d \t ahd_agdbmcb: %5d\n\n",
	      ahd->qhd_obj.qhd_qep.u1.s2.ahd_agobuf,
	      ahd->qhd_obj.qhd_qep.u1.s2.ahd_agowbuf,
	      ahd->qhd_obj.qhd_qep.u1.s2.ahd_agdmhcb);

	    /* Print bycount separately since it doesn't show in the
	    ** concatenated cmplist that we print below.
	    */
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	      OPT_PBLEN, "ahd_aggest: %d \tahd_agcbix:%5d  ahd_bycount:%d\n\n",
	      ahd->qhd_obj.qhd_qep.u1.s2.ahd_aggest,
	      ahd->qhd_obj.qhd_qep.u1.s2.ahd_agcbix,
	      ahd->qhd_obj.qhd_qep.u1.s2.ahd_bycount);

	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	      OPT_PBLEN, "\n\n");
	    opt_satts(global, 0, (DMF_ATTR_ENTRY **) NULL, 1,
		(DMT_KEY_ENTRY **) NULL, ahd->qhd_obj.qhd_qep.u1.s2.ahd_aghcmp);
	}
	else
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	      OPT_PBLEN, "ahd_a_row: %5d \t ahd_b_row: %5d\n\n",
	      ahd->qhd_obj.qhd_qep.u1.s1.ahd_a_row,
	      ahd->qhd_obj.qhd_qep.u1.s1.ahd_b_row);

	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	      OPT_PBLEN, "ahd_a_tid: %5d \t ahd_b_tid: %5d\n\n",
	      ahd->qhd_obj.qhd_qep.u1.s1.ahd_a_tid,
	      ahd->qhd_obj.qhd_qep.u1.s1.ahd_b_tid);
	
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	      OPT_PBLEN, "ahd_failed_a_temptable: %p \t ahd_failed_b_temptable: %p\n\n",
	      ahd->qhd_obj.qhd_qep.u1.s1.ahd_failed_a_temptable,
	      ahd->qhd_obj.qhd_qep.u1.s1.ahd_failed_b_temptable);
	}
	
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "ahd_offsetn: %5d\tahd_firstn: %5d\t ahd_firstncb: %5d\t ahd_seq: %p\n\n",
	    ahd->qhd_obj.qhd_qep.ahd_offsetn,
	    ahd->qhd_obj.qhd_qep.ahd_firstn,
	    ahd->qhd_obj.qhd_qep.ahd_firstncb,
	    ahd->qhd_obj.qhd_qep.ahd_seq);
	
	opt_adf(global, ahd->qhd_obj.qhd_qep.ahd_current, "ahd_current");
	opt_adf(global, ahd->qhd_obj.qhd_qep.ahd_constant, "ahd_constant");
	opt_adf(global, ahd->qhd_obj.qhd_qep.ahd_by_comp, "ahd_by_comp");
	if (ahd->ahd_atype == QEA_HAGGF)
	    opt_adf(global, ahd->qhd_obj.qhd_qep.u1.s2.ahd_agoflow, 
							"ahd_agoflow");

	if (ahd->qhd_obj.qhd_qep.ahd_upd_colmap)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "ahd_upd_colmap (first 2 words): %x, %x\n\n",
		ahd->qhd_obj.qhd_qep.ahd_upd_colmap->db_domset[0],
		ahd->qhd_obj.qhd_qep.ahd_upd_colmap->db_domset[1]);
	}

	if (ahd->qhd_obj.qhd_qep.ahd_seq != NULL)
	{
	    QEF_SEQ_LINK	*linkp;
	    QEF_SEQUENCE	*seqp;

	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\n\n");
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "Sequences: \n\n");

	    for (linkp = ahd->qhd_obj.qhd_qep.ahd_seq; linkp;
					linkp = linkp->qs_ahdnext)
	    {
		seqp = linkp->qs_qsptr;
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t\tseqname: %24s  .owner: %24s\n\n",
		    seqp->qs_seqname.db_name,
		    seqp->qs_owner.db_own_name);
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t\tseqID: (%5d, %5d)\n\n",
		    seqp->qs_id.db_tab_base, seqp->qs_id.db_tab_index);
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t\tdms_cb: %5d, rownum: %5d, qs_flag: %5d\n\n",
		    seqp->qs_cbnum, seqp->qs_rownum, seqp->qs_flag);
	    }
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\n\n");
	}

	if (ahd->qhd_obj.qhd_qep.ahd_qep != NULL)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "ahd_postfix:");
	    for (qn = ahd->qhd_obj.qhd_qep.ahd_postfix; 
		    qn != NULL; 
		    qn = qn->qen_postfix
		)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, " %5d", qn->qen_num);
	    }
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\n\n");

	    /* print out the tree structure of the qen tree; */
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "ahd_qep tree:\n\n");
	    uld_prtree((PTR) ahd->qhd_obj.qhd_qep.ahd_qep, opt_prqep, 
		    opt_lqep, opt_rqep, 4, 2, DB_OPF_ID);

	    /* print out the contents of the qen tree; */
	    opt_qenode(global, ahd->qhd_obj.qhd_qep.ahd_qep, aq);
	}
	if (ahd->qhd_obj.qhd_qep.ahd_before_act != NULL)
	{
	    next_aq =
		(OPT_AHDQEN *) opu_Gsmemory_get(global, sizeof (OPT_AHDQEN));
	    next_aq->opt_global = global;
	    next_aq->opt_aq_type = OPT_AQ_AHD;
	    next_aq->opt_parent = aq;
	    next_aq->opt_next = NULL;
	    prev_aq = opt_lastaq(root_aq);
	    prev_aq->opt_next = next_aq;
	    next_aq->opt_qps = NULL;
	    next_aq->opt_qpsp = aq->opt_qpsp;
	    next_aq->opt_aq_data.opt_pahd = ahd->qhd_obj.qhd_qep.ahd_before_act;
	    opt_ahd(global, next_aq, root_aq, "Before Action", ahd_no + 1);
	}
	if (ahd->qhd_obj.qhd_qep.ahd_after_act != NULL)
	{
	    next_aq =
		(OPT_AHDQEN *) opu_Gsmemory_get(global, sizeof (OPT_AHDQEN));
	    next_aq->opt_global = global;
	    next_aq->opt_aq_type = OPT_AQ_AHD;
	    next_aq->opt_parent = aq;
	    next_aq->opt_next = NULL;
	    prev_aq = opt_lastaq(root_aq);
	    prev_aq->opt_next = next_aq;
	    next_aq->opt_qps = NULL;
	    next_aq->opt_qpsp = aq->opt_qpsp;
	    next_aq->opt_aq_data.opt_pahd = ahd->qhd_obj.qhd_qep.ahd_after_act;
	    opt_ahd(global, next_aq, root_aq, "After Action", ahd_no + 1);
	}
	break;

    case QEA_EXEC_IMM:
	{
	    QEF_EXEC_IMM	*eximm = &ahd->qhd_obj.qhd_execImm;

	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "pst_basemode: %d, pst_execflags: %d\n\n",
		eximm->ahd_info->pst_basemode, eximm->ahd_info->pst_execflags);
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "execimm_text: %s\n\n",
		eximm->ahd_text->db_t_text);
	}
	break;

    case QEA_CREATE_INTEGRITY:
	{
	    QEF_CREATE_INTEGRITY_STATEMENT	*cint;

	    cint = &ahd->qhd_obj.qhd_createIntegrity;
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "cons_tabname: %24s\t cons_owner: %24s\t cons_flags: %x\n\n",
		cint->qci_cons_tabname.db_tab_name,
		cint->qci_cons_ownname.db_own_name,
		cint->qci_flags);
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "ref_tabname: %24s \t ref_owner: %24s\n\n",
		cint->qci_ref_tabname.db_tab_name,
		cint->qci_ref_ownname.db_own_name);
	    if (cint->qci_integrityQueryText &&
		cint->qci_integrityQueryText->db_t_count)
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN,
		"cons_qrytext: %#s\n\n",
		cint->qci_integrityQueryText->db_t_count,
		&cint->qci_integrityQueryText->db_t_text);
	    if (cint->qci_checkRuleText && cint->qci_checkRuleText->db_t_count)
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN,
		"cons_ruletext: %#s\n\n",
		cint->qci_checkRuleText->db_t_count,
		&cint->qci_checkRuleText->db_t_text);
	    if (cint->qci_flags & QCI_INDEX_WITH_OPTS)
	    {
		if (cint->qci_flags & (QCI_NEW_NAMED_INDEX |
			QCI_OLD_NAMED_INDEX))
	    	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "cons_index_name: %24s\n\n",
		cint->qci_idxname.db_tab_name);
		if (cint->qci_flags & QCI_OLD_NAMED_INDEX) break;
	        if (cint->qci_idxloc.data_in_size)
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN,
		"cons_idx_location: %#s\n\n",
		cint->qci_idxloc.data_in_size,
		cint->qci_idxloc.data_address);
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN,
		"idx_fillfac: %d, idx_leaff: %d, idx_nonleaff: %d\n\n",
		cint->qci_idx_fillfac, cint->qci_idx_leaff, 
		cint->qci_idx_nonleaff);
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN,
		"idx_page_size: %d, idx_minpgs: %d, idx_maxpgs: %d\n\n",
		cint->qci_idx_page_size, cint->qci_idx_minpgs,
		cint->qci_idx_maxpgs);
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN,
		"idx_alloc: %d, idx_extend: %d, idx_struct: %d\n\n",
		cint->qci_idx_alloc, cint->qci_idx_extend,
		cint->qci_idx_struct);
	    }
	    break;
	}

    case QEA_CREATE_TABLE:
	{
	    DMU_CB		*dmucb;
	    QEU_CB		*qeucb;

	    qeucb = (QEU_CB *) ahd->qhd_obj.qhd_DDLstatement.ahd_qeuCB;
	    dmucb = (DMU_CB *) qeucb->qeu_d_cb;
	    opt_dmucb(global, dmucb);
	    opt_lpchar_list(global, qeucb->qeu_logpart_list);

	    break;
	 }

     case QEA_DMU:
	switch (ahd->qhd_obj.qhd_dmu.ahd_func)
	{
	 case DMU_CREATE_TABLE:
	 case DMU_MODIFY_TABLE:
	 {
	    DMU_CB		*dmucb;

	    if (ahd->qhd_obj.qhd_dmu.ahd_func == DMU_CREATE_TABLE)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "ahd_func: DMU_CREATE_TABLE, ahd_alt: %p\n\n",
		    ahd->qhd_obj.qhd_dmu.ahd_alt);
	    }
	    else
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN,"ahd_func: DMU_MODIFY_TABLE, ahd_alt: %p\n\n",
		    ahd->qhd_obj.qhd_dmu.ahd_alt);
	    }
	    dmucb = (DMU_CB *) ahd->qhd_obj.qhd_dmu.ahd_cb;
	    opt_dmucb(global, dmucb);
	    opt_lpchar_list(global, ahd->qhd_obj.qhd_dmu.ahd_logpart_list);
	    break;
	 }

	 case DMT_CREATE_TEMP:
	 {
	    DMT_CB		*dmtcb;
	    DMF_ATTR_ENTRY	**atts;
	    DMF_ATTR_ENTRY	*att;
	    DMT_KEY_ENTRY	**keys;
	    DMT_KEY_ENTRY	*key;

	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "ahd_func: DMT_CREATE_TEMP, ahd_alt: %p\n\n",
		ahd->qhd_obj.qhd_dmu.ahd_alt);
	    dmtcb = (DMT_CB *) ahd->qhd_obj.qhd_dmu.ahd_cb;
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN,
		"dmt_table: %#s, dmt_location: %#s, dmt_flags_mask: %x\n\n",
		sizeof (dmtcb->dmt_table.db_tab_name),
		dmtcb->dmt_table.db_tab_name,
		dmtcb->dmt_location.data_in_size,
		dmtcb->dmt_location.data_address,
		dmtcb->dmt_flags_mask);
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "dmt_attr_array: in_count: %x, size: %x\n\n",
		dmtcb->dmt_attr_array.ptr_in_count,
		dmtcb->dmt_attr_array.ptr_size);
	    atts = (DMF_ATTR_ENTRY **) dmtcb->dmt_attr_array.ptr_address;
	    for (i = 0; i < dmtcb->dmt_attr_array.ptr_in_count; i += 1)
	    {
		att = atts[i];
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		     OPT_PBLEN,
		"[%2d] name: %#s, type: %x, size: %x, prec: %x, flags: %x\n\n",
		    i, sizeof(att->attr_name.db_att_name), 
		    att->attr_name.db_att_name, att->attr_type, 
		    att->attr_size, att->attr_precision, 
		    att->attr_flags_mask);
	    }

	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "dmt_key_array: in_count: %x, size: %x\n\n",
		dmtcb->dmt_key_array.ptr_in_count,
		dmtcb->dmt_key_array.ptr_size);
	    keys = (DMT_KEY_ENTRY **) dmtcb->dmt_key_array.ptr_address;
	    for (i = 0; i < dmtcb->dmt_key_array.ptr_in_count; i += 1)
	    {
		key = keys[i];
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "[%2d] name: %#s, order: %w\n\n", i,
		    sizeof(key->key_attr_name.db_att_name), 
		    key->key_attr_name.db_att_name,
		    "junk,ASCENDING,DESCENDING", key->key_order);
	    }
	 }
	 break;
	}
	break;

     case QEA_CREATE_VIEW:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN,"View name: %~t, flags: 0x%4x\n\n",
		sizeof(ahd->qhd_obj.qhd_create_view.ahd_view_name),
		&ahd->qhd_obj.qhd_create_view.ahd_view_name,
		ahd->qhd_obj.qhd_create_view.ahd_crt_view_flags);
	/* FIXME need to print other stuff here */
	break;

     case QEA_INVOKE_RULE:
	next_aq = (OPT_AHDQEN *) opu_Gsmemory_get(global, sizeof (OPT_AHDQEN));
	next_aq->opt_global = global;
	next_aq->opt_aq_type = OPT_AQ_AHD;
	next_aq->opt_parent = aq;
	next_aq->opt_next = NULL;
	prev_aq = opt_lastaq(root_aq);
	prev_aq->opt_next = next_aq;
	next_aq->opt_qps = NULL;
	next_aq->opt_qpsp = aq->opt_qpsp;

	if (ahd->qhd_obj.qhd_invoke_rule.ahd_rule_action_list != NULL)
	{
	    next_aq->opt_aq_data.opt_pahd = 
                 ahd->qhd_obj.qhd_invoke_rule.ahd_rule_action_list;
	    opt_ahd(global, next_aq, root_aq, "rule list", ahd_no + 1);
	}
	break;

     case QEA_CALLPROC:
        TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	 OPT_PBLEN, "rpname:%~t, ahd_procid: %d \t ahd_parm_cnt: %5d \t ttabno: %5d   ahd_flags: %3d\n\n", 
            sizeof(ahd->qhd_obj.qhd_callproc.ahd_rulename),
            &ahd->qhd_obj.qhd_callproc.ahd_rulename,
	    ahd->qhd_obj.qhd_callproc.ahd_procedureID.db_tab_base,
	    ahd->qhd_obj.qhd_callproc.ahd_pcount,
            (ahd->qhd_obj.qhd_callproc.ahd_proc_temptable) ?
            ahd->qhd_obj.qhd_callproc.ahd_proc_temptable->ttb_tempTableIndex :
              -1,      /* temptabno or -1 if none */
	    ahd->qhd_obj.qhd_callproc.ahd_flags);
	if (ahd->qhd_obj.qhd_callproc.ahd_gttid.db_tab_base)
          TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "global ttabid: %d\n\n",
            ahd->qhd_obj.qhd_callproc.ahd_gttid.db_tab_base); 

	/* Dump actual parameters. */
	for (i = 0; i < ahd->qhd_obj.qhd_callproc.ahd_pcount; i++)
	{
	    QEF_CP_PARAM	*aparm;

	    aparm = ahd->qhd_obj.qhd_callproc.ahd_params + i;
            TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  ahd_params[%d]: parm_name: %16s, parm_nlen: %d, parm_index: %d, parm_flags: %d\n\n",
        	i, aparm->ahd_parm.parm_name, aparm->ahd_parm.parm_nlen,
		aparm->ahd_parm.parm_index, aparm->ahd_parm.parm_flags);
            TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\tparm_row: %d, parm_offset: %d\n\n",
        	aparm->ahd_parm_row, aparm->ahd_parm_offset);
	}
	opt_adf(global, ahd->qhd_obj.qhd_callproc.ahd_procparams,              
                   "ahd_parambuild");
	opt_adf(global, ahd->qhd_obj.qhd_callproc.ahd_procp_return,            
                   "ahd_paramreturn");
	break;

     case QEA_PROC_INSERT:
        TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	  OPT_PBLEN, "ahd_temptab: %5d, ahd_tuple: %5d, ahd_noats: %5d\n\n",
	   ahd->qhd_obj.qhd_proc_insert.ahd_proc_temptable->ttb_tempTableIndex,
           ahd->qhd_obj.qhd_proc_insert.ahd_proc_temptable->ttb_tuple,
           ahd->qhd_obj.qhd_proc_insert.ahd_proc_temptable->ttb_attr_count);
	opt_adf(global, ahd->qhd_obj.qhd_proc_insert.ahd_procparams,
                   "ahd_procparams");
        break;

     case QEA_IPROC:
        TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	  OPT_PBLEN, "ahd_procnam: %~t, ahd_ownname: %~t\n\n",
	    sizeof(ahd->qhd_obj.qhd_iproc.ahd_procnam),
	    &ahd->qhd_obj.qhd_iproc.ahd_procnam,
	    sizeof(ahd->qhd_obj.qhd_iproc.ahd_ownname),
	    &ahd->qhd_obj.qhd_iproc.ahd_ownname);
        break;

     case QEA_CLOSETEMP:
        TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	  OPT_PBLEN, "ahd_temptable: %5d\n\n",
	    ahd->qhd_obj.qhd_closetemp.ahd_temptable_to_close->
            ttb_tempTableIndex);
        break;

     case QEA_IF:
        TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	  OPT_PBLEN, "ahd_true: %p, ahd_false: %p\n\n",
	    ahd->qhd_obj.qhd_if.ahd_true, ahd->qhd_obj.qhd_if.ahd_false);
	opt_adf(global, ahd->qhd_obj.qhd_if.ahd_condition, "ahd_condition");
	next_aq = (OPT_AHDQEN *) opu_Gsmemory_get(global, sizeof (OPT_AHDQEN));
	next_aq->opt_global = global;
	next_aq->opt_aq_type = OPT_AQ_AHD;
	next_aq->opt_parent = aq;
	next_aq->opt_next = NULL;
	prev_aq = opt_lastaq(root_aq);
	prev_aq->opt_next = next_aq;
	next_aq->opt_qps = NULL;
	next_aq->opt_qpsp = aq->opt_qpsp;
	next_aq->opt_aq_data.opt_pahd = NULL;

	if (ahd->qhd_obj.qhd_if.ahd_true != NULL &&
		!opt_ahd_seen(root_aq, ahd->qhd_obj.qhd_if.ahd_true))
	{
	    next_aq->opt_aq_data.opt_pahd = ahd->qhd_obj.qhd_if.ahd_true;
	    opt_ahd(global, next_aq, root_aq, "if (TRUE)", ahd_no + 1);
	}

	if (ahd->qhd_obj.qhd_if.ahd_false != NULL &&
		!opt_ahd_seen(root_aq, ahd->qhd_obj.qhd_if.ahd_false))
	{
	    next_aq->opt_aq_data.opt_pahd = ahd->qhd_obj.qhd_if.ahd_false;
	    opt_ahd(global, next_aq, root_aq, "if (FALSE)", ahd_no + 1);
	}
	if (ahd->qhd_obj.qhd_if.ahd_false == ahd->ahd_next ||
	    ahd->qhd_obj.qhd_if.ahd_true == ahd->ahd_next)
	    return;			/* we're done */

	break;

     case QEA_RETURN:
	opt_adf(global, ahd->qhd_obj.qhd_return.ahd_rtn, "ahd_rtn");
	break;

     case QEA_MESSAGE:
     case QEA_EMESSAGE:
	opt_adf(global, ahd->qhd_obj.qhd_message.ahd_mnumber,  "ahd_mnumber");
	opt_adf(global, ahd->qhd_obj.qhd_message.ahd_mtext, "ahd_mtext");
	break;

     case QEA_COMMIT:
     case QEA_ROLLBACK:
	/* there isn't any commit or rollback specific info yet. */
	break;

     case QEA_EVREGISTER:
     case QEA_EVDEREG:
     case QEA_EVRAISE:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "Event action contents not traced\n\n");
	break;

     case QEA_D2_GET:
     case QEA_D1_QRY:
     case QEA_D3_XFR:
     case QEA_D4_LNK:
     case QEA_D5_DEF:
     case QEA_D6_AGG:
     case QEA_D8_CRE:
     case QEA_D9_UPD:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n");
	opt_qtdump(global, ahd);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n");
	break;

     default:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "Unsupported or illegal type\n\n");
	break;
    }

    if (ahd->ahd_next != NULL && ahd->ahd_atype != QEA_RETURN &&
		!opt_ahd_seen(root_aq, ahd->ahd_next))
    {
	next_aq = (OPT_AHDQEN *) opu_Gsmemory_get(global, sizeof (OPT_AHDQEN));
	next_aq->opt_global = global;
	next_aq->opt_aq_type = OPT_AQ_AHD;
	next_aq->opt_parent = aq;
	next_aq->opt_next = NULL;
	prev_aq = opt_lastaq(root_aq);
	prev_aq->opt_next = next_aq;
	next_aq->opt_aq_data.opt_pahd = ahd->ahd_next;
	next_aq->opt_qps = NULL;
	next_aq->opt_qpsp = aq->opt_qpsp;
	opt_ahd(global, next_aq, root_aq, label, ahd_no + 1);
    }
}

/*
** Name: opt_dmucb -- Print out a DMU_CB
**
** Description:
**	Trace the interesting parts of a DMU_CB created for a
**	DMU action header or a CREATE TABLE action header.
**
** Inputs:
**	global		OPF request state
**	dmucb		The DMU_CB to add to the trace output
**
** Outputs:
**	No result/return value
**
** History:
**	10-Feb-2004 (schka24)
**	    Extract from action header printing.
*/

static void
opt_dmucb(OPS_STATE *global, DMU_CB *dmucb)
{
    DB_LOC_NAME *locn;
    DMF_ATTR_ENTRY	**atts;
    DMF_ATTR_ENTRY	*att;
    DMU_KEY_ENTRY	**keys;
    DMU_KEY_ENTRY	*key;
    i4 i;

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	OPT_PBLEN, "dmu_table_name: %24s \t dmu_owner: %24s\n\n",
	dmucb->dmu_table_name.db_tab_name,
	dmucb->dmu_owner.db_own_name);
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	OPT_PBLEN, "dmu_attr_array: in_count: %5d \t size: %5d\n\n",
	dmucb->dmu_attr_array.ptr_in_count,
	dmucb->dmu_attr_array.ptr_size);

    atts = (DMF_ATTR_ENTRY **) dmucb->dmu_attr_array.ptr_address;
    for (i = 0; i < dmucb->dmu_attr_array.ptr_in_count; i += 1)
    {
	att = atts[i];
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN,
"att#%3d name: %24s type: %3d  size: %5d  prec: %2d  flags: %5d\n\n",
	    i+1, att->attr_name.db_att_name, att->attr_type, 
	    att->attr_size, att->attr_precision, 
	    att->attr_flags_mask);
    }

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	OPT_PBLEN, "dmu_key_array: in_count: %5d \t size: %5d\n\n",
	dmucb->dmu_key_array.ptr_in_count,
	dmucb->dmu_key_array.ptr_size);

    keys = (DMU_KEY_ENTRY **) dmucb->dmu_key_array.ptr_address;
    for (i = 0; i < dmucb->dmu_key_array.ptr_in_count; i += 1)
    {
	key = keys[i];
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "[%2d] name: %24s  order: ", i,
	    key->key_attr_name.db_att_name);
	switch (key->key_order)
	{
	 case DMU_ASCENDING:
	    TRformat(opt_scc, (i4*)NULL,
		global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "DMU_ASCENDING\n\n");
	    break;

	 case DMU_DESCENDING:
	    TRformat(opt_scc, (i4*)NULL,
		global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "DMU_DESCENDING\n\n");
	    break;

	 default:
	    TRformat(opt_scc, (i4*)NULL,
		global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "Unknown Sorting Order\n\n");
	    break;
	}
    }
    TRformat(opt_scc, NULL, global->ops_cstate.opc_prbuf,
	OPT_PBLEN, "dmu_location: in_size: %5d\n\n",
	dmucb->dmu_location.data_in_size);
    locn = (DB_LOC_NAME *) dmucb->dmu_location.data_address;
    for (i = 0; i < dmucb->dmu_location.data_in_size; i = i + sizeof(DB_LOC_NAME))
    {
	TRformat(opt_scc, NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "   [%3d] locname: %24s\n\n",
		i, locn->db_loc_name);
	++locn;
    }
    if (dmucb->dmu_part_def != NULL)
	opt_part_def(global, dmucb->dmu_part_def);

} /* opt_dmucb */

/*
** Name: opt_lpchar_list -- Print out a QEU_LOGPART_CHAR list
**
** Description:
**	Re-partitioning query plans (CREATE TABLE in its flavors,
**	for now) may carry along a list of QEU_LOGPART_CHAR blocks.
**	Each one is a set of instructions for physical structuring of
**	one or more logical partitions.  Once the caller figures out
**	where said list is, if there is one, dump them out.
**
** Inputs:
**	global		OPF request state block
**	qeu_lpl_ptr	Pointer to first entry of the list
**
** Outputs:
**	none
**
** History:
**	10-Feb-2004 (schka24)
**	    Written for partitioning.
*/

static void
opt_lpchar_list(OPS_STATE *global, QEU_LOGPART_CHAR *qeu_lpl_ptr)
{
    DB_LOC_NAME *locn;
    i4 i;

    while (qeu_lpl_ptr != NULL)
    {
	TRformat(opt_scc, NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "qeu_logpart_char at 0x%p: dim %2d, partseq %5d, nparts %5d\n\n",
		qeu_lpl_ptr, qeu_lpl_ptr->dim, qeu_lpl_ptr->partseq,
		qeu_lpl_ptr->nparts);
	TRformat(opt_scc, NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "loc_array: in_size: %5d\n\n",
		qeu_lpl_ptr->loc_array.data_in_size);
	locn = (DB_LOC_NAME *) qeu_lpl_ptr->loc_array.data_address;
	for (i = 0; i < qeu_lpl_ptr->loc_array.data_in_size; i = i + sizeof(DB_LOC_NAME))
	{
	    TRformat(opt_scc, NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "   [%3d] locname: %24s\n\n",
		i, locn->db_loc_name);
	    ++locn;
	}
	qeu_lpl_ptr = qeu_lpl_ptr->next;
    }
} /* opt_lpchar_list */

/*{
** Name: OPT_ADF	- Print out an QEN_ADF struct
**
** Description:
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      27-OCT-86 (ERIC)
**          written
**      16-jul-92 (anitap)
**          Cast naked NULL being passed into ade_cxbrief_print().
**      15-feb-93 (ed)
**          fix prototype casting errors
[@history_template@]...
*/
static VOID
opt_adf(
	OPS_STATE   *global,
	QEN_ADF	    *adf,
	char	    *title )
{
    i4	    baseno;

    if (adf == NULL || adf->qen_ade_cx == NULL || adf->qen_mask == 0)
    {
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "The ADE_CX %s does nothing\n\n", title);
	return;
    }

    ade_cxbrief_print((PTR *)NULL, adf->qen_ade_cx, title);
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"QEN_ADF:   qen_pos: %5d \t\t qen_pcx_idx: %5d\n\n",
	adf->qen_pos, adf->qen_pcx_idx);
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"qen_uoutput: %5d  \t qen_output: %5d \t qen_input: %5d\n\n",
	adf->qen_uoutput, adf->qen_output, adf->qen_input);
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"qen_sz_base: %5d \t\t qen_max_base: %5d\n\n",
	adf->qen_sz_base, adf->qen_max_base);

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	OPT_PBLEN, "qen_base:\n\n");
    for (baseno = 0; baseno < adf->qen_sz_base; baseno += 1)
    {
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "[%2d] ", baseno + ADE_ZBASE);
	switch (adf->qen_base[baseno].qen_array)
	{
	 case QEN_ROW:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "QEN_ROW :      ");
	    break;

	 case QEN_KEY:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "QEN_KEY:       ");
	    break;

	 case QEN_PARM:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "QEN_PARM:      ");
	    break;

	 case QEN_OUT:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "QEN_OUT:       ");
	    break;

	 case QEN_VTEMP:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "QEN_VTEMP:     ");
	    break;

	 case QEN_DMF_ALLOC:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "QEN_DMF_ALLOC: ");
	    break;

	 default:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "Unknown Qen Array Type");
	    break;
	}
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "index: %5d \t parm_row: %5d\n\n", 
	    adf->qen_base[baseno].qen_index,
	    adf->qen_base[baseno].qen_parm_row);
    }
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	OPT_PBLEN, "\n\n");
}

/*{
** Name: OPT_LQEP	- Return the left of a QEN_NODE.
**
** Description:
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      28-oct-86 (eric)
**          written
[@history_template@]...
*/
static PTR
opt_lqep(
	PTR	p )
{
    QEN_NODE	*qen = (QEN_NODE *) p;

    switch (qen->qen_type)
    {
     case QE_TJOIN:
	return((PTR) qen->node_qen.qen_tjoin.tjoin_out);
	break;

     case QE_KJOIN:
	return((PTR) qen->node_qen.qen_kjoin.kjoin_out);
	break;

     case QE_HJOIN:
	return((PTR) qen->node_qen.qen_hjoin.hjn_out);
	break;

     case QE_EXCHANGE:
	return((PTR) qen->node_qen.qen_exch.exch_out);
	break;

     case QE_SEJOIN:
	return((PTR) qen->node_qen.qen_sejoin.sejn_out);
	break;

     case QE_FSMJOIN:
     case QE_ISJOIN:
     case QE_CPJOIN:
	return((PTR) qen->node_qen.qen_sjoin.sjn_out);
	break;

     case QE_SORT:
	return((PTR) qen->node_qen.qen_sort.sort_out);
	break;

     case QE_TSORT:
	return((PTR) qen->node_qen.qen_tsort.tsort_out);
	break;

     default:
	break;
    }
    return((PTR) NULL);
}

/*{
** Name: OPT_RQEP	- Return the right of a QEN_NODE.
**
** Description:
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      28-oct-86 (eric)
**          written
**	11-may-99 (inkdo01)
**	    Add hash join.
[@history_template@]...
*/
static PTR
opt_rqep(
	PTR	p )
{
    QEN_NODE	*qen = (QEN_NODE *) p;

    switch (qen->qen_type)
    {
     case QE_SEJOIN:
	return((PTR) qen->node_qen.qen_sejoin.sejn_inner);
	break;

     case QE_HJOIN:
	return((PTR) qen->node_qen.qen_hjoin.hjn_inner);
	break;

     case QE_FSMJOIN:
     case QE_ISJOIN:
     case QE_CPJOIN:
	return((PTR) qen->node_qen.qen_sjoin.sjn_inner);
	break;

     default:
	break;
    }

    return((PTR) NULL);
}

/*{
** Name: OPT_PRQEP	- Print a QEN_NODE for a QEP tree.
**
** Description:
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      28-oct-86 (eric)
**          written
[@history_template@]...
*/
static VOID
opt_prqep(
	PTR	p,
	PTR	control )
{
    QEN_NODE	*qen = (QEN_NODE *) p;
    OPS_CB	*opscb;		/* ptr to session control block */
    OPS_STATE   *global;	/* ptr to global state variable */

    opscb = ops_getcb();        /* get the session control block
                                ** from the SCF */
    global = opscb->ops_state;	/* ptr to global state variable */

    TRformat(uld_tr_callback, (i4 *)0,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"|%2d|\t %p", qen->qen_num, qen);
    uld_prnode(control, &global->ops_trace.opt_trformat[0]);
    switch (qen->qen_type)
    {
     case QE_TJOIN:
	uld_prnode(control, "TJN  ");
	break;

     case QE_KJOIN:
	uld_prnode(control, "KJN  ");
	break;

     case QE_HJOIN:
	uld_prnode(control, "HJN  ");
	break;

     case QE_EXCHANGE:
	uld_prnode(control, "EXCH ");
	break;

     case QE_SEJOIN:
	uld_prnode(control, "SEJN ");
	break;

     case QE_FSMJOIN:
	uld_prnode(control, "FSMJN");
	break;

     case QE_ISJOIN:
	uld_prnode(control, "ISJN ");
	break;

     case QE_CPJOIN:
	uld_prnode(control, "CPJN ");
	break;

     case QE_ORIG:
	uld_prnode(control, "ORIG ");
	break;

     case QE_TPROC:
	uld_prnode(control, "TPROC");
	break;

     case QE_SORT:
	uld_prnode(control, "SORT ");
	break;

     case QE_TSORT:
	uld_prnode(control, "TSRT ");
	break;

     case QE_QP:
	uld_prnode(control, "QP   ");
	break;

     default:
	uld_prnode(control, "BAD ");
	break;
    }
}

/*{
** Name: OPT_QENODE	- Trace most of an QEN_NODE struct
**
** Description:
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      5-sept-86 (eric)
**          written
**	26-mar-92 (rickh)
**	    Removed references to oj_create and oj_load, which weren't 
**	    being used.  Replaced references to oj_shd with oj_tidHoldFile.
**	    Added oj_ijFlagsFile.
**	11-jun-92 (anitap)
**	    changed the check in the FOR loop from opt_qp > NULL to 
**	    opt_qp != NULL to fix su4 acc warning. 
**	25-jun-93 (rickh)
**	    Added code to print out outer join info for tid joins.
**	03-feb-94 (rickh)
**	    Replaced 9999 diagnostics with real error messages.
**      19-jun-95 (inkdo01)
**          Replaced orig_tkey/ukey nat's with orig_flag
**	20-feb-96 (inkdo01)
**	    Changes to support reorganized query tree nodes.
**	3-nov-99 (inkdo01)
**	    Added display of orig_qualrows.
**	17-dec-03 (inkdo01)
**	    Added display of qen_high/low, orig node partitioning flds.
**	22-july-04 (inkdo01)
**	    Added support for DSH index arrays in EXCH nodes.
**	16-Apr-2007 (kschendel) b122118
**	    Spiff up the header flags display a little, and use a more visible
**	    separator than just formfeed.
**	14-May-2010 (kschendel) b123565
**	    Drop high/low, not used.
**	19-May-2010 (kschendel) b123759
**	    Added pqual count to the exchange node arrays.
**	10-Sep-2010 (kschendel) b124341
**	    SEjoin replace kcompare with cvmat.
*/
static VOID
opt_qenode(
	OPS_STATE   *global,
	QEN_NODE    *qn,
	OPT_AHDQEN  *paq )
{
    i4		attno;
    RDR_INFO	*rdrdesc;
    i4		rdrattno;
    i4		me_ret;
    i2		i, j, k;
    OPT_AHDQEN	*aq;
    char	*jntype = (char *)0;
    QEN_OJINFO	*oj = (QEN_OJINFO *)NULL;
    QEN_PART_INFO *partp = (QEN_PART_INFO *) NULL;

    if (qn == NULL)
	return;

    if (qn->qen_type == QE_QP)
    {
	if (paq->opt_qpsp && *paq->opt_qpsp)
	{
	    OPT_QP  *opt_qp;

	    for (opt_qp = *paq->opt_qpsp; opt_qp != NULL;
		 opt_qp = opt_qp->opt_nqp)
	    {
		if (opt_qp->opt_qennum == qn->qen_num)
		{
		    /* This node has already been processed. */
		    return;
		}
	    }
	}
    }

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	OPT_PBLEN, "---------------------\n\nQEN_NODE [%d]: ",
	qn->qen_num);
    switch (qn->qen_type)
    {
     case QE_TJOIN:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "QE_TJOIN");
	oj = qn->node_qen.qen_tjoin.tjoin_oj;
	break;

     case QE_KJOIN:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, 
	    OPT_PBLEN, "QE_KJOIN");
	oj = qn->node_qen.qen_kjoin.kjoin_oj;
	break;

     case QE_HJOIN:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, 
	    OPT_PBLEN, "QE_HJOIN");
	oj = qn->node_qen.qen_hjoin.hjn_oj;
	break;

     case QE_SEJOIN:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, 
	    OPT_PBLEN, "QE_SEJOIN");
	break;

     case QE_FSMJOIN:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, 
	    OPT_PBLEN, "QE_FSMJOIN");
	oj = qn->node_qen.qen_sjoin.sjn_oj;
	break;

     case QE_ISJOIN:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, 
	    OPT_PBLEN, "QE_ISJOIN");
	oj = qn->node_qen.qen_sjoin.sjn_oj;
	break;

     case QE_CPJOIN:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, 
	    OPT_PBLEN, "QE_CPJOIN");
	oj = qn->node_qen.qen_sjoin.sjn_oj;
	break;

     case QE_ORIG:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, 
	    OPT_PBLEN, "QE_ORIG");
	break;

     case QE_TPROC:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, 
	    OPT_PBLEN, "QE_TPROC");
	break;

     case QE_EXCHANGE:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, 
	    OPT_PBLEN, "QE_EXCH");
	break;

     case QE_SORT:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, 
	    OPT_PBLEN, "QE_SORT");
	break;

     case QE_TSORT:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, 
	    OPT_PBLEN, "QE_TSORT");
	break;

     case QE_QP:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, 
	    OPT_PBLEN, "QE_QP");
	break;

     default:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "UNKNOWN QEN TYPE");
    }

    if (oj)
    {
	switch (oj->oj_jntype)
	{
	    case DB_NOJOIN:
	    case DB_INNER_JOIN:
		jntype = "inner";
		break;
	    case DB_LEFT_JOIN:
		jntype = "left outer";
		break;
	    case DB_RIGHT_JOIN:
		jntype = "right outer";
		break;
	    case DB_FULL_JOIN:
		jntype = "full outer";
		break;
	    default:
		opx_error( E_OP039A_OJ_TYPE );
		/*FIXME*/
		break;
	}
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, " \t %s", jntype);
    }

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	" \t qen_flags: 0x%x: %v - %w\n\n", 
	qn->qen_flags,
	"PART_SEP,PART_NEST,PQ_RESET", qn->qen_flags&(~QEN_CARD_MASK),
	QEN_SELTYPES,(qn->qen_flags&QEN_CARD_MASK)>>3);

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"qen_size: %5d \t qen_row: %5d \t qen_rsz: %5d\n\n", 
	qn->qen_size, qn->qen_row, qn->qen_rsz);

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"qen_frow: %5d\t qen_nthreads: %5d\n\n\n", 
	qn->qen_frow, qn->qen_nthreads);

    /* Now lets print out the attribute names, plus the offsets if possible
    */
    if (qn->qen_type == QE_ORIG)
    {
	rdrdesc = opt_rdrdesc(global, 
			global->ops_cstate.opc_qp->qp_ahd, 
				    qn->node_qen.qen_orig.orig_get);
    }
    else if (qn->qen_type == QE_TJOIN)
    {
	rdrdesc = opt_rdrdesc(global, global->ops_cstate.opc_qp->qp_ahd, 
	    qn->node_qen.qen_tjoin.tjoin_get);
    }
    else if (qn->qen_type == QE_KJOIN)
    {
	rdrdesc = opt_rdrdesc(global, 
			global->ops_cstate.opc_qp->qp_ahd, 
				    qn->node_qen.qen_kjoin.kjoin_get);
    }
    else
    {
	rdrdesc = NULL;
    }
    for (attno = 0; attno < qn->qen_natts; attno += 1)
    {
	if (qn->qen_atts[attno] == NULL)
	    continue;
	if (rdrdesc == NULL)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "[%2d] name: %24s\n\n", attno, 
		qn->qen_atts[attno]->att_name.db_att_name);
	}
	else
	{
	    for (rdrattno = 1; 
		    rdrattno <= rdrdesc->rdr_rel->tbl_attr_count; 
		    rdrattno +=1
		)
	    {
		me_ret = MEcmp(qn->qen_atts[attno]->att_name.db_att_name, 
		    rdrdesc->rdr_attr[rdrattno]->att_name.db_att_name,
		    sizeof (qn->qen_atts[attno]->att_name.db_att_name));
		if (me_ret == 0)
		{
		    TRformat(opt_scc, (i4*)NULL,
			global->ops_cstate.opc_prbuf, OPT_PBLEN,
			"[%2d] name: %24s offset: %5d\n\n", 
			attno, qn->qen_atts[attno]->att_name.db_att_name,
			rdrdesc->rdr_attr[rdrattno]->att_offset);
		    break;
		}
	    }
	    if (me_ret != 0)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "[%2d] name: %24s\n\n", attno, 
		    qn->qen_atts[attno]->att_name.db_att_name);
	    }
	}
    }

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	OPT_PBLEN, "\n\n");
    opt_adf(global, qn->qen_prow, "qen_prow");
    if (qn->qen_fatts == NULL)
    {
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "No qen_fatts ADE_CX\n\n");
    }
    else
    {
	opt_adf(global, qn->qen_fatts, "qen_fatts");
    }

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	OPT_PBLEN, "\n\n");
    switch (qn->qen_type)
    {
     case QE_ORIG:
     {
	QEN_ORIG	*orig;

	orig = &qn->node_qen.qen_orig;
	rdrdesc = opt_rdrdesc(global, 
			    global->ops_cstate.opc_qp->qp_ahd, orig->orig_get);
	if (rdrdesc == NULL)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "table name: UNKNOWN\n\n");
	}
	else
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "table name: %24s\n\n", 
		rdrdesc->rdr_rel->tbl_name.db_tab_name);
	}
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "orig_get: %5d  \t orig_keys: %5d \t orig_tid: %5d\n\n", 
	    orig->orig_get, orig->orig_keys, orig->orig_tid);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
  	    OPT_PBLEN, "orig_ordcnt: %5d\t orig_qualrows: %5d\n\n", 
  	    orig->orig_ordcnt, orig->orig_qualrows);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "orig_flag: %x %v\n\n",
	    orig->orig_flag, "TKEY,UKEY,MAXOPT,MINOPT,RTREE,MCNSTTAB,READBACK,IXONLY,FIND_ONLY,MAXMIN",
	    orig->orig_flag);
	opt_qenpart(global, orig->orig_part, "orig_part");
	opt_qenpqual(global, orig->orig_pqual, "orig_pqual");
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n\n");
	opt_keys(global, orig->orig_pkeys);
	opt_adf(global, orig->orig_qual, "orig_qual");
	break;
     }

     case QE_TPROC:
     {
	QEN_TPROC	*tproc;

	tproc = &qn->node_qen.qen_tproc;
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "tproc_pcount: %5d  \t tproc_dshix: %5d \t tproc_flag: %5d\n\n", 
	    tproc->tproc_pcount, tproc->tproc_dshix, tproc->tproc_flag);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "tproc_dbpID: %5d  \t tproc_resix: %p\n\n", 
	    tproc->tproc_dbpID.db_tab_base, tproc->tproc_resix);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n\n");

	/* Dump actual parameter descriptors. */
	for (i = 0; i < tproc->tproc_pcount; i++)
	{
	    QEF_CP_PARAM	*aparm;

	    aparm = tproc->tproc_params + i;
            TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  tproc_params[%d]: parm_name: %16s, parm_nlen: %d, parm_index: %d, parm_flags: %d\n\n",
        	i, aparm->ahd_parm.parm_name, aparm->ahd_parm.parm_nlen,
		aparm->ahd_parm.parm_index, aparm->ahd_parm.parm_flags);
            TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\tparm_row: %d, parm_offset: %d\n\n",
        	aparm->ahd_parm_row, aparm->ahd_parm_offset);
	}
	opt_adf(global, tproc->tproc_parambuild, "tproc_parambuild");
	opt_adf(global, tproc->tproc_qual, "tproc_qual");
	break;
     }

     case QE_KJOIN:
     {
	QEN_KJOIN	*kjn;

	kjn = &qn->node_qen.qen_kjoin;
	rdrdesc = opt_rdrdesc(global, 
	    global->ops_cstate.opc_qp->qp_ahd, kjn->kjoin_get);
	if (rdrdesc == NULL)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "table name: UNKNOWN\n\n");
	}
	else
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "table name: %24s\n\n", 
		rdrdesc->rdr_rel->tbl_name.db_tab_name);
	}
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "kjoin_get: %5d \t kjoin_kuniq: %5d \t kjoin_nloff: %5d\n\n", 
	    kjn->kjoin_get, kjn->kjoin_kuniq, kjn->kjoin_nloff);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "kjoin_tid: %5d \t kjoin_tqual: %p\tkjoin_krow: %5d\n\n",
	    kjn->kjoin_tid, kjn->kjoin_tqual, kjn->kjoin_krow);
	opt_qenpart(global, kjn->kjoin_part, "kjoin_part");
	opt_qenpqual(global, kjn->kjoin_pqual, "kjoin_pqual");
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n\n");
	opt_keys(global, kjn->kjoin_kys);

	opt_adf(global, kjn->kjoin_key, "kjoin_key");
	opt_adf(global, kjn->kjoin_kcompare, "kjoin_kcompare");
	opt_adf(global, kjn->kjoin_jqual, "kjoin_jqual");
	opt_adf(global, kjn->kjoin_kqual, "kjoin_kqual");
	opt_adf(global, kjn->kjoin_iqual, "kjoin_iqual");
	if (oj)
	{
	if (oj->oj_oqual)
	    opt_adf(global, oj->oj_oqual, "oj_oqual");
	else
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	        OPT_PBLEN, "The ADE_CX oj_oqual does nothing\n\n");
	if (oj->oj_equal)
	    opt_adf(global, oj->oj_equal, "oj_equal");
	else
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	        OPT_PBLEN, "The ADE_CX oj_equal does nothing\n\n");
	if (oj->oj_lnull)
	    opt_adf(global, oj->oj_lnull, "oj_lnull");
	else
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	        OPT_PBLEN, "The ADE_CX oj_lnull does nothing\n\n");
	if (oj->oj_rnull)
	    opt_adf(global, oj->oj_rnull, "oj_rnull");
	else
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	        OPT_PBLEN, "The ADE_CX oj_rnull does nothing\n\n");

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "oj_tidHoldFile: %5d\t oj_heldTidRow: %5d\n\n", 
	    oj->oj_tidHoldFile, oj->oj_heldTidRow);

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "oj_ijFlagsFile: %5d\t oj_ijFlagsRow: %5d\n\n", 
	    oj->oj_ijFlagsFile, oj->oj_ijFlagsRow);

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "oj_resultEQCrow: %5d\t oj_specialEQCrow: %5d\n\n", 
	    oj->oj_resultEQCrow, oj->oj_specialEQCrow);
	}
	else
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	        OPT_PBLEN, "There is no outer join\n\n");

	opt_qenode(global, kjn->kjoin_out, paq);
	break;
     }

     case QE_TJOIN:
     {
	QEN_TJOIN	*tjn;

	tjn = &qn->node_qen.qen_tjoin;
	rdrdesc = opt_rdrdesc(global, 
	    global->ops_cstate.opc_qp->qp_ahd, tjn->tjoin_get);
	if (rdrdesc == NULL)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "table name: UNKNOWN\n\n");
	}
	else
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "table name: %24s\n\n", 
		rdrdesc->rdr_rel->tbl_name.db_tab_name);
	}
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "tjoin_get: %5d \ttjoin_flag: %5d\n\n", 
			tjn->tjoin_get, tjn->tjoin_flag);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "tjoin_orow: %5d \t tjoin_ooffset: %5d\n\n",
			tjn->tjoin_orow, tjn->tjoin_ooffset);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN,"tjoin_irow: %5d \t tjoin_ioffset: %5d\n\n",
			tjn->tjoin_irow, tjn->tjoin_ioffset);
	opt_qenpart(global, tjn->tjoin_part, "tjoin_part");
	opt_qenpqual(global, tjn->tjoin_pqual, "tjoin_pqual");
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n\n");

	if (oj)
	{
	if (oj->oj_oqual)
	    opt_adf(global, oj->oj_oqual, "oj_oqual");
	else
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	        OPT_PBLEN, "The ADE_CX oj_oqual does nothing\n\n");
	if (oj->oj_equal)
	    opt_adf(global, oj->oj_equal, "oj_equal");
	else
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	        OPT_PBLEN, "The ADE_CX oj_equal does nothing\n\n");
	if (oj->oj_lnull)
	    opt_adf(global, oj->oj_lnull, "oj_lnull");
	else
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	        OPT_PBLEN, "The ADE_CX oj_lnull does nothing\n\n");
	if (oj->oj_rnull)
	    opt_adf(global, oj->oj_rnull, "oj_rnull");
	else
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	        OPT_PBLEN, "The ADE_CX oj_rnull does nothing\n\n");
	}
	else
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	        OPT_PBLEN, "There is no outer join\n\n");

	opt_adf(global, tjn->tjoin_jqual, "tjoin_jqual");
	if (tjn->tjoin_isnull)
	    opt_adf(global, tjn->tjoin_isnull, "tjoin_isnull");
	else
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "The ADE_CX tjoin_isnull does nothing\n\n");

	opt_qenode(global, tjn->tjoin_out, paq);
	break;
     }

     case QE_EXCHANGE:
     {
	QEN_EXCH	*exch;
	i4		excnt = 1;
	i4		arroff;

	exch = &qn->node_qen.qen_exch;
	if (exch->exch_flag & EXCH_UNION)
	    excnt = exch->exch_ccount;
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN,
	    "exch_ccount: %5d \t exch_flag: %5d \t exch_dups: %5d\n\n", 
	    exch->exch_ccount, exch->exch_flag, exch->exch_dups);
	for (i = 0; i < excnt; i++)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "exch_ixes[%3d]:\n\n", i);
	    arroff = 0;
	    if (exch->exch_ixes[i].exch_row_cnt > 0)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "exch_row_cnt: %6d\t -", 
		    exch->exch_ixes[i].exch_row_cnt);
		for (j = arroff, k = 0; j < arroff + 
			exch->exch_ixes[i].exch_row_cnt; j++, k++)
		{
		    if (k >= 8)
		    {
			/* Max. 8 values per line. */
			k = 0;
			TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			    opc_prbuf, OPT_PBLEN, "\n\n");
		    }
		    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			opc_prbuf, OPT_PBLEN, "%6d", 
			exch->exch_ixes[i].exch_array1[j]);
		}
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\n");
		arroff += exch->exch_ixes[i].exch_row_cnt;
	    }

	    if (exch->exch_ixes[i].exch_hsh_cnt > 0)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "exch_hsh_cnt: %6d\t -", 
		    exch->exch_ixes[i].exch_hsh_cnt);
		for (j = arroff, k = 0; j < arroff + 
			exch->exch_ixes[i].exch_hsh_cnt; j++, k++)
		{
		    if (k >= 8)
		    {
			/* Max. 8 values per line. */
			k = 0;
			TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			    opc_prbuf, OPT_PBLEN, "\n\n");
		    }
		    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			opc_prbuf, OPT_PBLEN, "%6d", 
			exch->exch_ixes[i].exch_array1[j]);
		}
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\n");
		arroff += exch->exch_ixes[i].exch_hsh_cnt;
	    }

	    if (exch->exch_ixes[i].exch_hsha_cnt > 0)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "exch_hhash_cnt: %5d\t -", 
		    exch->exch_ixes[i].exch_hsha_cnt);
		for (j = arroff, k = 0; j < arroff + 
			exch->exch_ixes[i].exch_hsha_cnt; j++, k++)
		{
		    if (k >= 8)
		    {
			/* Max. 8 values per line. */
			k = 0;
			TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			    opc_prbuf, OPT_PBLEN, "\n\n");
		    }
		    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			opc_prbuf, OPT_PBLEN, "%6d", 
			exch->exch_ixes[i].exch_array1[j]);
		}
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\n");
		arroff += exch->exch_ixes[i].exch_hsha_cnt;
	    }

	    if (exch->exch_ixes[i].exch_stat_cnt > 0)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "exch_stat_cnt: %5d\t -", 
		    exch->exch_ixes[i].exch_stat_cnt);
		for (j = arroff, k = 0; j < arroff + 
			exch->exch_ixes[i].exch_stat_cnt; j++, k++)
		{
		    if (k >= 8)
		    {
			/* Max. 8 values per line. */
			k = 0;
			TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			    opc_prbuf, OPT_PBLEN, "\n\n");
		    }
		    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			opc_prbuf, OPT_PBLEN, "%6d", 
			exch->exch_ixes[i].exch_array1[j]);
		}
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\n");
		arroff += exch->exch_ixes[i].exch_stat_cnt;
	    }

	    if (exch->exch_ixes[i].exch_ttab_cnt > 0)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "exch_ttab_cnt: %5d\t -", 
		    exch->exch_ixes[i].exch_ttab_cnt);
		for (j = arroff, k = 0; j < arroff + 
			exch->exch_ixes[i].exch_ttab_cnt; j++, k++)
		{
		    if (k >= 8)
		    {
			/* Max. 8 values per line. */
			k = 0;
			TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			    opc_prbuf, OPT_PBLEN, "\n\n");
		    }
		    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			opc_prbuf, OPT_PBLEN, "%6d", 
			exch->exch_ixes[i].exch_array1[j]);
		}
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\n");
		arroff += exch->exch_ixes[i].exch_ttab_cnt;
	    }

	    if (exch->exch_ixes[i].exch_hld_cnt > 0)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "exch_hld_cnt: %6d\t -", 
		    exch->exch_ixes[i].exch_hld_cnt);
		for (j = arroff, k = 0; j < arroff + 
			exch->exch_ixes[i].exch_hld_cnt; j++, k++)
		{
		    if (k >= 8)
		    {
			/* Max. 8 values per line. */
			k = 0;
			TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			    opc_prbuf, OPT_PBLEN, "\n\n");
		    }
		    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			opc_prbuf, OPT_PBLEN, "%6d", 
			exch->exch_ixes[i].exch_array1[j]);
		}
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\n");
		arroff += exch->exch_ixes[i].exch_hld_cnt;
	    }

	    if (exch->exch_ixes[i].exch_shd_cnt > 0)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "exch_shd_cnt: %6d\t -", 
		    exch->exch_ixes[i].exch_shd_cnt);
		for (j = arroff, k = 0; j < arroff + 
			exch->exch_ixes[i].exch_shd_cnt; j++, k++)
		{
		    if (k >= 8)
		    {
			/* Max. 8 values per line. */
			k = 0;
			TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			    opc_prbuf, OPT_PBLEN, "\n\n");
		    }
		    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			opc_prbuf, OPT_PBLEN, "%6d", 
			exch->exch_ixes[i].exch_array1[j]);
		}
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\n");
		arroff += exch->exch_ixes[i].exch_shd_cnt;
	    }

	    if (exch->exch_ixes[i].exch_pqual_cnt > 0)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "exch_pqual_cnt: %6d\t -", 
		    exch->exch_ixes[i].exch_pqual_cnt);
		for (j = arroff, k = 0; j < arroff + 
			exch->exch_ixes[i].exch_pqual_cnt; j++, k++)
		{
		    if (k >= 8)
		    {
			/* Max. 8 values per line. */
			k = 0;
			TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			    opc_prbuf, OPT_PBLEN, "\n\n");
		    }
		    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			opc_prbuf, OPT_PBLEN, "%6d", 
			exch->exch_ixes[i].exch_array1[j]);
		}
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\n");
		arroff += exch->exch_ixes[i].exch_pqual_cnt;
	    }

	    arroff = 0;		/* reset for exch_array2 */
	    if (exch->exch_ixes[i].exch_cx_cnt > 0)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "exch_cx_cnt: %7d\t -", 
		    exch->exch_ixes[i].exch_cx_cnt);
		for (j = arroff, k = 0; j < arroff + 
			exch->exch_ixes[i].exch_cx_cnt; j++, k++)
		{
		    if (k >= 8)
		    {
			/* Max. 8 values per line. */
			k = 0;
			TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			    opc_prbuf, OPT_PBLEN, "\n\n");
		    }
		    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			opc_prbuf, OPT_PBLEN, "%6d", 
			exch->exch_ixes[i].exch_array2[j]);
		}
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\n");
		arroff += exch->exch_ixes[i].exch_cx_cnt;
	    }

	    if (exch->exch_ixes[i].exch_dmr_cnt > 0)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "exch_dmr_cnt: %6d\t -", 
		    exch->exch_ixes[i].exch_dmr_cnt);
		for (j = arroff, k = 0; j < arroff + 
			exch->exch_ixes[i].exch_dmr_cnt; j++, k++)
		{
		    if (k >= 8)
		    {
			/* Max. 8 values per line. */
			k = 0;
			TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			    opc_prbuf, OPT_PBLEN, "\n\n");
		    }
		    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			opc_prbuf, OPT_PBLEN, "%6d", 
			exch->exch_ixes[i].exch_array2[j]);
		}
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\n");
		arroff += exch->exch_ixes[i].exch_dmr_cnt;
	    }

	    if (exch->exch_ixes[i].exch_dmt_cnt > 0)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "exch_dmt_cnt: %6d\t -", 
		    exch->exch_ixes[i].exch_dmt_cnt);
		for (j = arroff, k = 0; j < arroff + 
			exch->exch_ixes[i].exch_dmt_cnt; j++, k++)
		{
		    if (k >= 8)
		    {
			/* Max. 8 values per line. */
			k = 0;
			TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			    opc_prbuf, OPT_PBLEN, "\n\n");
		    }
		    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			opc_prbuf, OPT_PBLEN, "%6d", 
			exch->exch_ixes[i].exch_array2[j]);
		}
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\n");
		arroff += exch->exch_ixes[i].exch_dmt_cnt;
	    }

	    if (exch->exch_ixes[i].exch_dmh_cnt > 0)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "exch_dmh_cnt: %6d\t -", 
		    exch->exch_ixes[i].exch_dmh_cnt);
		for (j = arroff, k = 0; j < arroff + 
			exch->exch_ixes[i].exch_dmh_cnt; j++, k++)
		{
		    if (k >= 8)
		    {
			/* Max. 8 values per line. */
			k = 0;
			TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			    opc_prbuf, OPT_PBLEN, "\n\n");
		    }
		    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.
			opc_prbuf, OPT_PBLEN, "%6d", 
			exch->exch_ixes[i].exch_array2[j]);
		}
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\n");
		arroff += exch->exch_ixes[i].exch_dmh_cnt;
	    }

	}
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n\n");
	opt_satts(global, exch->exch_acount, exch->exch_atts, 
	    exch->exch_macount, exch->exch_matts, NULL);
	opt_adf(global, exch->exch_mat, "exch_mat");
	opt_qenode(global, exch->exch_out, paq);
	break;
     }

     case QE_SORT:
     {
	QEN_SORT	*sort;

	sort = &qn->node_qen.qen_sort;
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN,
	    "sort_create: %5d \t sort_load: %5d \t sort_dups: %5d\n\n", 
	    sort->sort_create, sort->sort_load, sort->sort_dups);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "sort_shd: %5d\n\n", sort->sort_shd);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n\n");
	opt_satts(global, sort->sort_acount, sort->sort_atts, 
	    sort->sort_sacount, sort->sort_satts,
	    sort->sort_cmplist);
	opt_adf(global, sort->sort_mat, "sort_mat");
	opt_qenode(global, sort->sort_out, paq);
	break;
     }

     case QE_TSORT:
     {
	QEN_TSORT	*tsort;

	tsort = &qn->node_qen.qen_tsort;
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "tsort_create:%6d\t tsort_load:%6d\ttsort_shd:%6d\n\n", 
	    tsort->tsort_create, tsort->tsort_load, tsort->tsort_shd);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "tsort_get: %5d \t tsort_dups: %5d\n\n", 
	    tsort->tsort_get, tsort->tsort_dups);

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n\n");
	opt_satts(global, tsort->tsort_acount, tsort->tsort_atts, 
	    tsort->tsort_scount, tsort->tsort_satts,
	    tsort->tsort_cmplist);
	opt_adf(global, tsort->tsort_mat, "tsort_mat");
	opt_qenpqual(global, tsort->tsort_pqual, "tsort_pqual (for FSM)");
	opt_qenode(global, tsort->tsort_out, paq);
	break;
     }

     case QE_QP:
     {
	QEN_QP	    *ran;
	OPT_QP	    *opt_qp;

	ran = &qn->node_qen.qen_qp;
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "qp_flag: %5d\n\n", ran->qp_flag);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n\n");
	opt_adf(global, ran->qp_qual, "qp_qual");

	aq = (OPT_AHDQEN *) opu_Gsmemory_get(global, sizeof (OPT_AHDQEN));
	aq->opt_global = global;
	aq->opt_aq_type = OPT_AQ_AHD;
	aq->opt_parent = NULL;
	aq->opt_next = NULL;
	aq->opt_aq_data.opt_pahd = ran->qp_act;
	aq->opt_qps = NULL;
	aq->opt_qpsp = paq->opt_qpsp;
	if (paq->opt_qpsp)
	{
	    opt_qp = (OPT_QP *) opu_Gsmemory_get(global, sizeof (OPT_QP));

	    opt_qp->opt_qennum = qn->qen_num;

	    if (*paq->opt_qpsp)
	    {
		opt_qp->opt_nqp = *paq->opt_qpsp;
	    }
	    else
	    {
		opt_qp->opt_nqp = NULL;
	    }

	    *paq->opt_qpsp = opt_qp;
	}
	opt_ahd(global, aq, aq, "QP", 1);
	break;
     }

     case QE_SEJOIN:
     {
	QEN_SEJOIN	*sejn;
	char		*s, *s1;

	sejn = &qn->node_qen.qen_sejoin;

	if (sejn->sejn_hcreate == TRUE)
	    s = "TRUE";
	else
	    s = "FALSE";

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN,
	    "sejn_hget: %5d \t sejn_hcreate: %s \t sejn_hfile: %5d\n\n",
	    sejn->sejn_hget, s, sejn->sejn_hfile);
	switch (sejn->sejn_junc)
	{
	 case QESE1_CONT_AND:
	    s1 = "QESE1_CONT_AND";
	    break;

	 case QESE2_CONT_OR:
	    s1 = "QESE1_CONT_OR";
	    break;

	 case QESE3_RETURN_OR:
	    s1 = "QESE1_RETURN_OR";
	    break;

	 case QESE4_RETURN_AND:
	    s1 = "QESE1_RETURN_AND";
	    break;

	 default:
	    s1 = "UNKNOWN JUNCTION";
	    break;
	}
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "sejn_junc: %d %s\n\n",
	    sejn->sejn_junc, s1);

	switch (sejn->sejn_oper)
	{
	 case SEL_LESS:
	    s = "SEL_LESS";
	    break;

	 case SEL_EQUAL:
	    s = "SEL_EQUAL";
	    break;

	 case SEL_GREATER:
	    s = "SEL_GREATER";
	    break;

	 case SEL_OTHER:
	    s = "SEL_OTHER";
	    break;

	default:
	    s = "UNKNOWN SEL_OPER";
	    break;
	}
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "sejn_oper: %d %s\n\n",
	    sejn->sejn_oper, s);

	opt_adf(global, sejn->sejn_itmat, "sejn_itmat");
	opt_adf(global, sejn->sejn_okmat, "sejn_okmat");
	opt_adf(global, sejn->sejn_cvmat, "sejn_cvmat");
	opt_adf(global, sejn->sejn_ccompare, "sejn_ccompare");
	opt_adf(global, sejn->sejn_kqual, "sejn_kqual");
	opt_adf(global, sejn->sejn_oqual, "sejn_oqual");

	opt_qenode(global, sejn->sejn_out, paq);
	opt_qenode(global, sejn->sejn_inner, paq);
	break;
     }

     case QE_FSMJOIN:
     case QE_ISJOIN:
     case QE_CPJOIN:
     {
	QEN_SJOIN	*jn;

	jn = &qn->node_qen.qen_sjoin;
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN,
	    "sjn_hfile: %5d\t sjn_krow: %5d\n\n", 
	    jn->sjn_hfile, jn->sjn_krow);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "sjn_kuniq: %5d", jn->sjn_kuniq);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n\n");
	opt_adf(global, jn->sjn_itmat, "sjn_itmat");
	opt_adf(global, jn->sjn_okmat, "sjn_okmat");
	opt_adf(global, jn->sjn_okcompare, "sjn_okcompare");
	opt_adf(global, jn->sjn_joinkey, "sjn_joinkey");
	opt_adf(global, jn->sjn_jqual, "sjn_jqual");
	if (oj)
	{
	if (oj->oj_oqual)
	    opt_adf(global, oj->oj_oqual, "oj_oqual");
	else
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "The ADE_CX oj_oqual does nothing\n\n");
	if (oj->oj_equal)
	    opt_adf(global, oj->oj_equal, "oj_equal");
	else
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "The ADE_CX oj_equal does nothing\n\n");
	if (oj->oj_lnull)
	    opt_adf(global, oj->oj_lnull, "oj_lnull");
	else
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "The ADE_CX oj_lnull does nothing\n\n");
	if (oj->oj_rnull)
	    opt_adf(global, oj->oj_rnull, "oj_rnull");
	else
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "The ADE_CX oj_rnull does nothing\n\n");

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "oj_tidHoldFile: %5d\t oj_heldTidRow: %5d\n\n", 
	    oj->oj_tidHoldFile, oj->oj_heldTidRow);

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "oj_ijFlagsFile: %5d\t oj_ijFlagsRow: %5d\n\n", 
	    oj->oj_ijFlagsFile, oj->oj_ijFlagsRow);

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "oj_resultEQCrow: %5d\t oj_specialEQCrow: %5d\n\n", 
	    oj->oj_resultEQCrow, oj->oj_specialEQCrow);
	}
	else
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	        OPT_PBLEN, "There is no outer join\n\n");

	opt_qenode(global, jn->sjn_out, paq);
	opt_qenode(global, jn->sjn_inner, paq);
	break;
     }

     case QE_HJOIN:
     {
	QEN_HJOIN	*jn;

	jn = &qn->node_qen.qen_hjoin;
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN,
	    "hjn_hash: %5d, hjn_brow: %5d, hjn_prow: %5d\n\n", 
	    jn->hjn_hash, jn->hjn_brow, jn->hjn_prow);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "hjn_bmem: %d, hjn_kuniq: %2d, hjn_dmhcb: %5d, hjn_nullablekey:%2d\n\n", 
	    jn->hjn_bmem, jn->hjn_kuniq, jn->hjn_dmhcb, jn->hjn_nullablekey);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n\n");
	opt_satts(global, 0, NULL, jn->hjn_hkcount,
			(DMT_KEY_ENTRY **) NULL, jn->hjn_cmplist);
	opt_adf(global, jn->hjn_btmat, "hjn_btmat");
	opt_adf(global, jn->hjn_ptmat, "hjn_ptmat");
	opt_adf(global, jn->hjn_jqual, "hjn_jqual");
	if (oj)
	{
	    if (oj->oj_oqual)
		opt_adf(global, oj->oj_oqual, "oj_oqual");
	    else
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "The ADE_CX oj_oqual does nothing\n\n");
	    if (oj->oj_equal)
		opt_adf(global, oj->oj_equal, "oj_equal");
	    else
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "The ADE_CX oj_equal does nothing\n\n");
	    if (oj->oj_lnull)
		opt_adf(global, oj->oj_lnull, "oj_lnull");
	    else
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "The ADE_CX oj_lnull does nothing\n\n");
	    if (oj->oj_rnull)
		opt_adf(global, oj->oj_rnull, "oj_rnull");
	    else
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "The ADE_CX oj_rnull does nothing\n\n");

	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "oj_tidHoldFile: %5d\t oj_heldTidRow: %5d\n\n", 
		oj->oj_tidHoldFile, oj->oj_heldTidRow);

	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "oj_ijFlagsFile: %5d\t oj_ijFlagsRow: %5d\n\n", 
		oj->oj_ijFlagsFile, oj->oj_ijFlagsRow);

	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "oj_resultEQCrow: %5d\t oj_specialEQCrow: %5d\n\n", 
		oj->oj_resultEQCrow, oj->oj_specialEQCrow);
	}
	else
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	        OPT_PBLEN, "There is no outer join\n\n");

	opt_qenpqual(global, jn->hjn_pqual, "hjn_pqual");
	opt_qenode(global, jn->hjn_out, paq);
	opt_qenode(global, jn->hjn_inner, paq);
	break;
     }

     default:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "Illegal or unsupported type\n\n");
	break;
    }
}

/*
** Name: opt_qenpart - Print QEN_PART_INFO contents
**
** Description:
**	Prints the contents of a QEN_PART_INFO if a node points to one.
**	Node that potentially contain QEN_PART_INFO data are orig nodes,
**	t-join, and k-join nodes.
**
** Inputs:
**	global				Global state
**	    .ops_cstate.opc_prbuf	Trace-print buffer
**	partp				Pointer to QEN_PART_INFO
**	title				A title string
**
** Outputs:
**	None
**
** History:
**	28-Jun-2004 (schka24)
**	    Extract from in-line orig node so I can see part info for joins.
**	22-Jul-2005 (schka24)
**	    Add work bit map index.
**	16-Jun-2006 (kschendel)
**	    Changes for multi-dimensional qualification.
**	4-Aug-2007 (kschendel) SIR 122513
**	    Partition qual stuff removed to separate QP entity.
*/

static void
opt_qenpart(OPS_STATE *global, QEN_PART_INFO *partp, char *title)

{
    if (partp == NULL)
    {
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "%s: none\n\n", title);
    }
    else
    {
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "%s: 0x%p\n\n", title, partp);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "part_threads: %5d\t part_gcount: %5d\t part_totparts: %d\n\n", 
	    partp->part_threads, partp->part_gcount, partp->part_totparts);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "part_dmrix: %5d\t part_dmtix: %5d\t part_groupmap_ix: %d\n\n", 
	    partp->part_dmrix, partp->part_dmtix, partp->part_groupmap_ix);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "part_ktconst_ix: %d\t part_knokey_ix: %d\n\n", 
	    partp->part_ktconst_ix, partp->part_knokey_ix);
	if (partp->part_pcdim != (DB_PART_DIM *) NULL)
	    opt_part_dim(global, partp->part_pcdim, "part_pcdim");
    }
} /* opt_qenpart */

/* Name: opt_qenpqeval - Print partition qualification eval array
**
** Description:
**	Print one QEN_PQ_EVAL array including the CX that is (usually)
**	in the header.
**
** Inputs:
**	global				Global state
**	    .ops_cstate.opc_prbuf	Trace-print buffer
**	pqe				Pointer to QEN_PQ_EVAL header
**	title				A title string
**
** Outputs:
**	None
**
** History:
**	4-Aug-2006 (kschendel) SIR 122513
**	    Written.
*/
static void
opt_qenpqeval(OPS_STATE *global, QEN_PQ_EVAL *pqe, char *title)

{
    i4 ninstrs, n;

    if (pqe == NULL)
    {
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "%s: none\n\n", title);
	return;
    }
    ninstrs = pqe->un.hdr.pqe_ninstrs;
    n = 1;
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	OPT_PBLEN, "%s: 0x%p;  %d instructions, %d bytes\n\n", title, pqe,
	ninstrs, pqe->un.hdr.pqe_nbytes);
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	OPT_PBLEN, "DSH row for values: %d\n\n", pqe->un.hdr.pqe_value_row);
    opt_adf(global, pqe->un.hdr.pqe_cx, "partition qual");
    while (--ninstrs > 0)
    {
	pqe = (QEN_PQ_EVAL *) ((char *)pqe + pqe->pqe_size_bytes);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "pqe directive [%d]: %w (%d),  result-row: %d\n\n",
	    n, "HDR,ZERO,ANDMAP,ORMAP,NOTMAP,VALUE", pqe->pqe_eval_op,
	    pqe->pqe_eval_op, pqe->pqe_result_row);
	if (pqe->pqe_eval_op == PQE_EVAL_ANDMAP || pqe->pqe_eval_op == PQE_EVAL_ORMAP)
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "        AND/OR source row: %d\n\n",
		pqe->un.andor.pqe_source_row);
 	else if (pqe->pqe_eval_op == PQE_EVAL_VALUE)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "        nvalues: %d  of length %d, start-offset %d\n\n",
		pqe->un.value.pqe_nvalues, pqe->un.value.pqe_value_len,
		pqe->un.value.pqe_start_offset);
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "        dimension: %d,  relop %w (%d)\n\n",
		pqe->un.value.pqe_dim,
		"EQ, NE, LE, GE", pqe->un.value.pqe_relop, pqe->un.value.pqe_relop);
	}
	++n;
    }
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	OPT_PBLEN, "----- End of %s ------\n\n",title);
}

/*
** Name: opt_qenpqual - Print partition qualification struct(s)
**
** Description:
**	Print the linked list of QEN_PART_QUAL structs and the
**	associated QEN_PQ_EVAL arrays and CX's.
**
** Inputs:
**	global
**	pqual		Pointer to QEN_PART_QUAL block
**	title
**
** Outputs:
**	none
**
** History:
**	4-Aug-2007 (kschendel) SIR 122513
**	    Partition qual all different, update tracepoint output.
*/

static void
opt_qenpqual(OPS_STATE *global, QEN_PART_QUAL *pqual, char *title)

{
    i4 n = 0;

    if (pqual == NULL)
    {
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "%s: none\n\n", title);
	return;
    }
    do
    {
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "%s #%d: 0x%p (for orig node [%d])\n\n",
	    title, n, pqual, pqual->part_orig->qen_num);
	++n;
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "orig at: 0x%p    QEE_PART_QUAL cb ix: %d\n\n",
		pqual->part_orig, pqual->part_pqual_ix);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "DSH rows: constmap: %d,  lresult: %d,  work1: %d\n\n",
		pqual->part_constmap_ix, pqual->part_lresult_ix, 
		pqual->part_work1_ix);
	opt_qenpqeval(global, pqual->part_const_eval, "Const eval directives");
	opt_qenpqeval(global, pqual->part_join_eval, "Join eval directives");
	if (pqual->part_qdef != NULL)
	    opt_part_def(global, pqual->part_qdef);
	pqual = pqual->part_qnext;
    } while (pqual != NULL);
}

/*{
** Name: OPT_RDRDESC	- Find a table desc given a DSH DMR_CB number.
**
** Description:
**      This routine finds a table description given a DSH cb number for 
**      a DMR_CB. 
[@comment_line@]...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      25-Feb-88 (eric)
**          written
**	2-Dec-2004 (schka24)
**	    Prevent stack overflow on IF's.
[@history_template@]...
*/
static RDR_INFO *
opt_rdrdesc(
	OPS_STATE   *global,
	QEF_AHD	    *ahd,
	i4	    dmrcb )
{
    QEF_VALID	*valid;
    QEN_NODE	*qen;
    RDR_INFO	*rdrdesc;

    for ( ; ahd != NULL; ahd = ahd->ahd_next)
    {
	for (valid = ahd->ahd_valid; valid != NULL; valid = valid->vl_next)
	{
	    if (valid->vl_dmr_cb == dmrcb)
	    {
		return((RDR_INFO *) valid->vl_debug);
	    }
	}


        if (ahd->ahd_atype == QEA_IF)
	{
	    /* Don't follow the ahd_true branch because it's not null
	    ** terminated;  we would need some sort of opt_parent_seen
	    ** logic in the enclosing loop, because the true branch ends
	    ** up pointing back to an earlier ahd.  Instead, just return
	    ** a nothing-to-see-here for IF's.
	    */
	    return (NULL);
	}

	else if (ahd->ahd_atype != QEA_DMU && ahd->ahd_atype != QEA_RUP
			&& ahd->ahd_atype != QEA_RETURN)
	{
	    for (qen = ahd->qhd_obj.qhd_qep.ahd_postfix; 
		    qen != NULL;
		    qen = qen->qen_postfix
		)
	    {
		if (qen->qen_type == QE_QP)
		{
		    rdrdesc = 
			opt_rdrdesc(global, qen->node_qen.qen_qp.qp_act, dmrcb);

		    if (rdrdesc != NULL)
		    {
			return(rdrdesc);
		    }
		}
	    }
	}
    }

    return ((RDR_INFO *) NULL);
}

/*{
** Name: OPT_KEYS	- Trace a QEF_KEYS struct
**
** Description:
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      7-sept-86 (eric)
**          written
[@history_template@]...
*/
static VOID
opt_keys(
	OPS_STATE   *global,
	QEF_KEY	    *key )
{
    QEF_KATT	*katt;
    QEF_KAND	*kand;
    QEF_KOR	*kor;

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"    QEF_KEY: \n\n", key);
    if (key == NULL)
    {
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\t\tFULL SCAN\n\n\n");
	return;
    }

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"    key_width: %5d\n\n", key->key_width);

    for (kor = key->key_kor; kor != NULL; kor = kor->kor_next)
    {
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\tQEF_KOR: kor_numatt: %5d\n\n", kor->kor_numatt);

	for (kand = kor->kor_keys; kand != NULL; kand = kand->kand_next)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN,  "\t    QEF_KAND: kand_attno: %5d \t kand_type: %d %w\n\n",
		kand->kand_attno,
		kand->kand_type, "EQ,MIN,MAX", kand->kand_type);

	    for (katt = kand->kand_keys; katt != NULL; katt = katt->attr_next)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t\tQEF_KATT: attr_value: %d\n\n", 
		    katt->attr_value);
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t\tattr_attno: %d \t attr_operator: ",
		    katt->attr_attno);
		switch (katt->attr_operator)
		{
		 case DMR_OP_EQ:
		    TRformat(opt_scc, (i4*)NULL,
			global->ops_cstate.opc_prbuf, 
			OPT_PBLEN, "DMR_OP_EQ\n\n");
		    break;

		 case DMR_OP_LTE:
		    TRformat(opt_scc, (i4*)NULL,
			global->ops_cstate.opc_prbuf, 
			OPT_PBLEN, "DMR_OP_LTE\n\n");
		    break;

		 case DMR_OP_GTE:
		    TRformat(opt_scc, (i4*)NULL,
			global->ops_cstate.opc_prbuf, 
			OPT_PBLEN, "DMR_OP_GTE\n\n");
		    break;

		 case DMR_OP_INTERSECTS:
		    TRformat(opt_scc, (i4*)NULL,
			global->ops_cstate.opc_prbuf, 
			OPT_PBLEN, "DMR_OP_INTERSECTS\n\n");
		    break;

		 case DMR_OP_OVERLAY:
		    TRformat(opt_scc, (i4*)NULL,
			global->ops_cstate.opc_prbuf, 
			OPT_PBLEN, "DMR_OP_OVERLAY\n\n");
		    break;

		 case DMR_OP_INSIDE:
		    TRformat(opt_scc, (i4*)NULL,
			global->ops_cstate.opc_prbuf, 
			OPT_PBLEN, "DMR_OP_INSIDE\n\n");
		    break;

		 case DMR_OP_CONTAINS:
		    TRformat(opt_scc, (i4*)NULL,
			global->ops_cstate.opc_prbuf, 
			OPT_PBLEN, "DMR_OP_CONTAINS\n\n");
		    break;

		 default:
		    TRformat(opt_scc, (i4*)NULL,
			global->ops_cstate.opc_prbuf, 
			OPT_PBLEN, "Unknown operator: %d\n\n",
			katt->attr_operator);
		    break;
		}
	        TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		     OPT_PBLEN,
	       "\t\tattr_type: %5d \t attr_prec: %d \t attr_length: %5d\n\n\n",
		    katt->attr_type,
		    katt->attr_prec,
		    katt->attr_length);
	    }
	}
    }
}

/*{
** Name: OPT_SATTS	- Print out sort atts
**
** Description:
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      28-oct-86 (eric)
**          written
**	6-Dec-2005 (kschendel)
**	    Display compare-list entries too.  Display only relevant
**	    section(s) if any of atts, keys, or cmp is NULL.
[@history_template@]...
*/
static VOID
opt_satts(
	OPS_STATE	*global,
	i4		acount,
	DMF_ATTR_ENTRY	**atts,
	i4		kcount,
	DMT_KEY_ENTRY	**keys,
	DB_CMP_LIST	*cmp)
{
    DMF_ATTR_ENTRY	*att;
    DMT_KEY_ENTRY	*key;
    i4		    i;
    
    if (acount > 0 && atts != NULL)
    {
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	    "DMT_ATTR_ENTRY array:\n\n");
	for (i = 0; i < acount; i += 1)
	{
	    att = atts[i];
	    if (att == NULL)
		continue;
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "[%2d] name: %24s\n\n", i, att->attr_name.db_att_name);
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN,	
		"\ttype:%4d  len:%5d  prec:%5d  collID:%5d  flags: %d\n\n",
		att->attr_type, att->attr_size, 
		att->attr_precision, att->attr_collID, att->attr_flags_mask);
	}
    }

    if (kcount > 0 && keys != NULL)
    {
	char *orderstr;

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	    "DMT_KEY_ENTRY array:\n\n");
	for (i = 0; i < kcount; i += 1)
	{
	    key = keys[i];
	    orderstr = "Unknown key_order";
	    if (key->key_order == DMT_ASCENDING) orderstr = "DMT_ASCENDING";
	    if (key->key_order == DMT_DESCENDING) orderstr = "DMT_DESCENDING";
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "[%2d] name: %24s  order: %s\n\n", i, 
		key->key_attr_name.db_att_name, orderstr);
	}
    }

    if (kcount > 0 && cmp != NULL)
    {
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	    "DB_CMP_LIST array:\n\n");
	for (i = 0; i < kcount; i += 1)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN,	
		"[%2d] offset:%6d  type:%4d  len:%5d  prec:%5d  collID:%5d  dir:%2d\n\n",
		i,
		cmp->cmp_offset, cmp->cmp_type, cmp->cmp_length,
		cmp->cmp_precision, cmp->cmp_collID, cmp->cmp_direction);
	    ++ cmp;
	}
    }
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n\n");
}

/*{
** Name: OPT_OPKEYS	- Trace a OPC_KEY struct
**
** Description:
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      7-sept-86 (eric)
**          written
[@history_template@]...
*/
VOID
opt_opkeys(
		OPS_STATE   *global,
		OPC_QUAL    *cqual)
{
#if 0
    OPC_KOR	*kor;
    i4		i;
  
      TRdisplay("    OPC_KEY: addr: %p\n\n", key);
      if (key == NULL)
  	return;
  
     TRdisplay(
     "    opc_storage_type: %w, opc_nkeys: %x, opc_parameters: %x\n\n", 
     "x,x,x,DMT_HEAP_TYPE,x,DMT_ISAM_TYPE,x,DMT_HASH_TYPE,x,x,x,DMT_BTREE_TYPE",
  		    key->opc_storage_type, key->opc_nkeys, key->opc_parameters);
      TRdisplay("    opc_keymap: ");
      for (i = 0; i < global->ops_cstate.opc_subqry->ops_attrs.opz_av; i += 1)
      {
  	TRdisplay("%x ", key->opc_keymap[1]);
      }
  
      kor = key->opc_ors;
      do
      {
  	opt_opkor(global, key, kor);
  
  	kor = kor->opc_ornext;
      } while (kor != key->opc_ors); 
#endif
}

/*{
** Name: OPT_OPKOR	- Trace an OPC_KOR struct.
**
** Description:
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      15-OCT-86 (eric)
**          written
[@history_template@]...
*/
VOID
opt_opkor(
		OPS_STATE   *global,
		OPC_QUAL    *cqual,
		OPC_KOR	    *kor)
{
#if 0
    OPC_KAND	*kand;
    i4		i;

  	TRdisplay("\tOPC_KOR: opc_ornext: %p, opc_orprev: %p, opc_ands: %p\n\n",
  	    kor->opc_ornext, kor->opc_orprev, kor->opc_ands);
  
  	for (kand = kor->opc_ands, i = 0; 
  		i < cqual->opc_qsinfo.opc_nkeys;
  		kand += 1, i += 1
  	    )
  	{
  	    TRdisplay("\t    OPC_KAND: key number: %x\n\n", i);
  	    if (kand->opc_kle == NULL)
  	    {
  		TRdisplay("\t    opc_kle: NULL\n\n");
  	    }
  	    else
  	    {
  		TRdisplay("\t    opc_kle:\n\n");
  		adu_prdv(&kand->opc_kle->pst_sym.pst_dataval);
  	    }
  	    if (kand->opc_kge == NULL)
  	    {
  		TRdisplay("\t    opc_kge: NULL\n\n");
  	    }
  	    else
  	    {
  		TRdisplay("\t    opc_kge:\n\n");
  		adu_prdv(&kand->opc_kge->pst_sym.pst_dataval);
  	    }
  	}
#endif
}
