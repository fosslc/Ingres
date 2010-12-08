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
#define             OPT_STRACE          TRUE
#include    <opxlint.h>
/**
**
**  Name: OPTSTRACE.C - check if trace flag is set
**
**  Description:
**      Routine to check if trace flag is set
**
**  History:    
**      3-jul-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
bool opt_strace(
	OPS_CB *ops_cb,
	i4 traceflag);
bool opt_svtrace(
	OPS_CB *ops_cb,
	i4 traceflag,
	i4 *firstvalue,
	i4 *secondvalue);
bool opt_gtrace(
	OPG_CB *opg_cb,
	i4 traceflag);

/*{
** Name: opt_strace	- check if trace flag is set
**
** Description:
**      This routine will return TRUE if a trace flag is set  
**
** Inputs:
**	ops_cb                          ptr to session control block with
**                                      trace flags
**      traceflag                       number of trace flag to be checked
**
** Outputs:
**	Returns:
**	    TRUE - if trace flag is set
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	3-jul-86 (seputis)
**          initial creation
[@history_line@]...
*/
bool
opt_strace(
	OPS_CB		   *ops_cb,
	i4                traceflag)
{
    i4             firstvalue;
    i4             secondvalue;

    return (	ops_cb->ops_check &&
	        ult_check_macro( &ops_cb->ops_tt , 
                              traceflag, &firstvalue, &secondvalue )
           );
}

/*{
** Name: opt_svtrace	- check if trace flag is set
**
** Description:
**      This routine will return TRUE if a trace flag is set and return
**      values stored in tracing
**
** Inputs:
**	ops_cb                          ptr to session control block with
**                                      trace flags
**      traceflag                       number of trace flag to be checked
**
** Outputs:
**      firstvalue			ptr to value to be returned
**      secondvalue			ptr to value to be returned
**	Returns:
**	    TRUE - if trace flag is set and values are returned
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	6-nov-88 (seputis)
**          initial creation
[@history_line@]...
*/
bool
opt_svtrace(
	OPS_CB		*ops_cb,
	i4             traceflag,
	i4		*firstvalue,
	i4         *secondvalue)
{

    return (	ops_cb->ops_check &&
	        ult_check_macro( &ops_cb->ops_tt , 
                              traceflag, firstvalue, secondvalue )
           );
}

/*{
** Name: opt_gtrace	- check if trace flag is set in global server CB
**
** Description:
**      This routine will return TRUE if a trace flag is set  
**
** Inputs:
**	opg_cb                          ptr to server control block
**      traceflag                       number of trace flag to be checked
**
** Outputs:
**	Returns:
**	    TRUE - if trace flag is set
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	3-jun-88 (seputis)
**          initial creation
[@history_line@]...
*/
bool
opt_gtrace(
	OPG_CB		   *opg_cb,
	i4                traceflag)
{
    i4             firstvalue;
    i4             secondvalue;

    return (	opg_cb->opg_check &&
	        ult_check_macro( &opg_cb->opg_tt , 
                              traceflag, &firstvalue, &secondvalue )
           );
}
