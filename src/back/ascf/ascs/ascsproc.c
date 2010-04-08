/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: ASCSPROC.C -- procedure processing routines
**
** Description:
**    This module contains routines which are called to process
**    an execute procedure (method)
** History:
**	31-Dec-1997 (shero03)
**	    Created for Frontier
** 	20-Jan-1998 (wonst02)
** 	    Added calls to URS to build parameters and execute the method.
** 	23-Jun-1998 (wonst02)
** 	    Use ursb_num_ele & ursb_num_recs fields instead of others.
**      18-Jan-1999 (fanra01)
**          Renamed scs_gca_send, scs_gca_get, scs_gca_data_available,
**          scs_format_response, scs_get_next_token and scs_process_interrupt
**          to ascs equivalents.
**          Renamed scs_process_procedure entrypoint as ascs.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**  11-Feb-2005 (fanra01)
**      Sir 112482
**      Removed urs facility reference.
*/

#include    <compat.h>
#include    <gl.h>
#include    <cv.h>
#include    <ex.h>
#include    <sl.h>
#include    <st.h>
#include    <tm.h>
#include    <er.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulm.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dudbms.h>
#include    <scf.h>
#include    <gca.h>
#include    <gcd.h>
#include    <adf.h>
#include    <ulf.h>
#include    <ddb.h>
#include    <qsf.h>
#include    <urs.h>

#include    <copy.h>
#include    <dmrcb.h>
#include    <qefrcb.h>
#include    <qefqeu.h>
#include    <qefcopy.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0e.h>

/*
** Forward/Extern references
*/


/*
** Global variables owned by this file
*/


/*
** Name: ascs_process_procedure -- entry point
** Description:
** Input:
** 	scb				The SCD SCB
** Output:
** Return:
** History:
*/
DB_STATUS
ascs_process_procedure(SCD_SCB *scb)
{
    DB_STATUS 		status = E_DB_OK;
    DB_STATUS 		stat;
    URSB    		ursb ZERO_FILL;
    URSB_PARM  		ursbparm ZERO_FILL;
    GCD_ID		Proc_hdr;
    i4		ProcMask;
    i4			Method_len;
    char		Method[GCA_MAXNAME+1];
    i4			Interface_len;
    char		Interface[GL_MAXNAME+1];

    ursb.ursb_parm = &ursbparm;

    /*
    ** Get the method's Name (procedure name)
    */
    status = ascs_gca_get(scb, (PTR)&Proc_hdr, sizeof(GCD_ID));
    if (DB_FAILURE_MACRO(status))
    {
	return (status);
    }
    Method_len = STindex(Proc_hdr.gca_name, ERx(" "), GCA_MAXNAME)
    	       - &Proc_hdr.gca_name[0];
    STlcopy(Proc_hdr.gca_name, Method, Method_len);

    /*
    ** Get method's interface (Procedure Owner) Parm
    */
    status = ascs_gca_get(scb, (PTR)&Interface_len, sizeof(i4));
    if (DB_FAILURE_MACRO(status))
    {
	return (status);
    }

    if ((Interface_len > 0) && (Interface_len <= GL_MAXNAME))
    {
    	Interface_len = min(Interface_len, sizeof(Interface));
        status = ascs_gca_get(scb, (PTR)&Interface, Interface_len);
        if (DB_FAILURE_MACRO(status))
    	{
            return (status);
    	}
	Interface[Interface_len] = 0;   /* null-terminate the string */
    }
    else
    {
	return E_DB_OK;
    }

    /*
    ** Get method's mask
    */
    status = ascs_gca_get(scb, (PTR)&ProcMask, sizeof(ProcMask));
    if (DB_FAILURE_MACRO(status))
    {
	return (status);
    }

    /*
    ** Call URSM with the interface and method and setup the parms
    */
    status = ascs_bld_procedure(scb, &ursb, Method, Interface);

    /*
    ** Call URSM to execute the method
    */
    if (! DB_FAILURE_MACRO(status))
	status = ascs_dispatch_procedure(scb, &ursb);

    /*
    ** Finish the procedure call by sending info to the caller.
    */
    stat = ascs_finish_procedure(scb, &ursb);

    return ((status == E_DB_OK) ? stat : status);
}


