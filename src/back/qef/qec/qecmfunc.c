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
#include    <adf.h>
#include    <ade.h>
#include    <dudbms.h>
#include    <dmf.h> 
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <ex.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <scf.h>
#include    <qefmain.h>
#include    <qefscb.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <tm.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**
**  Name: QECMFUNC.C - map the function table
**
**  Description:
**      The function table is used to facilitate function calls
**  when processing query plans. Given a function id, the call
**  can be made quickly. 
**
**          qec_mfunc - map function table
**
**
**  History:    $Log-for RCS$
**      29-may-86 (daved)    
**          written
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      20-aug-95 (ramra01)
**          Defined a new node type ORIGAGG to deal with simple aggregates
**          for performance enhancement.
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**/


/*{
** Name: qec_mfunc      - map function table
**
** Description:
**      Map functions used by query processing. An array is created
** of function ids and the address of the appropriate function. 
**
** Inputs:
**      qef_func                        map array
**
** Outputs:
**      none
**
**      Returns:
**          E_DB_OK
**      Exceptions:
**          none
**
** Side Effects:
**          The function map is filled in.
**
** History:
**      29-may-86 (daved)
**          written
**      08-apr-93 (smc)
**          Cast the func_name elements to function pointers so
**          clear up compiler warnings.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	19-mar-99 (inkdo01)
**	    Added support for hash joins.
**	8-jan-04 (toumi01)
**	    Added support for exchange.
**	20-may-2008 (dougi)
**	    Added support for table procedures.
**	4-Jun-2009 (kschendel) b122118
**	    Don't need casting any more, QEF_FUNC is properly defined.
*/
VOID
qec_mfunc(
QEF_FUNC        qef_func[] ) 
{

    qef_func[QE_ORIG].func_id	    = QE_ORIG;             /* function id */
    qef_func[QE_ORIG].func_name	    = qen_orig;
    qef_func[QE_TJOIN].func_id	    = QE_TJOIN;            /* function id */
    qef_func[QE_TJOIN].func_name    = qen_tjoin;
    qef_func[QE_KJOIN].func_id	    = QE_KJOIN;            /* function id */
    qef_func[QE_KJOIN].func_name    = qen_kjoin;
    qef_func[QE_HJOIN].func_id	    = QE_HJOIN;            /* function id */
    qef_func[QE_HJOIN].func_name    = qen_hjoin;
    qef_func[QE_SORT].func_id	    = QE_SORT;             /* function id */
    qef_func[QE_SORT].func_name     = qen_sort;
    qef_func[QE_TSORT].func_id	    = QE_TSORT;            /* function id */
    qef_func[QE_TSORT].func_name    = qen_tsort;
    qef_func[QE_ISJOIN].func_id	    = QE_ISJOIN;           /* function id */
    qef_func[QE_ISJOIN].func_name   = qen_isjoin;
    qef_func[QE_FSMJOIN].func_id    = QE_FSMJOIN;          /* function id */
    qef_func[QE_FSMJOIN].func_name  = qen_fsmjoin;
    qef_func[QE_QP].func_id	    = QE_QP;		   /* function id */
    qef_func[QE_QP].func_name	    = qen_qp;
    qef_func[QE_SEJOIN].func_id	    = QE_SEJOIN;	   /* function id */
    qef_func[QE_SEJOIN].func_name   = qen_sejoin;
    qef_func[QE_CPJOIN].func_id	    = QE_CPJOIN;           /* function id */
    qef_func[QE_CPJOIN].func_name   = qen_cpjoin;
    qef_func[QE_ORIGAGG].func_id    = QE_ORIGAGG;           /* function id */
    qef_func[QE_ORIGAGG].func_name  = qen_origagg;
    qef_func[QE_EXCHANGE].func_id   = QE_EXCHANGE;         /* function id */
    qef_func[QE_EXCHANGE].func_name = qen_exchange;
    qef_func[QE_TPROC].func_id   = QE_TPROC;         /* function id */
    qef_func[QE_TPROC].func_name = qen_tproc;
}
