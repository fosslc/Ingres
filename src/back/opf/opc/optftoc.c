/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
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
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefqeu.h>
#include    <st.h>
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
#define		OPT_FTOC	TRUE
#include    <bt.h>
#include    <tr.h>
#include    <opxlint.h>
#include    <opclint.h>

/**
**
**  Name: OPTFTOC.C - Routines for tracing OPF to OPC input interface structs.
**
**  Description:
**      This file hold routines that print out the OPF data structures that 
**      are passed to OPC.  
**
**
**
**  History:
**      17-nov-87 (eric)
**          created
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**	27-jun-89 (neil)
**	    Term 2: Check validity of query mode before inspecting ops_qheader.
**	    Added code to skip empty entries in opc_grange().
**	25-may-90 (stec)
**	    Added opt_ce(), which prints contents of constant expressions in
**	    opb_bfconstants.
**	13-jul-90 (stec)
**	    Removed opt_printf(), which was non-portable. Replaced calls to the
**	    procedure with calls to TRformat(). All format strings with an '\n'
**	    at the end of the format string had an additional '\n' added, 
**	    because TRformat() strips off the first '\n' (at the end of the 
**	    format string).
**	26-nov-90 (stec)
**	    Removed globaldef opt_prbuf since this violated coding standards.
**	    We will use ops_prbuf field in OPS_STATE struct instead.
**	28-dec-90 (stec)
**	    Changed code to reflect fact that print buffer is in OPC_STATE
**	    struct.
**	08-jan-91 (stec)
**	    Modified opt_dbitmap() code.
**	11-jun-92 (anitap)
**	    Fixed su4 acc warning in opt_dbitmap(). 
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	30-jun-93 (rickh)
**	    Added TR.H.
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	11-oct-93 (johnst)
**	    Bug #56449
**	    Changed TRdisplay format args from %x to %p where necessary to
**	    display pointer types. We need to distinguish this on, eg 64-bit
**	    platforms, where sizeof(PTR) > sizeof(int).
**	15-jan-94 (ed)
**	    removed obsolete references to outer join structures
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opt_state(
	OPS_STATE *global);
static void opc_grange(
	OPS_STATE *global,
	OPV_GLOBAL_RANGE *grange);
static void opt_subquery(
	OPS_STATE *global,
	OPS_SUBQUERY *psubqry);
static void opt_rangev(
	OPS_STATE *global,
	OPS_SUBQUERY *subqry);
static void opt_jatts(
	OPS_STATE *global,
	OPS_SUBQUERY *subqry);
static void opt_fatts(
	OPS_STATE *global,
	OPS_SUBQUERY *subqry);
static void opc_eclass(
	OPS_STATE *global,
	OPS_SUBQUERY *subqry);
static void opt_msort(
	OPS_STATE *global,
	OPS_SUBQUERY *subqry);
static void opt_bfs(
	OPS_STATE *global,
	OPS_SUBQUERY *subqry);
static void opt_ojt(
	OPS_STATE *global,
	OPS_SUBQUERY *subqry);
static void opt_ce(
	OPS_STATE *global,
	OPS_SUBQUERY *subqry);
static void opt_cobfs(
	OPS_STATE *global,
	OPS_SUBQUERY *subqry,
	OPB_BMBF *bfactbm);
void opt_fcodisplay(
	OPS_STATE *global,
	OPS_SUBQUERY *subqry,
	OPO_CO *co);
static void opt_dbitmap(
	OPS_STATE *global,
	char *label,
	PTR tmap,
	i4 mapsize);

/*{
** Name: OPT_STATE	- Trace an OPS_STATE structure.
**
** Description:
**      This routine formats and displays an OPS_STATE strucuture 
[@comment_line@]...
**
** Inputs:
**  global -
**	The OPS_STATE struct to be printed out.
**
** Outputs:
**  none.
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-nov-87 (eric)
**          written
**	27-jun-89 (neil)
**	    Term 2: Check validity of query mode before inspecting ops_qheader.
**	    The statement compiled may be a random list of statements w/o any
**	    associated query tree.
[@history_template@]...
*/
VOID
opt_state(
		OPS_STATE   *global)
{
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN, 
	"***************************************************\n\n");
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN, 
	"***************************************************\n\n");
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN, 
	"**	The OPS_STATE structure			 **\n\n");
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"***************************************************\n\n");
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"***************************************************\n\n");

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"\n\n");
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"\t************************\n\n");
/*
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"\t*  The QTREE from PSF  *\n\n");
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"\t************************\n\n");
    if (global->ops_qheader != NULL)
	pst_1prmdump(global->ops_qheader->pst_qtree, global->ops_qheader,
		     &error);
*/

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
        "\n\n");
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
        "\t****************************\n\n");
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
        "\t*  The Global Range Table  *\n\n");
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
        "\t****************************\n\n");
    opc_grange(global, &global->ops_rangetab);

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"\n\n");
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"\t******************************\n\n");
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"\t*  The Main SUBQUERY Struct  *\n\n");
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"\t******************************\n\n");
    opt_subquery(global, global->ops_subquery);
}

