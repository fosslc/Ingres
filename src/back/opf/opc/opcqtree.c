/*
**Copyright (c) 2004, 2010 Ingres Corporation
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
#include    <qefmain.h>
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

/* external routine declarations definitions */
#define        OPC_QTREE 	TRUE
#include    <opxlint.h>
#include    <opclint.h>
#include    <bt.h>
#include    <me.h>

/**
**
**  Name: OPCQTREE.C - Routines to play around with query trees for compilation.
**
**  Description:
**      This file contains routines that perform various operations on query 
**      trees for query compilation. This tends to border on being a  
**      miscellaneous file, since any routines that are big and important 
**      enough have their own file, opcqual.c for example.  
**
**	Externally visible routines:
**          opc_keyqual()   - Build a query tree qualification.
**          opc_qconvert()  - Convert a const qtree node to a type and length.
**          opc_compare()   - Compare two qnodes.
**          opc_gtqual()    - Get the qualification to be applied at a co/qen node
**          opc_metaops()   - Return the number of meta operator nodes in a qtree
**          opc_cnst_expr() - Is a qual a constant expression?
**
**	Internal only routines:
**          opc_onlyatts()  - Check if a clause only contains specified atts
**          opc_cvclause()  - Convert a qtree qual into an OPC_QAND struct.
**	    opc_complex_clause() - Check if a BF contains AND nested under OR
**			      (possible in nonCNF parse trees).
**
**  History:
**      31-aug-86 (eric)
**          written
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**      15-aug-89 (eric)
**          Changed the way that repeat query parameters/dbp vars/expressions
**          are used in keying.
**	17-may-90 (stec)
**	    Fixed bug 8438 related to building keys for repeat queries.
**	26-nov-90 (stec)
**	    Made changes in opc_onlyatts() and opc_cvclause() for the
**	    Caroon & Black problem.
**	03-dec-90 (stec)
**	    Modified opc_onlyatts() code to fix bug 33762.
**	06-dec-90 (stec)
**	    Modified opc_cvclause() to fix a problem introduced by Caroon
**	    and Black code modification. Reorganized code in opc_onlyatts().
**	06-feb-91 (stec)
**	    Modified opc_cvclause() to fix timezone related bug #35790.
**	18-apr-91 (nancy)
**	    bugs 36870, 36711 fix opc_onlyatts(). See comments below.
**      18-aug-91 (eric)
**          Added minor supporting changes for bugs 37437 and 36518. See
**          opckey.c for comments on the big picture.
**      08-oct-91 (hhsiung)
**          Initialize cqual->opc_qsinfo.opc_ktype when first created,
**          for Bull DPX/2.
**	19-feb-92 (rickh)
**	    Removed opc_ionlyatts.  It doesn't do anything useful.
**      21-feb-92 (anitap)
**          Changed the initialization of cqual->opc_qsinfo.opc_ktype in
**          opc_keyqual().
**	13-may-92 (rick)
**	    The routines which bundle together inner join boolean factors
**	    (opc_gtqual and opc_gtiqual) should tie into their little
**	    packages any outer join boolean factors which enumeration
**	    pushed down out of the ON clauses of higher nodes.  Added
**	    cnode as an argument to opc_gtqual to help it do this job.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	22-jun-93 (rickh)
**	    A TID join should compile into its ON clause the ON clause of
**	    its degenerate inner ORIG node, since that ORIG node will not be
**	    separately compiled.
**	30-aug-93 (rickh)
**	    As usual, the above fix for KEY joins, too.
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	03-jan-96 (wilri01)
**	    Sort bfs by selectivity 
**	11-apr-96 (wilri01)
**	    Fixed chg on 03-jan-96 to allow for zero selectivity
**	11-Nov-2009 (kiria01) SIR 121883
**	    With PST_INLIST, check for PST_CONST on RHS as the flag
**	    may be applied to PST_SUBSEL to aid servers to distinguish
**	    simple = from IN and <> from NOT IN.
**	04-Aug-2010 (kiria01) b124178
**	    Allow for the potential of PST_COP nodes which can now be in
**	    a boolean position with ii_true/ii_false.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
OPC_QUAL *opc_keyqual(
	OPS_STATE *global,
	OPC_NODE *cnode,
	PST_QNODE *root_qual,
	OPV_IVARS varno,
	i4 tid_key);
static bool opc_complex_clause(
	OPS_STATE *global,
	PST_QNODE *clause,
	bool underor);
i4 opc_onlyatts(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPC_QUAL *cqual,
	PST_QNODE *root,
	OPV_IVARS varno,
	RDR_INFO *rel,
	OPC_QSTATS *qstats,
	i4 *ncompares);
i4 opc_cnst_expr(
	OPS_STATE *global,
	PST_QNODE *root,
	i4 *pm_flag,
	DB_LANG *qlang);
static void opc_cvclause(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPC_QUAL *cqual,
	OPC_QAND *cnf_and,
	PST_QNODE *root,
	RDR_INFO *rel);
PST_QNODE *opc_qconvert(
	OPS_STATE *global,
	DB_DATA_VALUE *dval,
	PST_QNODE *qconst);
i4 opc_compare(
	OPS_STATE *global,
	PST_QNODE *d1,
	PST_QNODE *d2);
void opc_gtqual(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPB_BMBF *bfactbm,
	PST_QNODE **pqual,
	OPE_BMEQCLS *qual_eqcmp);
void opc_gtiqual(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPB_BMBF *bfactbm,
	PST_QNODE **pqual,
	OPE_BMEQCLS *qual_eqcmp);
void opc_ogtqual(
	OPS_STATE *global,
	OPC_NODE *cnode,
	PST_QNODE **pqual,
	OPE_BMEQCLS *qual_eqcmp);
i4 opc_metaops(
	OPS_STATE *global,
	PST_QNODE *root);

/*{
** Name: OPC_KEYQUAL	- Build a query tree key qualification.
**
** Description:
**      Opc_keyqual() accepts a qualification that is to be applied to the
**	current orig node and returns a key qual in the form of an OPC_QUAL.
**	The key qual will contain an OPC_QAND structure which is a converted
**	form of all or part of the input qualification. The conversion is
**	is a one to one conversion in that it retains the conjunctive normal
**	form nature of the input qual. 
**
**	The algorithm for this routine is:
**	
** opc_keyqual()
** {
**	allocate an OPC_QUAL struct and give it some initial values;
**	for (each clause in the input qual)
**	{
**	    if (this clause only refers to key attributes and doesn't force
**		    a full scan of the relation
**		)
**	    {
**		update some stats in the OPC_QUAL;
**
**		Allocate an OPC_QAND struct and put it on the linked list
**		    of OPC_QANDs;
**		Convert the current clause into the OPC_QAND struct;
**	    }
**	}
**
**	Number all of the constants, which have been seperated by key number,
**	    in increasing order of value;
**
**	return the OPC_QUAL;
** }
**        
**      Note that numbering the key constants is a simple task since they 
**      are sorted by increasing value as they are added to the list of
**      constants. This sorting task occurs when the clause in converted 
**      into an OPC_QAND. 
[@comment_line@]...
**
** Inputs:
**  global
**	Used to find the qualification of the current subquery, and a parameter
**	to other routines.
**  cnode
**	Used as a parameter to other routines.
**  root_qual
**	This is a pointer to a qualification that should be applied to 
**	the current orig node. It will serve as the basic source of information
**	on how to key into the current relation.
**  varno
**	The current relation that is being originated.
**  tid_key
**	TRUE if we want to use the relation's tid as the key attribute. 
**	Otherwise it should be FALSE and the storage structures key attributes
**	will be used.
**
** Outputs:
**  none
**
**	Returns:
**	    A pointer to an OPC_QUAL structure, which contains the key
**	    qualification in the form of an OPC_QAND struct.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-aug-86 (eric)
**          written
**      1-aug-87 (eric)
**          re-written for performance, to produce better keying strategies,
**          and to fix some pathological bugs.
**      10-sept-91 (eric)
**          Initialize 'szparm' in all cases (the 1-aug-91 change didn't always
**          initialize it). Also, take the initialization out of a loop.
**      08-oct-91 (hhsiung)
**          Initialize cqual->opc_qsinfo.opc_kpype when first created,
**          for Bull DPX/2.
**      21-feb-92 (anitap)
**          Changed the initialization of cqual->opc_qsinfo.opc_kpype from 0
**          to PST_USER.
**      15-feb-93 (ed)
**          fix prototype casting error
**	9-may-97 (inkdo01)
**	    Add call to opc_complex_clause to determine if BF has nested AND
**	    under OR, disqualifying it for keying consideration. This is 
**	    part of non CNF support.
**	3-feb-04 (inkdo01)
**	    Changed DB_TID to DB_TID8 for table partitioning.
**	23-Mar-09 (kibro01) b121835
**	    Change opc_inequality to be the key sequence number of the first
**	    inequality, so a decision can be made on whether a series of OR
**	    exact matches can be employed on this key followed by ranges.
**	3-Jun-2009 (kschendel) b122118
**	    Nobody was using various counts, don't calculate them.
[@history_template@]...
*/
OPC_QUAL *
opc_keyqual(
		OPS_STATE   *global,
		OPC_NODE    *cnode,
		PST_QNODE   *root_qual,
		OPV_IVARS   varno,
		i4	    tid_key)
{
    OPS_SUBQUERY    *subqry = global->ops_cstate.opc_subqry;
    RDR_INFO	    *rel;
    i4		    nkey_atts;
    i4		    cqual_size;
    OPC_QUAL	    *cqual;
    i4		    keyno;
    OPC_KCONST	    *kconst;
    DMT_ATT_ENTRY   *att;
    OPC_QSTATS	    qstats;
    PST_QNODE	    *clause;
    i4		    ncompares;
    i4		    cnf_and_size;
    i4		    parm_array_size;
    OPC_QAND	    *cnf_and;
    OPC_QCONST	    *cqconst;
    i4		    constno;
    i4		    begin_hiparm;
    i4		    szparm;

    /* Initialize some vars to keep line length down. */
    rel = subqry->ops_vars.opv_base->opv_rt[varno]->opv_grv->opv_relation;

    /* Find out how many key attributes we have to play with */
    if (tid_key == TRUE)
    {
	nkey_atts = 1;
    }
    else
    {
	nkey_atts = rel->rdr_keys->key_count;
    }

    /* Lets allocate the OPC_QUAL struct that we'll be filling in here */
    cqual_size = sizeof (OPC_QUAL);
    if (nkey_atts > 1)
    {
	cqual_size += sizeof (OPC_KCONST) * (nkey_atts - 1);
    }
    cqual = (OPC_QUAL *) opu_Gsmemory_get(global, cqual_size);

    /* Now lets give cqual some initial values */
    cqual->opc_cnf_qual = NULL;
    cqual->opc_dnf_qual = NULL;
    cqual->opc_end_dnf = NULL;
    cqual->opc_qsinfo.opc_inequality = 0;
    cqual->opc_qsinfo.opc_nkeys = nkey_atts;
    cqual->opc_qsinfo.opc_storage_type = rel->rdr_rel->tbl_storage_type;
    cqual->opc_qsinfo.opc_tidkey = tid_key;
    cqual->opc_qsinfo.opc_nparms = 0;
    cqual->opc_qsinfo.opc_nconsts = 0;
    cqual->opc_qsinfo.opc_kptype = PST_USER;
    
    cqual->opc_qsinfo.opc_hiparm = global->ops_parmtotal + 1;
    if (global->ops_parmtotal > 0 && global->ops_procedure->pst_isdbp == FALSE)
    {
	cqual->opc_qsinfo.opc_kptype = PST_RQPARAMNO;
    }
    else if (global->ops_procedure->pst_isdbp == TRUE)
    {
	cqual->opc_qsinfo.opc_kptype = PST_LOCALVARNO;

    }
    if (cqual->opc_qsinfo.opc_hiparm == 0)
    {
	cqual->opc_qsinfo.opc_hiparm = 1;
	cqual->opc_qsinfo.opc_kptype = PST_USER;
    }
    begin_hiparm = cqual->opc_qsinfo.opc_hiparm;

    for (keyno = 0; keyno < nkey_atts; keyno += 1)
    {
	kconst = &cqual->opc_kconsts[keyno];

	kconst->opc_qconsts = NULL;
	kconst->opc_qparms = NULL;
	kconst->opc_dbpvars = NULL;
	kconst->opc_nuops = 0;

	/* Fill in what we can in kconst->opc_keybld. The .adc_kdv and
	** .adc_opkey will have to be filled in on a const by constant
	** basis.
	*/
	if (tid_key == TRUE)
	{
	    kconst->opc_keyblk.adc_lokey.db_datatype = DB_TID8_TYPE;
	    kconst->opc_keyblk.adc_lokey.db_length = DB_TID8_LENGTH;
	    kconst->opc_keyblk.adc_lokey.db_prec = 0;
	    kconst->opc_keyblk.adc_lokey.db_data = NULL;
	}
	else
	{
	    att = rel->rdr_attr[rel->rdr_keys->key_array[keyno]];
	    kconst->opc_keyblk.adc_lokey.db_datatype = att->att_type;
	    kconst->opc_keyblk.adc_lokey.db_length = att->att_width;
	    kconst->opc_keyblk.adc_lokey.db_prec = att->att_prec;
	    kconst->opc_keyblk.adc_lokey.db_data = NULL;
	}

	STRUCT_ASSIGN_MACRO(kconst->opc_keyblk.adc_lokey, 
						kconst->opc_keyblk.adc_hikey);
    }
    
    /* for (each clause) */
    qstats.opc_tidkey = tid_key;
    for (clause = root_qual;
	    clause != NULL && clause->pst_sym.pst_type != PST_QLEND;
	    clause = clause->pst_right
	)
    {
	/* if (this clause only contains references to key attributes
	**					of the relation being origed)
	*/
	qstats.opc_nparms = 0;
	qstats.opc_nconsts = 0;
	qstats.opc_inequality = 0;
	qstats.opc_hiparm = cqual->opc_qsinfo.opc_hiparm;
	qstats.opc_kptype = cqual->opc_qsinfo.opc_kptype;
	ncompares = 0;
	if (opc_complex_clause(global, clause, FALSE)) continue;
				/* if this one has AND under OR, skip it */
	if (opc_onlyatts(global, cnode, cqual, clause->pst_left, 
			varno, rel, &qstats, &ncompares) == TRUE
	    )
	{
	    /* We have established that the current clause is usefull for the
	    ** current keying task. So, lets convert it into a form that's
	    ** easier for the keying code to handle.
	    */

	    /* First, lets update the flags and counters in the OPC_QUAL */
	    cqual->opc_qsinfo.opc_nparms += qstats.opc_nparms;
	    cqual->opc_qsinfo.opc_nconsts += qstats.opc_nconsts;
	    cqual->opc_qsinfo.opc_hiparm = qstats.opc_hiparm;

	    /* Now lets allocate the memory to hold the current clause in
	    ** its converted form. This clause won't be converted until
	    ** all the keyable clauses are found however. This is to allow
	    ** us to determine the actual number of parameters that will
	    ** be used in this keying.
	    */
	    cnf_and_size = sizeof (OPC_QAND);
	    if (ncompares > 1)
	    {
		cnf_and_size += sizeof (OPC_QCMP) * (ncompares - 1);
	    }
	    cnf_and = (OPC_QAND *) opu_Gsmemory_get(global, cnf_and_size);

	    cnf_and->opc_anext = cqual->opc_cnf_qual;
	    cnf_and->opc_aprev = NULL;
	    cnf_and->opc_ncomps = ncompares;
	    cnf_and->opc_curcomp = -1;
	    cnf_and->opc_nnochange = 0;
	    cnf_and->opc_klo_old = NULL;
	    cnf_and->opc_khi_old = NULL;
	    cnf_and->opc_conjunct = clause->pst_left;

	    if (cqual->opc_cnf_qual != NULL)
	    {
		cqual->opc_cnf_qual->opc_aprev = cnf_and;
	    }
	    cqual->opc_cnf_qual = cnf_and;
	}

	/*
	** This should be set regardless of whether opc_onlyatts() returns true
	** or false. The reason is that this value may be set in opc_onlyatts()
	** even if false is returned.
	*/
	if (qstats.opc_inequality > 0)
	{
	    cqual->opc_qsinfo.opc_inequality = qstats.opc_inequality;
	}
    }

    /* Because of the anomaly of how parameters are numbered (starting with
    ** one rather than zero), we need to check here to see if we plan on
    ** using any parameters and set opc_hiparm and begin_hiparm to zero
    ** to insure that we don't allocate an parm related memory.
    */
    if (cqual->opc_qsinfo.opc_nparms == 0)
    {
	cqual->opc_qsinfo.opc_hiparm = begin_hiparm = 0;
    }
    
    /* now that we know the number of parameters that will be used in the
    ** keying, we can allocate the opc_qparms and opc_dbpvars arrays.
    */
    if (cqual->opc_qsinfo.opc_hiparm > 0)
    {
	szparm = cqual->opc_qsinfo.opc_hiparm;
	for (keyno = 0; keyno < nkey_atts; keyno += 1)
	{
	    kconst = &cqual->opc_kconsts[keyno];

	    kconst->opc_qconsts = NULL;

	    /* initialize opc_qparms and opc_dbpvars */
	    if (cqual->opc_qsinfo.opc_kptype == PST_LOCALVARNO)
	    {
		parm_array_size = 
		    sizeof (OPC_QCONST **) * cqual->opc_qsinfo.opc_hiparm;
		kconst->opc_dbpvars = 
		    (OPC_QCONST **) opu_Gsmemory_get(global, parm_array_size);
		MEfill(parm_array_size, (u_char)0, (PTR)kconst->opc_dbpvars);

		kconst->opc_qparms = NULL;
	    }
	    else
	    {
		cqual->opc_qsinfo.opc_kptype = PST_RQPARAMNO;
		parm_array_size = 
		    sizeof (OPC_QCONST **) * cqual->opc_qsinfo.opc_hiparm;
		kconst->opc_qparms = 
		    (OPC_QCONST **) opu_Gsmemory_get(global, parm_array_size);
		MEfill(parm_array_size, (u_char)0, (PTR)kconst->opc_qparms);

		kconst->opc_dbpvars = NULL;
	    }
	}
    }
    else
    {
	szparm = 0;
    }

    /* now that we have the list of qtree conjuncts to be converted into
    ** OPC form, and that we have allocated memory to hold the parameter
    ** key infomation, we can convert each of the qtree conjuncts into
    ** OPC form.
    */
    cqual->opc_qsinfo.opc_hiparm = begin_hiparm;
    for (cnf_and = cqual->opc_cnf_qual; 
	    cnf_and != NULL; 
	    cnf_and = cnf_and->opc_anext
	)
    {
	opc_cvclause(global, cnode, cqual, cnf_and, cnf_and->opc_conjunct, rel);
	cnf_and->opc_curcomp = -1;
    }

    /* Now that we've converted all of the keying clauses, lets number each 
    ** of the key constants.
    */
    for (keyno = 0; keyno < nkey_atts; keyno += 1)
    {
	kconst = &cqual->opc_kconsts[keyno];

	constno = 0;
	for (cqconst = kconst->opc_qconsts;
	     cqconst != NULL;
	     cqconst = cqconst->opc_qnext
	    )
	{
	    cqconst->opc_tconst_type = PST_USER;
	    cqconst->opc_const_no = constno++;
	}
    }

    return(cqual);
}

