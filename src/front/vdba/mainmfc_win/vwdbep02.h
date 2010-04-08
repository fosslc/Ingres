/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : VwDbeP02.h, Header File   (View of MDI Child Frame)                   //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut.                                                         //
//                                                                                     //
//                                                                                     //
//    Purpose  : The right pane for the DBEvent Trace.                                 //
//               It contains the Modeless Dialog (Header and ListCtrl)                 //
****************************************************************************************/
#ifndef VWDBEP02_HEADER
#define VWDBEP02_HEADER
#include "dgdbep02.h"
class CDbeventView2 : public CView
{
protected:
    CDbeventView2();           // protected constructor used by dynamic creation
    DECLARE_DYNCREATE(CDbeventView2)

public:
    CuDlgDBEventPane02* m_pDlgRaisedDBEvent;
    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDbeventView2)
	public:
    virtual void OnInitialUpdate();
	protected:
    virtual void OnDraw(CDC* pDC);      // overridden to draw this view
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

    // Implementation
protected:
    virtual ~CDbeventView2();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    // Generated message map functions
protected:
    //{{AFX_MSG(CDbeventView2)
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif
