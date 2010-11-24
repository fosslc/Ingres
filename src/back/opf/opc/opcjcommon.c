/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <st.h>
#include    <cs.h>
#include    <cv.h>
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
#include    <dmucb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefqeu.h>
#include    <qefmain.h>
#include    <me.h>
#include    <bt.h>
#include    <gwf.h>
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
#define        OPC_JCOMMON	TRUE
#include    <opxlint.h>
#include    <opclint.h>

/**
**
**  Name: OPCJCOMMON.C - Routines to support compiling co nodes into QEN_JOIN nodes
**
**  Description:
**      These routines are common to the compilation of all join nodes. 
**
**  History:
**      25-aug-86 (eric)
**          created
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**	11-oct-89 (stec)
**	    Implement new QEN_NODE types and outer join.
**	    Added boolean factor bitmap ptr to the list of args
**	    passed to opc_gtqual in opc_jqual.
**	18-may-90 (stec)
**	    Fixed bug 8438 related to building keys for repeat queries.
**	24-may-90 (stec)
**	    Added checks to use OPC_EQ, OPC_ATT contents only when filled in.
**	12-jun-90 (stec)
**	    Fixed bug 30712 in opc_kjkeyinfo().
**	20-jun-90 (stec)
**	    Fixed a potential problem related to the fact that BTcount()
**	    in opc_jqual() was counting bits in the whole bitmap.
**	16-jan-91 (stec)
**	    Added missing opr_prec initialization to opc_kjkeyinfo(),
**	    opc_mvkeyatt().
**	30-may-91 (nancy)
**	    Fixed error in opr_prec initialization in opc_mvkeyatt()
**      18-mar-92 (anitap)
**          Fixed bug 42363 by initializing opr_len and opr_prec in
**          opc_kjkeyinfo(). See routine opc_cqual() in OPCQUAL.C for
**          details.
**	25-mar-92 (rickh)
**	    Right joins now store tids in a real hold file.  Some related
**	    cleanup:  removed references to oj_create and oj_load
**	    which weren't used anywhere in QEF.  Replaced references to
**	    oj_shd with oj_tidHoldFile.  Also, right fsmjoins now
**	    remember which inner tuples inner joined by keeping a hold
**	    file of booleans called oj_ijFlagsFile.
**	13-may-92 (rickh)
**	    Added cnode as an argument to opc_gtqual and opc_gtiqual.  This
**	    enables these routines to tell which outer join boolean factors
**	    they shouldn't touch.
**	11-aug-92 (rickh)
**	    Implement temporary tables for right joins.
**	13-apr-93 (rickh)
**	    More cleanup of the outer join mysteries.  Added header comments to
**	    the LNULL/RNULL cx builder.  Reduced its complexity:  now it
**	    partitions output eqcs into 4 classes:  those set to 0, those
**	    set to 1, those filled with NULL, and those refreshed from the
**	    intact child.  Replaced opc_sprow with opc_outerJoinEQCrow.
**	7-may-93 (rickh)
**	    Corrected the declaration of tidatt in opc_ojqual.  A null pointer
**	    was being passed to opc_cqual.
**      15-jun-1993 (stevet)
**          Fix for 40762.  Modified opc_kjkeyinfo() to support
**          nullable joins by generating ADE_BYCOMPARE instruction and
**          passing ADI_NULJN_OP to key build routine.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	29-Jun-1993 (jhahn)
**	    Removed code in opc_kjkeyinfo that normalized the comparand in
**	    the outer stream through its equivalance class. This caused
**	    trouble with outer joins.
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	03-feb-94 (rickh)
**	    Replaced 9999 diagnostics with real error messages.
**	07-feb-94 (rickh)
**	    Doubled the size of the outer join TID temp table so that we
**	    can split the TID into two i4s that the sorter can understand.
**	    Compiled a key and attribute list for this temp table.
**      10-nov-95 (inkdo01)
**	    Changes to support replacement of QEN_ADF instances by pointers.
**	10-nov-95 (inkdo01)
**	    Changes to support reorganization of QEF_AHD, QEN_NODE structs.
**	6-june-96 (inkdo01)
**	    Added opc_spjoin for support of spatial key joins.
**	31-oct-96 (kch)
**	    In opc_outerJoinEQCrow(), we now add one to attributeWidth to
**	    increase the spacing between row buffers. This will guarantee
**	    enough space to copy the NULL 'versions' of non-nullable datatypes
**	    filled out in opc_cojnull(). This change fixes bug 78496.
**	21-Apr-1997 (shero03)
**	    Support 3D NBR.  Call adi_dt_rtree for rtree index information
**      03-mar-98 (sarjo01)
**          Bug 89325: increase max_base size in opc_kqual(), opc_jqual()
**          and opc_iqual() to fix some underestimations for outer joins.
**          This can cause E_OP0890 errors for some queries.
**      28-jul-1998 (horda03) - X-Integration of change 433205.
**          27-nov-97 (horda03)
**            In opc_ojqual(), we now add TWO to the opc_below_rows to ensure
**            that sufficient bases are allocated for compiling the CX.
**            This change fixes bug 84131.
**	28-jul-98 (hayke02 for sarjo01)
**	    Bugs 84246, 899047: for eqc forced to nullable, make sure that
**	    cnode also reflects new datatype, size for higher nodes in plan.
**	    Added new static func opc_cojnull_adj() which promotes non-
**	    nullable result columns to nullable.
**	28-jul-98 (hayke02)
**	    In the function opc_ojnull_build(), the left and right null
**	    equivalence class maps have been modified. These maps now
**	    contain all inner equivalence classes, rather than just those
**	    equivalence classes contained in the select list. This change
**	    fixes bug 91164.
**	23-mar-99 (sarjo01)
**	    Added new param to opc_ojnull_build() to mask proj list columns
**	    for promoting to nullable. Bug 95352 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      15-oct-2003 (chash01)
**          Add rmsflag to opc_ijkqual() arg list, and test it for certain
**          actions to fix rms gateway bug 110758 
**	17-dec-04 (inkdo01)
**	    Added many collID's.
**	3-Dec-2005 (kschendel)
**	    Clean out obsolete u_i2 casts while I'm in here.
**	17-Aug-2006 (kibro01) Bug 116481
**	    Mark that opc_adtransform is not being called from pqmat
**	2-Jun-2009 (kschendel) b122118
**	    Cleanups: delete unused adbase, gtqual params; delete ahd-key;
**	    delete unused sortedness return from jinouter;
**	    replace a few copy/not/and with BTclearmask.
**	27-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-selects - add opc_find_seltype to pick up any
**	    cardinality requirements.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opc_jinouter(
	OPS_STATE *global,
	OPC_NODE *pcnode,
	OPO_CO *co,
	i4 level,
	QEN_NODE **pqen,
	OPC_EQ **pceq,
	OPC_EQ *oceq);
void opc_jkinit(
	OPS_STATE *global,
	OPE_IEQCLS *jeqcs,
	i4 njeqcs,
	OPC_EQ *ceq,
	OPE_BMEQCLS *keqcmp,
	OPC_EQ **pkceq);
void opc_jmerge(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPC_EQ *iceq,
	OPC_EQ *oceq,
	OPC_EQ *ceq);
static void opc_findtjixeqcs(
	OPS_SUBQUERY *subqry,
	OPO_CO *co,
	OPE_BMEQCLS *tjeqcmpp,
	OPV_IVARS tjvarno);
void opc_jqual(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPE_BMEQCLS *keqcmp,
	OPC_EQ *iceq,
	OPC_EQ *oceq,
	QEN_ADF **qadf,
	i4 *tidatt);
void opc_kqual(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPC_EQ *iceq,
	OPC_EQ *oceq,
	QEN_ADF **qadf);
void opc_iqual(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPC_EQ *iceq,
	QEN_ADF **qadf,
	i4 *tidatt);
void opc_ijkqual(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPC_EQ *iceq,
	OPC_EQ *oceq,
	OPE_BMEQCLS *keymap,
	QEN_ADF **qadf,
	i4 *tidatt,
	bool rmsflag);
void opc_jnotret(
	OPS_STATE *global,
	OPO_CO *co,
	OPC_EQ *ceq);
void opc_jqekey(
	OPS_STATE *global,
	OPE_IEQCLS *jeqcs,
	i4 njeqcs,
	OPC_EQ *iceq,
	OPC_EQ *oceq,
	RDR_INFO *rel,
	QEF_KEY **pqekey);
bool opc_spjoin(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPO_CO *co,
	OPC_EQ *oceq,
	OPC_EQ *ceq);
void opc_kjkeyinfo(
	OPS_STATE *global,
	OPC_NODE *cnode,
	RDR_INFO *rel,
	PST_QNODE *key_qtree,
	OPC_EQ *oceq,
	OPC_EQ *iceq,
	OPE_BMEQCLS *keymap);
static void opc_mvkeyatt(
	OPS_STATE *global,
	OPC_ADF *cadf,
	QEF_KAND ***prev_kand,
	i4 keyno,
	ADE_OPERAND *keyvalop,
	RDR_INFO *rel,
	ADI_FI_ID opid,
	i4 key_type,
	i4 key_row,
	i4 *key_offset,
	i4 pmflag,
	QEF_KOR *qekor);
bool opc_hashatts(
	OPS_STATE *global,
	OPE_BMEQCLS *keqcmp,
	OPC_EQ *oceq,
	OPC_EQ *iceq,
	DB_CMP_LIST **pcmplist,
	i4 *kcount);
void opc_ojqual(
	OPS_STATE *global,
	OPC_NODE *cnode,
	QEN_OJINFO *oj,
	OPC_EQ *iceq,
	OPC_EQ *oceq);
void opc_speqcmp(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPO_CO *child,
	OPE_BMEQCLS *eqcmp0,
	OPE_BMEQCLS *eqcmp1);
void opc_ojequal_build(
	OPS_STATE *global,
	OPC_NODE *cnode,
	QEN_OJINFO *oj,
	OPC_EQ sceq[],
	OPE_BMEQCLS *returnedEQCbitMap);
void opc_emptymap(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPO_CO *cco,
	OPE_BMEQCLS *eqcmp);
static void opc_cojnull_adj(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPE_BMEQCLS *empeqcmp);
static void opc_cojnull(
	OPS_STATE *global,
	OPC_NODE *cnode,
	QEN_ADF **oj_null,
	OPE_BMEQCLS *sp0eqcmp,
	OPE_BMEQCLS *sp1eqcmp,
	OPE_BMEQCLS *empeqcmp,
	OPC_EQ *child_ceq);
void opc_ojnull_build(
	OPS_STATE *global,
	OPC_NODE *cnode,
	QEN_OJINFO *oj,
	OPC_EQ *iceq,
	OPC_EQ *oceq,
	OPE_BMEQCLS *returnedEQCbitMap);
void opc_ojinfo_build(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPC_EQ *iceq,
	OPC_EQ *oceq);
static void opc_buildTIDsort(
	OPS_STATE *global,
	QEN_TEMP_TABLE *tempTable);
void opc_tisnull(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPC_EQ *oceq,
	OPV_IVARS innerRelation,
	QEN_ADF **tisnull);
static void opc_outerJoinEQCrow(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPC_EQ *iceq,
	OPC_EQ *oceq,
	OPC_EQ *savedEQCs,
	OPE_BMEQCLS *returnedEQCbitMap);

/*
**  Structure definitions 
*/

/*}
** Name: OPC_KJCONST - Constant info for a key join
**
** Description:
**      This structure holds the constant qtree node and the op id
**      for a key attribute that can be used in a key join. The only
**	only op id's that will currently be used are ADI_EQ_OP and 
**	ADI_ISNULL_OP. This structure
**      is used exclusivly by opc_kjkeyinfo() to be able to map from key 
**      numbers to qtree constant info. 
**
** History:
**      24-july-87 (eric)
**          created
**	03-jan-90 (stec)
**	    Removed TYPEDEF keyword preceding struct definition.
*/
typedef struct _OPC_KJCONST OPC_KJCONST;
struct _OPC_KJCONST
{
    PST_QNODE		*opc_qconst;
    PST_QNODE		*opc_qop;
    ADI_OP_ID		opc_opid;
    OPC_KJCONST		*opc_kjnext;
    i4			opc_keytype;
};

/*
**  Defines of constants used in this file
*/
#define    OPC_UNKNOWN_LEN  0
#define    OPC_UNKNOWN_PREC 0

/*{
** Name: OPC_JINOUTER	- Set up the inner or outer of a join node
**
** Description:
**      Compile a given inner or outer co node and return the eq and att 
**      information. 
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
**      16-oct-86 (eric)
**          written
**	08-nov-89 (stec)
**	    Check sortedness with respect to join key
**	    even if sort node is to be "invented".
**	12-may-2008 (dougi)
**	    Added oceq parm to support table procedures.
**	2-Jun-2009 (kschendel) b122118
**	    Nobody cared about sortedness return.  Remove the calculations.
*/
VOID
opc_jinouter(
		OPS_STATE	*global,
		OPC_NODE	*pcnode,
		OPO_CO		*co,
		i4		level,
		QEN_NODE	**pqen,
		OPC_EQ		**pceq,
		OPC_EQ		*oceq)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    OPC_NODE		cnode;
    i4			ev_size;
    bool		left_input;

    /* fill in cnode to compile the inner or outer node */
    STRUCT_ASSIGN_MACRO(*pcnode, cnode);
    cnode.opc_cco = co;
    cnode.opc_level = level;
    cnode.opc_oceq = oceq;

    /* The "level" thing is named inner/outer, but it has special meaning
    ** to the merge joins;  figure out if we're doing the left or right input.
    */
    left_input = (pcnode->opc_cco->opo_outer == co);

    if (    (pcnode->opc_cco->opo_osort == TRUE && left_input)
	    ||
	    (pcnode->opc_cco->opo_isort == TRUE && (!left_input))
	)
    {
	cnode.opc_invent_sort = TRUE;
    }

    ev_size = subqry->ops_eclass.ope_ev * sizeof (OPC_EQ);
    cnode.opc_ceq = (OPC_EQ *) opu_Gsmemory_get(global, ev_size);
    MEfill(ev_size, (u_char)0, (PTR)cnode.opc_ceq);

    /* compile the inner or outer node */
    opc_qentree_build(global, &cnode);

    /* return the information generated from the compilation */
    *pqen = cnode.opc_qennode;
    *pceq = cnode.opc_ceq;
    pcnode->opc_below_rows += cnode.opc_below_rows;

}

/*{
** Name: OPC_JKINIT	- Initialize stuff for dealing with join keys
**
** Description:
**      This routine takes an array of join eqc's and produces an eqcmap, 
**      an OPC_EQ struct for the joining attributes (and eqcs). 
[@comment_line@]...
**
** Inputs:
**	global -
**	    This is the query wide state variable
**	jeqcs -
**	    This is a pointer to an array of eqc numbers that represent
**	    the join key.
**	njeqcs -
**	    This is the size of the jeqcs array.
**	ceq -
**	    This is the description of the eqcs for the outer node of the join
**	keqcmp -
**	    This is a pointer to an uninitialized eqcmap.
**	pkceq -
**	    This is a pointer to an uninitialized pointer.
**
** Outputs:
**	keqcmp -
**	pkceq -
**	    These will be filled in with the eqc and/or joinop attribute info
**	    describing the join key info.
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
**      16-oct-86 (eric)
**          written
[@history_template@]...
*/
VOID
opc_jkinit(
		OPS_STATE	*global,
		OPE_IEQCLS	*jeqcs,
		i4		njeqcs,
		OPC_EQ		*ceq,
		OPE_BMEQCLS	*keqcmp,
		OPC_EQ		**pkceq)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    i4			ev_size;
    OPC_EQ		*kceq;
    i4			jno;
    OPE_IEQCLS		eqcno;
    OPE_BMEQCLS		*eqcmp;

    /* allocate and zero out kceq */
    ev_size = sizeof (*kceq) * subqry->ops_eclass.ope_ev;
    kceq = (OPC_EQ *) opu_Gsmemory_get(global, ev_size);
    MEfill(ev_size, (u_char)0, (PTR)kceq);

    /* Zero out keqcmp */
    MEfill(sizeof (*keqcmp), (u_char)0, (PTR)keqcmp);

    /* fill in keqcmp and kceq for each joining eqc */
    if (njeqcs == 1 && jeqcs[0] >= subqry->ops_eclass.ope_ev)
    {
	eqcno = jeqcs[0] - subqry->ops_eclass.ope_ev;
	eqcmp = subqry->ops_msort.opo_base->opo_stable[eqcno]->opo_bmeqcls;
	for (eqcno = -1; 
		(eqcno = (OPE_IEQCLS) BTnext((i4) eqcno, (char *)eqcmp, 
				    (i4)subqry->ops_eclass.ope_ev)
		) != -1 &&
		eqcno < subqry->ops_eclass.ope_ev;
	    )
	{
	    STRUCT_ASSIGN_MACRO(ceq[eqcno], kceq[eqcno]);
	}
	MEcopy((PTR) eqcmp, sizeof (*keqcmp), (PTR)keqcmp);
    }
    else
    {
	for (jno = 0; jno < njeqcs; jno++)
	{
	    eqcno = jeqcs[jno];

	    BTset((i4)eqcno, (char *)keqcmp);
	    STRUCT_ASSIGN_MACRO(ceq[eqcno], kceq[eqcno]);
	}
    }

    /* return kceq correctly */
    *pkceq = kceq;
}

/*{
** Name: OPC_JMERGE	- Merge ceq's together.
**
** Description:
**
**	This routine figures out which DSH row and offset represents each
**	equivalence class.  This routine is called during the compilation
**	of a join node and makes a determination for each equivalence class
**	exposed by the current node.
**
**	For an equivalence class that doesn't represent an equijoin at this
**	node, the determination is simple:  the eqc only comes from one of
**	of our child nodes, so just force our representation (DSH row and
**	offset) to be the same as our child's.
**
**	However, for an equivalence class that does represent an equijoin
**	at this node, we have two copies to choose from:  one from each of
**	our children.  Here's our algorithm:
**
**	    non-outer-join:
**		whichever copy is nullable
**		if both are nullable, whichever is shorter
**		if both are nullable and the same size, use the copy
**			from the rescannable stream
**	    for outer joins:
**		whichever copy is nullable
**		if both are nullable, whichever is longer
**		if both are nullable and the same length, use the copy
**			from the rescannable stream
**
**	For outer joins, we call another routine to fabricate a row
**	for special equivalence classes.  We then force special eqcs
**	into that row.
**
**
**
** Inputs:
**	global		current compiler state
**	cnode		current join node being compiled
**	iceq		"inner" equivalence classes, that is, the list
**			of equivalence classes returned by this node's
**			rescannable child
**	oceq		"outer" equivalence classes, that is, the list
**			of equivalence classes returned by this node's
**			driving child
**
** Outputs:
**	ceq		list of equivalence classes returned by this join
**			node
**
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      16-oct-86 (eric)
**          written
**	09-nov-89 (stec)
**	    Change the way an attribute representing
**	    an equivalence class is chosen. The criteria
**	    are (order is important):
**		nullable
**		shorter
**		inner
**	    but for outer joins:
**		nullable
**		longer
**		inner
**	20-apr-93 (rickh)
**	    Added header and argument descriptions for the entertainment
**	    of future generations.
**	03-feb-94 (rickh)
**	    Replaced 9999 diagnostics with real error messages.
**      08-sep-2009 (huazh01)
**          Outer join under a PST_CASEOP in a view requires the oj's 
**          opl_innereqc to be set in the available map. (b121824)
[@history_template@]...
*/
VOID
opc_jmerge(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		OPC_EQ		*iceq,
		OPC_EQ		*oceq,
		OPC_EQ		*ceq)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    OPO_CO		*co = cnode->opc_cco;
    OPE_IEQCLS		eqcno;
    DB_JNTYPE		jntype;
    bool		is_jneqc;
    OPE_BMEQCLS		jeqcmp;
    OPL_IOUTER          ojid; 

    /* Initialize jeqcmp */
    MEfill(sizeof(jeqcmp), (u_char)0, (PTR)&jeqcmp);

    ojid = co->opo_variant.opo_local->opo_ojid; 

    if (ojid != OPL_NOOUTER)
    {
	/* The outer join case. */

	/* Type of outer join */
	switch (co->opo_variant.opo_local->opo_type)
	{
	    case OPL_RIGHTJOIN:
		jntype = DB_RIGHT_JOIN;
		break;
	    case OPL_LEFTJOIN:
		jntype = DB_LEFT_JOIN;
		break;
	    case OPL_FULLJOIN:
		jntype = DB_FULL_JOIN;
		break;
	    default:
		opx_error( E_OP039A_OJ_TYPE );
		/* FIXME */
		break;
	}

	/* Fill in a map of joining eqc for outer joins */
	MEcopy((PTR)&co->opo_inner->opo_maps->opo_eqcmap, sizeof(jeqcmp), (PTR)&jeqcmp);
	BTand((i4)BITS_IN(jeqcmp), (char *)&co->opo_outer->opo_maps->opo_eqcmap,
	    (char *)&jeqcmp);
    }
    else
    {
	/* Indicates other than outer join case. */
	jntype = DB_NOJOIN;
    }

    /* First, merge the ceq lists */
    for (eqcno = 0, is_jneqc = FALSE; eqcno < subqry->ops_eclass.ope_ev;
	 eqcno++, is_jneqc = FALSE)
    {
	DB_DATA_VALUE   *idv;
	DB_DATA_VALUE   *odv;

	idv = &iceq[eqcno].opc_eqcdv;
	odv = &oceq[eqcno].opc_eqcdv;

	if ((is_jneqc = BTtest((i4)eqcno, (char *)&jeqcmp)) == TRUE)
	{
	    /* both join eqc's must be available */
	    if (oceq[eqcno].opc_eqavailable != TRUE || 
		iceq[eqcno].opc_eqavailable != TRUE
		)
	    {
		opx_error( E_OP0889_EQ_NOT_HERE );
		/* FIXME */
	    }

	    /* An assumption is made that join attributes
	    ** have the same datatype, disregarding length
	    ** and nullability.
	    */
	    {
		if (idv->db_datatype != odv->db_datatype &&
		    idv->db_datatype != -odv->db_datatype)
		{
		    opx_error( E_OP08A5_DATATYPE_MISMATCH );
		}
	    }
	}

	if (is_jneqc == TRUE && jntype == DB_LEFT_JOIN)
	{
	    STRUCT_ASSIGN_MACRO(oceq[eqcno], ceq[eqcno]);
	}
	else if (is_jneqc == TRUE && jntype == DB_RIGHT_JOIN)
	{
	    STRUCT_ASSIGN_MACRO(iceq[eqcno], ceq[eqcno]);
	}
	else  if (is_jneqc == TRUE && jntype == DB_FULL_JOIN)
	{
	    /*
	    ** ceq values assigned here are temporary;
	    ** they indicate where the data for equijoin
	    ** situation will be coming from; opc_ojinfo_build()
	    ** will initialize ceq properly.
	    */
	    if (idv->db_datatype == odv->db_datatype)
	    {
		if (idv->db_length <= odv->db_length)
		{
		    STRUCT_ASSIGN_MACRO(iceq[eqcno], ceq[eqcno]);
		}
		else
		{
		    STRUCT_ASSIGN_MACRO(oceq[eqcno], ceq[eqcno]);
		}
	    }
	    else if (idv->db_datatype < 0)
	    {
		STRUCT_ASSIGN_MACRO(iceq[eqcno], ceq[eqcno]);
	    }
	    else
	    {
		STRUCT_ASSIGN_MACRO(oceq[eqcno], ceq[eqcno]);
	    }
	}
	else
	{
	    /* regular case */
	    /*
	    ** choose one of the inner and outer atts to pass up 
	    ** and use this to fill in cnode->opc_ceq[eqcno];
	    */

	    if (oceq[eqcno].opc_eqavailable == TRUE && 
		iceq[eqcno].opc_eqavailable == TRUE
		)
	    {
		if (idv->db_datatype == odv->db_datatype)
		{
		    if (idv->db_length == odv->db_length ||
			idv->db_length < odv->db_length
		       )
		    {
			/*
			** fill in cnode->opc_ceq
			** from the inner, if available.
			*/
			if (iceq[eqcno].opc_attavailable == TRUE)
			    STRUCT_ASSIGN_MACRO(iceq[eqcno], ceq[eqcno]);
			else
			    STRUCT_ASSIGN_MACRO(oceq[eqcno], ceq[eqcno]);
		    }
		    else
		    {
			/*
			** fill in cnode->opc_ceq
			** from the outer;
			*/
			STRUCT_ASSIGN_MACRO(oceq[eqcno], ceq[eqcno]);
		    }
		}
		else if (idv->db_datatype < 0)
		{
		    /*
		    ** fill in cnode->opc_ceq
		    ** from the inner, if available.
		    */
		    if (iceq[eqcno].opc_attavailable == TRUE)
			STRUCT_ASSIGN_MACRO(iceq[eqcno], ceq[eqcno]);
		    else
			STRUCT_ASSIGN_MACRO(oceq[eqcno], ceq[eqcno]);
		}
		else
		{
		    /* fill in cnode->opc_ceq from the outer; */
		    STRUCT_ASSIGN_MACRO(oceq[eqcno], ceq[eqcno]);
		}
	    }
	    else if (oceq[eqcno].opc_eqavailable == TRUE)
	    {
		/* fill in cnode->opc_ceq from the outer; */
		STRUCT_ASSIGN_MACRO(oceq[eqcno], ceq[eqcno]);
	    }
	    else
	    {
		/* fill in cnode->opc_ceq from the inner; */
		STRUCT_ASSIGN_MACRO(iceq[eqcno], ceq[eqcno]);
	    }
	}
    }

    /* Set up for function attribute processing */
    {
	OPZ_BMATTS	*attmap;
	OPE_BMEQCLS	need_eqcmp;
	OPE_BMEQCLS	avail_eqcmp;
	OPZ_ATTS	*jatt;
	OPZ_FATTS	*fatt;
	OPZ_IATTS	attno;
	OPZ_FT		*fbase;

	fbase = subqry->ops_funcs.opz_fbase;

	MEcopy((PTR)&co->opo_inner->opo_maps->opo_eqcmap, sizeof (avail_eqcmp),
	    (char *)&avail_eqcmp);
	BTor((i4)(BITS_IN(avail_eqcmp)),
	    (char *)&cnode->opc_cco->opo_outer->opo_maps->opo_eqcmap, (char *)&avail_eqcmp);

	for (eqcno = 0; eqcno < subqry->ops_eclass.ope_ev; eqcno++)
	{
	    if (BTtest((i4)eqcno, (char *)&cnode->opc_cco->opo_maps->opo_eqcmap) &&
		ceq[eqcno].opc_eqavailable == FALSE)
	    {
		attmap = &subqry->ops_eclass.ope_base->ope_eqclist[eqcno]->ope_attrmap;

		for (attno = -1;
		     (attno = BTnext((i4)attno, (char *)attmap, 
					(i4)subqry->ops_attrs.opz_av)) != -1;
		    )
		{
		    jatt = subqry->ops_attrs.opz_base->opz_attnums[attno];

		    /* If this is a function attribute */
		    if (jatt->opz_func_att >= 0)
		    {
                        fatt = fbase->opz_fatts[jatt->opz_func_att]; 
                        MEcopy((PTR)&fatt->opz_eqcm,
				sizeof (need_eqcmp), (PTR)&need_eqcmp);

                        /* b121824 */
                        if (ojid != OPL_NOOUTER &&
                            fatt->opz_function->pst_right->pst_sym.pst_type == PST_CASEOP
                           )
                        {
                            BTor((i4)(BITS_IN(avail_eqcmp)),
                                 (char *)&subqry->ops_oj.opl_base->opl_ojt[ojid]->\
					opl_innereqc->ope_bmeqcls,
                                 (char *)&avail_eqcmp);
                        }

			if (BTsubset((char *)&need_eqcmp, (char *)&avail_eqcmp,
			    (i4)subqry->ops_eclass.ope_ev)
			   )
			{
			    ceq[eqcno].opc_eqavailable = TRUE;
			    ceq[eqcno].opc_attno = attno;
			    break;
			}
		    }
		}
	    }
	}

	/* FIXME */
	/* What should we do about special function attributes
	** (perhaps disregard them here).
	*/
    }
}

