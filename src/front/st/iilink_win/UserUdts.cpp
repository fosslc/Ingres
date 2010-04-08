/*
**
**  Name: UserUdts.cpp
**
**  Description:
**	This file contains the routines to implement the file list selection
**	property page.
**
**  History:
**	23-apr-1999 (somsa01)
**	    Created.
**	30-apr-1999 (somsa01)
**	    Instead of using GetFirstSelectedItemPosition() and
**	    GetNextSelectedItem() (which are ONLY for MSVC 6.0), use
**	    GetNextItem().
**	28-Sep-2000 (noifr01)
**	    (bug 99242) cleanup for dbcs compliance
**	01-may-2001 (somsa01)
**	    Removed unneeded "i" variable from MakeShortString().
**	30-jul-2001 (somsa01)
**	    Fixed up compiler warnings.
*/

#include "stdafx.h"
#include "iilink.h"
#include "UserUdts.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* offsets for first and other columns */
#define OFFSET_FIRST	2
#define OFFSET_OTHER	6

#define NOCHECK 1
#define CHECK 2

/*
** UserUdts property page
*/
IMPLEMENT_DYNCREATE(UserUdts, CPropertyPage)

UserUdts::UserUdts() : CPropertyPage(UserUdts::IDD)
{
    //{{AFX_DATA_INIT(UserUdts)
	// NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

UserUdts::UserUdts(CPropertySheet *ps) : CPropertyPage(UserUdts::IDD)
{
    m_propertysheet = ps;
    m_psp.dwFlags &= ~(PSP_HASHELP);
	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}

UserUdts::~UserUdts()
{
}

void UserUdts::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(UserUdts)
    DDX_Control(pDX, IDC_LIST_FILES, m_FileSelectList);
    DDX_Control(pDX, IDC_IMAGE, m_image);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(UserUdts, CPropertyPage)
    //{{AFX_MSG_MAP(UserUdts)
    ON_NOTIFY(NM_CLICK, IDC_LIST_FILES, OnClickListFiles)
    ON_BN_CLICKED(IDC_BUTTON_SELECT, OnButtonSelect)
    ON_BN_CLICKED(IDC_BUTTON_DESELECT, OnButtonDeselect)
    ON_WM_DRAWITEM()
	ON_BN_CLICKED(IDC_BUTTON_VIEWSRC, OnButtonViewsrc)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LIST_FILES, OnKeydownListFiles)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_FILES, OnDblclkListFiles)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** UserUdts message handlers
*/
BOOL
UserUdts::OnSetActive() 
{
    if (LocChanged)
    {
	if (m_FileSelectList.GetItemCount())
	    m_FileSelectList.DeleteAllItems();
	ListViewInsert();
	LocChanged = FALSE;
    }
    m_propertysheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
    return CPropertyPage::OnSetActive();
}

void
UserUdts::InitListViewControl() 
{
    LV_COLUMN  col;
    CString ColumnHeader;
    CHAR Message[256];
    BOOL b;
    RECT rc;

    m_StateImageList = new (CImageList);
    b = m_StateImageList->Create(IDB_LV_STATEIMAGE, 16, 1, RGB(255,0,0));
    m_FileSelectList.SetImageList(m_StateImageList, LVSIL_STATE);

    sprintf(Message, "");
    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    m_FileSelectList.GetClientRect(&rc);
    col.cx = rc.right;
    col.fmt = LVCFMT_LEFT;
    col.iSubItem = 0;
    col.pszText = Message;
    m_FileSelectList.InsertColumn(0, &col);
}

int
UserUdts::ListViewInsert()
{ 
    LV_ITEM lvitem;
    CFileFind fFind;
    int iItem = 0;
    CHAR Message[1024];
    CString FileName;

    sprintf(Message, "%s\\*.*", UserLoc.GetBuffer(1024));
    if (!fFind.FindFile(Message, 0))
	return(FALSE);

    while (fFind.FindNextFile())
    {
	strcpy(Message, fFind.GetFileName());
	if (_tcsstr(Message, ".c") || _tcsstr(Message, ".obj") ||
	    _tcsstr(Message, ".lib"))
	{
	    lvitem.iItem = iItem;
	    lvitem.mask = LVIF_TEXT;
	    lvitem.iSubItem = 0;
	    FileName = fFind.GetFileName();
	    lvitem.pszText = FileName.GetBuffer(1024);
	    lvitem.iItem = m_FileSelectList.InsertItem(&lvitem);
	    m_FileSelectList.SetItemState( iItem, INDEXTOSTATEIMAGEMASK(NOCHECK),
					   LVIS_STATEIMAGEMASK);
	    iItem++;
	}
    }

    fFind.Close();
    return(TRUE);
}

