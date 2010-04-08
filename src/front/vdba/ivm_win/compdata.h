/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : compdata.h , Header File 
**    Project  : Ingres II / Visual Manager 
**    Author   : Sotheavut UK (uk$so01) 
**    Purpose  : Manipulation Ingres Visual Manager Data 
**
** History:
**
** 15-Dec-1998 (uk$so01)
**    Created
** 03-Mar-2000 (uk$so01)
**    SIR #100706. Provide the different context help id.
** 12-Fev-2001 (noifr01)
**    bug 102974 (manage multiline errlog.log messages)
** 20-Jun-2001 (noifr01)
**    (sir 105046) restored the ingres server classname in the
**    CaParseInstanceInfo class
** 14-Aug-2001 (uk$so01)
**    SIR #105383., Remove meber CaTreeComponentItemData::MakeActiveTab() and
**    CaTreeComponentInstallationItemData::MakeActiveTab()
** 01-nov-2001 (somsa01)
**    Cleaned up 64-bit compiler warnings.
** 07-Jun-2002 (uk$so01)
**    SIR #107951, Display date format according to the LOCALE.
** 02-Oct-2002 (noifr01)
**    (SIR 107587) have IVM manage DB2UDB gateway (mapping the GUI to
**    oracle gateway code until this is moved to generic management).
** 19-Dec-2002 (schph01)
**    bug 109330 Add definition of ExistInstance() method in 
**    CaTreeComponentItemData class .
** 07-May-2003 (uk$so01)
**    BUG #110195, issue #11179264. Ivm show the alerted messagebox to indicate that
**    there are alterted messages but the are not displayed in the right pane
**    according to the preferences
**/

#if !defined(AFX_COMPDATA_H__63A2E05A_936D_11D2_A2B5_00609725DDAF__INCLUDED_)
#define AFX_COMPDATA_H__63A2E05A_936D_11D2_A2B5_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "compbase.h"
#include "msgcateg.h"
#include "environm.h"

class CaLoggedEvent;
class CaIvmTree;
class CaPageInformation;
class CaTreeComponentInstanceItemData;



#define FILER_DATE_BEGIN 0x0001
#define FILER_DATE_END   0x0002
#define FILER_CATEGORY   0x0004
#define FILER_NAME       0x0008
#define FILER_INSTANCE   0x0010
#define FILER_ALL        (FILER_DATE_BEGIN|FILER_DATE_END|FILER_CATEGORY|FILER_NAME|FILER_INSTANCE)


class CaEventFilter: public CObject
{
public:
	CaEventFilter():m_nFilterFlag(FILER_ALL), m_strCategory(""), m_strName(""), m_strInstance(""){};
	~CaEventFilter(){};

	UINT GetFilterFlag(){return m_nFilterFlag;}
	void SetFilterFlag(UINT nFlag){m_nFilterFlag = nFlag;}

	CString m_strCategory;
	CString m_strName;
	CString m_strInstance;

protected:
	UINT m_nFilterFlag;
};



class CaTreeComponentItemData : public CObject
{
public:
	CaTreeComponentItemData();
	virtual ~CaTreeComponentItemData();
	virtual CaPageInformation* GetPageInformation (){return m_pPageInfo;};
	//
	// Add itself (this) as Folder of 'pTreeData':
	virtual BOOL AddFolder(CaIvmTree* pTreeData, int& nChange);
	virtual BOOL QueryInstance();
	virtual BOOL AddComponent(CaTreeComponentItemData* pComponent, int& nChange)
	{
		//
		// Must not called base class ::AddAddComponent()
		ASSERT (FALSE);
		return TRUE;
	}
	virtual void SetStartStopArg(LPCTSTR lpstartstoparg) { ASSERT(FALSE);}
	virtual BOOL AddInstance (CTypedPtrList<CObList, CaTreeComponentInstanceItemData*>* pListInstance);
	virtual CaTreeComponentInstanceItemData* FindInstance (CaTreeComponentInstanceItemData* pInstance);
	virtual void ResetState (CaTreeCtrlData::ItemState state, BOOL bInstance);
	virtual void UpdateTreeData();
	virtual void UpdateState();
	virtual int GetInstaneImageRead(){ASSERT (FALSE); return -1;}
	virtual int GetInstaneImageNotRead(){ASSERT (FALSE); return -1;}
	virtual BOOL AlertNew (CTreeCtrl* pTree, CaLoggedEvent* e);
	virtual BOOL AlertRead(CTreeCtrl* pTree, CaLoggedEvent* e);

