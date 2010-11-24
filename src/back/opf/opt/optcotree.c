/* Copyright (c) 1995, 2005 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <cui.h>
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
#include    <uld.h>
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
#define        OPT_COTREE      TRUE
#include    <me.h>
#include    <bt.h>
#include    <cv.h>
#include    <tr.h>
#include    <st.h>
#include    <opxlint.h>
/**
**
**  Name: OPTCOTREE.C - print trace information
**
**  Description:
**      Routines to print out trace information.  These routines are also
**      used directly by the a user of the VMS debugger which allows out of line
**      calls to procedures.  This is especially useful for probing data
**      structures symbolically while debugging.  This is the reason for 
**      the vast collection of
**      small routines, and the coding style which attempts to minimize the
**      number of parameters to each routine.  If the environment is set the
**      the VMS debug macros can be included by executing
**		@jup_opf_tst:dbginit.
**      To use most of the macros, the context of the subquery must be set 
**      so that the proper joinop tables can be referenced.  This can be
**      done with the set subquery ptr command.
**              ssq <subquery ptr>      
**
**	The following description gives 
**		a) the procedure name
**              b) macro name
**		c) macro parameters
**              d) followed by a short description
**      
****** ENUMERATION PHASE MACROS ******
** opt_rvtrl   prvtrl <varno>       joinop range var for trl to print
**          - print trl struct associated with joinop range var
** opt_trl     ptrl <trlp>          ptr to trl element
**          - print OPN_RLS struct
** opt_esp    pesp <eqsp>           ptr to OPN_EQS structure to print
**          - print OPN_EQS structure 
** opt_stp    pstp <stp>            ptr to OPN_SUBTREE structure to print
**          - print OPN_SUBTREE structure
** opt_evp	
**	    - print OPN_EVAR array
******* JOIN TREE MACROS *******
** opt_jnode  pj <jtreeptr>         ptr to OPN_JTREE structure
**          - print the JTREE structure
** opt_jtree  pjtree
**          - print the operator tree structure currently rooted in subquery
** opt_fjtree  pfjtree              
**          - print formatted join tree
** opt_ojstuff
**          - prints contents of local outer join table
******* HISTOGRAM MACROS *******
** opt_hist    ph  <histp>          ptr to histogram element
**          - print histogram element
** opt_hrv    phrv <varno>          joinop range var for histogram list
**          - print histogram list of trl struct associated with joinop var
******* CO TREE MACROS *******
** opt_cotree  pcot <cop>           ptr to root of CO tree to print
**          - print co tree
** opt_codump  pcop <cop>           ptr to CO node to print
**          - print one COP
** opt_bco     pco
**          - print the best CO tree found so far 
******* AGGREGRATE MACROS ******
** opt_agall   paggall
**          - print info on aggregates subqueries being computed
** opt_agg     pagg <aggno>         number of relative position in subquery list
**          - print info on particular aggregate subquery
******* EQUIVALENCE CLASS MACROS *******
** opt_eall    peall
**	    - print all the equivalence class structures
** opt_eclass  peq <eqcls>	    equivalence class number
**          - print info on one equivalence class
******* JOINOP ATTRIBUTE MACROS *******
** opt_anum    pan <attr no>        joinop attribute number
**          - print info on joinop attribute
** opt_attnums paall
**          - print info on joinop attributes table
** opt_fattr   pfa <function attr no> function attribute number
**          - print info on a function attribute
** opt_fall    pfall
**          - print info on all function attributes
** opt_ftype   pftype <class>       OPZ_FACLASS type
**          - function attribute class
******* RANGE TABLE MACROS ********
** opt_gtable  pgt
**          - print entire global range table
** opt_grange  pgv <grv>            global range var number
**          - print global range table element grv
** opt_rv      prv <rv>             joinop range variable number
**          - print range var element
** opt_rall    prall
**          - print joinop range table
******* BOOLEAN FACTOR MACROS ********
** opt_ball    pball
**          - print all information on boolean factors
** opt_bf      pbf <bf index>       boolean factor number
**          - print info on one particular boolean factor
** opt_big     pbig <bfkeyinfo ptr> ptr to bfkeyinfo structure
**          - print OPB_GLOBAL portion of bfkeyinfo struct
** opt_bi      pbi <bfkeyinfo ptr>  ptr to bfkeyinfo structure
**          - print info on OPB_BFKEYINFO struct
** opt_bv      pbv <bfvallist ptr>  ptr to bfvallist structure
**          - print info on OPB_BFVALLIST struct
** opt_bvlist  pbvl <bfvallist ptr> ptr to first bfvallist structure in list
**          - print list of bfvallist objects
******* RDF TRACE MACROS *********
** opt_rdall   prdfall
**          - print info about all RDF tables referenced in query
** srdf <rdrinfop>      RDF info ptr of relation to dump
**          - set context for all access by RDF commands
** opt_rdf     prdf 	            need srdf <rdrinfop> to set context
**          - print entire RDF info for this relation
** opt_key     prkey                need srdf <rdrinfop> to set context
**          - print RDF key list for relation ordering
** opt_pindex  pri <indexno>        offset in RDF array of index for table
**          - dump info on RDF index tuple
** opt_index   prindex              need srdf <rdrinfop> to set context
**          - print all RDF index info
** opt_prattr  pra <attr no>	    dmf attribute number of attr to print
**          - print info on one dmf attribute
** opt_rattr   prattr               need srdf <rdrinfop> to set context
**          - print all RDF attribute info
** opt_tbl     prtable              need srdf <rdrinfop> to set context
**          - print RDF table descriptor
******* QUERY TREE TRACE   *******
** opt_mode    pmode <int>          mode number to translate
**          - print symbolic name of mode number
******* PARSE TREE ROUTINES ******
** opt_pt_entry - Format and print contents of a query tree in the 
**                        more tabular structure-oriented fashion. (op170)
******* ADF TRACE ROUTINES *******
** opt_type    ptype <int>          ADF datatype id
**          - print symbolic name of ADF type
** opt_storage pstor <int>          dmf storage class
**          - print symbolic name of storage class
** opt_pop     pop <int>            ADF operator ID
**          - print adf operator name
** opt_sarg    psarg <int>          sarg integer value
**          - prints symbolic name of sarg
** opt_element pe   <valuep> <datatype>  valuep points to value and datatype 
**                                  points to a DB_DATA_VALUE
**          - print an element
** opt_string  ps   <valuep> <length> valuep points to value and length
**                                  is number of bytes to display
**          - print a character array and display all transparent characters
**	 prRangev	- print "vars" structs pointed at by Rangev array and
**		 	  the associated "co", "eqs" and "rls" structs
**			  pointed at by the "vars" struct.  Also print all
**			  "nrmlhist" structs in the linked list pointed
**			  at by the "rls" struct.
**	 prEqclist	- print "eqclist" structs pointed at by Jn.Eqclist
**			  array.
**	 prAttnums	- print "atts" structs pointed at by Jn.Attnums array.
**	 pSavearr	- print all "rls" structs in linked lists pointed at
**			  by the Jn.Savearr array.
**	 prls		- print all "rls" structs in a linked list along with
**			  all "eqs" structs in the linked list to which it
**			  points.
**	 prrls		- print "rls" struct.
**	 peqs		- print all "eqs" structs in a linked list along with
**			  the subtrees to which they point.
**	 preqs		- print an "eqs" struct.
**	 psubtrees	- print all "subtrees" structs in a linked list.
**	 prsubtrees	- print a "subtrees" struct along with all "co" structs
**			  in the linked list to which it points.
**	 pco		- print all "co" structs in a linked list.
**	 prco		- print a "co" struct.
**	 prBoolf	- print all "boolfact" structs pointed at by the
**			  Jne.Boolfact array including the tree representation
**			  of its boolean factor.
**	 prhist		- print all the "nrmlhist" structs in the linked list.
**			  For each "nrmlhist" struct print the cell counts
**			  and boundaries.
**	 pcotree	- print a binary tree of "co" structs which correspond
**			  to a solution.
**	 clson, crson	- return a pointer to the left or right subtree of a
**			  "co" struct.
**	 cptpr		- point the info for a node in the "co"-tree.
**	 pmap		- print a bitmap.
**	 prarray	- print an array of a given type and length.
**	 prelement	- print an element of a given type and length.
**	 printop	- convert a manifest constant into a mneumonic and
**			  print it.
**  History:
**      18-jul-86 (seputis)    
**          initial creation
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**	10-dec-90 (neil)
**	    Alerters: replaced cases of unused modes.
**	17-sep-90 (fpang)
**	    Cast dd_t1_name to DD_NAME *, for unix port.
**      26-mar-91 (fpang)
**          Renamed opdistributed.h to opdstrib.h
**	14-may-92 (rickh)
**	    New routine (opt_cotree_without_stats) which calls the extracted
**	    guts of opt_cotree to print out a cost tree without statistics.
**	    This new routine is called when OP199 is set.  The point is to
**	    write regression tests that can verify QEN tree shapes
**	    without vomiting up pages of difs whenever the stats change
**	    slightly.
**	4-may-1992 (bryanp)
**	    Add support for PSQ_DGTT and PSQ_DGTT_AS_SELECT
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      7-sep-93 (ed)
**          added uld.h for uld_prtree
**	15-sep-93 (swm)
**	    Moved cs.h include above other header files which need its
**	    definition of CS_SID.
**	11-oct-93 (johnst)
**	    Bug #56449
**	    Changed TRdisplay format args from %x to %p where necessary to
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
**	15-feb-94 (swm)
**	    Bug #59611
**	    Replace STprintf calls with calls to TRformat which checks
**	    for buffer overrun and use global format buffer to reduce
**	    likelihood of stack overflow.
**      16-nov-94 (inkdo01)
**          Introduced opt_ojstuff to print contents of outer join table
**      1-feb-95 (inkdo01)
**          Changed coprintGuts to use bfcount to determine NU for
**          secondary index reporting (bug 63300)
**      21-mar-95 (inkdo01)
**          Added opt_pt_... support for more detailed output (invoked by 
**          op170).
**      31-mar-95 (wolf) 
**          Change memset() call in opt_pt_prmdump to MEfill().
**      25-oct-95 (inkdo01)
**	    Minor fix to pst_s_const display
**	23-sep-1996 (canor01)
**	    Move global data definitions to optdata.c.
**	19-dec-1996 (canor01)
**	    Remove READONLY from GLOBALREF. This will not work with VMS.
**	6-may-97 (inkdo01)
**	    Changed numerous %x's to %p's and prepended &'s to numerous 
**	    structure function parms to make compilers generate correct 
**	    function calls.
[@history_line@]...
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**	2-sep-99 (inkdo01)
**	    Add more parse tree node types to literal array for case function.
**	11-jun-2001 (somsa01)
**	    In opt_element(), to save stack space, dynamically allocate
**	    space for tempbuf.
**	22-Jul-2005 (toumi01)
**	    Print new ops_mask2, the addition of which was necessitated
**	    by integration of change 472571 from the 2.6 code line,
**	    as Doug and Keith had completed to use up the last flag bit.
**	3-Sep-2005 (schka24)
**	    Remove bogus grv pointer to histogram, which was never set
**	    anywhere.
**      25-Oct-2005 (hanje04)
**          Move declaration of opt_ffat() here from opclint.h as it's a static
**          function and GCC 4.0 doesn't like it declared otherwise.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	20-may-2008 (dougi)
**	    Made a variety of changes to support the display of table procedure
**	    query plan nodes.
**	25-Jun-2008 (kiria01) SIR120473
**	    Move pst_isescape into new u_i2 pst_pat_flags.
**	    Widened pst_escape.
**	24-Feb-2009 (kiria01) b121707
**	    Support MOP and OPERAND properly and trigger error if
**	    bad node present.
**	3-Jun-2009 (kschendel) b122118
**	    Minor cleanups, mostly using %p instead of %x for addresses.
**	27-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-selects - cleaned up dump code to use the available
**	    table macros. Added PST_FWDVAR support.
**     28-Oct-2009 (maspa05) b122725
**          ult_print_tracefile now uses integer constants for the type
**          parameter e.g. SC930_LTYPE_QEP instead of "QEP"
**	18-Nov-2009 (kiria01) SIR 121883
**	    Dump union info too.
**	01-Mar-2010 (smeke01) b123333
**	    Improve trace point op188.
**	22-Mar-2010 (smeke01) b123454
**	    Add pst_opno translation to trace point op170.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      17-Aug-2010 (horda03) b124274
**          Enable segmented QEPs.
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
**/
struct	tab
{
	i4	t_opcode;
	char	*t_string;
};
#define OPT_TEND (-99)
GLOBALREF 	struct tab	opf_sjtyps_tab[];

/* Max number of subqueries to track for op170 */
#define MAX_ALREADY_DONE 50

/* More constant arrays */

static char *ntype_array[] = {
#define _DEFINE(n,s,v) "PST_" #n,
#define _DEFINEEND
        PST_TYPES_MACRO
#undef _DEFINE
#undef _DEFINEEND
};

static char *mtype_array[] = {
#define _DEFINE(n,v,x) #n,
#define _ENDDEFINE
    " ", PSQ_MODES_MACRO
#undef _DEFINE
#undef _ENDDEFINE
};

static void opt_big(
	OPB_BFKEYINFO      *bfinfop);

static VOID
opt_pt_prmdump(
	PST_QNODE*          node,
	i2                 indent);
static VOID
opt_pt_op_node(
	PST_OP_NODE*        node,
	char*              indent,
        char*              temp);
static VOID
opt_pt_var_node(
	PST_VAR_NODE*       node,
	char*              indent,
        char*              temp);
static VOID
opt_pt_fwdvar_node(
	PST_FWDVAR_NODE*   node,
	char*              indent,
        char*              temp);
static VOID
opt_pt_case_node(
	PST_CASE_NODE    *vnode,
        char            *indentstr,
        char            *temp);
static VOID
opt_pt_seqop_node(
	PST_SEQOP_NODE    *snode,
        char            *indentstr,
        char            *temp);
static VOID
opt_pt_gbcr_node(
	PST_GROUP_NODE    *snode,
        char            *indentstr,
        char            *temp);
static VOID
opt_pt_root_node(
	PST_RT_NODE*        node,
	char*              indent,
        char*              temp);
static VOID
opt_pt_resdom_node(
	PST_RSDM_NODE*      node,
	char*              indent,
        char*              temp);
static VOID
opt_pt_const_node(
	PST_CNST_NODE*      node,
	char*              indent,
        char*              temp);
static VOID
opt_pt_sort_node(
	PST_SRT_NODE*       node,
	char*              indent,
        char*              temp);
static VOID
opt_pt_tab_node(
	PST_TAB_NODE*       node,
	char*              indent,
        char*              temp);
static VOID
opt_pt_curval_node(
	PST_CRVAL_NODE*     node,
	char*              indent,
        char*              temp);
static VOID
opt_pt_rulevar_node(
	PST_RL_NODE*        node,
	char*              indent,
        char*              temp);
static VOID
opt_pt_hddump(
	PST_QTREE	   *header,
	bool		   printviews);
static VOID
opt_qep_codump(
	OPS_STATE      	   *global,
	OPS_SUBQUERY	   *subquery,
	OPO_CO		   *conode,
	i4		   indent);
static VOID
opt_qepd_alist(
	OPS_STATE	   *global,
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop,
	OPO_ISORT	   ordering,
	char		   *prbuf);
static VOID opt_ffat(
                OPS_SUBQUERY       *subquery,
                PST_QNODE          *qnode,
                OPT_NAME           *attname,
                bool               *found);

static VOID opt_1fidop(
        ADI_OP_ID          opno,
        char               *buf);

#if 0


prarray (name, arr, aln, typ, len)      /* "name" is a header, arr is a pointer
					   to an array with aln elements each
					   of type "typ" and length "len" */
char   *name;
char   *arr;
i4	aln, typ, len;
{
    register i4    i;
    register char  *a;
    register i4    l;

    a = arr;
    i = aln;
    l = len;

    TRdisplay ("%s:", name);

    while (i--)
    {
	prelement (a, typ, l);
	a += l;
    }
    opt_newline ();
}


printop (opcode, oparr)			/* print the val'th element of array
					   arr after checking to see if val is
					   within range (used to convert
					   manifest constants to more
					   meaningful mneumonics) */
register struct tab *oparr;
i4		opcode;
{
	for (; oparr->t_opcode != opcode && oparr->t_opcode != -99; oparr++);
	TRdisplay("%s", oparr->t_string);
}


tabtab ()
{
    TRdisplay ("\t\t");
}


prdbg (tt)				/* call with a trace flag number */
i4	tt;
{
    register i4    t;

    t = tt;

    if (!tTf(t, -1))
	return;
    if (tTf(t, 0))
	prRangev ();
    if (tTf(t, 1))
	prEqclist ();
    if (tTf(t, 2))
	prAttnums ();
    if (tTf(t, 3))
	fmttree (Jn.Root, pnode, lson, rson, (i4) 3, (i4) 5);
    if (tTf(t, 4))
	prBoolf ();
}

pSavearr (b, e)
i4	b, e;
{
    register i4    i;

    TRdisplay ("\nSAVEARR:\n");
    for (i = b; i < e; i++)
	prls (Jn.Savearr[i]);
}


prls (rp)
struct rls *rp;
{
    register struct rls *r;

    for (r = rp; r; r = r->rlnext)
	ptrls (r);
}

ptrls (rp)
struct rls *rp;
{
    register struct rls *r;

    r = rp;
    prrls (r);
    prhist (r->nhist);
    peqs (r->eqp);
}

prrls (rp)
struct rls *rp;
{
    register struct rls *r;

    r = rp;
    if (!r)
	return;
    TRdisplay ("\nrls struct, address:%p delflg:%d\n", r, r->delflg);
    TRdisplay ("\t\tereltups:%.1f pointers, rlnext:%p eqp:%p nhist:%x\n\t\t",
	    r->ereltups, r->rlnext, r->eqp, r->nhist);
    pmap ("relmap", r->relmap, (i4)JNMAXVARS);
}

peqs (ep)
struct eqs *ep;
{
    register struct eqs *e;

    for (e = ep; e; e = e->eqnext)
    {
	preqs (e);
	psubtrees (e->stp);
    }
}

preqs (ep)
struct eqs *ep;
{
    register struct eqs *e;

    e = ep;
    if (!e)
	return;
    TRdisplay ("\neqs struct, address:%p\n", e);
    TRdisplay ("\t\terelwid:%d pointers, eqnext:%p, stp:%p\n\t\t",
	    e->erelwid, e->eqnext, e->stp);
    pmap ("eqcmp", e->eqcmp, (i4)JNMAXATS);
}

psubtrees (sp)
struct subtrees *sp;
{
    register struct subtrees   *s;

    for (s = sp; s; s = s->stnext)
	prsubtrees (s);
}

prsubtrees (sp)
struct subtrees *sp;
{
    register struct subtrees   *s;

    s = sp;
    TRdisplay ("\nsubtrees struct, address:%p\n", s);
    TRdisplay ("\t\tpointers, coforw:%p coback:%p stnext:%p\n\t\t",
	    s->coforw, s->coback, s->stnext);
    prarray ("relasgn", s->relasgn, (i4)JNMAXLEAVES, (i4)INT, (i4)1);
    tabtab ();
    prarray ("structure", s->structure, (i4)JNMAXNODES, (i4)INT, (i4) 1);
    pco ((struct co *)s);
}

pco (cp)
struct co  *cp;
{
    register struct co *c;
    register struct co *cpp;

    cpp = cp;

    for (c = cpp->coforw; c != cpp; c = c->coforw)
	prco (c);
}


prco (cp)
struct co	*cp;
{
    register struct co	*c;
    FUNC_EXTERN char		*relstrct();


    c = cp;

    if (c)
    {
	    TRdisplay ("\nco struct, address:%p\n", c);
	    TRdisplay ("\t\tcdio:%.1f cpu:%.1f creltotb:%.1f %s cordeqc:%d\n",
		    c->cdio, c->cpu, c->creltotb, relstrct (c->opo_storage), c->cordeqc);
	    TRdisplay ("\t\tctups:%.1f cpagestouched:%.1f\n", c->ctups,
				c->cpagestouched);
	    TRdisplay ("\t\tcrelprim:%.1f cadf:%.1f csf:%.1f csjpr:%d\n",
		    c->crelprim, c->cadf, c->csf, c->csjpr);
	    TRdisplay ("\t\tcscanflag:%d csjpreq:%d cdeleteflag:%d cpointercnt:%d\n",
		    c->cscanflag, c->csjpreq, c->cdeleteflag, c->cpointercnt);
	    TRdisplay ("\t\topo_var:%d ctpb:%.2f cbi:%d cexistence_only:%d\n",
		    c->opo_union.opo_orig, c->ctpb, c->cbi, c->cexistence_only);
	    TRdisplay ("\t\t pointers, coforw:%p coback:%p inner:%p outer:%p\n",
		    c->coforw, c->coback, c->opo_inner, c->opo_outer);
	    if (c->ceqcmp)
		pmap ("\t\tceqcmp", c->ceqcmp, (i4)JNMAXATS);
	    else
		TRdisplay("\t\tceqcmp = 0\n");
    }
}


    for (i = 0; i < Jn.Rv; i++)
    {
    register i4			i;
    register struct vars	*r;
    i4				j,
				a;


    TRdisplay ("\nJn.Rangev: Jn.Rv:%d Jn.Prv:%d\n", Jn.Rv, Jn.Prv);

	r = Jn.Rangev[i];
	TRdisplay ("i:%d ", i);
	TRdisplay ("name:%14s\n", r->rldesc->rd_rel.relid);
	TRdisplay ("\t\tprimary:%d indx:%d\n",
		r->primary, r->indx);
	if (i >= Jn.Prv)
	    TRdisplay ("\tidomnumb:%d\n", r->idomnumb);
	else
	    TRdisplay ("\teqclass of implicit JNTID:%d\n", r->idomnmbt);

	if (r->ordattlp)
	{
		TRdisplay("\tordattlp:");
		for (j = 0; j <= MAXDOM; j++)
		{
			if ((a = r->ordattlp[j]) < 0)
			{
				opt_newline();
				break;
			}
			TRdisplay(" %d", a);
		}
	}
	if (r->mbf)
	{
		TRdisplay("\tmbf:");
		for (j = 0; j <= MAXDOM; j++)
		{
			if ((a = r->mbf[j].kattno) < 0)
			{
				opt_newline();
				break;
			}
			TRdisplay(" (%d,%d)", a, r->mbf[j].kbfi);
		}
	}
	prco (r->tcop);
	preqs (r->teqm);
	prls (r->trl);
    }
}
prBoolf ()
{
    i4    i;
    struct boolfact   *bp;

    TRdisplay ("\nJne.Boolfact:\n");
    for (i = 0; i < Jn.Bv; i++)
	if (bp = Jnn.Boolfact[i])
	{
	    TRdisplay ("\ti:%d eqcls:%d bfselect:%.5f\n",
		    i, bp->eqcls, bp->bfselect);
	    pmap ("eqcmap", bp->eqcmap, (i4) JNMAXATS);
	    fmttree (bp->clp, pnode, lson, rson, (i4) 3, (i4) 5);
	    prhist (bp->nrmp);
	    prbfkeylist(bp->bfkeys, (i4) 16);
	}
}

# endif

/*{
** Name: opt_scc	- send output line to front end
**
** Description:
**      send a formatted output line to the front end
**
** Inputs:
**      msg_buffer                      ptr to message
**      length                          length of message
**      global                          ptr to global state variable
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      13-jul-87 (seputis)
**          initial creation
**	21-may-89 (jrb)
**	    renamed scf_enumber to scf_local_error
**	19-Aug-2009 (kibro01) b122509
**	    Add in check that if sc930 tracing file is open, output to that
**	    instead of front end.
[@history_template@]...
*/
i4
opt_scc(
	PTR		   global,
	i4		   length,
	char               *msg_buffer)
{
    SCF_CB                 scf_cb;
    DB_STATUS              scf_status;
    char		   buffer[5];   /* temp buffer to contain newline*/
    PTR			   sc930_trace;

    if (length == 0)
    {
	msg_buffer = &buffer[0];
	(VOID) STprintf(msg_buffer, " \n");
	length = STlength(msg_buffer);
    }
    sc930_trace = ops_getcb()->sc930_trace;
    if (sc930_trace)
    {
	char *x;
	msg_buffer[length]=EOS;		    /* NULL terminate the string */
	for (x=msg_buffer;*x;x++)
	    if (*x=='\n') *x=' ';
	ult_print_tracefile(sc930_trace,SC930_LTYPE_QEP,msg_buffer);
	return (0);
    }
    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_OPF_ID;
    scf_cb.scf_nbr_union.scf_local_error = 0; /* this is an ingres error number
					** returned to user, use 0 just in case 
					*/
    scf_cb.scf_len_union.scf_blength = (u_i4)length;
    scf_cb.scf_ptr_union.scf_buffer = msg_buffer;
    scf_cb.scf_session = DB_NOSESSION;	    /* print to current session */
    scf_status = scf_call(SCC_TRACE, &scf_cb);
    if (scf_status != E_DB_OK)
    {
	TRdisplay("SCF error %d displaying formatted tree to user\n",
	    scf_cb.scf_error.err_code);
    }
    if (global)
    {	/* kludge - if NULL then do not TRdisplay into the log file
        ** should only be used for single thread tracing since TRdisplay
        ** uses static memory */
	char	    *basep;
	char	    *temp_ptr;
	msg_buffer[length]=EOS;		    /* NULL terminate the string */
	basep = msg_buffer;
	temp_ptr = msg_buffer;
	/* unfortunately TRdisplay does not handle line feeds very well
        ** it should handle them in the device dependent way in the internal 
	** "write_line" routine in
        ** TR.C but instead it does funny things with it in do_format */
	for(;*temp_ptr; temp_ptr++)
	{
	    if (*temp_ptr == '\n')
	    {	/* line feed found so print out string with line feed */
		*temp_ptr = 0;
		TRdisplay("%s\n", basep);
		basep = temp_ptr+1;
	    }
	}
	if (basep != temp_ptr)
	    TRdisplay("%s",basep);	/* print remainder with no line feed */
    }
}

/*{
** Name: opt_newline	- place newline in trace output
**
** Description:
**      Place a new line in the trace output
**
** Inputs:
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
**      21-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_newline(
	OPS_STATE	*global)
{   
    char		buffer[10];   /* temp buffer to contain newline*/

    (VOID) STprintf(&buffer[0], "\n");
    opt_scc((PTR)global, (i4)STlength(&buffer[0]), &buffer[0]);
}

