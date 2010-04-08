/*
**
**  Name: PersonalHelp.cpp
**
**  Description:
**	This file contains the routines necessary for the Personal Help Property
**	Page, which allows users to edit counters in their personal groups.
**
**  History:
**	06-sep-1999 (somsa01)
**	    Created.
**	15-oct-1999 (somsa01)
**	    Removed OnItemChangedPcList() function. Also, the Server selection
**	    page is now the final page.
**	29-oct-1999 (somsa01)
**	    Added the ability to add personal counters to the registry.
**	28-jan-2000 (somsa01)
**	    Added Sleep() call while waiting for threads.
**	07-May-2008 (drivi01)
**	    Apply Wizard97 template to this utility.
**	30-May-3008 (whiro01) Bug 119642, 115719; SD issue 122646
**	    Reference one common declaration of the main registry key (PERFCTRS_KEY).
*/

#include "stdafx.h"
#include "perfwiz.h"
#include "PersonalHelp.h"
#include "WaitDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int	PersonalIndex, GroupIndex;
static CString	OldName;

static int  CreateInitFiles();
static long LoadPerfCounters();

extern void UpdateCounterIDs();

/*
** CPersonalHelp property page
*/
IMPLEMENT_DYNCREATE(CPersonalHelp, CPropertyPage)

