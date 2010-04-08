/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <cs.h>
#include    <pc.h>
#include    <dbdbms.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <st.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <scf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dm.h>
#include    <dmxcb.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dm0m.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0c.h>
#include    <dmxe.h>
#include    <dm2d.h>
#include    <dm0pbmcb.h>

/**
** Name: DMCSHOW.C - Functions used to show characteristics.
**
** Description:
**      This file contains the functions necessary to show characteristics.
**      This file defines:
**    
**      dmc_show() -  Routine to perform the show
**                     characteristics operation.
**
** History:
**      01-sep-85 (jennifer)
**          Created.      
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**	20-may-1993 (robf)
**	    Implemented basic functions.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	13-sep-93 (swm)
**	    Added cs.h include to pickup definition of CS_SID before
**	    dmtcb.h include.
**	13-oct-93 (robf)
**          Shuffle include files so cs gets pc definitions.
**	02-jan-94 (andre)
**	    finally made dmc_show() something more than NOOP - it can now be 
**	    used to request information about journaling status of a database
**	5-jan-94 (robf)
**          Added support for DMC_C_XACT_TYPE request
**	06-mar-96 (nanpr01)
**          Added support for inquiring the pool sizes. 
**          Added support for inquiring the max tuple size based on pool size. 
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Jan-2001 (jenjo02)
**	    Find DML_SCB with macro instead of calling SCF.
**      06-apr-2001 (stial01)
**          Added DMC_DBMS_CONFIG_OP
**      22-oct-2002 (stial01)
**          Added DMC_SESS_LLID
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**      14-jan-2009 (stial01)
**          Added support for dbmsinfo('pagetype_v5')
**/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */


