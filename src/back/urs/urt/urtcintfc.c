/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <me.h>
#include    <st.h>
#include    <tm.h>

/*
** Test Methods in the "FasC" interface:
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
** GetTime([out] Time	varchar(32))
**
** 
** Sum([in] i4, [in] i4, [out] i4)
** 
*/

/*
** Globals
*/
static char sHello[] = "\005\000Hello";  /* These are Ingres VARCHARs */		
static char sWorld[] = "\005\000World";
static char sGoodbye[] = "\007\000Goodbye";
static char sNull[] = "\000\000";
static i4   iOne = 1;

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
** 	01-jul-1998 (wonst02)
** 	    Original.
** 
*/
int
GetInterfaces(/*out*/	char **InterfaceName,
	      /*out*/	char **InterfacePath,
	      /*out*/	char **InterfaceParm)
{
	STATUS		status;

	*InterfaceName = MEreqmem(0, sizeof sHello, FALSE, &status);
	MEcopy(sHello, sizeof sHello, *InterfaceName);
	*InterfacePath = MEreqmem(0, sizeof sNull, FALSE, &status);
	MEcopy(sNull, sizeof sNull, *InterfacePath);
	*InterfaceParm = MEreqmem(0, sizeof sNull, FALSE, &status);
	MEcopy(sNull, sizeof sNull, *InterfaceParm);

	return status;
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
** 
*/
int
GetMethods(/*in*/	char	*InterfaceName,
	   /*out*/	char	**MethodName,
	   /*out*/	i4	**MethodArgCount)
{
	STATUS		status;

	*MethodName = MEreqmem(0, sizeof sWorld, FALSE, &status);
	MEcopy(sWorld, sizeof sWorld, *MethodName);
	*MethodArgCount = (i4 *)MEreqmem(0, sizeof iOne, FALSE, &status);
	**MethodArgCount = iOne;

	return status;
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
** 
*/ 
int
GetArgs(/*in*/	char	*InterfaceName,
	/*in*/	char	*MethodName, 
	/*out*/	char	**ArgName,
	/*out*/	i4	**ds_dataType,
	/*out*/	i4	**ds_nullable,
	/*out*/	i4	**ds_length,
	/*out*/	i2	**ds_precision,
	/*out*/	i2	**ds_scale,
	/*out*/	i2	**ds_columnType)
{
	STATUS		status;

	*ArgName = MEreqmem(0, sizeof sGoodbye, FALSE, &status);
	MEcopy(sGoodbye, sizeof sGoodbye, *ArgName);
	*ds_dataType = (i4 *)MEreqmem(0, sizeof **ds_dataType, FALSE, &status);
	**ds_dataType = iOne;
	*ds_nullable = (i4 *)MEreqmem(0, sizeof **ds_nullable, FALSE, &status);
	**ds_nullable = iOne;
	*ds_length = (i4 *)MEreqmem(0, sizeof **ds_length, FALSE, &status);
	**ds_length = iOne;
	*ds_precision = (i2 *)MEreqmem(0, sizeof **ds_precision, FALSE, &status);
	**ds_precision = (i2)iOne;
	*ds_scale = (i2 *)MEreqmem(0, sizeof **ds_scale, FALSE, &status);
	**ds_scale = (i2)iOne;
	*ds_columnType = (i2 *)MEreqmem(0, sizeof **ds_columnType, FALSE, &status);
	**ds_columnType = (i2)iOne;

	return status;
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

/*{
** Name: GetTime - Get current time
** 
** Description:
**
** 	GetTime([out] Time	varchar(32))
** 
** Inputs:
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
** 	02-jul-1998 (wonst02)
** 	    Original.
** 
*/
int
GetTime(/*out*/	char	**Time)
{
	STATUS		status;
	SYSTIME		stime;

	TMnow(&stime);

	*Time = MEreqmem(0, 512, FALSE, &status);
	TMstr(&stime, *Time + 2);
	*(i2 *)*Time = STlength(*Time+2);

	return status;
}

/*{
** Name: Sum - return sum of two numbers
** 
** Description:
**
** 	Sum([in] i4, [in] i4, [out] i4)
** 
** Inputs:
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
** 	02-jul-1998 (wonst02)
** 	    Original.
** 
*/
int
Sum(/*in*/	i4	*i,
    /*in*/	i4	*j,
    /*out*/	i4	**result)
{
	STATUS		status;

	*result = (i4 *)MEreqmem(0, sizeof **result, FALSE, &status);
	**result = *i + *j;

	return status;
}