CPersonalHelp::CPersonalHelp() : CPropertyPage(CPersonalHelp::IDD)
{
    //{{AFX_DATA_INIT(CPersonalHelp)
	    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CPersonalHelp::CPersonalHelp(CPropertySheet *ps) : CPropertyPage(CPersonalHelp::IDD)
{
    m_propertysheet = ps;
	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}

CPersonalHelp::~CPersonalHelp()
{
}

void CPersonalHelp::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPersonalHelp)
    DDX_Control(pDX, IDC_PG_LIST, m_GroupList);
    DDX_Control(pDX, IDC_PC_LIST, m_PersonalCtrList);
    DDX_Control(pDX, IDC_IMAGE, m_image);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPersonalHelp, CPropertyPage)
    //{{AFX_MSG_MAP(CPersonalHelp)
    ON_WM_DRAWITEM()
    ON_NOTIFY(NM_DBLCLK, IDC_PC_LIST, OnDblclkPcList)
    ON_NOTIFY(NM_CLICK, IDC_PC_LIST, OnClickPcList)
    ON_CBN_SELCHANGE(IDC_PG_LIST, OnSelchangePgList)
    ON_BN_CLICKED(IDC_BUTTON_SAVE_HELP, OnButtonSaveHelp)
    ON_EN_UPDATE(IDC_EDIT_HELP, OnUpdateEditHelp)
    ON_BN_CLICKED(IDC_BUTTON_RESET, OnButtonReset)
    ON_MESSAGE(WM_APP, OnUserMessage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** CPersonalHelp message handlers
*/
LPCTSTR
CPersonalHelp::MakeShortString(CDC *pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset)
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
CPersonalHelp::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
    int			m_cxClient=0, nColumn, m_cxStateImageOffset=0, nRetLen;
    BOOL		m_bClientWidthSel=TRUE;
    COLORREF		m_inactive=::GetSysColor(COLOR_INACTIVEBORDER);
    COLORREF		m_clrTextBk=::GetSysColor(COLOR_WINDOW);
    COLORREF		m_clrBkgnd=::GetSysColor(COLOR_WINDOW);
    CListCtrl		&ListCtrl=m_PersonalCtrList;
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
CPersonalHelp::InitListViewControl()
{
    LV_COLUMN  col;
    CString ColumnHeader;
    RECT rc;

    ColumnHeader.LoadString(IDS_PCLIST_COLUMN);
    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    m_PersonalCtrList.GetClientRect(&rc);
    col.cx = rc.right;
    col.fmt = LVCFMT_LEFT;
    col.iSubItem = 0;
    col.pszText = ColumnHeader.GetBuffer(256);
    m_PersonalCtrList.InsertColumn(0, &col);
}


BOOL
CPersonalHelp::OnInitDialog() 
{
    CString		Message;
    struct grouplist	*gptr = GroupList;
    CEdit		*pEdit = (CEdit *)GetDlgItem(IDC_EDIT_HELP);
    CButton		*pButSave = (CButton *)GetDlgItem(IDC_BUTTON_SAVE_HELP);

    CPropertyPage::OnInitDialog();

    Message.LoadString(IDS_PERSONAL_HELP_HEADING);
    SetDlgItemText(IDC_PERSONAL_HELP_HEADING, Message);
    Message.LoadString(IDS_PERSONAL_GROUP_NAME);
    SetDlgItemText(IDC_PERSONAL_GROUP_NAME, Message);
    Message.LoadString(IDS_PERSONAL_HELP_COMBO);
    SetDlgItemText(IDC_PERSONAL_HELP_COMBO, Message);
    Message.LoadString(IDS_HELP_DESC);
    SetDlgItemText(IDC_HELP_DESC, Message);
    Message.LoadString(IDS_BUTTON_SAVE_HELP);
    SetDlgItemText(IDC_BUTTON_SAVE_HELP, Message);
    Message.LoadString(IDS_BUTTON_RESET);
    SetDlgItemText(IDC_BUTTON_RESET, Message);

    PersonalIndex = -1;
    pEdit->SetWindowText("");
    pEdit->EnableWindow(FALSE);
    pButSave->EnableWindow(FALSE);
    
    InitListViewControl();

    while (gptr)
    {
	m_GroupList.AddString(gptr->GroupName);
	gptr = gptr->next;
    }
    m_GroupList.SetCurSel(0);
    GroupIndex = 0;

    ListViewInsert();

    return TRUE;
}

void
CPersonalHelp::ListViewInsert()
{
    LV_ITEM		lvitem;
    CString		GroupName;
    struct grouplist	*gptr = GroupList;
    int			i, Index;

    m_GroupList.GetLBText(m_GroupList.GetCurSel(), GroupName);
    while (strcmp(gptr->GroupName, GroupName))
	gptr = gptr->next;

    for (i=0, Index=0; i<NUM_PERSONAL_COUNTERS; i++)
    {
	if (gptr->CounterList[i].selected)
	{
	    lvitem.iItem = Index++;
	    lvitem.mask = LVIF_TEXT;
	    lvitem.iSubItem = 0;
	    lvitem.pszText = gptr->CounterList[i].UserName;
	    lvitem.iItem = m_PersonalCtrList.InsertItem(&lvitem);
	}
    }

    /*
    ** Insert any personal counters.
    */
    if (gptr->PersCtrs)
    {
	struct PersCtrList  *PCPtr = gptr->PersCtrs;
	int		    Index = m_PersonalCtrList.GetItemCount();

	while (PCPtr)
	{
	    m_PersonalCtrList.InsertItem(Index++,
					 PCPtr->PersCtr.UserName);
	    PCPtr = PCPtr->next;
	}
    }
}

BOOL
CPersonalHelp::OnSetActive() 
{
    struct grouplist	*gptr = GroupList;
    CEdit		*pEdit = (CEdit *)GetDlgItem(IDC_EDIT_HELP);
    CButton		*pButSave = (CButton *)GetDlgItem(IDC_BUTTON_SAVE_HELP);

    m_propertysheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

    m_GroupList.ResetContent();
    while (gptr)
    {
	m_GroupList.AddString(gptr->GroupName);
	gptr = gptr->next;
    }
    m_GroupList.SetCurSel(0);
    GroupIndex = 0;

    PersonalIndex = -1;
    pEdit->SetWindowText("");
    pEdit->EnableWindow(FALSE);
    pButSave->EnableWindow(FALSE);

    m_PersonalCtrList.DeleteAllItems();
    ListViewInsert();	

    return CPropertyPage::OnSetActive();
}

void
CPersonalHelp::OnDblclkPcList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_HITTESTINFO FAR	pinfo;
    DWORD		dwpos;
    int			rc, colnum;
    POINT		point;

    /* Find out where the cursor was */
    dwpos = GetMessagePos();

    point.x = LOWORD(dwpos);
    point.y = HIWORD(dwpos);

    ::MapWindowPoints(HWND_DESKTOP, m_PersonalCtrList.m_hWnd, &point, 1);
    pinfo.pt = point;

    CPoint classpt(point);
    rc = m_PersonalCtrList.HitTestEx(classpt, &colnum);
    if (rc != -1)
    {
	OldName = m_PersonalCtrList.GetItemText(rc, colnum);
        m_PersonalCtrList.EditSubLabel(rc, colnum);
    }
	
    *pResult = 0;
}

