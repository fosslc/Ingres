#if !defined(AFX_DLGREQUE_H__4C5B91D2_E035_11D1_96C3_00609725DBF9__INCLUDED_)
#define AFX_DLGREQUE_H__4C5B91D2_E035_11D1_96C3_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// dlgreque.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CuDlgRaiseEventQuestion dialog

class CuDlgRaiseEventQuestion : public CDialog
{
// Construction
public:
	CuDlgRaiseEventQuestion(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CuDlgRaiseEventQuestion)
	enum { IDD = IDD_REPLIC_QUESTION };
	int		m_NewValue;
	CString	m_strQuestion;
	//}}AFX_DATA
	int m_iMin;
	int m_iMax;
	CString	m_strCaption;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgRaiseEventQuestion)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgRaiseEventQuestion)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGREQUE_H__4C5B91D2_E035_11D1_96C3_00609725DBF9__INCLUDED_)
