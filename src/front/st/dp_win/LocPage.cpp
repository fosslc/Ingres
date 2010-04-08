/*
**
**  Name: LocPage.cpp
**
**  Description:
**	This file contains the necessary routines to display the location
**	property page.
**
**  History:
**	10-jul-1999 (somsa01)
**	    Created.
*/

#include "stdafx.h"
#include "dp.h"
#include "LocPage.h"
#include "AddFileName.h"
#include "AddStructName.h"
#include <lmcons.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* offsets for first and other columns */
#define OFFSET_FIRST	2
#define OFFSET_OTHER	6

/*
** LocPage property page
*/
IMPLEMENT_DYNCREATE(LocPage, CPropertyPage)

LocPage::LocPage() : CPropertyPage(LocPage::IDD)
{
    //{{AFX_DATA_INIT(LocPage)
	    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

LocPage::~LocPage()
{
}

LocPage::LocPage(CPropertySheet *ps) : CPropertyPage(LocPage::IDD)
{
    m_propertysheet = ps;
    m_psp.dwFlags |= (PSP_HASHELP);
}

void LocPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(LocPage)
    DDX_Control(pDX, IDC_LIST_TABLES, m_TableList);
    DDX_Control(pDX, IDC_LIST_FILES, m_FileList);
    DDX_Control(pDX, IDC_IMAGE, m_image);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(LocPage, CPropertyPage)
    //{{AFX_MSG_MAP(LocPage)
    ON_BN_CLICKED(IDC_RADIO_CONNECT, OnRadioConnect)
    ON_BN_CLICKED(IDC_RADIO_FILE, OnRadioFile)
    ON_NOTIFY(LVN_KEYDOWN, IDC_LIST_FILES, OnKeydownListFiles)
    ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
    ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
    ON_NOTIFY(NM_CLICK, IDC_LIST_FILES, OnClickListFiles)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST_FILES, OnDblclkListFiles)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_FILES, OnItemchangedListFiles)
    ON_WM_DRAWITEM()
    ON_BN_CLICKED(IDC_BUTTON_ADD2, OnButtonAdd2)
    ON_BN_CLICKED(IDC_BUTTON_DELETE2, OnButtonDelete2)
    ON_NOTIFY(NM_CLICK, IDC_LIST_TABLES, OnClickListTables)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST_TABLES, OnDblclkListTables)
    ON_NOTIFY(LVN_KEYDOWN, IDC_LIST_TABLES, OnKeydownListTables)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** LocPage message handlers
*/
BOOL
LocPage::OnSetActive() 
{
    m_propertysheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
    return CPropertyPage::OnSetActive();
}

void
LocPage::OnRadioConnect() 
{
    CButton	*pRadioConnect = (CButton *)GetDlgItem(IDC_RADIO_CONNECT);
    BOOL	Enabled = pRadioConnect->GetCheck();
    CEdit	*pEditDB = (CEdit *)GetDlgItem(IDC_EDIT_DBNAME);
    CEdit	*pEditConn = (CEdit *)GetDlgItem(IDC_EDIT_USERID);
    CButton	*pDelete1 = (CButton *)GetDlgItem(IDC_BUTTON_DELETE);
    CButton	*pDelete2 = (CButton *)GetDlgItem(IDC_BUTTON_DELETE2);
    CButton	*pAdd1 = (CButton *)GetDlgItem(IDC_BUTTON_ADD);
    CButton	*pAdd2 = (CButton *)GetDlgItem(IDC_BUTTON_ADD2);
  
    /* Enable the database stuff */
    pEditDB->EnableWindow(Enabled);
    pEditConn->EnableWindow(Enabled);
    pAdd2->EnableWindow(Enabled);
    pDelete2->EnableWindow(Enabled);
    m_TableList.EnableWindow(Enabled);

    /* Disable the file stuff */
    pDelete1->EnableWindow(!Enabled);
    pAdd1->EnableWindow(!Enabled);
    m_FileList.EnableWindow(!Enabled);
    FileInput = FALSE;
}

void
LocPage::OnRadioFile() 
{
    CButton	*pRadioFile = (CButton *)GetDlgItem(IDC_RADIO_FILE);
    BOOL	Enabled = pRadioFile->GetCheck();
    CButton	*pDelete1 = (CButton *)GetDlgItem(IDC_BUTTON_DELETE);
    CButton	*pDelete2 = (CButton *)GetDlgItem(IDC_BUTTON_DELETE2);
    CEdit	*pEditDB = (CEdit *)GetDlgItem(IDC_EDIT_DBNAME);
    CEdit	*pEditConn = (CEdit *)GetDlgItem(IDC_EDIT_USERID);
    CButton	*pAdd1 = (CButton *)GetDlgItem(IDC_BUTTON_ADD);
    CButton	*pAdd2 = (CButton *)GetDlgItem(IDC_BUTTON_ADD2);

    /* Enable the file stuff */
    m_FileList.EnableWindow(Enabled);
    pDelete1->EnableWindow(Enabled);
    pAdd1->EnableWindow(Enabled);
    FileInput = TRUE;

    /* Disable the database stuff */
    m_TableList.EnableWindow(!Enabled);
    pEditDB->EnableWindow(!Enabled);
    pEditConn->EnableWindow(!Enabled);
    pDelete2->EnableWindow(!Enabled);
    pAdd2->EnableWindow(!Enabled);
}

BOOL
LocPage::OnInitDialog() 
{
    CButton	*pRadioConnect = (CButton *)GetDlgItem(IDC_RADIO_CONNECT);
    DWORD	size = UNLEN + 1;

    CPropertyPage::OnInitDialog();

    Message.LoadString(IDS_LOC_HEADING);
    SetDlgItemText(IDC_LOC_HEADING, Message);
    Message.LoadString(IDS_RADIO_CONNECT);
    SetDlgItemText(IDC_RADIO_CONNECT, Message);
    Message.LoadString(IDS_GROUP_CONNECT);
    SetDlgItemText(IDC_GROUP_CONNECT, Message);
    Message.LoadString(IDS_DB_NAME);
    SetDlgItemText(IDC_DB_NAME, Message);
    Message.LoadString(IDS_CONN_ID);
    SetDlgItemText(IDC_CONN_ID, Message);
    Message.LoadString(IDS_BUTON_ADD);
    SetDlgItemText(IDC_BUTTON_ADD, Message);
    SetDlgItemText(IDC_BUTTON_ADD2, Message);
    Message.LoadString(IDS_BUTTON_DELETE);
    SetDlgItemText(IDC_BUTTON_DELETE, Message);
    SetDlgItemText(IDC_BUTTON_DELETE2, Message);
    Message.LoadString(IDS_RADIO_FILE);
    SetDlgItemText(IDC_RADIO_FILE, Message);
    Message.LoadString(IDS_GROUP_FILEINFO);
    SetDlgItemText(IDC_GROUP_FILEINFO, Message);


    GetUserName(UserID, &size);
    SetDlgItemText(IDC_EDIT_USERID, UserID);
    pRadioConnect->SetCheck(TRUE);
    FileInput = FALSE;
    LocPage::OnRadioConnect();

    InitListViewControl();
    InitListViewControl2();
    FileIndex = -1;
	
    return TRUE;
}

LRESULT
LocPage::OnWizardNext() 
{
    CButton	*pRadioConnect = (CButton *)GetDlgItem(IDC_RADIO_CONNECT);
    CString	DbName;
    int		i, numfiles;

    if (pRadioConnect->GetCheck())
    {
	GetDlgItemText(IDC_EDIT_DBNAME, DbName);
	GetDlgItemText(IDC_EDIT_USERID, UserID, UNLEN + 1);

	if (DbName == "")
	{
	    Message.LoadString(IDS_NO_DBNAME);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	    return -1;
	}
	else if (!strcmp(UserID, ""))
	{
	    Message.LoadString(IDS_NEED_CONNUSER);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	    return -1;
	}

	numfiles = m_TableList.GetItemCount();
	if (numfiles == 0)
	{
	    Message.LoadString(IDS_NEED_TABLES);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	    return -1;
	}

	/* Put together the connection part of the command line */
	ConnectLine = CString("-db") + DbName;
	for (i=0; i<numfiles; i++)
	{
	    if (m_TableList.GetItemText(i, 1) == "TABLE")
		ConnectLine += CString(" -tb");
	    else
		ConnectLine += CString(" -ix");
	    ConnectLine += m_TableList.GetItemText(i, 0);
	}
	ConnectLine += CString(" -to") + CString(UserID);
    }
    else
    {
	int i, numfiles = m_FileList.GetItemCount();

	if (numfiles == 0)
	{
	    Message.LoadString(IDS_NEED_FILES);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	    return -1;
	}

	/*
	** Get the list of files and add them to the connection part
	** of the command line.
	*/
	ConnectLine = "";
	for (i=0; i<numfiles; i++)
	{
	    if (m_FileList.GetItemText(i, 1) == "TABLE")
		ConnectLine += CString(" -ft");
	    else
		ConnectLine += CString(" -fi");
	    ConnectLine += m_FileList.GetItemText(i, 0);
	}

    }

    return CPropertyPage::OnWizardNext();
}