void
CPersonalHelp::OnClickPcList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_HITTESTINFO FAR	pinfo;
    DWORD		dwpos;
    POINT		point;
    struct grouplist	*gptr = GroupList;
    CString		GroupName, CounterName;

    if (OldName != "")
    {
	LV_ITEM	lvi;

	/*
	** Re-select the item that was being edited.
	*/
	lvi.mask=LVIF_STATE;
	lvi.iItem=PersonalIndex;
	lvi.iSubItem=0;
	lvi.state=LVIS_SELECTED;
	lvi.stateMask=LVIS_SELECTED;
	m_PersonalCtrList.SetItem(&lvi);
	
	return;
    }

    /* Find out where the cursor was */
    dwpos = GetMessagePos();

    point.x = LOWORD(dwpos);
    point.y = HIWORD(dwpos);

    ::MapWindowPoints(HWND_DESKTOP, m_PersonalCtrList.m_hWnd, &point, 1);
    pinfo.pt = point;

    m_PersonalCtrList.HitTest(&pinfo);
    if (pinfo.flags & LVHT_ONITEM)
    {
	CEdit	*pEdit = (CEdit *)GetDlgItem(IDC_EDIT_HELP);
        CButton *pButSave = (CButton *)GetDlgItem(IDC_BUTTON_SAVE_HELP);
	CString Message, Message2;
	int	status;

	if (pEdit->GetModify())
	{
	    Message.LoadString(IDS_UPDATE_HELP);
	    Message2.LoadString(IDS_TITLE);
	    status = MessageBox(Message, Message2, MB_ICONQUESTION | MB_YESNO);
	    if (status == IDYES)
		OnButtonSaveHelp();
	}

	pEdit->EnableWindow(TRUE);
	pButSave->EnableWindow(FALSE);
	PersonalIndex = pinfo.iItem;

	/* first, find the right group pointer. */
	m_GroupList.GetLBText(m_GroupList.GetCurSel(), GroupName);
	while (strcmp(gptr->GroupName, GroupName))
	    gptr = gptr->next;

	/* Now, find the right counter. */
	if (OldName != "")
	    CounterName = OldName;
	else
	    CounterName = m_PersonalCtrList.GetItemText(pinfo.iItem, 0);
	PersonalIndex = 0;
	while ( (PersonalIndex < NUM_PERSONAL_COUNTERS) &&
		(strcmp(gptr->CounterList[PersonalIndex].UserName, CounterName)))
	    PersonalIndex++;

	if (PersonalIndex < NUM_PERSONAL_COUNTERS)
	    pEdit->SetWindowText(gptr->CounterList[PersonalIndex].UserHelp);
	else
	{
	    struct PersCtrList  *PCPtr = gptr->PersCtrs;

	    /*
	    ** This is a personal counter.
	    */
	    while (strcmp(PCPtr->PersCtr.UserName, CounterName))
		PCPtr = PCPtr->next;

	    pEdit->SetWindowText(PCPtr->PersCtr.UserHelp);
	}
    }
    else
	PersonalIndex = -1;

    *pResult = 0;
}

