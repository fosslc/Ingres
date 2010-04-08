/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qsqlfram.h, Header file    (MDI Child Frame) 
**    Project  : ingresII / vdba.exe.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : MDI Frame window for the SQL Test.
**
** History:
**
** 19-Feb-2002 (uk$so01)
**    Created
**    SIR #106648, Integrate SQL Test ActiveX Control
** 15-Mar-2002 (schph01)
**    bug 107331 add message WM_MDIACTIVATE to reinitialize the ingres version
**    at each time that the sql/test window get the focus.
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
*/

#if !defined(QSQLFRAME_HEADER_FILE)
#define QSQLFRAME_HEADER_FILE

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
class CdSqlQuery;
class CuSqlQueryControl;

class CfSqlQueryFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CfSqlQueryFrame)
public:
	enum {RUN=0, QEP, XML};
	CfSqlQueryFrame();

	void SetSqlDoc(CdSqlQuery* pDoc){m_pSqlDoc = pDoc;}
	CdSqlQuery* GetSqlDoc(){return m_pSqlDoc;}
	CuSqlQueryControl* GetSqlqueryControl();
	BOOL SetConnection(BOOL bDisplayMessage = TRUE);
	void CommitOldSession();

	CToolBar*      GetToolBar() {return &m_wndToolBar;}
	CComboBox* GetComboDatabase() {return (CComboBox*)&m_ComboDatabase;}
	//
	// This function might thorw the CMemoryExecption
	// If lpszDatabase is not Null, then the function tries to selected that Database
	BOOL RequeryDatabase (LPCTSTR lpszDatabase = NULL);
	void UpdateLoadedData(CdSqlQuery* pDoc);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfSqlQueryFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	//
	// Implementation
public:
	virtual ~CfSqlQueryFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CString     m_strNone;
	CdSqlQuery* m_pSqlDoc;
	CToolBar    m_wndToolBar;
	CComboBox   m_ComboDatabase;

	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);

	// Generated message map functions
	//{{AFX_MSG(CfSqlQueryFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnButtonQueryQep();
	afx_msg void OnButtonQueryGo();
	afx_msg void OnButtonQueryClear();
	afx_msg void OnButtonQueryOpen();
	afx_msg void OnButtonQuerySaveas();
	afx_msg void OnButtonSqlWizard();
	afx_msg void OnUpdateButtonQueryClear(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonQueryGo(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonQep(CCmdUI* pCmdUI);
	afx_msg void OnClose();
	afx_msg void OnButtonRaw();
	afx_msg void OnUpdateButtonRaw(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonPrint(CCmdUI* pCmdUI);
	afx_msg void OnButtonPrint();
	afx_msg void OnDestroy();
	afx_msg void OnButtonXml();
	afx_msg void OnButtonSetting();
	afx_msg void OnUpdateButtonXml(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonSetting(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonSpaceCalc(CCmdUI* pCmdUI);
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
	//}}AFX_MSG
	afx_msg void OnSelChangeDatabase();   // Select a database in the combo box
	afx_msg void OnDropDownDatabase();    // Expand a database combo box

	afx_msg LONG OnGetMfcDocType(UINT uu, LONG ll);
	afx_msg LONG OnGetNodeHandle(UINT uu, LONG ll);

	//
	// Toolbar management
	afx_msg LONG OnHasToolbar       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnIsToolbarVisible (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnSetToolbarState  (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnSetToolbarCaption(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};


#endif // !defined(QSQLFRAME_HEADER_FILE)
