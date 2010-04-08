//
//  IDMSUREG.H
//
//  Module Name:    idmsureg.h
//
//  description:    Registry definitions used by the Ingres server.
//
//	In Windows 95 and NT the registry replaces the ini files used in 
//	Windows 3.1.  For ODBC the structure of the ini files is mapped into
//	the registry with "sections" corresponding to "subkeys" and "keys"
//	corresponding to "values".
//  
//  The ODBCINST.INI file is mapped into the registry as a subkey of HKEY_LOCAL_MACHINE.
//  The ODBC.INI file is mapped into the registry twice, as a subkey of HKEY_LOCAL_MACHINE
//  for system data sources, and as a subkey of HKEY_CURRENT_USER for user data sources.
//  The structure of the data is basically the same as it was for the ini files.
//
//  The CADIDSI.INI file is no longer used.  The [Options] section is mapped into a
//  subkey of HKEY_LOCAL_MACHINE.  This includes all global options and default settings.
//  Data source definitions are now stored in the appropriate ODBC.INI subkey.  This
//  is done to support system and user data source definitions.  The structure of the
//  "caiddsi" information has changed also:
//
//  1. The [Custom] section is no longer used.  The one entry in it,
//     IdmsPath, is now found in the [Options] subkey under HKEY_LOCAL_MACHINE.
//
//  2. The [Data Sources], [Servers], and [Server <servername>] sections are
//     no longer used.  The dictionary, node, and alternate task are stored as
//     values of the data source definition of the ODBC.INI subkey. 
//
//	Strings defined for previous versions of the Server and related 
//	ODBC drivers are defined in idmseini.h.
//
//  Change History
//  --------------
//
//  date        programmer          description
//
//  01/31/1996  Dave Ross			Initial coding.
//  06/05/1996  Dave Ross			Revised for system/user DSNs.
//

#ifndef _INC_IDMSUREG
#define _INC_IDMSUREG

//
//	Global definitions:
//
#define REGSTR_PATH_IDMS           	"Software\\ComputerAssociates\\Ingres"
#define REGSTR_PATH_DBCS           	"Software\\ComputerAssociates\\Ingres\\DBCS Types"
#define REGSTR_PATH_DEFAULTS       	"Software\\ComputerAssociates\\Ingres\\Defaults"
#define REGSTR_PATH_OPTIONS        	"Software\\ComputerAssociates\\Ingres\\Options"
#define REGSTR_PATH_SERVER         	"Software\\ComputerAssociates\\Ingres\\Server "
#define REGSTR_PATH_SERVERS        	"Software\\ComputerAssociates\\Ingres\\Servers"
#define REGSTR_PATH_CURRENT_VERSION	"Software\\ComputerAssociates\\Ingres\\3.5"

//
//  UTIL Debug Options:
//
#define UTIL_TRACE_API		0x00000001          // Trace external calls
#define UTIL_TRACE_INTL		0x00000002          // Trace internal calls
#define UTIL_TRACE_ENTRY	0x00000004          // Trace DllEntry calls

//
//  UTIL Log File Options:
//
#define UTIL_LOG_APPEND     0x00000001          // append to existing log


#endif	// _INC_IDMSUREG
