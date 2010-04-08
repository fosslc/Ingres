/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : comllses.h: header file 
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Low lewel functions
**
** History:
**
** 12-Oct-2000 (uk$so01)
**    Created
**/

#if !defined(COMINGRSESS_HEADER)
#define COMINGRSESS_HEADER
class CmtSession;

BOOL INGRESII_llConnectSession   (CmtSession* pInfo);
BOOL INGRESII_llReleaseSession   (CmtSession* pInfo, BOOL bCommit = TRUE);
BOOL INGRESII_llDisconnectSession(CmtSession* pInfo);
BOOL INGRESII_llActivateSession  (CmtSession* pInfo);


#endif // COMINGRSESS_HEADER