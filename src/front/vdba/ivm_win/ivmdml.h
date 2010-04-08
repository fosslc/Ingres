/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ivmdml.h , Header File 
**    Project  : Ingres II / Visual Manager 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Data Manipulation Langage of IVM
**
** History:
** 21-Dec-1998 (uk$so01)
**    created
** 02-02-2000 (noifr01)
**   (bug 100315) added the IsRmcmdServer() function prototype
** 05-Apr-2001 (uk$so01)
**    Add new function INGRESII_QueryInstallationID() whose body codes are moved from
**    mainview.cpp.
** 27-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 02-Feb-2004 (noifr01)
**    removed any unneeded reference to iostream libraries, including now
**    unused internal test code 
** 02-May-2008 (drivi01)
**    Added a new function IVM_IsDBATools().
*/

#if !defined(IVMDML_HEADER)
#define IVMDML_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "compdata.h"


//
// Global functions:
// ----------------

BOOL IVM_llinit();
BOOL IVM_llterminate();

//
// Return TRUE if the Component Tree need to be updated:
BOOL IVM_llIsComponentChanged();

//return values for IVM_llGetNewEventsStatus() and IVM_llQueryLoggedEvent()
#define RET_LOGFILE_CHANGED_BY_HAND 0
#define RET_NO_NEW_EVENT            1
#define RET_NEW_EVENTS				2
#define RET_ERROR					3
#define RET_OK						4

int IVM_llGetNewEventsStatus(); // returns RET_LOGFILE_CHANGED_BY_HAND, RET_NO_NEW_EVENT, or RET_NEW_EVENTS


//  fills up:
//     pListEvent, the list of events that match the categories specified in the settings:
//     pQueryEventInfo, the information about the query of events (See detail of CaQueryEventInformation)
//  return values:
//     RET_LOGFILE_CHANGED_BY_HAND 
//     RET_ERROR
//     RET_OK

int IVM_llQueryLoggedEvent (
	CTypedPtrList<CObList, CaLoggedEvent*>* pListEvent,
	CaQueryEventInformation* pQueryEventInfo);

//
// Query The Instances of aComponent:
// Must call IVM_InitializeInstance first and then repeatly call IVM_llQueryInstance
BOOL IVM_InitializeInstance();
BOOL IVM_llQueryInstance (CaTreeComponentItemData* pComponent, CTypedPtrList<CObList, CaTreeComponentItemData*>& listInstance);

//
// Query The Parameters:
// bSystem = TRUE:  -> Query system parameter.
// bSystem = FALSE: -> Query user parameter.
BOOL IVM_llQueryParameter (CTypedPtrList<CObList, CaEnvironmentParameter*>& listParameter, BOOL bSystem = TRUE);


BOOL IVM_StartComponent (LPCTSTR lpszStartString);
BOOL IVM_StopComponent  (LPCTSTR lpszStopString);

//
// lpszInstance: instance's name to match the instances in the Tree Control
// Return NULL if not matched otherwise CaTreeComponentItemData class
// that holds the CATEGORY and COMPONENT NAME where the instance is belong to.
// NOTE: The caller must use delete operator to delete the return pointer.
// ----
CaTreeComponentItemData* IVM_MatchInstance (LPCTSTR lpszInstance);

//
// Check to see if we need to reinitialize the list of events:
BOOL IVM_IsReinitializeEventsRequired();

//
// Get Host Info:
BOOL IVM_GetHostInfo (CString& strInstall, CString& strHost, CString& strIISystem);

//
// Construct the list of Event Category:
//BOOL IVM_InitEventCategory(CTypedPtrList<CObList, CaEventCategory*>& listCategory);

//
// When reading event from errlog.log, each event must have
// a unique indentifier. This indentifier is used as a zero based
// index for the array of boolean to indicate if it is read or not.
// Return the count of events
long IVM_NewEvent (long nEventNumber, BOOL bRead = FALSE);
//
// Test to see if the given event is read:
BOOL IVM_IsReadEvent (long nEventNumber);
void IVM_SetReadEvent(long nEventNumber, BOOL bRead = TRUE);
//
// Query the list of Components,
// if bQueryInstance = TRUE, this function will query the list of instances for each component.
BOOL IVM_llQueryComponent (CTypedPtrList<CObList, CaTreeComponentItemData*>* pListComponent, BOOL bQueryInstance = TRUE);


CString IVM_GetTimeSinceNDays(UINT nDays);

//
// Run a process executeing INGSETENV lpszName lpszValue.
// If bUnset = TRUE, then the 'lpszValue' is ignored and the function
// runs the process executing INGUNSET lpszName
// If bSystem = TRUE  -> Ingsetenv or Ingunset
// If bSystem = FALSE -> ???
BOOL IVM_SetIngresVariable (LPCTSTR lpszName, LPCTSTR lpszValue, BOOL bSystem, BOOL bUnset = FALSE);


BOOL IVM_IsStartMessage    (long lmessageID);
BOOL IVM_IsStopMessage     (long lmessageID);
BOOL IVM_IsStartStopMessage(long lmessageID);
BOOL IVM_IsLoggingSystemMessage(long lmsgcat, long lmessageID);
BOOL IVM_IsLockingSystemMessage(long lmsgcat, long lmessageID);

void IVM_ll_RestartLFScanFromStart();

//
// Return the generic Message Text of a given message id:
CString IVM_IngresGenericMessage (long lCode, LPCTSTR lpszOriginMsg);



BOOL IVM_IsStarted (CTreeCtrl* pTree, int nComponentType);
//
// Return the empty string if allowed, otherwise
// the string contains the message of the reason why it is not allowed.
CString IVM_AllowedStart (CTreeCtrl* pTree, CaTreeComponentItemData* pItem);
CString IVM_AllowedStop  (CTreeCtrl* pTree, CaTreeComponentItemData* pItem);

//
// pTree is the Tree of Component on the left pane:
void IVM_ShutDownIngres(CTreeCtrl* pTree);
BOOL IVM_IsNameServerStarted();


BOOL SerializeLogFileCheck(CArchive& ar);
BOOL IsNameServer (CaPageInformation* pPageInfo);
BOOL IsRmcmdServer (CaPageInformation* pPageInfo);
void IVM_InitializeIngresVariable();
BOOL IVM_IsDBATools(CString strII);

#endif // !defined(IVMDML_HEADER)