/*{
** Name: opc_grange	- Print out a global range table.
**
** Description:
**      This routine prints out a global range table 
[@comment_line@]...
**
** Inputs:
**	global -
**	    the query wide state var
**	grange -
**	    the global range table
**
** Outputs:
**  none
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-Nov-87 (eric)
**          created
**	23-may-90 (stec)
**	    Added code to skip empty entries in the global range table
**	    to prevent AVs.
[@history_template@]...
*/
static VOID
opc_grange(
	OPS_STATE		*global,
	OPV_GLOBAL_RANGE	*grange )
{
    i4		grvno;
    OPV_GRV	*grv;
    i4		i;
    OPV_SEPARM	*separm;

    for (grvno = 0; grvno < grange->opv_gv; grvno += 1)
    {
	grv = grange->opv_base->opv_grv[grvno];

	/* The range table is not always dense, so skip empty entries. */
	if (grv == NULL)
	    continue;

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "Grange var #%d \t address: %p\n\n", grvno, grv);
	if (grv->opv_relation != NULL)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  relname: %24s, relowner: %24s\n\n", 
		&grv->opv_relation->rdr_rel->tbl_name,
		&grv->opv_relation->rdr_rel->tbl_owner);
	}
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "  gsubselect: %p \t qrt: %d\n\n",
	    grv->opv_gsubselect, grv->opv_qrt);

	switch (grv->opv_created)
	{
	 case OPS_MAIN:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  created: OPS_MAIN\n\n");
	    break;

	 case OPS_SAGG:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  created: OPS_SAGG\n\n");
	    break;

	 case OPS_FAGG:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  created: OPS_FAGG\n\n");
	    break;

	 case OPS_PROJECTION:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  created: OPS_PROJECTION\n\n");
	    break;

	 case OPS_SELECT:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  created: OPS_SELECT\n\n");
	    break;

	 case OPS_UNION:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  created: OPS_UNION\n\n");
	    break;

	 case OPS_VIEW:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  created: OPS_VIEW\n\n");
	    break;

	 case OPS_HFAGG:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  created: OPS_HFAGG\n\n");
	    break;

	 case OPS_RFAGG:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  created: OPS_RFAGG\n\n");
	    break;

	 case OPS_RSAGG:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  created: OPS_RSAGG\n\n");
	    break;
	}

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n\n");
    }

    for (grvno = 0; grvno < grange->opv_gv; grvno += 1)
    {
	grv = grange->opv_base->opv_grv[grvno];

	/* The range table is not always dense, so skip empty entries. */
	if (grv == NULL)
	    continue;

	if (grv->opv_gsubselect == NULL)
	    continue;

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\t*********************\n\n");
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\t* SUBQRY for GRV #%d *\n\n", grvno);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\t*********************\n\n");

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n*** The Global Separms ***\n\n");
	for (separm = grv->opv_gsubselect->opv_parm, i = 0;
		separm != NULL;
		separm = separm->opv_senext, i += 1
	    )
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "separm #%d\n\n", i);
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, " varno: %d \t attno: %d\n\n", 
		separm->opv_sevar->pst_sym.pst_value.pst_s_var.pst_vno,
		separm->opv_sevar->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id);
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  const param no: %d\n\n", 
		separm->opv_seconst->pst_sym.pst_value.pst_s_cnst.pst_parm_no);
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\n\n");
	}

	opt_subquery(global, grv->opv_gsubselect->opv_subquery);
    }
}

