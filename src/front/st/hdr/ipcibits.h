/*
**  Copyright (c) 2004 Ingres Corporation
**
**  ipcibits.h -- translation table that maps between the names and the
**  values of the bits in an authorization string.  This allows the
**  release description file to use a symbolic name for each licensing bits.
**
**  History:
**
**	xx-xxx-92 (jonb)
**		Created.
**	14-nov-93 (tyler)
**		Replaced hard-coded licensing bits with definitions from
**		<cicl.h>
**	29-mar-94 (vijay)
**		change ci-full-runtime and ci-part-runtime to the latest.
**	27-apr-94 (tomm)
**		Add C++ capibility.
**	23-jun-94 (michael)
**		Added new CI macros recently added to cicl.h
**	28-apr-95 (forky01)
**		Corrected CI_B1_SECURE #ifdef which had two definitions
**		in cibits for CI_B1_SECURE.  Remove obsolete CI_C2SECURE
**		entry which hard-coded to 43, an unused slot.  This could
**		have caused problems later.
**	02-apr-96 (rajus01)
**            Added  CI_INGRES_BRIDGE  capability.
**	09-nov-96 (mcgem01)
**		Added CI_OPAL capability.
**	25-mar-1998 (canor01)
**		Added GATEWAY cabability bits.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	31-oct-2002 (mofja02)
**	    Added CI_DB2_GATEWAY it was missing.
**	24-Feb-2005 (bonro01)
**	    Added CI_B1_SECURE back to support upgrade.
*/

# include <cicl.h>

typedef struct _CIBIT
{
    char *name;
    i4  bitno;

} CIBIT;

CIBIT cibits[] =
{ 
    "CI_BACKEND",		CI_BACKEND,
    "CI_CREATE_TABLE",		CI_CREATE_TABLE,
    "CI_VAR_INGRES",		CI_VAR_INGRES,
    "CI_INGRES_NET",		CI_INGRES_NET,
    "CI_QUEL_TM",		CI_QUEL_TM,
    "CI_SQL_TM",		CI_SQL_TM,
    "CI_ABF",			CI_ABF,
    "CI_GBF",			CI_GBF,
    "CI_NEW_GBF",		CI_NEW_GBF,
    "CI_MENU",			CI_MENU,
    "CI_OSLQUEL",		CI_OSLQUEL,
    "CI_OSLSQL",		CI_OSLSQL,
    "CI_QBF",			CI_QBF,
    "CI_RBF",			CI_RBF,
    "CI_REPORT",		CI_REPORT,
    "CI_VIFRED",		CI_VIFRED,
    "CIeqADA",			CIeqADA,
    "CIeqBASIC",		CIeqBASIC,
    "CIeqC",			CIeqC,
    "CIeqCOBOL",		CIeqCOBOL,
    "CIeqFORTRAN",		CIeqFORTRAN,
    "CIeqPASCAL",		CIeqPASCAL,
    "CIeqPL1",			CIeqPL1,
    "CIesqADA",			CIesqADA,
    "CIesqBASIC",		CIesqBASIC,
    "CIesqC",			CIesqC,
    "CIesqCOBOL",		CIesqCOBOL,
    "CIesqFORTRAN",		CIesqFORTRAN,
    "CIesqPASCAL",		CIesqPASCAL,
    "CIesqPL1",			CIesqPL1,
    "CI_esqCPLUSPLUS",		CI_esqCPLUSPLUS,
    "CI_PCLINK",		CI_PCLINK,
    "CI_CLUSTER_FLAG",		CI_CLUSTER_FLAG,
    "CI_BLOCK_3270",		CI_BLOCK_3270,
    "CI_DBE",			CI_DBE,
    "CI_CRGBLAST",		CI_CRGBLAST,
    "CI_RMS_GATEWAY",		CI_RMS_GATEWAY,
    "CI_SS_STAR",		CI_SS_STAR,
    "CI_RDB_GATEWAY",		CI_RDB_GATEWAY,
    "CI_PM_GATEWAY",		CI_PM_GATEWAY,
    "CI_VISION",		CI_VISION,
    "CI_RESRC_CNTRL",		CI_RESRC_CNTRL,
    "CI_USER_ADT",		CI_USER_ADT,
    "CI_ABF_IMAGE",		CI_ABF_IMAGE,
    "CI_INGTCP_NET",		CI_INGTCP_NET,
    "CI_INGSNA_NET",		CI_INGSNA_NET,
    "CI_WINDOWS4GL_DEV",	CI_WINDOWS4GL_DEV,
    "CI_WINDOWS4GL_RUNTIME",	CI_WINDOWS4GL_RUNTIME,
    "CI_CONVERSION_TOOLS",	CI_CONVERSION_TOOLS,
    "CI_REPLICATOR",		CI_REPLICATOR,
    "CI_CLASSOBJ_ADT",		CI_CLASSOBJ_ADT,
    "CI_XA_TUXEDO",		CI_XA_TUXEDO,
    "CI_XA_CICS",		CI_XA_CICS,
    "CI_XA_ENCINA",		CI_XA_ENCINA,
    "CI_B1_SECURE",		CI_BACKEND,
    "CI_SUPPORT",		CI_SUPPORT,
    "CI_esqCPLUSPLUS",		CI_esqCPLUSPLUS,
    "CI_PER_USER_LICENSING",	CI_PER_USER,
    "CI_INGRES_BRIDGE",		CI_INGRES_BRIDGE,
    "CI_OPAL",			CI_OPAL,
    "CI_ORACLE_GATEWAY",	CI_ORACLE_GATEWAY,
    "CI_INFORMIX_GATEWAY",	CI_INFORMIX_GATEWAY,
    "CI_SYBASE_GATEWAY",	CI_SYBASE_GATEWAY,
    "CI_MSSQL_GATEWAY",		CI_MSSQL_GATEWAY,
    "CI_ODBC_GATEWAY",		CI_ODBC_GATEWAY,
    "CI_DB2_GATEWAY",		CI_DB2_GATEWAY,
    "CI_ALB_GATEWAY",		CI_ALB_GATEWAY,
    "CI_DB2_GATEWAY",		CI_DB2_GATEWAY,
    "CI_DCOM_GATEWAY",		CI_DCOM_GATEWAY,
    "CI_IDMS_GATEWAY",		CI_IDMS_GATEWAY,
    "CI_IMS_GATEWAY",		CI_IMS_GATEWAY,
    "CI_VSAM_GATEWAY",		CI_VSAM_GATEWAY,
    "CI_NEW_RMS_GATEWAY",	CI_NEW_RMS_GATEWAY,
    "CI_NEW_RDB_GATEWAY",	CI_NEW_RDB_GATEWAY,
    NULL, -1
};