/*{
** Name: opc_findtjixeqcs - find composit of 2ndary ix eqcs for TJOIN node
**
** Description:
**	This function walks the tree that is outer to a TJOIN's OPO_CO
**	node and builds a bit map that flags the equivalence classes that
**	represent equijoins that can be considered implicit, since their
**	origin is secondary indexes for the TJOIN's base table. As such,
**	we need not build ADF code to test the equality, not only saving
**	some unnecessary tests, but also avoiding nasty issues of equijoin
**	semantics on null tuples when processing outer joins.
**
** Inputs:
**	subqry			ptr to subquery
**	co			ptr to CO-node outer to TJOIN CO-node
**	tjeqcmpp		ptr to OPE_BMEQCLS map to be filled in
**	tjvarno			varno of base table for TJOIN to match
**
** Outputs:
**	tjeqcmpp		map with matching equivalence classes flagged
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
**	24-may-05 (toumi01)
**	    Written.
[@history_line@]...
*/
static void 
opc_findtjixeqcs(
	OPS_SUBQUERY	*subqry,
	OPO_CO		*co,
    	OPE_BMEQCLS	*tjeqcmpp,
	OPV_IVARS	tjvarno)
{
    OPV_VARS	*varp;
	
    do {

	if (co->opo_sjpr == DB_ORIG)
	{
	    varp = subqry->ops_vars.opv_base->opv_rt[co->opo_union.opo_orig];
	    if ( varp->opv_mask & OPV_CPLACEMENT &&
		 varp->opv_index.opv_poffset == tjvarno )
	    {
		BTor((i4)BITS_IN(co->opo_maps->opo_eqcmap),
		    (char *)&co->opo_maps->opo_eqcmap, (char *)tjeqcmpp);
	    }
	}

	/* recurse on opo_inner */
	if (co->opo_inner)
	    opc_findtjixeqcs(subqry, co->opo_inner, tjeqcmpp, tjvarno);

    } while (co = co->opo_outer);
}

/*{
** Name: OPC_JQUAL	- Qualify a joined tuple.
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
**      16-oct-86 (eric)
**          written
**	11-oct-89 (stec)
**	    Added boolean factor bitmap ptr to the list of args
**	    passed to opc_gtqual.
**	20-jun-90 (stec)
**	    Fixed a potential problem related to the fact that BTcount()
**	    in opc_jqual was counting bits in the whole bitmap. Number
**	    of bits counted is determined by ops_eclass.ope_ev in the
**	    subquery struct.
**	12-feb-92 (rickh)
**	    In special TJOIN logic, fill in comp_tree.
**	13-may-92 (rickh)
**	    Added cnode as an argument to opc_gtqual and opc_gtiqual.  This
**	    enables these routines to tell which outer join boolean factors
**	    they shouldn't touch.
**	22-sep-92 (rickh)
**	    Prune out the outer join equijoins.  They will be enforced in
**	    the oqual and do not belong in the jqual.  It turned out that
**	    for CPJOINs we were compiling the outer join equijoins into both
**	    the oqual and the jqual.  Tsk, tsk, tsk.
**	1-may-96 (inkdo01)
**	    Mini-heuristic to prevent jqual from getting column comparison
**	    test for Rtree TID joins (since base table col doesn't exist in
**	    Rtree index).
**	28-Aug-2009 (kiria01)
**	    Pick up general selection modifiers.
**	13-Nov-2009 (kiria01) SIR 121883
**	    Distinguish between LHS and RHS card checks for 01.
[@history_template@]...
*/
VOID
opc_jqual(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		OPE_BMEQCLS	*keqcmp,
		OPC_EQ		*iceq,
		OPC_EQ		*oceq,
		QEN_ADF		**qadf,
		i4		*tidatt)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    OPO_CO	        *co = cnode->opc_cco;
    QEN_NODE		*qnode = cnode->opc_qennode;
    i4			ninstr, nops, nconst;
    i4		szconst;
    i4			max_base;
    OPC_ADF		cadf;
    OPE_BMEQCLS		inner_eqcmp;
    OPE_BMEQCLS		outer_eqcmp;
    OPE_BMEQCLS		*OJequijoins = co->opo_variant.opo_local->opo_ojoin;
    OPE_BMEQCLS		nkeqcmp;
    OPE_BMEQCLS		tjeqcmp;
    OPE_BMEQCLS		notused_eqcmp;
    i4			nknum;
    PST_QNODE		*comp_tree;
    OPE_IEQCLS		eqcno;

    /* figure out all the eqcs at this node. */
    MEfill(sizeof (inner_eqcmp), (u_char)0, (PTR)&inner_eqcmp);
    MEfill(sizeof (outer_eqcmp), (u_char)0, (PTR)&outer_eqcmp);
    for (eqcno = 0; eqcno < subqry->ops_eclass.ope_ev; eqcno += 1)
    {
	if (iceq[eqcno].opc_eqavailable == TRUE)
	{
	    BTset((i4)eqcno, (char *)&inner_eqcmp);
	}

	if (oceq[eqcno].opc_eqavailable == TRUE)
	{
	    BTset((i4)eqcno, (char *)&outer_eqcmp);
	}
    }

    /* Get the clauses out the qtree that 
    ** should be evaluated at this node.
    */
    if (qnode->qen_type == QE_TJOIN)
    {
	PST_QNODE   *t1_tree, *t2_tree;

	opc_gtqual(global, cnode,
	    cnode->opc_cco->opo_variant.opo_local->opo_bmbf,
	    &t1_tree, &notused_eqcmp);

	opc_gtqual(global, cnode,
	    cnode->opc_cco->opo_inner->opo_variant.opo_local->opo_bmbf,
	    &t2_tree, &notused_eqcmp);

	if (t1_tree != NULL && t2_tree != NULL)
	{
	    PST_QNODE *p;

	    /* Connect 2 trees */
	    for (p = t1_tree; p->pst_right != NULL; )
	    {
		p = p->pst_right;
	    }
	    p->pst_right = t2_tree;
	    comp_tree = t1_tree;
	}
	else if (t1_tree != NULL)
	{
	    comp_tree = t1_tree;
	}
	else
	{
	    comp_tree = t2_tree;
	}

    }
    else
    {
	opc_gtqual(global, cnode,
	    cnode->opc_cco->opo_variant.opo_local->opo_bmbf,
	    &comp_tree, &notused_eqcmp);
    }

    /* for TJOINs, find eqcs that we will be able to ignore as redundant when
     * we are building equijoin ADF code for non-key eqcs
     */
    if (qnode->qen_type == QE_TJOIN)
    {
	OPV_IVARS	tjvarno;
	/* TJOIN base table should be found in node immediately inner or the
	 * outer to the inner if immediate inner is DB_PR
	 */
	if (co->opo_inner->opo_sjpr == DB_ORIG)
	    tjvarno = co->opo_inner->opo_union.opo_orig;
	else
	    tjvarno = co->opo_inner->opo_outer->opo_union.opo_orig;
	MEfill(sizeof (tjeqcmp), (u_char)0, (PTR)&tjeqcmp);
	opc_findtjixeqcs(subqry, co->opo_outer, &tjeqcmp, tjvarno);
    }

    if (qnode->qen_type == QE_KJOIN)
    {
	/* We do not want any key related qualifications applied
	** here because this would screw up outer joins. Key related 
	** qualifications should have been applied by kqual CX.
	*/
	nknum  = 0;
    }
    else
    {
	QEN_NODE  *outp;
	/* figure out all the possible but non-key eqcs at this node */
	MEcopy((PTR)&inner_eqcmp, sizeof (nkeqcmp), (PTR)&nkeqcmp);
	BTand((i4)BITS_IN(nkeqcmp), (char *)&outer_eqcmp, (char *)&nkeqcmp);
	if (keqcmp != NULL)
	{
	    BTclearmask((i4)BITS_IN(nkeqcmp), (char *)keqcmp, (char *)&nkeqcmp);
	}

	/* eliminate unnecessary equijoin tests for TJOIN 2ndary index tuples */
	if (qnode->qen_type == QE_TJOIN)
	{
	    BTclearmask((i4)BITS_IN(tjeqcmp), (char *)&tjeqcmp, (char *)&nkeqcmp);
	}

	/* now prune out the outer join equijoins.  they will be dumped into
	** the oqual and do not belong in the jqual, thank you.
	*/
	if ( OJequijoins != NULL )
	{
	    BTclearmask((i4)BITS_IN(nkeqcmp), (char *)OJequijoins, (char *)&nkeqcmp);
	}

        nknum = BTcount((char *)&nkeqcmp, (i4)subqry->ops_eclass.ope_ev);
	/* Messy solution to problem of doing compares of non-existant Rtree
	** data column is to see if we're a TID join on top of an Rtree ORIG */
	if (qnode->qen_type == QE_TJOIN && 
		(outp = qnode->node_qen.qen_tjoin.tjoin_out) &&
		 (outp->qen_type == QE_ORIG ||
		  outp->qen_type == QE_TSORT && 
		 (outp = outp->node_qen.qen_tsort.tsort_out) &&
		  outp->qen_type == QE_ORIG))
	 if (outp->node_qen.qen_orig.orig_flag & ORIG_RTREE) nknum = 0;
    }
#if 0
    {
	/* Find a boolean factor or BOP & use its pst_opmeta to set any cardinality options */
	i4 i;
	for (i = 0; i < subqry->ops_bfs.opb_bv; i++)
	{
	    OPB_BOOLFACT *bf = subqry->ops_bfs.opb_base->opb_boolfact[i];
	    i4 curr_seltype = opc_find_seltype(bf->opb_qnode);
	    if (curr_seltype != PST_NOMETA &&
		(curr_seltype == -1 ||
		 curr_seltype != seltype))
		seltype = curr_seltype;
	}
	if (comp_tree)
	{
	    i4 curr_seltype = opc_find_seltype(comp_tree);
	    if (curr_seltype != PST_NOMETA &&
		(curr_seltype == -1 ||
		 curr_seltype != seltype))
		seltype = curr_seltype;
	}
	qnode->qen_flags &= ~QEN_CARD_MASK;
	switch (seltype)
	{
	case -1:
	    opx_1perror(E_OP009D_MISC_INTERNAL_ERROR,"seltype conflict");
	case PST_NOMETA:
	default:
	    /* qnode->qen_flags |= QEN_CARD_NONE */
	    break;
	case PST_CARD_ALL:
	    qnode->qen_flags |= QEN_CARD_ALL;
	    break;
	case PST_CARD_ANY:
	    qnode->qen_flags |= QEN_CARD_ANY;
	    break;
	case PST_CARD_01R:
	    qnode->qen_flags |= QEN_CARD_01R;
	    break;
	case PST_CARD_01L:
	    qnode->qen_flags |= QEN_CARD_01L;
	    break;
	}
    }
#endif
    qnode->qen_flags &= ~QEN_CARD_MASK;
    if (co->opo_card_inner == OPO_CARD_01)
	qnode->qen_flags |= QEN_CARD_01R;
    else if (co->opo_card_inner == OPO_CARD_ANY)
	qnode->qen_flags |= QEN_CARD_ANY;
    else if (co->opo_card_inner == OPO_CARD_ALL)
	qnode->qen_flags |= QEN_CARD_ALL;
    else if (co->opo_card_outer == OPO_CARD_01)
	qnode->qen_flags |= QEN_CARD_01L;
    else if (co->opo_card_own == OPO_CARD_01)
	qnode->qen_flags |= QEN_CARD_01;

    /* If there's nothing to do, then lets fill 
    ** the adf with zeros and leave.
    */
    if (nknum == 0 && comp_tree == NULL)
    {
	/* EJLFIX: change back when daved fixes null join_kcompares */
	(*qadf) = (QEN_ADF *)NULL;
	return;
    }

    /* figure out the size of the CX */
    ninstr = nops = nconst = 0;
    szconst = 0;
    max_base = cnode->opc_below_rows + 2;
    if (nknum != 0)
    {
	opc_emsize(global, &nkeqcmp, &ninstr, &nops);
    }
    opc_cxest(global, cnode, comp_tree, &ninstr, &nops, &nconst, &szconst);

    /* Begin compiling the CX */
    opc_adalloc_begin(global, &cadf, qadf, 
	ninstr, nops, nconst, szconst, max_base, OPC_STACK_MEM);

#ifdef TEST_THIS
    if (qnode->qen_type == QE_KJOIN)
    {
	/* If this is an kjoin node then add a DMF_ALLOC element to the 
	** base array and pretend that it is the base for qen_row
	** EJLFIX: clean up this hack.
	*/
	opc_adbase(global, &cadf, QEN_DMF_ALLOC, 0);
	cadf.opc_row_base[qnode->qen_row] = cadf.opc_dmf_alloc_base;
    }
#endif 

    /* Compile the comparisons of the non-key fields */
    if (nknum != 0)
    {
	opc_eminstrs(global, cnode, &cadf, &nkeqcmp, iceq, oceq);
    }

    /* now compile the qtree */
    if (comp_tree != NULL)
    {
	*tidatt = FALSE;
	opc_cqual(global, cnode, comp_tree, &cadf, ADE_SMAIN, 
	    (ADE_OPERAND *) NULL, tidatt);
    }

#ifdef TEST_THIS
    if (qnode->qen_type == QE_KJOIN && *tidatt == TRUE)
    {
	/* If we added a DMF_ALLOC base, but now realize that there are tids in
	** the qualification, then change the base to reference the row where
	** QEF, instead of DMF, will qualify the tuple.
	** EJLFIX: clean this up.
	*/
	(*qadf)->qen_input = -1;
	qbase = cadf.opc_row_base[qnode->qen_row] - ADE_ZBASE;
	(*qadf)->qen_base[qbase].qen_array = QEN_ROW;
	(*qadf)->qen_base[qbase].qen_index = qnode->qen_row;
    }
#endif 

    opc_adend(global, &cadf);
}

/*{
** Name: OPC_KQUAL	- Qualify a joined tuple.
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
**      16-oct-86 (eric)
**          written
**	11-oct-89 (stec)
**	    Added boolean factor bitmap ptr to the list of args
**	    passed to opc_gtqual.
**	 4-dec-03 (hayke02)
**	    Use ope_ev rather than sizeof(keqcmp) for the number of bits in
**	    the key eqcls map for the 'knum = BTcount(...keqcmp...)' call.
**	    This prevents an incorrect knum when we have an eqcls greater
**	    than or equal to 42 (sizeof(keqcmp)) in the key eqcls map.
**	    This change fixes problem INGSRV 2625, bug 111398.
[@history_template@]...
*/
VOID
opc_kqual(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		OPC_EQ		*iceq,
		OPC_EQ		*oceq,
		QEN_ADF		**qadf)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    i4			ninstr, nops, nconst;
    i4		szconst;
    i4			max_base;
    OPC_ADF		cadf;
    OPE_BMEQCLS		inner_eqcmp;
    OPE_BMEQCLS		outer_eqcmp;
    OPE_BMEQCLS		keqcmp;
    i4			knum;
    OPE_IEQCLS		eqcno;

    /* figure out all the eqcs at this node. */
    MEfill(sizeof (inner_eqcmp), (u_char)0, (PTR)&inner_eqcmp);
    MEfill(sizeof (outer_eqcmp), (u_char)0, (PTR)&outer_eqcmp);
    for (eqcno = 0; eqcno < subqry->ops_eclass.ope_ev; eqcno += 1)
    {
	if (iceq[eqcno].opc_eqavailable == TRUE)
	{
	    BTset((i4)eqcno, (char *)&inner_eqcmp);
	}

	if (oceq[eqcno].opc_eqavailable == TRUE)
	{
	    BTset((i4)eqcno, (char *)&outer_eqcmp);
	}

    }

    /* figure out all key eqcs at this node */
    MEcopy((PTR)&inner_eqcmp, sizeof (keqcmp), (PTR)&keqcmp);
    BTand((i4)BITS_IN(keqcmp), (char *)&outer_eqcmp, (char *)&keqcmp);
    knum = BTcount((char *)&keqcmp, (i4)subqry->ops_eclass.ope_ev);

    /* If there's nothing to do, then lets fill 
    ** the adf with zeros and leave.
    */
    if (knum == 0)
    {
	/* EJLFIX: change back when daved fixes null join_kcompares */
	(*qadf) = (QEN_ADF *)NULL;
	return;
    }

    /* figure out the size of the CX */
    ninstr = nops = nconst = 0;
    szconst = 0;
    max_base = cnode->opc_below_rows + 2;
    if (knum != 0)
    {
	opc_emsize(global, &keqcmp, &ninstr, &nops);
    }

    /* Begin compiling the CX */
    opc_adalloc_begin(global, &cadf, qadf, ninstr, nops, nconst,
	szconst, max_base, OPC_STACK_MEM);

#ifdef TEST_THIS
    if (qnode->qen_type == QE_KJOIN)
    {
	/* If this is an kjoin node then add a DMF_ALLOC element to the 
	** base array and pretend that it is the base for qen_row
	** EJLFIX: clean up this hack.
	*/
	opc_adbase(global, &cadf, QEN_DMF_ALLOC, 0);
	cadf.opc_row_base[qnode->qen_row] = cadf.opc_dmf_alloc_base;
    }
#endif 

    /* Compile the comparisons of the key fields */
    if (knum != 0)
    {
	opc_eminstrs(global, cnode, &cadf, &keqcmp, iceq, oceq);
    }

    opc_adend(global, &cadf);
}

