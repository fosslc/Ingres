/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : Ingres II/ VDBA.
**
**    Source : creatloc.h : header file
**
**    Author   : Schalk Philippe
**
**  Purpose  : defined Class to manage the dialog box for "location" command.
**
**   09-May-2001 (schph01)
**     SIR 102509 created
********************************************************************/
#if !defined(AFX_CREATLOC_H__BE98A613_3FBB_11D5_B605_00C04F1790C3__INCLUDED_)
#define AFX_CREATLOC_H__BE98A613_3FBB_11D5_B605_00C04F1790C3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CxDlgCreateLocation dialog

class CxDlgCreateLocation : public CDialog
{
// Construction
public:
	CxDlgCreateLocation(CWnd* pParent = NULL);   // standard constructor

	LPLOCATIONPARAMS m_lpLocation;
	BOOL m_bLocRawAvailable;          // TRUE for Unix platform
                                      // FALSE for NT and VMS Platform
	void IsRawLocAvailable();

// Dialog Data
	//{{AFX_DATA(CxDlgCreateLocation)
	enum { IDD = IDD_LOCATION };
	CEdit	m_ctrlEdPctRaw;
	CStatic	m_ctrlPctRaw;
	CSpinButtonCtrl	m_ctrlSpinPctRaw;
	CButton	m_ctrlCheckWork;
	CButton	m_ctrlCheckJournal;
	CButton	m_ctrlCheckDump;
	CButton	m_ctrlCheckDatabase;
	CButton	m_ctrlCheckCheckPoint;
	CEdit	m_ctrlEditLocationName;
	CEdit	m_ctrlEditArea;
	CButton	m_ctrlRawArea;
	BOOL	m_bCheckPoint;
	BOOL	m_bDatabase;
	BOOL	m_bDump;
	BOOL	m_bJournal;
	BOOL	m_bWork;
	CString	m_edArea;
	CString	m_edLocationName;
	BOOL	m_bRawArea;
	int		m_editPctRaw;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgCreateLocation)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void EnableDisableOKButton();
	void UpdateControlsFromStructure();
	BOOL FillStructureFromControls(LPLOCATIONPARAMS lpLocTempo);
	int CreateLocation();
	int AlterLocation();
	// Generated message map functions
	//{{AFX_MSG(CxDlgCreateLocation)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckRawArea();
	afx_msg void OnChangeEditArea();
	afx_msg void OnChangeEditLocName();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CREATLOC_H__BE98A613_3FBB_11D5_B605_00C04F1790C3__INCLUDED_)