/*{
** Name: opt_map	- print a bit map
**
** Description:
**      This routine will print a bitmap.
**      Print the contents of bit map "bitmapp" with "bitmapsize" bits with 
**      character string "name" as a header.
**
** Inputs:
**      name                            name of bitmap
**      bitmapp                         ptr to bitmap
**      bitmapsize                      number of bits in map
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
**      21-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_map(
	char               *name,
	PTR                bitmapp,
	i4                bitmapsize)
{
    i4		bitno;
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"%s: ", name);

    if (!bitmapp)
    {
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" NULL PTR ");
	return;
    }
    for (bitno = -1; 
	(bitno = BTnext((i4)bitno, (char *)bitmapp, (i4)bitmapsize)) >= 0;)
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"%d ", bitno);
}

/*{
** Name: opt_pmap	- print a map
**
** Description:
**      This routine will print a bytemap.
**      Print the contents of bit map "mapp" with "mapsize" bits with 
**      character string "name" as a header.
**
** Inputs:
**      name                            name of map
**      mapp                            ptr to map
**      mapsize                         number of bytess in map
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
**      21-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_pmap(
	char               *name,
	char               *mapp,
	i4                mapsize)
{
    i4		no;
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"%s: ", name);

    if (!mapp)
    {
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" NULL PTR ");
	return;
    }
    for (no = 0; no < mapsize; no++)
    {
	if (mapp[no])
	    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"%d ", no);
    }
}

/*{
** Name: opt_pvmap	- print an array of var numbers
**
** Description:
**      Print the contents of var array "mapp" with "mapsize" bits with 
**      character string "name" as a header.
**
** Inputs:
**      name                            name of map
**      mapp                            ptr to map
**      mapsize                         number of bytess in map
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
**      21-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_pvmap(
	char               *name,
	OPV_IVARS	   *mapp,
	i4                mapsize)
{
    i4		no;
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"%s: ", name);

    if (!mapp)
    {
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" NULL PTR ");
	return;
    }
    for (no = 0; no < mapsize; no++)
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"%d ", mapp[no]);
}

/*{
** Name: opt_p1map	- print an array of i1 numbers
**
** Description:
**      Print the contents of var array "mapp" with "mapsize" bytes with 
**      character string "name" as a header.
**
** Inputs:
**      name                            name of map
**      mapp                            ptr to map
**      mapsize                         number of bytess in map
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
**      13-sep-02 (inkdo01)
**	    Written to display i1 arrays (as in opn_sbstruct).

[@history_template@]...
*/
static VOID
opt_pi1map(
	char               *name,
	i1		   *mapp,
	i4                mapsize)
{
    i4		no;
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"%s: ", name);

    if (!mapp)
    {
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" NULL PTR ");
	return;
    }
    for (no = 0; no < mapsize; no++)
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"%d ", mapp[no]);
}

/*{
** Name: opt_noblanks	- remove trailing blanks
**
** Description:
**      This routine will scan from the end of a string and return the
**      number of characters without trailing blanks.
**
** Inputs:
**      strsize                         string size
**      charp                           ptr to char array
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
**      22-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
i4
opt_noblanks(
	i4                strsize,
	char               *charp)
{
    while (strsize-- && charp[strsize] == ' ');
    if (strsize < 0)
	return(1);			/* return at least one character */
    return (strsize+1);
}

/*{
** Name: opt_element	- print an element of a particular type
**
** Description:
**      Print an element of a particular type.  This routine should use
**      the ADF to convert the datatypes FIXME.
**
**      A space is preappended.
**
** Inputs:
**      valuep                          ptr to value to be printed
**                                      - does not need to be aligned
**      datatype                        ptr to datatype structure for value
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
**      21-jul-86 (seputis)
**          initial creation
**	06-apr-01 (inkdo01)
**	    Changed adc_tmlen parms from *i2 to *i4.
**	11-jun-2001 (somsa01)
**	    To save stack space, dynamically allocate space for tempbuf.
**	26-sep-01 (inkdo01)
**	    Fix above change to make tempbuf a simple "char *".
[@history_template@]...
*/
static VOID
opt_element(
	PTR                valuep,
	DB_DATA_VALUE      *datatype)
{
    i4                  default_width;
    i4                  worst_width;
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    if (datatype->db_length > DB_MAXSTRING)
    {
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"datatype too long to display\n\n");
	return;
    }
    datatype->db_data = valuep;
    {
	DB_STATUS           tmlen_status;
	tmlen_status = adc_tmlen( opscb->ops_adfcb, datatype, 
	    &default_width, &worst_width);
	if (tmlen_status != E_AD0000_OK)
	{
	    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"Error displaying value\n\n");
	    return;
	}
    }

    {
	DB_STATUS           tmcvt_status;
	i4		    outlen;
	char                *charbuf;
        DB_DATA_VALUE       dest_dt;
	char		    *tempbuf;

	tempbuf = (char *)MEreqmem(0, DB_MAXSTRING + 1, FALSE, NULL);

        datatype->db_data = valuep;
	if (worst_width > DB_MAXSTRING)
	    dest_dt.db_length = DB_MAXSTRING;
	else
	    dest_dt.db_length = worst_width;
        charbuf =
	dest_dt.db_data = (PTR)tempbuf;
	tmcvt_status
	    = adc_tmcvt( opscb->ops_adfcb, datatype, &dest_dt, &outlen); /* use the
					** terminal monitor conversion routines
                                        ** to display the results */
	if (tmcvt_status == E_AD0000_OK)
	{
	    charbuf[outlen]=0;		/* null terminate the buffer before
					** printing */
	    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"%s\n\n", charbuf);
	}
	else
	{
	    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"Error displaying value\n\n");
	}

	MEfree((PTR)tempbuf);
    }
}

/*{
** Name: opt_type	- print type info
**
** Description:
**      Print type info only
**
** Inputs:
**      datatype                        ADF datatype id
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
**      23-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_type(
	DB_DT_ID           type)
{
    ADI_DT_NAME         adi_dname;
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    if (adi_tyname(global->ops_adfcb, type, &adi_dname) != E_DB_OK)
    {
        TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"ADFERROR");
	return;
    }
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"%s", (PTR)&adi_dname);
}

/*{
** Name: opt_dt	- print datatype info
**
** Description:
**      Print datatype name and length
**
** Inputs:
**      datatype                        ptr to datatype info
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
**      23-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_dt(
	DB_DATA_VALUE      *datatype)
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" type=");
    opt_type(datatype->db_datatype);
    if (datatype->db_length)
    {
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"%d", (i4)datatype->db_length);
    }
}

/*{
** Name: opt_ffat	- find function attribute
**
** Description:
**      This routine will traverse a query tree fragment and look for a
**      var node which can provide a useful function attribute name
**      allowing the user to understand the query plan
**
** Inputs:
**      qnode                           ptr to query tree node
**
** Outputs:
**      attname                         ptr to location in which to
**                                      place attribute name
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-nov-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_ffat(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          *qnode,
	OPT_NAME	   *attname,
	bool               *found)
{
    if (qnode->pst_sym.pst_type == PST_VAR)
    {
	opt_paname(subquery, 
	    (OPZ_IATTS)qnode->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id, 
	    attname);
	*found = TRUE;
	return;
    }
    if (qnode->pst_left)
	opt_ffat(subquery, qnode->pst_left, attname, found);
    if ((!*found) && qnode->pst_right)
	opt_ffat(subquery, qnode->pst_right, attname, found);
}

/*{
** Name: opt_catname	- concat attribute name
**
** Description:
**      Routine attaches attribute name to attribute ID string 
**
** Inputs:
**      subquery                        subquery  being analyzed
**      qnode                           parse tree containing var node
**
** Outputs:
**      attrname                        ptr to string to concat name
**
**	Returns:
**	    
**	Exceptions:
**
** Side Effects:
**
** History:
**      11-aug-93 (ed)
**          initial creation, fixes stack overwrite problem
[@history_template@]...
*/
VOID
opt_catname(
	OPS_SUBQUERY	    *subquery,
	PST_QNODE	    *qnode,
	OPT_NAME	    *attrname)
{
    /* found node so find name of attribute in a var
    ** node use to define this resdom */
    OPT_NAME		    username;
    bool		    found;
    found = FALSE;
    opt_ffat(subquery, qnode, &username, &found);
    if (found)
    {
	i4		    maxlength;
	i4		    length;
	length = opt_noblanks((i4)sizeof(username), (char *)&username);
	maxlength = sizeof(OPT_NAME) -
	    STlength((char *) attrname) -1;
	if (length > maxlength)
	    length = maxlength;
	if (length > 0)
	{
	    username.buf[length] = 0; /* null 
			** terminate the username */
	    (VOID)STcat( (char *)attrname, (char *)&username);
	}
    }
}

/*{
** Name: opt_paname	- get attribute name
**
** Description:
**      Routine will return the name of the attribute 
**      For temporary relations it will use the attribute number as the
**      name but attempt to suffix the name of the attribute of the
**      base relation if possible
**
** Inputs:
**      subquery                          context within which to find name
**      attno                             joinop attribute number
**
** Outputs:
**      attname                           ptr to location to place attribute
**                                        name
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      8-aug-86 (seputis)
**          initial creation
[@history_template@]...
*/
VOID
opt_paname(
	OPS_SUBQUERY       *subquery,
	OPZ_IATTS          attno,
	OPT_NAME           *attrname)
{
    OPS_STATE           *global;    /* ptr to global state variable */

    global = subquery->ops_global;  /* ptr to global state variable */
    if (attno < 0)
    {
	STcopy( "No Attr", (PTR)attrname);
	return;
    }
    {
	/* copy the attribute name to the output buffer */
	DB_ATT_ID          dmfattr;
	DB_ATT_NAME	   *att_name;
	OPZ_ATTS           *attrp;
	OPV_GRV            *grvp;

	attrp = subquery->ops_attrs.opz_base->opz_attnums[attno];
	dmfattr.db_att_id = attrp->opz_attnm.db_att_id;
	if (dmfattr.db_att_id == 0)
	{
	    STcopy( "tid ", (PTR)attrname);
	    return;
	}
	if (dmfattr.db_att_id == OPZ_FANUM)
	{   /* attempt to add name of attribute upon which function attribute
            ** is based to the name */
	    OPZ_FATTS		*fattp;	    /* ptr to function attribute being
					    ** analyzed */
	    fattp = subquery->ops_funcs.opz_fbase->
		    opz_fatts[attrp->opz_func_att];
            /* traverse the query tree fragment representing the  function
            ** attribute and obtain the name of the first node which would
            ** help identify the function
            ** - the name will be constructed as follows
            ** - FA <unique number> <name of attribute in function>
            */
	    STcopy( "FA", (char *)attrname); /* function attribute */
            CVna( (i4)fattp->opz_fnum, (char *)&attrname->buf[2] ); /* unique 
					    ** identifier for function attribute
					    */
	    (VOID)STcat( (char *)attrname, " ");  /* place blank at end of FA id */
	    /* find the name of an attribute used within the function
	    ** attribute */
	    opt_catname(subquery, fattp->opz_function, attrname);
	    return;
	}
	grvp = subquery->ops_vars.opv_base->opv_rt[attrp->opz_varnm]->opv_grv;

	if (!grvp->opv_relation)
	{   /* special processing for temporary relation */
	    OPV_IGVARS  gvar;               /* global variable number */

	    for (gvar = 0; 
		 grvp != global->ops_rangetab.opv_base->opv_grv[gvar];
		 gvar++);
					    /* look in global range table for
                                            ** global range variable number */
	    attrname->buf[0] = 'T';	    /* global table number */
            CVla( (i4)gvar, (PTR)&attrname->buf[1]);
	    (VOID)STcat( (char *)attrname, "A");  /* place attr number at end */
	    CVla( (i4)dmfattr.db_att_id, (PTR)&attrname->buf[STlength((char *)attrname)]);
	    (VOID)STcat( (char *)attrname, " ");  /* place blank at end */
	    {
		/* look for the resdom associated with the varnode of the
                ** temporary and get the attribute name if available */
		OPS_SUBQUERY	    *tempsubquery;
		PST_QNODE	    *qnode;
		for (tempsubquery = global->ops_subquery;
		    tempsubquery && (tempsubquery->ops_gentry != gvar);
		    tempsubquery = tempsubquery->ops_next); /* find the
					    ** subquery which produced
					    ** the temporary */
		if (!tempsubquery)
		{
		    if (grvp->opv_gsubselect)
		    /* could also be a subselect subquery which has been
                    ** detached from the subquery list */
			tempsubquery = grvp->opv_gsubselect->opv_subquery;
		    else 
                 	/* could be a view */
			tempsubquery = grvp->opv_gquery;
                }
		for (qnode = tempsubquery->ops_root->pst_left;
		    qnode; qnode = qnode->pst_left)
		{   /* find resdom used to define this attribute */
		    if (qnode->pst_sym.pst_type == PST_RESDOM
			&&
			qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsno
			    == dmfattr.db_att_id
			)
		    {
			opt_catname(tempsubquery, qnode->pst_right, attrname);

			break;
		    }
		}
	    }
	    return;
	}

	STcopy(grvp->opv_relation->rdr_attr[dmfattr.db_att_id]->att_nmstr,
		(PTR)attrname);
	return;
    }
}

/*{
** Name: opt_anum	- print info about one attribute
**
** Description:
**      Dump info on one particular attribute
**
** Inputs:
**      attr                            joinop attribute number
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
**      23-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_anum(
	OPZ_IATTS          attr)
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPZ_ATTS            *attrp;     /* ptr to attribute being analyzed */
    OPT_NAME            attname;    /* name of attribute */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */

    attrp = subquery->ops_attrs.opz_base->opz_attnums[attr];
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "JOINOP ATT=%d, var=%d, dmfatt=%d, ",
	(i4)attr,
        (i4)attrp->opz_varnm,
        (i4)attrp->opz_attnm.db_att_id);
    opt_paname( subquery, attr, (OPT_NAME *)&attname);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "name=%s,", &attname );
    opt_dt(&attrp->opz_dataval);     /* print the datatype for attribute */
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 ", eqcls=%d, func=%d, ",
        (i4)attrp->opz_equcls,
        (i4)attrp->opz_func_att);
    opt_map ("outers=", (PTR)&attrp->opz_outervars, 
	(i4)32);
    opt_map ("eqcs=", (PTR)&attrp->opz_eqcmap,
	(i4)32);
    opt_newline(global);
}

/*{
** Name: opt_attnums	- print attnums array info
**
** Description:
**      Print the information about all the joinop attributes in the query.
**
** Inputs:
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
**      23-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_attnums()
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPZ_IATTS           attr;       /* current attribute being analyzed */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"JOINOP ATTRIBUTES: base=%p, number of attributes=%d\n\n",
	subquery->ops_attrs.opz_base, subquery->ops_attrs.opz_av);


    for (attr = 0; attr < subquery->ops_attrs.opz_av; attr++)
        opt_anum(attr);             /* print info about this attribute */
}

/*{
** Name: opt_ftype	- print name of OPZ_FACLASS type
**
** Description:
**      Print name of function attribute class type
**
** Inputs:
**      fclass                          function attribute class
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
**      14-aug-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_ftype(
	OPZ_FACLASS        fclass)
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    switch (fclass)
    {
    case OPZ_NULL:
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"NULL");
	break;
    case OPZ_VAR:
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"VAR");
	break;
    case OPZ_TCONV:
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"TCONV");
	break;
    case OPZ_SVAR:
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"SVAR");
	break;
    case OPZ_MVAR:
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"MVAR");
	break;
    case OPZ_QUAL:
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"QUAL");
	break;
    default:
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"UNKNOWN CLASS");
    }
}

/*{
** Name: opt_fattr	- print info on a function attribute
**
** Description:
**      Routine to print info on a function attribute
**
** Inputs:
**      fattr                           function attribute number
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
**      15-aug-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_fattr(
	OPZ_IFATTS         fattr)
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPZ_FATTS           *fattrp;    /* ptr to func attribute being analyzed */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */

    fattrp = subquery->ops_funcs.opz_fbase->opz_fatts[fattr];
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "FUNCTION ATT=%d, func=%p, attno=%d, computed=%d, ojid=%d, type=",
	(i4)fattr,
	(PTR)fattrp->opz_function,
        (i4)fattrp->opz_attno,
        (i4)fattrp->opz_computed, 
        (i4)fattrp->opz_ojid);
    opt_ftype(fattrp->opz_type);
    opt_newline(global);
    opt_map ("    eqcmap=", (PTR)&fattrp->opz_eqcm, 
	(i4)subquery->ops_eclass.ope_ev);
    opt_dt(&fattrp->opz_dataval);     /* print the datatype for attribute */
    if (fattrp->opz_type != OPZ_QUAL)
    {
	opt_newline(global);
	return;
    }
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "    left=");
    opt_ftype(fattrp->opz_left.opz_class);
    opt_map(", left map=", (PTR)&fattrp->opz_left.opz_varmap,
	(i4)subquery->ops_vars.opv_rv);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 ", right=");
    opt_ftype(fattrp->opz_right.opz_class);
    opt_map(", right map=", (PTR)&fattrp->opz_right.opz_varmap,
	(i4)subquery->ops_vars.opv_rv);
    opt_newline(global);
}

/*{
** Name: opt_fall	- print all info about function attributes
**
** Description:
**      Routine to print all the info about the function attributes table
**
** Inputs:
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
**      14-aug-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_fall()
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPZ_IFATTS          fattr;      /* current func attribute being analyzed */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"FUNCTION ATTRIBUTES: base=%p, number of attributes=%d\n\n",
	subquery->ops_funcs.opz_fbase, subquery->ops_funcs.opz_fv);

    for (fattr = 0; fattr < subquery->ops_funcs.opz_fv; fattr++)
        opt_fattr(fattr);            /* print info about this attribute */
}

/*{
** Name: opt_relstrct	- get name of storage structure
**
** Description:
**      This routines returns a readonly ptr to the name of the storage
**	storage structure
**
** Inputs:
**      storage                         dmf storage structure
**      cflag                           compressed flag
**
** Outputs:
**	Returns:
**	    PTR to readonly name of storage structure
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	18-jul-86 (seputis)
**          initial creation
[@history_line@]...
*/
static char *
opt_relstrct(
	OPO_STORAGE        storage,
	bool               cflag)
{
    char  *name;

    if (cflag)
    {
	switch (storage)
	{
	    case DB_HEAP_STORE:
		name = "cHeap";
		break;
	    case DB_ISAM_STORE:
		name = "cIsam";
		break;
	    case DB_HASH_STORE:
		name = "cHashed";
		break;
	    case DB_SORT_STORE:
		name = "cSorted";
		break;
	    case DB_BTRE_STORE:
		name = "cB-Tree";
		break;
	    case DB_RTRE_STORE:
		name = "cR-Tree";
		break;
	    case DB_TPROC_STORE:
		name = "cTproc";
		break;
	}
    }
    else
    {
	switch (storage)
	{
	    case DB_HEAP_STORE:
		name = "Heap";
		break;
	    case DB_ISAM_STORE:
		name = "Isam";
		break;
	    case DB_HASH_STORE:
		name = "Hashed";
		break;
	    case DB_SORT_STORE:
		name = "Sorted";
		break;
	    case DB_BTRE_STORE:
		name = "B-Tree";
		break;
	    case DB_RTRE_STORE:
		name = "R-Tree";
		break;
	    case DB_TPROC_STORE:
		name = "Tproc";
		break;
	}
    }
    return (name);
}
#ifdef xDEBUG

/*{
** Name: opt_storage	- print storage structure
**
** Description:
**      Print the storage structure name
**
** Inputs:
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
**      23-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_storage(
	OPO_STORAGE        storage)
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"%s", opt_relstrct(storage, FALSE));
}
#endif

/*{
** Name: opt_rop	- print operation from table
**
** Description:
**      This routine will scan a table and print the selected operation name
**	Print the mneumonic for an operator for the formatted tree printing
**	routines.
**
** Inputs:
**      operation                       operation code
**      table                           ptr to base of table of names
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
**	18-jul-86 (seputis)
**          initial creation
[@history_line@]...
*/
static VOID
opt_rop(
	PTR		    uld_control,
	i4		    operation,
	struct tab	    *table)
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    for (; table->t_opcode != operation && table->t_opcode != OPT_TEND; table++)
	;					/* search table for name */

    if (table->t_opcode != OPT_TEND)
    {
	if (uld_control)
	{
	    /* print name of operation */
	    TRformat(uld_tr_callback, (i4 *)0,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		"%s", table->t_string);
	    uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
	}
	else
	{
	    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" Operation = %s ", table->t_string);
	}
    }
    else
    {
	if (uld_control)
	{
	    /*operation not found */
	    TRformat(uld_tr_callback, (i4 *)0,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		"t=%d", operation);
	    uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
	}
	else
	{
	    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" Unknown operation =%d,", operation);
	}
    }
}

/*{
** Name: opt_interval	- print interval element for histogram
**
** Description:
**      Print an interval element for a histogram; i.e. the value which is
**      the lower boundary of the current cell being analyzed.
**
** Inputs:
**      valuep                          ptr to value to be printed
**      datatype                        ptr to datatype of histogram
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
**      21-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_interval(
	char                *valuep,
	DB_DATA_VALUE	    *datatype)
{
    if (datatype->db_datatype == DB_TXT_TYPE)
    {
	/* text data type for histograms needs to be treated specially */
	DB_DATA_VALUE	    txttype;

	STRUCT_ASSIGN_MACRO(*datatype, txttype);
	txttype.db_datatype = DB_CHR_TYPE;	/* make it look like a char */
	opt_element((PTR)valuep, &txttype);
    }
    else
	opt_element((PTR)valuep, datatype);
}

/*{
** Name: opt_pop	- print ADF operator name
**
** Description:
**      Routine to print ADF operator name
**
** Inputs:
**      adi_oid                         operator id to find name for
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
**      4-aug-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_pop(
	i4                opid)
{
    ADI_OP_NAME         name;
    DB_STATUS           status;
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    status = adi_opname( global->ops_adfcb, (ADI_OP_ID)opid, 
	(ADI_OP_NAME *)&name.adi_opname[0]);
    if (status != E_DB_OK)
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"UNKNOWN OPERATOR");
    else
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"%s", (PTR)&name.adi_opname[0]);
}

/*{
** Name: opt_sarg	- print name of sarg parameter
**
** Description:
**      Prints the name of an OPB_SARG value
**
** Inputs:
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
**      23-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_sarg(
	i4                sarg)
{
    char                *sargp;
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    switch (sarg)
    {
    case ADC_KNOMATCH:
	sargp = "NOMATCH";
	break;
    case ADC_KEXACTKEY:
	sargp = "EXACTKEY";
	break;
    case ADC_KRANGEKEY:
	sargp = "RANGEKEY";
	break;
    case ADC_KHIGHKEY:
	sargp = "HIGHKEY";
	break;
    case ADC_KLOWKEY:
	sargp = "LOWKEY";
	break;
    case ADC_KNOKEY:
	sargp = "NOTSARG";
	break;
    case ADC_KALLMATCH:
	sargp = "ALLMATCH";
	break;
    default:
	sargp = "UNDEFINED";
	break;
    }
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"%s", sargp);
}

/*{
** Name: opb_fbi	- find OPB_BFKEYINFO structure
**
** Description:
**      Routine to find OPB_BFKEYINFO ptr given the OPB_BFVALLIST ptr
**      - routine used to debugger display of OPB_BFVALLIST info
**
** Inputs:
**      bfvp                            ptr to OPB_BFVALLIST item for search
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
**      4-aug-86 (seputis)
**          initial creation
[@history_template@]...
*/
static OPB_BFKEYINFO *
opt_fbi(
	OPB_BFVALLIST      *bfvp)
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */
    {
	OPB_IBF		bf;	    /* current boolean factor being analyzed */
	/* search for the bfvp in the boolean factor structures */
        for (bf=0; bf < subquery->ops_bfs.opb_bv; bf++)
	{
	    OPB_BOOLFACT	*bfp;
	    if (bfp = subquery->ops_bfs.opb_base->opb_boolfact[bf])
	    {
		OPB_BFKEYINFO         *bfip;

		for( bfip=bfp->opb_keys; bfip ; bfip = bfip->opb_next)
		{
		    OPB_BFVALLIST	*tbfvp;
		    for( tbfvp = bfip->opb_keylist; tbfvp; tbfvp = tbfvp->opb_next)
			if (tbfvp == bfvp)
			    return(bfip);
		}
	    }
	}
    }
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"missing OPB_BFKEYINFO\n\n");
    return (NULL);
}

/*{
** Name: opt_bv	- print boolean factor value element
**
** Description:
**      Routine to print a boolean factor value (and histogram value)
**
** Inputs:
**      bfvp                            ptr to OPB_BFVALLIST 
**      bfip				ptr to OPB_BFKEYINFO
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
**      23-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_bv(
	OPB_BFVALLIST      *bfvp,
	OPB_BFKEYINFO      *bfip)
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    if (!bfip)
	bfip = opt_fbi(bfvp);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"OPB_BFVALLIST addr=%p, next=%p, operator=", 
        (PTR)bfvp,
	(PTR)bfvp->opb_next);
    opt_sarg( (i4)(bfvp->opb_operator));
    opt_newline(global);
    if (!bfip)
	return;
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    key=");
    if (bfvp->opb_keyvalue)
    	opt_element(bfvp->opb_keyvalue, bfip->opb_bfdt);
    else
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" NULL key value ");
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	", hist=");
    if (bfvp->opb_hvalue)
    	opt_interval(bfvp->opb_hvalue, bfip->opb_bfhistdt);
    else
        TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" NULL hist ptr ");
    opt_newline(global);
}

/*{
** Name: opt_bvlist	- print value list of a keyinfo ptr
**
** Description:
**      Routine to print the value list of a key info pointer
**
** Inputs:
**      bfvp                            ptr to beginning of list
**      bfip                            ptr to OPB_BFKEYINFO head of list
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
**      23-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_bvlist(
	OPB_BFVALLIST      *bfvp,
	OPB_BFKEYINFO      *bfip)
{
    if (!bfip)
	bfip = opt_fbi(bfvp);
    for(; bfvp; bfvp = bfvp->opb_next)
	opt_bv(bfvp, bfip);
}

/*{
** Name: opt_bi	- print info on an OPB_BFKEYINFO structure
**
** Description:
**      Print information on a boolean factor key info structure
**
** Inputs:
**      bfinfop                         ptr to structure to dump
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
**      23-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_bi(
	OPB_BFKEYINFO      *bfinfop)
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"OPB_BFKEYINFO: next=%p, eqc=%d, used=%d, sarg=",
	bfinfop->opb_next,
        bfinfop->opb_eqc,
        bfinfop->opb_used);
    opt_sarg((i4)bfinfop->opb_sargtype);
    opt_newline(global);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "    list=%p , data", bfinfop->opb_keylist);
    opt_dt(bfinfop->opb_bfdt);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 ", hist");
    opt_dt(bfinfop->opb_bfhistdt);
    opt_newline(global);
    opt_big(bfinfop);
    opt_bvlist(bfinfop->opb_keylist, bfinfop);
}

/*{
** Name: opt_big	- print global portion of bool info structure
**
** Description:
**      Print global portion of OPB_BFKEYINFO structure
**
** Inputs:
**      bfinfop                         ptr to boolean factor info ptr
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
**      23-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_big(
	OPB_BFKEYINFO      *bfinfop)
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPB_BFGLOBAL	*bfglobalp; /* ptr to what we're printing */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    bfglobalp = bfinfop->opb_global;
    if (bfglobalp == NULL)
    {
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "OPB_BFGLOBAL:  NULL");
	return;
    }
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"OPB_BFGLOBAL: addr=%p, single-bfi=%d, mult-bfi=%d, range-bfi=%d\n\n" ,
	bfglobalp,
	bfglobalp->opb_single,
	bfglobalp->opb_multiple,
	bfglobalp->opb_range);

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    bfmask=%x\n\n" ,
	bfglobalp->opb_bfmask);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    pmin=%p, pmax=%p\n xpmin=%p, xpmax=%p\n\n",
	bfglobalp->opb_pmin, bfglobalp->opb_pmax,
	bfglobalp->opb_xpmin, bfglobalp->opb_xpmax);
    opt_newline(global);
}

