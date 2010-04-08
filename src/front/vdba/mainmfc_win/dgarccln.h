/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgarccln.h : header file 
**    Project  : IngresII / VDBA.
**    Author   : Emmanuel Blattes
**
** History after 15-06-2001
**
** 15-Jun-2001 (uk$so01)
**    BUG #104993, Manage the push/pop help ID.
**
**/

#if !defined(AFX_DGARCCLN_H__5AF64EC2_1584_11D2_A38C_00609725DBFA__INCLUDED_)
#define AFX_DGARCCLN_H__5AF64EC2_1584_11D2_A38C_00609725DBFA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DgArccln.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicArcclean dialog

class CuDlgReplicArcclean : public CDialog
{
// Construction
public:
	CuDlgReplicArcclean(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CuDlgReplicArcclean)
	enum { IDD = IDD_REPLICATION_ARCCLEAN };
	CEdit	m_edBeforeTime;
	CString	m_csBeforeTime;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgReplicArcclean)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgReplicArcclean)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGARCCLN_H__5AF64EC2_1584_11D2_A38C_00609725DBFA__INCLUDED_)
