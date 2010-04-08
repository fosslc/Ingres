/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlcopy.h : header file
**    Project  : Data Loader 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : SQL Session & SQL Copy
**
** History:
**
** 02-Jan-2004 (uk$so01)
**    Created.
** 11-Jun-2004 (uk$so01)
**    SIR #111688, additional change to support multiple INFILEs
** 29-jan-2008 (huazh01)
**    replace all declarations of 'long' type variable into 'i4'.
**    Bug 119835.
*/

#if !defined (SQLCOPYxSESSION_HEADER)
#define SQLCOPYxSESSION_HEADER
#include "dtstruct.h"

#if !defined (UNIX)
typedef int (__cdecl * PfnIISQLHandler)(void);
#else
typedef int (* PfnIISQLHandler)(void);
#endif



int INGRESII_llSetCopyHandler(PfnIISQLHandler handler);
int INGRESII_llActivateSession(int nSessionNumer);
int INGRESII_llConnectSession(char* strFullVnode);
int INGRESII_llDisconnectSession(int nSession, int nCommit);
int INGRESII_llExecuteImmediate (char* strCopyStatement);
int INGRESII_llSequenceNext(char* strSequenceName, char* szNextValue);
int INGRESII_llExecuteCopyFromProgram (char* strCopyStatement, i4* lAffectRows);

char*INGRESII_llGetLastSQLError ();
void INGRESII_llCleanSql();
void INGRESII_llCommitSequence();
i4 INGRESII_llGetCPUSession();


char* GenerateCopySyntax(DTSTRUCT* pDataStructure, int num);
/*
** 19-Jan-2004: If the II_DATE_FORMAT is set, it should be the known one described 
** in sqlref.pdf, otherwise issue the error message:
*/
int CheckForKnownDateFormat();
/*
** Up to Francois, the input date format is always 'mm/dd/yyyy hh24:mi:ss'
** I conclude that this input format will be accepted by almost ingres II_DATE_FORMAT setting
** except that YMD, DMY, MDY.
** Thus, this function checks if YMD, DMY or MDY is being used as II_DATE_FORMAT, if so
** the conversion is necessary.
** OUT: szCurrentIIdateFormat is the II_DATE_FORMAT value
** RETURN 0 if (szCurrentIIdateFormat is one of YMD, DMY or MDY)
*/
int IsInputDateCompatible(char* szCurrentIIdateFormat);
/*
** Convert date in szDateInput to szDateOutput according to the 'szIIDate':
*/
void INGRESII_DateConvert(char* szIIDate, char* szDateInput, char* szDateOutput);


#endif /* SQLCOPYxSESSION_HEADER */

