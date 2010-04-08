/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : libextnc.h: General Interfaces for the C-Allication
**    Project  : Generic Library (libextnc.lib) 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : General Interfaces for the C-Allication
**
** History:
**
** 18-May-2001 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 30-Jan-2002 (uk$so01)
**    SIR #106648, Enhance the position of Sql Assistant on pop-up.
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 14-Fev-2002 (uk$so01)
**    SIR #106648, Enhance library.
** 13-Jun-2003 (uk$so01)
**    SIR #106648, Take into account the STAR database for connection.
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 22-Aug-2003 (uk$so01)
**    SIR #106648, Add silent mode in IEA. 
** 03-Feb-2004 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX or COM:
**    Integrate Export assistant into vdba
**/

#if !defined(LIB_EXTERNC_CALL_HEADER)
#define LIB_EXTERNC_CALL_HEADER
#if defined (__cplusplus)
extern "C"
{
#endif
#include "constdef.h"

/*
** Import Assistant Interfaces:
** ************************************************************************************************
*/
typedef struct tagIIASTRUCT
{
	WCHAR wczNode [64 +1];
	WCHAR wczServerClass [64 +1];
	WCHAR wczDatabase [64 +1];
	WCHAR wczTable [64 +1];
	WCHAR wczTableOwner [64 +1];
	WCHAR wczNewTable [64 +1];
	WCHAR wczSessionDescription [256];

	WCHAR wczParamFile  [_MAX_PATH];
	WCHAR wczLogFile    [_MAX_PATH];

	BOOL bNewTable;
	BOOL bLoop;
	BOOL bBatch;

	//
	// If bStaticTreeData = TRUE, the IIA will put the following data into the tree:
	// -list of nodes = wczNode
	// -server class  = [wczServerClass]
	// -list of databases = wczDatabase
	// -table:
	//    - list of tables = if wczTable is not set then all tables of wczDatabase (excluded system objects)
	//    - list of tables = wczTable if wczTable is set.
	BOOL bStaticTreeData;
	BOOL bDisableTreeControl; // TRUE: Disable the tree control in the first step
	BOOL bDisableLoadParam;   // TRUE: Disable the Load Parameter button.
	int  nSessionStart;       // In-Process svriia.dll will use the number of session from 'm_nSessionStart+1'.

} IIASTRUCT;

/*
**  Function: Ingres_ImportAssistant, Ingres_ImportAssistantWithParam
**
**  Summary:  Invoke the wizard Ingres Import Assistant dialog.
**
**  Args:
**    -hwndCaller : Handle of Window of the caller.
**    -pStruct    : Pointer to the structure IIASTRUCT to passe extra information if needed.
**
**  Returns:  int
**    1 = OK.
**    2 = User has cancelled the import assistant
**    3 = FAIL (Failed to initialize COM Library)
**    4 = (REGDB_E_CLASSNOTREG): 
**        A specified class is not registered in the registration database. Also can indicate that 
**        the type of server you requested in the CLSCTX enumeration is not registered or the values 
**        for the server types in the registry are corrupt.
**    5 = (CLASS_E_NOAGGREGATION):
**        This class cannot be created as part of an aggregate. 
**    6 = (E_NOINTERFACE):
**        Fail to query interface pointer.
*/
int Ingres_ImportAssistant(HWND hwndCaller);
int Ingres_ImportAssistantWithParam(HWND hwndCaller, IIASTRUCT* pStruct);


/*
** Export Assistant Interfaces:
** ************************************************************************************************
*/
typedef struct tagIEASTRUCT
{
	WCHAR wczNode [64 +1];
	WCHAR wczServerClass [64 +1];
	WCHAR wczDatabase [64 +1];
	WCHAR wczSessionDescription [256];
	BSTR  lpbstrStatement; // The caller must take care of allocate & free this member
	WCHAR wczExportFile [_MAX_PATH];
	WCHAR wczExportFormat[64 +1]; // "CSV", "DBF", "XML", "FIXED", "COPY"
	BOOL  bLoop;
	WCHAR wczParamFile  [_MAX_PATH];

	BOOL  bSilent;                   // silent mode
	WCHAR wczLogFile    [_MAX_PATH]; // silent mode only
	WCHAR wczListFile   [_MAX_PATH]; // silent mode only
	BOOL  bOverWrite;                // silent mode only
	int  nSessionStart;       // In-Process svriia.dll will use the number of session from 'm_nSessionStart+1'.

} IEASTRUCT;

/*
**  Function: Ingres_ExportAssistant, Ingres_ExportAssistantWithParam
**
**  Summary:  Invoke the wizard Ingres Import Assistant dialog.
**
**  Args:
**    -hwndCaller : Handle of Window of the caller.
**    -pStruct    : Pointer to the structure IIASTRUCT to passe extra information if needed.
**
**  Returns:  int
**    1 = OK.
**    2 = User has cancelled the import assistant
**    3 = FAIL (Failed to initialize COM Library)
**    4 = (REGDB_E_CLASSNOTREG): 
**        A specified class is not registered in the registration database. Also can indicate that 
**        the type of server you requested in the CLSCTX enumeration is not registered or the values 
**        for the server types in the registry are corrupt.
**    5 = (CLASS_E_NOAGGREGATION):
**        This class cannot be created as part of an aggregate. 
**    6 = (E_NOINTERFACE):
**        Fail to query interface pointer.
*/
int Ingres_ExportAssistant(HWND hwndCaller);
int Ingres_ExportAssistantWithParam(HWND hwndCaller, IEASTRUCT* pStruct);



/*
** SQL Assistant Interfaces:
** ************************************************************************************************
*/
typedef struct tagTOBJECTLIST
{
	WCHAR wczObject      [64 +1];
	WCHAR wczObjectOwner [64 +1];

	int   nObjType;
	struct tagTOBJECTLIST* pNext;
}TOBJECTLIST;
TOBJECTLIST* TOBJECTLIST_Add (TOBJECTLIST* lpList, TOBJECTLIST* lpObj);
TOBJECTLIST* TOBJECTLIST_Done(TOBJECTLIST* lpList);


typedef struct tagSQLASSISTANTSTRUCT
{
	WCHAR wczNode [64 +1];
	WCHAR wczServerClass [64 +1];
	WCHAR wczDatabase [64 +1];
	WCHAR wczUser [64 +1];
	WCHAR wczConnectionOption [512];
	WCHAR wczSessionDescription [256];

	int   nDbFlag;

	int  nSessionStart;       // In-Process svriia.dll will use the number of session from 'm_nSessionStart+1'.
	int  nActivation;         // 0=Ignore. 1=Call wizard for select statement. 2=Call Wizard for search condition.
	TOBJECTLIST* pListTables; // List tables or view. (Valid only if nActivation = 2)
	TOBJECTLIST* pListColumns;// List of columns. (Valid only if nActivation = 2)
	LPPOINT lpPoint;          // If not null, specified the top left corner (screen coordinate)

} SQLASSISTANTSTRUCT;

/*
**  Function: Ingres_SQLAssistant
**
**  Summary:  Invoke the wizard Ingres SQL Assistant dialog.
**
**  Args:
**    -hwndCaller : Handle of Window of the caller.
**    -pStruct    : Pointer to the structure SQLASSISTANTSTRUCT to passe extra information if needed.
**    -pBstrStatement: [out] the result sql statement.
**
**  Returns:  int
**    1 = OK.
**    2 = User has cancelled the SQL assistant
**    3 = FAIL (Failed to initialize COM Library)
**    4 = (REGDB_E_CLASSNOTREG): 
**        A specified class is not registered in the registration database. Also can indicate that 
**        the type of server you requested in the CLSCTX enumeration is not registered or the values 
**        for the server types in the registry are corrupt.
**    5 = (CLASS_E_NOAGGREGATION):
**        This class cannot be created as part of an aggregate. 
**    6 = (E_NOINTERFACE):
**        Fail to query interface pointer.
*/
int Ingres_SQLAssistant(HWND hwndCaller, SQLASSISTANTSTRUCT* pStruct, BSTR* pBstrStatement);



/*
** ************************************************************************************************
*/


#if defined (__cplusplus)
}
#endif
#endif // LIB_EXTERNC_CALL_HEADER
