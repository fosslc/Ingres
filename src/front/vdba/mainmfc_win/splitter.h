/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : Splitter.h, Header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Special Splitter Control for Window NT.                               //
****************************************************************************************/
#ifndef SPLITTER_HEADER
#define SPLITTER_HEADER
class CuSplitter : public CStatic
{
public:
    CuSplitter ();
    CuSplitter (UINT alignment);
    void SetAlignment (UINT alignment) {m_Alignment = alignment;};
    virtual ~CuSplitter();
    static DWORD GetWindowVersion ();
    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuSplitter)
    protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    //}}AFX_VIRTUAL


    // Generated message map functions
protected:
    UINT    m_Alignment;
    CBrush  m_BrushW95;

    //{{AFX_MSG(CuSplitter)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnPaint();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

#endif