/*{
** Name: OPT_SUBQUERY	- Print out a subquery struct
**
** Description:
**      This routine prints out a subquery struct
[@comment_line@]...
**
** Inputs:
**	global -
**	    the query wide state var
**	subqry -
**	    the subquery struct
**
** Outputs:
**  none
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-Nov-87 (eric)
**          created
**	25-may-90 (stec)
**	    Added a call to opt_ce() to print out boolean constants.
[@history_template@]...
*/
static VOID
opt_subquery(
	OPS_STATE		*global,
	OPS_SUBQUERY		*psubqry )
{
    OPS_SUBQUERY	*next_subqry;
    OPS_SUBQUERY	*subqry;
    DB_ERROR		error;

    for (next_subqry = psubqry; 
	    next_subqry != NULL;
	    next_subqry = next_subqry->ops_next
	)
    {
	if (next_subqry != psubqry)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\t*********************\n\n");
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\t*  The Next SUBQRY  *\n\n");
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\t*********************\n\n");
	}

	for (subqry = next_subqry;
		subqry != NULL;
		subqry = subqry->ops_union
	    )
	{
	    if (subqry != next_subqry)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t**********************\n\n");
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t*  The Union SUBQRY  *\n\n");
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t**********************\n\n");
	    }

	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "*** Random fields ***\n\n");
	    switch (subqry->ops_sqtype)
	    {
	     case OPS_MAIN:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "  sqtype: OPS_MAIN");
		break;

	     case OPS_SAGG:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "  sqtype: OPS_SAGG");
		break;

	     case OPS_FAGG:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "  sqtype: OPS_FAGG");
		break;

	     case OPS_PROJECTION:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "  sqtype: OPS_PROJECTION");
		break;

	     case OPS_SELECT:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "  sqtype: OPS_SELECT");
		break;

	     case OPS_UNION:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "  sqtype: OPS_UNION");
		break;

	     case OPS_VIEW:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "  sqtype: OPS_VIEW");
		break;

	     case OPS_HFAGG:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "  sqtype: OPS_HFAGG");
		break;

	     case OPS_RFAGG:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "  sqtype: OPS_RFAGG");
		break;

	     case OPS_RSAGG:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "  sqtype: OPS_RSAGG");
		break;
	    }

	    switch (subqry->ops_mode)
	    {
	     case PSQ_RETRIEVE:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, 
		    OPT_PBLEN, "\t mode: PSQ_RETRIEVE\n\n");
		break;

	     case PSQ_RETINTO:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t mode: PSQ_RETINTO\n\n");
		break;

	     case PSQ_APPEND:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t mode: PSQ_APPEND\n\n");
		break;

	     case PSQ_REPLACE:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t mode: PSQ_REPLACE\n\n");
		break;

	     case PSQ_DELETE:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t mode: PSQ_DELETE\n\n");
		break;

	     case PSQ_DEFCURS:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t mode: PSQ_DEFCURS\n\n");
		break;

	     case PSQ_REPCURS:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t mode: PSQ_REPCURS\n\n");
		break;

	     default:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t mode: UNKNOWN\n\n");
		break;
	    }
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  result: %d \t\t localres: %d\n\n", 
		subqry->ops_result, subqry->ops_localres);
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  projection: %d \t byexpr: %p\n\n",
		subqry->ops_projection, subqry->ops_byexpr);
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  first: %d \t\t ", subqry->ops_first);
	    switch (subqry->ops_duplicates)
	    {
	     case OPS_DUNDEFINED:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "duplicates: OPS_DUNDEFINED\n\n");
		break;

	     case OPS_DKEEP:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "duplicates: OPS_DKEEP\n\n");
		break;

	     case OPS_DREMOVE:
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, 
		    OPT_PBLEN, "duplicates: OPS_DREMOVE\n\n");
		break;
	    }

	    opt_dbitmap(global, "  correlated", (PTR)&subqry->ops_correlated,
		sizeof (subqry->ops_correlated));

	    if (subqry->ops_root->pst_left == NULL)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\n*** There's no target list ***\n\n");
	    }
	    else
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\n*** The Target List ***\n\n");
		pst_1prmdump(subqry->ops_root->pst_left,
		    (PST_QTREE *)NULL, &error);
	    }
/*

	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\n*** The Local Range Table ***\n\n");
	    opt_rangev(global, subqry);

	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\n*** The Attribute list ***\n\n");
	    opt_jatts(global, subqry);

	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\n*** The Function Attributes ***\n\n");
	    opt_fatts(global, subqry);

	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\n*** The Equivalence Classes ***\n\n");
	    opc_eclass(global, subqry);

	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\n*** The Multi-Att Sort/Join Info ***\n\n");
	    opt_msort(global, subqry);

	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\n*** The Boolean Factors List ***\n\n");
	    opt_bfs(global, subqry);

	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\n*** Constant Expression ***\n\n");
	    opt_ce(global, subqry);

	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\n\n*** Outer Join Info ***\n\n");
	    opt_ojt(global, subqry);
*/

	    if (subqry->ops_bestco == NULL)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\n*** There's no CO Tree ***\n\n");
	    }
	    else
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\n*** The CO Tree ***\n\n");
		global->ops_trace.opt_subquery = subqry;
		opt_cotree(subqry->ops_bestco);
		opt_fcodisplay(global, subqry, subqry->ops_bestco);
	    }
	}
    }
}