	virtual void UpdateIcon(CTreeCtrl* pTree){ASSERT (FALSE);};
	//
	// nTreeChanged is set to a specific value, if Add new item or delete old item
	// nMode = 0: Display m_strComponentType;
	// nMode = 1: Display m_strComponentName;
	virtual void Display (int& nTreeChanged, CTreeCtrl* pTree, HTREEITEM hParent, int nMode = 0);

	virtual void SetExpandImages (CTreeCtrl* pTree, BOOL bExpand){};

	CTypedPtrList<CObList, CaTreeComponentInstanceItemData*>* GetInstances (){return m_pListInstance;}
	virtual CTypedPtrList<CObList, CaTreeComponentItemData*>* GetListName(){return NULL;}

	virtual void ExpandAll(CTreeCtrl* pTree);
	virtual void CollapseAll(CTreeCtrl* pTree);
	//
	// Try to the Category and Component name whom this instance is belong to:
	virtual CaTreeComponentItemData* MatchInstance (LPCTSTR lpszInstance);

	virtual BOOL Start(){return TRUE;}
	virtual BOOL Stop (){return TRUE;}
	virtual BOOL IsStartEnabled(){return FALSE;}
	virtual BOOL IsStopEnabled (){return FALSE;}

	virtual BOOL IsMultipleInstancesRunning();
	virtual CString GetStartStopString(BOOL bStart = TRUE){return bStart? m_strStart: m_strStop;}
	virtual BOOL IsInstance(){return m_bIsInstance;}

	int  GetOrder(){return m_nOrder;}
	void SetOrder(int nOrder){m_nOrder = nOrder;}
	BOOL IsFolder(){return m_bFolder;}
	void SetDisplayInstance (BOOL bDisplayInstance){m_bDisplayInstance = bDisplayInstance;}
	int  GetItemStatus ();
	void SetItemStatus (int nStatus){m_nItemStatus = nStatus;}

	CaTreeCtrlData& GetNodeInformation() {return m_treeCtrlData;}
	int  GetStartupCount(){return m_nStartupCount;}
	void SetStartupCount(int nCount){m_nStartupCount = nCount;}

	BOOL IsQueryInstance(){return m_bQueryInstance;}
	void SetQueryInstance(BOOL bQuery){m_bQueryInstance = bQuery;}

	int  m_nStartupCount;
	int  m_nComponentType;
	BOOL m_bAllowDuplicate;
	CString m_strComponentTypes;
	CString m_strComponentType;
	CString m_strComponentName;
	CString m_strComponentInstance;

	CaTreeCtrlData m_treeCtrlData;

protected:
	BOOL ExistInstance();
	BOOL m_bFolder; // TRUE if Used as Folder
	int  m_nOrder;  // Order of Item in the tree
	//
	// TRUE if Instances should be displayed in the sub-branch:
	// The default is TRUE. The derived class can specialize this if required:
	BOOL m_bDisplayInstance;

	BOOL m_bQueryInstance;
	int  m_nItemStatus;

	CaPageInformation*  m_pPageInfo;
	CTypedPtrList<CObList, CaTreeComponentInstanceItemData*>* m_pListInstance;

	CString m_strStart;
	CString m_strStop;

	//
	// For Help management:
	BOOL m_bIsInstance;
};

class CaTreeComponentInstanceItemData: public CaTreeComponentItemData
{
public:
	CaTreeComponentInstanceItemData(): CaTreeComponentItemData()
	{
		m_treeCtrlData.SetImage (-1, -1);
		m_bIsInstance = TRUE;
	}
	virtual ~CaTreeComponentInstanceItemData(){};
	virtual CaPageInformation* GetPageInformation ();
	virtual BOOL QueryInstance(){return TRUE;}
	virtual BOOL IsStopEnabled (){return TRUE;}
	virtual CString GetStartStopString(BOOL bStart = TRUE);

	// nMode = 0: Display m_strComponentType;
	// nMode = 1: Display m_strComponentName;
	virtual void Display (int& nTreeChanged, CTreeCtrl* pTree, HTREEITEM hParent, int nMode = 0);
};


//
// Duplicated Component Folder
// ---------------------------
class CaTreeComponentDuplicatableFolder : public CaTreeComponentItemData
{
public:
	CaTreeComponentDuplicatableFolder();
	CaTreeComponentDuplicatableFolder(LPCTSTR lpszFolderName);
	