/*{
** Name: opt_bf	- print boolean factor info
**
** Description:
**      Print the boolean factor info
**
** Inputs:
**      bfi                             boolean factor number
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
**      22-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_bf(
	OPB_IBF            bfi)
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPB_BOOLFACT        *bfp;	    /* ptr to boolean factor */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */
    bfp = subquery->ops_bfs.opb_base->opb_boolfact[bfi]; /* ptr to boolean
				    ** factor element */
    
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "OPB_BOOLFACT: bfno=%d, addr=%p, histp=%p, or=%d, eqcls=%d, parms=%d\n\n",
	bfi,
	bfp,
	bfp->opb_histogram,
	bfp->opb_orcount,
	bfp->opb_eqcls,
	bfp->opb_parms);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "    qual=%d, bfkeyinfo=%p, querytree=%p, ojid=%d, mask=%x\n\n",
	bfp->opb_truequal,
	bfp->opb_keys,
	bfp->opb_qnode, bfp->opb_ojid, bfp->opb_mask);
    opt_map("   ojmap=", (PTR)&bfp->opb_ojmap, (i4)BITS_IN(bfp->opb_ojmap));
    opt_newline(global);
    opt_map (" eqcmap", (PTR)&bfp->opb_eqcmap,          
             	(i4)subquery->ops_eclass.ope_ev);       
    opt_newline(global);
    opt_map (" ojattr", (PTR)bfp->opb_ojattr,          
             	(i4)BITS_IN(*bfp->opb_ojattr));         
    opt_newline(global);
    /* ******** optional:  display the boolfact's qtree fragment.
    ** While this cross-call is grossly illegal, I found it very
    ** useful in understanding the boolfact stuff.  It's a bit
    ** wordy for everyday use so I'm commenting it out.  Perhaps
    ** its inclusion should be another trace point. [kschendel]
    ** pst_1prmdump(bfp->opb_qnode, NULL, NULL);
    *********** */
    {
	OPB_BFKEYINFO	    *bfkp;
	for (bfkp = bfp->opb_keys; bfkp; bfkp = bfkp->opb_next)
	{
	    opt_bi(bfkp);
	}
    }
}

/*{
** Name: opt_ball	- print all info about boolean factors
**
** Description:
**      The routine will dump all info about boolean factors
**
** Inputs:
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
**      23-jul-86 (seputis)
**          initial creation
**	9-sep-93 (swm)
**	    Bug #56449
**	    Changed TRformat calls to use %p for pointer values to
**	    avoid integer/pointer truncation errors.
[@history_template@]...
*/
static VOID
opt_ball()
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPB_IBF             bfi;	    /* ptr to boolean factor */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"OPB_BFSTATE: num bfs=%d, base=%p, constants=%p, qfalse=%d",
	(i4)subquery->ops_bfs.opb_bv,
	(PTR)subquery->ops_bfs.opb_base,
	(PTR)subquery->ops_bfs.opb_bfconstants,
	(i4)subquery->ops_bfs.opb_qfalse);
    if (subquery->ops_bfs.opb_bfeqc)
    {
	opt_map("    constant map=",
	    (PTR)subquery->ops_bfs.opb_bfeqc, 
	    (i4)subquery->ops_bfs.opb_bv);
    }
    opt_newline(global);
    for (bfi = 0; bfi < subquery->ops_bfs.opb_bv; bfi++)
	opt_bf(bfi);
}

/*{
** Name: OPT_MAPIT	- find map of joinop variables in the subquery
**
** Description:
**      Traverse the co tree and find all range variables referenced.
**
** Inputs:
**      cop                             ptr to root of CO tree to map
**
** Outputs:
**      map                             ptr to variable map to update
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	18-jul-86 (seputis)
**          initial creation
[@history_line@]...
*/
static VOID
opt_mapit(
	OPO_CO             *cop,
	OPV_BMVARS         *map)
{
    if (!cop)
	    return;
    if (cop->opo_sjpr == DB_ORIG)
	    BTset((i4)cop->opo_union.opo_orig, (char *)map);
    else
    {
	if (cop->opo_outer)
	    opt_mapit(cop->opo_outer, map);
	if (cop->opo_inner)
	    opt_mapit(cop->opo_inner, map);
    }
}

/*{
** Name: opt_attrnm	- print info on equivalence class
**
** Description:
**      This routine is used to print info on an equivalence class.  Used by the
**      node printing routines.
**
** Inputs:
**      subquery                        ptr to subquery being traced
**      cop                             ptr to co node to being printed
**      eqcls                           equivalence class referenced in node
**
** Outputs:
**      attrname                        name of attribute returned to caller
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	18-jul-86 (seputis)
**          initial creation
**	9-feb-03 (inkdo01)
**	    Catch bad eqclass passed to this func.
[@history_line@]...
*/
static 
opt_attrnm(
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop,
	OPE_IEQCLS         eqcls,
	OPT_NAME           *attrname)
{
    OPV_BMVARS          varmap;         /* bit map of range vars used in this
                                        ** subtree */
    OPZ_IATTS           atno;           /* attribute number associated with 
                                        ** eqcls of some relation of
                                        ** cop tree */
    OPZ_IATTS           saveatno;	/* attribute number associated with 
                                        ** eqcls of some temporary relation of
                                        ** cop tree */
    OPZ_BMATTS          (*attrmap);     /* bitmap of attributes in eqcls */
    OPZ_AT              *abase;		/* ptr to base of array of ptrs to
                                        ** joinop attributes */
    OPV_RT              *vbase;         /* ptr to base of array of ptrs to
                                        ** joinop range vars */

    if (eqcls < 0)
    {
	STcopy( "No Attr", (PTR)attrname);
	return;
    }
    else if (eqcls > subquery->ops_eclass.ope_ev)
    {
	STcopy( "Bad EQC", (PTR)attrname);
	return;
    }
    /* -1 for eqcls may come from the top sort node.  We can't in general
    ** find this attribute name cause if this is a retrieve to a terminal,
    ** then the target list name may not be around. 
    */
    MEfill(sizeof(varmap), (u_char)0, (PTR)&varmap);
    opt_mapit(cop, &varmap);		/*map of range variables in CO subtree*/

    /* search eqclass for an att that belongs to a relation in this
    ** subtree */
    attrmap = &subquery->ops_eclass.ope_base->ope_eqclist[eqcls]->ope_attrmap;
    abase = subquery->ops_attrs.opz_base;
    vbase = subquery->ops_vars.opv_base;
    saveatno = -1;			/* just in case */
    for (atno = -1;
	 (atno = BTnext((i4)atno, (char *)attrmap, (i4)BITS_IN(*attrmap))) >= 0;)
    {
	/* try to find the attribute name in a base relation rather than a
        ** aggregate temporary relation if possible since name is easier to
        ** relate to
        */
	OPV_IVARS	vno;
	if (BTtest((i4)(vno=abase->opz_attnums[atno]->opz_varnm), (char *)&varmap))
	{
	    saveatno = atno;
	    if ((abase->opz_attnums[atno]->opz_attnm.db_att_id > 0) /* avoid
					    ** function attributes if possible*/
		&&
		(vbase->opv_rt[vno]->opv_grv->opv_relation) /* avoid temps */
	       )
	    {
		opt_paname( subquery, atno, (OPT_NAME *)attrname);
		return;			    /* non temporary is found so
                                            ** exit with this name */
	    }
	}
    }
    opt_paname( subquery, saveatno, (OPT_NAME *)attrname);/* only temporary relations are
                                            ** available so print it */
}

/*{
** Name: opt_ptname	- print table name
**
** Description:
**      Routine to print the table name to a formatted tree
**
** Inputs:
**	subquery			ptr to subquery being analyzed
**      varno                           varno of table name to print
**
** Outputs:
**	namep				ptr to name of table
**	Returns:
**	    TRUE if corelation name is found and is different from base
**	    table name
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      3-aug-86 (seputis)
**          initial creation
**      26-feb-91 (seputis)
**          added improved diagnostics for b35862
**      9-jul-91 (seputis)
**          added parameter to print corelation name
[@history_template@]...
*/
bool
opt_ptname(
	OPS_SUBQUERY	    *subquery,
	OPV_IVARS	    varno,
	OPT_NAME	    *namep,
	bool		    corelation)
{
    OPS_STATE	    *global;
    bool	    retval;

    global = subquery->ops_global; /* ptr to global state variable */
    retval = FALSE;
    {
	OPV_GRV		*grv_ptr;   /* ptr to global range variable */
	grv_ptr = subquery->ops_vars.opv_base->opv_rt[varno]->opv_grv; /*
                                    ** get ptr to global range table element
                                    */
        if (grv_ptr->opv_relation)
	{
            /* there is a name if RDF information has been found - otherwise
            ** it is a temporary relation produced by aggregates */
            DB_TAB_NAME		*tabname;
            i4                  tabnamesize;
	    
	    if (corelation && (grv_ptr->opv_qrt >= 0))
	    {
		tabname = &global->ops_qheader->pst_rangetab[grv_ptr->opv_qrt]
		    ->pst_corr_name;
		retval = MEcmp((PTR)tabname, (PTR)&grv_ptr->opv_relation->rdr_rel->tbl_name,
			sizeof(*tabname)) != 0;
	    }
	    else
		tabname = &grv_ptr->opv_relation->rdr_rel->tbl_name;
            tabnamesize = opt_noblanks( sizeof(tabname->db_tab_name), 
		(char *)tabname->db_tab_name );		/* number of significant chars*/

	    if (tabnamesize >= (i4)sizeof(OPT_NAME))
		tabnamesize = sizeof(OPT_NAME) - 1;
	    MEcopy( (PTR)tabname, tabnamesize, (PTR)namep);
	    namep->buf[tabnamesize] = 0;
	}
	else if (grv_ptr->opv_gmask & OPV_TPROC)
	{
            DB_TAB_NAME		*tabname;
            i4                  tabnamesize;
	    
	    tabname = &global->ops_qheader->pst_rangetab[grv_ptr->opv_qrt]
		    ->pst_corr_name;
            tabnamesize = opt_noblanks( sizeof(tabname->db_tab_name), 
		(char *)tabname->db_tab_name );		/* number of significant chars*/

	    if (tabnamesize >= (i4)sizeof(OPT_NAME))
		tabnamesize = sizeof(OPT_NAME) - 1;
	    MEcopy( (PTR)tabname, tabnamesize, (PTR)namep);
	    namep->buf[tabnamesize] = 0;
	}
        else
        {
            OPV_IGVARS		gno;/* global range variable number */	    
	    /* for an aggregate temporary use the range table number */
            for (gno = 0; gno < OPV_MAXVAR; gno++)
	    {
		if (grv_ptr == global->ops_rangetab.opv_base->opv_grv[gno])
                    break;
	    }
	    (VOID) STprintf((char *)namep, "T%d", (i4)gno);
        }
    }
    return(retval);
}

/*{
** Name: opt_codump	- print a co node
**
** Description:
**      Dump the info in this cost ordering node
**
** Inputs:
**      cop                           ptr to CO node to be formatted
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
**	18-jul-86 (seputis)
**          initial creation
**	06-mar-96 (nanpr01)
**          Added pagesize parameter for printing in QP. 
[@history_line@]...
*/
static VOID
opt_codump(
	OPO_CO              *cop)
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"OPO_CO: ");
    if (!cop)
    {
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"NULL PTR \n\n");
	return;
    }
    if (cop->opo_sjpr == DB_ORIG)	    /* if this is the original relation */
    {
	OPT_NAME    tablename;
        (VOID)opt_ptname( subquery, cop->opo_union.opo_orig, &tablename, TRUE); 
				    /* print table correlation name */
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "%s ", (PTR)&tablename);
        (VOID)opt_ptname( subquery, cop->opo_union.opo_orig, &tablename, FALSE); 
				    /* print table name */
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "(%s), ", (PTR)&tablename);
    }
    else if (cop->opo_sjpr == DB_SJ)
    {
	if (cop->opo_sjeqc == OPE_NOEQCLS)
	    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"Cart-Prod, ");
	else
	{
	    OPT_NAME        attrname;	/* name of attribute associated with
                                        ** this equivalence class */
	    opt_attrnm(subquery, cop, (i4) cop->opo_sjeqc, &attrname);
	    if (cop->opo_existence)
		TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "Join(existence only), Ordered on = %s, ", &attrname);
	    else
		TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "Join, Ordered on = %s, ", &attrname);
	}
    }
    else if (cop->opo_sjpr != DB_RSORT)
	opt_rop ((PTR)NULL , (i4)cop->opo_sjpr, opf_sjtyps_tab); /* print type of 
					** operation occuring (project-restrict,
					** joining, reformating) */
    else
    {
	OPT_NAME        ordname;	/* name of attribute associated with
                                        ** this equivalence class */
	opt_attrnm(subquery, cop, (i4) cop->opo_ordeqc, &ordname );
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"Sort node ordered on = %s ", &ordname);
    }

    /* print equivalence class that this rel is ordered on, it's
    ** structure and the equivalence class the join is on */
    if (cop->opo_sjpr != DB_RSORT)
    {
	bool	    cflag;		/* compressed flag */

        cflag = FALSE;
        if (cop->opo_sjpr == DB_ORIG)
	{   /* set compressed flag */
	    OPV_GRV		*grv_ptr;   /* ptr to global range variable */

	    grv_ptr 
		= subquery->ops_vars.opv_base->opv_rt[cop->opo_union.opo_orig]->opv_grv;
					/* get ptr to global range table element
					*/
	    if (grv_ptr->opv_relation)
	    {
		cflag = (   grv_ptr->opv_relation->rdr_rel->tbl_status_mask
			    &
			    DMT_COMPRESSED
			) 
			!= 0;
	    }
	}
	if (cop->opo_storage == DB_HEAP_STORE)
	    TRformat(opt_scc, (i4 *)global,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		" Ordering = %s, ", opt_relstrct(cop->opo_storage,cflag));
	else if (cop->opo_ordeqc >= 0)
	{
	    OPT_NAME        stoname;	/* name of attribute associated with
                                        ** this equivalence class */
	    opt_attrnm(subquery, cop, (i4) cop->opo_ordeqc, &stoname);
	    TRformat(opt_scc, (i4 *)global,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		" Ordering = %s on Attribute =%s, ",
		opt_relstrct(cop->opo_storage, cflag), &stoname);
            if (cop->opo_sjpr == DB_ORIG)
	    {	/* check for multi-attribute keying and print all attr names
                ** involved in the key for ORIG node */
		OPV_VARS	*var_ptr;   /* ptr to global range variable */
		var_ptr = subquery->ops_vars.opv_base->opv_rt[cop->opo_union.opo_orig]; /*
					    ** get ptr to range table element */
                if (var_ptr->opv_mbf.opb_count > 1)
		{   /* more than one attribute used for key */
		    OPZ_IATTS		attno;
		    TRformat(opt_scc, (i4 *)global,
			(char *)&global->ops_trace.opt_trformat[0],
			(i4)sizeof(global->ops_trace.opt_trformat),
			" Secondary Attributes = ");
		    for (attno=1; attno < var_ptr->opv_mbf.opb_count; attno++)
		    {
			OPT_NAME	    attrname;
			opt_paname(subquery, 
			    var_ptr->opv_mbf.opb_kbase->
				opb_keyorder[attno].opb_attno, 
			    (OPT_NAME *)&attrname);
			TRformat(opt_scc, (i4 *)global,
			    (char *)&global->ops_trace.opt_trformat[0],
			    (i4)sizeof(global->ops_trace.opt_trformat),
			    "+ %s ", &attrname);
		    }
		}
	    }
	}
	else
	{
	    TRformat(opt_scc, (i4 *)global,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		 " No Ordering attr, Storage = %s, ",
		    opt_relstrct(cop->opo_storage, cflag));
	}
    }

    /* print the size, in blocks, of relation */
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" \n\n    Disk pages =%d, Total Tups = %d, Page Size = %d  ",
	    (i4)cop->opo_cost.opo_reltotb, 
	    (i4)cop->opo_cost.opo_tups,
	    (i4)cop->opo_cost.opo_pagesize);

    /* if an interior node, print cumulative cost to get to this node */
    if (cop->opo_sjpr != DB_ORIG)
    {
	i4    totalcpu;

	totalcpu = (i4)(cop->opo_cost.opo_cpu / 
	    subquery->ops_global->ops_cb->ops_alter.ops_cpufactor);
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "Diskio =%d, Cost=%d ", (i4)cop->opo_cost.opo_dio, 
	    totalcpu);
    }
    /* print out more info on CO node */
    opt_map ("eqclass returned=", (PTR)&cop->opo_maps->opo_eqcmap, 
	(i4)subquery->ops_eclass.ope_ev);
    opt_newline(global);
}

/*{
** Name: opo_fparent	- find the parent node of a CO (cost ordering node)
**
** Description:
**      This routine will traverse the subtree and find the parent node 
**      of the given CO node
**
** Inputs:
**      root                            root CO node
**      target                          child CO node to find parent of
**
** Outputs:
**      parent                          return pointer to parent here
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-oct-87 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opo_fparent(
	OPO_CO             *root,
	OPO_CO             *target,
	OPO_CO             **parent)
{
    if (root->opo_outer)
    {
	if (root->opo_outer == target)
	{
	    *parent = root;
	    return;
	}
	opo_fparent(root->opo_outer, target, parent);
	if (*parent)
	    return;
    }
    if (root->opo_inner)
    {
	if (root->opo_inner == target)
	{
	    *parent = root;
	    return;
	}
	opo_fparent(root->opo_inner, target, parent);
	if (*parent)
	    return;
    }
}

/*{
** Name: opt_onext	- get next ordering eqc
**
** Description:
**      This routine returns the next ordering equivalence class name 
**      of a multi-attribute ordering
**  
** Inputs:
**      subquery                        ptr to subquery to be analyzed
**      ordering                        ordering being decomposed
**      eqclsp                          ptr to last equivalence class
**                                      returned, use OPE_NOEQCLS for
**                                      first time thru
**      levelp                          ptr to number of last group 
**                                      of equivalence classes
**
** Outputs:
**      eqclsp                          ptr to next equivalence class
**      levelp                          ptr to group number in which
**                                      *eqcslp was found, all elements
**                                      in same group can be ordered
**                                      as needed by the parent
**                                      
**	Returns:
**	    TRUE - if no more equivalence classes exist
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-feb-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
static bool
opt_onext(
	OPS_SUBQUERY       *subquery,
	OPO_ISORT          ordering,
	OPE_IEQCLS         *eqclsp,
	i4                *levelp)
{
    OPE_IEQCLS	    maxeqcls;
    bool	    ret_value;

    maxeqcls = subquery->ops_eclass.ope_ev;
    ret_value = TRUE;
    if (ordering < maxeqcls)
    {	/* ordinary eqcls */
	ret_value = (*eqclsp == ordering);
	*eqclsp = ordering;
	return(ret_value);
    }
    else if(*eqclsp == OPE_NOEQCLS)
    {	/* multi-attribute ordering needs to be initialized */
	ret_value = FALSE;
        *levelp = 0;
    }
    {
	OPO_SORT	*orderp;
	orderp = subquery->ops_msort.opo_base->opo_stable[ordering-maxeqcls];
	if (orderp->opo_stype == OPO_SINEXACT)
	{
	    *eqclsp = BTnext((i4)*eqclsp, (char *)orderp->opo_bmeqcls, 
		(i4)maxeqcls);
	    ret_value = (*eqclsp == OPE_NOEQCLS);
	}
	else if (orderp->opo_stype == OPO_SEXACT)
	{
	    OPE_IEQCLS	    eqcls;
	    if (*eqclsp == OPE_NOEQCLS)
	    {	/* first time thru */
		*levelp = 0;
		*eqclsp = orderp->opo_eqlist->opo_eqorder[0];
		if (*eqclsp < maxeqcls)
		    return(FALSE);	/* return if a single eqcls found */
	    }
	    eqcls = orderp->opo_eqlist->opo_eqorder[*levelp];
	    if (eqcls >= maxeqcls)
	    {
		*eqclsp = BTnext((i4)*eqclsp, (char *)subquery->ops_msort.
		    opo_base->opo_stable[eqcls-maxeqcls]->opo_bmeqcls, 
		    (i4)maxeqcls);	/* this must be an inexact ordering */
		if (*eqclsp != OPE_NOEQCLS)
		    return(FALSE);	/* if bits are still being found then
					** return to caller */
	    }
	    (*levelp)++;		/* go to next level */
	    *eqclsp = orderp->opo_eqlist->opo_eqorder[*levelp];
	    if (*eqclsp >= maxeqcls)
	    {
		*eqclsp = BTnext((i4)OPE_NOEQCLS, (char *)subquery->ops_msort.
		    opo_base->opo_stable[*eqclsp-maxeqcls]->opo_bmeqcls, 
		    (i4)maxeqcls);
		return(FALSE);
	    }
	    ret_value = *eqclsp == OPE_NOEQCLS;
	}
    }
    return(ret_value);
}

/*{
** Name: opt_alist	- print attribute list
**
** Description:
**      This routine will dump an attribute list for a node 
**      in the tree printing routines
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
[@history_template@]...
*/
static VOID
opt_alist(
	OPS_SUBQUERY       *subquery,
	PTR                 uld_control,
	OPO_CO             *cop,
	OPO_ISORT	   ordering,
	char               *header,
	char		   *prefix)
{
    OPE_IEQCLS      eqcls;	    /* next attribute of a multi-attribute
				    ** ordering */
    bool	    eof;	    /* TRUE if the last attribute is
				    ** processed */
    i4		    level;	    /* used to process multi-attribute
				    ** ordering */
    char	    *delimiter;	    /* used to terminate or separate
				    ** an attribute list */
    OPT_NAME        stoname;	    /* name of attribute associated with
                                    ** this equivalence class */
    OPS_CB	    *opscb;	    /* ptr to session control block */
    OPS_STATE       *global;	    /* ptr to global state variable */

    opscb = ops_getcb();        /* get the session control block
                                ** from the SCF */
    global = opscb->ops_state;	    /* ptr to global state variable */

    eqcls = OPE_NOEQCLS;
    eof = opt_onext(subquery, ordering, &eqcls, &level); /* get the next
				    ** attribute of a multi-attribute ordering */
    do
    {
	opt_attrnm(subquery, cop, (i4) eqcls, &stoname); /* get the name of
				    ** the attribute associated with this
				    ** subtree cop */
	if(eof = opt_onext(subquery, ordering, &eqcls, &level))
	    delimiter = ")";	    /* use closing bracket for the last
				    ** attribute */
	else
	    delimiter = ",";	    /* use comma if this is not the last
				    ** attribute */
	TRformat(uld_tr_callback, (i4 *)0,
	    (char *)&global->ops_trace.opt_trformat[0],
	    (i4)sizeof(global->ops_trace.opt_trformat),
	    "%s%s%s%s", header, prefix,
	    &stoname, delimiter);
	uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
	prefix = " ";		    /* indent with blank for second and subsequent
				    ** attributes being listed */
	header = "";                /* storage structure is only printed on the first
				    ** pass */
    }
    while (!eof);
}

/*{
** Name: opt_cdump	- dump info on a CO node
**
** Description:
**      This routine will dump auxiliary information about the CO node 
[@comment_line@]...
**
** Inputs:
**      cop                             ptr to CO node to dump
**      uld_control                     query tree printing tree routine
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      1-mar-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
static VOID
opt_cdump(
	OPO_CO              *cop)
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    if (cop == NULL)
    {
	/* FIXME: print out a message to say that there isn't
	** any co tree.
	*/
	return;
    }
    

    if (cop->opo_outer)
	opt_cdump(cop->opo_outer);
    if (cop->opo_inner)
	opt_cdump(cop->opo_inner);

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */

    TRformat(opt_scc, (i4 *)NULL,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "\nNODE ID %d summary", cop->opo_variant.opo_local->opo_id); /* print
				    ** title of node to summary */
   
    opt_treetext(subquery, cop);    /* print information on target list and the
                                    ** joins and qualification */

    TRformat(opt_scc, (i4 *)NULL,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "\n    pages = %f, tuples = %f, byte count = %f, pagesize = %d", 
	cop->opo_cost.opo_reltotb, 
	cop->opo_cost.opo_tups,
	cop->opo_cost.opo_tups * ope_twidth(subquery, &cop->opo_maps->opo_eqcmap),
	cop->opo_cost.opo_pagesize);

    if (cop->opo_sjpr != DB_ORIG)
    {
	TRformat(opt_scc, (i4 *)NULL,
	    (char *)&global->ops_trace.opt_trformat[0],
	    (i4)sizeof(global->ops_trace.opt_trformat),
	     "\n    Disk I/O = %f, Tuple compares/moves = %f", 
	    cop->opo_cost.opo_dio,
	    cop->opo_cost.opo_cpu);
	if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
	{
	    TRformat(opt_scc, (i4 *)NULL,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		 ", Network costs = %f", 
		cop->opo_cost.opo_cvar.opo_network);
	}
    }

    if ((cop->opo_storage == DB_ISAM_STORE)
	||
	(cop->opo_storage == DB_HASH_STORE))
	TRformat(opt_scc, (i4 *)NULL,
	    (char *)&global->ops_trace.opt_trformat[0],
	    (i4)sizeof(global->ops_trace.opt_trformat),
	     "\n    Estimated primary blocks in relation = %f", 
	    cop->opo_cost.opo_cvar.opo_relprim);
    if ((cop->opo_sjpr == DB_PR)
	||
	(   (cop->opo_sjpr == DB_SJ)
	    &&
            (cop->opo_inner->opo_sjpr == DB_ORIG)
	))
    {
	char	*touch_type;
	if (cop->opo_sjpr == DB_PR)
	    touch_type = "project-restrict";
	else
	    touch_type = "inner relation";
	if ((subquery->ops_bestco == cop )
	    && 
	    (cop->opo_cost.opo_pagestouched==0.0))
	{   /* check for degenerate case of pages touched */
	    TRformat(opt_scc, (i4 *)NULL,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		 "\n    Page level locking preferred");
	}
	else
	    TRformat(opt_scc, (i4 *)NULL,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		 "\n    Estimated blocks touched for locking on %s = %f", 
		touch_type,
		cop->opo_cost.opo_pagestouched);
    }
    TRformat(opt_scc, (i4 *)NULL,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "\n    Average tuples in a run of; adjacent values= %f; sorted values= %f",
	cop->opo_cost.opo_adfactor,
	cop->opo_cost.opo_sortfact);
    TRformat(opt_scc, (i4 *)NULL,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"\n    Tuples per block = %f", 
	cop->opo_cost.opo_tpb);
}

