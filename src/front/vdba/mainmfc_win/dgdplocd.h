/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdplocd.h, header file
**    Project  : INGRES II/ VDBA (DOM Right pane).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of Table control. Pie Chart.
**               For a given location, the partitions used by multiple databases.
**
** 
** Histoty: (after 08-Apr-2003)
** 08-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
*/

#ifndef DOMPROP_LOCATIONDATABASES_HEADER
#define DOMPROP_LOCATIONDATABASES_HEADER
class CfStatisticFrame;

class CuDlgDomPropLocationDbs: public CDialog
{
public:
    CuDlgDomPropLocationDbs(CWnd* pParent = NULL);  
    void OnCancel() {return;}
    void OnOK()     {return;}

    // Dialog Data
    //{{AFX_DATA(CuDlgDomPropLocationDbs)
    enum { IDD = IDD_IPMDETAIL_LOCATION };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDomPropLocationDbs)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

    // Implementation
protected:
    CfStatisticFrame* m_pDlgFrame;
    void ResetDisplay();
    // Generated message map functions
    //{{AFX_MSG(CuDlgDomPropLocationDbs)
    virtual BOOL OnInitDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
    afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);
};

#endif  // DOMPROP_LOCATIONDATABASES_HEADER