void
CPersonalHelp::OnSelchangePgList() 
{
    CEdit *pEdit = (CEdit *)GetDlgItem(IDC_EDIT_HELP);
    CButton *pButSave = (CButton *)GetDlgItem(IDC_BUTTON_SAVE_HELP);
    CString Message, Message2;
    int	status;

    if (pEdit->GetModify())
    {
	Message.LoadString(IDS_UPDATE_HELP);
	Message2.LoadString(IDS_TITLE);
	status = MessageBox(Message, Message2, MB_ICONQUESTION | MB_YESNO);
	if (status == IDYES)
	    OnButtonSaveHelp();
    }

    if (m_GroupList.GetCurSel() != GroupIndex)
    {
	PersonalIndex = -1;
	pEdit->SetWindowText("");
	pEdit->EnableWindow(FALSE);
	pButSave->EnableWindow(FALSE);

	m_PersonalCtrList.DeleteAllItems();
	ListViewInsert();
	GroupIndex = m_GroupList.GetCurSel();
    }
}

void
CPersonalHelp::OnButtonSaveHelp() 
{
    CEdit		*pEdit = (CEdit *)GetDlgItem(IDC_EDIT_HELP);
    CButton		*pButSave = (CButton *)GetDlgItem(IDC_BUTTON_SAVE_HELP);
    CString		GroupName, HelpText;
    struct grouplist	*gptr = GroupList;

    if (PersonalIndex != -1)
    {
	GetDlgItemText(IDC_EDIT_HELP, HelpText);

	m_GroupList.GetLBText(m_GroupList.GetCurSel(), GroupName);
	while (strcmp(gptr->GroupName, GroupName))
	    gptr = gptr->next;

	if (PersonalIndex < NUM_PERSONAL_COUNTERS)
	{
	    free(gptr->CounterList[PersonalIndex].UserHelp);
	    gptr->CounterList[PersonalIndex].UserHelp = (char *)malloc(
		    HelpText.GetLength()+1);
	    strcpy(gptr->CounterList[PersonalIndex].UserHelp, HelpText);
	}
	else
	{
	    CString CounterName;
	    struct PersCtrList  *PCPtr = gptr->PersCtrs;

	    /*
	    ** This is a personal counter.
	    */
	    CounterName = m_PersonalCtrList.GetItemText(
		    m_PersonalCtrList.GetSelectionMark(), 0);
	    while (strcmp(PCPtr->PersCtr.UserName, CounterName))
		PCPtr = PCPtr->next;

	    free(PCPtr->PersCtr.UserHelp);
	    PCPtr->PersCtr.UserHelp = (char *)malloc(
		    HelpText.GetLength()+1);
	    strcpy(PCPtr->PersCtr.UserHelp, HelpText);
	}

	pEdit->SetModify(FALSE);
	pButSave->EnableWindow(FALSE);
    }
}

void
CPersonalHelp::OnUpdateEditHelp() 
{
    CButton *pButSave = (CButton *)GetDlgItem(IDC_BUTTON_SAVE_HELP);

    pButSave->EnableWindow(TRUE);
}