/*{
** Name: OPC_COMPLEX_CLAUSE - Check if BF has AND nested under OR.
**
** Description:
**	Opc_complex_clause() returns TRUE when the where clause fragment 
**	being tested contains an AND nested under an OR. This is only possible
**	in a where which has not been converted to conjunctive normal form.
**	Such complex boolean factors are ineligible for key transformation
**	(at least for the time being they are). 
**
** Inputs:
**	global - pointer to global state structure.
**	clause - pointer to where clause parse tree fragment being analyzed.
**	underor - flag indicating whether clause is under an OR or not.
**
** Outputs: none
**
** Returns:
**	TRUE - if clause contains AND nested under OR.
**	FALSE - if not.
**
** Side Effects: none
**
** History:
**
**	9-may-97 (inkdo01)
**	    Written.
[@history_template@]...
*/
static bool
opc_complex_clause(
	OPS_STATE	*global,
	PST_QNODE	*clause,
	bool		underor)
{

    /* This function simply analyzes the node types of the given where
    ** clause parse tree fragment. It recurses down the left side of the 
    ** fragment, and iterates down the right side. It is only interested
    ** in ANDs and ORs - all the rest just return FALSE. */

    while(TRUE)		/* stay in switch loop 'til we return */
     switch (clause->pst_sym.pst_type) {

     case PST_OR:
	if (opc_complex_clause(global, clause->pst_left, TRUE))
	    return(TRUE);		/* underor is TRUE here on down */
	clause = clause->pst_right;	/* prepare to iterate on right */
	if (!clause) return(FALSE);
	underor = TRUE;
	break;		/* leave switch, but NOT while loop */

     case PST_AND:
	if (underor) return(TRUE);	/* this is what we're looking for! */
	if (opc_complex_clause(global, clause->pst_left, underor))
	    return(TRUE);		/* likewise */
	clause = clause->pst_right;	/* prepare to iterate on right */
	if (!clause) return(FALSE);	/* but only if there IS a right */
	break;		/* leave switch, but NOT while loop */

     default:		/* everything else */
	return(FALSE); 
     }		/* end of switch (and also the while loop) */

    /* We should never get to here */

}	/* end of opc_complex_clause */