/*{
** Name: opt_coprintGuts - print a co node with or without stats
**
** Description:
**      This routine will print a single CO node. with or without statistics.
**
** Inputs:
**      coptr                           ptr to CO node to be formatted
**      uld_control                     - used for format control if tree
**                                      is being printed,
**                                      - NULL - indicates that a trace
**                                      format is requested
**	withStats			TRUE or FALSE:  whether or not to
**					print statistics
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
**	15-may-92 (rickh)
**	    Created.
**      5-jan-93 (ed)
**          - part of fix for bug 48049, print "partial" if ISAM ordering is
**          being used for a PSM strategy
**	27-nov-95 (stephenb)
**	    check that opo_variant has been set before switching on it (B72480)
**	06-mar-96 (nanpr01)
**	    Added pagesize for printing in QP. However this was ifdefed
**	    to minimize the run_all diferences.
**	16-mar-04 (inkdo01)
**	    Added number of partitions to display of partitioned table ORIG.
**	01-mar-10 (smeke01) b123333
**	    cop->opo_variant.opo_local is NULL when the join has not yet been
**	    determined. This is a different case to when opo_local is not NULL
**	    and opo_local->opo_jtype is not one of the recognised values. The
**	    latter is correctly displayed as 'UNKNOWN'; the former should be
**	    displayed as not yet determined or 'UNDETERMINED'. Also added in
**	    missing opo_jtype OPO_SJCARTPROD.
[@history_line@]...
*/
static VOID
opt_coprintGuts(
	OPO_CO          *cop,
	PTR             uld_control,
	bool		withStats)
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    i4			nparts = -1;

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */

    if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
    {
	/* distributed optimization requires a site for each node */
#if 0
	TRformat(uld_tr_callback, (i4 *)0,
	    (char *)&global->ops_trace.opt_trformat[0],
	    (i4)sizeof(global->ops_trace.opt_trformat), "Node ID %d",
	    cop->opo_variant.opo_local->opo_id);
	uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
	/* FIXME - place this node ID in local also once printing queries is debugged */
#endif
	if (cop->opo_variant.opo_local &&
	    cop->opo_variant.opo_local->opo_operation != 
	    cop->opo_variant.opo_local->opo_target)
	{
	    TRformat(uld_tr_callback, (i4 *)0,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		"Site ID %d->%d",
		cop->opo_variant.opo_local->opo_operation,
		cop->opo_variant.opo_local->opo_target);
	    uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
	}
						/* operation on different location from eventual
						** target */
	else
	{
	    /* operation on same location as target */
	    TRformat(uld_tr_callback, (i4 *)0,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		"Site ID %d",
		cop->opo_variant.opo_local->opo_target);   
	    uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
	}
    }
    if (!cop)
    {
	TRformat(opt_scc, (i4 *)global,
	    (char *)&global->ops_trace.opt_trformat[0],
	    (i4)sizeof(global->ops_trace.opt_trformat),
	    " NULL PTR \n\n");
	return;
    }
    switch (cop->opo_sjpr)
    {
    case DB_ORIG:
    {	/* if this is the original relation */
	OPT_NAME	tablename;
	RDR_INFO	*rdf_infop;
	OPV_VARS	*varp;
        (VOID)opt_ptname( subquery, cop->opo_union.opo_orig, &tablename, FALSE); 
				/* print table name */
	uld_prnode(uld_control, (char *)&tablename);

	/* print out alias name or base table name if it is different */
	varp = subquery->ops_vars.opv_base->opv_rt[cop->opo_union.opo_orig];
	rdf_infop = varp->opv_grv->opv_relation;
	if (rdf_infop)
	{
	    i4		tabnamesize;
	    DD_TAB_NAME		*tabname;
            if (rdf_infop->rdr_obj_desc)
	    {
		tabname = (DD_TAB_NAME *)rdf_infop->rdr_obj_desc->dd_o9_tab_info.dd_t1_tab_name;
		tabnamesize = opt_noblanks( sizeof(*tabname), (char *)tabname ); /* number of significant chars*/
		if ((tabnamesize > 0)
		    &&
		    MEcmp( (PTR)tabname, (PTR)&tablename, tabnamesize ))
		{   /* if the names are not equal then print the underlying
		    ** LDB or base table name */
		    if (tabnamesize >= (i4)sizeof(tablename))
			tabnamesize = sizeof(tablename)-1;
		    MEcopy((PTR)tabname, tabnamesize, (PTR)&tablename);
		    tablename.buf[tabnamesize]=0;   /* NULL terminate */
		    TRformat(uld_tr_callback, (i4 *)0,
			(char *)&global->ops_trace.opt_trformat[0],
			(i4)sizeof(global->ops_trace.opt_trformat),
			"(%s)",
			(char *)&tablename);
		    uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
		}
	    }
	    if ((cop->opo_union.opo_orig >= subquery->ops_vars.opv_prv)
		&&
		(varp->opv_index.opv_poffset >=0)
		&&
		(varp->opv_index.opv_poffset != cop->opo_union.opo_orig)
		)
	    {
		/* print out underlying base table name of
		** implicitly referenced index */
		(VOID)opt_ptname( subquery, varp->opv_index.opv_poffset, 
					    &tablename, TRUE);
		TRformat(uld_tr_callback, (i4 *)0,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "I(%s)",
		    (char *)&tablename);
		uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
	    }
	    /* print base table name if different from correlation
	    ** name */
	    else if (opt_ptname(subquery,  cop->opo_union.opo_orig, &tablename, TRUE))
	    {
		/* print out corelation name since it is 
		** different from the base table name */
		TRformat(uld_tr_callback, (i4 *)0,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "(%s)",
		    (char *)&tablename);
		uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
	    }
	}
	break;
    }
    case DB_SJ:
    {
        char        *outer_join;
	/*
	** opo_variant is not always setup (case in point is in
	** simple join with trace point op188 set). We need to
	** check for this to prevent a segv (Bug 72480)
	*/
	if (cop->opo_variant.opo_local == NULL)
	    outer_join = (char *)NULL;
	else
	{
            switch (cop->opo_variant.opo_local->opo_type)
            {
            case OPL_LEFTJOIN:
            	outer_join = "left join";
            	break;
            case OPL_RIGHTJOIN:
            	outer_join = "right join";
            	break;
            case OPL_FULLJOIN:
            	outer_join = "full join";
            	break;
            default:
            	outer_join = (char *)NULL;
            	break;
            }
	}

        if (outer_join)
            uld_prnode(uld_control, outer_join);

        if (cop->opo_variant.opo_local && 
	    cop->opo_variant.opo_local->opo_jtype == OPO_SJCARTPROD)
        {
	    char        *existtype;
	    if (cop->opo_existence)
		existtype = "(CO)";
	    else
		existtype = "";
            TRformat(uld_tr_callback, (i4 *)0,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		"Cart-Prod%s", existtype);
	    uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
        }
        else
	{
            char    *join_type; /* string defining type of join */
            char    *prefix;

	    if (cop->opo_existence)
		prefix = "(CO)(";
	    else
		prefix = "(";
	    if (cop->opo_variant.opo_local == NULL)
		join_type = "UNDETERMINED Join";
	    else
	    {
            	switch (cop->opo_variant.opo_local->opo_jtype)
            	{
                    case OPO_SJFSM:
                    	join_type = "FSM Join";/* full sort merge */
                    	break;

                    case OPO_SJPSM:
                    	join_type = "PSM Join";/* partial sort merge */
                    	break;

                    case OPO_SJHASH:
                    	join_type = "Hash Join";/* hash join */
                    	break;

                    case OPO_SEJOIN:
		    	if ((cop->opo_sjeqc == OPE_NOEQCLS)
			    && cop->opo_existence)
			    join_type = "SE Join(CO)";
		    	else
			    join_type = "SE Join"; /* subselect join */
                    	break;

                    case OPO_SJTID:
                    	join_type = "T Join"; /* TID join */
                    	break;

                    case OPO_SJKEY:
                        join_type = "K Join"; /* key join */
			if (cop->opo_inner && 
				cop->opo_inner->opo_sjpr == DB_ORIG
				&&
				cop->opo_inner->opo_storage == DB_RTRE_STORE)
			   join_type = "Spatial Join"; /* goofy spatial join */
                        break;

                    case OPO_SJCARTPROD:
                    	join_type = "CP Join"; /* cart prod join */
                    	break;

                    case OPO_SJNOJOIN:
                    default:
                    	join_type = "UNKNOWN Join"; /* error */
                    	break;
            	}
	    }
    
	    if (cop->opo_sjeqc != OPE_NOEQCLS)
		opt_alist(subquery, uld_control, cop, cop->opo_sjeqc, join_type, prefix);
	    else
		uld_prnode(uld_control, join_type);
	}
	break;
    }
    case OPO_REFORMATBASE:
	break;				/* sort information is printed below */
    default:
    {
	opt_rop (uld_control, (i4)cop->opo_sjpr, opf_sjtyps_tab);
					/* print type of 
					** operation occuring (project-restrict,
					** joining, reformating) */
	break;
    }
    }

    if (global->ops_trace.opt_conode)
    {
	OPO_CO	    *psortp;

	/* check for parent sort node required */
	psortp = NULL;
	opo_fparent(global->ops_trace.opt_conode, cop, &psortp);
	if (psortp
	    &&
	    (
		(   (psortp->opo_outer == cop)
		    &&
		    (psortp->opo_osort)
		)
		||
		(   (psortp->opo_inner == cop)
		    &&
		    (psortp->opo_isort)
		)
	    ))
	{   /* sort is expected by parent so indicate this
	    ** but unfortunately attributes are not readily available */
	    uld_prnode(uld_control, "Sort attr");
	}
    }
    if(cop->opo_psort || (cop->opo_sjpr == DB_RSORT))
    {
	if (cop->opo_ordeqc >=0)
	{   /* if inner or outer need to be sorted */
	    char	    *sort_str;
	    if ((subquery->ops_duplicates == OPS_DREMOVE)
		||
		(cop->opo_pdups && (subquery->ops_duplicates == OPS_DKEEP))
		)
		sort_str = "SortU on";
	    else
		sort_str = "Sort on";
	    opt_alist(subquery, uld_control, cop, cop->opo_ordeqc, sort_str, "(");
	}
	else
	{	/* this must be a top sort node so print duplicate handling */
	    char	    *unique_str;
	    if (subquery->ops_duplicates == OPS_DREMOVE)
		unique_str = "Sort Unique";
	    else
		unique_str = "Sort Keep dups";
	    uld_prnode(uld_control, unique_str);
	}
    }
    /* print equivalence class that this rel is ordered on, it's
    ** structure and the equivalence class the join is on */
    else
    {
	bool	    cflag;		/* compressed flag */
	char	    *structure;		/* ptr to name of storage structure */

        cflag = FALSE;
        if (cop->opo_sjpr == DB_ORIG)
	{   /* set compressed flag */
	    OPV_GRV		*grv_ptr;   /* ptr to global range variable */
	    grv_ptr = subquery->ops_vars.opv_base->opv_rt[cop->opo_union.opo_orig]->opv_grv; /*
					** get ptr to global range table element
					*/
	    if (grv_ptr->opv_relation)
	    {
		cflag = (   grv_ptr->opv_relation->rdr_rel->tbl_status_mask
			    &
			    DMT_COMPRESSED
			) 
			!= 0;
		if (grv_ptr->opv_relation->rdr_parts)
		    nparts = grv_ptr->opv_relation->rdr_parts->nphys_parts;
	    }
	}
	structure = opt_relstrct(cop->opo_storage,cflag);
	if (cop->opo_ordeqc >= 0)
	{
	    bool	    storage_used; /* TRUE if the storage structure
                                        ** is used */
	    OPV_VARS	    *var1_ptr;   /* ptr to global range variable */
	    if (cop->opo_sjpr == DB_ORIG)
	    {	/* check if the storage structure on this relation will be
                ** used, need to traverse CO tree to find the parent
                ** to see if a key join is used */
		var1_ptr = subquery->ops_vars.opv_base->opv_rt[cop->opo_union.opo_orig]; /*
					** get ptr to range table element */
                if (var1_ptr->opv_mbf.opb_count > 0)
		{   /* need to traverse tree to see if this orig node is
		    ** part of a key join */
		    OPO_CO	*parent_cop;
		    parent_cop = NULL;
		    if (global->ops_trace.opt_conode)
			opo_fparent(global->ops_trace.opt_conode, cop, &parent_cop);
		    else
			opo_fparent(subquery->ops_bestco, cop, &parent_cop); /*
					** traverse plan and find parent
					*/
		    storage_used = parent_cop 
			    && 
			    (	(parent_cop->opo_sjpr != DB_SJ && 
                                     var1_ptr->opv_mbf.opb_bfcount > 0)
				||
				(   /* check if TID join is being made and
				    ** indicate that storage structure is not
				    ** used if so */
				    (parent_cop->opo_sjeqc >= 0) /* not a cart product */
				    &&
				    (   var1_ptr->opv_primary.opv_tid 
					!= 
					parent_cop->opo_sjeqc
				    )
				)
			    );
		}
		else
		    storage_used = FALSE;
	    }
	    else
                storage_used =  TRUE;           /* print storage structure used
                                                ** to help partial sort merge strategy
                                                ** determination */
#if 0
                storage_used = (cop->opo_storage == DB_SORT_STORE); /* This ordering can only
                                                    ** be assumed if it is sorted */
#endif

            if (storage_used)
            {
                if (cop->opo_storage == DB_HEAP_STORE)
                    structure = "Partial";
                opt_alist(subquery, uld_control, cop, cop->opo_ordeqc, structure, "(");
                                                /* print out
                                                ** the attribute list for the
                                                ** storage structure */
            }
            else
	    {
                TRformat(uld_tr_callback, (i4 *)0,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "%s(NU)", structure);
		uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
	    }
	}
        else if (cop->opo_storage == DB_HEAP_STORE)
	{
            TRformat(uld_tr_callback, (i4 *)0,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		"%s", structure);
	    uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
	}
	else
	{
	    TRformat(uld_tr_callback, (i4 *)0,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		"%s(NU)", structure);
	    uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
	}
    }

    if ( withStats )
    {
        {
	    /* print the size, in blocks, of relation */
	    i4		reltotb;
	    i4		tups;
	    i4		gcount;
	    i4		reduction;
	    if (cop->opo_cost.opo_reltotb > MAXI4)
	        reltotb = MAXI4;
	    else
	        reltotb = cop->opo_cost.opo_reltotb + 0.5;
	    if (cop->opo_cost.opo_tups > MAXI4)
	        tups = MAXI4;
	    else
	        tups = cop->opo_cost.opo_tups + 0.5;
	    if (cop->opo_variant.opo_local)
		gcount = cop->opo_variant.opo_local->opo_pgcount;
	    else gcount = 0;
#ifndef PAGE_DISPLAY
	    if ((global->ops_cb->ops_check &&
		    opt_strace(global->ops_cb, OPT_F075_FLOATQEP)) ||
		cop->opo_cost.opo_reltotb > MAXI4 ||
		cop->opo_cost.opo_tups > MAXI4)
	    {
		TRformat(uld_tr_callback, (i4 *)0,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "Tups %f Pages %f",
		    cop->opo_cost.opo_tups, cop->opo_cost.opo_reltotb);
		uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
	    }
	    else
	    {
		TRformat(uld_tr_callback, (i4 *)0,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "Pages %d Tups %d",
		    reltotb, tups);
		uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
	    }
#else
	    TRformat(uld_tr_callback, (i4 *)0,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		"Pagesize %d",
		cop->opo_cost.opo_pagesize);
	    uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
#endif
	    if (nparts > 0)
	    {
		TRformat(uld_tr_callback, (i4 *)0,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "Partitions %d", nparts);
		uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
	    }
	    if (cop->opo_cost.opo_reduction > 0.0)
	    {
		reduction = cop->opo_cost.opo_reduction;
		TRformat(uld_tr_callback, (i4 *)0,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "Reduction %d", reduction);
		uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
	    }
	    if (cop->opo_sjpr == DB_EXCH)
	    {
		TRformat(uld_tr_callback, (i4 *)0,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "Threads %d", cop->opo_ccount);
		uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
	    }
	    if (gcount)
	    {
		TRformat(uld_tr_callback, (i4 *)0,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "PC Join count %d", gcount);
		uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
	    }
        }

        /* if an interior node, print cumulative cost to get to this node */
        if (cop->opo_sjpr != DB_ORIG)
        {
	    OPO_CPU	    float_totalcpu;
	    i4	    totalcpu;
	    i4	    dio;

    	    float_totalcpu = (cop->opo_cost.opo_cpu / 
	    global->ops_cb->ops_alter.ops_cpufactor);
	
	    if (float_totalcpu > MAXI4)
	        totalcpu = MAXI4;
	    else
	        totalcpu = float_totalcpu + 0.5;
	    if (cop->opo_cost.opo_dio > MAXI4)
	        dio = MAXI4;
	    else
	        dio = cop->opo_cost.opo_dio + 0.5;

	    if ((global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
	        &&
	        (cop->opo_sjpr != DB_ORIG)
	        )
	    {   /* add network/communication costs for non-orig nodes */
	        i4	network;
	        if (cop->opo_cost.opo_cvar.opo_network > MAXI4)
		    network = MAXI4;
	        else
		    network = cop->opo_cost.opo_cvar.opo_network + 0.5;
	        TRformat(uld_tr_callback, (i4 *)0,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "D%d C%d N%d",
		    dio, totalcpu, network);
		uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
	    }
	    else
	    {
		if ((global->ops_cb->ops_check &&
		    opt_strace(global->ops_cb, OPT_F075_FLOATQEP)) ||
		    cop->opo_cost.opo_cpu > MAXI4 ||
		    cop->opo_cost.opo_dio > MAXI4)
	         TRformat(uld_tr_callback, (i4 *)0,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "D%f C%f",
		    cop->opo_cost.opo_dio, float_totalcpu);
				/* float display for more accuracy */
	         else TRformat(uld_tr_callback, (i4 *)0,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "D%d C%d",
		    dio, totalcpu);
				/* old integer display */
		uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
	    }
        }
    }	/* endif withStats */
}

/*{
** Name: opt_coprintWithoutStats - print a co node without statistics
**
** Description:
**      This routine will print a single CO node without the statistics that
**	cause so many difs in SET QEP output.  This routine is called when
**	you set trace point OP199.
**
** Inputs:
**      coptr                           ptr to CO node to be formatted
**      uld_control                     - used for format control if tree
**                                      is being printed,
**                                      - NULL - indicates that a trace
**                                      format is requested
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
**	15-MAY-92 (rickh)
**	    Created.
[@history_line@]...
*/
static VOID
opt_coprintWithoutStats(
	OPO_CO              *cop,
	PTR                 uld_control)
{
    opt_coprintGuts(cop, uld_control, (bool) FALSE );
}

/*{
** Name: opt_coprint	- print a co node
**
** Description:
**      This routine will print a single CO node.  Note that the session
**      control block is used to access the global state variables which
**      contains the subquery which is associated with the CO node.
**
**	opt_coprint is needed for "set qep," hence can't be turned off.
** Inputs:
**      coptr                           ptr to CO node to be formatted
**      uld_control                     - used for format control if tree
**                                      is being printed,
**                                      - NULL - indicates that a trace
**                                      format is requested
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
**	18-jul-86 (seputis)
**          initial creation
**	21-dec-89 (seputis)
**	    -added checks so float to integer conversion errors do not occur
**	18-feb-89 (seputis)
**          add code for P cart-prod for physical join
**      04-oct-90 (stec)
**          Changed code which determines join type.
**	18-apr-91 (seputis)
**	    add support for op188 trace point
**	12-aug-91 (seputis)
**	    - changed QEP format to print correlation name and base table name
**	    - changed QEP format to print index name and base table name
**	15-may-92 (rickh)
**	    Extracted the guts of this into opt_coprintGuts so that it can
**	    be called to print statistics or not.
[@history_line@]...
*/
VOID
opt_coprint(
	OPO_CO              *cop,
	PTR                 uld_control)
{
    opt_coprintGuts(cop, uld_control, (bool) TRUE );
}

/*{
** Name: opt_tbl	- print formation on table
**
** Description:
**      This routine will dump the table information from RDF
**
** Inputs:
**      tablep                          ptr to DMF DMT_TBL_ENTRY structure
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
**      21-jul-86 (seputis)
**          initial creation
**      06-mar-96 (nanpr01)
**          Added page size for printing in QP.
**      07-Jun-2005 (inifa01) b111211 INGSRV2580
**          Verifydb reports error; S_DU1619_NO_VIEW iirelation indicates that
**          there is a view defined for table a (owner ingres), but none exists,
**          when view(s) on a table are dropped.
**          Were making DMT_ALTER call on DMT_C_VIEW, to turn on iirelation.relstat
**          TCB_VBASE bit, indicating view is defined over table(s) and setting
**          DMT_BASE_VIEW.  TCB_VBASE and DMT_BASE_VIEW
**          not tested anywhere in 2.6 and not turned off when view is dropped.
**          qeu_cview() call to dmt_alter() to set TCB_VBASE has been removed ,
**          removing references to TCB_VBASE, DMT_C_VIEW & DMT_BASE_VIEW.
[@history_template@]...
*/
VOID
opt_tbl(
	RDR_INFO           *rdrinfop)
{
    DMT_TBL_ENTRY       *tablep;
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    bool		tproc;

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    if (rdrinfop->rdr_dbp)
	tproc = TRUE;
    else tproc = FALSE;

    tablep = rdrinfop->rdr_rel;	
    if (!tablep)
    {
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "no table descriptor not in RDF" );
    }
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"DMT_TBL_ENTRY: table name= %.#s, id-base=%d\n\n",
        opt_noblanks( sizeof(tablep->tbl_name.db_tab_name),
			     (char *)tablep->tbl_name.db_tab_name
		    ),
        tablep->tbl_name.db_tab_name,
        tablep->tbl_id.db_tab_base);
    if (!tproc)
     TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    id-index=%d, owner=%.#s, loc=%.#s, filename= %.#s \n\n",
        tablep->tbl_id.db_tab_index,
        opt_noblanks( sizeof(tablep->tbl_owner.db_own_name),
			     (char *)tablep->tbl_owner.db_own_name
		    ),
	tablep->tbl_owner.db_own_name,
        opt_noblanks( sizeof(tablep->tbl_location.db_loc_name),
			     (char *)tablep->tbl_location.db_loc_name
		    ),
        tablep->tbl_location.db_loc_name,
        opt_noblanks( sizeof(tablep->tbl_filename.db_loc_name),
			     (char *)tablep->tbl_filename.db_loc_name
		    ),
        tablep->tbl_filename.db_loc_name);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    att count=%d, index cnt=%d, width=%d, storage=%s, pagesize=%d\n\n",
        tablep->tbl_attr_count,
        tablep->tbl_index_count,
        tablep->tbl_width,
	opt_relstrct((OPO_STORAGE)tablep->tbl_storage_type,
                     ((tablep->tbl_status_mask & DMT_COMPRESSED) != 0)),
	tablep->tbl_pgsize);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    masks=");
    if (tablep->tbl_status_mask & DMT_RD_ONLY)
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" RD_ONLY,");
    if (tablep->tbl_status_mask & DMT_PROTECTION)
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" PROTECTION,");
    if (tablep->tbl_status_mask & DMT_INTEGRITIES)
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" INTEGRITIES,");
    if (tablep->tbl_status_mask & DMT_CONCURRENCY)
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" CONCURRENCY,");
    if (tablep->tbl_status_mask & DMT_VIEW)
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" VIEW,");
    if (tablep->tbl_status_mask & DMT_IDX)
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" IDX,");
    if (tablep->tbl_status_mask & DMT_BINARY)
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" BINARY,");
    if (tablep->tbl_status_mask & DMT_COMPRESSED)
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" COMPRESSED,");
    if (tablep->tbl_status_mask & DMT_ALL_PROT)
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" ALL_PROT,");
    if (tablep->tbl_status_mask & DMT_RETRIEVE_PRO)
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" RETRIEVE_PRO,");
    if (tablep->tbl_status_mask & DMT_EXTENDED_CAT)
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" EXTENDED_CAT,");
    if (tablep->tbl_status_mask & DMT_UNIQUEKEYS)
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" UNIQUEKEYS,");
    if (tablep->tbl_status_mask & DMT_IDXD)
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" IDXD,");
    if (tablep->tbl_status_mask & DMT_JNL)
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" JNL,");
    if (tablep->tbl_status_mask & DMT_ZOPTSTATS)
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" ZOPTSTATS,");

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	
        " \n\n    rec cnt= %d, page cnt= %d, data page= %d, index page=%d \n\n",
        tablep->tbl_record_count,
        tablep->tbl_page_count,
        tablep->tbl_dpage_count,
        tablep->tbl_ipage_count);

}

/*{
** Name: opt_prattr	- print info on one attribute
**
** Description:
**      Routine to print info on one RDF attribute
**
** Inputs:
**      rdrinfop                        ptr to RDF table descriptor
**      dmfattr                         dmf attribute number to print
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
**      23-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
VOID
opt_prattr(
	RDR_INFO           *rdrinfop,
	i4                 dmfattr)
{
    DMT_ATT_ENTRY       *attrp;
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    attrp = rdrinfop->rdr_attr[dmfattr];
    /* attribute numbers are offset by one - so that attribute number 1
    ** will be in array location 0
    */

	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    Attr no= %d, name= %.#s, off= %d, key= %d,",
	    (i4)attrp->att_number,
	    attrp->att_nmlen, attrp->att_nmstr,
	    (i4)attrp->att_offset,
	    attrp->att_key_seq_number);
	{
	    DB_DATA_VALUE	datatype;
	    datatype.db_datatype = (i4)attrp->att_type;
	    datatype.db_prec = (i4)attrp->att_prec;
	    datatype.db_length = (i4)attrp->att_width;
	    opt_dt(&datatype);
	}
    opt_newline(global);

}

/*{
** Name: opt_rattr	- print RDF attribute info
**
** Description:
**      Print the RDF attribute information
**
** Inputs:
**      attrp                           ptr to attribute array
**      maxattr                         number of attributes defined
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
**      21-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
VOID
opt_rattr(
	RDR_INFO           *rdrinfop)
{
    i4                  dmfattr;
    i4                  maxattr;
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"DMT_ATT_ENTRY\n\n");
    maxattr = rdrinfop->rdr_rel->tbl_attr_count; 
    for (dmfattr=1; dmfattr<=maxattr; dmfattr++)
	opt_prattr(rdrinfop, dmfattr);		/* print attribute info */
}

/*{
** Name: opt_pindex	- print info on a particular index
**
** Description:
**      Print info on a particular index.
**
** Inputs:
**      rdrinfop                        ptr to rdf info structure containing
**                                      index
**      dmfindex                        index number to print
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
**      23-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_pindex(
	RDR_INFO           *rdrinfop,
	i4                 dmfindex)
{
    DMT_IDX_ENTRY      *indexp;
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    if (!rdrinfop->rdr_indx)
    {
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    INDEX tuple does not exist for this table\n\n");
	return;
    }
    indexp = rdrinfop->rdr_indx[dmfindex];
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    index no= %d, name= %.#s, keycount= %d, data page=%d\n\n",
	    dmfindex,
	    opt_noblanks( sizeof(indexp->idx_name.db_tab_name),
				 (char *)indexp->idx_name.db_tab_name
			),
	    indexp->idx_name.db_tab_name,
	    (i4)indexp->idx_array_count,
	    (i4)indexp->idx_dpage_count);
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    index page= %d, storage=%s, ",
	    (i4)indexp->idx_ipage_count,
            opt_relstrct((OPO_STORAGE)indexp->idx_storage_type, FALSE));
	{
	    /* print out info on attributes in the indexed table */
	    i4		keycount;	    /* current key being printed */

	    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "Indexed Attributes = " );
	    for (keycount = 0; keycount < indexp->idx_array_count; keycount++)
		TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 " %d ", (i4)indexp->idx_attr_id[keycount]);
	}
    opt_newline(global);
}

/*{
** Name: opt_index	- print RDF index info
**
** Description:
**      Print the RDF attribute information
**
** Inputs:
**      indexp                          ptr to index descriptor array
**      maxindex                        number of indexes defined
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
**      21-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
VOID
opt_index(
	RDR_INFO           *rdrinfop)
{
    i4                  dmfindex;
    i4                  maxindex;
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"DMT_INDEX_ENTRY:\n\n");
    maxindex = rdrinfop->rdr_rel->tbl_index_count;
    for (dmfindex=0; dmfindex<maxindex; dmfindex++)
	opt_pindex(rdrinfop, dmfindex);
}

/*{
** Name: opt_keys	- print info about RDF keys
**
** Description:
**      Print information about RDF key array
**
** Inputs:
**      keyp                            ptr to list of RDF keys
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
**      21-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_key(
	RDR_INFO           *rdrinfop)
{
    RDD_KEY_ARRAY       *keyp;
    i4                  dmfattr;
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    if ((keyp = rdrinfop->rdr_keys) && (keyp->key_count > 0))
    {
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "RDR_KEY: key count=%d, Keys=", keyp->key_count);
	for (dmfattr = 0; dmfattr < keyp->key_count; dmfattr++)
	{
	    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" %d ", (i4)keyp->key_array[dmfattr]);
	}
    }
    else
    {	/* NULL key means heap */
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "RDR_KEY: key count=0, NO KEYS");
    }
    
    opt_newline(global);
}

