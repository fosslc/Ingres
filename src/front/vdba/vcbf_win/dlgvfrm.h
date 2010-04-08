/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : DlgVFrm.h, Header file.                                               //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring, Modifying for .                             //
//               OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Frame for CFormView, Scrollable Dialog box of Detail Page.            //
//                                                                                     //
****************************************************************************************/
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
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	afx_msg LONG OnUpdateData   (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnValidateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnButton1      (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnButton2      (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnButton3      (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnButton4      (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnButton5      (WPARAM wParam, LPARAM lParam);
};

#endif