/*{
** Name: OPC_IQUAL	- Qualify a joined tuple in DMF.
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
**      05-jun-90 (stec)
**          written
**	19-feb-92 (rickh)
**	    Only use the boolean factors of the inner child.  OPF went to
**	    some trouble to figure out that they should be executed in the
**	    orig node.
**	13-may-92 (rickh)
**	    Added cnode as an argument to opc_gtqual,  This
**	    enables this routine to tell which outer join boolean factors
**	    it shouldn't touch.
**	28-may-97 (inkdo01)
**	    Fix TID qualifier processing for key joins.
*/
VOID
opc_iqual(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		OPC_EQ		*iceq,
		QEN_ADF		**qadf,
		i4		*tidatt)
{
    QEN_NODE		*qnode = cnode->opc_qennode;
    i4			ninstr, nops, nconst;
    i4		szconst;
    i4			max_base;
    OPC_ADF		cadf;
    OPE_BMEQCLS		qual_eqcmp;
    PST_QNODE		*comp_tree;
    i4			qbase;

    /*
    ** Get the clauses out the qtree that 
    ** should be evaluated at this node.
    */
    opc_gtiqual(global, cnode,
	cnode->opc_cco->opo_inner->opo_variant.opo_local->opo_bmbf,
	&comp_tree, &qual_eqcmp);

    /*
    ** If there's nothing to do, then lets fill the adf 
    ** with zeros and leave.
    */
    if (comp_tree == NULL)
    {
	(*qadf) = (QEN_ADF *)NULL;
	return;
    }

    /* figure out the size of the CX */
    ninstr = nops = nconst = 0;
    szconst = 0;
    max_base = cnode->opc_below_rows + 2;
    opc_emsize(global, &qual_eqcmp, &ninstr, &nops);
    opc_cxest(global, cnode, comp_tree, &ninstr, &nops, &nconst, &szconst);

    /* Begin compiling the CX */
    opc_adalloc_begin(global, &cadf, qadf, ninstr, nops, nconst, szconst,
	max_base, OPC_STACK_MEM);

    /*
    ** For a kjoin node add a DMF_ALLOC element to the base
    ** array and pretend that it is the base for qen_row
    */
    opc_adbase(global, &cadf, QEN_DMF_ALLOC, 0);
    cadf.opc_row_base[qnode->qen_row] = cadf.opc_dmf_alloc_base;

    /* now compile the qtree */
    if (comp_tree != NULL)
    {
	opc_cqual(global, cnode, comp_tree, &cadf, ADE_SMAIN, 
	    (ADE_OPERAND *) NULL, tidatt);
    }

    if (qnode->qen_type == QE_KJOIN && *tidatt == TRUE)
    {
	/* If we added a DMF_ALLOC base, but now realize that there are tids in
	** the qualification, then change the base to reference the row where
	** QEF, instead of DMF, will qualify the tuple.
	** EJLFIX: clean this up.
	*/
	(*qadf)->qen_input = -1;
	qbase = cadf.opc_row_base[qnode->qen_row] - ADE_ZBASE;
	(*qadf)->qen_base[qbase].qen_array = QEN_ROW;
	(*qadf)->qen_base[qbase].qen_index = qnode->qen_row;
    }

    opc_adend(global, &cadf);
}
/*{
** Name: OPC_IJKQUAL    - Qualify a joined tuple from a non-outer key join.
**
** Description: Builds single qualification expression from all non-outer
**      key join predicates. This helps non-outer key joins to run much faster.
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**      Returns:
**          {@return_description@}
**      Exceptions:
**          [@description_or_none@]
**
** Side Effects:
**          [@description_or_none@]
**
** History:
**      14-may-99 (inkdo01)
**          Cloned from opc_jqual (and a bit of iqual & kqual).
**	19-oct-99 (hayke02)
**	    The fix for bug 82438 - TID qualifier processing for key joins
**	    - has been copied from opc_iqual() to this function. This change
**	    fixes bug 99090.
**      15-oct-2003 (chash01)
**          Add rmsflag to opc_ijkqual() arg list, and test it for certain
**          actions to fix rms gateway bug 110758 
**	 4-dec-03 (hayke02)
**	    Use ope_ev rather than sizeof(keqcmp) for the number of bits in
**	    the key eqcls map for the 'knum = BTcount(...keqcmp...)' call.
**	    This prevents an incorrect knum when we have an eqcls greater
**	    than or equal to 42 (sizeof(keqcmp)) in the key eqcls map.
**	    This change fixes problem INGSRV 2625, bug 111398.
*/
VOID
opc_ijkqual(
                OPS_STATE       *global,
                OPC_NODE        *cnode,
                OPC_EQ          *iceq,
                OPC_EQ          *oceq,
		OPE_BMEQCLS	*keymap,
                QEN_ADF         **qadf,
                i4              *tidatt,
                bool            rmsflag)
{
    OPS_SUBQUERY        *subqry = global->ops_cstate.opc_subqry;
    QEN_NODE            *qnode = cnode->opc_qennode;
    i4                  ninstr, nops, nconst;
    i4                  szconst;
    i4                  max_base;
    OPC_ADF             cadf;
    OPE_BMEQCLS         inner_eqcmp;
    OPE_BMEQCLS         outer_eqcmp;
    OPE_BMEQCLS         keqcmp;
    OPE_BMEQCLS         notused_eqcmp;
    OPE_BMEQCLS         iqual_eqcmp;
    PST_QNODE           *comp_tree;
    PST_QNODE           *icomp_tree;
    PST_QNODE           **nodepp;
    i4                  qbase, knum;
    OPE_IEQCLS          eqcno;

    /* figure out all the eqcs at this node. */
    MEfill(sizeof (inner_eqcmp), (u_char)0, (PTR)&inner_eqcmp);
    MEfill(sizeof (outer_eqcmp), (u_char)0, (PTR)&outer_eqcmp);
    for (eqcno = 0; eqcno < subqry->ops_eclass.ope_ev; eqcno += 1)
    {
        if (iceq[eqcno].opc_eqavailable == TRUE)
        {
            BTset((i4)eqcno, (char *)&inner_eqcmp);
        }

        if (oceq[eqcno].opc_eqavailable == TRUE)
        {
            BTset((i4)eqcno, (char *)&outer_eqcmp);
        }
    }

    /* figure out all key eqcs at this node */
    MEcopy((PTR)&inner_eqcmp, sizeof (keqcmp), (PTR)&keqcmp);
    BTand((i4)BITS_IN(keqcmp), (char *)&outer_eqcmp, (char *)&keqcmp);
    /* Then eliminate those for key columns (since predicates are
    ** evaluated in DMF for them). */
    if ( !rmsflag )
    {
        BTnot((i4)BITS_IN(*keymap), (char *)keymap);
        BTand((i4)BITS_IN(keqcmp), (char *)keymap, (char *)&keqcmp);
    }
    knum = BTcount((char *)&keqcmp, (i4)subqry->ops_eclass.ope_ev);

    /* Get the clauses out the qtree that apply only to the inner table
    ** which should be evaluated at this node.
    */
    opc_gtiqual(global, cnode,
            cnode->opc_cco->opo_inner->opo_variant.opo_local->opo_bmbf,
            &icomp_tree, &iqual_eqcmp);

    /* Get the non-key join clauses out the qtree that 
    ** should be evaluated at this node.
    */
    opc_gtqual(global, cnode,
            cnode->opc_cco->opo_variant.opo_local->opo_bmbf,
            &comp_tree, &notused_eqcmp);

    /* If there's nothing to do, then lets fill 
    ** the adf with zeros and leave.
    */
    if (comp_tree == NULL && icomp_tree == NULL && knum == 0)
    {
        /* EJLFIX: change back when daved fixes null join_kcompares */
        (*qadf) = (QEN_ADF *)NULL;
        return;
    }
    else if (comp_tree != NULL)
    {
        if (icomp_tree != NULL) {
            /* If comp_tree and icomp_tree are both non-null, merge them (by
            ** running down icomp_tree, and attaching its bottommost AND to
            ** the top of comp_tree). */

            for (nodepp = &icomp_tree->pst_right; *nodepp != NULL;
                                nodepp = &(*nodepp)->pst_right);
            (*nodepp) = comp_tree;
        } 
        else 
        {
            /* "Merge" the empty icomp_tree and comp_tree */
            icomp_tree = comp_tree;
        }
    }

    /* figure out the size of the CX */
    ninstr = nops = nconst = 0;
    szconst = 0;
    max_base = cnode->opc_below_rows + 2;
    opc_emsize(global, &iqual_eqcmp, &ninstr, &nops);
    if (knum > 0)
     opc_emsize(global, &keqcmp, &ninstr, &nops);
    opc_cxest(global, cnode, icomp_tree, &ninstr, &nops, &nconst, &szconst);

    /* Begin compiling the CX */
    opc_adalloc_begin(global, &cadf, qadf, 
        ninstr, nops, nconst, szconst, max_base, OPC_STACK_MEM);

    /*
    ** For a kjoin node add a DMF_ALLOC element to the base
    ** array and pretend that it is the base for qen_row
    */
    opc_adbase(global, &cadf, QEN_DMF_ALLOC, 0);
    cadf.opc_row_base[qnode->qen_row] = cadf.opc_dmf_alloc_base;

    /* now compile the qtree */
    *tidatt = FALSE;
    if (icomp_tree)
	opc_cqual(global, cnode, icomp_tree, &cadf, ADE_SMAIN, 
	    (ADE_OPERAND *) NULL, tidatt);

    /* Finally, compile the comparisons of the key fields 
    ** (the kqual code). */
    if (knum != 0)
    {
        opc_eminstrs(global, cnode, &cadf, &keqcmp, iceq, oceq);
    }

    if (*tidatt == TRUE)
    {
	/* If we added a DMF_ALLOC base, but now realize that there are tids in
	** the qualification, then change the base to reference the row where
	** QEF, instead of DMF, will qualify the tuple.
	** EJLFIX: clean this up.
	*/
	(*qadf)->qen_input = -1;
	qbase = cadf.opc_row_base[qnode->qen_row] - ADE_ZBASE;
	(*qadf)->qen_base[qbase].qen_array = QEN_ROW;
	(*qadf)->qen_base[qbase].qen_index = qnode->qen_row;
    }

    opc_adend(global, &cadf);
}


/*{
** Name: OPC_JNOTRET	- Eliminate atts that aren't returned from this join
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
**      16-oct-86 (eric)
**          written
[@history_template@]...
*/
VOID
opc_jnotret(
		OPS_STATE	*global,
		OPO_CO		*co,
		OPC_EQ		*ceq)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    OPE_BMEQCLS		not_eqcmp;
    OPE_IEQCLS		eqcno;

    MEcopy((PTR)&co->opo_maps->opo_eqcmap, sizeof (not_eqcmp), (PTR)&not_eqcmp);
    BTnot((i4)BITS_IN(not_eqcmp), (char *)&not_eqcmp);
    for (eqcno = -1; 
	    (eqcno = (OPE_IEQCLS) BTnext((i4) eqcno, (char *)&not_eqcmp, 
				    (i4)subqry->ops_eclass.ope_ev)) != -1 &&
		eqcno < subqry->ops_eclass.ope_ev;
	)
    {
	ceq[eqcno].opc_eqavailable = FALSE;
	ceq[eqcno].opc_attavailable = FALSE;
    }
}

/*{
** Name: OPC_JQEKEY	- Fill in a QEF_KEY struct for a join (kjoin) node.
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
**      20-oct-86 (eric)
**          written
**	24-may-90 (stec)
**	    Added checks to use OPC_EQ, OPC_ATT contents only when filled in.
**	03-feb-94 (rickh)
**	    Replaced 9999 diagnostics with real error messages.
[@history_template@]...
*/
VOID
opc_jqekey(
		OPS_STATE	*global,
		OPE_IEQCLS	*jeqcs,
		i4		njeqcs,
		OPC_EQ		*iceq,
		OPC_EQ		*oceq,
		RDR_INFO	*rel,
		QEF_KEY		**pqekey)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    QEF_KEY		*qekey;
    OPZ_IATTS		ojattno;
    OPZ_IATTS		ijattno;
    QEF_KAND		*kand;
    QEF_KAND		**prev_kand;
    i4			jno;
    OPZ_ATTS		*oattnum;
    OPZ_ATTS		*iattnum;

    /* if there are no keys in this join, then QEF doesn't want a QEF_KEY
    ** struct.
    */
    if (njeqcs < 1)
    {
	*pqekey = NULL;
	/* EJLFIX: Put in an error here, since we shouldn't be generating
	** a join key with no attributes.
	*/
	return;
    }

    /* allocate and init a QEF_KEY; */
    qekey = *pqekey = (QEF_KEY *) opu_qsfmem(global, sizeof (QEF_KEY));
    qekey->key_width = njeqcs;

    /* allocate and init a QEF_KOR; */
    qekey->key_kor = (QEF_KOR *) opu_qsfmem(global, sizeof (QEF_KOR));
    qekey->key_kor->kor_next = NULL;
    qekey->key_kor->kor_keys = NULL;
    qekey->key_kor->kor_numatt = njeqcs;

    prev_kand = &qekey->key_kor->kor_keys;
    for (jno = 0; jno < njeqcs; jno += 1)
    {
	if (oceq[jeqcs[jno]].opc_eqavailable == FALSE ||
	    iceq[jeqcs[jno]].opc_eqavailable == FALSE
	   )
	{
	    opx_error(E_OP0889_EQ_NOT_HERE);
	}
	if (oceq[jeqcs[jno]].opc_attavailable == FALSE ||
	    iceq[jeqcs[jno]].opc_attavailable == FALSE
	   )
	{
	    opx_error(E_OP0888_ATT_NOT_HERE);
	}

	ojattno = oceq[jeqcs[jno]].opc_attno;
	if (ojattno == OPC_NOATT)
	{
	    /* FIXME */
	    opx_error( E_OP08A6_NO_ATTRIBUTE );
	}

	ijattno = iceq[jeqcs[jno]].opc_attno;
	if (ijattno == OPC_NOATT)
	{
	    /* FIXME */
	    opx_error( E_OP08A6_NO_ATTRIBUTE );
	}

	oattnum = subqry->ops_attrs.opz_base->opz_attnums[ojattno];
	iattnum = subqry->ops_attrs.opz_base->opz_attnums[ijattno];

	/* allocate and init a QEF_KAND; */
	*prev_kand = kand = (QEF_KAND *) opu_qsfmem(global, sizeof (QEF_KAND));
	prev_kand = &kand->kand_next;

	kand->kand_type = QEK_EQ;
	kand->kand_next = NULL;
	kand->kand_attno = iattnum->opz_attnm.db_att_id;

	/* allocate and init a QEN_KATT; */
	kand->kand_keys = (QEF_KATT *) opu_qsfmem(global, sizeof (QEF_KATT));
	kand->kand_keys->attr_attno = kand->kand_attno;
	kand->kand_keys->attr_type = (i4) oceq[jeqcs[jno]].opc_eqcdv.db_datatype;
	kand->kand_keys->attr_length = (i4) oceq[jeqcs[jno]].opc_eqcdv.db_length;
	kand->kand_keys->attr_prec = (i2) oceq[jeqcs[jno]].opc_eqcdv.db_prec;
	kand->kand_keys->attr_collID = (i2) oceq[jeqcs[jno]].opc_eqcdv.db_collID;
	kand->kand_keys->attr_operator = DMR_OP_EQ;

	/* Fill attr_value with the offset to the key data value, even
	** though the QEF document doesn't say to do this.
	*/
	kand->kand_keys->attr_value = oceq[jno].opc_dshoffset;
	kand->kand_keys->attr_next = NULL;
    }
}

/*{
** Name: OPC_SPJOIN	- build key join code for Rtree inners
**
** Description:
**      This routine builds code to materialize a spatial column value for   
**	the purpose of performing a key join to an Rtree secondary index.
**	(Currently, the index returns TIDs to be used to retrieve base 
**	table rows in a higher TID join on which spatial joins are 
**	performed with the outer table of this key join.) The NBR function 
**	is executed on the outer column value to project it into the key 
**	buffer, since this is what is used to probe the Rtree. A copy of
**	the original column value is also placed in the key buffer, and 
**	key compare code is built to detect consecutive outer rows with the 
**	same column value. Finally, a QEF key structure is built to define
**	the keying operation.
[@comment_line@]...
**
** Outputs:
**      None
**	Returns:
**	    TRUE, if the code is successfully built, otherwise FALSE.
**	Exceptions:
**	    Error E_OP08AD_SPATIAL_ERROR has been defined, if we have need
**	    for a general purpose exception code. 
**
** Side Effects:
**	    none
**
** History:
**      5-june-96 (inkdo01)
**          written
**	21-Apr-1997 (shero03)
**	    Support 3D NBR.  Call adi_dt_rtree for rtree index information
**	4-nov-97 (inkdo01)
**	    Remove comment delimiters from eqavailable reference (whyever
**	    they were there in the first place??!?)
[@history_template@]...
*/
bool opc_spjoin (OPS_STATE	*global,
		OPC_NODE	*cnode,
		OPO_CO		*co,
		OPC_EQ		*oceq,
		OPC_EQ		*ceq)

{
    PST_QNODE	range; 		/* range constant for NBR call */
    PST_QNODE	*spatbop;	/* spatial func's parse tree frag. */
    
    OPS_SUBQUERY *subqry = global->ops_cstate.opc_subqry;
    QEF_QP_CB	*qp = global->ops_cstate.opc_qp;
				/* current action header */
    OPC_ADF	key_cadf;	/* key materialize ADF descriptor */
    OPC_ADF	kcomp_cadf;	/* key compare ADF descriptor */
    i4		keyrow;		/* key buffer number */
    QEN_NODE	*kj = cnode->opc_qennode;
				/* our KJOIN node */
    
    QEF_KEY	*qekey;		/* key structure ptrs */
    QEF_KOR	*qekor;
    QEF_KAND	*qekand;
    QEF_KATT	*qekatt;

    OPV_IVARS	rvno;		/* local rt no of Rtree index */
    OPV_VARS	*rvarp;		/* ... and var ptr */
    OPZ_IATTS	jattno, ojattno;  /* joinop attr workfields */
    OPZ_ATTS	*jattrp;
    OPE_IEQCLS	eqc, oeqc, ieqc;  /* eqclass workfields */
    OPE_EQCLIST	*eqclp;
    OPB_IBF	bfi;		/* Boolean factor workfields */
    OPB_BOOLFACT *bfp;
    i4		maxattrs = subqry->ops_attrs.opz_av;
    i4		maxeqcs = subqry->ops_eclass.ope_ev;

    ADE_OPERAND	ops[3];
    ADI_OP_ID	inside_op, overlaps_op, intersects_op;
    ADI_DT_RTREE rtree_blk;	/* Information given the datatype of col indexed*/
    i4		dummy;

    /* Lookup various spatial codes (they're not intrinsics, so we have to ask
    ** for them). */
    if (adi_opid(global->ops_adfcb, "inside", &inside_op) ||
    	adi_opid(global->ops_adfcb, "intersects", &intersects_op) ||
    	adi_opid(global->ops_adfcb, "overlaps", &overlaps_op))
				/* first, load relevant "methods" operators */
    	return(FALSE);		/*  If any fail, return ** error */

    /* Locate the outer column which seeds the key join. This done by locating
    ** the indexed joinop attr, locating the spatial join eqclass containing 
    ** that joinop attr, then picking up the outer column's joinop attr number 
    ** from the spatial join eqclass attrmap field */

    rvno = co->opo_inner->opo_union.opo_orig;	/* local RT no for index */
    rvarp = subqry->ops_vars.opv_base->opv_rt[rvno];
    for (jattno = 0; jattno < maxattrs; jattno++)
     if ((jattrp = subqry->ops_attrs.opz_base->opz_attnums[jattno])->opz_varnm
		== rvno && jattrp->opz_attnm.db_att_id == 1) break;
					/* loop over joinop attr table, looking 
					** for col 1 of Rtree index */

    if (jattno >= maxattrs) return(FALSE);
					/* if joinop attr couldn't be found */

    for (eqc = 0; eqc < maxeqcs; eqc++)
     if ((eqclp = subqry->ops_eclass.ope_base->ope_eqclist[eqc])->ope_mask &
		OPE_SPATJ && BTtest((i4)jattno, (char *)&eqclp->ope_attrmap))
    {					/* got spatial eqclass on our index */
	if ((ojattno = BTnext((i4)-1, (char *)&eqclp->ope_attrmap, maxattrs)) 
		== jattno)
		ojattno = BTnext((i4)ojattno, (char *)&eqclp->ope_attrmap, 
		maxattrs);		/* first attr which ISN'T the index col
					** ought to be the outer col */
	break;
    }
    if (eqc >= maxeqcs || ojattno < 0) return(FALSE);
    oeqc = subqry->ops_attrs.opz_base->opz_attnums[ojattno]->opz_equcls;
					/* outer column's eqclass */
    if (oceq[oeqc].opc_eqavailable == FALSE) return(FALSE);
					/* make sure outer col is available */

    /* Locate Boolean factor from which join is derived. Loop over boolfact's 
    ** to find one whose opb_eqcmap contains both oeqc and ieqc. */

    ieqc = jattrp->opz_equcls;		/* index (& indexed) col's eqclass */
    spatbop = NULL;
    for (bfi = 0; bfi < subqry->ops_bfs.opb_bv; bfi++)
     if ((bfp = subqry->ops_bfs.opb_base->opb_boolfact[bfi])->opb_mask &
		OPB_SPATJ && BTtest((i4)ieqc, (char *)&bfp->opb_eqcmap) &&
		BTtest((i4)oeqc, (char *)&bfp->opb_eqcmap))
    {
	spatbop = bfp->opb_qnode;
	if (spatbop->pst_sym.pst_type == PST_BOP && 
	    (spatbop->pst_sym.pst_value.pst_s_op.pst_opno == inside_op ||
	    spatbop->pst_sym.pst_value.pst_s_op.pst_opno == intersects_op ||
	    spatbop->pst_sym.pst_value.pst_s_op.pst_opno == overlaps_op))
	 break;				/* infix spatial operator */
	spatbop = (spatbop->pst_left->pst_sym.pst_type == PST_BOP) ?
		spatbop->pst_left : spatbop->pst_right;
					/* otherwise choose spatbop assuming 
					** "func(...) eq 1" notation */
	break;
    }
    if (spatbop == NULL || spatbop->pst_sym.pst_type != PST_BOP)
		return(FALSE);		/* make sure we got it */

    /* We now have the outer column descriptor. Start piecing together the 
    ** key buffer materialization and comparison code. */

    /* Find the length of an NBR (the legitimate way) */
 
    if (adi_dt_rtree(global->ops_adfcb, 
    			oceq[oeqc].opc_eqcdv.db_datatype,
			&rtree_blk))
    	return(FALSE);

    /* Initiate key build and key compare ADF code chunks and define the
    ** key buffer. */

    opc_adalloc_begin(global, &key_cadf, &kj->node_qen.qen_kjoin.kjoin_key, 
	2, 6, 1, 12, 3, OPC_STACK_MEM);	/* key materialize expression */
    opc_adalloc_begin(global, &kcomp_cadf, &kj->node_qen.qen_kjoin.kjoin_kcompare, 
	1, 4, 0, 0, 3, OPC_STACK_MEM);	/* key compare expression */
    opc_ptrow(global, &keyrow, oceq[oeqc].opc_eqcdv.db_length + 
	(rtree_blk.adi_hilbertsize * 2)); 	/* get a row buffer for key */
    opc_adbase(global, &key_cadf, QEN_ROW, keyrow);
    kj->node_qen.qen_kjoin.kjoin_key->qen_output = keyrow;
    opc_adbase(global, &kcomp_cadf, QEN_ROW, keyrow);
					/* tell everyone about it */

    /* Build operand descriptor for source outer column. */
    ops[1].opr_dt = oceq[oeqc].opc_eqcdv.db_datatype;
    ops[1].opr_prec = oceq[oeqc].opc_eqcdv.db_prec;
    ops[1].opr_len = oceq[oeqc].opc_eqcdv.db_length;
    ops[1].opr_collID = oceq[oeqc].opc_eqcdv.db_collID;
    ops[1].opr_offset = oceq[oeqc].opc_dshoffset;
    opc_adbase(global, &key_cadf, QEN_ROW, oceq[oeqc].opc_dshrow);
    opc_adbase(global, &kcomp_cadf, QEN_ROW, oceq[oeqc].opc_dshrow);
					/* define outer row buffer */
    ops[1].opr_base = key_cadf.opc_row_base[oceq[oeqc].opc_dshrow];
    
    /* Now stick the "default range" parm to the NBR call into the
    ** constant buffer */
    range.pst_left = (PST_QNODE *)NULL;
    range.pst_right = (PST_QNODE *)NULL;
    range.pst_sym.pst_type = PST_CONST;
    range.pst_sym.pst_dataval.db_datatype = rvarp->opv_grv->opv_relation->
		rdr_rel->tbl_rangedt;
    range.pst_sym.pst_dataval.db_data = (char *)rvarp->opv_grv->opv_relation->
		rdr_rel->tbl_range;	/* range value from iirange */
    range.pst_sym.pst_dataval.db_length = rvarp->opv_grv->opv_relation->
		rdr_rel->tbl_rangesize;	/* (should be tbl_dim**tbl_dim*8) */
    range.pst_sym.pst_dataval.db_prec = 0;
    range.pst_sym.pst_dataval.db_collID = -1;
    MEfill(sizeof(range.pst_sym.pst_value.pst_s_cnst), (u_char)0,
	&range.pst_sym.pst_value.pst_s_cnst);
    range.pst_sym.pst_value.pst_s_cnst.pst_tparmtype = PST_USER;
    range.pst_sym.pst_value.pst_s_cnst.pst_cqlang = DB_SQL;
    ops[2].opr_dt = rtree_blk.adi_range_dtid; 
    ops[2].opr_len = OPC_UNKNOWN_LEN;
    ops[2].opr_prec = OPC_UNKNOWN_PREC;
    ops[2].opr_collID = -1;
    opc_cqual(global, cnode, &range, &key_cadf, ADE_SVIRGIN, &ops[2], &dummy);

    /* Set up NBR output operand, and load up the NBR instruction.
    ** This NBRizes the constant/parm into the key buffer for QEF. */
    ops[0].opr_dt = rtree_blk.adi_nbr_dtid;
    ops[0].opr_len = rtree_blk.adi_hilbertsize * 2;
    if (ops[1].opr_dt < 0)
    {		/* outer col is nullable */
	ops[0].opr_dt = -ops[0].opr_dt;		/* make nbr rslt nullable */
	kj->node_qen.qen_kjoin.kjoin_nloff = ops[0].opr_len;	
						/* save nullind offset */
	ops[0].opr_len++;
    }
    ops[0].opr_prec = 0;
    ops[0].opr_collID = -1;
    ops[0].opr_base = key_cadf.opc_row_base[keyrow];
    ops[0].opr_offset = 0;
    opc_adinstr(global, &key_cadf, rtree_blk.adi_nbr_fiid, ADE_SMAIN, 3, ops, 1);

    /* Then copy source outer column value for later compare code. */
    ops[0].opr_dt = ops[1].opr_dt;
    ops[0].opr_offset = ops[0].opr_len;  
    ops[0].opr_len = ops[1].opr_len;
    opc_adinstr(global, &key_cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 1);
    opc_adend(global, &key_cadf);	/* finish him off */

    /* Do the key compare expression, now. */

    ops[0].opr_base = kcomp_cadf.opc_row_base[keyrow];
    ops[1].opr_base = kcomp_cadf.opc_row_base[oceq[oeqc].opc_dshrow];
					/* establish operand buffers */
    opc_adinstr(global, &kcomp_cadf, ADE_COMPARE, ADE_SMAIN, 2, ops, 0);
    opc_adend(global, &kcomp_cadf);	/* not much to the compare */

    /* All that's left (?) is to allocate and format the QEF key 
    ** structure to define the Rtree probe */
    qekey = (QEF_KEY *)opu_qsfmem(global, (i4)(sizeof(QEF_KEY) +
	sizeof(QEF_KOR) + sizeof(QEF_KAND) + sizeof(QEF_KATT)));
					/* alloc, then chop into pieces */
    qekor = (QEF_KOR *)((char *)qekey + (i4)sizeof(QEF_KEY));
    qekand = (QEF_KAND *)((char *)qekor + (i4)sizeof(QEF_KOR));
    qekatt = (QEF_KATT *)((char *)qekand + (i4)sizeof(QEF_KAND));

    kj->node_qen.qen_kjoin.kjoin_kys = qekey;  /* attach to kjoin */
    qekey->key_kor = qekor;		/* qekey->qekor */
    qekey->key_width = 1;		/* always "1" */

    qekor->kor_next = (QEF_KOR *)NULL;
    qekor->kor_keys = qekand;		/* qekor->qekand */
    qekor->kor_id = qp->qp_kor_cnt++;
					/* update QP key count */
    qekor->kor_numatt = 1;

    qekand->kand_type = QEK_EQ;
    qekand->kand_next = (QEF_KAND *)NULL;
    qekand->kand_attno = 1;		/* MBR is col 1 of Rtree */
    qekand->kand_keys = qekatt;		/* qekand->qekatt */

    qekatt->attr_attno = 1;
    qekatt->attr_type = rtree_blk.adi_nbr_dtid;
    qekatt->attr_length = rtree_blk.adi_hilbertsize * 2;
    qekatt->attr_prec = 0;
    qekatt->attr_collID = -1;
    qekatt->attr_value = 0;
    qekatt->attr_next = (QEF_KATT *)NULL;

    /* Turn the spatial function opcode into DMF Rtree probe code */
    if (spatbop->pst_sym.pst_value.pst_s_op.pst_opno == intersects_op)
	qekatt->attr_operator = DMR_OP_INTERSECTS;
    else if (spatbop->pst_sym.pst_value.pst_s_op.pst_opno ==
	overlaps_op) qekatt->attr_operator = DMR_OP_OVERLAY;
    else if (spatbop->pst_sym.pst_value.pst_s_op.pst_opno ==
	inside_op)
    {
	/* "inside" is executed as INSIDE if the Rtree col is the right
	** operand and CONTAINS if it's the left operand */
	if (spatbop->pst_left->pst_sym.pst_type == PST_VAR &&
	    spatbop->pst_left->pst_sym.pst_value.pst_s_var.pst_vno == 
	    subqry->ops_vars.opv_base->opv_rt[rvno]->opv_index.opv_poffset)
	    qekatt->attr_operator = DMR_OP_INSIDE;
	else qekatt->attr_operator = DMR_OP_CONTAINS;
    }
    else return(FALSE);			/* something ain't right */

    ceq[jattrp->opz_equcls].opc_eqavailable = FALSE;
    ceq[jattrp->opz_equcls].opc_attavailable = FALSE;
    BTclear((i4)jattrp->opz_equcls, (char *)&co->opo_maps->opo_eqcmap);
					/* mustn't ref this col higher up */
    kj->node_qen.qen_kjoin.kjoin_kuniq = FALSE;	/* always, for Rtree inners */
    return(TRUE);

}		/* end of opc_spjoin */