/*{
** Name: opt_rdf	- print RDF information
**
** Description:
**      This routine will dump the RDF information associated with a global
**      range table
**
** Inputs:
**      relation                        ptr to RDR_INFO struct for table
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
**      21-jul-86 (seputis)
**          initial creation
**	7-may-2008 (dougi)
**	    Change to static and add support for table procedures.
*/
static VOID
opt_rdf(
	OPV_GRV		*grvp)
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    RDR_INFO		*rdrinfop = grvp->opv_relation;

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    if (!rdrinfop)
    {
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "Temporary relation\n\n");
	return;
    }
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"RDR_INFO: ");
    opt_tbl(rdrinfop);			/* print table info */
    opt_rattr(rdrinfop);		/* print attribute info */
    opt_index(rdrinfop);		/* print index info */
    opt_key(rdrinfop);                  /* print keys on primary */
}

/*{
** Name: opt_rdall	- print all RDF info referenced in this query
**
** Description:
**      Routine will print out all RDF information referenced in this query
**
** Inputs:
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
**      23-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_rdall()
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPV_IGVARS          grv;        /* global range variable being dumped*/

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"ALL RDF INFO IN QUERY\n\n");
    for (grv = 0; grv < global->ops_rangetab.opv_gv; grv++)
    {
	OPV_GRV 	*grvp;
	grvp = global->ops_rangetab.opv_base->opv_grv[grv];
        if (grvp && grvp->opv_relation)
	{
	    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"GLOBAL VAR=%d, ", grv);
	    opt_rdf(grvp);		/* dump RDF info for this relation */
	}
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	
"****************************************************************************\n\n"
                 );
    }
}

/*{
** Name: opt_mode	- print subquery mode
**
** Description:
**      routine to print subquery mode
**
** Inputs:
**      mode                            PSF mode of query
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
**      24-aug-86 (seputis)
**	    written
**	10-dec-90 (neil)
**	    Alerters: replaced cases of unused modes.
**	4-may-1992 (bryanp)
**	    Add support for PSQ_DGTT and PSQ_DGTT_AS_SELECT
**	02-nov-92 (jrb)
**	    Changed PSQ_SEMBED to PSQ_SWORKLOC since I'm reusing this query
**	    mode.
**	24-May-2006 (kschendel)
**	    Call generic statement-name routine instead of fixing up the
**	    big switch thing.
[@history_template@]...
*/
static VOID
opt_mode(
	i4                mode)
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"%s",uld_psq_modestr(mode));
}

/*{
** Name: opt_stype	- query tree node symbol type
**
** Description:
**      Print the query tree node symbol type
**
** Inputs:
**      symtype                         symtype of query tree node
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
**      30-aug-86 (seputis)
**          initial creation
**	2-sep-99 (inkdo01)
**	    Added node types for case function.
[@history_template@]...
*/
static VOID
opt_stype(
	i4                symtype)
{
    char                *sym;
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    if ((u_i4)symtype < PST_MAX_ENUM)
	sym = ntype_array[symtype];
    else
	sym = "PST_unknown";
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	sym);
}

/*{
** Name: opt_ptnode	- print query tree node info
**
** Description:
**      Routine to print query tree node info
**
** Inputs:
**      nodep                           ptr to query tree node
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
**      29-aug-86 (seputis)
**          initial creation
**	2-sep-99 (inkdo01)
**	    Added nodes for case function.
[@history_template@]...
*/
static VOID
opt_ptnode(
	PST_QNODE          *nodep)
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"PST_QNODE: left=%p, right=%p, type=",
	nodep->pst_left,
	nodep->pst_right);
    opt_stype(nodep->pst_sym.pst_type);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	", pst_len=%d\n\n",
        nodep->pst_sym.pst_len);

    switch ( nodep->pst_sym.pst_type)
    {
    case PST_UOP:   
    case PST_BOP:  
    case PST_AOP:  
    case PST_COP:  
    case PST_CONST:
    case PST_BYHEAD:
    case PST_RESDOM:
    case PST_VAR:
    case PST_AGHEAD:
    {
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    ");
	opt_dt(&nodep->pst_sym.pst_dataval);
	if (nodep->pst_sym.pst_dataval.db_data)
	{
	    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	", value=");
	    opt_element(nodep->pst_sym.pst_dataval.db_data, 
		&nodep->pst_sym.pst_dataval);
	}
	opt_newline(global);
    }
    }

    switch (nodep->pst_sym.pst_type)
    {
    case PST_AND:
    case PST_OR:
    case PST_UOP:   
    case PST_BOP:  
    case PST_AOP:  
    case PST_COP:  
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    opno=");
        opt_pop(nodep->pst_sym.pst_value.pst_s_op.pst_opno);
        TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	", opinst=%d lcvtid=%d rcvtid=%d\n\n",
	    nodep->pst_sym.pst_value.pst_s_op.pst_opinst,
	    nodep->pst_sym.pst_value.pst_s_op.pst_oplcnvrtid,
	    nodep->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid);
	break;
    case PST_CONST:
    {
	char		*pmspec;
	switch (nodep->pst_sym.pst_value.pst_s_cnst.pst_pmspec)
	{
	case PST_PMNOTUSED:
	    pmspec = "NOTUSED";
	    break;
	case PST_PMUSED:
	    pmspec = "USED";
	    break;
	case PST_PMMAYBE:
	    pmspec = "MAYBE";
	    break;
	default:
	    pmspec = "unknown";
	    break;
	}
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    parmno=%d, pmspec=PST_PM%s",
	    nodep->pst_sym.pst_value.pst_s_cnst.pst_parm_no,
	    pmspec);
	break;
    }
    case PST_BYHEAD:
    case PST_RESDOM:
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    rsno=%d, cnvrtid=%d, name=%.#s\n\n",
	    nodep->pst_sym.pst_value.pst_s_rsdm.pst_rsno,
	    nodep->pst_sym.pst_value.pst_s_rsdm.pst_rsupdt,
	    opt_noblanks( sizeof(nodep->pst_sym.pst_value.pst_s_rsdm.pst_rsname),
			  (char *)nodep->pst_sym.pst_value.pst_s_rsdm.pst_rsname
			),
	    nodep->pst_sym.pst_value.pst_s_rsdm.pst_rsname);
	break;
    case PST_VAR:
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    vno=%d, atno=%d, atname=%.#s\n\n",
	    nodep->pst_sym.pst_value.pst_s_var.pst_vno,
	    nodep->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id,
	    opt_noblanks( sizeof(nodep->pst_sym.pst_value.pst_s_var.pst_atname),
			  (char *)&nodep->pst_sym.pst_value.pst_s_var.pst_atname
			),
	    (char *)&nodep->pst_sym.pst_value.pst_s_var.pst_atname);
	break;
    case PST_ROOT:  
    case PST_AGHEAD:
        TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    tvrc=%d, lvrc=%d, ",
	    nodep->pst_sym.pst_value.pst_s_root.pst_tvrc,
	    nodep->pst_sym.pst_value.pst_s_root.pst_lvrc);
        opt_map("lvrm=", (PTR)&nodep->pst_sym.pst_value.pst_s_root.pst_lvrm, 
	    (i4)sizeof(i4));
        opt_map(", rvrm=", (PTR)&nodep->pst_sym.pst_value.pst_s_root.pst_rvrm, 
	    (i4)sizeof(i4));
        opt_newline(global);
	break;
    case PST_SORT:
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    srvno=%d, srasc=%d\n\n",
	    nodep->pst_sym.pst_value.pst_s_sort.pst_srvno,
	    nodep->pst_sym.pst_value.pst_s_sort.pst_srasc);
	break;
    case PST_CURVAL:
	break;
    case PST_NOT:	
    case PST_CASEOP:
    case PST_WHLIST:
    case PST_WHOP:
    case PST_QLEND: 
    case PST_GSET:
    case PST_GCL:
    case PST_TREE:;
    }
}


/*{
** Name: opa_pasubquery	- print aggregate info about a particular subquery
**
** Description:
**      This routine prints the aggregate processing phase information 
**      about a particular subquery
**
** Inputs:
**      subquery                        ptr to subquery node which will
**                                      be used to display info about
**                                      the aggregate
**      simple                          true if the subquery represents
**                                      a simple aggregate
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
**	11-apr-86 (seputis)
**          initial creation
[@history_line@]...
*/
static VOID
opa_pasubquery(
	OPS_SUBQUERY       *subquery,
	bool		   simple)
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"The address of this subquery list element is %%x%p\n\n", 
	subquery);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"   Address of subtituted node is %%x%p\n\n", 
	*subquery->ops_agg.opa_graft);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"   The address of this aggregate's father is %%x%p\n\n", 
	subquery->ops_agg.opa_father);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"This aggregate has %d aggregates under it\n\n", 
	subquery->ops_agg.opa_nestcnt);
    if (simple)
    {	/* simple aggregate found */
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"Print information on the substituted constant node\n\n");
    }
    else
    {	/* function aggregate found */
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"Print information on the substituted var node\n\n");
    }
    opt_ptnode(*subquery->ops_agg.opa_graft);
}

/*{
** Name: opa_pconcurrent - print info about aggregates executed together
**
** Description:
**      Print information about the aggregates which will be computed 
**      together.
**
** Inputs:
**          header                      first aggregate subquery in list
**                                      of aggregates to be computed
**                                      concurrently
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
**	11-apr-86 (seputis)
**          initial creation
[@history_line@]...
*/
VOID
opa_pconcurrent(
	OPS_SUBQUERY       *header)
{
    OPS_SUBQUERY        *subquery;	/* used to traverse the list of 
					** subqueries which will be evaluated
					** concurrently
					*/
    bool		simple;         /* true if subquery is simple */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    simple = (header->ops_agg.opa_byhead != NULL);
    if (simple)
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"Printing list of simple aggregates executed together\n\n");
    else
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"Printing list of function aggregates executed together\n\n");
    for ( subquery = header->ops_agg.opa_concurrent; /*first subquery in list */
	subquery;                       /* check for end of list */
	subquery = subquery->ops_agg.opa_concurrent) /* get next subquery
                                        ** which will be executed concurrently
                                        */
    {
        /* print aggregate info about a particular subquery, indicate whether
        ** the subquery is simple (i.e. it does not have a byhead node)
        */
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"The address of this aggregate state variable is %%x%p\n\n", 
	    subquery);
	opa_pasubquery(subquery, simple);
    }
}

/*{
** Name: opt_agg	- print information on a particular aggregate
**
** Description:
**      Trace routine to print information on a particular aggregate.
**      Subquery number is parameter to allow easy reference by debugger
**      macros
**
** Inputs:
**      sqno                            relative position of subquery in list
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
**      24-aug-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_agg(
	i4                sqno)
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    i4                  fsqno;      /* relative number of subquery */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    fsqno = 0;
    for (subquery = global->ops_subquery; 
	subquery && (fsqno != sqno); 
	subquery = subquery->ops_next, fsqno++);

    if (!subquery)
    {
        TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"Subquery %d not found\n\n",sqno);
        return;
    }

    {
	char                   *sqtype;
	switch (subquery->ops_sqtype)
	{
	case OPS_MAIN:
	    sqtype = "main";
	    break;
	case OPS_RSAGG:
	    sqtype = "correlated simple agg";
	    break;
	case OPS_SAGG:
	    sqtype = "simple agg";
	    break;
	case OPS_FAGG:
	    sqtype = "function agg";
	    break;
	case OPS_HFAGG:
	    sqtype = "hash function retrieve agg";
	    break;
	case OPS_RFAGG:
	    sqtype = "function retrieve agg";
	    break;
	case OPS_PROJECTION:
	    sqtype = "projection";
	    break;
	case OPS_SELECT:
	    sqtype = "subselect";
	    break;
	case OPS_UNION:
	    sqtype = "union";
	    break;
	case OPS_VIEW:
	    sqtype = "view";
	    break;
	default:
	    sqtype = "unknown";
	}
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"OPS_SUBQUERY: addr=%p, sqtype=%s, ", 
	    subquery,
	    sqtype);
    }
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"root=%p, result=%d\n\n",
        subquery->ops_root,
	subquery->ops_gentry);

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    projection=%d, mode=", subquery->ops_projection);
    opt_mode(subquery->ops_mode);   /* print mode of subquery */
    {
	char		    *duplicates;
	switch (subquery->ops_duplicates)
	{
	case OPS_DUNDEFINED:
	    duplicates = "OPS_DUNDEFINED";
	    break;
	case OPS_DKEEP:
	    duplicates = "OPS_DKEEP";
	    break;
	case OPS_DREMOVE:
	    duplicates = "OPS_DREMOVE";
	    break;
	default:
	    duplicates = "unknown";
	}
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	", duplicates=%s, ", duplicates);
    }
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"width=%d, cost=%.2f\n\n",
	subquery->ops_width,
	subquery->ops_cost);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"     bestco=%p, vmflag=%d\n\n",
        subquery->ops_bestco,
        subquery->ops_vmflag);
    /* print aggregate info */
    if (subquery->ops_agg.opa_graft && ( *(subquery->ops_agg.opa_graft)))
        opt_ptnode(*(subquery->ops_agg.opa_graft));
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    father=%p, concurrent=%p, byhead=%p\n\n",
	subquery->ops_agg.opa_father,
        subquery->ops_agg.opa_concurrent,
        subquery->ops_agg.opa_byhead);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    aop=%p, redundant=%d, nestcnt=%d, attlast=%d\n\n",
        subquery->ops_agg.opa_aop,
        subquery->ops_agg.opa_redundant,
        subquery->ops_agg.opa_nestcnt,
        subquery->ops_agg.opa_attlast);
    if ((subquery->ops_sqtype == OPS_FAGG)
	||
	(subquery->ops_sqtype == OPS_RFAGG)
	||
	(subquery->ops_sqtype == OPS_HFAGG)
       )
    {
	OPV_IGVARS	    maxvar;
	maxvar = global->ops_rangetab.opv_gv;
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    need proj=%d, multi_parent=%d, ",
	    subquery->ops_agg.opa_projection,
	    subquery->ops_agg.opa_multi_parent);
	opt_map("blmap=", (PTR)&subquery->ops_agg.opa_blmap, (i4)maxvar);
	opt_map(", visible=", (PTR)&subquery->ops_agg.opa_visible, (i4)maxvar);
        opt_newline(global);
    }
}
#ifdef xDEBUG

/*{
** Name: opt_agall	- print information on all the aggregates
**
** Description:
**      Trace routine to dump info on all the aggregates
**
** Inputs:
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
**      24-aug-86 (seputis)
**          initial creation
[@history_template@]...
*/
 VOID
opt_agall()
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    i4                  sqno;       /* relative number of subquery */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    sqno = 0;
    for (subquery = global->ops_subquery; 
	subquery; 
	subquery = subquery->ops_next, sqno++)
	opt_agg(sqno);              /* print subquery info */
    
}
#endif

/*{
** Name: opt_hist	- print info on histogram element
**
** Description:
**      This routine will dump info on a histogram element
**
** Inputs:
**      histp                           ptr to histogram element
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
**      21-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_hist(
	OPH_HISTOGRAM      *histp)
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPH_INTERVAL	*intervalp; /* attr associated with histogram */
    OPZ_ATTS            *attrp;     /* ptr to attribute with interval info */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    if (!histp)
    {
	TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"OPH_HISTOGRAM: NULL PTR \n\n");
	return;
    }
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */
    attrp = subquery->ops_attrs.opz_base->opz_attnums[histp->oph_attribute];
    intervalp = &attrp->opz_histogram;	    /* get ptr to interval information
                                            ** for the histogram */
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"OPH_HISTOGRAM: address=%p next=%p numcells=%d numexcells=%d\n\n",
	histp, histp->oph_next, intervalp->oph_numcells, histp->oph_numexcells);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    attnum=%d equcls=%d unique=%d complete=%d default=%d odefined=%d \n\n",
	histp->oph_attribute, 
	attrp->opz_equcls, 
	histp->oph_unique,
	histp->oph_complete,
	histp->oph_default,
	histp->oph_odefined);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    reptf:%.2f nuniq:%.0f prodarea=%.5f null=%.2f pnull=%.2f\n\n",
	histp->oph_reptf, 
	histp->oph_nunique,
	histp->oph_prodarea,
	histp->oph_null,
	histp->oph_pnull);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    origtuples=%f mask=%v pcnt=%p exactcount=%.5f\n\n",
	histp->oph_origtuples, 
	"OJLEFTHIST,COMPOSITE,INCLIST,KESTIMATE,ALLEXACT",histp->oph_mask,
	histp->oph_pcnt, histp->oph_exactcount);
    if (FALSE)		/* comment this out if you really want the cells */
    {
	OPH_COUNTS		counts;
	char			*hmm;
	i4                     cell;

	hmm = (char *)intervalp->oph_histmm;
	counts = histp->oph_fcnt;
	for (cell = 0; cell < intervalp->oph_numcells; cell++)
	{
	    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" count[%d]=%.3f, bound=", cell, *counts);
	    opt_interval(hmm, &intervalp->oph_dataval);
	    counts++;			    /* increment to next cell count*/
	    hmm += intervalp->oph_dataval.db_length;
	}
    }
    opt_newline (global);
}
#ifdef OPT_F029_HISTMAP

/*{
** Name: opt_ehistmap	- print map on histograms
**
** Description:
**      This routine will dump info on a histogram element
**
** Inputs:
**      histp                           ptr to histogram element
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
**      21-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
VOID
opt_ehistmap(
	OPE_BMEQCLS        *eqh)
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"BITMAP OF EQUIVALENCE CLASSES which require histograms\n\n");
    opt_map("    equivalence classes =", (PTR)eqh, (i4)BITS_IN(*eqh));
    opt_newline (global);
}
#endif

/*{
** Name: opt_trl	- ptr trl structure
**
** Description:
**      Routine to print OPN_RLN structure
**
** Inputs:
**      trl                             ptr to OPN_RLN structure to print
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
**      31-jul-86 (seputis)
**          initial creation
**	7-mar-96 (inkdo01)
**	    Add display of trl->opn_relsel.
[@history_template@]...
*/
static VOID
opt_trl(
	OPN_RLS            *trl)
{

    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"OPN_RLS: rlnext=%p, eqp=%p, histp=%p, reltups=%.2f, relsel=%.3f, del?=%d\n\n",
	trl->opn_rlnext,
	trl->opn_eqp,
        trl->opn_histogram,
	trl->opn_reltups,
	trl->opn_relsel, 
        trl->opn_delflag);
    opt_map("    relmap=", (PTR)&trl->opn_relmap, (i4)BITS_IN(trl->opn_relmap));
/*  opt_map(", interesting ord=", (PTR)&trl->opn_iomap, (i4)BITS_IN(trl->opn_iomap)); */
    opt_newline(global);
}

/*{
** Name: opt_esp	- print OPN_EQS structure
**
** Description:
**      Print the OPN_EQS structure
**
** Inputs:
**      eqsp                            ptr to OPN_EQS structure to print
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
**      4-aug-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_esp(
	OPN_EQS            *eqsp)
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "OPN_EQS: addr=%p, next=%p, subtree=%p, width=%d,",
	eqsp,
	eqsp->opn_eqnext,
	eqsp->opn_subtree,
	eqsp->opn_relwid);
    opt_map(" eqcls=", (PTR)&eqsp->opn_maps.opo_eqcmap, (i4)subquery->ops_eclass.ope_ev);
    opt_newline(global);
}

/*{
** Name: opt_stp	- print OPN_SUBTREE structure
**
** Description:
**      Print info in OPN_SUBTREE structure
**
** Inputs:
**      stp                             ptr to OPN_SUBTREE structure to print
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
**      4-aug-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_stp(
	OPN_SUBTREE        *stp,
	i4		i)
{
    i4                  count;		/* number of nodes in tree description*/
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"SUBTREE: addr=%p, coforw=%p, coback=%p, next=%p, ",
	stp,
	stp->opn_coforw,
	stp->opn_coback,
	stp->opn_stnext);

    opt_pvmap("relasgn=", (OPV_IVARS *)stp->opn_relasgn, (i4) i);
    {
	for (count=0; stp->opn_structure[count++] || (count > OPN_MAXNODE););
	opt_pi1map(", structure =", (i1 *)stp->opn_structure, (i4)count);
    }
    opt_newline(global);
}

/*{
** Name: opt_enum	- print entire enumeration memory structure
**
** Description:
**      Routine to print OPN enumeration memory structure
**
** Inputs:
**      none
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
**	1-oct-02 (inkdo01)
**	    Written.
[@history_template@]...
*/
VOID
opt_enum()
{

    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    i4			i;
    OPN_RLS		*rlp;
    OPN_EQS		*eqp;
    OPN_SUBTREE		*stp;

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    for (i = 1; i < 30; i++)
     if ((rlp = global->ops_estate.opn_sbase->opn_savework[i]))
     {
	opt_newline(global);
	TRformat(opt_scc, (i4 *)global,
	    (char *)&global->ops_trace.opt_trformat[0],
	    (i4)sizeof(global->ops_trace.opt_trformat),
	    "OPN enumeration memory structure: order %d subtrees\n\n", i);
	for ( ; rlp; rlp = rlp->opn_rlnext)
	{
	    opt_trl(rlp);
	    for (eqp = rlp->opn_eqp; eqp; eqp = eqp->opn_eqnext)
	    {
		opt_esp(eqp);
		for (stp = eqp->opn_subtree; stp; stp = stp->opn_stnext)
		 opt_stp(stp, i);
	    }
	}
     }
    opt_newline(global);

}

/*{
** Name: opt_evp	- print OPN_EVAR structure array
**
** Description:
**      Print info in OPN_EVAR structure array
**
** Inputs:
**      evarp				ptr to OPN_EVAR array to print
**	vcount				count of entries in evarp
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
**      4-oct-02 (inkdo01)
**          Written.
**	7-Nov-2008 (kschendel) b122118
**	    Add a little more flag info.
[@history_template@]...
*/
VOID
opt_evp(
	OPN_EVAR	*evp,
	i4		vcount)
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPS_SUBQUERY	*subquery;
    i4			i, j;

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */

    for (i = 0; i < vcount; i++)
    {
	TRformat(opt_scc, (i4 *)global,
	    (char *)&global->ops_trace.opt_trformat[0],
	    (i4)sizeof(global->ops_trace.opt_trformat),
	    "EVAR[%d]: %v", i,
		"TABLE,COMP,DONE",evp[i].opn_evmask);

	if (evp[i].opn_evmask & OPN_EVDONE)
	{
	    opt_newline(global);
	    continue;
	}
	if (evp[i].opn_evmask & OPN_EVTABLE)
	{
	    OPV_VARS *varp = subquery->ops_vars.opv_base->opv_rt[evp[i].u.v.opn_varnum];
	    if (varp->opv_mask & (OPV_CINDEX | OPV_CPLACEMENT))
	    {
		TRformat(opt_scc, (i4 *)global,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    " %s ix, prim %d",
		    (varp->opv_mask & OPV_CINDEX) ? "replace" : "norm",
		    varp->opv_index.opv_poffset);
	    }
	    opt_map(" ixmap=", (PTR)&evp[i].u.v.opn_ixmap,
					subquery->ops_vars.opv_rv);
	}
	opt_map(" varmap=", (PTR)&evp[i].opn_varmap, 
					subquery->ops_vars.opv_rv);
	opt_map(" joinable=", (PTR)&evp[i].opn_joinable, 
					subquery->ops_vars.opv_rv);
	opt_newline(global);
	opt_map("    equivalence classes =", (PTR)&evp[i].opn_eqcmap,
				(i4)BITS_IN(evp[i].opn_eqcmap));
	opt_newline(global);
	if (BTnext(-1, (PTR)&evp[i].opn_snumeqcmap, 
				BITS_IN(evp[i].opn_snumeqcmap)) >= 0)
	{
	    opt_map("    SEjoin eq classes =", (PTR)&evp[i].opn_snumeqcmap, 32);
	    opt_newline(global);
	}
    }
    opt_newline(global);
}

/*{
** Name: opt_rvtrl	- print trl associated with range variable
**
** Description:
**      Print the OPN_RLS structure associated with a joinop range variable
**
** Inputs:
**      rvar                            joinop range var number
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
**      31-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_rvtrl(
	OPV_IVARS          rv)
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPV_VARS            *rvp;       /* ptr to range var element to display*/

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */
    rvp = subquery->ops_vars.opv_base->opv_rt[rv];
    opt_trl(rvp->opv_trl);
}

/*{
** Name: opt_hrv	- print list of histograms associated with a var
**
** Description:
**      Print list of histograms associated with a joinop range var
**
** Inputs:
**      rv                              joinop range var for histogram list
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
**      31-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_hrv(
	OPV_IVARS          rv)
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPV_VARS            *rvp;       /* ptr to range var element to display*/
    OPH_HISTOGRAM       *histp;     /* ptr to histogram element */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */
    rvp = subquery->ops_vars.opv_base->opv_rt[rv];
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"VARS HISTOGRAM LIST: varno=%d\n\n", rv);
    for (histp = rvp->opv_trl->opn_histogram; histp; histp=histp->oph_next)
	opt_hist(histp);
}

/*{
** Name: opt_rv	- print info on a range variable
**
** Description:
**      This routine will print info on a joinop range variable
**
** Inputs:
**      rv                              joinop range variable number
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
**      23-jul-86 (seputis)
**          initial creation
**	7-apr-04 (inkdo01)
**	    Added display of OPV_PCJDESC structures (if any) for partitioned
**	    tables/parallel queries.
**	3-Sep-2005 (schka24)
**	    Added call to print histograms - code was there but nothing
**	    was calling it, seems to make sense to use it.
[@history_template@]...
*/
 VOID
opt_rv(
	OPV_IVARS          rv, OPS_SUBQUERY   *sq, bool	histdump)
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPV_VARS            *rvp;       /* ptr to range var element to display*/
    OPE_IEQCLS          maxeqcls;   /* number of equivalence classes */
    OPV_PCJDESC		*pcjdp;	    /* ptr to partition compatible join
				    ** structures (if any) */
    i4			i;

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    if (sq != NULL) subquery = sq;
      else
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */
    maxeqcls = subquery->ops_eclass.ope_ev;
    rvp = subquery->ops_vars.opv_base->opv_rt[rv];
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"OPV_VARS: var=%d, gvar=%d, grv=%p, index: primvar=%d, ord eqcls=%d\n\n",
	rv,
	rvp->opv_gvar,
	rvp->opv_grv,
        rvp->opv_index.opv_poffset,
        rvp->opv_index.opv_eqclass);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    primary: idx?=%d, tid eqc=%d, ojeqc=%d",
	rvp->opv_primary.opv_indexcnt,
	rvp->opv_primary.opv_tid, rvp->opv_ojeqc);
    opt_map(" teqcmp=", (PTR)&rvp->opv_primary.opv_teqcmp, maxeqcls);
    opt_newline(global);
    opt_map("    eqcmp=", (PTR)&rvp->opv_maps.opo_eqcmap, maxeqcls);
    opt_pmap("   joinable=", 
	(char *)&rvp->opv_joinable, 
	(i4)subquery->ops_vars.opv_rv);
    if ((rvp->opv_mask & OPV_LTPROC) && rvp->opv_parmmap)
	opt_map(" parmmap=", (PTR)rvp->opv_parmmap, subquery->ops_vars.opv_rv);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    trl=%p, mask=%%x%p, keywidth=%d\n\n",
	rvp->opv_trl, rvp->opv_mask, rvp->opv_mbf.opb_keywidth);
    opt_map("    ijmap=", (PTR)rvp->opv_ijmap,(i4)BITS_IN(*rvp->opv_ijmap));
    opt_map("  ojmap=", (PTR)rvp->opv_ojmap,(i4)BITS_IN(*rvp->opv_ojmap));
    opt_newline(global);
    /* Display partition compatible join information (if any) */
    for (pcjdp = rvp->opv_pcjdp; pcjdp; pcjdp = pcjdp->jn_next)
    {
	TRformat(opt_scc, (i4 *)global,
	    (char *)&global->ops_trace.opt_trformat[0],
	    (i4)sizeof(global->ops_trace.opt_trformat),
	    "    pcjoinable : var=%d, dim1=%d, dim2=%d, eqc: %d",
	    pcjdp->jn_var2, pcjdp->jn_dim1, pcjdp->jn_dim2,pcjdp->jn_eqcs[0]);
	for (i = 1; i < 6 && pcjdp->jn_eqcs[i] >= 0; i++)
	 TRformat(opt_scc, (i4 *)global,
	    (char *)&global->ops_trace.opt_trformat[0],
	    (i4)sizeof(global->ops_trace.opt_trformat),
	    " %d", pcjdp->jn_eqcs[i]);
	opt_newline(global);
    }
    if (sq == NULL) opt_codump(rvp->opv_tcop);
    if (histdump)
	opt_hrv(rv);
}