	virtual ~CaTreeComponentDuplicatableFolder();
	virtual CaPageInformation* GetPageInformation();
	virtual CaTreeComponentInstanceItemData* FindInstance (CaTreeComponentInstanceItemData* pInstance)
	{
		//
		// Folder does not have instances !!
		ASSERT (FALSE);
		return FALSE;
	}

	virtual BOOL AddComponent(CaTreeComponentItemData* pComponent, int& nChange);
	// nMode = 0: Display m_strComponentType;
	// nMode = 1: Display m_strComponentName;
	virtual void Display (int& nTreeChanged, CTreeCtrl* pTree, HTREEITEM hParent, int nMode = 0);

	virtual void UpdateTreeData();
	virtual void ResetState (CaTreeCtrlData::ItemState state, BOOL bInstance);
	virtual BOOL AlertNew (CTreeCtrl* pTree, CaLoggedEvent* e);
	virtual BOOL AlertRead(CTreeCtrl* pTree, CaLoggedEvent* e);

	virtual void UpdateIcon(CTreeCtrl* pTree);

	virtual void SetExpandImages (CTreeCtrl* pTree, BOOL bExpand);
	
	virtual CTypedPtrList<CObList, CaTreeComponentItemData*>* GetListName(){return &m_listComponent;}
	virtual void ExpandAll(CTreeCtrl* pTree);
	virtual void CollapseAll(CTreeCtrl* pTree);
	virtual CaTreeComponentItemData* MatchInstance (LPCTSTR lpszInstance);
	virtual BOOL IsMultipleInstancesRunning();

	CTypedPtrList<CObList, CaTreeComponentItemData*> m_listComponent;

};

class CaTreeComponentFolderItemData : public CaTreeComponentItemData
{
public:
	CaTreeComponentFolderItemData():CaTreeComponentItemData(){};
	virtual ~CaTreeComponentFolderItemData(){};
	virtual BOOL AddFolder(CaIvmTree* pTreeData, int& nChange);
	// nMode = 0: Display m_strComponentType;
	// nMode = 1: Display m_strComponentName;
	virtual void Display (int& nTreeChanged, CTreeCtrl* pTree, HTREEITEM hParent, int nMode = 0);
	virtual BOOL AlertNew (CTreeCtrl* pTree, CaLoggedEvent* e);
	virtual BOOL AlertRead(CTreeCtrl* pTree, CaLoggedEvent* e);



};


class CaTreeComponentGatewayItemData : public CaTreeComponentItemData
{
public:
	CaTreeComponentGatewayItemData():CaTreeComponentItemData()
	{
		m_nOrder = ORDER_GATEWAY;
	}
	virtual ~CaTreeComponentGatewayItemData(){};
	virtual int GetInstaneImageRead(){return IM_GATEWAY_R_INSTANCE;}
	virtual int GetInstaneImageNotRead(){return IM_GATEWAY_N_INSTANCE;}
	// nMode = 0: Display m_strComponentType;
	// nMode = 1: Display m_strComponentName;
	virtual void Display (int& nTreeChanged, CTreeCtrl* pTree, HTREEITEM hParent, int nMode = 0);
	virtual BOOL AddFolder(CaIvmTree* pTreeData, int& nChange);
	virtual void UpdateIcon(CTreeCtrl* pTree);

};


class CaTreeComponentInstallationItemData : public CaTreeComponentItemData
{
public:
	CaTreeComponentInstallationItemData();
	virtual ~CaTreeComponentInstallationItemData();
	virtual BOOL Start();
	virtual BOOL Stop ();
	virtual BOOL IsStartEnabled();
	virtual BOOL IsStopEnabled ();
	virtual void UpdateIcon(CTreeCtrl* pTree);

	CaPageInformation* GetPageInformation ();

	CString m_strFolderName;
private:
	void SortFolders (CTreeCtrl* pTree, HTREEITEM hRootInstallation);

};




class CaIvmTree: public CObject
{
	friend class CaTreeComponentItemData;
	friend class CaTreeComponentFolderItemData;
	friend class CaTreeComponentGatewayItemData;
public:
	CaIvmTree():m_hRootInstallation(NULL), m_bLoadString(FALSE){};
	virtual ~CaIvmTree();

	void CleanUp();
	CaTreeComponentItemData* FindFolder(LPCTSTR lpszFolderName);
	BOOL Initialize(CTypedPtrList<CObList, CaTreeComponentItemData*>* pListComponent, BOOL bInstance, int& nChange);