/*{
** Name: OPT_RANGEV	- Trace an OPV_RANGEV structure
**
** Description:
**      The routine prints out an OPV_RANGEV structure. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    the query wide state variable.
**	subqry -
**	    The subquery struct that holds the rangev struct
**
** Outputs:
**	none
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      4-dec-87 (eric)
**          written
[@history_template@]...
*/
static VOID
opt_rangev(
	OPS_STATE	*global,
	OPS_SUBQUERY	*subqry )
{
    OPV_VARS		*var;
    OPV_IVARS		varno;
    OPV_IGVARS		grvno;
    OPV_SUBSELECT	*subsel;
    OPV_SEPARM		*separm;
    i4			i;

    for (varno = 0; varno < subqry->ops_vars.opv_rv; varno += 1)
    {
	var = subqry->ops_vars.opv_base->opv_rt[varno];

	for (grvno = 0; grvno < global->ops_rangetab.opv_gv; grvno += 1)
	{
	    if (var->opv_grv == global->ops_rangetab.opv_base->opv_grv[grvno])
	    {
		break;
	    }
	}

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "Lvarno: %d \t gvarno: %d", varno, grvno);
	if (var->opv_grv->opv_relation == NULL)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\n\n");
	}
	else
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, " \t relname: %24s\n\n", 
		&var->opv_grv->opv_relation->rdr_rel->tbl_name);
	}

	opt_dbitmap(global, "  attrmap", (PTR)&var->opv_attrmap, 
	    sizeof (var->opv_attrmap));
	opt_dbitmap(global, "  eqcmap", (PTR)&var->opv_maps.opo_eqcmap, 
	    sizeof (var->opv_maps.opo_eqcmap));
	if (varno < subqry->ops_vars.opv_prv)
	{
	    /* This is a base relation */
            TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
                OPT_PBLEN, "  indexcnt: %d \t tid: %d\n\n",
                var->opv_primary.opv_indexcnt,
                var->opv_primary.opv_tid);
	}
	else
	{
	    /* This is an index */
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  poffset: %d \t eqclass: %d \t iindex %d\n\n",
		var->opv_index.opv_poffset, 
		var->opv_index.opv_eqclass,
		var->opv_index.opv_iindex);
	}

	if (var->opv_subselect != NULL)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\n  *** The OPV_SUBSELECT fields ***");
	}

	for (subsel = var->opv_subselect; 
		subsel != NULL; 
		subsel = subsel->opv_nsubselect
	    )
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\n    sgvar: %d \t atno: %d\n\n", 
		subsel->opv_sgvar, subsel->opv_atno);
	    opt_dbitmap(global, "    eqcrequired", (PTR)subsel->opv_eqcrequired,
		sizeof (*subsel->opv_eqcrequired));
	    opt_dbitmap(global, "    eqcmp", (PTR)&subsel->opv_eqcmp,
		sizeof (subsel->opv_eqcmp));

	    for (separm = subsel->opv_parm, i = 0;
		    separm != NULL;
		    separm = separm->opv_senext, i += 1
		)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\n\tseparm #%d\n\n", i);
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t varno: %d \t attno: %d\n\n", 
		    separm->opv_sevar->pst_sym.pst_value.pst_s_var.pst_vno,
		    separm->opv_sevar->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id);
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\t const param no: %d\n\n", 
		    separm->opv_seconst->
		    pst_sym.pst_value.pst_s_cnst.pst_parm_no);
	    }
	}
    }
}

/*{
** Name: OPT_JATTS	- Trace a joinop attribute list.
**
** Description:
**      This routine traces a OPZ_ATSTATE struct. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    The query wide state variable.
**	subqry -
**	    The subquery struct that holds the attribute list.
**
** Outputs:
**	none
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      4-Dec-87 (eric)
**          written
[@history_template@]...
*/
static VOID
opt_jatts(
	OPS_STATE	*global,
	OPS_SUBQUERY	*subqry )
{
    OPZ_IATTS		attno;
    OPZ_ATTS		*att;	
    OPV_GRV		*grv;
    DMT_ATT_ENTRY	*dmt_att;

    for (attno = 0; attno < subqry->ops_attrs.opz_av; attno += 1)
    {
	att = subqry->ops_attrs.opz_base->opz_attnums[attno];
	grv = subqry->ops_vars.opv_base->opv_rt[att->opz_varnm]->opv_grv;

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "Attno: %d\n\n", attno);

	if (att->opz_attnm.db_att_id < 0 || grv->opv_relation == NULL)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  attname: FUNCTION ATT\n\n");
	}
	else if (att->opz_attnm.db_att_id == 0)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  attname: TID\n\n");
	}
	else if (att->opz_attnm.db_att_id <= 
		grv->opv_relation->rdr_rel->tbl_attr_count)
	{
	    dmt_att = grv->opv_relation->rdr_attr[att->opz_attnm.db_att_id];
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  attname: %24s \t relname: %24s\n\n", 
		dmt_att->att_nmstr,
		&grv->opv_relation->rdr_rel->tbl_name);
	}
	else
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  attname: UNKNOWN\n\n");
	}

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "  varnm: %d \t attnm: %d \t\t equcls: %d\n\n", 
	    att->opz_varnm, att->opz_attnm.db_att_id, att->opz_equcls);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN,
      "  func_att: %d \t db_datatype: %d \t db_prec: %d \t db_length: %d\n\n\n",
	    att->opz_func_att, att->opz_dataval.db_datatype, 
	    att->opz_dataval.db_prec, att->opz_dataval.db_length);
    }
}