/*{
** Name: opt_ojstuff - print contents of OPF outer join table
**
** Description:
**      This routine will print contents of occupied ojt entries  
**
** Inputs:
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
**      21-oct-94 (inkdo01)
**          initial creation
[@history_template@]...
*/
VOID
opt_ojstuff(OPS_SUBQUERY  *sq1)
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPL_OUTER           *outerp;    /* ptr to current table entry */
    OPL_OJT             *lbase;     /* ptr to ojt ptr array */
    OPL_IOUTER           maxoj;     /* count of ojt entries       */
    i2                   i;         /* loop counter */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */
    if (sq1) subquery = sq1;       /* use caller's sq, if avail */
    if (!sq1 ||                    /* don't bother if there's nothin' here */
        !(lbase = subquery->ops_oj.opl_base) ||
	   !(maxoj = subquery->ops_oj.opl_lv)) return;
    opt_newline(global);
                                                       
    for (i = 0; i < maxoj; i++)
    {
	opt_newline(global);
	outerp =lbase->opl_ojt[i];     /* address the next ojt entry */
	opt_newline(global);
	TRformat(opt_scc, (i4 *)global,
       	   (char *)&global->ops_trace.opt_trformat[0],
   	   (i4)sizeof(global->ops_trace.opt_trformat),
   "OPL_OUTER: entry=%d, id=%d, gid=%d, node=%p, type=%d, translate=%d, mask=%d\n",
	   i, outerp->opl_id, outerp->opl_gid, outerp->opl_nodep, outerp->opl_type,
	   outerp->opl_translate, outerp->opl_mask);
       opt_newline(global);
      opt_map("  ivmap=", (PTR)outerp->opl_ivmap,(i4)BITS_IN(*outerp->opl_ivmap));
      opt_map("  ovmap=", (PTR)outerp->opl_ovmap,(i4)BITS_IN(*outerp->opl_ovmap));
      opt_map("maxojmap=", (PTR)outerp->opl_maxojmap,(i4)BITS_IN(*outerp->opl_maxojmap));
      opt_map("  idmap=", (PTR)outerp->opl_idmap,(i4)BITS_IN(*outerp->opl_idmap));
      opt_map("onclause=", (PTR)outerp->opl_onclause,(i4)BITS_IN(*outerp->opl_onclause));
      opt_map("reqinner=", (PTR)outerp->opl_reqinner,(i4)BITS_IN(*outerp->opl_reqinner));
      opt_map("innereqc=", (PTR)outerp->opl_innereqc,(i4)BITS_IN(*outerp->opl_innereqc));
    opt_newline(global);
      opt_map("rmaxojmap=", (PTR)outerp->opl_rmaxojmap,(i4)BITS_IN(*outerp->opl_rmaxojmap));
      opt_map("maxojmap=", (PTR)outerp->opl_maxojmap,(i4)BITS_IN(*outerp->opl_maxojmap));
      opt_map("minojmap=", (PTR)outerp->opl_minojmap,(i4)BITS_IN(*outerp->opl_minojmap));
      opt_map("ojtotal=", (PTR)outerp->opl_ojtotal,(i4)BITS_IN(*outerp->opl_ojtotal));
      opt_map("bvmap=", (PTR)outerp->opl_bvmap,(i4)BITS_IN(*outerp->opl_bvmap));
       opt_newline(global);
      opt_map("ojbvmap=", (PTR)outerp->opl_ojbvmap,(i4)BITS_IN(*outerp->opl_ojbvmap));
      opt_map("bfmap=", (PTR)outerp->opl_bfmap,(i4)BITS_IN(*outerp->opl_bfmap));
      opt_map("ojattr=", (PTR)outerp->opl_ojattr,(i4)BITS_IN(*outerp->opl_ojattr));
      opt_map("p1=", (PTR)outerp->opl_p1,(i4)BITS_IN(*outerp->opl_p1));
      opt_map("p2=", (PTR)outerp->opl_p2,(i4)BITS_IN(*outerp->opl_p2));
      TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"  \n\n************\n");  /* delimit entries */
      }     /* end of ojt loop */
    opt_newline(global);
}
/*{
** Name: opt_rall	- print all info about the joinop range table
**
** Description:
**      Prints all the info about the joinop range table
**
** Inputs:
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
**      23-jul-86 (seputis)
**          initial creation
**	29-aug-02 (inkdo01)
**	    Externalize for separate invocation by tp op153.
[@history_template@]...
*/
VOID
opt_rall(bool	histdump)
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPV_IVARS           rv;         /* joinop range variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */
    opt_newline(global);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"SUBQUERY (%d):\n", subquery->ops_tsubquery);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"OPV_RANGEV: prv=%d, rv=%d, base=%p, cartprod?=%d, ",
	subquery->ops_vars.opv_prv,
	subquery->ops_vars.opv_rv,
	subquery->ops_vars.opv_base,
	subquery->ops_vars.opv_cart_prod);
    opt_newline(global);
    for (rv = 0; rv < subquery->ops_vars.opv_rv; rv++)
	opt_rv(rv,NULL,histdump);
    opt_newline(global);
    /* Dump outer join descriptors, too. */
    if (subquery->ops_oj.opl_base)
	opt_ojstuff(subquery);

}

/*{
** Name: opt_grange	- print global range table element
**
** Description:
**      This routine will print a global range table element
**
** Inputs:
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
**      21-jul-86 (seputis)
**          initial creation
**      18-sep-92 (ed)
**          bug 44850 - changed global to local range table mapping
**	30-Jun-2006 (kschendel)
**	    gcop is gone, print interesting bits of gcost.
[@history_template@]...
*/
VOID
opt_grange(
	OPV_IGVARS         grv)
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPV_GRV             *grvp;      /* ptr to global range variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    grvp = global->ops_rangetab.opv_base->opv_grv[grv]; /* get ptr to global
				    ** range variable */
    if (!grvp)
	return;			    /* check if global range variable is used */
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	
        "OPV_GRV: grv:%d, relp:%p, gmask:%x, parser rv:%d, lrv:%d",
	grv,
        grvp->opv_relation,
	grvp->opv_gmask,
        grvp->opv_qrt,
        global->ops_rangetab.opv_lrvbase[grv]);
    opt_newline(global);
    opt_rdf(grvp);	    		/* print RDF information */
    /* print the size, in blocks, of output relation */
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"\n    Disk pages =%d, Total Tups = %d, Page Size = %d\n",
	    (i4)grvp->opv_gcost.opo_reltotb, 
	    (i4)grvp->opv_gcost.opo_tups,
	    (i4)grvp->opv_gcost.opo_pagesize);
}

/*{
** Name: opt_gtable	- print global range table
**
** Description:
**      Print all the elements in the global range table
**
** Inputs:
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
**      22-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_gtable()
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPV_IGVARS          grv;        /* global range variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "OPV_GLOBAL_RANGE: base=%p, num=%d\n\n",
	global->ops_rangetab.opv_base,
	global->ops_rangetab.opv_gv);
    for (grv = 0; grv < global->ops_rangetab.opv_gv; grv++)
    {
	opt_grange(grv);	
        TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	
"****************************************************************************\n\n"
                 );
    }
}

/*{
** Name: opt_jnode	- print OPN_JTREE node
**
** Description:
**      Print contents of an OPN_JTREE node
**
** Inputs:
**      nodep                           ptr to OPN_JTREE node to print
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
**      31-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
 VOID
opt_jnode(
	OPN_JTREE          *nodep)
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "OPN_JTREE: addr=%p, left=%p, right=%p, nchild=%d, npart=%d, ojid=%d\n\n",
	nodep,
        nodep->opn_child[OPN_LEFT],
        nodep->opn_child[OPN_RIGHT],
        nodep->opn_nchild,
	nodep->opn_npart,
	nodep->opn_ojid);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    nleaves=%d,", nodep->opn_nleaves);
    opt_pvmap(" prb=", nodep->opn_prb, (i4)nodep->opn_nleaves);
    opt_pvmap(", pra=", nodep->opn_pra, (i4)nodep->opn_nleaves);
    opt_newline(global);

    opt_pvmap("    rlasg=", nodep->opn_rlasg, (i4)nodep->opn_nleaves);
    {
	OPN_ITDSC	count;		/* get number of elements in sbstruct*/
	for (count=0; nodep->opn_sbstruct[count++] || (count > OPN_MAXNODE););
	opt_pi1map(", sbstruct=", (i1 *)nodep->opn_sbstruct, (i4)count);
    }
    opt_newline(global);

    opt_map("    eqm=", (PTR)&nodep->opn_eqm, (i4)BITS_IN(nodep->opn_eqm));
    opt_map("    rlmap=", (PTR)&nodep->opn_rlmap, (i4)BITS_IN(nodep->opn_rlmap));
    opt_map("ojevalmap=", (PTR)&nodep->opn_ojevalmap,
	    (i4)BITS_IN(nodep->opn_ojevalmap));
    opt_map("ojinnermap=", (PTR)&nodep->opn_ojinnermap,
	    (i4)BITS_IN(nodep->opn_ojinnermap));
    opt_newline(global);

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    left  child: pi=%d, csz=%d, psz=%d, cpeqc=%d\n\n",
	nodep->opn_pi[OPN_LEFT],
	nodep->opn_csz[OPN_LEFT],
	nodep->opn_psz[OPN_LEFT],
	nodep->opn_cpeqc[OPN_LEFT]);
    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"    right child: pi=%d, csz=%d, psz=%d, cpeqc=%d\n\n",
	nodep->opn_pi[OPN_RIGHT],
	nodep->opn_csz[OPN_RIGHT],
	nodep->opn_psz[OPN_RIGHT],
	nodep->opn_cpeqc[OPN_RIGHT]);
}

/*{
** Name: opt_rjtree	- recursive descent down the join tree
**
** Description:
**      print join tree node and children
**
** Inputs:
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
**      31-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_rjtree(
	OPN_JTREE          *nodep)
{
    if (!nodep)
	return;
    opt_jnode(nodep);
    if (nodep->opn_nleaves == 1)
	return;
    opt_rjtree(nodep->opn_child[OPN_LEFT]);
    opt_rjtree(nodep->opn_child[OPN_RIGHT]);
}

/*{
** Name: opt_jtree	- print join structurally unique tree
**
** Description:
**      Print the nodes in the join structurally unique tree 
**
** Inputs:
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
**      31-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
       VOID
opt_jtree()
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    opt_rjtree(global->ops_estate.opn_sroot); /* recursive descent to print 
                                    ** tree */
}

/*{
** Name: opt_eclass	- print info on one equivalence class
**
** Description:
**      This routine will print info on one equivalence class 
**
** Inputs:
**      eqcls                           equivalence class to print
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
**      21-jul-86 (seputis)
**         initial creation
[@history_template@]...
*/
VOID
opt_eclass(
	OPE_IEQCLS         eqcls)
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPE_EQCLIST         *eqclsp;    /* ptr to equivalence class element */
    char                *eqctype;   /* ptr to eqctype name */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */
    eqclsp = subquery->ops_eclass.ope_base->ope_eqclist[eqcls]; /* ptr to
                                    ** equivalence class element */
    if (eqclsp->ope_eqctype == OPE_NONTID)
	eqctype = "OPE_NONTID";
    else if (eqclsp->ope_eqctype == OPE_TID)
	eqctype = "OPE_TID   ";
    else
        eqctype = "UNKNOWN EQCTYPE";

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	" EQCLS= %d, eqctype= %s, bfindex= %d, nbf= %d, mask= %d, ",
	eqcls,
        eqctype,
	eqclsp->ope_bfindex,
        eqclsp->ope_nbf,
	eqclsp->ope_mask);

    opt_map ("attrmap", (PTR)&eqclsp->ope_attrmap, 
	(i4)subquery->ops_attrs.opz_av);
    opt_newline(global);
}

/*{
** Name: opt_eall	- print all the equivalence class structures
**
** Description:
**      This routine will print out info on all the equivalence classes
**      currently defined.
**
** Inputs:
**	Input parameters are implicitly obtained from the session control
**      block.
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
**      21-jul-86 (seputis)
**          initial creation prEqlist
[@history_template@]...
*/
VOID
opt_eall()
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPE_IEQCLS          eqcls;      /* current equivalence class being traced*/
    OPE_IEQCLS          maxeqcls;   /* number of equivalence classes defined */


    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */
    maxeqcls = subquery->ops_eclass.ope_ev; 

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"OPE_EQCLIST: maxeqcls = %d, ptr base= %p, ",
	maxeqcls,
        subquery->ops_eclass.ope_base);
    opt_map(", seqm", (PTR)&subquery->ops_eclass.ope_maps.opo_eqcmap, (i4)maxeqcls);
    opt_newline(global);

    for (eqcls = 0; eqcls < maxeqcls; eqcls++)
	if (subquery->ops_eclass.ope_base->ope_eqclist[eqcls] )
	    opt_eclass(eqcls);	    /* print the equivalence class info */
}

/*{
** Name: opt_jall   - print all the joinop table information
**
** Description:
**      This routine will print out info on all the joinop tables
**      currently defined.
**
** Inputs:
**      subquery			    subquery to be dumped
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
**      21-jul-86 (seputis)
**          initial creation prEqlist
[@history_template@]...
*/
VOID
opt_jall(
	OPS_SUBQUERY	*subquery)
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    global->ops_trace.opt_subquery = subquery ; /* save ptr to current subquery
                                    ** which is being traced */

    TRformat(opt_scc, (i4 *)global,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	"JOINOP TABLES \n\n");
    opt_rdall();		    /* RDF info */
    opt_newline(global);
    opt_gtable();		    /* global range table info */
    opt_newline(global);
    opt_rall(TRUE);                 /* local range table info */
    opt_newline(global);
    opt_attnums();		    /* joinop attributes array */
    opt_newline(global);
    opt_eall();                     /* equivalence class info */
    opt_newline(global);
    opt_fall();                     /* function attribute info */
    opt_newline(global);
    opt_ball();                     /* boolean factor info */
    opt_newline(global);
}

/*{
** Name: opt_coleft	- return left son of a CO tree
**
** Description:
**      This routine will return the left son of a CO tree.
**
** Inputs:
**      cop                             ptr to co struct parent of left son
**
** Outputs:
**	Returns:
**	    ptr to OPO_CO struct left son of node
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	18-jul-86 (seputis)
**          initial creation
[@history_line@]...
*/
PTR
opt_coleft(
	OPO_CO             *cop)
{
    return ((PTR) cop->opo_outer);
}

/*{
** Name: opt_coright	- return right son of a CO tree
**
** Description:
**      This routine will return the right son of a CO tree.
**
** Inputs:
**      cop                             ptr to co struct parent of right son
**
** Outputs:
**	Returns:
**	    ptr to OPO_CO struct right son of node
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	18-jul-86 (seputis)
**          initial creation
[@history_line@]...
*/
PTR
opt_coright(
	OPO_CO             *cop)
{
    return ((PTR)cop->opo_inner);
}

/*{
** Name: opt_jleft	- return ptr to left child of join tree
**
** Description:
**      Routine to return pointer of left child of join tree node
**
** Inputs:
**      jp                              ptr to join tree  node
**
** Outputs:
**	Returns:
**	    ptr to left child of join tree node
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      3-aug-86 (seputis)
**          initial creation
[@history_template@]...
*/
static PTR
opt_jleft(
	OPN_JTREE          *jp)
{
    return ((PTR)jp->opn_child[OPN_LEFT]);
}

/*{
** Name: opt_jright	- return ptr to right child of join tree
**
** Description:
**      Routine to return pointer of right child of join tree node
**
** Inputs:
**      jp                              ptr to join tree  node
**
** Outputs:
**	Returns:
**	    ptr to right child of join tree node
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      3-aug-86 (seputis)
**          initial creation
[@history_template@]...
*/
static PTR
opt_jright(
	OPN_JTREE          *jp)
{
    return ((PTR)jp->opn_child[OPN_RIGHT]);
}

/*{
** Name: opt_fjprint	- print a node of a join tree
**
** Description:
**      Routine to print a node of a formatted join tree
**
** Inputs:
**      jp                              ptr to a join tree node
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
**      3-aug-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_fjprint(
	OPN_JTREE          *jp,
	PTR                 uld_control)
{
    OPS_CB	    *opscb;	/* ptr to session control block */
    OPS_STATE       *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
				    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    if (jp->opn_nleaves > 1)
    {
	TRformat(uld_tr_callback, (i4 *)0,
	    (char *)&global->ops_trace.opt_trformat[0],
	    (i4)sizeof(global->ops_trace.opt_trformat),
	    "*", NULL);
	uld_prnode(uld_control, &global->ops_trace.opt_trformat[0]);
    }
    else
    {
	OPT_NAME	tablename;
	(VOID)opt_ptname(opscb->ops_state->ops_trace.opt_subquery, 
	    jp->opn_pra[0], &tablename, TRUE); /* print table name */
	uld_prnode(uld_control, (char *)&tablename);
    }
}
#ifdef xDEBUG

/*{
** Name: opt_fjtree	- formatted join tree
**
** Description:
**      Print a formatted join tree
**
** Inputs:
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
**      3-aug-86 (seputis)
**          initial creation
**      17-Aug-2010 (horda03) b124274
**          Pass Segmented QEP flag to tree output routine
[@history_template@]...
*/
static VOID
opt_fjtree()
{
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    /* whitesmith C CMS won't accept ptr to function casts  (cfr)
    ** Neither does UNIX (ejl)
    */
    uld_prtree_x(global->ops_cb->ops_alter.ops_qep_flag,
                 (PTR)global->ops_estate.opn_sroot,/* root of Join tree to print */
	opt_fjprint,                            /* routine to print a node */
        opt_jleft,                              /* get left child */
	opt_jright,                             /* get right child */
	(i4) 5, (i4) 2,                       /* dimensions 5 wide, 2 high */
	(i4) DB_OPF_ID,
	opscb->sc930_trace);
}
#endif

/*{
** Name: opt_paop	- print the AOP nodes for subquery being processed
**
** Description:
**      Informational dump routine to print AOP nodes.  Dump names assuming
**      and 80 character display line.
**
** Inputs:
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
**      28-nov-86 (seputis)
**          initial creation
**      18-aug-88 (seputis)
**          should not send aggregate info to log file.
[@history_line@]...
[@history_template@]...
*/
static VOID
opt_paop()
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    PST_QNODE		*resdom;    /* ptr to resdom node to be printed */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */
    for( resdom = subquery->ops_root->pst_left;
	 resdom;
	 resdom = resdom->pst_left)
    {
	if (resdom->pst_sym.pst_type != PST_RESDOM
	    ||
	    resdom->pst_right->pst_sym.pst_type != PST_AOP)
	    continue;
	/* resdom on top of AOP found - so name can be printed */
	{
	    /* get the ADF name of the AOP node */
	    DB_STATUS		status;
	    ADI_OP_NAME		adi_oname;
	    status = adi_opname(global->ops_adfcb,
		resdom->pst_right->pst_sym.pst_value.pst_s_op.pst_opno, 
		&adi_oname);
	    if (status != E_AD0000_OK)
		opx_verror(E_DB_ERROR, E_OP0794_ADI_OPNAME, 
		    global->ops_adfcb->adf_errcb.ad_errcode);
	    TRformat(opt_scc, (i4 *)NULL,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		"    aggregate expression -> ");
            if ((subquery->ops_sqtype == OPS_FAGG)
		||
		(subquery->ops_sqtype == OPS_HFAGG)
		||
		(subquery->ops_sqtype == OPS_RFAGG)
	       )
		TRformat(opt_scc, (i4 *)NULL,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "T%dA%d = ",
		    subquery->ops_gentry,
		    resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno);
            TRformat(opt_scc, (i4 *)NULL,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		"%s", (char *) &adi_oname);
	    /* print attribute name if simple */
	    {
		PST_QNODE	*aggexpr;   /* ptr to aggregate expression */
		aggexpr = resdom->pst_right->pst_left;
		if (!aggexpr)
		{   /* this is probably a select count(*) aggregate which
                    ** does not have an expression */
		    opt_newline((OPS_STATE *)NULL);
		}
		else if (aggexpr->pst_sym.pst_type == PST_VAR)
		{
		    DB_ATT_NAME		attrname;
		    opt_paname(subquery,
			(OPZ_IATTS)aggexpr->pst_sym.pst_value.pst_s_var.
			pst_atno.db_att_id, (OPT_NAME *)&attrname);
		    TRformat(opt_scc, (i4 *)NULL,
			(char *)&global->ops_trace.opt_trformat[0],
			(i4)sizeof(global->ops_trace.opt_trformat),
			"(%s)\n\n", (char *) &attrname);
		}
		else
		    TRformat(opt_scc, (i4 *)NULL,
			(char *)&global->ops_trace.opt_trformat[0],
			(i4)sizeof(global->ops_trace.opt_trformat),
			"(agg expr)\n\n");
	    }
	}
    }
}

/*{
** Name: opt_pbylist	- print bylist of aggregate being processed
**
** Description:
**      Informational dump routine for function aggregate.  Should be replaced
**      by a routine which will print a textual form of the query.
**
** Inputs:
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
**      28-nov-86 (seputis)
**          initial creation
**          [@comment_text@]
[@history_line@]...
**      18-aug-88 (seputis)
**          SET QEP should not send aggregate info to log file
[@history_line@]...
[@history_template@]...
*/
static VOID
opt_pbylist()
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    PST_QNODE		*resdom;    /* ptr to resdom node to be printed */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */
    for( resdom = subquery->ops_root->pst_left;
	 resdom;
	 resdom = resdom->pst_left)
    {
	if (resdom->pst_sym.pst_type != PST_RESDOM
	    ||
	    resdom->pst_right->pst_sym.pst_type == PST_AOP)
	    continue;
	/* resdom on top of AOP found - so name can be printed */
	{
	    TRformat(opt_scc, (i4 *)NULL,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		"    by expression attribute -> T%dA%d",
		subquery->ops_gentry,
		resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno);
	    /* print attribute name if simple */
	    {
		PST_QNODE	*byexpr;   /* ptr to aggregate expression */
		byexpr = resdom->pst_right;
		if (byexpr->pst_sym.pst_type == PST_VAR)
		{
		    DB_ATT_NAME		attrname;
		    opt_paname(subquery,
			(OPZ_IATTS)byexpr->pst_sym.pst_value.pst_s_var.
			pst_atno.db_att_id, (OPT_NAME *)&attrname);
		    TRformat(opt_scc, (i4 *)NULL,
			(char *)&global->ops_trace.opt_trformat[0],
			(i4)sizeof(global->ops_trace.opt_trformat),
			" = %s\n\n", (char *) &attrname);
		}
		else
		    TRformat(opt_scc, (i4 *)NULL,
			(char *)&global->ops_trace.opt_trformat[0],
			(i4)sizeof(global->ops_trace.opt_trformat),
			"\n");
	    }
	}
    }
}

/*{
** Name: opt_sdump	- dump site cost array
**
** Description:
**      Routine which will print site cost array information for distributed 
**      optimization 
[@comment_line@]...
**
** Inputs:
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
**      16-aug-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
static VOID
opt_sdump()
{
    OPD_ISITE		site;
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPD_ISITE		maxsite;

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */

    TRformat(opt_scc, (i4 *)NULL,
	(char *)&global->ops_trace.opt_trformat[0],
	(i4)sizeof(global->ops_trace.opt_trformat),
	 "Distributed Site information\n\n" );
    maxsite = global->ops_gdist.opd_dv;
    for (site = 0; site < maxsite; site++)
    {
	char	    nodename[DB_NODE_MAXNAME+1];
	char	    ldbname[DB_DB_MAXNAME+1];

	MEcopy((PTR)global->ops_gdist.opd_base->opd_dtable[site]->
		opd_ldbdesc->dd_l2_node_name,
	    DB_NODE_MAXNAME, (PTR)&nodename[0]);
	nodename[DB_NODE_MAXNAME] = 0;
	(VOID)STtrmwhite(&nodename[0]);
	MEcopy((PTR)global->ops_gdist.opd_base->opd_dtable[site]->
		opd_ldbdesc->dd_l3_ldb_name,
	    DB_DB_MAXNAME, (PTR)&ldbname[0]);
	ldbname[DB_DB_MAXNAME] = 0;
	(VOID)STtrmwhite(&ldbname[0]);
	TRformat(opt_scc, (i4 *)NULL,
	    (char *)&global->ops_trace.opt_trformat[0],
	    (i4)sizeof(global->ops_trace.opt_trformat),
	     "    Site %d has node name= %s, LDB name= %s", 
		site, 
		&nodename[0],
		&ldbname[0]);
	opt_newline((OPS_STATE *)NULL);
    }
#ifdef    xDEV_TEST
    for (site = 0; site < maxsite; site++)
    {
	OPD_SCOST   *factorp;
	OPD_ISITE   site1;

	TRformat(opt_scc, (i4 *)NULL,
	    (char *)&global->ops_trace.opt_trformat[0],
	    (i4)sizeof(global->ops_trace.opt_trformat),
	     "    Site %d cost array = ", site);
	factorp = global->ops_gdist.opd_base->opd_dtable[site]->opd_factor;
	for (site1 = 0; site1<maxsite; site1++)
	{
	    TRformat(opt_scc, (i4 *)NULL,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		 " %12.6e",	factorp->opd_intcosts[site1]);
	}
	opt_newline((OPS_STATE *)NULL);
    }
#endif
}

/*{
** Name: opt_printPlanType - print a line describing the type of QEP
**
** Description:
**	This routine prints out the type of the QEP for a SET QEP or OP199
**	output.
**
** Inputs:
**      global		ptr to global state variable
**	withStats	print with or without stats (TRUE or FALSE)
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
**	14-may-92 (rickh)
**	    Extracted from opt_cotree.  Souped it up to print or not print
**	    statistics.
**	1-dec-92 (ed)
**	    increase buffer size to accommodate new options
**	01-mar-10 (smeke01) b123333
**	    Add handling of plan fragment display for trace point op188.
[@history_line@]...
*/
static VOID
opt_printPlanType(
	OPS_STATE	*global,
	bool		withStats,
	char		*trailer,
	u_i4		currfrag,
	u_i4		bestfrag,
	i4		planid,
	i4		queryid,
	char		*timeout,
	char		*largetemp,
	char		*floatexp)
{
    char	buffer[256];
    char	*leader1 = "QUERY PLAN ";
    char	*leader2 = "PLAN FRAGMENT ";
    char	*format1 = "%d,%d%s%s%s";
    char	*format2 = "%d for SUBQUERY %d%s%s" ;

    if ( withStats == TRUE )
    {
	if (global->ops_trace.opt_conode == NULL )
	{
	    STpolycat( 3, leader1, format1, trailer, buffer );
	    TRformat(opt_scc, (i4 *)NULL,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		buffer,
		planid, queryid, timeout, largetemp, floatexp);
	}
	else
	{
	    STpolycat( 3, leader2, format2, trailer, buffer );
	    TRformat(opt_scc, (i4 *)NULL,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		buffer,
		currfrag, queryid, largetemp, floatexp);
	}
    }
    else
    {
	STpolycat( 2, leader1, trailer, buffer );
	TRformat(opt_scc, (i4 *)NULL,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		buffer );
    }
}