	void Update (CTypedPtrList<CObList, CaTreeComponentItemData*>* pLc, CTreeCtrl* pTree, BOOL bInstance, int& nChange);
	void Display(CTreeCtrl* pTree, int& nChange);
	void UpdateTreeData();
	void ResetState (CaTreeCtrlData::ItemState state, BOOL bInstance);
	void SortFolders (CTreeCtrl* pTree, HTREEITEM hRootInstallation);

	//
	// Mark the new event as an important event by changing it image:
	void AlertNew  (CTreeCtrl* pTree, CaLoggedEvent* e);
	//
	// Mark the an important event as read by changing it image (if posible):
	void AlertRead (CTreeCtrl* pTree, CaLoggedEvent* e);

	int AlertGetCount(){return m_installationBranch.m_treeCtrlData.AlertGetCount();}
	//
	// Update the state: started, stopped, ..., images:
	void UpdateIcon(CTreeCtrl* pTree);

	CTypedPtrList<CObList, CaTreeComponentItemData*>& GetListFolder() {return m_listFolder;}

	void ExpandAll(CTreeCtrl* pTree);
	void CollapseAll(CTreeCtrl* pTree);

	CaTreeComponentItemData* MatchInstance (LPCTSTR lpszInstance);
	BOOL IsMultipleInstancesRunning();
	int GetItemStatus(){return m_installationBranch.GetItemStatus();}

	UINT GetComponentState(){return m_installationBranch.GetNodeInformation().GetComponentState();}
	CString GetInstallationDisplayName(){return m_installationBranch.m_strFolderName;}
private:
	BOOL m_bLoadString;
	HTREEITEM m_hRootInstallation;
	CaTreeComponentInstallationItemData m_installationBranch;
	//
	// List of Folders:
	// A folder is either a Duplicatable or a Single component:
	// ie: DBMS Server (duplicatable)
	//   : Name Server (single)

	CTypedPtrList<CObList, CaTreeComponentItemData*> m_listFolder;

	void UpdateState();
};



//
// Logged events:
// **************
class CaLoggedEvent: public CObject
{
	DECLARE_SERIAL (CaLoggedEvent)
public:
	enum EventClass 
	{
		EVENT_NEW = 0, // event is newly queried from the file
		EVENT_EXIST    // event is put in the list control of Logged Event Page
	};
	CaLoggedEvent();
	CaLoggedEvent(LPCTSTR lpszDate, LPCTSTR lpszTime, LPCTSTR lpszComponent, LPCTSTR lpszCategoty, LPCTSTR lpszText);
	virtual ~CaLoggedEvent(){}
	UINT GetSize();
	BOOL MatchFilter (CaEventFilter* pFilter = NULL);

	void SetEventClass (EventClass evClass){m_eventClass = evClass;};
	EventClass GetEventClass(){return m_eventClass;};
	CaLoggedEvent* Clone();
	CString GetIvmFormatDate();
	CString GetDateLocale();

	CString m_strDate;
	CString m_strTime;
	CString m_strComponent; // dummy for now : may be removed in the future
	CString m_strCategory;  // dummy for now : may be removed in the future
	CString m_strText;

	//
	// Filter: use to match the item in the Tree Control
	CString m_strComponentType; 
	CString m_strComponentName;
	CString m_strInstance;

	BOOL IsAlert(){return m_bAlert;}
	void SetAlert(BOOL bAlert = TRUE)
	{
		if (bAlert) 
			m_nClass = IMSG_ALERT;
		m_bAlert = bAlert;
	}

	BOOL IsRead(){return m_bRead;}
	void SetRead (BOOL bRead){m_bRead = bRead;}
	BOOL IsAltertNotify(){return m_bAltertNotify;}
	void AltertNotify (BOOL bAlertNotify){m_bAltertNotify = bAlertNotify;}

	void SetIdentifier(long lId){m_nIdentifier = lId;};
	long GetIdentifier(){return m_nIdentifier;};

	void SetCatType(long ltype) {m_lCatType = ltype; };
	long GetCatType() { return m_lCatType; };

	void SetCode(long lCode) {m_lCode = lCode;};
	long GetCode() {return m_lCode; };

	Imsgclass GetClass(){return m_nClass;}
	void SetClass (Imsgclass nClass)
	{
		m_bAlert = (nClass == IMSG_ALERT);
		m_nClass = nClass;
	}
	BOOL IsClassified(){return m_bClassified;}
	void SetClassify (BOOL bClassified){m_bClassified = bClassified;}

