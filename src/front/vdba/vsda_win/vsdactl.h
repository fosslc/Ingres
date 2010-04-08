/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vsdactl.h : Declaration of the CuSdaCtrl ActiveX Control class.
**    Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Declaration of the CuSdaCtrl ActiveX Control class
**
** History:
**
** 26-Aug-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
*/

#if !defined(AFX_VSDACTL_H__CC2DA2C3_B8F1_11D6_87D8_00C04F1F754A__INCLUDED_)
#define AFX_VSDACTL_H__CC2DA2C3_B8F1_11D6_87D8_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "vsdafrm.h"

class CuSdaCtrl : public COleControl
{
	DECLARE_DYNCREATE(CuSdaCtrl)

public:
	CuSdaCtrl();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuSdaCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	~CuSdaCtrl();
	CfSdaFrame* m_pVsdFrame;

	DECLARE_OLECREATE_EX(CuSdaCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CuSdaCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CuSdaCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CuSdaCtrl)      // Type name and misc status

	// Message maps
	//{{AFX_MSG(CuSdaCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// Dispatch maps
	//{{AFX_DISPATCH(CuSdaCtrl)
	afx_msg void SetSchema1Param(LPCTSTR lpszNode, LPCTSTR lpszDatabase, LPCTSTR lpszUser);
	afx_msg void SetSchema2Param(LPCTSTR lpszNode, LPCTSTR lpszDatabase, LPCTSTR lpszUser);
	afx_msg void StoreSchema(LPCTSTR lpszNode, LPCTSTR lpszDatabase, LPCTSTR lpszUser, LPCTSTR lpszFile);
	afx_msg void LoadSchema1Param(LPCTSTR lpszFile);
	afx_msg void LoadSchema2Param(LPCTSTR lpszFile);
	afx_msg void DoCompare(short nMode);
	afx_msg void UpdateDisplayFilter(LPCTSTR lpszFilter);
	afx_msg BOOL IsEnableDiscard();
	afx_msg void DiscardItem();
	afx_msg BOOL IsEnableUndiscard();
	afx_msg void UndiscardItem();
	afx_msg void HideFrame(short nShow);
	afx_msg BOOL IsEnableAccessVdba();
	afx_msg void AccessVdba();
	afx_msg void SetSessionStart(long nStart);
	afx_msg void SetSessionDescription(LPCTSTR lpszDescription);
	afx_msg BOOL IsResultFrameVisible();
	afx_msg void DoWriteFile();
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	// Event maps
	//{{AFX_EVENT(CuSdaCtrl)
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

	// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CuSdaCtrl)
	dispidSetSchema1Param = 1L,
	dispidSetSchema2Param = 2L,
	dispidStoreSchema = 3L,
	dispidLoadSchema1Param = 4L,
	dispidLoadSchema2Param = 5L,
	dispidDoCompare = 6L,
	dispidUpdateDisplayFilter = 7L,
	dispidIsEnableDiscard = 8L,
	dispidDiscardItem = 9L,
	dispidIsEnableUndiscard = 10L,
	dispidUndiscardItem = 11L,
	dispidHideFrame = 12L,
	dispidIsEnableAccessVdba = 13L,
	dispidAccessVdba = 14L,
	dispidSetSessionStart = 15L,
	dispidSetSessionDescription = 16L,
	dispidIsResultFrameVisible = 17L,
	dispidDoWriteFile = 18L,
	//}}AFX_DISP_ID
	};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VSDACTL_H__CC2DA2C3_B8F1_11D6_87D8_00C04F1F754A__INCLUDED)