/*{
** Name: OPC_KJKEYINFO	- Compile the kjoin_key and kjoin_kcompare CXs and
**				fill in the kjoin_kys info for a KJOIN node.
**
** Description:
**      This routine fills in the kjoin_key, kjoin_kcompare CXs and kjoin_kys
**	info for a key join. This is done by first building maps that tell,
**      for each inner key attribute (1 - N) what, if any, outer attribute
**      is available for joining and what, if any, qtree expressions are 
**      available for keying.  
**        
**	Kjoin_key, kjoin_kcompare, and kjoin_kys are filled in by the aid
**	of these maps. The pseudo-code looks something like:
**
**	opc_kjkeyinfo()
**	{
**	    build an inner keyno to outer attno join map;
**
**	    build an inner keyno to constant qtree map;
**
**	    for (each inner keyno)
**	    {
**		if (an outer attribute is available for this keyno)
**		{
**		    Compile code to materialize the key, and compare the key
**			using the outer attribute
**		}
**		else (we must use the qtree if available)
**		{
**		    Compile code to materialize the key, and compare the key
**			using the computed constant query tree value.
**		}
**
**		Build a key description and add it to kjoin_kys;
**	    }
[@comment_line@]...
**
** Inputs:
**	global -
**	    The query wide state variable
**	cnode -
**	    This struct describes various info about the current node
**	    that we are compiling.
**	rel -
**	    This is the description of the inner relation.
**	key_qtree -
**	    This is the qual that must be applied at this node.
**	oceq -
**	iceq -
**	    These describe the outter and inner attribute and eqc info.
**
** Outputs:
**	cnode->opc_qennode->node_qen.qen_kjoin.kjoin_key -
**	cnode->opc_qennode->node_qen.qen_kjoin.kjoin_kcompare -
**	cnode->opc_qennode->node_qen.qen_kjoin.kjoin_kys -
**	    These are used by QEF to key into the inner relation.
**      keymap - returns bit map of key column eqc's (so they can be
**          eliminated from the kjoin_kqual compares)
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
**      6-june-87 (eric)
**          written
**      1-sep-88 (seputis)
**          fix unix lint problems
**	6-sep-88 (nancy) -- fix bug #2694 where a repeat query preprocessed
**	    in 5.0 will get adc_2004 invalid datatype when user parameter
**	    is part of key.  Fix how data value is sent to opc_adkeybld so
**	    that adc_keybld does not get a datatype of 0.
**	20-sept-88 (nancy) -- bug #3432, where joining a unique storage
**	    with only a partial key match will set kjoin_kuniq to TRUE
**	    resulting in the join not getting all inner rows.  This is set
**	    inadvertently because keyno is 1 more than the actual value of
**	    number of keys (coming out of a loop that increments keyno).  
**	    Fix test for kuniq to subtract 1 from keyno.
**      28-sept-88 (eric)
**          I corrected the fix for bug #2694 by adding back the test for
**          qconst->pst_sym.pst_type != PST_CONST in a crucial if statement.
**	17-jul-89 (jrb)
**	    Correctly initialize precision fields.
**      19-aug-89 (eric)
**          Fixed a bug in handling "backwards" comparisons, ie. comparisons
**          of the form "expr op att". In such comparisons, we were not
**          reversing the operator correctly.
**	18-may-90 (stec)
**	    Fixed bug 8438 related to building keys for repeat queries.
**	    Db_data ptr is set to null for constant nodes representing
**	    repeat query parms and local vars in database procedures.
**	    This needs to be done before a call to opc_adkeybld is issued.
**	24-may-90 (stec)
**	    Added checks to use OPC_EQ, OPC_ATT contents only when filled in.
**	12-jun-90 (stec)
**	    Fixed bug 30712. Placed code checking type of node after the code
**	    checking whether ptr is not null.
**      18-mar-92 (anitap)
**          Added opr_prec and opr_len initializations to fix bug 42363.
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**      15-jun-1993 (stevet)
**          Fix for 40762.  Add support for nullable joins by generating 
**          ADE_BYCOMPARE instruction and passing ADI_NULJN_OP to key build 
**          routine.
**	29-Jun-1993 (jhahn)
**	    Removed code that normalized the comparand in the outer stream
**	    through its equivalance class. This caused trouble with outer
**	    joins.
**	25-jun-99 (inkdo01)
**	    Identify key columns in join predicates to remove them from 
**	    key join kcompare code (since compare is already done in DMF).
**	11-apr-02 (inkdo01)
**	    Don't generate kcompare or key build code past the first key column
**	    without a "=" predicate. Fixes bug 107481.
**	24-apr-02 (hayke02)
**	    Modify the above fix for 107481 - we now only set keyloop_continue
**	    to FALSE if we have previously added an eqclass to the keymap,
**	    indicated by the new boolean keymap_set. This change fixes bug
**	    107650.
**	31-may-02 (inkdo01)
**	    Minor tweak to 107650 change to fix multi-column hash key joins.
**	22-dec-03 (inkdo01)
**	    Added support of kjoin_krow for better buffer management.
**	7-may-04 (inkdo01)
**	    Exclude packed IN-lists from key processing (they're the same 
**	    as OR's).
**	14-may-04 (hayke02)
**	    Only remove ADF key join kcompare code (SIR 97649) if attributes
**	    in the joining equiv class have matching lengths. This change
**	    fixes problem INGSRV 2811, bug 112210.
**	 6-sep-04 (hayke02)
**	    Do not add OPZ_COLLATT attributes to the key_to_jattno[] array.
**	    This change fixes problem INGSRV 2940, bug 112873.
**	24-mar-05 (inkdo01)
**	    Replace above change after it was unceremoniously removed by 
**	    an ill-considered cross-integration.
**	25-Jun-2008 (kiria01) SIR120473
**	    Move pst_isescape into new u_i2 pst_pat_flags.
**	    Made pst_opmeta into u_i2 and widened pst_escape.
**	11-Nov-2009 (kiria01) SIR 121883
**	    With PST_INLIST, check for PST_CONST on RHS as the flag
**	    may be applied to PST_SUBSEL to aid servers to distinguish
**	    simple = from IN and <> from NOT IN.
**	    
[@history_template@]...
*/
VOID
opc_kjkeyinfo(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		RDR_INFO	*rel,
		PST_QNODE	*key_qtree,
		OPC_EQ		*oceq,
		OPC_EQ		*iceq,
		OPE_BMEQCLS	*keymap)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    QEN_KJOIN		*jn = &cnode->opc_qennode->node_qen.qen_kjoin;
    OPO_CO		*co = cnode->opc_cco;
    OPZ_ATTS		*jatt;
    i4			keysize;
    i4			szkeyjattno;
    OPZ_IATTS		jattno;
    i4			szkeyconst;
    i4			keyno;
    i4			align;
    i4			iattno;
    PST_QNODE		*qand;
    PST_QNODE		*qop;
    PST_QNODE		*qvar;
    PST_QNODE		*qconst;
    i4			ninstr, nops, nconst;
    i4		szconst;
    i4			max_base;
    i4			key_type;
    i4			key_offset = 0;
    i4			prev_key_offset;
    i4			keyrow;
    QEF_KEY		*qekey;
    QEF_KAND		**prev_kand;
    ADI_OP_ID		opid;
    OPC_ADF		key_cadf;
    ADE_OPERAND		keyvalop;
    OPC_ADF		kcomp_cadf;
    ADE_OPERAND		kcomp_ops[2];
    OPZ_IATTS		*key_to_jattno;
    OPE_EQCLIST		**eqclist;
    OPC_KJCONST		**key_to_const;
    i4			att_width;
    i4			pm_flag;
    OPC_KJCONST		*kjconst;
    OPC_KJCONST		*new_kjconst;
    i4			cpm_flag;
    i4			cur_qlang;
    i4			save_qlang;
    i4			dummy;
    i4			inequality = FALSE;
    i4                  icode;
    ADI_FI_DESC		*cmplmnt_fidesc;
    ADI_FI_ID		opno;
    DB_STATUS		err;
    OPE_IEQCLS		equcls;
    bool		keyloop_continue = TRUE;
    bool		keymap_set = FALSE;
    bool                rmsflag = (rel->rdr_rel->tbl_relgwid == GW_RMS);

    /* If this is a heap, then don't bother trying to key into it. */
    if (rel->rdr_rel->tbl_storage_type == DMT_HEAP_TYPE)
    {
	jn->kjoin_key = (QEN_ADF *)NULL;
	jn->kjoin_kcompare = (QEN_ADF *)NULL;
	jn->kjoin_kys = NULL;
	jn->kjoin_kuniq = (i4) co->opo_existence;

	return;
    }

    ninstr = 0;			    /* FIXME EAS - added for lint */
    /* make up a translation table from key number to 
    **	joinop attribute numbers;
    */
    szkeyjattno = sizeof(*key_to_jattno) * ((i4) rel->rdr_keys->key_count + 1);
    key_to_jattno = (OPZ_IATTS *) opu_Gsmemory_get(global, szkeyjattno);
    eqclist = (OPE_EQCLIST **) subqry->ops_eclass.ope_base->ope_eqclist;
    for (keyno = 1; keyno <= rel->rdr_keys->key_count; keyno += 1)
    {
	key_to_jattno[keyno] = OPZ_NOATTR;
    }

    keysize = 0;
    for (jattno = 0; jattno < subqry->ops_attrs.opz_av; jattno += 1)
    {
	jatt = subqry->ops_attrs.opz_base->opz_attnums[jattno];
	if (jatt->opz_varnm == co->opo_inner->opo_union.opo_orig
	    &&
	    !(jatt->opz_mask & OPZ_COLLATT))
	{
	    iattno = jatt->opz_attnm.db_att_id;
	    if (iattno == DB_IMTID || iattno == OPZ_FANUM)
	    {
		keyno = 0;
	    }
	    else
	    {
		keyno = rel->rdr_attr[iattno]->att_key_seq_number;
	    }

	    if (keyno > 0)
	    {
		key_to_jattno[keyno] = jattno;
		ninstr += 1;

		keysize += rel->rdr_attr[iattno]->att_width;
		align = keysize % sizeof (ALIGN_RESTRICT);
		if (align != 0)
		{
		    keysize += (sizeof (ALIGN_RESTRICT) - align);
		}
	    }
	}
    }

    /* If there can be no key then give up */
    if (keysize == 0)
    {
	jn->kjoin_key = (QEN_ADF *)NULL;
	jn->kjoin_kcompare = (QEN_ADF *)NULL;
	jn->kjoin_kys = NULL;
	jn->kjoin_kuniq = (i4) co->opo_existence;

	return;
    }

    /* allocate and init an array to map between key number and qtree nodes; */
    szkeyconst = sizeof (*key_to_const) * ((i4) rel->rdr_keys->key_count + 1);
    key_to_const = (OPC_KJCONST **) opu_Gsmemory_get(global, szkeyconst);
    MEfill(szkeyconst, (u_char)0, (PTR)key_to_const);

    /* for (each and off of the qtree) */
    ninstr = 1;
    nconst = 0;
    szconst = 0;
    for (qand = key_qtree; 
	    qand != NULL && qand->pst_sym.pst_type == PST_AND; 
	    qand = qand->pst_right
	)
    {
	qop = qand->pst_left;

	/* If this isn't a unary or a binary operator directly off of the 
	** and node node, then it isn't usefull for keying. This is another way
	** of saying the we aren't going to deal with or's here.
	*/
	if (qop->pst_sym.pst_type != PST_UOP &&
		qop->pst_sym.pst_type != PST_BOP
	    )
	{
	    continue;
	}

	/* Check for and exclude packed IN-lists, as well. They are
	** effectively equivalent to OR's. (or ANDs if <>) */
	if (qop->pst_sym.pst_type == PST_BOP &&
	    qop->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLIST &&
	    qop->pst_right->pst_sym.pst_type == PST_CONST)
	{
	    continue;
	}

	/* find the var node and const node, if there is one; */
	if (qop->pst_left != NULL && 
		qop->pst_left->pst_sym.pst_type == PST_VAR
	    )
	{
	    qvar = qop->pst_left;
	    qconst = qop->pst_right;
	}
	else if (qop->pst_right != NULL && 
		    qop->pst_right->pst_sym.pst_type == PST_VAR
		)
	{
	    qvar = qop->pst_right;
	    qconst = qop->pst_left;
	}
	else
	{
	    continue;
	}

	if (qconst != NULL && qconst->pst_sym.pst_type == PST_CONST)
	{
	    cpm_flag = qconst->pst_sym.pst_value.pst_s_cnst.pst_pmspec;
	    cur_qlang = qconst->pst_sym.pst_value.pst_s_cnst.pst_cqlang;
	}
	else
	{
	    /* If we don't have a constant or a constant expression and we 
	    ** need one then this clause isn't usefull to us.
	    */
	    if (qop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_numargs > 1)
	    {
		cpm_flag = PST_PMNOTUSED;
		cur_qlang = 
		   subqry->ops_root->pst_sym.pst_value.pst_s_root.pst_qlang;
		if (opc_cnst_expr(global, qconst, &cpm_flag, 
							    &cur_qlang) == FALSE
		    )
		{
		    /* Qconst points to neither a constant or a constant
		    ** expression, so lets continue with the next clause.
		    */
		    continue;
		}
	    }
	    else
	    {
		/* There is no input to this operator. */
		qconst = NULL;
	    }
	}

	/* If we have parameters and this is a hash then it's not keyable. */
	if (cpm_flag == PST_PMUSED &&
		rel->rdr_rel->tbl_storage_type == DMT_HASH_TYPE
	    )
	{
	    continue;
	}

	/* Now lets figure out what the key number is for our new var node. */
	jattno = qvar->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
	jatt = subqry->ops_attrs.opz_base->opz_attnums[jattno];
	if (iceq[jatt->opz_equcls].opc_eqavailable == TRUE)
	{
	    jattno = iceq[jatt->opz_equcls].opc_attno;

	    if (jattno != OPC_NOATT)
		jatt = subqry->ops_attrs.opz_base->opz_attnums[jattno];
	    else
		continue;
	}
	else
	{
	    /* We haven't got this var at this node, so it isn't usefull for
	    ** this keying task.
	    */
	    continue;
	}

	iattno = jatt->opz_attnm.db_att_id;
	if (iattno == DB_IMTID || iattno == OPZ_FANUM)
	{
	    keyno = 0;
	}
	else
	{
	    keyno = rel->rdr_attr[iattno]->att_key_seq_number;
	}

	/* If this isn't a key attribute or it's already handled by
	** an outer value then go on to the next clause
	*/
	if (keyno == 0 || 
		(key_to_jattno[keyno] != OPZ_NOATTR &&
		    oceq[jatt->opz_equcls].opc_eqavailable == TRUE
		)
	    )
	{
	    continue;
	}

	/* now lets figure out the correct opid to use. This is necessary
	** because we normalize all comparisons of the form "expr op att"
	** to "att op expr". 
	*/
	if (qvar == qop->pst_left)
	{
	    opid = qop->pst_sym.pst_value.pst_s_op.pst_opno;
	}
	else
	{
	    opid = qop->pst_sym.pst_value.pst_s_op.pst_opno;

	    /* If this isn't an equals or not equals operator, then we
	    ** need to find the compliment of this operator to normalize
	    ** this comparison to att left and value right.
	    */
	    if (opid != global->ops_cb->ops_server->opg_eq &&
				opid != global->ops_cb->ops_server->opg_ne
		)
	    {
		err = adi_fidesc(global->ops_adfcb, 
		    qop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_cmplmnt,
		    &cmplmnt_fidesc);

		if (err != E_DB_OK)
		{
		    opx_verror(err, E_OP0781_ADI_FIDESC, 
				global->ops_adfcb->adf_errcb.ad_errcode);
		}
		opid = cmplmnt_fidesc->adi_fiopid;
	    }
	}

	/* if (we need more keying information for this key attribute) */
	kjconst = key_to_const[keyno];
	if (kjconst == NULL ||
		(kjconst->opc_kjnext == NULL &&
		    (kjconst->opc_keytype == ADC_KHIGHKEY ||
			kjconst->opc_keytype == ADC_KLOWKEY
		    )
		)
	    )
	{
	    save_qlang = global->ops_adfcb->adf_qlang;
	    global->ops_adfcb->adf_qlang = cur_qlang;

	    /* Set the data pointers to NULL in some DB_DATA_VALUEs that
	    ** should already be set, but usually aren't
	    */
	    jatt->opz_dataval.db_data = NULL;

	    if (qconst == NULL)
	    {
		/* if there is no input to this key operator, 
		** then call ADF with no input data or type
		** info to get the key type. 
		*/
		opc_adkeybld(global, &key_type, 
			opid, (DB_DATA_VALUE *)NULL, &jatt->opz_dataval, 
			&jatt->opz_dataval, qop);
	    }
	    else
	    {
		DB_DATA_VALUE	kdv;

		/* Set the data pointers to NULL in some DB_DATA_VALUEs that
		** should already be set, but usually aren't. Fix for bug 30712.
		*/
		if (qconst->pst_sym.pst_type != PST_CONST)
		{
		    qconst->pst_sym.pst_dataval.db_data = NULL;
		}

		/* Call the adf key building code to find out the key_type.
		** This handles the case where data input is required. Note
		** that if the input is a constant expression, then we give
		** the type, length, and prec info but not the actual data.
		** This is because the expression cannot be evaluated until
		** run-time. If this isn't a constant expression, then it must
		** be a real constant, in which case data is given if it's not
		** a repeat query parameter or a DB procecure local variable.
		*/

		STRUCT_ASSIGN_MACRO(qconst->pst_sym.pst_dataval, kdv);

		/*
		** For repeat queries and database procedures
		** repeat query parameters and local variables,
		** when processed in opc_adkeybld (below) should
		** have db_data ptr set to NULL, to have the key
		** type determined correctly (bug 8438).
		*/
		if (qconst->pst_sym.pst_type == PST_CONST &&
		    (qconst->pst_sym.pst_value.pst_s_cnst.
			pst_tparmtype == PST_RQPARAMNO
		     ||
		     qconst->pst_sym.pst_value.pst_s_cnst.
			pst_tparmtype == PST_LOCALVARNO
		    )
		   )
		{
		    kdv.db_data = NULL;
		}

		opc_adkeybld(global, &key_type, opid, &kdv,
		    &jatt->opz_dataval, &jatt->opz_dataval, qop);
	    }
	    global->ops_adfcb->adf_qlang = save_qlang;

	    /* If (the key is exact, range, high, low, or nomatch) */
	    if (key_type == ADC_KEXACTKEY || 
		    (rel->rdr_rel->tbl_storage_type != DMT_HASH_TYPE &&
			(key_type == ADC_KRANGEKEY ||
			    key_type == ADC_KHIGHKEY || 
			    key_type == ADC_KLOWKEY ||
			    key_type == ADC_KNOMATCH
			)
		    )
		)
	    {
		/* add this key to the key_to_const array; */
		new_kjconst = (OPC_KJCONST *) 
				opu_Gsmemory_get(global, sizeof (OPC_KJCONST));
		new_kjconst->opc_qconst = qconst;
		new_kjconst->opc_qop = qop;
		new_kjconst->opc_opid = opid;
		if (key_type == ADC_KEXACTKEY && cpm_flag == PST_PMUSED)
		{
		    new_kjconst->opc_keytype = ADC_KRANGEKEY;
		}
		else
		{
		    new_kjconst->opc_keytype = key_type;
		}

		/* If we've found a constant that isn't an exact key then
		** lets remember it.
		*/
		if (new_kjconst->opc_keytype != ADC_KEXACTKEY)
		{
		    inequality = TRUE;
		}

		new_kjconst->opc_kjnext = kjconst;
		key_to_const[keyno] = new_kjconst;

		/* increment the number of instructions, constants, and the
		** size of constants that will be required to build and
		** compare the keys.
		*/
		ninstr += 1;
		if (qconst != NULL)
		{
		    nconst += 1;
		    szconst += qconst->pst_sym.pst_dataval.db_length;
		}

		/* Increment the size required for the key row */
		att_width = rel->rdr_attr[iattno]->att_width;
		align = att_width % sizeof (ALIGN_RESTRICT);
		if (align != 0)
		{
		    att_width += (sizeof (ALIGN_RESTRICT) - align);
		}
		if (key_type == ADC_KRANGEKEY || key_type == ADC_KNOMATCH)
		{
		    att_width += att_width;
		}
		keysize += att_width;
	    }
	}
    }

    /* begin the kjoin_key and kjoin_kcompare CXs; */
    nops = 5 * ninstr;
    max_base = cnode->opc_below_rows + 2;
    opc_adalloc_begin(global, &key_cadf, &jn->kjoin_key, 
			ninstr, nops, nconst, szconst, max_base, OPC_STACK_MEM);
    opc_ptrow(global, &keyrow, keysize);
    opc_adbase(global, &key_cadf, QEN_ROW, keyrow);
    opc_adbase(global, &key_cadf, QEN_KEY, 0);
    jn->kjoin_key->qen_output = keyrow;
    jn->kjoin_krow = keyrow;

    nops = 2 * ninstr;
    nconst = 0;
    szconst = 0;
    opc_adalloc_begin(global, &kcomp_cadf, &jn->kjoin_kcompare, 
			ninstr, nops, nconst, szconst, max_base, OPC_STACK_MEM);
    opc_adbase(global, &kcomp_cadf, QEN_ROW, keyrow);

    /* allocate and init a QEF_KEY; */
    qekey = jn->kjoin_kys = (QEF_KEY *) opu_qsfmem(global, sizeof (QEF_KEY));
    qekey->key_width = 0;

    /* allocate and init a QEF_KOR; */
    qekey->key_kor = (QEF_KOR *) opu_qsfmem(global, sizeof (QEF_KOR));
    qekey->key_kor->kor_next = NULL;
    qekey->key_kor->kor_keys = NULL;
    qekey->key_kor->kor_numatt = 0;

    prev_kand = &qekey->key_kor->kor_keys;

    /* for (each key number) until a key without a predicate or the first
    ** key column after an inequality predicate. */
    for (keyno = 1; 
	    keyloop_continue && keyno <= rel->rdr_keys->key_count; 
	    keyno += 1
	)
    {
	/* if (there is an outer value) */
	jatt = subqry->ops_attrs.opz_base->opz_attnums[key_to_jattno[keyno]];
	if (key_to_jattno[keyno] != OPZ_NOATTR &&
	    oceq[equcls = jatt->opz_equcls].opc_eqavailable == TRUE
	    )
	{
	    /* else fill in the outer value operand info for the key and
	    ** kcompare CXs.
	    */
	    if (eqclist[equcls]->ope_nulljoin == TRUE)
	    {
	    	/* We want to include NULLs in the result of joins here, so
		** we use ADE_BYCOMPARE since it assumes that NULL = NULL
		*/
		icode = ADE_BYCOMPARE;
		opno = ADI_NULJN_OP;
	    }
	    else
	    {
		/* We want to exclude NULLs in the result of this join. So
		** we use ADE_COMPARE which assumes that NULL != NULL
		*/
		OPZ_BMATTS	*attrmap;
		OPZ_IATTS	maxattr;
		OPZ_IATTS	attribute;
		bool		length_match = TRUE;
		bool		first_time = TRUE;

		icode = ADE_COMPARE;
		opno = ADI_EQ_OP;
		attrmap =
		&subqry->ops_eclass.ope_base->ope_eqclist[equcls]->ope_attrmap;
		maxattr = subqry->ops_attrs.opz_av;
		for( attribute = -1;
		    (attribute = BTnext( (i4)attribute, (char *)attrmap,
		    (i4)maxattr)) >= 0;)
		{
		    OPS_DTLENGTH	attr_length = 0;
		    OPZ_ATTS		*attrp;

		    attrp = subqry->ops_attrs.opz_base->opz_attnums[attribute];
		    if (first_time)
		    {
			attr_length = attrp->opz_dataval.db_length;
			first_time = FALSE;
			continue;
		    }
		    if (attr_length != attrp->opz_dataval.db_length)
		    {
			length_match = FALSE;
			break;
		    }
		}
		if (length_match)
		{
		    BTset((i4)equcls, (char *)keymap);
		    if (!rmsflag)
			keymap_set = TRUE;
		}
                                /* set map of actual key columns */
	    }
	    keyvalop.opr_dt = oceq[equcls].opc_eqcdv.db_datatype;
	    keyvalop.opr_prec = oceq[equcls].opc_eqcdv.db_prec;
	    keyvalop.opr_len = oceq[equcls].opc_eqcdv.db_length;
	    keyvalop.opr_collID = oceq[equcls].opc_eqcdv.db_collID;
	    keyvalop.opr_offset = oceq[equcls].opc_dshoffset;
	    keyvalop.opr_base = 
		key_cadf.opc_row_base[oceq[equcls].opc_dshrow];

	    if (keyvalop.opr_base < ADE_ZBASE || 
			keyvalop.opr_base >= 
				    ADE_ZBASE + key_cadf.opc_qeadf->qen_sz_base
		)
	    {
		opc_adbase(global, &key_cadf, QEN_ROW, 
			oceq[equcls].opc_dshrow);
		opc_adbase(global, &kcomp_cadf, QEN_ROW, 
			oceq[equcls].opc_dshrow);
		keyvalop.opr_base = 
			key_cadf.opc_row_base[oceq[equcls].opc_dshrow];
	    }

	    /* compile the key build and build the key description */
	    prev_key_offset = key_offset;
	    opc_mvkeyatt(global, &key_cadf, &prev_kand, keyno, &keyvalop, 
			    rel, opno, ADC_KEXACTKEY,
			    keyrow, &key_offset, ADE_1RANGE_DONTCARE,
			    qekey->key_kor);

	    STRUCT_ASSIGN_MACRO(keyvalop, kcomp_ops[1]);
	    kcomp_ops[1].opr_base = 
		    kcomp_cadf.opc_row_base[oceq[equcls].opc_dshrow];

	    /* compile an ADE_COMPARE into kjoin_kcompare for this key; */
	    iattno = rel->rdr_keys->key_array[keyno - 1];
	    kcomp_ops[0].opr_dt = rel->rdr_attr[iattno]->att_type;
	    kcomp_ops[0].opr_len = rel->rdr_attr[iattno]->att_width;
	    kcomp_ops[0].opr_prec = rel->rdr_attr[iattno]->att_prec;
	    kcomp_ops[0].opr_collID = rel->rdr_attr[iattno]->att_collID;
	    kcomp_ops[0].opr_offset = prev_key_offset;
	    kcomp_ops[0].opr_base = kcomp_cadf.opc_row_base[keyrow];
	    opc_adinstr(global, &kcomp_cadf, icode, 
						ADE_SMAIN, 2, kcomp_ops, 0);
	}
	else
	{
	    /* if (there isn't a qnode for this keyno) */
	    if (key_to_const[keyno] == NULL)
	    {
		/* if (the inner is a hash) */
		if (rel->rdr_rel->tbl_storage_type == DMT_HASH_TYPE)
		{
		    keyno = 1;
		}
		break;
	    }
	    else
	    {
		u_i4 pat_flags;
		/* else there is one or more constants that we can use for 
		** the current key attribute, so
		** lets compile a key build for each constant and set things
		** up so that the built key value can be added to kjoin_key
		*/
		for (kjconst = key_to_const[keyno];
			kjconst != NULL;
			kjconst = kjconst->opc_kjnext
		    )
		{
		    qconst = kjconst->opc_qconst;
		    qop = kjconst->opc_qop;
		    opid = kjconst->opc_opid;
		    key_type = kjconst->opc_keytype;

		    if (qconst == NULL)
		    {
			/* We are building a key from an 'is null', or some
			** other similiar, operator.
			** ADF says that it ignores the source operand for 
			** key builds from 'is null' ops, so we fill the operand
			** in with zeros to insure that they aren't used.
			*/
			keyvalop.opr_dt = 0;
			keyvalop.opr_len = 0;
			keyvalop.opr_prec = 0;
			keyvalop.opr_collID = -1;
			keyvalop.opr_offset = 0;
			keyvalop.opr_base = 0;
			pm_flag = ADE_1RANGE_DONTCARE;
			if (keymap_set)
			    keyloop_continue = FALSE;
		    }
		    else if (qconst->pst_sym.pst_type == PST_CONST &&
			       qconst->pst_sym.pst_value.pst_s_cnst.pst_parm_no 
									== 0
			    )
		    {
			opc_adconst(global, &key_cadf, 
				&qconst->pst_sym.pst_dataval, &keyvalop, 
			    qconst->pst_sym.pst_value.pst_s_cnst.pst_cqlang,
								    ADE_SMAIN);
			pm_flag = ADE_1RANGE_DONTCARE;
			/* See if it isn't a "=" pred. and leave the loop, if so. */
			if (qop->pst_sym.pst_type != PST_BOP ||
			    qop->pst_sym.pst_value.pst_s_op.pst_opno !=
				global->ops_cb->ops_server->opg_eq)
			 if (keymap_set)
			    keyloop_continue = FALSE;
		    }
		    else
		    {
			keyvalop.opr_dt = 
				    qconst->pst_sym.pst_dataval.db_datatype;
			keyvalop.opr_len = OPC_UNKNOWN_LEN;
                        keyvalop.opr_prec = OPC_UNKNOWN_PREC;
                        keyvalop.opr_collID = -1;
			opc_cqual(global, cnode, qconst, &key_cadf,
						ADE_SMAIN, &keyvalop, &dummy);

			switch (key_type)
			{
			 case ADC_KEXACTKEY:
			    pm_flag = ADE_3RANGE_NO;
			    break;

			 case ADC_KRANGEKEY:
			 case ADC_KALLMATCH:
			 case ADC_KHIGHKEY:
			 case ADC_KLOWKEY:
			    pm_flag = ADE_2RANGE_YES;
			    if (keymap_set)
				keyloop_continue = FALSE;
			    break;

			 case ADC_KNOMATCH:
			 case ADC_KNOKEY:
			    pm_flag = ADE_1RANGE_DONTCARE;
			    if (keymap_set)
				keyloop_continue = FALSE;
			    break;
			}
		    }

		    /* If this operand needs escape characteristics, then compile an
		    ** instruction to let ADF know.
		    */
		    pat_flags = qop->pst_sym.pst_value.pst_s_op.pst_pat_flags;
		    if (pat_flags != AD_PAT_DOESNT_APPLY)
		    {
			ADE_OPERAND	    escape_ops[2];
			DB_DATA_VALUE	    escape_dv;
			i4		    nops = 1;

			escape_dv.db_prec = 0;
			escape_dv.db_collID = -1;
			escape_dv.db_data = 
				    (char*)&qop->pst_sym.pst_value.pst_s_op.pst_escape;
			if ((pat_flags & AD_PAT_ISESCAPE_MASK) == AD_PAT_HAS_UESCAPE)
			{
			    escape_dv.db_datatype = DB_NCHR_TYPE;
			    escape_dv.db_length = sizeof(UCS2);
			}
			else
			{
			    escape_dv.db_datatype = DB_CHA_TYPE;
			    escape_dv.db_length = CMbytecnt((char*)escape_dv.db_data);
			}
			opc_adconst(global, &key_cadf, &escape_dv, &escape_ops[0],
							    DB_SQL, ADE_SMAIN);
			if (pat_flags & ~AD_PAT_ISESCAPE_MASK)
			{
			    DB_DATA_VALUE sf_dv;
			    sf_dv.db_prec = 0;
			    sf_dv.db_collID = -1;
			    sf_dv.db_data = (char*)&qop->pst_sym.pst_value.pst_s_op.pst_pat_flags;
			    sf_dv.db_datatype = DB_INT_TYPE;
			    sf_dv.db_length = sizeof(u_i2);
			    opc_adconst(global, &key_cadf, &sf_dv, &escape_ops[1],
							    DB_SQL, ADE_SMAIN);
			    nops++;
			}
			opc_adinstr(global, &key_cadf, ADE_ESC_CHAR, ADE_SMAIN, 
							    nops, escape_ops, 0);
		    }

		    /* compile the key build and build the key description */
		    opc_mvkeyatt(global, &key_cadf, &prev_kand, 
				keyno, &keyvalop, rel, (ADI_FI_ID) opid,
				key_type, keyrow, &key_offset, pm_flag,
				qekey->key_kor);
		}
	    }
	}
    }
    qekey->key_width = qekey->key_kor->kor_numatt;

    /* Now lets finish off the CXs */
    opc_adinstr(global, &kcomp_cadf, ADE_AND, ADE_SMAIN, 0, kcomp_ops, 0);
    opc_adend(global, &kcomp_cadf);
    opc_adend(global, &key_cadf);

    /* If there aren't any key values available, then lets zero out the
    ** related keying data structures.
    */
    if (keyno == 1)
    {
	jn->kjoin_key = (QEN_ADF *)NULL;
	jn->kjoin_kcompare = (QEN_ADF *)NULL;
	jn->kjoin_kys = NULL;
	jn->kjoin_kuniq = (i4) co->opo_existence;

	return;
    }
    else
    {
	/* Have we build a key for a unique key relation that specifies one
	** exact full key? If so, lets tell QEF that so it can save some time
	** during processing.
	*/
	if ((rel->rdr_rel->tbl_status_mask & DMT_UNIQUEKEYS) &&
		inequality == FALSE &&
		qekey->key_kor != NULL &&
		qekey->key_kor->kor_next == NULL &&
		rel->rdr_keys->key_count == (keyno - 1)
	    )
	{
	    jn->kjoin_kuniq = TRUE;
	}
	else
	{
	    /* Otherwise, OPF may have been able to figure out that we don't
	    ** want any tuples from the inner (except for minor qualification).
	    ** If so, lets tell QEF that.
	    */
	    jn->kjoin_kuniq = (i4) co->opo_existence;
	}
    }
}