/*{
** Name: OPT_FATTS	- Trace a funtion attribute list.
**
** Description:
**      This routine traces a OPZ_FUNTION struct. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    The query wide state variable.
**	subqry -
**	    The subquery struct that holds the attribute list.
**
** Outputs:
**	none
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      4-Dec-87 (eric)
**          written
**	31-jul-97 (inkdo01)
**	    Added a couple more fields to display.
[@history_template@]...
*/
static VOID
opt_fatts(
	OPS_STATE	*global,
	OPS_SUBQUERY	*subqry )
{
    OPZ_IATTS	    attno;
    OPZ_FATTS	    *fatt;
    DB_ERROR	    error;

    for (attno = 0; attno < subqry->ops_funcs.opz_fv; attno += 1)
    {
	fatt = subqry->ops_funcs.opz_fbase->opz_fatts[attno];

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "Fattno: %d\n\n", attno);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "  attno: %d \t mask: %x\t ojid: %d\t type : ", 
	    fatt->opz_attno, fatt->opz_mask, fatt->opz_ojid);
	switch (fatt->opz_type)
	{
	 case OPZ_NULL:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "OPZ_NULL\n\n");
	    break;

	 case OPZ_VAR:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "OPZ_VAR\n\n");
	    break;

	 case OPZ_TCONV:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "OPZ_TCONV\n\n");
	    break;

	 case OPZ_SVAR:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "OPZ_SVAR\n\n");
	    break;

	 case OPZ_MVAR:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "OPZ_MVAR\n\n");
	    break;

	 case OPZ_QUAL:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "OPZ_QUAL\n\n");
	    break;

	 default:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "UNKNOWN\n\n");
	    break;
	}

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN,
	"  db_datatype: %d \t db_prec: %d \t db_length: %d\n\n",
	    fatt->opz_dataval.db_datatype, 
	    fatt->opz_dataval.db_prec,
	    fatt->opz_dataval.db_length);
	
	opt_dbitmap(global, "  eqcm", (PTR)&fatt->opz_eqcm,
	    sizeof (fatt->opz_eqcm));
	pst_1prmdump(fatt->opz_function, (PST_QTREE *)NULL, &error);
    }
}

/*{
** Name: opc_eclass	- Trace an eq class list
**
** Description:
**      Trace an OPE_EQUIVALENCE struct. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    The query wide state variable
**	subqry -
**	    The subquery struct that hold the eqc info.
**
** Outputs:
**	none
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      4-dec-87 (eric)
**          written
**      11-feb-94 (ed)
**          check for empty eqc slots
[@history_line@]...
[@history_template@]...
*/
static VOID
opc_eclass(
	OPS_STATE	*global,
	OPS_SUBQUERY	*subqry )
{
    OPE_IEQCLS	    eqcno;
    OPE_EQCLIST	    *eqc;

    for (eqcno = 0; eqcno < subqry->ops_eclass.ope_ev; eqcno += 1)
    {
	eqc = subqry->ops_eclass.ope_base->ope_eqclist[eqcno];

	if (eqc)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "Eqcno: %d \t eqctype: %d\t eqcmask: %x\n\n", 
		eqcno, eqc->ope_eqctype, eqc->ope_mask);
	    opt_dbitmap(global, "  attrmap", (PTR)&eqc->ope_attrmap, 
		sizeof (eqc->ope_attrmap));
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\n\n");
	}
    }
}

/*{
** Name: OPT_MSORT	- Print out OPO_STSTATE info.
**
** Description:
**      Print out an OPO_STSTATE structure
[@comment_line@]...
**
** Inputs:
**	global
**	    the query wide state info
**	subqry
**	    The subquery struct that holds the opo_ststate info.
**
** Outputs:
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    Prints out info to the user
**
** History:
**      10-june-88 (eric)
**          written
[@history_template@]...
*/
static VOID
opt_msort(
	OPS_STATE	*global,
	OPS_SUBQUERY	*subqry )
{
    i4		svno;
    OPO_SORT	*sort;
    OPO_ISORT	eqorder;

    for (svno = 0; svno < subqry->ops_msort.opo_sv; svno += 1)
    {
	sort = subqry->ops_msort.opo_base->opo_stable[svno];

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "SVNO: %d\n\n", svno + subqry->ops_eclass.ope_ev);
	if (sort->opo_eqlist != NULL)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  eqlist:");
	    for (eqorder = 0; 
		    sort->opo_eqlist->opo_eqorder[eqorder] != OPE_NOEQCLS;
		    eqorder += 1
		)
	    {
		if (eqorder == 6)
		{
		    TRformat(opt_scc, (i4*)NULL,
			global->ops_cstate.opc_prbuf, OPT_PBLEN,
			"\n             ");
		}
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "    [%2d] %2d", eqorder,
		    sort->opo_eqlist->opo_eqorder[eqorder]);
	    }
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "\n\n");
	}

	if (sort->opo_bmeqcls != NULL)
	{
	    opt_dbitmap(global, "  bmeqcls", (PTR)sort->opo_bmeqcls, 
		sizeof (*sort->opo_bmeqcls));
	}
    }
}

