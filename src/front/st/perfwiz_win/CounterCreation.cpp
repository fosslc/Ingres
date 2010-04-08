/*
**
**  Name: CounterCreation.cpp
**
**  Description:
**	This file contains the routines necessary for the Counter Creation
**	Property Page, which allows users to create/modify/delete personal
**	counters.
**
**  History:
**	25-oct-1999 (somsa01)
**	    Created.
**	26-jan-2000 (somsa01)
**	    In OnButtonEditCounter(0), when searching for a duplicate
**	    counter name, ignore a found entry if it is equal to the index
**	    of the item that we are currently editing.
**	07-May-2008 (drivi01)
**	    Apply Wizard97 template to this utility.
*/

#include "stdafx.h"
#include "perfwiz.h"
#include "CounterCreation.h"
#include "EditCounter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int  CounterIndex, GroupIndex;

/*
** CCounterCreation property page
*/
IMPLEMENT_DYNCREATE(CCounterCreation, CPropertyPage)

CCounterCreation::CCounterCreation() : CPropertyPage(CCounterCreation::IDD)
{
    //{{AFX_DATA_INIT(CCounterCreation)
	    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CCounterCreation::CCounterCreation(CPropertySheet *ps) : CPropertyPage(CCounterCreation::IDD)
{
    m_propertysheet = ps;
	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}

CCounterCreation::~CCounterCreation()
{
}

void CCounterCreation::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CCounterCreation)
    DDX_Control(pDX, IDC_PERSONAL_COUNTER_LIST, m_CounterList);
    DDX_Control(pDX, IDC_COMBO_GROUPS, m_ObjectList);
    DDX_Control(pDX, IDC_IMAGE, m_image);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CCounterCreation, CPropertyPage)
    //{{AFX_MSG_MAP(CCounterCreation)
    ON_WM_DRAWITEM()
	ON_CBN_SELCHANGE(IDC_COMBO_GROUPS, OnSelchangeComboGroups)
	ON_BN_CLICKED(IDC_BUTTON_CREATE_COUNTER, OnButtonCreateCounter)
	ON_BN_CLICKED(IDC_BUTTON_EDIT_COUNTER, OnButtonEditCounter)
	ON_NOTIFY(NM_CLICK, IDC_PERSONAL_COUNTER_LIST, OnClickPersonalCounterList)
	ON_NOTIFY(NM_DBLCLK, IDC_PERSONAL_COUNTER_LIST, OnDblclkPersonalCounterList)
	ON_NOTIFY(LVN_KEYDOWN, IDC_PERSONAL_COUNTER_LIST, OnKeydownPersonalCounterList)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_COUNTER, OnButtonDeleteCounter)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** CCounterCreation message handlers
*/
LPCTSTR
CCounterCreation::MakeShortString(CDC *pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset)
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
CCounterCreation::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
    int			m_cxClient=0, nColumn, m_cxStateImageOffset=0, nRetLen;
    BOOL		m_bClientWidthSel=TRUE;
    COLORREF		m_inactive=::GetSysColor(COLOR_INACTIVEBORDER);
    COLORREF		m_clrTextBk=::GetSysColor(COLOR_WINDOW);
    COLORREF		m_clrBkgnd=::GetSysColor(COLOR_WINDOW);
    CListCtrl		&ListCtrl=m_CounterList;
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

BOOL
CCounterCreation::OnInitDialog() 
{
    CString	Message;

    CPropertyPage::OnInitDialog();

    Message.LoadString(IDS_COUNTER_CREATION_HEADING);
    SetDlgItemText(IDC_COUNTER_CREATION_HEADING, Message);
    Message.LoadString(IDS_PERSONAL_GROUP_NAME);
    SetDlgItemText(IDC_PERSONAL_GROUP_NAME, Message);
    Message.LoadString(IDS_BUTTON_CREATE_COUNTER);
    SetDlgItemText(IDC_BUTTON_CREATE_COUNTER, Message);
    Message.LoadString(IDS_BUTTON_EDIT_COUNTER);
    SetDlgItemText(IDC_BUTTON_EDIT_COUNTER, Message);
    Message.LoadString(IDS_BUTTON_DELETE_COUNTER);
    SetDlgItemText(IDC_BUTTON_DELETE_COUNTER, Message);

    InitListViewControl();

    return TRUE;
}

