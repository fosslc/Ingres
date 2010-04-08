/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : mgrfrm.h, header file                                                 //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : Frame support to provide the splitter window                          //
****************************************************************************************/
/* History:
* --------
* uk$so01: (15-Dec-1998 created)
*
*
*/

#if !defined (IVM_MANAGER_FRAME_HEADER)
#define CONFFRM_HEADER
#define IVM_MANAGER_FRAME_HEADER

#include "maindoc.h"   // Main Document
#include "mgrvleft.h"  // Tree view (Left View)
#include "mgrvrigh.h"  // View that contains the Tab Control (Right View)

typedef struct tagNOTIFICATIONDATA
{
	UINT  nIconID;
	HICON hIcon;
} NOTIFICATIONDATA;


class CfManagerFrame : public CFrameWnd
{
public:
	//
	// public constructor used for manual create
	CfManagerFrame(CdMainDoc* pDoc);

	BOOL IsAllViewsCreated(){return m_bAllViewCreated;}
	CvManagerViewLeft*  GetLeftView();
	CvManagerViewRight* GetRightView();

	UINT GetHelpID();


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfManagerFrame)
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CdMainDoc* m_pMainDoc;
	CSplitterWnd m_Splitterwnd;
	BOOL m_bAllViewCreated;
	virtual ~CfManagerFrame();

	// Generated message map functions
	//{{AFX_MSG(CfManagerFrame)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif //#ifndef CONFFRM_HEADER