/*{
** Name: OPZ_BFS	- Trace a boolean factor list
**
** Description:
**      Trace an OPB_BFSTATE struct. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    The query wide state variable
**	subqry -
**	    The subquery struct that hold the eqc info.
**
** Outputs:
**	none
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      4-dec-87 (eric)
**          written
[@history_template@]...
*/
static VOID
opt_bfs(
	OPS_STATE	*global,
	OPS_SUBQUERY	*subqry )
{
    OPB_IBF	    bfno;
    OPB_BOOLFACT    *bf;
    DB_ERROR	    error;

    for (bfno = 0; bfno < subqry->ops_bfs.opb_bv; bfno += 1)
    {
	bf = subqry->ops_bfs.opb_base->opb_boolfact[bfno];

	if (bf->opb_ojid != OPL_NOOUTER)
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, 
	    OPT_PBLEN, "bfno: %d\topb_ojid: %d\topb_mask: %x\n\n", bfno, 
	    bf->opb_ojid, bf->opb_mask);
	else
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, 
	    OPT_PBLEN, "bfno: %d\topb_ojid: OPL_NOOUTER\topb_mask: %x\n\n", 
	    bfno, bf->opb_mask);
	opt_dbitmap(global, "  eqcmap", (PTR)&bf->opb_eqcmap, 
	    sizeof (bf->opb_eqcmap));
	pst_1prmdump(bf->opb_qnode, (PST_QTREE *)NULL, &error);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, 
	    OPT_PBLEN, "\n\n");
    }
}

/*{
** Name: OPT_OJT	- Trace outer join info.
**
** Description:
[@comment_line@]...
**
** Inputs:
**	global -
**	    The query wide state variable
**	subqry -
**	    The subquery struct that hold the eqc info.
**
** Outputs:
**	none
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      02-aug-90 (stec)
**          written
[@history_template@]...
*/
static VOID
opt_ojt(
	OPS_STATE	*global,
	OPS_SUBQUERY	*subqry )
{
    i4		    ojno;
    OPL_OUTER	    *oj;

    for (ojno = 0; ojno < subqry->ops_oj.opl_lv; ojno++)
    {
	oj = subqry->ops_oj.opl_base->opl_ojt[ojno];

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "ojno: %d\t\tjoin type: ", ojno);
	switch (oj->opl_type)
	{
	case OPL_UNKNOWN:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "OPL_UNKNOWN\n\n");
	    break;
	case OPL_RIGHTJOIN:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "OPL_RIGHTJOIN\n\n");
	    break;
	case OPL_LEFTJOIN:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "OPL_LEFTJOIN\n\n");
	    break;
	case OPL_FULLJOIN:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "OPL_FULLJOIN\n\n");
	    break;
	case OPL_INNERJOIN:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "OPL_INNERJOIN\n\n");
	    break;
	case OPL_FOLDEDJOIN:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "OPL_FOLDEDJOIN\n\n");
	    break;
	default:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "UNEXPECTED (Error)\n\n");
	    break;
	}

	opt_dbitmap(global, "  innereqc",
	    (PTR)oj->opl_innereqc, sizeof(*oj->opl_innereqc));
	opt_dbitmap(global, "  ojtotal",
	    (PTR)oj->opl_ojtotal, sizeof(*oj->opl_ojtotal));
	opt_dbitmap(global, "  onclause",
	    (PTR)oj->opl_onclause, sizeof(*oj->opl_onclause));
	opt_dbitmap(global, "  ivmap",
	    (PTR)oj->opl_ivmap, sizeof(*oj->opl_ivmap));
	opt_dbitmap(global, "  ovmap",
	    (PTR)oj->opl_ovmap, sizeof(*oj->opl_ovmap));
	opt_dbitmap(global, "  bvmap",
	    (PTR)oj->opl_bvmap, sizeof(*oj->opl_bvmap));
	opt_dbitmap(global, "  ojbvmap",
	    (PTR)oj->opl_ojbvmap, sizeof(*oj->opl_ojbvmap));
	opt_dbitmap(global, "  idmap",
	    (PTR)oj->opl_idmap, sizeof(*oj->opl_idmap));
	opt_dbitmap(global, "  p1",
	    (PTR)oj->opl_p1, sizeof(*oj->opl_p1));
	opt_dbitmap(global, "  p2",
	    (PTR)oj->opl_p2, sizeof(*oj->opl_p2));
	opt_dbitmap(global, "  agscope",
	    (PTR)oj->opl_agscope, sizeof(*oj->opl_agscope));

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "opl_mask: %x\t opl_id: %d\t opl_gid: %d\n\n",
	    oj->opl_mask, oj->opl_id, oj->opl_gid);

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n\n");
    }
}

/*{
** Name: OPZ_CE	- Trace constant expression.
**
** Description:
**      Trace an OPB_BFSTATE struct. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    The query wide state variable
**	subqry -
**	    The subquery struct that hold the eqc info.
**
** Outputs:
**	none
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-may-90 (stec)
**          written
[@history_template@]...
*/
static VOID
opt_ce(
	OPS_STATE	*global,
	OPS_SUBQUERY	*subqry )
{
    DB_ERROR	    error;

    if (subqry->ops_bfs.opb_bfconstants != NULL)
    {
	pst_1prmdump(subqry->ops_bfs.opb_bfconstants,
	    (PST_QTREE *)NULL, &error);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n\n");
    }
}