/*{
** Name: dmc_show - Shows control characteristics.
**
**  INTERNAL DMF call format:    status = dmc_show(&dmc_cb);
**
**  EXTERNAL call format:        status = dmf_call(DMC_SHOW,&dmc_cb);
**
** Description:	
**
**    The dmc_show function handles the showing of control information
**    for a server, asession, or a database.  This provides the caller with
**    a way to determine the current operating state of each of ehese objects.
**    The values displayed will be those currently defined to the system.
**    These values may differ fromt he default settings if the dmc_alter 
**    operation has been executed or a characteristic was altered when 
**    the object was initialized.  
**    This routine only returns information on one object
**    at a time and will only accept valid object identifiers.
**
** Inputs:
**      dmc_cb 
**          .type                           Must be set to DMC_CONTROL_CB.
**          .length                         Must be at least sizeof(DMC_CB).
**          .dmc_op_type                    Must be DMC_SERVER_OP or
**                                          DMC_DB_OP, DMC_SESSION_OP.
**                                          DMC_BMPOOL_OP or DMC_DBMS_CONFIG_OP
**					    NOTE: for now only DMC_DB_OP and
**					    DMC_SESSION_OP and DMC_BMPOOL_OP
**					    are  supported.
**          .dmc_session_id                 Must be a unique session identifier
**					    returned from a begin session 
**					    operation.
**	    .dmc_flags_mask		    indicates what info is requested
**		DMC_JOURNAL		    may be set if dmc_op_type==DMC_DB_OP
**					    indicates that the caller is 
**					    interested in journaling status of 
**					    a database
**          .dmc_char_array                 Pointer to an area used to return
**                                          an array of characteristics 
**                                          entries of type DMC_CHAR_ENTRY.
**          .dmc_db_id                      Must be zero or if dmc_op_type
**                                          is DMC_DATABASE_OP then it must be
**                                          a valid database identifier 
**                                          returned from an add database
**                                          operation.
**
** Output:
**      dmc_cb 
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM0010_BAD_DB_ID
**					    E_DM002A_BAD_PARAMETER
**                                          E_DM002F_BAD_SESSION_ID
**					    E_DM009F_ILLEGAL_OPERATION
**
**	    .dmc_xact_id	            DMF transaction id if session
**					    transaction id requested, NULL
**					    if no current transaction
**
**          .dmc_flags_mask                 If dmc_op_type is DMC_DATABASE_OP
**                                          will be DMC_NOJOURNAL or
**                                          DMC_JOURNAL.
**          .dmc_name                       Will be server or session name
**                                          depending on dmc_op_type.
**          .dmc_char_array                 Pointer to an area used to output
**                                          characteristic entries.  The
**                                          entries are of type DMC_CHAR_ENTRY.
**                                          See below for description of
**                                          <dmc_char_array> entries.
**
**
**          .dmc_db_name                    If dmc_op_type is DMC_DATABASE_OP
**                                          will be the database name.
**          .dmc_db_location                If dmc_op_type is DMC_DATABASE_OP
**                                          will be database location.
**          .dmc_db_access_mode             If dmc_op_type is DMC_DATABASE_OP
**                                          will be DMC_RS_READ_SHARE,
**                                          DMC_RE_READ_EXCL, 
**                                          DMC_WS_WRITE_SHARE, or
**                                          DMC_WE_WRITE_EXCL
**          .dmc_db_owner                   If dmc_op_type is DMC_DATABASE_OP
**
**          <dmc_char_array> entries are of type DMC_CHAR_ENTRY and
**          must have the following values:
**          char_id                         Will be one of following:
**					    DMC_C_DB_JOURN
**          char_value                      Will be an integer value or
**                                          one of the following:
**                                          DMC_C_ON
**                                          DMC_C_OFF
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmc_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmc_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmc_cb.error.err_code.
** History:
**      01-sep-85 (jennifer)
**          Created.
**	20-may-93 (robf)
**	    Implemented, not a NOOP anymore
**	02-jan-94 (andre)
**	    it's no longer a NOOP.  Will support requests to provide journaling 
**	    status of a database
**	17-feb-94 (robf)
**          Merged two versions of routine, including both sets of
**	    functionality.
**	3-mar-94 (robf)
**          Added check for NULL odcb, preventing an AV when dereferenced.
**	    Note that this probably indicates an error elsewhere since the
**	    odcb should not be NULL here.
**	06-mar-96 (nanpr01)
**          Added support for inquiring the pool sizes. 
**          Added support for inquiring the max tuple size based on pool size. 
**	    Return a the maxtup length given a page size based on the
**	    size of the array.
**	06-may-96 (nanpr01)
**	    Modified the dm2u_maxreclen calls.
**	08-Jan-2001 (jenjo02)
**	    Find DML_SCB with macro instead of calling SCF.
**	10-May-2004 (schka24)
**	    ODCB scb ptr need not equal thread SCB if factotum, remove test.
**	    (I don't think one would normally get here from a factotum
**	    thread, but I'm not sure about XA transaction abort.)
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add DMC_C_PAGETYPE_V6, DMC_C_PAGETYPE_V7
**
** Design:
**      This function needs to do the following things:
**
**      1.  If the operation type is DMC_DATABASE_OP indicating an show 
**          of database characteristics, insure that the database 
**          identifier is valid.  If the identifier is invalid 
**          return an error.  Otherwise, update the call control
**          block from the database control block.  Return E_DB_OK.
**        
**      2.  If the operation type is DMC_SERVER_OP or DMC_SESSION_OP 
**          validate the control indentifier to insure it is either
**          a valid server identifier or session identifier which ever
**          is appropriate.  If it is invalid return an error.
**          Otherwise, get the information from the appropriate control 
**          block (i.e. either the server control block or the session control
**          block and update the call control block charateristic array.  
**          If the characteristic array is not large enough for all
**          characteristics, return as many as possible.
**
**      3.  Return E_DB_OK.
**	
*/