void
LocPage::InitListViewControl()
{
    LV_COLUMN  col;
    CString ColumnHeader;
    RECT rc;

    m_FileList.GetClientRect(&rc);

    ColumnHeader = "File Name";
    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    col.cx = rc.right - m_FileList.GetStringWidth(ColumnHeader)-10;
    col.fmt = LVCFMT_LEFT;
    col.iSubItem = 0;
    col.pszText = ColumnHeader.GetBuffer(256);
    m_FileList.InsertColumn(0, &col);

    ColumnHeader = "File Type";
    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    col.cx = m_FileList.GetStringWidth(ColumnHeader)+14;
    col.fmt = LVCFMT_LEFT;
    col.iSubItem = 1;
    col.pszText = ColumnHeader.GetBuffer(256);
    m_FileList.InsertColumn(1, &col);
}

void
LocPage::InitListViewControl2()
{
    LV_COLUMN  col;
    CString ColumnHeader;
    RECT rc;

    m_TableList.GetClientRect(&rc);

    ColumnHeader = "Structure Name";
    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    col.cx = rc.right - m_TableList.GetStringWidth(ColumnHeader)-10;
    col.fmt = LVCFMT_LEFT;
    col.iSubItem = 0;
    col.pszText = ColumnHeader.GetBuffer(256);
    m_TableList.InsertColumn(0, &col);

    ColumnHeader = "Structure Type";
    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    col.cx = m_TableList.GetStringWidth(ColumnHeader)+14;
    col.fmt = LVCFMT_LEFT;
    col.iSubItem = 1;
    col.pszText = ColumnHeader.GetBuffer(256);
    m_TableList.InsertColumn(1, &col);
}

void
LocPage::ListViewInsert(CString value)
{ 
    LV_ITEM lvitem;

    lvitem.iItem = m_FileList.GetItemCount();
    lvitem.mask = LVIF_TEXT;
    lvitem.iSubItem = 0;
    lvitem.pszText = value.GetBuffer(0);
    lvitem.iItem = m_FileList.InsertItem(&lvitem);

    lvitem.iSubItem = 1;
    lvitem.pszText = "TABLE";
    m_FileList.SetItem(&lvitem);
}

void
LocPage::ListViewInsert2(CString value)
{ 
    LV_ITEM lvitem;

    lvitem.iItem = m_TableList.GetItemCount();
    lvitem.mask = LVIF_TEXT;
    lvitem.iSubItem = 0;
    lvitem.pszText = value.GetBuffer(0);
    lvitem.iItem = m_TableList.InsertItem(&lvitem);

    lvitem.iSubItem = 1;
    lvitem.pszText = "TABLE";
    m_TableList.SetItem(&lvitem);
}

void
LocPage::OnKeydownListFiles(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;

    if ((pLVKeyDow->wVKey == VK_DELETE) && (FileIndex != -1))
    {
	m_FileList.DeleteItem(FileIndex);
	FileIndex = -1;
    }

    *pResult = 0;
}

void
LocPage::OnButtonDelete() 
{
    if (FileIndex != -1)
    {
        m_FileList.DeleteItem(FileIndex);
	FileIndex = -1;
    }
}

void
LocPage::OnButtonAdd() 
{
    AddFileName afn_Dlg;

    if (afn_Dlg.DoModal() == IDOK)
	ListViewInsert(DestFile);
    DestFile = "";
}