/*{
** Name: OPC_ONLYATTS	- Check if a clause contains only specified attributes.
**
** Description:
**      Opc_onlyatts() returns TRUE if the var nodes of a given qtree refer
**      only to the key attributes of a given relation, and the var node
**	is involved in a comparison that is useful for keying. A comparison
**	is useful for keying if there is a constant opposite the var node
**	and ADF says that not all the relation will match the comparison.
**	For example, key_att = other_att + 5 isn't useful for keying. Likewise
**	key_att = "*" isn't useful.
**
** Inputs:
**  global
**	Used to find the current subquery.
**  cnode
**	Used to find the current co node being converted.
**  cqual
**	The key qualification that is in the process of being built.
**  root
**	This is the root of the qualification to be searched
**  varno
**	This is the variable number of the relation that is being originated.
**  rel
**	The RDR_INFO description of the relation that is being originated.
**  qstats
**	This tells various information about the clause being examined.
**	When its passed in, it should be initialized to beginning values.
**	Ie. opc_tidkey should be set according to whether we are building
**	a tid key qualification or not. Opc_nparms should be 0, opc_nconsts
**	should be zero, and opc_inequality should be FALSE.
**  ncompares
**	A pointer to an uninitialized nat.
**
** Outputs:
**  qstats
**	This will be filled in with the stats about root.
**  ncompares
**	This will tell the number of comparisions that are in root.
**	
**
**	Returns:
**	    TRUE or FALSE.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-AUG-86 (eric)
**          written
**	17-may-90 (stec)
**	    Fixed bug 8438 related to building keys for repeat queries.
**	    Db_data ptr is set to null for constant nodes representing
**	    repeat query parms and local vars in database procedures.
**	26-nov-90 (stec)
**	    Made changes for the Caroon & Black problem. The idea is that
**	    all qualifications containing attributes of the DATE datatype
**	    be handled at runtime by ADF, so we treat them pretty much as
**	    repeat query parms or database procedure parameters.
**	03-dec-90 (stec)
**	    Modified code to fix bug 33762. The fix is in the code checking
**	    if an attribute is useful for keying. The problem was that
**	    in case there was a primary key on a keyed relation and TID
**	    keying was requested, this procedure would indicate that qual.
**	    is useful for keying whereas it is not. Example:
**		select tid, x, y from b33762 where tid != 1536 and x = 1
**	    and x is a key attribute in the query above.
**	06-dec-90 (stec)
**	    Reorganized code to make it more readable.
**	18-apr-91 (nancy)
**	    bugs 36870, 36711 -- corrected change (above).  It caused
**	    consistency checks on create dbproc because of incorrect logic.
**	13-dec-96 (inkdo01)
**	    Added one more check to avoid index processing of <>  ("ne")
**	    predicates (after <> histogram selectivity improvements).
**	12-sep-03 (hayke02)
**	    Do not report error E_OP0793_ADE_ENDCOMP if adc_keybld() returns
**	    with an ad_errcode of E_AD3002_BAD_KEYOP, rather break with a
**	    ret of FALSE. This will allow like predicates on key decimal
**	    attributes to succeed. This change fixes bug 110667.
**	17-dec-03 (inkdo01)
**	    Added code to count IN-list constant entries from compacted
**	    IN-list structure.
**	22-nov-05 (inkdo01)
**	    Fixed bug with mixed constants/host var parms in IN-list.
**	17-jul-2006 (gupsh01)
**	    Added support for new ANSI datetime data types.
**	6-Oct-06 (kibro01) bug 116558
**	    Count IN-list constant entries even if they are dates
**	25-Jun-2008 (kiria01) SIR120473
**	    Move pst_isescape into new u_i2 pst_pat_flags.
**	23-Mar-09 (kibro01) b121835
**	    Change opc_inequality to be the key sequence number of the first
**	    inequality, so a decision can be made on whether a series of OR
**	    exact matches can be employed on this key followed by ranges.
**      11-nov-2009 (huazh01)
**          Add support for PST_CONST node. Constant folding project
**          make it possible that a PST_CONST node exists in query tree.
**          (b122750)
**	02-Dec-2009 (kiria01) b122952
**	    Ajust counts for sorted inlist to control keybld expectations
**	    and if needed to drop the hash lookup if only range key being
**	    built.
**	15-Dec-2009 (smeke01) b123057
**	    The variable incount counts from zero so comparison with 
**	    opf_inlist_thresh must be '>=' rather than '>'. 
[@history_template@]...
*/
i4
opc_onlyatts(
		OPS_STATE   *global,
		OPC_NODE    *cnode,
		OPC_QUAL    *cqual,
		PST_QNODE   *root,
		OPV_IVARS   varno,
		RDR_INFO    *rel,
		OPC_QSTATS  *qstats,
		i4	    *ncompares)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    i4			ret;
    OPZ_ATTS		*att;
    PST_QNODE		*qconst;
    PST_QNODE		*inlist;
    DMT_ATT_ENTRY	*relatt;
    PST_QNODE		*qvar;
    i4			attno;
    OPC_KCONST		*kconst;
    DB_STATUS		err;
    PTR			save_lokey;
    PTR			save_hikey;
    i4			incount = 0;
    i4			cpm_flag;
    DB_LANG		cur_qlang;
    DB_LANG		save_qlang;

    switch (root->pst_sym.pst_type)
    {
     case PST_UOP:
     case PST_BOP:
	    /* something to break out of */
	    for (;;)
	    {
		*ncompares += 1;
		ret = TRUE;

		/* Let's see if there's a var node on one side */
		if (root->pst_left != NULL &&
		    root->pst_left->pst_sym.pst_type == PST_VAR)
		{
		    qvar = root->pst_left;
		    qconst = root->pst_right;
		}
		else if (root->pst_right != NULL &&
			 root->pst_right->pst_sym.pst_type == PST_VAR)
		{
		    qvar = root->pst_right;
		    qconst = root->pst_left;
		}
		else
		{
		    ret = FALSE;
		    break;
		}

		/* Count IN-list constants linked down PST_CONST lhs. */
		if (root->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLIST &&
		    qconst->pst_sym.pst_type == PST_CONST &&
		    root->pst_sym.pst_value.pst_s_op.pst_opno ==
			global->ops_cb->ops_server->opg_eq)
		{
		    for (incount = 0, inlist = qconst->pst_left;
			inlist; inlist = inlist->pst_left, incount++);
		    /* incount comparison to ops_inlist_thresh is ">=" because we've counted from 0 */
		    if ((root->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLISTSORT) &&
			global->ops_cb->ops_alter.ops_inlist_thresh &&
			incount >= global->ops_cb->ops_alter.ops_inlist_thresh)
		    {
			incount = 0;
			/* If we are dealing with HASH then the large number
			** of in values will mean we will have switched to a range
			** lookup and a hash index will not support that. */
			if (rel->rdr_rel->tbl_storage_type == DMT_HASH_TYPE)
			   ret = FALSE;
		    }
		    else
			*ncompares += incount;	/* roll into total */
		}
		else incount = 0;

		/* Identify the attribute */
		attno = qvar->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
		att = subqry->ops_attrs.opz_base->opz_attnums[attno];
		if (cnode->opc_ceq[att->opz_equcls].opc_eqavailable == TRUE)
		{
		    attno = cnode->opc_ceq[att->opz_equcls].opc_attno;
		    att = subqry->ops_attrs.opz_base->opz_attnums[attno];
		}
		else
		{
		    ret = FALSE;  
		    break;
		}

		/*
		** Check if the attribute is a part of the key of our relation,
		** also find out if keying by TID has been requested. An
		** attribute is "intersting" only if it belongs to our relation
		** and is not a function attribute. If keying by TID has been
		** requested it must be a TID, else if keying by TID has not
		** been requested, it must not be a TID and must be part of the
		** relation key.
		*/
		relatt = rel->rdr_attr[att->opz_attnm.db_att_id];
		if (!(att->opz_varnm == varno && 
		      att->opz_attnm.db_att_id != OPZ_FANUM &&
		      (   
			(
			    qstats->opc_tidkey == TRUE && 
			    att->opz_attnm.db_att_id == DB_IMTID
			) ||
			(   /* Modified to fix bug 33762 */
			    qstats->opc_tidkey != TRUE && 
			    att->opz_attnm.db_att_id != DB_IMTID &&
			    relatt->att_key_seq_number > 0
			)
		      )
		     )
		   )
		{
		    ret = FALSE;
		    break;
		}

		/* Now lets see if there's a constant node on the other side */
		if (root->pst_sym.pst_type == PST_UOP)
		{
		    cpm_flag = PST_PMNOTUSED;
		    cur_qlang = 
		       subqry->ops_root->pst_sym.pst_value.pst_s_root.pst_qlang;
		    qconst = NULL;
		}
		else if (qconst->pst_sym.pst_type == PST_CONST)
		{
		    cpm_flag = qconst->pst_sym.pst_value.pst_s_cnst.pst_pmspec;
		    cur_qlang = qconst->pst_sym.pst_value.pst_s_cnst.pst_cqlang;
		}
		else
		{
		    cpm_flag = PST_PMNOTUSED;
		    cur_qlang = 
		       subqry->ops_root->pst_sym.pst_value.pst_s_root.pst_qlang;
		    if (opc_cnst_expr(global, qconst, &cpm_flag, &cur_qlang)
								     == FALSE)
		    {
			ret = FALSE;
			break;
		    }
		}

		/* If there is pattern matching in this constant and this
		** is a hash table then lets avoid the christmas rush and
		** return this conjunct as not being usable for keying.
		*/
		if (cpm_flag == PST_PMUSED &&
			rel->rdr_rel->tbl_storage_type == DMT_HASH_TYPE
		    )
		{
		    ret = FALSE;
		    break;
		}

		/* Lets add the constant info to qstats */
		if (qconst != NULL)
		{
		    if (qconst->pst_sym.pst_type != PST_CONST ||
		        att->opz_dataval.db_datatype == DB_DTE_TYPE
			|| att->opz_dataval.db_datatype == -DB_DTE_TYPE
			|| att->opz_dataval.db_datatype == DB_ADTE_TYPE
			|| att->opz_dataval.db_datatype == -DB_ADTE_TYPE
			|| att->opz_dataval.db_datatype == DB_TMWO_TYPE
			|| att->opz_dataval.db_datatype == -DB_TMWO_TYPE
			|| att->opz_dataval.db_datatype == DB_TMW_TYPE 
			|| att->opz_dataval.db_datatype == -DB_TMW_TYPE
			|| att->opz_dataval.db_datatype == DB_TME_TYPE 
			|| att->opz_dataval.db_datatype == -DB_TME_TYPE 
			|| att->opz_dataval.db_datatype == DB_TSWO_TYPE 
			|| att->opz_dataval.db_datatype == -DB_TSWO_TYPE
			|| att->opz_dataval.db_datatype == DB_TSW_TYPE 
			|| att->opz_dataval.db_datatype == -DB_TSW_TYPE
			|| att->opz_dataval.db_datatype == DB_TSTMP_TYPE
			|| att->opz_dataval.db_datatype == -DB_TSTMP_TYPE
			|| att->opz_dataval.db_datatype == DB_INYM_TYPE
			|| att->opz_dataval.db_datatype == -DB_INYM_TYPE
			|| att->opz_dataval.db_datatype == DB_INDS_TYPE
			|| att->opz_dataval.db_datatype == -DB_INDS_TYPE) 
		    {
			/* expressions for key values are treated as
			** repeat query parameters in OPC keying, so let's
			** reserve space for it here.
			**
		        ** Constants of DATE datatype need to be treated as
		        ** repeat query parms as well to fix the problem
		        ** with time zones (Caroon & Black).
		        */
			/* Add multiple parameters in if there is a
			** compacted in-clause, since we need to use the
			** space of each of them later on (and do, anyway)
			** Bug 116558
			*/
		        qstats->opc_hiparm += (1 + incount);
		        qstats->opc_nparms += (1 + incount);
		    }
		    else
		    {
			/* Constant, but not a DATE */
			for (inlist = qconst; inlist; 
					inlist = inlist->pst_left)
			{
			    /* Count consts/parms inside possible IN-list. */
			    if (inlist->pst_sym.pst_value.pst_s_cnst.
						pst_tparmtype == PST_USER)
				qstats->opc_nconsts += (1 + incount);
			    else
				qstats->opc_nparms += (1 + incount);
			}
		    }
		}
		else
		{
		    qstats->opc_nconsts += 1;
		}

		/* Now lets find out what keybld says and set the parameter
		** matching flags.
		*/
		if (qstats->opc_tidkey == TRUE)
		{
		    kconst = &cqual->opc_kconsts[0];
		}
		else
		{
		    kconst = 
			&cqual->opc_kconsts[relatt->att_key_seq_number - 1];
		}
		if (qconst == NULL)
		{
		    kconst->opc_keyblk.adc_kdv.db_datatype = DB_NODT;
		    kconst->opc_keyblk.adc_kdv.db_length = 0;
		    kconst->opc_keyblk.adc_kdv.db_prec = 0;
		    kconst->opc_keyblk.adc_kdv.db_data = 0;
		}
		else
		{
		    STRUCT_ASSIGN_MACRO(qconst->pst_sym.pst_dataval,
			    kconst->opc_keyblk.adc_kdv);

		    /*
		    ** For repeat queries and database procedures
		    ** repeat query parameters and local variables,
		    ** when processed in adc_keybld (below) should
		    ** have db_data ptr set to NULL, to have the key
		    ** type determined correctly (bug 8438).
		    */
		    if (qconst->pst_sym.pst_type == PST_CONST &&
			(qconst->pst_sym.pst_value.pst_s_cnst.
			    pst_tparmtype == PST_RQPARAMNO
			 ||
			 qconst->pst_sym.pst_value.pst_s_cnst.
			    pst_tparmtype == PST_LOCALVARNO)
		       )
		    {
			kconst->opc_keyblk.adc_kdv.db_data = NULL;
		    }
		}
		kconst->opc_keyblk.adc_opkey = 
		    root->pst_sym.pst_value.pst_s_op.pst_opno;

		/* Save the low and high key data pointers and the adf
		** query lang. 
		*/
		save_lokey = kconst->opc_keyblk.adc_lokey.db_data;
		save_hikey = kconst->opc_keyblk.adc_hikey.db_data;
		kconst->opc_keyblk.adc_lokey.db_data = NULL;
		kconst->opc_keyblk.adc_hikey.db_data = NULL;
		save_qlang = global->ops_adfcb->adf_qlang;
		global->ops_adfcb->adf_qlang = cur_qlang;
		if (root->pst_sym.pst_value.pst_s_op.pst_pat_flags & AD_PAT_HAS_ESCAPE)
		    kconst->opc_keyblk.adc_escape = 
			root->pst_sym.pst_value.pst_s_op.pst_escape;
		
		kconst->opc_keyblk.adc_pat_flags =
			root->pst_sym.pst_value.pst_s_op.pst_pat_flags;

		/* Find out the type of keying we can do */
		if ((err = adc_keybld(global->ops_adfcb, &kconst->opc_keyblk))
								    != E_DB_OK)
		{
		    /* EJLFIX: get a better error no */
		    if (global->ops_adfcb->adf_errcb.ad_errcode !=
							    E_AD3002_BAD_KEYOP)
			opx_verror(err, E_OP0793_ADE_ENDCOMP,
				    global->ops_adfcb->adf_errcb.ad_errcode);
		    else
		    {
			ret = FALSE;
			break;
		    }
		}

		/* restore the low and high key data pointers and the adf
		** query lang.
		*/
		kconst->opc_keyblk.adc_lokey.db_data = save_lokey;
		kconst->opc_keyblk.adc_hikey.db_data = save_hikey;
		global->ops_adfcb->adf_qlang = save_qlang;
		kconst->opc_keyblk.adc_pat_flags = AD_PAT_DOESNT_APPLY;

		if ((kconst->opc_keyblk.adc_tykey != ADC_KEXACTKEY &&
			    kconst->opc_keyblk.adc_tykey != ADC_KNOMATCH) ||
		     cpm_flag == PST_PMUSED
		    )
		{
		    if (qstats->opc_tidkey == TRUE)
		        qstats->opc_inequality = 1;
		    else if (qstats->opc_inequality == 0 ||
			     qstats->opc_inequality > relatt->att_key_seq_number)
			qstats->opc_inequality = relatt->att_key_seq_number;

		    if (rel->rdr_rel->tbl_storage_type == DMT_HASH_TYPE ||
			qstats->opc_tidkey == TRUE ||
			kconst->opc_keyblk.adc_tykey == ADC_KALLMATCH ||
			kconst->opc_keyblk.adc_tykey == ADC_KNOKEY)
		    {
			ret = FALSE;
			break;
		    }
		}

		/* exit the for statement */
		break;

	    }	/* for */
	    break;

     case PST_OR:
     case PST_AND:
	ret = TRUE;
	if (root->pst_left != NULL)
	{
	    ret = opc_onlyatts(global, cnode, cqual, root->pst_left,
		varno, rel, qstats, ncompares);
	}
	if (root->pst_right != NULL && ret == TRUE)
	{
	    ret = opc_onlyatts(global, cnode, cqual, root->pst_right,
		varno, rel, qstats, ncompares);
	}
	break;

     case PST_QLEND:
	ret = TRUE;
	break;

     case PST_COP:
     case PST_CONST: 
        ret = FALSE; 
        break;

     default:
	opx_error(E_OP0682_UNEXPECTED_NODE);
	break;
    }

    return(ret);
}

