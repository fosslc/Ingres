/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : extccall.h, header file
**
**  Project  : Ingres II/ VDBA.
**
**  Author   : UK Sotheavut
**
**  Purpose  : Interface for extern "C" caller
**
**  History:
**	15-feb-2000 (somsa01)
**	    Removed extra comma in enum declaration.
** 02-May-2001 (uk$so01)
**    SIR #102678
**    Support the composite histogram of base table and secondary index.
** 30-May-2001 (uk$so01)
**    SIR #104736, Integrate IIA and Cleanup old code of populate.
** 18-Jun-2001 (uk$so01)
**    BUG #104799, Deadlock on modify structure when there exist an opened 
**    cursor on the table.
** 14-Fev-2002 (uk$so01)
**    SIR #106648, Integrate SQL Assistant In-Process COM Server
**  26-Mar-2002 (noifr01)
**   (bug 107442) removed the "force refresh" dialog box in DOM windows: force
**    refresh now refreshes all data
** 22-Sep-2002 (schph01)
**    bug 108645 Add declaration for GetLocalVnodeName() and InitLocalVnode() functions.
** 22-Sep-2004 (uk$so01)
**    BUG #113104 / ISSUE 13690527, Add the functionality
**    to allow vdba to request the IPM.OCX and SQLQUERY.OCX
**    to know if there is an SQL session through a given vnode
**    or through a given vnode and a database.
** 09-Mar-2009 (smeke01) b121764
**    Change parameters to CreateSQLWizard CreateSelectStmWizard, CreateSQLWizard, 
**    and CreateSearchCondWizard to LPUCHAR so that we don't need to use the T2A macro.
** 11-May-2010 (drivi01)
**    Add tblType to DMLCREATESTRUCT for "Create Index" dialog.
**    So that in "Create Index" dialog the available storage structures for
**    the index reflect the storage structures available for the table type.
*/

#if !defined (EXTERN_C_CALL_HEADER)
#define EXTERN_C_CALL_HEADER

