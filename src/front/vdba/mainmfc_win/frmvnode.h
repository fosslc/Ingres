/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : FrmVNode.h, Header file    (MDI Child Frame)                          //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Virtual Node Dialog Bar Resizable becomes a MDI Child when            //
//               it is not a Docking View.                                             //
****************************************************************************************/
#ifndef FRMVNODE_HEADER
#define FRMVNODE_HEADER
class CFrmVNodeMDIChild : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CFrmVNodeMDIChild)
protected:
	CFrmVNodeMDIChild();           // protected constructor used by dynamic creation
	CToolBar    m_wndToolBar;
public:

    // Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFrmVNodeMDIChild)
	//}}AFX_VIRTUAL

protected:
	virtual ~CFrmVNodeMDIChild();

	// Generated message map functions
	//{{AFX_MSG(CFrmVNodeMDIChild)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	//}}AFX_MSG
  afx_msg LONG OnGetNodeHandle(UINT uu, LONG ll);
  afx_msg LONG OnGetMfcDocType(UINT uu, LONG ll);
	DECLARE_MESSAGE_MAP()
};

#endif
