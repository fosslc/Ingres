/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <cs.h>
#include    <pc.h>
#include    <me.h>
#include    <scf.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmxcb.h>
#include    <dmtcb.h>
#include    <lg.h>
#include    <dm.h>
#include    <dml.h>

/**
** Name: DMCSTUSR.C - Function used to reset the user id stored in the session
**		      control block.
**
** Description:
**      This file contains the function responsible for resetting the effective
**	user name stored in DML sesscion control block (DML_SCB).
**
**      This file defines:
**    
**      dmc_reset_eff_user_id () -  Reset the effective user id in DML_SCB
**				    associated with this session 
**
** History:
**      23-may-91 (andre)
**	    created for FIPS.
**	 8-jul-1992 (walt)
**	    Prototyping DMF.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system type definitions.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	31-jan-1994 (bryanp) B58379
**	    Log scf_error.err_code if scf_call fails. Added include of ulf.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Jan-2001 (jenjo02)
**	    Find DML_SCB with macro instead of calling SCF.
**	18-sep-2003 (abbjo03)
**	    Include pc.h for definition of PID (required by lg.h).
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**/

/*{
** Name: dmc_reset_eff_user_id - reset the effective user name known to this
**				 DMF session 
**
**  INTERNAL DMF call format:    status = dmc_reset_eff_user_id(&dmc_cb);
**
**  EXTERNAL call format:        status = dmf_call(DMC_RESET_EFF_USER_ID,
**						   &dmc_cb);
**
** Description:
**  This function obtains the session control block (DML_SCF) from SCF and
**  resets the effective user name (as is stored in DML_SCF.DML_SCB.scb_user).
**
** Inputs:
**      dmc_cb 
**          .type                           Must be set to DMC_CONTROL_CB.
**          .length                         Must be at least sizeof(DMC_CB).
**          .dmc_op_type                    Must be DMC_SESSION_OP.
**          .dmc_session_id		    Must be the SCF session id.
**          .dmc_flag_mask                  Must be zero
**	    .dmc_name			    New effective user name
**		data_address		    must be != NULL
**		data_in_size		    must be sizeof(DB_OWN_NAME)
**
** Output:
**      dmc_cb 
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM001A_BAD_FLAG
**					    E_DM002A_BAD_PARAMETER
**                                          E_DM002F_BAD_SESSION_ID
**          .error.err_data                 Set to characteristic in error 
**                                          by returning index into 
**                                          characteristic array.
**                     
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmc_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmc_cb.error.err_code.
**	    E_DB_SEVERE			    Function completed abnormally with
**                                          error which requires that the
**					    current session be brought down.
**                                          The fatal status is in
**					    dmc_cb.error.err_code.
** History:
**      23-may-91 (andre)
**	    Created for FIPS.
**	09-oct-93 (swm)
**	    Bug #56437
**	    Get scf_session_id from new dmc_session_id rather than dmc_id.
**	31-jan-1994 (bryanp) B58379
**	    Log scf_error.err_code if scf_call fails.
**	08-Jan-2001 (jenjo02)
**	    Find DML_SCB with macro instead of calling SCF.
*/

DB_STATUS
dmc_reset_eff_user_id(
DMC_CB    *dmc_cb)
{
    DMC_CB		*dmc = dmc_cb;
    DB_STATUS		status = E_DB_ERROR;
    DML_SCB		*scb;

    CLRDBERR(&dmc->error);

    /*  Check control block parameters. */

    if (dmc->dmc_op_type != DMC_SESSION_OP)
	SETDBERR(&dmc->error, 0, E_DM000C_BAD_CB_TYPE);
    else if (dmc->dmc_flags_mask != 0L)
	SETDBERR(&dmc->error, 0, E_DM001A_BAD_FLAG);
    else if (!dmc->dmc_name.data_address ||
	      dmc->dmc_name.data_in_size != sizeof(DB_OWN_NAME))
	SETDBERR(&dmc->error, 0, E_DM002A_BAD_PARAMETER);
    else if ( (scb = GET_DML_SCB(dmc->dmc_session_id)) == 0 || 
	      dm0m_check((DM_OBJECT *)scb, (i4)SCB_CB) )
	SETDBERR(&dmc->error, 0, E_DM002F_BAD_SESSION_ID);
    /* now reset the effective user name */
    else 
    {
	MEcopy(dmc->dmc_name.data_address, sizeof(DB_OWN_NAME),
		    (PTR) &scb->scb_user);
	status = E_DB_OK;
    }

    return (status);
}
