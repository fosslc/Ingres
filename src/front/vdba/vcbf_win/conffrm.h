/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : conffrm.h, header file                                                //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Frame support to provide the splitter in the Page (Configure) of      //
//               the class CMainTabDlg                                                 //
//               See the CONCEPT.DOC for more detail                                   //
****************************************************************************************/
/////////////////////////////////////////////////////////////////////////////
// CConfigFrame frame
#ifndef CONFFRM_HEADER
#define CONFFRM_HEADER
#include "cleftdlg.h"
#include "crightdg.h"

class CConfigFrame : public CFrameWnd
{
public:
	CConfigFrame();           // public constructor used for manual create
	BOOL IsAllViewsCreated(){return m_bAllViewCreated;}
	CConfLeftDlg*  GetCConfLeftDlg();
	CConfRightDlg* GetCConfRightDlg();
    BOOL DisplayContextBranch();
	UINT GetHelpID();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigFrame)
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CSplitterWnd m_Splitterwnd;
	BOOL m_bAllViewCreated;
	virtual ~CConfigFrame();

	// Generated message map functions
	//{{AFX_MSG(CConfigFrame)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif //#ifndef CONFFRM_HEADER
