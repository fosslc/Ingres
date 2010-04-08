/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : cachefrm.h, header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Frame to provide splitter of the DBMS Cache Page                      //
//               See the CONCEPT.DOC for more detail                                   //
****************************************************************************************/
/////////////////////////////////////////////////////////////////////////////
// CfDbmsCacheFrame frame
#ifndef CACHEFRM_HEADER
#define CACHEFRM_HEADER

class CfDbmsCacheFrame : public CFrameWnd
{
public:
	CfDbmsCacheFrame();           // public constructor used for manual create
	BOOL IsAllViewsCreated(){return m_bAllViewCreated;}
	CWnd* GetLeftPane (){return (CWnd*)m_Splitterwnd.GetPane (0, 0);}
	CWnd* GetRightPane(){return (CWnd*)m_Splitterwnd.GetPane (0, 1);}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfDbmsCacheFrame)
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CSplitterWnd m_Splitterwnd;
	BOOL m_bAllViewCreated;
	virtual ~CfDbmsCacheFrame();

	// Generated message map functions
	//{{AFX_MSG(CfDbmsCacheFrame)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif 