BOOL
UserUdts::OnInitDialog() 
{
    CString	Message;

    CPropertyPage::OnInitDialog();

    /*
    ** Load the dialog's strings.
    */
    Message.LoadString(IDS_USERUDT_TXT);
    SetDlgItemText(IDC_USERUDT_TXT, Message);
    Message.LoadString(IDS_BUTTON_SELECT_ALL);
    SetDlgItemText(IDC_BUTTON_SELECT, Message);
    Message.LoadString(IDS_BUTTON_DESELECT_ALL);
    SetDlgItemText(IDC_BUTTON_DESELECT, Message);
    Message.LoadString(IDS_BUTTON_VIEWSRC);
    SetDlgItemText(IDC_BUTTON_VIEWSRC, Message);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON_VIEWSRC), FALSE);
	
    InitListViewControl();
	
    /*
    ** return TRUE unless you set the focus to a control
    ** EXCEPTION: OCX Property Pages should return FALSE
    */
    return TRUE;
}

void
UserUdts::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
    int			m_cxClient=0, nColumn, m_cxStateImageOffset=0, nRetLen;
    BOOL		m_bClientWidthSel=TRUE;
    COLORREF		m_clrTextBk=::GetSysColor(COLOR_WINDOW);
    COLORREF		m_clrBkgnd=::GetSysColor(COLOR_WINDOW);
    CListCtrl		&ListCtrl=m_FileSelectList;
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
    rcAllLabels.left=rcLabel.left;
    if (m_bClientWidthSel && rcAllLabels.right<m_cxClient)
	rcAllLabels.right=m_cxClient;

    if(bSelected)
    {
	clrTextSave=pDC->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
	clrBkSave=pDC->SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));

	CBrush brush(::GetSysColor(COLOR_HIGHLIGHT));
	pDC->FillRect(rcAllLabels, &brush);
    }
    else
    {
	CBrush brush(m_clrTextBk);
	pDC->FillRect(rcAllLabels, &brush);
    }

    /* set color and mask for the icon */
    if(lvi.state & LVIS_CUT)
    {
	clrImage=m_clrBkgnd;
	uiFlags|=ILD_BLEND50;
    }
    else if(bSelected)
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
		CBrush::FromHandle((HBRUSH)GetStockObject(LTGRAY_BRUSH)));
        
    }

    /* draw focus rectangle if item has focus */
    if (lvi.state & LVIS_FOCUSED && bFocus)
	pDC->DrawFocusRect(rcAllLabels);

    /* set original colors if item was selected */
    if (bSelected)
    {
	pDC->SetTextColor(clrTextSave);
	pDC->SetBkColor(clrBkSave);
    }
}

LPCTSTR
UserUdts::MakeShortString(CDC* pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset)
{
    static const _TCHAR szThreeDots[]=_T("...");
    static _TCHAR szShort[MAX_PATH];
    int nStringLen=lstrlen(lpszLong), nAddLen;

    if (nStringLen==0 ||
	pDC->GetTextExtent(lpszLong,nStringLen).cx+nOffset<=nColumnLen)
	return(lpszLong);

    lstrcpy(szShort,lpszLong);
    nAddLen=pDC->GetTextExtent(szThreeDots,sizeof(szThreeDots)).cx;

	while ( _tcslen(szShort) >0) {
		TCHAR * pchar = _tcsdec(szShort, szShort + _tcslen(szShort));
		if (pchar == szShort)
			break; // don't want the result to be empty
		*pchar ='\0';
		if(pDC->GetTextExtent(szShort, (int)_tcslen(szShort)).cx+nOffset+nAddLen<=nColumnLen)
			break; 
	}

    lstrcat(szShort,szThreeDots);

    return(szShort);
}


