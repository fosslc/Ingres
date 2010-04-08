/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : fevsetti.h, Header file                                               //
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
* uk$so01: (21-May-1999 created)
*
*
*/

#if !defined (IVM_EVENTSETTING_FRAME_HEADER)
#define IVM_EVENTSETTING_FRAME_HEADER
#include "docevset.h"   // Frame View Document
#include "vevsettl.h"   // Tree view (Left View)
#include "vevsettr.h"   // Tree View (Right View)
#include "vevsettb.h"   // Bottom view (support of list control)
#include "msgntree.h"   //m_treeCategoryView and m_treeStateView


class CfEventSetting : public CFrameWnd
{
public:
	//
	// public constructor used for manual create
	CfEventSetting(CdEventSetting* pDoc);

	BOOL IsAllViewsCreated(){return m_bAllViewCreated;}
	CvEventSettingLeft*   GetLeftView();
	CvEventSettingRight*  GetRightView();
	CvEventSettingBottom* GetBottomView();

	UINT GetHelpID();

	CaTreeMessageCategory& GetTreeCategoryView(){return m_treeCategoryView;}
	CaTreeMessageState& GetTreeStateView(){return m_treeStateView;}

	BOOL SaveMessageState();
	BOOL SaveUserCategory();


	BOOL SaveSettings();
	CaMessageManager* GetMessageManager() {return m_pMessageEntry;}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfEventSetting)
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CaMessageManager* m_pMessageEntry;
	CaCategoryDataUserManager m_userCategory;

	CaTreeMessageCategory m_treeCategoryView;
	CaTreeMessageState m_treeStateView;


	CdEventSetting* m_pEventSettingDoc;
	CSplitterWnd m_MainSplitterWnd;
	CSplitterWnd m_NestSplitterWnd;
	BOOL m_bAllViewCreated;

	virtual ~CfEventSetting();

	// Generated message map functions
	//{{AFX_MSG(CfEventSetting)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif //#ifndef IVM_EVENTSETTING_FRAME_HEADER