#if defined (__cplusplus)
extern "C"
{
#endif
#include "dba.h"
#include "dbaset.h"
#include "main.h"
#include "dom.h"
#include "libextnc.h"
#include "domdata.h"
// mapping of functions that may interfer with VDBA "windows", and that can be called within the
// secondary thread when there is an "animation"
enum 
{
	ACTION_DOMUPDATEDISPLAYDATA,
	ACTION_UPDATENODEDISPLAY,
	ACTION_UPDATEDBEDISPLAY,
//	ACTION_DOMDISABLEDISPLAY, // not managed, since code does nothing (not even show the hourglass) when in secondary thread (test done in the base function)
//	ACTION_DOMENABLEDISPLAY,  // not managed, since code does nothing (not even show the hourglass) when in secondary thread (test done in the base function)
	ACTIONREFRESHPROPSWINDOW,
	ACTION_MESSAGEBOX,
	ACTION_GETFOCUS,
	ACTION_UPDATEMONDISPLAY,
	ACTION_UPDATERIGHTPANE,
	ACTION_SENDMESSAGE
};

/*
** Check to see if there is and ipm.ocx and/or sqlquery.ocx being loaded.
** the ocx has its own SQL session! */
BOOL ExistSqlQueryAndIpm(LPCTSTR lpVirtNode, LPCTSTR lpszDatabase); 
LONG ManageBaseListAction(WPARAM wParam, LPARAM lParam, BOOL *bActionComplete);

BOOL CanBeInSecondaryThead(void);
int Notify_MT_Action(WPARAM wParam, LPARAM lParam);
void NotifyAnimationDlgTxt(HWND hwnd, LPCTSTR buf);
//
// Run a modeless dialog which run the thread to excute the DBA[Add|Alter|Drop]Object
// The dialog allows user to cancel which cause to execute the IIbreak() and terminate the
// thread.

int Task_DbaAddObjectInteruptible   (LPCTSTR lpszVNode, int iObjType, LPVOID lpParam, int nhSession);
int Task_DbaAlterObjectInteruptible (LPVOID lpOriginParam, LPVOID lpNewParam, int iObjType, int nhSession);
int Task_DbaDropObjectInteruptible  (LPCTSTR lpszVNode, int iObjType, LPVOID lpParam, int nhSession);

//
// Call GetDetailInfo(LPUCHAR lpVirtNode,int iobjecttype, void* lpparameters, BOOL bRetainSessForLock, int* ilocsession):
int Task_GetDetailInfoInterruptible  (LPCTSTR lpszVNode, int iObjType, LPVOID lpParam, BOOL bRetainSessForLock, int* ilocsession);

//
// Call MonGetDetailInfo(int hnode, void* pbigstruct, int oldtype, int newtype);
int Task_MonGetDetailInfoInterruptible  (int nHNode, LPVOID lpBigStruct, int iObjType, int nNewType);

int Task_DOMGetFirstObjectInteruptible (
	int nHNode,
	int nOIType,
	int nLevel,
	LPUCHAR* lpParentString,
	BOOL bSystem,
	LPUCHAR lpfilterowner,
	LPUCHAR lpresultobjectname,
	LPUCHAR lpresultowner,
	LPUCHAR lpresultextrastring);

int Task_GetNodesList(void);
LPUCHAR * Task_GetGWList(LPUCHAR host);
int Task_BkRefresh (time_t refreshtime);

int Task_DBAGetFirstObjectInterruptible (
	LPUCHAR lpVirtNode,
	int     iobjecttype,
	int     level,
	LPUCHAR *parentstrings,
	BOOL    bWithSystem,
	LPUCHAR lpobjectname,
	LPUCHAR lpownername,
	LPUCHAR lpextradata);

int GetDetailInfoLL(LPUCHAR lpVirtNode,int iobjecttype, void *lpparameters,
                  BOOL bRetainSessForLock, int *ilocsession);


//
// Create Index Dialog:
typedef struct tagDMLCREATESTRUCT
{
	TCHAR tchszDatabase   [MAXOBJECTNAME];
	TCHAR tchszObject     [MAXOBJECTNAME];
	TCHAR tchszObjectOwner[MAXOBJECTNAME];

	LPCTSTR lpszStatement; // Copy pointer only (do not free it!)
	LPCTSTR lpszStatement2;// If this pointer is not null, the ESL_FreeMem must be called !

	int tblType; //Ingres VectorWise table types
} DMLCREATESTRUCT;

int VDBA_CreateIndex(HWND hwndParent, DMLCREATESTRUCT* pCr);
void INDEXPARAMS2DMLCREATESTRUCT (LPVOID pIndexParams, DMLCREATESTRUCT* pCr);

//
// Constraint Enhancement (Index Option):
// nCallFor = 0 : primary key constraint enforcement.
// nCallFor = 1 : unique key constraint enforcement.
// nCallFor = 2 : foreign key constraint enforcement.
int VDBA_IndexOption(
	HWND hwndParent, 
	LPCTSTR lpszDatabase, 
	int nCallFor, 
	INDEXOPTION*  pConstraintWithClause,
	LPTABLEPARAMS pTable);

//
// Table Primary Key:
int VDBA_TablePrimaryKey(
	HWND hwndParent, 
	LPCTSTR lpszDatabase, 
	PRIMARYKEYSTRUCT* pPrimaryKey,
	LPTABLEPARAMS pTable);

BOOL INDEXOPTION_COPY (INDEXOPTION* pDest, INDEXOPTION* pSource);

//
// Return TRUE:  if disable/enable sucessfully.
//        FALSE: if the dialog box is dismissed.
BOOL VDBA_EnableDisableSecurityLevel();


typedef struct tagDROPOBJECTSTRUCT
{
	//
	// TRUE: Drop with restrict, FALSE drop with cascade;
	BOOL bRestrict;

} DROPOBJECTSTRUCT;
//
// Return TRUE if OK is pressed:
//        FALSE if the dlg is dismissed.
BOOL VDBA_DropObjectCacadeRestrict(DROPOBJECTSTRUCT* pStruct, LPCTSTR lpszTitle);


// parameters of DOMUpdateDisplayData, passed back from secondary thread to animation dialog box
typedef struct tagDOMUPDATEDISPLAYDATA
{
	int        hnodestruct;
	int        iobjecttype;
	int        level;
	LPUCHAR *  parentstrings;
	BOOL       bWithChildren;
	int        iAction;
	DWORD      idItem;
	HWND       hwndDOMDoc;
} DOMUPDATEDISPLAYDATA;

typedef struct tagUPDATEDBEDISPLAYPARAMS
{
	int    hnodestruct;
	char * dbname;
} UPDATEDBEDISPLAYPARAMS;

typedef struct tagUPDATEREFRESHPROPSPARAMS
{
	BOOL    bOnLoad;
	time_t  refreshtime;
} UPDATEREFRESHPROPSPARAMS;

typedef struct tagMESSAGEBOXPARAMS
{
	HWND    hWnd;
	LPCTSTR pTxt;
	LPCTSTR pTitle;
	UINT    uType;
} MESSAGEBOXPARAMS;

typedef struct tagUPDMONDISPLAYPARAMS
{
	int   hnodestruct;
	int   iobjecttypeParent;
	void *pstructParent;
	int   iobjecttypeReq;
} UPDMONDISPLAYPARAMS;

typedef struct tagUPDATERIGHTPANEPARAMS
{
  HWND hwndDoc;
  LPDOMDATA lpDomData;
  BOOL bCurSelStatic;
  DWORD dwCurSel;
  int CurItemObjType;
  LPTREERECORD lpRecord;
  BOOL bHasProperties;
  BOOL bUpdate;
  int initialSel;
  BOOL bClear;
} UPDATERIGHTPANEPARAMS, FAR* LPUPDATERIGHTPANEPARAMS;

typedef struct tagSENDMESSAGEPARAMS
{
	HWND hWnd;
	UINT Msg; 
	WPARAM wParam;
	LPARAM lParam ;
} SENDMESSAGEPARAMS, FAR* LPSENDMESSAGEPARAMS;


//
// CONSTANT FOR INTERRUPT MODE:
// ---------------------------
#define INTERRUPT_NOT_ALLOWED 0x0000
#define INTERRUPT_QUERY       0x0001
#define INTERRUPT_EXIT_LOOP   0x0002
#define INTERRUPT_ALL_TYPE    (INTERRUPT_QUERY | INTERRUPT_EXIT_LOOP)

//
// This function return one of the above value:
UINT  VDBA_GetInterruptType();
void VDBA_SetInterruptType(UINT nType);
BOOL HasLoopInterruptBeenRequested();
BOOL HasSelectLoopInterruptBeenRequested();

//
// General Display Settings:
BOOL VDBA_DlgGeneralDisplaySetting();
//
// Preference Settings:
BOOL VDBA_DlgPreferences();

void IIbreak(void);


// for Expand branch
int Task_ManageExpandBranchInterruptible(HWND hwndMdi, LPDOMDATA lpDomData, DWORD dwItem, HWND hWndTaskDlg);

// for Expand all
int Task_ManageExpandAllInterruptible(HWND hwndMdi, LPDOMDATA lpDomData, HWND hWndTaskDlg);

// for Drag-Drop
int Task_StandardDragDropEndInterruptible(HWND hwndMdi, LPDOMDATA lpDomData, BOOL bRightPane, LPTREERECORD lpRightPaneRecord, HWND hwndDomDest, LPDOMDATA lpDestDomData);

// for Refresh branch and it's subbranches
int Task_ManageForceRefreshCursubInterruptible(HWND hwndMdi, LPDOMDATA lpDomData, DWORD dwItem, HWND hWndTaskDlg);

// for Refresh all branches
int Task_ManageForceRefreshAllInterruptible(HWND hwndMdi, LPDOMDATA lpDomData, HWND hWndTaskDlg);

//
// New implementation: Extract an icon from the image list.
// The zero base index of image list is calculate from the [in] parameter
// type.
HICON LoadIconInCache(int type);

//
// Update the Icon image in the header of the DOM Right pane:
void TS_DOMRightPaneUpdateBitmap(HWND hwndMdi, LPVOID lpDomData, DWORD dwCurSel, LPVOID lpRecord);

//
// Dialog box for selecting indexes:
// lpListObjects [in]  list of pre-selected indexes
// lpListObjects [out] list of selected indexes.
int VDBA_ChooseIndex(HWND hwndParent, LPCTSTR lpszDatabase, LPCHECKEDOBJECTS* lpListObjects);



typedef struct tagREQUESTMDIINFO
{
	HWND hWndMDI; // MDI Window
	HWND hWnd;    // The window (it can be hWndMDI itself) about it the nInfo is concerned.
	UINT nInfo;   // Information.
} REQUESTMDIINFO;

#define FINDCURSORLEN 64
typedef struct tagFINDCURSOR
{
	TCHAR tchszNode      [FINDCURSORLEN+1];
	TCHAR tchszServer    [FINDCURSORLEN+1];
	TCHAR tchszDatabase  [FINDCURSORLEN+1];
	TCHAR tchszTable     [FINDCURSORLEN+1];
	TCHAR tchszTableOwner[FINDCURSORLEN+1];

	BOOL  bCloseCursor;  // TRUE to indicate the function 'IsExistOpenCursor()' to close the cursor.

} FINDCURSOR;

#define OPEN_CURSOR_DOM     1
#define OPEN_CURSOR_SQLACT  (OPEN_CURSOR_DOM   << 1)
//
// Return one or more of the above definition:
// The caller must deallocate (use the C-function free) pArrayInfo if the returned iSize > 0.
UINT GetExistOpenCursor (FINDCURSOR* pFindCursor, REQUESTMDIINFO** pArrayInfo, int* iSize);
BOOL IsReadLockActivated();
//
// return the WM_QUERY_OPEN_CURSOR;
// because winmain has no access to the wmusrmsg.h in mainmfc.
UINT GetUserMessageQueryOpenCursor();

//
// SQL Assistant
LPTSTR CreateSelectStmWizard(HWND hwndOwner, LPUCHAR lpVirtNodeName, LPUCHAR lpDatabaseName);
LPTSTR CreateSQLWizard(HWND hwndOwner, LPUCHAR lpVirtNodeName, LPUCHAR lpDatabaseName);
LPTSTR CreateSearchCondWizard(HWND hwndOwner, LPUCHAR lpVirtNodeName, LPUCHAR lpDatabaseName, TOBJECTLIST* pTableList, TOBJECTLIST* pListCol);

BOOL InitLocalVnode ( void );
void GetLocalVnodeName (TCHAR *tclocnode, int iLenBuffer);


#if defined (__cplusplus)
}
#endif

#if defined (__cplusplus)
CString SqlWizard(HWND hParent, SQLASSISTANTSTRUCT* piiparam);
#endif



#endif  // EXTERN_C_CALL_HEADER