/*{
** Name: opt_printCostTree - print the CO tree with or without cost statistics
**
** Description:
**	This routine prints out a cost tree.  If so instructed, it will also
**	print out the statistics in tree.
**
** Inputs:
**      global		ptr to global state variable
**	withStats	print with or without stats (TRUE or FALSE)
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
**	14-may-92 (rickh)
**	    Extracted from opt_cotree.  Souped it up to print or not print
**	    statistics.
**      21-may-92 (seputis)
**          xDEBUG trace point op168 since it does not work in all cases
**	22-sep-00 (inkdo01)
**	    Add support of except, intersect set operators.
**	23-jan-01 (inkdo01)
**	    Tiny fix to support hash aggregation.
**	14-Mar-2007 (kschendel) SIR 122513
**	    Print PC-aggregation indicator.
**	13-Oct-2008 (kschendel)
**	    PC-agg gcount is an estimate only, improve heading.
**	01-Mar-2010 (smeke01) b123333
**	    Moved NULL-ing of ops_trace.opt_conode, to enable trace point
**	    op188 to call opt_cotree() just as trace point op145 (set qep) 
**	    does, so that we get similar output from both. Added in display 
**	    of current and best fragment numbers and 'Best QUERY PLAN so far'. 
**      17-Aug-2010 (horda03) b124274
**          Pass Segmented QEP flag to tree output routine
*/
VOID
opt_printCostTree(
	OPO_CO		*cop,
	bool		withStats)
{

    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */
    VOID		(*printNode)( );	/* ptr to routine that prints
						** an individual node */
    bool		bPlanFragment = FALSE;

    if ( withStats )
    {
	printNode = opt_coprint;
    }
    else
    {
	printNode = opt_coprintWithoutStats;
    }

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */

    if (global->ops_trace.opt_conode)
	bPlanFragment = TRUE;

    {
	/* Display CPU time expended to date. */
	i4	nowcpu;
	TIMERSTAT	timeblock;
	STATUS		cs_stat;
	f4		floatcpu;

	cs_stat = CSstatistics(&timeblock, (i4)0);
	if (FALSE && cs_stat == E_DB_OK)
	{
	    nowcpu = timeblock.stat_cpu - global->ops_estate.opn_startime;
	    floatcpu = (f4)nowcpu / 1000;	/* make it seconds */
	    opt_newline((OPS_STATE *)NULL);
	    TRformat(opt_scc, (i4 *)NULL,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "CPU time so far: %10.3f seconds\n\n", floatcpu);
	}
    }
    if (cop)
    {
	/* the purpose of this code is to print more identifying info
        ** on the subqueries so the user can understand the decomposition
        ** of the query */
	i4	    queryid;
	i4	    planid;
	u_i4	    currfrag;
	u_i4	    bestfrag;
	char	    *timeout;	/* ptr to timeout characteristic */
	char	    *floatexp;	/* ptr to float exception string */
	char	    *largetemp; /* ptr to large temp string */
	OPV_GRV	    *grv;

	queryid = subquery->ops_tsubquery;  /* unique ID for subquery */
	planid = subquery->ops_tplan;	/* unique plan ID with subquery */
	currfrag = subquery->ops_currfragment; /* # of the current fragment */
	bestfrag = subquery->ops_bestfragment; /* # of the best fragment (so far) */

	if (subquery->ops_timeout)
	    timeout = ", timed out,";
	else
	    timeout = ", no timeout,";
	if (subquery->ops_mask & OPS_TMAX)
	    largetemp = ", large temporaries,";
	else
	    largetemp = "";
	if (subquery->ops_mask & OPS_SFPEXCEPTION)
	    floatexp = ", float/integer exceptions,";
	else
	    floatexp = "";
	opt_newline((OPS_STATE *)NULL);		/* separate QEPs with a new line */
        switch (subquery->ops_sqtype)
	{
	case OPS_MAIN:
	    opt_printPlanType( global, withStats, " of main query\n\n",
	     (u_i4)currfrag, (u_i4)bestfrag,  (i4)planid, (i4)queryid, (char *)timeout,
	     (char *)largetemp, (char *)floatexp);

	    if (bPlanFragment)
	    {
		if (planid > 0)
		    TRformat(opt_scc, (i4 *)NULL,
			(char *)&global->ops_trace.opt_trformat[0],
			(i4)sizeof(global->ops_trace.opt_trformat),
			"    Best QUERY PLAN so far is (%d, %d) using PLAN FRAGMENT %d\n\n",
			planid, queryid, bestfrag);
		else
		    TRformat(opt_scc, (i4 *)NULL,
			(char *)&global->ops_trace.opt_trformat[0],
			(i4)sizeof(global->ops_trace.opt_trformat),
			"    No best QUERY PLAN so far\n\n");
	    }
	    break;
	case OPS_RSAGG:
	case OPS_SAGG:
	    opt_printPlanType( global, withStats, " of simple aggregate\n\n",
	     (u_i4)currfrag, (u_i4)bestfrag,  (i4)planid, (i4)queryid, (char *)timeout,
	     (char *)largetemp, (char *)floatexp);

	    opt_paop();			/* print the AOP nodes of the
					** subquery being processed */
	    if (bPlanFragment)
	    {
		if (planid > 0)
		    TRformat(opt_scc, (i4 *)NULL,
			(char *)&global->ops_trace.opt_trformat[0],
			(i4)sizeof(global->ops_trace.opt_trformat),
			"    Best QUERY PLAN so far is (%d, %d) using PLAN FRAGMENT %d\n\n",
			planid, queryid, bestfrag);
		else
		    TRformat(opt_scc, (i4 *)NULL,
			(char *)&global->ops_trace.opt_trformat[0],
			(i4)sizeof(global->ops_trace.opt_trformat),
			"    No best QUERY PLAN so far\n\n");
	    }
	    else
	    opt_newline((OPS_STATE *)NULL);

	    break;
	case OPS_HFAGG:
	case OPS_RFAGG:
	case OPS_FAGG:
	    opt_printPlanType( global, withStats,
	     (subquery->ops_sqtype == OPS_HFAGG) ? 
	     " of hash func aggregate " : " of function aggregate ",
	     (u_i4)currfrag, (u_i4)bestfrag,  (i4)planid, (i4)queryid, (char *)timeout,
	     (char *)largetemp, (char *)floatexp );

	    TRformat(opt_scc, (i4 *)NULL,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "producing temporary table T%d\n\n",
		    (i4)subquery->ops_gentry);

	    if (bPlanFragment)
	    {
		if (planid > 0)
		    TRformat(opt_scc, (i4 *)NULL,
			(char *)&global->ops_trace.opt_trformat[0],
			(i4)sizeof(global->ops_trace.opt_trformat),
			"    Best QUERY PLAN so far is (%d, %d) using PLAN FRAGMENT %d\n\n",
			planid, queryid, bestfrag);
		else
		    TRformat(opt_scc, (i4 *)NULL,
			(char *)&global->ops_trace.opt_trformat[0],
			(i4)sizeof(global->ops_trace.opt_trformat),
			"    No best QUERY PLAN so far\n\n");
	    }

	    if (!bPlanFragment && subquery->ops_mask & OPS_PCAGG)
	    {
		TRformat(opt_scc, (i4 *)NULL,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "    partition compatible (est %d groups)\n\n",
		    subquery->ops_gcount);
	    }

	    if (!bPlanFragment && subquery->ops_gentry != OPV_NOGVAR)
	    {
		grv = global->ops_rangetab.opv_base->opv_grv[subquery->ops_gentry];
		TRformat(opt_scc, (i4 *)NULL,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "    estimated %d tups, %d pages\n\n",
		    (i4)grv->opv_gcost.opo_tups, (i4)grv->opv_gcost.opo_reltotb);
	    }

	    opt_paop();			/* print the AOP nodes of the
					** subquery being processed */
	    opt_pbylist();		/* print the bylist of the subquery
                                        ** being processed */
	    opt_newline((OPS_STATE *)NULL);
	    break;
	case OPS_PROJECTION:
	    opt_printPlanType( global, withStats,
	     " of projection for function aggregate ",
	     (u_i4)currfrag, (u_i4)bestfrag,  (i4)planid, (i4)queryid, (char *)timeout,
	     (char *)largetemp, (char *)floatexp );

	    TRformat(opt_scc, (i4 *)NULL,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "T%d\n\n",
                     (i4)subquery->ops_next->ops_gentry);
					/* assume projection
					** immediately preceeds the respective
					** function attribute */

            TRformat(opt_scc, (i4 *)NULL,
                (char *)&global->ops_trace.opt_trformat[0],
                (i4)sizeof(global->ops_trace.opt_trformat),
                 "    producing temporary table T%d\n\n",
                (i4)subquery->ops_gentry);

	    if (bPlanFragment)
	    {
		if (planid > 0)
		    TRformat(opt_scc, (i4 *)NULL,
			(char *)&global->ops_trace.opt_trformat[0],
			(i4)sizeof(global->ops_trace.opt_trformat),
			"    Best QUERY PLAN so far is (%d, %d) using PLAN FRAGMENT %d\n\n",
			planid, queryid, bestfrag);
		else
		    TRformat(opt_scc, (i4 *)NULL,
			(char *)&global->ops_trace.opt_trformat[0],
			(i4)sizeof(global->ops_trace.opt_trformat),
			"    No best QUERY PLAN so far\n\n");
	    }

	    break;
	case OPS_SELECT:
	    opt_printPlanType( global, withStats,
	     " of subselect ",
	     (u_i4)currfrag, (u_i4)bestfrag,  (i4)planid, (i4)queryid, (char *)timeout,
	     (char *)largetemp, (char *)floatexp );

	    TRformat(opt_scc, (i4 *)NULL,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "T%d\n\n",
                     (i4)subquery->ops_gentry);
				/* subselect temporary is
                                ** not created but global range variable
                                ** is reserved for reference */

	    if (bPlanFragment)
	    {
		if (planid > 0)
		    TRformat(opt_scc, (i4 *)NULL,
			(char *)&global->ops_trace.opt_trformat[0],
			(i4)sizeof(global->ops_trace.opt_trformat),
			"    Best QUERY PLAN so far is (%d, %d) using PLAN FRAGMENT %d\n\n",
			planid, queryid, bestfrag);
		else
		    TRformat(opt_scc, (i4 *)NULL,
			(char *)&global->ops_trace.opt_trformat[0],
			(i4)sizeof(global->ops_trace.opt_trformat),
			"    No best QUERY PLAN so far\n\n");
	    }

	    break;
	case OPS_VIEW:
	    if (!subquery->ops_union
		&&
		!(global->ops_gmask & OPS_UV))
	    {
	        opt_printPlanType( global, withStats,
	         " of union view ",
	         (u_i4)currfrag, (u_i4)bestfrag,  (i4)planid, (i4)queryid, (char *)timeout,
	         (char *)largetemp, (char *)floatexp );

	        TRformat(opt_scc, (i4 *)NULL,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "T%d\n\n",
		    (i4)subquery->ops_gentry);

		if (bPlanFragment)
		{
		    if (planid > 0)
			TRformat(opt_scc, (i4 *)NULL,
			    (char *)&global->ops_trace.opt_trformat[0],
			    (i4)sizeof(global->ops_trace.opt_trformat),
			    "    Best QUERY PLAN so far is (%d, %d) using PLAN FRAGMENT %d\n\n",
			    planid, queryid, bestfrag);
		    else
			TRformat(opt_scc, (i4 *)NULL,
			    (char *)&global->ops_trace.opt_trformat[0],
			    (i4)sizeof(global->ops_trace.opt_trformat),
			    "    No best QUERY PLAN so far\n\n");
		}

		break;
	    }
	case OPS_UNION:
	    if (subquery->ops_union)
	    {
		opt_printPlanType( global, withStats,
		 " of union subquery\n\n" ,
		 (u_i4)currfrag, (u_i4)bestfrag,  (i4)planid, (i4)queryid, (char *)timeout,
		 (char *)largetemp, (char *)floatexp );
	    }
	    else
	    {
		opt_printPlanType( global, withStats,
		 " of union ",
		 (u_i4)currfrag, (u_i4)bestfrag,  (i4)planid, (i4)queryid, (char *)timeout,
		 (char *)largetemp, (char *)floatexp );

		TRformat(opt_scc, (i4 *)NULL,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		    "T%d\n\n",
		    (i4)subquery->ops_gentry);
	    }

	    if (bPlanFragment)
	    {
		if (planid > 0)
		    TRformat(opt_scc, (i4 *)NULL,
			(char *)&global->ops_trace.opt_trformat[0],
			(i4)sizeof(global->ops_trace.opt_trformat),
			"    Best QUERY PLAN so far is (%d, %d) using PLAN FRAGMENT %d\n\n",
			planid, queryid, bestfrag);
		else
		    TRformat(opt_scc, (i4 *)NULL,
			(char *)&global->ops_trace.opt_trformat[0],
			(i4)sizeof(global->ops_trace.opt_trformat),
			"    No best QUERY PLAN so far\n\n");
	    }
	    break;

	default:
	    opx_error(E_OP048D_QUERYTYPE);
	}
    }
    /* whitesmith C CMS won't accept ptr to function casts  (cfr)
    ** Neither does UNIX (ejl)
    */
    uld_prtree_x(global->ops_cb->ops_alter.ops_qep_flag,
        (PTR)cop,				/* root of CO tree to print */
	printNode,                              /* routine to print a node */
        opt_coleft,                             /* get left child */
	opt_coright,                            /* get right child */
	(i4) 12, (i4) 4,                      /* dimensions 12 wide, 8 high */
        (i4) DB_OPF_ID,
	opscb->sc930_trace);
#ifdef xDEBUG
    if (global->ops_cb->ops_check 
	&& 
	opt_strace( global->ops_cb, OPT_F040_SQL_TEXT)
	)
	opt_cdump (cop);		    /* print more diagnostic info */
#endif
    if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
    {
        if (subquery->ops_sqtype == OPS_MAIN)
	    opt_sdump ();		/* print site array information */
    }
}

/*{
** Name: opt_cotree_without_stats - print the CO tree without cost statistics
**
** Description:
**	This routine calls the extracted guts of opt_cotree to print out a
**	cost tree without statistics.  This new routine is called when OP199
**	is set.  The point is to write regression tests that can verify
**	QEN tree shapes without vomiting up pages of difs whenever the stats
**	change slightly.  This routine is used with "set qep," which must
**	work even if tracing is compiled out.
**
** Inputs:
**      global
**	    ptr to global state variable
**
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
**	14-may-92 (rickh)
**	    Created.
[@history_line@]...
*/
VOID
opt_cotree_without_stats(
	OPS_STATE          	*global)
{
    OPS_SUBQUERY	*next_subqry;
    char		temp[OPT_PBLEN + 1];
    bool		init = 0;

    if (global->ops_statement != NULL)
    {
        if (global->ops_cstate.opc_prbuf == NULL)
        {
	    global->ops_cstate.opc_prbuf = temp;
	    init++;
        }
    
        for (next_subqry = global->ops_osubquery;
	     next_subqry != NULL;
	     next_subqry = next_subqry->ops_onext
	    )
        {
            if (next_subqry->ops_sunion.opu_mask & OPU_NOROWS) continue;
            if (next_subqry->ops_bestco != NULL)
	    {
	        TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
		        OPT_PBLEN, "\n*** The QUERY Tree Minus Statistics ***\n\n");
                global->ops_trace.opt_subquery = next_subqry;
	        opt_printCostTree(next_subqry->ops_bestco, (bool)FALSE);
	    }
        }

        if (init)
        {
	    global->ops_cstate.opc_prbuf = NULL;
        }
    }	/* endif ops_statement is filled in */
}

/*{
** Name: opt_cotree	- print the CO tree
**
** Description:
**	This routine is used with "set qep," which must work even if tracing is 
**      compiled out.
**
** Inputs:
**      opt_subquery		        ptr to subquery associated with CO tree
**                                      must be set in the global state variable
**                                      in order to define the joinop tables
**                                      associated with this cop tree
**      cop                             ptr to root of CO tree to be printed
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
**	18-jul-86 (seputis)
**          initial creation
**	18-apr-91 (seputis)
**	    print exceptions on a subquery basis
[@history_line@]...
*/
VOID
opt_cotree(
	OPO_CO             *cop)
{
    opt_printCostTree( cop, (bool) TRUE );
}
#ifdef xDEBUG

/*{
** Name: opt_bco	- print the best CO tree
**
** Description:
**      Print the best co tree found so far.
**
** Inputs:
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
**      23-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_bco()
{
    OPS_SUBQUERY        *subquery;  /* ptr to subquery being traced */
    OPS_CB              *opscb;	    /* ptr to session control block */
    OPS_STATE           *global;    /* ptr to global state variable */

    opscb = ops_getcb();            /* get the session control block
                                    ** from the SCF */
    global = opscb->ops_state;      /* ptr to global state variable */
    subquery = global->ops_trace.opt_subquery; /* get ptr to current subquery
                                    ** which is being traced */
    opt_cotree(subquery->ops_bestco); /* print the best CO tree found so far */
}
#endif

/*{
** Name: opt_pt_sq	- print subquery and contained parse tree
**			(inkdo01 style)
**
** Description:
**      Routine will print a subquery structure (incl contained parse tree)
[@comment_line@]...
**
** Inputs:
**      root                            ptr to root of query tree
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
**      19-feb-03 (inkdo01)
**          initial creation
**	15-Dec-2004 (schka24)
**	    Subquery can appear in multiple concurrent/next links,
**	    only print each one once.  Fix C-precedence typo.
[@history_template@]...
*/
static VOID
opt_pt_sq(global, header, subqry, nest, already_done)
OPS_STATE          *global;
PST_QTREE          *header;
OPS_SUBQUERY	   *subqry;
i4		   *nest;
OPS_SUBQUERY	    **already_done;
{
    int i;

    for (; subqry != NULL; subqry = subqry->ops_next)
    {
	/* See if we printed this one already, don't repeat it */
	for (i=0; i<MAX_ALREADY_DONE; ++i)
	    if (already_done[i] == NULL || subqry == already_done[i])
		break;
	if (i < MAX_ALREADY_DONE)
	{
	    if (already_done[i] != NULL)
	    {
		/* Did it already */
		TRformat(opt_scc, NULL, global->ops_cstate.opc_prbuf,
		    OPT_PBLEN, "\nSubquery: 0x%p  was already dumped\n\n", subqry);
		continue;
	    }
	    already_done[i] = subqry;
	}
        TRformat(opt_scc, (i4 *)NULL, global->ops_cstate.opc_prbuf,
            OPT_PBLEN, "\nSubquery: 0x%p\n\n", subqry);
        TRformat(opt_scc, (i4 *)NULL, global->ops_cstate.opc_prbuf,
          OPT_PBLEN, "  ops_root: 0x%p   ops_next: 0x%p   ops_union: 0x%p\n\n",
               subqry->ops_root, subqry->ops_next, subqry->ops_union);
        TRformat(opt_scc, (i4 *)NULL, global->ops_cstate.opc_prbuf,
          OPT_PBLEN, "  ops_sqtype: %d   ops_gentry: %d   ops_mode: %d   ops_byexpr: 0x%p\n\n",   
               subqry->ops_sqtype, subqry->ops_gentry, subqry->ops_mode,
		subqry->ops_byexpr);
        TRformat(opt_scc, (i4 *)NULL, global->ops_cstate.opc_prbuf,
          OPT_PBLEN, "  ops_duplicates: %d   ops_mask: 0x%x   ops_mask2: 0x%x   ops_onext: 0x%p\n\n",
               subqry->ops_duplicates, subqry->ops_mask, subqry->ops_mask2, subqry->ops_onext);
	TRformat(opt_scc, (i4 *)NULL, global->ops_cstate.opc_prbuf,
	  OPT_PBLEN, "  ops_agg.opa_concurrent: 0x%p, .opa_graft: 0x%p->0x%p, .opa_father: 0x%p\n\n",
		subqry->ops_agg.opa_concurrent, subqry->ops_agg.opa_graft,
		subqry->ops_agg.opa_graft&&*subqry->ops_agg.opa_graft?*subqry->ops_agg.opa_graft:NULL,
		subqry->ops_agg.opa_father);
        if (header != (PST_QTREE *) NULL) opt_pt_hddump(header,FALSE);

        opt_pt_prmdump(subqry->ops_root, 1);
        header = NULL;      /* only dump header once */

	if (subqry->ops_agg.opa_concurrent && *nest < 10)
	{
            TRformat(opt_scc, (i4 *)NULL, global->ops_cstate.opc_prbuf,
        	OPT_PBLEN, "\nConcurrent Subquery: \n\n");
	    (*nest)++;
	    opt_pt_sq(global, header, subqry->ops_agg.opa_concurrent, nest,
			already_done);
				/* recurse on concurrent ones */
	    (*nest)--;
	}
    }
}
/*{
** Name: opt_pt_entry- print query tree (inkdo01 style)
**
** Description:
**      Routine will print a query tree given the root 
[@comment_line@]...
**
** Inputs:
**      root                            ptr to root of query tree
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
**      21-mar-95 (inkdo01)
**          initial creation
**	19-feb-03 (inkdo01)
**	    Change to recurse on ops_agg.opa_concurrent.
**	18-Mar-2010 (kiria01) b123438
**	    Added the missing output of pst_ttargtype from op170
**	    and support pst_ss_joinid
[@history_template@]...
*/
VOID
opt_pt_entry(global, header, root, origin)
OPS_STATE          *global;
PST_QTREE          *header;
PST_QNODE          *root;
char               *origin;
{
    OPS_SUBQUERY*       subqry;
    DB_STATUS           status;
    i4			nest = 0;
    DB_ERROR            error;
    char                temp[OPT_PBLEN+1];
    bool                init = 0;
    OPS_SUBQUERY	*already_done[MAX_ALREADY_DONE];

    if (!global->ops_cstate.opc_prbuf)
    {
       global->ops_cstate.opc_prbuf = temp;
       init++;
    }
    if (global->ops_subquery == NULL)
    {
       TRformat(opt_scc, (i4 *)NULL, global->ops_cstate.opc_prbuf,
           OPT_PBLEN, "\n\nParse tree dumped %s:\n\n", origin);
       if (init) global->ops_cstate.opc_prbuf = NULL;
       /* Show view trees when we puke things out the first time */
       if (header != (PST_QTREE *) NULL) opt_pt_hddump(header,TRUE);

       opt_pt_prmdump(root, 1);
       return;
    }

    TRformat(opt_scc, (i4 *)NULL, global->ops_cstate.opc_prbuf,
       OPT_PBLEN, "\n\nParse tree dumped %s:\n\n", origin);

    MEfill(sizeof(already_done), 0, (PTR) &already_done);
    opt_pt_sq(global, header, global->ops_subquery, &nest, &already_done[0]);

    if (init) global->ops_cstate.opc_prbuf = NULL;
}

/*{
** Name: opt_pt_prmdump    - dump query tree a la Doug.         
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
*/
static VOID opt_pt_prmdump(
	     PST_QNODE	   *qnode,
	     i2             indent)
{
   char      indentstr[256];           /* indentation string */
   char      temp[OPT_PBLEN+1];

   MEfill(indent, ' ', (PTR)indentstr);
   indentstr[indent] = '\0';           /* indent by 'n' blanks */

   /* Following loop continues until union'd selects are done */

   do {
      /* start printing - first the background, shared info.      */

      TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	 "\n%s===========================================\n", indentstr);
      TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	 "\n%s%s: 0x%p\n", indentstr, ntype_array[qnode->pst_sym.pst_type],
	 qnode);                       /* node type and addr   */
      TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	 "\n%spst_left: 0x%p (%s)\n",indentstr,qnode->pst_left, 
         (qnode->pst_left) ? ntype_array[qnode->pst_left->pst_sym.pst_type] :
         "nil");
      TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	 "\n%spst_right: 0x%p (%s)\n",indentstr,qnode->pst_right, 
         (qnode->pst_right) ? ntype_array[qnode->pst_right->pst_sym.pst_type] :
         "nil");
				       /* left and right subaddrs */
      TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_dataval: <%d,%d,%d,%d,%d>\n",indentstr,
	qnode->pst_sym.pst_dataval.db_datatype,
        qnode->pst_sym.pst_dataval.db_length,
        qnode->pst_sym.pst_dataval.db_prec/256,
	qnode->pst_sym.pst_dataval.db_prec-
            qnode->pst_sym.pst_dataval.db_prec/256*256,
        qnode->pst_sym.pst_dataval.db_collID);
      if (qnode->pst_sym.pst_dataval.db_data != NULL)
	 TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	     "\n%spst_dataval: addr 0x%p,value 0x%x\n",
	     indentstr, qnode->pst_sym.pst_dataval.db_data,
             *((int *)qnode->pst_sym.pst_dataval.db_data));
				       /* if there's real data ... */
      TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_len: %d\n", indentstr, qnode->pst_sym.pst_len);

      /* now switch to call node specific routines to handle details */

      switch (qnode->pst_sym.pst_type) {

       case PST_AND:
       case PST_OR:
       case PST_UOP:
       case PST_BOP:
       case PST_AOP:
       case PST_COP:
       case PST_MOP:
	 opt_pt_op_node(&qnode->pst_sym.pst_value.pst_s_op,indentstr,temp);
	 break;

       case PST_CONST:
	 opt_pt_const_node(&qnode->pst_sym.pst_value.pst_s_cnst,indentstr,temp);
	 break;

       case PST_RESDOM:
       case PST_BYHEAD:
	 opt_pt_resdom_node(&qnode->pst_sym.pst_value.pst_s_rsdm,indentstr,temp);
	 break;

       case PST_VAR:
	 opt_pt_var_node(&qnode->pst_sym.pst_value.pst_s_var,indentstr,temp);
	 break;

       case PST_FWDVAR:
	 opt_pt_fwdvar_node(&qnode->pst_sym.pst_value.pst_s_fwdvar,indentstr,temp);
	 break;

       case PST_ROOT:
       case PST_AGHEAD:
       case PST_SUBSEL:
	 opt_pt_root_node(&qnode->pst_sym.pst_value.pst_s_root,indentstr,temp);
	 break;

       case PST_SORT:
	 opt_pt_sort_node(&qnode->pst_sym.pst_value.pst_s_sort,indentstr,temp);
	 break;

       case PST_TAB:
	 opt_pt_tab_node(&qnode->pst_sym.pst_value.pst_s_tab,indentstr,temp);
	 break;

       case PST_CURVAL:
	 opt_pt_curval_node(&qnode->pst_sym.pst_value.pst_s_curval,indentstr,temp);
	 break;

       case PST_RULEVAR:
	 opt_pt_rulevar_node(&qnode->pst_sym.pst_value.pst_s_rule,indentstr,temp);
	 break;

       case PST_CASEOP:
	 opt_pt_case_node(&qnode->pst_sym.pst_value.pst_s_case,indentstr,temp);
	 break;

       case PST_SEQOP:
	 opt_pt_seqop_node(&qnode->pst_sym.pst_value.pst_s_seqop, indentstr, temp);
	 break;

       case PST_GBCR:
	 opt_pt_gbcr_node(&qnode->pst_sym.pst_value.pst_s_group, indentstr, temp);
	 break;

       case PST_WHLIST:
       case PST_WHOP:
       case PST_QLEND:
       case PST_TREE:
       case PST_GSET:
       case PST_GCL:
       case PST_NOT:
       case PST_OPERAND:
	 break;
       default:
	 opx_error(E_OP0681_UNKNOWN_NODE);
       }  /* end of pst_type switch */

      /* Finished with this node - now descend the left & right subtrees */

      if (qnode->pst_left) opt_pt_prmdump(qnode->pst_left, indent+1);
      if (qnode->pst_right) opt_pt_prmdump(qnode->pst_right, indent+1);

      /* Finally, if this is a ROOT, check for unions and iterate */

      if (qnode->pst_sym.pst_type == PST_ROOT && qnode->pst_sym.pst_value.
	    pst_s_root.pst_union.pst_next)
	 {
         TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	      "\n\n%s===========================================\n", 
              indentstr);
	 TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
              "\n UNION %s \n\n", (qnode->pst_sym.pst_value.
	      pst_s_root.pst_union.pst_dups == PST_ALLDUPS) ? "ALL" : " ");
	 qnode = qnode->pst_sym.pst_value.pst_s_root.pst_union.pst_next;
	 }
       else break;

      } while(TRUE);         /* stays in big loop for all the unions */

   }                   /* end of opt_pt_prmdump */