void
LocPage::OnClickListFiles(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_HITTESTINFO FAR	pinfo;
    DWORD		dwpos;
    POINT		point;

    /* Find out where the cursor was */
    dwpos = GetMessagePos();

    point.x = LOWORD(dwpos);
    point.y = HIWORD(dwpos);

    ::MapWindowPoints(HWND_DESKTOP, m_FileList.m_hWnd, &point, 1);
    pinfo.pt = point;

    m_FileList.HitTest(&pinfo);
    if (pinfo.flags & LVHT_ONITEM)
	FileIndex = pinfo.iItem;
    *pResult = 0;
}


void
LocPage::OnDblclkListFiles(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_HITTESTINFO FAR	pinfo;
    DWORD		dwpos;
    POINT		point;
    int			rc, colnum;

    /* Find out where the cursor was */
    dwpos = GetMessagePos();

    point.x = LOWORD(dwpos);
    point.y = HIWORD(dwpos);

    ::MapWindowPoints(HWND_DESKTOP, m_FileList.m_hWnd, &point, 1);
    pinfo.pt = point;

    CPoint classpt(point);
    rc = m_FileList.HitTestEx(classpt, &colnum);
    if (rc != -1)
    {
	if (colnum == 0)
	{
	    OldItemValue = m_FileList.GetItemText(rc, 0);
	    m_FileList.EditSubLabel(rc, 0);
	}
	else
	{
	    CString FileType = m_FileList.GetItemText(rc, 1);

	    if (FileType == "TABLE")
		m_FileList.SetItemText(rc, 1, "INDEX");
	    else
		m_FileList.SetItemText(rc, 1, "TABLE");
	}

	FileIndex = rc;
    }
	
    *pResult = 0;
}

void
LocPage::OnItemchangedListFiles(NMHDR* pNMHDR, LRESULT* pResult) 
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    
    if (pNMListView->uChanged & LVIF_TEXT)
    {
	CString NewFileName =
		m_FileList.GetItemText(pNMListView->iItem, 0);

	if (NewFileName == "")
	{
	    FileIndex = pNMListView->iItem;
	    LocPage::OnButtonDelete();
	    OldItemValue = "";
	}
	else if (GetFileAttributes(NewFileName) == -1)
	{
	    Message.LoadString(IDS_BAD_FILEEDIT);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	    m_FileList.EditSubLabel(pNMListView->iItem, 0);
	}
	else
	    OldItemValue = "";
    }

    *pResult = 0;
}

LPCTSTR
LocPage::MakeShortString(CDC* pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset)
{
    static const _TCHAR	szThreeDots[]=_T("...");
    int			nStringLen=lstrlen(lpszLong);
    static _TCHAR	szShort[MAX_PATH];
    int			nAddLen;

    if (nStringLen==0 ||
	pDC->GetTextExtent(lpszLong,nStringLen).cx+nOffset<=nColumnLen)
	return(lpszLong);

    lstrcpy(szShort,lpszLong);
    nAddLen=pDC->GetTextExtent(szThreeDots,sizeof(szThreeDots)).cx;

    for(int i=nStringLen-1; i>0; i--)
    {
	szShort[i]=0;
	if(pDC->GetTextExtent(szShort,i).cx+nOffset+nAddLen<=nColumnLen)
	    break;
    }

    lstrcat(szShort,szThreeDots);

    return(szShort);
}

