// DgDbeP02.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "dbedoc.h"
#include "dgdbep01.h"
#include "dgdbep02.h"
#include "dbeframe.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define LAYOUT_NUMBER 5


extern "C"
{
#include "dba.h"   
#include "dbaginfo.h"   
#include "domdata.h"
}


/////////////////////////////////////////////////////////////////////////////
// CuDlgDBEventPane02 dialog


CuDlgDBEventPane02::CuDlgDBEventPane02(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDBEventPane02::IDD, pParent)
{
    m_bSpecial = FALSE;
    //{{AFX_DATA_INIT(CuDlgDBEventPane02)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDBEventPane02::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CuDlgDBEventPane02)
    DDX_Control(pDX, IDC_MFC_EDIT1, m_cHeader1);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDBEventPane02, CDialog)
    //{{AFX_MSG_MAP(CuDlgDBEventPane02)
    ON_WM_SIZE()
    ON_EN_UPDATE(IDC_MFC_EDIT1, OnUpdateEditHeader)
    //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_DBEVENT_TRACE_INCOMING, OnDbeventTraceIncoming)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDBEventPane02 message handlers
void CuDlgDBEventPane02::ResetDBEvent()
{
    CView* pView = (CView*)GetParent();
    ASSERT (pView);
    CDbeventDoc* pDoc = (CDbeventDoc*)pView->GetDocument();
    ASSERT (pDoc);
//    pDoc->m_nCounter1 = 1;
//    pDoc->m_nCounter2 = 0;
    m_cListRaisedDBEvent.DeleteAllItems();
}

void CuDlgDBEventPane02::PostNcDestroy() 
{
    delete this;    
    CDialog::PostNcDestroy();
}