/*
** Name: ascs_bld_procedure -- build the GCA_INVPROC, GCA1_INVPROC messages
** Description:
** 	Use the GCA message to build a parameter list for the execute method.
** Input:
** 	scb				The SCD SCB
** 	ursb				User Request Services block
** 	Method				Contains name of method
** 	Interface			Name of the interface which owns
** 					the method.
** Output:
** Return:
** History:
**    31-Dec-1997 (shero03)
**        created for Frontier
** 	19-Jan-1998 (wonst02)
** 	    Added call to URS to build parameters for the method.
**  11-Feb-2005 (fanra01)
**      Rmoved urs_call reference.
*/

DB_STATUS
ascs_bld_procedure(SCD_SCB 	*scb,
    		  URSB    	*ursb,
		  char		*Method,
		  char		*Interface)
{
    DB_STATUS		status = E_DB_OK;
    GCA_DATA_VALUE	value_hdr;
    i4			ParmNameLen;
    i4		ParmMask;
    char		ParmName[GCA_MAXNAME];
    char		Value[4096];
    URSB_PARM		*ursbparm = ursb->ursb_parm;
    DB_ERROR		err;

    ursb->ursb_flags = 0;
    
    /*
    ** Prevent further execution of this function since urs has been removed
    */
    ursb->ursb_error.err_code = E_SC0025_NOT_YET_AVAILABLE;
    sc0e_0_put( ursb->ursb_error.err_code, 0 );
	return E_DB_ERROR;
# if 0
    /*
    ** The first parameter in the GCA message has the number of parameters
    */
    status = ascs_get_parm(scb, &ParmNameLen, ParmName, &ParmMask,
			  &value_hdr, Value);
    if (DB_FAILURE_MACRO(status))
	return status;

    /*
    ** Call URSM with the parm information
    */
    ursbparm->interfacename = Interface;
    ursbparm->methodname = Method;
    ursbparm->data_value.db_data = (PTR)Value;
    ursbparm->data_value.db_length = (i4)value_hdr.gca_l_value;
    ursbparm->data_value.db_datatype = (DB_DT_ID)value_hdr.gca_type;
    ursbparm->data_value.db_prec = (i2)value_hdr.gca_precision;

    status = urs_call(URS_BEGIN_PARM, ursb, &err);
    if (status)
    {
	sc0e_0_put(err.err_code, 0);
	return status;
    }

    while (status == E_DB_OK && ascs_gca_data_available(scb))
    {
	status = ascs_get_parm(scb, &ParmNameLen, ParmName, &ParmMask,
			      &value_hdr, Value);
	if (DB_FAILURE_MACRO(status))
	    break;

	/*
	** Call URSM with the parm information
	*/
	ursbparm->data_value.db_data = (PTR)Value;
	ursbparm->data_value.db_length = (i4)value_hdr.gca_l_value;
	ursbparm->data_value.db_datatype = (DB_DT_ID)value_hdr.gca_type;
	ursbparm->data_value.db_prec = (i2)value_hdr.gca_precision;

	status = urs_call(URS_ADD_PARM, ursb, &err);
	if (status)
	{
	    sc0e_0_put(err.err_code, 0);
	    break;
	}
    }
# endif /* 0 */
    return (status);
}

/*
** Name: ascs_dispatch_procedure -- dispatch GCA_INVPROC, GCA1_INVPROC
** Description:
** Input:
** 	scb				The SCD SCB
** 	ursb				User Request Services block
** Output:
** Return:
** History:
**    31-Dec-1997 (shero03)
**        created for Frontier
** 	20-Jan-1998 (wonst02)
** 	    Added call to URS to execute the method.
** 	23-Jun-1998 (wonst02)
** 	    Use ursb_num_ele & ursb_num_recs fields instead of others.
**  11-Feb-2005 (fanra01)
**      Rmoved urs_call reference.
*/
DB_STATUS
ascs_dispatch_procedure(SCD_SCB *scb, URSB *ursb)
{
    DB_STATUS   status  = E_DB_OK;
    DB_STATUS   ret_val = E_DB_OK;
    DB_ERROR	err;

    /*
    ** Prevent further execution of this function since urs has been removed
    */
    ursb->ursb_error.err_code = E_SC0025_NOT_YET_AVAILABLE;
    sc0e_0_put( ursb->ursb_error.err_code, 0 );
	return E_DB_ERROR;
# if 0
    /*
    ** Call URSM to execute the method
    */
    ursb->ursb_flags = 0;

    do                                  /* A block to break from */
    {
	status = urs_call(URS_EXEC_METHOD, ursb, &err);

	if (DB_FAILURE_MACRO(status))
	    break;

	/*
	** Format and send results to FE
	*/
	if (ursb->ursb_num_ele &&
	    ursb->ursb_num_recs)
	{
	    status = ascs_format_results(scb, ursb);
	    if (DB_FAILURE_MACRO(status))
		break;
	}
    } while (0);

    if (DB_FAILURE_MACRO(status))
    	ret_val = E_DB_ERROR;

    /*
    **	Free the method's parameter list
    */
    status = urs_call(URS_FREE_PARM, ursb, &err);
    if (status)
    {
	sc0e_0_put(err.err_code, 0);
	if (ret_val == 0)
	    ret_val = E_DB_ERROR;
    }
# endif /* 0 */
    return (ret_val);
}

