#ifndef DOMPROP_DB_ALARMS_HEADER
#define DOMPROP_DB_ALARMS_HEADER

#include "domseri.h"
#include "domilist.h"  // CuDomImageList

class CuDlgDomPropDbAlarms : public CDialog
{
public:
    CuDlgDomPropDbAlarms(CWnd* pParent = NULL);  
    void OnCancel() {return;}
    void OnOK()     {return;}

    // Dialog Data
    //{{AFX_DATA(CuDlgDomPropDbAlarms)
    enum { IDD = IDD_DOMPROP_DB_ALARMS };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA
    CuListCtrlCheckmarks  m_cListCtrl;
    CImageList            m_ImageCheck;
    CuDomImageList        m_ImageList;

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDomPropDbAlarms)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

private:
  void RefreshDisplay();
  void AddDbAlarm (CuDbAlarm* pDbAlarm);

private:
  CuDomPropDataDbAlarms m_Data;

    // Implementation
protected:
    void ResetDisplay();
    // Generated message map functions
    //{{AFX_MSG(CuDlgDomPropDbAlarms)
    virtual BOOL OnInitDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnClickList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
    afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};

#endif  // DOMPROP_DB_ALARMS_HEADER