static VOID
opt_pt_var_node(PST_VAR_NODE    *vnode,
	     char            *indentstr,
             char            *temp)
{
   /* Just print the VAR node fields */
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_vno: %d\n", indentstr, vnode->pst_vno);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_atno: (db_att_id) %d  pst_atname: %32s\n", indentstr,
	vnode->pst_atno.db_att_id, &vnode->pst_atname);
}    /* end of opt_pt_var_node */

static VOID
opt_pt_fwdvar_node(PST_FWDVAR_NODE    *vnode,
	     char            *indentstr,
             char            *temp)
{
   /* Just print the FWDVAR node fields */
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_vno: %d\n", indentstr, vnode->pst_vno);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_atno: %d\n", indentstr, vnode->pst_atno.db_att_id);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_atname: %32s\n", indentstr, &vnode->pst_atname);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_fwdref: 0x%x\n",indentstr, vnode->pst_fwdref);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_rngvar: 0x%x\n",indentstr, vnode->pst_rngvar);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_star_seen: 0x%x\n",indentstr, vnode->pst_star_seen);
}    /* end of opt_pt_fwdvar_node */


static VOID
opt_pt_case_node(PST_CASE_NODE    *vnode,
	     char            *indentstr,
             char            *temp)
{
   /* Just print the CASE node fields */
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_srceval: <%d,%d,%d,%d>\n",indentstr,
	vnode->pst_casedt,
	vnode->pst_caselen,
	vnode->pst_caseprec/256,
	vnode->pst_caseprec-
	    vnode->pst_caseprec/256*256);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_casetype: %d\n", indentstr, vnode->pst_casetype);
}    /* end of opt_pt_case_node */


static VOID
opt_pt_seqop_node(PST_SEQOP_NODE    *snode,
	     char            *indentstr,
             char            *temp)
{
   /* Just print the SEQOP node fields */
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_owner: %32s\n", indentstr, &snode->pst_owner);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_seqname: %32s\n", indentstr, &snode->pst_seqname);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_seqid: (0x%x, 0x%x), pst_seqflag: %d\n", indentstr, 
	snode->pst_seqid.db_tab_base, snode->pst_seqid.db_tab_index,
	snode->pst_seqflag);
}    /* end of opt_pt_var_node */


static VOID
opt_pt_gbcr_node(PST_GROUP_NODE    *gnode,
	     char            *indentstr,
             char            *temp)
{
   /* Just print the GBCR node field */
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_groupmask: %d\n", indentstr, gnode->pst_groupmask);
}    /* end of opt_pt_gbcr_node */


static VOID
opt_pt_curval_node(PST_CRVAL_NODE *cnode,
		char            *indentstr,
                char            *temp)
{
   /* Just print the CURVAL node fields */
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_cursor: %d, %d - %32s\n", indentstr,
       cnode->pst_cursor.db_cursor_id[0],
       cnode->pst_cursor.db_cursor_id[1],
       cnode->pst_cursor.db_cur_name);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_curcol: %d\n", indentstr, cnode->pst_curcol);
}   /* end of opt_pt_curval_node */

static VOID
opt_pt_root_node(PST_RT_NODE    *rnode,
	      char             *indentstr,
              char            *temp)
{
   /* Just print the ROOT node fields */
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_tvrc: %d\n",indentstr,rnode->pst_tvrc);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_tvrm: 0x%x, 0x%x, 0x%x, 0x%x\n",indentstr,
	rnode->pst_tvrm.pst_j_mask[0], rnode->pst_tvrm.pst_j_mask[1],
	rnode->pst_tvrm.pst_j_mask[2], rnode->pst_tvrm.pst_j_mask[3]);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_lvrc: %d\n",indentstr,rnode->pst_lvrc);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_lvrm: 0x%x, 0x%x, 0x%x, 0x%x\n",indentstr,
	rnode->pst_lvrm.pst_j_mask[0], rnode->pst_lvrm.pst_j_mask[1],
	rnode->pst_lvrm.pst_j_mask[2], rnode->pst_lvrm.pst_j_mask[3]);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_mask1: 0x%x\n",indentstr,rnode->pst_mask1);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_rvrm: 0x%x, 0x%x, 0x%x, 0x%x\n",indentstr,
	rnode->pst_rvrm.pst_j_mask[0], rnode->pst_rvrm.pst_j_mask[1],
	rnode->pst_rvrm.pst_j_mask[2], rnode->pst_rvrm.pst_j_mask[3]);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_qlang: %d\n",indentstr,rnode->pst_qlang);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_rtuser: %d\n",indentstr,rnode->pst_rtuser);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_project: %d\n",indentstr,rnode->pst_project);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_dups: %d pst_ss_joinid: %d\n",
	indentstr, (i4)rnode->pst_dups, (i4)rnode->pst_ss_joinid);
   if (rnode->pst_union.pst_next)
   {
     TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_union.pst_dups: %d .pst_setop: %d .pst_nest: %d .pst_next: 0x%p\n",
	indentstr,(i4)rnode->pst_union.pst_dups, (i4)rnode->pst_union.pst_setop,
	(i4)rnode->pst_union.pst_nest, rnode->pst_union.pst_next);
   }
}   /* end of opt_pt_root_node */

static VOID
opt_pt_op_node(PST_OP_NODE      *onode,
	    char             *indentstr,
             char            *temp)
{
   char buf[DB_MAXNAME + 3]; /* Same size used in pst_1fidop() */

   /* Just print the OP node fields */
   opt_1fidop(onode->pst_opno, buf);
   buf[DB_MAXNAME + 1] = '\0';
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_opno: %d (%s)\n",indentstr,onode->pst_opno, buf);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_opinst: %d\n",indentstr,onode->pst_opinst);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_distinct: %d   pst_retnull: %d\n",indentstr,
	onode->pst_distinct, onode->pst_retnull);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_oplcnvrtid: %d   pst_oprcnvrtid: %d\n",indentstr,
	onode->pst_oplcnvrtid, onode->pst_oprcnvrtid);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_opparm: %d\n",indentstr,onode->pst_opparm);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_fdesc: 0x%p\n",indentstr,onode->pst_fdesc);
   if (onode->pst_fdesc)
   {
   /* Format some of the ADI_FI_DESC fields */
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n  %sadi_fiopid: %d\n",indentstr,onode->pst_fdesc->adi_fiopid);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n  %sadi_finstid: %d\n",indentstr,onode->pst_fdesc->adi_finstid);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n  %sadi_fitype: %d\n",indentstr,onode->pst_fdesc->adi_fitype);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n  %sadi_numargs: %d   adi_npre_instr: %d\n",indentstr,
	onode->pst_fdesc->adi_numargs, onode->pst_fdesc->adi_npre_instr);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n  %sadi_dtresult: %d\n",indentstr,onode->pst_fdesc->adi_dtresult);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n  %sadi_dt1: %d\n",indentstr,onode->pst_fdesc->adi_dt[0]);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n  %sadi_dt2: %d\n",indentstr,onode->pst_fdesc->adi_dt[1]);
   }
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_opmeta: %d\n",indentstr,onode->pst_opmeta);
   if (!(onode->pst_pat_flags & AD_PAT_ISESCAPE_MASK) != AD_PAT_DOESNT_APPLY)
   {
	TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	    "\n%spst_escape: 0x%x\n",indentstr,onode->pst_escape);
	TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	    "\n%spst_pat_flags: 0x%x\n",indentstr,onode->pst_pat_flags);
   }
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_joinid: %d   pst_flags: 0x%x\n",indentstr,
	onode->pst_joinid, onode->pst_flags);
}   /* end of opt_pt_op_node */

static VOID
opt_pt_resdom_node(PST_RSDM_NODE   *rnode,
		char            *indentstr,
                char            *temp)
{
   /* Just print the RESDOM node fields */
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_rsno: %d\n",indentstr,rnode->pst_rsno);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_ntargno: %d  pst_ttargtype: %d\n",indentstr,
	rnode->pst_ntargno, rnode->pst_ttargtype);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_rsupdt: %d   pst_rsflags: 0x%x\n",indentstr,
	rnode->pst_rsupdt, rnode->pst_rsflags);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_rsname: %32s\n",indentstr,rnode->pst_rsname);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_defid: (0x%x, 0x%x)\n",indentstr,
	 rnode->pst_defid.db_tab_base, rnode->pst_defid.db_tab_index);
}   /* end of opt_pt_resdom_node */

static VOID
opt_pt_sort_node(PST_SRT_NODE    *snode,
	      char             *indentstr,
              char            *temp)
{
   /* Just print the SORT node fields */
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_srvno: %d   pst_srasc: %d\n", indentstr,
	snode->pst_srvno, snode->pst_srasc);
}   /* end of opt_pt_sort_node */

static VOID
opt_pt_tab_node(PST_TAB_NODE    *tnode,
	      char             *indentstr,
              char            *temp)
{
   /* Just print the TABLE node fields */
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_owner: %32s\n", indentstr, &tnode->pst_owner);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_tabname: %32s\n", indentstr, &tnode->pst_tabname);
}   /* end of opt_pt_tab_node */

static VOID
opt_pt_const_node(PST_CNST_NODE   *cnode,
	       char            *indentstr,
               char            *temp)
{
   /* Just print the CONST node fields */
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_tparmtype: %d   pst_parm_no: %d\n", indentstr,
	cnode->pst_tparmtype, cnode->pst_parm_no);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_pmspec: %d   pst_cqlang: %d\n", indentstr,
	cnode->pst_pmspec, cnode->pst_cqlang);
   if (cnode->pst_origtxt != NULL)     /* if there's real data ... */
	TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	    "\n%s pst_origtxt: addr 0x%p, value 0x%x\n",indentstr,
	   cnode->pst_origtxt,*((int *)cnode->pst_origtxt));
}   /* end of opt_pt_const_node */

static VOID
opt_pt_rulevar_node(PST_RL_NODE   *rnode,
		 char          *indentstr,
                 char            *temp)
{
   /* Just print the RULEVAR node fields */
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_rltype: %d\n", indentstr, rnode->pst_rltype);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_rltargtype: %d\n", indentstr, rnode->pst_rltargtype);
   TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n%spst_rltno: %d\n", indentstr, rnode->pst_rltno);
}   /* end of opt_pt_rulevar_node */

/*{
** Name: opt_pt_hddump	- Dump out a parse tree QTREE header structure
**
** Description:
**	This function formats and prints a query tree header for debugging
**	purposes.
**
** Inputs:
**	header				Pointer to the query tree header
**	printviews			TRUE to print view tree
**
** Outputs:
**	None
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** History:
**	17-Nov-2009 (kschendel) SIR 122890
**	    Update resjour display.
**	15-Oct-2010 (kschendel) SIR 124544
**	    Bunch of stuff removed from pst-restab, fix here.
*/
static VOID
opt_pt_hddump(
	PST_QTREE	   *header,
	bool		   printviews)
{
    i4  		i;
    PST_QNODE		*node;
    char                temp[OPT_PBLEN+1];

    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\nQuery tree header (PST_QTREE):\n\n");

    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n\npst_mode: %d - %s\n", header->pst_mode,
       (header->pst_mode <= 15) ? mtype_array[header->pst_mode] : "??");
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_vsn: %d\n", header->pst_vsn);

    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_rangetab: %d entries\n",header->pst_rngvar_count);
    for (i = 0; i < header->pst_rngvar_count; i++)
    {
	/* note that some entries may remain set to NULL */
	if (header->pst_rangetab[i] == (PST_RNGENTRY *) NULL)
	    continue;
	TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	   "\n  pst_rngvar[%d]: (0x%x,0x%x)\n",i,
	      header->pst_rangetab[i]->pst_rngvar.db_tab_base,
	      header->pst_rangetab[i]->pst_rngvar.db_tab_index);
	TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	   "\n    pst_timestamp: (%d,%d)\n",
	      header->pst_rangetab[i]->pst_timestamp.db_tab_high_time,
	      header->pst_rangetab[i]->pst_timestamp.db_tab_low_time);
	TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	   "\n    pst_corr_name: %32s\n",
	      &header->pst_rangetab[i]->pst_corr_name);
	TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	   "\n    pst_rgtype: %d\n", header->pst_rangetab[i]->pst_rgtype);
	TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	   "\n    pst_rgroot: 0x%p\n",
	      header->pst_rangetab[i]->pst_rgroot);
	TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	   "\n    pst_rgparent: %d\n",
	      header->pst_rangetab[i]->pst_rgparent);
	TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	   "\n    pst_inner_rel: 0x%x\n",
	      header->pst_rangetab[i]->pst_inner_rel.pst_j_mask[0]);
	TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	   "\n    pst_outer_rel: 0x%x\n",
	      header->pst_rangetab[i]->pst_outer_rel.pst_j_mask[0]);
	if (header->pst_rangetab[i]->pst_rgroot && printviews &&
		(header->pst_rangetab[i]->pst_rgtype == PST_RTREE ||
		 header->pst_rangetab[i]->pst_rgtype == PST_DRTREE ||
		 header->pst_rangetab[i]->pst_rgtype == PST_WETREE ||
		 header->pst_rangetab[i]->pst_rgtype == PST_TPROC))
	{	/* if it's a view, print its parse tree, too */
	    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
		"\n    view expansion:\n");
            opt_pt_prmdump(header->pst_rangetab[i]->pst_rgroot, 7);
	}
    }

    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_qtree: 0x%p\n", header->pst_qtree);
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_agintree: %s\n", header->pst_agintree ? "TRUE" : "FALSE");
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_subintree: %s\n", header->pst_subintree ? "TRUE" : "FALSE");
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_wchk: %s\n", header->pst_wchk ? "TRUE" : "FALSE");
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_rptqry: %s\n", (header->pst_mask1 & PST_RPTQRY) ? "TRUE" : "FALSE");
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_numparm: %d\n", header->pst_numparm);
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_stflag: %s\n", header->pst_stflag ? "TRUE" : "FALSE");
    if (header->pst_stflag)
    {
	TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	   "\npst_stlist:\n");
	for (node = header->pst_stlist; node != (PST_QNODE *) NULL;
	    node = node->pst_right)
	  TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	     "\n  pst_srvno: %d %s\n",
	     node->pst_sym.pst_value.pst_s_sort.pst_srvno,
	    (node->pst_sym.pst_value.pst_s_sort.pst_srasc) ? "ASC" : "DESC");
    }
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_restabid: (%d,%d)\n",
	 header->pst_restab.pst_restabid.db_tab_base,
	 header->pst_restab.pst_restabid.db_tab_index);
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_resname: %32s\n", &header->pst_restab.pst_resname);
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_resvno: %d\n", header->pst_restab.pst_resvno);
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_cursid: (%d,%d) %32s\n",
	 header->pst_cursid.db_cursor_id[0],header->pst_cursid.db_cursor_id[1],
	 header->pst_cursid.db_cur_name);
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_delete: %s\n", header->pst_delete ? "TRUE" : "FALSE");
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_updtmode: %d\n", header->pst_updtmode);
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_updmap: 0x%x,0x%x,0x%x,0x%x\n", 
       header->pst_updmap.psc_colmap[0],
       header->pst_updmap.psc_colmap[1],
       header->pst_updmap.psc_colmap[2],
       header->pst_updmap.psc_colmap[3]);
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_numjoins: %d\n", header->pst_numjoins);
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_firstn: %p\n", header->pst_firstn);
    if (header->pst_firstn != (PST_QNODE *) NULL)
	opt_pt_prmdump(header->pst_firstn, 5);
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_offsetn: %p\n", header->pst_offsetn);
    if (header->pst_offsetn != (PST_QNODE *) NULL)
	opt_pt_prmdump(header->pst_offsetn, 5);
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_mask1: 0x%x\n", header->pst_mask1);
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_hintmask: 0x%x\n", header->pst_hintmask);
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\npst_hintcount: %d\n", header->pst_hintcount);
    for (i = 0; i < header->pst_hintcount; i++)
    {
	PST_HINTENT	*hintp;

	hintp = &header->pst_tabhint[i];
	TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	    "\n    hint: %d, name1: %32s,  name2: %32s",
	    hintp->pst_hintcode, &hintp->pst_dbname1, 
	    &hintp->pst_dbname2);
    }
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	"\n");

    return;
}

/*{
** Name: OPT_QEP_DUMP - dump contents of a CO tree tabular style
**
** Description:
**      Prints CO tree with all fields displayed (not tree style)
[@comment_line@]...
**
** Inputs:
**      subquery		ptr to subquery anchoring CO tree of query tree
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
**      1-feb-00 (inkdo01)
**          Written
**	22-sep-00 (inkdo01)
**	    Add support of except, intersect set operators.
**	31-Aug-2006 (kschendel)
**	    add HFAGG.
*/
VOID
opt_qep_dump(
	OPS_STATE	*global,
	OPS_SUBQUERY	*subquery)

{
    char                temp[OPT_PBLEN+1];
    char		*sqtype;
    bool                init = FALSE;

    if (!global->ops_cstate.opc_prbuf)
    {
       global->ops_cstate.opc_prbuf = temp;
       init = TRUE;
    }

    if (subquery == NULL) return;

    switch (subquery->ops_sqtype) {

     case OPS_MAIN:
	sqtype = "main";
	break;
     case OPS_RSAGG:
	sqtype = "correlated simple agg";
	break;
     case OPS_SAGG:
	sqtype = "simple agg";
	break;
     case OPS_FAGG:
	sqtype = "function agg";
	break;
     case OPS_RFAGG:
	sqtype = "function retrieve agg";
	break;
     case OPS_HFAGG:
	sqtype = "hash func retrieve agg";
	break;
     case OPS_PROJECTION:
	sqtype = "projection";
	break;
     case OPS_SELECT:
	sqtype = "subselect";
	break;
     case OPS_UNION:
	sqtype = "union";
	break;
     case OPS_VIEW:
	sqtype = "view";
	break;
     default:
	sqtype = "unknown";
    }

    /* Header for this subquery. */
    TRformat(opt_scc, (i4 *)NULL, global->ops_cstate.opc_prbuf,
           OPT_PBLEN, "\n\nDump of QEP for subquery plan (%d, %d), type: %s, duplicates: %d\n\n",
	   subquery->ops_tsubquery, subquery->ops_tplan, sqtype,
	   subquery->ops_duplicates);

    /* Dump the CO-tree. */
    opt_qep_codump(global, subquery, subquery->ops_bestco, 1);

    if (init) global->ops_cstate.opc_prbuf = NULL;

}

/*{
** Name: OPT_QEP_CODUMP	- dump contents of CO node - tabular style
**
** Description:
**      Prints CO tree with all fields displayed (not tree style)
[@comment_line@]...
**
** Inputs:
**      subquery		ptr to subquery anchoring CO tree of query tree
**	conode			ptr to node in CO tree
**	indent			no. of spaces to indent display
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
**      1-feb-00 (inkdo01)
**          Written
**	29-oct-04 (inkdo01)
**	    Remove display of opo_memory - deprecated.
[@history_template@]...
*/
static VOID
opt_qep_codump(
	OPS_STATE      	   *global,
	OPS_SUBQUERY	   *subquery,
	OPO_CO		   *conode,
	i4		   indent)

{
    char      indentstr[256];           /* indentation string */
    char        temp[OPT_PBLEN+1];
    char	*jointype = " ";
    char	*storage_struct;
    char	*nodetype;
    OPT_NAME	tabname;
    i4		tablen;

    MEfill(indent, ' ', (PTR)indentstr);
    indentstr[indent] = '\0';           /* indent by 'n' blanks */

    /* Simply dump the contents of the CO tree node. */

    if (conode == NULL) return;

    /* Start by decoding some codes. */
    switch (conode->opo_sjpr) {
     case DB_ORIG:
	nodetype = "DB_ORIG (";
	opt_ptname(subquery, conode->opo_union.opo_orig, &tabname, FALSE);
	tablen = STlength(&tabname.buf[0]);
	tabname.buf[tablen] = ')';
	tabname.buf[tablen+1] = 0;
	jointype = &tabname.buf[0];
	break;
     case DB_PR:
	nodetype = "DB_PR";
	break;
     case DB_EXCH:
	nodetype = "DB_EXCH";
	break;
     case DB_SJ:
	nodetype = "DB_SJ";
	switch (conode->opo_variant.opo_local->opo_jtype) {
	 case OPO_SJFSM:
	    jointype = " (FSM)";
	    break;
	 case OPO_SJPSM:
	    jointype = " (PSM)";
	    break;
	 case OPO_SJTID:
	    jointype = " (Tid)";
	    break;
	 case OPO_SJKEY:
	    jointype = " (Key)";
	    break;
	 case OPO_SJHASH:
	    jointype = " (Hash)";
	    break;
	 case OPO_SJCARTPROD:
	    jointype = " (Cartprod)";
	    break;
	 case OPO_SJNOJOIN:
	    jointype = " (Nojoin)";
	    break;
	}
	break;
     default:
	nodetype = "reformat";
	break;
    }

    switch (conode->opo_storage) {
     case DB_SORT_STORE:
	storage_struct = "Sort";
	break;
     case DB_HEAP_STORE:
	storage_struct = "Heap";
	break;
     case DB_ISAM_STORE:
	storage_struct = "Isam";
	break;
     case DB_HASH_STORE:
	storage_struct = "Hash";
	break;
     case DB_BTRE_STORE:
	storage_struct = "B-tree";
	break;
     default:
	storage_struct = "Other";
	break;
    }

    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	 "\n%s===========================================\n", indentstr);
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	 "\n%sCO Node: 0x%p, type: %s%s, storage: %s\n", 
	indentstr, conode, nodetype, jointype, storage_struct);
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	 "\n%sopo_outer: 0x%p, opo_inner: 0x%p\n", 
	indentstr, conode->opo_outer, conode->opo_inner);
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	 "\n%sopo_cost: (cpu: %f, dio: %f, reltotb: %f, pagestouched: %f\n",
	indentstr, conode->opo_cost.opo_cpu, conode->opo_cost.opo_dio, 
	conode->opo_cost.opo_reltotb, conode->opo_cost.opo_pagestouched);
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	 "\n%s           tups: %f, adfactor: %f, sortfact: %f, tpb: %f)\n", 
	indentstr, conode->opo_cost.opo_tups, conode->opo_cost.opo_adfactor,
	conode->opo_cost.opo_sortfact, conode->opo_cost.opo_tpb);
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	 "\n%sFlags - delete: %s, existence: %s, odups: %s, idups: %s\n",   
	indentstr, (conode->opo_deleteflag) ? "T" : "F", 
	(conode->opo_existence) ? "T" : "F", (conode->opo_odups) ? "T" : "F",
	(conode->opo_idups) ? "T" : "F");
    TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	 "\n%s        osort: %s, isort: %s, psort: %s, pdups: %s\n",
	indentstr, (conode->opo_osort) ? "T" : "F", (conode->opo_isort) ? "T" : "F", 
	(conode->opo_psort) ? "T" : "F", (conode->opo_pdups) ? "T" : "F");
    if (conode->opo_ordeqc > OPE_NOEQCLS)
    {
	char	temp1[OPT_PBLEN+1];

	opt_qepd_alist(global, subquery, conode, conode->opo_ordeqc, temp1);
	TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	 "\n%sopo_ordeqc: %s\n", indentstr, temp1);
    }

    if (conode->opo_sjeqc > OPE_NOEQCLS)
    {
	char	temp1[OPT_PBLEN+1];

	opt_qepd_alist(global, subquery, conode, conode->opo_sjeqc, temp1);
	TRformat(opt_scc, (i4 *)NULL, temp, OPT_PBLEN,
	 "\n%sopo_sjeqc: %s\n", indentstr, temp1);
    }

    /* Now recurse on opo_outer/inner. */
    if (conode->opo_outer != NULL)
	opt_qep_codump(global, subquery, conode->opo_outer, indent+2);
    if (conode->opo_inner != NULL)
	opt_qep_codump(global, subquery, conode->opo_inner, indent+2);

}

/*{
** Name: OPT_QEPD_ALIST	- QEP dump version of opt_alist
**
** Description:
**      This routine will dump an attribute list for a node 
**      in the QEP dumping routines
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
**	1-feb-00 (inkdo01)
**	    Cloned from opt_alist
*/
static VOID
opt_qepd_alist(
	OPS_STATE	   *global,
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop,
	OPO_ISORT	   ordering,
	char		   *prbuf) 
{
    OPE_IEQCLS      eqcls;	    /* next attribute of a multi-attribute
				    ** ordering */
    bool	    eof;	    /* TRUE if the last attribute is
				    ** processed */
    i4		    level;	    /* used to process multi-attribute
				    ** ordering */
    i4		    off = 0;	    /* Offset to next print buffer entry */
    char	    *delimiter;	    /* used to terminate or separate
				    ** an attribute list */
    char	    *prefix;
    OPT_NAME        stoname;	    /* name of attribute associated with
                                    ** this equivalence class */

    eqcls = OPE_NOEQCLS;
    eof = opt_onext(subquery, ordering, &eqcls, &level); /* get the next
				    ** attribute of a multi-attribute ordering */
    MEfill(OPT_PBLEN+1, (u_char)0, (PTR)prbuf);
    prefix = "(";
    do
    {
	opt_attrnm(subquery, cop, (i4) eqcls, &stoname); /* get the name of
				    ** the attribute associated with this
				    ** subtree cop */
	if(eof = opt_onext(subquery, ordering, &eqcls, &level))
	    delimiter = ")";	    /* use closing bracket for the last
				    ** attribute */
	else
	    delimiter = ",";	    /* use comma if this is not the last
				    ** attribute */
	TRformat(NULL, (i4 *) NULL, &prbuf[off], OPT_PBLEN-off,
	    "%s%s%s", prefix, &stoname, delimiter);
	prefix = " ";		    /* indent with blank for second and subsequent
				    ** attributes being listed */
	off = STlength(prbuf);
    }
    while (!eof);
}

/*{
** Name: opt_1fidop - support routine to enable call to pst_1fidop for pst_opno
**       translation.
**
** Description:
**      This routine takes a pst_opno and translates it to the equivalent symbol
**      or name via a call to pst_1fidop. The translation is a '\0' terminated
**      string, which is placed in the buffer pointed to by buf, supplied by the
**      caller and assumed to be big enough.
**
** Inputs:
**      opno	pst_opno value to be translated
**      buf	pointer to char buffer to contain the translation. Buffer is
**		supplied by caller and assumed by this function to be big enough.
**
** Outputs:
**	Returns:	None.
**	Exceptions:	None.
**
** Side Effects:	None.
**
** History:
**	22-Mar-2010 (smeke01) b123454
**	    Created.
*/
static VOID
opt_1fidop(
        ADI_OP_ID          opno,
        char               *buf)
{
    PST_QNODE qnode; /* needed simply to pass the opno to pst_1prnode */

    qnode.pst_sym.pst_value.pst_s_op.pst_opno = opno;
    pst_1fidop(&qnode, buf);
    /* pst_1fidop doesn't truncate "BAD OP ID" string */
    if ( STncmp(buf, "BAD OP ID", 9) == 0 )
	buf[9]='\0';
}
