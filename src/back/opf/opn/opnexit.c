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
#include    <ex.h>
#define		OPN_EXIT	TRUE
#define		OPX_EXROUTINES	TRUE
#include    <opxlint.h>


/**
**
**  Name: OPNEXIT.C - initiate return to start of enumeration nesting
**
**  Description:
**      This routine is used for normal exit of the enumeration phase.
**
**  History:
**      7-jun-86 (seputis)    
**          initial creation from nerr in nerr.c
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opn_exit(
	OPS_SUBQUERY *subquery,
	bool longjump);

/*{
** Name: opn_exit	- initiate return to start of enumeration nesting
**
** Description:
**	This routine is called when enumeration routines exit normally.  This
**      will place all the cleanup code for enumeration in one spot.
**
** Inputs:
**
** Outputs:
**	Returns:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	7-jun-86 (seputis)
**          initial creation
**      20-dec-90 (seputis)
**          added support for floating point exception handling
**	19-nov-99 (inkdo01)
**	    Removed EXsignal which incurred excessive kernel overhead. Now, 
**	    calling funcs just return OPN_SIGEXIT code to indicate we're in 
**	    exit processing.
**	30-mar-04 (hayke02)
**	    Add new boolean, longjump, to indicate whether we do an
**	    EXsignal(...EX_JNP_LJMP...).
[@history_line@]...
*/
VOID
opn_exit(
	OPS_SUBQUERY        *subquery,
	bool		    longjump)
{
    OPS_STATE	    *global;

    global = subquery->ops_global;
    if (global->ops_estate.opn_tidrel)
    {   /* need to add a sort node to the query tree header so that
	** query compilation will sort on TID, a sort node should have
	** been added to support DEFERRED */
	PST_QNODE	    *sortnode;

	sortnode =
	global->ops_qheader->pst_stlist = global->ops_estate.opn_sortnode;/*
					** grab the sort node allocated
					** earlier so that no out of memory
					** errors will occur */
	sortnode->pst_left = NULL;
	sortnode->pst_right = NULL;
	sortnode->pst_sym.pst_type = PST_SORT;
	sortnode->pst_sym.pst_dataval.db_datatype = DB_TID_TYPE;
	sortnode->pst_sym.pst_dataval.db_prec = 0;
	sortnode->pst_sym.pst_dataval.db_length = DB_TID_LENGTH;
	sortnode->pst_sym.pst_dataval.db_data = NULL;
	sortnode->pst_sym.pst_len = sizeof(PST_SRT_NODE);
	sortnode->pst_sym.pst_value.pst_s_sort.pst_srvno = DB_IMTID;
	sortnode->pst_sym.pst_value.pst_s_sort.pst_srasc = TRUE;

	global->ops_qheader->pst_stflag = TRUE;
    }
    EXmath(EX_OFF);			/* turn off math exception handling 
					** since this is the default state
					** for query processing */
#	ifdef xNTR1
	if (tTf(26, 12))
	    estqeptim();
#	endif


    if (longjump)
	EXsignal( (EX)EX_JNP_LJMP, (i4)0);
}
