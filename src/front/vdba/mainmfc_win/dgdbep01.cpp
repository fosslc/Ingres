/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : dgdbep01.cpp, Implementation file                                     //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Modeless Dialog Box that will appear at the left pane of the          //
//               DBEvent Trace Frame.                                                  //
****************************************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "dbedoc.h"
#include "dgdbep01.h"
#include "dgdbep02.h"
#include "dbeframe.h"
#include "uchklbox.h"

extern "C" 
{
#include "dba.h"
#include "dbaset.h"
#include "domdata.h"
#include "dbaginfo.h"   
#include "domdisp.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



BOOL CuDlgDBEventPane01::Find (LPCTSTR lpszDbe, LPCTSTR lpszDbeOwner)
{
    TCHAR   tchszAll1[MAXOBJECTNAME];
    TCHAR   tchszAll2[MAXOBJECTNAME];
    LPTSTR  lpszOwner;
    CString strItem;
    int i, nCount = m_cListDBEvent.GetCount();
    StringWithOwner ((LPUCHAR)lpszDbe, (LPUCHAR)lpszDbeOwner, (LPUCHAR)tchszAll1);
    for (i=0; i<nCount; i++)
    {
        m_cListDBEvent.GetText (i, strItem);
        lpszOwner = (LPTSTR)m_cListDBEvent.GetItemData (i);
        ASSERT (lpszOwner);
        StringWithOwner ((LPUCHAR)(LPCTSTR)strItem, (LPUCHAR)lpszDbeOwner, (LPUCHAR)tchszAll2);
        if (lstrcmpi (tchszAll1, tchszAll2) == 0)
            return TRUE;
    }
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CuDlgDBEventPane01 dialog
static void  CleanListBox (CCheckListBox* pList)
{
    LPTSTR lpszOwner = NULL;
    if (IsWindow (pList->m_hWnd))
    {
        int i, nCount;
        nCount = pList->GetCount();
        for (i=0; i<nCount; i++)
        {
            lpszOwner = (LPTSTR)pList->GetItemData (i);
            if (lpszOwner)
                delete [] lpszOwner;
        }
    }
}

CuDlgDBEventPane01::CuDlgDBEventPane01(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDBEventPane01::IDD, pParent)
{
    m_strCurrentDB = "";
    m_bDBEInit = FALSE;     // Initialize the DBEvent register failed.
    //{{AFX_DATA_INIT(CuDlgDBEventPane01)
    //}}AFX_DATA_INIT
}


void CuDlgDBEventPane01::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CuDlgDBEventPane01)
    DDX_Control(pDX, IDC_MFC_STATIC1, m_cHeader1);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDBEventPane01, CDialog)
    //{{AFX_MSG_MAP(CuDlgDBEventPane01)
    ON_WM_SIZE()
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
    ON_CLBN_CHKCHANGE (IDC_MFC_LIST1, OnCheckChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDBEventPane01 message handlers

void CuDlgDBEventPane01::PostNcDestroy() 
{
    delete this;    
    CDialog::PostNcDestroy();
}

BOOL CuDlgDBEventPane01::OnInitDialog() 
{
    CDialog::OnInitDialog();
    VERIFY (m_cListDBEvent.SubclassDlgItem (IDC_MFC_LIST1, this));

    CView* pView = (CView*)GetParent();
    ASSERT (pView);
    CDbeventDoc* pDoc = (CDbeventDoc*)pView->GetDocument();
    ASSERT (pDoc);
    pDoc->SetRegisteredDBEventList (&m_cListDBEvent);
    try
    {
        //
        // If the Document is load, then we update the control.
        UpdateControl();
    }
    catch (CMemoryException* e)
    {
        VDBA_OutOfMemoryMessage();
        e->Delete();
    }
    catch (...)
    {
        throw;
    }
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDBEventPane01::OnSize(UINT nType, int cx, int cy) 
{
    CDialog::OnSize(nType, cx, cy);
    if (!IsWindow (m_cListDBEvent.m_hWnd) || !IsWindow (m_cHeader1.m_hWnd))
        return;
    CRect rDlg, r;
    GetClientRect (rDlg);
    m_cHeader1.GetWindowRect (r);
    ScreenToClient (r);
    r.top  = rDlg.top;
    r.left = rDlg.left;
    r.right= rDlg.right;
    m_cHeader1.MoveWindow (r);
    
    m_cListDBEvent.GetWindowRect (r);
    ScreenToClient (r);
    r.left  = rDlg.left;
    r.right = rDlg.right;
    r.bottom= rDlg.bottom;
    m_cListDBEvent.MoveWindow (r);
}

void CuDlgDBEventPane01::ForceRefresh()
{
    TRACE0 ("CuDlgDBEventPane01::ForceRefresh() ...\n");
    CString strNone;
    CView* pView = (CView*)GetParent();
    ASSERT (pView);
    CDbeventDoc* pDoc = (CDbeventDoc*)pView->GetDocument();
    ASSERT (pDoc);
    LPUCHAR      parentstrings [MAXPLEVEL];
    if (strNone.LoadString (IDS_DATABASE_NONE) == 0)
        strNone = "<None>";
    if (pDoc->m_strDBName == "" || pDoc->m_strDBName == strNone)
        return;
    parentstrings[0] = (LPUCHAR)(LPCTSTR)pDoc->m_strDBName;
    //
    // Refresh the cach
    DOMDisableDisplay (pDoc->m_hNode, NULL);
    UpdateDOMData (
        pDoc->m_hNode, 
        OT_DBEVENT, 
        1,
        parentstrings,
        FALSE,
        FALSE);
    DOMUpdateDisplayData (
        pDoc->m_hNode, 
        OT_DBEVENT, 
        1,
        parentstrings,
        FALSE,
        ACT_ADD_OBJECT,
        0L,
        0);
    //
    // Refresh data of this Window
    OnRefresh ();
}


void CuDlgDBEventPane01::OnRefresh()
{
    TRACE0 ("CuDlgDBEventPane01::OnRefresh() ...\n");
    int     nCount, i, index =CB_ERR;
    CString strItem;
    CView* pView = (CView*)GetParent();
    ASSERT (pView);
    CDbeventDoc* pDoc = (CDbeventDoc*)pView->GetDocument();
    ASSERT (pDoc);
    //
    // Refresh data of this Window
    // Refresh the DB Events of One Database.
    CString strNone;
    strNone.LoadString (IDS_DATABASE_NONE);
    if (pDoc->m_strDBName == strNone || pDoc->m_strDBName == "")
        return;
    CCheckListBox list;
    list.Create (WS_CHILD|LBS_HASSTRINGS|LBS_OWNERDRAWFIXED, CRect (0,0, 10, 10), this, (UINT)-1);
    ASSERT (IsWindow (list.m_hWnd));
    InitializeDBEvent (m_strCurrentDB, &list);
    
    nCount = m_cListDBEvent.GetCount();
    for (i = 0; i < nCount; i++)
    {
        if (m_cListDBEvent.GetCheck(i) == 1)
        {
            m_cListDBEvent.GetText (i, strItem);
            index = list.FindStringExact (-1, strItem);
            if (index == LB_ERR)
            {
                CString msg;
                AfxFormatString1 (msg, IDS_DBECHECKED_HASBEEN_REMOVED, (LPCTSTR)strItem);
                BfxMessageBox (msg);
            }
            else
            {
                list.SetCheck (index, 1);
            }
        }
    }
    CleanListBox (&m_cListDBEvent);
    m_cListDBEvent.ResetContent ();
    nCount = list.GetCount();
    for (i = 0; i < nCount; i++)
    {
        list.GetText (i, strItem);
        index = m_cListDBEvent.AddString (strItem);
        if (index != LB_ERR)
        {
            LPTSTR lpszOwner = (LPTSTR)list.GetItemData (i);
            m_cListDBEvent.SetItemData (i, (DWORD)lpszOwner);
            if (list.GetCheck (i) == 1)
                m_cListDBEvent.SetCheck (index, 1);
        }
    }
}


void CuDlgDBEventPane01::InitializeDBEvent (CString& str, CCheckListBox* pList)
{
    CWinApp* app = NULL;
    CView* pView = (CView*)GetParent();
    ASSERT (pView);
    CDbeventDoc* pDoc = (CDbeventDoc*)pView->GetDocument();
    ASSERT (pDoc);

    int      cur, ires;
    TCHAR    buf [MAXOBJECTNAME];
    TCHAR    buffilter [MAXOBJECTNAME];
    LPUCHAR  parentstrings [MAXPLEVEL];
    memset (buf, 0, sizeof (buf));
    memset (buffilter, 0, sizeof (buffilter));
    if (!pList)
    {
        CleanListBox (&m_cListDBEvent);
        m_cListDBEvent.ResetContent();
    }
    CString strNone;
    strNone.LoadString (IDS_DATABASE_NONE);
    if (str == strNone)
    {
        m_strCurrentDB = "";
        return;
    }
    parentstrings [0] = (LPUCHAR)(LPCTSTR)str;
    parentstrings [1] = NULL;
    app = AfxGetApp();
    app->DoWaitCursor (1);
    ires = DOMGetFirstObject (
        pDoc->m_hNode, 
        OT_DBEVENT, 
        1, 
        parentstrings, 
        pDoc->m_bSysDBEvent, 
        NULL, 
        (LPUCHAR)buf, 
        (LPUCHAR)buffilter, 
        NULL);
    while (ires == RES_SUCCESS)
    {
        if (pList)
            cur = pList->AddString ((LPCTSTR)buf);
        else
            cur = m_cListDBEvent.AddString ((LPCTSTR)buf);
        if (cur != LB_ERR)
        {
            LPTSTR lpszOwner;
            lpszOwner = new TCHAR [lstrlen (buffilter)+1];
            lstrcpy (lpszOwner, (LPTSTR)buffilter);
            if (pList)
                pList->SetItemData (cur, (DWORD)lpszOwner);
            else
                m_cListDBEvent.SetItemData (cur, (DWORD)lpszOwner);
        }
        ires  = DOMGetNextObject ((LPUCHAR)buf, (LPUCHAR)buffilter, NULL);
    }
    app->DoWaitCursor (-1);
    m_strCurrentDB = str;
}


void CuDlgDBEventPane01::OnDestroy() 
{
    UnregisterDBEvents();
    CleanListBox (&m_cListDBEvent);
    CDialog::OnDestroy();
}

void CuDlgDBEventPane01::OnCheckChange()
{
    CDbeventFrame*  pFrame;
    CView*          pView;
    CDbeventDoc*    pDoc;
    int     nCheck, index   = LB_ERR;
    CString strItem;
    CString strTime;
    LPTSTR  lpszOwner;

    CTime time = CTime::GetCurrentTime();
    strTime = time.Format ("%c");
    index = m_cListDBEvent.GetCaretIndex();
    if (index == LB_ERR)
        return;
    pView = (CView*)GetParent();
    ASSERT (pView);
    pDoc  = (CDbeventDoc*)pView->GetDocument();
    ASSERT (pDoc);
    CWnd* pSplitter = pView->GetParent();
    ASSERT (pSplitter);
    pFrame = (CDbeventFrame*)pSplitter->GetParent();
    ASSERT (pFrame);
    if (!pDoc->m_bDBInit)
    {
        BfxMessageBox (VDBA_MfcResourceString(IDS_E_DB_EVENT_REGISTER));//"DBEvent Initialization Failure: Unable to register."
        nCheck = m_cListDBEvent.GetCheck (index);
        if (nCheck == 1)
            m_cListDBEvent.SetCheck (index, 0); // Stay unckecked
        return;
    }
    CuDlgDBEventPane02* pRaisePanel = pFrame->GetPaneRaisedDBEvent();
    ASSERT (pRaisePanel);
    m_cListDBEvent.GetText (index, strItem);
    lpszOwner = (LPTSTR)m_cListDBEvent.GetItemData (index);
    nCheck = m_cListDBEvent.GetCheck (index);
    if (nCheck == 1)
    {
        if (DBETraceRegister (pDoc->m_nHandle, (LPUCHAR)(LPCTSTR)strItem, (LPUCHAR)lpszOwner) != RES_SUCCESS)
        {
            BfxMessageBox (VDBA_MfcResourceString(IDS_E_REGISTERING_DB_EVENT));//"Error while Registering DB Event"
            m_cListDBEvent.SetCheck (index, 0); // Stay unckecked
            return;
        }
        CString st1;
        st1.Format (IDS_DBEVENT_CHECK, (LPCTSTR)strItem);//"<Check %s>"
        pRaisePanel->IncomingDBEvent (pDoc, "?", strTime, st1, "", "");
    }
    else
    if (nCheck == 0)
    {
        CString st1;
        int ans = DBETraceUnRegister (pDoc->m_nHandle, (LPUCHAR)(LPCTSTR)strItem, (LPUCHAR)lpszOwner);
        if (ans != RES_SUCCESS && ans != RES_MOREDBEVENT)
        {
            BfxMessageBox (VDBA_MfcResourceString(IDS_E_UNREGISTERING_DB_EVENT));//"Error while unregistering DB Event"
            m_cListDBEvent.SetCheck (index, 1); // Stay ckecked
            st1.Format (IDS_E_UNCHECK, (LPCTSTR)strItem);//"<ERROR in Uncheck %s>"
            pRaisePanel->IncomingDBEvent (pDoc, "?", strTime, st1, "", "");
            return;
        }
        st1.Format (IDS_I_UNCHECK, (LPCTSTR)strItem);//"<Uncheck %s>"
        pRaisePanel->IncomingDBEvent (pDoc, "?", strTime, st1, "", "");
    }
}

void CuDlgDBEventPane01::UpdateControl()
{
    CView*  pView = (CView*)GetParent();
    ASSERT  (pView);
    CDbeventDoc* pDoc = (CDbeventDoc*)pView->GetDocument();
    ASSERT  (pDoc);
    CWnd*   pSplitter = pView->GetParent();
    ASSERT (pSplitter);
    CDbeventFrame* pFrame = (CDbeventFrame*)pSplitter->GetParent();
    ASSERT (pFrame);
    //
    // Remark:
    // InitializeDBEvent() is called on the CBN_SELCHANGE of Database ComboBox.
    CString strNone;
    if (strNone.LoadString (IDS_DATABASE_NONE) == 0)
        strNone = "<None>";
    if (pDoc->m_strDBName == "" || pDoc->m_strDBName == strNone)
        return;
    
    // Load ...
    // Initialize the new Database.
    if (DBETraceInit (pDoc->m_hNode, (LPUCHAR)(LPCTSTR)pDoc->m_strDBName , pFrame->m_hWnd, &(pDoc->m_nHandle)) == RES_SUCCESS)
    {
        pDoc->m_bDBInit = TRUE;
        m_strCurrentDB  = pDoc->m_strDBName;
    }
    else
    {
        pDoc->m_bDBInit = FALSE;
        pDoc->m_nHandle = -1;
        BfxMessageBox (VDBA_MfcResourceString(IDS_E_INITIALIZE_DB_EVENT));//"Error while initializing the Database for DB Event Registration"
    }
    if (pDoc->m_bDBInit)
    {
        int index;
        CTypedPtrList<CObList, CuDataRegisteredDBevent*>& listDBEvent = pDoc->m_listRegisteredDBEvent;

        POSITION pos = listDBEvent.GetHeadPosition();
        while  (pos != NULL)
        {
            CuDataRegisteredDBevent* dbe = listDBEvent.GetNext (pos);

            index = m_cListDBEvent.AddString (dbe->m_strDBEvent);
            if (index != LB_ERR)
            {
                LPTSTR lpszOwner = new TCHAR [dbe->m_strOwner.GetLength() +1];
                lstrcpy (lpszOwner, (LPTSTR)(LPCTSTR)dbe->m_strOwner);
                m_cListDBEvent.SetItemData (index, (DWORD)lpszOwner);
                if (dbe->m_bRegistered)
                {
                    if (DBETraceRegister (pDoc->m_nHandle, (LPUCHAR)(LPCTSTR)dbe->m_strDBEvent, (LPUCHAR)(LPCTSTR)dbe->m_strOwner) != RES_SUCCESS)
			            BfxMessageBox (VDBA_MfcResourceString(IDS_E_REGISTERING_DB_EVENT));//"Error while Registering DB Event"
                    else
                        m_cListDBEvent.SetCheck (index, 1);
                }
            }
        }
    }
}

void CuDlgDBEventPane01::UnregisterDBEvents()
{
    CDbeventDoc* pDoc;
    CView* pView = (CView*)GetParent();
    ASSERT (pView);
    pDoc  = (CDbeventDoc*)pView->GetDocument();
    ASSERT (pDoc);
    if (!pDoc)
        return;
    //
    // Unregister the DB Events:
    int     nCount, i;
    CString strItem;
    nCount = m_cListDBEvent.GetCount();
    for (i=0; i<nCount; i++)
    {
        m_cListDBEvent.GetText (i, strItem);
        LPTSTR lpszOwner = (LPTSTR)m_cListDBEvent.GetItemData (i);
        if (m_cListDBEvent.GetCheck (i) == 1)
		{
            DBETraceUnRegister (pDoc->m_nHandle, (LPUCHAR)(LPCTSTR)strItem, (LPUCHAR)lpszOwner);
			m_cListDBEvent.SetCheck (i, 0);
		}
    }
    CString strNone;
    if (pDoc && m_strCurrentDB != "" && m_strCurrentDB != strNone)
    {
        // 
        // Reset the Database ...
        if (pDoc->m_bDBInit && pDoc->m_nHandle != -1)
        {
            DBETraceTerminate (pDoc->m_nHandle);
        }
    }

}