/*{
** Name: OPZ_COBFS	- Trace a boolean factor list at a CO node
**
** Description:
**      Trace an OPB_BFSTATE struct. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    The query wide state variable
**	subqry -
**	    The subquery struct that hold the eqc info.
**
** Outputs:
**	none
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-jul-90 (stec)
**          written
[@history_template@]...
*/
static VOID
opt_cobfs(
	OPS_STATE	*global,
	OPS_SUBQUERY	*subqry,
	OPB_BMBF	*bfactbm )
{
    OPB_IBF	    bfno;
    OPB_BOOLFACT    *bf;
    DB_ERROR	    error;

    if (bfactbm == (OPB_BMBF *)NULL)
    {
	return;
    }

    for (bfno = -1;
	 (bfno = BTnext(bfno, (char *)bfactbm, sizeof(OPB_BMBF))) > -1;
	)
    {
	bf = subqry->ops_bfs.opb_base->opb_boolfact[bfno];

	if (bf->opb_ojid != OPL_NOOUTER)
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "bfno: %d\topb_ojid: %d\n\n", 
		bfno, bf->opb_ojid);
	else
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "bfno: %d\topb_ojid: OPL_NOOUTER\n\n", bfno);
	opt_dbitmap(global, "  eqcmap", (PTR)&bf->opb_eqcmap, 
	    sizeof (bf->opb_eqcmap));
	pst_1prmdump(bf->opb_qnode, (PST_QTREE *)NULL, &error);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n\n");
    }
}

/*{
** Name: OPT_FCODISPLAY	- Display a full CO tree to the users terminal.
**
** Description:
**      This routine displays all (or most) of the information that a CO tree 
**      holds in a post-order traversal form. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    The query wide state variable.
**	subqry -
**	    THe subquery struct for the CO tree.
**	co -
**	    The CO tree to be printed out.
**
** Outputs:
**	none
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      18-nov-87 (eric)
**          written
**	11-mar-92 (rickh)
**	    Tuple counts are now float4s, not integers.
[@history_template@]...
*/
VOID
opt_fcodisplay(
	OPS_STATE	*global,
	OPS_SUBQUERY	*subqry,
	OPO_CO		*co )
{
    OPV_VARS		*var;

    switch (co->opo_sjpr)
    {
     case DB_ORIG:
	if (subqry->ops_vars.opv_base->opv_rt[co->opo_union.opo_orig]->opv_subselect
								    == NULL)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "** ORIG CO node **\n\n");
	}
	else
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "** QP CO node **\n\n");
	}
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "  var: %d\n\n", co->opo_union.opo_orig);
	break;

     case DB_PR:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "** PR CO node **\n\n");
	break;

     case DB_SJ:
	if (co->opo_inner->opo_sjpr == DB_ORIG)
	{
	    var = 
		subqry->ops_vars.opv_base->opv_rt[co->opo_inner->opo_union.opo_orig];

	    if (var->opv_subselect != NULL)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "** SEJOIN CO node **\n\n");
	    }
	    else if (co->opo_sjeqc == OPE_NOEQCLS)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "** JOIN CO node **\n\n");
	    }
	    else if (co->opo_sjeqc == var->opv_primary.opv_tid)
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "** TJOIN CO node **\n\n");
	    }
	    else
	    {
		TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "** KJOIN CO node **\n\n");
	    }
	}
	else if (co->opo_inner->opo_sjpr == DB_RSORT)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "** SIJOIN CO node **\n\n");
	}
	else
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "** JOIN CO node **\n\n");
	}
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "  sjeqc: %d \t existence: %d\n\n", 
	    co->opo_sjeqc, co->opo_existence);
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "  osort: %1d \t isort: %1d\t pdups: %1d\n\n", 
	    co->opo_osort, co->opo_isort, co->opo_pdups);
	switch (co->opo_odups)
	{
	 case OPO_DREMOVE:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  odups: OPO_DREMOVE");
	    break;

	 case OPO_DDEFAULT:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  odups: OPO_DDEFAULT");
	    break;

	 default:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  odups: UNKNOWN");
	    break;
	}
	switch (co->opo_idups)
	{
	 case OPO_DREMOVE:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, " \t idups: OPO_DREMOVE\n\n");
	    break;

	 case OPO_DDEFAULT:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, " \t idups: OPO_DDEFAULT\n\n");
	    break;

	 default:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, " \t idups: UNKNOWN\n\n");
	    break;
	}
	break;

     case DB_RSORT:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "** SORT CO node **\n\n");
	switch (co->opo_odups)
	{
	 case OPO_DREMOVE:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  odups: OPO_DREMOVE\n\n");
	    break;

	 case OPO_DDEFAULT:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  odups: OPO_DDEFAULT\n\n");
	    break;

	 default:
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  odups: UNKNOWN\n\n");
	    break;
	}
	break;

     default:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "** UNKNOWN CO node **\n\n");
	break;
    }

    switch (co->opo_storage)
    {
     case DB_SORT_STORE:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "  storage: DB_SORT_STORE \t");
	break;

     case DB_HEAP_STORE:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "  storage: DB_HEAP_STORE \t");
	break;

     case DB_ISAM_STORE:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "  storage: DB_ISAM_STORE \t");
	break;

     case DB_HASH_STORE:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "  storage: DB_HASH_STORE \t");
	break;

     case DB_BTRE_STORE:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "  storage: DB_BTRE_STORE \t");
	break;

     default:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "  storage: UNKNOWN \t");
	break;
    }
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	" ordeqc: %d\n\n", co->opo_ordeqc);
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"  pagestouched: %d \t tups: %f\n\n", 
	(i4) co->opo_cost.opo_pagestouched, co->opo_cost.opo_tups);
    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf, OPT_PBLEN,
	"  psort %1d", (i4) co->opo_psort);
    switch (co->opo_odups)
    {
     case OPO_DREMOVE:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, " \t odups: OPO_DREMOVE\n\n");
	break;

     case OPO_DDEFAULT:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, " \t odups: OPO_DDEFAULT\n\n");
	break;

     default:
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, " \t odups: UNKNOWN\n\n");
	break;
    }

    if (co->opo_variant.opo_local)
    {
	if (co->opo_variant.opo_local->opo_ojid == OPL_NOOUTER)
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  ojid: OPL_NOOUTER\n\n");
	else
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, "  ojid: %d\n\n", co->opo_variant.opo_local->opo_ojid);
	opt_dbitmap(global, "  bfmap", (PTR)co->opo_variant.opo_local->opo_bmbf,
	    sizeof (*co->opo_variant.opo_local->opo_bmbf));
    }
    opt_dbitmap(global, "  eqcmap", (PTR)&co->opo_maps->opo_eqcmap, 
					sizeof (co->opo_maps->opo_eqcmap));
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, " \t eqcmap: %x, emap: %x\n\n", 
co->opo_maps, &subqry->ops_eclass.ope_maps);

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	OPT_PBLEN, "\n\n");
    
    if (co->opo_inner != NULL)
    {
	opt_fcodisplay(global, subqry, co->opo_inner);
    }
    if (co->opo_outer != NULL)
    {
	opt_fcodisplay(global, subqry, co->opo_outer);
    }
}

