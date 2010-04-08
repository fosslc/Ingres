/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <dmucb.h>
#include    <dmu.h>

/**
** Name:  DMUALTER.C - Functions used to alter a DMF characteristics.
**
** Description:
**      This file contains the functions necessary to alter configuration 
**      parameters on a table, server, session, database or transaction.
**      Common routines used by all DMU functions are not included here, but
**      are found in DMUCOMMON.C.  This file defines:
**    
**      dmu_alter()        -  Routine to perfrom the normal alter
**                            operation.
**
** History:
**      01-sep-85 (jennifer) 
**        Created.
**	17-jan-1990 (rogerk)
**	    Add support for not writing savepoint records in readonly xacts.
**	6-jul-1992 (bryanp)
**	    Prototyping DMF.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-nov-2005 (horda03) Bug 46859/INGSRV 2716
**          A TX can now be started with a non-journaled BT record. This
**          leaves the XCB_DELAYBT flag set but sets the XCB_NONJNL_BT
**          flag. Need to write a journaled BT record if the update
**          will be journaled.
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */


/*{
** Name:  dmu_alter - Alters DMF characteristics.
**
**  INTERNAL DMF call format:          status = dmu_alter(&dmu_cb);
**
**  EXTERNAL call format:              status = dmf_call(DMU_ALTER,&dmu_cb);
**
** Description: >>>>>>> NOT CURRENTLY IMPLEMENTED <<<<<<<
**
**      The dmu_alter function handles the modification of runtime 
**      or configuration parameters.  A configuration parameter is
**      permanent characteristic of a database.  Some of the characteristics
**      that have include: the database locations, the log file location,
**      journalled or non-journalled, readonly, etc.  A call to this
**      function is not allowed inside a user specified transaction.  The
**      caller specifies the configuration paramater changes by providing a
**      list of parameter identifiers indicating which parameter to change
**      and their correspnding values.
**
** Inputs:
**      dmu_cb  
**          .type                           Must be set to DMU_UTILITY_CB.
**          .length                         Must be at least sizeof(DMU_CB).
**          .dmu_trans_id                   Must be the current transaction 
**                                          identifier returned from the 
**                                          begin transaction operation.
**          .dmu_flags_mask                 Must be zero.
**          .dmu_conf_array.data_address    Pointer to an area used to input
**                                          an array of entries of type
**                                          DMU_CONF_ENTRY.  See below for
**                                          description of <dmu_conf_array> 
**                                          entries.
**          .dmu_conf_array.data_in_size    Length of conf_array  in bytes.
**
**          <dmu_conf_array> entries are of type DMU_CONF_ENTRY and 
**          must have following format:
**          conf_param_id                   Identifier of parameter.
**                                          Some parameter identifiers are:
**                                          DMU_P_LOG_FILE, DMU_P_OWNER,
**                                          DMU_P_VERSION, DMU_P_LOCATION.
**          conf_binary_value               Must be one of the configuration 
**                                          constants or any appropriate 
**                                          binary value.
**          conf_ascii_value.data_address   Pointer to an area containing an
**                                          ascii string.
**          conf_ascii_value.data_in_size   Length of ascii sting.
**          conf_type                       Must be DMU_PERMANENT or
**                                          DMU_DYNAMIC.
**
** Output:
**      dmu_cb
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM00XX_USER_INTR
**                                          E_DM00XX_USER_ABORT
**                                          E_DM00XX_BAD_CB_TYPE
**                                          E_DM00XX_BAD_CB_LENGTH
**                                          E_DM00XX_INTERNAL_ERROR
**                                          E_DM00XX_BAD_FLAG
**                                          E_DM00XX_BAD_TRANS_ID
**                                          E_DM00XX_BAD_TABLE_NAME
**                                          E_DM00XX_NONEXSISTENT_TABLE
**                                          E_DM00XX_BAD_TABLE_OWNER
**                                          E_DM00XX_REL_UPDATE_ERR
**                                          E_DM00XX_CONFIG_UPDATE_ERR
**                                          E_DM00XX_BAD_LOG_WRITE
**                                          E_DM00XX_BAD_ALTER_PARAM_ID
**                                          E_DM00XX_BAD_ALTER_PARAM_VALUE
**                                          E_DM00XX_BAD_ALTER_FLAG
**                     
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARNING                    Function completed normally with 
**                                          a termination status which is in 
**                                          dmu_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmu_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmu_cb.error.err_code.
**
** History:
**      01-sep-85 (jennifer) 
**          Created.
**	 3-mar-89 (EdHsu)
**	    Added param to dmxe_writebt interface
**	17-jan-1990 (rogerk)
**	    Add support for not writing savepoint records in readonly xacts.
**	    Pass xcb argument to dmxe_writebt routine.
**
** Design:
**      This function does the following things:
**
**      1.  First verify that the configuration  list entries are  valid 
**          (i.e. valid parameter ids, values match id, if table id 
**          specified, table name given in dmu_table_id, etc.).  If any 
**          are in error, return E_DB_ERROR, place the appropriate error 
**          into err_code and place the index of the one in error into 
**          err_data.  If an error is detected in any entry, no entries 
**          are processed. 
**
**      2.  Call dmxe_savepoint to have a restart point.
**
**      3.  Log each change with a new and a old value.
**
**      4.  Return E_DB_OK.
**                 
*/

DB_STATUS
dmu_alter(DMU_CB    *dmu_cb)
{
#ifdef NOTDONE
    DB_STATUS	status;

    /*
    ** If this is the first write operation for this transaction,
    ** then we need to write the begin transaction record.
    */
    if (xcb->xcb_flags & XCB_DELAYBT)
    {
	status = dmxe_writebt(xcb, FALSE, &dmu_cb->error);
	if (status != E_DB_OK)
	{
	    xcb->xcb_state |= XCB_TRANABORT;
	    break;
	}
    }
#endif /* NOTDONE */

    return (E_DB_OK);
}