/*{
** Name: OPC_CNST_EXPR	- Does a Qtree expression only use constants?
**
** Description:
**      This routine searches the given qtree and returns TRUE if only 
**      constants (and operators) are in the expression. Otherwise, the  
**      tree holds var nodes and FALSE will be returned. Also, we will let the
**      caller know if any of the constants hold pattern matching chars. 
[@comment_line@]...
**
** Inputs:
**  global -
**	The global state variable fow the current query.
**  root -
**	The root of the expression
**  pm_flag -
**  qlang -
**	Both are pointers to an initialized nat.
**
** Outputs:
**  pm_flag -
**	This will be set to PST_USED if a constant uses pattern matching chars
**	otherwise it will remain unchanged.
**  qlang -
**	This will be set to the query language used by the constant(s)
**	in 'root'
**
**	Returns:
**	    TRUE or FALSE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      3-oct-87 (eric)
**          written
[@history_template@]...
*/
i4
opc_cnst_expr(
		OPS_STATE	*global,
		PST_QNODE	*root,
		i4		*pm_flag,
		DB_LANG		*qlang)
{
    i4	    ret;

    if (root == NULL)
    {
	return(TRUE);	
    }

    if (root->pst_sym.pst_type == PST_VAR)
    {
	ret = FALSE;
    }
    else if (root->pst_sym.pst_type == PST_CONST)
    {
	ret = TRUE;

	*qlang = root->pst_sym.pst_value.pst_s_cnst.pst_cqlang;
	if (root->pst_sym.pst_value.pst_s_cnst.pst_pmspec == PST_PMUSED)
	{
	    *pm_flag = PST_PMUSED;
	}
    }
    else
    {
	ret = TRUE;

	if (root->pst_left != NULL)
	{
	    ret = opc_cnst_expr(global, root->pst_left, pm_flag, qlang);
	}
	if (ret == TRUE && root->pst_right != NULL)
	{
	    ret = opc_cnst_expr(global, root->pst_right, pm_flag, qlang);
	}
    }

    return(ret);
}