void
CPersonalHelp::OnButtonReset() 
{
    CEdit		*pEdit = (CEdit *)GetDlgItem(IDC_EDIT_HELP);
    CString		GroupName, HelpText;
    struct grouplist	*gptr = GroupList;
    LV_ITEM		lvi;

    if (PersonalIndex != -1)
    {
	GetDlgItemText(IDC_EDIT_HELP, HelpText);

	m_GroupList.GetLBText(m_GroupList.GetCurSel(), GroupName);
	while (strcmp(gptr->GroupName, GroupName))
	    gptr = gptr->next;

	if (PersonalIndex < NUM_PERSONAL_COUNTERS)
	{
	    free(gptr->CounterList[PersonalIndex].UserName);
	    gptr->CounterList[PersonalIndex].UserName = (char *)malloc(
		    strlen(gptr->CounterList[PersonalIndex].PCounterName)+1);
	    strcpy( gptr->CounterList[PersonalIndex].UserName,
		    gptr->CounterList[PersonalIndex].PCounterName );

	    free(gptr->CounterList[PersonalIndex].UserHelp);
	    gptr->CounterList[PersonalIndex].UserHelp = (char *)malloc(
		    strlen(gptr->CounterList[PersonalIndex].CounterHelp)+1);
	    strcpy( gptr->CounterList[PersonalIndex].UserHelp,
		    gptr->CounterList[PersonalIndex].CounterHelp );

	    SetDlgItemText(IDC_EDIT_HELP, gptr->CounterList[PersonalIndex].UserHelp);

	    lvi.mask=LVIF_TEXT;
	    lvi.iItem=m_PersonalCtrList.GetSelectionMark();
	    lvi.iSubItem=0;
	    lvi.pszText = gptr->CounterList[PersonalIndex].UserName;
	    m_PersonalCtrList.SetItem(&lvi);
	}
	else
	{
	    CString CounterName;
	    struct PersCtrList  *PCPtr = gptr->PersCtrs;

	    /*
	    ** This is a personal counter.
	    */
	    CounterName = m_PersonalCtrList.GetItemText(
		    m_PersonalCtrList.GetSelectionMark(), 0);
	    while (strcmp(PCPtr->PersCtr.UserName, CounterName))
		PCPtr = PCPtr->next;

	    free(PCPtr->PersCtr.UserName);
	    PCPtr->PersCtr.UserName = (char *)malloc(
		    strlen(PCPtr->PersCtr.PCounterName)+1);
	    strcpy( PCPtr->PersCtr.UserName,
		    PCPtr->PersCtr.PCounterName );

	    free(PCPtr->PersCtr.UserHelp);
	    PCPtr->PersCtr.UserHelp = (char *)malloc(
		    strlen(PCPtr->PersCtr.CounterHelp)+1);
	    strcpy( PCPtr->PersCtr.UserHelp,
		    PCPtr->PersCtr.CounterHelp );

	    SetDlgItemText(IDC_EDIT_HELP, PCPtr->PersCtr.UserHelp);

	    lvi.mask=LVIF_TEXT;
	    lvi.iItem=m_PersonalCtrList.GetSelectionMark();
	    lvi.iSubItem=0;
	    lvi.pszText = PCPtr->PersCtr.UserName;
	    m_PersonalCtrList.SetItem(&lvi);
	}

	pEdit->SetModify(FALSE);
    }	
}

LRESULT
CPersonalHelp::OnUserMessage(WPARAM wParam, LPARAM lParam)
{
    CString		NewName, GroupName;
    struct grouplist	*gptr = GroupList;

    if (lParam == -1)
    {
	OldName = "";
	return 0;
    }

    NewName = m_PersonalCtrList.GetItemText((int)lParam, 0);
    if (NewName == "")
    {
	PersonalIndex = (int)lParam;
	m_PersonalCtrList.SetItemText((int)lParam, 0, OldName);
	m_PersonalCtrList.EditSubLabel((int)lParam, 0);
	return 1;
    }

    /* first, find the right group pointer. */
    m_GroupList.GetLBText(m_GroupList.GetCurSel(), GroupName);
    while (strcmp(gptr->GroupName, GroupName))
	gptr = gptr->next;

    /* Now, find the right counter. */
    PersonalIndex = 0;
    while ( (PersonalIndex < NUM_PERSONAL_COUNTERS) &&
	    (strcmp(gptr->CounterList[PersonalIndex].UserName, OldName)))
	PersonalIndex++;

    if (PersonalIndex < NUM_PERSONAL_COUNTERS)
    {
	free(gptr->CounterList[PersonalIndex].UserName);
	gptr->CounterList[PersonalIndex].UserName = (char *)malloc(
		NewName.GetLength()+1);
	strcpy(gptr->CounterList[PersonalIndex].UserName, NewName);
    }
    else
    {
	struct PersCtrList  *PCPtr = gptr->PersCtrs;

	/*
	** This is a personal counter.
	*/
	while (strcmp(PCPtr->PersCtr.UserName, OldName))
	    PCPtr = PCPtr->next;

	free(PCPtr->PersCtr.UserName);
	PCPtr->PersCtr.UserName = (char *)malloc(NewName.GetLength()+1);
	strcpy(PCPtr->PersCtr.UserName, NewName);
    }

    OldName = "";

    return 0;
}


