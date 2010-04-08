/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : viewleft.h, Header file
**    Project  : INGRESII / Monitoring.
**    Author   : UK Sotheavut (old code from IpmView1.cpp of Emmanuel Blattes)
**    Purpose  : TreeView of the left pane
**
** History:
**
** 12-Nov-2001 (uk$so01)
**    Created
**
*/

#if !defined (IPM_VIEWLEFT_HEADER)
#define IPM_VIEWLEFT_HEADER

class CaFindInformation
{
public:
	CaFindInformation()
	{
		Clean();
	}
	~CaFindInformation(){}
	void Clean();
	
	void SetFlag(UINT nFlags){m_nFlags = nFlags;}
	UINT GetFlag(){return m_nFlags;}
	void SetWhat(LPCTSTR lpszWhat){m_strWhat = lpszWhat;}
	CString GetWhat(){return m_strWhat;}
	
	void  SetFindTargetPane(CWnd* pWnd){m_pWndPane = pWnd;}
	CWnd* GetFindTargetPane(){return m_pWndPane;}

protected:
	CString m_strWhat;// Text to search
	UINT  m_nFlags;   // Same as in FINDREPLACE structure.
	CWnd* m_pWndPane; // Current Pane of Find Target that contains the list control or edit control
};

inline void CaFindInformation::Clean()
{
	m_strWhat = _T("");
	m_nFlags   = 0;
	m_pWndPane = NULL;
}


class CdIpmDoc;
class CvIpmLeft : public CTreeView
{
protected:
	CvIpmLeft();
	DECLARE_DYNCREATE(CvIpmLeft)

	// Operations
public:
	void ProhibitActionOnTreeCtrl(BOOL bProhibit){m_bProhibitActionOnTreeCtrl = bProhibit;}
	void SearchItem();

	static UINT WM_FINDREPLACE;
	static CFindReplaceDialog* m_pDlgFind;
	CString m_strLastFindWhat;
	BOOL m_bMatchCase;
	BOOL m_bWholeWord;


private:
	BOOL m_bProhibitActionOnTreeCtrl;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvIpmLeft)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	// Implementation
protected:
	virtual ~CvIpmLeft();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CvIpmLeft)
	afx_msg void OnDestroy();
	afx_msg void OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMenuCloseServer();
	afx_msg void OnMenuOpenServer();
	afx_msg void OnMenuForceRefresh();
	afx_msg void OnMenuReplicationChangeRunNode();
	afx_msg void OnMenuShutdown();
	afx_msg void OnUpdateMenuCloseServer(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuOpenServer(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuForceRefresh(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuReplicationChangeRunNode(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuShutdown(CCmdUI* pCmdUI);
	//}}AFX_MSG

	afx_msg LONG OnFindReplace(WPARAM wParam, LPARAM lParam);
	afx_msg long OnSettingChange(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

#endif // IPM_VIEWLEFT_HEADER