/*{
** Name: OPC_CVCLAUSE	- Convert a Qtree clause into an OPC_QAND struct
**
** Description:
**      Opc_cvclause() converts a query tree clause into an OPC_QAND struct.
**      In practice, the qtree that is passed in is a clause from the users 
**      qualification that opc_onlyatts() has said is usefull for keying.
**        
**      We are converting the qtree to allow opc_range() to figure out how
**      to key into relations more efficiently. You might look at the OPC_QAND
**      struct (which you should) and wonder why we bother to take the time 
**      and space to convert the tree. After all, wouldn't it be easier and 
**      faster to make opc_range() work on qtrees rather than OPC_QAND structs? 
**      Well, if opc_range() were to work on qtrees then it would have to be 
**      extremely recursive, requiring many, many passes over the qtree.
**      Since opc_range() works on OPC_QAND structs, it isn't recursive and 
**      we save many procedure calls. As for the space required for the 
**      OPC_QAND struct, the same amount, or more, would be required be the 
**	stack space of the recursive version.
**        
**      The pseudo-code for this routine is: 
**       
**      opc_cvclause()
**      { 
**	    if (root is an 'and' or an 'or' node)
**	    { 
**		call opc_cvclause() on the right and left side of root;
**	    } 
**	    else if (root is a bop or a uop node)
**	    { 
**		Find the var and const node on the left and/or right of the 
**		    of the op node; 
**		Figure out what key number the var node refers to;
**        
**		Allocate memory for the low and high key; 
**		Ask ADF to build the key for us; 
**		  
**		If (the constant is a repeat query parameter) 
**		{ 
**		    Place the key value information on the repeat query list 
**			for the current keyno, keeping the list sorted as we go;
**		} 
**		else 
**		{ 
**		    Place the key value information on the normal (non-repeat 
**			query) list for the current keyno, keeping the list
**			sorted as we go;
**		} 
**	    }
**      } 
[@comment_line@]...
**
** Inputs:
**  global -
**	A pointer to the global state information for the current query. This
**	is used mostly for getting memory and calling ADF.
**  cnode -
**	A pointer to information about the current CO/QEN node being compiled.
**	This is used primarily to determine what attributes are available for
**	which eqcs.
**  cqual -
**	A point to the OPC keying qualification that is being built. This is
**	used mostly to know whether we are using the primary or tid key for
**	the current relation. It is also used for output, see below.
**  cnf_and -
**	cnf_and points to an initialized OPC_QAND struct. 
**  root -
**	This points to the root of a qtree that should be converted.
**  rel -
**	This is a pointer to a relation description for the relation that is
**	being keyed into.
**
** Outputs:
**  cqual -
**	New key values are added to the key attribute lists (ie. opc_qconsts,
**	opc_qparms, and opc_dbpvars)
**  cnf_and -
**	This holds the converted qtree. The opc_curcomp entry in the opc_comps
**	array is fill in with the info for the current operation being
**	performed. Also, opc_curcomp will be incremented to indicate that 
**	another entry in opc_comps has been used.
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      1-aug-87 (eric)
**          written
**      1-sep-88 (seputis)
**          fix unix linting problems, bugs in escape code
**      25-nov-88 (eric)
**          Fixed a problem where the code was assuming that the constant
**          would be on the right hand side of the query tree.
**	17-may-90 (stec)
**	    Fixed bug 8438 related to building keys for repeat queries.
**	    Db_data ptr is set to null for constant nodes representing
**	    repeat query parms and local vars in database procedures.
**	26-nov-90 (stec)
**	    Made changes for the Caroon & Black problem. The idea is that
**	    all qualifications containing attributes of the DATE datatype
**	    be handled at runtime by ADF, so we treat them pretty much as
**	    repeat query parms or database procedure parameters.
**	06-dec-90 (stec)
**	    Fixed problem related to Caroon & Black code modification showing
**	    as an AV when processing database procedure definition in the  test
**	    script b8629_proc.qry (test suite).
**	06-feb-91 (stec)
**	    Modified code to fix timezone related bug #35790. Constants of DATE
**	    datatype related to key attributes need to be processed in the same
**	    way as expressions.
**	19-nov-96 (inkdo01)
**	    Changes to support "<>" (NE) predicates in key structures, due to
**	    NE histogram selectivity support.
**	17-dec-03 (inkdo01)
**	    Changes to support compacted IN-list structure.
**	8-jan-03 (inkdo01)
**	    Added goto to keep RQPARAMs in the loop.
**	17-jul-2006 (gupsh01)
**	    Added support for ANSI datetime datatypes.
**	23-oct-2006 (gupsh01)
**	    Added missing case for DB_ADTE_TYPE.
**	25-Jun-2008 (kiria01) SIR120473
**	    Move pst_isescape into new u_i2 pst_pat_flags.
**	03-Nov-2009 (kiria01) b122822
**	    Pre- and post-sort PST_INLIST constants to ease the sorted insert
**	    as otherwise ascending sorted data is the worst case.
**	02-Dec-2009 (kiria01) b122952
**	    Build real range key for sorted inlist if over the threshold.
**      29-jul-2010 (huazh01)
**          reset opn_ncomps if we build a range key. (b124150)
[@history_template@]...
*/
static VOID
opc_cvclause(
	OPS_STATE	*global,
	OPC_NODE	*cnode,
	OPC_QUAL	*cqual,
	OPC_QAND	*cnf_and,
	PST_QNODE	*root,
	RDR_INFO	*rel )
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    OPC_QCMP		*cnf_comp;
    OPC_QCONST		**kqconst;
    DB_DATA_VALUE	sortdv;
    i4			sort_comp;
    PST_QNODE		*qvar;
    PST_QNODE		*qconst;
    PST_QNODE		*origrhs = root->pst_right;
    i4			attno;
    OPZ_ATTS		*att;
    OPC_QCONST		*new_qconst;
    OPC_KCONST		*kconst;
    DB_STATUS		err;
    PTR			save_lokey;
    PTR			save_hikey;
    DB_LANG		cur_qlang;
    DB_LANG		save_qlang;
    i4			cpm_flag;
    ADI_FI_DESC		*cmplmnt_fidesc;
    i4			parmno;
    ADI_OP_ID		opid;

    switch (root->pst_sym.pst_type)
    {
     case PST_BOP:
	if (global->ops_cb->ops_alter.ops_inlist_thresh &&
		(root->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLISTSORT))
	{
	    PST_QNODE *p = root->pst_right;
	    i4 cnt = 1;
	    while (p->pst_left)
	    {
		p = p->pst_left;
		cnt++;
	    }
	    if (cnt > global->ops_cb->ops_alter.ops_inlist_thresh)
	    {
		/* We're expected to build a range key */
		cnf_and->opc_ncomps = cnf_and->opc_curcomp += 1;
		cnf_comp = &cnf_and->opc_comps[cnf_and->opc_curcomp];

		qvar = root->pst_left;
		qconst = root->pst_right;

		cpm_flag = qconst->pst_sym.pst_value.pst_s_cnst.pst_pmspec;
		cur_qlang = qconst->pst_sym.pst_value.pst_s_cnst.pst_cqlang;

		attno = qvar->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
		att = subqry->ops_attrs.opz_base->opz_attnums[attno];
		attno = cnode->opc_ceq[att->opz_equcls].opc_attno;

		/* lets find out what the key number we're dealing with is */
		if (cqual->opc_qsinfo.opc_tidkey == TRUE)
		{
		    cnf_comp->opc_keyno = 0;
		}
		else
		{
		    if (cnode->opc_ceq[att->opz_equcls].opc_eqavailable == FALSE)
		    {
			opx_error(E_OP0889_EQ_NOT_HERE);
		    }

		    if (attno != OPC_NOATT)
		    {
			att = subqry->ops_attrs.opz_base->opz_attnums[attno];
			cnf_comp->opc_keyno =  rel->rdr_attr[att->
			    opz_attnm.db_att_id]->att_key_seq_number - 1;
		    }
		    else
		    {
			/* ASK_ERIC */
			cnf_comp->opc_keyno = 0;
		    }
		}

		opid = root->pst_sym.pst_value.pst_s_op.pst_opno;

		/* lets finish filling in the key block. */
		kconst = &cqual->opc_kconsts[cnf_comp->opc_keyno];

		/* For all cases for which the key needs to be build
		** at run time just find out the type of key.
		*/
		/* allocate memory to for the low and high key */
		if (kconst->opc_keyblk.adc_lokey.db_data == NULL)
		{
		    kconst->opc_keyblk.adc_lokey.db_data =
			(PTR)opu_Gsmemory_get(global, 
			    kconst->opc_keyblk.adc_lokey.db_length);
		}
		if (kconst->opc_keyblk.adc_hikey.db_data == NULL)
		{
		    kconst->opc_keyblk.adc_hikey.db_data =
			(PTR)opu_Gsmemory_get(global, 
				kconst->opc_keyblk.adc_hikey.db_length);
		}

		/* Now lets ask ADF to build the high and low keys for us */
		STRUCT_ASSIGN_MACRO(qconst->pst_sym.pst_dataval,
			kconst->opc_keyblk.adc_kdv);
		kconst->opc_keyblk.adc_opkey = ADI_GE_OP;
		save_qlang = global->ops_adfcb->adf_qlang;
		global->ops_adfcb->adf_qlang = cur_qlang;
		if (root->pst_sym.pst_value.pst_s_op.pst_pat_flags & AD_PAT_HAS_ESCAPE)
		    kconst->opc_keyblk.adc_escape = 
			root->pst_sym.pst_value.pst_s_op.pst_escape;
		kconst->opc_keyblk.adc_pat_flags =
			root->pst_sym.pst_value.pst_s_op.pst_pat_flags;

		if ((err = adc_keybld(global->ops_adfcb, 
			    &kconst->opc_keyblk)) != E_DB_OK)
		{
		    /* EJLFIX: get a better error no */
		    opx_verror(err, E_OP0793_ADE_ENDCOMP,
			global->ops_adfcb->adf_errcb.ad_errcode);
		}
		STRUCT_ASSIGN_MACRO(p->pst_sym.pst_dataval,
			kconst->opc_keyblk.adc_kdv);
		kconst->opc_keyblk.adc_opkey = ADI_LE_OP;
		global->ops_adfcb->adf_qlang = cur_qlang;
		if (root->pst_sym.pst_value.pst_s_op.pst_pat_flags & AD_PAT_HAS_ESCAPE)
		    kconst->opc_keyblk.adc_escape = 
			root->pst_sym.pst_value.pst_s_op.pst_escape;
		kconst->opc_keyblk.adc_pat_flags =
			root->pst_sym.pst_value.pst_s_op.pst_pat_flags;

		if ((err = adc_keybld(global->ops_adfcb, 
			    &kconst->opc_keyblk)) != E_DB_OK)
		{
		    /* EJLFIX: get a better error no */
		    opx_verror(err, E_OP0793_ADE_ENDCOMP,
			global->ops_adfcb->adf_errcb.ad_errcode);
		}
		global->ops_adfcb->adf_qlang = save_qlang;

		kconst->opc_keyblk.adc_pat_flags = AD_PAT_DOESNT_APPLY;
		kconst->opc_keyblk.adc_tykey = ADC_KRANGEKEY;

		/* Lets put the low key on the 
		** key list and set cnf_comp->opc_lo_const
		*/
		STRUCT_ASSIGN_MACRO(kconst->opc_keyblk.adc_lokey, sortdv);
		for (kqconst = &kconst->opc_qconsts; 
		     /* No exit condition here */;
		     kqconst = &(*kqconst)->opc_qnext
		    )
		{
		    if (*kqconst == NULL)
		    {
			sort_comp = -1;
		    }
		    else
		    {
			sortdv.db_data = (*kqconst)->opc_keydata.opc_data.db_data;
			if ((err = adc_compare(global->ops_adfcb, 
				&kconst->opc_keyblk.adc_lokey,
				&sortdv, &sort_comp)) != E_AD0000_OK)
			{
			    opx_verror(err, E_OP078C_ADC_COMPARE,
				global->ops_adfcb->adf_errcb.ad_errcode);
			}
		    }

		    if (sort_comp == 0)
		    {
			cnf_comp->opc_lo_const = *kqconst;
			break;
		    }
		    else if (sort_comp < 0)
		    {
			new_qconst = (OPC_QCONST *)
				    opu_Gsmemory_get(global, sizeof (OPC_QCONST));
			STRUCT_ASSIGN_MACRO(kconst->opc_keyblk.adc_lokey,
						new_qconst->opc_keydata.opc_data);
			new_qconst->opc_qnext = *kqconst;
			new_qconst->opc_tconst_type = PST_USER;
			new_qconst->opc_const_no = 0;
			new_qconst->opc_qlang = cur_qlang;
			new_qconst->opc_qop = root;
			new_qconst->opc_dshlocation = -1;
			new_qconst->opc_key_type = ADC_KNOKEY;

			*kqconst = new_qconst;
			kconst->opc_keyblk.adc_lokey.db_data = NULL;
			cnf_comp->opc_lo_const = new_qconst;
			break;
		    }
		}

		/* Put the high on the key list and set cnf_comp->opc_hi_const */
		/* Note that we can starting looking in the key list
		** where we left off for the low key, since the hi
		** key won't be lower than the low key.
		*/
		for ( ; ; kqconst = &(*kqconst)->opc_qnext)
		{
		    if (*kqconst == NULL)
		    {
			sort_comp = -1;
		    }
		    else
		    {
			sortdv.db_data = 
				(*kqconst)->opc_keydata.opc_data.db_data;
			if ((err = adc_compare(global->ops_adfcb, 
				&kconst->opc_keyblk.adc_hikey,
				&sortdv, &sort_comp)) != E_AD0000_OK
			    )
			{
			    opx_verror(err, E_OP078C_ADC_COMPARE,
				global->ops_adfcb->adf_errcb.ad_errcode);
			}
		    }
		    if (sort_comp == 0)
		    {
			cnf_comp->opc_hi_const = *kqconst;
			break;
		    }
		    else if (sort_comp < 0)
		    {
			new_qconst = (OPC_QCONST *)
				opu_Gsmemory_get(global, sizeof (OPC_QCONST));
			STRUCT_ASSIGN_MACRO(
				kconst->opc_keyblk.adc_hikey,
				new_qconst->opc_keydata.opc_data);
			new_qconst->opc_qnext = *kqconst;
			new_qconst->opc_tconst_type = PST_USER;
			new_qconst->opc_const_no = 0;
			new_qconst->opc_qlang = cur_qlang;
			new_qconst->opc_qop = root;
			new_qconst->opc_dshlocation = -1;
			new_qconst->opc_key_type = ADC_KNOKEY;

			*kqconst = new_qconst;
			kconst->opc_keyblk.adc_hikey.db_data = NULL;
			cnf_comp->opc_hi_const = new_qconst;
			break;
		    }
		}

		break;
	    }
	}
	if ((root->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLIST) &&
		root->pst_right->pst_sym.pst_type == PST_CONST)
	{
	    /* We are likely to be about to insert a sorted list into a
	    ** sorted list that will be built in the same order and
	    ** sequentially scanned so to make the loop O(n) instead of
	    ** O(n*n) we reverse and at end reverse back. */
	    PST_QNODE *p = root->pst_right;
	    root->pst_right = NULL;
	    while (p)
	    {
		PST_QNODE *t = p->pst_left;
		p->pst_left = root->pst_right;
		root->pst_right = p;
		p = t;
	    }
	    origrhs = root->pst_right; /* This is what we'll need restoring */
	}
	/*FALLTHROUGH*/
     case PST_UOP:
	while (TRUE) {	/* loop for processing constant lists of
			** new compacted IN-list structure */
	    cnf_and->opc_curcomp += 1;
	    cnf_comp = &cnf_and->opc_comps[cnf_and->opc_curcomp];

	    /* Lets find the var node on one side */
	    if (root->pst_left != NULL &&
		root->pst_left->pst_sym.pst_type == PST_VAR)
	    {
		qvar = root->pst_left;
		qconst = root->pst_right;
	    }
	    else if (root->pst_right != NULL &&
		     root->pst_right->pst_sym.pst_type == PST_VAR)
	    {
		qvar = root->pst_right;
		qconst = root->pst_left;
	    }

	    /* Now lets find the constant node on the other side */
	    if (root->pst_sym.pst_type == PST_UOP)
	    {
		qconst = NULL;
		cpm_flag = PST_PMNOTUSED;
		cur_qlang = 
		    subqry->ops_root->pst_sym.pst_value.pst_s_root.pst_qlang;
	    }
	    else if (qconst->pst_sym.pst_type == PST_CONST)
	    {
		cpm_flag = qconst->pst_sym.pst_value.pst_s_cnst.pst_pmspec;
		cur_qlang = qconst->pst_sym.pst_value.pst_s_cnst.pst_cqlang;
	    }
	    else
	    {
		cpm_flag = PST_PMNOTUSED;
		cur_qlang = 
		       subqry->ops_root->pst_sym.pst_value.pst_s_root.pst_qlang;
		(VOID) opc_cnst_expr(global, qconst, &cpm_flag, &cur_qlang);
		/* FIXME - EAS should this value be ignored ?? */
	    }

	    attno = qvar->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
	    att = subqry->ops_attrs.opz_base->opz_attnums[attno];
	    attno = cnode->opc_ceq[att->opz_equcls].opc_attno;

	    /* lets find out what the key number we're dealing with is */
	    if (cqual->opc_qsinfo.opc_tidkey == TRUE)
	    {
		cnf_comp->opc_keyno = 0;
	    }
	    else
	    {
		if (cnode->opc_ceq[att->opz_equcls].opc_eqavailable == FALSE)
		{
		    opx_error(E_OP0889_EQ_NOT_HERE);
		}

		if (attno != OPC_NOATT)
		{
		    att = subqry->ops_attrs.opz_base->opz_attnums[attno];
		    cnf_comp->opc_keyno =  rel->rdr_attr[att->
			opz_attnm.db_att_id]->att_key_seq_number - 1;
		}
		else
		{
		    /* ASK_ERIC */
		    cnf_comp->opc_keyno = 0;
		}
	    }

	    /* For keying purposes we assume that qualifications will always
	    ** have the VAR node as the left child of the operator node,
	    ** thus if that is not true, we need to find the compliment of
	    ** the function id involved.
	    */
	    if (qvar == root->pst_left)
	    {
		opid = root->pst_sym.pst_value.pst_s_op.pst_opno;
	    }
	    else
	    {
		opid = root->pst_sym.pst_value.pst_s_op.pst_opno;

		/* If this isn't an equals or not equals operator, then we
		** need to find the compliment of this operator to normalize
		** this comparison to att left and value right.
		*/
		if (opid != global->ops_cb->ops_server->opg_eq &&
		    opid != global->ops_cb->ops_server->opg_ne)
		{
		    err = adi_fidesc(global->ops_adfcb, 
			root->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_cmplmnt,
			&cmplmnt_fidesc);

		    if (err != E_DB_OK)
		    {
			opx_verror(err, E_OP0781_ADI_FIDESC, 
			    global->ops_adfcb->adf_errcb.ad_errcode);
		    }
		    opid = cmplmnt_fidesc->adi_fiopid;
		}
	    }

	    /* lets finish filling in the key block. */
	    kconst = &cqual->opc_kconsts[cnf_comp->opc_keyno];
	    if (root->pst_sym.pst_type == PST_UOP)
	    {
		kconst->opc_nuops += 1;
	    }

	    if (qconst == NULL)
	    {
		kconst->opc_keyblk.adc_kdv.db_datatype = DB_NODT;
		kconst->opc_keyblk.adc_kdv.db_length = 0;
		kconst->opc_keyblk.adc_kdv.db_prec = 0;
		kconst->opc_keyblk.adc_kdv.db_data = 0;
	    }
	    else
	    {
		STRUCT_ASSIGN_MACRO(qconst->pst_sym.pst_dataval,
			kconst->opc_keyblk.adc_kdv);

		/*
		** For repeat queries and database procedures
		** repeat query parameters and local variables,
		** when processed in adc_keybld (below) should
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
		    kconst->opc_keyblk.adc_kdv.db_data = NULL;
		}
	    }
	    kconst->opc_keyblk.adc_opkey = opid;

	    /* For all cases for which the key needs to be build
	    ** at run time just find out the type of key.
	    */
	    if (qconst != NULL &&
		    (qconst->pst_sym.pst_type != PST_CONST
		     ||
		     qconst->pst_sym.pst_dataval.db_data == NULL
		     ||
		     att->opz_dataval.db_datatype == DB_DTE_TYPE
		     ||
		     att->opz_dataval.db_datatype == -DB_DTE_TYPE
		     || 
		     att->opz_dataval.db_datatype == DB_ADTE_TYPE
		     || 
		     att->opz_dataval.db_datatype == -DB_ADTE_TYPE
		     || 
		     att->opz_dataval.db_datatype == DB_TMWO_TYPE
		     || 
		     att->opz_dataval.db_datatype == -DB_TMWO_TYPE
		     ||
		     att->opz_dataval.db_datatype == DB_TMW_TYPE 
		     || 
		     att->opz_dataval.db_datatype == -DB_TMW_TYPE
		     || 
		     att->opz_dataval.db_datatype == DB_TME_TYPE 
		     || 
		     att->opz_dataval.db_datatype == -DB_TME_TYPE
		     || 
		     att->opz_dataval.db_datatype == DB_TSWO_TYPE 
		     || 
		     att->opz_dataval.db_datatype == -DB_TSWO_TYPE
		     || 
		     att->opz_dataval.db_datatype == DB_TSW_TYPE 
		     || 
		     att->opz_dataval.db_datatype == -DB_TSW_TYPE
		     || 
		     att->opz_dataval.db_datatype == DB_TSTMP_TYPE
		     || 
		     att->opz_dataval.db_datatype == -DB_TSTMP_TYPE
		     || 
		     att->opz_dataval.db_datatype == DB_INYM_TYPE
		     || 
		     att->opz_dataval.db_datatype == -DB_INYM_TYPE
		     || 
		     att->opz_dataval.db_datatype == DB_INDS_TYPE
		     || 
		     att->opz_dataval.db_datatype == -DB_INDS_TYPE) 
		)
	    {
		save_lokey = kconst->opc_keyblk.adc_lokey.db_data;
		save_hikey = kconst->opc_keyblk.adc_hikey.db_data;
		kconst->opc_keyblk.adc_lokey.db_data = NULL;
		kconst->opc_keyblk.adc_hikey.db_data = NULL;
	    }
	    else
	    {
		/* allocate memory to for the low and high key */
		if (kconst->opc_keyblk.adc_lokey.db_data == NULL)
		{
		    kconst->opc_keyblk.adc_lokey.db_data =
			(PTR)opu_Gsmemory_get(global, 
			    kconst->opc_keyblk.adc_lokey.db_length);
		}
		if (kconst->opc_keyblk.adc_hikey.db_data == NULL)
		{
		    kconst->opc_keyblk.adc_hikey.db_data =
			(PTR)opu_Gsmemory_get(global, 
				kconst->opc_keyblk.adc_hikey.db_length);
		}
	    }

	    /* Now lets ask ADF to build the high and low keys for us */
	    save_qlang = global->ops_adfcb->adf_qlang;
	    global->ops_adfcb->adf_qlang = cur_qlang;
	    if (root->pst_sym.pst_value.pst_s_op.pst_pat_flags & AD_PAT_HAS_ESCAPE)
		kconst->opc_keyblk.adc_escape = 
		    root->pst_sym.pst_value.pst_s_op.pst_escape;
	    kconst->opc_keyblk.adc_pat_flags =
		    root->pst_sym.pst_value.pst_s_op.pst_pat_flags;

	    if ((err = adc_keybld(global->ops_adfcb, 
			&kconst->opc_keyblk)) != E_DB_OK)
	    {
		/* EJLFIX: get a better error no */
		opx_verror(err, E_OP0793_ADE_ENDCOMP,
		    global->ops_adfcb->adf_errcb.ad_errcode);
	    }
	    global->ops_adfcb->adf_qlang = save_qlang;
	    kconst->opc_keyblk.adc_pat_flags = AD_PAT_DOESNT_APPLY;

	    /* For all cases for which the key needs to be 
	    ** build at run time restore saved values.
	    */
	    if (qconst != NULL &&
		    (qconst->pst_sym.pst_type != PST_CONST
		     ||
		     qconst->pst_sym.pst_dataval.db_data == NULL
		     ||
		     att->opz_dataval.db_datatype == DB_DTE_TYPE
		     ||
		     att->opz_dataval.db_datatype == -DB_DTE_TYPE
		     || 
		     att->opz_dataval.db_datatype == DB_ADTE_TYPE
		     || 
		     att->opz_dataval.db_datatype == -DB_ADTE_TYPE
		     || 
		     att->opz_dataval.db_datatype == DB_TMWO_TYPE
		     || 
		     att->opz_dataval.db_datatype == -DB_TMWO_TYPE
		     ||
		     att->opz_dataval.db_datatype == DB_TMW_TYPE 
		     || 
		     att->opz_dataval.db_datatype == -DB_TMW_TYPE
		     || 
		     att->opz_dataval.db_datatype == DB_TME_TYPE 
		     || 
		     att->opz_dataval.db_datatype == -DB_TME_TYPE
		     || 
		     att->opz_dataval.db_datatype == DB_TSWO_TYPE 
		     || 
		     att->opz_dataval.db_datatype == -DB_TSWO_TYPE
		     || 
		     att->opz_dataval.db_datatype == DB_TSW_TYPE 
		     || 
		     att->opz_dataval.db_datatype == -DB_TSW_TYPE
		     || 
		     att->opz_dataval.db_datatype == DB_TSTMP_TYPE
		     || 
		     att->opz_dataval.db_datatype == -DB_TSTMP_TYPE
		     || 
		     att->opz_dataval.db_datatype == DB_INYM_TYPE
		     || 
		     att->opz_dataval.db_datatype == -DB_INYM_TYPE
		     || 
		     att->opz_dataval.db_datatype == DB_INDS_TYPE
		     || 
		     att->opz_dataval.db_datatype == -DB_INDS_TYPE) 
		)
	    {
		kconst->opc_keyblk.adc_lokey.db_data = save_lokey;
		kconst->opc_keyblk.adc_hikey.db_data = save_hikey;
	    }

	    /* For all cases for which the key
	    ** needs to be build at run time.
	    */
	    if (qconst != NULL &&
		(qconst->pst_sym.pst_type != PST_CONST
		 ||
		 qconst->pst_sym.pst_value.pst_s_cnst.pst_tparmtype != PST_USER
		 ||
		 att->opz_dataval.db_datatype == DB_DTE_TYPE
		 ||
		 att->opz_dataval.db_datatype == -DB_DTE_TYPE
		 ||
		 att->opz_dataval.db_datatype == DB_ADTE_TYPE
		 ||
		 att->opz_dataval.db_datatype == -DB_ADTE_TYPE
		 || 
		 att->opz_dataval.db_datatype == DB_TMWO_TYPE
		 || 
		 att->opz_dataval.db_datatype == -DB_TMWO_TYPE
		 ||
		 att->opz_dataval.db_datatype == DB_TMW_TYPE 
		 || 
		 att->opz_dataval.db_datatype == -DB_TMW_TYPE
		 || 
		 att->opz_dataval.db_datatype == DB_TME_TYPE 
		 || 
		 att->opz_dataval.db_datatype == -DB_TME_TYPE
		 || 
		 att->opz_dataval.db_datatype == DB_TSWO_TYPE 
		 || 
		 att->opz_dataval.db_datatype == -DB_TSWO_TYPE
		 || 
		 att->opz_dataval.db_datatype == DB_TSW_TYPE 
		 || 
		 att->opz_dataval.db_datatype == -DB_TSW_TYPE
		 || 
		 att->opz_dataval.db_datatype == DB_TSTMP_TYPE
		 || 
		 att->opz_dataval.db_datatype == -DB_TSTMP_TYPE
		 || 
		 att->opz_dataval.db_datatype == DB_INYM_TYPE
		 || 
		 att->opz_dataval.db_datatype == -DB_INYM_TYPE
		 || 
		 att->opz_dataval.db_datatype == DB_INDS_TYPE
		 || 
		 att->opz_dataval.db_datatype == -DB_INDS_TYPE) 
		)
	    {
		if (global->ops_procedure->pst_isdbp == TRUE)
		{
		    kqconst = kconst->opc_dbpvars; 
		}
		else
		{
		    kqconst = kconst->opc_qparms; 
		}

		/*
		** Put the key value information on the 
		** repeat query list for the current keyno
		** Fix for bug number 35790.
		*/
		if (qconst->pst_sym.pst_type != PST_CONST ||
		    (qconst->pst_sym.pst_type == PST_CONST &&
		     qconst->pst_sym.pst_value.pst_s_cnst.pst_tparmtype 
							!= PST_RQPARAMNO &&
		     (att->opz_dataval.db_datatype == DB_DTE_TYPE ||
		      att->opz_dataval.db_datatype == -DB_DTE_TYPE
			|| att->opz_dataval.db_datatype == -DB_DTE_TYPE
			|| att->opz_dataval.db_datatype == DB_ADTE_TYPE
			|| att->opz_dataval.db_datatype == -DB_ADTE_TYPE
			|| att->opz_dataval.db_datatype == DB_TMWO_TYPE
			|| att->opz_dataval.db_datatype == -DB_TMWO_TYPE
			|| att->opz_dataval.db_datatype == DB_TMW_TYPE 
			|| att->opz_dataval.db_datatype == -DB_TMW_TYPE
			|| att->opz_dataval.db_datatype == DB_TME_TYPE 
			|| att->opz_dataval.db_datatype == -DB_TME_TYPE
			|| att->opz_dataval.db_datatype == DB_TSWO_TYPE 
			|| att->opz_dataval.db_datatype == -DB_TSWO_TYPE
			|| att->opz_dataval.db_datatype == DB_TSW_TYPE 
			|| att->opz_dataval.db_datatype == -DB_TSW_TYPE
			|| att->opz_dataval.db_datatype == DB_TSTMP_TYPE
			|| att->opz_dataval.db_datatype == -DB_TSTMP_TYPE
			|| att->opz_dataval.db_datatype == DB_INYM_TYPE
			|| att->opz_dataval.db_datatype == -DB_INYM_TYPE
			|| att->opz_dataval.db_datatype == DB_INDS_TYPE
			|| att->opz_dataval.db_datatype == -DB_INDS_TYPE) 
		    )
		   )
		{
		    /*	
		    ** This statement block is executed for expressions
		    ** and user specified constants associated with key
		    ** attributes of DATE datatype that are not repeat
		    ** query parms.
		    */
		    new_qconst = (OPC_QCONST *)
			    opu_Gsmemory_get(global, sizeof (OPC_QCONST));
		    new_qconst->opc_tconst_type = PST_RQPARAMNO;
		    new_qconst->opc_const_no = cqual->opc_qsinfo.opc_hiparm;
		    new_qconst->opc_keydata.opc_pstnode = qconst;
		    new_qconst->opc_qlang = cur_qlang;
		    new_qconst->opc_qnext = kqconst[0];
		    new_qconst->opc_qop = root;
		    new_qconst->opc_dshlocation = -1;
		    if (cpm_flag == PST_PMUSED &&
			kconst->opc_keyblk.adc_tykey == ADC_KEXACTKEY)
		    {
			new_qconst->opc_key_type = ADC_KRANGEKEY;
		    }
		    else
 		    {
			new_qconst->opc_key_type = 
				kconst->opc_keyblk.adc_tykey;
		    }

		    cqual->opc_qsinfo.opc_hiparm += 1;
		    kqconst[new_qconst->opc_const_no] = new_qconst;
		}
		else
		{
		    parmno = qconst->pst_sym.pst_value.pst_s_cnst.pst_parm_no;
		    if (kqconst[parmno] == NULL)
		    {
			new_qconst = (OPC_QCONST *)
				opu_Gsmemory_get(global, sizeof (OPC_QCONST));
			new_qconst->opc_tconst_type = 
			    qconst->pst_sym.pst_value.pst_s_cnst.pst_tparmtype;
			new_qconst->opc_const_no = 
			    qconst->pst_sym.pst_value.pst_s_cnst.pst_parm_no;
			new_qconst->opc_keydata.opc_pstnode = qconst;
			new_qconst->opc_qlang = cur_qlang;
			new_qconst->opc_qnext = NULL;
			new_qconst->opc_qop = root;
			new_qconst->opc_dshlocation = -1;
			if (cpm_flag == PST_PMUSED &&
			    kconst->opc_keyblk.adc_tykey == ADC_KEXACTKEY)
			{
			    new_qconst->opc_key_type = ADC_KRANGEKEY;
			}
			else
			{
			    new_qconst->opc_key_type = 
				    kconst->opc_keyblk.adc_tykey;
			}

			kqconst[parmno] = new_qconst;
		    }
		    else
		    {
			new_qconst = kqconst[parmno];
		    }
		}

		switch (kconst->opc_keyblk.adc_tykey)
		{
		 case ADC_KNOMATCH:
		 case ADC_KEXACTKEY:
		 case ADC_KRANGEKEY:
		 case ADC_KNOKEY:
		 case ADC_KALLMATCH:
		 default:
		    if (kconst->opc_keyblk.adc_tykey == ADC_KNOKEY &&
			kconst->opc_keyblk.adc_opkey == ADI_NE_OP)
		      cnf_comp->opc_lo_const = NULL;
		    else
		      cnf_comp->opc_lo_const = new_qconst;
		    cnf_comp->opc_hi_const = new_qconst;
		    break;

		 case ADC_KHIGHKEY:
		    cnf_comp->opc_lo_const = NULL;
		    cnf_comp->opc_hi_const = new_qconst;
		    break;

		 case ADC_KLOWKEY:
		    cnf_comp->opc_lo_const = new_qconst;
		    cnf_comp->opc_hi_const = NULL;
		    break;
		}
		goto inlist_break;
	    }

	    /* At this point, we know that we aren't dealing with a
	    ** repeat query parameter. So, lets put the low key on the 
	    ** key list and set cnf_comp->opc_lo_const
	    */
	    STRUCT_ASSIGN_MACRO(kconst->opc_keyblk.adc_lokey, sortdv);
	    for (kqconst = &kconst->opc_qconsts; 
		 /* No exit condition here */;
		 kqconst = &(*kqconst)->opc_qnext
		)
	    {
		if (*kqconst == NULL)
		{
		    sort_comp = -1;
		}
		else
		{
		    sortdv.db_data = (*kqconst)->opc_keydata.opc_data.db_data;
		    if ((err = adc_compare(global->ops_adfcb, 
			    &kconst->opc_keyblk.adc_lokey,
			    &sortdv, &sort_comp)) != E_AD0000_OK)
		    {
			opx_verror(err, E_OP078C_ADC_COMPARE,
			    global->ops_adfcb->adf_errcb.ad_errcode);
		    }
		}

		if (sort_comp == 0)
		{
		    cnf_comp->opc_lo_const = *kqconst;
		    break;
		}
		else if (sort_comp < 0)
		{
		    new_qconst = (OPC_QCONST *)
				opu_Gsmemory_get(global, sizeof (OPC_QCONST));
		    STRUCT_ASSIGN_MACRO(kconst->opc_keyblk.adc_lokey,
					    new_qconst->opc_keydata.opc_data);
		    new_qconst->opc_qnext = *kqconst;
		    new_qconst->opc_tconst_type = PST_USER;
		    new_qconst->opc_const_no = 0;
		    new_qconst->opc_qlang = cur_qlang;
		    new_qconst->opc_qop = root;
		    new_qconst->opc_dshlocation = -1;
		    new_qconst->opc_key_type = ADC_KNOKEY;

		    *kqconst = new_qconst;
		    kconst->opc_keyblk.adc_lokey.db_data = NULL;
		    cnf_comp->opc_lo_const = new_qconst;
		    break;
		}
	    }

	    /* Put the high on the key list and set cnf_comp->opc_hi_const */
	    if (kconst->opc_keyblk.adc_tykey == ADC_KEXACTKEY ||
		kconst->opc_keyblk.adc_tykey == ADC_KNOMATCH)
	    {
		cnf_comp->opc_hi_const = cnf_comp->opc_lo_const;
	    }
	    else
	    {
		/* Note that we can starting looking in the key list
		** where we left off for the low key, since the hi
		** key won't be lower than the low key.
		*/
		for ( ; ; kqconst = &(*kqconst)->opc_qnext)
		{
		    if (*kqconst == NULL)
		    {
			sort_comp = -1;
		    }
		    else
		    {
			sortdv.db_data = 
				(*kqconst)->opc_keydata.opc_data.db_data;
			if ((err = adc_compare(global->ops_adfcb, 
				&kconst->opc_keyblk.adc_hikey,
				&sortdv, &sort_comp)) != E_AD0000_OK
			    )
			{
			    opx_verror(err, E_OP078C_ADC_COMPARE,
				global->ops_adfcb->adf_errcb.ad_errcode);
			}
		    }
		    if (sort_comp == 0)
		    {
			cnf_comp->opc_hi_const = *kqconst;
			break;
		    }
		    else if (sort_comp < 0)
		    {
			new_qconst = (OPC_QCONST *)
				opu_Gsmemory_get(global, sizeof (OPC_QCONST));
			STRUCT_ASSIGN_MACRO(
				kconst->opc_keyblk.adc_hikey,
				new_qconst->opc_keydata.opc_data);
			new_qconst->opc_qnext = *kqconst;
			new_qconst->opc_tconst_type = PST_USER;
			new_qconst->opc_const_no = 0;
			new_qconst->opc_qlang = cur_qlang;
			new_qconst->opc_qop = root;
			new_qconst->opc_dshlocation = -1;
			new_qconst->opc_key_type = ADC_KNOKEY;

			*kqconst = new_qconst;
			kconst->opc_keyblk.adc_hikey.db_data = NULL;
			cnf_comp->opc_hi_const = new_qconst;
			break;
		    }
		}
	    }

inlist_break:
	    /* Tests for IN-list constant loop. */
	    if (!(root->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLIST) ||
		root->pst_right->pst_sym.pst_type != PST_CONST)
		break;
	    if (root->pst_sym.pst_value.pst_s_op.pst_opno !=
				global->ops_cb->ops_server->opg_eq)
		break;
	    root->pst_right = root->pst_right->pst_left;
	    if (root->pst_right == (PST_QNODE *) NULL)
	    {
		root->pst_right = origrhs;	/* restore original rhs */
		break;
	    }
	}	/* end of big while */
	if ((root->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLIST) &&
		root->pst_right->pst_sym.pst_type == PST_CONST)
	{
	    /* We need to quickly restore the order - see head of big while */
	    PST_QNODE *p = root->pst_right;
	    root->pst_right = NULL;
	    while (p)
	    {
		PST_QNODE *t = p->pst_left;
		p->pst_left = root->pst_right;
		root->pst_right = p;
		p = t;
	    }
	}
	break;

     case PST_OR:
	if (root->pst_left != NULL)
	{
	    opc_cvclause(global, cnode, cqual, cnf_and, root->pst_left, rel);
	}
	if (root->pst_right != NULL)
	{
	    opc_cvclause(global, cnode, cqual, cnf_and, root->pst_right, rel);
	}
	break;

     case PST_COP:
     case PST_QLEND:
	break;

     default:
	opx_error(E_OP0682_UNEXPECTED_NODE);
	break;
    }
}

