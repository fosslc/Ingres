/*
** Copyright (c) 1990, 2007 Ingres Corporation
** All Rights Reserved
*/
#ifndef	GL_INCLUDE
#define                 GL_INCLUDE      0
/**
** Name: GL.H - common header file for CL
**
** Description:
**      This file will contain all common defines/structures
**	which between various CL components, and also the mainline.
**	The members of this header file should not be machine
**      specific.
**	This file should logically follow compat.h in the list of
**	includes
**
** History:
**      14-feb-93 (ed)
**          initial creation for GL_MAXNAME
**	10-oct-1996 (canor01)
**	    Add generic system default defines.
**	31-oct-1996 (canor01)
**	    Allow system defaults to be found in DLLs.
**      11-dec-1996 (reijo01)
**          Change JASMINE_HOME to JAS_SYSTEM. Add global name for terminal
**          type aka TERM_INGRES.
**      11-dec-1996 (reijo01)
**          Add global name for executable prefixes.
**      17-dec-1996 (reijo01)
**          Add global name for build name (Used in forms).
**	26-mar-97 (mcgem01)
**	    Modify product name to OpenIngres.
**      09-oct-1997 (canor01)
**          Add generic system copyright message.
**      14-oct-1997 (funka01)
**          Add generic name for netutil.
**      13-feb-1998 (johnny)
**          Add 1998 to system copyright message.
**	24-mar-98 (mcgem01)
**	    Change the product name to Ingres for the new Ingres II
**	    evaluation release.
**	25-mar-98 (mcgem01)
**	    Update copyright notices.
**	08-apr-98 (mcgem01)
**	    Change product name to Ingres.
**	28-may-98 (mcgem01)
**	    Missed another instance of OpenIngres in this file.
**      13-aug-1998 (canor01)
**	    Add intial defaults for Rosetta.
**	14-aug-1998 (canor01)
**	    Use ROS_SYSTEM instead of ROS_HOME.
**	25-mar-1999 (mcgem01)
**	    Add initial defaults for other products and remove the Rosetta
**	    entries which are no longer required.
**	20-may-2000 (somsa01)
**	    For other products, the SYSTEM_TERM_TYPE should be TERM_<product>.
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**      05-Jan-2005 (hweho01)
**          Changed product name from Ingres to Ingres 2006.
**    	24-Jan-2006 (drivi01)
**	    SIR 115615
**	    Defined SYSTEM_SERVICE_NAME and SystemServiceName to reference
**	    windows service throughout the code.
**      28-Apr-2006 (hweho01)
**          Changed product name to Ingres 2007.
**      23-oct-2006 (stial01)
**          Changed product name for Ingres 2006 Release 2.
**      22-Jan-2007 (bonro01)
**          Changed product name for Ingres 2006 Release 3.
**	14-Jun-2007 (bonro01)
**          Changed product name for Ingres 2006 Release 4.
**	16-Jun-2008 (drivi01)
**	    Dropping "Ingres 2006" string from release name.
**	    Ingres release product name will be just "Ingres" now.
**	29-Sep-2009 (frima01) 122490
**	    Add prototype for iiCL_get_fd_table_size().
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	29-Nov-2010 (frima01) SIR 124685
**	    Prototype for iiCL_get_fd_table_size now in clnfile.h.
**/

/*
**  Forward and/or External typedef/struct references.
*/


/*
**  Defines of other constants.
*/

/* maximum length of an object in the DBMS */
#define                 GL_MAXNAME      256

/* define compile-time defaults */
# define SYSTEM_LOCATION_VARIABLE      "II_SYSTEM"
# define SYSTEM_LOCATION_SUBDIRECTORY  "ingres"
# define SYSTEM_PRODUCT_NAME           "Ingres"
# define SYSTEM_SERVICE_NAME	       "Ingres"
# define SYSTEM_DBMS_NAME              "INGRES"
# define SYSTEM_VAR_PREFIX             "II"
# define SYSTEM_CFG_PREFIX             "ii"
# define SYSTEM_CAT_PREFIX             "ii"
# define SYSTEM_ADMIN_USER             "ingres"
# define SYSTEM_SYSCAT_OWNER           "$ingres"
# define SYSTEM_TERM_TYPE              "TERM_INGRES"
# define SYSTEM_EXEC_NAME              "ing"
# define SYSTEM_ALT_EXEC_NAME          "ii"
# define SYSTEM_BUILD_NAME             "INGBUILD"
# define SYSTEM_NETUTIL_NAME           "netutil"
# define SYSTEM_COPYRIGHT_MSG          \
         "-- Copyright (c) 1989, 2007 Ingres Corporation"

# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF char *SystemLocationVariable     ;
GLOBALDLLREF char *SystemLocationSubdirectory ;
GLOBALDLLREF char *SystemProductName          ;
GLOBALDLLREF char *SystemServiceName	      ;
GLOBALDLLREF char *SystemDBMSName             ;
GLOBALDLLREF char *SystemVarPrefix            ;
GLOBALDLLREF char *SystemCfgPrefix            ;
GLOBALDLLREF char *SystemCatPrefix            ;
GLOBALDLLREF char *SystemAdminUser            ;
GLOBALDLLREF char *SystemSyscatOwner          ;
GLOBALDLLREF char *SystemTermType             ;
GLOBALDLLREF char *SystemExecName             ;
GLOBALDLLREF char *SystemAltExecName          ;
GLOBALDLLREF char *SystemBuildName            ;
GLOBALDLLREF char *SystemNetutilName          ;
GLOBALDLLREF char *SystemCopyrightMsg         ;
# else /* NT_GENERIC */
GLOBALREF char *SystemLocationVariable     ;
GLOBALREF char *SystemLocationSubdirectory ;
GLOBALREF char *SystemProductName          ;
GLOBALREF char *SystemServiceName	   ;
GLOBALREF char *SystemDBMSName             ;
GLOBALREF char *SystemVarPrefix            ;
GLOBALREF char *SystemCfgPrefix            ;
GLOBALREF char *SystemCatPrefix            ;
GLOBALREF char *SystemAdminUser            ;
GLOBALREF char *SystemSyscatOwner          ;
GLOBALREF char *SystemTermType             ;
GLOBALREF char *SystemExecName             ;
GLOBALREF char *SystemAltExecName          ;
GLOBALREF char *SystemBuildName            ;
GLOBALREF char *SystemNetutilName          ;
GLOBALREF char *SystemCopyrightMsg         ;
# endif /* NT_GENERIC */

#endif /* GL_INCLUDE */
