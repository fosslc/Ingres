/****************************************************************************************
//                                                                                     //
//  Copyright (C) Feb, 1997 Computer Associates International, Inc.                    //
//                                                                                     //
//    Source   : DgDbeP01.h, Header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Modeless Dialog Box that will appear at the left pane of the          //
//               DBEvent Trace Frame.                                                  //
****************************************************************************************/
#ifndef DGDBEP01_HEADER
#define DGDBEP01_HEADER
class CuDlgDBEventPane01 : public CDialog
{
public:
    CuDlgDBEventPane01(CWnd* pParent = NULL);   

    // Dialog Data
    //{{AFX_DATA(CuDlgDBEventPane01)
    enum { IDD = IDD_DBEVENTPANE1 };
    CStatic    m_cHeader1;
    //}}AFX_DATA
    CString       m_strCurrentDB;           // The current database being used.
    CCheckListBox m_cListDBEvent;
    void OnCancel() {return;}
    void OnOK()     {return;}
    void ForceRefresh();                    // Cash and DOM
    void OnRefresh();                       // The control itself. (Check list box)
    void InitializeDBEvent (
        CString& str, 
        CCheckListBox* pList = NULL);       // Requery the DBEvent of the Database.
    void UnregisterDBEvents();
    BOOL Find (LPCTSTR lpszDbe, LPCTSTR lpszDbeOwner);

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDBEventPane01)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

    // Implementation
protected:
    BOOL m_bDBEInit;

    void UpdateControl();
    // Generated message map functions
    //{{AFX_MSG(CuDlgDBEventPane01)
    virtual BOOL OnInitDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnDestroy();
    //}}AFX_MSG
    afx_msg void OnCheckChange();

    DECLARE_MESSAGE_MAP()
};

#endif