void
CCounterCreation::InitListViewControl()
{
    LV_COLUMN  col;
    CString ColumnHeader;
    RECT rc;

    ColumnHeader.LoadString(IDS_PERSONAL_COUNTERS_COLUMN);
    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    m_CounterList.GetClientRect(&rc);
    col.cx = rc.right;
    col.fmt = LVCFMT_LEFT;
    col.iSubItem = 0;
    col.pszText = ColumnHeader.GetBuffer(256);
    m_CounterList.InsertColumn(0, &col);
}

BOOL CCounterCreation::OnSetActive() 
{
    struct grouplist    *gptr = GroupList;
    LV_ITEM		lvitem;
    CString		GroupName;
    struct PersCtrList  *PCPtr;

    /*
    ** Add the list of personal groups.
    */
    m_ObjectList.ResetContent();
    while (gptr)
    {
	m_ObjectList.AddString(gptr->GroupName);
	gptr = gptr->next;
    }
    m_ObjectList.SetCurSel(0);
    GroupIndex = 0;

    /*
    ** Add the list of personal counters (if any) for the group.
    */
    m_CounterList.DeleteAllItems();
    m_ObjectList.GetLBText(m_ObjectList.GetCurSel(), GroupName);
    gptr = GroupList;
    while (strcmp(gptr->GroupName, GroupName))
	gptr = gptr->next;

    PCPtr = gptr->PersCtrs;
    while (PCPtr)
    {
	lvitem.iItem = m_CounterList.GetItemCount();
	lvitem.mask = LVIF_TEXT;
	lvitem.iSubItem = 0;
	lvitem.pszText = PCPtr->PersCtr.CounterName;
	lvitem.iItem = m_CounterList.InsertItem(&lvitem);

	PCPtr = PCPtr->next;
    }
    CounterIndex = -1;

    m_propertysheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

    return CPropertyPage::OnSetActive();
}

void CCounterCreation::OnSelchangeComboGroups() 
{
    struct grouplist    *gptr = GroupList;
    LV_ITEM		lvitem;
    CString		GroupName;
    struct PersCtrList  *PCPtr;

    if (m_ObjectList.GetCurSel() != GroupIndex)
    {
	m_CounterList.DeleteAllItems();

	m_ObjectList.GetLBText(m_ObjectList.GetCurSel(), GroupName);
	gptr = GroupList;
	while (strcmp(gptr->GroupName, GroupName))
	    gptr = gptr->next;

	PCPtr = gptr->PersCtrs;
	while (PCPtr)
	{
	    lvitem.iItem = m_CounterList.GetItemCount();
	    lvitem.mask = LVIF_TEXT;
	    lvitem.iSubItem = 0;
	    lvitem.pszText = PCPtr->PersCtr.CounterName;
	    lvitem.iItem = m_CounterList.InsertItem(&lvitem);

	    PCPtr = PCPtr->next;
	}
	CounterIndex = -1;

	GroupIndex = m_ObjectList.GetCurSel();
    }	
}