/*{
** Name: OPC_MVKEYATT	- This moves a key attribute to a row and builds a
**			    description for it.
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
**      7-aug-87 (eric)
**          written
**	17-jul-89 (jrb)
**	    Correctly initialize precision fields.
**	30-may-91 (nancy)
**	    Fixed error in opr_prec initialization.
[@history_template@]...
*/
static VOID
opc_mvkeyatt(
	OPS_STATE   *global,
	OPC_ADF	    *cadf,
	QEF_KAND    ***prev_kand,
	i4	    keyno,
	ADE_OPERAND *keyvalop,
	RDR_INFO    *rel,
	ADI_FI_ID   opid,
	i4	    key_type,
	i4	    key_row,
	i4	    *key_offset,
	i4	    pmflag,
	QEF_KOR	    *qekor )
{
	i4		iattno;
	ADE_OPERAND	key_ops[5];
	i4		align;
	i4		att_width;
	QEF_KAND	*kand;

	/* Move the located value into the key row; */
	iattno = rel->rdr_keys->key_array[keyno - 1];
	key_ops[1].opr_dt = key_ops[2].opr_dt = rel->rdr_attr[iattno]->att_type;
	key_ops[1].opr_prec = key_ops[2].opr_prec = 
					    rel->rdr_attr[iattno]->att_prec;
	key_ops[1].opr_len = key_ops[2].opr_len = 
					    rel->rdr_attr[iattno]->att_width;
	key_ops[1].opr_collID = key_ops[2].opr_collID =
					    rel->rdr_attr[iattno]->att_collID;

	if (key_type == ADC_KEXACTKEY ||
		key_type == ADC_KLOWKEY ||
		key_type == ADC_KRANGEKEY ||
		key_type == ADC_KNOMATCH
	    )
	{
	    key_ops[1].opr_offset = *key_offset;
	    key_ops[1].opr_base = cadf->opc_row_base[key_row];

	    /* figure out the width of this attribute 
	    ** and add it to key_offset 
	    */
	    att_width = rel->rdr_attr[iattno]->att_width;
	    align = att_width % sizeof (ALIGN_RESTRICT);
	    if (align != 0)
	    {
		att_width += (sizeof (ALIGN_RESTRICT) - align);
	    }
	    *key_offset += att_width;
	}
	else
	{
	    key_ops[1].opr_offset = 0;
	    key_ops[1].opr_base = ADE_NULLBASE;
	}

	if (key_type == ADC_KHIGHKEY ||
		key_type == ADC_KRANGEKEY ||
		key_type == ADC_KNOMATCH
	    )
	{
	    key_ops[2].opr_offset = *key_offset;
	    key_ops[2].opr_base = cadf->opc_row_base[key_row];

	    /* figure out the width of this attribute 
	    ** and add it to key_offset 
	    */
	    att_width = rel->rdr_attr[iattno]->att_width;
	    align = att_width % sizeof (ALIGN_RESTRICT);
	    if (align != 0)
	    {
		att_width += (sizeof (ALIGN_RESTRICT) - align);
	    }
	    *key_offset += att_width;
	}
	else
	{
	    key_ops[2].opr_offset = 0;
	    key_ops[2].opr_base = ADE_NULLBASE;
	}

	STRUCT_ASSIGN_MACRO(*keyvalop, key_ops[3]);

	/* Fill in the operand telling ADE where to put the key type. Currently
	** QEF doesn't use this info.
	*/
	key_ops[0].opr_dt = DB_INT_TYPE;
	key_ops[0].opr_prec = 0;
	key_ops[0].opr_len = 2;
	key_ops[0].opr_collID = -1;
	key_ops[0].opr_base = cadf->opc_key_base;
	key_ops[0].opr_offset = 0;

	/* Now lets fill in the overloaded operator */
	key_ops[4].opr_len = pmflag;
	key_ops[4].opr_prec = 0;
	key_ops[4].opr_collID = -1;
	key_ops[4].opr_base = 0;
	key_ops[4].opr_offset = 0;
	key_ops[4].opr_dt = opid; 

	/* Now that all of the operands are filled in, lets compile the
	** key build instruction.
	*/
	opc_adinstr(global, cadf, ADE_KEYBLD, ADE_SMAIN, 5, key_ops, 3);

	/* build a key description; */

	/* allocate and init a QEF_KAND; */
	**prev_kand = kand = (QEF_KAND *) opu_qsfmem(global, sizeof (QEF_KAND));
	*prev_kand = &kand->kand_next;
	kand->kand_next = NULL;
	kand->kand_attno = rel->rdr_keys->key_array[keyno - 1];

	/* allocate and init a QEN_KATT; */
	kand->kand_keys = (QEF_KATT *) opu_qsfmem(global, sizeof (QEF_KATT));
	kand->kand_keys->attr_attno = kand->kand_attno;
	kand->kand_keys->attr_next = NULL;

	qekor->kor_numatt += 1;

	switch (key_type)
	{
	 case ADC_KNOMATCH:
	 case ADC_KEXACTKEY:
	    kand->kand_type = QEK_EQ;
	    kand->kand_keys->attr_operator = DMR_OP_EQ;
	    kand->kand_keys->attr_type = (i4) key_ops[1].opr_dt;
	    kand->kand_keys->attr_length = (i4) key_ops[1].opr_len;
	    kand->kand_keys->attr_prec = (i2) key_ops[1].opr_prec;
	    kand->kand_keys->attr_collID = (i2) key_ops[1].opr_collID;
	    kand->kand_keys->attr_value = key_ops[1].opr_offset;
	    break;

	 case ADC_KHIGHKEY:
	    kand->kand_type = QEK_MAX;
	    kand->kand_keys->attr_operator = DMR_OP_LTE;
	    kand->kand_keys->attr_type = (i4) key_ops[2].opr_dt;
	    kand->kand_keys->attr_length = (i4) key_ops[2].opr_len;
	    kand->kand_keys->attr_prec = (i2) key_ops[2].opr_prec;
	    kand->kand_keys->attr_collID = (i2) key_ops[2].opr_collID;
	    kand->kand_keys->attr_value = key_ops[2].opr_offset;
	    break;

	 case ADC_KRANGEKEY:
	 case ADC_KLOWKEY:
	    kand->kand_type = QEK_MIN;
	    kand->kand_keys->attr_operator = DMR_OP_GTE;
	    kand->kand_keys->attr_type = (i4) key_ops[1].opr_dt;
	    kand->kand_keys->attr_length = (i4) key_ops[1].opr_len;
	    kand->kand_keys->attr_prec = (i2) key_ops[1].opr_prec;
	    kand->kand_keys->attr_collID = (i2) key_ops[1].opr_collID;
	    kand->kand_keys->attr_value = key_ops[1].opr_offset;
	    break;
	}

	if (key_type == ADC_KRANGEKEY ||
		key_type == ADC_KNOMATCH
	    )
	{
	    **prev_kand = kand = (QEF_KAND *) opu_qsfmem(global, sizeof (QEF_KAND));
	    *prev_kand = &kand->kand_next;
	    kand->kand_next = NULL;
	    kand->kand_attno = rel->rdr_keys->key_array[keyno - 1];

	    /* allocate and init a QEN_KATT; */
	    kand->kand_keys = (QEF_KATT *) opu_qsfmem(global, sizeof (QEF_KATT));
	    kand->kand_keys->attr_attno = kand->kand_attno;
	    kand->kand_keys->attr_next = NULL;

	    kand->kand_type = QEK_MAX;
	    kand->kand_keys->attr_operator = DMR_OP_LTE;
	    kand->kand_keys->attr_type = (i4) key_ops[2].opr_dt;
	    kand->kand_keys->attr_length = (i4) key_ops[2].opr_len;
	    kand->kand_keys->attr_prec = (i2) key_ops[2].opr_prec;
	    kand->kand_keys->attr_collID = (i2) key_ops[2].opr_collID;
	    kand->kand_keys->attr_value = key_ops[2].opr_offset;

	    qekor->kor_numatt += 1;
	}
}

/*{
** Name: OPC_HASHATTS	- Build atts from eqc(s) used for hash key.
**
** Description:	This function creates a DB_CMP_LIST array to house
**		the hash join key column descriptors.  One entry per
**		join column is created;  later on, the compare-list
**		may get collapsed into one giant BYTE(n) key, but not
**		until all opc processing for the hash-join node is
**		completed.  The offset part of the compare entries is
**		left zero, hash-materialize can fill that in.
**
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    TRUE if any join column was nullable, else FALSE
**
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      26-feb-99 (inkdo01)
**	    Cloned from opc_esoatts for hash join support.
**	7-may-99 (inkdo01)
**	    Fix to generate merged key attr (from outer/inner attrs).
**	7-Apr-2004 (schka24)
**	    Set attr default stuff in case someone looks at it.
**	7-Dec-2005 (kschendel)
**	    Generate DB_CMP_LIST entries instead of DMT_ATTR_ENTRYs.
**	    It turns out that nobody benefits from the latter, so
**	    we'll go straight to the compare-list.
[@history_template@]...
*/
bool
opc_hashatts(
	OPS_STATE	*global,
	OPE_BMEQCLS	*keqcmp,
	OPC_EQ		*oceq,
	OPC_EQ		*iceq,
	DB_CMP_LIST	**pcmplist,	/* Where to return array base ptr */
	i4		*kcount)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    DB_CMP_LIST		*cmp;		/* An entry in the array */
    DB_DATA_VALUE	rdv;
    i4			neqcs;
    OPE_IEQCLS		eqcno;
    bool		anynulls;


    /* Count entries and allocate compare-list array. */
    neqcs = BTcount((char *)keqcmp, (i4)subqry->ops_eclass.ope_ev);
    cmp = (DB_CMP_LIST *) opu_qsfmem(global, neqcs * sizeof(DB_CMP_LIST));
    *pcmplist = cmp;
    *kcount = neqcs;

    anynulls = FALSE;
    eqcno = -1;
    while ( (eqcno = BTnext((i4)eqcno, (char *)keqcmp,
			(i4)subqry->ops_eclass.ope_ev)) != -1)
    {
	/* First, assure the key type is merged result of inner and
	** outer join attrs. */
	opc_resolve(global, &oceq[eqcno].opc_eqcdv, 
	    &iceq[eqcno].opc_eqcdv, &rdv);

	if (rdv.db_datatype < 0) anynulls = TRUE;
	cmp->cmp_type = rdv.db_datatype;
	cmp->cmp_length = rdv.db_length;
	cmp->cmp_precision = rdv.db_prec;
	cmp->cmp_collID = rdv.db_collID;
	cmp->cmp_offset = 0;
	cmp->cmp_direction = 0;
	++ cmp;
    }
    return (anynulls);
}


