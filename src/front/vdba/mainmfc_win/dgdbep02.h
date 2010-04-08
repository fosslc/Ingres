/****************************************************************************************
//                                                                                     //
//    Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                 //
//                                                                                     //
//    Source   : DgDbeP02.h, Header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Modeless Dialog Box that will appear at the right pane of the          //
//               DBEvent Trace Frame.                                                  //
****************************************************************************************/
#ifndef DGDBEP02_HEADER
#define DGDBEP02_HEADER
#include "calsctrl.h"
#include "dbedoc.h"

class CuDlgDBEventPane02 : public CDialog
{
public:
    CuDlgDBEventPane02(CWnd* pParent = NULL); 

    // Dialog Data
    //{{AFX_DATA(CuDlgDBEventPane02)
    enum { IDD = IDD_DBEVENTPANE2 };
    CStatic    m_cHeader1;
    //}}AFX_DATA
    CuListCtrl m_cListRaisedDBEvent;
    void OnCancel() {return;}
    void OnOK()     {return;}
    void IncomingDBEvent (
        CDbeventDoc* pDoc, 
        LPCTSTR strNum, 
        LPCTSTR strTime, 
        LPCTSTR strDbe, 
        LPCTSTR strOwner, 
        LPCTSTR strText,
        LPCTSTR strExtra = NULL);
    void ResetDBEvent ();
    BOOL OnMaxLineChange (int nMax);
    void SetHeader (LPCTSTR lpszHeader){m_cHeader1.SetWindowText (lpszHeader);};
    void SetSpecialState (BOOL bSpecial){m_bSpecial = bSpecial;}
    BOOL GetSpecialState () {return m_bSpecial;}
    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDBEventPane02)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

    // Implementation
protected:
    BOOL m_bSpecial;

    void UpdateControl();

    // Generated message map functions
    //{{AFX_MSG(CuDlgDBEventPane02)
    virtual BOOL OnInitDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnUpdateEditHeader();
    //}}AFX_MSG
    afx_msg LONG OnDbeventTraceIncoming (WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()
};

#endif
