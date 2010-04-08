/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vcdactl.h : Declaration of the CuVcdaCtrl ActiveX Control class
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Ocx control interfaces 
**
** History:
**
** 02-Oct-2002 (uk$so01)
**    SIR #109221, created
**    Created
** 30-Dec-2004 (komve01)
**    BUG #113714, Issue #13858925,
**    Added a BOOL indicator to track if current installation was compared.
*/

#if !defined(AFX_VCDACTL_H__EAF345F7_D514_11D6_87EA_00C04F1F754A__INCLUDED_)
#define AFX_VCDACTL_H__EAF345F7_D514_11D6_87EA_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
class CuDlgMain;
class CuVcdaCtrl : public COleControl
{
	DECLARE_DYNCREATE(CuVcdaCtrl)

public:
	CuVcdaCtrl();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuVcdaCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	//}}AFX_VIRTUAL

	// Implementation
	void DoCompare(){Compare();}
protected:
	~CuVcdaCtrl();
	CuDlgMain* m_pDlgMain;
	int m_nMode; // -1:unknow. 0:installation<->file. 1:file <-> file
	CString m_strFile1;
	CString m_strFile2;
	CString m_strInvitation;
	BOOL m_bComparedCurrInstall;

	DECLARE_OLECREATE_EX(CuVcdaCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CuVcdaCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CuVcdaCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CuVcdaCtrl)      // Type name and misc status

	// Message maps
	//{{AFX_MSG(CuVcdaCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// Dispatch maps
	//{{AFX_DISPATCH(CuVcdaCtrl)
	afx_msg void SaveInstallation(LPCTSTR lpszFile);
	afx_msg void SetCompareFile(LPCTSTR lpszFile);
	afx_msg void SetCompareFiles(LPCTSTR lpszFile1, LPCTSTR lpszFile2);
	afx_msg void Compare();
	afx_msg void HideFrame(short nShow);
	afx_msg void SetInvitationText(LPCTSTR lpszText);
	afx_msg void RestoreInstallation();
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	// Event maps
	//{{AFX_EVENT(CuVcdaCtrl)
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

	// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CuVcdaCtrl)
	dispidSaveInstallation = 1L,
	dispidSetCompareFile = 2L,
	dispidSetCompareFiles = 3L,
	dispidCompare = 4L,
	dispidHideFrame = 5L,
	dispidSetInvitationText = 6L,
	dispidRestoreInstallation = 7L,
	//}}AFX_DISP_ID
	};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VCDACTL_H__EAF345F7_D514_11D6_87EA_00C04F1F754A__INCLUDED)
