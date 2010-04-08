/*
**Copyright (c) 2004 Ingres Corporation
*/


/**
**
**  Name: DMFSINFO.C - Entry point into DMF used by the ADF FEXI functions
**  			wanting session information:
**			 last_identity() - last number generated for an 
**			 		identity column in this session
**
**  Description:
**
**	    dmf_last_id - Entry point for ADF FEXI functions 
**			   last_identity()
**
**
**  History:
**  	7-sep-2009 (stephenb)
**  	    Created.
*/

#include    <compat.h>
#include    <gl.h>
#include    <tm.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>

#include    <ex.h>
#include    <cs.h>
#include    <st.h>
#include    <me.h>
#include    <pc.h>

#include    <dmf.h>
#include    <dm.h>
#include    <dmccb.h>
#include    <dmdcb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dmucb.h>
#include    <dmmcb.h>
#include    <lk.h>
#include    <lg.h>
#include    <scf.h>
#include    <adf.h>

#include    <dmp.h>
#include    <dml.h>
#include    <dmu.h>
#include    <dmm.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dmd.h>


/*
 * Name: dmf_last_id - get last identifier generated for this session
 * 
 * Description:
 * 	get most recent number generated for an identity column in this
 * 	session. 
 * 	NOTE: currently identity columns must be an integer type (int or bigint)
 * 	      the function assumes that, and returns an integer value only
 * 
 * Inputs:
 * 	None.
 * 
 * Outputs:
 * 	id - The last number generated for an identifier column in this session
 * 
 * Returns:
 * 	E_DB_OK - got last ID
 * 	E_DB_WARN - No last id
 * 
 * History:
 * 	7-sep-2009 (stephenb)
 * 	    Created.
*/
DB_STATUS
dmf_last_id(i8	*id)
{
    DML_SCB	*scb;
    CS_SID	sid;
    
    CSget_sid(&sid);
    scb = GET_DML_SCB(sid);
    
    if (*id = scb->scb_last_id.intval)
    	return(E_DB_OK);
   else
       return(E_DB_WARN);
}