void
UserUdts::OnClickListFiles(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_HITTESTINFO FAR	pinfo;
    DWORD		dwpos;
    int			rc;
    UINT		state; 
    POINT		point;
    char		FileName[48];

    /* Find out where the cursor was */
    dwpos = GetMessagePos();

    point.x = LOWORD(dwpos);
    point.y = HIWORD(dwpos);

    ::MapWindowPoints(HWND_DESKTOP, m_FileSelectList.m_hWnd, &point, 1);
    pinfo.pt = point;

    rc = m_FileSelectList.HitTest (&pinfo);
    if (pinfo.flags & LVHT_ONITEM)
    {
	/* Set the state to the opposite of what it is set to. */
	state = m_FileSelectList.GetItemState(pinfo.iItem, LVIS_STATEIMAGEMASK);
	state = (state == INDEXTOSTATEIMAGEMASK (NOCHECK)) ? 
			  INDEXTOSTATEIMAGEMASK (CHECK) : 
			  INDEXTOSTATEIMAGEMASK (NOCHECK);

	m_FileSelectList.SetItemState(pinfo.iItem, state, LVIS_STATEIMAGEMASK);

	/*
	** If this is not a .c file, disable the "view source" button.
	*/
	m_FileSelectList.GetItemText(pinfo.iItem, 0, (LPTSTR)&FileName,
				     sizeof(FileName));
	if (_tcsstr(FileName, ".c"))
	    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON_VIEWSRC), TRUE);
	else
	    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON_VIEWSRC), FALSE);
    }

    *pResult = 0;
}

LRESULT
UserUdts::OnWizardNext() 
{
    int		iItem, NumItems = m_FileSelectList.GetItemCount();
    UINT	state;
    struct node	*NodePtr, *NodePtr2, *NodePtr3;
    char	*cptr;
    CString	Message, Message2;
    int		NumChecked;

    /*
    ** First, free up the existing lists.
    */
    while (FileSrcList)
    {
	NodePtr = FileSrcList;
	FileSrcList = FileSrcList->next;
	free(NodePtr);
    }
    while (FileObjList)
    {
	NodePtr = FileObjList;
	FileObjList = FileObjList->next;
	free(NodePtr);
    }

    /*
    ** Now, create the lists of chosen files.
    */
    NumChecked = 0;
    for (iItem = 0; iItem < NumItems; iItem++)
    {
	state = m_FileSelectList.GetItemState(iItem, LVIS_STATEIMAGEMASK);
	if (state == INDEXTOSTATEIMAGEMASK (CHECK))
	{
	    NumChecked++;
	    NodePtr = (struct node *)malloc(sizeof(struct node));
	    m_FileSelectList.GetItemText(iItem, 0, NodePtr->filename,
					 sizeof(NodePtr->filename));
	    NodePtr->next = NULL;
	    if (_tcsstr(NodePtr->filename, ".c"))
	    {
		/*
		** Add to the source list.
		*/
		if (!FileSrcList)
		    FileSrcList = NodePtr;
		else
		{
		    NodePtr2 = FileSrcList;
		    while (NodePtr2->next)
			NodePtr2 = NodePtr2->next;
		    NodePtr2->next = NodePtr;
		}
	    }
	    else
	    {
		/*
		** Add to the object / library list.
		*/
		if (!FileObjList)
		    FileObjList = NodePtr;
		else
		{
		    NodePtr2 = FileObjList;
		    while (NodePtr2->next)
			NodePtr2 = NodePtr2->next;
		    NodePtr2->next = NodePtr;
		}
	    }
	}
    }

    if (!NumChecked)
    {
	Message.LoadString(IDS_NON_CHOSEN);
	Message2.LoadString(IDS_TITLE);
	MessageBox(Message, Message2, MB_OK | MB_ICONINFORMATION);
	return -1;
    }

    /*
    ** Now, for every source file, we need to add its resulting object
    ** file name to the object list.
    */
    NodePtr = FileSrcList;
    while (NodePtr)
    {
	NodePtr2 = (struct node *)malloc(sizeof(struct node));
	strcpy(NodePtr2->filename, NodePtr->filename);
	cptr = _tcschr(NodePtr2->filename, '.');
	strcpy(cptr, ".obj");
	NodePtr2->next = NULL;
	if (!FileObjList)
	    FileObjList = NodePtr2;
	else
	{
	    NodePtr3 = FileObjList;
	    while (NodePtr3->next)
		NodePtr3 = NodePtr3->next;
	    NodePtr3->next = NodePtr2;
	}

	NodePtr = NodePtr->next;
    }
	
    return CPropertyPage::OnWizardNext();
}

