/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dlgvfrm.h, Header file.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Frame for CFormView, Scrollable Dialog box of Detail Page
**
** History:
**
** xx-Mar-1997 (uk$so01)
**    Created
*/

#ifndef DLGVFRM_HEADER
#define DLGVFRM_HEADER

class CuDlgViewFrame : public CFrameWnd
{
public:
	CuDlgViewFrame();
	CuDlgViewFrame(UINT nDlgID);


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgViewFrame)
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

	// Implementation
protected:
	UINT m_nDlgID;

	virtual ~CuDlgViewFrame();

	// Generated message map functions
	//{{AFX_MSG(CuDlgViewFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLeavingPage(WPARAM wParam, LPARAM lParam);
};

#endif
