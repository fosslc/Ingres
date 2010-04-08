# include	<compat.h>
# include	<gl.h>
# include	<te.h>

/*
** Copyright (c) 1985, 2003 Ingres Corporation
**
** gv.c -- INGRES Global Variable Definitions
**
LIBRARY = IMPCOMPATLIBDATA
**
**	History:
**	Log:	gv.c,v $
**		Revision 30.2  86/06/27  12:43:58  boba
**		Add initialization of globals (for hp5 port).
**		
**		Revision 30.1  85/08/27  07:44:14  wong
**		Extracted from old "gv.c" (which became "gver.roc").
**	10-oct-1996 (canor01)
**	    Add generic system global storage.
**      11-dec-1996 (reijo01)
**          Add generic terminal type global storage.
**      11-dec-1996 (reijo01)
**          Add generic executable prefix type global storage.
**      17-dec-1996 (reijo01)
**          Add generic build name for forms.
**	04-mar-1997 (canor01)
**	    Change GLOBALREFs to GLOBALDEFs.
**	09-oct-1997 (canor01)
**	    Add generic system copyright message.
**	14-oct-1997 (funka01)
**	    Add generic name for netutil. 
**	28-apr-1999 (mcgem01)
**	    Needed to add SystemAltExecName to differentiate the ii<blah>
**	    binaries from their edbc<blah> counterparts.
**	08-may-2003 (abbj03)
**	    Add include of compat.h so that this can be built on VMS.
**    	24-Jan-2006 (drivi01)
**	    SIR 115615
**	    Replaced references to SystemProductName with SystemServiceName
**	    and SYSTEM_PRODUCT_NAME with SYSTEM_SERVICE_NAME due to the fact
**	    that we do not want changes to product name effecting service name.
**		
*/

/*
**	global variable used by journal and qrymod
*/

char	Terminal[TTYIDSIZE + 1] ZERO_FILL;


/* 
** Generic system ids --
** global storage with compile-time default values 
*/

GLOBALDEF char *SystemLocationVariable     = SYSTEM_LOCATION_VARIABLE;
GLOBALDEF char *SystemLocationSubdirectory = SYSTEM_LOCATION_SUBDIRECTORY;
GLOBALDEF char *SystemProductName          = SYSTEM_PRODUCT_NAME;
GLOBALDEF char *SystemServiceName	   = SYSTEM_SERVICE_NAME;
GLOBALDEF char *SystemDBMSName             = SYSTEM_DBMS_NAME;
GLOBALDEF char *SystemVarPrefix            = SYSTEM_VAR_PREFIX;
GLOBALDEF char *SystemCfgPrefix            = SYSTEM_CFG_PREFIX;
GLOBALDEF char *SystemCatPrefix            = SYSTEM_CAT_PREFIX;
GLOBALDEF char *SystemAdminUser            = SYSTEM_ADMIN_USER;
GLOBALDEF char *SystemSyscatOwner          = SYSTEM_SYSCAT_OWNER;
GLOBALDEF char *SystemTermType             = SYSTEM_TERM_TYPE;
GLOBALDEF char *SystemExecName             = SYSTEM_EXEC_NAME;
GLOBALDEF char *SystemAltExecName          = SYSTEM_ALT_EXEC_NAME;
GLOBALDEF char *SystemBuildName            = SYSTEM_BUILD_NAME;
GLOBALDEF char *SystemCopyrightMsg         = SYSTEM_COPYRIGHT_MSG;
GLOBALDEF char *SystemNetutilName          = SYSTEM_NETUTIL_NAME;
