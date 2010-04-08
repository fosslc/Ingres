/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : logfsdlg.h, Header File
**
**  Project  : OpenIngres Configuration Manager
**
**  Author   : Blatte Emmanuel
**
**  Purpose  : Modeless Dialog, Page (Secondary Log) of Transaction Log
**		See the CONCEPT.DOC for more detail
**
**  History:
**	04-feb-2000 (somsa01)
**	    Due to problems building on HP, changed second definition of
**	    OnDestroy() to be called OnDestroyLog().
**	06-Jun-2000: (uk$so01) 
**	    (BUG #99242)
**	    Cleanup code for DBCS compliant
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings / errors.
** 03-Jun-2003 (uk$so01)
**    SIR #110344, Show the Raw Log information on the Unix Operating System.
*/

#if !defined(LOGFILESECDLG_HEADER)
#define LOGFILESECDLG_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CuDlgLogfileSecondary dialog

class CuLOGFILEItemData;
class CuDlgLogfileSecondary : public CFormView
{
protected:
	CuDlgLogfileSecondary();   // standard constructor
	DECLARE_DYNCREATE(CuDlgLogfileSecondary)
private:
  CuLOGFILEItemData* m_pLogFileData;
  void UpdateButtonsStates(BOOL bUpdateLeftPane = TRUE);
  void UpdateControlsStates();
public:

	// Dialog Data
	//{{AFX_DATA(CuDlgLogfileSecondary)
	enum { IDD = IDD_LOGFILE_PAGE_SECONDARY };
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
	//{{AFX_VIRTUAL(CuDlgLogfileSecondary)
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
	//{{AFX_MSG(CuDlgLogfileSecondary)
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

#endif // !LOGFILESECDLG_HEADER
