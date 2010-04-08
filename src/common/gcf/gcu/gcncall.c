/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>

#include    <qu.h>
#include    <si.h>
#include    <sl.h>
#include    <tm.h>

#include    <iicommon.h>
#include    <gca.h>
#include    <gcn.h>
#include    <gcnint.h>

/**
**
**  Name: gcncall.c
**
**  Description:
**      Contains the following functions:
**
**          IIgcn_call - User entry routine for GCN
**
**
**  History:    $Log-for RCS$
**	    
**      03-Sep-87   (lin)
**          Initial creation.
**	29-Nov-95 (gordy)
**	    Standardized interface, added terminate function.
**	 8-Dec-95 (gordy)
**	    Internalized the GCN_SHUTDOWN command for IINAMU.
**	11-Jan-96 (gordy)
**	    GCN_SHUTDOWN was returning the wrong status, added missing break.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-Mar-02 (gordy)
**	    Removed unused GCN_SECURITY.
**/

/*{
** Name: IIGCn_call	- GCN external entry function
**
** Description:
**	This routine is the common interface entry providing Name Service to
**	the caller. Currently it is only called by the Name Server 
**	Management Utility (NSMU). No other user program should use this
**	interface.
**
** Inputs:
**      service_code			Identifier of the requested GCN service.
**      parameter_list                  Pointer to the data structure containing
**                                      the parameters required by the requested
**                                      service.
**
** Outputs:
**
**
**      status                          Result of the service request execution.
**                                      The following values may be returned:
**
**		E_GCN000_OK		Request accepted for execution.
**		E_GCN003_INV_SVC_CODE	Invalid service code
**		E_GCN004_INV_PLIST_PTR	Invalid parameter list pointer
**
**
**	Returns:
**	    DB_STATUS:
**		E_DB_OK		
**		E_DB_ERROR		
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**
**      03-Sep-87 (Lin)
**          Initial function creation.
**
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	29-Nov-95 (gordy)
**	    Standardized interface, added terminate function.
**	 8-Dec-95 (gordy)
**	    Internalized the GCN_SHUTDOWN command for IINAMU.
**	11-Jan-96 (gordy)
**	    GCN_SHUTDOWN was returning the wrong status, added missing break.
**	22-Mar-02 (gordy)
**	    Removed unused GCN_SECURITY.
*/

STATUS
IIGCn_call( i4  service_code, GCN_CALL_PARMS *parameter_list )
{
    STATUS return_code = E_DB_OK;

    switch( service_code )
    {
	case GCN_INITIATE:
	    return_code = IIgcn_initiate();
	    break;    

	case GCN_TERMINATE:
	    return_code = IIgcn_terminate();
	    break;

	case GCN_DEL:
	case GCN_ADD:
	case GCN_RET:
	    return_code = IIgcn_oper( service_code, parameter_list );
	    break;

	case GCN_SHUTDOWN:
	    return_code = IIgcn_stop();
	    break;

	default:
		return_code = E_DB_ERROR;
		break;
    }

    return( return_code );
} /* end IIGCn_call */
