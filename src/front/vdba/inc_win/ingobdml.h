/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ingobdml.h: interface for the CaLLAddAlterDrop class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Access low lewel data (Ingres Database)
**
** History:
**
** 23-Dec-1999 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 30-Jan-2002 (uk$so01)
**    SIR #106648, Enhance the library.
** 04-Apr-2002 (uk$so01)
**    BUG #107506, Handle the reserved keyword
** 10-Apr-2002 (uk$so01)
**    BUG #107506, Add the function INGRESII_llQuote to automatically quote string.
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 10-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 13-Oct-2003 (schph01)
**    SIR 109864 Add prototype for INGRESII_MuteRefresh and INGRESII_UnMuteRefresh
**    functions
** 19-Dec-2003 (uk$so01)
**    SIR #111475, Coorperative shutdown between the DBMS Client & Server.
** 21-Oct-2004 (uk$so01)
**    BUG #113280 / ISSUE 13742473 (VDDA should minimize the number of DBMS connections)
**/

#if !defined(INGOBJDML_HEADER)
#define INGOBJDML_HEADER
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "dmlbase.h"
#include "qryinfo.h"

class CaFileSqlError;
class CaSession;
//
//  Function: INGRESII_llExecuteImmediate
//
//  Summary:  Execute the SQL statement.
//  Args:     pExecParam: contains the Node, Database, ..., statement to be executed.
//            pSessionManager: If this paramerter is NULL then the caller must manage
//                             the session itself (connect, disconnect, rollback or commit)
//                             If this parameter is not NULL, the session is committed after
//                             executing the statement or rollback if an error occured.
//  Returns:  BOOL
//            TRUE if success; FALSE if not.
//  The function might raise the exception class "CeSqlException"
//  ***********************************************************************************************
BOOL INGRESII_llExecuteImmediate (CaLLAddAlterDrop* pExecParam, CaSessionManager* pSessionManager);

BOOL INGRESII_llQueryObject (CaLLQueryInfo* pQueryInfo, CTypedPtrList< CObList, CaDBObject* >& listObj, void* pOpaque=NULL, CaSession* pCurrentSession=NULL);
BOOL INGRESII_llQueryDetailObject (CaLLQueryInfo* pQueryInfo, CaDBObject* pObject, CaSessionManager* pSessionManager);
//
// The INGRESII_llQueryObject2 and INGRESII_llQueryDetailObject2 must be called within an opened session !
// This function allows user to specify a specific session:
BOOL INGRESII_llQueryObject2 (CaLLQueryInfo* pQueryInfo, CTypedPtrList< CObList, CaDBObject* >& listObj, CaSession* pSession);
BOOL INGRESII_llQueryDetailObject2 (CaLLQueryInfo* pQueryInfo, CaDBObject* pObject, CaSession* pSession);
//
// If session manager 'pSessionManager' is NULL, 
// Then you must create your session first.
BOOL INGRESII_llSelectCount (int& nObjectCount, CaLLQueryInfo* pQueryInfo, CaSessionManager* pSessionManager= NULL);
//
// Table name and owner are required in pQueryInfo.
BOOL INGRESII_llIsTableEmpty(CaLLQueryInfo* pQueryInfo, CaSessionManager* pSessionManager);
//
// This function should be called within an opened session:
BOOL INGRESII_llQueryUserInCurrentSession (CString &username);

//
// Return:
// 0: Length is not required
// 1: Length is required
// 2: Precision is required
// 3: Precision and scale are required
int INGRESII_llNeedLength(int iIngresDataType);

//
// Convert the ingres column type name to the equivalent ingres numeric value:
// Ex: lpszColType = "char". return 20.
int INGRESII_llIngresColumnType2i(LPCTSTR lpszColType, int nLen = -1);
//
// Convert the ingres column numeric value type to the equivalent ingres name value:
// Ex: iColType = +/-20: return "char".
CString INGRESII_llIngresColumnType2Str(int iColType, int nLen);
//
// Return the Application defined type INGTYPE_XXX:
class CaColumn;
int INGRESII_llIngresColumnType2AppType(CaColumn* pCol);


typedef int (__cdecl * PfnIISQLHandler)(void);

BOOL INGRESII_llIsReservedIngresKeyword(LPCTSTR lpszIngresObject);
//
// The INGRESII_llIsNeedQuote is inspired from the function in VDBA called 
// LPCTSTR QuoteIfNeeded(LPCTSTR lpName). The only different is INGRESII_llIsNeedQuote
// return a BOOL instead the Quote String. This is because we don't want modify the object
// that has been passed as argument if such object is manipulated by referenced.
// EX: CString& strTable = myObject->GetName();
//     INGRESII_llIsNeedQuote(strTable) does not modify strTable.
BOOL INGRESII_llIsNeededQuote(LPCTSTR lpszIngresObject);
//
// Return the quote string if neccessary:
// The text qualifier is tchTQ.
CString INGRESII_llQuoteIfNeeded(LPCTSTR lpszIngresObject, TCHAR tchTQ = _T('\"'), BOOL bIngresCommand = FALSE);
//
// Always Return the quote string:
// The text qualifier is tchTQ.
CString INGRESII_llQuote(LPCTSTR lpszIngresObject, TCHAR tchTQ = _T('\"'), BOOL bIngresCommand = FALSE);

//
// Test system object:
BOOL INGRESII_llIsSystemObject (LPCTSTR lpszObject, LPCTSTR lpszObjectOwner = NULL, int nOTType = -1);

//
// Check is the database is belong to the gateway node.
// NOT all the members of CaLLQueryInfo are required, Only Node name and Database name are required.
// You can set extra optional information on the connection ... See the CaConnectInfo
BOOL INGRESII_llIsDatabaseGateway(CaLLQueryInfo* pQueryInfo, CaSessionManager* pSessionManager);

//
// Get the installation ID
// if bFormated = TRUE it return the string in the format:
// [<installation id>] preceeded by one space.
CString INGRESII_QueryInstallationID(BOOL bFormated=TRUE);
//
// Get the Local host name
CString GetLocalHostName();
//
// Get the DBMS info:
// lpszConstName is a constant name, ex: '_version', 'page_size_16k', ... 
CString INGRESII_llDBMSInfo(LPCTSTR lpszConstName);
//
// Check a distributed database:
int INGRESII_llGetTypeDatabase(LPCTSTR lpszDbName, CaLLQueryInfo* pQueryInfo, CaSessionManager* pSessionManager);
//
//
void INGRESII_MuteRefresh();
void INGRESII_UnMuteRefresh();
BOOL INGRESII_llGetDbevent(LPCTSTR lpszEName, LPCTSTR lpszEOwner, int nWait, CString& strEventText);
//
// Error file management
void INGRESII_llSetErrorLogFile(LPTSTR lpszErrorFileName);

#endif // !defined(INGOBJDML_HEADER)