LRESULT
CPersonalHelp::OnWizardNext() 
{
    CWaitDlg	wait;
    HANDLE	hThread;
    DWORD	ThreadID, status = 0;
    CString	Message, Message2;
    CEdit	*pEdit = (CEdit *)GetDlgItem(IDC_EDIT_HELP);

    if (pEdit->GetModify())
    {
	Message.LoadString(IDS_UPDATE_HELP);
	Message2.LoadString(IDS_TITLE);
	status = MessageBox(Message, Message2, MB_ICONQUESTION | MB_YESNO);
	if (status == IDYES)
	    OnButtonSaveHelp();
    }

    Message.LoadString(IDS_ADDING_GROUPS_INFO);
    Message2.LoadString(IDS_TITLE);
    status = MessageBox(Message, Message2, MB_ICONQUESTION | MB_YESNO);
    if (status == IDNO)
	return (-1);

    wait.m_WaitHeading.LoadString(IDS_ADDING_GROUPS);
    MainWnd = NULL;
    hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)LoadPerfCounters,
			    NULL, 0, &ThreadID );
    wait.DoModal();
    while (WaitForSingleObject(hThread, 0) != WAIT_OBJECT_0)
		Sleep(200);
    GetExitCodeThread(hThread, &status);

    if (status == ERROR_SUCCESS)
    {
	Message.LoadString(IDS_FINISH_PERS);
	Message2.LoadString(IDS_TITLE);
	MessageBox(Message, Message2, MB_OK | MB_ICONINFORMATION);
    }
    else
    {
	char	outmsg[2048];

	Message.LoadString(IDS_ERROR_LODCTR);
	sprintf(outmsg, Message, status);
	Message2.LoadString(IDS_TITLE);
	MessageBox(outmsg, Message2, MB_OK | MB_ICONINFORMATION);
	return(-1);
    }

    if (UseExisting)
	return IDD_SERVER_PAGE;
    else
	return CPropertyPage::OnWizardNext();
}