/*{
** Name: OPC_QCONVERT	- Convert a const qtree node to a type and length
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
**      19-aug-86 (eric)
**          written
**	17-Dec-2008 (kiria01) SIR120473
**	    Initialise the ADF_FN_BLK.adf_pat_flags
**	23-Sep-2009 (kiria01) b122578
**	    Initialise the ADF_FN_BLK .adf_fi_desc and adf_dv_n members.
[@history_template@]...
*/
PST_QNODE *
opc_qconvert(
		OPS_STATE	*global,
		DB_DATA_VALUE	*dval,
		PST_QNODE	*qconst)
{
    i4		    ret;
    PST_QNODE	    *qret;
    ADF_FN_BLK	    adffn;

    /* first, allocate the new query tree node and start filling it out. */
    qret = (PST_QNODE *) opu_Gsmemory_get(global, sizeof (PST_QNODE));
    STRUCT_ASSIGN_MACRO(*qconst, *qret);
    STRUCT_ASSIGN_MACRO(*dval, qret->pst_sym.pst_dataval);

    /* get the function instance id for this conversion */
    if ((ret = adi_ficoerce(global->ops_adfcb, 
			    qconst->pst_sym.pst_dataval.db_datatype, 
			    dval->db_datatype, &adffn.adf_fi_id)) != E_DB_OK
	)
    {
	opx_verror(ret, E_OP0783_ADI_FICOERCE,
				global->ops_adfcb->adf_errcb.ad_errcode);
    }

    /* Now lets fill in the datatype length info and allocate space for the 
    ** data.
    */
    if ((ret = adi_fidesc(global->ops_adfcb, adffn.adf_fi_id, &adffn.adf_fi_desc)) 
								!= E_DB_OK
	)
    {
	/* EJLFIX: get a better error no */
	opx_verror(ret, E_OP0783_ADI_FICOERCE,
				global->ops_adfcb->adf_errcb.ad_errcode);
    }
    qret->pst_sym.pst_dataval.db_data = (PTR) opu_Gsmemory_get(global, 
					qret->pst_sym.pst_dataval.db_length);

    STRUCT_ASSIGN_MACRO(qret->pst_sym.pst_dataval, adffn.adf_r_dv);
    STRUCT_ASSIGN_MACRO(qconst->pst_sym.pst_dataval, adffn.adf_1_dv);
    adffn.adf_dv_n = 1;
    adffn.adf_pat_flags = AD_PAT_NO_ESCAPE;

    if ((ret = adf_func(global->ops_adfcb, &adffn)) != E_DB_OK)
    {
	opx_verror(ret, E_OP0700_ADC_CVINTO,
				global->ops_adfcb->adf_errcb.ad_errcode);
    }

    return(qret);
}

