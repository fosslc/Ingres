/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : ivmsgdml.h , Header File                                              //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : Data Manipulation of Message Setting of IVM                           //
****************************************************************************************/
/* History:
* --------
* uk$so01: (21-June-1999 created)
*
*
*/

#if !defined(IVMSGDML_HEADER)
#define IVMSGDML_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "msgntree.h"

//
// Find the message in the message Entry:
CaMessage* IVM_LookupMessage (long lCategory, long lCode);
CaMessage* IVM_LookupMessage (long lCategory, long lCode, CaMessageManager* pMsgManager);

//
// User defined folder/category:
BOOL IVM_LoadUserCategory (CTypedPtrList<CObList, CaCategoryDataUser*>& listUserFolder, LPCTSTR lpszFile);
BOOL IVM_SaveUserCategory (CTypedPtrList<CObList, CaCategoryDataUser*>& listUserFolder, LPCTSTR lpszFile);



BOOL IVM_ReadUserCategory (CaMessageManager& messageManager);



//
// Initialize the Message Entry:
BOOL IVM_InitializeUnclassifyCategory (CaMessageManager& messageManager);

//
// For each message entry, initialize its states (Alert, Notify, Discard).
// For each message entry, initialize the user's specialized messages.
BOOL IVM_SerializeUserSpecializedMessage (CaMessageManager& messageManager, LPCTSTR lpszFile, BOOL bLoad);

//
// Serialize the message entries classes {A|N|D}
// Serialize the user defined folders:
BOOL IVM_SerializeMessageSetting(CaMessageManager* pMessageMgr, CaCategoryDataUserManager* pUserFolder, BOOL bStoring);



//
// Query the Ingres messages of a given Ingres's Category:
BOOL IVM_QueryIngresMessage (CaMessageEntry* pEntry, CTypedPtrList<CObList, CaLoggedEvent*>& lsMsg);



typedef int (CALLBACK* PFNEVENTCOMPARE)(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
//
// nMin, nMax: zero base index range of elements in the array:
int  IVM_DichoInsertSort (CObArray& T, int nMin, int nMax, CObject* e, LPARAM sr, PFNEVENTCOMPARE compare);
void IVM_DichotomySort(CObArray& T, PFNEVENTCOMPARE compare, LPARAM lParamSort, CProgressCtrl* pCtrl);
int  IVM_GetDichoRange(CObArray& T, int nMin, int nMax, CObject* e, LPARAM sr, PFNEVENTCOMPARE compare);



#endif // IVMSGDML_HEADER
