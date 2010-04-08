/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Source   : DlgLogSz.h, Header File
**
** Project  : OpenIngres Configuration Manager
** Author   : Blatte Emmanuel
**
** History:
**	08-feb-2000 (somsa01)
**	    Added return type to CreateDirectoryRecursively().
**	20-Jun-2003 (schph01)
**	    (bug 110430) Add prototype for function IsExistDirectory().
*/

#if !defined(AFX_DLGLOGSZ_H__55562D61_38D5_11D1_A303_00609725DBFA__INCLUDED_)
#define AFX_DLGLOGSZ_H__55562D61_38D5_11D1_A303_00609725DBFA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgLogSz.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CuDlgLogSize dialog

class CuDlgLogSize : public CDialog
{
// Construction
public:
	CuDlgLogSize(CWnd* pParent = NULL);   // standard constructor

    bool m_bPrimary;
    int m_iMaxLogFiles;
    LPTRANSACTINFO m_lpS;

    int CreateDirectoryRecursively(CString strDirectory);
    int IsExistingDirectory(CString strDir);

// Dialog Data
	//{{AFX_DATA(CuDlgLogSize)
	enum { IDD = IDD_LOGFILE_CREATE };
	CListBox	m_lbRootLocs;
	int		m_size;
	float	m_edSizePerFile;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgLogSize)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgLogSize)
	afx_msg void OnAdd();
	afx_msg void OnRemove();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnChangeSizeTotal();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGLOGSZ_H__55562D61_38D5_11D1_A303_00609725DBFA__INCLUDED_)