/*{
** Name: OPC_OJQUAL - Produce an outer join qualification.
**
** Description:
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
**      10-nov-89 (stec)
**          written
**	7-may-93 (rickh)
**	    Corrected the declaration of tidatt.  A null pointer was being
**	    passed to opc_cqual.
**      27-Nov-97 (horda03)
**          Repeated Selects that perform a KJOIN with a B-Tree can require
**          an additional base.
**	06-feb-03 (hayke02)
**	    DBPs containing parameters and 'not exists'/'not in' subselects
**	    transformed into outer joins may require an additional base.
**	    This change fixes bug 109257.
**      18-mar-2004 (huazh01)
**          Modify the fix for b109257. We now determine the number of 
**          base required by counting the number of PST_AND node 
**          specificied in the outer join qualification. 
**          This fixes INGSRV2727, bug 111887.
**	12-Dec-2005 (kschendel)
**	    Push more work off to opcadf.
[@history_template@]...
*/
VOID
opc_ojqual(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		QEN_OJINFO	*oj,
		OPC_EQ		*iceq,
		OPC_EQ		*oceq)
{
    OPO_CO	    *co = cnode->opc_cco;
    PST_QNODE	    *oqual = (PST_QNODE *) NULL;
    OPE_BMEQCLS	    oqual_eqcmp;
    i4		    max_base;
    PST_QNODE       *oqualp = (PST_QNODE *) NULL; 

    /* Set some variables to their beginning values */
    MEfill(sizeof (oqual_eqcmp), (u_char)0, (PTR)&oqual_eqcmp);

    /* Get the outer join qualification tree. */
    opc_ogtqual(global, cnode, &oqual, &oqual_eqcmp);

    /*
    ** Fill out oj_oqual QEN_ADF.
    ** We need to remember that besides the qualification tree pointed
    ** to by oqual, there is an equijoin conjunct, which needs to be ANDed
    ** with tree, for the oj_oqual to represent the full ON clause.
    **
    */
    {
	OPC_ADF	    cadf;
	i4	    ninstr, tinstr;
	i4	    nops, tops;

	/* Begin the adf; */
	tinstr = tops = ninstr = nops = 0;

	/* Estimate the size for the join eqc */
	if (co->opo_variant.opo_local->opo_ojoin != NULL)
	{
	    opc_emsize(global, co->opo_variant.opo_local->opo_ojoin,
		&ninstr, &nops);
	}

	/* Estimate the size for the oqual tree */
	opc_emsize(global, &oqual_eqcmp, &tinstr, &tops);

	/* Add the estimates. */
	ninstr += tinstr;
	nops += tops;

	max_base = cnode->opc_below_rows + 2;

        oqualp = oqual; 
        while (oqualp)
        {
             max_base = max_base + 1; 
             oqualp = oqualp->pst_right;
        }

	/* Start compiling CX */
	opc_adalloct_begin(global, &cadf, &oj->oj_oqual, ninstr, nops, 0, (i4) 0, 
	    max_base, OPC_STACK_MEM);

	/* Compile the qtree */
	if (oqual != NULL)
	{
	    i4	tidatt;
	    tidatt = FALSE;

	    opc_cqual(global, cnode, oqual, &cadf, ADE_SMAIN, 
		(ADE_OPERAND *) NULL, &tidatt);
	}

	/* Compile the join equivalence class. */
	if (co->opo_variant.opo_local->opo_ojoin != NULL)
	{
	    opc_eminstrs(global, cnode, &cadf,
		co->opo_variant.opo_local->opo_ojoin,
		oceq, iceq);
	}

	/* End compiling CX */
	opc_adend(global, &cadf);
    }	    
}

/*{
** Name: OPC_SPEQCMP - Produce map of special OJ eqcs.
**
** Description:
[@comment_line@]...
**	Generate maps of special equivalence classes. These maps are used
**	to compile CXs that set certain special equivalence classes to 1 and
**	others to 0.  Two maps are generated: the map of equivalence
**	classes to be set to 1 when some condition occurs and the map
**	of equivalence classes to be set to 0.  The possible conditions that
**	call this routine are Left Join, Right Join, and Inner Join.
**
**
**
** Inputs:
[@PARAM_DESCR@]...
**	global		Optimizer state information.
**
**	cnode		Current node in the cost tree.  This is the parent
**			node of the following node.
**
**	child		If this is the inner child, then we're preparing
**			equivalence class maps for a left join "condition."
**
**			If this is the outer child, then we're preparing
**			equivalence class maps for the right join "condition."
**
**			If this is NULL, then we're preparing eqc maps for
**			the inner join "condition."
**
** Outputs:
**	eqcmp0		These are the special equivalence classes that will
**			be set to 0 when the "condition" (left or right join)
**			occurs.
**
**	eqcmp1		These are the special equivalence classes that will
**			be set to 1 when the "condition" occurs.
**
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
**      10-jan-90 (stec)
**          written
**	19-oct-92 (rickh)
**	    Added header information.
**	15-jan-94 (ed)
**	    simplify OPF/OPC interface, remove all references to
**	    outer join descriptor
[@history_template@]...
*/
VOID
opc_speqcmp(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		OPO_CO		*child,
		OPE_BMEQCLS	*eqcmp0,
		OPE_BMEQCLS	*eqcmp1)
{
    OPS_SUBQUERY    *subqry = global->ops_cstate.opc_subqry;
    OPO_CO	    *co = cnode->opc_cco;
    OPE_BMEQCLS	    *specialp;

    specialp = co->opo_variant.opo_local->opo_special;

    /* Initialize the output maps. */
    MEfill(sizeof(*eqcmp0), 0, (PTR)eqcmp0);
    MEfill(sizeof(*eqcmp1), 0, (PTR)eqcmp1);

    if (child != NULL)
    {
	/*
	** If child is specified, then this is the no match case and
	** all special eqcs derived from the child and the ones created
	** at this node and are related to the child need to be initialized.
	*/
	OPV_IVARS   varno;
	OPV_VARS    *var;

	MEcopy((PTR)co->opo_variant.opo_local->opo_innereqc,
	    sizeof(*eqcmp0), (PTR)eqcmp0);
	BTand((i4)BITS_IN(*eqcmp0), (char *)&child->opo_maps->opo_eqcmap, (char *)eqcmp0);

	/* We need to determine, which one from opl_special are interesting. */
	for (varno = -1;
	     (varno = BTnext((i4)varno,
		(char *)child->opo_variant.opo_local->opo_bmvars,
		subqry->ops_vars.opv_rv)) > -1;
	    )
	{
	    var = subqry->ops_vars.opv_base->opv_rt[varno];

	    if (var->opv_ojeqc == OPE_NOEQCLS)
		continue;

	    if (BTtest((i4)var->opv_ojeqc, (char *)specialp))
	    {
		BTset((i4)var->opv_ojeqc, (char *)eqcmp0);
	    }
	}

	MEcopy((PTR)specialp, sizeof(*eqcmp1), (PTR)eqcmp1);
	BTclearmask((i4)BITS_IN(*eqcmp1), (char *)eqcmp0, (char *)eqcmp1);
    }
    else
    {
	/*
	** If no child is specified, then this is the match
	** case and all special eqcs created at this node
	** need to be initialized to 1.
	*/
	BTor((i4)BITS_IN(*eqcmp1), (char *)specialp,
	    (char *)eqcmp1);
    }

    return;
}

/*{
** Name: OPC_OJEQUAL_BUILD - Fill the oj_equal CX in the outer 
**			     join structure.
**
** Description:
**	This routine compiles the CX that is executed when an inner join
**	condition occurs.  This CX sets to 1 the special EQCs originating
**	at this node.  Also, for full joins, this CX populates the result
**	row with values from the node's children.  This special processing
**	for full joins compensates for the fact that a previous LNULL or
**	RNULL may have pre-populated the result row with junk.
**
**
**
** Inputs:
**	global			current state of compilation
**	cnode			current node being compiled
**	oj			outer join descriptor for this node
**	sceq			saved equivalence classes.  this was the
**				state of the full join equivalence classes
**				before a special row was created for them
**	returnedEQCbitMap	equivalence classes in the special row that
**				we will populate for full joins
**
** Outputs:
**	fills in the OJEQUAL CX
**
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      10-nov-89 (stec)
**          written
**	29-apr-93 (rickh)
**	    Added returnedEQCbitMap argument.   For all outer joins, we use this
**	    to figure out which EQCs to refresh.  Added header.  Upped number
**	    of ADF bases to account for the oj result row into which returned
**	    eqcs are copied.
**	30-nov-05 (inkdo01)
**	    Changes to compile function attrs that sneak in here (because they 
**	    involve parameters from the inner table of the OJ and can't be 
**	    compiled any sooner). Fixes bug 115529.
**	12-Dec-2005 (kschendel)
**	    Push QEN_ADF allocation off to opcadf.
[@history_template@]...
*/
VOID
opc_ojequal_build(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		QEN_OJINFO	*oj,
		OPC_EQ		sceq[],
		OPE_BMEQCLS	*returnedEQCbitMap
)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    OPZ_ATTS		**attnums;
    OPC_EQ		*ceq;
    OPC_ADF		cadf;
    i4			ninstr, tinstr;
    i4			nops, tops;
    DB_DATA_VALUE	dv;
    OPL_OJSPEQC		val;
    ADE_OPERAND		adeops[2];
    OPE_BMEQCLS		sp0eqcmp;
    OPE_BMEQCLS		sp1eqcmp;
    OPE_IEQCLS		eqcno;
    i4			i, sprowcnt;
    i4			max_base, dummy;
    OPZ_ATTS		*attrp;
    OPZ_FATTS		*fattrp;

    /* Create the special OJ eqc map to be initialized */
    opc_speqcmp(global, cnode, (OPO_CO *)NULL, &sp0eqcmp, &sp1eqcmp);

    /* Init some vars to keep line length down */
    attnums = (OPZ_ATTS **) subqry->ops_attrs.opz_base->opz_attnums;
    ceq = cnode->opc_ceq;

    tinstr = tops = ninstr = nops = 0;

    sprowcnt = 0;
    for (i = 0; i < subqry->ops_eclass.ope_ev; i++)
    {
	if (sceq[i].opc_eqavailable == TRUE)
		sprowcnt++;
    }

    ninstr = sprowcnt;
    nops = 2 * sprowcnt;

    /* Estimate the size of the CX */
    for (eqcno = -1; (eqcno = BTnext(eqcno,
	    (char *)&sp1eqcmp, OPE_MAXEQCLS)) > -1; )
    {
	/* Estimate the size for the join eqc */
	opc_ecsize(global, eqcno, &tinstr, &tops);

	ninstr += tinstr;
	nops += tops;
    }

    max_base = cnode->opc_below_rows + 1;
    max_base++;		/* account for the oj result row */

    /* Start compiling CX */
    opc_adalloc_begin(global, &cadf, &oj->oj_equal, ninstr, nops, 0, (i4) 0, 
	max_base, OPC_STACK_MEM);

    /* Compile the constant into CX */
    val = OPL_1_OJSPEQC;
    dv.db_data = &val;
    dv.db_datatype = DB_INT_TYPE;
    dv.db_prec = 0;
    dv.db_length = sizeof(OPL_OJSPEQC);
    dv.db_collID = -1;
    opc_adconst(global, &cadf, &dv, &adeops[1], DB_SQL, ADE_SMAIN);

    /* For each special attribute compile a move instruction */
    for (eqcno = -1; (eqcno = BTnext((i4)eqcno,
	    (char *)&sp1eqcmp, OPE_MAXEQCLS)) > -1; )
    {
	adeops[0].opr_dt = cnode->opc_ceq[eqcno].opc_eqcdv.db_datatype;
	adeops[0].opr_prec = cnode->opc_ceq[eqcno].opc_eqcdv.db_prec;
	adeops[0].opr_len = cnode->opc_ceq[eqcno].opc_eqcdv.db_length;
	adeops[0].opr_collID = cnode->opc_ceq[eqcno].opc_eqcdv.db_collID;
	adeops[0].opr_offset = ceq[eqcno].opc_dshoffset;
	adeops[0].opr_base = 
		    cadf.opc_row_base[cnode->opc_ceq[eqcno].opc_dshrow];
	if (adeops[0].opr_base < ADE_ZBASE || 
		adeops[0].opr_base >= ADE_ZBASE + cadf.opc_qeadf->qen_sz_base
	    )
	{
	    /* if the CX base that we need hasn't already been added
	    ** to this CX then add it now.
	    */
	    opc_adbase(global, &cadf, QEN_ROW, 
		cnode->opc_ceq[eqcno].opc_dshrow);
	    adeops[0].opr_base = 
		cadf.opc_row_base[cnode->opc_ceq[eqcno].opc_dshrow];
	}

	opc_adinstr(global, &cadf, ADE_MECOPY, ADE_SMAIN, 2, adeops, 1);
    }

    /* Move data to the special buffer. */
    for ( i = -1;
	    ( i =
	      BTnext( i, ( char * ) returnedEQCbitMap, OPE_MAXEQCLS)) > -1; )
    {
	if (sceq[i].opc_eqavailable == TRUE)
	{
	    adeops[0].opr_dt = cnode->opc_ceq[i].opc_eqcdv.db_datatype;
	    adeops[0].opr_prec = cnode->opc_ceq[i].opc_eqcdv.db_prec;
	    adeops[0].opr_len = cnode->opc_ceq[i].opc_eqcdv.db_length;
	    adeops[0].opr_collID = cnode->opc_ceq[i].opc_eqcdv.db_collID;
	    adeops[0].opr_offset = ceq[i].opc_dshoffset;
	    adeops[0].opr_base = 
			    cadf.opc_row_base[cnode->opc_ceq[i].opc_dshrow];
	    if (adeops[0].opr_base < ADE_ZBASE || 
			adeops[0].opr_base >= ADE_ZBASE + cadf.opc_qeadf->qen_sz_base
		    )
	    {
		/* if the CX base that we need hasn't already been added
		** to this CX then add it now.
		*/
		opc_adbase(global, &cadf, QEN_ROW, 
		    cnode->opc_ceq[i].opc_dshrow);
		adeops[0].opr_base = 
			cadf.opc_row_base[cnode->opc_ceq[i].opc_dshrow];
	    }

	    /* Start preparation of source operand descriptor. */
	    adeops[1].opr_dt = sceq[i].opc_eqcdv.db_datatype;
	    adeops[1].opr_prec = sceq[i].opc_eqcdv.db_prec;
	    adeops[1].opr_len = sceq[i].opc_eqcdv.db_length;
	    adeops[1].opr_collID = sceq[i].opc_eqcdv.db_collID;

	    if (sceq[i].opc_attavailable == FALSE)
	    {
		attrp = subqry->ops_attrs.opz_base->opz_attnums[
							sceq[i].opc_attno];
		if (attrp->opz_attnm.db_att_id != OPZ_FANUM)
		    opx_error(E_OP0888_ATT_NOT_HERE);

		/* Must materialize function attribute into work
		** buffer. It isn't available yet because it has a 
		** parm from the inner side of the outer join. */
		fattrp = subqry->ops_funcs.opz_fbase->opz_fatts[
							attrp->opz_func_att];
		opc_cqual(global, cnode, fattrp->opz_function->pst_right,
				&cadf, ADE_SMAIN, &adeops[1], &dummy);
	    }
	    else
	    {
		/* Finish preparation of adeops[1]. */
		adeops[1].opr_offset = sceq[i].opc_dshoffset;
		adeops[1].opr_base = cadf.opc_row_base[sceq[i].opc_dshrow];
		if (adeops[1].opr_base < ADE_ZBASE || 
			adeops[1].opr_base >= ADE_ZBASE + cadf.opc_qeadf->qen_sz_base
		    )
		{
		    /* if the CX base that we need hasn't already been added
		    ** to this CX then add it now.
		    */
		    opc_adbase(global, &cadf, QEN_ROW, sceq[i].opc_dshrow);
		    adeops[1].opr_base = 
			    cadf.opc_row_base[sceq[i].opc_dshrow];
		}
	    }

	    if (adeops[0].opr_dt	!= adeops[1].opr_dt	||
		    adeops[0].opr_prec	!= adeops[1].opr_prec	||
		    adeops[0].opr_len	!= adeops[1].opr_len
		   )
	    {
		opc_adtransform(global, &cadf, &adeops[1], &adeops[0],
			ADE_SMAIN, (bool)FALSE, (bool)FALSE);
	    }
	    else
	    {
		opc_adinstr(global, &cadf, ADE_MECOPY, ADE_SMAIN,
			2, adeops, 1);
	    }
	}	/* end if the saved version of this eqc is available */
    }	/* end of loop through returned equivalence classes */

    /* End compiling CX */
    opc_adend(global, &cadf);

    return;
}

/*{
** Name: OPC_EMPTYMAP - Fill in the "empty" eqc map.
**
** Description:
[@comment_line@]...
**	Produces the map of equivalence classes to be forced to NULL when
**	an outer join condition occurs.  Depending on what our caller wants,
**	this could be for a "left join" or a "right join."  For a left join,
**	we return a map of all eqcs from the inner stream that don't
**	equijoin at this node.  Similarly, for a right join, we return a
**	map of all outer eqcs that don't equijoin at this node.
**
**
**
** Inputs:
[@PARAM_DESCR@]...
**	global		Optimizer state
**
**	cnode		Current node.  Parent of the following variable:
**
**	cco		Outer or inner child of the above variable.
**
** Outputs:
[@PARAM_DESCR@]...
**	eqcmp		Map of equivalence classes to be filled in with
**			nulls when a "condition" (left join or right join)
**			occurs.  If cco is the outer child of cnode, then
**			this will be a map of all the eqcs from the outer
**			stream that are not join eqcs at this node.  This
**			is the set of eqcs to be forced to NULL if a right
**			join occurs.
**
**			Similarly, if cco is the inner child, then this will
**			be the map of equivalence classes to be forced to
**			NULL if a left join occurs.
**
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      20-feb-90 (stec)
**          written
**	19-oct-92 (rickh)
**	    Wrote the header.
**	03-feb-94 (rickh)
**	    Replaced 9999 diagnostics with real error messages.
**	23-oct-02 (toumi01) bug 108774
**	    Correct BITS_IN(eqcmp) to BITS_IN(*eqcmp) so that we correctly
**	    BTand our eqc maps.
[@history_template@]...
*/
VOID
opc_emptymap(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		OPO_CO		*cco,
		OPE_BMEQCLS	*eqcmp)
{
    OPO_CO	    *co = cnode->opc_cco;

    /* Initialize the output map. */
    MEfill(sizeof(*eqcmp), 0, (PTR)eqcmp);

    /* If no child specified, exit. */
    if (cco == NULL)
    {
	opx_error( E_OP08A7_BARREN );
	/* FIXME */
    }

    MEcopy((PTR)&cco->opo_maps->opo_eqcmap, sizeof(*eqcmp), (PTR)eqcmp);
    BTand((i4)BITS_IN(*eqcmp), (char *)&co->opo_maps->opo_eqcmap, (char *)eqcmp);

    /* From this map "subtract" the map of joining eqc's. */
    {
	OPO_CO	*outer = cnode->opc_cco->opo_outer;
	OPO_CO	*inner = cnode->opc_cco->opo_inner;
	OPE_BMEQCLS tmp;

	MEcopy((PTR)&outer->opo_maps->opo_eqcmap, sizeof(tmp), (PTR)&tmp);
	BTand((i4)BITS_IN(tmp), (char *)&inner->opo_maps->opo_eqcmap, (char *)&tmp);
	
	/* tmp holds the map of joining eqc's. */

	BTclearmask((i4)BITS_IN(*eqcmp), (char *)&tmp, (char *)eqcmp);
    }

    return;
}
/*
** Name: OPC_COJNULL_ADJ - Promotes non-nullable result columns
**                         to nullable.
**
**
*/
static VOID
opc_cojnull_adj(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		OPE_BMEQCLS	*empeqcmp)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    OPE_IEQCLS		eqcno;

    for (eqcno = -1; (eqcno = BTnext(eqcno, (char *)empeqcmp,
	    subqry->ops_eclass.ope_ev)) > -1;
	)
    {
	if ( cnode->opc_ceq[eqcno].opc_eqcdv.db_datatype > 0 )
	{
	    cnode->opc_ceq[eqcno].opc_eqcdv.db_datatype =
		- cnode->opc_ceq[eqcno].opc_eqcdv.db_datatype;
	    cnode->opc_ceq[eqcno].opc_eqcdv.db_length++;
	}
    }
}

