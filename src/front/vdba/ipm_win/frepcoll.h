/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : frepcoll.h, header file 
**    Project  : INGRES II/ Replication Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Frame to provide splitter of the Replication Page for Collision
**
** History:
**
** xx-Sep-1997 (uk$so01)
**    Created
*/


#ifndef FRAME_REPLICATION_PAGE_COLLISION_HEADER
#define FRAME_REPLICATION_PAGE_COLLISION_HEADER

class CfReplicationPageCollision : public CFrameWnd
{
public:
	CfReplicationPageCollision();           // public constructor used for manual create
	BOOL IsAllViewsCreated(){return m_bAllViewCreated;}
	//
	// Tree View
	CWnd* GetLeftPane (){return (CWnd*)m_Splitterwnd.GetPane (0, 0);}
	//
	// The Modeless Dialog: CuDlgReplicationPageCollisionRight
	CWnd* GetRightPane(){return (CWnd*)m_Splitterwnd.GetPane (0, 1);}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfReplicationPageCollision)
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CSplitterWnd m_Splitterwnd;
	BOOL m_bAllViewCreated;
	virtual ~CfReplicationPageCollision();

	// Generated message map functions
	//{{AFX_MSG(CfReplicationPageCollision)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif 
