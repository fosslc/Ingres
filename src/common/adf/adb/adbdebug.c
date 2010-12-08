/*
**	Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <tr.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adftrace.h>
/*
[@#include@]...
*/

/**
**
**  Name: ADBDEBUG.C - Holds general debugging routines used for debugging ADF.
**
**  Description:
**      This file contains generally usable debugging routines for ADF.
**
**	The externally visible routines contained in this file are:
**
**          adb_trace() - Set or clear ADF trace points.
[@func_list@]...
**
**
**  History:
**      28-oct-86 (thurston)
**          Initial creation.
**      25-feb-87 (thurston)
**          Made changes for using server CB instead of GLOBALDEF/REFs.
**	06-mar-89 (jrb)
**	    Changed usage of adi_optabfi from ptr to adi_fi_desc to index into
**	    fi tab.
**	07-apr-89 (jrb)
**	    Added adb_breakpoint.
**      21-dec-1992 (stevet)
**          Added function prototype.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
[@history_template@]...
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
[@static_variable_or_function_definitions@]
*/


/*{
** Name: adb_trace() - Set or clear ADF trace points.
**
** Description:
**      This routines sets or clears ADF trace points. 
**
** Inputs:
**      adb_debug_cb                    A standard debug control block.
**
** Outputs:
**      none
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARN
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sets or clears ADF trace points as specified.  Currently all ADF
**	    trace points are defined at the SERVER level, and exist in the ADF
**	    server control block.
**
** History:
**      28-oct-86 (thurston)
**          Initial creation.
**	09-nov-2010 (gupsh01) SIR 124685
**	    Protype cleanup.
[@history_template@]...
*/
DB_STATUS
adb_trace(
DB_DEBUG_CB *adb_debug_cb)
{
    i4             ts = adb_debug_cb->db_trswitch;
    i4             tp = adb_debug_cb->db_trace_point;


    if (tp < ADF_MINTRACE || tp > ADF_MAXTRACE)
	return(E_DB_WARN);	/* Unknown tracepoint */

    switch (ts)
    {
      case DB_TR_NOCHANGE:
	return(E_DB_OK);
	break;

      case DB_TR_ON:
	if (ult_set_macro(&Adf_globs->Adf_trvect, tp, 0, 0) != E_DB_OK)
	    return(E_DB_WARN);
	else
	    return(E_DB_OK);
	break;

      case DB_TR_OFF:
	if (ult_clear_macro(&Adf_globs->Adf_trvect, tp) != E_DB_OK)
	    return(E_DB_WARN);
	else
	    return(E_DB_OK);
	break;

      default:
	return(E_DB_ERROR);	/* Bad db_trswitch */
	break;
    }
}


/*
** Name: adb_optab_print() - Print the contents of ADF's Operations table.
**
** Description:
**      This routine is a debugging tool that prints the contents of ADF's 
**      operations table via TRdisplay.
**
** Inputs:
**      none
**
** Outputs:
**      none
**
**	Returns:
**	    VOID
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-oct-86 (thurston)
**          Initial creation.
**	06-mar-89 (jrb)
**	    Changed usage of adi_optabfi from ptr to adi_fi_desc to index into
**	    fi tab.
[@history_template@]...
*/

VOID
adb_optab_print(void)
{
    ADI_OPRATION        *op;


    TRdisplay("op-name         op-id  op-type   count  1st-fiidx\n");
    TRdisplay("-------------------------------------------------\n");
    for (op = &Adf_globs->Adi_operations[0]; op->adi_opid != ADI_NOOP; op++)
    {
	TRdisplay("%12s ", op->adi_opname.adi_opname);
	TRdisplay("%7d   ", (int) op->adi_opid);
	TRdisplay("%10w ",
		",COMPARISON,OPERATOR,AGG_FUNC,NORM_FUNC,COERCION",
		(int) op->adi_optype);
	TRdisplay("%3d ", (int)op->adi_opcntfi);
	if (op->adi_optabfidx != ADZ_NOFIIDX)
	    TRdisplay("%8d\n", (int) op->adi_optabfidx);
	else
	    TRdisplay("  < none >\n");
    }
    TRdisplay("------------------------------------------------\n");
    return;
}


/*{
** Name: adb_breakpoint	- Breakpoint can be set here to catch certain errors
**
** Description:
**	This routine serves as a place to set a breakpoint so that certain error
**	conditions can be detected.
**
** Inputs:
**      none
**
** Outputs:
**	none
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-apr-89 (jrb)
**          Initial creation.
[@history_template@]...
*/
VOID
adb_breakpoint(void)
{
}

/*
[@function_definition@]...
*/