/*{
** Name: OPC_COJNULL - Compile CX for oj_lnull or oj_rnull.
**
** Description:
**	This routine fills in a CX to null out the attributes from
**	one of this node's children.  This CX is executed when an
**	"outer join condition" occurs.  If the condition is "left join"
**	then we are being called to compile the LNULL CX, which nulls
**	out the attributes from the re-scannable stream.  If the condition
**	is "right join" then we are being called to compile the RNULL CX,
**	which nulls out the attributes from the driving stream.
**
**	This routine partitions the node's eqcs into 4 classes:
**
**		o	those to be set to 0
**
**		o	those to be set to 1
**
**		o	those to be filled with NULL
**
**		o	all others, which should be refreshed from the intact
**			child.
**
**
**
**
** Inputs:
**	global		describes the current state of compilation
**
**	cnode		current cost node being compiled
**
**	sp0eqcmp	special equivalence classes to force to 0
**
**	sp1eqcmp	special equivalence classes to force to 1
**
**	empeqcmp	equivalence classes which don't equijoin at this node
**			and must be filled with null values as a 
**			result of the outer join condition.  why?  it
**			turns out that a table's oj flag (special equivalence
**			class) is not visible to the QEF sorter.  however,
**			the sorter insists on sorting by ALL the attributes
**			in the tuple.  so it may happen that there is
**			garbage in these values which will cause an exception
**			during SORT.  also, these eqcs aren't being built
**			into parent node keys with references to their
**			corresponding special eqc flags.
**
**	child_ceq	equivalence classes of intact child (driving or 
**			rescannable stream), that is, the stream which is NOT
**			forced into an outer join condition.
**
** Outputs:
**	oj_null		CX is filled in.
**
**	Returns:
**	    void
**	Exceptions:
**	    maybe some ADF exceptions
**
** Side Effects:
**	    none that I know of
**
** History:
**      20-feb-90 (stec)
**          written
**	13-apr-93 (rickh)
**	    More cleanup of the outer join mysteries.  Added header comments to
**	    the LNULL/RNULL cx builder.  Reduced its complexity:  now it
**	    partitions output eqcs into 4 classes:  those set to 0, those
**	    set to 1, those filled with NULL, and those refreshed from the
**	    intact child.
**	26-may-93 (rickh)
**	    When estimating the number of instructions needed to refresh
**	    EQCs from the intact child, we were looking at a (possibly
**	    NULL) pointer in the cost node rather than at the bit map
**	    of refresh EQCs which we had just meticulously constructed.
**	20-Nov-2007 (kibro01) b119504
**	    Only add the ADE_MECOPY if the two sides are different.  Dummy cols
**	    created to prevent 0-length tuples get referenced here, and memcopy
**	    can't find the right address to copy since they haven't been 
**	    allocated any space at this point.
**	12-Dec-2005 (kschendel)
**	    Pass ptr to ptr to QEN_ADF in the node.  Make static.
[@history_template@]...
*/
static VOID
opc_cojnull(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		QEN_ADF		**oj_null,
		OPE_BMEQCLS	*sp0eqcmp,
		OPE_BMEQCLS	*sp1eqcmp,
		OPE_BMEQCLS	*empeqcmp,
		OPC_EQ		*child_ceq)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    OPO_CO		*co = cnode->opc_cco;
    OPZ_ATTS		**attnums;
    OPC_EQ		*ceq = cnode->opc_ceq;
    OPC_ADF		cadf;
    DB_DATA_VALUE	dv;
    OPL_OJSPEQC		val;
    OPE_BMEQCLS		refreshEQCs;
    ADE_OPERAND		adeops[2];
    OPE_IEQCLS		eqcno;
    i4			ninstr, tinstr;
    i4			nops, tops;
    i4			max_base;

    /*
    ** First, construct the map of all eqcs to be refreshed from the
    ** intact child.  These are all the eqcs that are "none of the above,"
    ** that is aren't set to 0, aren't set to 1, and aren't filled with
    ** NULL.
    */

    MEcopy( ( PTR ) sp0eqcmp, sizeof( refreshEQCs ),
	    ( PTR ) &refreshEQCs );
    BTor( ( i4  ) BITS_IN( *sp1eqcmp ), ( char *) sp1eqcmp,
	    ( char * ) &refreshEQCs );
    BTor( ( i4  ) BITS_IN( *empeqcmp ), ( char *) empeqcmp,
	    ( char * ) &refreshEQCs );
    BTnot( ( i4  ) BITS_IN( refreshEQCs ), ( char * ) &refreshEQCs );
    BTand( ( i4  ) BITS_IN( refreshEQCs ),
	   ( char * ) &co->opo_maps->opo_eqcmap, ( char * ) &refreshEQCs );



    attnums = (OPZ_ATTS **) subqry->ops_attrs.opz_base->opz_attnums;

    /* Estimate the size of the CX */
    tinstr = tops = ninstr = nops = 0;

    /* eqcs to be initialized to 0 */
    for (eqcno = -1; (eqcno = BTnext(eqcno,
	    (char *)sp0eqcmp, OPE_MAXEQCLS)) > -1; )
    {
	/* Estimate the size for the join eqc */
	opc_ecsize(global, eqcno, &tinstr, &tops);

	ninstr += tinstr;
	nops += tops;
    }

    /* eqcs to be initialized to 1 */
    for (eqcno = -1; (eqcno = BTnext(eqcno,
	    (char *)sp1eqcmp, OPE_MAXEQCLS)) > -1; )
    {
	/* Estimate the size for the join eqc */
	opc_ecsize(global, eqcno, &tinstr, &tops);

	ninstr += tinstr;
	nops += tops;
    }

    /* eqcs to be initialized with an empty value */
    tinstr = BTcount((char *)empeqcmp, (i4)subqry->ops_eclass.ope_ev);
    tops = tinstr * 2;

    ninstr += tinstr;
    nops += tops;

    /* Estimate no. of instructions, operands
    ** for refreshing eqcs from the intact child.
    */
    tinstr = BTcount((char *) &refreshEQCs, (i4)subqry->ops_eclass.ope_ev);
    tops = tinstr * 2;

    ninstr += tinstr;
    nops += tops;

    max_base = cnode->opc_below_rows + 2;

    /* Start compiling CX */
    opc_adalloc_begin(global, &cadf, oj_null, ninstr, nops, 0, (i4) 0, 
	max_base, OPC_STACK_MEM);

    /* Compile the constant into CX. */
    val = OPL_0_OJSPEQC;
    dv.db_data = &val;
    dv.db_datatype = DB_INT_TYPE;
    dv.db_prec = 0;
    dv.db_length = sizeof(OPL_OJSPEQC);
    dv.db_collID = -1;
    opc_adconst(global, &cadf, &dv, &adeops[1], DB_SQL, ADE_SMAIN);

    /* For each special eqc to be initialized to 0 compile a move instruction */
    for (eqcno = -1; (eqcno = BTnext(eqcno, (char *)sp0eqcmp,
	    subqry->ops_eclass.ope_ev)) > -1;
	)
    {
	if (cnode->opc_ceq[eqcno].opc_eqavailable == FALSE)
	{
	    opx_error(E_OP0889_EQ_NOT_HERE);
	}

	adeops[0].opr_dt = cnode->opc_ceq[eqcno].opc_eqcdv.db_datatype;
	adeops[0].opr_prec = cnode->opc_ceq[eqcno].opc_eqcdv.db_prec;
	adeops[0].opr_len = cnode->opc_ceq[eqcno].opc_eqcdv.db_length;
	adeops[0].opr_collID = cnode->opc_ceq[eqcno].opc_eqcdv.db_collID;
	adeops[0].opr_offset = ceq[eqcno].opc_dshoffset;
	adeops[0].opr_base = 
		    cadf.opc_row_base[ceq[eqcno].opc_dshrow];
	if (adeops[0].opr_base < ADE_ZBASE || 
		adeops[0].opr_base >= ADE_ZBASE + cadf.opc_qeadf->qen_sz_base
	    )
	{
	    /* if the CX base that we need hasn't already been added
	    ** to this CX then add it now.
	    */
	    opc_adbase(global, &cadf, QEN_ROW, 
				ceq[eqcno].opc_dshrow);
	    adeops[0].opr_base = 
		    cadf.opc_row_base[ceq[eqcno].opc_dshrow];
	}

	opc_adinstr(global, &cadf, ADE_MECOPY, ADE_SMAIN, 2, adeops, 1);
    }

    /* Compile the constant into CX. */
    val = OPL_1_OJSPEQC;
    dv.db_data = &val;
    dv.db_datatype = DB_INT_TYPE;
    dv.db_prec = 0;
    dv.db_length = sizeof(OPL_OJSPEQC);
    dv.db_collID = -1;
    opc_adconst(global, &cadf, &dv, &adeops[1], DB_SQL, ADE_SMAIN);

    /* For each special eqc to be initialized to 1 compile a move instruction */
    for (eqcno = -1; (eqcno = BTnext(eqcno, (char *)sp1eqcmp,
	    subqry->ops_eclass.ope_ev)) > -1;
	)
    {
	if (cnode->opc_ceq[eqcno].opc_eqavailable == FALSE)
	{
	    opx_error(E_OP0889_EQ_NOT_HERE);
	}

	adeops[0].opr_dt = cnode->opc_ceq[eqcno].opc_eqcdv.db_datatype;
	adeops[0].opr_prec = cnode->opc_ceq[eqcno].opc_eqcdv.db_prec;
	adeops[0].opr_len = cnode->opc_ceq[eqcno].opc_eqcdv.db_length;
	adeops[0].opr_collID = cnode->opc_ceq[eqcno].opc_eqcdv.db_collID;
	adeops[0].opr_offset = ceq[eqcno].opc_dshoffset;
	adeops[0].opr_base = 
		    cadf.opc_row_base[ceq[eqcno].opc_dshrow];
	if (adeops[0].opr_base < ADE_ZBASE || 
		adeops[0].opr_base >= ADE_ZBASE + cadf.opc_qeadf->qen_sz_base
	    )
	{
	    /* if the CX base that we need hasn't already been added
	    ** to this CX then add it now.
	    */
	    opc_adbase(global, &cadf, QEN_ROW, 
				ceq[eqcno].opc_dshrow);
	    adeops[0].opr_base = 
		    cadf.opc_row_base[ceq[eqcno].opc_dshrow];
	}

	opc_adinstr(global, &cadf, ADE_MECOPY, ADE_SMAIN, 2, adeops, 1);
    }

    /*
    ** For each eqc that should be initialized compile NULL
    ** constants and MOVE instruction.
    */
    for (eqcno = -1; (eqcno = BTnext(eqcno, (char *)empeqcmp,
	    subqry->ops_eclass.ope_ev)) > -1;
	)
    {
	DB_DATA_VALUE	dv;

	dv.db_data = NULL;
	dv.db_length = 0;
	dv.db_datatype = 0;
	dv.db_prec = 0;
	dv.db_collID = -1;

	/*
	** make sure the datatype is nullable.  this will force adc_getempty
	** to fill in a NULL value.
	*/

	if ( cnode->opc_ceq[eqcno].opc_eqcdv.db_datatype < 0 )
	{
	    adeops[0].opr_dt = cnode->opc_ceq[eqcno].opc_eqcdv.db_datatype;
	    adeops[0].opr_len = cnode->opc_ceq[eqcno].opc_eqcdv.db_length;
	}
	else
	{
	    adeops[0].opr_dt = -( cnode->opc_ceq[eqcno].opc_eqcdv.db_datatype );
	    adeops[0].opr_len = cnode->opc_ceq[eqcno].opc_eqcdv.db_length + 1;
	}

	adeops[0].opr_prec = cnode->opc_ceq[eqcno].opc_eqcdv.db_prec;
	adeops[0].opr_collID = cnode->opc_ceq[eqcno].opc_eqcdv.db_collID;
	adeops[0].opr_offset = ceq[eqcno].opc_dshoffset;
	adeops[0].opr_base = 
		    cadf.opc_row_base[ceq[eqcno].opc_dshrow];
	if (adeops[0].opr_base < ADE_ZBASE || 
		adeops[0].opr_base >= ADE_ZBASE + cadf.opc_qeadf->qen_sz_base
	    )
	{
	    /* if the CX base that we need hasn't already been added
	    ** to this CX then add it now.
	    */
	    opc_adbase(global, &cadf, QEN_ROW, 
				ceq[eqcno].opc_dshrow);
	    adeops[0].opr_base = 
		    cadf.opc_row_base[ceq[eqcno].opc_dshrow];
	}

	/* Get the default value for this att; */
	opc_adempty(global,
		    &dv,
		    adeops[0].opr_dt,
		    (i2) adeops[0].opr_prec,
		    (i4) adeops[0].opr_len);

	/* compile in the default value as a const; */
	opc_adconst(global, &cadf, &dv, &adeops[1], DB_SQL, ADE_SMAIN);

	/* compile an instruction to move the const to the result row; */
	opc_adinstr(global, &cadf, ADE_MECOPY, ADE_SMAIN, 2, adeops, 1);
    }

    /* Now move values from the intact child */

    for (eqcno = -1; (eqcno = BTnext(eqcno, (char *) &refreshEQCs,
		subqry->ops_eclass.ope_ev)) > -1;
	    )
    {
	adeops[0].opr_dt = cnode->opc_ceq[eqcno].opc_eqcdv.db_datatype;
	adeops[0].opr_prec = cnode->opc_ceq[eqcno].opc_eqcdv.db_prec;
	adeops[0].opr_len = cnode->opc_ceq[eqcno].opc_eqcdv.db_length;
	adeops[0].opr_collID = cnode->opc_ceq[eqcno].opc_eqcdv.db_collID;
	adeops[0].opr_offset = ceq[eqcno].opc_dshoffset;
	adeops[0].opr_base = 
			cadf.opc_row_base[ceq[eqcno].opc_dshrow];
	if (adeops[0].opr_base < ADE_ZBASE || 
		    adeops[0].opr_base >=
			ADE_ZBASE + cadf.opc_qeadf->qen_sz_base
		)
	{
	    /* if the CX base that we need hasn't already been added
	    ** to this CX then add it now.
	    */
	    opc_adbase(global, &cadf, QEN_ROW, 
				    ceq[eqcno].opc_dshrow);
	    adeops[0].opr_base = 
			cadf.opc_row_base[ceq[eqcno].opc_dshrow];
	}

	adeops[1].opr_dt = child_ceq[eqcno].opc_eqcdv.db_datatype;
	adeops[1].opr_prec = child_ceq[eqcno].opc_eqcdv.db_prec;
	adeops[1].opr_len = child_ceq[eqcno].opc_eqcdv.db_length;
	adeops[1].opr_collID = child_ceq[eqcno].opc_eqcdv.db_collID;
	adeops[1].opr_offset = child_ceq[eqcno].opc_dshoffset;
	adeops[1].opr_base = 
			cadf.opc_row_base[child_ceq[eqcno].opc_dshrow];
	if (adeops[1].opr_base < ADE_ZBASE || 
		    adeops[1].opr_base >=
			ADE_ZBASE + cadf.opc_qeadf->qen_sz_base
		)
	{
	    /* if the CX base that we need hasn't already been added
	    ** to this CX then add it now.
	    */
	    opc_adbase(global, &cadf, QEN_ROW, 
				    child_ceq[eqcno].opc_dshrow);
	    adeops[1].opr_base = 
			cadf.opc_row_base[child_ceq[eqcno].opc_dshrow];
	}

	if (adeops[0].opr_dt	!= adeops[1].opr_dt	||
		adeops[0].opr_prec	!= adeops[1].opr_prec	||
		adeops[0].opr_len	!= adeops[1].opr_len
	       )
	{
	    opc_adtransform(global, &cadf, &adeops[1], &adeops[0],
		    ADE_SMAIN, (bool)FALSE, (bool)FALSE);
	}
	/* Only copy if the two sides are different - they will be
	** the same for a dummy column created purely to prevent
	** 0 columns within a temporary table (kibro01) b119504
	*/
	else if (adeops[0].opr_base != adeops[1].opr_base ||
		 adeops[0].opr_offset != adeops[1].opr_offset)
	{
	    opc_adinstr(global, &cadf, ADE_MECOPY, ADE_SMAIN,
		    2, adeops, 1);
	}
    }	/* end of loop through eqcs to refresh from intact child */

    /* End compiling CX */
    opc_adend(global, &cadf);
}

/*{
** Name: OPC_OJNULL_BUILD - Fill in the LNULL and RNULL CXs
**
** Description:
**	This routine fills in the Compiled Expressions (CXs) that force
**	attributes to null when an "outer join condition" occurs.  The
**	possible outer  join conditions are left and right join.  When
**	a left join occurs, the LNULL CX is executed:  this CX forces
**	all the attributes from the rescannable stream to be null.  Similarly,
**	when a right join occurs, the RNULL CX is executed:  this CX
**	forces all the attributes from the driving stream to be null.
**
**	This routine therefore compiles the LNULL and RNULL CXs.
**
**
** Inputs:
**	global		current state of the compiler
**	cnode		current cost node that we are compiling
**	iceq		equivalence classes of the inner (rescannable) stream
**	oceq		equivalence classes of the outer (driving) stream
**
** Outputs:
**	oj		outer join descriptor.  LNULL and RNULL CXs are
**			compiled here.
**
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      10-nov-89 (stec)
**          written
**	13-apr-93 (rickh)
**	    Filled in the header.  Also, changed the calling sequence of
**	    opc_cojnull.
**	03-feb-94 (rickh)
**	    Replaced 9999 diagnostics with real error messages.
**	28-jul-98 (hayke02)
**	    The left and right null equivalence class maps (LNULLsecialEQCSto0
**	    and RNULLsecialEQCSto0, passed to opc_cojnull_adj() and
**	    opc_cojnull()) have been modified. These maps now contain all
**	    inner equivalence classes, rather than just those equivalence
**	    classes contained in the select list. This prevents incorrect
**	    resuls when a view, containing ifnull'ed result columns and an
**	    outer join, is joined to another table and the ifnull'ed columns
**	    (which are not nullable) are not contained in the select-list.
**	    This change fixes bug 91164.
**	19-jun-02 (hayke02)
**	    Extend the fix for bug 95352 to right joins by adding a
**	    BTand(..., returnedEQCbitMap, RNULLempeqcmp) call when the
**	    oj_jntype is DB_RIGHT_JOIN. This change fixes bug 99600.
[@history_template@]...
*/
VOID
opc_ojnull_build(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		QEN_OJINFO	*oj,
		OPC_EQ		*iceq,
		OPC_EQ		*oceq,
		OPE_BMEQCLS	*returnedEQCbitMap
)
{
    OPO_CO		*co = cnode->opc_cco;
    OPO_CO		*leftChild;
    OPO_CO		*rightChild;
    OPE_BMEQCLS		LNULLspecialEQCSto0;
    OPE_BMEQCLS		LNULLspecialEQCSto1;
    OPE_BMEQCLS		LNULLspecialEQCSto2;
    OPE_BMEQCLS		LNULLempeqcmp;
    OPE_BMEQCLS		RNULLspecialEQCSto0;
    OPE_BMEQCLS		RNULLspecialEQCSto1;
    OPE_BMEQCLS		RNULLspecialEQCSto2;
    OPE_BMEQCLS		RNULLempeqcmp;

    oj->oj_lnull = (QEN_ADF *)NULL;
    oj->oj_rnull = (QEN_ADF *)NULL;

    rightChild = co->opo_inner;
    leftChild = co->opo_outer;

    /* Depending on type of outer join */
    switch (oj->oj_jntype)
    {
	case DB_LEFT_JOIN:
	{
	    /* Create the special OJ eqc map to be initialized */
	    MEcopy((PTR)co->opo_variant.opo_local->opo_innereqc,
		sizeof(LNULLspecialEQCSto0),
		(PTR)&LNULLspecialEQCSto0);
	    MEfill(sizeof(LNULLspecialEQCSto1), (u_char)0,
		(PTR)&LNULLspecialEQCSto1);
/*	    opc_speqcmp(global, cnode, rightChild, &LNULLspecialEQCSto0,
		&LNULLspecialEQCSto1 ); */


	    /* Create a map of attributes to be
	    ** initialized with "empty" values.
	    */
	    opc_emptymap(global, cnode, rightChild, &LNULLempeqcmp);

	    /*
	    ** Remove these eqcs that are already in the sp0eqcmp, sp1eqcmp;
	    ** there is no need to initialize them twice.
	    */
	    BTclearmask((i4)BITS_IN(LNULLempeqcmp), (char *)&LNULLspecialEQCSto0,
			(char *)&LNULLempeqcmp);
	    BTclearmask((i4)BITS_IN(LNULLempeqcmp), (char *)&LNULLspecialEQCSto1,
			(char *)&LNULLempeqcmp);
	    MEcopy((PTR)co->opo_variant.opo_local->opo_ojlnull.opo_nulleqcmap,
		sizeof(LNULLspecialEQCSto2), (PTR)&LNULLspecialEQCSto2);
	    BTor((i4)BITS_IN(LNULLempeqcmp), (char *)&LNULLspecialEQCSto2,
		(char *)&LNULLempeqcmp);

	    BTand((i4)BITS_IN(LNULLempeqcmp), (char *)returnedEQCbitMap,
				(char *)&LNULLempeqcmp);

	    opc_cojnull_adj(global, cnode, &LNULLempeqcmp);
	    opc_cojnull(global, cnode, &oj->oj_lnull, &LNULLspecialEQCSto0,
		&LNULLspecialEQCSto1, &LNULLempeqcmp, oceq );
	    break;
	}

	case DB_RIGHT_JOIN:
	{
	    /* Create the special OJ eqc map to be initialized */
	    MEcopy((PTR)co->opo_variant.opo_local->opo_innereqc,
		sizeof(RNULLspecialEQCSto0),
		(PTR)&RNULLspecialEQCSto0);
	    MEfill(sizeof(RNULLspecialEQCSto1), (u_char)0,
		(PTR)&RNULLspecialEQCSto1);
/*	    opc_speqcmp(global, cnode, leftChild, &RNULLspecialEQCSto0,
		&RNULLspecialEQCSto1 ); */


	    /* Create a map of attributes to be
	    ** initialized with "empty" values.
	    */
	    opc_emptymap(global, cnode, leftChild, &RNULLempeqcmp);

	    /*
	    ** Remove these eqcs that are already in the sp0eqcmp, sp1eqcmp;
	    ** there is no need to initialize them twice.
	    */
	    BTclearmask((i4)BITS_IN(RNULLempeqcmp), (char *)&RNULLspecialEQCSto0,
			(char *)&RNULLempeqcmp);
	    BTclearmask((i4)BITS_IN(RNULLempeqcmp), (char *)&RNULLspecialEQCSto1,
			(char *)&RNULLempeqcmp);
	    MEcopy((PTR)co->opo_variant.opo_local->opo_ojrnull.opo_nulleqcmap,
		sizeof(RNULLspecialEQCSto2), (PTR)&RNULLspecialEQCSto2);
	    BTor((i4)BITS_IN(RNULLempeqcmp), (char *)&RNULLspecialEQCSto2,
		(char *)&RNULLempeqcmp);

	    BTand((i4)BITS_IN(RNULLempeqcmp), (char *)returnedEQCbitMap,
				(char *)&RNULLempeqcmp);

	    opc_cojnull_adj(global, cnode, &RNULLempeqcmp);
	    opc_cojnull(global, cnode, &oj->oj_rnull, &RNULLspecialEQCSto0,
		&RNULLspecialEQCSto1, &RNULLempeqcmp, iceq );
	    break;
	}

	case DB_FULL_JOIN:
	{
	    /* Create the special LNULL eqc map to be initialized */
	    opc_speqcmp(global, cnode, rightChild, &LNULLspecialEQCSto0,
		&LNULLspecialEQCSto1 );

	    /* Create the special RNULL eqc map to be initialized */
	    opc_speqcmp(global, cnode, leftChild, &RNULLspecialEQCSto0,
		&RNULLspecialEQCSto1 );

	    /* Create a map of attributes to be
	    ** initialized with "empty" values by the LNULL cx
	    */
	    opc_emptymap(global, cnode, rightChild, &LNULLempeqcmp);
	    /*
	    ** Remove these eqcs that are already in the sp0eqcmp, sp1eqcmp;
	    ** there is no need to initialize them twice.
	    */
	    BTclearmask((i4)BITS_IN(LNULLempeqcmp), (char *)&LNULLspecialEQCSto0,
			(char *)&LNULLempeqcmp);
	    BTclearmask((i4)BITS_IN(LNULLempeqcmp), (char *)&LNULLspecialEQCSto1,
			(char *)&LNULLempeqcmp);

	    /* Create a map of attributes to be
	    ** initialized with "empty" values by the RNULL cx
	    */
	    opc_emptymap(global, cnode, leftChild, &RNULLempeqcmp);
	    /*
	    ** Remove these eqcs that are already in the sp0eqcmp, sp1eqcmp;
	    ** there is no need to initialize them twice.
	    */
	    BTclearmask((i4)BITS_IN(RNULLempeqcmp), (char *)&RNULLspecialEQCSto0,
			(char *)&RNULLempeqcmp);
	    BTclearmask((i4)BITS_IN(RNULLempeqcmp), (char *)&RNULLspecialEQCSto1,
			(char *)&RNULLempeqcmp);
	    opc_cojnull_adj(global, cnode, &LNULLempeqcmp);
	    opc_cojnull_adj(global, cnode, &RNULLempeqcmp);

	    /* Left outer join part of the full join.*/
	    opc_cojnull(global, cnode, &oj->oj_lnull, &LNULLspecialEQCSto0,
		&LNULLspecialEQCSto1, &LNULLempeqcmp, oceq);

	    /* Right outer join part of the full join.*/
	    opc_cojnull(global, cnode, &oj->oj_rnull, &RNULLspecialEQCSto0,
		&RNULLspecialEQCSto1, &RNULLempeqcmp, iceq);
	    break;
	}

	default:
	{
	    opx_error( E_OP039A_OJ_TYPE );
	    /* FIXME */
	    break;
	}
    }

    return;
}