/*{
** Name: OPT_DBITMAP	- Display a bit map
**
** Description:
**      This routine displays a bit map. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    the query wide state variable
**	label -
**	    The name of the map;
**	map -
**	    The map to be displayed.
**	mapsize -
**	    The size of the map in bytes.
**
** Outputs:
**	none
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-nov-87 (eric)
**          created
**      18-dec-88 (seputis)
**          changed to have tmap as PTR for lint errors
**	08-jan-91 (stec)
**	    Added code to print a message when bitmap does not exist and to
**	    print bit numbers that are set. Modified code to display correctly
**	    values formatted with "%1x" - there is a bug in TRformat which is
**	    not going to be fixed, since too many people already "depend" on it.
**	    Example: this bug causes number 0x06 to be printed as 0; this is 
**	    caused by the fact that CVlx() in do_format() converts the number 
**	    to the ASCII string: "00000006" and then the first char is placed
**	    in the output buffer. This behaviour was discussed on 1/8/91 with 
**	    BobM, who owns the code, and we decided to find a workaround, which
**	    is shifting the number appropriately (see code below).
**	11-mar-92 (rickh)
**	    Re-enable the decoding of bitmaps into lists of numbers, which
**	    had been ifdef'd out.
**	11-jun-92 (anitap)
**	    Fixed su4 acc warning by explicitly casting (*map & 0x0f0) to 
**	    (u_i1). 
[@history_template@]...
*/
static VOID
opt_dbitmap(
	OPS_STATE	*global,
	char		*label,
	PTR		tmap,
	i4		mapsize )
{
    u_i1	bits;
    u_i1	*map;
    i4	parm;
    i4		i, k, len;

    map = (u_i1 *)tmap;	    /* fix lint errors */
    if (label != NULL)
    {
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "%s: ", label);
    }

    if (tmap == NULL)
    {
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "nonexistent\n");
	return;
    }

    for (len = 0; len < mapsize; map++, len++)
    {
	if (len && len % 4 == 0)
	{
	    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		OPT_PBLEN, " ");
	}

	bits = (*map & 0x0f);
	parm = bits << 28;  /* to compensate TRformat bug. */
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "%1x", parm);

	bits = (u_i1)(*map & 0x0f0) >> 4;
	parm = bits << 28;  /* to compensate TRformat bug. */
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "%1x", parm);
    }

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	OPT_PBLEN, "\n\n");

    TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	OPT_PBLEN, "  ");

    for (i = -1, k = 0;
	 (i = BTnext(i, (char *)tmap, BITSPERBYTE * mapsize)) > -1; k++ )
    {
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "%d ", i);
    }

    if (k > 0)
	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n\n");
}