void CCounterCreation::OnButtonCreateCounter() 
{
    CEditCounter	ecDlg;
    LV_ITEM		lvitem;
    LVFINDINFO		info;
    CString		GroupName, Message, Message2;
    struct grouplist	*gptr;
    struct PersCtrList	*node;
    
    ecDlg.m_CounterScale = 0;
    while (ecDlg.DoModal() == IDOK)
    {
	info.flags = LVFI_STRING;
	info.psz = ecDlg.m_CounterName.GetBuffer(0);
	if (m_CounterList.FindItem(&info) != -1)
	{
	    /*
	    ** Duplicate Counter Name!
	    */
	    Message.LoadString(IDS_DUPLICATE_COUNTER);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(Message, Message2, MB_OK | MB_ICONINFORMATION);
	    continue;
	}

	/*
	** Now, add the personal counter to the selected personal
	** group.
	*/
	node = (struct PersCtrList *)malloc(
			sizeof(struct PersCtrList));
	node->next = NULL;
	node->PersCtr.DefineName = NULL;
	node->PersCtr.UserSelected = FALSE;
	node->PersCtr.classid = PersonalClass;
	node->PersCtr.groupid = -1;
	node->PersCtr.ctrid = -1;
	node->PersCtr.CounterID = -1;
	node->PersCtr.PCounterName =
		(char *)malloc(ecDlg.m_CounterName.GetLength()+1);
	strcpy(node->PersCtr.PCounterName, ecDlg.m_CounterName);
	node->PersCtr.CounterName =
		(char *)malloc(ecDlg.m_CounterName.GetLength()+1);
	strcpy(node->PersCtr.CounterName, ecDlg.m_CounterName);
	node->PersCtr.UserName =
		(char *)malloc(ecDlg.m_CounterName.GetLength()+1);
	strcpy(node->PersCtr.UserName, ecDlg.m_CounterName);
	node->PersCtr.CounterHelp =
		(char *)malloc(ecDlg.m_CounterDescription.GetLength()+1);
	strcpy(node->PersCtr.CounterHelp, ecDlg.m_CounterDescription);
	node->PersCtr.UserHelp =
		(char *)malloc(ecDlg.m_CounterDescription.GetLength()+1);
	strcpy(node->PersCtr.UserHelp, ecDlg.m_CounterDescription);
	node->PersCtr.selected = TRUE;

	node->dbname =
		(char *)malloc(ecDlg.m_CounterDBName.GetLength()+1);
	strcpy(node->dbname, ecDlg.m_CounterDBName);
	node->qry =
		(char *)malloc(ecDlg.m_CounterQuery.GetLength()+1);
	strcpy(node->qry, ecDlg.m_CounterQuery);
	node->scale = ecDlg.m_CounterScale;

	m_ObjectList.GetLBText(m_ObjectList.GetCurSel(), GroupName);
	gptr = GroupList;
	while (strcmp(gptr->GroupName, GroupName))
	    gptr = gptr->next;

	if (!gptr->PersCtrs)
	    gptr->PersCtrs = node;
	else
	{
	    struct PersCtrList	*PCPtr = gptr->PersCtrs;

	    while (PCPtr->next)
		PCPtr = PCPtr->next;
	    PCPtr->next = node;
	}

	lvitem.iItem = m_CounterList.GetItemCount();
	lvitem.mask = LVIF_TEXT;
	lvitem.iSubItem = 0;
	lvitem.pszText = node->PersCtr.CounterName;
	lvitem.iItem = m_CounterList.InsertItem(&lvitem);

	break;
    }
}

void CCounterCreation::OnButtonEditCounter() 
{
    CEditCounter	ecDlg;
    LVFINDINFO		info;
    CString		GroupName, Counter, Message, Message2;
    int			FindIndex;
    struct grouplist	*gptr;
    struct PersCtrList	*PCPtr;

    if (CounterIndex != -1)
    {
	m_ObjectList.GetLBText(m_ObjectList.GetCurSel(), GroupName);
	gptr = GroupList;
	while (strcmp(gptr->GroupName, GroupName))
	    gptr = gptr->next;

	Counter = m_CounterList.GetItemText(CounterIndex, 0);
	PCPtr = gptr->PersCtrs;
	while (strcmp(PCPtr->PersCtr.CounterName, Counter))
	    PCPtr = PCPtr->next;

	ecDlg.m_CounterDBName = CString(PCPtr->dbname);
	ecDlg.m_CounterDescription = CString(PCPtr->PersCtr.CounterHelp);
	ecDlg.m_CounterName = CString(PCPtr->PersCtr.CounterName);
	ecDlg.m_CounterQuery = CString(PCPtr->qry);
	ecDlg.m_CounterScale = PCPtr->scale;

	while (ecDlg.DoModal() == IDOK)
	{
	    info.flags = LVFI_STRING;
	    info.psz = ecDlg.m_CounterName.GetBuffer(0);
	    FindIndex = m_CounterList.FindItem(&info);
	    if ((FindIndex != -1) && (FindIndex != CounterIndex) &&
		(m_CounterList.GetItemCount() > 1))
	    {
		/*
		** Duplicate Counter Name!
		*/
		Message.LoadString(IDS_DUPLICATE_COUNTER);
		Message2.LoadString(IDS_TITLE);
		MessageBox(Message, Message2, MB_OK | MB_ICONINFORMATION);
		continue;
	    }

	    /*
	    ** Now, modify the personal counter.
	    */
	    free(PCPtr->PersCtr.PCounterName);
	    PCPtr->PersCtr.PCounterName =
		    (char *)malloc(ecDlg.m_CounterName.GetLength()+1);
	    strcpy(PCPtr->PersCtr.PCounterName, ecDlg.m_CounterName);

	    free(PCPtr->PersCtr.CounterName);
	    PCPtr->PersCtr.CounterName =
		    (char *)malloc(ecDlg.m_CounterName.GetLength()+1);
	    strcpy(PCPtr->PersCtr.CounterName, ecDlg.m_CounterName);

	    free(PCPtr->PersCtr.UserName);
	    PCPtr->PersCtr.UserName =
		    (char *)malloc(ecDlg.m_CounterName.GetLength()+1);
	    strcpy(PCPtr->PersCtr.UserName, ecDlg.m_CounterName);

	    free(PCPtr->PersCtr.CounterHelp);
	    PCPtr->PersCtr.CounterHelp =
		    (char *)malloc(ecDlg.m_CounterDescription.GetLength()+1);
	    strcpy(PCPtr->PersCtr.CounterHelp, ecDlg.m_CounterDescription);

	    free(PCPtr->PersCtr.UserHelp);
	    PCPtr->PersCtr.UserHelp =
		    (char *)malloc(ecDlg.m_CounterDescription.GetLength()+1);
	    strcpy(PCPtr->PersCtr.UserHelp, ecDlg.m_CounterDescription);

	    free(PCPtr->dbname);
	    PCPtr->dbname =
		    (char *)malloc(ecDlg.m_CounterDBName.GetLength()+1);
	    strcpy(PCPtr->dbname, ecDlg.m_CounterDBName);

	    free(PCPtr->qry);
	    PCPtr->qry =
		    (char *)malloc(ecDlg.m_CounterQuery.GetLength()+1);
	    strcpy(PCPtr->qry, ecDlg.m_CounterQuery);

	    PCPtr->scale = ecDlg.m_CounterScale;

	    m_CounterList.SetItemText(CounterIndex, 0, PCPtr->PersCtr.CounterName);

	    break;
	}
    }
}

