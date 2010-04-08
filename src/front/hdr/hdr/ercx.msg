SQLSTATE_MAPPING_REQUIRED

/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ercx.msg -	Message file for class CX
**
** Description:
**	This file defines the messages for class CX. These are generalized
**      Context Mgmt routines, initially for use by LIBQXA, to be enhanced
**      for use by LIBQ, as well as other facilities, at a later time.
**
** History:
**	01-oct-1992 (mani)
**		Created.
**	17-dec-1992 (sandyd)
**		Added "NO_SQLSTATE_MAPPING" directive at the end, so that this
**		message file can be concatenated with all the other FE .msg 
**		files and ercompiled as a unit.
**	19-oct-96 (mcgem01)
**		MISC_ERRORS change to MISC_ERRORS for the Jasmine/Ingres
**		code sharing project.
**/

# define	_CX_CLASS        215


E_CX0001_INTERNAL:SS50000_MISC_ERRORS
"Internal Error detected. Contact the Ingres CS&S \n
for further assistance. The error is: %0c."


/*
** Explanation:
**      An internal error was found. This is unexpected. Please contact 
**      Ingres CS&S folks for follow-up and assistance.
**
** System Status:
**
** User Action:
*/


E_CX0002_CX_CB_ALLOC:SS50003_EXHAUSTED_RESOURCE
 "Unable to allocate memory: cl_status = %0c \n
for control block = %1c."

/*
** Explanation:
**	When connecting to the database, an internal descriptor is
**	allocated to maintain the database session's state. If this is an
**      XA application, each active XA transaction at this application server
**      (AS) also requires a number of internal structures. Either of these
**      could not be allocated due to the shortage of memory. Try one of the
**      following:
**      a) reduce the number of database connections and/or concurrent XA
**         transactions in this application program, (start more ASs)
**      b) try closing some cursors, if too many are open at the same time.
**      
**
** System Status:
**
** User Action:
*/


E_CX0003_CX_CB_FREE:SS50000_MISC_ERRORS
"Unable to free memory: cl_status = %0c \n
for control block = %1c. Contact your INGRES DBA or the Ingres CS&S \n
for further assistance."

/*
** Explanation:
**	When connecting to the database, an internal descriptor is
**	allocated to maintain the database session's state. If this is an
**      XA application, each active XA transaction at this application server
**      (AS) also requires a number of internal structures. Either of these
**      could not be allocated due to the shortage of memory. Try one of the
**      following:
**      a) reduce the number of database connections and/or concurrent XA
**         transactions in this application program, (start more ASs)
**      b) try closing some cursors, if too many are open at the same time.
**      
**
** System Status:
**
** User Action:
*/



E_CX0004_BAD_CX_ID:SS22023_INV_PARAM_VAL
"Internal Error. Bad CX_ID. Cx_type = %0c \n
Operation = %1c.  CX_ID = %2c."


/*
** Explanation:
**      A bad CX_ID was found. The CX_TYPE and the CX_ID encountered are
**      returned. 
**
** System Status:
**
** User Action:
*/


E_CX0005_BAD_CX_TYPE:SS22023_INV_PARAM_VAL
"Internal Error. Bad cx_type. Cx_type = %0c \n
Operation = %1c."


/*
** Explanation:
**      A bad CX_TYPE was found. The CX_TYPE is returned.
**
** System Status:
**
** User Action:
*/



E_CX0006_DUP_CX_ID:SS50000_MISC_ERRORS
"Internal Error. Duplicate CX_ID. Cx_type = %0c \n
Operation = %1c.  CX_ID = %2c."


/*
** Explanation:
**      A bad CX_ID was found. The CX_TYPE and the CX_ID encountered are
**      returned. 
**
** System Status:
**
** User Action:
*/


E_CX0007_BAD_XA_XID:SS22023_INV_PARAM_VAL
"Internal Error. Invalid XA XID format found. \n
XA XID = %0c."


/*
** Explanation:
**      An Invalid XA XID was found.
**
** System Status:
**
** User Action:
*/

E_CX0008_DUP_RMI_NAME_OR_ID:SS22023_INV_PARAM_VAL
"User Error:The database name or static sid specified in the RMI open string \n
is not unique across all RMI open strings bound to this TP application \n
program. static sid = %0c, database name = %1c."


/*
** Explanation:
**      Two or more RMI open strings cannot specify the same database name 
**      or the same static RMI identifier (session identifier).
**
** System Status:
**
** User Action:
*/



E_CX0009_RMI_NOT_FOUND:SS22023_INV_PARAM_VAL
"User Error: RMI information for RMI id = %0c, was not found. Check the 
initialization sequence and/or the ESQL switching code to this RMI."

/*
** Explanation:
**      The RMI CB was not found. There may have been an AS programmer error.
**      The RMI id specified in the ESQL set_sql sequence may not have been
**      previously bound to the AS program at initialization time.
**
** System Status:
**
** User Action:
*/

NO_SQLSTATE_MAPPING