void
LocPage::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
    int			m_cxClient=0, nColumn, m_cxStateImageOffset=0, nRetLen;
    BOOL		m_bClientWidthSel=TRUE;
    COLORREF		m_inactive=::GetSysColor(COLOR_INACTIVEBORDER);
    COLORREF		m_clrTextBk=::GetSysColor(COLOR_WINDOW);
    COLORREF		m_clrBkgnd=::GetSysColor(COLOR_WINDOW);
    CListCtrl		&ListCtrl=
			((lpDrawItemStruct->CtlID == IDC_LIST_FILES) ?
			m_FileList : m_TableList);
    CDC			*pDC=CDC::FromHandle(lpDrawItemStruct->hDC);
    CRect		rcItem(lpDrawItemStruct->rcItem);
    UINT		uiFlags=ILD_TRANSPARENT, nStateImageMask, nJustify;
    CImageList		*pImageList;
    int			nItem=lpDrawItemStruct->itemID;
    BOOL		bFocus=(GetFocus()==this);
    COLORREF		clrTextSave, clrBkSave;
    COLORREF		clrImage=m_clrBkgnd;
    static _TCHAR	szBuff[MAX_PATH];
    LPCTSTR		pszText;
    LV_ITEM		lvi;
    LV_COLUMN		lvc;
    BOOL		bSelected;
    CRect		rcAllLabels, rcLabel, rcIcon, rcRect;

    /* get item data */
    lvi.mask=LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvi.iItem=nItem;
    lvi.iSubItem=0;
    lvi.pszText=szBuff;
    lvi.cchTextMax=sizeof(szBuff);
    lvi.stateMask=0xFFFF;	/* get all state flags */
    ListCtrl.GetItem(&lvi);

    bSelected=(lvi.state & LVIS_SELECTED);

    /* set colors if item is selected */
    ListCtrl.GetItemRect(nItem,rcAllLabels,LVIR_BOUNDS);
    ListCtrl.GetItemRect(nItem,rcLabel,LVIR_LABEL);
    rcAllLabels.left=rcLabel.left - OFFSET_FIRST;
    if (m_bClientWidthSel && rcAllLabels.right<m_cxClient)
	rcAllLabels.right=m_cxClient;

    if(bSelected && ListCtrl.IsWindowEnabled())
    {
	clrTextSave=pDC->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
	clrBkSave=pDC->SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));

	CBrush brush(::GetSysColor(COLOR_HIGHLIGHT));
	pDC->FillRect(rcAllLabels, &brush);
    }
    else
    {
	CBrush brush1(m_clrTextBk), brush2(m_inactive);

	if (ListCtrl.IsWindowEnabled())
	    pDC->FillRect(rcAllLabels, &brush1);
	else
	    pDC->FillRect(rcAllLabels, &brush2);
    }

    /* set color and mask for the icon */
    if(lvi.state & LVIS_CUT)
    {
	clrImage=m_clrBkgnd;
	uiFlags|=ILD_BLEND50;
    }
    else if(bSelected && ListCtrl.IsWindowEnabled())
    {
	clrImage=::GetSysColor(COLOR_HIGHLIGHT);
	uiFlags|=ILD_BLEND50;
    }

    /* draw state icon */
    nStateImageMask=lvi.state & LVIS_STATEIMAGEMASK;
    if (nStateImageMask)
    {
	int nImage=(nStateImageMask>>12)-1;

	pImageList=ListCtrl.GetImageList(LVSIL_STATE);
	if(pImageList)
	    pImageList->Draw(pDC,nImage,CPoint(rcItem.left,rcItem.top),
			ILD_TRANSPARENT);
    }

    /* draw normal and overlay icon */
    ListCtrl.GetItemRect(nItem,rcIcon,LVIR_ICON);

    pImageList=ListCtrl.GetImageList(LVSIL_SMALL);
    if (pImageList)
    {
	UINT nOvlImageMask=lvi.state & LVIS_OVERLAYMASK;

	if(rcItem.left<rcItem.right-1)
	    ImageList_DrawEx(pImageList->m_hImageList,lvi.iImage,
			     pDC->m_hDC,rcIcon.left,rcIcon.top,16,16,
			     m_clrBkgnd,clrImage,uiFlags | nOvlImageMask);
    }

    /* draw item label */
    ListCtrl.GetItemRect(nItem,rcItem,LVIR_LABEL);
    rcItem.right-=m_cxStateImageOffset;

    pszText=MakeShortString(pDC,szBuff,rcItem.right-rcItem.left,
			    2*OFFSET_FIRST);

    rcLabel=rcItem;
    rcLabel.left+=OFFSET_FIRST;
    rcLabel.right-=OFFSET_FIRST;

    pDC->DrawText(pszText,-1,rcLabel,
		DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);

    /* draw labels for extra columns */
    lvc.mask=LVCF_FMT | LVCF_WIDTH;

    for (nColumn=1; ListCtrl.GetColumn(nColumn,&lvc); nColumn++)
    {
	rcItem.left=rcItem.right;
	rcItem.right+=lvc.cx;

	nRetLen=ListCtrl.GetItemText(nItem,nColumn,szBuff,sizeof(szBuff));
	if(nRetLen==0)
	    continue;

	pszText=MakeShortString(pDC,szBuff,rcItem.right-rcItem.left,
				2*OFFSET_OTHER);

	nJustify=DT_LEFT;

	if (pszText==szBuff)
	{
	    switch (lvc.fmt & LVCFMT_JUSTIFYMASK)
	    {
		case LVCFMT_RIGHT:
		    nJustify=DT_RIGHT;
		    break;

		case LVCFMT_CENTER:
		    nJustify=DT_CENTER;
		    break;

		default:
		    break;
	    }
	}

	rcLabel=rcItem;
	rcLabel.left+=OFFSET_OTHER;
	rcLabel.right-=OFFSET_OTHER;

	pDC->DrawText(pszText,-1,rcLabel,
		nJustify|DT_SINGLELINE|DT_NOPREFIX|DT_NOCLIP|DT_VCENTER);

        /* Draw a line separating each column... */
        CRect rcRect=rcItem;
        rcRect.right = rcRect.left;
        rcRect.left -= 1;
        
	pDC->FillRect(rcRect, 
		CBrush::FromHandle(GetSysColorBrush(COLOR_INACTIVEBORDER)));
    }
    rcRect = rcItem;
    rcRect.left = rcItem.right - 1;
    pDC->FillRect(rcRect, 
		CBrush::FromHandle(GetSysColorBrush(COLOR_INACTIVEBORDER)));

    /* Draw horizontal lines between each row ... */
    rcRect = rcItem;
    rcRect.top = rcRect.bottom - 1;
    rcRect.left = 0;
    pDC->FillRect(rcRect, 
		CBrush::FromHandle(GetSysColorBrush(COLOR_INACTIVEBORDER)));

    /* draw focus rectangle if item has focus */
    if (lvi.state & LVIS_FOCUSED && bFocus)
	pDC->DrawFocusRect(rcAllLabels);

    /* set original colors if item was selected */
    if (bSelected && ListCtrl.IsWindowEnabled())
    {
	pDC->SetTextColor(clrTextSave);
	pDC->SetBkColor(clrBkSave);
    }
}

