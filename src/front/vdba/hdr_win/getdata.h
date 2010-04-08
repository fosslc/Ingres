/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : getdata.h
//
//    
//
********************************************************************/

#ifndef    GETDATA_HEADER
#define    GETDATA_HEADER

#include <windows.h>
#include "dom.h"



int  GetMDINodeHandle (HWND hwndMDI);
BOOL GetMDISystemFlag (HWND hwndMDI);
int  GetCurMdiNodeHandle ();
BOOL GetSystemFlag ();
LPDOMDATA GetCurLpDomData ();
//void GetUserName (const char* VirtNodeName, char* UserNameBuffer);

/*
//_____________________________________________________________________________
//                                                                             &
//                                                                             &
// FUNCTION: DBA_GetObjectType returns grantee type                                                &
//                                                                             &
// Return object type as :                                                     &
//                                                                             &
// OT_USER, OT_GROUP, OT_ROLE, OT_UNKNOWN                                      &
//                                                                             &
//_____________________________________________________________________________&
*/
int DBA_GetObjectType (LPUCHAR objectname, LPDOMDATA lpDomData);


#endif