/*{
** Name: OPC_COMPARE	- Compare two qnodes
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
**      20-aug-86 (eric)
**          written
[@history_template@]...
*/
i4
opc_compare(
		OPS_STATE   *global,
		PST_QNODE   *d1,
		PST_QNODE   *d2)
{
    i4		    ret;
    DB_STATUS	    err;

    if ((err = adc_compare(global->ops_adfcb, 
				&d1->pst_sym.pst_dataval,
				&d2->pst_sym.pst_dataval,
						    &ret)) != E_AD0000_OK
	)
    {
	opx_verror(err, E_OP078C_ADC_COMPARE,
				global->ops_adfcb->adf_errcb.ad_errcode);
    }
    return(ret);
}

/*{
** Name: OPC_GTQUAL	- Get the qualification to be applied at a co/qen node.
**
** Description:
**      This routine returns the qualification that should be evaluated 
**      at a co/qen node.  Qualifications are sorted by selectivity
**	so that the caller can generate the most selective tests first.
**        
**      The source for the qualification clauses is the boolean factor 
**      array that OPF builds into the subquery structure. In the new 
**	processing scheme "interesting" boolean factors are indicated
**	in the boolean factor bit map.  We build up an PST_AND qtree node to 
**      hold the boolfacts subqtree and link it into the qualification 
**      that will be returned. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    State info for the current query
** 	cnode -
**	    Current cost node, needed mainly for the outer join ID.
**	bfactbm -
**	    ptr to a bit map of boolean factors to be processed at this node.
**	pqual -
**	    A pointer to uninitialized pointer
**	qual_eqcmp -
**	    A pointer to an uninitialized eqcmap.
**
** Outputs:
**	pqual -
**	    Pqual will point to the qualification that should be evaluated
**	    or will be NULL.
**	qual_eqcmp -
**	    Qual_eqcmp will be set with all of the eqcs that are used in the
**	    qualification.
**
**	Returns:
**	    Nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-oct-89 (stec)
**	    Added ptr to boolan facto bitmap to the interface.
**	    Changed code to take advantage of the boolean factor bit maps
**	    provided by OPF.
**	13-may-92 (rick)
**	    The routines which bundle together inner join boolean factors
**	    (opc_gtqual and opc_gtiqual) should tie into their little
**	    packages any outer join boolean factors which enumeration
**	    pushed down out of the ON clauses of higher nodes.  Added
**	    cnode as an argument to help opc_gtqual do this job.  Also,
**	    removed a big pile of ifdef'd code.
**	03-jan-96 (wilri01)
**	    Sort bfs by selectivity to optimize evaluation.
**	11-apr-96 (wilri01)
**	    Fixed chg on 03-jan-96 to allow for zero selectivity
**	30-jul-96 (inkdo01)
**	    Make Rtree predicates appear at top of AND list.
**	3-sep-98 (inkdo01)
**	    Make long Rtree predicates appear at end of AND list.
**	24-mar-99 (wanfr01)
**	    Add a check to avoid accidentally dropping the where clause
**	    during this procedure (bug 95384, INGSRV 721)
**	28-may-03 (inkdo01)
**	    Additional change to the above to recover from 95384 when its
**	    cause is a NaN in opb_selectivity (bug 109507).
**	4-Dec-2006 (kschendel) b122118
**	    Delete unused routine parameters, just caused confusion.
[@history_template@]...
*/
VOID
opc_gtqual(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		OPB_BMBF	*bfactbm,
		PST_QNODE	**pqual,
		OPE_BMEQCLS	*qual_eqcmp)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    PST_QNODE		*new_and;
    OPB_BOOLFACT	*boolfact;
    OPB_IBF		ibf;
    OPO_CO		*co = cnode->opc_cco;
    OPL_IOUTER		outerJoinID = co->opo_variant.opo_local->opo_ojid;

    /* Set the output variables to their beginning values */
    *pqual = (PST_QNODE *)NULL;
    MEfill(sizeof (*qual_eqcmp), (u_char)0, (PTR)qual_eqcmp);

    if (bfactbm == (OPB_BMBF *)NULL)
    {
	return;
    }

    for (ibf = -1;
	 (ibf = BTnext(ibf, (char *)bfactbm, subqry->ops_bfs.opb_bv)) > -1; )
    {
	boolfact = subqry->ops_bfs.opb_base->opb_boolfact[ibf];

	/*
	** The only boolean factors we AREN'T interested in are those
	** with outer join ids that are the same as our current node's.
	** That is, we DON'T want to assemble here pieces of this node's
	** ON clause.  That ON clause will be assembled by opc_ogtqual.
	** It is possible for Enumeration to push conjuncts of higher
	** ON clauses down into our node.  That means that Enumeration
	** has determined that those conjuncts really reduce to inner
	** join restrictions.  This routine only bundles up inner join
	** boolean factors.
	*/

	if ( outerJoinID != OPL_NOOUTER && boolfact->opb_ojid == outerJoinID )
	    continue;

	BTor((i4)subqry->ops_eclass.ope_ev, (char *)&boolfact->opb_eqcmap, 
	    (char *)qual_eqcmp);

	/* Fiddle selectivity of spatial predicates to force them to end of
	** AND-list (after selectivity sort). opcorig and opcjcommon expect
	** them there. */
	if (boolfact->opb_mask & (OPB_SPATF | OPB_SPATJ))
	    boolfact->opb_selectivity = 998.9;	/* 2nd biggest possible! */
	/* Another fine Rtree kludge, to see if this factor involves a "long"
	** type. If so, assign it the largest selectivity. */
	if (subqry->ops_mask & OPS_SPATF && 
			boolfact->opb_qnode->pst_sym.pst_type == PST_BOP)
	{
	    DB_STATUS	status;
	    i4		bits = 0;

	    status = adi_dtinfo(global->ops_adfcb, 
		boolfact->opb_qnode->pst_left->pst_sym.pst_dataval.db_datatype,
		&bits);
	    if ((bits & AD_PERIPHERAL) == 0)  /* if left op not long, try right */
	     status = adi_dtinfo(global->ops_adfcb, 
		boolfact->opb_qnode->pst_right->pst_sym.pst_dataval.db_datatype,
		&bits);
	    if (bits & AD_PERIPHERAL) boolfact->opb_selectivity = 999.9;
	}

	/* allocate an 'and' qtree node, point it to the boolfact qtree, and
	** point it to the rest of the 'and' nodes;
	*/
	new_and = (PST_QNODE *) opu_Gsmemory_get(global, sizeof (PST_QNODE));
	new_and->pst_sym.pst_type = PST_AND;
	new_and->pst_sym.pst_len = ibf;		/* temp save */	
	new_and->pst_left = boolfact->opb_qnode;
	new_and->pst_right = *pqual;
	*pqual = new_and;
    }
    /* ************************************************************************
    **                       SORT AND NODES BY SELECTIVITY
    ** ************************************************************************/
    {
    	OPN_PERCENT	 selMax;
        OPB_BOOLFACT	*bf;    	
   	PST_QNODE	*p;
    	PST_QNODE	*pPrev;
    	PST_QNODE	*pNewBeg;
    	PST_QNODE	*pOldBeg;
    	PST_QNODE	*pMax;
    	PST_QNODE	*pMaxPrev;    	

	/* Rebuild AND-list with most selective factors at the top. This results
	** in most restrictive predicates executing first. */
    	for (pNewBeg = 0, pOldBeg = *pqual;;)	/* Loop until all nodes moved to new list */
    	{
    	    for (selMax = -2, pMax = 0, pPrev = 0, p = pOldBeg; /* Get least restrictive node */
    	         p != NULL && p->pst_sym.pst_type == PST_AND;
    	         pPrev = p, p = p->pst_right)
    	    {
		bf = subqry->ops_bfs.opb_base->opb_boolfact[p->pst_sym.pst_len];    	    
    	    	if (bf->opb_selectivity > selMax)
    	    	{
    	    	    selMax = bf->opb_selectivity;
    	    	    pMax = p;
    	    	    pMaxPrev = pPrev;
    	    	}
		else if(pMax == (PST_QNODE *) NULL)
		{
		    /* Alternate collating sequences (and other conditions?)
		    ** can leave NaN in opb_selectivity, so its bfactor may
		    ** unintentionally be dropped here. This prevents that 
		    ** from happening. */
		    pMax = p;
		    pMaxPrev = pPrev;
		}
    	    }
    	    if (!pMax)				/* All copied?			*/
    	        break;    	        
    	        
	    if (pMaxPrev)			/* Take out of old list		*/
	    	pMaxPrev->pst_right = pMax->pst_right;
	    else
	    	pOldBeg = pMax->pst_right;
	    	
	    pMax->pst_right = pNewBeg;		/* Put as head of new list	*/
	    pNewBeg = pMax;
	    pMax->pst_sym.pst_len = 0;		/* reset			*/	    	
    	}
  
        /*** Check for bug 95384 - a dropped where qualifier ***/
        if ((pNewBeg == NULL) && (*pqual != NULL)) {
            opx_error(E_OP08A2_EXCEPTION);
        }
	*pqual = pNewBeg;
    }        
}    

