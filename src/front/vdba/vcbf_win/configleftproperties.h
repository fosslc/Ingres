#if !defined(AFX_CONFIGLEFTPROPERTIES_H__EC0976D1_36D9_11D2_9FDB_006008924264__INCLUDED_)
#define AFX_CONFIGLEFTPROPERTIES_H__EC0976D1_36D9_11D2_9FDB_006008924264__INCLUDED_

#include "cbfitem.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ConfigLeftProperties.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConfigLeftProperties dialog

class CConfigLeftProperties : public CDialog
{
// Construction
public:
	CConfigLeftProperties(CWnd* pParent = NULL);   // standard constructor
    CuCbfListViewItem* m_ItemData;
    BOOL m_bFocusCount;

    // Dialog Data
	//{{AFX_DATA(CConfigLeftProperties)
	enum { IDD = IDD_CONFIG_LEFT_PROPERTIES };
	CString	m_Component;
	CString	m_Name;
	CString	m_StartupCount;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigLeftProperties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConfigLeftProperties)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIGLEFTPROPERTIES_H__EC0976D1_36D9_11D2_9FDB_006008924264__INCLUDED_)