void
UserUdts::OnButtonSelect() 
{
    int iItem, NumItems = m_FileSelectList.GetItemCount();

    for (iItem = 0; iItem < NumItems; iItem++)
	m_FileSelectList.SetItemState( iItem, INDEXTOSTATEIMAGEMASK(CHECK),
					LVIS_STATEIMAGEMASK);
}

void
UserUdts::OnButtonDeselect() 
{
    int iItem, NumItems = m_FileSelectList.GetItemCount();

    for (iItem = 0; iItem < NumItems; iItem++)
	m_FileSelectList.SetItemState( iItem, INDEXTOSTATEIMAGEMASK(NOCHECK),
					LVIS_STATEIMAGEMASK);
}

void UserUdts::OnButtonViewsrc() 
{
    int		nItem;
    char	FileName[48];
    CString	FilePath;

    nItem = m_FileSelectList.GetNextItem(-1, LVIS_SELECTED);
    m_FileSelectList.GetItemText(nItem, 0, (LPTSTR)&FileName,
				 sizeof(FileName));
    FilePath = CString("notepad ") + UserLoc + CString("\\") + CString(FileName);
    WinExec(FilePath, SW_SHOWDEFAULT);
}

void UserUdts::OnKeydownListFiles(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;
    int		nItem;
    char	FileName[48];
    UINT	state;

    if ((pLVKeyDow->wVKey == VK_UP) || (pLVKeyDow->wVKey == VK_DOWN))
    {
	nItem = m_FileSelectList.GetNextItem(-1, LVIS_SELECTED);

	if ((pLVKeyDow->wVKey == VK_UP) && (nItem))
	    nItem--;
	else if ((pLVKeyDow->wVKey == VK_DOWN) &&
		(nItem+1 != m_FileSelectList.GetItemCount()))
	    nItem++;

	m_FileSelectList.GetItemText(nItem, 0, (LPTSTR)&FileName,
				     sizeof(FileName));
	if (_tcsstr(FileName, ".c"))
	    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON_VIEWSRC), TRUE);
	else
	    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON_VIEWSRC), FALSE);
    }
    else if (pLVKeyDow->wVKey == VK_SPACE)
    {	
	nItem = m_FileSelectList.GetNextItem(-1, LVIS_SELECTED);

	/*
	** Set the state to the opposite of what it is.
	*/
	state = m_FileSelectList.GetItemState(nItem, LVIS_STATEIMAGEMASK);
	state = (state == INDEXTOSTATEIMAGEMASK (NOCHECK)) ? 
			  INDEXTOSTATEIMAGEMASK (CHECK) : 
			  INDEXTOSTATEIMAGEMASK (NOCHECK);

	m_FileSelectList.SetItemState(nItem, state, LVIS_STATEIMAGEMASK);
    }

    *pResult = 0;
}

void UserUdts::OnDblclkListFiles(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_HITTESTINFO FAR	pinfo;
    DWORD		dwpos;
    int			rc;
    POINT		point;
    char		FileName[48];

    /* Find out where the cursor was */
    dwpos = GetMessagePos();

    point.x = LOWORD(dwpos);
    point.y = HIWORD(dwpos);

    ::MapWindowPoints(HWND_DESKTOP, m_FileSelectList.m_hWnd, &point, 1);
    pinfo.pt = point;

    rc = m_FileSelectList.HitTest (&pinfo);
    if (pinfo.flags & LVHT_ONITEM)
    {
	/*
	** If this is a .c file, view it with notepad.
	*/
	m_FileSelectList.GetItemText(pinfo.iItem, 0, (LPTSTR)&FileName,
				     sizeof(FileName));
	if (_tcsstr(FileName, ".c"))
	    UserUdts::OnButtonViewsrc();
    }

    *pResult = 0;
}
