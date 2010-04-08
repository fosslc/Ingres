/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <lo.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <cs.h>
#include    <iiapi.h>
#include    <ursm.h>

#include    <urs.h>

/*
** Internal Methods in the "Frontier" interface:
** 
** 	Retrieve the interfaces defined for this Frontier server, including:
** 	name, path to the ".img" file, and parameter used for initialization.
** 
** GetInterfaces([out] InterfaceName	varchar(32),
** 		 [out] InterfacePath	varchar(32),
** 		 [out] InterfaceParm	varchar(32))
** 
** 
** 	Retrieve the methods in the specified interface, including:
** 	name and number of arguments.
** 
** GetMethods([in] InterfaceName	varchar(32), 
** 	      [out] MethodName		varchar(32),
**	      [out] MethodArgCount	integer)
** 
** 
** 	Retrieve the arguments in the specified method, including:
** 	name, data type, nullable flag, length, precision, scale, and 
** 	usage (column type). 
** 
** GetArgs([in] InterfaceName	varchar(32), 
** 	   [in] MethodName	varchar(32), 
** 	   [out] ArgName	varchar(32),
**	   [out] ds_dataType	integer,	-- IIAPI_DT_ID
**         [out] ds_nullable	integer,	-- II_BOOL	
**         [out] ds_length	integer,	-- II_UNIT2	
**         [out] ds_precision	integer,	-- II_INT2	
**         [out] ds_scale	integer,        -- II_INT2	
**         [out] ds_columnType	integer,        -- II_INT2	
** # define IIAPI_COL_PROCBYREFPARM	1 -- procedure byref parm column type
** # define IIAPI_COL_PROCPARM		2 -- procedure parm column type   
**         [out] ds_columnName	varchar(32))	-- II_CHAR II_FAR *
** 
** 
**	Refresh an interface and all its methods by inactivating it, 
**	re-reading its definition from the repository, and activating it.
**
** RefreshInterface([in] InterfaceName	varchar(32))
*/

/*
** Global variables
*/

GLOBALREF   URS_MGR_CB		*Urs_mgr_cb;	/* URS Manager control block */

/*{
** Name: GetInterfaces
**
** Description:
** 	Retrieve the interfaces defined for this Frontier server, including:
** 	name, path to the ".img" file, and parameter used for initialization.
** GetInterfaces([out] InterfaceName	varchar(32),
** 		 [out] InterfacePath	varchar(32),
** 		 [out] InterfaceParm	varchar(32))
**
** Inputs:
**      None
**
** Outputs:
**      InterfaceName			Name of interface
**      InterfacePath                   Path the ".img" file
**      InterfaceParm                   Parameter for initialization
** 
** Returns:
**
** Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
** 	27-may-1998 (wonst02)
** 	    Original.
** 	18-jun-1998 (wonst02)
** 	    Add real workings.
**	11-Aug-1998 (fanra01)
**	    Removed C++ style comments for building on unix.
** 
*/
int
GetInterfaces(URS_MGR_CB	*ursm,
	      URSB		*ursb)
{
	DB_STATUS	status;

	status = urs_get_intfc(Urs_mgr_cb, ursb);

	return (status == E_DB_OK) ? 0 : 1;
}	

/*{
** Name: GetMethods
** 
** Description:
** 	Retrieve the methods in the specified interface, including:
** 	name and number of arguments.
** 
** GetMethods([in] InterfaceName	varchar(32), 
** 	      [out] MethodName		varchar(32),
**	      [out] MethodArgCount	integer)
** 
** Inputs:
**      InterfaceName                   Name of interface
**
** Outputs:
**      MethodName			Name of method
** 	MethodArgCount			Number of arguments for the method
** 
** Returns:
**
** Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
** 	27-may-1998 (wonst02)
** 	    Original.
**	11-Aug-1998 (fanra01)
**	    Removed C++ style comments for building on unix.
** 
*/
int
GetMethods(URS_MGR_CB	*ursm,
	   URSB		*ursb)
{
	DB_STATUS	status;

	status = urs_get_method(Urs_mgr_cb, ursb);

	return (status == E_DB_OK) ? 0 : 1;
}	

/*{
** Name: GetArgs
** 
** Description:
** 	Retrieve the arguments in the specified method, including:
** 	name, data type, nullable flag, length, precision, scale, and 
** 	usage (column type). 
** 
** GetArgs([in] InterfaceName	varchar(32), 
** 	   [in] MethodName	varchar(32), 
** 	   [out] ArgName	varchar(32),
**	   [out] ds_dataType	integer,	-- IIAPI_DT_ID
**         [out] ds_nullable	integer,	-- II_BOOL	
**         [out] ds_length	integer,	-- II_UNIT2	
**         [out] ds_precision	integer,	-- II_INT2	
**         [out] ds_scale	integer,        -- II_INT2	
**         [out] ds_columnType	integer,        -- II_INT2	
** # define IIAPI_COL_PROCBYREFPARM	1 -- procedure byref parm column type
** # define IIAPI_COL_PROCPARM		2 -- procedure parm column type   
**         [out] ds_columnName	varchar(32))	-- II_CHAR II_FAR *
** 
** Inputs:
**      InterfaceName                   Name of interface
**      MethodName			Name of method
**
** Outputs:
**      ArgName	     			Name of argument
**      ds_dataType	 		Type (IIAPI_DT_ID)
**      ds_nullable	 		Nullable flag (II_BOOL)
**      ds_length			Argument length
**      ds_precision			Number of precision digits
**      ds_scale			Scale
**      ds_columnType			Column usage (1=byref, 2=input parm)
** 
** Returns:
**
** Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
** 	27-may-1998 (wonst02)
** 	    Original.
**	11-Aug-1998 (fanra01)
**	    Removed C++ style comments for building on unix.
** 
*/ 
int
GetArgs(URS_MGR_CB	*ursm,
	URSB		*ursb)
{
	DB_STATUS	status;

	status = urs_get_arg(Urs_mgr_cb, ursb);

	return (status == E_DB_OK) ? 0 : 1;
}	

/*{
** Name: RefreshInterface
** 
** Description:
**	Refresh an interface and all its methods by inactivating it, 
**	re-reading its definition from the repository, and activating it.
**
** RefreshInterface([in] InterfaceName	varchar(32))
** 
** Inputs:
**      InterfaceName                   Name of interface
**
** Outputs:
** 
** Returns:
**
** Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
** 	27-may-1998 (wonst02)
** 	    Original.
** 
*/
int
RefreshInterface(char *InterfaceName)
{
	return 0;
}	