	BOOL IsNotFirstLine(){return m_bNotFirstLine;}
	void SetNotFirstLine (BOOL bNotFirstLine){m_bNotFirstLine = bNotFirstLine;}

	BOOL IsNameServerStartedup(){return (m_lCode == 0xC0151);}
	//
	// Display only: use for counting when grouping the message ID:
	int  GetCount(){return m_nCount;};
	void SetCount(int nCount){m_nCount = nCount;}
	int  GetExtraCount(){return m_nExtraCount;};
	void SetExtraCount(int nCount){m_nExtraCount = nCount;}

private:
	EventClass m_eventClass;
	BOOL m_bAlert;
	BOOL m_bRead;
	BOOL m_bAltertNotify; // TRUE if the alerted event has been notified.
	long m_nIdentifier;
	int  m_lCatType;
	long m_lCode;
	Imsgclass m_nClass;
	BOOL m_bClassified;
	BOOL m_bNotFirstLine;
	//
	// Display only: use for counting when grouping the message ID:
	int m_nCount;
	int m_nExtraCount;

	CTime GetCTime();
};



class CaIvmEvent: public CObject
{
	DECLARE_SERIAL (CaIvmEvent)
public:
	CaIvmEvent(){};
	virtual ~CaIvmEvent();
	void CleanUp();

	//
	// Note: Only the pointers of object 'CaLoggedEvent' are copied out
	// into the list, do not use the 'delete' operator to destroy the objects !
	BOOL Get (CTypedPtrList<CObList, CaLoggedEvent*>& listEvent, CaEventFilter* pFilter = NULL, BOOL bNew = FALSE);
	CTypedPtrList<CObList, CaLoggedEvent*>& Get(){return m_listEvent;}
	BOOL GetAll (CTypedPtrList<CObList, CaLoggedEvent*>& listEvent);

	//
	// Return the total events (CaLoggedEvent):
	INT_PTR GetEventCount(){return m_listEvent.GetCount();}
	//
	// Return the total size (in bytes) of events (CaLoggedEvent):
	UINT GetEventSize();

protected:
	CTypedPtrList<CObList, CaLoggedEvent*> m_listEvent;

};


class CaIvmEventStartStop: public CaIvmEvent
{
public:
	CaIvmEventStartStop();
	virtual ~CaIvmEventStartStop();
	void CleanUp();

	void Add (CaLoggedEvent* pEvent);

};



class CaIvmDataMutex
{
public:
	CaIvmDataMutex(){};
	~CaIvmDataMutex(){};
	BOOL IsStatusOK(){return m_bStatus;}
protected:
	BOOL m_bStatus;
};

class CaTreeDataMutex: public CaIvmDataMutex
{
public:
	CaTreeDataMutex();
	~CaTreeDataMutex();
	BOOL IsStatusOK(){return m_bStatus;}
protected:
	BOOL m_bStatus;
};

class CaEventDataMutex: public CaIvmDataMutex
{
public:
	CaEventDataMutex(long lmsWait = 0);
	~CaEventDataMutex();
};

class CaMutexInstance:  public CaIvmDataMutex
{
public:
	CaMutexInstance();
	~CaMutexInstance();
};

class CaParseInstanceInfo: public CObject
{
public:
	CaParseInstanceInfo(LPCTSTR pstr1, LPCTSTR pstr2, LPCTSTR pserverclass, int isvrtype)
		{m_csInstance=pstr1;
	     m_csConfigName=pstr2;
		 m_csServerClass=pserverclass;
		 m_iservertype=isvrtype;
		 m_bMsgInstNoConfigNameShown = FALSE;}
	~CaParseInstanceInfo() {};
	CString m_csInstance;
	CString m_csConfigName;
	CString m_csServerClass;
	int		m_iservertype;
	BOOL	m_bMsgInstNoConfigNameShown;
};


// special ingres server class names, and configuration names

class CaParseSpecialInstancesInfo: public CObject
{
public:
	CaParseSpecialInstancesInfo(){}
	~CaParseSpecialInstancesInfo(){CleanUp();}
	void CleanUp();

	CTypedPtrList<CObList, CaParseInstanceInfo*>& GetSpecialInstancesList(){return m_listSpecialInstances;}

protected:
	CTypedPtrList<CObList, CaParseInstanceInfo *> m_listSpecialInstances;

};


#endif // !defined(AFX_COMPDATA_H__63A2E05A_936D_11D2_A2B5_00609725DDAF__INCLUDED_)
