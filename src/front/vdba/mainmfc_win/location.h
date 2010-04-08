#if !defined(AFX_LOCATION_H__372C77E3_5239_11D2_9720_00609725DBF9__INCLUDED_)
#define AFX_LOCATION_H__372C77E3_5239_11D2_9720_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// location.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CuDlgcreateDBLocation dialog

class CuDlgcreateDBLocation : public CDialog
{
// Construction
public:
	CuDlgcreateDBLocation(CWnd* pParent = NULL);   // standard constructor
	void OnOK();
	void OnCancel();

// Dialog Data
	//{{AFX_DATA(CuDlgcreateDBLocation)
	enum { IDD = IDD_CREATEDB_LOCATION };
	CComboBox	m_ctrlLocWork;
	CComboBox	m_ctrlLocJournal;
	CComboBox	m_ctrlLocDump;
	CComboBox	m_ctrlLocDatabase;
	CComboBox	m_ctrlLocCheckPoint;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgcreateDBLocation)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgcreateDBLocation)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOCATION_H__372C77E3_5239_11D2_9720_00609725DBF9__INCLUDED_)