/*
** Name: ascs_finish_procedure -- respond to client
** Description:
** Input:
** 	scb				The SCD SCB
** 	ursb				User Request Services block
** Output:
** Return:
** History:
** 	16-apr-1998 (wonst02)
** 	    Original
*/
DB_STATUS
ascs_finish_procedure(SCD_SCB *scb, URSB *ursb)
{
    DB_STATUS   status  = E_DB_OK;
    DB_STATUS   ret_val;
    i4          qry_status = GCA_OK_MASK;
    i4          response_type = GCA_RESPONSE;
    bool        free_entry = FALSE;

    /*
    ** Check the status of the URSM call.
    */
    if (DB_FAILURE_MACRO(ursb->ursb_error.err_data))
    {
	ret_val = ursb->ursb_error.err_data;
	free_entry = TRUE;
	qry_status = GCA_FAIL_MASK;
	qry_status |= GCA_CONTINUE_MASK;
	qry_status |= GCA_END_QUERY_MASK;
    }
    else
    {
	ret_val = E_DB_OK;
    }

    if (   (scb->scb_sscb.sscb_state == SCS_PROCESS)
	|| (scb->scb_sscb.sscb_interrupt)
	|| (qry_status == GCA_FAIL_MASK))
    {
	if (scb->scb_sscb.sscb_interrupt == 0)
	{
	    /*
	    ** Prepare and format the response message
	    */
	    status = ascs_format_response(scb, response_type,
					 qry_status, ursb->ursb_num_recs);
	    if (DB_FAILURE_MACRO(status))
	    {
		ret_val |= status;
		goto func_exit;
	    }

	}
	else  /* interrupted */
	{
	    status = ascs_process_interrupt(scb);
	    if (DB_FAILURE_MACRO(status))
	    {
		ret_val |= status;
		goto func_exit;
	    }
	}
    }

    /*
    ** Send the result back to the client
    */
    ret_val |= ascs_gca_send(scb);

func_exit:
    return (ret_val);
}

/*
** Name: ascs_get_parm	- Get next parameter from GCA message
** Description:
** 	Get the next parameter from the GCA message
** Input:
** 	scb				The SCD SCB
** Output:
** Return:
** History:
** 	19-Jan-1998 wonst02
** 	    Original.
*/

DB_STATUS
ascs_get_parm(SCD_SCB 		*scb,
	     i4		*ParmNameLen,
	     char		*ParmName,
	     i4		*ParmMask,
	     GCA_DATA_VALUE	*value_hdr,
	     char		*Value)
{
    i4	val_hdr_size =
		    sizeof(value_hdr->gca_type) +
		    sizeof(value_hdr->gca_precision) +
		    sizeof(value_hdr->gca_l_value);
    DB_STATUS	status = E_DB_OK;

    /*
    ** read the parm name length
    */
    status = ascs_gca_get(scb, (PTR)ParmNameLen, sizeof(*ParmNameLen));
    if (DB_FAILURE_MACRO(status))
	return status;

    /*
    ** read the parm name
    */
    if (*ParmNameLen > 0)
    {
	status = ascs_gca_get(scb, (PTR)ParmName, *ParmNameLen);
	if (DB_FAILURE_MACRO(status))
	    return status;
    }

    /*
    ** read the parm mask
    */
    status = ascs_gca_get(scb, (PTR)ParmMask, sizeof(*ParmMask));
    if (DB_FAILURE_MACRO(status))
	return status;

    /*
    ** read the gca data value descriptor
    */
    status = ascs_gca_get(scb, (PTR)value_hdr, val_hdr_size);
    if (DB_FAILURE_MACRO(status))
	/*
	** We have only been able to read a part of the
	** message. This is an error.
	*/
	return status;

    if (value_hdr->gca_l_value > 0)
    {
	/*
	** read the gca data value
	** (Ensure there is enough room)
	*/
	status = ascs_gca_get(scb, Value, value_hdr->gca_l_value);
	if (DB_FAILURE_MACRO(status))
	    return status;
    }

    return E_DB_OK;
}