/*{
** Name: OPC_GTIQUAL	- Get the qualification to be applied by DMF.
**
** Description:
**      This routine returns the qualification that should be evaluated 
**      at a co/qen node.  It's pretty much the same as gtqual, except
**	we don't bother sorting the generated AND qtree into selectivity
**	order.
**        
**      The source for the qualification clauses is the boolean factor 
**      array that OPF builds into the subquery structure. In the new 
**	processing scheme "interesting" boolean factors are indicated
**	in the boolean factor bit map.  We build up an PST_AND qtree node to 
**      hold the boolfacts subqtree and link it into the qualification 
**      that will be returned. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    State info for the current query
**	global->ops_cstate.opc_subqry -
**	    Eqc and boolfact information is used from the current subquery
**	    struct.
**	bfactbm -
**	    ptr to a bit map of boolean factors to be processed at this node.
**	pqual -
**	    A pointer to uninitialized pointer
**	qual_eqcmp -
**	    A pointer to an uninitialized eqcmap.
**
** Outputs:
**	pqual -
**	    Pqual will point to the qualification that should be evaluated
**	    or will be NULL.
**	qual_eqcmp -
**	    Qual_eqcmp will be set with all of the eqcs that are used in the
**	    qualification.
**
**	Returns:
**	    Nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-oct-89 (stec)
**	    Added ptr to boolan facto bitmap to the interface.
**	    Changed code to take advantage of the boolean factor bit maps
**	    provided by OPF.
**	19-feb-92 (rickh)
**	    Removed call to opc_ionlyatts.  It didn't do anything useful.
**	13-may-92 (rick)
**	    The routines which bundle together inner join boolean factors
**	    (opc_gtqual and opc_gtiqual) should tie into their little
**	    packages any outer join boolean factors which enumeration
**	    pushed down out of the ON clauses of higher nodes.
**	4-Dec-2006 (kschendel) b122118
**	    Delete unused routine parameters, just caused confusion.
[@history_template@]...
*/
VOID
opc_gtiqual(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		OPB_BMBF	*bfactbm,
		PST_QNODE	**pqual,
		OPE_BMEQCLS	*qual_eqcmp)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    PST_QNODE		*new_and;
    OPB_BOOLFACT	*boolfact;
    OPB_IBF		ibf;
    OPO_CO		*co = cnode->opc_cco;
    OPL_IOUTER		outerJoinID = co->opo_variant.opo_local->opo_ojid;

    /* Set the output variables to their beginning values */
    *pqual = (PST_QNODE *)NULL;
    MEfill(sizeof (*qual_eqcmp), (u_char)0, (PTR)qual_eqcmp);

    if (bfactbm == (OPB_BMBF *)NULL)
    {
	return;
    }

    for (ibf = -1; 
	 (ibf = BTnext(ibf, (char *)bfactbm, subqry->ops_bfs.opb_bv)) > -1; )
    {
	boolfact = subqry->ops_bfs.opb_base->opb_boolfact[ibf];

	/*
	** The only boolean factors we AREN'T interested in are those
	** with outer join ids that are the same as our current node's.
	** That is, we DON'T want to assemble here pieces of this node's
	** ON clause.  That ON clause will be assembled by opc_ogtqual.
	** It is possible for Enumeration to push conjuncts of higher
	** ON clauses down into our node.  That means that Enumeration
	** has determined that those conjuncts really reduce to inner
	** join restrictions.  This routine only bundles up inner join
	** boolean factors.
	*/

	if ( outerJoinID != OPL_NOOUTER && boolfact->opb_ojid == outerJoinID )
	    continue;

	BTor((i4)subqry->ops_eclass.ope_ev, (char *)&boolfact->opb_eqcmap, 
	    (char *)qual_eqcmp);

	/* allocate an 'and' qtree node, point it to the boolfact qtree, and
	** point it to the rest of the 'and' nodes;
	*/
	new_and = (PST_QNODE *) opu_Gsmemory_get(global, sizeof (PST_QNODE));
	new_and->pst_sym.pst_type = PST_AND;
	new_and->pst_sym.pst_len = 0;
	new_and->pst_left = boolfact->opb_qnode;
	new_and->pst_right = *pqual;
	*pqual = new_and;
    }
}    

/*{
** Name: OPC_OGTQUAL	- Get the outer join qualification to be applied 
**			  at a co/qen node.
**
** Description:
**      This routine returns the qualification that should be evaluated 
**      at a co/qen node. Neither the co nor the qen node is specified as 
**      a parameter to this routine. Instead, one or two eqc maps are given 
**      since this encapsulates all of the information about the co/qen 
**      node that we need to know.  
**        
**      The source for the qualification clauses is the boolean factor 
**      array that OPF builds into the subquery structure. In the new 
**	processing scheme "interesting" boolean factors are indicated
**	in the boolean factor bit map. The rest is is as described
**	below.
**
**	The old processing scheme:
**	For each boolfact (ie. clause in the qtree), we first ask if this
**      node  has enough information (eqcs) to be able to evaluate the boolfact. 
**      If this is a join node, then we also ask if the boolfact has  
**      already been evaluated at a lower node. If the boolfact passes 
**      both of these tests, then we build up an PST_AND qtree node to 
**      hold the boolfacts subqtree and link it into the qualification 
**      that will be returned. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    State info for the current query
**	global->ops_cstate.opc_subqry -
**	    Eqc and boolfact information is used from the current subquery
**	    struct.
**	bfactbm -
**	    ptr to a bit map of boolean factors to be processed at this node.
**	pqual -
**	    A pointer to uninitialized pointer
**	qual_eqcmp -
**	    A pointer to an uninitialized eqcmap.
**
** Outputs:
**	pqual -
**	    Pqual will point to the qualification that should be evaluated
**	    or will be NULL.
**	qual_eqcmp -
**	    Qual_eqcmp will be set with all of the eqcs that are used in the
**	    qualification.
**
**	Returns:
**	    Nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	03-jan-90 (stec)
**	    Written.
**	22-jun-93 (rickh)
**	    A TID join should compile into its ON clause the ON clause of
**	    its degenerate inner ORIG node, since that ORIG node will not be
**	    separately compiled.
**	30-aug-93 (rickh)
**	    As usual, the above fix for KEY joins, too.
[@history_template@]...
*/
VOID
opc_ogtqual(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		PST_QNODE	**pqual,
		OPE_BMEQCLS	*qual_eqcmp)
{
    OPS_SUBQUERY    *subqry = global->ops_cstate.opc_subqry;
    OPB_BOOLFACT    *boolfact;
    OPO_CO	    *co = cnode->opc_cco;
    OPO_CO	    *innerChild = co->opo_inner;
    OPB_BMBF	    *innerChildsQualification;
    QEN_NODE	    *qenNode = cnode->opc_qennode;
    OPB_BMBF	    *bfactbm = co->opo_variant.opo_local->opo_bmbf;
    OPB_BMBF	    thisNodesQualification;
    PST_QNODE	    *qual = (PST_QNODE *) NULL;
    PST_QNODE	    *new_and;
    OPB_IBF	    i;

    MEcopy( ( PTR ) bfactbm, ( u_i2 ) sizeof( OPB_BMBF ),
	    ( PTR ) &thisNodesQualification );

    /*
    ** For a TID join, we must compile in the ON clause of our degenerate
    ** inner ORIG node.  This is because we do not compile that ORIG node
    ** into its own QEN node.
    */

    if ( ( ( qenNode->qen_type == QE_TJOIN ) ||
	   ( qenNode->qen_type == QE_KJOIN ) ) &&
	 ( innerChild != ( OPO_CO * ) NULL ) )
    {
	innerChildsQualification =
	    innerChild->opo_variant.opo_local->opo_bmbf;

	if ( innerChildsQualification != ( OPB_BMBF * ) NULL )
	{
	    BTor( ( i4  ) BITS_IN( *innerChildsQualification ),
		  ( char * ) innerChildsQualification,
		  ( char * ) &thisNodesQualification );
	}
    }

    /*
    ** Now we have constructed a bitmap of all the qualifications to be
    ** applied at this node. Sift out only those conjuncts which belong
    ** to the ON clause of this node.
    */

    for (i = -1;
	 (i = BTnext( i, ( char * ) &thisNodesQualification,
		      subqry->ops_bfs.opb_bv)) > -1; )
    {
	boolfact = subqry->ops_bfs.opb_base->opb_boolfact[i];

	if (co->opo_variant.opo_local->opo_ojid != boolfact->opb_ojid)
	    continue;

	/* allocate an 'and' qtree node, point it to the boolfact qtree, and
	** point it to the rest of the 'and' nodes;
	*/
	new_and = (PST_QNODE *) opu_Gsmemory_get(global, sizeof (PST_QNODE));
	new_and->pst_sym.pst_type = PST_AND;
	new_and->pst_sym.pst_len = 0;
	new_and->pst_left = boolfact->opb_qnode;
	new_and->pst_right = qual;
	qual = new_and;
    }

    *pqual = qual;
}

/*{
** Name: OPC_METAOPS	- Return the number of meta operator nodes in a qtree.
**
** Description:
**      This returns the number of bop nodes whose value for pst_opmeta is
**      something other than PST_NOMETA. 
[@comment_line@]...
**
** Inputs:
**  global -
**	The global state information for the current query.
**  root -
**	The qtree to be scanned for meta ops.
**
** Outputs:
**  none
**
**	Returns:
**	    The number of meta op nodes.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      8-aug-87 (eric)
**          created
[@history_line@]...
[@history_template@]...
*/
i4
opc_metaops(
		OPS_STATE   *global,
		PST_QNODE   *root)
{
    i4	    ret;

    switch (root->pst_sym.pst_type)
    {
     case PST_BOP:
	if (root->pst_sym.pst_value.pst_s_op.pst_opmeta == PST_NOMETA)
	{
	    ret = 0;
	}
	else
	{
	    ret = 1;
	}
	break;

     case PST_AND:
     case PST_OR:
	ret = 0;
	if (root->pst_left != NULL)
	{
	    ret += opc_metaops(global, root->pst_left);
	}

	if (root->pst_right != NULL)
	{
	    ret += opc_metaops(global, root->pst_right);
	}
	break;

     default:
	ret = 0;
	break;
    }

    return(ret);
}
