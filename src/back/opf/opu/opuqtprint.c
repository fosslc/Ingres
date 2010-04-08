/*
**Copyright (c) 2004 Ingres Corporation
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
#define        OPU_QTPRINT      TRUE
#include    <opxlint.h>

/**
**
**  Name: OPUQTPRINT.C - print the query tree
**
**  Description:
**      Routine will print the query tree 
**
**  History:    
**      20-jul-86 (seputis)
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	15-sep-93 (swm)
**	    Moved cs.h include above other header files which need its
**	    definition of CS_SID.
[@history_template@]...
**/

/*{
** Name: opu_qtprint	- print query tree
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
**      20-jul-86 (seputis)
**          initial creation
**      9-nov-92 (ed)
**          use less verbose mode of query tree print routine
**	22-mar-10 (smeke01) b123457
**	    Add feature to print origin of call (cf trace point op170).
[@history_template@]...
*/
VOID
opu_qtprint(
OPS_STATE          *global,
PST_QNODE          *root,
char               *origin)
{
    DB_STATUS           status;
    DB_ERROR            error;
    char                temp[OPT_PBLEN+1];
    bool                init = FALSE;

    if (origin)
    {
	if (!global->ops_cstate.opc_prbuf)
	{
	    global->ops_cstate.opc_prbuf = temp;
	    init = TRUE;
	}
	TRformat(opt_scc, (i4 *)NULL, global->ops_cstate.opc_prbuf,
	    OPT_PBLEN, "\n\nParse tree dumped %s:\n", origin);
	if (init) 
	    global->ops_cstate.opc_prbuf = NULL;
    }
    status = pst_1prmdump(root, (PST_QTREE *)NULL, &error);
# ifdef E_OP0683_DUMPQUERYTREE
    if (status != E_DB_OK)
	opx_verror(status, E_OP0683_DUMPQUERYTREE, error.err_code);
#endif
}