DB_STATUS
dmc_show(
DMC_CB    *dmc_cb)
{
    DMC_CB		*dmc = dmc_cb;
    DML_ODCB            *odcb;
    DML_SCB             *scb;
    DB_STATUS           status = E_DB_OK;
    DMC_CHAR_ENTRY      *chr;
    i4             chr_count;
    DML_XCB		*xcb;
    i4			i;
    DM_SVCB         	*svcb = dmf_svcb;
    i4		page_size, pgsize;

    CLRDBERR(&dmc->error);

    for(;;)
    {
	   chr = (DMC_CHAR_ENTRY*) dmc->dmc_char_array.data_address;
	   chr_count = 
		dmc->dmc_char_array.data_out_size / sizeof(DMC_CHAR_ENTRY);

	   /*
	   ** char aray must contain at least one entry, unless
	   ** getting session info
	   */
	   if ((!chr || !chr_count) && dmc->dmc_op_type!=DMC_SESSION_OP)
	   {
		SETDBERR(&dmc->error, 0, E_DM002A_BAD_PARAMETER);
	   }
	   switch(dmc->dmc_op_type)
	   {
	   case DMC_SERVER_OP:
		/*
		** Server operations, none yet
		*/
		break;

	    case DMC_DATABASE_OP:
		{
		/*
		** Database operations
		*/
		i4	valid_flags = DMC_JOURNAL;

		if (   dmc->dmc_flags_mask & ~valid_flags
			|| !(dmc->dmc_flags_mask & valid_flags))
		{
			SETDBERR(&dmc->error, 0, E_DM009F_ILLEGAL_OPERATION);
			break;
		}

		if ( (scb = GET_DML_SCB(dmc->dmc_session_id)) == 0 || 
		      dm0m_check((DM_OBJECT *)scb, (i4)SCB_CB) )
		{
		    SETDBERR(&dmc->error, 0, E_DM002F_BAD_SESSION_ID);
		    break;
		}

		odcb = (DML_ODCB *)dmc->dmc_db_id;

		if ( !odcb || dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) != E_DB_OK)
		{
			SETDBERR(&dmc->error, 0, E_DM0010_BAD_DB_ID);
			break;
		}

		if (dmc->dmc_flags_mask & DMC_JOURNAL)
		{
			/* report DB's journaling status */
			chr->char_id = DMC_C_DB_JOURN;

			if (odcb->odcb_dcb_ptr->dcb_status & DCB_S_JOURNAL)
			{
			    chr->char_value = DMC_C_ON;
			}
			else
			{
			    chr->char_value = DMC_C_OFF;
			}
	        }
	    }
	    break;

	    case DMC_SESSION_OP:
		/*
		** Session operations
		*/
		/* get session id */
		if ( (scb = GET_DML_SCB(dmc->dmc_session_id)) == 0 || 
		      dm0m_check((DM_OBJECT *)scb, (i4)SCB_CB) )
		{
			SETDBERR(&dmc->error, 0, E_DM002F_BAD_SESSION_ID);
			break;
		}


		/* Now figure out the info requested for the session */
		switch(dmc->dmc_flags_mask)
		{
		case DMC_XACT_ID:
			/*
			** Request transaction id (if any)
			*/
			if (scb->scb_x_ref_count == 0)
				dmc->dmc_xact_id=NULL;
			else
				dmc->dmc_xact_id= (PTR)scb->scb_x_next;
			break;

		case DMC_SESS_LLID:
			/*
			** Request session lock list
			*/
			dmc->dmc_scb_lkid = (PTR)&scb->scb_lock_list;
			break;

		default:
			SETDBERR(&dmc->error, 0, E_DM009F_ILLEGAL_OPERATION);
			status = E_DB_ERROR;
			break;
		}
	        for (i = 0; DB_SUCCESS_MACRO(status) && (i < chr_count); i++)
	        {
		   switch (chr[i].char_id)
		   {
		   case DMC_C_XACT_TYPE:
			/*
			** Get xact type
			*/
			if (scb->scb_x_ref_count == 0)
				chr[i].char_value=DMC_C_NO_XACT;
			else 
			{
				xcb=(DML_XCB*)scb->scb_x_next;
				if (xcb->xcb_x_type & XCB_RONLY)
					chr[i].char_value=DMC_C_READ_XACT;
				else
					chr[i].char_value=DMC_C_WRITE_XACT;
			}
			break;
		   default:
			SETDBERR(&dmc->error, 0, E_DM000D_BAD_CHAR_ID);
			status = E_DB_ERROR;
			break;
		   }
		}
		break;

	    case DMC_BMPOOL_OP:
		/*
		** Now figure out the info requested for the session
		**
		** For 2k pages the max tuple size depends on page type.
		** For 4k-64k pages max tuple size is INDEPENDENT of page type.
		** Since this information may be used for tuple buffer 
		** allocations, return the largest for any page type
		** Since V1 has smallest page/row overhead, use page_type V1
		*/
		switch(dmc->dmc_flags_mask)
		{
		  case DMC_MAXTUP :
		        {
		  	  chr[0].char_id = DMC_C_MAXTUP;
	        	  for (page_size = 2048; page_size <= 65536; 
		     	       page_size *= 2)
			  {
			    if (dm0p_has_buffers(page_size))
			      chr[0].char_value = dm2u_maxreclen(TCB_PG_V1, 
				      page_size);
			  }
			}
			break;
		  case DMC_TUPSIZE :
			if (chr_count == DM_MAX_CACHE)
			{
			  /*
			  ** Get from server what buffer pools are available
			  ** Should loop 6 times - 2K, 4K, 8K, 16K, 32K and 64K
			  */
	        	  for (i = 0, page_size = 2048; i < chr_count; 
		     	       i++, page_size *= 2)
	        	  {
		  	    chr[i].char_id = DMC_C_TUPSIZE;
			    if (dm0p_has_buffers(page_size))
			      chr[i].char_value = dm2u_maxreclen(TCB_PG_V1, 
						page_size);
		  	    else
		    	      chr[i].char_value = DMC_C_OFF;
	        	  }
			}
			else
			{
			  /*
			  ** Get from server what the max tuple size is given 
			  ** a page size. Donot bother to check here if
			  ** buffer pool exists for this page size. -- STAR
			  */
			  pgsize = chr[0].char_value;
	        	  for (i = 0, page_size = 2048; i < DM_MAX_CACHE; 
		     	       i++, page_size *= 2)
	        	  {
			    if (page_size == pgsize)
			    {
		  	      chr[0].char_id = DMC_C_TUPSIZE;
			      chr[0].char_value = dm2u_maxreclen(TCB_PG_V1, 
						page_size);
			      break;
			    }
	        	  }
			}
		        break;
		  default :
			/*
			** Get from server what buffer pools are available
			** Should loop 6 times - 2K, 4K, 8K, 16K, 32K and 64K
			*/
	        	for (i = 0, page_size = 2048; i < chr_count; 
		     	     i++, page_size *= 2)
	        	{
		  	  chr[i].char_id = DMC_C_BMPOOL;
			  if (dm0p_has_buffers(page_size))
		    	    chr[i].char_value = DMC_C_ON;
		  	  else
		    	    chr[i].char_value = DMC_C_OFF;
	        	}
		        break;
	        }
		break;

	    case DMC_DBMS_CONFIG_OP:
		for (i = 0; i < chr_count; i++)
		{
		    switch(chr[i].char_id)
		    {
			case DMC_C_PAGETYPE_V1:
			{
			    if (svcb->svcb_config_pgtypes & SVCB_CONFIG_V1)
				chr[i].char_value = DMC_C_ON;
			    else
				chr[i].char_value = DMC_C_OFF;
			    break;
			}
			case DMC_C_PAGETYPE_V2:
			{
			    if (svcb->svcb_config_pgtypes & SVCB_CONFIG_V2)
				chr[i].char_value = DMC_C_ON;
			    else
				chr[i].char_value = DMC_C_OFF;
			    break;
			}
			case DMC_C_PAGETYPE_V3:
			{
			    if (svcb->svcb_config_pgtypes & SVCB_CONFIG_V3)
				chr[i].char_value = DMC_C_ON;
			    else
				chr[i].char_value = DMC_C_OFF;
			    break;
			}
			case DMC_C_PAGETYPE_V4:
			{
			    if (svcb->svcb_config_pgtypes & SVCB_CONFIG_V4)
				chr[i].char_value = DMC_C_ON;
			    else
				chr[i].char_value = DMC_C_OFF;
			    break;
			}
			case DMC_C_PAGETYPE_V5:
			{
			    if (svcb->svcb_config_pgtypes & SVCB_CONFIG_V5)
				chr[i].char_value = DMC_C_ON;
			    else
				chr[i].char_value = DMC_C_OFF;
			    break;
			}
			case DMC_C_PAGETYPE_V6:
			{
			    if (svcb->svcb_config_pgtypes & SVCB_CONFIG_V6)
				chr[i].char_value = DMC_C_ON;
			    else
				chr[i].char_value = DMC_C_OFF;
			    break;
			}
			case DMC_C_PAGETYPE_V7:
			{
			    if (svcb->svcb_config_pgtypes & SVCB_CONFIG_V7)
				chr[i].char_value = DMC_C_ON;
			    else
				chr[i].char_value = DMC_C_OFF;
			    break;
			}
			default:
			    TRdisplay("Bad id %d\n", chr[i].char_id);
			    SETDBERR(&dmc->error, 0, E_DM000D_BAD_CHAR_ID);
			    status = E_DB_ERROR;
			    break;
		    }
		}
		break;
	    default:
		SETDBERR(&dmc->error, 0, E_DM001A_BAD_FLAG);
		status=E_DB_ERROR;
		break;
	    }
	    break;
    }
    return (status);
}
