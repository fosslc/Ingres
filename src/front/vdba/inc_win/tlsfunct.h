/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : tlsfunct.h, header file 
**    Project  : Visual DBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : General function prototypes
**
** History:
**
** 23-Oct-2001 (uk$so01)
**    Created
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 27-May-2002 (uk$so01)
**    BUG #107880, Add the XML decoration encoding='UTF-8' or 'WINDOWS-1252'...
**    depending on the Ingres II_CHARSETxx.
** 04-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 14-Nov-2003 (uk$so01)
**    BUG #110983, If GCA Protocol Level = 50 then use MIB Objects 
**    to query the server classes.
** 19-Dec-2003 (uk$so01)
**    SIR #111475, Coorperative shutdown between the DBMS Client & Server.
**    Created.
** 06-Fev-2004 (schph01)
**    SIR 111752 Add define for new function INGRESII_IsDBEventsEnabled(),
**    for managing DBEvent on Gateway.
** 06-Fev-2004 (schph01)
**    (SIR 111752) canceled previous change, that was causing a build problem
**    in some directories. moved definition of INGRESII_IsDBEventsEnabled()
**    to monitor.h
*/


#if !defined (GENERAL_FUNCTIONS_PROTOTYPES_HEADER)
#define GENERAL_FUNCTIONS_PROTOTYPES_HEADER

TCHAR* fstrncpy(TCHAR* pu1, TCHAR* pu2, int n);
char* _fstrncpy(char* pu1, char* pu2, int n);
void RemoveEndingCRLF(LPTSTR lpszText);
void RCTOOL_CR20x0D0x0A(CString& item);
void RCTOOL_EliminateTabs (CString& item);

//
// Return
//   -the character-set "UTF-8", "IBMPC850", ... corresponding to the II_CHARSET:
//   -the empty string if II_CHARSET is not set.
//        
CString UNICODE_IngresMapCodePage();
void INGRESII_BuildVersionInfo (CString& strVersion, int& nYear);
BOOL INGRESII_IsIngresGateway(LPCTSTR lpszServerClass);

//
// Given a PID, this function attempt to the main application window
// that has the PID by using the EnumWindows and GetWindowThreadProcessId.
// Normally, it should give anly one window (the main window):
void RCTOOL_GetWindowTitle (long PID, CStringList& listCaption);
void RCTOOL_GetWindowTitle (long PID, CArray <HWND, HWND>& arrayWnd);


#endif // GENERAL_FUNCTIONS_PROTOTYPES_HEADER