BOOL CuDlgDBEventPane02::OnInitDialog() 
{
    CDialog::OnInitDialog();
    VERIFY (m_cListRaisedDBEvent.SubclassDlgItem (IDC_MFC_LIST1, this));
    //
    // Initalize the Column Header of CListCtrl (CuListCtrl)
    //
    LV_COLUMN lvcolumn;
    UINT      strHeaderID[LAYOUT_NUMBER] = 
    {
        IDS_RAISEDDBE_NUM,
        IDS_RAISEDDBE_TIME, 
        IDS_RAISEDDBE_NAME, 
        IDS_RAISEDDBE_OWNER, 
        IDS_RAISEDDBE_TEXT
    };
    int i, hWidth   [LAYOUT_NUMBER] = {40, 100, 80, 100, 400};
    //
    // Set the number of columns: LAYOUT_NUMBER
    //
    memset (&lvcolumn, 0, sizeof (LV_COLUMN));
    lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
    for (i=0; i<LAYOUT_NUMBER; i++)
    {
        CString strHeader;
        strHeader.LoadString (strHeaderID[i]);
        lvcolumn.fmt = LVCFMT_LEFT;
        lvcolumn.pszText = (LPTSTR)(LPCTSTR)strHeader;
        lvcolumn.iSubItem = i;
        lvcolumn.cx = hWidth[i];
        m_cListRaisedDBEvent.InsertColumn (i, &lvcolumn); 
    }
    CView* pView = (CView*)GetParent();
    ASSERT (pView);
    CDbeventDoc* pDoc = (CDbeventDoc*)pView->GetDocument();
    ASSERT (pDoc);
    pDoc->SetRaisedDBEventList (&m_cListRaisedDBEvent);
    try
    {
        //
        // If the Document is load, then we update the control.
        UpdateControl();
    }
    catch (CMemoryException* e)
    {
        VDBA_OutOfMemoryMessage ();
        e->Delete();
    }
    catch (...)
    {
        throw;
    }
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDBEventPane02::OnSize(UINT nType, int cx, int cy) 
{
    CDialog::OnSize(nType, cx, cy);
    if (!IsWindow (m_cListRaisedDBEvent.m_hWnd) || !IsWindow (m_cHeader1.m_hWnd))
        return;
    CRect rDlg, r;
    GetClientRect (rDlg);
    m_cListRaisedDBEvent.GetWindowRect (r);
    ScreenToClient (r);
    r.left  = rDlg.left;
    r.right = rDlg.right;
    r.bottom= rDlg.bottom;
    m_cListRaisedDBEvent.MoveWindow (r);
}


void CuDlgDBEventPane02::IncomingDBEvent (
    CDbeventDoc* pDoc, 
    LPCTSTR strNum, 
    LPCTSTR strTime, 
    LPCTSTR strDbe, 
    LPCTSTR strOwner, 
    LPCTSTR strText,
    LPCTSTR strExtra)
{
    CString strLine;
    int diff, nCount, index;
    nCount = m_cListRaisedDBEvent.GetItemCount ();
   
    if (nCount >= pDoc->m_nMaxLine)
    {
        //
        // Special State  ???
        if (m_bSpecial)
            return;
        diff = nCount - pDoc->m_nMaxLine;
        if (pDoc->m_bClearFirst)
        {
            for (int i=0; i<=diff; i++)
                m_cListRaisedDBEvent.DeleteItem (i);
        }
        else
        {
            int answ;
            //"Cannot continue. DB Event Trace is interrupted until\n"
            //"you clear the list, change the max lines, or\n"
            //"check the 'Clear First When Max Reached' check box.";
            CString strMsg = VDBA_MfcResourceString(IDS_E_DB_EVENT_TRACE);
            //
            // DB Event is raised ...
            CTime   time = CTime::GetCurrentTime();
            CString strTf = time.Format ("%c");
            if (lstrlen (strNum) > 0 && lstrcmpi (strNum, "r") == 0)
            {	//"DB Event was raised, but the list is full. Clear first entry in the list ?";
                CString msg = VDBA_MfcResourceString(IDS_E_DB_EVENT_RAISED);
                answ = BfxMessageBox (msg, MB_YESNO|MB_ICONQUESTION);
                if (answ == IDNO)
                {
                    m_bSpecial = TRUE;
                    BfxMessageBox (strMsg, MB_OK);
                    strLine = ""; 
                    index   = m_cListRaisedDBEvent.InsertItem (nCount, (LPTSTR)(LPCTSTR)strLine, 0);
                    m_cListRaisedDBEvent.SetItemText (index, 1, (LPTSTR)(LPCTSTR)strTf);
                    m_cListRaisedDBEvent.SetItemText (index, 2, VDBA_MfcResourceString(IDS_INTERRUPTED));//"<Interrupted>"
                    return;
                }
            }
            //
            // User registers or unregisters DB Event...
            else
            if (lstrlen (strNum) > 0 && lstrcmpi (strNum, "?") == 0)
            {//"The list is full. Clear first entry in the list ?"
                CString msg = VDBA_MfcResourceString(IDS_E_DB_EVENT_LIST_FULL);
                answ = BfxMessageBox (msg, MB_YESNO|MB_ICONQUESTION);
                if (answ == IDNO)
                {
                    BfxMessageBox (strMsg, MB_OK);
                    m_bSpecial = TRUE;
                    strLine = "";
                    index   = m_cListRaisedDBEvent.InsertItem (nCount, (LPTSTR)(LPCTSTR)strLine, 0);
                    m_cListRaisedDBEvent.SetItemText (index, 1, (LPTSTR)(LPCTSTR)strTf);
                    m_cListRaisedDBEvent.SetItemText (index, 2, VDBA_MfcResourceString(IDS_INTERRUPTED));//"<Interrupted>"
                    return;
                }
            }
            //
            // Fill up the list while loading ....
            else
            {//"The list is full. Clear first entry in the list ?"
                CString msg = VDBA_MfcResourceString(IDS_E_DB_EVENT_LIST_FULL);
                answ = BfxMessageBox (msg, MB_YESNO|MB_ICONQUESTION);
                if (answ == IDNO)
                {
                    m_bSpecial = TRUE;
                    BfxMessageBox (strMsg, MB_OK);
                }
                if (nCount == pDoc->m_nMaxLine)
                {
                    // We have add the line  "<Interrupted>"
                    // We cannot add the line "<Restarted>" so we just return;
                    return;
                }
            }
            if (answ == IDYES)
            {
                for (int i=0; i<=diff; i++)
                    m_cListRaisedDBEvent.DeleteItem (i);
            }
        }
    }
    else
    {
        m_bSpecial = FALSE;
    }
    if (lstrlen (strNum) > 0)
    {
        if (lstrcmpi (strNum, "?") == 0 || lstrcmpi (strNum, "r") == 0)
        {
            //
            // Define the line number;
            pDoc->m_nCounter2++;
            if (pDoc->m_nCounter2 < 0)
            {
                pDoc->m_nCounter1++;
                pDoc->m_nCounter2 = 1;
            }
            if (pDoc->m_nCounter1 == 1)
            {
                strLine.Format ("%4d", pDoc->m_nCounter2);
            }
            else
            {
                strLine.Format ("%d/%d", pDoc->m_nCounter1, pDoc->m_nCounter2);
            }
            index  = m_cListRaisedDBEvent.InsertItem (nCount, (LPTSTR)(LPCTSTR)strLine, 0);
        }
        else
            index  = m_cListRaisedDBEvent.InsertItem (nCount, (LPTSTR)(LPCTSTR)strNum, 0);
    }
    else
        index  = m_cListRaisedDBEvent.InsertItem (nCount, (LPTSTR)"", 0);
    //
    // Add the Time
    m_cListRaisedDBEvent.SetItemText (index, 1, (LPTSTR)strTime);
    //
    // Add the DBEvent
    if (strExtra)
    {
        CString strStar = strExtra;
        strStar += strDbe;
        m_cListRaisedDBEvent.SetItemText (index, 2, (LPTSTR)(LPCTSTR)strStar);
    }
    else
        m_cListRaisedDBEvent.SetItemText (index, 2, (LPTSTR)strDbe);
    //
    // Add the Owner
    m_cListRaisedDBEvent.SetItemText (index, 3, (LPTSTR)strOwner);
    //
    // Add the Text
    m_cListRaisedDBEvent.SetItemText (index, 4, (LPTSTR)strText);
}

LONG CuDlgDBEventPane02::OnDbeventTraceIncoming (WPARAM wParam, LPARAM lParam)
{
    LPRAISEDDBE lpStruct = (LPRAISEDDBE)lParam;
    CView*  pView = (CView*)GetParent();
    ASSERT  (pView);
    CDbeventDoc* pDoc = (CDbeventDoc*)pView->GetDocument();
    ASSERT  (pDoc);
    CSplitterWnd* pSplitter = (CSplitterWnd*)pView->GetParent();
    ASSERT (pSplitter);
    CDbeventFrame*  pFrame = (CDbeventFrame*)pSplitter->GetParent();
    ASSERT  (pFrame);
    if (!pFrame->GetPaneRegisteredDBEvent()->Find ((LPCTSTR)lpStruct->DBEventName, (LPCTSTR)lpStruct->DBEventOwner))
    {
        IncomingDBEvent (
            pDoc, 
            "r",
            (LPCTSTR)lpStruct->StrTimeStamp, 
            (LPCTSTR)lpStruct->DBEventName, 
            (LPCTSTR)lpStruct->DBEventOwner, 
            (LPCTSTR)lpStruct->DBEventText,
            "*");
    }
    else
    {
        IncomingDBEvent (
            pDoc, 
            "r",
            (LPCTSTR)lpStruct->StrTimeStamp, 
            (LPCTSTR)lpStruct->DBEventName, 
            (LPCTSTR)lpStruct->DBEventOwner, 
            (LPCTSTR)lpStruct->DBEventText);
    }
    if (pDoc->m_bPopupOnRaise)
    {
        CString strMsg;
        strMsg.GetBuffer (520);
        TCHAR tchszAll [80];

        StringWithOwner ((LPUCHAR)lpStruct->DBEventName, (LPUCHAR)lpStruct->DBEventOwner, (LPUCHAR)tchszAll);
        //"%s: Database Event %s '%s' was raised on node %s."
            strMsg.Format (IDS_I_DB_EVENT_RAISED,
            (LPCTSTR)lpStruct->StrTimeStamp,
            (LPCTSTR)tchszAll,
            (LPCTSTR)lpStruct->DBEventText,
            (LPCTSTR)GetVirtNodeName (pDoc->m_hNode));
        BfxMessageBox (strMsg);
    }
    return RES_SUCCESS;
}

BOOL CuDlgDBEventPane02::OnMaxLineChange (int nMax)
{
    CView*  pView = (CView*)GetParent();
    ASSERT  (pView);
    CDbeventDoc* pDoc = (CDbeventDoc*)pView->GetDocument();
    ASSERT  (pDoc);

    int nCount = m_cListRaisedDBEvent.GetItemCount();
    if (nCount > nMax)
    {
        int i, diff = nCount - nMax;
        if (pDoc->m_bClearFirst)
        {
            for (i=0; i<diff; i++)
                m_cListRaisedDBEvent.DeleteItem (0); 
        }
        else
        {
            //CString strMsg = "New value is smaller than actual number of lines,\n"
            //                 "and 'Clear First When Max Reached is not checked'.\n"
            //                 "Previous value is kept";
            BfxMessageBox (VDBA_MfcResourceString(IDS_E_NEW_VALUE), MB_OK|MB_ICONEXCLAMATION);
            return FALSE;
        }
    }
    return TRUE;
}

void CuDlgDBEventPane02::UpdateControl()
{
    CView*  pView = (CView*)GetParent();
    ASSERT  (pView);
    CDbeventDoc* pDoc = (CDbeventDoc*)pView->GetDocument();
    ASSERT  (pDoc);
    CWnd*   pSplitter = pView->GetParent();
    ASSERT (pSplitter);
    CDbeventFrame* pFrame = (CDbeventFrame*)pSplitter->GetParent();
    ASSERT (pFrame);

    CString strNone;
    if (strNone.LoadString (IDS_DATABASE_NONE) == 0)
        strNone = "<None>";
    if (pDoc->m_strDBName == "" || pDoc->m_strDBName == strNone)
        return;
    
    // Load ...
    int nCount;
    CTypedPtrList<CObList, CuDataRaisedDBevent*>& listRaisedDBEvent = pDoc->m_listRaisedDBEvent;
    nCount = listRaisedDBEvent.GetCount();
    POSITION pos = listRaisedDBEvent.GetHeadPosition();
    while  (pos != NULL)
    {
        CuDataRaisedDBevent* dbe = listRaisedDBEvent.GetNext (pos);
        IncomingDBEvent (pDoc, dbe->m_strNum, dbe->m_strTime, dbe->m_strDBEvent, dbe->m_strDBEOwner, dbe->m_strDBEText);
    }
    if ((pDoc->m_nMaxLine+1) == nCount)
    {
        CTime   t = CTime::GetCurrentTime();
        CString st= t.Format ("%c");
        nCount = m_cListRaisedDBEvent.GetItemCount ();
        int index  = m_cListRaisedDBEvent.InsertItem (nCount, (LPTSTR)"", 0);
        m_cListRaisedDBEvent.SetItemText (index, 1, (LPTSTR)(LPCTSTR)st);
        m_cListRaisedDBEvent.SetItemText (index, 2, VDBA_MfcResourceString(IDS_INTERRUPTED));//"<Interrupted>"
    }     
    SetHeader (pDoc->m_strRaisedSince);
}

void CuDlgDBEventPane02::OnUpdateEditHeader() 
{
    CRect r;
    SIZE  size;
    CClientDC dc (this);
    m_cHeader1.GetWindowRect (r);
    ScreenToClient (r);
    CString strText;

    m_cHeader1.GetWindowText (strText);
    if (strText.GetLength() == 0)
        return;
    if (GetTextExtentPoint32 (dc.m_hDC, (LPCTSTR)strText, strText.GetLength(), &size))
    {
        r.right = r.left + size.cx;
        m_cHeader1.MoveWindow (r); 
    }
}
