/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : taskxprm.h, Header file 
**    Project  : Ingres II/VDBA 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Allow interupt task
**               (Parameters and execute task, ie DBAAddObject (...)
**
** History:
** xx-Aug-1998 (uk$so01)
**   Created.
** 05-Jul-2001 (uk$so01)
**    SIR #105199. Integrate & Generate XML.
**  26-Mar-2002 (noifr01)
**   (bug 107442) removed the "force refresh" dialog box in DOM windows: force
**    refresh now refreshes all data
*/

#if !defined (TASK_EXEC_PARAM_HEADER)
#define TASK_EXEC_PARAM_HEADER
#include "mainmfc.h"

extern "C" {
#include "dba.h"
#include "main.h"
#include "dom.h"
}

//
// Base Class:
class CaExecParam: public CObject
{
	DECLARE_SERIAL (CaExecParam)
public:
	//
	// Default constructor: not interuptible !!
	CaExecParam();
	void Copy(const CaExecParam& c);
	//
	// Second constructor, allow to choose interrupt of execution:
	// For the possible values of 'nType', please see the #define INTERRUPT_XXX in the file 'extccall.h' 
	CaExecParam(UINT nInteruptType);
	virtual ~CaExecParam()
	{
		//
		// DlgWait has finished running !!
		theApp.SetDlgWaitState(FALSE);
	}
	virtual int Run(HWND hWndTaskDlg = NULL){ASSERT(FALSE); return -1;};
	virtual void Serialize (CArchive& ar);
	BOOL Lock();
	BOOL Unlock();
	//
	// These two members must be called inclosed by Lock() ... Unlock()
	BOOL IsInterrupted(){return m_bInterrupted;}
	void SetInterrupted(){m_bInterrupted = TRUE;}

	BOOL IsExecuteImmediately(){return m_bExecuteImmediately;}
	UINT  GetInterruptType(){return m_nInterruptType;}
protected:
	//
	// 'm_bExecuteImmediately' Indicates to launch the Dialog immediately
	//  when the Thread begins its execution:
	BOOL  m_bExecuteImmediately;

private:
	HANDLE m_hMutex;
	BOOL m_bInterrupted;
	UINT m_nInterruptType;

	void Init();
};


//
// Base Class:
class CaExecParamAction: public CaExecParam
{
public:
	//
	// Default constructor: not interuptible !!
	CaExecParamAction():CaExecParam(){};
	//
	// Second constructor, allow to choose interrupt of execution:
	// For the possible values of 'nType', please see the #define INTERRUPT_XXX in the file 'extccall.h' 
	CaExecParamAction (UINT nInteruptType): CaExecParam(nInteruptType){}
	virtual ~CaExecParamAction(){}

	virtual int Action (WPARAM wParam){ASSERT(FALSE); return -1;};
	virtual int Run(HWND hWndTaskDlg = NULL){ASSERT(FALSE); return -1;};
};

//
// Generic Add Object:
class CaExecParamAddObject: public CaExecParam
{
public:
	CaExecParamAddObject();
	virtual ~CaExecParamAddObject(){}
	void SetExecParam (LPCTSTR lpszVNode, int iObjType, LPVOID lpParam, int nhSession);
	virtual int Run(HWND hWndTaskDlg = NULL);

	CString m_strVNode;
	LPVOID  m_lParam;
	int     m_iObjType;
	int     m_nhSession;
};

//
// Generic Alter Object:
class CaExecParamAlterObject: public CaExecParam
{
public:
	CaExecParamAlterObject();
	virtual ~CaExecParamAlterObject(){}
	void SetExecParam (LPVOID lporiginParam, LPVOID lpnewParam, int iObjType, int nhSession);
	virtual int Run(HWND hWndTaskDlg = NULL);

	LPVOID  m_loriginParam;
	LPVOID  m_lnewParam;
	int     m_iObjType;
	int     m_nhSession;
};

//
// Generic Drop Object:
class CaExecParamDropObject: public CaExecParamAddObject
{
public:
	CaExecParamDropObject();
	virtual ~CaExecParamDropObject(){}
	virtual int Run(HWND hWndTaskDlg = NULL);
};

//
// Generic Get Detail Info:
class CaExecParamGetDetailInfo: public CaExecParam
{
public:
	CaExecParamGetDetailInfo();
	virtual ~CaExecParamGetDetailInfo(){}
	virtual int Run(HWND hWndTaskDlg = NULL);
	void SetExecParam (void * lpNode, int iObjType, LPVOID lpParam, BOOL bRetainSessForLock, int* ilocsession);

	void *  m_lpNode;
	LPVOID  m_lParam;
	int     m_iObjType;
	int*    m_pLocSession;
	BOOL    m_bRetainSessForLock;
};

//
// Generic Mon Get Detail Info:
class CaExecParamMonGetDetailInfo: public CaExecParam
{
public:
	CaExecParamMonGetDetailInfo();
	virtual ~CaExecParamMonGetDetailInfo(){}
	virtual int Run(HWND hWndTaskDlg = NULL);
	void SetExecParam (int nHNode, LPVOID lpBigStruct, int nOldType, int nNewType);

	int     m_nHNode;
	LPVOID  m_lpBigStruct;
	int     m_nOldType;
	int     m_nNewType;
};


//
// Generic DomGetFirstObject:
class CaExecParamDOMGetFirstObject: public CaExecParam
{
public:
	CaExecParamDOMGetFirstObject();
	virtual ~CaExecParamDOMGetFirstObject(){}
	virtual int Run(HWND hWndTaskDlg = NULL);
	void SetExecParam (int nHNode, int nOIType, int nLevel, LPVOID lpParentString, BOOL bSystem);

	int     m_nHNode;                // Handle of Node Struct
	int     m_nOIype;                // Object Type
	int     m_nLevel;                // Level
	LPVOID  m_lpParentString;        // Parent string (LPUCHAR* parentstrings)
	BOOL    m_bSystemObject;         // System Object
	TCHAR   m_tchszFilterOwner [128];// Filter of Owner
	TCHAR   m_tchszResultObject[128];// Result Object
	TCHAR   m_tchszResultOwner[128]; // Result Owner
	TCHAR   m_tchszResultExtra[128]; // Result extra info
};

//
// Generic DomGetFirstObject:
class CaExecParamDBAGetFirstObject: public CaExecParam
{
public:
	CaExecParamDBAGetFirstObject();
	virtual ~CaExecParamDBAGetFirstObject(){}
	virtual int Run(HWND hWndTaskDlg = NULL);
	void SetExecParam (void * lpNode,int iobjtype,int nLevel, void * parents, BOOL bSystem,void * lpobjname,void * lpowner,void * lpextra);
	void * m_lpNode;
	int    m_iobjtype;
	int    m_nLevel;
	void * m_parents;
	BOOL   m_bSystem;
	void * m_lpobjname;
	void * m_lpowner;
	void * m_lpextra;
};



class CaExecParamGetNodesList: public CaExecParam
{
public:
	CaExecParamGetNodesList();
	virtual ~CaExecParamGetNodesList(){}
	virtual int Run(HWND hWndTaskDlg = NULL);
};

//
// Background refresh:
class CaExecParamBkRefresh: public CaExecParam
{
public:
	CaExecParamBkRefresh();
	virtual ~CaExecParamBkRefresh(){}
	virtual int Run(HWND hWndTaskDlg = NULL);
	void SetExecParam (time_t refreshtime);

	time_t m_refreshtime;
};

//
// Getting list of Gateways on a given node:
class CaExecParamGetGWList: public CaExecParam
{
public:
	CaExecParamGetGWList();
	virtual ~CaExecParamGetGWList(){}
	virtual int Run(HWND hWndTaskDlg = NULL);
	void SetExecParam (TCHAR  * host);

	TCHAR * m_lphost;
	TCHAR ** m_presult;
};

//
// Expand Branch
class CaExecParamExpandBranch: public CaExecParam
{
public:
	CaExecParamExpandBranch();
	virtual ~CaExecParamExpandBranch(){}
	virtual int Run(HWND hWndTaskDlg = NULL);
	void SetExecParam (HWND hwndMdi, LPDOMDATA lpDomData, DWORD dwItem);

  HWND      m_hwndMdi;
  LPDOMDATA m_lpDomData;
  DWORD     m_dwItem;
};

//
// Expand all Branches
class CaExecParamExpandAll: public CaExecParam
{
public:
	CaExecParamExpandAll();
	virtual ~CaExecParamExpandAll(){}
	virtual int Run(HWND hWndTaskDlg = NULL);
	void SetExecParam (HWND hwndMdi, LPDOMDATA lpDomData);

  HWND      m_hwndMdi;
  LPDOMDATA m_lpDomData;
};

// Drag Drop object
class CaExecParamDragDropEnd: public CaExecParam
{
public:
	CaExecParamDragDropEnd();
	virtual ~CaExecParamDragDropEnd(){}
	virtual int Run(HWND hWndTaskDlg = NULL);
	void SetExecParam (HWND hwndMdi, LPDOMDATA lpDomData, BOOL bRightPane, LPTREERECORD lpRightPaneRecord, HWND hwndDomDest, LPDOMDATA lpDestDomData);

  HWND          m_hwndMdi;
  LPDOMDATA     m_lpDomData;
  BOOL          m_bRightPane;
  LPTREERECORD  m_lpRightPaneRecord;
  HWND          m_hwndDomDest;
  LPDOMDATA     m_lpDestDomData;
};


//
// Refresh branch and it's subbranches
class CaExecParamForceRefreshCurSub: public CaExecParam
{
public:
	CaExecParamForceRefreshCurSub();
	virtual ~CaExecParamForceRefreshCurSub(){}
	virtual int Run(HWND hWndTaskDlg = NULL);
	void SetExecParam (HWND hwndMdi, LPDOMDATA lpDomData, DWORD dwItem);

  HWND      m_hwndMdi;
  LPDOMDATA m_lpDomData;
  DWORD     m_dwItem;
};

//
// Refresh all branches
class CaExecParamForceRefreshAll: public CaExecParam
{
public:
	CaExecParamForceRefreshAll();
	virtual ~CaExecParamForceRefreshAll(){}
	virtual int Run(HWND hWndTaskDlg = NULL);
	void SetExecParam (HWND hwndMdi, LPDOMDATA lpDomData);

  HWND      m_hwndMdi;
  LPDOMDATA m_lpDomData;
};


#endif // TASK_EXEC_PARAM_HEADER