/*{
** Name: OPC_OJINFO_BUILD - Fill the outer join structure.
**
** Description:
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
**      10-nov-89 (stec)
**          written
**	25-mar-92 (rickh)
**	    Right joins now store tids in a real hold file.  Some related
**	    cleanup:  removed references to oj_create and oj_load
**	    which weren't used anywhere in QEF.  Replaced references to
**	    oj_shd with oj_tidHoldFile.  Also, right fsmjoins now
**	    remember which inner tuples inner joined by keeping a hold
**	    file of booleans called oj_ijFlagsFile.
**	11-aug-92 (rickh)
**	    Implement temporary tables for right joins.
**	12-feb-93 (jhahn)
**	    Added support for statement level rules. (FIPS)
**	29-apr-93 (rickh)
**	    Replaced call to opc_sprow with one to opc_outerJoinEQCrow.
**	    All outer joins allocate a result row now, not just full
**	    joins.
**	03-feb-94 (rickh)
**	    Replaced 9999 diagnostics with real error messages.
**	07-feb-94 (rickh)
**	    Doubled the size of the outer join TID temp table so that we
**	    can split the TID into two i4s that the sorter can understand.
**	    Compiled a key and attribute list for this temp table.
**	06-mar-96 (nanpr01)
**	    init the temp table page size. 
**	26-feb-99 (inkdo01)
**	    Added support for hash joins.
**	30-nov-05 (inkdo01)
**	    Removed an apparently superfluous call to opc_ojnull_build()
**	    added in rev 28.
**	1-dec-05 (inkdo01)
**	    Bad guess - removing the above call changed the oj_equal code 
**	    (presumably because it altered EQC descriptors in cnode).
[@history_template@]...
*/
VOID
opc_ojinfo_build(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		OPC_EQ		*iceq,
		OPC_EQ		*oceq)
{
    OPO_CO	*co = cnode->opc_cco;
    QEN_SJOIN	*sjn;
    QEN_KJOIN	*kjn;
    QEN_HJOIN	*hjn;
    QEN_TJOIN	*tjn;
    QEN_OJINFO	*oj;
    QEN_OJINFO	**ojp;
    OPE_BMEQCLS	returnedEQCbitMap;
    i4		ev_size;

    switch (cnode->opc_qennode->qen_type)
    {
	case QE_FSMJOIN:
	case QE_ISJOIN:
	case QE_CPJOIN:
	    sjn = &cnode->opc_qennode->node_qen.qen_sjoin;
	    ojp = &sjn->sjn_oj;
	    break;

	case QE_TJOIN:
	    tjn = &cnode->opc_qennode->node_qen.qen_tjoin;
	    ojp = &tjn->tjoin_oj;
	    break;

	case QE_KJOIN:
	    kjn = &cnode->opc_qennode->node_qen.qen_kjoin;
	    ojp = &kjn->kjoin_oj;
	    break;

	case QE_HJOIN:
	    hjn = &cnode->opc_qennode->node_qen.qen_hjoin;
	    ojp = &hjn->hjn_oj;
	    break;

	default:
	    opx_error( E_OP068E_NODE_TYPE );
	    /* FIXME */
	    break;
    }

    /* First, see if we've even got an oj (usually, we won't) */

    if (co->opo_variant.opo_local->opo_ojid == OPL_NOOUTER)
    {
	*ojp = (QEN_OJINFO *)NULL;	/* stuff NULL into join node */
	return;
    }

    /* If we have got an outer join, allocate the OJINFO struct */

    oj = *ojp = (QEN_OJINFO *)opu_qsfmem(global, sizeof(QEN_OJINFO));

    /* by default, the following hold files and their dsh rows don't exist */

    oj->oj_tidHoldFile = -1;
    oj->oj_heldTidRow = -1;
    oj->oj_ijFlagsFile = -1;
    oj->oj_ijFlagsRow = -1;
    oj->oj_innerJoinedTIDs = (QEN_TEMP_TABLE *)NULL;

    {
	/* The outer join case - fill it in */

	OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
	OPC_EQ		*sceq = (OPC_EQ *)NULL;

	/* Type of outer join */
	switch (co->opo_variant.opo_local->opo_type)
	{
	    case OPL_RIGHTJOIN:
		oj->oj_jntype = DB_RIGHT_JOIN;
		break;
	    case OPL_LEFTJOIN:
		oj->oj_jntype = DB_LEFT_JOIN;
		break;
	    case OPL_FULLJOIN:
		oj->oj_jntype = DB_FULL_JOIN;
		break;
	    default:
		opx_error( E_OP039A_OJ_TYPE );
		/* FIXME */
		break;
	}

	/* Fill in outer join qualifications */
	opc_ojqual(global, cnode, oj, iceq, oceq);

	/* FIXME */
	/* Get stack memory ???. */
	ev_size = subqry->ops_eclass.ope_ev * sizeof (OPC_EQ);
	sceq = (OPC_EQ *) opu_Gsmemory_get(global, ev_size);
	MEfill(ev_size, (u_char)0, (PTR)sceq);

	/* We need a special row. */
	opc_outerJoinEQCrow( global, cnode, iceq, oceq, sceq,
				&returnedEQCbitMap );

	/* This call fiddles with the EQC descriptors in cnode and is
	** essential to building the oj_equal code, even if the lnull/rnull 
	** code is replaced by the subsequent call to opc_ojnull_build(). */
	opc_ojnull_build(global, cnode, oj, iceq, oceq, &returnedEQCbitMap);

	/* Create CX for the match case. */
	opc_ojequal_build(global, cnode, oj, sceq, &returnedEQCbitMap );
	
	/* Create CX for the no match case.
	** Null/default info for left and/or right.
	*/
	opc_ojnull_build(global, cnode, oj, iceq, oceq, &returnedEQCbitMap);

	if (oj->oj_jntype == DB_RIGHT_JOIN  ||
	    oj->oj_jntype == DB_FULL_JOIN
	   )
	{
	    switch (cnode->opc_qennode->qen_type)
	    {
		case QE_ISJOIN:
		case QE_CPJOIN:

		    opc_tempTable( global, &oj->oj_innerJoinedTIDs,
			2 * sizeof( i4 ), (OPC_OBJECT_ID **) NULL);

		    opc_buildTIDsort( global, oj->oj_innerJoinedTIDs );

		    break;

		case QE_FSMJOIN:
		    /*
		    ** Fill in the info for the hold file of inner join
		    ** flags.  Each row in the hold file has one attribute:
		    ** a boolean containing TRUE or FALSE.
		    */

		    opc_pthold(global, &oj->oj_ijFlagsFile );
		    opc_ptrow(global, &oj->oj_ijFlagsRow, sizeof( bool ) );
		    break;

		default:
		    break;
	    }
	}
    }
}

/*{
** Name: opc_buildTIDsort - Build structures for sorting a file of TIDs.
**
** Description:
**	This routine builds the attribute and key structures needed to
**	sort a temporary table of TIDs.  This is used by the ISJOIN and
**	CPJOIN nodes when processing right joins.
**
**	The name (and description!) is slightly misleading.  We aren't
**	building anything for QEF-sorting to use, and there's no need to
**	allow for the usual DB_CMP_LIST in the qp.  The structures built
**	here are eventually fed into DMF more or less directly.
**
** Inputs:
**	global		global state variable
**
** Outputs:
**	tempTable
**	    ->ttb_attr_list	pointer to array of attribute descriptors
**	    ->ttb_attr_count	number of attribute descriptors (2)
**	    ->ttb_key_list	pointer to array of key descriptors
**	    ->ttb_key_count	number of key descriptors (2)
**
**	Returns:
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	07-feb-94 (rickh)
**	    Created.
**	06-mar-96 (nanpr01)
**	    Initialize ttb_pagesize variable to 2k here since row width is
**	    only 8.
**
**
[@history_template@]...
*/

#define	TID_SPLIT	2

static VOID
opc_buildTIDsort(
		OPS_STATE	*global,
		QEN_TEMP_TABLE	*tempTable
)
{
    DMF_ATTR_ENTRY	**attr_list;
    DMF_ATTR_ENTRY	*attr;
    i4			i;
    char		buf[DB_MAXNAME];
    DMT_KEY_ENTRY	**keyArray;
    DMT_KEY_ENTRY	*key;

    /* build 2 attributes.  we split the TID into two i4s */

    attr_list = (DMF_ATTR_ENTRY **)
	opu_qsfmem(global, sizeof(DMF_ATTR_ENTRY *) * TID_SPLIT );

    buf[0] = 'a';
    for ( i = 0; i < TID_SPLIT; i++ )
    {
	attr = attr_list[ i ] = ( DMF_ATTR_ENTRY * )
		opu_qsfmem(global, sizeof(DMF_ATTR_ENTRY));
	attr->attr_type = DB_INT_TYPE;
	attr->attr_size = 4;
	attr->attr_precision = 0;
	attr->attr_collID = -1;
	attr->attr_flags_mask = 0;
	SET_CANON_DEF_ID( attr->attr_defaultID, DB_DEF_ID_0 );
	attr->attr_defaultTuple = NULL;

	/* name this attribute */

	CVla((i4) i, &buf[1]);
	STmove(buf, ' ', sizeof(attr_list[i]->attr_name),
		(char*) &attr_list[i]->attr_name);
    }	/* end for */

    /* now build corresponding keys */

    keyArray = ( DMT_KEY_ENTRY ** )
	opu_qsfmem(global, sizeof( DMT_KEY_ENTRY * ) * TID_SPLIT );

    for ( i = 0; i < TID_SPLIT; i++ )
    {
	key = keyArray[ i ] = ( DMT_KEY_ENTRY * )
		opu_qsfmem(global, sizeof( DMT_KEY_ENTRY ) );

	key->key_order = DMT_ASCENDING;
	STmove( attr_list[ i ]->attr_name.db_att_name, ' ', 
	        sizeof(key->key_attr_name.db_att_name), 
		key->key_attr_name.db_att_name);

    }	/* end for */

    /* now stuff our return variables */

    tempTable->ttb_attr_list = attr_list;
    tempTable->ttb_attr_count = TID_SPLIT;
    tempTable->ttb_key_list = keyArray;
    tempTable->ttb_key_count = TID_SPLIT;
    tempTable->ttb_pagesize = DB_MINPAGESIZE;
}

/*{
** Name: OPC_TISNULL - Compile CX to check if the true TID value is NULL.
**
** Description:
**	
**	At QEF time, TID joins need to know whether they have been passed
**	a real TID from the outer stream or whether they have been passed
**	a null TID.  The latter case can occur with outer joins.  When
**	it does occur, the tjoin doesn't have to bother probing the inner
**	relation:  the columns from that relation will all be nulls.
**	
**	This routine compiles up the CX which determines whether the TID
**	is real or null.  The CX returns ADE_TRUE if the TID is null, and
**	ADE_NOT_TRUE otherwise.  QEF knows to interpret an empty CX
**	as ADE_NOT_TRUE.
**	
**	We look at the TID's eq class.  There are three cases:
**	
**	o	The TID was determined by a full join.  In this case,
**		the TID's eq class will have no attribute assigned to
**		it at compile time.  Instead, at QEF time, an RNULL
**		or LNULL CX will move the TID value into an
**		intermediate location.  Fortunately, this intermediate
**		location is known at compile time;  it is simply the
**		DSH row and offset in the TID's eq class descriptor.
**	
**		In this case, that DSH row and offset will always point
**		to a real TID.  The CX can be empty.
**	
**	o	The TID was determined by an inner join.  In this case,
**		too, the DSH row and offset in the TID's eq class will
**		point at a real TID, so the CX can be empty.
**	
**	o	Otherwise, the TID was determined by a left/right join.
**		In this case, the TID's eq class descriptor will contain
**		a valid attribute.  The relation to which that attribute
**		belongs will contain outer join information.  In particular,
**		that relation will have an associated "special equivalence
**		class."
**	
**		This is the only case in which we compile up a CX.  That
**		CX will simply check the special equivalence class to
**		see whether the TID is real or null.
**	
**	
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
**      20-feb-90 (stec)
**          written
**	23-jan-92 (rickh)
**	    Rewrote.  Used to error out for all inner joins.
**	9-feb-92 (rickh)
**	    Fixed the initialization of nops.
**	11-aug-93 (ed)
**	    assume left tid join only if join node indicates it
**	03-feb-94 (rickh)
**	    Replaced 9999 diagnostics with real error messages.
**	7-oct-99 (inkdo01)
**	    Long standing bug (99086) involving generation of TID isnull
**	    code. Check existence of TID before generating code.
[@history_template@]...
*/
VOID
opc_tisnull(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		OPC_EQ		*oceq,
		OPV_IVARS	innerRelation,
		QEN_ADF		**tisnull)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    OPC_ADF		cadf;
    DB_DATA_VALUE	dv;
    OPL_OJSPEQC		val;
    ADE_OPERAND		adeops[2];
    i4			ninstr;
    i4			nops;
    i4			max_base;
    OPE_IEQCLS		TIDeqcno;
    OPV_VARS		*opv;
    OPV_IVARS		varno;
    OPZ_IATTS		TIDattno;
    i4			i;
    i4			maxAttno;
    OPE_IEQCLS		specialEqcno;
    OPZ_ATTS		*jatt;

    /*
    ** The default case is to return a null CX.  This instructs QEF's
    ** CX executor to return FALSE, meaning that the TID is real and
    ** should be used to probe the base relation.
    */

    *tisnull = (QEN_ADF *) NULL;

    /*
    ** find the equivalence class to which this TID belongs.  first
    ** find the TID's joinop attribute.
    */

    maxAttno = (i4)subqry->ops_attrs.opz_av;
    for ( i = 0; i < maxAttno; i++ )
    {
	jatt = subqry->ops_attrs.opz_base->opz_attnums[ i ];
	if ( ( jatt->opz_varnm == innerRelation ) &&
	     ( jatt->opz_attnm.db_att_id == DB_IMTID )
	   )
	{
	    break;	/* found it! */
	}
    }
    if ( i >= maxAttno )
    {
	opx_error( E_OP08A8_MISSING_TID );	/* couldn't find the TID */
    }
    TIDeqcno = jatt->opz_equcls;

    /* now find the distinguished attribute in this equivalence class */

    TIDattno = oceq[ TIDeqcno ].opc_attno;

    /* if the TID is not the joining attribute of a FULL JOIN */

    if ( TIDattno != OPC_NOATT )
    {

	/* find the relation to which this attribute belongs */

	varno = subqry->ops_attrs.opz_base->opz_attnums[ TIDattno ]->opz_varnm;
	if ( varno < 0 )
	{
	    opx_error( E_OP08A9_BAD_RELATION );
	}
	opv = subqry->ops_vars.opv_base->opv_rt[varno];

	/*
	** now get the number of the special equivalence class for this
	** relation.
	*/

	specialEqcno = opv->opv_ojeqc;

	/*
	** If eqc not materialized by now, we probably don't need to do
	** this (e.g., if tid comes from the outer side of a join and 
	** therefore can't be null).
	*/
	if (!oceq[specialEqcno].opc_eqavailable) return;

	/*
	** If this is in fact a RIGHT or LEFT JOIN, then this is the
	** only case in which we have to compile up a CX.
	*/

	if (( specialEqcno != OPE_NOEQCLS)
	    &&
	    ( cnode->opc_cco->opo_union.opo_ojid != OPL_NOOUTER))
	{
	    /* Estimate the size of the CX */
	    ninstr = 1;
	    nops = 2;

	    max_base = cnode->opc_below_rows;

	    /* Start compiling CX */
	    opc_adalloc_begin(global, &cadf, tisnull, ninstr, nops, 0, (i4) 0, 
		max_base, OPC_STACK_MEM);

	    /* Compile "empty" constant and a BYCOMPARE instruction. */
	    val = OPL_0_OJSPEQC;	/* means no match occurred */
	    dv.db_data = &val;
	    dv.db_datatype = DB_INT_TYPE;
	    dv.db_prec = 0;
	    dv.db_length = sizeof(OPL_OJSPEQC);
	    dv.db_collID = -1;
	    opc_adconst(global, &cadf, &dv, &adeops[1], DB_SQL, ADE_SMAIN);

	    adeops[0].opr_dt = oceq[ specialEqcno ].opc_eqcdv.db_datatype;
	    adeops[0].opr_prec = oceq[ specialEqcno ].opc_eqcdv.db_prec;
	    adeops[0].opr_len = oceq[ specialEqcno ].opc_eqcdv.db_length;
	    adeops[0].opr_collID = oceq[ specialEqcno ].opc_eqcdv.db_collID;
	    adeops[0].opr_offset = oceq[ specialEqcno ].opc_dshoffset;
	    adeops[0].opr_base = 
		    cadf.opc_row_base[oceq[ specialEqcno ].opc_dshrow];

	    if (adeops[0].opr_base < ADE_ZBASE || 
		adeops[0].opr_base >= ADE_ZBASE + cadf.opc_qeadf->qen_sz_base
	    )
	    {
	        /* if the CX base that we need hasn't already been added
	        ** to this CX then add it now.
	        */
	        opc_adbase(global, &cadf, QEN_ROW,
				oceq[ specialEqcno ].opc_dshrow);
	        adeops[0].opr_base = 
		    cadf.opc_row_base[oceq[ specialEqcno ].opc_dshrow];
	    }

	    opc_adinstr(global, &cadf, ADE_COMPARE, ADE_SMAIN, 2, adeops, 0);

	    /* End compiling CX */
	    opc_adend(global, &cadf);
	}
    }

    return;
}

/*{
** Name: opc_outerJoinEQCrow - Allocate a row for eqcs
**
** Description:
**	This routine allocates a row for the eqcs returned by
**	an outer join.  Since an outer join generates an LNULL, an
**	RNULL and an OJEQUAL CX, each of which populates eqcs, it's
**	important that none of these CXs erases data needed by
**	the others.  In particular, it's important that the RNULL not
**	overwrite eqcs from the driving stream since these may
**	be needed by an immediately following LNULL.
**
**	Therefore, we allocate a row for the eqcs returned by
**	outer joins.  Both the LNULL and RNULL CXs write into this row,
**	leaving eqcs from the child streams unmolested.  The OJEQUAL
**	CX also writes into this row.
**
**
** Inputs:
**	global			current state of compilation
**	cnode			current node being compiled
**	iceq			inner child's equivalence classes
**	oceq			outer child's equivalence classes
**
** Outputs:
**	savedEQCs		snapshot of this node's equivalence classes
**				before the special row was allocated
**
**	returnedEQCbitMap	equivalence classes in the special row
**
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**	20-apr-93 (rickh)
**	    Wrote it.  Supercedes opc_sprow.  Filled in the attribute number
**	    for equivalence classes which can only come from one stream.
**	    This fixes a problem with full joins feeding tidjoins:  the
**	    TISNULL CX was being left empty because the compiler couldn't
**	    figure out which special eqc to look at.
**	03-feb-94 (rickh)
**	    Replaced 9999 diagnostics with real error messages.
**	31-oct-96 (kch)
**	    Add one to attributeWidth to increase the spacing between row
**	    buffers. This will guarantee enough space to copy the NULL
**	    'versions' of non-nullable datatypes filled out in opc_cojnull().
**	    This change fixes bug 78496.
**	26-feb-99 (inkdo01)
**	    Added support for hash joins.
**
[@history_template@]...
*/
static VOID
opc_outerJoinEQCrow(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		OPC_EQ		*iceq,
		OPC_EQ		*oceq,
		OPC_EQ		*savedEQCs,
		OPE_BMEQCLS	*returnedEQCbitMap
)
{
    OPO_CO		*co = cnode->opc_cco;
    OPC_EQ		*currentEQC, *innerEQC, *outerEQC;
    OPE_IEQCLS		eqcno;
    QEN_OJINFO		*ojInfo;
    QEN_SJOIN		*sortMergeJoinDescriptor;
    QEN_KJOIN		*keyJoinDescriptor;
    QEN_TJOIN		*tidJoinDescriptor;
    QEN_HJOIN		*hashJoinDescriptor;
    OPO_CO		*leftChild;
    OPO_CO		*rightChild;
    OPC_EQ		*thisNodesEQCs = cnode->opc_ceq;
    i4			rowSize;
    i4			alignmentPad;
    i4			attributeWidth;
    i4			EQCrowNumber = global->ops_cstate.opc_qp->qp_row_cnt;
    DB_DATA_VALUE	*dataValue;
    DB_DATA_VALUE	rdv;	/* result data value */
    DB_DATA_VALUE  	*idv;	/* inner data value */
    DB_DATA_VALUE	*odv;	/* outer data value */

    switch ( cnode->opc_qennode->qen_type )
    {
	case QE_FSMJOIN:
	case QE_ISJOIN:
	case QE_CPJOIN:
	    sortMergeJoinDescriptor = &cnode->opc_qennode->node_qen.qen_sjoin;
	    ojInfo  = sortMergeJoinDescriptor->sjn_oj;
	    break;

	case QE_TJOIN:
	    tidJoinDescriptor = &cnode->opc_qennode->node_qen.qen_tjoin;
	    ojInfo  = tidJoinDescriptor->tjoin_oj;
	    break;

	case QE_KJOIN:
	    keyJoinDescriptor = &cnode->opc_qennode->node_qen.qen_kjoin;
	    ojInfo  = keyJoinDescriptor->kjoin_oj;
	    break;

	case QE_HJOIN:
	    hashJoinDescriptor = &cnode->opc_qennode->node_qen.qen_hjoin;
	    ojInfo  = hashJoinDescriptor->hjn_oj;
	    break;

	default:
	    opx_error( E_OP068E_NODE_TYPE );
	    /* FIXME */
	    break;
    }

    /* assume there is no special row for returned eqcs */

    ojInfo->oj_resultEQCrow = -1;

    /*
    ** First, we construct a bitmap of the eqcs we care about.
    ** These are all the eqcs created by our children that we
    ** also return.
    */

    rightChild = co->opo_inner;
    leftChild = co->opo_outer;

    MEfill( sizeof( *returnedEQCbitMap ), ( u_i2 ) 0,
	    ( PTR ) returnedEQCbitMap );

    BTor( ( i4  ) BITS_IN( OPE_BMEQCLS ), /* re-scannable child's eqcs */
	  ( char * ) &rightChild->opo_maps->opo_eqcmap,
	  ( char * ) returnedEQCbitMap );
    BTor( ( i4  ) BITS_IN( OPE_BMEQCLS ), /* driving child's eqcs */
	  ( char * ) &leftChild->opo_maps->opo_eqcmap,
	  ( char * ) returnedEQCbitMap );
    BTand( ( i4  ) BITS_IN( OPE_BMEQCLS ), /* this node's result eqcs */
	   ( char * ) &co->opo_maps->opo_eqcmap, ( char * ) returnedEQCbitMap );

    for ( eqcno = -1, rowSize = 0;
	  ( eqcno =
	  BTnext( eqcno, ( char * ) returnedEQCbitMap, OPE_MAXEQCLS)) > -1; )
    {
	currentEQC = &thisNodesEQCs[ eqcno ];
	innerEQC = &iceq[ eqcno ];
	outerEQC = &oceq[ eqcno ];

	if ( currentEQC->opc_eqavailable != TRUE)
	{
	    opx_error( E_OP0889_EQ_NOT_HERE );
	    /* FIXME */
	}

	/* Save old ceq value for building the OJEQUAL CX */
	STRUCT_ASSIGN_MACRO( *currentEQC, savedEQCs[ eqcno ] );

	/*
	**	For an outer join, the resulting datatype
	**	must be such that both contributing attributes
	**	can be coerced into it (this special row is shared).
	**	So it must be nullable if any of the attributes is
	**	nullable and the length must be max of both lengths.
	*/

	idv = &innerEQC->opc_eqcdv;
	odv = &outerEQC->opc_eqcdv;

	if ( innerEQC->opc_eqavailable != TRUE )
	{
	    STRUCT_ASSIGN_MACRO( *odv, rdv );
	    currentEQC->opc_attno = outerEQC->opc_attno;
	}
	else if ( outerEQC->opc_eqavailable != TRUE )
	{
	    STRUCT_ASSIGN_MACRO( *idv, rdv );
	    currentEQC->opc_attno = innerEQC->opc_attno;
	}
	else
	{
	    rdv.db_datatype = DB_NODT;
	    rdv.db_prec = 0;
	    rdv.db_length = 0;
	    rdv.db_collID = -1;
	    rdv.db_data = NULL;

	    opc_resolve(global, idv, odv, &rdv);

	    currentEQC->opc_attno = OPC_NOATT;
	}

	/*
	** add this eqc to the output row.  note that the eqc now
	** lives in this special row at this offset.
	*/

	/* Assign new values. */
	currentEQC->opc_eqavailable = TRUE;
	currentEQC->opc_attavailable = TRUE;
	currentEQC->opc_dshrow = EQCrowNumber;
	currentEQC->opc_dshoffset = rowSize;
	STRUCT_ASSIGN_MACRO( rdv, currentEQC->opc_eqcdv );

	dataValue = &currentEQC->opc_eqcdv;

	attributeWidth = dataValue->db_length;
	if (dataValue->db_datatype > 0)attributeWidth++;
			/* we don't always get the forced null cols
			** right, so add a byte to the buffer for
			** NOT NULL's, just to be safe */
	alignmentPad = attributeWidth % sizeof ( ALIGN_RESTRICT );
	if ( alignmentPad != 0)
	{
	    attributeWidth += ( sizeof (ALIGN_RESTRICT) - alignmentPad );
	}
	rowSize += attributeWidth;

    }	/* end of loop through equivalence classes */

    /* allocate the row to expose these eqcs to higher nodes */

    if ( rowSize > 0 )
    { opc_ptrow( global, &ojInfo->oj_resultEQCrow, rowSize ); }
}
