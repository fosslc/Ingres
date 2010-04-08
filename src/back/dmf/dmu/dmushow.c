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
** Name: DMUSHOW.C - Functions used to show DMF characteristics.
**
** Description:
**      This file contains the functions necessary to show configuration 
**      parameters on a table, server, session, database or transaction.
**      Common routines used by all DMU functions are not included here, but
**      are found in DMUCOMMON.C.  This file defines:
**    
**      dmu_show()        -  Routine to perform the normal show 
**                           operation.
**
** History:
**      01-sep-85 (jennifer)
**          Created.      
**	6-jul-1992 (bryanp)
**	    Prototyping DMF.
**
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */


/*{
** Name: dmu_show - Shows the current DMF characteristics.
**
**  INTERNAL DMF call format:      status = dmu_show(&dmu_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMU_SHOW,&dmu_cb);
**
** Description:
**    The dmu_show function handles the return of runtime or configuration
**    parameters.  Configuration parameters are permanent attributes of
**    a database that are stored in the configuration file in the database.
**    The caller specifies the parameters to be returned by providing a list
**    of parameter identifiers which indicate the corresonding parameter
**    values to be returned.
**
** Inputs:
**      dmu_cb 
**          .type                           Must be set to DMU_UTILITY_CB.
**          .length                         Must be at least sizeof(DMU_CB).
**          .dmu_conf_array.data_address    Pointer to an area used to input
**                                          an array of entries of type
**                                          DMU_CONF_ENTRY.
**                                          See below for description of 
**                                          <dmu_conf_array> entries.
**          .dmu_conf_array.data_in_size    Length of conf_array in bytes.
**
**          <dmu_conf_array> entries are of type DMU_CONF_ENTRY and
**          must have following format:
**          conf_param_id                   Identifier of parameter.
**          conf_binary_value               Will be set if the identifier has
**                                          a binary value.
**          conf_ascii_value.data_address   Pointer to an appropriate ascii 
**                                          string.  Will be filled in if the 
**                                          identfier has an ascii value.
**          conf_ascii_value.data_in_size   Length of ascii string.
**          conf_ascii_value.data_out_size  Length of result string.
**          conf_type                       Must be DMU_PERMANENT or
**                                          DBM_DYNAMIC.
**
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
**                                          E_DM00XX_BAD_TRAN_ID
**                                          E_DM00XX_BAD_TABLE_NAME
**                                          E_DM00XX_NONEXSISTENT_TABLE
**                                          E_DM00XX_BAD_TABLE_OWNER
**                                          E_DM00XX_REL_UPDATE_E_DMDMF
**                                          E_DM00XX_CONFIG_UPDATE_E_DMDMF
**                                          E_DM00XX_BAD_LOG_WRITE
**                                          E_DM00XX_BAD_ALTER_PARAM_ID
**                                          E_DM00XX_BAD_ALTER_PARAM_VALUE
**                                          E_DM00XX_BAD_ALTER_FLAG
**                     
**          .error.err_data                 Set to entry in error by returning
**                                          index into conf_array.
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
** History:
**      01-sep-85 (jennifer)
**          Created.
**
** Design:
**      This function needs to do the following things:
**
**      1.  Verify the dmu_type,dmu_flags_mask, and dmu_trans_id fields of
**          the call control block.
**        
**      2.  First verify that the set list entries are  valid (i.e. valid 
**          parameter ids, values match id, if table id specified, table 
**          name given in dmu_table_id, etc.).  If any are in error, return 
**          E_DB_ERROR, place the appropriate error into err_code and place 
**          the index of the one in error into err_data.  If an error is 
**          detected in any entry, no entries are processed. 
**
**      3.  For each permanent entry search the appropriate system table.  This
**          may require opening the system table (such as config) if it is
**          not open.  Place the value into the dmu_conf_array provided by the 
**          caller.
**
**      4.  For each temporary entry search the appropriate control block
**          in memory.  Place the value in the dmu_conf_array provided by the
**          caller.
**
**       5.  Return E_DB_OK.
**                 
*/

DB_STATUS
dmu_show(DMU_CB    *dmu_cb)
{
    return (E_DB_OK);
}
