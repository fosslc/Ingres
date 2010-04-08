/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : logfidlg.h, Header File
**
**  Project  : OpenIngres Configuration Manager
**
**  Author   : Blatte Emmanuel
**
**  Purpose  : Modeless Dialog, Page (Primary Log) of Transaction Log
**		See the CONCEPT.DOC for more detail
**
**  History:
**	04-feb-2000 (somsa01)
**	    Due to problems building on HP, changed second definition of
**	    OnDestroy() to be called OnDestroyLog().
**	06-Jun-2000 (uk$so01)
**	    bug 99242 Handle DBCS
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings / errors.
** 03-Jun-2003 (uk$so01)
**    SIR #110344, Show the Raw Log information on the Unix Operating System.
*/

#if !defined(LOGFILEDLG_HEADER)
#define LOGFILEDLG_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CuDlgLogfileConfigure dialog

class CuLOGFILEItemData;
class CuDlgLogfileConfigure : public CFormView
{
protected:
	CuDlgLogfileConfigure();   // standard constructor
	DECLARE_DYNCREATE(CuDlgLogfileConfigure)
private:
  CuLOGFILEItemData* m_pLogFileData;
  void UpdateButtonsStates(BOOL bUpdateLeftPane = TRUE);
  void UpdateControlsStates();
public:

	// Dialog Data
	//{{AFX_DATA(CuDlgLogfileConfigure)
	enum { IDD = IDD_LOGFILE_PAGE_CONFIGURE };
	CButton	m_cCheckRaw;
	CStatic	m_StatusBitmap;
	CButton	m_CheckEnabled;
	CEdit	m_EditPath;
	CEdit	m_EditSize;
	CStatic	m_StaticState;
	CListBox m_FileNameList;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgLogfileConfigure)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	UINT_PTR m_TimerID;
    void SetTreeCtrlItem(LPCTSTR szItemText);
    BOOL VerifyIfTransactionsPending();

	// Generated message map functions
	//{{AFX_MSG(CuDlgLogfileConfigure)
	afx_msg void OnDestroy();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnSelchangeFilenameList();
	afx_msg void OnPaint();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnReformat   (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnDisable    (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnErase      (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnDestroyLog (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnCreate     (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !LOGFILEDLG_HEADER
