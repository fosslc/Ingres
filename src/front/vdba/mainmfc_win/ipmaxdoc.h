/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ipmaxdoc.h: Header File.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : interface of the CdIpm class (MFC frame/view/doc).
**
** History:
**
** 20-Fev-2002 (uk$so01)
**    Created
**    SIR #106648, Integrate ipm.ocx.
*/



#if !defined(AFX_IPMAXDOC_H__AE712EFA_E8C7_11D5_8792_00C04F1F754A__INCLUDED_)
#define AFX_IPMAXDOC_H__AE712EFA_E8C7_11D5_8792_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "axipm.h"
class CfIpm;

class CaIpmFrameFilter: public CObject
{
	DECLARE_SERIAL (CaIpmFrameFilter)
public:
	CaIpmFrameFilter();
	virtual ~CaIpmFrameFilter(){}
	virtual void Serialize (CArchive& ar);
	BOOL IsActivatedExpressRefresh(){return m_bExpreshRefresh;}
	void SetActivatedExpressRefresh(BOOL bSet){m_bExpreshRefresh = bSet;}
	CArray <short, short>& GetFilter(){return m_arrayFilter;}

protected:
	CArray <short, short> m_arrayFilter;
	BOOL m_bExpreshRefresh;
};

class CdIpm : public CDocument
{
protected: // create from serialization only
	CdIpm();
	DECLARE_DYNCREATE(CdIpm)


public:
	CString GetNode(){return m_strNode;}
	CString GetServer(){return m_strServer;}
	CString GetUser(){return m_strUser;}
	CString GetDefaultConnectedUser(){return _T("");}

	void SetNode(LPCTSTR lpszItem);
	void SetServer(LPCTSTR lpszItem);
	void SetUser(LPCTSTR lpszItem);
	void Connected(){UpdateAllViews(NULL, 1);}
	void SetConnectState(BOOL bOK);
	void SetDefaultConnectedUser(){m_strUser = GetDefaultConnectedUser();}
	BOOL IsConnected(){return m_bConnected;}
	int  GetExpressRefreshFrequency();
	void SetExpressRefreshFrequency(int nFrequency);

	HANDLE GetEventExpreshRefresh(){return m_hEventQuickRefresh;}
	CfIpm* GetFrameWindow();
	CDialogBar* GetToolBar();
	CuIpm* GetIpmCtrl();

	int GetNodeHandle(){return m_nNodeHandle;}
	void SetNodeHandle(int nhNode){m_nNodeHandle = nhNode;}
	void SetSeqNum(int val){m_SeqNum = val;}
	void SerializeIpmControl(CArchive& ar);
	BOOL IsLoadedDoc(){return m_bLoaded;}
	void SetIngresVersion(int iDBVers){m_IngresVersion = iDBVers;}
	int  GetIngresVersion(){return m_IngresVersion;}
	IUnknown* GetIpmUnknown();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CdIpm)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CdIpm();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	BOOL IsToolbarVisible(){return m_bToolbarVisible;}
	CDockState& GetToolbarState(){return m_toolbarState;}
	WINDOWPLACEMENT& GetWindowPlacement() {return  m_wplj;}
	COleStreamFile& GetIpmStreamFile(){return m_ipmStream;}
	CaIpmFrameFilter& GetFilter(){return m_filter;}

protected:
	CString m_strNode;
	CString m_strServer;
	CString m_strUser;
	BOOL    m_bConnected;
	int     m_nExpressRefreshFrequency;
	HANDLE  m_hEventQuickRefresh;
	HANDLE  m_hMutexAccessnExpressRefreshFrequency;

	int  m_SeqNum;
	int  m_nNodeHandle;
	int  m_IngresVersion;
	BOOL m_bLoaded;
	BOOL m_bToolbarVisible;
	CDockState      m_toolbarState;
	WINDOWPLACEMENT m_wplj;
	CaIpmFrameFilter m_filter;
	COleStreamFile m_ipmStream;
	// Generated message map functions
protected:
	//{{AFX_MSG(CdIpm)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IPMAXDOC_H__AE712EFA_E8C7_11D5_8792_00C04F1F754A__INCLUDED_)