static int
CreateInitFiles()
{
    FILE		*hdrptr, *iniptr;
    struct grouplist	*gptr;
    int			GroupNum, DefineID, i, numbytes;
    DWORD		size=0;
    CString		DefineName, IniLine;
    char		mline[2048], buffer[20];
    char		*RegEntry=NULL, *cptr;

    CString MarkHeader = CString(getenv("II_SYSTEM")) +
			 CString("\\ingres\\files\\oipfctrs.h");
    CString InitHeader = CString(getenv("II_SYSTEM")) +
			 CString("\\ingres\\files\\oipfpers.h");
    CString MarkFile = CString(getenv("II_SYSTEM")) +
		       CString("\\ingres\\files\\oipfctrs.ini");
    CString InitFile = CString(getenv("II_SYSTEM")) +
		       CString("\\ingres\\files\\oipfpers.ini");

    if ((GetFileAttributes(MarkHeader) == -1) ||
	(GetFileAttributes(MarkFile) == -1))
	return(1);

    /*
    ** First, make the personal init files a duplicate of the default ones.
    */
    CopyFile(MarkHeader, InitHeader, FALSE);
    CopyFile(MarkFile, InitFile, FALSE);

    /*
    ** Modify the symbolfile entry in the ini file to match that
    ** of the new header file.
    */
    iniptr = fopen(InitFile, "r+");
    numbytes = 0;
    while (1)
    {
	fgets(mline, sizeof(mline), iniptr);
	if (strstr(mline, "symbolfile"))
	    break;
	numbytes += (int)strlen(mline);
    }
    strcpy(mline, "symbolfile=oipfpers.h");
    fflush(iniptr);
    fseek(iniptr, numbytes, SEEK_SET);
    fprintf(iniptr, "\n%s", mline);
    fclose(iniptr);

    /*
    ** Now, append the new #define's to the header file, and the
    ** new groups to the ini file.
    */
    GroupNum = 1;
    DefineID = FIRST_PERSONAL_DEF_ID;
    hdrptr = fopen(InitHeader, "a");
    iniptr = fopen(InitFile, "a");
    gptr = GroupList;
    while (gptr)
    {
	/*
	** Write out the group define name and id.
	*/
	fprintf(iniptr, "\n");
	DefineName = CString("PFCTRS_PERS") + CString(itoa(GroupNum, buffer, 10)) +
		     CString("_OBJ");
	IniLine = DefineName + CString("_009_NAME=") + CString(gptr->GroupName);
	fprintf(iniptr, "%s\n", IniLine);
	IniLine = DefineName + CString("_009_HELP=") +
		  CString(gptr->GroupDescription);
	fprintf(iniptr, "%s\n", IniLine);
	sprintf(mline, "#define %-60s %d", DefineName, DefineID);
	fprintf(hdrptr, "%s\n", mline);
	DefineID += 2;

	/*
	** Now, write out the counter defines and ids.
	*/
	for (i=0; i<NUM_PERSONAL_COUNTERS; i++)
	{
	    DefineName = CString("PFCTRS_PERS") + CString(itoa(GroupNum, buffer, 10)) +
			 CString(gptr->CounterList[i].DefineName);
	    if (gptr->CounterList[i].selected)
	    {
		IniLine = DefineName + CString("_009_NAME=") +
			  CString(gptr->CounterList[i].UserName);
		fprintf(iniptr, "%s\n", IniLine);
		IniLine = DefineName + CString("_009_HELP=") +
			  CString(gptr->CounterList[i].UserHelp);
		fprintf(iniptr, "%s\n", IniLine);
	    }
	    sprintf(mline, "#define %-60s %d", DefineName, DefineID);
	    fprintf(hdrptr, "%s\n", mline);
	    DefineID += 2;
	}

	/*
	** Also write out any personal counter info.
	*/
	if (gptr->PersCtrs)
	{
	    struct PersCtrList  *PCPtr = gptr->PersCtrs;
	    int			Index, oldsize;
	    char		GroupID[20], Scale[20];

	    i = 1;
	    while (PCPtr)
	    {
		DefineName = CString("PFCTRS_PERS") + CString(itoa(GroupNum, buffer, 10)) +
			     CString("_PERSCTR") + CString(itoa(i++, buffer, 10));

		IniLine = DefineName + CString("_009_NAME=") +
			  CString(PCPtr->PersCtr.UserName);
		fprintf(iniptr, "%s\n", IniLine);
		IniLine = DefineName + CString("_009_HELP=") +
			  CString(PCPtr->PersCtr.UserHelp);
		fprintf(iniptr, "%s\n", IniLine);

		sprintf(mline, "#define %-60s %d", DefineName, DefineID);
		fprintf(hdrptr, "%s\n", mline);
		DefineID += 2;

		/*
		** The format is:
		** "GroupName;GroupID;CounterName;DefaultScale;Database;Query;"
		*/
		oldsize = size;
		if (oldsize)
		{
		    /*
		    ** Add one byte for the '\0' which terminates the
		    ** previous entry.
		    */
		    size++;
		}

		itoa(gptr->CounterID, GroupID, 10);
		itoa(PCPtr->scale, Scale, 10);
		size += (DWORD)(strlen(gptr->GroupName) + 1 +
			strlen(GroupID) + 1 +
			strlen(PCPtr->PersCtr.UserName) + 1 +
			strlen(Scale) + 1 +
			strlen(PCPtr->dbname) + 1 +
			strlen(PCPtr->qry) + 1);
		RegEntry = (char *)realloc(RegEntry, size);
		cptr = RegEntry;
		cptr += oldsize;

		if (oldsize)
		    *(cptr++) = '\0';

		Index = 0;
		while (Index < (int)strlen(gptr->GroupName))
		    *(cptr++) = gptr->GroupName[Index++];
		*(cptr++) = ';';

		Index = 0;
		while (Index < (int)strlen(GroupID))
		    *(cptr++) = GroupID[Index++];
		*(cptr++) = ';';

		Index = 0;
		while (Index < (int)strlen(PCPtr->PersCtr.UserName))
		    *(cptr++) = PCPtr->PersCtr.UserName[Index++];
		*(cptr++) = ';';

		Index = 0;
		while (Index < (int)strlen(Scale))
		    *(cptr++) = Scale[Index++];
		*(cptr++) = ';';

		Index = 0;
		while (Index < (int)strlen(PCPtr->dbname))
		    *(cptr++) = PCPtr->dbname[Index++];
		*(cptr++) = ';';

		Index = 0;
		while (Index < (int)strlen(PCPtr->qry))
		    *(cptr++) = PCPtr->qry[Index++];
		*(cptr++) = ';';


		PCPtr = PCPtr->next;
	    }
	}

	GroupNum++;
	gptr = gptr->next;
    }
    fclose(hdrptr);
    fclose(iniptr);

    if (RegEntry)
    {
	HKEY	hKeyDriverPerf;

	/* Add the space for the ending '\0' characters */
	size += 2;
	RegEntry = (char *)realloc(RegEntry, size);
	cptr = RegEntry;
	cptr += size - 2;
	*(cptr++) = '\0';
	*(cptr++) = '\0';

	/*
	** Add the buffer to the registry.
	*/
	if (RegOpenKeyEx (
		HKEY_LOCAL_MACHINE,
		PERFCTRS_KEY,
		0L,
		KEY_SET_VALUE,
		&hKeyDriverPerf) != ERROR_SUCCESS)
	    return(2);

	if (RegSetValueEx(
		hKeyDriverPerf,
		"Personal Counters",
		0,
		REG_MULTI_SZ,
		(LPBYTE)RegEntry,
		size) != ERROR_SUCCESS)
	{
	    RegCloseKey (hKeyDriverPerf);
	    return(2);
	}

	RegCloseKey (hKeyDriverPerf);
    }

    return(0);
}