void
LocPage::OnButtonAdd2() 
{
    AddStructName asn_Dlg;

    if (asn_Dlg.DoModal() == IDOK)
	ListViewInsert2(StructName);
    StructName = "";	
}

void
LocPage::OnButtonDelete2() 
{
    if (TableIndex != -1)
    {
        m_TableList.DeleteItem(TableIndex);
	TableIndex = -1;
    }	
}

void
LocPage::OnClickListTables(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_HITTESTINFO FAR	pinfo;
    DWORD		dwpos;
    POINT		point;

    /* Find out where the cursor was */
    dwpos = GetMessagePos();

    point.x = LOWORD(dwpos);
    point.y = HIWORD(dwpos);

    ::MapWindowPoints(HWND_DESKTOP, m_TableList.m_hWnd, &point, 1);
    pinfo.pt = point;

    m_TableList.HitTest(&pinfo);
    if (pinfo.flags & LVHT_ONITEM)
	TableIndex = pinfo.iItem;
    *pResult = 0;
}

void
LocPage::OnDblclkListTables(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_HITTESTINFO FAR	pinfo;
    DWORD		dwpos;
    POINT		point;
    int			rc, colnum;

    /* Find out where the cursor was */
    dwpos = GetMessagePos();

    point.x = LOWORD(dwpos);
    point.y = HIWORD(dwpos);

    ::MapWindowPoints(HWND_DESKTOP, m_TableList.m_hWnd, &point, 1);
    pinfo.pt = point;

    CPoint classpt(point);
    rc = m_TableList.HitTestEx(classpt, &colnum);
    if (rc != -1)
    {
	if (colnum == 0)
	{
	    OldItemValue = m_TableList.GetItemText(rc, 0);
	    m_TableList.EditSubLabel(rc, 0);
	}
	else
	{
	    CString TableType = m_TableList.GetItemText(rc, 1);

	    if (TableType == "TABLE")
		m_TableList.SetItemText(rc, 1, "INDEX");
	    else
		m_TableList.SetItemText(rc, 1, "TABLE");
	}

	TableIndex = rc;
    }
	
    *pResult = 0;
}

void
LocPage::OnKeydownListTables(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;

    if ((pLVKeyDow->wVKey == VK_DELETE) && (TableIndex != -1))
    {
	m_TableList.DeleteItem(TableIndex);
	TableIndex = -1;
    }

    *pResult = 0;
}