void CCounterCreation::OnClickPersonalCounterList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_HITTESTINFO FAR	pinfo;
    DWORD		dwpos;
    POINT		point;

    /* Find out where the cursor was */
    dwpos = GetMessagePos();

    point.x = LOWORD(dwpos);
    point.y = HIWORD(dwpos);

    ::MapWindowPoints(HWND_DESKTOP, m_CounterList.m_hWnd, &point, 1);
    pinfo.pt = point;

    m_CounterList.HitTest(&pinfo);
    if (pinfo.flags & LVHT_ONITEM)
	CounterIndex = pinfo.iItem;
    *pResult = 0;
}

void CCounterCreation::OnDblclkPersonalCounterList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    OnButtonEditCounter();

    *pResult = 0;
}

void CCounterCreation::OnKeydownPersonalCounterList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;

    if (pLVKeyDow->wVKey == VK_DELETE)
	OnButtonDeleteCounter();
    
    *pResult = 0;
}

void CCounterCreation::OnButtonDeleteCounter() 
{
    CString		GroupName, Counter;
    struct grouplist	*gptr;
    struct PersCtrList	*PCPtr, *PCPtr2;

    if (CounterIndex != -1)
    {
	m_ObjectList.GetLBText(m_ObjectList.GetCurSel(), GroupName);
	gptr = GroupList;
	while (strcmp(gptr->GroupName, GroupName))
	    gptr = gptr->next;

	Counter = m_CounterList.GetItemText(CounterIndex, 0);
	PCPtr = gptr->PersCtrs;
	PCPtr2 = NULL;
	while (strcmp(PCPtr->PersCtr.CounterName, Counter))
	{
	    PCPtr2 = PCPtr;
	    PCPtr = PCPtr->next;
	}

	if (PCPtr2)
	    PCPtr2->next = PCPtr->next;
	else
	    gptr->PersCtrs = PCPtr->next;

	/*
	** Free up its members.
	*/
	free(PCPtr->dbname);
	free(PCPtr->qry);
	free(PCPtr->PersCtr.CounterName);
	free(PCPtr->PersCtr.PCounterName);
	free(PCPtr->PersCtr.CounterHelp);
	free(PCPtr->PersCtr.UserName);
	free(PCPtr->PersCtr.UserHelp);
	if (PCPtr->PersCtr.DefineName)
	    free(PCPtr->PersCtr.DefineName);
	free(PCPtr);

	m_CounterList.DeleteItem(CounterIndex);
	CounterIndex = -1;
    }
}