static long
LoadPerfCounters()
{
    DWORD    status = 0;
    CString InitFile = CString(getenv("II_SYSTEM")) +
		       CString("\\ingres\\files\\oipfpers.ini");
    CString CmdLine = CString("lodctr ") + InitFile;
    STARTUPINFO	    siStInfo;
    PROCESS_INFORMATION piProcInfo;

    /*
    ** Set up the counter id's.
    */
    UpdateCounterIDs();

    /*
    ** Create the initialization and definition files.
    */
    if ((status = CreateInitFiles()))
    {
	while (!MainWnd);
	::SendMessage(MainWnd, WM_CLOSE, 0, 0);
	return(status);
    }

    /*
    ** Now, add the groups to the registry.
    */
    memset (&siStInfo, 0, sizeof (siStInfo)) ;

    siStInfo.cb          = sizeof (siStInfo);
    siStInfo.lpReserved  = NULL;
    siStInfo.lpReserved2 = NULL;
    siStInfo.cbReserved2 = 0;
    siStInfo.lpDesktop   = NULL;
    siStInfo.dwFlags     = STARTF_USESHOWWINDOW;
    siStInfo.wShowWindow = SW_HIDE;

    CreateProcess(NULL, "unlodctr oipfctrs", NULL, NULL, TRUE, 0,
		  NULL, NULL, &siStInfo, &piProcInfo);
    WaitForSingleObject(piProcInfo.hProcess, INFINITE);
    GetExitCodeProcess(piProcInfo.hProcess, &status);

    CreateProcess(NULL, CmdLine.GetBuffer(0), NULL, NULL, TRUE, 0,
		  NULL, NULL, &siStInfo, &piProcInfo);
    WaitForSingleObject(piProcInfo.hProcess, INFINITE);
    GetExitCodeProcess(piProcInfo.hProcess, &status);

    Sleep(2000);
    while (!MainWnd);
    ::SendMessage(MainWnd, WM_CLOSE, 0, 0);
    return(status);
}

LRESULT
CPersonalHelp::OnWizardBack() 
{
    CEdit   *pEdit = (CEdit *)GetDlgItem(IDC_EDIT_HELP);
    CString Message, Message2;
    DWORD   status;

    if (pEdit->GetModify())
    {
	Message.LoadString(IDS_UPDATE_HELP);
	Message2.LoadString(IDS_TITLE);
	status = MessageBox(Message, Message2, MB_ICONQUESTION | MB_YESNO);
	if (status == IDYES)
	    OnButtonSaveHelp();
    }	

    return CPropertyPage::OnWizardBack();
}
